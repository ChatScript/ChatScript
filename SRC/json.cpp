#include "common.h"

#ifdef INFORMATION
Json units(JSON arraysand objects) consist of a dictionary entry labelled ja - ... or jo - ...
Facts are stored on these.When initially created as a permanent unit, there is a special
fact to represent the empty value so the dictionary entry will not disappear across volleys.
Otherwise it would be destroyedand a new unit arbitrarily created later that has no connection
to the one the user just created.

When you add an item to an array via JsonArrayInsert, this empty fact is destroyed.And when the
last array value is destroyed via JsonArrayDelete, the marker fact is recreated.
When you add an item to an object via JsonObjectInsert or by direct assignment, the marker fact
is destroyed.When you use Assign of null to a json object, if that was the last field, then the
marker fact is recreated.

The marker fact is normally bypassed by all accesses to the items of a json object(so it does not
	query or count or whatever) except when specially accessed by the above routines.
#endif

	// GENERAL JSON SUPPORT
char detectedlanguage[100];
unsigned int build0jid = 0;
unsigned int build1jid = 0;
unsigned int buildbootjid = 0;
unsigned int builduserjid = 0;
unsigned int buildtransientjid = 0;

int conceptSeen = 0;
MEANING jsonconceptName;
char* jsonconceptname;
static char jsonoldlang[MAX_WORD_SIZE];

#include "jsmn.h"
static bool jsonpurify = false;
static bool curl_done_init = false;
static int jsonCreateFlags = 0;
#define MAX_JSON_LABEL 50
static char jsonLabel[MAX_JSON_LABEL + 1];
bool safeJsonParse = false;
int jsonIdIncrement = 1;
int jsonDefaults = 0;
int json_open_counter = 0;
uint64 json_open_time = 0;
bool curlFail = false;
int jsonStore = 0; // where to put json fact refs
int jsonIndex;
int jsonOpenSize = 0;
static unsigned int jsonPermanent = FACTTRANSIENT;
bool jsonNoArrayduplicate = false;
bool jsonObjectDuplicate = false;
bool jsonSetDuplicate = false;
bool jsonDontKill = false;
bool directJsonText = false;
static char* curlBufferBase = NULL;
static int objectcnt = 0;
static FunctionResult JSONpath(char* buffer, char* path, char* jsonstructure, bool raw, bool nofail);
static MEANING jcopy(MEANING M);

typedef enum {
	URL_SCHEME = 0,
	URL_AUTHORITY,
	URL_PATH,
	URL_QUERY,
	URL_FRAGMENT
} urlSegment;

static bool IsTransientJson(char* name)
{
	return name[3] == 't';
}

static bool IsBootJson(char* name)
{
	return name[3] == 'b';
}

static bool IsDuplicate(char* name)
{
	if (IsTransientJson(name)) return (name[4] == '+');
	else return name[3] == '+';
}

static bool AllowDuplicates(char* name, bool checkedArgs = false)
{
    bool defaultDup = false;
    
    // duplicate/unique can be specified in JSON args to override the default or definition
    if (IsValidJSONName(name, 'o'))
    {
        if (checkedArgs && jsonSetDuplicate) return jsonObjectDuplicate;
        defaultDup = jsonDefaults & JSON_OBJECT_DUPLICATE;
    }
    else if (IsValidJSONName(name, 'a'))
    {
        if (checkedArgs && jsonSetDuplicate) return !jsonNoArrayduplicate;
        defaultDup = !(jsonDefaults & JSON_ARRAY_UNIQUE);
    }
    else return false;
        
    // fallback is via the actual name
    if (IsDuplicate(name)) return true;

    // transients will have been created with/without the +
    // but permanents may have been created before the + was used
    if (!IsTransientJson(name)) return defaultDup;
    
    return false;
}

void InitJson()
{
	json_open_counter = 0;
	json_open_time = 0;
	jsonDefaults = 0;
	curlFail = false;
	InitJSONNames();
}

static void JsonReuseKill(FACT* F);

static void jreusekillfact(WORDP D)
{
	if (!D) return;
	FACT* F = GetSubjectNondeadHead(D); // delete everything including marker
	while (F)
	{
		FACT* G = GetSubjectNondeadNext(F);
		if (F->flags & (JSON_ARRAY_FACT | JSON_OBJECT_FACT)) JsonReuseKill(F); // json object/array no longer has this fact
		if (F->flags & FACTAUTODELETE) { ; } // AutoKillFact(F->object); // kill the fact
		F = G;
	}
}

static void JsonReuseKill(FACT* F)
{
	if (!F || F->flags & FACTDEAD) return; // already dead

	if (F <= factsPreBuild[currentBeforeLayer])
		return;	 // may not kill off facts built into world

	F->flags |= FACTDEAD;

	if (trace & TRACE_FACT && CheckTopicTrace())
	{
		Log(USERLOG, "Kill: ");
		TraceFact(F);
	}
	if (F->flags & FACTAUTODELETE) // not doing facts as fields here
	{
		return;
		//	if (F->flags & FACTSUBJECT) AutoKillFact(F->subject);
		//	if (F->flags & FACTVERB) AutoKillFact(F->verb);
		//	if (F->flags & FACTOBJECT) AutoKillFact(F->object);
	}

	// recurse on JSON datastructures below if they are being deleted on right side
	if (F->flags & (JSON_ARRAY_VALUE | JSON_OBJECT_VALUE))
	{
		WORDP jsonarray = Meaning2Word(F->object);
		// should it recurse to kill guy refered to?
		// if no other living fact refers to it, you can also kill the referred object/array
		FACT* H = GetObjectNondeadHead(jsonarray);  // facts which link to this
		if (!H) jkillfact(jsonarray);
	}
	// we dont renumber arrays because we will be destroying the whole thing
	// if (F->flags & JSON_ARRAY_FACT) JsonRenumber(F);// have to renumber this array

	if (planning) SpecialFact(Fact2Index(F), 0, 0); // save to restore

	// if this fact has facts depending on it, they too must die
	FACT* G = GetSubjectNondeadHead(F);
	while (G)
	{
		JsonReuseKill(G);
		G = GetSubjectNondeadNext(G);
	}
	G = GetVerbNondeadHead(F);
	while (G)
	{
		JsonReuseKill(G);
		G = GetVerbNondeadNext(G);
	}
	G = GetObjectNondeadHead(F);
	while (G)
	{
		JsonReuseKill(G);
		G = GetObjectNondeadNext(G);
	}

	F->subject = Fact2Index(factFreeList);
	factFreeList = F;
}

FunctionResult JsonReuseKillCode(char* buffer)
{
	char* arg = ARGUMENT(1);
	if (!IsValidJSONName(arg)) return FAILRULE_BIT;
	WORDP D = FindWord(arg);
	FACT* F = GetSubjectNondeadHead(D);
	while (F)
	{
		JsonReuseKill(F);
		F = GetSubjectNondeadNext(F);
	}
	return NOPROBLEM_BIT;
}

static int JSONArgs()
{
	int index = 1;
	directJsonText = false;
	bool used = false;
	jsonCreateFlags = 0;
	jsonPermanent = FACTTRANSIENT; // default
	jsonNoArrayduplicate = false;
	jsonObjectDuplicate = false;
    jsonSetDuplicate = false;
	if (jsonDefaults & JSON_ARRAY_UNIQUE) jsonNoArrayduplicate = true;
	if (jsonDefaults & JSON_OBJECT_DUPLICATE) jsonObjectDuplicate = true;
	char* arg1 = ARGUMENT(1);
	if (*arg1 == '"') // remove quotes
	{
		++arg1;
		size_t len = strlen(arg1);
		if (arg1[len - 1] == '"') arg1[len - 1] = 0;
	}
	char word[MAX_WORD_SIZE];
	while (*arg1)
	{
		arg1 = ReadCompiledWord(arg1, word);
		if (!stricmp(word, (char*)"permanent"))
		{
			jsonPermanent = 0;
			used = true;
		}
		else if (!stricmp(word, (char*)"boot")) // build to migrate to system boot layer
		{
			jsonPermanent = FACTBOOT;
			used = true;
		}
		else if (!stricmp(word, (char*)"USER_FLAG3"))
		{
			jsonCreateFlags |= USER_FLAG3;
			used = true;
		}
		else if (!stricmp(word, (char*)"USER_FLAG2"))
		{
			jsonCreateFlags |= USER_FLAG2;
			used = true;
		}
		else if (!stricmp(word, (char*)"USER_FLAG1"))
		{
			jsonCreateFlags |= USER_FLAG1;
			used = true;
		}
		else if (!stricmp(word, (char*)"autodelete"))
		{
			jsonCreateFlags |= FACTAUTODELETE;
			used = true;
		}
		else if (!stricmp(word, (char*)"unique"))
		{
			jsonNoArrayduplicate = true;
			jsonObjectDuplicate = false;
            jsonSetDuplicate = true;
			used = true;
		}
		else if (!stricmp(word, (char*)"duplicate"))
		{
			jsonObjectDuplicate = true;
			jsonNoArrayduplicate = false;
            jsonSetDuplicate = true;
			used = true;
		}
		else if (!stricmp(word, (char*)"transient"))  used = true;
		else if (!stricmp(word, (char*)"direct"))  used = directJsonText = true; // used by jsonopen
		else if (!stricmp(word, (char*)"safe")) safeJsonParse = used = true;
		if (!used) break; // must find at start
	}
	if (used) ++index;
	return index;
}

void InitJSONNames()
{
	objectcnt = 0; // also for json arrays
	jsonIdIncrement = 1;
}

MEANING GetUniqueJsonComposite(char* prefix, unsigned int permanent)
{
	int index = 0;
	char* permanence = "";
	char newlabel[100];
	char* label = jsonLabel;
	if (permanent == FACTTRANSIENT)
	{
		permanence = "t";
		index = ++buildtransientjid;
	}
	else if (jsonPermanent == FACTTRANSIENT)
	{
		permanence = "t";
		index = ++buildtransientjid;
	}
	else if (jsonPermanent == FACTBOOT)
	{
		permanence = "b";
		index = ++buildbootjid;
	}
	else if (compiling && csapicall == NO_API_CALL && buildID == BUILD0)
	{
		index = ++build0jid;
		sprintf(newlabel, "qx%s", jsonLabel); // build0 not api
		label = newlabel;
	}
	else if (compiling && csapicall == NO_API_CALL && buildID == BUILD1)
	{
		index = ++build1jid;
		sprintf(newlabel, "qy%s", jsonLabel); // buil1 not api
		label = newlabel;
	}
	else
	{
		index = ++builduserjid;
		sprintf(newlabel, "qu%s", jsonLabel); // api composite
		label = newlabel;
	}
	const char* dup = (!jsonNoArrayduplicate || jsonObjectDuplicate) ? "+" : "";
	char namebuff[MAX_WORD_SIZE];
	sprintf(namebuff, "%s%s%s%s%d", prefix, permanence, dup, label, index);
	WORDP D = StoreWord(namebuff, AS_IS);
	return MakeMeaning(D);
}

bool IsValidJSONName(char* word, char type)
{
	size_t n = strlen(word);
	if (n < 4) return false;

	// must at least start with the right prefix
	if (word[0] != 'j' || word[2] != '-') return false;
	if (type)
	{
		if (word[1] != type) return false;
	}
	else if (word[1] != 'a' && word[1] != 'o') return false;

	// the label cannot contain certain characters
	if (strchr(word, ' ') ||
		strchr(word, '\\') || strchr(word, '"') || strchr(word, '\'') || strchr(word, 0x7f) || strchr(word, '\n') // refuse illegal content
		|| strchr(word, '\r') || strchr(word, '\t')) return false;    // must be legal unescaped json content and safe CS content

	// jo-abc@def.com is not a valid JSON name
	// last part must be numbers
	char* end = word + n - 1;
	char* at = end;
	while (IsDigit(*at)) --at;
	if (at == end) return false;

	long len = at - word - 3;
	if (IsTransientJson(word) || IsBootJson(word)) --len;
	if (IsDuplicate(word)) --len;
	if (len > MAX_JSON_LABEL) return false;

	return true;
}

static char* IsJsonNumber(char* str)
{
	if (IsDigit(*str) || (*str == '-' && IsDigit(str[1]))) // +number is illegal in json
	{
		// validate the number
		char* at = str;
		if (*at != '-') --at;
		bool periodseen = false;
		bool exponentseen = false;
		while (*++at)
		{
			if (*at == '.' && !periodseen && !exponentseen) periodseen = true;
			else if ((*at == 'e' || *at == 'E') && !exponentseen)
			{
				if (IsSign(at[1])) ++at;
				exponentseen = true;
			}
			else if (*at == ' ' || *at == ',' || *at == '}' || *at == ']') return at;
			else if (!IsDigit(*at)) return NULL; // cannot be number
		}
		return at;
	}
	return NULL;
}

static void StripEscape(char* from) // some escaped characters to normal, since cs doesnt use  them
{
	char* at = from;
	while ((at = strchr(at, '\\')))
	{
		char* base = at++; // base is start of escaped string
		if (*at != 'u' || !IsHexDigit(at[1]) || !IsHexDigit(at[2]) || !IsHexDigit(at[3]) || !IsHexDigit(at[4]))
		{
			if (*at == '"' || *at == '\\') // json escaped \" stuff is not escaped in CS
			{
				memmove(base, at, strlen(base)); // skip over the escaped char (in case its a \)
				at = base + 1;
			}
		}
	}
}

static int factsJsonHelper(int depth, char* jsontext, jsmntok_t* tokens, int currToken, MEANING* retMeaning, int* flags, bool key, bool nofail, int enableadjust) {
	// Always build with duplicate on. create a fresh copy of whatever
	jsmntok_t curr = tokens[currToken];
	*flags = 0;
	int retToken = currToken + 1;
	size_t size = curr.end - curr.start;
	char* text = jsontext + curr.start;
	char endchar = text[size];
	text[size] = 0; // terminating token allows us to keep mods to it from affecting other tokens

	switch (curr.type) {
	case JSMN_PRIMITIVE: { //  true  false, numbers, null
		if (size >= 100)
		{
			if (!nofail) ReportBug((char*)"Bad Json primitive size  %s", jsontext); // if we are not expecting it to fail
			return 0;
		}
		// see if its string literal using single quotes instead of double quotes. 
		if (*text == '\'' && text[size - 1] == '\'')
		{
			*flags = JSON_STRING_VALUE; // string null
			text[size - 1] = 0;
			++text;
			StripEscape(text);
			if (jsonpurify) 	PurifyInput(text, text, size, INPUT_PURIFY); // change out special or illegal characters
			// enableadjust bit 2 means do not return this field	
			if (!(enableadjust & 2)) *retMeaning = MakeMeaning(StoreWord(text, AS_IS));
			else *retMeaning = 1; // lie
			break;
		}

		if (!strncmp(text, "ja-", 3)) *flags = JSON_ARRAY_VALUE;
		else if (!strnicmp(text, "jo-", 3)) *flags = JSON_OBJECT_VALUE;
		else *flags = JSON_PRIMITIVE_VALUE; // json primitive type
		if (*text == USERVAR_PREFIX && text[1] == '^')
		{ // user function variable does not have a value we want
			*flags = JSON_STRING_VALUE;
		}
		else if (*text == USERVAR_PREFIX || *text == SYSVAR_PREFIX || *text == '_' || *text == '\'') // variable values from CS
		{
			// get path to safety if any
			char mainpath[MAX_WORD_SIZE];
			char* path = strchr(text, '.');
			char* pathbracket = strchr(text, '[');
			char* first = path;
			if (pathbracket && path && pathbracket < path) first = pathbracket;
			if (first) strcpy(mainpath, first);
			else *mainpath = 0;
			if (path) *path = 0;
			if (pathbracket) *pathbracket = 0;

			char word1[MAX_WORD_SIZE];
			FunctionResult result;
			ReadShortCommandArg(text, word1, result); // get the basic item
			strcpy(text, word1);
			char* numberEnd = NULL;

			// now see if we must process a path
			if (*mainpath) // access field given
			{
				char word[MAX_WORD_SIZE];
				result = JSONpath(word, mainpath, word, true, nofail); // raw mode
				if (result != NOPROBLEM_BIT)
				{
					if (!nofail) ReportBug((char*)"INFO: Bad Json path building facts from template %s%s data", text, mainpath, jsontext); // if we are not expecting it to fail
					return 0;
				}
				else strcpy(word, word);
			}

			if (!*text) // empty string treated as null
			{
				strcpy(text, (char*)"null");
				*flags = JSON_STRING_VALUE; // string null
			}
			else if (!strcmp(text, (char*)"true") || !strcmp(text, (char*)"false"))
			{
			}
			else if (!strncmp(text, (char*)"ja-", 3) || !strncmp(text, (char*)"jo-", 3))
			{
				*flags = (text[1] == 'a') ? JSON_ARRAY_VALUE :  JSON_OBJECT_VALUE;
				if (!(enableadjust &  2)) 
				{
					MEANING M = jcopy(MakeMeaning(StoreWord(text, AS_IS)));
					WORDP D = Meaning2Word(M);
					strcpy(text, D->word);
				}
				else *text = 0;
			}
			else if ((numberEnd = IsJsonNumber(text)) && numberEnd == (text + strlen(text))) { ; }
			else *flags = JSON_STRING_VALUE; // cannot be number
		}
		if (!(enableadjust & 2)) *retMeaning = MakeMeaning(StoreWord(text, AS_IS));
		else *retMeaning = 1; // lie
		break;
	}
	case JSMN_STRING: 
	{
		StripEscape(text);
		if (jsonpurify) 	PurifyInput(text, text, size, INPUT_PURIFY); // change out special or illegal characters
		*flags = JSON_STRING_VALUE; // string null
		if (enableadjust & 8) // convert to underscore all spaces in string
		{
			char* under = text - 1;
			while ((under = strchr(under, ' '))) *under = '_';
		}

		if (!(enableadjust & 2))
		{
		
			*retMeaning = MakeMeaning(StoreWord(text, AS_IS));
		}
		else *retMeaning = 1;
		break;
	}
	case JSMN_OBJECT: 
	{
		// Build the object name
		MEANING objectName = 0;
		if (!(enableadjust & 2))
		{
			objectName = GetUniqueJsonComposite((char*)"jo-");
			*retMeaning = objectName;
		}
		else *retMeaning = 1;

	
		for (int i = 0; i < curr.size / 2; i++)  // each entry takes an id and a value
		{
				MEANING keyMeaning = 0;
				int jflags = 0;
				retToken = factsJsonHelper(depth+1,jsontext, tokens, retToken, &keyMeaning, &jflags, true, nofail, enableadjust);
				if (retToken == 0) return 0;

				// dont expand into facts?
				int oldenable = enableadjust;
				WORDP field = Meaning2Word(keyMeaning);
				char* fieldname =  field->word;

				// high speed concepts bypass enable
				// concepts": [{"name": "~badanswerwords", "values": ["confuse
				if (conceptSeen && depth <= conceptSeen) // reset after completion of concept
				{
					conceptSeen = 0;
					if (*detectedlanguage) SetLanguage(jsonoldlang); // restore prior language
				}
				 if (!stricmp(fieldname, "concepts"))
				{ // while whole of json is in default language, the concepts are in given language
					 // which may not have been parsed yet. so we change language since we looked ahead
					conceptSeen = depth;
					if (*detectedlanguage)
					{
						SetLanguage(detectedlanguage); // restore prior language
					}
				}
				if ((enableadjust & 1) && field->internalBits & SETIGNORE)  enableadjust |= 2;
				if ((enableadjust & 4) && field->internalBits & SETUNDERSCORE)  enableadjust |= 8;
				MEANING valueMeaning = 0;
				retToken = factsJsonHelper(depth+1,jsontext, tokens, retToken, &valueMeaning, &jflags, false, nofail, enableadjust);
				if (retToken == 0) return 0;
				// dont swallow null field value, discard field - but accept null input field
				char* key = Meaning2Word(keyMeaning)->word;
				char* value = Meaning2Word(valueMeaning)->word;
				if (conceptSeen && !strcmp(fieldname, "name"))
				{
					if (!stricmp(value, "~noun") || !stricmp(value, "~verb") ||
						!stricmp(value, "~adjective") || !stricmp(value, "~adverb") ||
						!stricmp(value, "~replace_spelling"))
					{
						jsonconceptName = 0;
					}
					else
					{
						jsonconceptName = valueMeaning;
						jsonconceptname = value;
						Meaning2Word(jsonconceptName)->internalBits |= OVERRIDE_CONCEPT;
					}
				}
				if (!(enableadjust & 2) ) 
					CreateFact(objectName, keyMeaning, valueMeaning, jsonCreateFlags | jsonPermanent | jflags | JSON_OBJECT_FACT); // only the last value of flags matters. 5 means object fact in subject
				enableadjust = oldenable; // restore
		}
		*flags = JSON_OBJECT_VALUE;
		break;
	}
	case JSMN_ARRAY: 
	{
		MEANING arrayName = 0;
		if (!(enableadjust &2))
		{
			arrayName = GetUniqueJsonComposite((char*)"ja-");
			*retMeaning = arrayName;
		}
		else *retMeaning = 1;
		int arrayindex = 0;
		for (int i = 0; i < curr.size; i++)
		{
			MEANING arrayMeaning = 0;
			int jflags = 0;
			retToken = factsJsonHelper(depth + 1, jsontext, tokens, retToken, &arrayMeaning, &jflags, false, nofail, enableadjust);
			if (retToken == 0) return 0;
			char* word = Meaning2Word(arrayMeaning)->word;
			
			// concept value formats include:
			bool conceptvalid = false;
			if (jsonconceptName) conceptvalid = true;
			char* value = Meaning2Word(arrayMeaning)->word;
			if (conceptvalid)
			{
				char* safestart = SkipWhitespace(value);
				if (strchr(value, '|')) conceptvalid = false; // remap concept field
				else if (*value == '"' && (*safestart == '(' || safestart[1] == '(')) conceptvalid = false; // pattern in "
				else if (*safestart == '(' && strchr(value, ')')) conceptvalid = false; // pattern in  ()  -- skip over leading whitespace
				else if (*value == '^' && value[1] == '(') conceptvalid = false;
			}
			if (conceptvalid) // express concept create
			{
				int flags = FACTTRANSIENT | OVERRIDE_MEMBER_FACT | FACTDUPLICATE;
				char hold[MAX_WORD_SIZE];
				char* at = hold;
				if (*value == '~') strcpy(hold, value);
				else strcpy(hold, JoinWords(BurstWord(value, CONTRACTIONS))); // unneeded because compile should have handled it BUG 
				at = hold;
				if (*at == '\'')
				{
					flags |= ORIGINAL_ONLY;
					at++;
				}
				arrayMeaning = MakeMeaning(StoreWord(at, AS_IS));
				CreateFastFact(arrayMeaning, Mmember, jsonconceptName, flags);
			}
			else if (!(enableadjust & 2))
			{
				MEANING index = 0;
				char namebuff[256];
				sprintf(namebuff, "%d", arrayindex++); // Build the array index
				index = MakeMeaning(StoreWord(namebuff, AS_IS));
				CreateFact(arrayName, index, arrayMeaning, jsonCreateFlags | jsonPermanent | jflags | JSON_ARRAY_FACT); // flag6 means subject is arrayfact
			}
		}
		jsonconceptName = 0;
		*flags = JSON_ARRAY_VALUE;
		break;
	}
	default:
		if (!(enableadjust & 2))
		{
			char* str = AllocateBuffer("jsmn default"); // cant use InfiniteStack because ReportBug will;.
			strncpy(str, jsontext + curr.start, size);
			str[size] = 0;
			FreeBuffer("jsmn default");
			ReportBug((char*)"FATAL: (factsJsonHelper) Unknown JSON type encountered: %s", str);
		}
	}
	text[size] = endchar;

	currentFact = NULL;
	return retToken;
}

// Define our struct for accepting LCs output
struct CurlBufferStruct {
	char* buffer;
	size_t size;
};

static void dump(const char* text, FILE * stream, unsigned char* ptr, size_t size) // libcurl  callback when verbose is on
{
	size_t i;
	size_t c;
	unsigned int width = 0x10;
	(*printer)((char*)"%s, %10.10ld bytes (0x%8.8lx)\n", text, (long)size, (long)size);
	for (i = 0; i < size; i += width)
	{
		(*printer)((char*)"%4.4lx: ", (long)i);

		/* show hex to the left */
		for (c = 0; c < width; c++)
		{
			if (i + c < size)  (*printer)((char*)"%02x ", ptr[i + c]);
			else (*printer)((char*)"%s", (char*)"   ");
		}

		/* show data on the right */
		for (c = 0; (c < width) && (i + c < size); c++) (*printer)("%c", (ptr[i + c] >= 0x20) && (ptr[i + c] < 0x80) ? ptr[i + c] : '.');
		(*printer)((char*)"%s", (char*)"\n");
	}
}

/*
----------------------
FUNCTION: JSONOpenCode

		  Function arguments :
optional argument(1) - permanent, transient, direct
ARGUMENT(1) - request method : POST, GET, POSTU, GETU
ARGUMENT(2) - URL. The URL to use in the request
ARGUMENT(3) - If a POST request, this argument contains the post data
ARGUMENT(4) - This argument contains any needed extra REQUEST headers for the request(see note above).
ARGUMENT(5) - concept of fields to ignore returning.

	e.g.
	$$url = "https://api.github.com/users/test/repos"
	$$user_agent = ^"User-Agent: Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.2; WOW64; Trident/6.0)"
	^jsonopen(GET $$url "" $$user_agent)

	# GitHub requires a valid user agent header or it will reject the request.  Note, although
	#  not shown, if there are multiple extra headers they should be separated by the
	#  tilde character ("~").

	E.g.
	$$url = "https://en.wikipedia.org/w/api.php?action=query&titles=Main%20Page&rvprop=content&format=json"
	$$user_agent = ^"myemail@hotmail.com User-Agent: Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.2; WOW64; Trident/6.0)"

*/
#define REQUEST_HEADER_NVP_SEPARATOR "~"
#define REQUEST_NVP_SEPARATOR ':'

// This function reimplements the semi-standard function strlcpy so we can use it on both Windows, Linux and Mac
size_t our_strlcpy(char* dst, const char* src, size_t siz) {
	char* d = dst;
	const char* s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0) break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0) *d = '\0';                /* NUL-terminate dst */
		while (*s++) { ; }
	}

	return(s - src - 1);    /* count does not include NUL */
}

#ifdef WIN32
// If we're on Windows, just use the safe strncpy version, strncpy_s.
# define SAFE_SPRINTF sprintf_s
#else
// Use snprintf for Linux.
# define SAFE_SPRINTF snprintf
#endif 

static int EncodingValue(char* name, char* field, int value)
{
	size_t len = strlen(name);
	char* at = strstr(field, name);
	if (!at) return value; // not found
	at += len;
	if (at[0] != ';') return 2; // autho
	if (at[1] != 'q' && at[1] != 'Q') return 2;
	if (at[2] != '=' || at[3] != '0') return 2;  // gzip;q=1
	if (at[4] == '.') return 2; // gzip;q=0.5
	return 1;
}


CURL* curl;

static int my_trace(CURL * handle, curl_infotype type, char* data, size_t size, void* userp)
{
	const char* text;
	(void)handle; /* prevent compiler warning */

	switch (type) {
	case CURLINFO_TEXT:
		(*printer)("== Info: %s", data);
	default: /* in case a new one is introduced to shock us */
		return 0;
	case CURLINFO_HEADER_OUT:
		text = "=> Send header";
		break;
	case CURLINFO_DATA_OUT:
		text = "=> Send data";
		break;
	case CURLINFO_SSL_DATA_OUT:
		text = "=> Send SSL data";
		break;
	case CURLINFO_HEADER_IN:
		text = "<= Recv header";
		break;
	case CURLINFO_DATA_IN:
		text = "<= Recv data";
		break;
	case CURLINFO_SSL_DATA_IN:
		text = "<= Recv SSL data";
		break;
	}
	dump(text, stderr, (unsigned char*)data, size);
	return 0;
}

// This is the function we pass to LC, which writes the output to a BufferStruct
static size_t CurlWriteMemoryCallback(void* ptr, size_t size, size_t nmemb, void* data) {
	size_t realsize = size * nmemb;
	static char* currentLoc;
	static char* limit; // where we can allocate to
	struct CurlBufferStruct* mem = (struct CurlBufferStruct*)data;
	if (!curlBufferBase)
	{
		curlBufferBase = InfiniteStack(limit, "CurlWriteMemoryCallback"); // can only be released from JSONOpenCode
		currentLoc = curlBufferBase;
		mem->buffer = curlBufferBase;
	}
	mem->size += realsize;
	if ((int)mem->size > (int)(limit - curlBufferBase)) ReportBug("FATAL: out of curlmemory"); // out of memory
	memcpy(currentLoc, ptr, realsize); // add to buffer
	currentLoc[realsize] = 0;
	currentLoc += realsize;
	return realsize;
}

void CurlShutdown()
{
	if (curl) {
		curl_easy_cleanup(curl);
		curl = NULL;
	}
	if (curl_done_init) curl_global_cleanup();
	curl_done_init = false;
}

const char* CurlVersion()
{
    static char curlversion[MAX_WORD_SIZE] = "";
    if (*curlversion) return(curlversion);
    
    curl_version_info_data data = *curl_version_info(CURLVERSION_NOW);
    
    sprintf(curlversion,"%s, %s, libz/%s", data.version, data.ssl_version, data.libz_version);
    return curlversion;
}

FunctionResult InitCurl()
{
	// Get curl ready -- do this ONCE only during run of CS
	if (!curl_done_init) {
#ifdef WIN32
		if (InitWinsock() == FAILRULE_BIT) // only init winsock one per any use- we might have done this from TCPOPEN or PGCode
		{
			ReportBug((char*)"INFO: Winsock init failed");
			return FAILRULE_BIT;
		}
#endif
		curl_global_init(CURL_GLOBAL_SSL);
		curl_done_init = true;
	}
	return NOPROBLEM_BIT;
}

char* UrlEncodePiece(char* input)
{
	InitCurl();
	CURL* curlEscape = curl_easy_init();
	if (!curlEscape)
	{
		if (trace & TRACE_JSON) Log(USERLOG, "Curl easy init failed");
		return NULL;
	}
	char* fixed = curl_easy_escape(curlEscape, input, 0);
	char* limit;
	char* buffer = InfiniteStack(limit, "UrlEncode");
	strcpy(buffer, fixed);
	curl_free(fixed);
	curl_easy_cleanup(curlEscape);
	ReleaseInfiniteStack();
	return buffer;
}

char* encodeSegment(char** fixed, char* at, char* start, CURL * curlptr)
{
	char* segment = *fixed;
	if (at[-1] == '\\') return start; // escaped, allow as is
	if (at != start) // url encode segment
	{
		strncpy(segment, start, at - start);
		segment[at - start] = 0;
		char* coded = curl_easy_escape(curlptr, segment, 0);
		strcpy(segment, coded);
		curl_free(coded);
		segment += strlen(segment);
	}
	*segment++ = *at;
	*segment = 0;
	*fixed = segment;
	return(at + 1);
}

// RFC 3986
static char* JSONUrlEncode(char* urlx, char* fixedUrl, CURL * curlptr)
{
	char* fixed = fixedUrl;
	*fixed = 0;
	char* at = urlx - 1;
	char* start = urlx;
	char c;
	urlSegment currentSegment = URL_SCHEME;
	while ((c = *++at)) // url encode segments
	{
		if (currentSegment == URL_SCHEME) {
			if (c == ':' || c == '+' || c == '-' || c == '.') {
				start = encodeSegment(&fixed, at, start, curlptr);
				if (start == at) continue;
				if (c == ':') {
					*fixed++ = *++at; // double /
					*fixed++ = *++at;
					start += 2;
					currentSegment = URL_AUTHORITY;
				}
			}
		}
		else if (currentSegment == URL_AUTHORITY) {
			if (c == '/' || c == '?' || c == '#' || c == '@' || c == ':' || c == '[' || c == ']' ||
				c == '!' || c == '$' || c == '&' || c == '(' || c == ')' ||
				c == '*' || c == '+' || c == ',' || c == ';' || c == '=' || c == '\'') {
				start = encodeSegment(&fixed, at, start, curlptr);
				if (start == at) continue;
				if (c == '/') {
					currentSegment = URL_PATH;
				}
				else if (c == '?') {
					currentSegment = URL_QUERY;
				}
				else if (c == '#') {
					currentSegment = URL_FRAGMENT;
				}
			}
		}
		else if (currentSegment == URL_PATH) {
			if (c == '/' || c == '?' || c == '#' || c == ':' || c == '@' ||
				c == '!' || c == '$' || c == '&' || c == '(' || c == ')' ||
				c == '*' || c == '+' || c == ',' || c == ';' || c == '=' || c == '\'') {
				start = encodeSegment(&fixed, at, start, curlptr);
				if (start == at) continue;
				if (c == '?') {
					currentSegment = URL_QUERY;
				}
				else if (c == '#') {
					currentSegment = URL_FRAGMENT;
				}
			}
		}
		else if (currentSegment == URL_QUERY) {
			if (c == '#' || c == '&' || c == '=' || c == '/' || c == '?') {
				start = encodeSegment(&fixed, at, start, curlptr);
				if (start == at) continue;
				if (c == '#') {
					currentSegment = URL_FRAGMENT;
				}
			}
		}
		else if (currentSegment == URL_FRAGMENT) {
			if (c == '/' || c == '?') {
				start = encodeSegment(&fixed, at, start, curlptr);
			}
		}
	}
	// remaining piece
	start = encodeSegment(&fixed, at, start, curlptr);
	return fixedUrl;
}

// Open a URL using the given arguments and return the JSON object's returned by querying the given URL as a set of ChatScript facts.
char fieldName[1000];
char fieldValue[1000];
char headerLine[1000];

FunctionResult JSONOpenCode(char* buffer)
{
	int index = JSONArgs();
	size_t len;
	curlBufferBase = NULL;
	char* arg = NULL;
	char* extraRequestHeadersRaw = NULL;
	char kind = 0;

	char* raw_kind = ARGUMENT(index++);
	if (!stricmp(raw_kind, "POST"))  kind = 'P';
	else if (!stricmp(raw_kind, "GET")) kind = 'G';
	else if (!stricmp(raw_kind, "POSTU")) kind = 'P';
	else if (!stricmp(raw_kind, "GETU")) kind = 'G';
	else if (!stricmp(raw_kind, "PUT"))  kind = 'U';
	else if (!stricmp(raw_kind, "DELETE"))  kind = 'D';
	else {
		char* msg = "jsonopen- only POST, GET, PUT AND DELETE allowed\r\n";
		SetUserVariable((char*)"$$tcpopen_error", msg);	// pass along the error
		ReportBug(msg);
		return FAILRULE_BIT;
	}

	char* urlx = ARGUMENT(index++);

	// Now fix starting and ending quotes around url if there are any

	if (*urlx == '"') ++urlx;
	len = strlen(urlx);
	if (urlx[len - 1] == '"') urlx[len - 1] = 0;

	// convert \" to " within params and remove any wrapper
	arg = ARGUMENT(index++);
	if (*arg == '"') ++arg;
	len = strlen(arg);
	if (arg[len - 1] == '"') arg[len - 1] = 0;
	if (!stricmp(arg, (char*)"null")) *arg = 0; // empty string replaces null

	// convert json ref to text
	if (IsValidJSONName(arg))
	{
		WORDP D = FindWord(arg);
		if (!D) return FAILRULE_BIT;
		NextInferMark();
		unsigned int limit  = 1000000;
		arg = AllocateStack(NULL, limit);
		jwrite(arg, arg, D, 1, false, limit); // it is subject field
	}

	bool bIsExtraHeaders = false;

	extraRequestHeadersRaw = ARGUMENT(index++);

	char* ignore = NULL;
	if (*ARGUMENT(index) == '~') ignore = ARGUMENT(index++); // fields to ignore
	char* underscore = NULL;
	if (*ARGUMENT(index) == '~') underscore = ARGUMENT(index++); // fields to change underscore to spaces

	char* timeoutstr = GetUserVariable("$cs_jsontimeout", false); // bot level
	if ( IsDigit(*ARGUMENT(index))) timeoutstr = ARGUMENT(index); // local call override
	if (!*timeoutstr)  timeoutstr  = GetUserVariable("$cs_overrideJsontimelimit", false); // override user from cs_init world
	long timelimit = (*timeoutstr) ? atoi(timeoutstr) : 300L;
	if (timelimit <= 0)
	{
		http_response = -1;
		return FAILRULE_BIT; // instant fail
	}

	// Make sure the raw extra REQUEST headers parameter value is not empty and
	//  not the ChatScript empty argument character.
	if (*extraRequestHeadersRaw)
	{
		// If the parameter value is only 1 characters long and it is a question mark,
		//  then ignore it since it's the "placeholder" (i.e. - "empty") parameter value
		//  indicating the parameter should be ignored.
		if (!((strlen(extraRequestHeadersRaw) == 1) && (*extraRequestHeadersRaw == '?')))
		{
			// Remove surrounding double-quotes if found.
			if (*extraRequestHeadersRaw == '"') ++extraRequestHeadersRaw;
			len = strlen(extraRequestHeadersRaw);
			if (extraRequestHeadersRaw[len - 1] == '"') extraRequestHeadersRaw[len - 1] = 0;
			bIsExtraHeaders = true;
		}

	} // if (strlen(extraRequestHeadersRaw) > 0)

	uint64 start_time = ElapsedMilliseconds();

	CURLcode res;
	struct CurlBufferStruct output;
	output.buffer = NULL;
	output.size = 0;
	// Get curl ready -- do this ONCE only during run of CS
	if (InitCurl() != NOPROBLEM_BIT) return FAILRULE_BIT;
	if (trace & TRACE_JSON)
	{
		Log(USERLOG, "\r\nCurl version: %s \r\n", curl_version());
	}
	// Only need to get one curl easy handle so that can cache connections
	if (curl)
	{
		// reinitialize all the options
		// but leaves the connections and DNS cache
		curl_easy_reset(curl);
	}
	else
	{
		curl = curl_easy_init();
		if (!curl)
		{
			if (trace & TRACE_JSON) Log(USERLOG, (char*)"Curl easy init failed");
			return FAILRULE_BIT;
		}
	}

	// url encode as necessary
	char* fixedUrl = AllocateBuffer();
	JSONUrlEncode(urlx, fixedUrl, curl);

	if (trace & TRACE_JSON)
	{
		Log(USERLOG, "\r\n");
		Log(USERLOG, "Json method/url: %s %s\r\n", raw_kind, fixedUrl);
		if (kind == 'P' || kind == 'U')
		{
			Log(USERLOG, "\r\n");
			len = strlen(arg);
			if (len < (size_t)(logsize - SAFE_BUFFER_MARGIN)) Log(USERLOG, "Json  data %d bytes: %s\r\n ", len, arg);
			else Log(USERLOG, "Json  data %d bytes\r\n ", len);
			Log(USERLOG, "");
		}
	}
	// Add the necessary headers for the request.
	struct curl_slist* header = NULL;

	if (kind == 'P')
	{
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, arg);
	}
	if (kind == 'U')
	{
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, arg);
	}
	if (kind == 'D')
	{
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, arg);
	}

	// Assuming a content return type of JSON.
	int gzipstatus = 0;
	int deflate = 0;
	int compress = 0;
	int identity = 0;
	int wild = 0;
	bool contentSeen = false;

	// If any extra REQUEST headers were specified, add them now.
	if (bIsExtraHeaders)
	{
		// REQUEST header name/value pairs are separated by tildes ("~").
		char* p = strtok(extraRequestHeadersRaw, REQUEST_HEADER_NVP_SEPARATOR);

		// Process each REQUEST header.
		while (p)
		{
			// Split the REQUEST header label and field value.
			char* p2 = strchr(p, REQUEST_NVP_SEPARATOR);

			if (p2)
			{
				// Delimiter found.  Split out the field name and it's value.
				*p2 = 0;
				our_strlcpy(fieldName, p, sizeof(fieldName));
				char name[MAX_WORD_SIZE];
				MakeLowerCopy(name, fieldName);

				p2++;
				our_strlcpy(fieldValue, p2, sizeof(fieldValue));
				char value[MAX_WORD_SIZE];
				MakeLowerCopy(value, fieldValue);
				len = strlen(value);
				while (value[len - 1] == ' ') value[--len] = 0;	// remove trailing blanks, forcing the field to abut the ~
				if (!strnicmp(name, "Content-type", 12)) contentSeen = true;
				if (strstr(name, (char*)"accept-encoding"))
				{
					gzipstatus = EncodingValue((char*)"gzip", value, gzipstatus);
					deflate = EncodingValue((char*)"deflate", value, deflate);
					compress = EncodingValue((char*)"compress", value, compress);
					identity = EncodingValue((char*)"identity", value, identity);
					wild = EncodingValue((char*)"*", value, wild);
				}
			}
			else
			{
				// No delimiter found.  Use the entire string as the field name and wipe the field value.
				our_strlcpy(fieldName, p, sizeof(fieldValue));
				strcpy(fieldValue, "");
				char value[MAX_WORD_SIZE];
				MakeLowerCopy(value, fieldName);
				if (strstr(value, (char*)"accept-encoding"))
				{
					gzipstatus = EncodingValue((char*)"gzip", value, gzipstatus);
					deflate = EncodingValue((char*)"deflate", value, deflate);
					compress = EncodingValue((char*)"compress", value, compress);
					identity = EncodingValue((char*)"identity", value, identity);
					wild = EncodingValue((char*)"*", value, wild);
				}
			}
            
			// Trim trailing spaces from header key and value, https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html
            char* name = TrimSpaces(fieldName, true);
            char* value = TrimSpaces(fieldValue, true);
            
			// Build the REQUEST header line for CURL.
			SAFE_SPRINTF(headerLine, sizeof(headerLine), "%s: %s", name, value);

			// Add the new REQUEST header to the headers list for this request.
			header = curl_slist_append(header, headerLine);

			// Next REQUEST header.
			p = strtok(NULL, REQUEST_HEADER_NVP_SEPARATOR);
		} // while (p)

	} // if (extraRequestHeadersRaw)
	if (!contentSeen) header = curl_slist_append(header, "Content-Type: application/json");
	header = curl_slist_append(header, "User-Agent: ChatScript");
	char* correlationId = GetUserVariable("$correlation_id", false);	
	if (*correlationId)
	{
		SAFE_SPRINTF(headerLine, sizeof(headerLine), "correlation-id: %s", correlationId);
		header = curl_slist_append(header, headerLine);
	}
    
	char coding[MAX_WORD_SIZE];
	*coding = 0;
	if (wild == 2) // authorizes anything not mentioned
	{
		if (gzipstatus == 0) gzipstatus = 2;
		if (compress == 0) compress = 2;
		if (identity == 0) identity = 2;
		if (deflate == 0) deflate = 2;
	}
	if (compress == 2)
	{
		if (gzipstatus == 0) gzipstatus = 2;
		if (deflate == 0) deflate = 2;
	}
	if (gzipstatus == 2) strcat(coding, (char*)"gzip,");
	if (deflate == 2) strcat(coding, (char*)"deflate,");
	if (identity == 2) strcat(coding, (char*)"identity,");
	if (!*coding) strcpy(coding, (char*)"identity,");
	size_t len1 = strlen(coding);
	coding[len1 - 1] = 0; // remove terminal comma
#if LIBCURL_VERSION_NUM >= 0x071506
	// CURLOPT_ACCEPT_ENCODING renamed in curl 7.21.6
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, coding);
#else
	curl_easy_setopt(curl, CURLOPT_ENCODING, coding);
#endif

	// Set up the CURL request.
	res = CURLE_OK;

	// proxy ability
	char* proxyuser = GetUserVariable("$cs_proxycredentials", false); // "myname:thesecret"
	if (res == CURLE_OK && proxyuser && *proxyuser) res = curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxyuser);
	char* proxyserver = GetUserVariable("$cs_proxyserver", false); // "http://local.example.com:1080"
	if (res == CURLE_OK && proxyserver && *proxyserver) res = curl_easy_setopt(curl, CURLOPT_PROXY, proxyserver);
	char* proxymethod = GetUserVariable("$cs_proxymethod", false); // "CURLAUTH_ANY"
	if (res == CURLE_OK && proxymethod && *proxymethod) res = curl_easy_setopt(curl, CURLOPT_PROXYAUTH, atol(proxymethod));

	if (res == CURLE_OK) res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteMemoryCallback); // callback for memory
	if (res == CURLE_OK) res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&output); // store output here
	if (res == CURLE_OK) res = curl_easy_setopt(curl, CURLOPT_URL, fixedUrl);
	if (res == CURLE_OK) curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
	if (res == CURLE_OK) curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timelimit); // 300 second timeout to connect (once connected no effect)
	if (res == CURLE_OK) curl_easy_setopt(curl, CURLOPT_TIMEOUT, timelimit * 2);
	if (res == CURLE_OK) curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // dont generate signals in unix

#ifdef PRIVATE_CODE
// Check for private hook function to record start of the request
// not static because if ^csboot() makes ^jsonOpen() calls then PrivateInit has not been called and hence hooks are not registered
	HOOKPTR fns = FindHookFunction((char*)"JsonOpenStart");
	if (fns) ((JsonOpenStartHOOKFN)fns)(curl, header);
#endif

	if (res == CURLE_OK) res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);

	if (trace & TRACE_JSON)
	{
		Log(USERLOG, "\r\n");
		curl_slist* list = header;
		while (list)
		{
			Log(USERLOG, "JSON header: %s\r\n", list->data);
			list = list->next;
		}
		Log(USERLOG, "\r\n");
	}

	/* the DEBUGFUNCTION has no effect until we enable VERBOSE */
	if (trace & TRACE_JSON && deeptrace) curl_easy_setopt(curl, CURLOPT_VERBOSE, (long)1);
	if (res == CURLE_OK) res = curl_easy_perform(curl);

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response);
	if (curlBufferBase) CompleteBindStack();
	if (trace & TRACE_JSON && res != CURLE_OK)
	{
		char word[MAX_WORD_SIZE * 10];
		char* at = word;
		sprintf(at, "Json method/url: %s %s -- ", raw_kind, fixedUrl);
		at += strlen(at);
		if (bIsExtraHeaders)
		{
			sprintf(at, "Json header: %s -- ", extraRequestHeadersRaw);
			at += strlen(at);
			if (kind == 'P' || kind == 'U')  sprintf(at, "Json  data: %s\r\n ", arg);
		}

		if (res == CURLE_URL_MALFORMAT) 
		{
			http_response = -5;
			ReportBug((char*)"\r\nINFO: Json url malformed %s", word);
		}
		else if (res == CURLE_GOT_NOTHING) 
		{
			http_response = -4;
			ReportBug((char*)"\r\nINFO: Curl got nothing %s", word);
		}
		else if (res == CURLE_UNSUPPORTED_PROTOCOL) 
		{
			http_response = -3;
			ReportBug((char*)"\r\nINFO: Curl unsupported protocol %s", word);
		}
		else if (res == CURLE_COULDNT_CONNECT || res == CURLE_COULDNT_RESOLVE_HOST || res == CURLE_COULDNT_RESOLVE_PROXY)
		{
			http_response = -2;
			Log(USERLOG, "\r\nJson connect failed ");
		}
		else if (res == CURLE_OPERATION_TIMEDOUT) 
		{ 
			http_response = -7;
			ReportBug((char*)"\r\nINFO: Curl timeout "); 
		}
		else
		{
			http_response = -6;
			if (output.buffer && output.size) { ReportBug((char*)"\r\nINFO: Other curl return code %d %s  - %s ", (int)res, word, output.buffer); }
			else { ReportBug((char*)"\r\nINFO: Other curl return code %d %s", (int)res, word); }
		}
	}
	if (CURLE_OK == res && (http_response == 200 || http_response == 204)) 
	{
		double namelookuptime;
		double proxyconnecttime;
		double appconnecttime;
		double pretransfertime;
		double totaltransfertime;
		curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME, &namelookuptime);
		curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &proxyconnecttime);
		curl_easy_getinfo(curl, CURLINFO_APPCONNECT_TIME, &appconnecttime);
		curl_easy_getinfo(curl, CURLINFO_PRETRANSFER_TIME, &pretransfertime);
		curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &totaltransfertime);
		//sprintf(lastcurltime,"Time Analysis:\nName Look up:%.6f\nHost/proxy connect:%.6f\nApp(SSL) connect:%.6f\nPretransfer:%.6f\nTotal Transfer:%.6f\nEND\n", namelookuptime, proxyconnecttime, appconnecttime, pretransfertime, totaltransfertime);
		sprintf(lastcurltime, "%.6fs %.6fs %.6fs %.6fs %.6fs", namelookuptime, proxyconnecttime, appconnecttime, pretransfertime, totaltransfertime);
	}
	else if (timelimit)
	{
		if (!http_response) http_response = -7;
		curlFail = true;
	}

	int timediff = (int)(ElapsedMilliseconds() - start_time);
	++json_open_counter;
	json_open_time += timediff;

#ifdef PRIVATE_CODE
	// Check for private hook function to record end of request
	HOOKPTR fne = FindHookFunction((char*)"JsonOpenEnd");
	if (fne) ((JsonOpenEndHOOKFN)fne)();
#endif

	if (timing & TIME_JSON)
	{
		if (timing & TIME_ALWAYS || timediff > 0) Log(STDTIMELOG, (char*)"Json open time: %d ms for %s %s\r\n", timediff, raw_kind, fixedUrl);
	}

	// cleanup
	FreeBuffer();
	if (header)  curl_slist_free_all(header);

	if (res != CURLE_OK)
	{
		if (curlBufferBase) ReleaseStack(curlBufferBase);
		return FAILRULE_BIT;
	}
	// 300 seconds is the default timeout
	// CUrl is gone, we have the json data now to convert
	FunctionResult result = NOPROBLEM_BIT;
	size_t size;
	bool oldapicall = outputapicall;
	CheckForOutputAPI(output.buffer);
	if (directJsonText)
	{
		PurifyInput(output.buffer, output.buffer, size, INPUT_PURIFY); // change out special or illegal characters
		strcpy(buffer, output.buffer);
		jsonOpenSize = output.size;
	}
	else
	{
		result = ParseJson(buffer, output.buffer, output.size, false,ignore,true,underscore); // purifies locally for strings
		char x[MAX_WORD_SIZE];
		if (result == NOPROBLEM_BIT)
		{
			ReadCompiledWord(buffer, x);
			if (*x) // empty json object should not be returned, will not survive on its own
			{
				WORDP X = FindWord(x);
				if (X && X->subjectHead == 0)
					*buffer = 0;
			}
		}
	}
	outputapicall = oldapicall;

	if (trace & TRACE_JSON)
	{
		Log(USERLOG, "\r\n");
		Log(USERLOG, "\r\nJSON response: %d size: %d - ", http_response, output.size);
		if (output.size < (size_t)(logsize - SAFE_BUFFER_MARGIN)) Log(USERLOG, "%s\r\n", output.buffer);
		Log(USERLOG, "");
	}
	if (curlBufferBase) ReleaseStack(curlBufferBase);

	return result;
}

/**
 * Creates a new parser based over a given  buffer with an array of tokens
 * available.
 */
static void jsmn_init(jsmn_parser* parser) {
	parser->pos = 0;
	parser->toknext = 0;
	parser->toksuper = -1;
}

static void MarkSet(FACT* F, unsigned int bit)
{
	while (F) // mark fields to ignore
	{
		WORDP subject = Meaning2Word(F->subject);
		subject->internalBits |= bit;
		F = GetObjectNext(F);
	}
}

static void UnmarkSet(FACT* F, unsigned int bit)
{
	while (F) // mark fields to ignore
	{
		WORDP subject = Meaning2Word(F->subject);
		subject->internalBits &= -1 ^ bit;
		F = GetObjectNext(F);
	}
}

static void Markem(char* ignore, char* underscore,FACT*& setignore, FACT*& setunderscore)
{
	MEANING id = 0;
	MEANING retMeaning = 0;
	int flags = 0;

	setignore = NULL;
	char* defaultignore = GetUserVariable("$cs_ignore"); // external default
	if (*defaultignore)
	{
		WORDP D = StoreWord(defaultignore, AS_IS);
		D->internalBits |= SETIGNORE; // no need to undo it
	}

	if (ignore && *ignore == '~') // fields to avoid processing
	{
		setignore = GetObjectHead(FindWord(ignore));
		MarkSet(setignore, SETIGNORE);
	}
	setunderscore = NULL;
	if (underscore && *underscore == '~') // fields to convert spaces to underscores
	{
		setunderscore = GetObjectHead(FindWord(underscore));
		MarkSet(setunderscore, SETUNDERSCORE);
	}
}

FunctionResult ParseXML(char* buffer, char* ptr, size_t size, bool nofail, char* ignore, bool purify, char* underscore)
{
	MEANING levels[100];
	bool blocked[100];
	MEANING objects[100];
	memset(levels, 0, sizeof(levels));
	memset(blocked, 0, sizeof(blocked));
	memset(objects, 0, sizeof(objects));
	unsigned int oldlanguage = language_bits;
	language_bits = LANGUAGE_UNIVERSAL;
	int depth = 0;
	MEANING M = 0;
	FACT* F;
	//<note>
	//	<to>Tove< / to>
	//	<from>Jani< / from>
	//	<heading>Reminder< / heading>
	//	<body>Don't forget me this weekend!</body>
	//	< / note >
	// <a href = "http://www.tutorialspoint.com/">Tutorialspoint!</a>
	// has a name, a value, and possible attributes before a > which faces the value
	// if next name at same level is same, we have array, otherwise object field
	//<note >
	//	<to>Tove</to>
	//	<a href = "http://www.tutorialspoint.com/" bing = "test">Tutorialspoint!</a>
	//	</note>
	//trace = -1;
	//echo = true;
	while (ptr && *ptr)
	{
		ptr = SkipWhitespace(ptr);
		bool endtag = (*ptr == '<' && (ptr[1] == '/' || ptr[2] == '/') );
		if (*ptr == '/' && ptr[1] == '>') endtag = true;
		if (endtag) // end tag
		{
			MEANING current = levels[depth]; // what we finished
			M = levels[--depth]; // resume prior level
			if (depth == 0) break; // top level

			printf("return to %d %s\r\n", depth, Meaning2Word(M)->word);
			FACT* F = GetSubjectNondeadHead(Meaning2Word(objects[depth]));
			if (F && F->verb== levels[depth + 1]) // we have a fact already of this name, make an array
			{
				F->flags |= FACTDEAD;
				MEANING xarray = GetUniqueJsonComposite((char*)"ja-");
				CreateFact(F->subject, F->verb, xarray, JSON_OBJECT_FACT | JSON_ARRAY_VALUE);
				objects[depth] = xarray; // change over to array basis
				CreateFact(xarray, MakeMeaning(StoreWord("0",AS_IS)), F->object, JSON_ARRAY_FACT | JSON_OBJECT_VALUE);
				CreateFact(xarray, MakeMeaning(StoreWord("1", AS_IS)), objects[depth + 1], JSON_ARRAY_FACT | JSON_OBJECT_VALUE);
			}
			else if (F) // adding to array
			{
				int index = 0;
				FACT* G = GetSubjectNondeadHead(Meaning2Word(objects[depth]));
				while (G)
				{
					++index;
					G = GetSubjectNondeadNext(G);
				}
				char label[20];
				sprintf(label, "%d", index);
				CreateFact(objects[depth], MakeMeaning(StoreWord(label, AS_IS)), objects[depth + 1], JSON_ARRAY_VALUE | JSON_OBJECT_FACT);
			}
			else
			{
				CreateFact(objects[depth], levels[depth + 1], objects[depth + 1], JSON_OBJECT_VALUE | JSON_OBJECT_FACT);
			}
			ptr = strchr(ptr, '>');
			++ptr; // past the close
		}
		else if (*ptr == '<') // start tag - 
		{
			char name[MAX_WORD_SIZE];
			char attribute[MAX_WORD_SIZE];
			ptr = ReadToken(ptr + 1, name);
			MEANING namemeaning = MakeMeaning(StoreWord(name));
			levels[++depth] = namemeaning;
			char* end = strchr(ptr, '>'); // would close any attributes and preceed the value 
			if (end && *(end - 1) == '/') --end;
			if (end) *end = 0; // block out the area of this level
			M = GetUniqueJsonComposite((char*)"jo-");
			objects[depth] = M; // we store attributes of this level here
			if (strchr(ptr, '=')) // we have attributes
			{
				// attributes of this level
				while (ptr && *ptr)
				{
					char* assign = strchr(ptr, '=');
					if (!assign) break; // end of attributes
					*assign = 0;

					ptr = ReadToken(ptr, attribute);
					char value[MAX_WORD_SIZE];
					ptr = ReadToken(assign+1, value);
					char* quote = value;
					while ((quote = strchr(value, '\"'))) memmove(quote, quote + 1, strlen(quote));
					unsigned int flags = 0;
					MEANING valx = jsonValue(value, flags, true);// not deleting using json literal   ^"" or "" would be the literal null in json
					if (valx == 0)
					{
						flags |= JSON_PRIMITIVE_VALUE;
						valx = MakeMeaning(StoreWord("null", AS_IS));
					}
		//			F = CreateFact(M, MakeMeaning(StoreWord(attribute)), valx, JSON_OBJECT_FACT | flags);
				}

				if (end[1] == '>') *end = '/';
				else *end = '>'; // end of attributes

				// now is either value or <... nest or close
				ptr = SkipWhitespace(end+1); // now at value if any
				if (*end == '/' && end[1] == '>') { ptr = end; }  // no value, closing level
				else if (*ptr == '<') {  }  // no value, nesting entity
				else // text value of field
				{
					end = strchr(ptr, '<');
					if (!*end) return FAILRULE_BIT;
					*end = 0;
					char* back = end;
					while (*--back == ' ') *back = 0;
					if (back != ptr) // non blank value
					{
						strcpy(attribute, ptr);
						F = CreateFact(M, levels[depth], MakeMeaning(StoreWord(attribute)), JSON_OBJECT_FACT | JSON_STRING_VALUE);
					}
					*end = '<';
				}
			}
			else // only have a text value and no attributes
			{
				ptr = ReadToken(ptr, attribute);
				char* quote = ptr;
				while ((quote == strchr(ptr, '\"'))) memmove(ptr, ptr + 1, strlen(ptr));
				F = CreateFact(M, levels[depth], MakeMeaning(StoreWord(attribute)), JSON_OBJECT_FACT | JSON_STRING_VALUE);
				*end = '>';
				ptr = end + 1;
			}
		}
	}
	language_bits = oldlanguage;
	currentFact = NULL;
	sprintf(buffer, "%s", Meaning2Word(objects[1])->word);
	return NOPROBLEM_BIT;
}

FunctionResult ParseJson(char* buffer, char* message, size_t size, bool nofail,char* ignore,bool purify,char* underscore)
{
	jsonpurify = purify;
	jsonIdIncrement = 1000; // fast hunt for a slot for names since this collection may be sizeable
	*buffer = 0;
	if (trace & TRACE_JSON)
	{
		if (size < (maxBufferSize - 100)) Log(USERLOG, "JsonParse Call: %s", message);
		else
		{
			char msg[MAX_WORD_SIZE];
			strncpy(msg, message, MAX_WORD_SIZE - 100);
			msg[MAX_WORD_SIZE - 100] = 0;
			Log(USERLOG, "JsonParse Call: %d-byte message too big to show all %s\r\n", size, msg);
		}
	}
	if (size < 1) return NOPROBLEM_BIT; // nothing to parse

	message = SkipWhitespace(message);
	if (*message != '<' && *message != '{' && *message != '[') // not json or xml
	{
		return FAILRULE_BIT;
	}
	FACT* setignore = NULL;
	FACT* setunderscore = NULL;
	Markem(ignore, underscore, setignore, setunderscore);

	// its XML, not json, convert to json 
	if (*message == '<')
	{
		FunctionResult result = ParseXML(buffer, message, size, nofail, ignore, purify, underscore);
		if (setignore) UnmarkSet(setignore, SETIGNORE);
		if (setunderscore) UnmarkSet(setunderscore, SETUNDERSCORE);
		return result;
	}

	uint64 start_time = ElapsedMilliseconds();

	FACT* start = lastFactUsed;
	char* limit;
	char* oldstack = stackFree;
	jsmntok_t* tokens = (jsmntok_t*)InfiniteStack(limit, "JSONPARSE");
	jsmn_parser parser;
	jsmn_init(&parser);
	*detectedlanguage = 0;
	jsmnerr_t  jtokenCount = jsmn_parse(&parser, message, size, tokens);
	int used = (jtokenCount > 0) ? (sizeof(jsmntok_t) * jtokenCount) : 0;
	CompleteBindStack(used);
	strcpy(jsonoldlang, current_language);
	MEANING id = 0;
	if (jtokenCount < 1) id = 0; // error in json syntax
	else
	{
		MEANING retMeaning = 0;
		int flags = 0;
		int enableadjust = 0;
		conceptSeen = 0;
		if (ignore) enableadjust |= 1;
		if (underscore) enableadjust |= 4;
		int X = factsJsonHelper(0,message, tokens, 0, &retMeaning, &flags, false, nofail,enableadjust);
		id = (X) ? retMeaning : 0;
		currentFact = NULL;	 // used up by putting into json

		if (setignore) UnmarkSet(setignore, SETIGNORE);
		if (setunderscore) UnmarkSet(setunderscore, SETUNDERSCORE);
	}
	if (timing & TIME_JSON)
	{
		int diff = (int)(ElapsedMilliseconds() - start_time);
		if (timing & TIME_ALWAYS || diff > 0) Log(STDTIMELOG, (char*)"Json parse time: %d ms\r\n", diff);
	}

	if (id != 0) // worked, this is name of json structure
	{
		if (trace & TRACE_JSON )
		{
			NextInferMark();
			WORDP D = Meaning2Word(id);
			jwritehierarchy(false, false, 2, currentOutputBase, D,(D->word[1] == 'o') ? JSON_OBJECT_VALUE : JSON_ARRAY_VALUE, 100); // nest of 0 (unspecified) is infinitiy
			Log(ECHOUSERLOG, "%s\r\n", currentOutputBase);
		}

		strcpy(buffer, Meaning2Word(id)->word);
		nofail = true;
	}
	else // failed, delete any facts created along the way
	{
		while (lastFactUsed > start) FreeFact(lastFactUsed--); //   restore back to facts alone
		FILE* out = FopenUTF8WriteAppend("tmp/badjson.txt");
		fprintf(out, "%s\r\n", message);
		FClose(out);
	}
	stackFree = oldstack;
	return (nofail) ? NOPROBLEM_BIT : FAILRULE_BIT;
}

static char* jtab(int depth, char* buffer)
{
	while (depth--) *buffer++ = ' ';
	*buffer = 0;
	return buffer;
}

int orderJsonArrayMembers(WORDP D, FACT * *store,size_t & size)
{
	int max = -1;
	FACT* G = GetSubjectNondeadHead(D);
	bool show = false;

	// nominally how many elements are there we need to initialize.
	while (G)
	{
		if (G->flags & JSON_ARRAY_FACT)
		{
			int index = atoi(Meaning2Word(G->verb)->word);
			if (index > max) max = index;
			if (G->flags & JSON_STRING_VALUE)
				size += strlen(Meaning2Word(G->object)->word);
			else if (G->flags & JSON_PRIMITIVE_VALUE) size += 4;
			else size += 1000;
		}
		G = GetSubjectNondeadNext(G);
	}
	memset(store, 0, sizeof(FACT*) * (max + 1));

	G = GetSubjectNondeadHead(D);
	while (G) // get facts in order - but if user manually deleted externally, we will have a hole.
	{
		if (show) TraceFact(G);
		if (G->flags & JSON_ARRAY_FACT) // in case of accidental collisions with normal words
		{
			int index = atoi(Meaning2Word(G->verb)->word);
			store[index] = G;
		}
		G = GetSubjectNondeadNext(G);
	}
	return max + 1; // for the 0th value
}

char* jwritehierarchy(bool log, bool defaultZero, int depth, char* buffer, WORDP D, int subject, int nest)
{
	char* xxoriginal = buffer;
	char* basis = buffer;
	unsigned int size = (buffer - currentOutputBase + 200); // 200 slop to protect us
	if (size >= currentOutputLimit)
	{
		ReportBug((char*)"INFO: Too much json hierarchy");
		return buffer; // too much output
	}

	int index = 0;
	if (!stricmp(D->word, (char*)"null"))
	{
		strcpy(buffer, (subject & JSON_STRING_VALUE) ? (char*)"\"null\"" : D->word);
		return buffer + strlen(buffer);
	}
	if (!(subject & (JSON_ARRAY_VALUE | JSON_OBJECT_VALUE)))
	{
		if (subject & JSON_STRING_VALUE) *buffer++ = '"';
		if (subject & JSON_STRING_VALUE) AddEscapes(buffer, D->word, true, currentOutputLimit - (buffer - currentOutputBase), false);
		else strcpy(buffer, D->word);
		buffer += strlen(buffer);
		if (subject & JSON_STRING_VALUE) *buffer++ = '"';
		return buffer;
	}

	if (D->inferMark == inferMark)
	{
		char* kind = (D->word[1] == 'a') ? (char*)"[...]" : (char*)"{...}";
		sprintf(buffer, "%s %s", D->word, kind);
		return buffer + strlen(buffer);
	}
	D->inferMark = inferMark;

	if (D->word[1] == 'a') 
		strcat(buffer, (char*)"[");
	else strcat(buffer, (char*)"{    ");

	if (nest <= 1) strcat(buffer, (char*)"... "); // show we had more but stopped
	else if (D->word[1] == 'o')
	{
		strcat(buffer, (char*)"# ");
		strcat(buffer, D->word);
	}
	buffer += strlen(buffer);

	if (nest-- <= 0) // immediately close a composite
	{
		if (D->word[1] == 'a') strcpy(buffer, (char*)"]");
		else strcpy(buffer, (char*)"}");
		buffer += strlen(buffer);
		return buffer; // do nothing now. dont do this composite
	}
	if (log)
	{
		Log(USERLOG, "%s", basis);
		basis = buffer;
	}

	size_t textsize = 0;
	FACT* F = GetSubjectNondeadHead(MakeMeaning(D));
	int indexsize = 0;
	bool invert = false;
	char* limit;
	FACT** stack = (FACT**)InfiniteStack(limit, "jwritehierarchy");
	if (F && F->flags & JSON_ARRAY_FACT)
	{
		indexsize = orderJsonArrayMembers(D, stack, textsize); // tests for illegal delete
	}
	else // json object
	{
		invert = true;
		while (F) // stack object key data
		{
			if (F->flags & JSON_OBJECT_FACT)
			{
				stack[index++] = F;
				++indexsize;
			}
			F = GetSubjectNondeadNext(F);
			if (indexsize > 1999) F = 0; // abandon extra
		}
	}
	CompleteBindStack64(indexsize, (char*)stack);

	if (textsize && textsize < 52);
	else if (D->word[1] == 'o') strcat(buffer, (char*)"\r\n");
	buffer += strlen(buffer);
	bool tight = (textsize && textsize < 52);
	for (int i = 0; i < indexsize; ++i)
	{
		unsigned int itemIndex = (invert) ? (indexsize - i - 1) : i;
		size = (buffer - currentOutputBase + 400); // 400 slop to protect us
		if (size >= currentOutputLimit)
		{
			// ReportBug((char*)"INFO: JsonWrite too much output %d items size %d", indexsize, size);
			ReleaseStack((char*)stack);
			return buffer; // too much output
		}
		F = stack[itemIndex];
		if (!F) continue; // not actually in order
		else if (F->flags & JSON_ARRAY_FACT)
		{
			if (tight)
			{
				strcat(buffer, "  ");
				buffer += 2;
			}
			else buffer = jtab(depth, buffer);
		}
		else if (F->flags & JSON_OBJECT_FACT)
		{
			buffer = jtab(depth, buffer);
			strcpy(buffer++, (char*)"\""); // write key in quotes, might need to have escapes
            buffer = AddEscapes(buffer, Meaning2Word(F->verb)->word, true, currentOutputLimit - (buffer - currentOutputBase), false);
			strcpy(buffer, (char*)"\": ");
			buffer += 3;
		}
		else continue;	 // not a json fact, an accident of something else that matched
		// continuing composite
		buffer = jwritehierarchy(log, defaultZero, depth + 2, buffer, Meaning2Word(F->object), F->flags, nest);
		if (i < (indexsize - 1)) strcpy(buffer++, (char*)",");
		if (tight) strcpy(buffer, (char*)"  ");
		else strcpy(buffer, (char*)"\r\n"); // always close objects
		buffer += 2;
		if (log)
		{
			char* nl = strchr(buffer, '\n');
			if (nl) basis = nl + 1;
			Log(USERLOG, "%s", basis);
			basis = buffer;
		}
		if (defaultZero) break;
	}
	if (!tight) buffer = jtab(depth - 2, buffer);
	if (D->word[1] == 'a')
	{
		strcpy(buffer, (char*)" ]   ");
		strcat(buffer, (char*)"# ");
		strcat(buffer, D->word);
	}
	else strcpy(buffer, (char*)"}");
	buffer += strlen(buffer);
	ReleaseStack((char*)stack);
	return buffer;
}

FunctionResult JSONTreeCode(char* buffer)
{
	char* arg1 = ARGUMENT(1); // names a fact label
	WORDP D = FindWord(arg1);
	if (!D)
	{
		if (IsValidJSONName(arg1, 'a')) // empty array
		{
			strcpy(buffer, "[ ]");
			return NOPROBLEM_BIT;
		}
		else if (IsValidJSONName(arg1, 'o')) // empty object
		{
			strcpy(buffer, "{ }");
			return NOPROBLEM_BIT;
		}
		else return FAILRULE_BIT;
	}
	char* arg2 = ARGUMENT(2);
	int nest = atoi(arg2);
	*buffer = 0;
	//strcpy(buffer, (char*)"JSON=> \r\n");
	NextInferMark();
	bool log = false;
	bool defaultZero = false;
	if (*ARGUMENT(3))
	{
		log = true;
		if (*ARGUMENT(4) != 0) defaultZero = true;
	}
	buffer += strlen(buffer);
	buffer = jwritehierarchy(log, defaultZero, 2, buffer, D, (arg1[1] == 'o') ? JSON_OBJECT_VALUE : JSON_ARRAY_VALUE, nest > 0 ? nest : 20000); // nest of 0 (unspecified) is infinitiy
    //strcpy(buffer, (char*)"\r\n<=JSON \r\n");
	strcpy(buffer, (char*)"\r\n");
	return NOPROBLEM_BIT;
}

FunctionResult JSONKindCode(char* buffer)
{
	char* arg = ARGUMENT(1);
	if (!IsValidJSONName(arg)) return FAILRULE_BIT;
	if (!strnicmp(arg, "jo-", 3)) strcpy(buffer, (char*)"object");
	else if (!strnicmp(arg, "ja-", 3)) strcpy(buffer, (char*)"array");
	else return FAILRULE_BIT; // otherwise fails
	return NOPROBLEM_BIT;
}

FunctionResult JSONStorageCode(char* buffer)
{
	char* arg = ARGUMENT(1);
	if (!IsValidJSONName(arg)) return FAILRULE_BIT;
	if (IsTransientJson(arg) ) strcpy(buffer, (char*)"transient");
	else if (IsBootJson(arg)) strcpy(buffer, (char*)"boot");
	else strcpy(buffer, (char*)"permanent");
	return NOPROBLEM_BIT;
}

static FunctionResult JSONpath(char* buffer, char* path, char* jsonstructure, bool raw, bool nofail)
{
	char mypath[MAX_WORD_SIZE];
	if (*path != '.' && *path != '[')  // default a dot notation if not given
	{
		mypath[0] = '.';
		strcpy(mypath + 1, path);
	}
	else strcpy(mypath, path);
	path = mypath;

	WORDP D = FindWord(jsonstructure);
	FACT* F;
	if (!D) return FAILRULE_BIT;
	path = SkipWhitespace(path);
	MEANING M;
	if (trace & TRACE_JSON)
	{
		Log(USERLOG, "\r\n");
		Log(USERLOG, "");
	}

	while (1)
	{
		path = SkipWhitespace(path);
		if (!*path) // reached the bottom of the path
		{
			if (!D) return FAILRULE_BIT;
			// if it has whitespace or JSON special characters in it, we must return it as a string in quotes so JsonFormat can detect
			// unless raw was requested
			if (!raw)
			{
				char* at = D->word - 1; // D cannot have eol,eor,or tab in it
				while (*++at)
				{
					char c = *at;
					if (c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == ':' || IsWhiteSpace(c)) break;
				}
				if (!*at) raw = true; // safe to use raw output
			}
			if (!raw) *buffer++ = '"';
			strcpy(buffer, D->word);
			if (!raw)
			{
				buffer += strlen(buffer);
				*buffer++ = '"';
				*buffer = 0;
			}
			break;
		}
		if (*path == '[' || *path == '.') // we accept field names as single words in CS, not as quoted possibly weird strings
		{
			if (!D) return FAILRULE_BIT;
			char* next = path + 1;
			if (*next == '"') // handle quoted key
			{
				while (*++next)
				{
					if (*next == '"' && *(next - 1) != '\\')  break; // leave on the closing quote
				}
			}
			while (*next && *next != '[' && *next != ']' && *next != '.' && *next != '*') ++next; // find token break
			char c = *next;
			*next = 0; // close off continuation
			if (*path == '[' && !IsDigit(path[1]) && path[1] != USERVAR_PREFIX) return FAILRULE_BIT;
			F = GetSubjectNondeadHead(D);
			char what[MAX_WORD_SIZE];
			if (path[1] == '"') // was quoted fieldname
			{
				strcpy(what, path + 2);
				what[strlen(what) - 1] = 0; // remove closing quote
			}
			else strcpy(what, path + 1);
			if (*what == USERVAR_PREFIX) strcpy(what, GetUserVariable(what, false));
			M = MakeMeaning(FindWord(what)); // is CASE sensitive
			if (!M) return FAILRULE_BIT; // cant be in a fact if it cant be found
			while (F)
			{
				if (F->verb == M)
				{
					if (trace & TRACE_JSON) TraceFact(F); // dont trace all, just the matching one
					D = Meaning2Word(F->object);
					break;
				}
				F = GetSubjectNondeadNext(F);
			}
			*next = c;
			path = next;
			if (*path == ']') ++path;

			if (F && *path == '*' && !path[1]) // return the fact ID at the bottom
			{
				sprintf(buffer, "%u", Fact2Index(F));
				return NOPROBLEM_BIT;
			}

			if (!F)
				return FAILRULE_BIT;
		}
		else return FAILRULE_BIT;
	}
	return NOPROBLEM_BIT;
}

FunctionResult JSONPathCode(char* buffer)
{
	char* path = ARGUMENT(1);
	bool safe = !stricmp(ARGUMENT(3), (char*)"safe"); // dont quote the selected value if a complex string
	if (*path == '^') ++path; // skip opening functional marker
	if (*path == '"') ++path; // skip opening quote
	size_t len = strlen(path);
	if (path[len - 1] == '"') path[len - 1] = 0;
	if (*path == 0)
	{
		strcpy(buffer, ARGUMENT(2));	// return the item itself
		return NOPROBLEM_BIT;
	}
	return JSONpath(buffer, path, ARGUMENT(2), !safe, false);
}

static MEANING jcopy(MEANING M)
{
	if (!M) return 0;
	WORDP D = Meaning2Word(M);
	int index = 0;
	MEANING composite = 0;
	if (D->word[1] == 'a')  composite = GetUniqueJsonComposite((char*)"ja-");
	else composite = GetUniqueJsonComposite((char*)"jo-");

	bool invert = false;
	int indexsize = 0;
	size_t textsize = 0;
	FACT* F = GetSubjectNondeadHead(D);
	char* limit;
	FACT** stack = (FACT**)InfiniteStack(limit, "jcopy");
	if (F && F->flags & JSON_ARRAY_FACT) indexsize = orderJsonArrayMembers(D, stack,textsize); // tests for illegal delete
	else
	{
		invert = true;
		while (F) // stack them
		{
			if (F->flags & JSON_OBJECT_FACT) // no collision with possible outside weird words
			{
				stack[index++] = F;
				++indexsize;
			}
			F = GetSubjectNondeadNext(F);
			if (indexsize > 1999) F = 0; // abandon extra
		}
	}
	CompleteBindStack64(indexsize, (char*)stack);
	int flags = 0;
	for (int i = 0; i < indexsize; ++i)
	{
		unsigned int itemIndex = (invert) ? (indexsize - i - 1) : i;
		F = stack[itemIndex];
		if (!F) continue; // missing
		else if (F->flags & (JSON_ARRAY_VALUE | JSON_OBJECT_VALUE)) // composite
			CreateFact(composite, F->verb, jcopy(F->object), (F->flags & JSON_FLAGS) | jsonPermanent);
		else CreateFact(composite, F->verb, F->object, (F->flags & JSON_FLAGS) | jsonPermanent); // noncomposite
	}
	ReleaseStack((char*)stack);
	return composite;
}

void jkillfact(WORDP D)
{
	if (!D) return;
	FACT* F = GetSubjectNondeadHead(D); // delete everything including marker
	while (F)
	{
		FACT* G = GetSubjectNondeadNext(F);
		if (F->flags & (JSON_ARRAY_FACT | JSON_OBJECT_FACT)) KillFact(F, true, false); // json object/array no longer has this fact
		if (F->flags & FACTAUTODELETE) AutoKillFact(F->object); // kill the fact
		F = G;
	}
}

static bool NotPlain(char* word)
{
	--word;
	while (*++word)
	{
		if (IsWhiteSpace(*word) || *word == ':' || *word == '\\') return true;
	}
	return false;
}

unsigned int jwritesize(WORDP D, int subject, bool plain, unsigned int size)
{// subject is 1 (starting json object) or flags (internal json indicating type and subject/object)
    size_t len = D->length;
    int index = 0;
    unsigned int limit = 10000;

    if (!(subject & (JSON_OBJECT_VALUE | JSON_ARRAY_VALUE)) && subject & JSON_FLAGS)
    { // this is json primitive or string
        if (subject & JSON_STRING_VALUE)
        {
            if (!stricmp(D->word, (char*)"null"))
            {
                size += 6;
            }
            else
            {
                bool usequote = !plain || !NotPlain(D->word);
                if (usequote) size += 2;
                char* buffer = AllocateStack(NULL, limit);
                char* end = AddEscapes(buffer, D->word, true, limit, false, true);
                size += strlen(buffer);
                ReleaseStack(buffer);
            }
        }
        else // true/false/number/null
        {
            size += len;
        }
        return size;
    }

    // CS (not JSON) can have recursive structures. Protect against this
    if (D->inferMark == inferMark)
    {
        return size + len;
    }
    D->inferMark = inferMark;

    ++size; // json array/object
	size_t textsize = 0;
    bool invert = false;
    int indexsize = 0;
    FACT* F = GetSubjectNondeadHead(D);
    char* xlimit;
    FACT** stack = (FACT**)InfiniteStack(xlimit, "jwrite");
    if (F && F->flags & JSON_ARRAY_FACT)
    {
        indexsize = orderJsonArrayMembers(D, stack, textsize); // tests for illegal delete
    }
    else
    {
        invert = true;
        while (F) // stack them
        {
            if (F->flags & JSON_OBJECT_FACT) // no collision with possible outside weird words
            {
                stack[index++] = F;
                ++indexsize;
            }
            F = GetSubjectNondeadNext(F);
        }
    }
    CompleteBindStack64(indexsize, (char*)stack);
    for (int i = 0; i < indexsize; ++i)
    {
        unsigned int itemIndex = (invert) ? (indexsize - i - 1) : i;
        F = stack[itemIndex];
        if (!F) continue;
        else if (F->flags & JSON_ARRAY_FACT) { ; } // write out its elements
        else if (F->flags & JSON_OBJECT_FACT)
        {
            WORDP D = Meaning2Word(F->verb);

            bool usequote = !plain || !NotPlain(D->word);
            char* buffer = AllocateStack(NULL, limit);
            char* end = AddEscapes(buffer, D->word, true, limit, false, true);
            size += strlen(buffer);
            ReleaseStack(buffer);
            size += (usequote ? 4 : 2);
        }
        else continue;     // not a json fact, an accident of something else that matched

        size = jwritesize(Meaning2Word(F->object), F->flags & JSON_FLAGS, plain, size);
        if (i < (indexsize - 1))
        {
            size += 2;
        }
    }
    // close the composite
    size += 2;

    return size;
}

char* jwrite(char* start, char* buffer, WORDP D, int subject, bool plain,unsigned int limit)
{// subject is 1 (starting json object) or flags (internal json indicating type and subject/object)
	size_t len = WORDLENGTH(D);
	char* base = buffer;
	unsigned int actual = buffer - start + len + 200; // some slop
	if (actual  > limit)
	{
		ReportBug("Jwrite too much %d of %d", actual, limit);
		return buffer;
	}
	int index = 0;
	if (!(subject & (JSON_OBJECT_VALUE | JSON_ARRAY_VALUE)) && subject & JSON_FLAGS)
	{ // this is json primitive or string
		if (subject & JSON_STRING_VALUE) 
		{
			if (!stricmp(D->word, (char*)"null"))
			{
				strcpy(buffer, (char*)"\"null\"");
				buffer += 6;
			}
			else if (D->word[0] == '"' && D->word[1] == '"')
			{
				strcpy(buffer, D->word);
				buffer += 2;
			}
			else
			{
				bool usequote = !plain || !NotPlain(D->word);
				if (usequote) strcpy(buffer++, (char*)"\"");
				buffer = AddEscapes(buffer, D->word, true, limit, false,true);
				if (usequote) strcpy(buffer++, (char*)"\"");
			}
		}
		else // true/false/number/null
		{
			strcpy(buffer, D->word);
			buffer += len;
		}

		return buffer;
	}

	// CS (not JSON) can have recursive structures. Protect against this
	if (D->inferMark == inferMark)
	{
		strcpy(buffer, D->word);
		return buffer + len;
	}
	D->inferMark = inferMark;

	if (D->word[1] == 'a') 
		strcpy(buffer++, (char*)"["); // json array
	else strcpy(buffer++, (char*)"{"); // json object
	
	bool invert = false;
	int indexsize = 0;
	size_t textsize = 0;
	FACT* F = GetSubjectNondeadHead(D);
	char* xlimit;
	FACT** stack = (FACT**)InfiniteStack(xlimit, "jwrite");
	if (F && F->flags & JSON_ARRAY_FACT)
	{
		indexsize = orderJsonArrayMembers(D, stack,textsize); // tests for illegal delete
	}
	else
	{
		invert = true;
		while (F) // stack them
		{
			if (F->flags & JSON_OBJECT_FACT) // no collision with possible outside weird words
			{
				stack[index++] = F;
				++indexsize;
			}
			F = GetSubjectNondeadNext(F);
		}
	}
	CompleteBindStack64(indexsize, (char*)stack);
	for (int i = 0; i < indexsize; ++i)
	{
		unsigned int itemIndex = (invert) ? (indexsize - i - 1) : i;
		F = stack[itemIndex];
		if (!F) continue;
		else if (F->flags & JSON_ARRAY_FACT) { ; } // write out its elements
		else if (F->flags & JSON_OBJECT_FACT)
		{
			WORDP E = Meaning2Word(F->verb);
			size_t size = (buffer - start + 400 + WORDLENGTH(E));  // 400 slop
			if (size >= limit)
			{
				ReportBug("jwrite overflow %s AND %s", start, E->word);
				return buffer; // too much output
			}
			
			bool usequote = !plain || !NotPlain(E->word);
			if (usequote) strcpy(buffer++, (char*)"\""); // put out key in quotes, possibly with escapes
            buffer = AddEscapes(buffer, E->word, true, limit, false,true);
			if (usequote)
			{
				strcpy(buffer, (char*)"\": ");
				buffer += 3;
			}
			else
			{
				strcpy(buffer, (char*)": ");
				buffer += 2;
			}
		}
		else continue;	 // not a json fact, an accident of something else that matched
		char* before = buffer;
		buffer = jwrite(start,buffer, Meaning2Word(F->object), F->flags & JSON_FLAGS, plain,limit);
		if (i < (indexsize - 1))
		{
			strcpy(buffer, (char*)", ");
			buffer += 2;
		}
	}
	// close the composite
	if (D->word[1] == 'a')  strcpy(buffer, (char*)"] ");
	else strcpy(buffer, (char*)"} ");
	buffer += 2;

	ReleaseStack((char*)stack);
	return buffer;
}

FunctionResult JSONWriteCode(char* buffer) // FACT to text
{
	char* arg1 = ARGUMENT(1); // names a fact label
	bool plain = false;
	char* start = buffer;
	if (!stricmp(arg1, "plain"))
	{
		arg1 = ARGUMENT(2);
		plain = true;
	}
	WORDP D = FindWord(arg1);
	if (!D)
	{
		if (IsValidJSONName(arg1, 'a')) // empty array
		{
			strcpy(buffer, "[ ]");
			return NOPROBLEM_BIT;
		}
		else if (IsValidJSONName(arg1, 'o')) // empty object
		{
			strcpy(buffer, "{ }");
			return NOPROBLEM_BIT;
		}
		else return FAILRULE_BIT;
	}
	uint64 start_time = ElapsedMilliseconds();
	NextInferMark();
	jwrite(currentOutputBase,buffer, D, 1, plain, currentOutputLimit); // json structure

	if (timing & TIME_JSON) {
		int diff = (int)(ElapsedMilliseconds() - start_time);
		if (timing & TIME_ALWAYS || diff > 0) Log(STDTIMELOG, (char*)"Json write time: %d ms\r\n", diff);
	}
	return NOPROBLEM_BIT;
}

FunctionResult JSONUndecodeStringCode(char* buffer) // undo escapes
{
	char* arg1 = ARGUMENT(1);
	unsigned int remainingSize = currentOutputLimit - (buffer - currentOutputBase) - 1;
	CopyRemoveEscapes(buffer, arg1, remainingSize, true); // removing ALL escapes
	return NOPROBLEM_BIT;
}

static MEANING MergeObject(bool keyonly, int sum, MEANING obj1, MEANING obj2)
{ // prefer values of obj1 over obj2
	if (!obj1) return jcopy(obj2);
	if (!obj2) return jcopy(obj1); // maybe need to copy?
	MEANING M = jcopy(obj1); // raw copy

	// now augment this with extra data in obj2, but obj1 wins in same field
	FACT* obj2G = GetSubjectNondeadHead(obj2);
	while (obj2G) // see if new field in old
	{
		FACT* obj1F = GetSubjectNondeadHead(M);
		while (obj1F) // do we already have this field? 
		{
			if (obj2G->verb == obj1F->verb) break; // same field (old preference)
			obj1F = GetSubjectNondeadNext(obj1F);
		}
		
		int kind = obj2G->flags & (JSON_PRIMITIVE_VALUE | JSON_STRING_VALUE);
		if (kind) // add primitive
		{
            // if we already have this field, we win, ignore other
			if (!obj1F && sum != 2) CreateFact(M, obj2G->verb, obj2G->object, kind | JSON_OBJECT_FACT | jsonPermanent);
            else if (sum && obj1F)
            {
                // unless summing them together
                WORDP w1 = Meaning2Word(obj1F->object);
                WORDP w2 = Meaning2Word(obj2G->object);
                double number1 = (strchr(w1->word,'.')) ? Convert2Double(w1->word) : (double)Convert2Integer(w1->word);
                double number2 = (strchr(w2->word,'.')) ? Convert2Double(w2->word) :  (double)Convert2Integer(w2->word);
                double result = number1 + number2;
                if (result != number1)
                {
                    char* resultText = AllocateBuffer();
                    long x = (long) result;
                    if ((double)x == result) strcpy(resultText, Print64(x));
                    else WriteFloat(resultText,result);

                    WORDP newObject = StoreWord(resultText,AS_IS);
                    MEANING value = MakeMeaning(newObject);

                    UnweaveFactObject(obj1F);
                    SetObjectHead(newObject, AddToList(GetObjectHead(newObject), obj1F, GetObjectNext, SetObjectNext));
                    obj1F->object = value;
                    ModBaseFact(obj1F);
                    FreeBuffer();
                }
            }
		}
		else if (obj2G->flags & JSON_OBJECT_VALUE)
		{
			if (!obj1F) // add new field to our object
			{
				CreateFact(M, obj2G->verb, jcopy(obj2G->object), JSON_OBJECT_FACT | JSON_OBJECT_VALUE | jsonPermanent);
			}
		}
		else return 0; // we dont do array field
		obj2G = GetSubjectNondeadNext(obj2G);
	}
	return M;
}

static MEANING MergeArray(bool keyonly,MEANING ar1, MEANING ar2)
{ // merge means ar1 values have preference if collide with ar2
	if (!ar1) return jcopy(ar2);
	if (!ar2) return jcopy(ar1); // maybe need to copy?
	// merging array of primitives is to add missing primitives
	// Problem with objects is some field names and values may be the same
	//Like:  {name: bob, age:20}   vs {name: harry, age:20} 
	// So we will assume the FIRST field/value pair defines a unique object
	// And we merge same ones. No match creates new one
	MEANING M = jcopy(ar1); // start with first array in full
	int index = 0;
	FACT* C = GetSubjectNondeadHead(ar1);
	while (C) // current array member count of primary array
	{
		++index;
		C = GetSubjectNondeadNext(C);
	}

	// now augment with extra data
	FACT* ar2G = GetSubjectNondeadHead(ar2);
	while (ar2G) // look at new array values
	{
		int flags = ar2G->flags & (JSON_PRIMITIVE_VALUE | JSON_STRING_VALUE);
		if (flags) // simple type
		{
			FACT* ar1F = GetSubjectNondeadHead(M);
			while (ar1F)
			{
				if (ar1F->object == ar2G->object) break; // have this
				ar1F = GetSubjectNondeadNext(ar1F);
			}
			if (!ar1F) // add primitive to array at end, ignore if already present
			{
				char counter[20];
				sprintf(counter, "%d", index);
				CreateFact(M, MakeMeaning(StoreWord(counter, AS_IS)), ar2G->object, flags | JSON_ARRAY_FACT | jsonPermanent);
			}
		}
		else if (ar2G->flags & JSON_OBJECT_VALUE)
		{ // 2nd array has an object as value, find corresponding object in our 1st copy
			FACT* basis = EarliestFact(ar2G->object);
			if (!basis) return 0;
			MEANING label = basis->verb;
			MEANING value = basis->object; // presumed simple type for now
			// we want to merge ar2G into some other object where first field matches
			FACT* ar1F = GetSubjectNondeadHead(M); // any object which has field match
			while (ar1F) // walk original array facts looking for objects
			{
				if (ar1F->flags & JSON_OBJECT_VALUE) // found an object
				{
					FACT* R = EarliestFact(ar1F->object);
					if (R->verb == label && (keyonly || R->object == value)) break;// field is a label
				}
				ar1F = GetSubjectNondeadNext(ar1F);
			}
			if (!ar1F) // need to add object at new index, never found match
			{
				MEANING X = jcopy(ar2G->object);
				char counter[20];
				sprintf(counter, "%d", index++);
				CreateFact(M, MakeMeaning(StoreWord(counter, AS_IS)), X, JSON_OBJECT_VALUE | JSON_ARRAY_FACT | jsonPermanent);
			}
			// otherwise since we have object of same name, the old is irrelevant
		}
		else return 0; // we dont do arrays of arrays
		ar2G = GetSubjectNondeadNext(ar2G);
	}
	return M;
}

FunctionResult JSONMergeCode(char* buffer)
{ 
	int index = JSONArgs(); // not needed but allowed
	char* arg0 = ARGUMENT(index++); // kind of it
	bool keyonly = true;
    int sum = 0;
	if (!stricmp(arg0, "key")) keyonly = true;
	else if (!stricmp(arg0, "key-value")) keyonly = false;
    else if (!stricmp(arg0, "sum")) sum = 1;
    else if (!stricmp(arg0, "sumif")) sum = 2;
	else return FAILRULE_BIT;
	char* arg1 = ARGUMENT(index++); // names a  label
	char* arg2 = ARGUMENT(index); // names a  label
	WORDP answer = NULL;
	if (IsValidJSONName(arg1, 'o') && (IsValidJSONName(arg2, 'o') || !*arg2)) 
	{
		answer = Meaning2Word(MergeObject(keyonly,sum,MakeMeaning(FindWord(arg1)),
				MakeMeaning(FindWord(arg2))));
	}
	else if (IsValidJSONName(arg1, 'a') && (IsValidJSONName(arg2, 'a') || !*arg2)) 
	{
		answer = Meaning2Word(MergeArray(keyonly,MakeMeaning(FindWord(arg1)),
			MakeMeaning(FindWord(arg2))));
	}
	currentFact = NULL;
	if (!answer)  return FAILRULE_BIT; 
	strcpy(buffer, answer->word);
	return NOPROBLEM_BIT;
}

FunctionResult JSONLabelCode(char* buffer)
{
	char* arg1 = ARGUMENT(1); // names a  label
	if (strlen(arg1) > MAX_JSON_LABEL) arg1[MAX_JSON_LABEL] = 0;
	if (*arg1 == '"')
	{
		size_t len = strlen(arg1);
		arg1[len - 1] = 0;
		++arg1;
	}
	if (strchr(arg1, ' ') ||
		strchr(arg1, '\\') || strchr(arg1, '"') || strchr(arg1, '\'') || strchr(arg1, 0x7f) || strchr(arg1, '\n') // refuse illegal content
		|| strchr(arg1, '\r') || strchr(arg1, '\t')) return FAILRULE_BIT;	// must be legal unescaped json content and safe CS content
	if (*arg1 == 't' && !arg1[1]) return FAILRULE_BIT;	//  reserved for CS to mark transient
	strcpy(jsonLabel, arg1);
	return NOPROBLEM_BIT;
}

static bool jsonGather(WORDP D, int limit, int depth)
{
	if (limit && depth >= limit) return true;	// not allowed to do this level or lower
	FACT* F = GetSubjectNondeadHead(D);
	while (F) // flip the order
	{
		if (jsonIndex >= MAX_FIND) return false; // abandon extra
		if (F->flags & JSON_FLAGS) factSet[jsonStore][++jsonIndex] = F;
		if (F->flags & (JSON_ARRAY_VALUE| JSON_OBJECT_VALUE) && !jsonGather(Meaning2Word(F->object), limit, depth + 1)) return false;
		F = GetSubjectNondeadNext(F);
	}
	return true;
}

FunctionResult JSONGatherCode(char* buffer) // jason FACT cluster by name to factSet
{
	int index = 1;
	if (impliedSet != ALREADY_HANDLED) jsonStore = impliedSet;
	else
	{
		jsonStore = GetSetID(ARGUMENT(index++));
		if (jsonStore == ILLEGAL_FACTSET) return FAILRULE_BIT;
	}
	if (impliedSet == ALREADY_HANDLED) // clear the set passed as argument
	{
		jsonIndex = 0;
		SET_FACTSET_COUNT(jsonStore, 0);
	}
	else jsonIndex = FACTSET_COUNT(jsonStore); // keep going in set (like +=)

	WORDP D = FindWord(ARGUMENT(index++));
	if (!D) return FAILRULE_BIT;
	if (!IsValidJSONName(D->word)) return FAILRULE_BIT;
	char* depth = ARGUMENT(index);
	int level = atoi(depth);	// 0 is infinite, 1 is top level
	if (!jsonGather(D, level, 0)) return FAILRULE_BIT;
	SET_FACTSET_COUNT(jsonStore, jsonIndex);
	impliedSet = ALREADY_HANDLED;
	return NOPROBLEM_BIT;
}

static bool jsonWalkNode(WORDP D, FACT_FUNCTION func, uint64 data, int limit, int depth)
{
    if (limit && depth >= limit) return true;    // not allowed to do this level or lower
    FACT* F = GetSubjectNondeadHead(D);
    while (F) // flip the order
    {
        if (F->flags & JSON_FLAGS) (func)(F,data);
        if (F->flags & (JSON_ARRAY_VALUE | JSON_OBJECT_VALUE) && !jsonWalkNode(Meaning2Word(F->object), func, data, limit, depth + 1)) return false;
        F = GetSubjectNondeadNext(F);
    }
    return true;
}

void WalkJsonHierarchy(WORDP root, FACT_FUNCTION func, uint64 data, int depth)
{
    if (!root ||!func) return;
    if (!IsValidJSONName(root->word)) return;
    if (!jsonWalkNode(root, func, data, depth, 0)) return;
}

FunctionResult JSONParseCode(char* buffer)
{
	safeJsonParse = false;
	int index = JSONArgs();
	bool nofail = !stricmp(ARGUMENT(index), (char*)"nofail");
	if (nofail) index++;
	char* data = ARGUMENT(index++);
	if (!*data) return FAILRULE_BIT;

	if (*data == '^') ++data; // skip opening functional marker
	if (*data == '"') ++data; // skip opening quote
	size_t len = strlen(data);
	if (len && data[len - 1] == '"') data[--len] = 0;
	data = SkipWhitespace(data);
	char close = 0;
	char* closer = NULL;

	// if safe, locate proper end of OOB data we assume all [] are balanced except for final OOB which has the extra ]
	int bracket = 1; // for the initial one  - match off {} and [] and stop immediately after
	if (safeJsonParse)
	{
		safeJsonParse = false;
		char* at = data - 1;
		bool quote = false;
		while (*++at)
		{
			if (*at == '\\')
			{
				++at;
				continue; // escaped
			}
			if (quote)
			{
				if (*at == '"' && at[-1] != '\\') quote = false; // turn off quoted expr
				continue;
			}
			else if (*at == ':' || *at == ',' || IsWhiteSpace(*at)) continue;
			else if (*at == '{' || *at == '[') ++bracket; // an opener
			else if (*at == '}' || *at == ']') // a closer
			{
				--bracket;
				// have we ended the item
				if (bracket <= 1)
				{
					closer = at + 1;
					close = *closer;
					at[1] = 0;
					break;
				}
			}
			else if (*at == '"' && !quote)
			{
				quote = true;
				if (bracket == 1)
					return FAILRULE_BIT; // dont accept quoted string as top level
			}
		}
		len = strlen(data);
	}
	char* ignoreset = ARGUMENT(index++);;
	char* underscoreset = ARGUMENT(index++);;
	FunctionResult result = ParseJson(buffer, data, len, nofail, ignoreset,false, underscoreset);

	if (close) *closer = close;
	return result;
}

FunctionResult JSONParseFileCode(char* buffer)
{
	safeJsonParse = false;
	int index = JSONArgs();
	FILE* in = NULL;
	char* file = ARGUMENT(index);
	if (*file == '"')
	{
		++file;
		size_t len = strlen(file);
		if (file[len - 1] == '"') file[len - 1] = 0; // remove parens
	}
	in = FopenReadOnly(file);
	if (!in) return FAILRULE_BIT;
	char* start = AllocateStack(NULL, maxBufferSize);
	char* data = start;
	*data++ = ' '; // insure buffer starts with space
	bool quote = false;
	while (ReadALine(readBuffer, in) >= 0)
	{
		char* at = readBuffer - 1;
		while (*++at) // copy over w/o excessive whitespace
		{
			if (quote) { ; } // dont touch quoted data
			else if (*at == ' ' || *at == '\n' || *at == '\r' || *at == '\t') // dont copy over if can be avoided
			{
				if (at != readBuffer && *(at - 1) == ' ') continue; // already have one
			}
			*data++ = *at;
			if ((data - start) >= (int)maxBufferSize)
			{
				start = 0; // signal failure
				break;
			}
			if (*at == '"' && *(at - 1) != '\\') quote = !quote; // flip quote state
		}
	}
	FClose(in);
	FunctionResult result = FAILRULE_BIT;
	if (start) // didnt overflow bufer
	{
		*data = 0;
		result = ParseJson(buffer, start, data - start, false,NULL);
	}
	ReleaseStack(start);
	return result;
}

#define OBJECTJ 1
#define ARRAYJ 2
#define DIDONE 3
#define MAXKIND 8000

FunctionResult JSONFormatCode(char* buffer)
{
	char* original = buffer;
	int index = 0;
	char* arg = ARGUMENT(1);
	int field = 0;
	char* numberEnd = NULL;
	--arg;
	char kind[MAXKIND]; // just assume it wont overflow (json not deep, however wide it may go)
	int level = 0;
	*kind = 0;
	while (*++arg)
	{
		if (*arg == ' ' || *arg == '\n' || *arg == '\r' || *arg == '\t') *buffer++ = ' '; // special characters disappear
		else if (kind[0] == DIDONE) break; // finished already, shouldnt see more
		else if (*arg == '{') // json object open
		{
			*buffer++ = *arg;
			field = 1; // expecting element
			if (level >= MAXKIND) break;
			kind[++level] = OBJECTJ;
		}
		else if (*arg == '[') // json array open
		{
			*buffer++ = *arg;
			if (level >= MAXKIND) break;
			kind[++level] = ARRAYJ;
		}
		else if (*arg == '}' || *arg == ']') // object/array close
		{
			if (*arg == '}' && kind[level] != OBJECTJ) break;
			if (*arg == ']' && kind[level] != ARRAYJ) break;
			--level;
			if (!level) kind[level] = DIDONE;
			--index;
			*buffer++ = *arg;
			field = 0;
		}
		else if (*arg == ',')
		{
			// Unexpectedly came across the end of a property without a value
			if (field == 2) {
				*buffer++ = '"';
				*buffer++ = '"';
			}
			if (kind[level] != OBJECTJ && kind[level] != ARRAYJ) break;
			*buffer++ = *arg;
			field = 1;  // ready for fresh element - eg the keyword
		}
		else if (*arg == ':')
		{
			if (kind[level] != OBJECTJ) break;
			*buffer++ = *arg;
			field = 2;
		}
		else if (*arg == '"')
		{
			char* start = arg + 1;
			while (*++arg)
			{
				if (*arg == '"') break;
				if (*arg == '\\') {
					if (!*(arg + 1)) return FAILRULE_BIT;  // nothing left to escape
					arg++;
					if (*arg == '\"' || *arg == '/' || *arg == '\\' || *arg == 'b' || *arg == 'f' || *arg == 'r' || *arg == 'n' || *arg == 't');
					else if (*arg == 'u') {
						arg++;
						// must have 4 hex digits
						for (int i = 0; i < 4 && *arg; i++) {
							char c = GetLowercaseData(*arg++);
							if (!IsAlphaUTF8OrDigit(c) || c > 'f') return FAILRULE_BIT;  // not a hex character
						}
						arg--;
					}
					else return FAILRULE_BIT;  // not a valid escaped character
				}
			}
			if (!*arg) return FAILRULE_BIT;
			*arg = 0; // remove closing quote

			*buffer++ = '"';
			AddEscapes(buffer, start, true, currentOutputLimit - (buffer - currentOutputBase));
			buffer += strlen(buffer);
			*buffer++ = '"'; // add back trailing quote
			if (level == 0) kind[0] = DIDONE;
			if (field == 2) field = 0;
		}
		else if ((numberEnd = IsJsonNumber(arg)) != NULL) // json number
		{
			strncpy(buffer, arg, numberEnd - arg);
			buffer += numberEnd - arg;
			arg = numberEnd - 1;
			if (level == 0) kind[0] = DIDONE;
			if (field == 2) field = 0;
		}
		else // literal or simple field name nonquoted
		{
			int fieldType = field;
			char word[MAX_WORD_SIZE];
			char* at = word;
			*at++ = *arg++;
			while (*arg && *arg != ',' && *arg != '}' && *arg != ']' && *arg != ':') *at++ = *arg++;
			--arg;
			*at = 0;
			TrimSpaces(word);

			if (!strcmp(word, (char*)"null") || !strcmp(word, (char*)"false") || !strcmp(word, (char*)"true")) fieldType = 0; // simple literal
			if (fieldType == 1 || fieldType == 2) *buffer++ = '"';	// field name quote
			if (fieldType == 1 || fieldType == 2) AddEscapes(buffer, word, true, currentOutputLimit - (buffer - currentOutputBase));
			else strcpy(buffer, word);
			buffer += strlen(buffer);
			if (fieldType == 1 || fieldType == 2) *buffer++ = '"';	// field name closing quote
			if (level == 0) kind[0] = DIDONE;
			if (field == 2) field = 0;
		}
	}
	if (*arg) // didnt finish, must have been faulty format
	{
		*original = 0;
		return FAILRULE_BIT;
	}
	*buffer = 0;
	return NOPROBLEM_BIT;
}

MEANING jsonValue(char* value, unsigned int& flags, bool stripQuotes)
{
    // Don't need to modify the incoming value as this function may be called again with the same pointer (when need to update the flags)
	char* val = AllocateBuffer();
    strcpy(val, value);

	bool number = IsDigitWord(value, AMERICAN_NUMBERS, false, true); // to be a primitive number then must strictly be numeric
	if (*val == '+' || *val == '#' || (*val == '0' && strlen(val) > 1 && IsInteger(val, false, AMERICAN_NUMBERS))) number = false; // JSON does not allow + sign or leading zero in a multi-digit integer
	if (number) // cs considers currency as numbers, json doesnt
	{
		char* digits;
		if (GetCurrency((unsigned char*)val, digits)) number = false;
	}

	if (number) flags |= JSON_PRIMITIVE_VALUE;
	else if (!*value || !strcmp(value, (char*)"null") )
    {
        FreeBuffer();
        return 0; // to delete
    }
	else if (!strcmp(value, (char*)"json_null") || !strcmp(value, (char*)"json-null"))
	{
		flags |= JSON_PRIMITIVE_VALUE;
		val = "null";
	}
	else if (!strcmp(value, (char*)"true") || !strcmp(value, (char*)"false"))  flags |= JSON_PRIMITIVE_VALUE;
	else if (IsValidJSONName(value, 'o')) flags |= JSON_OBJECT_VALUE;
	else if (IsValidJSONName(value, 'a'))  flags |= JSON_ARRAY_VALUE;
	else 
	{
		flags |= JSON_STRING_VALUE; 
		if (*value == '"' && stripQuotes) 
		{
			// strip off quotes for CS, replace later in jsonwrite for output
			// special characters will be escaped later on serialization
			size_t len = strlen(value);
			if (len > 1 && value[len - 1] == '"') // ends in quote as well
			{
				// but make sure there are no internal quotes as well
				char* at = value;
				while (*++at)
				{
					at = strchr(at , '"'); // where is next one
					if (*(at-1) == '\\') continue; // ignore escaped one
				    else if (at == (value + len - 1))
					{
						val[--len] = 0;
						++val;
					}
					break; // we are done
				}
			}
		}
	}

    MEANING w = MakeMeaning(StoreWord(val, AS_IS)); // adjusted value
    FreeBuffer();
    return w;
}

static void  DoJSONAssign(WORDP base,bool changedBase,WORDP leftside, WORDP keyname, char* value, bool stripQuotes, char* fullpath, unsigned int flags, bool checkedArgs = false)
{
	//	Literal use of null clears a value - $x.field = null
	//	Use of a value which is null clears variable $_x.obj = $xx
	//	Use of "null" is the string null  $x.field = "null"
	//	use of "" is the empty string in JSON $x.field = ^ ""
	//	$x.field = "" means empty string
	// Use of "json_null" means primitive json null
	uint64 oldbase = (uint64)base->w.userValue;

    if (!flags)
    {
        if (IsTransientJson(leftside->word)) flags |= FACTTRANSIENT; // like jo-t34
        else if (IsBootJson(leftside->word)) flags |= FACTBOOT; // like jo-b34
    }
    flags |= (leftside->word[1] == 'a') ? JSON_ARRAY_FACT : JSON_OBJECT_FACT;

	MEANING object = MakeMeaning(leftside);
	MEANING key = MakeMeaning(keyname);
	MEANING valx = jsonValue(value, flags, stripQuotes);// not deleting using json literal   ^"" or "" would be the literal null in json

    bool dup = AllowDuplicates(leftside->word, checkedArgs);

	if (!keyname) // [] array, need to assign to next index in sequence
	{
		char loc[100];
		FACT* F = GetSubjectNondeadHead(leftside);
		DoJSONArrayInsert(!dup, leftside, valx, flags, loc);
		if (F != GetSubjectNondeadHead(leftside)) changedBase = true;
	}

    // now at final resting place (array or object), do the assign/erase behavior
	FACT* oldfact = NULL;
	FACT* F = GetSubjectNondeadHead(leftside);

    if (leftside->word[1] == 'o' && valx && dup)
		F = NULL; // allow multiple values - if not allowing multiple values, remove all
	else if (leftside->word[1] == 'a' && valx && dup)
		F = NULL; // allow multiple values - if not allowing multiple values, remove all
	while (F)	// already there, delete it if not referenced elsewhere
	{
		FACT* H = GetSubjectNondeadNext(F);
		if (F->verb == key && (!dup || !valx)) // have instance of this key already (either array OR object)
		{
			if (F->object == valx)
			{
					return; // already have fact here, do nothing
			}
			if (valx && !(F->flags & (JSON_OBJECT_VALUE | JSON_ARRAY_VALUE))) // not clearing
			{
				oldfact = F;
				break;	// not going to kill, going to substitute non-json value
			}

			FACT* G = GetObjectNondeadHead(Meaning2Word(F->object));
			bool jsonkill = false;
			if (safeJsonParse) jsonkill = false; // dont destroy (assumed stored on global var)
			else if (!G) jsonkill = true; // should never be true, since this fact counts
			else if (!GetObjectNondeadNext(G)) jsonkill = true;
			if (trace & TRACE_VARIABLESET)
			{
				char* recurse = (jsonkill) ? (char*)"recurse" : (char*)"once";
				Log(USERLOG, " JsonVar kill: %s %s ", fullpath, recurse);
				TraceFact(F, true);
			}
			KillFact(F, jsonkill); // wont do it in boot or earlier
			changedBase = true;
			if (!dup) break;
		}
		F = H;
	}

	// add new fact
	if (key && valx) // null assign gives 0 meaning valx
	{
		if (!F || F->object != valx)
		{
			// do simple substition if not a json entity. otherwise would leave freestanding potentially unreferenced json
			if (F && oldfact && !(F->flags & (JSON_OBJECT_VALUE | JSON_ARRAY_VALUE)))
			{
				WORDP oldObject = Meaning2Word(F->object);
				WORDP newObject = Meaning2Word(valx);
				if (trace & TRACE_JSON)
				{
					Log(USERLOG, "Revising (%s %s %s) to %s ",
						Meaning2Word(F->subject)->word,
						Meaning2Word(F->verb)->word,
						oldObject->word, newObject->word);
				}
				flags = F->flags & (-1 ^ FACTBOOT);
				flags &= -1 ^ (JSON_PRIMITIVE_VALUE | JSON_STRING_VALUE | JSON_OBJECT_VALUE | JSON_ARRAY_VALUE);
				valx = jsonValue(value, flags);
				newObject = Meaning2Word(valx);
				FACT* X = DeleteFromList(GetObjectHead(oldObject), F, GetObjectNext, SetObjectNext);  // dont use nondead
				SetObjectHead(oldObject, X);

				SetObjectHead(newObject, AddToList(GetObjectHead(newObject), F, GetObjectNext, SetObjectNext));
				F->object = valx;
				F->flags = flags;	// revised for possible new object type
				ModBaseFact(F);
			}
			else CreateFact(object, key, valx, flags);// not deleting using json literal   ^"" or "" would be the literal null in json
			changedBase = true;
		}
	}

	if (changedBase && (csapicall == TEST_PATTERN || csapicall == TEST_OUTPUT) && base->word[1] != '$' && base->word[1] != '_') // track we changed this
	{
		variableChangedThreadlist = AllocateHeapval(HV1_WORDP|HV2_INT,variableChangedThreadlist,(uint64)base, oldbase);
	}

	jsonNoArrayduplicate = false;
	jsonObjectDuplicate = false;
	currentFact = NULL;	 // used up by putting into json
}

FunctionResult JSONObjectInsertCode(char* buffer) //  objectname objectkey objectvalue  
{
	int index = JSONArgs();
	char* objectname = ARGUMENT(index++);
	if (!IsValidJSONName(objectname, 'o')) return FAILRULE_BIT;
	if (IsBootJson(objectname)) jsonPermanent = FACTBOOT;
	unsigned int flags = JSON_OBJECT_FACT | jsonPermanent | jsonCreateFlags;
	WORDP D = FindWord(objectname);
	if (!D) return FAILRULE_BIT;

	char* keyname = ARGUMENT(index++);
	if (!*keyname) return FAILRULE_BIT;

	if (*keyname == '"')  // remove dq on keyname
	{
		size_t len = strlen(keyname);
		if (keyname[len - 1] == '"') keyname[--len] = 0;
		++keyname;
	}
	
	// adding escape sequences to keyname
	char* limit;
	char* objectKeyname = InfiniteStack(limit, "SafeLine");
	AddEscapes(objectKeyname, keyname, true, MAX_BUFFER_SIZE, false);
	ReleaseInfiniteStack();
	strcpy(keyname, objectKeyname);
	*objectKeyname = 0;

	WORDP keyvalue = StoreWord(keyname, AS_IS); // new key - can be ANYTHING
	char* val = ARGUMENT(index);

	char fullpath[100];
	DoJSONAssign(D, false, D, keyvalue, val, true, fullpath, flags, true);
	return NOPROBLEM_BIT;
}

FunctionResult JSONVariableAssign(char* word, char* value, bool stripQuotes)
{
	if (*word != USERVAR_PREFIX) return FAILRULE_BIT; // must be user variable reference at start
	char basicname[MAX_WORD_SIZE] = "";
	strcpy(basicname, word);
	char fullpath[MAX_WORD_SIZE] = "";
	*fullpath = 0;
	bool changedBase = false;
	
	// locate 1st piecef
	char* separator = strchr(word + 1, '.');
	char* openbracket = strchr(word + 1, '[');
	if (separator && openbracket && openbracket < separator) separator = openbracket;
	else if (!separator && openbracket) separator = openbracket;
	if (!separator) return FAILRULE_BIT;

	char c = *separator;
	*separator = 0;

	// now find the root
	WORDP base = FindWord(word);
	char* val = GetUserVariable(word, false); // gets the initial variable value
	if (*val == '$') // auto indirect
	{
		base = FindWord(val);
	}

	*separator = c;
	WORDP leftside = FindWord(val); // actual JSON structure  name
	if (!leftside)
	{
		if (trace & TRACE_VARIABLESET) Log(USERLOG, "AssignFail leftside: %s\r\n", word);
		return FAILRULE_BIT;	// doesnt exist?
	}
	if (c == '.' && !IsValidJSONName(val, 'o'))
	{
		if (trace & TRACE_VARIABLESET) Log(USERLOG, "AssignFail JSONObject: %s->%s\r\n", word, val);
		return FAILRULE_BIT;	// not a json object
	}
	else if (c == '[' && !IsValidJSONName(val, 'a'))
	{
		if (trace & TRACE_VARIABLESET) Log(USERLOG, "AssignFail JSONArray: %s->%s\r\n", word, val);
		return FAILRULE_BIT;	// not a json array
	}

	bool bootfact = (IsBootJson(val)); // ja-b or jo-b

	// leftside is the left side of a . or [] operator
	if (trace & TRACE_JSON) strcpy(fullpath, val);

#ifndef DISCARDTESTING
	if (debugVar) (*debugVar)(basicname, value);
#endif

	//now we look at key-- $x.key or $x[] or $x[n]
	WORDP keyname = NULL;
	while (c)
	{
		// is there a followon after this or is it final
		char* followOnSeparator = strchr(separator + 1, '.');
		char* bracket1 = strchr(separator + 1, '[');
		if (followOnSeparator && bracket1 && bracket1 < followOnSeparator) followOnSeparator = bracket1;
		else if (!followOnSeparator && bracket1) followOnSeparator = bracket1;

		if (followOnSeparator) // the current level is not the end, there will be more
		{
			if (*(followOnSeparator - 1) == ']') --followOnSeparator;	// true end of key token if we have $x[5] [$t]
			c = *followOnSeparator;
			*followOnSeparator = 0; // end of current key name (the array index before)
		}

		// what is the object key or array name [index]?
		char keyx[MAX_WORD_SIZE];
		strcpy(keyx, separator + 1);
		if (*separator == '[')
		{
			if (*keyx == ']') {} // [] empty array
			else if (strchr(keyx, ']')) *strchr(keyx, ']') = 0; // terminate a given index.
		}
		if (*keyx == '$') // indirection key user variable
		{
			char* answer = GetUserVariable(keyx, false);
			strcpy(keyx, answer);
			if (!*keyx)  return FAILRULE_BIT;
		}
		else if ((*keyx == '_' && IsDigit(keyx[1])) || (*keyx == '\'' && keyx[1] == '_' && IsDigit(keyx[2]))) // indirection key match variable
		{
			int index = (*keyx == '_') ? atoi(keyx + 1) : atoi(keyx + 2);
			if (*keyx == '_') strcpy(keyx, wildcardCanonicalText[index]);
			else strcpy(keyx, wildcardOriginalText[index]);
			if (!*keyx)
				return FAILRULE_BIT;
		}
		else if (*keyx == '\\' && keyx[1] == '$') memmove(keyx, keyx + 1, strlen(keyx));

		// now we have retrieved the key/index
		if (*keyx == ']') keyname = NULL;  // [] use
		else
		{
			keyname = StoreWord(keyx, AS_IS);  // key indexing - case sensitive
		}

		// show the path for tracing
		if (trace & TRACE_JSON)
		{
			if (*separator == '.') strcat(fullpath, ".");
			else strcat(fullpath, "[");
			strcat(fullpath, keyx);
			if (*separator == '[') strcat(fullpath, "]");
		}

		if (followOnSeparator) // goto next piece
		{
			*followOnSeparator = c; // restore normal values
			if (*followOnSeparator == ']') ++followOnSeparator; // end of indexed array ref

			// we must find this and keep going if there is something later
			FACT* F = GetSubjectNondeadHead(leftside);
			WORDP priorLeftside = leftside;
			FACT* arrayfact = NULL;
			int lastindex = -1;
			while (F)
			{
				if (F->verb == MakeMeaning(keyname)) break; // here is the object or array key
				if (F->flags & JSON_ARRAY_FACT)
				{
					arrayfact = F; // array has at least some members
					int index = atoi(Meaning2Word(F->object)->word);
					if (index > lastindex) lastindex = index;
				}
				F = GetSubjectNondeadNext(F);
			}
			if (!F) // key is not found (we need to auto create something - maybe just the fact )
			{ // NOTE we cannot create an array index out of order. We can create the next index in line however.
				// if we are not replacing a value, dont save a null value
				if (!*value) return NOPROBLEM_BIT; // we dont need to report NEW null values, only replacement and only to api calls
				
				char* arg1;
				if (*separator == '[' && keyname && atoi(keyx) != (lastindex + 1))
					return FAILRULE_BIT; // attempt to create out of order index to array
				if (bootfact) arg1 = (char*)"boot";
				else if (IsTransientJson(priorLeftside->word)) arg1 = (char*)"transient";
				else if (IsBootJson(priorLeftside->word)) arg1 = (char*)"boot";
				else arg1 = (char*)"permanent";
				char loc[100] = "";

				// create appropriate structure
				if (c == '.')
				{
					InternalCall("^JSONCreateCode", JSONCreateCode, arg1, (char*)"object", NULL, loc); // followon is an object, not an array
					changedBase = true;
				}
				else if (!arrayfact)
				{
					InternalCall("^JSONCreateCode", JSONCreateCode, arg1, (char*)"array", NULL, loc); // we have no array facts yet
					changedBase = true;
				}

				else
				{
					if (!*separator) break; // its final, so drop down
					return FAILRULE_BIT; 	// we know this array, we just dont have this element
				}
				WORDP rightside = FindWord(loc); // newly created structure

				unsigned int flags = (priorLeftside->word[1] == 'o') ? JSON_OBJECT_FACT : JSON_ARRAY_FACT;
				if (bootfact) flags |= FACTBOOT;
				else if (IsTransientJson(loc)) flags |= FACTTRANSIENT;
				else if (IsBootJson(loc)) flags |= FACTBOOT;
				MEANING valx = jsonValue(rightside->word, flags);
				F = CreateFact(MakeMeaning(priorLeftside), MakeMeaning(keyname), valx, flags); // now have array to use
				leftside = rightside; // this will become the new path to continue
				separator = followOnSeparator;
				if (*separator == ']') ++separator; // what happens AFTER the [] part?
				else if (*separator == '[' && separator[1] == ']') separator += 2; // after the array fillin
				if (!*separator && leftside->word[1] == 'a') // creating default array at end. we will assign incoming value as 0th element
				{
					keyname = StoreWord("0", AS_IS);  // initial array index
				}
			}
			else
			{
				separator = followOnSeparator;
				if (*separator == ']') ++separator; // what happens AFTER the [] part?
				else if (*separator == '[' && separator[1] == ']') separator += 2; // after the array fillin
				leftside = Meaning2Word(F->object); // already exists
				if (!*separator && followOnSeparator[0] == '[' && followOnSeparator[1] == ']' && leftside->word[1] == 'a') // figure out index we need
				{
					F = GetSubjectNondeadHead(leftside);
					while (F)
					{
						// already have a value of this in array which must be unique
						if (jsonDefaults & JSON_ARRAY_UNIQUE && !strcmp(Meaning2Word(F->object)->word, value))
						{
							return NOPROBLEM_BIT;
						}
						char* ind = Meaning2Word(F->verb)->word;
						int index = atoi(ind);
						if (index > lastindex) lastindex = index;
						F = GetSubjectNondeadNext(F);
					}
					lastindex++;
					char tmp[100];
					sprintf(tmp, "%d", lastindex);
					keyname = StoreWord(tmp, AS_IS);  // initial array index
				}
			}
			c = *separator;
		}
		else c = 0;
	}

	// now at final resting place, do assignment there
	DoJSONAssign(base, changedBase, leftside, keyname, value, stripQuotes, fullpath, 0);

	if (trace & TRACE_VARIABLESET) Log(USERLOG, " JsonVar: %s(%s) -> `%s` ", basicname,fullpath, value);
	if (base->internalBits & MACRO_TRACE)
	{
		char pattern[MAX_WORD_SIZE];
		char label[MAX_LABEL_SIZE];
		GetPattern(currentRule, label, pattern, true, 100);  // go to output
		Log(ECHOUSERLOG, "%s(%s) -> `%s` at %s.%d.%d %s %s\r\n", basicname,base->word, value, GetTopicName(currentTopicID), TOPLEVELID(currentRuleID), REJOINDERID(currentRuleID), label, pattern);
	}

	return NOPROBLEM_BIT;
}

FunctionResult JSONArraySizeCode(char* buffer) // like ^length()
{
	return LengthCode(buffer); // a rename only
}

FunctionResult JSONArrayDeleteCode(char* buffer) //  array, index	
{
	char arrayname[MAX_WORD_SIZE];
	int index = 0;
	MEANING object = 0;
	char* arg1 = ARGUMENT(1);
	bool useIndex = true;
	MakeLowerCase(arg1);

	// check mode of use 
	bool deleteAll = false;
	bool safe = false;
	if (strstr(arg1, "index")) index = atoi(ARGUMENT(3));
	else if (strstr(arg1, "value"))
	{
		char* match = ARGUMENT(3);
		WORDP D = FindWord(match);
		if (!D) return FAILRULE_BIT;
		object = MakeMeaning(D);
		useIndex = false;
	}
	else return FAILRULE_BIT;
	if (strstr(arg1, "all")) deleteAll = true; // default is one but can do ALL
	if (strstr(arg1, "safe")) safe = true; // default is delete

	// get array and prove it legal
	strcpy(arrayname, ARGUMENT(2));
	if (!IsValidJSONName(arrayname, 'a')) return FAILRULE_BIT;
	WORDP O = FindWord(arrayname);
	if (!O) return FAILRULE_BIT;
	FACT* F = GetSubjectNondeadHead(O);
	bool once = false;
DOALL:
	// find the fact we want (we only delete 1 fact per call) if you have dups say all
	F = GetSubjectNondeadHead(O);
	while (F)
	{
		if (useIndex)
		{
			int val = atoi(Meaning2Word(F->verb)->word);
			if (val == index) break;	// found it
		}
		else if (object == F->object) break;
		F = GetSubjectNondeadNext(F);
	}
	if (!F)
	{
		if (once) return NOPROBLEM_BIT;
		return FAILRULE_BIT;		// not findable.
	}
	once = true;
	WORDP objectx = Meaning2Word(F->object);
	FACT* G = GetObjectNondeadHead(objectx);
	bool jsonkill = false;
	if (safe) jsonkill = false;
	else if (!G) jsonkill = true; // should never be true, since this fact counts
	else if (!GetObjectNondeadNext(G)) jsonkill = true;
	KillFact(F, jsonkill, true);		// delete it, not recursive json structure, just array element
	if (deleteAll) goto DOALL; // keep trying

	return NOPROBLEM_BIT;
}

FunctionResult DoJSONArrayInsert(bool nodup, WORDP array, MEANING value, int flags, char* buffer) //  objectfact objectvalue  BEFORE/AFTER 
{
	// get the field values
	char arrayIndex[20];

	// cannot insert a 0 meaning value
	if (!value) return NOPROBLEM_BIT;

	// how many existing elements
	FACT* F = GetSubjectNondeadHead(array);
	int count = 0;
	FACT* lastF = F;
	while (F)
	{
		lastF = F;
		if (nodup && F->object == value)  return NOPROBLEM_BIT;	// already there UNIQUE flag given else defaults DUPLICATE
		++count;
		F = GetSubjectNondeadNext(F);
	}

	// you may NOT add permanent values after transient values lest earlier elements disappear out of order
	if (lastF && lastF->flags & FACTTRANSIENT)  flags |= FACTTRANSIENT;

	sprintf(arrayIndex, (char*)"%d", count); // add at end
	WORDP Idex = StoreWord(arrayIndex, AS_IS);

	// create fact
	CreateFact(MakeMeaning(array), MakeMeaning(Idex), value, flags);
	currentFact = NULL;	 // used up by putting into json
	return NOPROBLEM_BIT;
}

FunctionResult JSONArrayInsertCode(char* buffer) //  objectfact objectvalue  BEFORE/AFTER 
{
	int index = JSONArgs();
	char* arrayname = ARGUMENT(index++);
	if (!IsValidJSONName(arrayname, 'a')) return FAILRULE_BIT;
	WORDP O = FindWord(arrayname);
	if (!O) return FAILRULE_BIT;
	char* val = ARGUMENT(index);
	jsonPermanent = (IsTransientJson(arrayname)) ? FACTTRANSIENT : 0; // ja-t1
	if (IsValidJSONName(val) && IsTransientJson(val)) jsonPermanent = FACTTRANSIENT;
	if (IsBootJson(arrayname)) jsonPermanent = FACTBOOT; // ja-b1
	unsigned int flags = JSON_ARRAY_FACT | jsonPermanent | jsonCreateFlags;
	MEANING value = jsonValue(val, flags); // sets flags and may revise the val

	bool dup = AllowDuplicates(O->word, true);

	FACT* F = GetSubjectNondeadHead(O);
	FunctionResult result = DoJSONArrayInsert(!dup, O, value, flags, buffer);
	if (F != GetSubjectNondeadHead(O)) // we added element
	{
		if ((csapicall == TEST_PATTERN || csapicall == TEST_OUTPUT) && O->word[1] != '$' && O->word[1] != '_') // track we changed this
		{
			variableChangedThreadlist = AllocateHeapval(HV1_WORDP | HV2_STRING, variableChangedThreadlist, (uint64)O, (uint64)O->w.userValue);
		}
	}
	return result;
}

FunctionResult JSONTextCode(char* buffer)
{
	char* id = ARGUMENT(1);
	FACT* F = Index2Fact(atoi(id));
	if (!F) return FAILRULE_BIT;
	if (!(F->flags & (JSON_ARRAY_FACT | JSON_OBJECT_FACT))) return FAILRULE_BIT;
	char* name = Meaning2Word(F->object)->word;
	if (F->flags & JSON_STRING_VALUE) sprintf(buffer, "\"%s\"", name);
	else strcpy(buffer, name);
	return NOPROBLEM_BIT;
}

FunctionResult JSONCopyCode(char* buffer)
{
	int index = JSONArgs();
	char* arg = ARGUMENT(index++);
	WORDP D = FindWord(arg);
	if (!D || !IsValidJSONName(D->word))
	{
		strcpy(buffer, arg);
		return NOPROBLEM_BIT;
	}
	MEANING M = jcopy(MakeMeaning(D)); // will not correctly copy empty array or object (but dont really care)
	currentFact = NULL; // used up in json
	D = Meaning2Word(M);
	strcpy(buffer, D->word);
	return NOPROBLEM_BIT;
}

FunctionResult JSONCreateCode(char* buffer)
{
	int index = JSONArgs(); // not needed but allowed
	char* arg = ARGUMENT(index);
	MEANING M;
	if (!stricmp(arg, (char*)"array")) M = GetUniqueJsonComposite((char*)"ja-");
	else if (!stricmp(arg, (char*)"object"))  M = GetUniqueJsonComposite((char*)"jo-");
	else return FAILRULE_BIT;
	WORDP D = Meaning2Word(M);
	sprintf(buffer, "%s", D->word);
	return NOPROBLEM_BIT;
}

static void FixArrayFact(FACT * F, int index)
{
	char val[MAX_WORD_SIZE];
	sprintf(val, "%d", index);
	WORDP newverb = StoreWord(val);
	UnweaveFactVerb(F);
	SetVerbHead(newverb, AddToList(GetVerbHead(newverb), F, GetVerbNext, SetVerbNext));  // dont use nondead
	F->verb = MakeMeaning(newverb);
	ModBaseFact(F);
	
}

void JsonRenumber(FACT * G) // given array fact dying, renumber around it
{
	WORDP D = Meaning2Word(G->subject);
	int index = atoi(Meaning2Word(G->verb)->word); // this one is dying, above it must go?
	size_t textsize = 0;
	char* limit;
	FACT** stack = (FACT**)InfiniteStack(limit, "JsonRenumber");
	int indexsize = orderJsonArrayMembers(D, stack, textsize);
	CompleteBindStack64(indexsize, (char*)stack);
	int downindex = 0;
	for (int i = 0; i < index && i < indexsize; ++i)
	{
		FACT* F = stack[i];
		if (!F) ++downindex; // we are missing a fact here - already deleted?
		else if (downindex) FixArrayFact(F, atoi(Meaning2Word(F->verb)->word) - downindex); // this living earlier fact needs renumbering
	}
	++downindex; // for the element we deleted
	for (int i = index + 1; i < indexsize; ++i) // renumber these downwards for sure
	{
		FACT* F = stack[i];
		if (!F) ++downindex;
		else FixArrayFact(F, atoi(Meaning2Word(F->verb)->word) - downindex);
	}
	ReleaseStack((char*)stack);
}

FunctionResult JSONDeleteCode(char* buffer)
{// json reference should never be able to mix languages or bots in its facts
	char* arg = ARGUMENT(1); // a json object or array
	WORDP D = FindWord(arg);
	if (!D)  return NOPROBLEM_BIT;

	FACT* F = GetSubjectHead(D); // if we have someone below us, not json, thats a problem
	if (F && !(F->flags & JSON_FLAGS)) return FAILRULE_BIT;
	if (F) jkillfact(D); // kill everything we own

	// remove upper link to us if someone above us uses us JSON wise
	F = GetObjectHead(D); // who are we the object of, can only be one
	while (F)
	{
		if (F->flags & (JSON_ARRAY_FACT | JSON_OBJECT_FACT)) // we are object of array or json object
		{
			KillFact(F);
			break;
		}
		F = GetObjectNext(F);
	}

	return NOPROBLEM_BIT;
}

static char* SetArgument(char* ptr, size_t len)
{
	char* data = AllocateStack(NULL, len + 3); // include eos marker
	*data++ = ENDUNIT;
	*data++ = ENDUNIT;
	strcpy(data, ptr);
	return data;
}

FunctionResult JSONReadCSVCode(char* buffer)
{
	int index = JSONArgs(); // not needed but allowed
	bool commadelimit = (!stricmp(ARGUMENT(index), "comma"));
	bool tabdelimit = (!stricmp(ARGUMENT(index), "tab"));
	bool wholeLine = (!stricmp(ARGUMENT(index), "line"));
	bool pause = false;
	if (!tabdelimit && !wholeLine) return FAILRULE_BIT;
	++index;
	char* data;
	char* name = ARGUMENT(index++);
	FILE* in = FopenReadOnly(name);
	if (!in) return FAILRULE_BIT;
	char var[100];

	char fn[MAX_WORD_SIZE];
	FunctionResult fnresult;
	char* fnname = fn;
	char* ptr = ReadFunctionCommandArg(ARGUMENT(index), fnname, fnresult, true);
	if (fnresult != NOPROBLEM_BIT) return fnresult;
	if (strlen(fnname) == 0) fnname = 0;
	if (wholeLine && (!fnname || !*fnname)) return FAILRULE_BIT; // cannot build json, must call

	unsigned int arrayflags = JSON_ARRAY_FACT | jsonPermanent | jsonCreateFlags | JSON_OBJECT_VALUE;
	unsigned int flags = JSON_OBJECT_FACT | jsonPermanent | jsonCreateFlags;
	MEANING arrayName = 0;
	int arrayIndex = 0;
	if (!fnname) //  building json structure, set header that we will return
	{
		arrayName = GetUniqueJsonComposite((char*)"ja-");
		WORDP D = Meaning2Word(arrayName);
		sprintf(buffer, "%s", D->word);
	}
	char call[400];
	FunctionResult result = NOPROBLEM_BIT;
	while (1)
	{
		char* allocation = AllocateStack(NULL, 4); // reserve a freeing id. variables passed in can be freed on return of call
		int field = 0;
		sprintf(call, "( ");
		if (!wholeLine)
		{
			if (ReadALine(readBuffer, in, maxBufferSize, false, false) < 0) break;
			char* ptrx = readBuffer;
			if ((unsigned char)ptrx[0] == 0xEF && (unsigned char)ptrx[1] == 0xBB && (unsigned char)ptrx[2] == 0xBF) ptrx += 3;// UTF8 BOM

			MEANING object = 0;
			if (!fnname) // not passing to a routine, building json structure
			{
				object = GetUniqueJsonComposite((char*)"jo-");
				WORDP E = Meaning2Word(object);
				CreateFact(arrayName, MakeMeaning(StoreIntWord(arrayIndex++)), object, arrayflags); // WATCH OUT FOR EMPTY SET!!!
			}
			strcpy(ptrx + strlen(ptrx), "\t"); // trailing tab forces recog of all fields
			char* tab;
			while ((tab = strchr(ptrx, '\t'))) // for each field ending in a tab
			{
				int len = tab - ptrx;
				char* limit;
				data = InfiniteStack(limit, "JSONReadCSVFileCode");
				if (fnname)
				{
					*data = ENDUNIT;
					data[1] = ENDUNIT;
					data += 2;
				}
				strncpy(data, ptrx, len);
				data[len] = 0;
				CompleteBindStack();

				if (!fnname) // not passing to a routine, building json structure
				{
					char indexval[400];
					sprintf(indexval, "%d", field++);
					MEANING key = MakeMeaning(StoreWord(indexval));
					if (*data)
					{
						MEANING valx = jsonValue(data, flags);
						CreateFact(object, key, valx, flags);
					}
					ReleaseStack(data);
				}
				else
				{
					sprintf(var, "$__%d", field++); // internal variable cannot be entered in script
					WORDP V = StoreWord(var);
					V->w.userValue = SetArgument(ptrx, len); // has the 2 hidden markers of preevaled
					strcat(call, var);
					strcat(call, " ");
				}

				ptrx = tab + 1;
			}
		}
		else // get whole line untouched by readaline
		{
			*readBuffer = 0;
			fgets(readBuffer, maxBufferSize, in);
			if (!*readBuffer) break;
			char* ptrx = readBuffer;
			if ((unsigned char)ptrx[0] == 0xEF && (unsigned char)ptrx[1] == 0xBB && (unsigned char)ptrx[2] == 0xBF) ptrx += 3;// UTF8 BOM
			ptrx = SkipWhitespace(ptrx);
			if (*ptrx == '#') continue;
			if (!strnicmp(ptrx, ":trace ", 7))
			{
				if (strstr(ptrx, "all")) trace = (unsigned int)-1;
				else trace = 0;
				echo = true;
				continue;
			}
			if (!strnicmp(ptrx, ":quit", 5) || !strnicmp(ptrx, ":exit", 5))
			{
				break;
			}
			if (!strnicmp(ptrx, ":pause", 6))
			{
				pause = true;
				continue;
			}
			if (!strnicmp(ptrx, ":resume", 7))
			{
				pause = false;
				continue;
			}
			if (pause) continue;

			sprintf(var, "$__%d", field++); // internal variable cannot be entered in script
			WORDP V = StoreWord(var);
			size_t len = strlen(ptrx);
			if (ptrx[len - 1] == '\n') --len; // remove trailing line data
			if (ptrx[len - 1] == '\r') --len;
			V->w.userValue = SetArgument(ptrx, len);
			strcat(call, var);
			strcat(call, " ");
		}

		if (fnname) // invoke function
		{
			strcat(call, ")");
			DoFunction(fnname, call, buffer, result);
			if (result != NOPROBLEM_BIT) break;
		}
		ReleaseStack(allocation); // deallocate all transient var data on call
		for (int i = 0; i < field; ++i) // remove bad pointers
		{
			sprintf(var, "$__%d", i); // internal variable cannot be entered in script
			WORDP V = FindWord(var);
			if (V) V->w.userValue = NULL;
		}
	}

	FClose(in);
	currentFact = NULL;
	return result;
}
