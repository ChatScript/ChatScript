#include "common.h"
//------------------------
// ALWAYS AVAILABLE
//------------------------
static unsigned int undefinedCallThreadList = 0;
static bool nospellcheck = false;
static int complexity = 0;
static int priorLine = 0;
bool autoset = false;
static char macroName[MAX_WORD_SIZE];
static uint64 macroid;
char* dataBase = NULL;
static char* dataChunk = NULL;
static char* outputStart = NULL;
static char* lineStart = NULL;
static bool globalBotScope = false;
char* newBuffer = NULL;
static char* oldBuffer = NULL;
static char display[MAX_DISPLAY][100];
static int displayIndex = 0;
static char* incomingPtrSys = 0;			// cache AFTER token find ptr when peeking.
static char lookaheadSys[MAX_WORD_SIZE];	// cache token found when peeking
static unsigned int hasWarnings;			// number of warnings generated
unsigned int hasErrors;
uint64 grade = 0;						// vocabulary warning
char* lastDeprecation = 0;
bool compiling = false;			// script compiler in progress
bool patternContext = false;	// current compiling a pattern
unsigned int buildId; // current build
static int callingSystem = 0;
static bool chunking = false;
static unsigned int substitutes;
static unsigned int cases;
static unsigned int badword;
static unsigned int functionCall;
static bool isDescribe = false;

#define MAX_WARNINGS 200
static char warnings[MAX_WARNINGS][MAX_WORD_SIZE];
static unsigned int warnIndex = 0;
static char baseName[SMALL_WORD_SIZE];

#define MAX_ERRORS 200
static char errors[MAX_ERRORS][MAX_WORD_SIZE];
static unsigned int errorIndex = 0;
static char functionArguments[MAX_ARGUMENT_COUNT+1][500];
static int functionArgumentCount = 0;
char botheader[MAX_WORD_SIZE];
static bool renameInProgress = false;
static bool endtopicSeen = false; // needed when ending a plan

unsigned int buildID = 0;

static char* topicFiles[] = //   files created by a topic refresh from scratch 
{
	(char*)"describe",		//   document variables functions concepts topics etc
	(char*)"facts",			//   hold facts	
	(char*)"keywords",		//   holds topic and concepts keywords
	(char*)"macros",			//   holds macro definitions
	(char*)"map",			//   where things are defined
	(char*)"script",			//   hold topic definitions
	(char*)"plans",			//   hold plan definitions
	(char*)"patternWords",	//   things we want to detect in patterns that may not be normal words
	(char*)"dict",			//   dictionary changes	
	(char*)"private",		//   private substitutions changes	
	(char*)"canon",			//   private canonical values 	
	0
};
static void WritePatternWord(char* word);
static void WriteKey(char* word);
static FILE* mapFile = NULL;					// for IDE


void InitScriptSystem()
{
	mapFile = NULL;
	outputStart = NULL;
}

void AddWarning(char* buffer)
{
	sprintf(warnings[warnIndex++],(char*)"line %d of %s: %s",currentFileLine,currentFilename,buffer);
	if (strstr(warnings[warnIndex-1],(char*)"is not a known word")) {++badword;}
	else if (strstr(warnings[warnIndex-1],(char*)" changes ")) {++substitutes;}
	else if (strstr(warnings[warnIndex-1],(char*)"is unknown as a word")) {++badword;}
	else if (strstr(warnings[warnIndex-1],(char*)"in opposite case")){++cases;}
    else if (strstr(warnings[warnIndex - 1], (char*)"multiple spellings")) { ++cases; }
    else if (strstr(warnings[warnIndex-1],(char*)"a function call")){++functionCall;}
	if (warnIndex >= MAX_WARNINGS) --warnIndex;
}

void ScriptWarn()
{
	if (compiling)
	{
		++hasWarnings; 
		if (*currentFilename)
		{
			if (*botheader) Log(STDTRACELOG, (char*)"*** Warning- line %d of %s bot:%s : ", currentFileLine, currentFilename, botheader);
			else Log(STDTRACELOG, (char*)"*** Warning- line %d of %s: ", currentFileLine, currentFilename);
		}
		else Log(STDTRACELOG, (char*)"*** Warning-  ");
	}
}

void AddError(char* buffer)
{
    if (*botheader) sprintf(errors[errorIndex++],(char*)"*** Error- line %d of %s bot:%s : %s\r\n", currentFileLine, currentFilename, botheader,buffer);
    else sprintf(errors[errorIndex++], (char*)"*** Error- line %d of %s: %s\r\n", currentFileLine, currentFilename,buffer);
	if (errorIndex >= MAX_ERRORS) --errorIndex;
}

void ScriptError()
{
	callingSystem = 0;
	chunking = false;
	outputStart = NULL;

	renameInProgress = false;
	if (compiling)
	{
		++hasErrors; 
		patternContext = false; 
        if (*botheader) Log(STDTRACELOG, (char*)"*** Error- line %d of %s bot:%s : ", currentFileLine, currentFilename, botheader);
        else Log(STDTRACELOG, (char*)"*** Error- line %d of %s: ", currentFileLine, currentFilename);
	}
}

static void AddDisplay(char* word)
{
	MakeLowerCase(word);
	for (int i = 0; i < displayIndex; ++i)
	{
		if (!strcmp(word,display[i])) return;	// no duplicates needed
	}
	strcpy(display[displayIndex],word);
	if (++displayIndex >= MAX_DISPLAY) BADSCRIPT("Display argument limited to %d:  %s\r\n",MAX_DISPLAY,word)
}

static char* ReadDisplay(FILE* in, char* ptr)
{
	char word[SMALL_WORD_SIZE];
	ptr = ReadNextSystemToken(in,ptr,word,false);
	while (1)
	{
		ptr = ReadNextSystemToken(in,ptr,word,false);
		if (*word == ')') break;
		if (*word != USERVAR_PREFIX) 
			BADSCRIPT("Display argument must be uservar of $$ $ or $_: %s\r\n",word)
		if (strchr(word,'.'))
			BADSCRIPT("Display argument cannot be dot-selected %s\r\n",word)
		AddDisplay(word); // explicit display
	}
	return ptr;
}

static char* WriteDisplay(char* pack)
{
	*pack++ = '(';
	*pack++ = ' ';
	if (displayIndex) // show and merge in the new stuff
	{
		for (int i = 0; i < displayIndex; ++i)
		{
			strcpy(pack,display[i]);
			pack += strlen(pack);
			*pack++ = ' ';
		}
		displayIndex = 0;
	}
	*pack++ = ')';
	*pack++ = ' ';
	*pack = 0;
	return pack;
}

void EraseTopicFiles(unsigned int build,char* name)
{
	int i = -1;
	while (topicFiles[++i])
	{
		char file[SMALL_WORD_SIZE];
		sprintf(file,(char*)"%s/%s%s.txt",topic,topicFiles[i],name);
		remove(file);
		sprintf(file,(char*)"%s/BUILD%s/%s%s.txt",topic,name,topicFiles[i],name);
		remove(file);
	}
}


static char* FindComparison(char* word)
{
	if (!*word || !word[1] || !word[2]) return NULL; //   if token is short, we cannot do the below word+1 scans 
	if (*word == '.') return NULL; //  .<_3 is not a comparison
	if (*word == '\\') return NULL; // escaped is not a comparison
	if (*word == '!' && word[1] == '?' && word[2] == '$') return NULL;
	if (*word == '_' && word[1] == '?' && word[2] == '$') return NULL;
	if (*word == '?' && word[1] == '$') return NULL;
	char* at = strchr(word+1,'!'); 
	if (at && *word == '!') at = NULL;	 // ignore !!
	if (!at) at = strchr(word+1,'<');
	if (!at) at = strchr(word+1,'>');
	if (!at) 
	{
		at = strchr(word+1,'&');
		if (at && (at[1] == '_' || at[1] == ' ')) at = 0;	// ignore & as part of a name
	}
	if (!at) at = strchr(word+1,'=');
	if (!at) at = strchr(word+1,'?');  //   member of set
	if (!at)
	{
		at = strchr(word+1,'!');  //   negation
		if (at && (at[1] == '=' || at[1] == '?'));
		else at = NULL;
	}
	return at;
}

static void AddMapOutput(int line)
{
	// if we are mapping (:build) and have started output and some data storage change has happened
    if (mapFile && dataBase && lineStart != dataChunk  &&  strnicmp(macroName, "^tbl:", 5))
    {
        *dataChunk = 0;
        char src[MAX_WORD_SIZE];
        strncpy(src, lineStart, 30);
        src[30] = 0;
        fprintf(mapFile, (char*)"          line: %d %d  # %s\r\n", line, (int)(lineStart - dataBase),src); // readBuffer
    }
    lineStart = dataChunk; // used to detect new line needs tracking
}

char* ReadNextSystemToken(FILE* in,char* ptr, char* word, bool separateUnderscore, bool peek)
{ 
#ifdef INFORMATION
The outside can ask for the next real token or merely peek ahead one token. And sometimes the outside
after peeking, decides it wants to back up a real token (passing it to some other processor).

To support backing up a real token, the system must keep the current readBuffer filled with the data that
led to that token (to allow a ptr - strlen(word) backup).

To support peeking, the system may have to read a bunch of lines in to find a token. It is going to need to
track that buffer separately, so when it needs a real token which was the peek, it can both get the peek value
and be using contents of the new buffer thereafter. 

So peeks must never touch the real readBuffer. And real reads must know whether the last token was peeked 
and from which buffer it was peeked.

And, if someone wants to back up to allow the old token to be reread, they have to CANCEL any peek data, so the token
comes from the old buffer. Meanwhile the newbuffer continues to have content for when the old buffer runs out.
#endif
	int line = currentFileLine;

	//   clear peek cache
	if (!in && !ptr) // clear cache request, next get will be from main buffer (though secondary buffer may still have peek read data)
	{
		if (word) *word = 0;
		incomingPtrSys = NULL; // no longer holding a PEEK value.
		return NULL;
	}

	char* result = NULL;
	if (incomingPtrSys ) //  had a prior PEEK, now in cache. use up cached value, unless duplicate peeking
	{
		result = incomingPtrSys; // caller who is peeking will likely ignore this
		if (!peek)
		{
			currentFileLine = maxFileLine;	 // revert to highest read
			// he wants reality now...
			if (newBuffer && *newBuffer) // prior peek was from this buffer, make it real data in real buffer
			{
				strcpy(readBuffer,newBuffer);
				result = (result - newBuffer) + readBuffer; // adjust pointer to current buffer
				*newBuffer = 0;
			}
			strcpy(word,lookaheadSys);
			incomingPtrSys = 0;
		}
		else 
		{
			strcpy(word,lookaheadSys); // duplicate peek
			result = (char*)1;	// NO ONE SHOULD KEEP A PEEKed PTR
		}

		return result;
	}

	*word = 0;

	if (ptr) result = ReadSystemToken(ptr,word,separateUnderscore);
	bool newline = false;
	while (!*word) // found no token left in existing buffer - we have to juggle buffers now unless running overwrite 
	{
		if (!newline && newBuffer && *newBuffer) // use pre-read buffer per normal, it will have a token
		{
			strcpy(readBuffer,newBuffer);
			*newBuffer = 0;
			result = ReadSystemToken(readBuffer,word,separateUnderscore);
			break;
		}
		else // read new line into hypothetical buffer, not destroying old actual buffer yet
		{
			if (!in || ReadALine(newBuffer,in) < 0) return NULL; //   end of file
			if (!strnicmp(newBuffer,(char*)"#ignore",7)) // hit an ignore zone
			{
				unsigned int ignoreCount = 1;
				while (ReadALine(newBuffer,in) >= 0)
				{
					if (!strnicmp(newBuffer,(char*)"#ignore",7)) ++ignoreCount;
					else if (!strnicmp(newBuffer,(char*)"#endignore",10))
					{
						if (--ignoreCount == 0)
						{
							if (ReadALine(newBuffer,in) < 0) return NULL;	// EOF
							break;
						}
					}
				}
				if (ignoreCount) return NULL;	//EOF before finding closure
			}
			result = ReadSystemToken(newBuffer,word,separateUnderscore); // result is ptr into NEWBUFFER
			newline = true;
		}
	}

	if (peek) //   save request - newBuffer has implied newline if any
	{
		incomingPtrSys = result;  // next location in whatever buffer
		strcpy(lookaheadSys,word); // save next token peeked
		result = (char*)1;	// NO ONE SHOULD KEEP A PEEKed PTR
		currentFileLine = line; // claim old value
	}
	else if (newline && newBuffer) // live token from new buffer, adjust pointers and buffers to be fully up to date
	{
		strcpy(readBuffer,newBuffer);
		result = (result - newBuffer) + readBuffer; // ptr into current readBuffer now
		*newBuffer = 0;
	}

	return result; // ptr into READBUFFER or 1 if from peek zone
}

static void InsureAppropriateCase(char* word)
{
	char c;
	char* at = FindComparison(word);
	//   force to lower case various standard things
	//   topcs/sets/classes/user vars/ functions and function vars  are always lower case
 	if (at) //   a comparison has 2 sides
	{
		c = *at;
		*at = 0;
		InsureAppropriateCase(word);
		if (at[1] == '=' || at[1] == '?') InsureAppropriateCase(at+2); // == or >= or such
		else InsureAppropriateCase(at+1);
		*at = c;
	}
	else if (*word == '_' || *word == '\'') InsureAppropriateCase(word+1);
	else if (*word == USERVAR_PREFIX)
	{
		char* dot = strchr(word,'.');
		if (dot) *dot = 0;
		MakeLowerCase(word);	
		if (dot) *dot = '.';
	}
	else if ((*word == '^' && word[1] != '"') || *word == '~' ||  *word == SYSVAR_PREFIX || *word == '|' ) MakeLowerCase(word);	
	else if (*word == '@' && IsDigit(word[1])) MakeLowerCase(word);	//   potential factref like @2subject
}

static int GetFunctionArgument(char* arg) //   get index of argument (0-based) if it is value, else -1
{
	for (int i = 0; i < functionArgumentCount; ++i)
	{
		if (!stricmp(arg,functionArguments[i])) return i;
	}
	return -1; //   failed
}

static void FindDeprecated(char* ptr, char* value, char* message)
{
	char* comment = strstr(ptr,(char*)"# ");
	char* at = ptr;
	size_t len = strlen(value);
	while (at)
	{
		at = strstr(at,value);
		if (!at) break;
		if (*(at-1) == USERVAR_PREFIX) // $$xxx should be ignored
		{
			at += 2;
			continue;
		}
		if (comment && at > comment) return; // inside a comment
		char word[MAX_WORD_SIZE];
		ReadCompiledWord(at,word);
		if (!stricmp(value,word))
		{
			lastDeprecation = at;
			BADSCRIPT(message);
		}
		at += len;
	}
}

char* ReadSystemToken(char* ptr, char* word, bool separateUnderscore) //   how we tokenize system stuff (rules and topic system) words -remaps & to AND
{
	*word = 0;
    if (!ptr)  return 0;
	char tmp[MAX_WORD_SIZE];
    char* start = word;
    ptr = SkipWhitespace(ptr);
	FindDeprecated(ptr,(char*)"$bot",(char*)"Deprecated $bot needs to be $cs_bot");
	FindDeprecated(ptr,(char*)"$login",(char*)"Deprecated $login needs to be $cs_login");
	FindDeprecated(ptr,(char*)"$userfactlimit",(char*)"Deprecated $userfactlimit needs to be $cs_userfactlimit");
	FindDeprecated(ptr,(char*)"$crashmsg",(char*)"Deprecated $crashmsg needs to be $cs_crashmsg");
	FindDeprecated(ptr,(char*)"$token",(char*)"Deprecated $token needs to be $cs_token");
	FindDeprecated(ptr,(char*)"$response",(char*)"Deprecated $response needs to be $cs_response");
	FindDeprecated(ptr,(char*)"$randindex",(char*)"Deprecated $randindex needs to be $cs_randindex");
	FindDeprecated(ptr,(char*)"$wildcardseparator",(char*)"Deprecated $wildcardseparator needs to be $cs_wildcardseparator");
	FindDeprecated(ptr,(char*)"$abstract",(char*)"Deprecated $abstract needs to be $cs_abstract");
	FindDeprecated(ptr,(char*)"$prepass",(char*)"Deprecated $prepass needs to be $cs_prepass");
	FindDeprecated(ptr,(char*)"$control_main",(char*)"Deprecated $control_main needs to be $cs_control_main");
	FindDeprecated(ptr,(char*)"$control_pre",(char*)"Deprecated $control_pre needs to be $cs_control_pre");
	FindDeprecated(ptr,(char*)"$control_post",(char*)"Deprecated $control_post needs to be $cs_control_post");
#ifdef INFORMATION
	A token is nominally a contiguous collection of characters broken off by tab or space (since return and newline are stripped off).
	Tokens to include whitespace are encased in doublequotes.

	Characters with reserved status automatically also break into individual tokens and to include them you must put \ before them. These include:
	[ ]  ( )  { }  always and separate into individual tokens except for _(  _[   _{ 

	< > and << >> are reserved, but only when at start or end of token. Allowed comparisons embedded. As is <= and >=
	Tokens ending with ' or 's break off (possessive) in patterns.  
	
	Tokens starting with prefix characters ' or ! or _ keep together, except per reserved tokens.	'$junk  is one token.
	Variables ending with punctuation separate the punctuation.  $hello. is two tokens as is _0.

	Reserved characters in a composite token with _ before or after are kept. E.g. This_(_story_is_)_done
	You can include a reserved tokens by putting \ in front of them.

	Some tokens revise their start, like the pattern tokens representing comparison. They do this in the script compiler.
#endif

	// strings
	if (*ptr == '"' || ( *ptr  == '^' && ptr[1] == '"') || ( *ptr  == '^' && ptr[1] == '\'') || (*ptr == '\\' && ptr[1] == '"')) //   doublequote maybe with functional heading
	{
		// simple \"
		if (*ptr == '\\' && (!ptr[2] || ptr[2] == ' ' || ptr[2] == '\t' || ptr[2] == ENDUNIT)) // legal
		{
			*word = '\\';
			word[1] = '"';
			word[2] = 0;
			return ptr+2;
		}
		bool backslash = false;
		bool noblank = true;
		bool functionString = false;
		if (*ptr == '^') 
		{
			*word++ = *ptr++;	// ^"script"    swallows ^
			noblank = false; // allowed blanks at start or rear
			functionString = true;
		}
		else if (*ptr == '\\') //  \"string is this"
		{
			backslash = true;
			++ptr;
		}
		char* end = ReadQuote(ptr,word,backslash,noblank,MAX_WORD_SIZE);	//   swallow ending marker and points past
		if (!callingSystem && !isDescribe && !chunking && !functionString && *word == '"' && word[1] != '^' && strstr(word,"$_")) 
			WARNSCRIPT((char*)"%s has potential local var $_ in it. This cannot be passed as argument to user macros. Is it intended to be?\r\n",word)
		if (end)  
		{
			if (*word == '"' && word[1] != FUNCTIONSTRING && !functionString) return end; // all legal within
			// NOW WE SEE A FUNCTION STRING
			// when seeing ^, see if it remaps as a function argument
			// check for internal ^ also...
			char* hat = word-1;
			if ((*word == '"' || *word == '\'') && functionString) hat = word; // came before
			else if (*word == '"' && word[1] == FUNCTIONSTRING) hat = word+1;
			else if ((word[1] == '"' || word[1] == '\'') && *word == FUNCTIONSTRING) hat = word;

			// locate any local variable references in active strings
			char* at = word;
			while ((at = strchr(at,USERVAR_PREFIX)))
			{
				if (at[1] == LOCALVAR_PREFIX)
				{
					char* start = at;
					while (++at)
					{
						if (!IsAlphaUTF8OrDigit(*at) && *at != '_' && *at != '-')
						{
							char c = *at;
							*at = 0;
							AddDisplay(start);
							*at = c;
							break;
						}
					}
				}
				else ++at;
			}
			
			while ( (hat = strchr(hat+1,'^'))) // find a hat within
			{
				if (IsDigit(hat[1])) continue; // normal internal
				if (*(hat-1) == '\\') continue;	// escaped
				char* at = hat;
				while (*++at && (IsAlphaUTF8OrDigit(*at)  || *at == '_')){;}
				char c = *at;
				*at = 0;
				int index = GetFunctionArgument(hat);
				WORDP D = FindWord(hat); // in case its a function name
				*at = c;
				if (index >= 0) // was a function argument
				{
					strcpy(tmp,at); // protect chunk
					sprintf(hat,(char*)"^%d%s",index,tmp);
				}
				else if (D && D->internalBits & FUNCTION_NAME){;}
				else if (!renameInProgress && !(hat[1] == USERVAR_PREFIX || hat[1] == '_'))  
				{
					*at = 0;
					WARNSCRIPT((char*)"%s is not a recognized function argument. Is it intended to be?\r\n",hat)
					*at = c;
				}
			}

			hat = word-1;
			while ((hat = strchr(hat+1,'_'))) // rename _var? 
			{
				if (IsAlphaUTF8OrDigit(*(hat-1) ) || *(hat-1) == '_' || *(hat-1) == '-') continue; // not a starter
				if (IsDigit(hat[1])) continue; // normal _ var
				if (*(hat-1) == '\\' || *(hat-1) == '"') continue;	// escaped or quoted
				char* at = hat;
				while (*++at && (IsAlphaUTF8OrDigit(*at))){;} // find end
				WORDP D = FindWord(hat,at-hat,LOWERCASE_LOOKUP);
				if (D && D->internalBits & RENAMED) // remap matchvar inside  string
				{
					strcpy(tmp,at); // protect chunk
					sprintf(hat+1,(char*)"%d%s",(unsigned int)D->properties,tmp);
				}
			}

			hat = word-1;
			while ((hat = strchr(hat+1,'@'))) // rename @set?  
			{
				if (IsAlphaUTF8OrDigit(*(hat-1) )) continue; // not a starter
				if (IsDigit(hat[1]) || hat[1] == '_') continue; // normal @ var or @_marker
				if (*(hat-1) == '\\') continue;	// escaped
				char* at = GetSetEnd(hat);
				WORDP D = FindWord(hat,at-hat,LOWERCASE_LOOKUP);
				if (D && D->internalBits & RENAMED)  // rename @set inside string
				{
					strcpy(tmp,at); // protect chunk
					sprintf(hat+1,(char*)"%d%s",(unsigned int)D->properties,tmp);
				}
				else if (!renameInProgress)  // can do anything safely in a simple quoted string
				{
					char c = *at;
					*at = 0;
					WARNSCRIPT((char*)"%s is not a recognized @rename. Is it intended to be?\r\n",hat)
					*at = c;
				}
			}
			hat = word-1;
			if (strstr(readBuffer, "rename:")) // accept rename of existing constant twice in a row
				hat = " ";
			while ((hat = strchr(hat+1,'#'))) // rename #constant or ##constant
			{
				if (*(hat-1) == '\\') continue;	// escaped
				if (IsAlphaUTF8OrDigit(*(hat-1) ) || IsDigit(hat[1]) || *(hat - 1) == '&') continue; // not a starter, maybe #533; constant stuff
				char* at = hat;
				if (at[1] == '#')  ++at;	// user constant
				while (*++at && (IsAlphaUTF8OrDigit(*at) || *at == '_')){;} // find end
				strcpy(tmp,at); // protect chunk
				*at = 0;
				uint64 n;
				if (hat[1] == '#' && IsAlphaUTF8(hat[2])) // user constant
				{
					WORDP D = FindWord(hat,at-hat,LOWERCASE_LOOKUP);
					if (D && D->internalBits & RENAMED)  // remap #constant inside string
					{
						n = D->properties; 
						if (D->systemFlags & CONSTANT_IS_NEGATIVE) 
						{
							int64 x = (int64) n;
							x = -x;
#ifdef WIN32
							sprintf(hat,(char*)"%I64d%s",(long long int) x,tmp); 
#else
							sprintf(hat,(char*)"%lld%s",(long long int) x,tmp); 
#endif					
						}
						else
						{
#ifdef WIN32
							sprintf(hat,(char*)"%I64d%s",(long long int) n,tmp); 
#else
							sprintf(hat,(char*)"%lld%s",(long long int) n,tmp); 
#endif		
						}
					}
				}
				else // system constant
				{
					n = FindValueByName(hat+1);
					if (!n) n = FindSystemValueByName(hat+1);
					if (!n) n = FindParseValueByName(hat+1);
					if (!n) n = FindMiscValueByName(hat+1);
					if (n)
					{
#ifdef WIN32
						sprintf(hat,(char*)"%I64d%s",(long long int) n,tmp); 
#else
						sprintf(hat,(char*)"%lld%s",(long long int) n,tmp); 
#endif		
					}
				}
				if (!*hat) 
				{
					*hat = '#';
					BADSCRIPT((char*)"Bad # constant %s\r\n",hat)
				}
			}

			return end;					//   if we did swallow a string
		}
		if (*ptr == '\\') // was this \"xxx with NO closing
		{
			memmove(word+1,word,strlen(word)+1);
			*word = '\\';
		}
		else
		{
			word = start;
			if (*start == '^') --ptr;
		}
	}

	// the normal composite token
	bool quote = false;
	char* xxorig = ptr;
	bool var = (*ptr == '$');
	int brackets = 0;
	while (*ptr) 
	{
		if (*ptr == ENDUNIT) break;
		if (patternContext && quote) {} // allow stuff in comparison quote
		else if (*ptr == ' ' || *ptr == '\t') break; // legal
		if (patternContext && *ptr == '"') quote = !quote;

		char c = *ptr++;
		*word++ = c;
		*word = 0;
		if ((word - start) > (MAX_WORD_SIZE - 2)) break; // avoid overflow
		// want to leave array json notation alone but react to [...] touching a variable - $var]
		if (var && c == '[') // ANY variable should be separated by space from a [ if not json array
		{
			++brackets; // this MUST then be a json array and brackets will balance
			if (brackets > 1) BADSCRIPT("$var MUST be separated from [ unless you intend json array reference\r\n")
		}
		else if (var && c == ']') 
		{
			if (--brackets < 0) // if brackets is set, we must be in json array
			{
				--ptr;
				--word;
				break;  
			}
		}
		else if (GetNestingData(c)) // break off nesting attached to a started token unless its an escaped token
		{
			size_t len = word - start;
			if (len == 1) break;		// automatically token by itself
			if (len == 2)
			{
				if ((*start == '_' || *start == '!') && (c == '[' || c == '(' || c == '{')) break; // one token as _( or !( 
				if (*start == '\\') break;	// one token escaped
			}
			// split off into two tokens
			--ptr;
			--word;
			break;  
		}
	}
	*word = 0;

    word = start;
	size_t len = strlen(word);
	if (len == 0) return ptr; 
	if (patternContext && word[len - 1] == '"' && word[len - 2] != '\\')
	{
		char* quote = strchr(word, '"');
		if (quote == word+len-1) BADSCRIPT("Tailing quote without start: %s\r\n", word)
	}
	if (*word == '#' && !strstr(readBuffer,"rename:")) // is this a constant from dictionary.h? or user constant
	{
		uint64 n;
		if (word[1] == '#' && IsAlphaUTF8(word[2])) // user constant
		{
			WORDP D = FindWord(word,0,LOWERCASE_LOOKUP);
			if (D && D->internalBits & RENAMED)  // remap #constant 
			{
				n = D->properties; 
				if (D->systemFlags & CONSTANT_IS_NEGATIVE) 
				{
					int64 x = (int64) n;
					x = -x;
#ifdef WIN32
					sprintf(word,(char*)"%I64d",(long long int) x); 
#else
					sprintf(word,(char*)"%lld",(long long int) x); 
#endif					
				}
				else
				{
#ifdef WIN32
					sprintf(word,(char*)"%I64d",(long long int) n); 
#else
					sprintf(word,(char*)"%lld",(long long int) n); 
#endif		
				}
			}

			else if (renameInProgress) {;} // leave token alone, defining
			else BADSCRIPT((char*)"Bad user constant %s\r\n",word)
		}
		else // system constant
		{
			n = FindValueByName(word+1);
			if (!n) n = FindSystemValueByName(word+1);
			if (!n) n = FindParseValueByName(word+1);
			if (!n) n = FindMiscValueByName(word+1);
			if (n)
			{
#ifdef WIN32
				sprintf(word,(char*)"%I64d",(long long int) n); 
#else
				sprintf(word,(char*)"%lld",(long long int) n); 
#endif		
			}
			else if (!IsDigit(word[1]) && word[1] != '!') //treat rest as a comment line (except if has number after it, which is user text OR internal arg reference for function
			{
				if (IsAlphaUTF8(word[1])) 
					BADSCRIPT((char*)"Bad numeric # constant %s\r\n",word)
				*ptr = 0;
				*word = 0;
			}
		}
	}
	if ( *word == '_' && (IsAlphaUTF8(word[1]) ) ) // is this a rename _
	{
		WORDP D = FindWord(word);
		if (D && D->internalBits & RENAMED) sprintf(word+1,(char*)"%d",(unsigned int)D->properties); // remap match var convert to number
		// patterns can underscore ANYTING
	}
	if (*word == '\'' &&  word[1] == '_' && (IsAlphaUTF8(word[2]) ) ) // is this a rename _ with '
	{
		WORDP D = FindWord(word+1);
		if (D && D->internalBits & RENAMED) sprintf(word+2,(char*)"%d",(unsigned int)D->properties); // remap match var convert to number
		else if (!renameInProgress && !patternContext)  // patterns can underscore ANYTING
			WARNSCRIPT((char*)"%s is not a recognized _rename. Should it be?\r\n",word+1)
	}
	if ( *word == '@' && IsAlphaUTF8(word[1]) ) // is this a rename @
	{
		char* at = GetSetEnd(word);
		WORDP D = FindWord(word,at-word);
		if (D && D->internalBits & RENAMED) // remap @set in string
		{
			strcpy(tmp,at); // protect chunk
			sprintf(word+1,(char*)"%d%s",(unsigned int)D->properties,tmp);
		}
		else if (!renameInProgress)  WARNSCRIPT((char*)"%s is not a recognized @rename. Is it intended to be?\r\n",word)
	}
	if ( *word == '@' && word[1] == '_' && IsAlphaUTF8(word[2])) // is this a rename @_0+
	{
		size_t len = strlen(word);
		WORDP D = FindWord(word + 1, len - 1); // @_data marker
		char c = 0;
		if (!D)
		{
			c = word[len - 1];
			word[len - 1] = 0;
			D = FindWord(word + 1, len - 2);
			word[len - 1] = c;
		}
		if (D && D->internalBits & RENAMED) 
		{
			if (c) sprintf(word+2,(char*)"%d%c",(unsigned int)D->properties,c); // remap @set in string
			else sprintf(word + 2, (char*)"%d", (unsigned int)D->properties); // remap @set in string
		}
		else if (!renameInProgress)  WARNSCRIPT((char*)"%s is not a recognized @rename. Is it intended to be?\r\n",word)
	}

	// some tokens require special splitting
		
	//   break off starting << from <<hello   
	if (*word == '<' && word[1] != '=')
	{
		if (len == 3 && *word == word[1] && word[2] == '=') {;}
		else if (word[1] == '<')
		{
			if (word[2]) // not assign operator
			{
				ptr -= strlen(word) - 2;  // safe
				word[2] = 0;
				len -= 2;
			}
		}
	}

	//   break off ending  >> from hello>> 
	if (len > 2 && word[len-1] == '>')
	{
		if (len == 3 && *word == word[1] && word[2] == '=') {;}
		else if (word[len-2] == '>')
		{
			ptr -= 2;
			word[len-2] = 0;
			len -= 2;
		}
	}

	// break off punctuation from variable end 
	if (len > 2 && ((*word == USERVAR_PREFIX && !IsDigit(word[1]))  || *word == '^' || (*word == '@' && IsDigit(word[1])) || *word == SYSVAR_PREFIX || (*word == '_' && IsDigit(word[1])) || (*word == '\'' && word[1] == '_'))) // not currency
	{
		if (!patternContext || word[len-1] != '?') // BUT NOT $$xxx? in pattern context
		{
			while (IsRealPunctuation(word[len-1])) // one would be enough, but $hello... needs to be addressed
			{
				--len;
				--ptr;
			}
			word[len] = 0;
		}
	}

	// break off opening < in pattern
	if (patternContext && *word == '<' && word[1] != '<')
	{
		ptr -= len - 1;
		len = 1;
		word[1] = 0;
	}

	// break off closing > in pattern unless escaped or notted
	if (len == 2 && (*word == '!' || *word == '\\')){;}
	else if (patternContext && len > 1 && word[len-1] == '>' && word[len-2] != '>' && word[len-2] != '_' && word[len-2] != '!')
	{
		ptr -= len - 1;
		--len;
		word[len-1] = 0;
	}

	// find internal comparison op if any
	char* at = (patternContext) ? FindComparison(word) : 0;
	if (at && *word == '*' && !IsDigit(word[1])) 
	{
		if (compiling) BADSCRIPT((char*)"TOKENS-1 Cannot do comparison on variable gap %s . Memorize and compare against _# instead later.\r\n",word)
	}
	
	if (at && *at == '!' && at[1] == '$') { ; } // allow !$xxx
	else if (at) // revise comparison operators
	{
		if (*at == '!') ++at;
		++at;
		
		if (*at == '^' && at[1]) // remap function arg on right side.
		{
			int index = GetFunctionArgument(at);
			if (index >= 0) sprintf(at,(char*)"^%d",index);
		}
		if (*at == '_' && IsAlphaUTF8(word[1]) ) // remap rename matchvar arg on right side.
		{
			WORDP D = FindWord(at);
			if (D && D->internalBits & RENAMED) sprintf(at,(char*)"_%d",(unsigned int)D->properties);
		}
		if (*at == '@' && IsAlphaUTF8(word[1]) ) // remap @set arg on right side.
		{
			char* at1 = GetSetEnd(at);
			WORDP D = FindWord(at,at1-at);
			if (D && D->internalBits & RENAMED) // remap @set on right side
			{
				strcpy(tmp,at1); // protect chunk
				sprintf(at+1,(char*)"%d%s",(unsigned int)D->properties,tmp);
			}
		}

		// check for remap on LHS
		if (*word == '^')
		{
			char c = *--at;
			*at = 0;
			int index = GetFunctionArgument(word);
			*at = c;
			if (index >= 0) 
			{
				sprintf(tmp,(char*)"^%d%s",index,at);
				strcpy(word,tmp);
			}
		}
		// check for rename on LHS
		if (*word == '_' && IsAlphaUTF8(word[1]) )
		{
			char* at = word;
			while (IsAlphaUTF8OrDigit(*++at)){;}
			WORDP D = FindWord(word,at-word);
			if (D && D->internalBits & RENAMED) // remap match var
			{
				sprintf(tmp,(char*)"%d%s",(unsigned int)D->properties,at);
				strcpy(word+1,tmp);
			}
		}
		// check for rename on LHS
		if (*word == '@' && IsAlphaUTF8(word[1]) ) 
		{
			char* at = GetSetEnd(word);
			WORDP D = FindWord(word,at-word);
			if (D && D->internalBits & RENAMED) // remap @set in string
			{
				strcpy(tmp,at); // protect chunk
				sprintf(word+1,(char*)"%d%s",(unsigned int)D->properties,tmp);
			}
		}
	}
	// when seeing ^, see if it remaps as a function argument
	// check for internal ^ also...
	char* hat = word-1;
	while ( (hat = strchr(hat+1,'^'))) // find a hat within
	{
		char* at = hat;
		while (*++at && (IsAlphaUTF8(*at)  || *at == '_' || IsDigit(*at))){;}
		char c = *at;
		*at = 0; // terminate it so internal ^ is recognized uniquely
		strcpy(tmp,hat);
		*at = c;

		while (*tmp)
		{
			int index = GetFunctionArgument(tmp);
			if (index >= 0) 
			{
				char remainder[MAX_WORD_SIZE];
				strcpy(remainder,at); // protect chunk AFTER this
				sprintf(hat,(char*)"^%d%s",index,remainder);
				break;
			}
			else tmp[0] = 0;	// just abort it for now shrink it smaller, to handle @9subject kinds of behaviors 
		}
	}

	// same for quoted function arg
	if (*word == '\'' && word[1] == '^' && word[2])
	{
		int index = GetFunctionArgument(word+1);
		if (index >= 0) sprintf(word,(char*)"'^%d",index);
	}

	// break apart math on variables eg $value+2 as a service to the user
	if ((*word == '%'  || *word == '$') && word[1]) // cannot use _ here as that will break memorization pattern tokens
	{
		char* at = word + 1;
		if (at[1] == '$' || at[1] == '_') ++at;	// skip over 2ndary marker
		--at;
		while (LegalVarChar(*++at) );  // find end of initial word
		if (*word == '$' && (*at == '.'  || *at == '[' || *at == ']') &&  (LegalVarChar(at[1]) || at[1] == '$' || at[1] == '[' || at[1] == ']'))// allow $x.y as a complete name
		{
			while (LegalVarChar(*++at) || *at == '.' || *at == '$' || (*at == '[' || *at == ']') );  // find end of field name sequence
			if (*(at-1) == '.') --at; // tailing period cannot be part of it
		}  
		if (*at && IsPunctuation(*at) & ARITHMETICS && *at != '=')
		{
            if (*at == '.' && at[1] == '_' && IsDigit(at[2])) {} // json field reference indirection
            else if (*at == '.' && at[1] == '\'' && at[2] == '_' && IsDigit(at[3])) {} // json field reference indirection
                                                                // - is legal in a var or word token
			else if (*at != '-' || (!IsAlphaUTF8OrDigit(at[1]) && at[1] != '_'))
			{
				ptr -= strlen(at);
				*at = 0;
				len = at - start;
			}
		}
	}
	char* tilde = (IsAlphaUTF8(*word)) ? strchr(word+1,'~') : 0;
	if (tilde) // has specific meaning like African-american~1n or African-american~1
	{
		if (IsDigit(*++tilde)) // we know the meaning, removing any POS marker since that is redundant
		{
			if (IsDigit(*++tilde)) ++tilde;
			if (*tilde && !tilde[1])  *tilde = 0; // trim off pos marker

			// now force meaning to master
			MEANING M = ReadMeaning(word,true,false);
			if (M) 
			{
				M = GetMaster(M);
				sprintf(word,(char*)"%s~%d",Meaning2Word(M)->word,Meaning2Index(M));
			}
		}
	}

	// universal cover of simple use - complex tokens require processing elsewhere
	if (*word == USERVAR_PREFIX && word[1] == LOCALVAR_PREFIX) 
	{
		char* at = word + 1;
		while (*++at)
		{
			if (!IsAlphaUTF8OrDigit(*at) && *at != '-' && *at != '_') break;
		}
		if (*at == '.')  // root of a dotted variable
		{
			*at = 0;
			AddDisplay(word);
			*at = '.';
		}
		else if (!*at)  AddDisplay(word);
	}

	InsureAppropriateCase(word);
	return ptr; 
}

char* ReadDisplayOutput(char* ptr,char* buffer) // locate next output fragment to display (that will be executed)
{
	char next[MAX_WORD_SIZE];
	char* hold;
	*buffer = 0;
	char* out = buffer;
	while (*ptr != ENDUNIT) // not end of data
	{
        char* before = ptr;
		ptr = ReadCompiledWord(ptr,out); // move token 
        if (*out && out[1] && out[2] && (out[3] == '{' || out[3] == '(') && !out[4]) // accellerator + opening?
        {   
            ptr = before + ACCELLSIZE; // ignore accel
            continue;
        }
        if (*out && out[1] && out[2] && !out[3] && ptr[0] == '{') // accellerator + opening?
        {
            continue; // accel before else final code
        }
        if (!strnicmp(ptr, "else", 4) && !out[3])
        {
            continue; // skip accel before else 
        }
		char* copied = out;
		out += strlen(out);
		strcpy(out,(char*)" ");
		++out;
		*out = 0;
		hold = ReadCompiledWord(ptr,next); // and the token after that?
		if (IsAlphaUTF8OrDigit(*copied) ) // simple output word was copied
		{
			if (!*next || !IsAlphaUTF8OrDigit(*next)) break; //  followed by something else simple
		}
		else if (*buffer == ':' && buffer[1]) // testing command occupies the rest always
		{
			char* end = strchr(ptr,ENDUNIT);
			if (end)
			{
				strncpy(out,ptr,end-ptr);
				out += end-ptr;
				*out = 0;
			}
			ptr = NULL;
			break;
		}
		else if (*buffer == '^' && *next == '(') // function call
		{
			char* end = BalanceParen(ptr+1,true,false); // function call args
			strncpy(out,ptr,end-ptr);
			out += end-ptr;
			*out = 0;
			ptr = end;
			break;
		}
		else if ((*buffer == USERVAR_PREFIX && (buffer[1] == LOCALVAR_PREFIX || buffer[1] == TRANSIENTVAR_PREFIX || IsAlphaUTF8(buffer[1]) )) || (*buffer == SYSVAR_PREFIX && IsAlphaUTF8(buffer[1])) || (*buffer == '@' && IsDigit(buffer[1])) || (*buffer == '_' && IsDigit(buffer[1]))  ) // user or system variable or factset or match variable
		{
			if (*next != '=' && next[1] != '=') break; // not an assignment statement
			while (hold) // read op, value pairs
			{
				strcpy(out,next); // transfer assignment op or arithmetic op 
				out += strlen(out);
				strcpy(out,(char*)" ");
				++out;
				ptr = ReadCompiledWord(hold,next); // read value
				strcpy(out,next); // transfer value
				out += strlen(out);
			
				// if value is a function call, get the whole call
				if (*next == '^' && *ptr == '(')
				{
					char* end = BalanceParen(ptr+1,true,false); // function call args
					strncpy(out,ptr,end-ptr);
					out += end-ptr;
					*out = 0;
					ptr = end;
				}

				strcpy(out,(char*)" ");
				++out;
				if (*ptr != ENDUNIT) // more to rule
				{
					hold = ReadCompiledWord(ptr,next); // is there more to assign
					if (IsArithmeticOperator(next)) continue; // need to swallow op and value pair
				}
				break;
			}
			break;
		}
		else if (*buffer == '[') // choice area
		{
			//   find closing ]
			char* end = ptr-1;
			while (ALWAYS) 
			{
				end = strchr(end+1,']'); // find a closing ] 
				if (!end) break; // failed
				if (*(end-1) != '\\') break; // ignore literal \[
			}
			if (end) // found end of a [] pair
			{
				++end;
				strncpy(out,ptr,end-ptr);
				out += end-ptr;
				*out = 0;
				ptr = end + 1;
				if (*ptr != '[') break; // end of choice zone
			}
		}
		else break;
	}
	if (!stricmp(buffer,(char*)"^^loop ( -1 ) "))  strcpy(buffer,(char*)"^^loop ( ) ");	// shorter notation
	return ptr;
}

////////////////// CAN BE COMPILED AWAY

#ifndef DISCARDSCRIPTCOMPILER

#define MAX_TOPIC_SIZE  500000
#define MAX_TOPIC_RULES 32767
#define MAX_TABLE_ARGS 20

static unsigned int hasPlans;					// how many plans did we read

static int missingFiles;						// how many files of topics could not be found

static int spellCheck = 0;						// what do we spell check
static int topicCount = 0;						// how many topics did we compile
static char duplicateTopicName[MAX_WORD_SIZE];	// potential topic name repeated
static char assignKind[MAX_WORD_SIZE];	// what we are assigning from in an assignment call
static char currentTopicName[MAX_WORD_SIZE];	// current topic being read
static char lowercaseForm[MAX_WORD_SIZE];		// a place to put a lower case copy of a token
static WORDP currentFunctionDefinition;			// current macro defining or executing
static FILE* patternFile = NULL; // where to store pattern words
static char* nextToken;			// current lookahead token

static char verifyLines[100][MAX_WORD_SIZE];	// verification lines for a rule to dump after seeing a rule
static unsigned int verifyIndex = 0;			// index of how many verify lines seen
static char* ReadIf(char* word, char* ptr, FILE* in, char* &data,char* rejoinders);
static char* ReadLoop(char* word, char* ptr, FILE* in, char* &data,char* rejoinders);


#ifdef INFORMATION
Script compilation validates raw topic data files amd converts them into efficient-to-execute forms.
This means creating a uniform spacing of tokens and adding annotations as appropriate.

Reading a topic file (on the pattern side) often has tokens jammed together. For example all grouping characters like
() [ ] { } should be independent tokens. Possessive forms like cat's and cats' should return as two tokens.

Just as all contractions will get expanded to the full word.

Some tokens can be prefixed with ! or single-quote or _ .
In order to be able to read special characters (including prefix characters) literally, one can prefix it with \  as in \[  . The token returned includes the \.
\! means the exclamation mark at end of sentence. 
You are not required to do \? because it is directly a legal token, but you can.  
You CANNOT test for . because it is the default and is subsumed automatically.
#endif

static void AddMap(char* kind,char* msg)
{
	if (!mapFile) return;
	if (kind)
	{
        char value[MAX_WORD_SIZE];
        strcpy(value, kind);
        char* at = strchr(value, ':');
        if (at)
        {
            sprintf(at + 1, " %d ", currentFileLine);
            at = strchr(kind, ':');
            strcat(value, at + 1);
        }
		fprintf(mapFile,(char*)"%s %s",value, (msg) ? msg : ((char*)""));
        if (myBot  && !strstr(value,"rule:") && !strstr(value, "complexity of"))
        {
#ifdef WIN32
            fprintf(mapFile, (char*)" %I64u", (int64)myBot);
#else
            fprintf(mapFile, (char*)" %llu", (int64)myBot);
#endif
        }
        fprintf(mapFile, (char*)"\r\n");
    }
	else fprintf(mapFile,(char*)"%s\r\n",msg); // for complexity metric
}

static void ClearBeenHere(WORDP D, uint64 junk)
{
	RemoveInternalFlag(D,BEEN_HERE);
    // clear transient ignore spell warning flag
    if (D->internalBits & DO_NOISE && !(D->internalBits & HAS_SUBSTITUTE))
        RemoveInternalFlag(D, DO_NOISE);
}

bool TopLevelUnit(char* word) // major headers (not kinds of rules)
{
	return (!stricmp(word,(char*)":quit") || !stricmp(word,(char*)"canon:")  || !stricmp(word,(char*)"replace:") || !stricmp(word, (char*)"ignorespell:") || !stricmp(word, (char*)"prefer:") || !stricmp(word,(char*)"query:")  || !stricmp(word,(char*)"concept:") || !stricmp(word,(char*)"data:") || !stricmp(word,(char*)"plan:")
		|| !stricmp(word,(char*)"outputMacro:") || !stricmp(word,(char*)"patternMacro:") || !stricmp(word,(char*)"dualMacro:")  || !stricmp(word,(char*)"table:") || !stricmp(word,(char*)"tableMacro:") || !stricmp(word,(char*)"rename:") || 
		!stricmp(word,(char*)"describe:") ||  !stricmp(word,(char*)"bot:") || !stricmp(word,(char*)"topic:") || (*word == ':' && IsAlphaUTF8(word[1])) );	// :xxx is a debug command
}

static char* FlushToTopLevel(FILE* in,unsigned int depth,char* data)
{
	globalDepth = depth;
	if (data) *data = 0; // remove data
	char word[MAX_WORD_SIZE];
	int oldindex = jumpIndex;
	jumpIndex = -1; // prevent ReadNextSystemToken from possibly crashing.
	if (newBuffer) *newBuffer = 0;
	ReadNextSystemToken(NULL,NULL,word,false);	// clear out anything ahead
	char* ptr = readBuffer + strlen(readBuffer) - 1;
	while (ALWAYS)
	{
		char* quote = NULL;
		while ((quote = strchr(ptr,'"'))) ptr = quote + 1; //  flush quoted things
		ptr = ReadNextSystemToken(in,ptr,word,false);
		if (!*word) break;
		MakeLowerCopy(lowercaseForm,word);
		if (TopLevelUnit(lowercaseForm) || TopLevelRule(lowercaseForm)) 
		{
			ptr -= strlen(word); // safe
			break;
		}
	}
	jumpIndex = oldindex;
	return ptr;
}

static bool IsSet(char* word)
{
	if (!word[1]) return true;
	WORDP D = FindWord(word,0,LOWERCASE_LOOKUP);
	return (D && D->internalBits & CONCEPT) ? true : false;
}

static bool IsTopic(char* word)
{
	if (!word[1]) return true;
	WORDP D = FindWord(word,0,LOWERCASE_LOOKUP);
	return (D && D->internalBits & TOPIC) ? true : false;
}

static void DoubleCheckSetOrTopic()
{
	char file[200];
	sprintf(file,"%s/missingsets.txt",topic);
	FILE* in = FopenReadWritten(file);
	if (!in) return;
	*currentFilename = 0; // dont tell the name of the file
	while (ReadALine(readBuffer,in)  >= 0) 
    {
		char word[MAX_WORD_SIZE];
		ReadCompiledWord(readBuffer,word);
		if (!IsSet(word) && !IsTopic(word)) 
			WARNSCRIPT((char*)"Undefined set or topic %s\r\n",readBuffer)
	}
	FClose(in);
	remove(file);
}

static void CheckSetOrTopic(char* name) // we want to prove all references to set get defined
{
	if (*name == '~' && !name[1]) return; // simple ~ reference
	char word[MAX_WORD_SIZE];
	MakeLowerCopy(word,name);
	char* label = strchr(word,'.'); // set reference might be ~set or  ~set.label
	if (label) *label = 0; 
	if (IsSet(word) || IsTopic(word)) return;

	WORDP D = StoreWord(word);
	if (D->internalBits & BEEN_HERE) return; // already added to check file
	AddInternalFlag(D,BEEN_HERE);
	char file[200];
	sprintf(file,"%s/missingsets.txt",topic);
	FILE* out = FopenUTF8WriteAppend(file);
	fprintf(out,(char*)"%s line %d in %s\r\n",word,currentFileLine,currentFilename);
	fclose(out); // dont use FClose
}

static char* AddVerify(char* kind, char* sample)
{
	char* comment = strstr(sample,(char*)"# "); // locate any comment on the line and kill it
	if (comment) *comment = 0;
	sprintf(verifyLines[verifyIndex++],(char*)"%s %s",kind,SkipWhitespace(sample)); 
	return 0;	// kill rest of line
}

static void WriteVerify()
{
	if (!verifyIndex) return;
	char name[100];
	sprintf(name,(char*)"VERIFY/%s-b%c.txt",currentTopicName+1, (buildID == BUILD0) ? '0' : '1'); 
	FILE* valid  = FopenUTF8WriteAppend(name);
	static bool init = true;
	if (!valid && init) 
	{
		MakeDirectory((char*)"VERIFY");
		init = false;
		valid  = FopenUTF8WriteAppend(name);
	}

	if (valid)
	{
		char* space = "";
		if (REJOINDERID(currentRuleID)) space = "    ";
		for (unsigned int i = 0; i < verifyIndex; ++i) 
		{
			fprintf(valid,(char*)"%s %s.%d.%d %s\r\n",space,currentTopicName,TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID),verifyLines[i]); 
		}
		fclose(valid); // dont use FClose
	}

	verifyIndex = 0;
}

#ifdef INFORMATION
We mark words that are not normally in the dictionary as pattern words if they show up in patterns.
For example, the names of synset heads are not words, but we use them in patterns. They will
be marked during the scan phase of matching ONLY if some pattern "might want them". I.e., 
they have a pattern-word mark on them. same is true for multiword phrases we scan.

Having marked words also prevents us from spell-correcting something we were expecting but which is not a legal word.
#endif


static void DownHierarchy(MEANING T, FILE* out, int depth)
{
    if ( !T) return;
    WORDP D = Meaning2Word(T);
	if (D->internalBits & VAR_CHANGED) return;
	if (*D->word == '~') fprintf(out,(char*)"%s depth=%d\r\n",D->word,depth); 
	else  fprintf(out,(char*)"  %s\r\n",D->word); 
	D->internalBits |= VAR_CHANGED;
    unsigned int size = GetMeaningCount(D);
    if (!size) size = 1; 
	if (*D->word == '~') // show set members
	{
		FACT* F = GetObjectNondeadHead(D);
		while (F)
		{
			if (F->verb == Mmember) DownHierarchy(F->subject,out,depth+1);
			F = GetObjectNondeadNext(F);
		}
		fprintf(out,(char*)". depth=%d\r\n",depth); 
	}
}

static void WriteKey(char* word)
{
	if (!compiling || spellCheck != NOTE_KEYWORDS || *word == '_' || *word == '\'' || *word == USERVAR_PREFIX || *word == SYSVAR_PREFIX || *word == '@') return;
	StoreWord(word);
    char file[SMALL_WORD_SIZE];
    sprintf(file,(char*)"%s/keys.txt",tmp);
	FILE* out = FopenUTF8WriteAppend(file);
	if (out)
	{
		DownHierarchy(MakeMeaning(StoreWord(word)),out,0);
		fclose(out); // dont use Fclose
	}
}

static void WritePatternWord(char* word)
{
	if (*word == '~' || *word == USERVAR_PREFIX || *word == '^') return; // not normal words

	if (IsDigit(*word)) // any non-number stuff
	{
		char* ptr = word;
		while (*++ptr)
		{
			if (!IsDigit(*ptr) && *ptr != '.' && *ptr != ',') break;
		}
		if (!*ptr) return;	// ordinary number
	}
	
	// do we want to note this word
	WORDP D = StoreWord(word);
	// if word is not resident, maybe its properties are transient so we must save pattern marker
	if (!(D->properties & NORMAL_WORD)) AddSystemFlag(D,PATTERN_WORD);
	if (!compiling) return; 

	// case sensitivity?
	char tmp[MAX_WORD_SIZE];
	MakeLowerCopy(tmp,word);
	WORDP lower = FindWord(word,0,LOWERCASE_LOOKUP);
	WORDP upper = FindWord(word,0,UPPERCASE_LOOKUP);
	char utfcharacter[10];
	char* x = IsUTF8(word, utfcharacter); 
	if (!strcmp(tmp,word) || utfcharacter[1])  {;} // came in as lower case or as UTF8 character, including ones that don't have an uppercase version?
    else if (nospellcheck) {;}
    else if (lower && lower->internalBits & DO_NOISE && !(lower->internalBits & HAS_SUBSTITUTE)) {} // told not to check
    else if (upper && (GetMeaningCount(upper) > 0 || upper->properties & NORMAL_WORD )){;} // clearly known as upper case
	else if (lower && lower->properties & NORMAL_WORD && !(lower->properties & (DETERMINER|AUX_VERB)))
		WARNSCRIPT((char*)"Keyword %s should not be uppercase - did prior rule fail to close\r\n",word)
	else if (spellCheck && lower && lower->properties & VERB && !(lower->properties & NOUN))
		WARNSCRIPT((char*)"Uppercase keyword %s is usually a verb.  Did prior rule fail to close\r\n",word)
	
        char* pos;
	if ((pos = strchr(word,'\'')) && (pos[1] != 's' && !pos[1]))  WARNSCRIPT((char*)"Contractions are always expanded - %s won't be recognized\r\n",word)
	if (D->properties & NORMAL_WORD) return;	// already a known word
	if (D->internalBits & BEEN_HERE) return;	//   already written to pattern file or doublecheck topic ref file
	if (patternFile) 
	{
		fprintf(patternFile,(char*)"%s\r\n",word);
		AddInternalFlag(D,BEEN_HERE);
	}
}

static void NoteUse(char* label,char* topicName)
{
	MakeUpperCase(label);
	WORDP D = FindWord(label);
	if (!D || !(D->internalBits & LABEL))
	{
		char file[200];
		sprintf(file,"%s/missingLabel.txt",topic);
		FILE* out = FopenUTF8WriteAppend(file);
		if (out)
		{
			fprintf(out,(char*)"%s %s %s %d %s\r\n",label,topicName,currentFilename,currentFileLine, botheader);
			fclose(out); // dont use FClose
		}
	}
}

static void ValidateCallArgs(WORDP D,char* arg1, char* arg2,char* argset[ARGSETLIMIT+1], bool needToField)
{
	if (needToField) // assigning query to var, must give TO field value
	{
		if (!*argset[1] || !*argset[2] || !*argset[3] || !*argset[4] || !*argset[5] || !*argset[6])
			BADSCRIPT((char*)"CALL- 62 query assignment to variable requires TO field\r\n")
		char* p = argset[7];
		while (IsDigit(*++p)){} // skip 
		if (!*p) WARNSCRIPT((char*)"Query assignment requires field name in %s, I don't see one.\r\n",argset[7])
	}
	if (!stricmp(D->word,(char*)"^next"))
	{	
		if (stricmp(arg1,(char*)"RESPONDER") && stricmp(arg1,(char*)"LOOP") && stricmp(arg1,(char*)"REJOINDER") && stricmp(arg1,(char*)"RULE") && stricmp(arg1,(char*)"GAMBIT") && stricmp(arg1,(char*)"INPUT") && stricmp(arg1,(char*)"FACT")) 
			BADSCRIPT((char*)"CALL- 62 1st argument to ^next must be FACT OR LOOP OR INPUT or RULE or GAMBIT or RESPONDER or REJOINDER - %s\r\n",arg1)
	}	
	else if(!stricmp(D->word,(char*)"^jsonarraydelete"))
	{
		MakeLowerCase(arg1);
		if (!strstr(arg1,(char*)"index") && !strstr(arg1,(char*)"value") )
			BADSCRIPT((char*)"CALL- ? 1st argument to ^jsonarraydelete must be INDEX or VALUE - %s\r\n",arg1)
	}
	else if(!stricmp(D->word,(char*)"^keephistory"))
	{
		if (stricmp(arg1,(char*)"USER") && stricmp(arg1,(char*)"BOT") )
			BADSCRIPT((char*)"CALL- ? 1st argument to ^keephistory must be BOT OR USER - %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^conceptlist"))
	{
		if (stricmp(arg1,(char*)"TOPIC") && stricmp(arg1,(char*)"CONCEPT") && stricmp(arg1,(char*)"BOTH"))
			BADSCRIPT((char*)"CALL- ? 1st argument to ^conceptlist must be CONCEPT or TOPIC or BOTH - %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^field") && IsAlphaUTF8(*arg2))
	{	
		if (*arg2 != '$' && *arg2 != '^' && *arg2 != 's' && *arg2 != 'S' &&  *arg2 != 'v' && *arg2 != 'V' && *arg2 != 'O' && *arg2 != 'o'  && *arg2 != 'F' && *arg2 != 'f' && *arg2 != 'A' && *arg2 != 'a' && *arg2 != 'R' && *arg2 != 'r') 
			BADSCRIPT((char*)"CALL- 9 2nd argument to ^field must be SUBJECT or VERB or OBJECT or ALL or RAW or FLAG- %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^decodepos") )
	{
		if (stricmp(arg1,(char*)"POS") && stricmp(arg1,(char*)"ROLE")) 
			BADSCRIPT((char*)"CALL- ? 1st argument to ^decodepos must be POS or ROLE - %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^position") )
	{
		if (stricmp(arg1,(char*)"START") && stricmp(arg1,(char*)"END") && stricmp(arg1,(char*)"BOTH")) 
			BADSCRIPT((char*)"CALL- ? 1st argument to ^position must be START, END, BOTH, - %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^getparse") )
	{
		if (stricmp(arg2,(char*)"PHRASE") && stricmp(arg2,(char*)"VERBAL") && stricmp(arg2,(char*)"CLAUSE")&& stricmp(arg2,(char*)"NOUNPHRASE")) 
			BADSCRIPT((char*)"CALL- ? 2nd argument to ^getparse must be PHRASE, VERBAL, CLAUSE, NOUNPHRASE- %s\r\n",arg2)
	}
    else if (!stricmp(D->word, (char*)"^reset"))
    {
        if (stricmp(arg1, (char*)"history") &&  stricmp(arg1, (char*)"facts") &&  stricmp(arg1, (char*)"variables") && stricmp(arg1, (char*)"user") && stricmp(arg1, (char*)"topic") && stricmp(arg1, (char*)"output") && *arg1 != '@')
            BADSCRIPT((char*)"CALL- 10 1st argument to ^reset must be USER or TOPIC or OUTPUT or VARIABLES or FACTS or HISTORY or an @set- %s\r\n", arg1)
    }
    else if (!stricmp(D->word,(char*)"^substitute"))
	{
		if (stricmp(arg1,(char*)"word") && stricmp(arg1,(char*)"character")  && stricmp(arg1,(char*)"insensitive") && *arg1 != '"' && *arg1 != '^') 
			BADSCRIPT((char*)"CALL- 11 1st argument to ^substitute must be WORD or CHARACTER or INSENSITIVE- %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^setrejoinder"))
	{
		if (*arg2 && stricmp(arg1,(char*)"input") && stricmp(arg1,(char*)"output") && stricmp(arg1,(char*)"copy") ) 
			BADSCRIPT((char*)"CALL- 63 call to ^setrejoinder requires INPUT or OUTPUT or COPY as the 1st arg.\r\n")
		if (!*arg2 && (!stricmp(arg1,(char*)"input") || !stricmp(arg1,(char*)"output") || !stricmp(arg1,(char*)"copy")) )
			BADSCRIPT((char*)"CALL- 63 call to ^setrejoinder requires 2nd argument naming what rule to use as rejoinder\r\n")
	}
	else if (!stricmp(D->word, (char*)"^pos"))
	{
		if (stricmp(arg1, (char*)"conjugate") && stricmp(arg1, (char*)"raw") && stricmp(arg1, (char*)"allupper") && stricmp(arg1, (char*)"syllable") && stricmp(arg1, (char*)"ADJECTIVE") && stricmp(arg1, (char*)"ADVERB") && stricmp(arg1, (char*)"VERB") && stricmp(arg1, (char*)"AUX") && stricmp(arg1, (char*)"PRONOUN") && stricmp(arg1, (char*)"TYPE") && stricmp(arg1, (char*)"HEX32") && stricmp(arg1, (char*)"HEX64")
			&& stricmp(arg1, (char*)"NOUN") && stricmp(arg1, (char*)"DETERMINER") && stricmp(arg1, (char*)"PLACE") && stricmp(arg1, (char*)"common")
			&& stricmp(arg1, (char*)"capitalize") && stricmp(arg1, (char*)"uppercase") && stricmp(arg1, (char*)"lowercase")
			&& stricmp(arg1, (char*)"canonical") && stricmp(arg1, (char*)"integer") && stricmp(arg1, (char*)"IsModelNumber") && stricmp(arg1, (char*)"IsInteger") && stricmp(arg1, (char*)"IsUppercase") && stricmp(arg1, (char*)"IsFloat"))
			BADSCRIPT((char*)"CALL- 12 1st argument to ^pos must be SYLLABLE or ALLUPPER or VERB or AUX or PRONOUN or NOUN or ADJECTIVE or ADVERB or DETERMINER or PLACE or COMMON or CAPITALIZE or UPPERCASE or LOWERCASE or CANONICAL or INTEGER or HEX32 or HEX64 or ISMODELNUMBER or ISINTEGER or ISUPPERCASE or ISFLOAT - %s\r\n", arg1)
	}
	else if (!stricmp(D->word,(char*)"^getrule"))
	{
		if (stricmp(arg1,(char*)"TOPIC") &&  stricmp(arg1,(char*)"OUTPUT") && stricmp(arg1,(char*)"PATTERN") && stricmp(arg1,(char*)"LABEL") && stricmp(arg1,(char*)"TYPE")  && stricmp(arg1,(char*)"TAG") && stricmp(arg1,(char*)"USABLE")) 
 			BADSCRIPT((char*)"CALL- 13 1st argument to ^getrule must be TAG or TYPE or LABEL or PATTERN or OUTPUT or TOPIC or USABLE - %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^poptopic"))
	{
		if (*arg1 && *arg1 != '~' && *arg1 != USERVAR_PREFIX && *arg1 != '_' && *arg1 != SYSVAR_PREFIX && *arg1 != '^')
 			BADSCRIPT((char*)"CALL- 61 1st argument to ^poptopic must be omitted or a topic name or variable which will return a topic name - %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^nextrule"))
	{
		if (stricmp(arg1,(char*)"GAMBIT") && stricmp(arg1,(char*)"RESPONDER") && stricmp(arg1,(char*)"REJOINDER")  && stricmp(arg1,(char*)"RULE")) 
 			BADSCRIPT((char*)"CALL- 14 1st argument to ^getrule must be TAG or TYPE or LABEL or PATTERN or OUTPUT - %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^end"))
	{
		if (stricmp(arg1,(char*)"RULE") && stricmp(arg1,(char*)"CALL") && stricmp(arg1,(char*)"LOOP") && stricmp(arg1,(char*)"TOPIC") && stricmp(arg1,(char*)"SENTENCE") && stricmp(arg1,(char*)"INPUT")  && stricmp(arg1,(char*)"PLAN")) 
 			BADSCRIPT((char*)"CALL- 15 1st argument to ^end must be RULE or LOOP or TOPIC or SENTENCE or INPUT or PLAN- %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^fail"))
	{
		if (stricmp(arg1,(char*)"RULE") && stricmp(arg1,(char*)"CALL") && stricmp(arg1,(char*)"LOOP")  && stricmp(arg1,(char*)"TOPIC") && stricmp(arg1,(char*)"SENTENCE") && stricmp(arg1,(char*)"INPUT")) 
 			BADSCRIPT((char*)"CALL- 16 1st argument to ^fail must be RULE or LOOP or TOPIC or SENTENCE or INPUT - %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^nofail"))
	{
		if (stricmp(arg1,(char*)"RULE") &&  stricmp(arg1,(char*)"LOOP") && stricmp(arg1,(char*)"TOPIC") && stricmp(arg1,(char*)"SENTENCE") && stricmp(arg1,(char*)"INPUT")) 
 			BADSCRIPT((char*)"CALL- 16 1st argument to ^nofail must be RULE or LOOP or TOPIC or SENTENCE or INPUT - %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^compute"))
	{
		char* op = arg2;
		if (stricmp(op,(char*)"+") && stricmp(op,(char*)"plus") && stricmp(op,(char*)"add") && stricmp(op,(char*)"and") &&
			stricmp(op,(char*)"sub") && stricmp(op,(char*)"minus") && stricmp(op,(char*)"subtract") && stricmp(op,(char*)"deduct") && stricmp(op,(char*)"-")  &&
			stricmp(op,(char*)"x") && stricmp(op,(char*)"times") && stricmp(op,(char*)"multiply") && stricmp(op,(char*)"*") &&
			stricmp(op,(char*)"divide") && stricmp(op,(char*)"quotient") && stricmp(op,(char*)"/") &&
			stricmp(op,(char*)"remainder") && stricmp(op,(char*)"modulo") && stricmp(op,(char*)"mod") && stricmp(op,(char*)"%") && stricmp(op,(char*)"random") &&
			stricmp(op,(char*)"root") && stricmp(op,(char*)"square_root")   && stricmp(op,(char*)"power") && stricmp(op,(char*)"exponent") && *op != '^' && *op != '_' && *op != '$')  // last covers macro args and exponents

			BADSCRIPT((char*)"CALL- 17 2nd argument to ^compute must be numeric operation - %s\r\n",op)
	}
	else if (!stricmp(D->word,(char*)"^counttopic") && IsAlphaUTF8(*arg1))
	{
		if (strnicmp(arg1,(char*)"gambit",6) && stricmp(arg1,(char*)"used") && strnicmp(arg1,(char*)"rule",4) && stricmp(arg1,(char*)"available"))  
			BADSCRIPT((char*)"CALL-20 CountTopic 1st arg must be GAMBIT or RULE or AVAILABLE or USED - %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^phrase"))
	{
		if (stricmp(arg1,(char*)"adjective") && stricmp(arg1,(char*)"verbal")&& stricmp(arg1,(char*)"noun")  && stricmp(arg1,(char*)"preposition"))  BADSCRIPT((char*)"CALL-21 ^Phrase 1st arg must be adjective or verbal or noun or preposition - %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^hasgambit") && IsAlphaUTF8(*arg2))
	{
		if (stricmp(arg2,(char*)"last") && stricmp(arg2,(char*)"any") )  BADSCRIPT((char*)"CALL-21 HasGambit 2nd arg must be omitted or be LAST or ANY - %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^lastused" ))
	{
		if (strnicmp(arg2,(char*)"gambit",6) && strnicmp(arg2,(char*)"rejoinder",9) && strnicmp(arg2,(char*)"responder",9) && stricmp(arg2,(char*)"any"))  BADSCRIPT((char*)"CALL-22 LastUsed 2nd arg must be GAMBIT or REJOINDER or RESPONDER or ANY - %s\r\n",arg2)
	}
	else if ((!stricmp(nextToken,(char*)"^first") || !stricmp(nextToken,(char*)"^last") || !stricmp(nextToken,(char*)"^random")) && *arg2) BADSCRIPT((char*)"CALL-23 Too many arguments to first/last/random - %s\r\n",arg2)
	else if (!stricmp(D->word,(char*)"^respond") && atoi(arg1))  BADSCRIPT((char*)"CALL-? argument to ^respond should be a topic, not a number. Did you intend ^response? - %s\r\n",arg1)
	else if (!stricmp(D->word,(char*)"^respond") && !stricmp(arg1,currentTopicName)) WARNSCRIPT((char*)"Recursive call to topic - possible infinite recursion danger %s\r\n",arg1)
	else if (!stricmp(D->word,(char*)"^gambit") && !stricmp(arg1,currentTopicName)) WARNSCRIPT((char*)"Recursive call to topic - possible infinite recursion danger %s\r\n",arg1)
	else if (!stricmp(D->word,(char*)"^response") && *arg1 == '~')  BADSCRIPT((char*)"CALL-? argument to ^response should be a number, not a topic. Did you intend ^respond? - %s\r\n",arg1)
	else if (!stricmp(D->word,(char*)"^burst") && !stricmp(arg1,(char*)"wordcount")) BADSCRIPT((char*)"CALL-? argument to ^burst renamed. Use 'count' instead of 'wordcount'\r\n")
	//   validate inference calls if we can
	else if (!strcmp(D->word,(char*)"^query"))
	{
		unsigned int flags = atoi(argset[9]);
		if (flags & (USER_FLAG1|USER_FLAG2|USER_FLAG3|USER_FLAG4) && !strstr(arg1,(char*)"flag_")) BADSCRIPT((char*)"CALL-24 ^query involving USER_FLAG1 must be named xxxflag_\r\n")
		if (!stricmp(arg1,(char*)"direct_s") || !stricmp(arg1,(char*)"exact_s"))
		{
			if (!*arg2 || *arg2 == '?') BADSCRIPT((char*)"CALL-24 Must name subject argument to query\r\n")
			if (*argset[3] && *argset[3] != '?') BADSCRIPT((char*)"CALL-25 Cannot name verb argument to query %s - %s\r\n",arg1,argset[3])
			if (*argset[4] && *argset[4] != '?') BADSCRIPT((char*)"CALL-26 Cannot name object argument to query %s - %s\r\n",arg1,argset[4])
			if (*argset[8] && *argset[8] != '?') BADSCRIPT((char*)"CALL-27 Cannot name propgation argument to query %s - %s\r\n",arg1,argset[8])
			if (*argset[9] && *argset[9] != '?') BADSCRIPT((char*)"CALL-28 Cannot name match argument to query %s - %s\r\n",arg1,argset[9])
		}
		flags = atoi(argset[5]);
		if (flags & (USER_FLAG1|USER_FLAG2|USER_FLAG3|USER_FLAG4)  && flags < 0x00ffffff) WARNSCRIPT((char*)"Did you want a xxxflag_ query with USER_FLAG in 9th position for %s\r\n",arg1)
		if (!stricmp(arg1,(char*)"direct_v") || !stricmp(arg1,(char*)"exact_v"))
		{
			if (*arg2 && *arg2 != '?') BADSCRIPT((char*)"CALL-29 Cannot name subject argument to query - %s\r\n",arg2)
				if (!*argset[3] || *argset[3] == '?') BADSCRIPT((char*)"CALL-30 Must name verb argument to query\r\n")
			if (*argset[4] && *argset[4] != '?') BADSCRIPT((char*)"CALL-31 Cannot name object argument to query %s - %s\r\n",arg1,argset[4])
			if (*argset[8] && *argset[8] != '?') BADSCRIPT((char*)"CALL-32 Cannot name propgation argument to query %s - %s\r\n",arg1,argset[8])
			if (*argset[9] && *argset[9] != '?') 
				BADSCRIPT((char*)"CALL-33 Cannot name match argument to query %s - %s\r\n",arg1,argset[9])
		}
		if (!stricmp(arg1,(char*)"direct_o") || !stricmp(arg1,(char*)"exact_o"))
		{
			if (*arg2 && *arg2 != '?') BADSCRIPT((char*)"CALL-34 Cannot name subject argument to query -%s\r\n",arg2)
			if (*argset[3] && *argset[3] != '?') BADSCRIPT((char*)"CALL-35 Cannot name verb argument to query %s - %s\r\n",arg1,argset[3])
			if (!*argset[4] || *argset[4] == '?') BADSCRIPT((char*)"CALL-36 Must name object argument to query\r\n")
			if (*argset[8] && *argset[8] != '?') BADSCRIPT((char*)"CALL-37 Cannot name propgation argument to query %s - %s\r\n",arg1,argset[8])
			if (*argset[9] && *argset[9] != '?') BADSCRIPT((char*)"CALL-38 Cannot name match argument to query %s - %s\r\n",arg1,argset[9])
		}
		if (!stricmp(arg1,(char*)"direct_sv") || !stricmp(arg1,(char*)"exact_sv") )
		{
			if (!*arg2 || *arg2 == '?') BADSCRIPT((char*)"CALL-39 Must name subject argument to query\r\n")
			if (!*argset[3] || *argset[3] == '?') BADSCRIPT((char*)"CALL-40 Must name verb argument to query\r\n")
			if (*argset[4] && *argset[4] != '?') BADSCRIPT((char*)"CALL-41 Cannot name object argument to query %s - %s\r\n",arg1,argset[4])
			if (*argset[8] && *argset[8] != '?') BADSCRIPT((char*)"CALL-42 Cannot name propgation argument to query %s - %s\r\n",arg1,argset[8])
			if (*argset[9] && *argset[9] != '?') BADSCRIPT((char*)"CALL-43 Cannot name match argument to query %s - %s\r\n",arg1,argset[9])
		}
		if (!stricmp(arg1,(char*)"direct_sv_member"))
		{
			if (!*arg2 || *arg2 == '?') BADSCRIPT((char*)"CALL-44 Must name subject argument to query\r\n")
			if (!*argset[3] || *argset[3] == '?') BADSCRIPT((char*)"CALL-45 Must name verb argument to query\r\n")
			if (*argset[4] && *argset[4] != '?') BADSCRIPT((char*)"CALL-46 Cannot name object argument to query %s - %s\r\n",arg1,argset[4])
			if (*argset[8] && *argset[8] != '?') BADSCRIPT((char*)"CALL-47 Cannot name propgation argument to query %s - %s\r\n",arg1,argset[8])
			if (*argset[9] && *argset[9] == '?') BADSCRIPT((char*)"CALL-48 Must name match argument to query %s - %s\r\n",arg1,argset[9])
		}
		if (!stricmp(arg1,(char*)"direct_vo")|| !stricmp(arg1,(char*)"exact_vo"))
		{
			if (*arg2 && *arg2 != '?') BADSCRIPT((char*)"CALL-49 Cannot name subject argument to query -%s\r\n",arg2)
			if (!*argset[3] || *argset[3] == '?') BADSCRIPT((char*)"CALL-50 Must name verb argument to query\r\n")
			if (!*argset[4] || *argset[4] == '?') BADSCRIPT((char*)"CALL-51 Must name object argument to query\r\n")
			if (*argset[8] && *argset[8] != '?') BADSCRIPT((char*)"CALL-52 Cannot name propgation argument to query %s - %s\r\n",arg1,argset[8])
			if (*argset[9] && *argset[9] != '?') BADSCRIPT((char*)"CALL-53 Cannot name match argument to query %s - %s\r\n",arg1,argset[9])
		}
		if (!stricmp(arg1,(char*)"direct_svo") || !stricmp(arg1,(char*)"exact_svo") )
		{
			if (!*arg2 || *arg2 == '?') BADSCRIPT((char*)"CALL-54 Must name subject argument to query\r\n")
			if (!*argset[3] || *argset[3] == '?') BADSCRIPT((char*)"CALL-55 Must name verb argument to query\r\n")
			if (!*argset[4] || *argset[4] == '?') BADSCRIPT((char*)"CALL-56 Must name object argument to query\r\n")
			if (*argset[8] && *argset[8] != '?') BADSCRIPT((char*)"CALL-57 Cannot name propgation argument to query %s - %s\r\n",arg1,argset[8])
			if (*argset[9] && *argset[9] != '?') BADSCRIPT((char*)"CALL-58 Cannot name match argument to query %s - %s\r\n",arg1,argset[9])
		}
	}
}

static char* ReadCall(char* name, char* ptr, FILE* in, char* &data,bool call, bool needTofield)
{	//   returns with no space after it
	//   ptr is just after the ^name -- user can make a call w/o ^ in name but its been patched. Or this is function argument
	char reuseTarget1[SMALL_WORD_SIZE];
	char reuseTarget2[SMALL_WORD_SIZE];
	char* xxstartit = data;
	int oldcallingsystem = callingSystem;
	*reuseTarget2 = *reuseTarget1  = 0;	//   in case this turns out to be a ^reuse call, we want to test for its target
	char* argset[ARGSETLIMIT+1];
	char word[MAX_WORD_SIZE];
	char* arguments = ptr;
	// locate the function
	WORDP D = FindWord(name,0,LOWERCASE_LOOKUP);
	if (!call || !D || !(D->internalBits & FUNCTION_NAME))  //   not a function, is it a function variable?
	{
		if (IsDigit(name[1])) // function variable
		{
			*data++ = *name++;
			*data++ = *name++;
			if (IsDigit(*name)) *data++ = *name++;
			*data = 0;
			return ptr;
		}
	}
	
	SystemFunctionInfo* info = NULL;
	bool isStream = false;		//   dont check contents of stream, just format it
	if (D && !(D->internalBits & FUNCTION_BITS))			//   system function  (not pattern macro, outputmacro, dual macro, tablemacro, or plan macro)
	{
		++callingSystem;
		info = &systemFunctionSet[D->x.codeIndex];
		if (info->argumentCount == STREAM_ARG) isStream = true;

		if (!stricmp(name,"^jsonarraysize")) WARNSCRIPT((char*)"^jsonarraysize deprecated in favor of ^length\r\n")
		if (!stricmp(name,"^jsondelete")) WARNSCRIPT((char*)"^jsondelete deprecated in favor of ^delete\r\n")
	}
	else if (patternContext && D && (D->internalBits & IS_PLAN_MACRO) == IS_PLAN_MACRO) BADSCRIPT((char*)"CALL-2 cannot invoke plan from pattern area - %s\r\n", name)
	else if (patternContext && D && !(D->internalBits & (IS_PATTERN_MACRO | IS_OUTPUT_MACRO))) BADSCRIPT((char*)"CALL-2 Can only call patternmacro or dual macro from pattern area - %s\r\n",name)
	else if (!patternContext && D && !(D->internalBits &  (IS_OUTPUT_MACRO | IS_TABLE_MACRO))) BADSCRIPT((char*)"CALL-3 Cannot call pattern or table macro from output area - %s\r\n",name)
	
	memset(argset,0,sizeof(argset)); //   default EVERYTHING before we test it later
	if (D && !stricmp(D->word,(char*)"^debug")) 
		DebugCode(NULL); // a place for a script compile breakpoint

	// write call header
	strcpy(data,name); 
	data += strlen(name);
	*data++ = ' ';	
	*data++ = '(';	
	*data++ = ' ';	
	char* strbase = AllocateStack(NULL,1);
	bool oldContext = patternContext;
	patternContext = false;
    priorLine = currentFileLine;
	//   validate argument counts and stuff locally, then swallow data offically as output data
	int parenLevel = 1;
	int argumentCount = 0;
	ptr = ReadNextSystemToken(in,ptr,word,false); // skip (
    if (priorLine != currentFileLine)
    {
        AddMapOutput(priorLine);
        priorLine = currentFileLine;
    }
        
    while (ALWAYS) //   read as many tokens as needed to complete the call, storing them locally
	{
		ptr = ReadNextSystemToken(in,ptr,word,false);
        if (priorLine != currentFileLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
        if (!*word) break;
		if (*word == '#' && word[1] == '!') BADSCRIPT((char*)"#! sample input seen during a call. Probably missing a closing )\r\n");
	
		// closing paren stuck onto token like _) - break it off 
		size_t len = strlen(word);
		if (word[len-1] == ')' && len > 1 && (*word != '\\' || len > 2) )
		{
			--ptr;
			if (*ptr != ')') ptr -= 1;
			word[len-1] = 0;
		}
		MakeLowerCopy(lowercaseForm,word);

		//   note that in making calls, [] () and {}  count as a single argument with whatver they have inside
		switch(*word)
		{
		case '(': case '[': case '{':
			++parenLevel;
			break;
		case ')': case ']': case '}':
			--parenLevel;
			if (parenLevel == 1) ++argumentCount;	//   completed a () argument
			break;
		case '"':
			if (word[1] != FUNCTIONSTRING && oldContext) // simple string is in pattern context, flip to underscore form
			{
				// convert string into its pattern form.
				unsigned int n = BurstWord(word,0);
				if (n > 1) strcpy(word,JoinWords(n));
			}
			// DROPPING THRU
		case '\'':  
			// DROPPING THRU
		default:
			if (*word == '~' ) CheckSetOrTopic(word); // set or topic

			if (!stricmp(word,(char*)"PLAN") && !stricmp(name,(char*)"^end")) endtopicSeen = true;
			if (parenLevel == 1)  
			{
				if (*word == FUNCTIONSTRING && word[1] == '"')  
				{
					word[0] = '"';
					word[1] = FUNCTIONSTRING; // show we know it
					if (word[2] == ':')	 strcpy(word+3,CompileString(word+1)+2);
				}
				ReadNextSystemToken(in,ptr,nextToken,false,true);
                if (priorLine != currentFileLine)
                {
                    AddMapOutput(priorLine);
                    priorLine = currentFileLine;
                }
                // argument is a function without its ^ ?  // but be wary of doing this in createfact, which can have nested facts
				if (*word != '^' && *nextToken == '(' && stricmp(name,(char*)"^createfact")) //   looks like a call, reformat it if it is
				{
					char fnname[SMALL_WORD_SIZE];
					*fnname = '^';
					MakeLowerCopy(fnname+1,word);	
					WORDP D = FindWord(fnname,0,PRIMARY_CASE_ALLOWED);
					if (D && D->internalBits & FUNCTION_NAME) strcpy(word,fnname); 
				}

				if (*word == '^' && (*nextToken == '(' || IsDigit(word[1])))   //   function call or function var ref 
				{
					WORDP D = FindWord(word,0,LOWERCASE_LOOKUP);
					if (!IsDigit(word[1]) && word[1] != USERVAR_PREFIX && *nextToken == '(' && (!D || !(D->internalBits & FUNCTION_NAME)))
						BADSCRIPT((char*)"CALL-1 Default call to function not yet defined %s\r\n",word)
					if (*nextToken != '(' && !IsDigit(word[1])) BADSCRIPT((char*)"CALL-? Unknown function variable %s\r\n",word)

					char* arg = data;
					if (!stricmp(word,"^if")) 
					{
						ptr = ReadIf(word,ptr,in,data,NULL);
						*data++ = ' ';
					}
					else if (!stricmp(word,"^loop")) 
					{
						ptr = ReadLoop(word,ptr,in,data,NULL);
					}
					else 
					{
						ptr = ReadCall(word,ptr,in,data,*nextToken == '(',false);
						*data++ = ' ';
					}
					if (argumentCount < ARGSETLIMIT) 
					{
						argset[++argumentCount] = AllocateStack(arg);
					}
					continue;
				}
				if (*word == '^' && word[1] == '\''){;}
				else if (*word == '^' && *nextToken != '(' && word[1] != '^' && word[1] != USERVAR_PREFIX && word[1] != '_' && word[1] != '"' && !IsDigit(word[1]))
				 // ^^ indicated a deref of something
					BADSCRIPT((char*)"%s is either a function missing arguments or an undefined function variable.\r\n",word) //   not function call or function var ref
				// track only initial arguments for verify. can have any number when its a stream
				if (argumentCount < ARGSETLIMIT) 
				{
					argset[++argumentCount] = AllocateStack(word);
				}
			}
			else 
			{
				ReadNextSystemToken(in,ptr,nextToken,false,true);
				if (*word == '^' && *nextToken != '(' && *nextToken != ')' && word[1] != '"' && !IsDigit(word[1])) 
					WARNSCRIPT((char*)"Is %s intended as a function call?\r\n",word) //   not function call or function var ref
			}
		}
	
		if (oldContext && IsAlphaUTF8(*word) && stricmp(name,(char*)"^incontext") && stricmp(name,(char*)"^reuse") )  {
			WritePatternWord(word);
			WriteKey(word);
		}

		//   add simple item into data
		strcpy(data,word);
		data += strlen(data);
		if (parenLevel == 0) break;	//   we finished the call (no trailing space)
		*data++ = ' ';	
	}
	*--data = 0;  //   remove closing paren
						
	int cntr = argumentCount;
	while (cntr < 15) argset[++cntr] = AllocateStack("");

	char* arg1 = argset[1];
	char* arg2 = argset[2];

	// validate assignment calls if we can - this will be a first,last,random call
	if (*assignKind && (!stricmp(name,(char*)"^first") || !stricmp(name,(char*)"^last") || !stricmp(name,(char*)"^random") || !stricmp(name,(char*)"^nth") ) )
	{
		char kind = arg1[2];
		if (*arg1 == '~') {;} // get nth of a concept
		else if (!kind) BADSCRIPT((char*)"CALL-5 Assignment must designate how to use factset (s v or o)- %s  in %s %s\r\n",assignKind,name,arguments)
		else if ((kind == 'a' || kind == '+' || kind == '-') && *assignKind != '_')  
			BADSCRIPT((char*)"CALL-6 Can only spread a fact onto a match var - %s\r\n",assignKind)
		else if (*assignKind == SYSVAR_PREFIX && (kind == 'f' ||  !kind))  BADSCRIPT((char*)"CALL-7 cannot assign fact into system variable\r\n") // into system variable
		else if (*assignKind == '@' && kind != 'f') BADSCRIPT((char*)"CALL-8 Cannot assign fact field into fact set\r\n") // into set, but not a fact
	}
	
	if (D && !stricmp(D->word,(char*)"^reuse") && (IsAlphaUTF8(*arg1) || *arg1 == '~')) 
	{
		MakeUpperCopy(reuseTarget1,arg1); // topic names & labels must be upper case
	}
	else if (D && !stricmp(D->word,(char*)"^enable") && IsAlphaUTF8(*arg1))
	{
		if (stricmp(arg1,(char*)"topic") && stricmp(arg1,(char*)"write") && stricmp(arg1,(char*)"rule") && stricmp(arg1,(char*)"usedrules") ) BADSCRIPT((char*)"CALL-18 Enable 1st arg must be TOPIC or RULE or USEDRULES - %s\r\n",arg1)
		if (*arg2 == '@'){;}
		else if (*arg2 != '~' || strchr(arg2,'.')) // not a topic or uses ~topic.rulename notation
		{
			MakeUpperCopy(reuseTarget1,arg2); // topic names & labels must be upper case
		}
	}
	else if (D && !stricmp(D->word,(char*)"^disable") && IsAlphaUTF8(*arg1))
	{
		if (stricmp(arg1,(char*)"topic") && stricmp(arg1,(char*)"rule")  && stricmp(arg1,(char*)"write")&& stricmp(arg1,(char*)"rejoinder") && stricmp(arg1,(char*)"inputrejoinder") && stricmp(arg1,(char*)"outputrejoinder")  && stricmp(arg1,(char*)"save")  ) BADSCRIPT((char*)"CALL-19 Disable 1st arg must be TOPIC or RULE or INPUTREJOINDER or OUTPUTREJOINDER or SAVE - %s\r\n",arg1)
		if (*arg2 == '@'){;}
		else if (!stricmp(arg1,(char*)"rejoinder")){;}
		else if (*arg2 != '~' || strchr(arg2,'.'))  MakeUpperCopy(reuseTarget1,arg2); // topic names & labels must be upper case 
	}

	if (D) ValidateCallArgs(D,arg1,arg2,argset,needTofield);

	if (parenLevel != 0)
		BADSCRIPT((char*)"CALL-59 Failed to properly close (or [ in call to %s\r\n",D->word)

	if (isStream){;}  // no cares
	else if (info) // system function
	{
		if (argumentCount != (info->argumentCount & 255) && info->argumentCount != VARIABLE_ARG_COUNT && info->argumentCount != UNEVALED && info->argumentCount != STREAM_ARG) 
			BADSCRIPT((char*)"CALL-60 Incorrect argument count to system function %s- given %d instead of required %d\r\n",name,argumentCount,info->argumentCount & 255)
	}
	else if (D && (D->internalBits & FUNCTION_BITS) == IS_PLAN_MACRO) 
	{
		if (argumentCount != (int)D->w.planArgCount)
			BADSCRIPT((char*)"CALL-60 Incorrect argument count to plan %s- given %d instead of required %d\r\n",name,argumentCount,D->w.planArgCount)
	}
	else if (!D)
	{
		// generate crosscheck data
		char* nameData = AllocateHeap(NULL, strlen(name) + 2, 1);
		*nameData = argumentCount;
		strcpy(nameData + 1, name);
		char* filename = AllocateHeap(currentFilename, 0, 1);
		int* data = (int*)AllocateHeap(NULL, 4, 4);
		data[0] = undefinedCallThreadList;
		data[1] = Heap2Index(nameData);
		data[2] = Heap2Index(filename);
		data[3] = currentFileLine;
		undefinedCallThreadList = Heap2Index((char*)data);
	}
	else // std macro (input, output table)
	{
		unsigned char* defn = GetDefinition(D);
		if (defn && argumentCount != MACRO_ARGUMENT_COUNT(defn) && !(D->internalBits & VARIABLE_ARGS_TABLE)) 
			BADSCRIPT((char*)"CALL-60 Incorrect argument count to macro %s- given %d instead of required %d\r\n",name,argumentCount,MACRO_ARGUMENT_COUNT(GetDefinition(D)))
	}

	// handle crosscheck of labels
	char* dot = strchr(reuseTarget1,'.');
	if (!*reuseTarget1);
	else if (dot) // used dotted notation, split them up
	{
		strcpy(reuseTarget2,dot+1);
		*dot = 0;
	}
	else if (*reuseTarget1 != '~') //  only had name, not topic.name, fill in
	{
		strcpy(reuseTarget2,reuseTarget1);
		if (currentFunctionDefinition) strcpy(reuseTarget1,currentFunctionDefinition->word);
		else strcpy(reuseTarget1,currentTopicName);
	}

	if (*reuseTarget1 && (*reuseTarget1 != '$' && *reuseTarget1 != '^' && *reuseTarget1 != '_' && *reuseTarget2 != USERVAR_PREFIX && *reuseTarget2 != '_')) //   we cant crosscheck variable choices
	{
		if (*reuseTarget1 != '~')
		{
			memmove(reuseTarget1+1,reuseTarget1,strlen(reuseTarget1)+1);
			*reuseTarget1 = '~';
		}
		strcat(reuseTarget1,(char*)".");
		strcat(reuseTarget1,reuseTarget2); // compose the name
		NoteUse(reuseTarget1,currentFunctionDefinition ? currentFunctionDefinition->word : currentTopicName);
	}

	//   now generate stuff as an output stream with its validation
	patternContext = oldContext;
	if (D && !(D->internalBits & FUNCTION_BITS))  --callingSystem;

	callingSystem = oldcallingsystem;
	*data++ = ')'; //   outer layer generates trailing space
	*data = 0;
	ReleaseStack(strbase); // short term
	return ptr;	
}

static void TestSubstitute(char* word,char* message)
{
	WORDP D = FindWord(word);
	if (!D) return;
	WORDP E = GetSubstitute(D);
	if (E)
	{
		if (E->word[0] == '!') return; // ignore conditional
		char* which = "Something";
		if (D->internalBits & DO_SUBSTITUTES) which = "Substitutes.txt";
		if (D->internalBits & DO_CONTRACTIONS) which = "Contractions.txt";
		if (D->internalBits & DO_ESSENTIALS) which = "Essentials.txt";
		if (D->internalBits & DO_INTERJECTIONS) which = "Interjections.txt";
		if (D->internalBits & DO_BRITISH) which = "British.txt";
		if (D->internalBits & DO_SPELLING) which = "Spelling.txt";
		if (D->internalBits & DO_TEXTING) which = "Texting.txt";
		if (D->internalBits & DO_PRIVATE) which = "user private substitution";
		if (E->word[1])	WARNSCRIPT((char*)"%s changes %s to %s %s\r\n",which,word,E->word,message)
		else  WARNSCRIPT((char*)"%s erases %s %s\r\n",which,word,message)
	}
}

static void SpellCheckScriptWord(char* input,int startSeen,bool checkGrade) 
{
	if (!stricmp(input,(char*)"null")) return; // assignment clears
    if (nospellcheck) return;

	// remove any trailing punctuation
	char word[MAX_WORD_SIZE];

    // see if supposed to ignore capitalization differences
    MakeLowerCopy(word, input);
    WORDP X = FindWord(word, 0, LOWERCASE_LOOKUP);
    if (X && X->internalBits & DO_NOISE && !(X->internalBits & HAS_SUBSTITUTE))
        return;
	strcpy(word,input);
	size_t len = strlen(word);
	while (len > 1 && !IsAlphaUTF8(word[len-1]) && word[len-1] != '.') word[--len] = 0;
    
    WORDP set[20];
    int i = GetWords(word, set, false); // words in any case and with mixed underscore and spaces
    char text[MAX_WORD_SIZE];
    *text = 0;
    int nn = 0;
    if (i > 1) // multiple spell?
    {
        for (int x = 0; x < i; ++x)
        {
            if (GETMULTIWORDHEADER(set[x]))
            {
                if (!(set[x]->properties & TAG_TEST)) 
                    continue; // dont care
            }
            if (set[x]->properties & NOUN_FIRSTNAME)  // Will, June, etc
                continue;
            strcat(text, set[x]->word);
            strcat(text, " ");
            ++nn;
        }
    }
    if (nn > 1)
    {
        WARNSCRIPT((char*)"Word \"%s\" known in multiple spellings %s\r\n", word, text)
        return;
    }

	WORDP D = FindWord(word,0,LOWERCASE_LOOKUP);
	WORDP entry = D;
	WORDP canonical = D;
	if (word[1] == 0 || IsUpperCase(*input) || !IsAlphaUTF8(*word)  || strchr(word,'\'') || strchr(word,'.') || strchr(word,'_') || strchr(word,'-') || strchr(word,'~')) {;} // ignore proper names, sets, numbers, composite words, wordnet references, etc
	else if (stricmp(language,"english")) {;} // dont complain on foreign
	else if (!D || (!(D->properties & NORMAL_WORD) && !(D->systemFlags & PATTERN_WORD)))
	{ // we dont know this word in lower case
		uint64 sysflags = 0;
		uint64 cansysflags = 0;
		wordStarts[0] = wordStarts[1] = wordStarts[2] = wordStarts[3] = AllocateHeap((char*)"");
		wordCount = 1;
		WORDP revise;
		uint64 flags = GetPosData(-1,word,revise,entry,canonical,sysflags,cansysflags,false,true,0);
		if (!flags) // try upper case
		{
			WORDP E = FindWord(word,0,SECONDARY_CASE_ALLOWED); // uppercase
			if (E && E != D && E->word[2]) 
                WARNSCRIPT((char*)"Word %s only known in upper case\r\n",word)   
			else if (E && !(E->internalBits & UPPERCASE_HASH) && !D ) WARNSCRIPT((char*)"%s is not a known word. Is it misspelled?\r\n",word)
			canonical = E; // the base word
		}
	}
	// check vocabularly limits?
	if (grade && checkGrade && !stricmp(language,"English"))
	{
		if (canonical && !IsUpperCase(*input) && !(canonical->systemFlags & grade) && !strchr(word,'\'')) // all contractions are legal
			Log(STDTRACELOG,(char*)"Grade Limit: %s\r\n",D->word);
	}

	// see if substitition will ruin this word
	if (!(spellCheck & NO_SUBSTITUTE_WARNING) ) 
	{
		if (startSeen != -1) TestSubstitute(word,(char*)"anywhere in input");
		char test[MAX_WORD_SIZE];
		sprintf(test,(char*)"<%s",word);
		if (startSeen == 0) TestSubstitute(test,(char*)"at input start");
		sprintf(test,(char*)"%s>",word);
		if (startSeen != -1) TestSubstitute(test,(char*)"at input end");
		sprintf(test,(char*)"<%s>",word);
		if (startSeen == 0) TestSubstitute(test,(char*)"as entire input");
	}
}

static char* GetRuleElipsis(char* rule)
{
	static char value[50];
	strncpy(value,rule,45);
	value[45] = 0;
	return value;
}

static bool PatternRelationToken(char* ptr)
{
	if (*ptr == '!' && (ptr[1] == '=' || ptr[1] == '?')) return true;
	if (*ptr == '>' || *ptr == '<' || *ptr == '?' || *ptr == '&') return true;
	if (*ptr == '=') return true;;
	return false;
}

static bool RelationToken(char* word)
{
	if (*word == '=') return (word[1] == '=' || !word[1]);
	return (*word == '<' ||  *word == '>' ||  *word == '?'  || (*word == '!'  && word[1] == '=') || *word == '&');
}

static char* ReadDescribe(char* ptr, FILE* in,unsigned int build)
{
	while (ALWAYS) //   read as many tokens as needed to complete the definition (must be within same file)
	{
		char word[MAX_WORD_SIZE];
		char description[MAX_WORD_SIZE];
		ptr = ReadNextSystemToken(in,ptr,word,false);
		if (!stricmp(word,(char*)"describe:")) ptr = ReadNextSystemToken(in,ptr,word,false); // keep going with local  loop
		if (!*word) break;	//   file ran dry
		size_t len = strlen(word);
		if (TopLevelUnit(word)) //   definition ends when another major unit starts
		{
			ptr -= len; //   let someone else see this starter 
			break; 
		}
		if (*word != USERVAR_PREFIX && *word != '_' && *word != '^' && *word != '~')
			BADSCRIPT((char*)"Described entity %s is not legal to describe- must be variable or function or concept/topic\r\n", word)
		isDescribe = true;
		ptr = ReadNextSystemToken(in,ptr,description,false);
		isDescribe = false;
		char file[SMALL_WORD_SIZE];
		sprintf(file,(char*)"%s/BUILD%s/describe%s.txt",topic,baseName,baseName);
		FILE* out = FopenUTF8WriteAppend(file);
		fprintf(out,(char*)" %s %s\r\n",word,description);
		fclose(out); // dont use Fclose
	}
	return ptr;
}

bool StartScriptCompiler()
{
	if (nextToken) return false;
	oldBuffer = newBuffer;
	newBuffer = AllocateStack(NULL,MAX_BUFFER_SIZE);
	nextToken = AllocateStack(NULL, MAX_BUFFER_SIZE); // able to swallow big
	return true;

}

void EndScriptCompiler()
{
	ReleaseStack(nextToken);
	nextToken = NULL;
	if (newBuffer)
	{
		ReleaseStack(newBuffer);
		newBuffer = oldBuffer;
	}
}

char* ReadPattern(char* ptr, FILE* in, char* &data,bool macro, bool ifstatement, bool livecall)
{ //   called from topic or patternmacro
#ifdef INFORMATION //   meaning of leading characters
< >	 << >>	sentence start & end boundaries, any 
!			NOT 
nul			end of data from macro definition or argument substitution
* *1 *~ *~2 *-1	 gap (infinite, exactly 1 word, 0-2 words, 0-2 words, 1 word before)
_  _2		memorize next match or refer to 3rd memorized match (0-based)
@			factset references @5subject  and _1 (positional set)  
$			user variable 
^	^1		function call or function argument (user)
()[]{}		nesting of some kind (sequence AND, OR, OPTIONAL)
dquote		string token
?			a question input
~dat  ~		topic/set reference or current topic 
%			system variable 
=xxx		comparison test (= > < )
apostrophe and apostrophe!		non-canonical meaning on next token or exclamation test
\			escape next character as literal (\$ \= \~ \(etc)
#xxx		a constant number symbol, but only allowed on right side of comparison
------default values
-1234567890	number token
12.14		number token
1435		number token
a-z,A-Z,|,_	normal token
,			normal token (internal sentence punctuation) - period will never exist since we strip tail and cant be internal

----- these are things which must all be insured lower case (some can be on left or right side of comparison op)
%			system variable 
~dat 		topic/set reference
a: thru u:	responder codes
if/loop/loopcount	constructs
^call  call	function/macro calls with or without ^
^fnvar		function variables
^$glblvar	global function variables
$			user variable 
@			debug ahd factset references
labels on responders
responder types s: u: t: r: 
name of topic or concept

#endif

	char word[MAX_WORD_SIZE];
	char nestKind[1000];
    int nestLine[1000];
	int nestIndex = 0;
	patternContext = true;
	char* start = ptr;

	//   if macro call, there is no opening ( or closing )
	//   We claim an opening and termination comes from finding a toplevel token
	if (macro) nestKind[nestIndex++] = '(';

	bool variableGapSeen = false; // wildcard pending

	// prefix characters
	bool memorizeSeen = false; // memorization pending
	bool quoteSeen = false;	// saw '
	bool notSeen = false;	 // saw !
	bool doubleNotSeen = false; // saw !!
	size_t len;
	bool startSeen = false; // starting token or not
	char* startPattern = data;
	char* startOrig = ptr;
	char* backup;
	while (ALWAYS) //   read as many tokens as needed to complete the definition
	{
		backup = ptr;
		ptr = ReadNextSystemToken(in,ptr,word);
		if (!*word) break; //   end of file

		// we came from pattern IF and lack a (
		if (ifstatement && *word != '(' && nestIndex == 0) nestKind[nestIndex++] = '(';

		MakeLowerCopy(lowercaseForm,word);
		if (TopLevelUnit(lowercaseForm) || TopLevelRule(lowercaseForm)) // end of pattern
		{
			ptr -= strlen(word);  // safe
			break;
		}
		char c = 0;
		char* comparison = FindComparison(word);
		if (comparison) // comparison, do normal analysis on 1st argument
		{
			c = *comparison;
			*comparison = 0;
		}
		switch(*word) // ordinary tokens, not the composite comparison blob
		{
			// token prefixes
			case '!': //   NOT
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-1 Cannot have ' and ! in succession\r\n")
				if (memorizeSeen) BADSCRIPT((char*)"PATTERN-2 Cannot use _ before _\r\n")
				if (notSeen) BADSCRIPT((char*)"PATTERN-3 Cannot have two ! in succession\r\n")
				if (!word[1] && !livedata) 
					BADSCRIPT((char*)"PATTERN-4 Must attach ! to next token. If you mean exclamation match, use escaped ! \r\n %s\r\n",ptr)
				notSeen = true;
				if (word[1] == '!') 
					doubleNotSeen = true;
				if (comparison) *comparison = c;
				ptr -= strlen(word);  // safe
				if (*ptr == '!') ++ptr;
				if (*ptr == '!') ++ptr;	// possible !! allowed
				continue;
			case '_':	//   memorize OR var reference
				if (quoteSeen && !IsDigit(word[1])) BADSCRIPT((char*)"PATTERN-1 Cannot have ' and _ in succession except when quoting a match variable. Need to reverse them\r\n")
				if (memorizeSeen) BADSCRIPT((char*)"PATTERN-6 Cannot have two _ in succession\r\n")
				if (!word[1]) // allow separation which will be implied as needed
				{
					if (!ifstatement) BADSCRIPT((char*)"PATTERN-7 Must attach _ to next token. If you mean _ match, use escaped _. %s\r\n",ptr)
				}
				if (IsDigit(word[1])) // match variable
				{
					if (GetWildcardID(word) < 0) BADSCRIPT((char*)"PATTERN-8 _%d is max match reference - %s\r\n",MAX_WILDCARDS-1,word)
					break;
				}

				memorizeSeen = true;
				quoteSeen = false;
				if (comparison) *comparison = c;

				len = strlen(word) - 1 ;
				ptr -= len;  // the remnant
				strncpy(ptr,word+1, len); // this allows a function parameter (originally ^word but now ^0) to properly reset
				continue;
			case '\'': //   original (non-canonical) token - possessive must be \'s or \'
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-10 Cannot have two ' in succession\r\n")
				if (!word[1]) BADSCRIPT((char*)"PATTERN-11 Must attach ' to next token. If you mean ' match, use \' \r\n %s\r\n",ptr)
				quoteSeen = true;
				variableGapSeen = false;
				if (comparison) *comparison = c;
				
				len = strlen(word) - 1 ;
				ptr -= len;  // the remnant
				strncpy(ptr,word+1, len); // this allows a function parameter (originally ^word but now ^0) to properly reset
				continue;
			case '<':	//   sentence start <  or  unordered start <<
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-12 Cannot use ' before < or <<\r\n")
				if (memorizeSeen) BADSCRIPT((char*)"PATTERN-13 Cannot use _ before < or <<\r\n")
				if (word[1] == '<')  //   <<  unordered start
				{
					if (memorizeSeen) BADSCRIPT((char*)"PATTERN-14 Cannot use _ before << \r\n")
					if (nestKind[nestIndex-1] == '<') BADSCRIPT((char*)"PATTERN-15 << already in progress\r\n")
					if (variableGapSeen) BADSCRIPT((char*)"PATTERN-16 Cannot use * before <<\r\n")
                    nestLine[nestIndex] = currentFileLine;
                    nestKind[nestIndex++] = '<';
				}
				else if (word[1])  BADSCRIPT((char*)"PATTERN-17 %s cannot start with <\r\n",word)
				variableGapSeen = false;
				break; 
			case '@': 
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-18 Quoting @ is meaningless.\r\n");
				if (memorizeSeen) BADSCRIPT((char*)"PATTERN-19 Cannot use _ before  @\r\n")
				if (word[1] == '_') // set match position  @_5
				{
					if (GetWildcardID(word+1) >= MAX_WILDCARDS) 
						BADSCRIPT((char*)"PATTERN-? %s is not a valid positional reference - must be < %d\r\n",word,MAX_WILDCARDS) 
					char* end = word + 3; 
					while (IsDigit(*end)) ++end;
					if (*end)
					{
						if (*end == '+' && (!end[1] || end[1] == 'i')) {;}
						else if (*end == '-' &&  (!end[1] || end[1] == 'i')) {;}
						else  BADSCRIPT((char*)"PATTERN-? %s is not a valid positional reference - @_2+ or @_2- or @_2 would be\r\n",word)  
					}
					variableGapSeen = false; // no longer after anything. we are changing direction
				}
				else if (GetSetID(word) < 0)
					BADSCRIPT((char*)"PATTERN-20 %s is not a valid factset reference\r\n",word)  // factset reference
				else if (!GetSetMod(word)) 
					BADSCRIPT((char*)"PATTERN-20 %s is not a valid factset reference\r\n",word)  // factset reference
				break;
			case '>':	//   sentence end > or unordered end >>
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-21 Cannot use ' before > or >>\r\n")
				if (word[1] == '>') //   >>
				{
                    
					if (memorizeSeen) BADSCRIPT((char*)"PATTERN-22 Cannot use _ before  >> \r\n")
					if (nestKind[nestIndex - 1] != '<') BADSCRIPT((char*)"PATTERN-23 Have no << in progress\r\n");
					if (variableGapSeen) BADSCRIPT((char*)"PATTERN-24 Cannot use wildcard inside >>\r\n")
                    if (nestKind[--nestIndex] != '<') BADSCRIPT((char*)"PATTERN-24 >> should be closing %c started at line %d\r\n", nestKind[nestIndex],nestLine[nestIndex])

				}
				variableGapSeen = false;
				break; //   sentence end align
			case '(':	//   sequential pattern unit begin
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-25 Quoting ( is meaningless.\r\n");
                nestLine[nestIndex] = currentFileLine;
                nestKind[nestIndex++] = '(';
				break;
			case ')':	//   sequential pattern unit end
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-26 Quoting ) is meaningless.\r\n");
				if (memorizeSeen && !ifstatement) BADSCRIPT((char*)"PATTERN-27 Cannot use _ before  )\r\n")
				if (variableGapSeen && nestIndex > 1) 
					BADSCRIPT((char*)"PATTERN-26 Cannot have wildcard followed by )\r\n")
				if (nestKind[--nestIndex] != '(') 
					BADSCRIPT((char*)"PATTERN-9 ) should be closing %c started at line %d\r\n", nestKind[nestIndex], nestLine[nestIndex])
				break;
			case '[':	//   list of pattern choices begin
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-30 Quoting [ is meaningless.\r\n");
                nestLine[nestIndex] = currentFileLine;
                nestKind[nestIndex++] = '[';
				break;
			case ']':	//   list of pattern choices end
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-31 Quoting ] is meaningless.\r\n");
				if (memorizeSeen) BADSCRIPT((char*)"PATTERN-32 Cannot use _ before  ]\r\n")
				if (variableGapSeen) BADSCRIPT((char*)"PATTERN-33 Cannot have wildcard followed by ]\r\n")
				if (nestKind[--nestIndex] != '[')
					BADSCRIPT((char*)"PATTERN-34 ] should be closing %c started at line %d\r\n", nestKind[nestIndex], nestLine[nestIndex])
				break;
			case '{':	//   list of optional choices begins
				if (variableGapSeen)
				{
					// if we can see end of } and it has a gap after it... thats a problem - two gaps in succession is the equivalent
					char* end = strchr(ptr,'}');
					if (end)
					{
						end = SkipWhitespace(end);
						if (*end == '*') WARNSCRIPT((char*)"Wildcard before and after optional will probably not work since wildcards wont know where to end if optional fails. Use some other formulation\r\n")
					}
				}
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-35 Quoting { is meaningless.\r\n");
				if (notSeen)  BADSCRIPT((char*)"PATTERN-36 !{ is pointless since { can fail or not anyway\r\n")
				if (nestIndex && nestKind[nestIndex-1] == '{') BADSCRIPT((char*)"PATTERN-37 {{ is illegal\r\n")
                nestLine[nestIndex] = currentFileLine;
                nestKind[nestIndex++] = '{';
				break;
			case '}':	//   list of optional choices ends
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-38 Quoting } is meaningless.\r\n");
				if (memorizeSeen) BADSCRIPT((char*)"PATTERN-39 Cannot use _ before  }\r\n")
				if (variableGapSeen) BADSCRIPT((char*)"PATTERN-40 Cannot have wildcard followed by }\r\n")
				if (nestKind[--nestIndex] != '{') BADSCRIPT((char*)"PATTERN-41 } should be closing %c started at line %d\r\n", nestKind[nestIndex], nestLine[nestIndex])
				break;
			case '\\': //   literal next character
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-42 Quoting an escape is meaningless.\r\n");
				if (!word[1]) BADSCRIPT((char*)"PATTERN-43 Backslash must be joined to something to escape\r\n")
				variableGapSeen = false;
				if (word[1] && IsAlphaUTF8(word[1] )) memmove(word,word+1,strlen(word)); // escaping a real word, just use it
				break;
			case '*': //   gap: * *1 *~2 	(infinite, exactly 1 word, 0-2 words, 0-2 words, 1 word before) and *alpha*x* is form match
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-44 Quoting a wildcard\r\n");
                if (nestKind[nestIndex - 1] == '<') BADSCRIPT((char*)"PATTERN-45 Can not have wildcard %s inside << >>\r\n", word)
                if (nestKind[nestIndex-1] != '(' && (word[1] == '~' || !word[1])) BADSCRIPT((char*)"PATTERN-45 Can only have variable wildcard %s inside ( )\r\n",word)
				if (variableGapSeen) 
					BADSCRIPT((char*)"PATTERN-46 Cannot have wildcard followed by %s\r\n",word)
				if (IsAlphaUTF8(word[1]) )
					break; // find this word as fragmented spelling like sch*ding* since it will have * as a prefix
				
				// gaps of various flavors
				if (notSeen)  BADSCRIPT((char*)"PATTERN-47 cannot have ! before gap - %s\r\n",word)
				if (IsDigit(word[1])) //   enumerated gap size
				{
					int n = word[1] - '0';
					if (n == 0) BADSCRIPT((char*)"PATTERN-48 *0 is meaningless\r\n")	 
					if (word[2]) BADSCRIPT((char*)"PATTERN-49 *9 is the largest gap allowed or bad stuff is stuck to your token- %s\r\n",word)
				}
				else if (word[1] == '-') // backwards
				{
					int n = word[2] - '0';
					if (n == 0) BADSCRIPT((char*)"PATTERN-50 *-1 is the smallest backward wildcard allowed - %s\r\n",word)
					if (word[3]) BADSCRIPT((char*)"PATTERN-51 *-9 is the largest backward wildcard or bad stuff is stuck to your token- %s\r\n",word)
				}
				else if (word[1] == '~') // close-range gap
				{
					if (nestKind[nestIndex-1] == '{' || nestKind[nestIndex-1] == '[')
						BADSCRIPT((char*)"PATTERN-5? cannot stick %s wildcard inside {} or []\r\n",word)
					variableGapSeen = true;
					int n = word[2] - '0';
					if (!word[2]) BADSCRIPT((char*)"PATTERN-52 *~ is not legal, you need a digit after it\r\n")
					else if (n == 0) BADSCRIPT((char*)"PATTERN-53 *~1 is the smallest close-range gap - %s\r\n",word)
					else if (word[3] && word[3] != 'b') BADSCRIPT((char*)"PATTERN-54 *~9 is the largest close-range gap or bad stuff is stuck to your token- %s\r\n",word)
				}
				else if (word[1]) BADSCRIPT((char*)"PATTERN-55 * jammed against some other token- %s\r\n",word)
				else 
				{
					if (nestKind[nestIndex-1] == '{' || nestKind[nestIndex-1] == '[')
						BADSCRIPT((char*)"PATTERN-5? cannot stick * wildcard inside {} or []\r\n")
					variableGapSeen = true; // std * unlimited wildcard
				}
				startSeen = true;
				break;
			case '?': //   question input ?   
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-56 Quoting a ? is meaningless.\r\n");
				if (memorizeSeen && word[1] != '$') BADSCRIPT((char*)"PATTERN-57 Cannot use _ before ?\r\n")
				if (variableGapSeen) BADSCRIPT((char*)"PATTERN-58 Cannot have wildcards before ?\r\n")
				break;
			case USERVAR_PREFIX:	//   user var
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-59 Quoting a $ variable is meaningless - %s\r\n",word);
				variableGapSeen = false;
				break;
			case '"': //   string
				{
					// you can quote a string, because you are quoting its members
					variableGapSeen = false;
					strcpy(word,JoinWords(BurstWord(word,CONTRACTIONS)));// change from string to std token
					if (!livecall) 
					{
						WritePatternWord(word); 
						WriteKey(word);
					}
					unsigned int n = 0;
					char* ptr = word;
					while ((ptr = strchr(ptr,'_')))
					{
						++n;
						++ptr;
					}
					if (n >= SEQUENCE_LIMIT) BADSCRIPT((char*)"PATTERN-? Too many  words in string %s, will never match\r\n",word)
				}
				break;
			case SYSVAR_PREFIX: //   system data
				// you can quote system variables because %topic returns a topic name which can be quoted to query
				if (memorizeSeen) BADSCRIPT((char*)"PATTERN-60 Cannot use _ before system variable - %s\r\n",word)
				if (!word[1]); //   simple %
				else if (!FindWord(word)) BADSCRIPT((char*)"PATTERN-61 %s is not a system variable\r\n",word)
				if (comparison) *comparison = c;
				variableGapSeen = false;
				break;
			case '~':
				variableGapSeen = false;
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-61 cannot quote set %s because it can't be determined if set comes from original or canonical form\r\n",word)
				startSeen = true;
				if (!livecall) 
				{
					WriteKey(word);
					CheckSetOrTopic(word); // set or topic
				}
				break;
			default: //   normal token ( and anon function call)

				//    MERGE user pattern words into one? , e.g. drinking age == drinking_age in dictionary
				//   only in () sequence mode. Dont merge [old age] or {old age} or << old age >>
				if (nestKind[nestIndex-1] == '(') //   BUG- do we need to see what triples etc wordnet has
				{ // dont want pattern may 9 to merge
					ReadNextSystemToken(in,ptr,nextToken,true,true); 
					WORDP F = FindWord(word);
					WORDP E = FindWord(nextToken);
					if (E && F && E->properties & PART_OF_SPEECH  && F->properties & PART_OF_SPEECH)
					{
						char join[MAX_WORD_SIZE];
						sprintf(join,(char*)"%s_%s",word,nextToken);
						E = FindWord(join,0,PRIMARY_CASE_ALLOWED); // must be direct match
						if (E && E->properties & PART_OF_SPEECH && !IS_NEW_WORD(E)) // change to composite
						{
							strcpy(word,join);		//   joined word replaces it
							ptr = ReadNextSystemToken(in,ptr,nextToken,true,false); // swallow the lookahead
							*nextToken = 0;
						}
					}
				}
				variableGapSeen = false;
				startSeen = true;
				break;
		}

		if (comparison) //   is a comparison of some kind
		{
			if (memorizeSeen && comparison[1]) 
				 BADSCRIPT((char*)"PATTERN-57 Cannot use _ before a comparison\r\n")
			if (variableGapSeen) BADSCRIPT((char*)"PATTERN-16 Cannot use * before comparison since memorization will be incomplete\r\n")
			if (*word == USERVAR_PREFIX && word[1] == LOCALVAR_PREFIX)
			{
				char* dot = strchr(word,'.');
				if (dot) *word = 0;
				AddDisplay(word);
				if (dot) *word = '.';
			}
	 		*comparison = c;
			if (c == '!') // move not operator out in front of token
			{
				*data++ = '!';
				*data++ = ' '; // and space it, so if we see "!=shec?~hello" we wont think != is an operator, instead = is a jump infof

				size_t len = strlen(comparison);
				memmove(comparison,comparison+1,len);
			}
			if (quoteSeen && *word == '_' && IsDigit(word[1])) // quoted match variable
			{
				quoteSeen = false;
				memmove(word+1,word,strlen(word)+1);
				*word = '\'';
				++comparison;	 // moved over for the added ' 
			}

			char* rhs = comparison+1;
			if (*rhs == '=' || *rhs == '?') ++rhs;
			if (*rhs == '^' && IsAlphaUTF8(rhs[1])) BADSCRIPT((char*)"%s is not a current function variable\r\n",rhs);
			if (!*rhs && *word == USERVAR_PREFIX) {} // allowed member in sentence
			else if (!*rhs && *word == '_' && IsDigit(word[1])); // allowed member in sentence
			else if (*rhs == '#') // names a constant #define to replace with number value
			{
				uint64 n = FindValueByName(rhs+1);
				if (!n) n = FindSystemValueByName(rhs+1);
				if (!n) n = FindParseValueByName(rhs+1);
				if (!n) n = FindMiscValueByName(rhs+1);
				if (!n) BADSCRIPT((char*)"PATTERN-63 No #constant recognized - %s\r\n",rhs+1)
#ifdef WIN32
			sprintf(rhs,(char*)"%I64d",(long long int) n); 
#else
			sprintf(rhs,(char*)"%lld",(long long int) n); 
#endif	
			}
			else if (IsAlphaUTF8DigitNumeric(*rhs)  ) // LITERAL has no constraints
			{
		////		if (!livecall) 
		//		{
			//		WriteKey(rhs); 
			//		WritePatternWord(rhs);		//   ordinary token
			//	}
			}
			else if (*rhs == '~') 
			{
				MakeLowerCase(rhs);
				if (!livecall) CheckSetOrTopic(rhs);	
			}
			else if (*rhs == '_' || *rhs == '@');	// match variable or factset variable
			else if (*rhs == USERVAR_PREFIX) 
			{
				MakeLowerCase(rhs);	// user variable 
				if (rhs[1] == LOCALVAR_PREFIX) 
				{
					char* dot = strchr(rhs,'.');
					if (dot) *rhs = 0;
					AddDisplay(rhs);
					if (dot) *word = '.';
				}
			}
			else if (*rhs == SYSVAR_PREFIX) MakeLowerCase(rhs);	// system variable
			else if (*rhs == '^' && (rhs[1] == '_' || rhs[1] == USERVAR_PREFIX || IsDigit(rhs[1]))) MakeLowerCase(rhs); // indirect match variable or indirect user vaiable or function variable
			else if (!*rhs && *comparison == '?' && !comparison[1]);
			else if (*rhs == '\'' && (rhs[1] == USERVAR_PREFIX || rhs[1]== '_')); //   unevaled user variable or raw match variable
			else if (!comparison[2] && *word == USERVAR_PREFIX); // find in sentence
			else if (*rhs == '"' && rhs[strlen(rhs)-1] == '"'){;} // quoted string
			else BADSCRIPT((char*)"PATTERN-64 Illegal comparison %s or failed to close prior rule starting at %s\r\n",word, GetRuleElipsis(start))
			int len = (comparison - word) + 2; //   include the = and jump code in length

			//   rebuild token
			char tmp[MAX_WORD_SIZE];
			*tmp = '=';		//   comparison header
			if (len > 70) BADSCRIPT((char*)"PATTERN-65 Left side of comparison must not exceed 70 characters - %s\r\n",word)
			char* x = tmp+1;
			Encode(len,x,1);
			strcpy(tmp+2,word); //   copy left side over
			strcpy(word,tmp);	//   replace original token
		}
		else if (*word == '~') 
		{
			if (!livecall) CheckSetOrTopic(word); 
		}
		ReadNextSystemToken(in,ptr,nextToken,true,true);
		
		//   see if we have an implied call (he omitted the ^)
		if (*word != '^' && *nextToken == '(') //   looks like a call, reformat it if it is
		{
			char rename[MAX_WORD_SIZE];
			*rename = '^';
			strcpy(rename+1,word);	//   in case user omitted the ^
			WORDP D = FindWord(rename,0,LOWERCASE_LOOKUP);
			if (D && D->internalBits & FUNCTION_NAME) strcpy(word,D->word); //   a recognized call
		}
		if (*word == '^')   //   function call or function var ref or indirect function variable assign ref like ^$$tmp = null
		{
			if (quoteSeen) BADSCRIPT((char*)"PATTERN-? Cannot use quote before ^ function call or variable\r\n")
			if (notSeen) 
			{
				*data++ = '!';
				if (doubleNotSeen) *data++ = '!';
				doubleNotSeen = notSeen = false;
			}
			if (memorizeSeen) 
			{
				if (!IsDigit(word[1]) && word[1] != USERVAR_PREFIX) BADSCRIPT((char*)"PATTERN-66 Cannot use _ before ^ function call\r\n")
				*data++ = '_';
				memorizeSeen = false;
			}
			if (word[1] == USERVAR_PREFIX)
			{
				strcpy(data,word);
				data += strlen(data);
			}
			else 
			{
				ptr = ReadCall(word,ptr,in,data,*nextToken == '(',false);
				if (PatternRelationToken(ptr)) // immediate relation bound to call?
				{
					ptr = ReadNextSystemToken(in,ptr,word);
					strcpy(data,word);
					data += strlen(data);
				}
			}
			*data++ = ' ';
			continue;
		}
		
		//   put out the next token and space 
		if (notSeen) 
		{
			if (memorizeSeen) BADSCRIPT((char*)"PATTERN-67 Cannot have ! and _ together\r\n")
			*data++ = '!';
			if (doubleNotSeen) *data++ = '!';
			doubleNotSeen = notSeen = false;
		}
		if (quoteSeen) 
		{
			*data++ = '\'';
			quoteSeen = false;
		}
		if (memorizeSeen) 
		{
			*data++ = '_';
			if (ifstatement) *data++ = ' ';
			memorizeSeen = false;
		}

		if (IsAlphaUTF8(*word) || (*word == '*' &&  IsAlphaUTF8(word[1])) )
		{
			char* p;
			if ((p = strchr(word,'*'))) // wild word fragment?  reformat to have leading * and lower case the test
			{
				char hold[MAX_WORD_SIZE];
				MakeLowerCopy(hold,word);
				*word = '*';
				strcpy(word+1,hold);
			}
			else // ordinary word - break off possessives as needed
			{
				size_t len = strlen(word);
				unsigned int ignore = 0;
				if (len > 1 && word[len-1] == '\'' && word[len-2] != '_') // ending ' possessive plural
				{
					if (!livecall)
					{
						if (ifstatement && !strcmp(word,"PATTERN")) {;} // allow uppercase
						else
						{
							WritePatternWord(word);
							WriteKey(word);
						}
					}
					word[--len] = 0;
					ignore = 1;
				}
				else if (len > 2 && word[len-1] == 's' && word[len-2] == '\'' && word[len-3] != '_') // ending 's possessive singular
				{
					if (!livecall)
					{
						WriteKey(word);
						WritePatternWord(word);
					}
					len -= 2;
					word[len] = 0;
					ignore = 2;
				}
				strcpy(word,JoinWords(BurstWord(word,CONTRACTIONS))); // change to std token
				if (!livecall && spellCheck && !(spellCheck & NO_SPELL)) SpellCheckScriptWord(word,startSeen ? 1 : 0,false);
				if (!livecall && strcmp(word,"PATTERN"))
				{
					WriteKey(word); 
					WritePatternWord(word); //   memorize it to know its important
				}
				if (ignore)
				{
					strcpy(data,word);
					data += strlen(data);
					*data++ = '_';	
					if (ignore == 1) strcpy(word,(char*)"'");
					else strcpy(word,(char*)"'s");
				}
			}
		}

		strcpy(data,word);
		data += strlen(data);
		*data++ = ' ';	

		if (nestIndex == 0) break; //   we completed this level
	}
	*data = 0;

	//   leftovers?
	if (macro && nestIndex != 1) 
		BADSCRIPT((char*)"PATTERN-68 Failed to balance ( or [ or { properly in macro for %s\r\n",startPattern)
	else if (!macro && nestIndex != 0) 
		BADSCRIPT((char*)"PATTERN-69 Failed to close %c started at line %d : %s\r\n",nestKind[nestIndex-1],nestLine[nestIndex-1],startPattern);

	patternContext = false;
	return ptr;
}

static char* ReadChoice(char* word, char* ptr, FILE* in, char* &data,char* rejoinders)
{	//   returns the stored data, not the ptr, starts with the [
	*data++ = '[';
	*data++ = ' ';
	ReadNextSystemToken(in,ptr,word,true); // get possible rejoinder label
	if (word[1] == ':' && !word[2]) // is rejoinder label
	{
		if (*word < 'a' || *word >= 'q') BADSCRIPT((char*)"CHOICE-1 Bad level label %s in [ ]\r\n",word)
		if (rejoinders) rejoinders[(int)(*word - 'a' + 1)] = 2; //   authorized level
		*data++ = *word;
		*data++ = word[1];
		*data++ = ' ';
		ptr = ReadNextSystemToken(in,ptr,word,false);
	}
	ptr = ReadOutput(false,true,ptr,in,data,rejoinders,NULL,NULL,true);
	*data = 0;
	return ptr;
}

static bool ValidIfOperand(char c)
{
    return (c != '<' && c != '+' && c != '-' && c != '*'
        && c != '/' && c != '&' && c != '|' && c != '%' && c != '='
        && c != '>' && c != '^' && c != '!' && c != '?');
}

static char* ReadIfTest(char* ptr, FILE* in, char* &data)
{
    priorLine = currentFileLine;
	char word[MAX_WORD_SIZE];
	int paren = 1;
	//   test is either a function call OR an equality comparison OR an IN relation OR an existence test
	//   the followup will be either (or  < > ==  or  IN  or )
	//   The function call returns a status code, you cant do comparison on it
	//   but both function and existence can be notted- IF (!$var) or IF (!read(xx))
	//   You can have multiple tests, separated by AND and OR.
	pattern: 
	ptr = ReadNextSystemToken(in,ptr,word,false,false); 
    if (priorLine != currentFileLine)
    {
        AddMapOutput(priorLine);
        priorLine = currentFileLine;
    }
    if (*word == '~') CheckSetOrTopic(word);
	// separate ! from things if not  != and !?
	if (*word == '!' && word[1] && word[1] != '=' && word[1] != '?') 
	{
		while (*--ptr != '!' && *ptr);
		++ptr;
		word[1] = 0;
	}
    // actually a test joined on?
    char* at = word;
    while (*++at && ValidIfOperand(*at)) {;} // never look at first character
	if (*at)
	{
        size_t len = strlen(at);
		if (*at == '-' && *word == '$') {;} // $atat-ata could be subtract or name, but cannot be subtract in if test
		else if (*at == '^' && *word == '\'') {;} // '^arg is not an operator
		else
		{
			*at = 0;
			ptr -= len; // back up to rescan
			at = ptr;
			while (!ValidIfOperand(*at)) {++at;} // where does operand end?
			memmove(at + 1, at, strlen(at)+1);
			*at = ' '; // separate operand
		}
	}

	bool notted = false;
	if (*word == '!' && !word[1]) 
	{
		notted = true;
		*data++ = '!';
		*data++ = ' ';
		ptr = ReadNextSystemToken(in,ptr,word,false,false); 
        if (priorLine != currentFileLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
    }
	if (*word == '\'' && !word[1]) 
	{
		*data++ = '\'';
		ptr = ReadNextSystemToken(in,ptr,word,false,false); 
        if (priorLine != currentFileLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
        if (*word != '_' && *word != '^')
			BADSCRIPT((char*)"IF-3 Can only quote _matchvar (or functionvar of one) in IF test\r\n")
	}
	if (*word == '!') BADSCRIPT((char*)"IF-4 Cannot do two ! in a row\r\n")
	ReadNextSystemToken(in,ptr,nextToken,false,true); 
    if (priorLine != currentFileLine)
    {
        AddMapOutput(priorLine);
        priorLine = currentFileLine;
    }
    MakeLowerCase(nextToken);
	
	if (*nextToken != '(' && *word == '^' && word[1] != '"' && IsAlphaUTF8(word[1])) 
        BADSCRIPT((char*)"%s is not the name of a local function argument\r\n",word)
	if (*nextToken == '(')  // function call?
	{
		if (*word != '^') //     a call w/o its ^
		{
			char rename[MAX_WORD_SIZE];
			*rename = '^';
			strcpy(rename+1,word);	//   in case user omitted the ^
			strcpy(word,rename);
		}
		ptr = ReadCall(word,ptr,in,data,true,false);  //   read call
		ReadNextSystemToken(in,ptr,nextToken,false,true); 
        if (priorLine != currentFileLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
        if (RelationToken(nextToken))
		{
			if (notted) BADSCRIPT((char*)"IF-5 cannot do ! in front of comparison %s\r\n",nextToken)
			*data++ = ' ';
			ptr =  ReadNextSystemToken(in,ptr,word,false,false); //   swallow operator
            if (priorLine != currentFileLine)
            {
                AddMapOutput(priorLine);
                priorLine = currentFileLine;
            }
            strcpy(data,word);
			data += strlen(word);
			*data++ = ' ';
			ptr =  ReadNextSystemToken(in,ptr,word,false,false); //   swallow value
            if (priorLine != currentFileLine)
            {
                AddMapOutput(priorLine);
                priorLine = currentFileLine;
            }
            strcpy(data,word);
			data += strlen(word);
		}
	}
	else if (*nextToken == '!' && nextToken[1] == '?')
	{
		if (notted) BADSCRIPT((char*)"IF-6 cannot do ! in front of query %s\r\n",nextToken)
		if (*word == '\'' && word[1] == '_') {;}
		else if (*word != '@' &&*word != USERVAR_PREFIX && *word != '_' && *word != '^' && *word != SYSVAR_PREFIX) 
			BADSCRIPT((char*)"IF test query must be with $var, _# or '_#, %sysvar, @1subject or ^fnarg -%s\r\n",word)
		strcpy(data,word);
		data += strlen(word);
		*data++ = ' ';
		ptr =  ReadNextSystemToken(in,ptr,word,false,false); //   swallow operator
		strcpy(data,word);
		data += strlen(word);
		*data++ = ' ';
		ptr =  ReadNextSystemToken(in,ptr,word,false,false); //   swallow value
		if (*word == '^' && !IsDigit(word[1]))  BADSCRIPT((char*)"IF-7 not allowed 2nd function call in relation - %s\r\n",word)
		if (*word == '~') CheckSetOrTopic(word); 
		strcpy(data,word);
		data += strlen(word);
	}
	else if (RelationToken(nextToken))
	{
		if (notted && *nextToken != '?') BADSCRIPT((char*)"IF-8 cannot do ! in front of comparison %s\r\n",nextToken)
		if (*word == '\'' && ((word[1] == '^' && IsDigit(word[2])) || word[1] == USERVAR_PREFIX || word[1] == '_')) {;} // quoted variable
		else if (*word != '@' && *word != USERVAR_PREFIX && *word != '_' && *word != '^' && *word != SYSVAR_PREFIX && !IsAlphaUTF8(*word)  && !IsDigit(*word) && *word != '+' && *word != '-') 
			BADSCRIPT((char*)"IF test comparison 1st value must be number, word, $var, _#, sysvar, @1subject or ^fnarg -%s\r\n",word)
		strcpy(data,word);
		data += strlen(word);
		*data++ = ' ';
		ptr =  ReadNextSystemToken(in,ptr,word,false,false); //   swallow operator
		strcpy(data,word);
		data += strlen(word);
		*data++ = ' ';
		ptr =  ReadNextSystemToken(in,ptr,word,false,false); //   swallow value
		if (*word == '~') CheckSetOrTopic(word);
		if (*word == '^' && !IsDigit(word[1])) 
			BADSCRIPT((char*)"IF-9 not allowed function call or active string in relation as 2nd arg - %s\r\n",word)
		strcpy(data,word);
		data += strlen(word);
	}
	else if (*nextToken == ')' || !stricmp(nextToken,(char*)"and") || !stricmp(nextToken, (char*)"&") ||  !stricmp(nextToken,(char*)"or")) //   existence test
	{
		if (*word != USERVAR_PREFIX && *word != '_' && *word != '@' && *word != '^'  && *word != SYSVAR_PREFIX && *word != '?' ) 
			BADSCRIPT((char*)"IF-10 existence test - %s. Must be uservar or systemvar or _#  or ? or @# or ~concept or ^^var \r\n",word)
		strcpy(data,word);
		data += strlen(word);
	}
	else BADSCRIPT((char*)"IF-11 illegal test %s %s . Use (X > Y) or (Foo()) or (X IN Y) or ($var) or (_3)\r\n",word,nextToken) 
	*data++ = ' ';
	
	//   check for close or more conditions
	ptr =  ReadNextSystemToken(in,ptr,word,false,false); //   )
	if (*word == '~') CheckSetOrTopic(word); 
	if (*word == ')')
	{
		*data++ = ')';
		*data++ = ' ';
	}
	else if (!stricmp(word,(char*)"or") || !stricmp(word,(char*)"and") || !stricmp(word, (char*)"&"))
	{
		MakeLowerCopy(data,word);
		data += strlen(word);
		*data++ = ' ';
		goto pattern;	//   handle next element
	}
	else BADSCRIPT((char*)"IF-12 comparison must close with ) -%s .. Did you make a function call as 1st argument? that's illegal\r\n",word)
	*data = 0;
	return ptr;
}

static char* ReadBody(char* word, char* ptr, FILE* in, char* &data,char* rejoinders)
{	//    stored data starts with the {
	char* start = data;
	ptr = ReadOutput(false,true,ptr,in,data,rejoinders,NULL,NULL); 
	size_t len = strlen(start);
	if ((len + 3) >= maxBufferSize) BADSCRIPT((char*)"BODY-4 Body exceeding limit of %d bytes\r\n",maxBufferSize)
	return ptr;
}

#ifdef INFORMATION

An IF consists of:
	if (test-condition code) xx
	{body code} yy
	else (test-condition code) xx
	{body code} yy
	else (1) xx
	{body code} yy
	
spot yy is offset to end of entire if and xx if offset to next branch of if before "else".

#endif

static char* ReadIf(char* word, char* ptr, FILE* in, char* &data,char* rejoinders)
{
	char* bodyends[1000];				//   places to patch for jumps
	unsigned int bodyendIndex = 0;
	char* original = data;
	strcpy(data,(char*)"^if ");
	data += 4;
    if (mapFile && dataBase)
    {
        fprintf(mapFile, (char*)"          if %d %d  \r\n", currentFileLine, (int)(data - dataBase)); // readBuffer
    }

	patternContext = false;
	++complexity;
    priorLine = currentFileLine;
	while (ALWAYS)
	{
		char* testbase = data;
		*data++ = 'a'; //   reserve space for offset past pattern
		*data++ = 'a'; // next will be (
		*data++ = 'a';
		ptr = ReadNextSystemToken(in,ptr,word,false); //   the '('
        if (priorLine != currentFileLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
        MakeLowerCopy(lowercaseForm,word);
		if (!*word || TopLevelUnit(word) || TopLevelRule(lowercaseForm) || Rejoinder(lowercaseForm)) BADSCRIPT((char*)"IF-1 Incomplete IF statement - %s\r\n",word)
		if (*word != '(') BADSCRIPT((char*)"IF-2 Missing (for IF test - %s\r\n",word)
		*data++ = '(';
		*data++ = ' ';

		if (!strnicmp(ptr,(char*)"pattern ",7))
		{
			ptr = ReadNextSystemToken(in,ptr,word,false); //   pattern
            if (priorLine != currentFileLine)
            {
                AddMapOutput(priorLine);
                priorLine = currentFileLine;
            }
            strcpy(data,word);
			data += strlen(data);
			*data++ = ' ';
			char* original = data;
			patternContext = true;
			// swallow pattern
			ptr = ReadPattern(ptr,in,data,false,true); //   read ( for real in the paren for pattern
            if (priorLine != currentFileLine)
            {
                AddMapOutput(priorLine);
                priorLine = currentFileLine;
            }
            patternContext = false;
		}
		else 
		{
			ptr = ReadIfTest(ptr, in, data); // starts by reading the ( and ends having read )
		}
		Encode((unsigned int)(data-testbase),testbase);	// offset to after pattern    
	//	Encode(xcounter,data,2);
		//--- format:  branch to after pattern, pattern, branch around next pattern, pattern, branch around next pattern or to end of if code
	
		char* ifbase = data;
		*data++ = 'a'; //   reserve space for offset after the closing ), which is how far to go past body
		*data++ = 'a';
		*data++ = 'a';

		//   swallow body of IF after test --  must have { surrounding now
		ReadNextSystemToken(in,ptr,word,false,true); //   {
        if (priorLine != currentFileLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
        if (*word != '{')
		{
			char hold[MAX_WORD_SIZE];
			strcpy(hold,word);
			ptr = ReadNextSystemToken(in,ptr,word,false,false); 
			ReadNextSystemToken(in,ptr,word,false,true); 
            if (priorLine != currentFileLine)
            {
                AddMapOutput(priorLine);
                priorLine = currentFileLine;
            }
            if (*word != '{') BADSCRIPT((char*)"IF-13 body must start with { instead of %s  -- saw pattern %s\r\n",word,readBuffer,original)
		}
		ptr = ReadBody(word,ptr,in,data,rejoinders); // comes with space after it
		bodyends[bodyendIndex++] = data; //   jump offset to end of if (backpatched)
		DummyEncode(data); //   reserve space for offset after the closing ), which is how far to go past body
		*data++ = ' ';
		Encode((unsigned int)(data-ifbase),ifbase);	// offset to ELSE or ELSE IF from body start 
			
		//   now see if ELSE branch exists
		ReadNextSystemToken(in,ptr,word,false,true); //   else?
        if (priorLine != currentFileLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
        if (stricmp(word,(char*)"else"))  break; //   caller will add space after our jump index

		//   there is either else if or else
		ptr = ReadNextSystemToken(in,ptr,word,false,false); //   swallow the else
        if (priorLine != currentFileLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
        strcpy(data,(char*)"else ");
		data += 5;

		ReadNextSystemToken(in,ptr,word,false,true); //   see if or {
        if (priorLine != currentFileLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
        if (mapFile && dataBase)
        {
            if (!stricmp(word,"if")) fprintf(mapFile, (char*)"          elseif %d %d  \r\n", currentFileLine, (int)(data - dataBase)); // readBuffer
            else fprintf(mapFile, (char*)"          else %d %d  \r\n", currentFileLine, (int)(data - dataBase)); // readBuffer
        }
        if (*word == '{') //   swallow the ELSE body now since no IF - add fake successful test
		{
			//   successful test condition for else
			*data++ = '(';
			*data++ = ' ';
			*data++ = '1';
			*data++ = ' ';
			*data++ = ')';
			*data++ = ' ';

			ifbase = data; 
			DummyEncode(data);//   reserve skip data
			*data++ = ' ';
			ptr = ReadBody(word,ptr,in,data,rejoinders);
			bodyends[bodyendIndex++] = data; //   jump offset to end of if (backpatched)
			DummyEncode(data);//   reserve space for offset after the closing ), which is how far to go past body
			Encode((unsigned int)(data-ifbase),ifbase);	// offset to ELSE or ELSE IF from body start (accelerator)
			break;
		}
		else ++complexity;
		ptr = ReadNextSystemToken(in,ptr,word,false,false); //   eat the IF
        if (priorLine != currentFileLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
    }
	if (*(data-1) == ' ') --data;	//   remove excess blank
	patternContext = false;
    if (mapFile)
    {
        fprintf(mapFile, (char*)"          ifend %d %d  \r\n", currentFileLine, (int)(data - dataBase)); // readBuffer
    }
	//   store offsets from successful bodies to the end
	while (bodyendIndex != 0)
	{
		char* at = bodyends[--bodyendIndex];
		Encode((unsigned int)(data-at+1),at); // accerators on completion of if to end of whole if
	}

	*data = 0;
	return ptr; //   we return with no extra space after us, caller adds it
}

static char* ReadLoop(char* word, char* ptr, FILE* in, char* &data,char* rejoinders)
{
    priorLine = currentFileLine;
    char* original = data;
	strcpy(data,(char*)"^loop ");
	data += 6;
    if (mapFile && dataBase)
    {
        fprintf(mapFile, (char*)"          loop %d %d  \r\n", currentFileLine, (int)(data - dataBase)); // readBuffer
    }

	ptr = ReadNextSystemToken(in,ptr,word,false,false); //   (
    if (priorLine != currentFileLine)
    {
        AddMapOutput(priorLine);
        priorLine = currentFileLine;
    }
      *data++ = '(';
	*data++ = ' ';
	if (*word != '(') BADSCRIPT((char*)"LOOP-1 count must be ()  or (count) -%s\r\n",word)
	ptr = ReadNextSystemToken(in,ptr,word,false,false); //   counter - 
    if (priorLine != currentFileLine)
    {
        AddMapOutput(priorLine);
        priorLine = currentFileLine;
    }
    if (*word == '^'  && IsAlphaUTF8(word[1]))
    {
        WORDP D = FindWord(word, 0, LOWERCASE_LOOKUP);
        if (!D || !(D->internalBits & FUNCTION_NAME))
            BADSCRIPT((char*)"%s is not the name of a local function argument\r\n", word)
            ReadNextSystemToken(in, ptr, nextToken, false, true);
            ptr = ReadCall(word, ptr, in, data, *nextToken == '(', false); //   add function call
            ptr = ReadNextSystemToken(in, ptr, word, false, false);
            if (priorLine != currentFileLine)
            {
                AddMapOutput(priorLine);
                priorLine = currentFileLine;
            }
    }
	else if (*word == ')')  strcpy(data,(char*)"-1"); //   omitted, use -1
	else if (!stricmp(word,(char*)"-1")) // precompiled previously -1
	{
		strcpy(data,word);
		ptr = ReadNextSystemToken(in,ptr,word,false,false); // read closing paren
		if (*word != ')') BADSCRIPT((char*)"Loop counter %s was not closed by )\r\n",word);
	}
	else if (!IsDigit(*word) && *word != USERVAR_PREFIX && *word != '_' && *word != SYSVAR_PREFIX  && *word != '^'  && *word != '@') 
		BADSCRIPT((char*)"LOOP-2 counter must be $var, _#, %var, @factset or ^fnarg or function call -%s",word)
	else 
	{
		strcpy(data,word);
		ptr = ReadNextSystemToken(in,ptr,word,false,false);
        if (priorLine != currentFileLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
    }
	data += strlen(data);
	*data++ = ' ';
	if (*word != ')') 
        BADSCRIPT((char*)"LOOP-3 counter must end with )  -%s\r\n",word)
	*data++ = ')';
	*data++ = ' ';
	char* loopstart = data;
	DummyEncode(data);  // reserve loop jump to end accelerator
	*data++ = ' ';
//	Encode(loopCounter,data,2);

	//   now do body
	ReadNextSystemToken(in,ptr,word,false,true); 
    if (priorLine != currentFileLine)
    {
        AddMapOutput(priorLine);
        priorLine = currentFileLine;
    }
    if (*word != '{') // does it have precompiled accelerator
	{
		char hold[MAX_WORD_SIZE];
		strcpy(hold,word);
		ptr = ReadNextSystemToken(in,ptr,word,false,false); 
		ReadNextSystemToken(in,ptr,word,false,true); 
        if (priorLine != currentFileLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
        if (*word != '{') BADSCRIPT((char*)"LOOP-4 body must start with { -%s\r\n",hold)
	}
	char* bodystart = data;
	ptr = ReadBody(word,ptr,in,data,rejoinders);
	if (bodystart[0] == '{' && bodystart[1] == ' ' && bodystart[2] == '}') 
		BADSCRIPT((char*)"LOOP-4 body makes no sense being empty\r\n")
	Encode((unsigned int)(data - loopstart),loopstart);	// offset to body end from body start (accelerator)
	*data = 0;
    if (mapFile )
    {
        fprintf(mapFile, (char*)"          loopend %d %d  \r\n", currentFileLine, (int)(data - dataBase)); // readBuffer
    }
	return ptr; // caller adds extra space after
}

static char* ReadJavaScript(FILE* in, char* &data,char* ptr)
{
	strcpy(data,"*JavaScript");
	data += strlen(data);
	*data++ = ' ';
	strcpy(data,ptr);
	data += strlen(data);
	char word[MAX_WORD_SIZE];
	while (ReadALine(readBuffer,in) >= 0)
	{
		char* comment = strstr(readBuffer,"//");
		if (comment) *comment = 0;	// erase comments to end of line
		if (strstr(readBuffer,"/*")) BADSCRIPT("Cannot use /* ... */ comments in CS JavaScript: %s\r\n", readBuffer);
		char* ptr = SkipWhitespace(readBuffer);
		if (!*ptr) continue;
		ReadCompiledWord(ptr,word);
		if (TopLevelUnit(word) || !stricmp(word,(char*)"datum:"))  break;
		*data++ = ' ';
		strcpy(data,ptr);
		data += strlen(data);
	}

	return readBuffer;
}

char* ReadOutput(bool optionalBrace,bool nested,char* ptr, FILE* in,char* &mydata,char* rejoinders,char* supplement,WORDP call, bool choice)
{
    priorLine = currentFileLine;
	char* originalptr = ptr;
	char* oldOutputStart = outputStart; // does not matter if script error grabs control
	dataChunk = mydata; // global visible for use when changing lines for mapping
	if (!nested)
	{
		lineStart = dataChunk; // where line begins
		outputStart = dataChunk;
	}
	char* original = dataChunk;
	*dataChunk = 0;
	int bracket = 0;
	int paren = 0;
	int squiggle = 0;
	char* startparen = 0;
	char* startsquiggle = 0;
	char* startbracket = 0;
	int startpline = 0;
	int startbline = 0;
	int startsline = 0;

	char word[MAX_WORD_SIZE * 4];
	char assignlhs[MAX_WORD_SIZE];
	*assignlhs = 0;
	*assignKind = 0;
	int level = 0;
	int insert = 0;
	bool oldContext = patternContext;
	patternContext = false;
	char hold[MAX_WORD_SIZE];
	*hold = 0;
	char startkind = 0;
	if (choice)
	{
		startkind = '[';
		++level;
		startbracket = ptr;
		startbline = currentFileLine;
	}
	bool start = true;
	bool needtofield = false;
	bool javascript = false;
	while (ALWAYS) //   read as many tokens as needed to complete the responder definition
	{
		if ((dataChunk-original) >= MAX_JUMP_OFFSET) 
			BADSCRIPT((char*)"OUTPUT-1 code exceeds size limit of %d bytes\r\n",MAX_JUMP_OFFSET)

		if (*hold) // pending assignment code
		{
			if (*hold == '=')
			{
				strcpy(word,(char*)"=");
				memmove(hold,hold+1,strlen(hold));
			}
			else
			{
				strcpy(word,hold);
				*hold = 0;
			}
		}
		else if (supplement && *supplement)
		{
			strcpy(word,supplement);
			supplement = NULL;
		}
		else ptr = ReadNextSystemToken(in,ptr,word,false); 
		if (!*word)  break; //   end of file

        if (currentFileLine != priorLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
		if (start && !stricmp(word,"javascript"))
		{
			ptr = ReadJavaScript(in,dataChunk,ptr);
			javascript = true;
			break;
		}

		if (*word == USERVAR_PREFIX) // jammed together asignment?
		{
			char* assign = strchr(word,'=');
			if (assign)
			{
				strcpy(hold,assign);
				*assign = 0;
			}
		}
		if (insert) --insert;
		MakeLowerCopy(lowercaseForm,word);
		if (*word == '#' && word[1] == '!')  //   special comment
		{
			ptr -= strlen(word); //   let someone else see this  also  // safe
			break; 
		}
		if (*word == 'a' && word[2] == 0 && (word[1] == ';' || word[1] == '"' || word[1] == '\'' ) ) 
			WARNSCRIPT((char*)"Is %s supposed to be a rejoinder marker?\r\n",word,currentFilename);

		if ((*word == '}' && level == 0) || TopLevelUnit(word) || TopLevelRule(lowercaseForm) || Rejoinder(lowercaseForm) || !stricmp(word,(char*)"datum:")) //   responder definition ends when another major unit or top level responder starts
		{
			if (*word != ':') // allow commands here 
			{
				ptr -= strlen(word); //   let someone else see this starter also  // safe
				break; 
			}
			else if (level >= 1)
			{
				if (startkind == '[') BADSCRIPT((char*)"CHOICE-2 Fail to close code started with %s upon seeing %s\r\n", originalptr,word)
				else BADSCRIPT((char*)"BODY-1 Fail to close code started with %s upon seeing %s\r\n", originalptr,word)
			}
		}

		ReadNextSystemToken(in,ptr,nextToken,false,true); //   caching request
		if (!startkind) startkind = *word; // may be ( or { or [  or other
		switch(*word)
		{
			case '{':
				++level;

				if (!squiggle++) 
				{
					startsquiggle = ptr;
					startsline = currentFileLine;
				}
				break;
			case '(': 
				++level;

				if (!paren++) 
				{
					startparen = ptr;
					startpline = currentFileLine;
				}
	
				break;
			case '[':
				startbracket = ptr;
				startbline = currentFileLine;
				ptr = ReadChoice(word,ptr,in,dataChunk,rejoinders); // but might have been json array ref
				continue;
			case ')': case ']': case '}':
				if (*word == '}')
				{
					--squiggle;
					if (!squiggle && startkind == '{') // closing level
					{
						if (paren)  BADSCRIPT((char*)"BODY-3 Fail to close ( on line %d - (%s \r\n",startpline,startparen)
						if (bracket) 
							BADSCRIPT((char*)"BODY-2 Fail to close [ on line %d - [%s \r\n",startbline,startbracket)
					}
				}
				else if (*word == ')')
				{
					--paren;
					if (!paren && startkind == '(') // closing level
					{
						if (squiggle)  BADSCRIPT((char*)"BODY-3 Fail to close { on line %d - (%s \r\n",startsline,startsquiggle)
						if (bracket) BADSCRIPT((char*)"BODY-2 Fail to close [ on line %d - [%s \r\n",startbline,startbracket)
					}
				}
				else if (*word == ']')
				{
					--bracket;
					if (!bracket && startkind == '[') // closing level
					{
						if (squiggle)  BADSCRIPT((char*)"BODY-3 Fail to close { on line %d - (%s \r\n",startsline,startsquiggle)
						if (paren) BADSCRIPT((char*)"BODY-2 Fail to close ( on line %d - [%s \r\n",startpline,startparen)
					}
				}

				--level;
                *dataChunk = 0;
                if (level < 0)
                    BADSCRIPT((char*)"OUTPUT-3 Unbalanced %s in %s\r\n", word, outputStart)
				else if (!level && (startkind == '(' || startkind == '{' || startkind == '[')) 
				{
					strcpy(dataChunk,word);
					dataChunk += strlen(dataChunk);
					*word = 0;	// end loop
				}
				break;
			case '\'': 
				strcpy(dataChunk,word);
				dataChunk += strlen(dataChunk);
				if (*word == '\'' && word[1] == 's' && !word[2] && IsAlphaUTF8OrDigit(*nextToken) ) *dataChunk++ = ' ';
 				else if (word[1] == 0 && (*ptr == '_' || IsAlphaUTF8(*ptr)  ))  {;} // if  isolated like join(' _1) then  add space
				else *dataChunk++ = ' ';  
				continue;
			case '@': //   factset ref
				if (!IsDigit(word[1])) 
					BADSCRIPT((char*)"OUTPUT-4 bad factset reference - %s\r\n",word)
				if (!stricmp(nextToken,(char*)"+=") || !stricmp(nextToken,(char*)"-=") ) insert = 2;
				break;
		}
		if (!*word) break; // end of body
        if (!stricmp("$cs_botid", assignlhs) && IsDigit(word[0]))
        { // bot id declaration
            macroid = atoi64(word);
        }
		if (*assignlhs) // during continued assignment?
		{
			if (!stricmp(word,(char*)"^") || !stricmp(word,(char*)"|") || !stricmp(word,(char*)"&") || (!stricmp(word,(char*)"+") || !stricmp(word,(char*)"-") || !stricmp(word,(char*)"*") || !stricmp(word,(char*)"/")))
			{
				if (!stricmp(nextToken,assignlhs)) 
				{
					WARNSCRIPT((char*)"Possibly faulty assignment. %s has changed value during prior assignment.\r\n",assignlhs)
					*assignlhs = 0;
				}
			}
			else if (!stricmp(nextToken,(char*)"^") || !stricmp(nextToken,(char*)"|") || !stricmp(nextToken,(char*)"&") || !stricmp(nextToken,(char*)"+") || !stricmp(nextToken,(char*)"-") || !stricmp(nextToken,(char*)"*") || !stricmp(nextToken,(char*)"/"))  {}
			else *assignlhs = 0;
		}
	
		char* nakedNext = nextToken;
		if (*nakedNext == '^') ++nakedNext;	// word w/o ^ 
		char* nakedWord = word;
		if (*nakedWord == '^') ++nakedWord;	// word w/o ^ 
		
		if (*word == '^')
		{
			if (!stricmp(word,"^if") || !stricmp(word,"^loop")) {;}
			else if (*nextToken != '(' && word[1] != '^'  && word[1] != '=' && word[1] != USERVAR_PREFIX && word[1] != '_' && word[1] != '"' && word[1] != '\'' && !IsDigit(word[1])) 
				BADSCRIPT((char*)"%s either references a function w/o arguments or names a function variable that doesn't exist\r\n",word)
		}

		// note left hand of assignment
		if (IsComparator(nextToken))  strcpy(assignlhs,word);
		if (*nextToken == '=' && !nextToken[1]) // simple assignment
		{
			*assignKind = 0;
			strcpy(dataChunk,word);	//   add simple item into data
			dataChunk += strlen(dataChunk);
			*dataChunk++ = ' ';
			ptr = ReadNextSystemToken(in,ptr,nextToken,false,false); //   use up lookahead of =
			strcpy(dataChunk,(char*)"=");	
			++dataChunk;
			*dataChunk++ = ' ';
			ReadNextSystemToken(in,ptr,nextToken,false,true); //   aim lookahead at followup
			if (!stricmp(nakedNext,(char*)"first") || !stricmp(nakedNext,(char*)"last") || !stricmp(nakedNext,(char*)"random") || !stricmp(nakedNext,(char*)"nth") ) 
				strcpy(assignKind,word); // verify usage fact retrieved from set
			if (*nextToken == '=' || *nextToken == '<' || *nextToken == '>')
			{
				if (!IsAlphaUTF8(nextToken[1])) 
					WARNSCRIPT((char*)"Possibly assignment followed by another binary operator")
			}
			// assigning to variable only works if tofield value is given
			if (*word == USERVAR_PREFIX && (!stricmp(nextToken,"^query") || !stricmp(nextToken,"query"))) 
				needtofield = true;
			continue;
		}
		else if (*nextToken == '{' && !stricmp(nakedWord,(char*)"loop"))  // loop missing () 
		{
			ptr = ReadLoop(word,ptr,in,dataChunk,rejoinders);
			continue;
		}
		else if (*nextToken != '(') // doesnt look like a function
		{
		}
		else if (!stricmp(nakedWord,(char*)"if"))  // strip IF of ^
		{
			ptr = ReadIf(word,ptr,in,dataChunk,rejoinders);
			*dataChunk++ = ' ';
			continue;
		}
		else if (!stricmp(nakedWord,(char*)"loop"))  // strip LOOP of ^
		{
			ptr = ReadLoop(word,ptr,in,dataChunk,rejoinders);
			continue;
		}
		else if (*word != '^' && (!call || stricmp(call->word,(char*)"^createfact"))) //   looks like a call ... if its ALSO a normal word, presume it is not a call, like: I like (American) football
		{
			// be wary.. respond(foo) might have been text...  
			// How does he TELL us its text? interpret normal word SPACE ( as not a function call?
			char rename[MAX_WORD_SIZE];
			*rename = '^';
			strcpy(rename+1,word);	//   in case user omitted the ^
			MakeLowerCase(rename);
			WORDP D = FindWord(rename,0,PRIMARY_CASE_ALLOWED);
			if (D && D->internalBits & FUNCTION_NAME) // it is a function
			{
				// is it also english. If builtin function, do that for sure
				// if user function AND english, more problematic.  maybe he forgot
				WORDP E = FindWord(word);
				if (!E || !(E->properties & PART_OF_SPEECH) || D->x.codeIndex) strcpy(word,rename); //   a recognized call
				else if (*ptr == '(') strcpy(word,rename); // use his spacing to decide
			}
		}
        if (currentFileLine != priorLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
		// a function call, 
		if (*word == '^' && !IsDigit(word[1]) && word[1] != '^'&& word[1] != '=' && word[1] != '"' && word[1] != '\'' && word[1] != USERVAR_PREFIX && word[1] != '_' && word[1] && *nextToken == '(' )
		{
			ptr = ReadCall(word,ptr,in,dataChunk,*nextToken == '(',needtofield); //   add function call
			needtofield = false;
			*assignKind = 0;
		}
		else if (*word == '^' && IsDigit(word[1]) ) // fn var
		{
			strcpy(dataChunk,word);	//   add simple item into data
			dataChunk += strlen(dataChunk);
		}		
		else 
		{
			if (*word == '~' ) CheckSetOrTopic(word);
			if (IsAlphaUTF8(*word) && spellCheck == OUTPUT_SPELL) SpellCheckScriptWord(word,-1,true);
			strcpy(dataChunk,word);	//   add simple item into data
			dataChunk += strlen(dataChunk);
		}
		*dataChunk++ = ' ';
	}
	while (*(dataChunk-1) == ' ') *--dataChunk = 0;
	*dataChunk++ = ' ';
	*dataChunk = 0;

	//   now verify no choice block exceeds CHOICE_LIMIT and that each [ is closed with ]
	if (!javascript) while (*original)
	{
		original = ReadCompiledWord(original,word);
		if (*original != '[') continue;

		unsigned int count = 0;
		char* at = original;
		while (*at == '[')
		{
			//   find the closing ]
			while (ALWAYS) 
			{
				at = strchr(at+1,']'); //   find closing ] - we MUST find it (check in initsql)
				if (!at) BADSCRIPT((char*)"OUTPUT-5 Failure to close [ choice\r\n")
				if (*(at-2) != '\\') break; //   found if not a literal \[
			}
            ++count;
			at += 2;	//   at next token
		}
		if (count >= (CHOICE_LIMIT - 1)) BADSCRIPT((char*)"OUTPUT-6 Max %d choices in a row\r\n",CHOICE_LIMIT)
		original = at;
	}
	patternContext = oldContext;
	outputStart = oldOutputStart;
	mydata = dataChunk;
	return ptr;
}

static void ReadTopLevelRule(char* typeval,char* &ptr, FILE* in,char* data,char* basedata)
{//   handles 1 responder/gambit + all rejoinders attached to it
	char type[100];
	complexity = 1;
	strcpy(type,typeval);
	char info[400];
	char kind[MAX_WORD_SIZE];
	strcpy(kind,type);
	char word[MAX_WORD_SIZE];
	char rejoinders[256];	//   legal levels a: thru q:
	memset(rejoinders,0,sizeof(rejoinders));
	WriteVerify();	// dump any accumulated verification data before the rule
	//   rejoinders == 1 is normal, 2 means authorized in []  3 means authorized and used
	*rejoinders = 1;	//   we know we have a responder. we will see about rejoinders later
	while (ALWAYS) //   read responser + all rejoinders
	{
		MakeLowerCase(kind);
		char* original = data;
		int level = 0;
		//   validate rejoinder is acceptable
		if (Rejoinder(kind))
		{
			complexity = 1;
			int count = level = *kind - 'a' + 1;	//   1 ...
			if (rejoinders[level] >= 2) rejoinders[level] = 3; //   authorized by [b:] and now used
			else if (!rejoinders[level-1]) BADSCRIPT((char*)"RULE-1 Illegal rejoinder level %s\r\n",kind)
			else rejoinders[level] = 1; //   we are now at this level, enables next level
			//   levels not authorized by [b:][g:] etc are disabled
			while (++count < 20)
			{
				if (rejoinders[count] == 1) rejoinders[count] = 0;
			}
			
			currentRuleID += ONE_REJOINDER;
			WriteVerify();
		}
		strcpy(data,kind); 
		data += 2;
		*data++ = ' ';	
		bool patternDone = false;

#ifdef INFORMATION

A responder of any kind consists of a prefix of `xx  spot xx is an encoded jump offset to go the the
end of the responder. Then it has the kind item (t:   s:  etc). Then a space.
Then one of 3 kinds of character:
	a. a (- indicates start of a pattern
	b. a space - indicates no pattern exists
	c. a 1-byte letter jump code - indicates immediately followed by a label and the jump code takes you to the (

#endif
		char label[MAX_WORD_SIZE];
		char labelName[MAX_WORD_SIZE];
		*label = 0;
		*labelName = 0;
     
		while (ALWAYS) //   read as many tokens as needed to complete the responder definition
		{
			ptr = ReadNextSystemToken(in,ptr,word,false); 
			if (!*word)  break;
			MakeLowerCopy(lowercaseForm,word);

			size_t len = strlen(word);
			if (TopLevelUnit(word) || TopLevelRule(lowercaseForm) || !stricmp(word,(char*)"datum:")) 
			{
				*word = 0;
				break;//   responder definition ends when another major unit or top level responder starts
			}

			if (*word == '(') //   found pattern, no label
			{
				sprintf(info,"        rule: %s.%d.%d %s",currentTopicName,TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID),kind);
				AddMap(info,NULL); // rule
				ptr = ReadPattern(ptr-1,in,data,false,false); //   back up and pass in the paren for pattern
				patternDone = true;
				*word = 0;
				break;
			}
			else //   label or start of output
			{
				ReadNextSystemToken(in,ptr,nextToken,false,true);	//   peek what comes after

				if (*nextToken == '(' && (IsAlphaUTF8(*word)  ||IsDigit(*word))) //  label exists
				{
					if (!IsLegalName(word,true)) BADSCRIPT((char*)"? Illegal characters in rule label %s\r\n", word)
					char name[MAX_WORD_SIZE];
					*name = '^';
					strcpy(name+1,word);
					WORDP D = FindWord(name,0,LOWERCASE_LOOKUP);
					if (D && D->internalBits & FUNCTION_NAME && (*kind == GAMBIT || *kind == RANDOM_GAMBIT)) 
						WARNSCRIPT((char*)"label: %s is a potential macro in %s. Add ^ if you want it treated as such.\r\n",word,currentFilename)
					else if (!stricmp(word,(char*)"if") || !stricmp(word,(char*)"loop")) WARNSCRIPT((char*)"label: %s is a potential flow control (if/loop) in %s. Add ^ if you want it treated as a control word.\r\n",word,currentFilename)
					sprintf(info,"        rule: %s.%d.%d-%s %s",currentTopicName,TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID),name+1, kind);
					AddMap(info,NULL); // rule
					//  potential ^reuse label
					strcpy(label,currentTopicName); 
					strcat(label,(char*)".");
					MakeUpperCase(word); // full label to test if exists.
					strcat(label,word);
                    if (!stricmp(word, "COUNTER"))
                    {
                        int xx = 0;
                    }
					strcpy(labelName,word);
					MakeUpperCase(label); // full label to test if exists.
					WORDP E = StoreWord(label,AS_IS);
					AddInternalFlag(E,LABEL);
					if (strchr(word,'.')) BADSCRIPT((char*)"RULE-2 Label %s must not contain a period\r\n",word)
					if (len > 160) BADSCRIPT((char*)"RULE-2 Label %s must be less than 160 characters\r\n",word)
					int fulllen = len;
					if (len > 40)
					{
						int tens = len / 40; // how many 40s does it hold
						len -= (tens * 40);
						*data++ = (char) (tens + '*');	// detectable as a 2char label
						*data++ = (char)('0' + len + 2); //   prefix attached to label
					}
					else *data++ = (char)('0' + len + 2); //   prefix attached to label
					strcpy(data,word);
					data += fulllen;
					*data++ = ' ';
					ReadNextSystemToken(NULL,NULL,NULL); // drop lookahead token
					ptr = ReadPattern(ptr,in,data,false,false); //   read ( for real in the paren for pattern
					patternDone = true;
					*word = 0;
				}
				else //   we were seeing start of output (no label and no pattern) for gambit, proceed to output
				{
					sprintf(info,"        rule: %s.%d.%d-%s %s",currentTopicName,TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID),labelName,kind);
					AddMap(info,NULL); // rule
					if (*type != GAMBIT && *type != RANDOM_GAMBIT) BADSCRIPT((char*)"RULE-3 Missing pattern for responder\r\n")
					*data++ = ' ';
					patternDone = true;
					// leave word intact to pass to readoutput
				}
				break;
			}
		} //   END OF WHILE
		if (patternDone) 
		{
            dataBase = data;
            ptr = ReadOutput(false,false,ptr,in,data,rejoinders,word,NULL);
            dataBase = NULL;
            char complex[MAX_WORD_SIZE];
			sprintf(complex,"          Complexity of rule %s.%d.%d-%s %s %d", currentTopicName,TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID),labelName,kind,complexity);
			AddMap(NULL, complex); // complexity

			//   data points AFTER last char added. Back up to last char, if blank, leave it to be removed. else restore it.
			while (*--data == ' '); 
			*++data = ' ';
			strcpy(data+1,ENDUNITTEXT); //   close out last topic item+
			data += strlen(data);

			while (ALWAYS) // read all verification comments for next rule if any, getting the next real word token
			{
				ptr = ReadNextSystemToken(in,ptr,word,false); 
				if (*word != '#' || word[1] != '!') break;
				ptr = AddVerify(word,ptr);
			}

			MakeLowerCopy(lowercaseForm,word);
			if (!*word || TopLevelUnit(word) || TopLevelRule(lowercaseForm) || !stricmp(word,(char*)"datum:"))  
			{
				ptr -= strlen(word);  // safe
				break;//   responder definition ends when another major unit or top level responder starts
			}

			//  word is a rejoinder type
			strcpy(kind,lowercaseForm);
		}
		else ReportBug((char*)"unexpected word in ReadTopLevelRule - %s",word)
	}

	//   did he forget to fill in any [] jumps
	for (unsigned int i = ('a'-'a'); i <= ('q'-'a'); ++i)
	{
		if (rejoinders[i] == 2) BADSCRIPT((char*)"RULE-4 Failed to define rejoinder %c: for responder just ended\r\n", i + 'a' - 1)
	}

	*data = 0;
    dataBase = NULL;
}

static char* ReadMacro(char* ptr,FILE* in,char* kind,unsigned int build)
{
	bool table = !stricmp(kind,(char*)"table:"); // create as a transient notwrittentofile 
	*currentTopicName = 0;
	displayIndex = 0;
	complexity = 1;
	uint64 typeFlags = 0;
	if (!stricmp(kind,(char*)"tableMacro:") || table) typeFlags = IS_TABLE_MACRO;
	else if (!stricmp(kind,(char*)"outputMacro:")) typeFlags = IS_OUTPUT_MACRO;
	else if (!stricmp(kind,(char*)"patternMacro:")) typeFlags = IS_PATTERN_MACRO;
	else if (!stricmp(kind,(char*)"dualMacro:")) typeFlags = IS_PATTERN_MACRO | IS_OUTPUT_MACRO;
	*macroName = 0;
    macroid = 0;
	functionArgumentCount = 0;
	char data[MAX_BUFFER_SIZE];
	*data = 0;
	char* pack = data;
	int64 macroFlags = 0;
	int parenLevel = 0;
	WORDP D = NULL;
	bool gettingArguments = true;
	patternContext = false;
	char word[MAX_WORD_SIZE];
	while (gettingArguments) //   read as many tokens as needed to get the name and argumentList
	{
		ptr = ReadNextSystemToken(in,ptr,word,false);
		if (!*word) break; //   end of file
		if (!*macroName) //   get the macro name
		{
			if (*word == '^' || *word == '~') memmove(word,word+1,strlen(word)); //   remove his ^
			MakeLowerCase(word);
			if (!table && !IsAlphaUTF8(*word) ) BADSCRIPT((char*)"MACRO-1 Macro name must start alpha %s\r\n",word)
			if (table)
			{
				strcpy(macroName,(char*)"^tbl:");
				strcat(macroName,word);
				Log(STDTRACELOG,(char*)"Reading table %s\r\n",macroName);
			}
			else
			{
				if (!IsLegalName(word)) BADSCRIPT((char*)"MACRO-2 Illegal characters in function name %s\r\n",word)
				*macroName = '^';
				strcpy(macroName+1,word);
				Log(STDTRACELOG,(char*)"Reading %s %s\r\n",kind,macroName);
				AddMap((char*)"    macro:", macroName);
			}
			D = StoreWord(macroName);
			if (D->w.fndefinition && D->internalBits & FUNCTION_NAME && !table) // must be different BOT ID
			{
				int64 bid;
				ReadInt64((char*)GetDefinition(D),bid); 
				if (bid == (int64)myBot)BADSCRIPT((char*)"MACRO-3 macro %s already defined\r\n",macroName)
			}
			continue;
		}
		if (parenLevel == 0 && !stricmp(word,(char*)"variable")) // putting "variable" before the args list paren allows you to NAME all args but get ones not supplied filled in with * (tables) or null (macros)
		{
			D->internalBits |= VARIABLE_ARGS_TABLE;
			continue;
		}

		size_t len = strlen(word);
		if (TopLevelUnit(word)) //   definition ends when another major unit starts
		{
			ptr -= len; //   let someone else see this starter also
			break; 
		}
		char* restrict = NULL;
		switch(*word)
		{
			case '(': 
				if (parenLevel++ != 0) BADSCRIPT((char*)"MACRO-4 bad paren level in macro definition %s\r\n",macroName)
				continue; //   callArgumentList open
			case ')':
				if (--parenLevel != 0) BADSCRIPT((char*)"MACRO-5 bad closing paren in macro definition %s\r\n",macroName)
				gettingArguments = false;
				break;
			case '$': // declaring local
				if (typeFlags & IS_PATTERN_MACRO) BADSCRIPT((char*)"MACRO-? May not use locals in a pattern/dual macro - %s\r\n",word)
				if (word[1] != '_') BADSCRIPT((char*)"MACRO-? Variable name as argument must be local %s\r\n",word)
				if (strchr(word,'.') || strchr(word,'[')) 
                    BADSCRIPT((char*)"MACRO-? Variable name as argument must be simple, not json reference %s\r\n",word)
				AddDisplay(word);
				strcpy(functionArguments[functionArgumentCount++],word);
				if (functionArgumentCount > MAX_ARG_LIMIT)  BADSCRIPT((char*)"MACRO-7 Too many callArgumentList to %s - max is %d\r\n",macroName,MAX_ARG_LIMIT)
				continue;
			case '^':  //   declaring a new argument
				if (IsDigit(word[1])) 
					BADSCRIPT((char*)"MACRO-6 Function arguments must be alpha names, not digits like %s\r\n",word)
				restrict = strchr(word,'.');
				if (restrict)
				{
					if (!stricmp(restrict+1,(char*)"KEEP_QUOTES") && (typeFlags == IS_TABLE_MACRO || typeFlags == IS_OUTPUT_MACRO))	macroFlags |= 1ull << functionArgumentCount; // a normal string where spaces are kept instead of _ (format string)
					else if (!stricmp(restrict+1,(char*)"HANDLE_QUOTES"))	
					{
						if (typeFlags != IS_OUTPUT_MACRO) BADSCRIPT((char*)"MACRO-? HANDLE_QUOTES only valid with OUTPUTMACRO or DUALMACRO - %s \r\n",word)
						if (functionArgumentCount > 15) 
						{
							int64 flag = 1ull << (functionArgumentCount - 16); // outputmacros
							flag <<= 32;
							macroFlags |= flag;
						}
						else macroFlags |= 1ull << functionArgumentCount; // outputmacros
					}
					else if (!stricmp(restrict+1,(char*)"COMPILE") && typeFlags == IS_TABLE_MACRO) 
					{
						if (functionArgumentCount > 15)
						{
							int64 flag = (1 << 16) << (functionArgumentCount - 16); // outputmacros
							flag <<= 32;
							macroFlags |= flag;

						}
						else macroFlags |= (1 << 16) << functionArgumentCount; // a compile string " " becomes "^:"
					}
					else if (!stricmp(restrict+1,(char*)"UNDERSCORE") && typeFlags == IS_TABLE_MACRO) {;} // default for quoted strings is _ 
					else if (typeFlags != IS_TABLE_MACRO && typeFlags != IS_OUTPUT_MACRO) BADSCRIPT((char*)"Argument restrictions only available on Table Macros or OutputMacros  - %s \r\n",word)
					else  BADSCRIPT((char*)"MACRO-? Table/Tablemacro argument restriction must be KEEP_QUOTES OR COMPILE or UNDERSCORE - %s \r\n",word)
					*restrict = 0;
				}
                else {}// default for quoted strings on argumet is UNDERSCORE
				{
					WORDP X = FindWord(word);
					if (X && X->internalBits & FUNCTION_NAME) BADSCRIPT((char*)"MACRO-8 Function argument %s is also name of a function\r\n",word);
				}
				AddDisplay(word);
				strcpy(functionArguments[functionArgumentCount++],word);
				if (functionArgumentCount > MAX_ARG_LIMIT)  BADSCRIPT((char*)"MACRO-7 Too many callArgumentList to %s - max is %d\r\n",macroName,MAX_ARG_LIMIT)
				continue;
			default:
				BADSCRIPT((char*)"MACRO-7 Bad argument to macro definition %s\r\n",macroName)
		}
	}
    if (!D)
    {
        dataBase = NULL;
        return ptr; //   nothing defined
    }
	if (functionArgumentCount > ARGSETLIMIT) BADSCRIPT((char*)"MACRO-7 Argument count to macro definition %s limited to %d\r\n",macroName,ARGSETLIMIT)
	AddInternalFlag(D,(unsigned int)(FUNCTION_NAME|build|typeFlags));
	if (functionArgumentCount > 15) *pack++ = (unsigned char)(functionArgumentCount - 15 + 'a');
	else *pack++ = (unsigned char)(functionArgumentCount + 'A'); // some 10 can be had ^0..^9
	
	bool optionalBrace = false;
	currentFunctionDefinition = D;
	char d[MAX_BUFFER_SIZE];
    dataBase = NULL;
	if ( (typeFlags & FUNCTION_BITS) == IS_PATTERN_MACRO)  
	{
        char* at = d;
		ptr = ReadPattern(ptr,in,at,true,false);
		*at = 0;
		// insert display and add body back
		pack = WriteDisplay(pack);
		strcpy(pack,d);
		pack += at - d;
	}
	else 
	{
		ReadNextSystemToken(in,ptr,word,false,true);

		// check for optional display variables
		if (*word == '(') 
		{
			ptr = ReadDisplay(in,ptr);
			ReadNextSystemToken(in,ptr,word,false,true);
		}
		if (*word == '{') // see if he used optional {  syntax
		{
			ReadNextSystemToken(in,ptr,word,false);
			optionalBrace = true;
		}

        dataBase = d;
        if ((typeFlags & FUNCTION_BITS) == IS_PATTERN_MACRO) dataBase = NULL;

		// now read body of macro
		char* at = d;
        // if on same line we have issue?
		ptr = ReadOutput(optionalBrace,false,ptr,in,at,NULL,NULL,NULL);
		ReadNextSystemToken(in,ptr,word,true);
		if (optionalBrace && *word == '}') ptr = ReadNextSystemToken(in,ptr,word,false);
		else if (optionalBrace) BADSCRIPT("Missing closing optional brace in reading macro %s\r\n", macroName)

		*at = 0;
		// insert display and add body back
		pack = WriteDisplay(pack);
		strcpy(pack,d);
		pack += at - d;
	}
	*pack++ = '`'; // add closing marker to script
	*pack = 0;

	//   record that it is a macro, with appropriate validation information
	char botid[MAX_WORD_SIZE];
#ifdef WIN32
	sprintf(botid, (char*)"%I64u", (int64)myBot);
#else
	sprintf(botid, (char*)"%llu", (int64)myBot);
#endif
	char* revised = AllocateBuffer();
	revised[0] = revised[1] = revised[2] = revised[3] = 0;
	revised += 4;
#ifdef WIN32
	sprintf(revised, (char*)"%s %I64d ", botid, macroFlags);
#else
	sprintf(revised, (char*)"%s %lld ", botid, macroFlags);
#endif
	strcat(revised, data);
	size_t len = strlen(revised) + 4;

	if (table && D->w.fndefinition) // need to link old definition to new one
	{
		unsigned int heapIndex = Heap2Index((char*)D->w.fndefinition);
		revised[0] = (heapIndex >> 24) & 0xff;
		revised[1] = (heapIndex >> 16) & 0xff;
		revised[2] = (heapIndex >> 8) & 0xff;
		revised[3] = heapIndex & 0xff;
	}

	D->w.fndefinition = (unsigned char*)AllocateHeap(revised - 4, strlen(revised) + 4);
	FreeBuffer();

	if (!table) // tables are not real macros, they are temporary
	{
		char filename[SMALL_WORD_SIZE];
		sprintf(filename,(char*)"%s/BUILD%s/macros%s.txt",topic,baseName,baseName);
		//   write out definition -- this is the real save of the data
		FILE* out = FopenUTF8WriteAppend(filename);

		if ((D->internalBits & FUNCTION_BITS) ==  IS_TABLE_MACRO) fprintf(out,(char*)"%s t %s\r\n",macroName,GetDefinition(D));
		else if ((D->internalBits & FUNCTION_BITS) == (IS_OUTPUT_MACRO|IS_PATTERN_MACRO))  fprintf(out,(char*)"%s d %s\r\n",macroName,GetDefinition(D));
		else fprintf(out,(char*)"%s %c %s\r\n",macroName,((D->internalBits & FUNCTION_BITS) == IS_OUTPUT_MACRO) ? 'o' : 'p',GetDefinition(D));
		fclose(out); // dont use Fclose

		char complex[MAX_WORD_SIZE];
		sprintf(complex, "          Complexity of %s: %d", macroName, complexity);
		AddMap(NULL, complex); // complexity
        if (macroid != 0)
        {
            char name[MAX_WORD_SIZE];
#ifdef WIN32
            sprintf(name, (char*)"          bot: 0 %s %I64u ", macroName,macroid);
#else
            sprintf(name, (char*)"          bot: 0 %s %llu ", macroName,macroid);
#endif
            AddMap(NULL, name); // bot macro
        }
	}
    *macroName = 0;
    dataBase = NULL;
	return ptr;
}

static char* ReadTable(char* ptr, FILE* in,unsigned int build,bool fromtopic)
{
    bool oldecho = echo;
    int oldtrace = trace;
	char name[MAX_WORD_SIZE];
	char word[MAX_WORD_SIZE];
	char post[MAX_WORD_SIZE]; 
	char args[MAX_TABLE_ARGS+1][MAX_WORD_SIZE];
	unsigned short quoteProcessing = 0;
	unsigned int indexArg = 0;
	char* pre = NULL;
	ptr = SkipWhitespace(ptr);
	ReadNextSystemToken(in,ptr,name,false,true); 
	if (*name == '~')
	{
		*name = '^';
		char* at = strchr(ptr, '~');
		*at = '^';
	}
	else if (*name != '^')  // add function marker if it lacks one
	{
		memmove(name+1,name,strlen(name)+1);
		*name = '^';
	}
	currentFunctionDefinition = FindWord(name);
	unsigned int sharedArgs;
	bool tableMacro = false;
	if (!currentFunctionDefinition) // go define a temporary tablemacro function since this is a spontaneous table  Table:
	{
		if (fromtopic) BADSCRIPT((char*)"datum: from topic must use predefined table %s",name)
		ptr = ReadMacro(ptr,in,(char*)"table:",build); //   defines the name,argumentList, and script
		ptr = ReadNextSystemToken(in,ptr,word,false,false); //   the DATA: separator
		if (stricmp(word,(char*)"DATA:")) 	BADSCRIPT((char*)"TABLE-1 missing DATA: separator for table - %s\r\n",word)
		sharedArgs = 0;
	}
	else // this is an existing table macro being executed
	{
		tableMacro = true;
		ptr = ReadNextSystemToken(in,ptr,word,false,false);  // swallow function name
		ptr = ReadNextSystemToken(in,ptr,word,false,false);  // swallow (
		if (*word != '(') BADSCRIPT((char*)"TABLE-2 Must have ( before arguments")
		while (ALWAYS) // read argument values we supply to the existing tablemacro
		{
			ptr = ReadNextSystemToken(in,ptr,args[indexArg],false,false);  
			if (*args[indexArg] == ')') break;
			if (*args[indexArg] == '^' && args[indexArg][1] != '"') 
				BADSCRIPT((char*)"TABLE-3 TableMacro %s requires real args, not redefinition args",currentFunctionDefinition->word)
			if (++indexArg >= MAX_TABLE_ARGS) BADSCRIPT((char*)"TABLE-4 too many table args\r\n")
		}
		sharedArgs = indexArg;
	}
	
	unsigned char* defn = GetDefinition(currentFunctionDefinition);
	unsigned int wantedArgs = MACRO_ARGUMENT_COUNT(defn);

	char junk[MAX_WORD_SIZE];
	defn = (unsigned char*) ReadCompiledWord((char*)defn, junk); // read bot id
	int flags;
	ReadInt((char*)defn, flags);
	quoteProcessing = (short int) flags; // values of KEEP_QUOTES for each argument

	// now we have the function definition and any shared arguments. We need to read the real arguments per table line now and execute.

	char* argumentList = AllocateBuffer();
	++jumpIndex;
	int holdDepth = globalDepth;
	char* xxbase = ptr;  // debug hook
	while (ALWAYS) 
	{
		if (setjmp(scriptJump[jumpIndex])) // flush on error
		{
			ptr = FlushToTopLevel(in,holdDepth,0);
			break;
		}
		ptr = ReadNextSystemToken(in,ptr,word,false,false); 
		char* original = ptr - strlen(word);
		if (*word == '\\' && word[1] == 'n') continue; // newline means pretend new table entry

		if (*word == ':' && word[1]) // debug command
		{
			ptr = original;  // safe
			char output[MAX_WORD_SIZE];
			DoCommand(ptr,output);
			*ptr = 0;
			continue;
		}
		if (!*word || TopLevelUnit(word) || TopLevelRule(word)) // end
		{
			ptr = original;  // safe
			break;
		}
	
		
		//   process a data set from the line
		char* systemArgumentList = argumentList;
		*systemArgumentList++ = '(';
		*systemArgumentList++ = ' ';
		unsigned int argCount = 0;

		// common arguments processing
		for (unsigned int i = 0; i < sharedArgs; ++i)
		{
			if (*args[i] == '^' && args[i][1] == '"')
			{
				FunctionResult result;
				char* oldoutputbase = currentOutputBase;
				currentOutputBase = systemArgumentList;
				ReformatString(args[i][1],args[i]+2,systemArgumentList,result);
				currentOutputBase = oldoutputbase;
			}
			else strcpy(systemArgumentList,args[i]);
			systemArgumentList += strlen(systemArgumentList);
			*systemArgumentList++ = ' ';
			++argCount;
		}

		// now fill in args of table data from a single line
		char* choiceArg = NULL; //   the multiple interior
		bool startup = true;
        trace = 0;
        while (ALWAYS)
		{
            if (!startup) ptr = ReadSystemToken(ptr,word);	//   next item to associate
			if (!stricmp(word,(char*)":debug")) 
			{
				DebugCode(word);
				continue;
			}
            startup = false;
			if (!*word) break;					//   end of LINE of items stuff

			if (!stricmp(word,(char*)"...")) break;	// pad to end of arg count

			if (!stricmp(word,(char*)"\\n"))			// fake end of line
			{
				memmove(readBuffer,ptr,strlen(ptr)+1);	//   erase earlier stuff we've read
				ptr = readBuffer;
				break; 
			}

			if (*word == '[' ) // choice set (one per line allowed)
			{
				if (choiceArg) BADSCRIPT((char*)"TABLE-5 Only allowed 1 multiple choice [] arg\r\n")
				pre = systemArgumentList;  //   these are the fixed arguments before the multiple choice one
				choiceArg = ptr; //   multiple choices arg 
				char* at = strchr(ptr,']'); //  find end of multiple choice
				if (!at) BADSCRIPT((char*)"TABLE-6 bad [ ] ending %s in table %s\r\n",readBuffer,currentFunctionDefinition->word)
				ptr = at + 1; //   continue fixed argumentList AFTER the multiple choices set (but leave blank if there)
				++argCount;
				continue; //   skipping over this arg, move on to next arg now.
			}
			uint64 flag = 0;

			// how do we store string arguments - with underscores, as is, compiled,
			bool keepQuotes = (quoteProcessing & ( 1 << argCount)) ? 1 : 0; // want to use quotes and spaces, instead of none and convert to _ which is the default // a normal string where spaces are kept instead of _ (format string)
			bool xxotherNotation = (quoteProcessing & ( (1 << 16) << argCount)) ? 1 : 0; // unused at present  
			if (*word == FUNCTIONSTRING && (word[1] == '"' || word[1] == '\''))
			{
				strcpy(word,CompileString(word)); // no underscores in string, compiled as executable // a compile string " " becomes "^:"
				flag = AS_IS;
			}
			else if (*word == '"' && keepQuotes) // no underscores in string, preserve string. Quotes needed to detect as single argument for fact creation
			{
				flag = AS_IS;
			}
			else 
			{
				unsigned int n = BurstWord(word,(*word == '"') ? POSSESSIVES : 0);
				strcpy(word,JoinWords(n)); // by default strings are stored with _, pretending they are composite words.
				if (n > 1)  flag = AS_IS;
			}
			if ( *word == '\\') memmove(word,word+1,strlen(word)); // remove escape
			if (*word == '"' && !word[1]) BADSCRIPT((char*)"TABLE-? isolated doublequote argument- start of string not recognized?\r\n");
			if (flag != AS_IS && *word != '"' && strstr(word,(char*)" ")) BADSCRIPT((char*)"TABLE-7 unexpected space in string %s - need to use doublequotes around string\r\n",word);
			WORDP baseWord = StoreWord(word,flag);
			strcpy(word,baseWord->word); 
	
			strcpy(systemArgumentList,word);
			systemArgumentList += strlen(systemArgumentList);
			*systemArgumentList++ = ' ';
			++argCount;

			//   handle synonyms as needed
			ptr = SkipWhitespace(ptr); //   to align to see if (given 
			MEANING base = MakeMeaning(baseWord);
			if (*ptr == '(' && ++ptr) while (ALWAYS) // synonym listed, create a fact for it
			{
				ptr = ReadSystemToken(ptr,word);
				if (!*word || *word == '[' || *word == ']')  BADSCRIPT((char*)"TABLE-8 Synomym in table %s lacks token\r\n",currentFunctionDefinition->word)
				if (*word == ')') break;	//   end of synonms
				strcpy(word,JoinWords(BurstWord(word,CONTRACTIONS)));
				if (IsUpperCase(*word)) CreateFact(MakeMeaning(StoreWord(word,NOUN|NOUN_PROPER_SINGULAR)),Mmember,base); 
				else CreateFact(MakeMeaning(StoreWord(word,NOUN|NOUN_SINGULAR)),Mmember,base);
			}
			if ((wantedArgs - sharedArgs) == 1)
			{
				memmove(readBuffer,ptr,strlen(ptr)+1);	
				ptr = readBuffer;
				break;
			}
		}

		while ( argCount < wantedArgs && (!stricmp(word,(char*)"...") || currentFunctionDefinition->internalBits & VARIABLE_ARGS_TABLE))
		{
			strcpy(systemArgumentList,(char*)"*");
			systemArgumentList += strlen(systemArgumentList);
			*systemArgumentList++ = ' ';
			++argCount;
		}

		*systemArgumentList = 0;
		if (choiceArg) strcpy(post,pre); // save argumentList after the multiple choices

		//   now we have one map of the argumentList row
		if (argCount && argCount != wantedArgs)
			BADSCRIPT((char*)"TABLE-9 Bad table %s in table %s, want %d arguments and have %d\r\n",original,currentFunctionDefinition->word, wantedArgs,argCount)

		//   table line is read, now execute rules on it, perhaps multiple times, after stuffing in the choice if one
		if (argCount) //   we swallowed a dataset. Process it
		{
			while (ALWAYS)
			{
				//   prepare variable argumentList
				if (choiceArg) //   do it with next multi
				{
					choiceArg = ReadSystemToken(choiceArg,word); //   get choice
					if (!*word || *word == ']') break;			//   end of multiple choice

					unsigned int control = 0;
					if (*word == FUNCTIONSTRING && word[1] == '"') strcpy(word,CompileString(word)); // readtable
					else strcpy(word,JoinWords(BurstWord(word,CONTRACTIONS|control)));
					strcpy(word,StoreWord(word,(control) ? AS_IS : 0)->word); 

					if (*word == '\'') //   quoted value
					{
						choiceArg = ReadSystemToken(choiceArg,word); //   get 1st of choice
						if (!*word || *word == ']') break;			//   end of LINE of items stuff
						ForceUnderscores(word);
						strcpy(pre,StoreWord(word)->word); //   record the local w/o any set expansion
					}
					else 
					{
						WORDP D = StoreWord(word);
						strcpy(pre,D->word); //   record the multiple choice
						choiceArg = SkipWhitespace(choiceArg);
						if (*choiceArg == '(' && ++choiceArg) while(choiceArg) //   synonym 
						{
							choiceArg = ReadSystemToken(choiceArg,word);
							if (!*word) BADSCRIPT((char*)"TABLE-10 Failure to close synonym list in table %s\r\n",currentFunctionDefinition->word)
							if (*word == ')') break;	//   end of synonms
							ForceUnderscores(word);
							CreateFact(MakeMeaning(StoreWord(word)),Mmember,MakeMeaning(D)); 
						}
					}		

					char* at = pre + strlen(pre);
					*at++ = ' ';
					strcpy(at,post); //   add rest of argumentList
					systemArgumentList = at + strlen(post);
				}
				*systemArgumentList++ = ')';	//   end of call setup
				*systemArgumentList = 0;
				
				currentRule = NULL;
				FunctionResult result;
				AllocateOutputBuffer();
				DoFunction(currentFunctionDefinition->word,argumentList,currentOutputBase,result);
				FreeOutputBuffer();
				if (!choiceArg) break;
			}
		}
		if (fromtopic) break; // one entry only
	}
	FreeBuffer();

	if (!tableMacro)  // delete dynamic function
	{
		currentFunctionDefinition->internalBits &= -1LL ^ FUNCTION_NAME;
		currentFunctionDefinition->w.fndefinition = NULL;
		AddInternalFlag(currentFunctionDefinition,DELETED_MARK);
	}
	currentFunctionDefinition = NULL; 
	--jumpIndex;
    echo = oldecho;
    trace = oldtrace;
	return ptr;
}

static void SetJumpOffsets(char* data) // store jump offset for each rule
{
    char* at = data;
    char* end = data;
    while (*at && *++at) // find each responder end
    {
        if (*at == ENDUNIT) 
        {
            int diff = (int)(at - end  + 1);
			if (diff > MAX_JUMP_OFFSET) BADSCRIPT((char*)"TOPIC-9 Jump offset too far - %d but limit %d near %s\r\n",diff,MAX_JUMP_OFFSET,readBuffer) //   limit 2 char (12 bit) 
			Encode(diff,end);
            end = at + 1;
        }
    }
 }

static char* ReadKeyword(char* word,char* ptr,bool &notted, bool &quoted, MEANING concept,uint64 type,bool ignoreSpell,unsigned int build,bool duplicate)
{
	// read the keywords zone of the concept
	char* at;
	MEANING M;
	WORDP D;
	size_t len = strlen(word);
	switch(*word) 
	{
		case '!':	// excuded keyword
			if (len == 1) BADSCRIPT((char*)"CONCEPT-5 Must attach ! to keyword in %s\r\n",Meaning2Word(concept)->word);
			if (notted) BADSCRIPT((char*)"CONCEPT-5 Cannot use ! after ! in %s\r\n",Meaning2Word(concept)->word);
			notted = true;
			ptr -= len;
			if (*ptr == '!') ++ptr;
			break;
		case '\'': 
			if (len == 1) BADSCRIPT((char*)"CONCEPT-6 Must attach ' to keyword in %s\r\n",Meaning2Word(concept)->word);
			if (quoted) BADSCRIPT((char*)"CONCEPT-5 Cannot use ' after ' in %s\r\n",Meaning2Word(concept)->word);
			quoted = true;	//   since we emitted the ', we MUST emit the next token
			ptr -= len;
			if (*ptr == '\'') ++ptr;
			break;
		default:
			if (*word == USERVAR_PREFIX || (*word == '_' && IsDigit(word[1])) || *word == SYSVAR_PREFIX) BADSCRIPT((char*)"CONCEPT-? Cannot use $var or _var or %var as a keyword in %s\r\n",Meaning2Word(concept)->word);
			if (*word == '~') MakeLowerCase(word); //   sets are always lower case
			if ((at = strchr(word+1,'~'))) //   wordnet meaning request, confirm definition exists
			{
				char level[MAX_WORD_SIZE];
				strcpy(level,at);
				M = ReadMeaning(word);
				if (!M) BADSCRIPT((char*)"CONCEPT-7 WordNet word doesn't exist %s\r\n",word)
				WORDP D = Meaning2Word(M);
				int index = Meaning2Index(M);
				if ((GetMeaningCount(D) == 0 && !(GETTYPERESTRICTION(M) & BASIC_POS)) || (index && !strcmp(word,D->word) && index > GetMeaningCount(D)))
				{
					if (index) 
						WARNSCRIPT((char*)"WordNet word does not have such meaning %s\r\n",word)
					M &= -1 ^ INDEX_BITS;
				}
			}
			else // ordinary word or concept-- see if it makes sense
			{
				char end = word[strlen(word)-1];
                if (nospellcheck) {}
				else if (!IsAlphaUTF8OrDigit(end) && end != '"' && strlen(word) != 1)
				{
					if (end != '.' || strlen(word) > 6) WARNSCRIPT((char*)"last character of keyword %s is punctuation. Is this intended?\r\n", word)
				}
				else if (end == '"' && word[(strlen(word) - 2)] == ' ') BADSCRIPT((char*) "CONCEPT-? Keyword %s ends in illegal space\r\n", word)
				if (*word == '\\') memcpy(word,word+1,strlen(word)); // how to allow $e as a keyword
				M = ReadMeaning(word);
				D = Meaning2Word(M);
				uint64 type1 = type;
				if (type & NOUN_SINGULAR && D->internalBits & UPPERCASE_HASH)
				{
					type1 ^= NOUN_SINGULAR;
					type1 |= NOUN_PROPER_SINGULAR;
				}
				if (type) AddProperty(D,type1); // augment its type

				if (*D->word == '~') // concept
				{
					if (M == concept) 
						BADSCRIPT((char*)"CONCEPT-8 Cannot include topic into self - %s\r\n",D->word);
					CheckSetOrTopic(D->word);
				}
				else if ( ignoreSpell || !spellCheck || strchr(D->word,'_') || !D->word[1] || D->internalBits & UPPERCASE_HASH) {;}	// ignore spelling issues, phrases, short words &&  proper names
				else if (!(D->properties & PART_OF_SPEECH) && !(D->systemFlags & PATTERN_WORD))
				{
					if (!(spellCheck & NO_SPELL)) SpellCheckScriptWord(D->word,-1,false);
					WriteKey(D->word);
					WritePatternWord(D->word);
				}
			} // end ordinary word
			unsigned int flags = quoted ? ORIGINAL_ONLY : 0;
			if (duplicate) flags |= FACTDUPLICATE;
			if (build & BUILD1) flags |= FACTBUILD1; // concept facts from build 1
			else if (build & BUILD2) flags |= FACTBUILD2; // concept facts from build 1
			FACT* F = CreateFact(M,(notted) ? Mexclude : Mmember,concept, flags); 
			quoted = false;
			notted = false;
	} 
	return ptr;
}

static char* ReadBot(char* ptr)
{
	*botheader = ' ';
	ptr = SkipWhitespace(ptr);
	char* original = ptr;
	if (IsDigit(*ptr))
	{
		int64 n;
		ptr = ReadInt64(ptr,n); // change bot id
		myBot = n;
	}
	MakeLowerCopy(botheader+1,ptr); // presumes til end of line
	size_t len = strlen(botheader);
	while (botheader[len-1] == ' ') botheader[--len] = 0;
	if (len == 0) 
	{
		Log(STDTRACELOG,(char*)"Reading bot restriction: %s\r\n",original);
		return ""; // there is no header anymore
	}

	strcat(botheader," "); // single trailing space
	char* x;
	while ((x = strchr(botheader,','))) *x = ' ';	// change comma to space. all bot names have spaces on both sides
	Log(STDTRACELOG,(char*)"Reading bot restriction: %s\r\n",original);
	return "";
}

static char* ReadTopic(char* ptr, FILE* in,unsigned int build)
{
	patternContext = false;
	displayIndex = 0;
	char* data = (char*) malloc(MAX_TOPIC_SIZE); // use a big chunk of memory for the data
	*data = 0;
	char* pack = data;

	++topicCount;
	*currentTopicName = 0;
	unsigned int flags = 0;
	bool topicFlagsDone = false;
	bool keywordsDone = false;
	int parenLevel = 0;
	bool quoted = false;
	bool notted = false;
	MEANING topicValue = 0;
	int holdDepth = globalDepth;
	WORDP topicName = NULL;
	unsigned int gambits = 0;
	unsigned int toplevelrules = 0; // does not include rejoinders
	currentRuleID = 0;	// reset rule notation
	verifyIndex = 0;	
	bool stayRequested = false;
	if (setjmp(scriptJump[++jumpIndex])) 
	{
		ptr = FlushToTopLevel(in,holdDepth,data); //   if error occurs lower down, flush to here
	}
	while (ALWAYS) //   read as many tokens as needed to complete the definition
	{
		char word[MAX_WORD_SIZE];
		ptr = ReadNextSystemToken(in,ptr,word,false);
		if (!*word) break;

		if (!*currentTopicName) //   get the topic name
		{
			if (*word != '~') BADSCRIPT((char*)"Topic name - %s must start with ~\r\n",word)
			strcpy(currentTopicName,word);
    		Log(STDTRACELOG,(char*)"Reading topic %s\r\n",currentTopicName);
			topicName = FindWord(currentTopicName);
			if (!myBot && topicName && topicName->internalBits & CONCEPT && !(topicName->internalBits & TOPIC) && topicName->internalBits & (BUILD0|BUILD1|BUILD2))
				WARNSCRIPT((char*)"TOPIC-1 Concept already defined with this topic name %s\r\n",currentTopicName)
			topicName = StoreWord(currentTopicName);
			if (!IsLegalName(currentTopicName)) BADSCRIPT((char*)"TOPIC-2 Illegal characters in topic name %s\r\n",currentTopicName)
			topicValue = MakeMeaning(topicName);
			// handle potential multiple topics of same name
			duplicateCount = 0;
			while (topicName->internalBits & TOPIC)
			{
				++duplicateCount;
				char name[MAX_WORD_SIZE];
				sprintf(name,(char*)"%s%c%d",currentTopicName,DUPLICATETOPICSEPARATOR,duplicateCount);
				topicName = StoreWord(name);
				if (!*duplicateTopicName) 
					strcpy(duplicateTopicName,currentTopicName);
			}
			strcpy(currentTopicName,topicName->word);
            AddMap((char*)"    topic:", currentTopicName);

			AddInternalFlag(topicName,(unsigned int)(build|CONCEPT|TOPIC));
			topicName->w.botNames = NULL;
			continue;
		}

		if (TopLevelUnit(word)) //   definition ends when another major unit starts
		{
			ptr -= strlen(word); //   let someone else see this starter also  // safe
			break; 
		}

		switch(*word)
		{
		case '(': case '[':
			if (!keywordsDone && topicFlagsDone) BADSCRIPT((char*)"TOPIC-3 Illegal bracking in topic keywords %s\r\n",word)
			if (flags & TOPIC_SHARE && flags & TOPIC_SYSTEM) 
				BADSCRIPT((char*)"TOPIC-? Don't need SHARE on SYSTEM topic %s, it is already shared via system\r\n",currentTopicName)
			topicFlagsDone = true; //   topic flags must occur before list of keywords
			++parenLevel;
			break;
		case ')': case ']':
			--parenLevel;
			if (parenLevel == 0) 
			{
				keywordsDone = true;
				ReadNextSystemToken(in,ptr,word,false,true);

				// check for optional display variables
				if (*word == '(') ptr = ReadDisplay(in,ptr);
			}
			break;
		case '#':
			if (*word == '#' && word[1] == '!')  ptr = AddVerify(word,ptr);
			continue;
		default:
			MakeLowerCopy(lowercaseForm,word);
			if (!topicFlagsDone) //   do topic flags
			{
				if (!strnicmp(word,(char*)"bot=",4)) // bot restriction on the topic
				{
					char botlist[MAX_WORD_SIZE];
					MakeLowerCopy(botlist,word+4);
					char* x;
					while ((x = strchr(botlist,','))) *x = ' ';	// change comma to space. all bot names have spaces on both sides
					topicName->w.botNames = AllocateHeap(botlist,strlen(botlist)); // bot=harry,georgia,roger
				}
                else if (!stricmp(word,(char*)"deprioritize")) flags |= TOPIC_LOWPRIORITY; 
				else if (!stricmp(word,(char*)"noblocking")) flags |= TOPIC_NOBLOCKING; 
 				else if (!stricmp(word,(char*)"nopatterns") || !stricmp(word,(char*)"nopattern")) flags |= TOPIC_NOPATTERNS; 
 				else if (!stricmp(word,(char*)"nogambits") || !stricmp(word,(char*)"nogambit")) flags |= TOPIC_NOGAMBITS; 
 				else if (!stricmp(word,(char*)"nosamples") || !stricmp(word,(char*)"nosample")) flags |= TOPIC_NOSAMPLES; 
 				else if (!stricmp(word,(char*)"nokeys") || !stricmp(word,(char*)"nokeywords")  )  flags |= TOPIC_NOKEYS; 
                else if (!stricmp(word,(char*)"keep")) flags |= TOPIC_KEEP; 
 				else if (!stricmp(word,(char*)"norandom")) flags &= -1 ^TOPIC_RANDOM;
				else if (!stricmp(word,(char*)"normal")) flags &= -1 ^TOPIC_PRIORITY;
				else if (!stricmp(word,(char*)"norepeat")) flags &= -1 ^TOPIC_REPEAT;
				else if (!stricmp(word,(char*)"nostay")) flags |= TOPIC_NOSTAY;
				else if (!stricmp(word,(char*)"priority"))  flags |= TOPIC_PRIORITY; 
				else if (!stricmp(word,(char*)"random")) flags |= TOPIC_RANDOM;
				else if (!stricmp(word,(char*)"repeat")) flags |= TOPIC_REPEAT; 
				else if (!stricmp(word,(char*)"safe")) flags |= TOPIC_SAFE;
				else if (!stricmp(word,(char*)"share")) flags |= TOPIC_SHARE;
				else if (!stricmp(word,(char*)"stay")) 
				{
					flags &= -1 ^TOPIC_NOSTAY;
					stayRequested = true;
				}
				else if (!stricmp(word,(char*)"erase")) flags &= -1 ^TOPIC_KEEP;
				else if (!stricmp(word,(char*)"system")) 
				{
					flags |= TOPIC_SYSTEM | TOPIC_KEEP | TOPIC_NOSTAY;
					if (stayRequested) BADSCRIPT((char*)"TOPIC-4 Topic %s cannot be both STAY and SYSTEM\r\n",currentTopicName)
				}
				else if (!stricmp(word,(char*)"user"));
                else BADSCRIPT((char*)"Bad topic flag %s for topic %s\r\n",word,currentTopicName)
			}
			else if (!keywordsDone) ptr = ReadKeyword(word,ptr,notted,quoted,topicValue,0,false,build,false);//   absorb keyword list
			else if (!stricmp(word,(char*)"datum:")) // absorb a top-level data table line
			{
				ptr = ReadTable(ptr,in,build,true);
			}
			else if (TopLevelRule(lowercaseForm))//   absorb a responder/gambit and its rejoinders
			{
				if (IsUpperCase(*word)) BADSCRIPT((char*)"Rule ID must be lower case: %s\r\n",word);
				++toplevelrules;
				if (TopLevelGambit(word)) ++gambits;
				if (pack == data)
				{
					strcpy(pack,ENDUNITTEXT+1);	//   init 1st rule
					pack += strlen(pack);
				}
				ReadTopLevelRule(lowercaseForm,ptr,in,pack,data);
				currentRuleID = TOPLEVELID(currentRuleID) + 1;
				pack += strlen(pack);
				if ((pack - data) > (MAX_TOPIC_SIZE - 2000)) BADSCRIPT((char*)"TOPIC-4 Topic %s data too big. Split it by calling another topic using u: () respond(~subtopic) and putting the rest of the rules in that subtopic\r\n",currentTopicName)
			}
			else BADSCRIPT((char*)"Expecting responder for topic %s, got %s",currentTopicName,word)
		}
	}

	--jumpIndex;

	if (parenLevel) BADSCRIPT((char*)"TOPIC-5 Failure to balance ( in %s\r\n",currentTopicName)
	if (!topicName) BADSCRIPT((char*)"TOPIC-6 No topic name?\r\n")
	if (toplevelrules > MAX_TOPIC_RULES) BADSCRIPT((char*)"TOPIC-8 %s has too many rules- %d must be limited to %d. Call a subtopic.\r\n",currentTopicName,toplevelrules,MAX_TOPIC_RULES)
	if (!topicName->w.botNames && *botheader) topicName->w.botNames = AllocateHeap(botheader,strlen(botheader)); //  harry,georgia,roger

	size_t len = pack-data;
    SetJumpOffsets(data); 
	if (displayIndex)
	{
		char display[MAX_WORD_SIZE * 10];
		char* at = WriteDisplay(display);
		size_t displayLen = at - display;
		memmove(data+displayLen,data,len+1);	// shift it all down + 1 for space separator replaceing string end
		len += displayLen;
		memmove(data,display,displayLen);
	}
	bool hasUpperCharacters;
	bool hasUTF8Characters;
	unsigned int checksum = (unsigned int) (Hashit((unsigned char*) data, len,hasUpperCharacters,hasUTF8Characters) & 0x0ffffffff);
	
	//   trailing blank after jump code
	if (len >= (MAX_TOPIC_SIZE-100)) BADSCRIPT((char*)"TOPIC-7 Too much data in one topic\r\n")
	char filename[SMALL_WORD_SIZE];
	sprintf(filename,(char*)"%s/BUILD%s/script%s.txt",topic,baseName,baseName);
	FILE* out = FopenUTF8WriteAppend(filename);
	if (out)
	{
		// write out topic data
		char* restriction = (topicName->w.botNames) ? topicName->w.botNames : (char*)"all";
		unsigned int len1 = (unsigned int)strlen(restriction);
		fprintf(out, (char*)"TOPIC: %s 0x%x %d %d %d %d %s\r\n", currentTopicName, (unsigned int)flags, (unsigned int)checksum, (unsigned int)toplevelrules, (unsigned int)gambits, (unsigned int)(len + len1 + 7), currentFilename);
		fprintf(out, (char*)"\" %s \" ", restriction);
		fprintf(out, (char*)"%s\r\n", data);
		fclose(out); // dont use FClose
	}
	free(data);
	
	return ptr;
}

static char* ReadRename(char* ptr, FILE* in,unsigned int build)
{
	renameInProgress = true;
	while (ALWAYS) //   read as many tokens as needed to complete the definition
	{
		char word[MAX_WORD_SIZE];
		char basic[MAX_WORD_SIZE];
		ptr = ReadNextSystemToken(in,ptr,word,false);
		if (!*word) break;
		if (*word == '#' && (word[1] != '#' || !IsAlphaUTF8(word[2])))
		{
			*ptr = 0;
			break; // comment ends it also
		}

		if (TopLevelUnit(word)) //   definition ends when another major unit starts
		{
			ptr -= strlen(word); //   let someone else see this starter also  // safe
			break; 
		}
		if (*word != '_' && *word != '@' && (*word != '#' || word[1] != '#')) 
			BADSCRIPT((char*)"Rename  %s must start with _ or @ or ##\r\n",word)
		ptr = ReadNextSystemToken(in,ptr,basic,false);
		if (*word != '#' && (*basic != *word || !IsDigit(basic[1]) )) BADSCRIPT((char*)"Rename  %s must start same as %s and have a number after it\r\n",basic,word)
		if (*word == '#' && !IsDigit(*basic) && *basic != '-' && *basic != '+') 
			BADSCRIPT((char*)"Rename  %s followed by number or sign as %s\r\n",word,basic)
		MakeLowerCase(word); 
		int64 n;
		if (*word == '#') 
		{
			ReadInt64(basic,n);
			if (*basic == '-') n = -n; // force positive
		}
		else ReadInt64(basic+1,n);
		WORDP D = FindWord(word);
		if (D && !myBot)  
			WARNSCRIPT((char*)"Already have a rename for %s\r\n", word)
		D = StoreWord(word,n);
		AddInternalFlag(D,(unsigned int)(RENAMED|build)); 
		if (*word == '#' && *basic == '-') AddSystemFlag(D,CONSTANT_IS_NEGATIVE);
		Log(STDTRACELOG,(char*)"Rename %s as %s\r\n",basic,word);
	}	
	renameInProgress = false;
	return ptr;
}

static char* ReadPlan(char* ptr, FILE* in,unsigned int build)
{
	if (build == BUILD2) BADSCRIPT((char*)"Not allowed plans in layer 2 at present\r\n")
	char planName[MAX_WORD_SIZE];
	char baseName[MAX_WORD_SIZE];
	displayIndex = 0;
	*planName = 0;
	functionArgumentCount = 0;
	int parenLevel = 0;
	WORDP D = NULL;
	bool gettingArguments = true;
	endtopicSeen = false;
	patternContext = false;
	int baseArgumentCount = 0;
	unsigned int duplicateCount = 0;
	while (gettingArguments) //   read as many tokens as needed to get the name and argumentList
	{
		char word[MAX_WORD_SIZE];
		ptr = ReadNextSystemToken(in,ptr,word,false);
		if (!*word) break; //   end of file

		if (!*planName) //   get the plan name
		{
			if (*word == '^') memmove(word,word+1,strlen(word)); //   remove his ^
			MakeLowerCase(word);
			if (!IsAlphaUTF8(*word) ) BADSCRIPT((char*)"PLAN-1 Plan name must start alpha %s\r\n",word)
			if (!IsLegalName(word)) BADSCRIPT((char*)"PLAN-2 Illegal characters in plan name %s\r\n",word)
			*planName = '^';
			strcpy(planName+1,word);
			strcpy(baseName,planName);
			Log(STDTRACELOG,(char*)"Reading plan %s\r\n",planName);

			// handle potential multiple plans of same name
			WORDP plan = FindWord(planName);
			char name[MAX_WORD_SIZE];
			strcpy(name,planName);
			if (plan) baseArgumentCount = plan->w.planArgCount;
			while (plan && plan->internalBits & FUNCTION_NAME)
			{
				++duplicateCount;
				sprintf(name,(char*)"%s%c%d",baseName,DUPLICATETOPICSEPARATOR,duplicateCount);
				plan = FindWord(name);
				strcpy(planName,name);
			}

			D = StoreWord(planName);
			continue;
		}

		size_t len = strlen(word);
		if (TopLevelUnit(word)) //   definition ends when another major unit starts
		{
			ptr -= len; //   let someone else see this starter also
			break; 
		}
		switch(*word)
		{
			case '(': 
				if (parenLevel++ != 0) BADSCRIPT((char*)"PLAN-4 bad paren level in plan definition %s\r\n",planName)
				continue; //   callArgumentList open
			case ')':
				if (--parenLevel != 0) BADSCRIPT((char*)"PLAN-5 bad closing paren in plan definition %s\r\n",planName)
				gettingArguments = false;
				break;
			case '^':  //   declaring a new argument
				if (IsDigit(word[1])) BADSCRIPT((char*)"PLAN-6 Plan arguments must be alpha names, not digits like %s\r\n",word)
				strcpy(functionArguments[functionArgumentCount++],word);
				if (functionArgumentCount > MAX_ARG_LIMIT)  BADSCRIPT((char*)"PLAN-7 Too many callArgumentList to %s - max is %d\r\n",planName,MAX_ARG_LIMIT)
				continue;
			default:
				BADSCRIPT((char*)"PLAN-7 Bad argument to plan definition %s",planName)
		}
	}
	if (!D) return ptr; //   nothing defined
	if (parenLevel) BADSCRIPT((char*)"PLAN-5 Failure to balance ( in %s\r\n",planName)
	if (duplicateCount && functionArgumentCount != baseArgumentCount) 
		BADSCRIPT((char*)"PLAN->? Additional copies of %s must have %d arguments\r\n",planName,baseArgumentCount)
	AddInternalFlag(D,(unsigned int)(FUNCTION_NAME|build|IS_PLAN_MACRO));
	D->w.planArgCount = functionArgumentCount;
	currentFunctionDefinition = D;
	
	char* data = (char*) malloc(MAX_TOPIC_SIZE); // use a big chunk of memory for the data
	*data = 0;
	char* pack = data;

	int holdDepth = globalDepth;
	unsigned int toplevelrules = 0; // does not include rejoinders

	if (setjmp(scriptJump[++jumpIndex])) 
	{
		ptr = FlushToTopLevel(in,holdDepth,data); //   if error occurs lower down, flush to here
	}
	while (ALWAYS) //   read as many tokens as needed to complete the definition
	{
		char word[MAX_WORD_SIZE];
		ptr = ReadNextSystemToken(in,ptr,word,false);
		if (!*word) break;

		if (TopLevelUnit(word)) //   definition ends when another major unit starts
		{
			ptr -= strlen(word); //   let someone else see this starter also  // safe
			break; 
		}

		switch(*word)
		{
		case '#':
			if (*word == '#' && word[1] == '!')  BADSCRIPT((char*)"PLAN-? Verification not meaningful in a plan\r\n")
			continue;
		default:
			MakeLowerCopy(lowercaseForm,word);
			if (TopLevelRule(lowercaseForm))//   absorb a responder/gambit and its rejoinders
			{
				++toplevelrules;
				if (pack == data)
				{
					strcpy(pack,ENDUNITTEXT+1);	//   init 1st rule
					pack += strlen(pack);
				}
				ReadTopLevelRule(lowercaseForm,ptr,in,pack,data);
				pack += strlen(pack);
				if ((pack - data) > (MAX_TOPIC_SIZE - 2000)) BADSCRIPT((char*)"PLAN-4 Plan %s data too big. Split it by calling another topic using u: () respond(~subtopic) and putting the rest of the rules in that subtopic\r\n",planName)
			}
			else BADSCRIPT((char*)"Expecting responder for plan %s, got %s\r\n",planName,word)
		}
	}

	--jumpIndex;

	if (toplevelrules > MAX_TOPIC_RULES) BADSCRIPT((char*)"PLAN-8 %s has too many rules- %d must be limited to %d. Call a plantopic.\r\n",planName,toplevelrules,MAX_TOPIC_RULES)

	size_t len = pack-data;
	if (!len)  WARNSCRIPT((char*)"No data in plan %s\r\n",currentTopicName)

	if (!endtopicSeen) BADSCRIPT((char*)"PLAN-8 Plan %s cannot succeed since no ^end(plan) exists\r\n",planName)

	//   trailing blank after jump code
    SetJumpOffsets(data); 
	if (len >= (MAX_TOPIC_SIZE-100)) BADSCRIPT((char*)"PLAN-7 Too much data in one plan\r\n")
	*pack = 0;
		
	char file[200];
	if (build == BUILD0) sprintf(file,"%s/BUILD0/plans0.txt",topic);
	else sprintf(file,"%s/BUILD0/plans1.txt",topic);
		
	//   write how many plans were found (for when we preload during normal startups)
	if (hasPlans == 0)
	{
		FILE* out = FopenUTF8Write(file);
		fprintf(out,(char*)"%s",(char*)"0     \r\n"); //   reserve 5-digit count for number of plans
		fclose(out); // dont use Fclose
	}
	++hasPlans;

	// write out plan data
	FILE* out = FopenUTF8WriteAppend(file);
	char* restriction =  (char*)"all";
	unsigned int len1 = (unsigned int)strlen(restriction);
	fprintf(out,(char*)"PLAN: %s %d %d %d %s\r\n",planName,(unsigned int) functionArgumentCount,(unsigned int) toplevelrules,(unsigned int)(len + len1 + 7),currentFilename); 
	fprintf(out,(char*)"\" %s \" %s\r\n",restriction,data);
	fclose(out); // dont use FClose

	free(data);
	return ptr;
}

static char* ReadQuery(char* ptr, FILE* in, unsigned int build) // readquery: name "xxxxx" text
{
	while (ALWAYS) //   read as many tokens as needed to complete the definition (must be within same file)
	{
		char word[MAX_WORD_SIZE];
		char query[MAX_WORD_SIZE];
		ptr = ReadNextSystemToken(in,ptr,word,false); // name of query
		if (!*word) break;	
		size_t len = strlen(word);
		if (!IsAlphaUTF8(*word)) BADSCRIPT((char*)"query label %s must be alpha\r\n",word);
		if (TopLevelUnit(word)) //   definition ends when another major unit starts
		{
			ptr -= len; //   let someone else see this starter 
			break; 
		}
		ptr = ReadNextSystemToken(in,ptr,query,false);
		if (*query != '"') BADSCRIPT((char*)"query body %s must be in quotes\r\n",query);
		WORDP D = StoreWord(word);
		AddInternalFlag(D, (unsigned int)(QUERY_KIND|build));
		char* at = strchr(query+1,'"'); 
		*at = 0;
 	    D->w.userValue = AllocateHeap(query+1);    
	}
	return ptr;
}

static char* ReadReplace(char* ptr, FILE* in, unsigned int build)
{
	while (ALWAYS) //   read as many tokens as needed to complete the definition (must be within same file)
	{
		char word[MAX_WORD_SIZE];
		char replace[MAX_WORD_SIZE];
		ptr = ReadNextSystemToken(in,ptr,word,false);
		if (!stricmp(word,(char*)"replace:")) ptr = ReadNextSystemToken(in,ptr,word,false); // keep going with local replace loop
		if (!*word) break;	//   file ran dry
		if (TopLevelUnit(word)) //   definition ends when another major unit starts
		{
			ptr -= strlen(word); //   let someone else see this starter 
			break; 
		}
		ptr = ReadNextSystemToken(in,ptr,replace,false);
		if (TopLevelUnit(replace)) //   definition ends when another major unit starts
		{
			ptr -= strlen(replace); //   let someone else see this starter 
			break; 
		}
		char filename[SMALL_WORD_SIZE];
		sprintf(filename,(char*)"%s/BUILD%s/private%s.txt",topic,baseName,baseName);
		FILE* out = FopenUTF8WriteAppend(filename);
		fprintf(out,(char*)" %s %s\r\n",word,replace);
		fclose(out); // dont use FClose
	}
	return ptr;
}

static char* ReadIgnoreSpell(char* ptr, FILE* in, unsigned int build)
{
    while (ALWAYS) //   read as many tokens as needed to complete the definition (must be within same file)
    {
        char ignore[MAX_WORD_SIZE];
        ptr = ReadNextSystemToken(in, ptr, ignore, false);
        if (!stricmp(ignore, (char*)"ignorespell:")) ptr = ReadNextSystemToken(in, ptr, ignore, false); // keep going with local ignore loop
        if (!*ignore) break;	//   file ran dry
        if (TopLevelUnit(ignore)) //   definition ends when another major unit starts
        {
            ptr -= strlen(ignore); //   let someone else see this starter 
            break;
        }
        if (TopLevelUnit(ignore)) //   definition ends when another major unit starts
        {
            ptr -= strlen(ignore); //   let someone else see this starter 
            break;
        }
        if (*ignore == '*' && !ignore[1])
        {
            nospellcheck = true;
            continue;
        }
        if (*ignore == '!' && ignore[1] == '*' && !ignore[2])
        {
            nospellcheck = false;
            continue;
        }
        WORDP D = StoreWord(ignore, 0);
        if (!(D->internalBits & HAS_SUBSTITUTE)) D->internalBits |= DO_NOISE;
    }
    return ptr;
}

static char* ReadPrefer(char* ptr, FILE* in, unsigned int build)
{
	while (ALWAYS) //   read as many tokens as needed to complete the definition (must be within same file)
	{
		char word[MAX_WORD_SIZE];
		ptr = ReadNextSystemToken(in, ptr, word, false);
		if (!*word) break;	//   file ran dry
		if (TopLevelUnit(word)) //   definition ends when another major unit starts
		{
			ptr -= strlen(word); //   let someone else see this starter 
			break;
		}
		WORDP D = StoreWord(word);
		D->internalBits |= PREFER_THIS_UPPERCASE;
	}
	return ptr;
}

void SaveCanon(char* word, char* canon)
{
	char filename[SMALL_WORD_SIZE];
	sprintf(filename,(char*)"%s/BUILD%s/canon%s.txt",topic,baseName,baseName);
	FILE* out = FopenUTF8WriteAppend(filename);
	fprintf(out,(char*)" %s %s\r\n",word,canon);
	fclose(out); // dont use FClose
	WritePatternWord(word);		// must recognize this word for spell check
	WritePatternWord(canon);	// must recognize this word for spell check
}

static char* ReadCanon(char* ptr, FILE* in, unsigned int build)
{
	while (ALWAYS) //   read as many tokens as needed to complete the definition (must be within same file)
	{
		char word[MAX_WORD_SIZE];
		char canon[MAX_WORD_SIZE];
		ptr = ReadNextSystemToken(in,ptr,word,false);
		if (!stricmp(word,(char*)"canonical:")) ptr = ReadNextSystemToken(in,ptr,word,false); // keep going with local  loop
		if (!*word) break;	//   file ran dry
		size_t len = strlen(word);
		if (TopLevelUnit(word)) //   definition ends when another major unit starts
		{
			ptr -= len; //   let someone else see this starter 
			break; 
		}
		ptr = ReadNextSystemToken(in,ptr,canon,false);
		SaveCanon(word,canon);
	}
	return ptr;
}

static char* ReadConcept(char* ptr, FILE* in,unsigned int build)
{
	char conceptName[MAX_WORD_SIZE];
	*conceptName = 0;
	MEANING concept = 0;
	WORDP D;
	bool ignoreSpell = false;
	
	patternContext = false;
	bool quoted = false;
	bool notted = false;
	bool more = false;
	bool undeclared = true;
	int parenLevel = 0;
	uint64 type = 0;
	uint64 sys;
	bool duplicate = false;
	while (ALWAYS) //   read as many tokens as needed to complete the definition (must be within same file)
	{
		char word[MAX_WORD_SIZE];
		ptr = ReadNextSystemToken(in,ptr,word,false);
		if (!*word) break;	//   file ran dry
		size_t len = strlen(word);
		if (TopLevelUnit(word)) //   definition ends when another major unit starts
		{
			if (TopLevelUnit(word)) ptr -= len; //   let someone else see this starter 
			break; 
		}

		// establish name and characteristics of the concept
		if (!*conceptName) //   get the concept name, will be ~xxx or :xxx 
		{
			if (*word != '~' ) BADSCRIPT((char*)"CONCEPT-1 Concept name must begin with ~ or : - %s\r\n",word)

			// Users may not create repeated user topic names. Ones already saved in dictionary are fine to overwrite
			MakeLowerCopy(conceptName,word);
			if (!IsLegalName(conceptName)) BADSCRIPT((char*)"CONCEPT-2 Illegal characters in concept name %s\r\n",conceptName)
				// create concept header
			D = StoreWord(conceptName);
			concept = MakeMeaning(D);
			sys = type = 0;
			parenLevel = 0;
			Log(STDTRACELOG,(char*)"Reading concept %s\r\n",conceptName);
			AddMap((char*)"    concept:", conceptName);
			// read the control flags of the concept
			ptr = SkipWhitespace(ptr);
			while (*ptr && *ptr != '(' && *ptr != '[' && *ptr != '"') // not started and no concept comment given (concept comments come after all control flags
			{
				ptr = ReadCompiledWord(ptr,word);
				len = strlen(word);
				if (word[len-1] == '(') 
				{
					word[len-1] = 0;
					--ptr;
					if (*ptr != '(') --ptr;
				}
				if (!stricmp(word,(char*)"more"))
				{
					more = true;
					continue;
				}
				if (!stricmp(word,(char*)"duplicate")) // allow duplicate keywords
				{
					duplicate = true;
					continue;
				}
				char* paren = strchr(word,'(');
				if (paren) // handle attachment of paren + stuff
				{
					while (*--ptr != '(');
					*paren = 0;
				}

				ptr = SkipWhitespace(ptr);
				uint64 bits = FindValueByName(word);
				type |= bits;
				uint64 bits1 = FindSystemValueByName(word);
				sys |= bits1;
				unsigned int bits2 = (unsigned int)FindParseValueByName(word);

				if (sys & NOCONCEPTLIST)
				{
					AddInternalFlag(D,FAKE_NOCONCEPTLIST);
					sys ^= NOCONCEPTLIST;
				}
				if (bits) AddProperty(D, bits);
				else if (bits1) AddSystemFlag(D, bits1);
				else if (bits2) AddParseBits(D,bits2);
				else if (!stricmp(word,(char*)"IGNORESPELLING")) ignoreSpell = true;
				else if (!stricmp(word,(char*)"UPPERCASE_MATCH")) AddInternalFlag(D,UPPERCASE_MATCH);
				else if (!stricmp(word,(char*)"ONLY_NOUNS")) AddSystemFlag(D,NOUN);
				else if (!stricmp(word,(char*)"ONLY_VERBS")) AddSystemFlag(D,VERB);
				else if (!stricmp(word,(char*)"ONLY_ADJECTIVES"))  AddSystemFlag(D,ADJECTIVE);
				else if (!stricmp(word,(char*)"ONLY_ADVERBS")) AddSystemFlag(D,ADVERB);
				else if (!stricmp(word,(char*)"ONLY_NONE"))  AddSystemFlag(D,ONLY_NONE); // disable ONLY here and below
				else BADSCRIPT((char*)"CONCEPT-4 Unknown concept property %s\r\n",word) 
			}
			continue;  // read more tokens now that concept has been established
		}
		if (undeclared)
		{
			undeclared = false; // dont test this again
			if (!more)
			{
				if (!myBot && D->internalBits & CONCEPT && D->internalBits & (BUILD0|BUILD1|BUILD2))
					WARNSCRIPT((char*)"CONCEPT-3 Concept/topic already defined %s\r\n",conceptName)
			}
			AddInternalFlag(D,(unsigned int)(build|CONCEPT));
		}

		// read the keywords zone of the concept
		switch(*word) //   THE MEAT OF CONCEPT DEFINITIONS
		{
			case '(':  case '[':	// start keyword list
				if (parenLevel) BADSCRIPT((char*)"CONCEPT-5 Cannot use [ or ( within a keyword list for %s\r\n",conceptName);
				parenLevel++;
				break;
			case ')': case ']':		// end keyword list
				--parenLevel;
				if (parenLevel < 0) BADSCRIPT((char*)"CONCEPT-6 Missing ( for concept definition %s\r\n",conceptName)
				break;
			default: 
				 ptr = ReadKeyword(word,ptr,notted,quoted,concept,type,ignoreSpell,build,duplicate);
		}
		if (parenLevel == 0) break;

	}
	if (parenLevel) BADSCRIPT((char*)"CONCEPT-7 Failure to give closing ( in concept %s\r\n",conceptName)

	return ptr;
}

static void ReadTopicFile(char* name,uint64 buildid) //   read contents of a topic file (.top or .tbl)
{	
	callingSystem = 0;
	chunking = false;
	unsigned int build = (unsigned int) buildid;
	size_t len = strlen(name);
	if (len <= 4) return;

	// Check the filename is at least four characters (the ext plus one letter)
	// and matches either .top or .tbl
	char* suffix = name + len - 4;
	if (stricmp(suffix, (char*) ".top") && stricmp(suffix, (char*) ".tbl")) return;

	FILE* in = FopenReadNormal(name);
	if (!in) 
	{
		if (strchr(name,'.') || build & FROM_FILE) // names a file, not a directory
		{
			WARNSCRIPT((char*)"Missing file %s\r\n",name) 
			++missingFiles;
		}
		return;
	}
	build &= -1 ^ FROM_FILE; // remove any flag indicating it came as a direct file, not from a directory listing

	Log(STDTRACELOG,(char*)"\r\n----Reading file %s\r\n",currentFilename);
	char map[MAX_WORD_SIZE];
	char file[MAX_WORD_SIZE];
	GetCurrentDir(file, MAX_WORD_SIZE);
	sprintf(map,"%s/%s",file,name);
	char* find = map;
	while ((find = strchr(find,'\\'))) *find = '/';
	AddMap((char*)"file:", map);

	//   if error occurs lower down, flush to here
	int holdDepth = globalDepth;
	patternContext = false;
	char* ptr = "";
	if (setjmp(scriptJump[++jumpIndex])) 
	{
		ptr = FlushToTopLevel(in,holdDepth,0);
	}
	char word[MAX_WORD_SIZE];
	while (ALWAYS) 
	{
		ptr = ReadNextSystemToken(in,ptr,word,false); //   eat tokens (should all be top level)
		if (!*word) break;						//   no more tokens found

		currentFunctionDefinition = NULL; //   can be set by ReadTable or ReadMacro
		if (!stricmp(word,(char*)":quit")) break;
				
		if (*word == ':' && word[1])		// testing command
		{
			char output[MAX_WORD_SIZE];
			DoCommand(readBuffer,output);
			*readBuffer = 0;
			*ptr = 0;
		}
		else if (!stricmp(word,(char*)"concept:")) ptr = ReadConcept(ptr,in,build);
		else if (!stricmp(word,(char*)"query:")) ptr = ReadQuery(ptr,in,build);
		else if (!stricmp(word,(char*)"replace:")) ptr = ReadReplace(ptr,in,build);
        else if (!stricmp(word, (char*)"ignorespell:")) ptr = ReadIgnoreSpell(ptr, in, build);
        else if (!stricmp(word, (char*)"prefer:")) ptr = ReadPrefer(ptr, in, build);
		else if (!stricmp(word,(char*)"canon:")) ptr = ReadCanon(ptr,in,build);
		else if (!stricmp(word,(char*)"topic:"))  ptr = ReadTopic(ptr,in,build);
		else if (!stricmp(word,(char*)"plan:"))  ptr = ReadPlan(ptr,in,build);
		else if (!stricmp(word,(char*)"bot:")) 
		{
			globalBotScope = false; // lasts for this file
			ptr = ReadBot(ptr);
		}
		else if (!stricmp(word,(char*)"table:")) ptr = ReadTable(ptr,in,build,false);
		else if (!stricmp(word,(char*)"rename:")) ptr = ReadRename(ptr,in,build);
		else if (!stricmp(word,(char*)"describe:")) ptr = ReadDescribe(ptr,in,build);
		else if (!stricmp(word,(char*)"patternMacro:") || !stricmp(word,(char*)"outputMacro:") || !stricmp(word,(char*)"dualMacro:") || !stricmp(word,(char*)"tableMacro:")) ptr = ReadMacro(ptr,in,word,build);
		else BADSCRIPT((char*)"FILE-1 Unknown top-level declaration %s in %s\r\n",word,name)
	}
	FClose(in); // this should be the only such, not fclose.
	--jumpIndex;
	if (hasHighChar) WARNSCRIPT((char*)"File %s has no utf8 BOM but has character>127 - extended Ansi changed to normal Ascii\r\n", name) // should have been utf 8 or have no high data.
	if (!globalBotScope) // restore any local change from this file
	{
		myBot = 0;
		*botheader = 0;
	}
}

static void DoubleCheckFunctionDefinition()
{
	unsigned int list = undefinedCallThreadList;
	while (list)
	{
		unsigned int* data = (unsigned int*) Index2Heap(list);
		char* functionName = Index2Heap(data[1]);
		int args = *functionName++ ;
		WORDP D = FindWord(functionName);
		strcpy(currentFilename,Index2Heap(data[2]));
		currentFileLine = data[3];
		if (D && D->internalBits & FUNCTION_BITS)
		{
			unsigned char* defn = GetDefinition(D);
			int want = MACRO_ARGUMENT_COUNT(defn);
			if (args != want && !(D->internalBits & VARIABLE_ARGS_TABLE))
			{
				Log(BADSCRIPTLOG, (char*)"*** Error- Function %s wrong argument count %d expected %d given \r\n", functionName,want,args);
				++hasErrors;
			}
		}
		else if (functionName[1] != USERVAR_PREFIX) // allow function calls indirect off variables
		{
			Log(BADSCRIPTLOG, (char*)"*** Error- Undefined function %s \r\n", functionName);
			++hasErrors;
		}
		list = ((unsigned int*) Index2Heap(list))[0];
	}

}

static void DoubleCheckReuse()
{
	char file[200];
	sprintf(file,"%s/missingLabel.txt",topic);
	FILE* in = FopenReadWritten(file);
	if (!in) return;

	char word[MAX_WORD_SIZE];
	char topicname[MAX_WORD_SIZE];
	while (ReadALine(readBuffer,in) >= 0)
	{
		char *ptr = ReadCompiledWord(readBuffer,word);		// topic + label
		ptr = ReadCompiledWord(ptr,topicname);					// from topic
		ptr = ReadCompiledWord(ptr,tmpWord);				// from file
		int line;
		ptr = ReadInt(ptr,line);									// from line
        char bothead[MAX_WORD_SIZE];
        ptr = ReadCompiledWord(ptr, bothead);				// from bot
        WORDP D = FindWord(word);
		if (!D) // cannot find full label
		{
			if (!strcmp(topicname,word))  WARNSCRIPT((char*)"Missing local label %s for reuse/unerase in topic %s in File: %s Line: %d Bot: %s\r\n",word,topicname,tmpWord,line,bothead)
			else  WARNSCRIPT((char*)"Missing cross-topic label %s for reuse in File: %s Line: %d Bot: %s\r\n",word,tmpWord,line,bothead)
		}
	}
	fclose(in); // dont use Fclose
	remove(file);
}

static void WriteConcepts(WORDP D, uint64 build)
{
	char* name = D->word;
	if (*name != '~' || !(D->internalBits & build)) return; // not a topic or concept or not defined this build
	RemoveInternalFlag(D,(BUILD0|BUILD1));
	// write out keywords 
	FILE* out = NULL;
	char filename[SMALL_WORD_SIZE];
	sprintf(filename,(char*)"%s/BUILD%s/keywords%s.txt",topic,baseName,baseName);
	out = FopenUTF8WriteAppend(filename);
	fprintf(out,(D->internalBits & TOPIC) ? (char*)"T%s " : (char*)"%s ", D->word);
	uint64 properties = D->properties;	
	uint64 bit = START_BIT;
	while (properties && bit)
	{
		if (properties & bit && bit)
		{
			properties ^= bit;
			fprintf(out,(char*)"%s ",FindNameByValue(bit));
		}
		bit >>= 1;
	}

	properties = D->systemFlags;	
	bit = START_BIT;
	while (properties && bit)
	{
		// dont write this out in keywords see FAKE_NOCONCEPTLIST - these go in DICTn file
		if (properties & bit && !(bit & (PATTERN_WORD|NOCONCEPTLIST)))
		{
			char* name = FindSystemNameByValue(bit);
			properties ^= bit;
			fprintf(out,(char*)"%s ",name);
		}
		bit >>= 1;
	}
	if (D->internalBits & FAKE_NOCONCEPTLIST) fprintf(out,(char*)"%s",(char*)"NOCONCEPTLIST ");
	if (D->internalBits & UPPERCASE_MATCH) fprintf(out,(char*)"%s",(char*)"UPPERCASE_MATCH ");
	fprintf(out,(char*)"%s",(char*)"( ");

	size_t lineSize = 0;
	NextInferMark();

	FACT* F = GetObjectNondeadHead(D);
	if (F)
	{
		char* word = AllocateStack(NULL,MAX_BUFFER_SIZE);
		while (F) 
		{
			if (build == BUILD1 && !(F->flags & FACTBUILD1)) {;} // defined by earlier level
			else if (build == BUILD2 && !(F->flags & FACTBUILD2)) {;} // defined by earlier level
			else if (F->verb == Mmember|| F->verb == Mexclude) // the only relevant facts
			{
				WORDP E = Meaning2Word(F->subject);
				AddInternalFlag(E,BEEN_HERE);
				if (*E->word == '"') // change string to std token
				{
					strcpy(word,E->word+1);
					size_t len = strlen(word);
					word[len-1] = ' ';			// remove trailing quote
					ForceUnderscores(word); 
				}
				else if (F->flags & ORIGINAL_ONLY) sprintf(word,(char*)"'%s ",WriteMeaning(F->subject,true));
				else sprintf(word,(char*)"%s ",WriteMeaning(F->subject,true));

				char* dict = strchr(word+1,'~'); // has a wordnet attribute on it
				if (*word == '~' || dict  ) // concept or full wordnet word reference
				{
					if (E->inferMark != inferMark) SetTriedMeaning(E,0);
					E->inferMark = inferMark; 
					if (dict)
					{
						unsigned int which = atoi(dict+1);
						if (which) // given a meaning index, mark it
						{
							uint64 offset = 1ull << which;
							SetTriedMeaning(E,GetTriedMeaning(E) | offset);	
						}
					}
				}

				// write it out- this INVERTS the order now and when read back in, will be reestablished correctly 
				// but dictionary storage locations will be inverted
				if (F->verb == Mexclude) fwrite((char*)"!",1,1,out);

				size_t wlen = strlen(word);
				if (F->botBits) // add restrictor 
				{
#ifdef WIN32
					sprintf(word+wlen-1,(char*)"`%I64u ",F->botBits); 
#else
					sprintf(word+wlen-1,(char*)"`%llu ",F->botBits); 
#endif
					wlen = strlen(word);
				}
				fwrite(word,1,wlen,out);
				lineSize += wlen;
				if (lineSize > 500) // avoid long lines
				{
					fprintf(out,(char*)"%s",(char*)"\r\n    ");
					lineSize = 0;
				}
				KillFact(F);
			}
			F = GetObjectNondeadNext(F);
		}
		ReleaseStack(word);
	}

	fprintf(out,(char*)"%s",(char*)")\r\n");
	fclose(out); // dont use Fclose
}

static void WriteDictionaryChange(FILE* dictout, unsigned int build)
{
	// Note that topic labels (topic.name) and pattern words  will not get written
	FILE* in = NULL;
    char file[SMALL_WORD_SIZE];
	int layer = 0;
	if ( build == BUILD0) 
	{
		sprintf(file,(char*)"%s/prebuild0.bin",tmp);
		in = FopenReadWritten(file);
		layer = 0;
	}
	else if ( build == BUILD1) 
	{
		sprintf(file,(char*)"%s/prebuild1.bin",tmp);
		in = FopenReadWritten(file);
		layer = 1;
	}
	if (!in)  
	{
		ReportBug((char*)"prebuild bin not found")
		return;
	}
	for (WORDP D = dictionaryBase+1; D < dictionaryFree; ++D) 
	{
		uint64 oldproperties = 0;
		uint64 oldflags = 0;
		bool notPrior = false;
		if (D < dictionaryPreBuild[layer]) // word preexisted this level, so see if it changed
		{
			unsigned int offset = D - dictionaryBase;
			unsigned int xoffset;
			int result = fread(&xoffset,1,4,in);
			if (result != 4) // ran out
			{
				int xx = 0;
				break;
			}
			if (xoffset != offset) 
				(*printer)((char*)"%s",(char*)"Bad dictionary change test\r\n");
			fread(&oldproperties,1,8,in);
			fread(&oldflags,1,8,in);
			fread(&xoffset,1,4,in); //old internal
			char junk;
			fread(&junk,1,1,in); // multiword header info
			fread(&junk,1,1,in); // 0 marker
			if (junk != 0) 
				(*printer)((char*)"%s",(char*)"out of dictionary change data2?\r\n"); // multiword header 
		}
		else notPrior = true;

		if (!D->word || *D->word == USERVAR_PREFIX) continue;		// dont variables
		if (!strnicmp(D->word, "jo-", 3) || !strnicmp(D->word, "ja-", 3)) continue; // no need to note json composites
		if (*D->word == '~' && !( D->systemFlags & NOCONCEPTLIST) ) continue;		// dont write topic names or concept names, let keywords do that and  no variables
		if (D->internalBits & FUNCTION_BITS) continue;	 // functions written out in macros file.

		if (D->internalBits & QUERY_KIND && D->internalBits & build && *D->word != '@' && *D->word != '#' && *D->word != '_') 
		{
			fprintf(dictout,(char*)"+query %s \"%s\" \r\n",D->word,D->w.userValue); // query defn , not a rename
			continue;
		}

		uint64 prop = D->properties;
		uint64 flags = D->systemFlags;
		if ((*D->word == '_' || *D->word == '@' || *D->word == '#' ) && D->internalBits & RENAMED) 
		{
			if (!notPrior) continue;	// written out before
		}
		else if (D->properties & AS_IS) 
		{
			RemoveProperty(D,AS_IS); // fact field value
			uint64 prop1 = D->properties;
			prop1 &= -1LL ^ oldproperties;
			uint64 sys1 = flags;
			sys1 &= -1LL ^ oldflags;
			sys1 &= -1LL ^ (NO_EXTENDED_WRITE_FLAGS); // we dont need these- concepts will come from keywords file
			if ( (build == BUILD0 && D < dictionaryPreBuild[LAYER_0] ) ||
				(build == BUILD1 && D < dictionaryPreBuild[LAYER_1]) ||
				(build == BUILD2 && D < dictionaryPreBuild[LAYER_BOOT]))
			{
				if (!prop1 && !sys1) continue;	// no need to write out, its in the prior world (though flags might be wrong)
			}
		}
		else if (D->systemFlags & SUBSTITUTE_RECIPIENT) continue; // ignore pattern words, etc EXCEPT when field of a fact
		else if (D->systemFlags & NO_EXTENDED_WRITE_FLAGS && !GetSubjectNondeadHead(D) && !GetVerbNondeadHead(D) && !GetObjectNondeadHead(D)) continue; // ignore pattern words, etc EXCEPT when field of a fact
		else if (D->properties & (NOUN_NUMBER|ADJECTIVE_NUMBER)  && IsDigit(*D->word)) continue; // no numbers
		else if (!D->properties && D->internalBits & UPPERCASE_HASH && !D->systemFlags) continue; // boring uppercase pattern word, just not marked as pattern word because its uppercase

		char* at = D->word - 1;
		while (IsDigit(*++at)){;}
		if (*at == 0) continue;  // purely a number - not allowed to write it out. not allowed to have unusual flags

		// only write out changes in flags and properties
		D->properties &= -1LL ^ oldproperties; // remove the old properties
		D->systemFlags &= -1 ^  oldflags; // remove the old flags

		// if the ONLY change is an existing word got made into a concept, dont write it out anymore
		if (!D->properties && !D->systemFlags && D->internalBits & CONCEPT && D <= dictionaryPreBuild[LAYER_0] ) {;}  // preexisting word a concept
		else if (D->properties || D->systemFlags || notPrior ||  ((*D->word == '_' || *D->word == '@' || *D->word == '#') && D->internalBits & RENAMED))  // there were changes
		{
			fprintf(dictout,(char*)"+ %s ",D->word);
			if ((*D->word == '_' || (*D->word == '@')) && D->internalBits & RENAMED) fprintf(dictout,(char*)"%d",(unsigned int)D->properties); // rename value
			else if (*D->word == '#' &&  D->internalBits & RENAMED)
			{
				int64 x = (int64)D->properties;
				if (D->systemFlags & CONSTANT_IS_NEGATIVE) 
				{
					fprintf(dictout,(char*)"%c",'-');
					x = -x;
				}
#ifdef WIN32
				fprintf(dictout,(char*)"%I64d",x); 
#else
				fprintf(dictout,(char*)"%lld",x); 
#endif
			}
			else
			{
				char flags[MAX_WORD_SIZE];
				WriteDictionaryFlags(D, flags); // write the new
				fprintf(dictout, "%s", flags);
			}
			fprintf(dictout,(char*)"%s",(char*)"\r\n");
		}
		D->properties = prop;
		D->systemFlags = flags;
	}
	fclose(in); // dont use Fclose
}

static void WriteExtendedFacts(FILE* factout,FILE* dictout,unsigned int build)
{
	if (!factout || !dictout) return;

	char* buffer = AllocateBuffer();
	bool oldshared = shared;
	shared = false;
	char* ptr = WriteUserVariables(buffer,false,true,NULL);
	shared = oldshared;
	fwrite(buffer,ptr-buffer,1,factout);
	FreeBuffer();

	WriteDictionaryChange(dictout,build);
	if (build == BUILD0) WriteFacts(factout,factsPreBuild[LAYER_0]);
	else if (build == BUILD1) WriteFacts(factout,factsPreBuild[LAYER_1]);
	//else if (build == BUILD2) WriteFacts(factout,factsPreBuild[LAYER_BOOT],FACTBUILD2);
	// factout closed by Writefacts
}

static void ClearTopicConcept(WORDP D, uint64 build)
{
	unsigned int k = (ulong_t) build;
	if ((D->internalBits & (TOPIC | CONCEPT)) & k)   RemoveInternalFlag(D,CONCEPT|BUILD0|BUILD1|BUILD2|TOPIC);
}

static void DumpErrors()
{
	if (errorIndex) Log(ECHOSTDTRACELOG,(char*)"\r\n ERROR SUMMARY: \r\n");
	for (unsigned int i = 0; i < errorIndex; ++i) Log(ECHOSTDTRACELOG,(char*)"  %s",errors[i]);
}

static void DumpWarnings()
{
	if (warnIndex) Log(ECHOSTDTRACELOG,(char*)"\r\nWARNING SUMMARY: \r\n");
	for (unsigned int i = 0; i < warnIndex; ++i) 
	{
		if (strstr(warnings[i],(char*)"is not a known word")) {}
		else if (strstr(warnings[i],(char*)" changes ")) {}
		else if (strstr(warnings[i],(char*)"is unknown as a word")) {}
		else if (strstr(warnings[i],(char*)"in opposite case")){}
		else if (strstr(warnings[i],(char*)"a function call")){}
        else if (strstr(warnings[i], (char*)"multiple spellings")) {}
        else Log(ECHOSTDTRACELOG,(char*)"  %s",warnings[i]);
	}
}

static void EmptyVerify(char* name, uint64 junk)
{
	char* x = strstr(name,(char*)"-b");
	if (!x) return;
	char c = (buildID == BUILD0) ? '0' : '1';
	if (x[2] == c) unlink(name);
}

int ReadTopicFiles(char* name,unsigned int build,int spell)
{
	int resultcode = 0;
    nospellcheck = false;
	undefinedCallThreadList = 0;
	isDescribe = false;
	*botheader = 0;
	myBot = 0;
	globalBotScope = false;
	if (build == BUILD2) // for dynamic segment, we are allowed full names
	{
		strcpy(baseName,name+5);
		char* dot = strchr(baseName,'.');
		*--dot = 0; // remove the 2 at the end
		char dir[SMALL_WORD_SIZE];
		sprintf(dir,"%s/BUILD%s",topic,baseName);
		MakeDirectory(dir);
	}
	else if (build == BUILD1) 
	{
		strcpy(baseName,(char*)"1");
		char dir[200];
		sprintf(dir,"%s/BUILD1",topic);
		MakeDirectory(dir);
	}
	else 
	{
		char dir[200];
		sprintf(dir,"%s/BUILD0",topic);
		MakeDirectory(dir);
		strcpy(baseName,(char*)"0");
	}

	char* output = testOutput;
	testOutput = NULL;
	FILE* in = FopenReadNormal(name); // default was top level chatscript
	if (!in)
	{
		char file[SMALL_WORD_SIZE];
        if (*buildfiles)
        {
            sprintf(file, (char*)"%s/%s", buildfiles, name); // 2nd default is rawdata itself
            in = FopenReadNormal(file);
        }
        if (!in)
        {
            sprintf(file, (char*)"RAWDATA/%s", name); // 2nd default is rawdata itself
            in = FopenReadNormal(file);
        }
        if (!in)
		{
			sprintf(file,(char*)"private/%s",name); // 3rd default is private
			in = FopenReadNormal(file);
			if (!in)
			{
				sprintf(file,(char*)"../%s",name); // 4th default is just above chatscript folder
				in = FopenReadNormal(file);
				if (!in)
				{
					(*printer)((char*)"%s not found\r\n",name);
					return 4;
				}
			}
		}
	}
	lastDeprecation = 0;
	hasPlans = 0;
	char word[MAX_WORD_SIZE];
	buildID = build;				// build 0 or build 1
	*duplicateTopicName = 0;	// an example of a repeated topic name found
	missingFiles = 0;
	spellCheck = spell;			// what spell checking to perform

	//   erase facts and dictionary to appropriate level
	ClearUserVariables();
	if (build == BUILD2) ReturnToAfterLayer(2, false); // layer 2 is boot layer before a user 2 layer. rip dictionary back to start of build (but props and systemflags can be wrong)
	else if (build == BUILD1) ReturnToAfterLayer(0,true); // rip dictionary back to start of build (but props and systemflags can be wrong)
	else  ReturnDictionaryToWordNet();
	WalkDictionary(ClearTopicConcept,build);				// remove concept/topic flags from prior defined by this build
	EraseTopicFiles(build,baseName);
	char file[SMALL_WORD_SIZE];
	sprintf(file,(char*)"%s/missingLabel.txt",topic);
	remove(file);
	sprintf(file,(char*)"%s/missingSets.txt",topic);
	remove(file);

	WalkDirectory((char*)"VERIFY",EmptyVerify,0,false); // clear verification of this level

	compiling = true;
	errorIndex = warnIndex = hasWarnings = hasErrors =  0;
	substitutes = cases = functionCall = badword = 0;

	//   store known pattern words in pattern file that we want to recognize (not spellcorrect on input)
	char filename[SMALL_WORD_SIZE];
	sprintf(filename,(char*)"%s/BUILD%s/patternWords%s.txt",topic,baseName,baseName);
	patternFile = FopenUTF8Write(filename);
	if (!patternFile)
	{
		(*printer)((char*)"Unable to create %s? Make sure this directory exists and is writable.\r\n",filename);
		return 4;
	}
	sprintf(filename,(char*)"%s/BUILD%s/map%s.txt",topic,baseName,baseName);
	mapFile = FopenUTF8Write(filename);
    fprintf(mapFile, "\r\n"); // so bytemark not with data
    fprintf(mapFile, "# file: 0 full_path_to_file optional_botid\r\n"); // so bytemark not with data
    fprintf(mapFile, "# macro: start_line_in_file name_of_macro optional_botid (definition of user function)\r\n"); // so bytemark not with data
    fprintf(mapFile, "# line: start_line_in_file offset_byte_in_script (action unit in output) \r\n"); // so bytemark not with data
    fprintf(mapFile, "# concept: start_line_in_file name_of_concept optional_botid (concept definition) \r\n"); // so bytemark not with data
    fprintf(mapFile, "# topic: start_line_in_file name_of_topic optional_botid (topic definition) \r\n"); // so bytemark not with data
    fprintf(mapFile, "# rule: start_line_in_file full_rule_tag_with_possible_label rule_kind (rule definition) \r\n"); // so bytemark not with data
    fprintf(mapFile, "# Complexity of name_of_macro complexity_metric (complexity metric for function) \r\n"); // so bytemark not with data
    fprintf(mapFile, "# Complexity of rule full_rule_tag_with_possible_label rule_kind complexity_metric (complexity metric for rule) \r\n"); // so bytemark not with data
    fprintf(mapFile, "# bot: name_of_macro_it_happens_in botid (possible bot macro) \r\n"); // so bytemark not with data
    fprintf(mapFile, "\r\n"); // so bytemark not with data
    AllocateOutputBuffer();

	// init the script output file
	sprintf(filename,(char*)"%s/BUILD%s/script%s.txt",topic,baseName,baseName);
	FILE* out = FopenUTF8Write(filename);
	if (strlen(name) > 100) name[99] = 0;
	if (!strnicmp(name,(char*)"files",5)) name += 5; // dont need the prefix
	char* at = strchr(name,'.');
	*at = 0;
	fprintf(out,(char*)"0     %s %s %s\r\n",GetMyTime(time(0)),name,version); //   reserve 5-digit count for number of topics + timestamp  (AFTER BOM)
	fclose(out); // dont use fclose
	
	uint64 oldtokenControl = tokenControl;
	tokenControl = 0;
	topicCount = 0;
	StartScriptCompiler();
	
	//   read file list to service, may also have bot: commands
	while (ReadALine(readBuffer,in) >= 0)
	{
		char* at = ReadCompiledWord(readBuffer,word);
		if (*word == '#' || !*word) continue;
		if (!stricmp(word,(char*)"stop") || !stricmp(word,(char*)"exit")) break; //   fast abort

		if (!stricmp(word,"bot:"))
		{
			globalBotScope = true; // lasts til changed
			ReadBot(at);
			continue;
		}

		size_t len = strlen(word);
		char output[MAX_WORD_SIZE];
		if (word[len-1] == '/') // directory request
		{
			Log(STDTRACELOG,(char*)"\r\n>>Reading folder %s\r\n",word);
            bool recurse = word[len - 2] == '/';
            if (recurse) word[len - 2] = 0;
            WalkDirectory(word,ReadTopicFile,build,recurse); // read all files in folder (top level)
			Log(STDTRACELOG,(char*)"\r\n<<end folder %s\r\n",word);
		}
		else if (*word == ':' && word[1]) DoCommand(readBuffer,output); // testing command
		else ReadTopicFile(word,build|FROM_FILE); // was explicitly named
	}
	if (in) fclose(in); 
	fclose(patternFile); 
	fclose(mapFile); 
	EndScriptCompiler();
	StartFile((char*)"Post compilation Verification");
    nospellcheck = false;

	// verify errors across all files
	DoubleCheckSetOrTopic();	//   prove all sets/topics he used were defined
	DoubleCheckReuse();		// see if jump labels are defined
	DoubleCheckFunctionDefinition();
	if (*duplicateTopicName)  WARNSCRIPT((char*)"At least one duplicate topic name, i.e., %s, which may intended if bot restrictions differ.\r\n",duplicateTopicName)
	WalkDictionary(ClearBeenHere,0);

	// write out compiled data

	//   write how many topics were found (for when we preload during normal startups)
	sprintf(filename,(char*)"%s/BUILD%s/script%s.txt",topic,baseName,baseName);
	out = FopenUTF8WriteAppend(filename,(char*)"rb+");
	if (out)
	{
		fseek(out,0,SEEK_SET);
		sprintf(word,(char*)"%05d",topicCount);
		unsigned char bom[3];
		bom[0] = 0xEF;
		bom[1] = 0xBB;
		bom[2] = 0xBF;
		fwrite(bom,1,3,out);
		fwrite(word,1,5 * sizeof(char),out);
		fclose(out); // dont use Fclose
	}

	if (hasPlans)
	{
		sprintf(filename,(char*)"%s/BUILD%s/plans%s.txt",topic,baseName,baseName);
		out = FopenUTF8WriteAppend(filename,(char*)"rb+");
		if (out)
		{
			char word[MAX_WORD_SIZE];
			fseek(out,0,SEEK_SET);
			sprintf(word,(char*)"%05d",hasPlans);
			fwrite(word,1,5 * sizeof(char),out);
			fclose(out); // dont use FClose
		}
	}

	// we delay writing out keywords til now, allowing multiple accumulation across tables and concepts
	WalkDictionary(WriteConcepts,build);
	WalkDictionary(ClearBeenHere,0);

	// dump variables, dictionary changes, topic facts
	sprintf(filename,(char*)"%s/BUILD%s/facts%s.txt",topic,baseName,baseName);
	char filename1[MAX_WORD_SIZE];
	sprintf(filename1,(char*)"%s/BUILD%s/dict%s.txt",topic,baseName,baseName);
	FILE* dictout = FopenUTF8Write(filename1);
	FILE* factout = FopenUTF8Write(filename);
	WriteExtendedFacts(factout,dictout,  build); 
    fclose(dictout); // dont use FClose
	// FClose(factout); closed from within writeextendedfacts
	
	// cleanup
	buildID = 0;
	numberOfTopics = 0;
	tokenControl  = oldtokenControl;
	currentRuleOutputBase = currentOutputBase = NULL;
	FreeOutputBuffer();
	compiling = false;
	jumpIndex = 0;
	testOutput = output; // allow summary to go out the server
	if (hasErrors) 
	{
		EraseTopicFiles(build,baseName);
		DumpErrors();
		if (missingFiles) Log(ECHOSTDTRACELOG,(char*)"%d topic files were missing.\r\n",missingFiles);
		Log(ECHOSTDTRACELOG,(char*)"\r\n%d errors - press Enter to quit. Then fix and try again.\r\n",hasErrors);
		if (!server && !commandLineCompile) ReadALine(readBuffer,stdin);
		resultcode = 4; // error
	}
	else if (hasWarnings) 
	{
		DumpWarnings();
		if (missingFiles) Log(ECHOSTDTRACELOG,(char*)"%d topic files were missing.\r\n",missingFiles);
		Log(ECHOSTDTRACELOG,(char*)"%d serious warnings, %d function warnings, %d spelling warnings, %d case warnings, %d substitution warnings\r\n    ",hasWarnings-badword-substitutes-cases,functionCall,badword,cases,substitutes);
	}
	else 
	{
		if (missingFiles) Log(ECHOSTDTRACELOG,(char*)"%d topic files were missing.\r\n",missingFiles);
		Log(ECHOSTDTRACELOG,(char*)"No errors or warnings\r\n\r\n");
	}
	ReturnDictionaryToWordNet();
	Log(ECHOSTDTRACELOG,(char*)"\r\n\r\nFinished compile\r\n\r\n");
	return resultcode;
}

char* CompileString(char* ptr) // incoming is:  ^"xxx" or ^'xxxx'
{
	char tmp[MAX_WORD_SIZE * 2];
	strcpy(tmp,ptr); // protect copy from multiple readcalls
	size_t len = strlen(tmp);
	if (tmp[len-1] != '"' && tmp[len-1] != '\'') BADSCRIPT((char*)"STRING-1 String not terminated with doublequote %s\r\n",tmp)
	tmp[len-1] = 0;	// remove trailing quote

	// flip the FUNCTION marker inside the string
	static char data[MAX_WORD_SIZE * 2];	
	char* pack = data;
	*pack++ = '"';
	*pack++ = FUNCTIONSTRING;
	*pack++ = ':'; // a internal marker that is has in fact been compiled - otherwise it is a format string whose spaces count but cant fully execute

	if (tmp[2] == '(')  ReadPattern(tmp+2,NULL,pack,false,false); // incoming is:  ^"(xxx"
	else ReadOutput(false,false,tmp+2,NULL,pack,NULL,NULL,NULL);

	TrimSpaces(data,false);
	len = strlen(data);
	data[len]  = '"';	// put back closing quote
	data[len+1] = 0;
	return data;
}
#endif
