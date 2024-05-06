#include "common.h"
//------------------------
// ALWAYS AVAILABLE
//------------------------
#define PATTERNDEPTH 1000
bool nowarndupconcept = false;

static unsigned char mathphrase[20];
unsigned int supplementalColumn = 0;
static HEAPREF undefinedCallThreadList = NULL;
static HEAPREF undefinedConceptThreadList = NULL;
unsigned int currentBuild = 0;
static bool nospellcheck = false;
static bool noPatternOptimization = true;
static unsigned int conceptID = 0; // name of concept set
char* patternStarter = NULL;
char* patternEnder = NULL;
const char* linestartpoint = NULL;
static bool isConcept = false;
static WORDP currentFunctionDefinition;			// current macro defining or executing
HEAPREF deadfactsList = 0;
HEAPREF languageadjustedfactsList = 0;
static unsigned int complexity = 0;
static bool livecall = false;
bool echorulepattern = false;
static unsigned int priorLine = 0;
static char* currentTopicBots = NULL;
bool autoset = false;
static char macroName[MAX_WORD_SIZE];
static uint64 macroid;
char* dataBase = NULL;
static char* dataChunk = NULL;
static char* outputStart = NULL;
static char* lineStart = NULL;
static bool globalBotScope = false;
static HEAPREF beenHereThreadList = NULL;
char* newScriptBuffer = NULL;
char* oldScriptBuffer = NULL;
static char display[MAX_DISPLAY][100];
static int displayIndex = 0;
static char* incomingPtrSys = 0;			// cache AFTER token find ptr when peeking.
static char lookaheadSys[MAX_WORD_SIZE];	// cache token found when peeking
static unsigned int hasWarnings;			// number of warnings generated
unsigned int hasErrors;
uint64 grade = 0;						// vocabulary warning
char* lastDeprecation = 0;
CompileStatus compiling = NOT_COMPILING;			// script compiler in progress
bool patternContext = false;	// current compiling a pattern
unsigned int buildId; // current build
static int callingSystem = 0;
static bool chunking = false;
static unsigned int substitutes;
static unsigned int cases;
static unsigned int badword;
static unsigned int functionCall;
static bool isDescribe = false;
char* tableinput = NULL;
static char functionArguments[MAX_ARG_LIMIT + 1][500];
static int functionArgumentCount = 0;

char warnings[MAX_WARNINGS][MAX_WORD_SIZE];
unsigned int warnIndex = 0;
static char baseName[SMALL_WORD_SIZE];

char errors[MAX_ERRORS][MAX_WORD_SIZE];
unsigned int errorIndex = 0;
char scopeBotName[MAX_WORD_SIZE];
static bool renameInProgress = false;
static bool endtopicSeen = false; // needed when ending a plan
static char* nextToken;			// current lookahead token

unsigned int buildID = 0;

static char* topicFiles[] = //   files created by a topic refresh from scratch 
{
	(char*)"describe",		//   document variables functions concepts topics etc
	(char*)"variables",		//   hold variables	
	(char*)"facts",			//   hold facts	
	(char*)"allfacts",		//   hold all binary fast facts	
	(char*)"allwords",		//   hold all binary fast words	
	(char*)"keywords",		//   holds topic and concepts keywords
	(char*)"macros",			//   holds macro definitions
	(char*)"map",			//   where things are defined (will also do map.json)
	(char*)"script",			//   hold topic definitions
	(char*)"plans",			//   hold plan definitions
	(char*)"dict",			//   dictionary changes	
	(char*)"canon",			//   private canonical values 	
	0
};
static void WriteKey(char* word);
static FILE* mapFile = NULL;					// for IDE
static FILE* mapFileJson = NULL;                // easier to parse
static char* ReadMacro(char* ptr, FILE* in, char* kind, unsigned int build,char* data);
static unsigned int mapTopicFileCount = 0;
static unsigned int mapItemCount = 0;
static unsigned int mapRuleCount = 0;

void EraseTopicBin(unsigned int build, char* name)
{
	int i = -1;
	int result;
	char file[SMALL_WORD_SIZE];
	while (topicFiles[++i])
	{
		sprintf(file, (char*)"%s/BUILD%s/%s%s.bin", topicfolder, name, topicFiles[i], name); // new style
		result = remove(file);
	}
	sprintf(file, (char*)"%s/BUILD%s/%sallfacts.bin", topicfolder, name, name); // new style
	result = remove(file);
	sprintf(file, (char*)"%s/BUILD%s/%sallwords.bin", topicfolder, name, name); // new style
	result = remove(file);
}

void InitScriptSystem()
{
	compiling = NOT_COMPILING;
	undefinedCallThreadList = NULL;
	undefinedConceptThreadList = NULL;
	oldScriptBuffer = NULL;
	newScriptBuffer = NULL;
	mapFile = NULL;
    mapFileJson = NULL;
	outputStart = NULL;
 }

void AddWarning(char* buffer)
{
    char c = buffer[MAX_WORD_SIZE - 300];
	size_t len = strlen(buffer);
    if (len > (MAX_WORD_SIZE - 300)) buffer[MAX_WORD_SIZE - 300] = 0;
	bool chop = false;
	if (buffer[len - 2] == '\r') buffer[len - 2] = 0;
	sprintf(warnings[warnIndex], (char*)"%s ending line %u col %u", buffer, currentFileLine, currentLineColumn);
	char* ptr = warnings[warnIndex] + strlen(warnings[warnIndex]);
	if (*currentFilename) sprintf(ptr, " of %s", currentFilename);
	warnIndex++;
	buffer[MAX_WORD_SIZE - 300] = c;
	strcat(buffer, "\r\n");
    
    if (strstr(warnings[warnIndex-1],(char*)"is not a known word")) {++badword;}
	else if (strstr(warnings[warnIndex-1],(char*)" changes ")) {++substitutes;}
	else if (strstr(warnings[warnIndex-1],(char*)"is unknown as a word")) {++badword;}
	else if (strstr(warnings[warnIndex-1],(char*)"in opposite case")){++cases;}
    else if (strstr(warnings[warnIndex - 1], (char*)"multiple spellings")) { ++cases; }
    else if (strstr(warnings[warnIndex-1],(char*)"a function call")){++functionCall;}
	if (warnIndex >= MAX_WARNINGS) --warnIndex;
}

bool StartScriptCompiler(bool normal)
{
#ifndef DISCARDSCRIPTCOMPILER
    if (nextToken && normal) return false; // we are already in a build
    if (oldScriptBuffer) return false; // already running one
    conceptID = 0;
    beenHereThreadList = NULL;
    livecall = !normal;
    oldScriptBuffer = newScriptBuffer; // so we can nest calls to script compiler
    warnIndex = errorIndex = 0;
    newScriptBuffer = AllocateStack(NULL, maxBufferSize);
    nextToken = AllocateStack(NULL, maxBufferSize); // able to swallow big token
    
#endif
    return true;
}

void EndScriptCompiler()
{
#ifndef DISCARDSCRIPTCOMPILER
    if (newScriptBuffer)
    {
        ReleaseStack(newScriptBuffer);
        newScriptBuffer = oldScriptBuffer;
        oldScriptBuffer = NULL;
        nextToken = NULL;
    }
#endif
}

void ScriptError()
{
#ifndef DISCARDSCRIPTCOMPILER
    callingSystem = 0;
    chunking = false;
    outputStart = NULL;
    renameInProgress = false;
    if (compiling || csapicall == TEST_OUTPUT || csapicall == TEST_PATTERN)
    {
		++hasErrors;
        patternContext = false;
        if (*scopeBotName) Log(USERLOG,"*** Error- line %d col %d of %s bot:%s : ", currentFileLine, currentLineColumn + supplementalColumn, currentFilename, scopeBotName);
        else Log(USERLOG,"*** Error- line %d col %d of %s: ", currentFileLine, currentLineColumn + supplementalColumn, currentFilename);
    }
#endif
}

#ifndef DISCARDSCRIPTCOMPILER

void ScriptWarn()
{
	if (compiling)
	{
		++hasWarnings; 
		if (*currentFilename)
		{
			if (*scopeBotName) Log(USERLOG,"*** Warning- line %d col %d of %s bot:%s : ", currentFileLine, currentLineColumn + supplementalColumn,currentFilename, scopeBotName);
			else Log(USERLOG,"*** Warning- line %d col %d of %s: ", currentFileLine, currentLineColumn+supplementalColumn,currentFilename);
		}
		else Log(USERLOG,"*** Warning-  ");
	}
}

static void AddBeenHere(WORDP D)
{
    D->internalBits |= BEEN_HERE;
    beenHereThreadList = AllocateHeapval(HV1_WORDP,beenHereThreadList, (uint64)D);// save name
}

void UnbindBeenHere()
{
    while (beenHereThreadList)
    {
        uint64 D;
        beenHereThreadList = UnpackHeapval(beenHereThreadList, D,discard);
        ((WORDP)D)->internalBits &=  -1 ^ BEEN_HERE;
    }
}

#endif

void AddError(char* buffer)
{
	char seen[MAX_WORD_SIZE];
	*seen = 0;
	char* at = seen;
	if (patternStarter && patternEnder && csapicall != COMPILE_PATTERN && csapicall != COMPILE_OUTPUT)
	{
		strcpy(at, "--> ");
		at += 4;
		size_t len = patternEnder - patternStarter;
		*patternEnder = 0;
		if (len < 100) strcpy(at, patternStarter);
		else
		{
			strncpy(at, patternStarter,50);
			at += 50;
			sprintf(at, "%s"," ... ");
			sprintf(at + 5, "%s", patternEnder - 50);
		}
		strcat(at, " <--");
	}

	char message[MAX_WORD_SIZE];
	if (*buffer == '\r') ++buffer;
	if (*buffer == '\n') ++buffer;
	size_t len = strlen(buffer);
    char* ptr = buffer + ( len > MAX_WORD_SIZE - 300 ? MAX_WORD_SIZE - 300 : len) - 1;
    while (ptr > buffer && (*ptr == '\n' || *ptr == '\r')) --ptr;
    char c = *(++ptr);
    *ptr = 0;
	if (!*currentFilename) // dse compilepattern
	{
		sprintf(message, "%s ended %s line %u col %u ", buffer, seen, currentFileLine, currentLineColumn+supplementalColumn);
		strcat(message, "\r\n");
	}
	else sprintf(message, "%s ended line %u col %u of %s %s\r\n", buffer, currentFileLine, currentLineColumn+supplementalColumn, currentFilename, scopeBotName);
    *ptr = c;
	sprintf(errors[errorIndex++], (char*)"%s\r\n", message);
	if (errorIndex >= MAX_ERRORS) --errorIndex;
}

static char* FindComparison(char* word)
{
    if (!*word || !word[1] || !word[2] || *word == '"') return NULL; //   if token is short, we cannot do the below word+1 scans 
    if (*word == '.') return NULL; //  .<_3 is not a comparison
    if (*word == '\\') return NULL; // escaped is not a comparison
    if (*word == '!' && word[1] == '?' && word[2] == '$') return NULL;
    if (*word == '_' && word[1] == '?' && word[2] == '$') return NULL;
    if (*word == '?' && word[1] == '$') return NULL;
    char* at = strchr(word + 1, '!');
    if (at && *word == '!') at = NULL;	 // ignore !!
    if (!at)
    {
        at = strchr(word + 1, '<');
        if (at && at[1] == '<') return NULL; // << is not a comparison
    }
    if (!at)
    {
        at = strchr(word + 1, '>');
        if (at && at[1] == '>') return NULL; // >> is not a comparison
    }
    if (!at)
    {
        at = strchr(word + 1, '&');
        if (at && (at[1] == '_' || at[1] == ' ' || !(IsDigit(at[1]) || at[1] == USERVAR_PREFIX || at[1] == '#'))) at = 0;	// ignore & as part of a name, e.g. AT&T
    }
    if (!at) at = strchr(word + 1, '=');
    if (!at) at = strchr(word + 1, '?');  //   member of set
    if (!at)
    {
        at = strchr(word + 1, '!');  //   negation
        if (at && (at[1] == '=' || at[1] == '?'));
        else at = NULL;
    }
    return at;
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
        if (at[1] == '=' || at[1] == '?') InsureAppropriateCase(at + 2); // == or >= or such
        else InsureAppropriateCase(at + 1);
        *at = c;
    }
    else if (*word == '_' || *word == '\'') InsureAppropriateCase(word + 1);
    else if (*word == USERVAR_PREFIX)
    {
        char* dot = strchr(word, '.');
        if (dot) *dot = 0;
        MakeLowerCase(word);
        if (dot) *dot = '.';
    }
    else if ((*word == '^' && word[1] != '"') || *word == '~' || *word == SYSVAR_PREFIX || *word == '|') MakeLowerCase(word);
    else if (*word == '@' && IsDigit(word[1])) MakeLowerCase(word);	//   potential factref like @2subject
}

static int GetFunctionArgument(char* arg) //   get index of argument (1-based) if it is value, else -1
{
    for (int i = 1; i <= functionArgumentCount; ++i)
    {
        if (!stricmp(arg, functionArguments[i])) return i;
    }
    return -1; //   failed
}

static void FindDeprecated(char* ptr, char* value, char* message)
{
    char* comment = strstr(ptr, (char*)"# ");
    char* at = ptr;
    size_t len = strlen(value);
    while (at)
    {
        at = strstr(at, value);
        if (!at) break;
        if (*(at - 1) == USERVAR_PREFIX) // $$xxx should be ignored
        {
            at += 2;
            continue;
        }
        if (comment && at > comment) return; // inside a comment
        char word[MAX_WORD_SIZE];
        ReadCompiledWord(at, word);
        if (!stricmp(value, word))
        {
            lastDeprecation = at;
            BADSCRIPT(message);
        }
        at += len;
    }
}

static void AddDisplay(char* word)
{
    MakeLowerCase(word);
    for (int i = 0; i < displayIndex; ++i)
    {
        if (!strcmp(word, display[i])) return;	// no duplicates needed
    }
    strcpy(display[displayIndex], word);
    if (++displayIndex >= MAX_DISPLAY) BADSCRIPT("Display argument limited to %d:  %s\r\n", MAX_DISPLAY, word)
}

static char* ReadDisplay(FILE* in, char* ptr)
{
    char word[SMALL_WORD_SIZE];
    ptr = ReadNextSystemToken(in, ptr, word, false);
    while (1)
    {
        ptr = ReadNextSystemToken(in, ptr, word, false);
        if (*word == ')') break;
        if (*word != USERVAR_PREFIX)
            BADSCRIPT("Display argument must be uservar of $$ $ or $_: %s\r\n", word)
            if (strchr(word, '.'))
                BADSCRIPT("Display argument cannot be dot-selected %s\r\n", word)
                AddDisplay(word); // explicit display
    }
    return ptr;
}

char* ReadSystemToken(char* ptr, char* word, bool separateUnderscore) //   how we tokenize system stuff (rules and topic system) words -remaps & to AND
{
    *word = 0;
    if (!ptr)  return 0;
    char tmp[MAX_WORD_SIZE];
    char* start = word;
    *start = 0;
    ptr = SkipWhitespace(ptr);

	while (compiling == PIECE_COMPILE && *ptr == '\\' && ptr[1] == 'n') // api calls
	{
		ptr = SkipWhitespace(ptr + 2); 
		currentFileLine += 1;
		currentLineColumn = 0;
	}

    FindDeprecated(ptr, (char*)"$login", (char*)"Deprecated $login needs to be $cs_login\r\n");
    FindDeprecated(ptr, (char*)"$userfactlimit", (char*)"Deprecated $userfactlimit needs to be $cs_userfactlimit\r\n");
    FindDeprecated(ptr, (char*)"$crashmsg", (char*)"Deprecated $crashmsg needs to be $cs_crashmsg\r\n");
    FindDeprecated(ptr, (char*)"$randindex", (char*)"Deprecated $randindex needs to be $cs_randindex\r\n");
    FindDeprecated(ptr, (char*)"$wildcardseparator", (char*)"Deprecated $wildcardseparator needs to be $cs_wildcardseparator\r\n");
    FindDeprecated(ptr, (char*)"$abstract", (char*)"Deprecated $abstract needs to be $cs_abstract\r\n");
    FindDeprecated(ptr, (char*)"$prepass", (char*)"Deprecated $prepass needs to be $cs_prepass\r\n");
    FindDeprecated(ptr, (char*)"$control_main", (char*)"Deprecated $control_main needs to be $cs_control_main\r\n");
    FindDeprecated(ptr, (char*)"$control_pre", (char*)"Deprecated $control_pre needs to be $cs_control_pre\r\n");
    FindDeprecated(ptr, (char*)"$control_post", (char*)"Deprecated $control_post needs to be $cs_control_post\r\n");
#ifdef INFORMATION
	/***
    A token is nominally a contiguous collection of characters broken off by tab or space(since return and newline are stripped off).
        Tokens to include whitespace are encased in doublequotes.

        Characters with reserved status automatically also break into individual tokens and to include them you must put \ before them.These include :
    []() {}  always and separate into individual tokens except for _(_[_{

        < > and << >> are reserved, but only when at start or end of token.Allowed comparisons embedded.As is <= and >=
        Tokens ending with ' or 's break off(possessive) in patterns.

        Tokens starting with prefix characters ' or ! or _ keep together, except per reserved tokens.	'$junk  is one token.
        Variables ending with punctuation separate the punctuation.$hello.is two tokens as is _0.

        Reserved characters in a composite token with _ before or after are kept.E.g.This_(_story_is_)_done
        You can include a reserved tokens by putting \ in front of them.

        Some tokens revise their start, like the pattern tokens representing comparison.They do this in the script compiler.
		***/
		#endif

        // strings
		if (*ptr == '\'' && ptr[1] == '"')
		{
			*word++ = *ptr++;
		}
        if (*ptr == '"' || (*ptr == '^' && ptr[1] == '"') || (*ptr == '^' && ptr[1] == '\'') || (*ptr == '\\' && ptr[1] == '"')) //   doublequote maybe with functional heading
        {
            // simple \"
            if (*ptr == '\\' && (!ptr[2] || ptr[2] == ' ' || ptr[2] == '\t' || ptr[2] == '}' || ptr[2] == ENDUNIT)) // legal
            {
                *word = '\\';
                word[1] = '"';
                word[2] = 0;
                return ptr + 2;
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
            if (end)
                {
                    if (*word == '"' && word[1] != FUNCTIONSTRING && !functionString) return end; // all legal within
                                                                                                  // NOW WE SEE A FUNCTION STRING
                                                                                                  // when seeing ^, see if it remaps as a function argument
                                                                                                  // check for internal ^ also...
                    char* hat = word - 1;
                    if ((*word == '"' || *word == '\'') && functionString) hat = word; // came before
                    else if (*word == '"' && word[1] == FUNCTIONSTRING) hat = word + 1;
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

                    while ((hat = strchr(hat + 1,'^'))) // find a hat within
                    {
                        if (IsDigit(hat[1])) continue; // normal internal
                        if (*(hat - 1) == '\\') continue;	// escaped
                        char* atx = hat;
                        while (*++atx && (IsAlphaUTF8OrDigit(*atx) || *atx == '_')) { ; }
                        char c = *atx;
                        *atx = 0;
                        int index = GetFunctionArgument(hat);
                        WORDP D = FindWord(hat); // in case its a function name
                        *atx = c;
                        if (index >= 0) // was a function argument
                        {
                            strcpy(tmp,atx); // protect chunk
                            sprintf(hat,(char*)"^%d%s",index,tmp);
                        }
                        else if (D && D->internalBits & FUNCTION_NAME) { ; }
                        else if (!renameInProgress && !(hat[1] == USERVAR_PREFIX || hat[1] == MATCHVAR_PREFIX))
                        {
                            *atx = 0;
                            WARNSCRIPT((char*)"%s is not a recognized function argument. Is it intended to be?",hat)
                                *atx = c;
                        }
                    }

                    hat = word - 1;
                    while ((hat = strchr(hat + 1,'_'))) // rename _var? 
                    {
						if (*(hat - 1) == '$') continue;
                        if (IsAlphaUTF8OrDigit(*(hat - 1)) || *(hat - 1) == '_' || *(hat - 1) == '-') continue; // not a starter
                        if (IsDigit(hat[1])) continue; // normal _ var
                        if (*(hat - 1) == '\\' || *(hat - 1) == '"') continue;	// escaped or quoted
                        char* atx = hat;
                        while (*++atx && (IsAlphaUTF8OrDigit(*atx))) { ; } // find end
                        WORDP D = FindWord(hat,atx - hat,LOWERCASE_LOOKUP);
                        if (D && D->internalBits & RENAMED) // remap matchvar inside  string
                        {
                            strcpy(tmp,atx); // protect chunk
                            sprintf(hat + 1,(char*)"%u%s",(unsigned int)D->properties,tmp);
                        }
                    }

                    hat = word - 1;
                    while ((hat = strchr(hat + 1,'@'))) // rename @set?  
                    {
                        if (IsAlphaUTF8OrDigit(*(hat - 1))) continue; // not a starter
                        if (IsDigit(hat[1]) || hat[1] == '_') continue; // normal @ var or @_marker
                        if (*(hat - 1) == '\\') continue;	// escaped
                        char* atx = GetSetEnd(hat);
                        WORDP D = FindWord(hat,atx - hat,LOWERCASE_LOOKUP);
                        if (D && D->internalBits & RENAMED)  // rename @set inside string
                        {
                            strcpy(tmp,atx); // protect chunk
                            sprintf(hat + 1,(char*)"%u%s",(unsigned int)D->properties,tmp);
                        }
                        else if (!renameInProgress)  // can do anything safely in a simple quoted string
                        {
                            char c = 0;
                            if (at && *at)
                            {
                                c = *at;
                                *at = 0;
                            }
                            WARNSCRIPT((char*)"%s is not a recognized @rename. Is it intended to be?\r\n",hat)
                            if (c) *at = c;
                        }
                    }
                    hat = word - 1;
                    if (strstr(readBuffer, "rename:")) // accept rename of existing constant twice in a row
                        hat = " ";
                    while ((hat = strchr(hat + 1,'#'))) // rename #constant or ##constant
                    {
                        if (*(hat - 1) == '\\') continue;	// escaped
                        if (IsAlphaUTF8OrDigit(*(hat - 1)) || IsDigit(hat[1]) || *(hat - 1) == '&') continue; // not a starter, maybe #533; constant stuff
                        char* at = hat;
                        if (at[1] == '#')  ++at;	// user constant
                        while (*++at && (IsAlphaUTF8OrDigit(*at) || *at == '_')) { ; } // find end
                        strcpy(tmp,at); // protect chunk
                        *at = 0;
                        uint64 n;
                        if (hat[1] == '#' && IsAlphaUTF8(hat[2])) // user constant
                        {
                            WORDP D = FindWord(hat,at - hat,LOWERCASE_LOOKUP);
                            if (D && D->internalBits & RENAMED)  // remap #constant inside string
                            {
                                n = D->properties;
                                if (D->internalBits & CONSTANT_IS_NEGATIVE)
                                {
                                    int64 x = (int64)n;
                                    x = -x;
                                    sprintf(hat,(char*)"%lld%s",(long long int) x,tmp);
                                }
                                else
                                {
                                    sprintf(hat,(char*)"%lld%s",(long long int) n,tmp);
                                }
                            }
                        }
                        else // system constant
                        {
                            n = FindPropertyValueByName(hat + 1);
                            if (!n) n = FindSystemValueByName(hat + 1);
                            if (!n) n = FindParseValueByName(hat + 1);
                            if (!n) n = FindMiscValueByName(hat + 1);
                            if (n)
                            {
                                sprintf(hat,(char*)"%lld%s",(long long int) n,tmp);
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
                memmove(word + 1,word,strlen(word) + 1);
                *word = '\\';
            }
            else
            {
                word = start;
                if (*start == '^') --ptr;
            }
        }

	// reading concept, careful handling for emoticons emoji
	if (isConcept)
	{
		if (!*ptr) return ptr;
		while (*ptr != ' ' && *ptr) *word++ = *ptr++;
		*word = 0;
		if (*(word - 1) == ')') // closing ) for concept, not part of emoticon?
		{
			
			if (( *start == '~' || IsAlphaUTF8(*start)) && IsAlphaUTF8(start[1]) && IsAlphaUTF8(start[2])) // clear word
			{
				--ptr;
				--word;
			}
		}
		*word = 0;
		if (start) 	return ptr; // test always true, but makes debug visible on start
	}

    // the normal composite token
    bool quote = false;
    char* xxorig = ptr;
    bool var = (*ptr == '$');
    int brackets = 0;
    char quotechar = '"';
    bool activestring = false;
    while (*ptr)
    {
        if (*ptr == ENDUNIT) break;
        if (patternContext && quote) {} // allow stuff in comparison quote  
        else if (*ptr == ' ' || (*ptr == '\t' && convertTabs) || (*ptr == '\\' && ptr[1] == 'n' && compiling == PIECE_COMPILE)) break; // legal
        if (patternContext && activestring == false && *ptr == '^' && (ptr[1] == '"' || ptr[1] == '\''))
        {
            quotechar = ptr[1];
            activestring = true;
        }

        if (patternContext && *ptr == quotechar) 
            quote = !quote;

        char c = *ptr++;
        if (c == '\t' && !convertTabs && word != start)
        {
            --ptr;
            break; // end word with tab
        }
		// not for output, but for patterns, track line numbers
		if (csapicall == COMPILE_PATTERN && c == '\\' && (*ptr == 'n' || *ptr == '\t' || *ptr == '\r')) // break off manual new line
		{
			--ptr;
			break;
		}
        *word++ = c;
        *word = 0;
        if (*start == '\t' && !convertTabs ) 
                break;    // return tab as unique word
        if ((word - start) > (MAX_WORD_SIZE - 2)) break; // avoid overflow
        if (c == '\\')  *word++ = *ptr++; //escaped
        // want to leave array json notation alone but react to [...] touching a variable - $var]
        else if (var && c == '[') // ANY variable should be separated by space from a [ if not json array
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
        else if (GetNestingData(c) && !quote) // break off nesting attached to a started token unless its an escaped token
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
        if (quote == word + len - 1) 
            BADSCRIPT("Tailing quote without start: %s\r\n", word)
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
                if (D->internalBits & CONSTANT_IS_NEGATIVE)
                {
                    int64 x = (int64)n;
                    x = -x;
                    sprintf(word,(char*)"%lld",(long long int) x);
                }
                else
                {
                    sprintf(word,(char*)"%lld",(long long int) n);
                }
            }

            else if (renameInProgress) { ; } // leave token alone, defining
            else BADSCRIPT((char*)"Bad user constant %s\r\n",word)
        }
        else // system constant
        {
            n = FindPropertyValueByName(word + 1);
            if (!n) n = FindSystemValueByName(word + 1);
            if (!n) n = FindParseValueByName(word + 1);
            if (!n) n = FindMiscValueByName(word + 1);
            if (n)
            {
                sprintf(word,(char*)"%lld",(long long int) n);
            }
            else if (!IsDigit(word[1]) && word[1] != '!') //treat rest as a comment line (except if has number after it, which is user text OR internal arg reference for function
            {
				if (!stricmp(word, "#Tokens:")) { ; }
                else if (IsAlphaUTF8(word[1]))
                    BADSCRIPT((char*)"Bad numeric # constant %s\r\n",word)
                    *ptr = 0;
                *word = 0;
            }
        }
    }
    if (*word == '_' && (IsAlphaUTF8(word[1]))) // is this a rename _
    {
        WORDP D = FindWord(word);
        if (D && D->internalBits & RENAMED) sprintf(word + 1,(char*)"%u",(unsigned int)D->properties); // remap match var convert to number
                                                                                                       // patterns can underscore ANYTING
    }
    if (*word == '\'' &&  word[1] == '_' && (IsAlphaUTF8(word[2]))) // is this a rename _ with '
    {
        WORDP D = FindWord(word + 1);
        if (D && D->internalBits & RENAMED) sprintf(word + 2,(char*)"%u",(unsigned int)D->properties); // remap match var convert to number
        else if (!renameInProgress && !patternContext)  // patterns can underscore ANYTING
            WARNSCRIPT((char*)"%s is not a recognized _rename. Should it be?\r\n",word + 1)
    }
    if (*word == '@' && IsAlphaUTF8(word[1])) // is this a rename @
    {
        char* at = GetSetEnd(word);
        WORDP D = FindWord(word,at - word);
        if (D && D->internalBits & RENAMED) // remap @set in string
        {
            strcpy(tmp,at); // protect chunk
            sprintf(word + 1,(char*)"%u%s",(unsigned int)D->properties,tmp);
        }
        else if (!renameInProgress)  WARNSCRIPT((char*)"%s is not a recognized @rename. Is it intended to be?\r\n",word)
    }
    if (*word == '@' && word[1] == '_' && IsAlphaUTF8(word[2])) // is this a rename @_0+
    {
        size_t lenx = strlen(word);
        WORDP D = FindWord(word + 1, lenx - 1); // @_data marker
        char c = 0;
        if (!D)
        {
            c = word[lenx - 1];
            word[lenx - 1] = 0;
            D = FindWord(word + 1, lenx - 2);
            word[lenx - 1] = c;
        }
        if (D && D->internalBits & RENAMED)
        {
            if (c) sprintf(word + 2,(char*)"%u%c",(unsigned int)D->properties,c); // remap @set in string
            else sprintf(word + 2, (char*)"%u", (unsigned int)D->properties); // remap @set in string
        }
        else if (!renameInProgress)  WARNSCRIPT((char*)"%s is not a recognized @rename. Is it intended to be?\r\n",word)
    }

    // some tokens require special splitting

    //   break off starting << from <<hello   
    if (*word == '<' && word[1] != '=')
    {
        if (len == 3 && *word == word[1] && word[2] == '=') { ; }
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
    if (len > 2 && word[len - 1] == '>')
    {
        if (len == 3 && *word == word[1] && word[2] == '=') { ; }
        else if (word[len - 2] == '>')
        {
            ptr -= 2;
            word[len - 2] = 0;
            len -= 2;
        }
    }

    // break off punctuation from variable end 
    if (len > 2 && ((*word == USERVAR_PREFIX && !IsDigit(word[1])) || *word == '^' || (*word == '@' && IsDigit(word[1])) || *word == SYSVAR_PREFIX || (*word == MATCHVAR_PREFIX && IsDigit(word[1])) || (*word == '\'' && word[1] == '_'))) // not currency
    {
		if (word[len - 2] == '\\') {} // not escaped ending punctuationes
        else if (!patternContext || word[len - 1] != '?') // BUT NOT $$xxx? in pattern context  
        {
            while (IsRealPunctuation(word[len - 1])) // one would be enough, but $hello... needs to be addressed
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
    if (len == 2 && (*word == '!' || *word == '\\')) { ; }
    else if (patternContext && len > 1 && word[len - 1] == '>' && word[len - 2] != '>' && word[len - 2] != '_' && word[len - 2] != '!')
    {
        ptr -= 1;
        --len;
        word[len] = 0;
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
        if (*at == '_' && IsAlphaUTF8(word[1])) // remap rename matchvar arg on right side.
        {
            WORDP D = FindWord(at);
            if (D && D->internalBits & RENAMED) sprintf(at,(char*)"_%u",(unsigned int)D->properties);
        }
        if (*at == '@' && IsAlphaUTF8(word[1])) // remap @set arg on right side.
        {
            char* at1 = GetSetEnd(at);
            WORDP D = FindWord(at,at1 - at);
            if (D && D->internalBits & RENAMED) // remap @set on right side
            {
                strcpy(tmp,at1); // protect chunk
                sprintf(at + 1,(char*)"%u%s",(unsigned int)D->properties,tmp);
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
        if (*word == '_' && IsAlphaUTF8(word[1]))
        {
            char* atx = word;
            while (IsAlphaUTF8OrDigit(*++atx)) { ; }
            WORDP D = FindWord(word,atx - word);
            if (D && D->internalBits & RENAMED) // remap match var
            {
                sprintf(tmp,(char*)"%u%s",(unsigned int)D->properties,atx);
                strcpy(word + 1,tmp);
            }
        }
        // check for rename on LHS
        if (*word == '@' && IsAlphaUTF8(word[1]))
        {
            char* atx = GetSetEnd(word);
            WORDP D = FindWord(word,atx - word);
            if (D && D->internalBits & RENAMED) // remap @set in string
            {
                strcpy(tmp,atx); // protect chunk
                sprintf(word + 1,(char*)"%u%s",(unsigned int)D->properties,tmp);
            }
        }
    }
    // when seeing ^, see if it remaps as a function argument
    // check for internal ^ also...
    char* hat = word - 1;
	if (*word == '$' && word[1] == '^' && strstr(word, ":=")) { return ptr; } // fn define
	else while ((hat = strchr(hat + 1,'^'))) // find a hat within
    {
        char* at = hat;
        while (*++at && (IsAlphaUTF8(*at) || *at == '_' || IsDigit(*at))) { ; }
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
        int index = GetFunctionArgument(word + 1);
        if (index >= 0) sprintf(word,(char*)"'^%d",index);
    }

    // break apart math on variables eg $value+2 as a service to the user
    if ((*word == '%' || *word == '$') && word[1]) // cannot use _ here as that will break memorization pattern tokens
    {
        char* atx = word + 1;
        if (atx[1] == '$' || atx[1] == '_') ++atx;	// skip over 2ndary marker
        --atx;
        while (LegalVarChar(*++atx) || (*atx == '\\' && atx[1] == '$'));  // find end of initial word - allowing \ for json $
        if (*word == '$' && (*atx == '.' || *atx == '[' || *atx == ']') && (LegalVarChar(atx[1]) || atx[1] == '$' || atx[1] == '[' || atx[1] == ']' || (atx[1] == '\\' && atx[2] == '$')))// allow $x.y as a complete name
        {
            while (LegalVarChar(*++atx) || *atx == '.' || *atx == '$' || (*atx == '[' || *atx == ']' || (*atx == '\\' && atx[1] == '$')));  // find end of field name sequence
            if (*(atx - 1) == '.') --atx; // tailing period cannot be part of it
        }
        if (*atx && IsPunctuation(*atx) & ARITHMETICS && *atx != '=')
        {
            if (*atx == '.' && atx[1] == '_' && IsDigit(atx[2])) {} // json field reference indirection
            else if (*atx == '.' && atx[1] == '\'' && atx[2] == '_' && IsDigit(atx[3])) {} // json field reference indirection
                                                                                           // - is legal in a var or word token
            else if (*atx != '-' || (!IsAlphaUTF8OrDigit(atx[1]) && atx[1] != '_'))
            {
                ptr -= strlen(atx);
                *atx = 0;
                len = atx - start;
            }
        }
    }
    char* tilde = (IsAlphaUTF8(*word)) ? strchr(word + 1,'~') : 0;
    if (tilde && *word != '$') // has specific meaning like African-american~1n or African-american~1 - we compile to master to detect meaning
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
                sprintf(word,(char*)"%s~%u",Meaning2Word(M)->word,Meaning2Index(M));
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

	if (csapicall != NO_API_CALL && !stricmp(word, "#!")) // compilepattern api comment
	{
		ptr = strstr(ptr, "!#");
		if (!ptr) return ptr; // failed somehow
		ptr += 2;
		if (*ptr) ptr = ReadSystemToken(ptr, word, separateUnderscore); 
	}
    return ptr;
}

void EraseTopicFiles(unsigned int build, char* name)
{
    int i = -1;
    while (topicFiles[++i])
    {
        char file[SMALL_WORD_SIZE];
        sprintf(file, (char*)"%s/%s%s.txt", topicfolder, topicFiles[i], name); // old style
		int result = remove(file);
        sprintf(file, (char*)"%s/BUILD%s/%s%s.txt", topicfolder, name, topicFiles[i], name); // new style
        result = remove(file);

        if (!strcmp(topicFiles[i],(char*)"map"))
        {
            sprintf(file, (char*)"%s/BUILD%s/%s%s.json", topicfolder, name, topicFiles[i], name); // new style
			result = remove(file);
		}
	}
	EraseTopicBin(build, name);
}

#ifndef DISCARDSCRIPTCOMPILER

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

static char* FindAssignment(char* word)
{
    char* assign = strchr(word + 1, ':');
    if (!assign || (assign[1] != '=' && assign[2] != '=')) return NULL;
    return assign;
}

static void AddMapOutput(int line)
{
    if (livecall) return;

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
	The outside can ask for the next real token or merely peek ahead one token.And sometimes the outside
		after peeking, decides it wants to back up a real token(passing it to some other processor).

		To support backing up a real token, the system must keep the current readBuffer filled with the data that
		led to that token(to allow a ptr - strlen(word) backup).

		To support peeking, the system may have to read a bunch of lines in to find a token.It is going to need to
		track that buffer separately, so when it needs a real token which was the peek, it can both get the peek value
		and be using contents of the new buffer thereafter.

		So peeks must never touch the real readBuffer.And real reads must know whether the last token was peeked
		and from which buffer it was peeked.

		And, if someone wants to back up to allow the old token to be reread, they have to CANCEL any peek data, so the token
		comes from the old buffer.Meanwhile the newbuffer continues to have content for when the old buffer runs out.
#endif
	if (!peek) currentFileLine = maxFileLine; // return to context real
	int line = currentFileLine;
	int column = currentLineColumn;

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
			if (newScriptBuffer && *newScriptBuffer) // prior peek was from this buffer, make it real data in real buffer
			{
				strcpy(readBuffer,newScriptBuffer);
				result = (result - newScriptBuffer) + readBuffer; // adjust pointer to current buffer
				*newScriptBuffer = 0;
			}
			strcpy(word,lookaheadSys);
			incomingPtrSys = 0;
		}
		else 
		{
			strcpy(word,lookaheadSys); // duplicate peek
			result = (char*)1;	// NO ONE SHOULD KEEP A PEEKed PTR
		}
        if (result == (char*)1) { }
		else if ((csapicall == COMPILE_PATTERN || csapicall == COMPILE_OUTPUT)) currentLineColumn = (result - linestartpoint);
		else currentLineColumn = (result - readBuffer);

		if (currentFunctionDefinition && (currentFunctionDefinition->internalBits  & FUNCTION_BITS) == IS_PATTERN_MACRO)
		{
			char* bad = strstr(word, "$_");
			if (bad)
				BADSCRIPT("Not allowed to use local variables %s in a pattern macro %s", word, currentFunctionDefinition->word)
		}

		return result;
	}

	*word = 0;

    if (ptr)
    {
        result = ReadSystemToken(ptr, word, separateUnderscore);
    }
	bool newln = false;
	while (!*word) // found no token left in existing buffer - we have to juggle buffers now unless running overwrite 
	{
		if (!newln && newScriptBuffer && *newScriptBuffer) // use pre-read buffer per normal, it will have a token
		{
			strcpy(readBuffer,newScriptBuffer);
			*newScriptBuffer = 0;
			result = ReadSystemToken(readBuffer,word,separateUnderscore);
			break;
		}
		else // read new line into hypothetical buffer, not destroying old actual buffer yet
		{
			if (!in || ReadALine(newScriptBuffer,in, maxBufferSize,false, convertTabs) < 0) return NULL; //   end of file
			if (!strnicmp(newScriptBuffer,(char*)"#ignore",7)) // hit an ignore zone
			{
				unsigned int ignoreCount = 1;
				while (ReadALine(newScriptBuffer,in) >= 0)
				{
					if (!strnicmp(newScriptBuffer,(char*)"#ignore",7)) ++ignoreCount;
					else if (!strnicmp(newScriptBuffer,(char*)"#endignore",10))
					{
						if (--ignoreCount == 0)
						{
							if (ReadALine(newScriptBuffer,in) < 0) return NULL;	// EOF
							break;
						}
					}
				}
				if (ignoreCount) return NULL;	//EOF before finding closure
			}
			result = ReadSystemToken(newScriptBuffer,word,separateUnderscore); // result is ptr into NEWBUFFER
			newln = true;
		}
	}

	if (peek) //   save request - newScriptBuffer has implied newln if any
	{
		incomingPtrSys = result;  // next location in whatever buffer
		strcpy(lookaheadSys,word); // save next token peeked
		result = (char*)1;	// NO ONE SHOULD KEEP A PEEKed PTR
		currentFileLine = line; // claim old value
		currentLineColumn = column;
	}
	else if (newln && newScriptBuffer) // live token from new buffer, adjust pointers and buffers to be fully up to date
	{
		strcpy(readBuffer,newScriptBuffer);
		result = (result - newScriptBuffer) + readBuffer; // ptr into current readBuffer now
		*newScriptBuffer = 0;
	}
    if (result == (char*)1 ) {  }
	else if ((csapicall == COMPILE_PATTERN || csapicall == COMPILE_OUTPUT)) currentLineColumn = (result - linestartpoint);
	else currentLineColumn = (result - readBuffer);

	if (currentFunctionDefinition && (currentFunctionDefinition->internalBits  & FUNCTION_BITS) == IS_PATTERN_MACRO)
	{
		char* bad = strstr(word, "$_");
		if (bad)
			BADSCRIPT("Not allowed to use local variables %s in a pattern macro %s", word, currentFunctionDefinition->word)
	}

	return result; // ptr into READBUFFER or 1 if from peek zone
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

static char verifyLines[100][MAX_WORD_SIZE];	// verification lines for a rule to dump after seeing a rule
static unsigned int verifyIndex = 0;			// index of how many verify lines seen
static char* ReadLoop(char* word, char* ptr, FILE* in, char* &data,char* rejoinders,bool json);


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

static void AddMap(char* kind,char* msg,unsigned int *itemCount)
{
	if (!mapFile) return;
	if (kind)
	{
        char value[MAX_WORD_SIZE];
        strcpy(value, kind);
        char* at = strchr(value, ':');
        if (at)
        {
            sprintf(at + 1, " %u ", currentFileLine);
            at = strchr(kind, ':');
            strcat(value, at + 1);
        }
		fprintf(mapFile,(char*)"%s %s",value, (msg) ? msg : ((char*)""));
        if (myBot  && !strstr(value,"rule:") && !strstr(value, "complexity of"))
        {
            fprintf(mapFile, (char*)" %s", PrintU64(myBot));
        }
        fprintf(mapFile, (char*)"\r\n");
        if (mapFileJson)
        {
            char keyword[MAX_WORD_SIZE];
            strcpy(keyword, kind);
            char* key = TrimSpaces(keyword, true);
            at = strchr(key, ':');
            if (at && *at) *at = 0;
            
            if (itemCount) ++(*itemCount);
            if (*itemCount > 1) fprintf(mapFileJson, (char*)",\r\n");
            fprintf(mapFileJson, (char*)"{\r\n");
            if (!strcmp(key,"rule"))
            {
                char* rulekind = strrchr(at+1, ' ');
                if (rulekind) *rulekind = 0;
                char* tag = strchr(at+1, '.');
                if (tag) *tag = 0;
                char* label = strrchr(tag+1, '-');
                if (label && IsDigit(*(label-1))) *label = 0;
                fprintf(mapFileJson, (char*)"\"%s\" : \"%s\"",key,tag+1);
                if (rulekind) fprintf(mapFileJson, (char*)",\r\n\"kind\" : \"%s\"",rulekind+1);
                if (label) fprintf(mapFileJson, (char*)",\r\n\"label\" : \"%s\"",label+1);
            }
            else fprintf(mapFileJson, (char*)"\"%s\" : \"%s\"",key,msg);
            
            if (currentFileLine > 0) fprintf(mapFileJson, (char*)",\r\n\"line\" : %u",currentFileLine);
            if (!strcmp(key,"concept")) fprintf(mapFileJson,"\r\n}");
        }
    }
	else
    {
        fprintf(mapFile,(char*)"%s\r\n",msg); // for complexity metric
		if (mapFileJson && strstr(msg, "omplexity"))
		{
			char* at = strrchr(msg, ':');
			if (at)
			{
				unsigned int comp = atoi(at + 2);
				fprintf(mapFileJson, (char*)",\r\n\"complexity\" : %u", comp);
				fprintf(mapFileJson, "\r\n}");
			}
		}
	}
}

static void ClearBeenHere(WORDP D, uint64 junk)
{
	RemoveInternalFlag(D,BEEN_HERE);
    // clear transient ignore spell warning flag
    if (*D->word != '^' && *D->word != '~' && *D->word != '$' &&  D->internalBits & DO_NOISE && !(D->systemFlags & HAS_SUBSTITUTE))
        RemoveInternalFlag(D, DO_NOISE);
}

bool TopLevelUnit(char* word) // major headers (not kinds of rules)
{
	return (!stricmp(word,(char*)":quit") || !stricmp(word,(char*)"canon:")  || !stricmp(word,(char*)"replace:") || !stricmp(word, (char*)"debug:")  || !stricmp(word, (char*)"ignorespell:") || !stricmp(word, (char*)"prefer:") || !stricmp(word,(char*)"query:") || !stricmp(word, (char*)"word:") || !stricmp(word,(char*)"concept:") || !stricmp(word,(char*)"data:") || !stricmp(word,(char*)"plan:")
		|| !stricmp(word,(char*)"outputMacro:") || !stricmp(word,(char*)"patternMacro:") || !stricmp(word,(char*)"dualMacro:")  || !stricmp(word,(char*)"table:") || !stricmp(word,(char*)"tableMacro:") || !stricmp(word,(char*)"rename:") || 
		!stricmp(word,(char*)"describe:") ||  !stricmp(word,(char*)"bot:") || !stricmp(word, (char*)"language:")  || !stricmp(word,(char*)"topic:") || (*word == ':' && IsLowerCase( word[1]) && IsLowerCase(word[2]) && !IsEmojiShortname(word)) );	// :xxx is a debug command
}

static char* FlushToTopLevel(FILE* in,unsigned int depth,char* data)
{
	globalDepth = depth;
	if (data) *data = 0; // remove data
	char word[MAX_WORD_SIZE];
	int oldindex = jumpIndex;
	jumpIndex = -1; // prevent ReadNextSystemToken from possibly crashing.
	if (newScriptBuffer) *newScriptBuffer = 0;
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
	if (*word != '~') return false;
	if (!word[1]) return true; // single ~ is a set, representing current topic
	WORDP D = FindWord(word,0,LOWERCASE_LOOKUP);
	return (D ) ? true : false;
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
	*scopeBotName = 0; // no longer compiling a specific bot
	sprintf(file,"%s/missingsets.txt", topicfolder);
	FILE* in = FopenReadWritten(file);
	if (!in) return;
	*currentFilename = 0; // dont tell the name of the file
	while (ReadALine(readBuffer,in)  >= 0) 
    {
		char* ptr = readBuffer;
		char word[MAX_WORD_SIZE];
		char line[MAX_WORD_SIZE];
		char column[MAX_WORD_SIZE];
		ptr = ReadCompiledWord(readBuffer,word);
		ptr = ReadCompiledWord(ptr, line);
		currentFileLine = atoi(line);
		ptr = ReadCompiledWord(ptr, column);
		currentLineColumn = atoi(column);
		ReadCompiledWord(ptr, currentFilename);
		char* data = strchr(word, '$');
		if (data) *data = 0;
		if (!IsSet(word) && !IsTopic(word) && !IsDigit(word[1]))  // NEVER defined
			WARNSCRIPT((char*)"Undefined set or topic %s\r\n",word)
		else if (data && data[1])// check if defined for appropriate bot
		{
			if (data) *data = '$';
			if (!data || !data[1]) continue; // no bot specific given
			WORDP D = FindWord(word);
			if (!D || !(D->internalBits & BEEN_HERE))
			{
				if (data) *data = 0;
				WARNSCRIPT("Undefined set or topic %s for bot %s\r\n", word, data+1)
			}
		}
	}
	FClose(in);
	remove(file);
}

static void CheckSetOrTopic(char* name) // we want to prove all references to set get defined
{
    if (livecall) return;

	if (*name == '~' && !name[1]) return; // simple ~ reference
	char word[MAX_WORD_SIZE];
	MakeLowerCopy(word,name);
	char* label = strchr(word,'.'); // set reference might be ~set or  ~set.label
	if (label) *label = 0; 

	if ((IsTopic(word) || IsSet(word))) 
		return; // we know this already

	char combo[MAX_WORD_SIZE];
	strcpy(combo, word);
	strcat(combo, "$");
	char bot[MAX_WORD_SIZE];
	if (*scopeBotName == ' ') strcpy(bot, scopeBotName + 1);
	else strcpy(bot, scopeBotName);
	size_t len = strlen(bot);
	if (bot[len - 1] == ' ') bot[len - 1] = 0;
	strcat(combo, bot);
	if (FindWord(combo)) return; // we know this already

	WORDP D = StoreWord(combo);
	char file[200];
	sprintf(file,"%s/missingsets.txt", topicfolder);
	FILE* out = FopenUTF8WriteAppend(file);
	fprintf(out,(char*)"%s  %u  %u  %s\r\n",combo,currentFileLine,currentLineColumn, currentFilename);
	fclose(out); // dont use FClose
}

static char* AddVerify(char* kind, char* sample)
{
	char* comment = strstr(sample,(char*)"# "); // locate any comment on the line and kill it
	if (comment) *comment = 0;
	sprintf(verifyLines[verifyIndex++],(char*)"%s %s",kind,SkipWhitespace(sample)); 
	return 0;	// kill rest of line
}

static void WriteVerify(char* label)
{
	if (!verifyIndex) return;
	char name[100];
	sprintf(name, (char*)"VERIFY/%s-b%c.txt", currentTopicName + 1, (buildID == BUILD0) ? '0' : '1');
	FILE* valid = FopenUTF8WriteAppend(name);
	static bool init = true;
	if (!valid && init)
	{
		MakeDirectory((char*)"VERIFY");
		init = false;
		valid = FopenUTF8WriteAppend(name);
		if (!valid) printf("Unable to create VERIFY directory.\r\n");
	}

	if (valid)
	{
		char* space = "";
		if (REJOINDERID(currentRuleID)) space = "    ";
		for (unsigned int i = 0; i < verifyIndex; ++i)
		{
			if (*label) fprintf(valid, (char*)"%s%s.%u.%u=%s  %s\r\n", space, currentTopicName, TOPLEVELID(currentRuleID), REJOINDERID(currentRuleID), label, verifyLines[i]);
			else fprintf(valid, (char*)"%s%s.%u.%u  %s\r\n", space, currentTopicName, TOPLEVELID(currentRuleID), REJOINDERID(currentRuleID), verifyLines[i]);
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
			if (F->verb == Mmember)  DownHierarchy(F->subject,out,depth+1);
			F = GetObjectNondeadNext(F);
		}
		fprintf(out,(char*)". depth=%d\r\n",depth); 
	}
}

static void WriteKey(char* word)
{
    if (!compiling || spellCheck != NOTE_KEYWORDS || *word == '_' || *word == '\'' || *word == USERVAR_PREFIX || *word == SYSVAR_PREFIX || *word == '@') return;
	if (!stricmp(current_language, "Japanese") || !stricmp(current_language, "chinese")) return; // no spellcheck happens

	// cheap test when not using Language param yet
	unsigned char japanletter[8];
	int kind;
	if (csapicall == COMPILE_PATTERN && IsJapanese(word, (unsigned char*)&japanletter, kind)) return;
	
	StoreWord(word);
    if (livecall)
    {
        return;
    }
    char file[SMALL_WORD_SIZE];
    sprintf(file,(char*)"%s/keys.txt", tmpfolder);
	FILE* out = FopenUTF8WriteAppend(file);
	if (out)
	{
		DownHierarchy(MakeMeaning(StoreWord(word)),out,0);
		fclose(out); // dont use Fclose
	}
}

static void WritePatternWord(char* word)
{
    if (IsUniversal(word,0)) return; // not normal words
	if (!stricmp(current_language, "japanese") || !stricmp(current_language, "chinese")) return; // no spell check
	unsigned char japanletter[8];
	int kind;
	if (csapicall == COMPILE_PATTERN && IsJapanese(word, (unsigned char*)&japanletter, kind)) return;
    if (!compiling) return;

	// do we want to note this word
	WORDP D = StoreWord(word,AS_IS);
	// if word is not resident, maybe its properties are transient so we must save pattern marker
	if (!(D->properties & NORMAL_WORD) && !(D->systemFlags & PATTERN_WORD))
	{
		AddSystemFlag(D, PATTERN_WORD);
		if (compiling == CONCEPTSTRING_COMPILE)
		{
			heapPatternThread = AllocateHeapval(HV1_WORDP,heapPatternThread, (uint64)D);
			return;
		}
	}

    // case sensitivity?
	char tmp[MAX_WORD_SIZE];
	MakeLowerCopy(tmp,word);
	WORDP lower = FindWord(word,0,LOWERCASE_LOOKUP);
	WORDP upper = FindWord(word,0,UPPERCASE_LOOKUP);
	char utfcharacter[10];
	char* x = IsUTF8(word, utfcharacter); 
	if (!strcmp(tmp,word) || utfcharacter[1])  {;} // came in as lower case or as UTF8 character, including ones that don't have an uppercase version?
    else if (nospellcheck) {;}
    else if (lower && lower->internalBits & DO_NOISE && !(lower->systemFlags & HAS_SUBSTITUTE)) {} // told not to check
    else if (upper && (GetMeaningCount(upper) > 0 || upper->properties & NORMAL_WORD )){;} // clearly known as upper case
	else if ( !nomixedcase && !livecall && !(spellCheck & NO_SPELL) && lower && lower->properties & NORMAL_WORD && !(lower->properties & (DETERMINER|AUX_VERB)))
		WARNSCRIPT((char*)"Keyword %s should not be uppercase - did prior rule fail to close\r\n",word)
	else if (!nomixedcase && !livecall && !(spellCheck & NO_SPELL) && spellCheck && lower && lower->properties & VERB && !(lower->properties & NOUN))
		WARNSCRIPT((char*)"Uppercase keyword %s is usually a verb.  Did prior rule fail to close\r\n",word)
	
    char* pos = strchr(word, '\'');
	if ( pos && pos[1] != 's' && !pos[1] && (pos-word) > 1 && *(pos-2) != '_') // not possessive '1 or separated french or other l'
	{
		WARNSCRIPT((char*)"Contractions are always expanded - %s won't be recognized\r\n",word)
	}
	if (D->properties & NORMAL_WORD) return;	// already a known word
	if (D->internalBits & BEEN_HERE) return;	//   already written to pattern file or doublecheck topic ref file
    if (compiling != CONCEPTSTRING_COMPILE) AddBeenHere(D);
    if (livecall ) 
    {
        patternwordthread = AllocateHeapval(HV1_WORDP,patternwordthread, (uint64)D);// save name
    }
}

static void NoteUse(char* label,char* topicName)
{
    char xlabel[MAX_WORD_SIZE];
    char* dot = strchr(label, '.');
    char* tilde = strchr(label + 1, '~');
    if (tilde)
    {
        *tilde = 0;
        sprintf(xlabel, "%s%s", label, dot);
        *tilde = '~';
    }
    else strcpy(xlabel, label);

    char labelx[MAX_WORD_SIZE];
    char bots[MAX_WORD_SIZE];
    strcpy(bots, scopeBotName);
    if (*bots == ' ') strcpy(bots, scopeBotName + 1);
    if (!*bots) strcpy(bots, "*");
    int len = strlen(bots);
    if (bots[len - 1] == ' ') bots[len - 1] = 0;
    MakeUpperCase(bots);

    sprintf(labelx, "%s-%s", xlabel,bots);
    MakeUpperCase(labelx);
	WORDP D = FindWord(labelx);
	if (!D || !(D->internalBits & LABEL)) // bug doesnt look for generic if bots exists
	{
		char file[200];
		sprintf(file,"%s/missingLabel.txt", topicfolder);
		FILE* out = FopenUTF8WriteAppend(file);
		if (out)
		{
            if (*scopeBotName) fprintf(out, (char*)"%s %s %s %u\r\n", xlabel, scopeBotName, currentFilename, currentFileLine); // generic
			else fprintf(out,(char*)"%s * %s 0 %u\r\n",xlabel,currentFilename,currentFileLine); // specific bot
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
		if (stricmp(arg1,(char*)"RESPONDER") && stricmp(arg1,(char*)"LOOP") && stricmp(arg1, (char*)"JSONLOOP") && stricmp(arg1,(char*)"REJOINDER") && stricmp(arg1,(char*)"RULE") && stricmp(arg1,(char*)"GAMBIT") && stricmp(arg1,(char*)"INPUT") && stricmp(arg1,(char*)"FACT"))
			BADSCRIPT((char*)"CALL- 62 1st argument to ^next must be FACT or LOOP or JSONLOOP or INPUT or RULE or GAMBIT or RESPONDER or REJOINDER - %s\r\n",arg1)
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
		if (stricmp(arg1, (char*)"conjugate") && stricmp(arg1, (char*)"preexists") && stricmp(arg1, (char*)"raw") && stricmp(arg1, (char*)"allupper") && stricmp(arg1, (char*)"syllable") && stricmp(arg1, (char*)"ADJECTIVE") && stricmp(arg1, (char*)"ADVERB") && stricmp(arg1, (char*)"VERB") && stricmp(arg1, (char*)"AUX") && stricmp(arg1, (char*)"PRONOUN") && stricmp(arg1, (char*)"TYPE") && stricmp(arg1, (char*)"HEX32") && stricmp(arg1, (char*)"HEX64")
			&& stricmp(arg1, (char*)"NOUN") && stricmp(arg1, (char*)"DETERMINER") && stricmp(arg1, (char*)"PLACE") && stricmp(arg1, (char*)"common")
			&& stricmp(arg1, (char*)"capitalize") && stricmp(arg1, (char*)"uppercase") && stricmp(arg1, (char*)"lowercase")
			&& stricmp(arg1, (char*)"canonical") && stricmp(arg1, (char*)"grade") 
			&& stricmp(arg1, (char*)"mixcase") && stricmp(arg1, (char*)"substitute")
			&& stricmp(arg1, (char*)"isword")  && stricmp(arg1, (char*)"integer") && stricmp(arg1, (char*)"IsModelNumber") && stricmp(arg1, (char*)"IsFunction") && stricmp(arg1, (char*)"IsInteger") && stricmp(arg1, (char*)"IsUppercase") && stricmp(arg1, (char*)"IsAllUppercase") && stricmp(arg1, (char*)"IsFloat") && stricmp(arg1, (char*)"IsMixedCase") && stricmp(arg1, (char*)"Xref"))
			BADSCRIPT((char*)"CALL- 12 1st argument to ^pos must be ISWORD or SYLLABLE or ALLUPPER or VERB or AUX or GRADE or PRONOUN or NOUN or ADJECTIVE or ADVERB or DETERMINER or PLACE or COMMON or CAPITALIZE or UPPERCASE or LOWERCASE or CANONICAL or INTEGER or HEX32 or HEX64 or ISMODELNUMBER or ISFUNCTION or ISINTEGER or ISUPPERCASE or ISALLUPPERCASE or ISFLOAT or ISMIXEDCASE or XREF - %s\r\n", arg1)
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
		if (stricmp(arg1,(char*)"RULE") && stricmp(arg1,(char*)"CALL") && stricmp(arg1,(char*)"LOOP") && stricmp(arg1, (char*)"JSONLOOP") && stricmp(arg1,(char*)"TOPIC") && stricmp(arg1,(char*)"SENTENCE") && stricmp(arg1,(char*)"INPUT")  && stricmp(arg1,(char*)"PLAN"))
 			BADSCRIPT((char*)"CALL- 15 1st argument to ^end must be RULE or LOOP or JSONLOOP or TOPIC or SENTENCE or INPUT or PLAN- %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^fail"))
	{
		if (stricmp(arg1, (char*)"TOPRULE") && stricmp(arg1, (char*)"CALL") && stricmp(arg1, (char*)"RULE") && stricmp(arg1, (char*)"CALL") && stricmp(arg1, (char*)"LOOP") && stricmp(arg1, (char*)"JSONLOOP") && stricmp(arg1, (char*)"TOPIC") && stricmp(arg1, (char*)"SENTENCE") && stricmp(arg1, (char*)"INPUT"))
			BADSCRIPT((char*)"CALL- 16 1st argument to ^fail must be CALL or RULE or TOPRULE or LOOP or JSONLOOP or TOPIC or SENTENCE or INPUT - %s\r\n", arg1)
	}
	else if (!stricmp(D->word,(char*)"^nofail"))
	{
		if (stricmp(arg1,(char*)"RULE") && stricmp(arg1, (char*)"CALL") && stricmp(arg1,(char*)"LOOP") && stricmp(arg1, (char*)"JSONLOOP")  && stricmp(arg1,(char*)"TOPIC") && stricmp(arg1,(char*)"SENTENCE") && stricmp(arg1,(char*)"INPUT"))
 			BADSCRIPT((char*)"CALL- 16 1st argument to ^nofail must be CALL or RULE or LOOP or JSONLOOP or TOPIC or SENTENCE or INPUT - %s\r\n",arg1)
	}
	else if (!stricmp(D->word,(char*)"^compute"))
	{
		char* op = arg2;
		if (*op != '\'' && stricmp(op, (char*)"+") && stricmp(op, (char*)"plus") && stricmp(op, (char*)"add") && stricmp(op, (char*)"and") &&
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
	else if (!stricmp(D->word, (char*)"^jsonmerge") && !stricmp(arg1, "key") && !stricmp(arg1, "key-value")) WARNSCRIPT((char*)"^jsonmerge requires key or key-value  %s\r\n", arg1)
	else if (!stricmp(D->word,(char*)"^gambit") && !stricmp(arg1,currentTopicName)) WARNSCRIPT((char*)"Recursive call to topic - possible infinite recursion danger %s\r\n",arg1)
	else if (!stricmp(D->word,(char*)"^response") && *arg1 == '~')  BADSCRIPT((char*)"CALL-? argument to ^response should be a number, not a topic. Did you intend ^respond? - %s\r\n",arg1)
	else if (!stricmp(D->word,(char*)"^burst") && !stricmp(arg1,(char*)"wordcount")) BADSCRIPT((char*)"CALL-? argument to ^burst renamed. Use 'count' instead of 'wordcount'\r\n")
	//   validate inference calls if we can
	else if (!strcmp(D->word,(char*)"^query"))
	{
		unsigned int flags = atoi(argset[9]);
		if (flags & (USER_FLAG1|USER_FLAG2|USER_FLAG3) && !strstr(arg1,(char*)"flag_")) BADSCRIPT((char*)"CALL-24 ^query involving USER_FLAG1 must be named xxxflag_\r\n")
		if (!stricmp(arg1,(char*)"direct_s") || !stricmp(arg1,(char*)"exact_s"))
		{
			if (!*arg2 || *arg2 == '?') BADSCRIPT((char*)"CALL-24 Must name subject argument to query\r\n")
			if (*argset[3] && *argset[3] != '?') BADSCRIPT((char*)"CALL-25 Cannot name verb argument to query %s - %s\r\n",arg1,argset[3])
			if (*argset[4] && *argset[4] != '?') BADSCRIPT((char*)"CALL-26 Cannot name object argument to query %s - %s\r\n",arg1,argset[4])
			if (*argset[8] && *argset[8] != '?') BADSCRIPT((char*)"CALL-27 Cannot name propgation argument to query %s - %s\r\n",arg1,argset[8])
			if (*argset[9] && *argset[9] != '?') BADSCRIPT((char*)"CALL-28 Cannot name match argument to query %s - %s\r\n",arg1,argset[9])
		}
		flags = atoi(argset[5]);
		if (flags & (USER_FLAG1|USER_FLAG2|USER_FLAG3)  && flags < 0x00ffffff) WARNSCRIPT((char*)"Did you want a xxxflag_ query with USER_FLAG in 9th position for %s\r\n",arg1)
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

static void CheckTopicExists(char* name)
{ 
	if (*name != '~' || !name[1]) return;  // not a direct topic name

	WORDP D = FindWord(name);
	if (D && D->internalBits & TOPIC) return;

	// generate crosscheck data
	int lsize = (strlen(name) + 2 + 8 + strlen(scopeBotName) + 1 + 7) / 8;
	char* nameData = AllocateHeap(NULL, lsize, 8);
	*(uint64*)nameData = myBot;
	nameData[8] = 0;
	strcpy(nameData + 9, name);
	char* more = nameData + 10 + strlen(nameData + 9);
	strcpy(more, scopeBotName);
	char* filename = AllocateHeap(currentFilename, 0, 1);
	undefinedConceptThreadList = AllocateHeapval(HV1_STRING | HV2_STRING | HV3_INT, undefinedConceptThreadList, (uint64)nameData, (uint64)filename, (uint64)currentFileLine);
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
	int argumentcolumn = currentLineColumn;
	int argumentline = currentFileLine;
	int callline = currentFileLine;
	int callcolumn = currentLineColumn - strlen(name) ;
	
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
		if (csapicall != NO_API_CALL && call && strnicmp(name,"^dp_",4)) // compile or execute are both bad
		{
			BADSCRIPT("Undefined function: %s", name)
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
	strcpy(data++, " ");
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
		if (*word == '#' && word[1] == '!') 
			BADSCRIPT((char*)"#! sample input seen during a call to %s. Probably missing a closing )\r\n",name);
	
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
			if (*word == '~') CheckSetOrTopic(word); // set or topic

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
					WORDP DX = FindWord(fnname,0,PRIMARY_CASE_ALLOWED);
					if (DX && DX->internalBits & FUNCTION_NAME) strcpy(word,fnname); 
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
						strcpy(data++, " ");
					}
					else if (!stricmp(word,"^loop")) 
					{
						ptr = ReadLoop(word,ptr,in,data,NULL,false);
					}
                    else if (!stricmp(word, "^jsonloop"))
                    {
                        ptr = ReadLoop(word, ptr, in, data, NULL,true);
                    }
                    else
					{
						ptr = ReadCall(word,ptr,in,data,*nextToken == '(',false);
						strcpy(data++, " ");
					}
					if (argumentCount < ARGSETLIMIT) 
					{
						argset[++argumentCount] = AllocateStack(arg);
					}
					continue;
				}
				if (*word == '^' && word[1] == '\''){;}
				// join can use function names without being a call
				else if (*word == '^' && stricmp(name,"^join") &&  * nextToken && *nextToken != '(' && word[1] != '^' && word[1] != USERVAR_PREFIX && word[1] != '_' && word[1] != '"' && !IsDigit(word[1]))
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
	
		if (oldContext && IsAlphaUTF8(*word) && stricmp(name,(char*)"^incontext") && stricmp(name,(char*)"^reuse") )  
        {
			//WritePatternWord(word); // we dont consider function datat to be keywords 
			// WriteKey(word);
		}

		if (D && !stricmp(D->word, (char*)"^reuse") )
		{
			MakeUpperCase(word); // topic names are lower case  & labels must be upper case
			char* dot = strchr(word, '.');
			if (dot)
			{
				*dot = 0;
				MakeLowerCase(word);
				*dot = '.';
			}
			
			argset[1] = AllocateStack(word);
		}

		//   add simple item into data
		if (!stricmp(word, "call") && (!stricmp(D->word, (char*)"^fail") || !stricmp(D->word, (char*)"^nofail")))
			strcpy(word, "RULE");
		strcpy(data,word);
		data += strlen(data);
		if (parenLevel == 0) break;	//   we finished the call (no trailing space)
		strcpy(data++, " ");
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
	
	// functions taking an explicit  topic name in argument need to confirm it exists
	if (D && (!stricmp(D->word, (char*)"^gambit")   || !stricmp(D->word, (char*)"^respond") )
		&& *arg1 == '~')
	{
		MakeLowerCase(arg1); // topic names are lower case  
		CheckTopicExists(arg1);
	}
	else if (D && !stricmp(D->word, (char*)"^setrejoinder") )
	{
		if (!stricmp(arg1, "input") || !stricmp(arg1, "output") || !stricmp(arg1, "copy")) 
		{
			MakeUpperCase(arg2); // rulename are uppercase and topic names are lower case  
			char* dot = strchr(arg2, '.');
			if (dot)
			{
				*dot = 0;
				MakeLowerCase(arg2);
				CheckTopicExists(arg2);
				*dot = '.';
			}
		}
		else
		{
			MakeUpperCase(arg1); // rulename are uppercase and topic names are lower case  
			char* dot = strchr(arg1, '.');
			if (dot)
			{
				*dot = 0;
				MakeLowerCase(arg1);
				CheckTopicExists(arg1);
				*dot = '.';
			}
		}
	}
	else if (D && (!stricmp(D->word, (char*)"^refine") || !stricmp(D->word, (char*)"^reuse"))&& (IsAlphaUTF8(*arg1) || *arg1 == '~'))
	{ // arg1 may be rulename or topic.rulename
		MakeUpperCopy(reuseTarget1, arg1); // topic names are lower case  & labels must be upper case
		char* dot = strchr(reuseTarget1, '.');
		if (dot)
		{
			*dot = 0;
			MakeLowerCase(reuseTarget1);
			CheckTopicExists(reuseTarget1);
			*dot = '.';
		}
	}
	else if (D && !stricmp(D->word,(char*)"^enable") && IsAlphaUTF8(*arg1))
	{
		if (stricmp(arg1,(char*)"topic") && stricmp(arg1,(char*)"write") && stricmp(arg1,(char*)"rule") && stricmp(arg1,(char*)"usedrules") ) BADSCRIPT((char*)"CALL-18 Enable 1st arg must be TOPIC or RULE or USEDRULES - %s\r\n",arg1)
		if (*arg2 == '@'){;}
		else if (*arg2 != '~' || strchr(arg2,'.')) // not a topic or uses ~topic.rulename notation
		{
			MakeUpperCopy(reuseTarget1,arg2); // topic names & labels must be upper case
			char* dot = strchr(reuseTarget1, '.');
			if (dot)
			{
				*dot = 0;
				MakeLowerCase(reuseTarget1);
				CheckTopicExists(reuseTarget1);
				*dot = '.';
			}
		}
		else CheckTopicExists(arg2);
	}
	else if (D && !stricmp(D->word,(char*)"^disable") && IsAlphaUTF8(*arg1))
	{
		if (stricmp(arg1,(char*)"topic") && stricmp(arg1,(char*)"rule")  && stricmp(arg1,(char*)"write")&& stricmp(arg1,(char*)"rejoinder") && stricmp(arg1,(char*)"inputrejoinder") && stricmp(arg1,(char*)"outputrejoinder")  && stricmp(arg1,(char*)"save")  ) BADSCRIPT((char*)"CALL-19 Disable 1st arg must be TOPIC or RULE or INPUTREJOINDER or OUTPUTREJOINDER or SAVE - %s\r\n",arg1)
		if (*arg2 == '@'){;}
		else if (!stricmp(arg1,(char*)"rejoinder")){;}
		else if (*arg2 != '~' || strchr(arg2, '.'))
		{
			MakeUpperCopy(reuseTarget1, arg2); // topic names & labels must be upper case 
			char* dot = strchr(reuseTarget1, '.');
			if (dot)
			{
				*dot = 0;
				MakeLowerCase(reuseTarget1);
				CheckTopicExists(reuseTarget1);
				*dot = '.';
			}
		}
		else CheckTopicExists(arg2);
	}

	if (D) ValidateCallArgs(D,arg1,arg2,argset,needTofield);

    if (parenLevel != 0)
    {
        char* value = (D) ? D->word : (char*)"unknown";
        BADSCRIPT((char*)"CALL-59 Failed to properly close (or [ in call to %s started at line %d col %d\r\n", value, callline, callcolumn + supplementalColumn)
    }

	FunctionResult result;
	if (isStream){;}  // no cares
	else if (info) // system function
	{
		if (argumentCount != (info->argumentCount & 255) && info->argumentCount != VARIABLE_ARG_COUNT && info->argumentCount != UNEVALED && info->argumentCount != STREAM_ARG) 
			BADSCRIPT((char*)"CALL-60 Incorrect argument count to system function %s- given %d instead of required %d starting at line %d col %d \r\n",name,argumentCount,info->argumentCount & 255, argumentline, argumentcolumn)
	}
	else if (D && (D->internalBits & FUNCTION_BITS) == IS_PLAN_MACRO) 
	{
		if (argumentCount != (int)D->w.planArgCount)
			BADSCRIPT((char*)"CALL-60 Incorrect argument count to plan %s- given %d instead of required %d\r\n",name,argumentCount,D->w.planArgCount)
	}
	else if ((!D || !FindAppropriateDefinition(D, result, true)) && strnicmp(name,"^dp_",4))
	{
		// generate crosscheck data
		int lsize = (strlen(name) + 2 + 8 + strlen(scopeBotName) + 1 + 7) / 8;
		char* nameData = AllocateHeap(NULL, lsize, 8);
		*(uint64*)nameData = myBot;
		nameData[8] = (char)argumentCount;
		strcpy(nameData + 9, name);
		char* more = nameData + 10 + strlen(nameData + 9);
		strcpy(more, scopeBotName);
		char* filename = AllocateHeap(currentFilename, 0, 1);
        undefinedCallThreadList = AllocateHeapval(HV1_STRING|HV2_STRING|HV3_INT,undefinedCallThreadList, (uint64)nameData,(uint64) filename, (uint64)currentFileLine);
	}
	else if (strnicmp(name, "^dp_", 4))// std macro (input, output table)
	{
		unsigned char* defn = GetDefinition(D);
		if (defn && argumentCount != (int)MACRO_ARGUMENT_COUNT(defn) && !(D->internalBits & VARIABLE_ARGS_TABLE)) 
			BADSCRIPT((char*)"CALL-60 Incorrect argument count to macro %s- given %d instead of required %d starting line %d col %d \r\n",name,argumentCount,MACRO_ARGUMENT_COUNT(GetDefinition(D)), argumentline, argumentcolumn)
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
	strcpy(data++, ")"); //   outer layer generates trailing space
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
		size_t len = strlen(D->word);
		currentLineColumn -= len;
		if (E->word[1] && E->word[0] != '~' && E->word[0] != '(')	// concept changes of words will be considered interjections and pattern match form is accepted
		{
			WARNSCRIPT((char*)"%s changes %s to %s %s\r\n", which, word, E->word, message)
		}
		else if (!E->word[1])  WARNSCRIPT((char*)"%s erases %s %s\r\n", which, word, message)
		currentLineColumn += len;
	}
}

static void SpellCheckScriptWord(char* input,int startSeen,bool checkGrade) 
{
	if (!stricmp(input,(char*)"null")) return; // assignment clears
    if (nospellcheck) return;
	if (!stricmp(current_language, "japanese") || !stricmp(current_language, "chinese")) return;
	int kind = 0;
	bool japanese = (!stricmp(current_language, "japanese") || !stricmp(current_language, "chinese"));
	unsigned char japanletter[8];
	if (!japanese) japanese = csapicall == COMPILE_PATTERN && IsJapanese(input, (unsigned char*)&japanletter, kind);

	// remove any trailing punctuation
	char word[MAX_WORD_SIZE];

    // see if supposed to ignore capitalization differences
    MakeLowerCopy(word, input);
    WORDP X = FindWord(word, 0, LOWERCASE_LOOKUP);
    if (X && X->internalBits & DO_NOISE && !(X->systemFlags & HAS_SUBSTITUTE))
        return;
	strcpy(word,input);
	size_t len = strlen(word);
	while (len > 1 && !IsAlphaUTF8(word[len-1]) && word[len-1] != '.') word[--len] = 0;
    
    WORDP set[GETWORDSLIMIT];
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
    if (nn > 1 && !nomixedcase)
    {
        WARNSCRIPT((char*)"Word \"%s\" known in multiple spellings %s\r\n", word, text)
        return;
    }

	WORDP D = FindWord(word,0,LOWERCASE_LOOKUP);
	WORDP entry = D;
	WORDP canonical = D;
	if (word[1] == 0 || IsUpperCase(*input) || !IsAlphaUTF8(*word)  || strchr(word,'\'') || strchr(word,'.') || strchr(word,'_') || strchr(word,'-') || strchr(word,'~')) {;} // ignore proper names, sets, numbers, composite words, wordnet references, etc
	else if (stricmp(current_language,"english")) {;} // dont complain on foreign
	else if (!D || (!(D->properties & NORMAL_WORD) && !(D->systemFlags & PATTERN_WORD)))
	{ // we dont know this word in lower case
		uint64 sysflags = 0;
		uint64 cansysflags = 0;
		wordStarts[0] = wordStarts[1] = wordStarts[2] = wordStarts[3] = AllocateHeap((char*)"");
		wordCount = 1;
		WORDP revise;
		uint64 flags = 0;
		if (D)
		{
			ongoingDictChanges = NULL; 
			ongoingUniversalDictChanges = NULL;
			monitorDictChanges = true;
			flags = GetPosData((unsigned int)-1, word, revise, entry, canonical, sysflags, cansysflags, false, true, 0);
			// GetPosData can add properties, revert additions
			monitorDictChanges = false;
			ReverseDictionaryChanges(ongoingDictChanges);
			ReverseUniversalDictionaryChanges(ongoingUniversalDictChanges);
			ongoingDictChanges = NULL;
			ongoingUniversalDictChanges = NULL;
		}
		if (!flags) // try upper case
		{
			WORDP E = FindWord(word,0,SECONDARY_CASE_ALLOWED); // uppercase
			if (E && E != D && E->word[2] && !nomixedcase) 
                WARNSCRIPT((char*)"Word %s only known in upper case\r\n",word)   
			else if (E && !(E->internalBits & UPPERCASE_HASH) && !D ) WARNSCRIPT((char*)"%s is not a known word. Is it misspelled?\r\n",word)
			canonical = E; // the base word
		}
	}
	// check vocabularly limits?
	if (grade && checkGrade && !stricmp(current_language,"English"))
	{
		if (canonical && !IsUpperCase(*input) && !(canonical->systemFlags & grade) && !strchr(word,'\'')) // all contractions are legal
			Log(USERLOG,"Grade Limit: %s\r\n",D->word);
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
	if (*ptr == '=') return true;
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
		sprintf(file,(char*)"%s/BUILD%s/describe%s.txt", topicfolder,baseName,baseName);
		FILE* out = FopenUTF8WriteAppend(file);
		fprintf(out,(char*)" %s %s\r\n",word,description);
		fclose(out); // dont use Fclose
	}
	return ptr;
}

static void OverCover(char* laterword, STACKREF keywordList[PATTERNDEPTH],
    char nestKind[PATTERNDEPTH], int nestIndex)
{ //  [ your  "your own"] has your blocking detection of your_own and we want the longer match
	if (!stricmp(current_language, "japanese") || !stricmp(current_language, "chinese")) return;

	if (nestKind[nestIndex - 1] != '[' && nestKind[nestIndex - 1] != '{') return;
    char word1[MAX_WORD_SIZE];
    strcpy(word1, laterword);
    char* underscore = strchr(word1, '_');
    if (underscore) // see if masked by earlier word
    {
        *underscore = 0; // initial word of phrase
        STACKREF item = keywordList[nestIndex - 1];
        while (item)
        {
            uint64 priorword;
            item = UnpackStackval(item, priorword);
            if (!strcmp((char*)priorword, word1)) BADSCRIPT((char*)"Keyword phrase %s occluded by %s. Switch their order.", laterword, priorword)
        }
        *underscore = '_';
    }

    // add to list
    char* word = AllocateStack(laterword);
    keywordList[nestIndex - 1] = AllocateStackval(keywordList[nestIndex - 1], (uint64)word);
} // never freed during compile

char* ReadPattern(char* ptr, FILE* in, char* &data,bool macro, bool ifstatement)
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
if/loop/jsonloop	constructs
^call  call	function/macro calls with or without ^
^fnvar		function variables
^$glblvar	global function variables
$			user variable 
@			debug ahd factset references
labels on responders
responder types s: u: t: r: 
name of topic or concept

$x : = y(do assignment and do not fail)
$x:+=y, $x : -= y, $x : *= y, $x : /= y, $x : %= y, $x : ^= y,
$^x:=.... define function
#endif

	patternStarter = data; // for bug messages
	int oldsupplementalColumn = supplementalColumn;
	supplementalColumn = 0;
	char word[MAX_WORD_SIZE * 10];
	char nestKind[PATTERNDEPTH];
	char* nestData[PATTERNDEPTH];
    STACKREF keywordList[PATTERNDEPTH];
    int nestLine[PATTERNDEPTH];
	int nestIndex = 0;
	patternContext = true;

    unsigned int conceptIndex = 0; // id of concept set
    char* conceptBufferLevelStart[PATTERNDEPTH]; //start of currentConceptBuffer
    char* currentConceptBuffer = NULL; // movable ptr
   static char* conceptbase = NULL; // start of xfer buffer
    char* currentConceptXfer = NULL; // movable ptr
    char conceptStarted[PATTERNDEPTH]; // have we inited this level yet?

    char* stackbase = AllocateStack(NULL, 4);
	char* start = ptr;

	//   if macro call, there is no opening ( or closing )
	//   We claim an opening and termination comes from finding a toplevel token
	if (macro)
	{
		nestData[nestIndex] = data;
		nestKind[nestIndex++] = '(';
	}

	bool variableGapSeen = false; // wildcard pending

	// prefix characters
	bool memorizeSeen = false; // memorization pending
	bool quoteSeen = false;	// saw '
	bool notSeen = false;	 // saw !
    bool bidirectionalSeen = false; // saw *~nb
	bool doubleNotSeen = false; // saw !!
	size_t len;
	bool startSeen = false; // starting token or not
	char* startPattern = data;
	char* startOrig = ptr;
	char* backup;

    // these buffer allocations must be last to balance Freebuffers in endScriptCompiler
    currentConceptBuffer = conceptBufferLevelStart[0] = AllocateBuffer();
    conceptIndex = 0;
    conceptStarted[0] = 0;
    currentConceptXfer = conceptbase = AllocateBuffer();

	while (ALWAYS) //   read as many tokens as needed to complete the definition
	{
		backup = ptr;
		bool blockspace = false;
		unsigned int priorColumn = currentLineColumn;
		ptr = ReadNextSystemToken(in,ptr,word);
		if (!*word) break; //   end of file

		if (!strcmp(word, "==") || !strcmp(word, "=")) WARNSCRIPT((char*)"== or = used standalone in pattern. Shouldn't it be attached to left and right tokens?\r\n")
		// we came from pattern IF and lack a (
		if (ifstatement && *word != '(' && nestIndex == 0)
		{
			nestData[nestIndex] = data;
			nestKind[nestIndex++] = '(';
		}

		MakeLowerCopy(lowercaseForm,word);
		if (TopLevelUnit(lowercaseForm) || TopLevelRule(lowercaseForm)) // end of pattern
		{
			ptr -= strlen(word);  // safe
			break;
		}
		char c = 0;
        char* assignment = FindAssignment(word);
        if (assignment) // assignment, do normal analysis on 1st argument
        {
            c = *assignment;
            *assignment = 0;
        }

		char* comparison = FindComparison(word);
		char comparisonchar = 0;
		if (comparison) // comparison, do normal analysis on 1st argument
		{
			comparisonchar= c = *comparison;
			*comparison = 0;
		}
		switch(*word) // ordinary tokens, not the composite comparison blob
		{
			// token prefixes
			case '!': //   NOT
				if (quoteSeen) BADSCRIPT((char*)"PATTERN-1 Cannot have ' and ! in succession\r\n")
				if (memorizeSeen) BADSCRIPT((char*)"PATTERN-2 Cannot use _ before _\r\n")
				if (notSeen) BADSCRIPT((char*)"PATTERN-3 Cannot have two ! in succession\r\n")
				if (!word[1]) 
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
				if (quoteSeen && !IsDigit(word[1]))
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-1 Cannot have ' and _ in succession except when quoting a match variable. Need to reverse them\r\n")
				}
				if (memorizeSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-6 Cannot have two _ in succession\r\n")
				}
				if (!word[1]) // allow separation which will be implied as needed
				{
					if (!ifstatement)
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-7 Must attach _ to next token. If you mean _ match, use escaped _. %s\r\n", ptr)
					}
				}
				if (IsDigit(word[1])) // match variable
				{
					if (GetWildcardID(word) < 0)
					{
						BADSCRIPT((char*)"PATTERN-8 _%d is max match reference - %s\r\n", MAX_WILDCARDS - 1, word)
						patternEnder = data;
					}
					char* follow = word + 2;
					if (IsDigit(*follow)) ++follow;
					if (*follow)
					{
						if (*follow == '-' || *follow == '+') BADSCRIPT((char*)"PATTERN-8 %s has attached stuff. Did you intend @%s expression?\r\n", word,word)
					}
					if (*ptr == '[' || *ptr == '{' || *ptr == '(') blockspace = true; 
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
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-10 Cannot have two ' in succession\r\n")
				}
				if (!word[1])
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-11 Must attach ' to next token. If you mean ' match, use \' \r\n %s\r\n", ptr)
				}
				quoteSeen = true;
				variableGapSeen = false;
				if (comparison) *comparison = c;
				
				len = strlen(word) - 1 ;
				ptr -= len;  // the remnant
				strncpy(ptr,word+1, len); // this allows a function parameter (originally ^word but now ^0) to properly reset
				continue;
			case '<':	//   sentence start <  or  unordered start <<
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-12 Cannot use ' before < or <<\r\n")
				}
				if (memorizeSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-13 Cannot use _ before < or <<\r\n")
				}
				if (word[1] == '<')  //   <<  unordered start
				{
					if (memorizeSeen)
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-14 Cannot use _ before << \r\n")
					}
					if (nestKind[nestIndex - 1] == '<')
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-15 << already in progress\r\n")
					}
					if (variableGapSeen)
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-16 Cannot use * before <<\r\n")
					}
                        // close [ or { for a moment
                        if (nestKind[nestIndex - 1] == '[' || nestKind[nestIndex - 1] == '{')
                        {
                            currentConceptBuffer = conceptBufferLevelStart[conceptIndex]; // resume here
                            strcpy(currentConceptXfer, currentConceptBuffer);
                            currentConceptXfer += strlen(currentConceptXfer);
                            if (!livecall && *currentConceptBuffer) // we had some member
                            {
                                sprintf(currentConceptXfer, "%s", ")\r\n");
                                currentConceptXfer += 3;
                            }
                            *currentConceptBuffer = 0;
                            *currentConceptXfer = 0;

                            conceptBufferLevelStart[conceptIndex] = currentConceptBuffer;
                            conceptStarted[conceptIndex] = 0;
                        }

                    nestLine[nestIndex] = (currentFileLine << 16) | currentLineColumn;
					nestData[nestIndex] = data;
                    nestKind[nestIndex++] = '<';
				}
				else if (word[1])
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-17 %s cannot start with <\r\n", word)
				}
				variableGapSeen = false;
				break; 
			case '@': 
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-18 Quoting @ is meaningless.\r\n");
				}
				if (memorizeSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-19 Cannot use _ before  @\r\n")
				}
				if (word[1] == '_') // set match position  @_5
				{
					if (GetWildcardID(word + 1) >= MAX_WILDCARDS)
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-? %s is not a valid positional reference - must be < %d\r\n", word, MAX_WILDCARDS)
					}
					char* end = word + 3;
					while (IsDigit(*end)) ++end;
					if (*end)
					{
						if (*end == '+' && (!end[1] || end[1] == 'i')) {;}
						else if (*end == '-' &&  (!end[1] || end[1] == 'i')) {;}
						else
						{
							patternEnder = data;
							BADSCRIPT((char*)"PATTERN-? %s is not a valid positional reference - @_2+ or @_2- or @_2 would be\r\n", word)
						}
					}
					variableGapSeen = false; // no longer after anything. we are changing direction
				}
                else if (!stricmp(word, "@retry")) {}
				else if (GetSetID(word) < 0)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-20 %s is not a valid factset reference\r\n", word)  // factset reference
				}
				else if (!GetSetMod(word))
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-20 %s is not a valid factset reference\r\n", word)  // factset reference
				}
				break;
			case '>':	//   sentence end > or unordered end >>
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-21 Cannot use ' before > or >>\r\n")
				}
				if (word[1] == '>') //   >>
				{
                    
					if (memorizeSeen)
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-22 Cannot use _ before  >> \r\n")
					}
					if (nestKind[nestIndex - 1] != '<')
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-23 Have no << in progress\r\n")
					}
					if (variableGapSeen)
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-24 Cannot use wildcard inside >>\r\n")
					}
					if (nestKind[--nestIndex] != '<')
					{
						patternStarter = nestData[nestIndex];
						patternEnder = data;
						char d[100];
						strncpy(d, patternStarter, 60);
						d[59] = 0;
						BADSCRIPT((char*)"PATTERN-24 >> should be closing %c started at line %d col %d\r\n", nestKind[nestIndex], nestLine[nestIndex] >> 16, nestLine[nestIndex] & 0x00ffff,d)
					}
				}
				variableGapSeen = false;
				break; //   sentence end align
			case '(':	//   sequential pattern unit begin
                        // close [ or { for a moment
                if (nestKind[nestIndex-1] == '[' || nestKind[nestIndex-1] == '{')
                {
                    currentConceptBuffer = conceptBufferLevelStart[conceptIndex]; // resume here
                    strcpy(currentConceptXfer, currentConceptBuffer);
                    currentConceptXfer += strlen(currentConceptXfer);
                    if (!livecall && *currentConceptBuffer) // we had some member
                    {
                        sprintf(currentConceptXfer, "%s", ")\r\n");
                        currentConceptXfer += 3;
                    }
                    *currentConceptBuffer = 0;
                    *currentConceptXfer = 0;

                    conceptBufferLevelStart[conceptIndex] = currentConceptBuffer;
                    conceptStarted[conceptIndex] = 0;
                }

				if (bidirectionalSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-34 ] Cant use ( after bidirectional gap- scanning backwards is bad\r\n")
				}
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-25 Quoting ( is meaningless.\r\n")
				}
                nestLine[nestIndex] = (currentFileLine << 16) | currentLineColumn;
				nestData[nestIndex] = data;
				nestKind[nestIndex++] = '(';
				break;
			case ')':	//   sequential pattern unit end
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-26 Quoting ) is meaningless.\r\n")
				}
				if (memorizeSeen && !ifstatement)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-27 Cannot use _ before  )\r\n")
				}
				if (variableGapSeen && nestIndex > 1)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-26 Cannot have wildcard followed by )\r\n")
				}
				if (nestKind[--nestIndex] != '(')
				{
					patternEnder = data;
					patternStarter = nestData[nestIndex];
					char d[100];
					strncpy(d, patternStarter, 60);
					d[59] = 0;
					BADSCRIPT((char*)"PATTERN-9 ) should be closing %c started at line %d col %d  %s\r\n", nestKind[nestIndex], nestLine[nestIndex] >> 16, nestLine[nestIndex] & 0x00ffff,d)
				}
				break;
			case '[':	//   list of pattern choices begin
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-30 Quoting [ is meaningless.\r\n");
				}
				if (nestKind[nestIndex - 1] == '[' || nestKind[nestIndex - 1] == '{')
                {
                    currentConceptBuffer = conceptBufferLevelStart[conceptIndex]; // resume here
                    strcpy(currentConceptXfer, currentConceptBuffer);
                    currentConceptXfer += strlen(currentConceptXfer);
                    if (!livecall && *currentConceptBuffer) // we had some member
                    {
                        sprintf(currentConceptXfer, "%s", ")\r\n");
                        currentConceptXfer += 3;
                    }
                    *currentConceptBuffer = 0;
                    *currentConceptXfer = 0;

                    conceptBufferLevelStart[conceptIndex] = currentConceptBuffer;
                    conceptStarted[conceptIndex] = 0;
                }
                
                nestLine[nestIndex] = (currentFileLine << 16) | currentLineColumn;
                keywordList[nestIndex] = 0;
				nestData[nestIndex] = data;
				nestKind[nestIndex++] = '[';

                conceptBufferLevelStart[++conceptIndex] = currentConceptBuffer;
                conceptStarted[conceptIndex] = 0;
				break;
			case ']':	//   list of pattern choices end
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-31 Quoting ] is meaningless.\r\n")
				}
				if (memorizeSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-32 Cannot use _ before  ]\r\n")
				}
				if (variableGapSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-33 Cannot have wildcard followed by ]\r\n")
				}
				if (nestKind[--nestIndex] != '[')
				{
					patternEnder = data;
					patternStarter = nestData[nestIndex];
					char d[100];
					strncpy(d, patternStarter, 60);
					d[59] = 0;
					BADSCRIPT((char*)"PATTERN-34 ] should be closing %c started at line %d col %d  %s\r\n", nestKind[nestIndex], nestLine[nestIndex] >> 16, nestLine[nestIndex] & 0x00ffff,d)
				}
                currentConceptBuffer = conceptBufferLevelStart[conceptIndex--]; // resume here
                strcpy(currentConceptXfer, currentConceptBuffer);
                currentConceptXfer += strlen(currentConceptXfer);
                if (!livecall && *currentConceptBuffer) // we had some member
                {   
                    sprintf(currentConceptXfer,"%s",")\r\n");
                    currentConceptXfer += 3;
                }
                *currentConceptBuffer = 0;
                *currentConceptXfer = 0;
                break;
			case '{':	//   list of optional choices begins
				if (nestKind[nestIndex - 1] == '[')
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-15 {} within [] is pointless because it always matches\r\n")
				}
				if (nestKind[nestIndex - 1] == '<' && !memorizeSeen && !strstr(start,"^matches") && compiling != PIECE_COMPILE)
                    WARNSCRIPT((char*)"PATTERN-15 {} within << >> is pointless unless you memorize or use ^matches because it always matches\r\n")
				if (bidirectionalSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-34 ] Cant use { after bidirectional gap - will always match scanning backwards\r\n")
				}
				// close [ or { for a moment
                if (nestKind[nestIndex - 1] == '[' || nestKind[nestIndex - 1] == '{')
                {
                        currentConceptBuffer = conceptBufferLevelStart[conceptIndex]; // resume here
                        strcpy(currentConceptXfer, currentConceptBuffer);
                        currentConceptXfer += strlen(currentConceptXfer);
                        if (!livecall && *currentConceptBuffer) // we had some member
                        {
                            sprintf(currentConceptXfer, "%s", ")\r\n");
                            currentConceptXfer += 3;
                        }
                        *currentConceptBuffer = 0;
                        *currentConceptXfer = 0;

                        conceptBufferLevelStart[conceptIndex] = currentConceptBuffer;
                        conceptStarted[conceptIndex] = 0;
                    }
                
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
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-35 Quoting { is meaningless.\r\n");
				}
				if (notSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-36 !{ is pointless since { can fail or not anyway\r\n")
				}
				if (nestIndex && nestKind[nestIndex - 1] == '{') BADSCRIPT((char*)"PATTERN-37 {{ is illegal\r\n")
                keywordList[nestIndex] = 0;
                nestLine[nestIndex] = (currentFileLine << 16) | currentLineColumn;
				nestData[nestIndex] = data;
				nestKind[nestIndex++] = '{';

                conceptBufferLevelStart[++conceptIndex] = currentConceptBuffer;
                conceptStarted[conceptIndex] = 0;
                break;
			case '}':	//   list of optional choices ends
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-38 Quoting } is meaningless.\r\n");
				}
				if (memorizeSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-39 Cannot use _ before  }\r\n")
				}
				if (variableGapSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-40 Cannot have wildcard followed by }\r\n")
				}
				if (nestKind[--nestIndex] != '{')
				{
					patternEnder = data;
					patternStarter = nestData[nestIndex];
					char d[100];
					strncpy(d, patternStarter, 60);
					d[59] = 0;
					BADSCRIPT((char*)"PATTERN-41 } should be closing %c started at line %d col %d  %s\r\n", nestKind[nestIndex], nestLine[nestIndex] >> 16, nestLine[nestIndex] & 0x00ffff,d)
				}
                currentConceptBuffer = conceptBufferLevelStart[conceptIndex--]; // resume here
                strcpy(currentConceptXfer, currentConceptBuffer);
                currentConceptXfer += strlen(currentConceptXfer);
                if (!livecall && *currentConceptBuffer) // we had some member
                {
                    sprintf(currentConceptXfer, "%s", ")\r\n");
                    currentConceptXfer += 3;
                }
                *currentConceptBuffer = 0;
                *currentConceptXfer = 0;                 
				break;
			case '\\': //   literal next character
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-42 Quoting an escape is meaningless.\r\n");
				}
				if (!word[1])
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-43 Backslash must be joined to something to escape\r\n")
				}
				variableGapSeen = false;
				if (word[1] && IsAlphaUTF8(word[1] )) memmove(word,word+1,strlen(word)); // escaping a real word, just use it
				break;
			case '*': //   gap: * *1 *~2 	(infinite, exactly 1 word, 0-2 words, 0-2 words, 1 word before) and *alpha*x* is form match
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-44 Quoting a wildcard\r\n");
				}
				if (nestKind[nestIndex - 1] == '<')
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-45 Can not have wildcard %s inside << >>\r\n", word)
				}
				if (nestKind[nestIndex - 1] != '(' && (word[1] == '~' || !word[1]))
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-45 Can only have variable wildcard %s inside ( )\r\n", word)
				}
				if (variableGapSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-46 Cannot have wildcard followed by %s\r\n", word)
				}
				if (IsAlphaUTF8(word[1]))
					break; // find this word as fragmented spelling like sch*ding* since it will have * as a prefix
				
				// gaps of various flavors
				if (notSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-47 cannot have ! before gap - %s\r\n", word)
				}
				if (IsDigit(word[1])) //   enumerated gap size
				{
					int n = word[1] - '0';
					if (n == 0)
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-48 *0 is meaningless\r\n")
					}
					if (word[2])
					{
						BADSCRIPT((char*)"PATTERN-49 *9 is the largest gap allowed or bad stuff is stuck to your token- %s\r\n", word)
					}
				}
				else if (word[1] == '-') // backwards
				{
					int n = word[2] - '0';
					if (n == 0)
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-50 *-1 is the smallest backward wildcard allowed - %s\r\n", word)
					}
					if (word[3])
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-51 *-9 is the largest backward wildcard or bad stuff is stuck to your token- %s\r\n", word)
					}
				}
				else if (word[1] == '~') // close-range gap
				{
					if (nestKind[nestIndex - 1] == '{' || nestKind[nestIndex - 1] == '[')
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-5? cannot stick %s wildcard inside {} or []\r\n", word)
					}
					variableGapSeen = true;
					int n = word[2] - '0';
					if (!word[2])
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-52 *~ is not legal, you need a digit after it\r\n")
					}

					else if (n == 0 && word[2] != '0')
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-53 *~1 is the smallest close-range gap - %s\r\n", word)
					}
					else if (word[3] && word[3] != 'b')
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-54 *~9 is the largest close-range gap or bad stuff is stuck to your token- %s\r\n", word)
					}
				}
				else if (word[1])
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-55 * jammed against some other token- %s\r\n", word)
				}
				else
				{
					if (nestKind[nestIndex - 1] == '{' || nestKind[nestIndex - 1] == '[')
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-5? cannot stick * wildcard inside {} or []\r\n")
					}
					variableGapSeen = true; // std * unlimited wildcard
				}
				startSeen = true;
				break;
			case '?': //   question input ?   
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-56 Quoting a ? is meaningless.\r\n");
				}
				if (memorizeSeen && word[1] != '$')
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-57 Cannot use _ before ?\r\n")
				}
				if (variableGapSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-58 Cannot have wildcards before ?\r\n")
				}
				break;
			case USERVAR_PREFIX:	//   user var
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-59 Quoting a $ variable is meaningless - %s\r\n", word);
				}
				// $x? is a search to match and is legal
				if (variableGapSeen && memorizeSeen && comparisonchar != '?')	 BADSCRIPT((char*)"PATTERN-59 Variable gap in progress, cannot reference variable - %s\r\n", word);
				break;
			case '"': //   string
				{
					// you can quote a string, because you are quoting its members
					variableGapSeen = false;
					strcpy(word,JoinWords(BurstWord(word,CONTRACTIONS)));// change from string to std token
				    WritePatternWord(word); 
					WriteKey(word);
					unsigned int n = 0;
					char* ptr = word;
					while ((ptr = strchr(ptr,'_')))
					{
						++n;
						++ptr;
					}
					if (n >= SEQUENCE_LIMIT) WARNSCRIPT((char*)"PATTERN-? Too many  words in string %s, may never match unless you set $cs_sequence high enough\r\n",word)
				}
				goto DEFLT;
			case SYSVAR_PREFIX: //   system data
				// you can quote system variables because %topic returns a topic name which can be quoted to query
				if (memorizeSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-60 Cannot use _ before system variable - %s\r\n", word)
				}
				if (!word[1]); //   simple %
				else if (!FindWord(word) && stricmp(word, "%trace_on") && stricmp(word, "%trace_off") && stricmp(word, "%testpattern-prescan") && stricmp(word, "%testpattern-nosave"))
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-61 %s is not a system variable\r\n", word)
				}
				if (comparison) *comparison = c;
				variableGapSeen = false;
				break;
			case '~':
				variableGapSeen = false;
				if (quoteSeen)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-61 cannot quote set %s because it can't be determined if set comes from original or canonical form\r\n", word)
				}
				startSeen = true;
				WriteKey(word);
				CheckSetOrTopic(word); // set or topic
				goto DEFLT;
			default: //   normal token ( and anon function call)
                DEFLT: 
	            if (noPatternOptimization  || assignment || comparison || *word == '^' || *word == '_' || *word == '@' || *word == '$' || *word == '%') {;}
                else if (strchr(word, '*') || strchr(word, '?')) { ; } // wildcard word patterns
                else if (nestKind[nestIndex - 1] == '[' || nestKind[nestIndex - 1] == '{')
                { // generate compile-time anonymous concept set for word list [] {}. 
					// but it makes ^mark and ^unmark from outside problematic
                    char controls[100];
                    *controls = 0;
                    if (notSeen) strcat(controls, "!");
					if (*controls && memorizeSeen)
					{
						patternEnder = data;
						BADSCRIPT((char*)"PATTERN-67 Cannot have ! and _ together\r\n")
					}
                    if (quoteSeen) strcat(controls, "'");
                    notSeen = quoteSeen = false;

                    char name[MAX_WORD_SIZE];
                    if (!conceptStarted[conceptIndex ]) // not yet started
                    {
                        unsigned int layer = 1;
                        if (buildId == BUILD0) layer = 0;
                        else if (buildId == BUILD1) layer = 1;
						if (myBot && !livecall)
						{
							sprintf(name, (char*)"~%u%05u`%s", layer, ++conceptID, PrintU64(myBot));
						}
                        else sprintf(name, "~%u%05u", layer,++conceptID);
						conceptStarted[conceptIndex ] = 1;
                        if (!livecall) sprintf(currentConceptBuffer, "%s ( ", name);
                        else sprintf(currentConceptBuffer, "%s ", name);
                        currentConceptBuffer += strlen(currentConceptBuffer);
                        sprintf(currentConceptBuffer, "%s%s ", controls,word);
                        currentConceptBuffer += strlen(currentConceptBuffer);
						sprintf(word, "~%u%05u", layer, conceptID); // no bot id attached
					}
                    else
                    {
                        sprintf(currentConceptBuffer, "%s%s ", controls,word);
                        currentConceptBuffer += strlen(currentConceptBuffer);
                        *word = 0; // dont use it
                    }
                }

				variableGapSeen = false;
				startSeen = true;
				break;
		}

        if (assignment)
        {
            *assignment = c;
			if (memorizeSeen && assignment[1])
			{
				patternEnder = data;
				BADSCRIPT((char*)"PATTERN-57 Cannot use _ before an assignment\r\n")
			}
			if (variableGapSeen) BADSCRIPT((char*)"PATTERN-16 Cannot use * before assignment since memorization will be incomplete\r\n")
            //   rebuild token
            char tmp[MAX_WORD_SIZE * 4];
            *tmp = ':';		//   assignment header
			len = (assignment - word) + 2; //   include the : and jump code in length
			if (len > 70)
			{
				patternEnder = data;
				BADSCRIPT((char*)"PATTERN-65 Left side of assignment must not exceed 70 characters - %s\r\n", word)
			}
			char* x = tmp + 1;
            Encode(len, x, 1);

			// function definition
			if (*word == '$' && word[1] == '^' && *assignment == ':' && assignment[1] == '=')
			{
				char* fn = assignment + 4; // the function definition code
				len = strlen(fn);
				if (fn[len - 1] == '"') fn[len - 1] = 0;
				char* data = AllocateBuffer();
				char* holdstart = (char*)linestartpoint;
				linestartpoint = fn; // lie about where data came from to get accurate column data
				supplementalColumn = priorColumn + fn - word;
				currentLineColumn = 0;
				fn = ReadMacro(fn, NULL, "apimacro:", 0,data);
				linestartpoint = holdstart;
				currentLineColumn = supplementalColumn;
				supplementalColumn = 0;
				char* end = strchr(fn, '`');
				*end = 0;
				char* aftername = strchr(assignment + 4, '^');
				char* paren = strchr(aftername, '(');
				char* space = strchr(aftername, ' ');
				if (space && space < paren) paren = space;
				*paren = ' ';
				strcpy(paren +1, fn);
				strcpy(tmp + 2, word); //   copy left side over
				strcat(tmp,"\"");
				FreeBuffer();
				patternContext = true;
			}
			else strcpy(tmp + 2, word); //   copy left side over
			char* end = word + strlen(word);
			if (*(end - 1) == '=' ) BADSCRIPT("PATTERN-57x pattern assignment missing right hand side- %s\r\n",word)
            strcpy(word, tmp);	//   replace original token
        }
		else if (comparison) //   is a comparison of some kind
		{
			if (memorizeSeen && comparison[1])
			{
				patternEnder = data;
				BADSCRIPT((char*)"PATTERN-57 Cannot use _ before a comparison\r\n")
			}
			if (variableGapSeen && comparisonchar != '?' && memorizeSeen)
			{
				patternEnder = data;
				BADSCRIPT((char*)"PATTERN-16 Cannot use * before comparison since memorization will be incomplete\r\n")
			}
			if (*word == USERVAR_PREFIX && word[1] == LOCALVAR_PREFIX)
			{
				char* dot = strchr(word,'.');
				if (dot) *dot = 0;
				AddDisplay(word);
				if (dot) *dot = '.';
			}
	 		*comparison = comparisonchar;
			if (comparisonchar == '!') // move not operator out in front of token
			{
				*data++ = '!';
				strcpy(data++, " "); // and space it, so if we see "!=shec?~hello" we wont think != is an operator, instead = is a jump infof

				len = strlen(comparison);
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
			if (*rhs == '^' && IsAlphaUTF8(rhs[1]))
			{
				patternEnder = data;
				BADSCRIPT((char*)"%s is not a current function variable\r\n", rhs)
			}
			if (!*rhs && *word == USERVAR_PREFIX) {} // allowed member in sentence
			else if (!*rhs && *word == '_' && IsDigit(word[1])); // allowed member in sentence
			else if (*rhs == '#') // names a constant #define to replace with number value
			{
				uint64 n = FindPropertyValueByName(rhs+1);
				if (!n) n = FindSystemValueByName(rhs+1);
				if (!n) n = FindParseValueByName(rhs+1);
				if (!n) n = FindMiscValueByName(rhs+1);
				if (!n)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-63 No #constant recognized - %s\r\n", rhs + 1)
				}
			sprintf(rhs,(char*)"%lld",(long long int) n); 
			}
			else if (IsAlphaUTF8DigitNumeric(*rhs)  ) // LITERAL has no constraints
			{
			//		WriteKey(rhs); 
			//		WritePatternWord(rhs);		//   ordinary token
			}
			else if (*rhs == '~') 
			{
				MakeLowerCase(rhs);
				CheckSetOrTopic(rhs);	
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
			else
			{
				patternEnder = data;
				BADSCRIPT((char*)"PATTERN-64 Illegal comparison %s or failed to close prior rule starting at %s\r\n", word, GetRuleElipsis(start))
			}
			 len = (comparison - word) + 2; //   include the = and jump code in length

			//   rebuild token
			char tmp[MAX_WORD_SIZE];
			*tmp = '=';		//   comparison header
			if (len > 70)
			{
				patternEnder = data;
				BADSCRIPT((char*)"PATTERN-65 Left side of comparison must not exceed 70 characters - %s\r\n", word)
			}
			char* x = tmp + 1;
			Encode(len,x,1);
			strcpy(tmp+2,word); //   copy left side over
			strcpy(word,tmp);	//   replace original token
		}
		else if (*word == '~')  CheckSetOrTopic(word); 
		ReadNextSystemToken(in,ptr,nextToken,true,true);
		
		if (*word == '^')   //   function call or function var ref or indirect function variable assign ref like ^$$tmp = null
		{
			if (quoteSeen)
			{
				patternEnder = data;
				BADSCRIPT((char*)"PATTERN-? Cannot use quote before ^ function call or variable\r\n")
			}
			if (notSeen)
			{
				*data++ = '!';
				if (doubleNotSeen) *data++ = '!';
				doubleNotSeen = notSeen = false;
			}
			if (memorizeSeen) 
			{
				if (!IsDigit(word[1]) && word[1] != USERVAR_PREFIX)
				{
					patternEnder = data;
					BADSCRIPT((char*)"PATTERN-66 Cannot use _ before ^ function call\r\n")
				}
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
			strcpy(data++, " ");
			continue;
		}
		
		//   put out the next token and space 
		if (notSeen) 
		{
			if (memorizeSeen)
			{
				patternEnder = data;
				BADSCRIPT((char*)"PATTERN-67 Cannot have ! and _ together\r\n")
			}
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
			if (ifstatement) strcpy(data++, " ");
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
            else if (IsPunctuation(*word) && !word[1]) // punctuation
            {
                WriteKey(word);
                WritePatternWord(word); //   memorize it to know its important
            }
			else // ordinary word - break off possessives as needed
			{
				size_t lenx = strlen(word);
				unsigned int ignore = 0;
				if (lenx > 1 && word[lenx-1] == '\'' && word[lenx-2] != '_') // ending ' possessive plural
				{
					if (ifstatement && !strcmp(word,"PATTERN")) {;} // allow uppercase
					else
					{
                        OverCover(word, keywordList,nestKind,nestIndex);
                        WritePatternWord(word);
						WriteKey(word);
					}
					word[--lenx] = 0;
					ignore = 1;
				}
				else if (lenx > 2 && word[lenx-1] == 's' && word[lenx-2] == '\'' && word[lenx-3] != '_') // ending 's possessive singular
				{
                    OverCover(word, keywordList, nestKind, nestIndex);
                    WriteKey(word);
					WritePatternWord(word);
					lenx -= 2;
					word[lenx] = 0;
					ignore = 2;
				}
				if (stricmp(current_language, "japanese") || !stricmp(current_language, "chinese"))
				{
					strcpy(word, JoinWords(BurstWord(word, CONTRACTIONS))); // change to std token
					if (!livecall && spellCheck && !(spellCheck & NO_SPELL)) SpellCheckScriptWord(word, startSeen ? 1 : 0, false);
					else if (livecall && compiling == PIECE_COMPILE) SpellCheckScriptWord(word, startSeen ? 1 : 0, false);
					if (strcmp(word, "PATTERN") )
					{
						OverCover(word, keywordList, nestKind, nestIndex);
						WriteKey(word); 
						WritePatternWord(word); //   memorize it to know its important
					}
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

		strcpy(data,word); // default unchanged (but not updated yet to accept)
		len = strlen(data);
		data += len;
		if (!blockspace) strcpy(data++, " ");
		else blockspace = false; //  _1[  for instance

        bidirectionalSeen = false;
		if (nestIndex == 0) break; //   we completed this level
        if (*word == '*' && word[1] == '~' && word[3] && word[3] == 'b') bidirectionalSeen = true;
    }   
	*data = 0;
	if (csapicall == COMPILE_PATTERN && ptr && *ptr)
	{
		ptr = TrimSpaces(ptr);
		if (*ptr)
		{
			char msg[MAX_WORD_SIZE];
			strncpy(msg, ptr, 20);
			msg[20] = 0;
			BADSCRIPT((char*)"PATTERN-78 Excess data after pattern closed %s \r\n", msg)
		}
	}
	//   leftovers?
	if (macro && nestIndex != 1)
	{
		patternEnder = data;
		BADSCRIPT((char*)"PATTERN-68 Failed to balance ( or [ or { properly in macro for %s\r\n", startPattern)
	}
	else if (!macro && nestIndex != 0)
	{
		patternEnder = data;
		patternStarter = nestData[nestIndex-1];
		BADSCRIPT((char*)"PATTERN-69 Failed to close %c started at line %d col %d : %s\r\n", nestKind[nestIndex - 1], nestLine[nestIndex-1] >> 16, nestLine[nestIndex-1] & 0x00ffff, startPattern);
	}
	patternContext = false;
    ReleaseStack(stackbase);
    if (!*conceptbase) {;} // no optimization happened
    else if (!livecall) // not from compilepattern or dynamic testpattern -- empty files to start
    {
        char filename[SMALL_WORD_SIZE];
        int layer = 1;
        if (buildId == BUILD0) layer = 0;
        else if (buildId == BUILD1) layer = 1;
        sprintf(filename, (char*)"%s/BUILD%d/keywords%d.txt", topicfolder, layer, layer);
        FILE* out = FopenUTF8WriteAppend(filename);
        fprintf(out, "%s", conceptbase);
        fclose(out);
	}
    else // ^compilepattern optimzation
    {
        char* revised = AllocateBuffer();
        strcpy(revised, startPattern);
        strcpy(startPattern, conceptbase);
        strcat(startPattern, revised);
        FreeBuffer();
     }
    FreeBuffer(); // conceptBufferLevelStart
    FreeBuffer(); // conceptbase

	patternStarter = NULL;
	patternEnder = NULL;
	supplementalColumn = oldsupplementalColumn;
	return ptr;
}

static char* ReadChoice(char* word, char* ptr, FILE* in, char* &data,char* rejoinders)
{	//   returns the stored data, not the ptr, starts with the [
	*data++ = '[';
	strcpy(data++, " ");
	ReadNextSystemToken(in,ptr,word,true); // get possible rejoinder label
	if (word[1] == ':' && !word[2]) // is rejoinder label
	{
		if (*word >= 'A' && *word < 'Q') *word = *word - 'A' + 'a';
		if (*word < 'a' || *word >= 'q') BADSCRIPT((char*)"CHOICE-1 Bad level label %s in [ ]\r\n",word)
		if (rejoinders) rejoinders[(int)(*word - 'a' + 1)] = 2; //   authorized level
		*data++ = *word;
		*data++ = word[1];
		strcpy(data++, " ");
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

char* ReadIfTest(char* ptr, FILE* in, char* &data)
{
    priorLine = currentFileLine;
	char word[MAX_WORD_SIZE];
	int paren = 1;
	size_t len;
	//   test is either a function call OR an equality comparison OR an IN relation OR an existence test
	//   the followup will be either (or  < > ==  or  IN  or )
	//   The function call returns a status code, you cant do comparison on it
	//   but both function and existence can be notted- IF (!$var) or IF (!read(xx))
	//   You can have multiple tests, separated by AND and OR.
	PATTERN: 
	ptr = ReadNextSystemToken(in,ptr,word,false,false); 
    if (priorLine != currentFileLine)
    {
        AddMapOutput(priorLine);
        priorLine = currentFileLine;
    }
    if (*word == '~' ) CheckSetOrTopic(word);
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
        len = strlen(at);
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
		strcpy(data++, " ");
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
			strcpy(data++, " ");
			ptr =  ReadNextSystemToken(in,ptr,word,false,false); //   swallow operator
            if (priorLine != currentFileLine)
            {
                AddMapOutput(priorLine);
                priorLine = currentFileLine;
            }
            strcpy(data,word);
			data += strlen(word);
			strcpy(data++, " ");
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
		strcpy(data++, " ");
		ptr =  ReadNextSystemToken(in,ptr,word,false,false); //   swallow operator
		strcpy(data,word);
		data += strlen(word);
		strcpy(data++, " ");
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
		strcpy(data++, " ");
		ptr =  ReadNextSystemToken(in,ptr,word,false,false); //   swallow operator
		strcpy(data,word);
		data += strlen(word);
		strcpy(data++, " ");
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
		strcpy(data++, " ");

	//   check for close or more conditions
	ptr =  ReadNextSystemToken(in,ptr,word,false,false); //   )
	if (*word == '~') CheckSetOrTopic(word);
	if (*word == ')')
	{
		*data++ = ')';
		strcpy(data++, " ");
	}
	else if (!stricmp(word,(char*)"or") || !stricmp(word,(char*)"and") || !stricmp(word, (char*)"&"))
	{
		MakeLowerCopy(data,word);
		data += strlen(word);
		strcpy(data++, " ");
		goto PATTERN;	//   handle next element
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

char* ReadIf(char* word, char* ptr, FILE* in, char* &data,char* rejoinders)
{
	char* bodyends[PATTERNDEPTH];				//   places to patch for jumps
	unsigned int bodyendIndex = 0;
	char* original = data;
	strcpy(data,(char*)"^if ");
	data += 4;
    if (mapFile && dataBase && !livecall)
    {
        fprintf(mapFile, (char*)"          if %u %u  \r\n", currentFileLine, (unsigned int)(data - dataBase)); // readBuffer
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
		strcpy(data++, " ");
		ptr = SkipWhitespace(ptr);
		if (!strnicmp(ptr,(char*)"pattern ",7))
		{
            if (livecall) 
				BADSCRIPT((char*)"Cannot use Pattern If during live compilation\r\n")

			ptr = ReadNextSystemToken(in,ptr,word,false); //   pattern
            if (priorLine != currentFileLine)
            {
                AddMapOutput(priorLine);
                priorLine = currentFileLine;
            }
            strcpy(data,word);
			data += strlen(data);
			strcpy(data++, " ");
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
		strcpy(data++, " ");
		Encode((unsigned int)(data-ifbase),ifbase);	// offset to ELSE or ELSE IF from body start 
			
		//   now see if ELSE branch exists
		ReadNextSystemToken(in,ptr,word,false,true); //   else?
        if (priorLine != currentFileLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
		if (stricmp(word, (char*)"else"))
		{
			if (csapicall == COMPILE_OUTPUT && errorOnMissingElse) BADSCRIPT((char*)"Missing ELSE clause.\r\n");
			break; //   caller will add space after our jump index
		}

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
        if (mapFile && dataBase && !livecall)
        {
            if (!stricmp(word,"if")) fprintf(mapFile, (char*)"          elseif %u %u  \r\n", currentFileLine, (unsigned int)(data - dataBase)); // readBuffer
            else fprintf(mapFile, (char*)"          else %u %u  \r\n", currentFileLine, (unsigned int)(data - dataBase)); // readBuffer
        }
        if (*word == '{') //   swallow the ELSE body now since no IF - add fake successful test
		{
			//   successful test condition for else
			*data++ = '(';
			*data++ = ' ';
			*data++ = '1';
			*data++ = ' ';
			*data++ = ')';
			strcpy(data++, " ");

			ifbase = data; 
			DummyEncode(data);//   reserve skip data
			strcpy(data++, " ");
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
    if (mapFile && !livecall)
    {
        fprintf(mapFile, (char*)"          ifend %u %u  \r\n", currentFileLine, (unsigned int)(data - dataBase)); // readBuffer
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

static char* ReadLoop(char* word, char* ptr, FILE* in, char* &data,char* rejoinders,bool json)
{
    priorLine = currentFileLine;
    char* original = data;
    if (json)
    {
        strcpy(data, (char*)"^jsonloop ");
        data += 10;
    }
    else
    {
        strcpy(data, (char*)"^loop ");
        data += 6;
    }
    if (mapFile && dataBase)
    {
        if (json) fprintf(mapFile, (char*)"          jsonloop %u %u  \r\n", currentFileLine, (unsigned int)(data - dataBase)); // readBuffer
        else fprintf(mapFile, (char*)"          loop %u %u  \r\n", currentFileLine, (unsigned int)(data - dataBase)); // readBuffer
    }

	ptr = ReadNextSystemToken(in,ptr,word,false,false); //   (
    if (priorLine != currentFileLine)
    {
        AddMapOutput(priorLine);
        priorLine = currentFileLine;
    }
    *data++ = '(';
	strcpy(data++, " ");
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
    else if (*word == ')')
    {
        if (!json) strcpy(data, (char*)"-1"); //   omitted, use -1
        else BADSCRIPT("^JSONLOOP missing arguments")
    }
	else if (!stricmp(word,(char*)"-1") && !json) // precompiled previously -1
	{
		strcpy(data,word);
		ptr = ReadNextSystemToken(in,ptr,word,false,false); // read closing paren
		if (*word != ')') BADSCRIPT((char*)"Loop counter %s was not closed by )\r\n",word);
	}
	else if (!json && !IsDigit(*word) && *word != USERVAR_PREFIX && *word != '_' && *word != SYSVAR_PREFIX  && *word != '^'  && *word != '@') 
		BADSCRIPT((char*)"LOOP-2 counter must be $var, _#, %var, @factset or ^fnarg or function call -%s",word)
	else 
	{
		strcpy(data,word);
        ptr = ReadNextSystemToken(in,ptr,word,false, false);
        if (priorLine != currentFileLine)
        {
            AddMapOutput(priorLine);
            priorLine = currentFileLine;
        }
        if (json) // 2 more args
        {
            data += strlen(data);
			strcpy(data++, " ");
			if (*word != '$' && *word != '_')  BADSCRIPT((char*)"LOOP-2 control must be $var or matchvar", word)

            strcpy(data, word);
            if (priorLine != currentFileLine)
            {
                AddMapOutput(priorLine);
                priorLine = currentFileLine;
            }
            ptr = ReadNextSystemToken(in, ptr, word, false, false); //  
            if (*word != '$' && *word != '_')  BADSCRIPT((char*)"LOOP-2 control must be $var or matchvar", word)
            data += strlen(data);
			strcpy(data++, " ");
			strcpy(data, word);
            if (priorLine != currentFileLine)
            {
                AddMapOutput(priorLine);
                priorLine = currentFileLine;
            }
            ptr = ReadNextSystemToken(in, ptr, word, false, false);
        }
    }
	data += strlen(data);
	strcpy(data++, " ");
	if (*word != ')' && stricmp(word,"new") && stricmp(word,"old"))
        BADSCRIPT((char*)"LOOP-3 control must end with ) or NEW or OLD -%s\r\n", word)
    if (!stricmp(word, "new") || !stricmp(word, "old"))
    {
        strcpy(data, word);
        data += 3;
		strcpy(data++, " ");
	}
	*data++ = ')';
	strcpy(data++, " ");
	char* loopstart = data;
	DummyEncode(data);  // reserve loop jump to end accelerator
	strcpy(data++, " ");
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
        fprintf(mapFile, (char*)"          loopend %u %u  \r\n", currentFileLine, (unsigned int)(data - dataBase)); // readBuffer
    }
	return ptr; // caller adds extra space after
}

static char* ReadJavaScript(FILE* in, char* &data,char* ptr)
{
	strcpy(data,"*JavaScript");
	data += strlen(data);
	strcpy(data++, " ");
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
		strcpy(data++, " ");
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
	int pendingmathassign = 0;
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
				pendingmathassign = 1;
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
        if (!strcmp(word,"==")) WARNSCRIPT((char*)"== used in output. Did you want assignment = ?\r\n")

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
		if (*word == '}' && level == 0 && !optionalBrace) BADSCRIPT("extra } closing nothing")
		if ((*word == '}' && level == 0 && optionalBrace) ||  TopLevelUnit(word) || TopLevelRule(lowercaseForm) || Rejoinder(lowercaseForm) || !stricmp(word,(char*)"datum:")) //   responder definition ends when another major unit or top level responder starts
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
		if (pendingmathassign && *word != '(')
			pendingmathassign = 0; // turn off math assign
		if ((*word == '$' || *word == '_') && IsAssignOp(nextToken)) pendingmathassign = 1;
		if (pendingmathassign && (!stricmp(word, "and") || !stricmp(word, "or")))
		{
			BADSCRIPT("%s is not a legal math operator. CS has no boolean type.",word)
		}
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
				else if (!level && (startkind == '{' || startkind == '[' || startkind == '('))
				{
					strcpy(dataChunk,word);
					dataChunk += strlen(dataChunk);
					if (startkind != '(' ) *word = 0;	// end loop for bodies and choices
				}
				break;
			case '\'': 
				strcpy(dataChunk,word);
				dataChunk += strlen(dataChunk);
				if (*word == '\'' && word[1] == 's' && !word[2] && IsAlphaUTF8OrDigit(*nextToken) ) *dataChunk++ = ' ';
 				else if (word[1] == 0 && (*ptr == '_' || IsAlphaUTF8(*ptr)  ))  {;} // if  isolated like join(' _1) then  add space
				else *dataChunk++ = ' ';  
				continue;
			case '@':
				if (!IsDigit(word[1]) && word[1] != '_') // not factset nor place locator
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
			if (!stricmp(word,"^if") || !stricmp(word,"^loop") || !stricmp(word, "^jsonloop")) {;}
			else if (*nextToken != '(' && word[1] != '^'  && word[1] != '=' && word[1] != USERVAR_PREFIX && word[1] != '_' && word[1] != '"' && word[1] != '\'' && !IsDigit(word[1])) 
				BADSCRIPT((char*)"%s either references a function w/o arguments or names a function variable that doesn't exist started at line %d col %d\r\n",word,currentFileLine,currentLineColumn - strlen(word))
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
			ptr = ReadLoop(word,ptr,in,dataChunk,rejoinders,false);
			continue;
		}
        else if (*nextToken == '{' && !stricmp(nakedWord, (char*)"jsonloop"))  // loop missing () 
        {
            ptr = ReadLoop(word, ptr, in, dataChunk, rejoinders,true);
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
			ptr = ReadLoop(word,ptr,in,dataChunk,rejoinders,false);
			continue;
		}
        else if (!stricmp(nakedWord, (char*)"jsonloop"))  // strip LOOP of ^
        {
            ptr = ReadLoop(word, ptr, in, dataChunk, rejoinders,true);
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

static void ReadTopLevelRule(WORDP topicName, char* typeval,char* &ptr, FILE* in,char* data,char* basedata)
{//   handles 1 responder/gambit + all rejoinders attached to it
	char type[100];
	complexity = 1;
	strcpy(type,typeval);
	char info[400];
	char kind[MAX_WORD_SIZE];
	

    char tname[MAX_WORD_SIZE];
    strcpy(tname, topicName->word);
    char* tilde = strchr(tname + 1, '~');
    if (tilde) *tilde = 0; // remove dup index
	strcpy(kind,type);
	char word[MAX_WORD_SIZE];
	char rejoinders[256];	//   legal levels a: thru q:
	memset(rejoinders,0,sizeof(rejoinders));
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
			rejoinders[level] = 1; //   we are now at this level, enables next level
			//   levels not authorized by [b:][g:] etc are disabled
			while (++count < 20)
			{
				if (rejoinders[count] == 1) rejoinders[count] = 0;
			}
			
			currentRuleID += ONE_REJOINDER;
			WriteVerify("");
		}
		strcpy(data,kind); 
		data += 2;
		strcpy(data++, " ");
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
		bool verified = false;

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
				sprintf(info,"        rule: %s.%u.%u %s",currentTopicName,TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID),kind);
                AddMap(info,NULL,&mapRuleCount); // rule
				char* pstart = data;
				ptr = ReadPattern(ptr-1,in,data,false,false); //   back up and pass in the paren for pattern
				patternDone = true;
				*word = 0;
				if (echorulepattern) Log(ECHOUSERLOG, "rule %s\r\n", pstart);
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
					WORDP D = FindWord(name,0,LOWERCASE_LOOKUP); // all functions are lower case - all labels are uppercase)
					if (D && D->internalBits & FUNCTION_NAME && (*kind == GAMBIT || *kind == RANDOM_GAMBIT)) 
						WARNSCRIPT((char*)"label: %s is a potential macro in %s. Add ^ if you want it treated as such.\r\n",word,currentFilename)
					else if (!stricmp(word,(char*)"if") || !stricmp(word,(char*)"loop") || !stricmp(word, (char*)"jsonloop")) WARNSCRIPT((char*)"label: %s is a potential flow control (if/loop/jsonloop) in %s. Add ^ if you want it treated as a control word.\r\n",word,currentFilename)
					sprintf(info,"        rule: %s.%u.%u-%s %s",currentTopicName,TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID),name+1, kind);
					AddMap(info,NULL,&mapRuleCount); // rule

					MakeUpperCase(word); // labels are uppercase (topics and functions are lower case)

                    char* bots = topicName->w.topicBots;
                    if (!bots || !*bots)
                    {
                        bots = "*"; // general access
                    }
                     while (*bots)
                    {
                        char bot[MAX_WORD_SIZE];
                        bots = ReadCompiledWord(bots, bot);
                        if (!*bot) break;
                        sprintf(label, "%s.%s-%s", tname, word, bot );
                        MakeUpperCase(label);
                        WORDP E = StoreWord(label, AS_IS);
                        AddInternalFlag(E, LABEL);
                    }

					strcpy(labelName,word);
					if (strchr(word,'.')) BADSCRIPT((char*)"RULE-2 Label %s must not contain a period\r\n",word)
					if (len > 160) BADSCRIPT((char*)"RULE-2 Label %s must be less than 160 characters\r\n",word)
					if (!verified)
					{
						WriteVerify(word);	// dump any accumulated verification data before the rule
						verified = true;
					}
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
					strcpy(data++, " ");
					ReadNextSystemToken(NULL,NULL,NULL); // drop lookahead token
					char* pstart = data;
					ptr = ReadPattern(ptr,in,data,false,false); //   read ( for real in the paren for pattern
					patternDone = true;
					*word = 0;
					if (echorulepattern) Log(ECHOUSERLOG, "rule %s\r\n", pstart);
				}
				else //   we were seeing start of output (no label and no pattern) for gambit, proceed to output
				{
					sprintf(info,"        rule: %s.%u.%u-%s %s",currentTopicName,TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID),labelName,kind);
					AddMap(info,NULL,&mapRuleCount); // rule
					if (*type != GAMBIT && *type != RANDOM_GAMBIT) 
						BADSCRIPT((char*)"RULE-3 Missing pattern for responder\r\n")
					strcpy(data++, " ");
					patternDone = true;
					// leave word intact to pass to readoutput
				}
				break;
			}
		} //   END OF WHILE
		if (patternDone) 
		{
			if (!verified)
			{
				WriteVerify("");	// dump any accumulated verification data before the rule
				verified = true;
			}
			dataBase = data;
            ptr = ReadOutput(false,false,ptr,in,data,rejoinders,word,NULL);
            dataBase = NULL;
            char complex[MAX_WORD_SIZE];
			sprintf(complex,"          Complexity of rule %s.%u.%u-%s %s %u", currentTopicName,TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID),labelName,kind,complexity);
			AddMap(NULL, complex, NULL); // complexity

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
		else ReportBug((char*)"Prior script not complete- unexpected top level word %s after seeing %s", lowercaseForm, data - 20);
	}

	//   did he forget to fill in any [] jumps
	for (unsigned int i = ('a'-'a'); i <= ('q'-'a'); ++i)
	{
		if (rejoinders[i] == 2) BADSCRIPT((char*)"RULE-4 Failed to define rejoinder %c: for responder just ended\r\n", i + 'a' - 1)
	}

	*data = 0;
    dataBase = NULL;
}

static void ErasePendingFunction(WORDP D,int functionArgumentCount)
{
	HEAPREF list = undefinedCallThreadList;
	while (list)
	{
		uint64 functionNamex;
		uint64 filenamex;
		uint64 linex;
		HEAPREF oldlist = list;
		list = UnpackHeapval(list, functionNamex, filenamex, linex);
		if (!functionNamex) 
			continue; // reference deleted previously

		char* functionData = (char*)functionNamex;
		uint64 bot = *(uint64*)functionData;
		if (bot != myBot || strcmp(D->word, functionData + 9)) continue;
		
		if (functionData[8] != functionArgumentCount && !(D->internalBits & VARIABLE_ARGS_TABLE))
			BADSCRIPT( (char*)"Function %s wrong argument count %d expected %d given for bot %s \r\n", D->word, functionData[8], functionArgumentCount, scopeBotName);
		uint64* data = (uint64*)oldlist;
		data[1] = 0; // kill reference
	}
}

static char* ReadMacro(char* ptr,FILE* in,char* kind,unsigned int build,char* data)
{
	bool table = !stricmp(kind,(char*)"table:"); // create as a transient notwrittentofile 
	bool apimacro = !stricmp(kind, (char*)"apimacro:"); // create as a transient notwrittentofile 
	if (!apimacro) *currentTopicName = 0;
	displayIndex = 0;
	complexity = 1;
	uint64 typeFlags = 0;
	if (!stricmp(kind,(char*)"tableMacro:") || table) typeFlags = IS_TABLE_MACRO;
	else if (!stricmp(kind,(char*)"outputMacro:") || apimacro) typeFlags = IS_OUTPUT_MACRO;
	else if (!stricmp(kind,(char*)"patternMacro:")) typeFlags = IS_PATTERN_MACRO;
	else if (!stricmp(kind,(char*)"dualMacro:")) typeFlags = IS_PATTERN_MACRO | IS_OUTPUT_MACRO;
	*macroName = 0;
    macroid = 0;
	functionArgumentCount = 0;
    char* d = AllocateBuffer();
    char* revised = AllocateBuffer();
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
			if (!table && !IsAlphaUTF8(*word) ) BADSCRIPT((char*)"MACRO-1 Macro name must start alpha ^%s\r\n",word)
			if (apimacro && strncmp(word, "dp_", 3)) 
				BADSCRIPT((char*)"MACRO-1 Macro name must start with ^dp_  - ^%s\r\n", word)
			if (table)
			{
				strcpy(macroName,(char*)"^tbl:");
				strcat(macroName,word);
				Log(USERLOG,"Reading table %s\r\n",macroName);
			}
			else
			{
				if (!IsLegalName(word)) BADSCRIPT((char*)"MACRO-2 Illegal characters in function name %s\r\n",word)
				*macroName = '^';
				strcpy(macroName+1,word);
				if (!apimacro)
				{
					Log(USERLOG, "Reading %s %s\r\n", kind, macroName);
                    AddMap((char*)"    macro:", macroName, &mapItemCount);
				}
			}
			D = StoreWord(macroName,AS_IS);
			if (D->x.codeIndex) BADSCRIPT((char*)"MACRO-3 may not redefine system function %s\r\n", word)
			if (D->w.fndefinition && D->internalBits & FUNCTION_NAME && !table) // must be different BOT ID
			{
				int64 bid;
				ReadInt64((char*)GetDefinition(D),bid); 
                // have to allow multiple instances of boot bot
				if (bid == (int64)myBot && stricmp(D->word,"^csboot")) BADSCRIPT((char*)"MACRO-3 macro %s already defined\r\n",macroName)
			}
			continue;
		}
		if (parenLevel == 0 && !stricmp(word,(char*)"variable")) // putting "variable" before the args list paren allows you to NAME all args but get ones not supplied filled in with * (tables) or null (macros)
		{
			D->internalBits |= VARIABLE_ARGS_TABLE;
			continue;
		}
        if (parenLevel == 0 && !stricmp(word, (char*)"tab")) 
        {
            D->internalBits |= TABBED;
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
                restrict = strchr(word, '.');
                if (restrict)
                {
                    if (!stricmp(restrict + 1, (char*)"KEEP_QUOTES") && (typeFlags == IS_TABLE_MACRO || typeFlags == IS_OUTPUT_MACRO))	macroFlags |= 1ull << functionArgumentCount; // a normal string where spaces are kept instead of _ (format string)
                    else if (!stricmp(restrict + 1, (char*)"HANDLE_QUOTES"))
                    {
                        if (typeFlags != IS_OUTPUT_MACRO) BADSCRIPT((char*)"MACRO-? HANDLE_QUOTES only valid with OUTPUTMACRO or DUALMACRO - %s \r\n", word)
                            if (functionArgumentCount > 15)
                            {
                                int64 flag = 1ull << (functionArgumentCount - 16); // outputmacros
                                flag <<= 32;
                                macroFlags |= flag;
                            }
                            else macroFlags |= 1ull << functionArgumentCount; // outputmacros
                    }
                    else if (!stricmp(restrict + 1, (char*)"COMPILE") && typeFlags == IS_TABLE_MACRO)
                    {
                        if (functionArgumentCount > 15)
                        {
                            int64 flag = (1ull << 16) << (functionArgumentCount - 16); // outputmacros
                            flag <<= 32;
                            macroFlags |= flag;
                        }
                        else macroFlags |= (1ull << 16) << functionArgumentCount; // a compile string " " becomes "^:"
                    }
                    else if (!stricmp(restrict + 1, (char*)"UNDERSCORE") && typeFlags == IS_TABLE_MACRO) { ; } // default for quoted strings is _ 
                    else if (typeFlags != IS_TABLE_MACRO && typeFlags != IS_OUTPUT_MACRO) BADSCRIPT((char*)"Argument restrictions only available on Table Macros or OutputMacros  - %s \r\n", word)
                    else  BADSCRIPT((char*)"MACRO-? Table/Tablemacro argument restriction must be KEEP_QUOTES OR COMPILE or UNDERSCORE - %s \r\n", word)
                        *restrict = 0;
                }

				if (typeFlags & IS_PATTERN_MACRO) BADSCRIPT((char*)"MACRO-? May not use locals in a pattern/dual macro - %s\r\n",word)
				if (word[1] != '_') BADSCRIPT((char*)"MACRO-? Variable name as argument must be local %s\r\n",word)
				if (strchr(word, '.') || strchr(word, '['))
				{
                    BADSCRIPT((char*)"MACRO-? Variable name as argument must be simple, not json reference %s\r\n", word)
                }
				AddDisplay(word);
				strcpy(functionArguments[++functionArgumentCount],word);
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
							int64 flag = (1ull << 16) << (functionArgumentCount - 16); // outputmacros
							flag <<= 32;
							macroFlags |= flag;
						}
						else macroFlags |= (11ull << 16) << functionArgumentCount; // a compile string " " becomes "^:"
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
				strcpy(functionArguments[++functionArgumentCount],word);
				if (functionArgumentCount > MAX_ARG_LIMIT)  BADSCRIPT((char*)"MACRO-7 Too many callArgumentList to %s - max is %d\r\n",macroName,MAX_ARG_LIMIT)
				continue;
			default:
				BADSCRIPT((char*)"MACRO-7 Bad argument %s to macro definition %s\r\n",word,macroName)
		}
	}
    if (!D)
    {
        dataBase = NULL;
        return ptr; //   nothing defined
    }
	AddInternalFlag(D,(unsigned int)(FUNCTION_NAME|build|typeFlags));
	if (functionArgumentCount > 15) *pack++ = (unsigned char)(functionArgumentCount - 15 + 'a');
	else *pack++ = (unsigned char)(functionArgumentCount + 'A'); // some 10 can be had ^0..^9
	
	bool optionalBrace = false;
	currentFunctionDefinition = D;
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
	strcpy(botid, "0"); // default universal for api compilations
	if (!apimacro) sprintf(botid, (char*)"%s", PrintU64(myBot));
	revised[0] = revised[1] = revised[2] = revised[3] = 0;
	revised += 4; // empty link
	sprintf(revised, (char*)"%s %lld ", botid, macroFlags);
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

	if (!apimacro) D->w.fndefinition = (unsigned char*)AllocateHeap(revised - 4, strlen(revised) + 4);
	ErasePendingFunction(D,functionArgumentCount);
	if (apimacro)
	{
		revised -= 2;
		*revised = (D->internalBits & VARIABLE_ARGS_TABLE) ? 'O' : 'o';
		revised[1] = ' ';
		*macroName = 0;
		dataBase = NULL;
		FreeBuffer();
		FreeBuffer();
		return revised;
	}

	if (!table) // tables are not real macros, they are temporary
	{
		char filename[SMALL_WORD_SIZE];
		sprintf(filename,(char*)"%s/BUILD%s/macros%s.txt", topicfolder,baseName,baseName);
		//   write out definition -- this is the real save of the data
		FILE* out = FopenUTF8WriteAppend(filename);

		if ((D->internalBits & FUNCTION_BITS) ==  IS_TABLE_MACRO) fprintf(out,(char*)"%s t %s\r\n",macroName,GetDefinition(D));
		else if ((D->internalBits & FUNCTION_BITS) == (IS_OUTPUT_MACRO|IS_PATTERN_MACRO))  fprintf(out,(char*)"%s d %s\r\n",macroName,GetDefinition(D));
		else
		{
			if (D->internalBits & VARIABLE_ARGS_TABLE)
				fprintf(out, (char*)"%s %c %s\r\n", macroName, ((D->internalBits & FUNCTION_BITS) == IS_OUTPUT_MACRO) ? 'O' : 'P', GetDefinition(D));
			else fprintf(out, (char*)"%s %c %s\r\n", macroName, ((D->internalBits & FUNCTION_BITS) == IS_OUTPUT_MACRO) ? 'o' : 'p', GetDefinition(D));
		}
		fclose(out); // dont use Fclose

		char complex[MAX_WORD_SIZE];
		sprintf(complex, "          Complexity of %s: %u", macroName, complexity);
		AddMap(NULL, complex, NULL); // complexity
        if (macroid != 0)
        {
            char name[MAX_WORD_SIZE];
            sprintf(name, (char*)"          bot: 0 %s %s ", macroName,PrintU64(macroid));
            AddMap(NULL, name, NULL); // bot macro
        }
	}
    *macroName = 0;
    dataBase = NULL;
    FreeBuffer();
    FreeBuffer();
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
	if (*name == '~') // table: ~x =>  table: ^x
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
		if (fromtopic) BADSCRIPT((char*)"datum: from topic must use predefined table %s", name)
		char* data = AllocateBuffer();
		ptr = ReadMacro(ptr,in,(char*)"table:",build,data); //   defines the name,argumentList, and script
		FreeBuffer();
		ptr = ReadNextSystemToken(in, ptr, word, false, false); //   the DATA: separator
		if (stricmp(word,(char*)"DATA:")) 	BADSCRIPT((char*)"TABLE-1 missing DATA separator for table or corresponding tablemacro not yet defined- %s\r\n",word)
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
    convertTabs = (currentFunctionDefinition->internalBits & TABBED) ? false : true;
	char* argumentList = AllocateBuffer();
    tableinput = NULL;
	++jumpIndex;
	int holdDepth = globalDepth;
	char* xxbase = ptr;  // debug hook
	tableinput = AllocateBuffer();
	while (ALWAYS)
	{
		if (setjmp(scriptJump[jumpIndex])) // flush on error
		{
			ptr = FlushToTopLevel(in,holdDepth,0);
			break;
		}
		ptr = ReadNextSystemToken(in,ptr,word,false,false); // real token read
		char* original = ptr - strlen(word);
		if (*word == '\\' && word[1] == 'n') continue; // newline means pretend new table entry
		if (!stricmp(word, (char*)":debug"))
		{
			DebugCode(word);
			continue;
		}
		if (!stricmp(word, (char*)":quit"))
		{
			myexit("table has :quit");
		}
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
		int holdtrace = trace;
        trace = 0;
		strcpy(tableinput, readBuffer);
        while (ALWAYS)
		{
            if (!startup) ptr = ReadSystemToken(ptr,word);	//   next item to associate
			if (!stricmp(word,(char*)":debug")) 
			{
				DebugCode(word);
				continue;
			}
			if (!stricmp(word, (char*)":trace"))
			{
				trace = (unsigned int) -1;
				echo = true;
				continue;
			}
            startup = false;
			if (!*word) break;					//   end of LINE of items stuff
            if (*word == '\t') 
                *word = '*';     // tab forces fill with *

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
			MEANING base = MakeMeaning(baseWord);
            if (convertTabs) ptr = SkipWhitespace(ptr);
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
        *post = 0;
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
					systemArgumentList = at + strlen(at);
				}
				*systemArgumentList++ = ')';	//   end of call setup
				*systemArgumentList = 0;
				
				currentRule = NULL;
				FunctionResult result;
				AllocateOutputBuffer();
				trace = holdtrace;
				if (trace) Log(USERLOG, "%s%s\r\n", currentFunctionDefinition->word, argumentList);
				DoFunction(currentFunctionDefinition->word, argumentList, currentOutputBase, result);
				trace = 0;
				FreeOutputBuffer();
				if (!choiceArg) break;
			}
		}
		if (fromtopic) break; // one entry only
	}
    convertTabs = true;
	FreeBuffer();
	tableinput = NULL;
	FreeBuffer(); // not required to happen if error happens

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

static char* ReadKeyword(char* word,char* ptr,bool& notted, int& quoted,MEANING concept,uint64 type,bool ignoreSpell,unsigned int build,bool duplicate,bool startOnly,bool endOnly,bool emoticon)
{
	if (emoticon) type |= EMOJI;
	// read the keywords zone of the concept
	char* at;
	MEANING M;
	WORDP D;
	size_t len = strlen(word);
	switch(*word) 
	{
		case '!':	// excuded keyword
			if (len == 1) 
                BADSCRIPT((char*)"CONCEPT-5 Must attach ! to keyword in %s\r\n",Meaning2Word(concept)->word);
			if (word[1] == '!') BADSCRIPT((char*)"CONCEPT-5 Cannot use ! after !! in %s\r\n", Meaning2Word(concept)->word);
			notted = true;
			ptr -= len;
			if (*ptr == '!') ++ptr;
			break;
		case '\'': 
			if (len == 1) BADSCRIPT((char*)"CONCEPT-6 Must attach ' to keyword in %s\r\n",Meaning2Word(concept)->word);
			if (word[1] == '\'')
			{
				if (word[2] == '\'') BADSCRIPT((char*)"CONCEPT-5 Cannot use ' after ' in %s\r\n", Meaning2Word(concept)->word);
				quoted = 2;
			}
			else quoted = 1;	//   since we emitted the ', we MUST emit the next token
			ptr -= len;
			if (*ptr == '\'') ++ptr;
			if (*ptr == '\'') ++ptr;
			break;
		default:
			if (*word == USERVAR_PREFIX || (*word == '_' && IsDigit(word[1])) || (*word == SYSVAR_PREFIX && IsLowerCase(word[1]) && IsLowerCase(word[2]))) BADSCRIPT((char*)"CONCEPT-? Cannot use $var or _var or %var as a keyword in %s\r\n",Meaning2Word(concept)->word);
			if (*word == '~') MakeLowerCase(word); //   sets are always lower case
			at = strchr(word + 1, '~'); //   wordnet meaning request, confirm definition exists
			if ((*word != '"' && *word != '\'') && at && stricmp(current_language, "english") &&  at[-1] != '*' && IsDigit(at[1])) *at = 0;
			if (*word == '"' && word[1] == '(')// pattern word
            {
                unsigned int flags = 0;
                if (build & BUILD1) flags |= FACTBUILD1; // concept facts from build 1
                MEANING conceptPattern = MakeMeaning(StoreWord("conceptPattern", AS_IS));
                size_t len = strlen(word);
                if (word[len-1] != '"') BADSCRIPT("ConceptPattern not closing with quote")
                if (word[len - 2] != ')') BADSCRIPT("ConceptPattern not closing with )")
                word[len - 1] = 0;
                char* data = AllocateBuffer() ;
                char* startData = data++;
                *startData = '^'; // compiled pattern marker
                ReadPattern(word+1, NULL, data, false, false); //   back up and pass in the paren for pattern
                FreeBuffer();
                M = MakeMeaning(StoreWord(startData, AS_IS));
                CreateFact(M, conceptPattern, concept, flags);
                return ptr;
            }
            else if (at) //   wordnet meaning request, confirm definition exists
			{
				char level[MAX_WORD_SIZE];
				strcpy(level,at);
				M = ReadMeaning(word);
				if (!M) BADSCRIPT((char*)"CONCEPT-7 WordNet word doesn't exist %s\r\n",word)
				WORDP D = Meaning2Word(M);
				int index = Meaning2Index(M);
				if ((GetMeaningCount(D) == 0 && !(GETTYPERESTRICTION(M) & BASIC_POS)) || (index && !strcmp(word,D->word) && index > GetMeaningCount(D)))
				{
					if (index && !stricmp(current_language, "english"))
						WARNSCRIPT((char*)"WordNet word does not have such meaning %s\r\n", word)
					M &= -1 ^ INDEX_BITS;
				}
			}
			else // ordinary word or concept-- see if it makes sense
			{
				char end = word[strlen(word)-1];
                if (nospellcheck) {}
				else if (!IsAlphaUTF8OrDigit(end) && end != '"' && strlen(word) != 1 && !nomixedcase)
				{
					if (IsPunctuation(end)); // no warnings for emoji
					else if (!IsAlphaUTF8DigitNumeric(end));
					else if (end != '.' || strlen(word) > 6) 
						WARNSCRIPT((char*)"last character of keyword %s is punctuation. Is this intended?\r\n", word)
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
			unsigned int flags = 0;
			if (quoted == 1) flags |= ORIGINAL_ONLY;
			else if (quoted == 2) flags |= RAWCASE_ONLY;
			if (duplicate) flags |= FACTDUPLICATE;
            if (startOnly) flags |= START_ONLY;
            if (endOnly) flags |= END_ONLY;
			if (build & BUILD1) flags |= FACTBUILD1; // concept facts from build 1
			FACT* F = CreateFact(M,(notted) ? Mexclude : Mmember,concept, flags); 
			quoted = 0;
			notted = false;
	} 
	return ptr;
}

bool HasBotMember(WORDP concept, uint64 id)
{
    FACT* F = GetObjectHead(concept);
    while (F)
    {
        if (F->verb == Mmember) // manual ValidMemberFact(F) because bot id is passed in
        {
			// if language flags exist, confirm those
			unsigned int factlanguage = (F->flags & FACTLANGUAGEBITS) << LANGUAGE_SHIFT;
			// if we are in universal language, we can see all facts
			// if a fact is in universal language, it can be seen by any language - limit 7 languages
			if (factlanguage && language_bits && factlanguage != language_bits) return false;

            // limited to a specific bot
            // We ARE allowed to add to general in existing layer
            if (id && F->botBits & id) return true; // not allow generic fact?
            // we are general and it has general already
            if (!id) return true;
        }
        F = GetObjectNext(F);
    }
    return false;
}

static char* ReadBot(char* ptr)
{
	*scopeBotName = ' ';
	ptr = SkipWhitespace(ptr);
	char* original = ptr;
	if (IsDigit(*ptr))
	{
		int64 n;
		ptr = ReadInt64(ptr,n); // change bot id
		myBot = n;
	}
	MakeLowerCopy(scopeBotName+1,ptr); // presumes til end of line
	size_t len = strlen(scopeBotName);
	while (scopeBotName[len-1] == ' ') scopeBotName[--len] = 0;
	bool oldecho = echo;
	echo = true;
	if (len != 0)
	{
		strcat(scopeBotName, " "); // single trailing space
		char* x;
		while ((x = strchr(scopeBotName, ','))) *x = ' ';	// change comma to space. all bot names have spaces on both sides
	}
	Log(USERLOG,"Reading bot restriction: %s\r\n", original);
	echo = oldecho;
	return "";
}

static char* ReadTopic(char* ptr, FILE* in,unsigned int build)
{
	patternContext = false;
	displayIndex = 0;
	char* data = (char*) mymalloc(MAX_TOPIC_SIZE); // use a big chunk of memory for the data
	if (!data) ReportBug("FATAL: ReadTopic malloc failed");
	*data = 0;
	char* pack = data;
	
	++topicCount;
	*currentTopicName = 0;
	unsigned int flags = 0;
	bool topicFlagsDone = false;
	bool keywordsDone = false;
	int parenLevel = 0;
	MEANING topicValue = 0;
	WORDP topicName = NULL;
	unsigned int gambits = 0;
	unsigned int toplevelrules = 0; // does not include rejoinders
	currentRuleID = 0;	// reset rule notation
	verifyIndex = 0;	
	bool notted = false;
	int quoted = 0;
	bool stayRequested = false;
    int buffercount = bufferIndex;
	int frameindex = globalDepth;
	if (setjmp(scriptJump[++jumpIndex]))
	{
        bufferIndex = buffercount;
		ptr = FlushToTopLevel(in, frameindex,data); //   if error occurs lower down, flush to here
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
    		Log(USERLOG,"Reading topic %s\r\n",currentTopicName);
			topicName = FindWord(currentTopicName);
			if (topicName && (topicName->internalBits & CONCEPT))
				WARNSCRIPT((char*)"TOPIC-1 Concept already defined with this topic name %s\r\n", currentTopicName)
			if (!IsLegalName(currentTopicName)) BADSCRIPT((char*)"TOPIC-2 Illegal characters in topic name %s\r\n", currentTopicName)
			topicName = StoreWord(currentTopicName);
			
			// note we have seen definition
			char cumulate[MAX_WORD_SIZE];
			strcpy(cumulate, topicName->word);
			strcat(cumulate, "$");
			char* bot = (*scopeBotName == ' ') ? (scopeBotName + 1) : scopeBotName;
			strcat(cumulate, bot);
			size_t l = strlen(cumulate);
			if (cumulate[l - 1] == ' ') cumulate[l - 1] = 0; // remove trailing blank
			WORDP alt = StoreWord(cumulate, AS_IS);
			alt->internalBits |= BEEN_HERE; // has been defined
			
			topicValue = MakeMeaning(topicName);
			// handle potential multiple topics of same name
			duplicateCount = 0;
			while (topicName->internalBits & TOPIC || topicName->objectHead)// allow concepts and topics to be referenced
			{
				++duplicateCount;
				char name[MAX_WORD_SIZE];
				sprintf(name,(char*)"%s%c%u",currentTopicName,DUPLICATETOPICSEPARATOR,duplicateCount);
				topicName = StoreWord(name);
				if (!*duplicateTopicName) 
					strcpy(duplicateTopicName,currentTopicName);
			}
			strcpy(currentTopicName,topicName->word);
            AddMap((char*)"    topic:", topicName->word, &mapItemCount);
            if (mapFileJson) fprintf(mapFileJson, (char*)",\r\n\"rules\" : [\r\n");
            mapRuleCount = 0;

			AddInternalFlag(topicName,(unsigned int)(build|TOPIC));
			topicName->w.topicBots = NULL;
            currentTopicBots = NULL;
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
            if (!topicName->w.topicBots && *scopeBotName)  topicName->w.topicBots = AllocateHeap(scopeBotName);
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
					topicName->w.topicBots = AllocateHeap(botlist,strlen(botlist)); // bot=harry,georgia,roger
                    currentTopicBots = topicName->w.topicBots;
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
			else if (!keywordsDone) ptr = ReadKeyword(word,ptr,notted,quoted,topicValue,0,false,build,false,false,false,false);//   absorb keyword list
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
					strcpy(pack,&ENDUNITTEXT[1]);	//   init 1st rule
					pack += strlen(pack);
				}
				ReadTopLevelRule(topicName,lowercaseForm,ptr,in,pack,data);
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
	unsigned int checksum = ((unsigned int) Hashit((unsigned char*) data, len,hasUpperCharacters,hasUTF8Characters, hasSeparatorCharacters)) & 0x0fffffff;
	
	//   trailing blank after jump code
	if (len >= (MAX_TOPIC_SIZE-100)) BADSCRIPT((char*)"TOPIC-7 Too much data in one topic\r\n")
	char filename[SMALL_WORD_SIZE];
	sprintf(filename,(char*)"%s/BUILD%s/script%s.txt", topicfolder,baseName,baseName);
	FILE* out = FopenUTF8WriteAppend(filename);
	if (out)
	{
		// write out topic data
		char* restriction = (topicName->w.topicBots) ? topicName->w.topicBots : (char*)"all";
		unsigned int len1 = (unsigned int)strlen(restriction);
		char qualname[100];
		if (language_bits)
		{
			sprintf(qualname, "%s`-%d", currentTopicName,language_bits >> LANGUAGE_SHIFT);
		}
		else strcpy(qualname, currentTopicName);
		fprintf(out, (char*)"TOPIC: %s 0x%x %u %u %u %u %s\r\n", qualname, (unsigned int)flags, (unsigned int)checksum, (unsigned int)toplevelrules, (unsigned int)gambits, (unsigned int)(len + len1 + 7), currentFilename);
		fprintf(out, (char*)"\" %s \" ", restriction);
		fprintf(out, (char*)"%s\r\n", data);
		fclose(out); // dont use FClose
	}
	myfree(data);
    if (mapFileJson) fprintf(mapFileJson, (char*)"]\r\n}\r\n");

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
            *readBuffer = 0;
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
		if (*word == '#' && *basic == '-') AddInternalFlag(D,CONSTANT_IS_NEGATIVE);
		Log(USERLOG,"Rename %s as %s\r\n",basic,word);
	}	
	renameInProgress = false;
	return ptr;
}

static char* ReadPlan(char* ptr, FILE* in,unsigned int build)
{
	char planName[MAX_WORD_SIZE];
	char baseName[MAX_WORD_SIZE];
	displayIndex = 0;
	*planName = 0;
	functionArgumentCount = 0;
	int parenLevel = 0;
	bool gettingArguments = true;
	endtopicSeen = false;
	patternContext = false;
	int baseArgumentCount = 0;
	unsigned int duplicateCount = 0;
    WORDP plan = NULL;
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
			Log(USERLOG,"Reading plan %s\r\n",planName);

			// handle potential multiple plans of same name
			plan = FindWord(planName);
			char name[MAX_WORD_SIZE];
			strcpy(name,planName);
			if (plan) baseArgumentCount = plan->w.planArgCount;
			while (plan && plan->internalBits & FUNCTION_NAME)
			{
				++duplicateCount;
				sprintf(name,(char*)"%s%c%u",baseName,DUPLICATETOPICSEPARATOR,duplicateCount);
				plan = FindWord(name);
				strcpy(planName,name);
			}

            plan = StoreWord(planName);
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
				strcpy(functionArguments[++functionArgumentCount],word);
				if (functionArgumentCount > MAX_ARG_LIMIT)  BADSCRIPT((char*)"PLAN-7 Too many callArgumentList to %s - max is %d\r\n",planName,MAX_ARG_LIMIT)
				continue;
			default:
				BADSCRIPT((char*)"PLAN-7 Bad argument to plan definition %s",planName)
		}
	}
	if (!plan) return ptr; //   nothing defined
	if (parenLevel) BADSCRIPT((char*)"PLAN-5 Failure to balance ( in %s\r\n",planName)
	if (duplicateCount && functionArgumentCount != baseArgumentCount) 
		BADSCRIPT((char*)"PLAN->? Additional copies of %s must have %d arguments\r\n",planName,baseArgumentCount)
	AddInternalFlag(plan,(unsigned int)(FUNCTION_NAME|build|IS_PLAN_MACRO));
    plan->w.planArgCount = functionArgumentCount;
	currentFunctionDefinition = plan;
	
	char* data = (char*) mymalloc(MAX_TOPIC_SIZE); // use a big chunk of memory for the data
	if (!data) ReportBug("Malloc failed for ReadPlan");
	*data = 0;
	char* pack = data;

	unsigned int toplevelrules = 0; // does not include rejoinders
    int buffercount = bufferIndex;
	int frameindex = globalDepth;
	if (setjmp(scriptJump[++jumpIndex]))
	{
		globalDepth = frameindex;
        bufferIndex = buffercount;
        ptr = FlushToTopLevel(in, frameindex,data); //   if error occurs lower down, flush to here
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
					strcpy(pack,&ENDUNITTEXT[1]);	//   init 1st rule
					pack += strlen(pack);
				}
				ReadTopLevelRule(plan,lowercaseForm,ptr,in,pack,data);
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
	if (build == BUILD0) sprintf(file,"%s/BUILD0/plans0.txt", topicfolder);
	else sprintf(file,"%s/BUILD0/plans1.txt", topicfolder);
		
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
	fprintf(out,(char*)"PLAN: %s %u %u %u %s\r\n",planName,(unsigned int) functionArgumentCount,(unsigned int) toplevelrules,(unsigned int)(len + len1 + 7),currentFilename); 
	fprintf(out,(char*)"\" %s \" %s\r\n",restriction,data);
	fclose(out); // dont use FClose

	myfree(data);
	return ptr;
}

static char* ReadWord(char* ptr, FILE* in, unsigned int build) // readquery: name "xxxxx" text
{
	char word[MAX_WORD_SIZE];
	char word1[MAX_WORD_SIZE];
	char word2[MAX_WORD_SIZE];
	char tmp[MAX_WORD_SIZE];
	//word: canonical form / pluralorpast / pastparticile  FLAG... Flagn  "xxxxxxx"
	ptr = ReadNextSystemToken(in, ptr, word, false); // name of word
	if (!*word) return ptr;
	if (!IsAlphaUTF8(*word)) BADSCRIPT((char*)"word defined  %s must be alpha\r\n", word);
	if (TopLevelUnit(word)) //   definition ends when another major unit starts
	{
		ptr -= strlen(word); //   let someone else see this starter 
		return ptr;
	}
	ptr = ReadNextSystemToken(in, ptr, word1, false);
	bool form1 = false;
	bool form2 = false;
	if (*word1 == '/')
	{
			ptr = ReadNextSystemToken(in, ptr, word1, false);
			if (!IsAlphaUTF8(*word1)) BADSCRIPT((char*)"word form1 defined  %s must be alpha\r\n", word1);
			form1 = true;
			ptr = ReadNextSystemToken(in, ptr, word2, false);
			if (*word2 == '/')
			{
				ptr = ReadNextSystemToken(in, ptr, word2, false);
				if (!IsAlphaUTF8(*word2)) BADSCRIPT((char*)"word form2 defined  %s must be alpha\r\n", word2);
				form2 = true;
				ptr = ReadNextSystemToken(in, ptr, tmp, false);
			}
			else strcpy(tmp, word2);
	}
	else strcpy(tmp, word1);
	WORDP D = StoreWord(word, AS_IS);
	WORDP E = NULL;
	WORDP F = NULL;
	if (form1)  E = StoreWord(word1, AS_IS);
	if (form2) F = StoreWord(word2, AS_IS);

	while (*tmp && !TopLevelUnit(tmp))
	{
		if (*tmp != '"') // not a gloss (currently gloss not supported)
		{
			uint64 properties = FindPropertyValueByName(tmp);
			uint64 sysflags = FindSystemValueByName(word);
			unsigned int bits2 = (unsigned int)FindParseValueByName(word);
			if (properties & NOUN)
			{
				if (D->internalBits & UPPERCASE_HASH) properties |= NOUN_PROPER_SINGULAR;
				else properties |= NOUN_SINGULAR;
			}
			else if (properties & VERB) properties |= VERB_INFINITIVE;

			if (properties) AddProperty(D, properties);
			if (sysflags) AddSystemFlag(D, sysflags);
			if (bits2) AddParseBits(D, bits2);

			if (form1)
			{
				if (properties & NOUN)
				{
					if (D->internalBits & UPPERCASE_HASH) AddProperty(E, NOUN_PROPER_PLURAL|NOUN);
					else AddProperty(E, NOUN_PLURAL|NOUN);
				}
				else if (properties & VERB)
				{
					AddProperty(E, VERB_PAST|VERB);
					if (form2)  AddProperty(F, VERB_PAST_PARTICIPLE|VERB);
				}
			}
		}
		ptr = ReadNextSystemToken(in, ptr, tmp, false);
	}
	if (TopLevelUnit(tmp))  ptr -= strlen(tmp); //   let someone else see this starter 
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
		if (!at) { BADSCRIPT((char*)"query body %s must end in quotes\r\n", query); }
		else *at = 0;
 	    D->w.userValue = AllocateHeap(query+1);    
	}
	return ptr;
}

static char* ReadReplace(char* ptr, FILE* in, unsigned int build)
{
	while (ALWAYS) //   read as many tokens as needed to complete the definition (must be within same file)
	{
		char word[MAX_WORD_SIZE];
		char* original = word;
		char replace[MAX_WORD_SIZE];
		ptr = ReadNextSystemToken(in,ptr,original,false);
		if (!stricmp(original,(char*)"replace:")) ptr = ReadNextSystemToken(in,ptr,original,false); // keep going with local replace loop
		if (!*original) break;	//   file ran dry
		if (TopLevelUnit(original)) //   definition ends when another major unit starts
		{
			ptr -= strlen(original); //   let someone else see this starter 
			break; 
		}
		ptr = ReadNextSystemToken(in,ptr,replace,false);
		if (*replace == '(')
		{
				char data[MAX_WORD_SIZE];
				char* pack = data;
				while (*--ptr != '('); // back up to the (
				ptr = ReadPattern(ptr, in, pack, false, false);
				char* pat = data-1;
				while ((pat = strchr(++pat, '~')) )
				{
					if (*(pat - 1) == ' ')
					{
						BADSCRIPT((char*)"CONCEPT-1 Concept name not allowed in replace: pattern %s\r\n", pat - 1)
					}
				}
				strcpy(replace, data);
		}
		else if (!stricmp(replace, "![")) // reacquire ![xxx xxx]value
		{
		    // replace: jack_rusell ![terrier]Jack + Russell + terrier
			// This says to replace jack_russell with Jack + Russel + terrior ONLY if the word immediately following the match
			char* end = strchr(ptr, ']');
			*end = 0;
			strcat(replace, ptr);
			strcat(replace, "]");
			*end = ']';
			char extra[MAX_WORD_SIZE];
			ptr = ReadNextSystemToken(in, end+1, extra, false);
			strcat(replace, extra);
		}
		if (TopLevelUnit(replace)) //   definition ends when another major unit starts
		{
			ptr -= strlen(replace); //   let someone else see this starter 
			break; 
		}

		if (*original == '\'') // original only - single quoted
        {
            memmove(original + 1, original, strlen(original)+1);
            *original = '*';
        }
		SetSubstitute( original, replace, build, DO_PRIVATE, 0);
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
		if (!(D->systemFlags & HAS_SUBSTITUTE) && *D->word != '^' && *D->word != '$' &&
			*D->word != '%' && *D->word != '~')
		{
			D->internalBits |= DO_NOISE;
		}
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

void WriteCanon(char* word, char* canon, char* form)
{
	WritePatternWord(canon);	// must recognize this word for spell check
	WritePatternWord(word);		// must recognize this word for spell check

	// handle multiple language restrictions
	char restrict[MAX_WORD_SIZE];
	*restrict = 0;
	if (multidict )
	{
		sprintf(restrict, "~l%d", languageIndex);
		strcat(canon, restrict);
		strcat(word, restrict);
	}

	char filename[SMALL_WORD_SIZE];
	sprintf(filename,(char*)"%s/BUILD%s/canon%s.txt", topicfolder,baseName,baseName);
	FILE* out = FopenUTF8WriteAppend(filename);
	if (!form) form = "";
	fprintf(out,(char*)" %s %s %s\r\n", canon,word,form);
	fclose(out); // dont use FClose
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
		char form[MAX_WORD_SIZE];
		*form = 0;
		if (*ptr) ReadNextSystemToken(in, ptr, form, false,true);
		if (!stricmp(form, "MORE_FORM") || !stricmp(form, "MOST_FORM"))
		{
			ptr = ReadNextSystemToken(in, ptr, form, false);
		}
		else *form = 0;
		WriteCanon(word,canon,form);
	}
	return ptr;
}

static char* ReadConcept(char* ptr, FILE* in,unsigned int build)
{
	char conceptName[MAX_WORD_SIZE];
	*conceptName = 0;
	MEANING concept = 0;
	WORDP D = NULL;
	bool ignoreSpell = false;
	
	patternContext = false;
	bool more = false;
	bool undeclared = true;
    bool startOnly = false;
    bool endOnly = false;
	int parenLevel = 0;
	uint64 type = 0;
	uint64 sys;
	bool notted = false;
	bool emoticon = false;
	int quoted = 0;
	bool duplicate = false;
	while (ALWAYS) //   read as many tokens as needed to complete the definition (must be within same file)
	{
		char word[MAX_WORD_SIZE];
		char* z = ReadNextSystemToken(in,ptr,word,false);
		ptr = z;
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
			D = StoreWord(conceptName,AS_IS); 
			D->internalBits |= CONCEPT;

			// note we have seen definition
			char cumulate[MAX_WORD_SIZE];
			strcpy(cumulate, conceptName);
			strcat(cumulate, "$");
			char* bot = (*scopeBotName == ' ') ? (scopeBotName + 1) : scopeBotName;
			strcat(cumulate, bot);
			size_t l = strlen(cumulate);
			if (cumulate[l - 1] == ' ') cumulate[l - 1] = 0; // remove trailing blank
			WORDP alt = StoreWord(cumulate, AS_IS);
			alt->internalBits |= BEEN_HERE; // has been defined

			concept = MakeMeaning(D);
			sys = type = 0;
			parenLevel = 0;
			Log(USERLOG,"Reading concept %s\r\n",conceptName);
			AddMap((char*)"    concept:", conceptName, &mapItemCount);
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
                if (!stricmp(word, (char*)"INTERJECTION")) // match member as interjection
                {
                    endOnly = startOnly = true;
                    continue;
                }
				if (!stricmp(word, (char*)"EMOTICON"))
				{
					emoticon = true;
					continue;
				}
                if (!stricmp(word, (char*)"START_ONLY")) // match member only at start of sentence
                {
                    startOnly = true;
                    continue;
                }
                if (!stricmp(word, (char*)"END_ONLY")) // match member only at end of sentence
                {
                    endOnly = true;
                    continue;
                }
                char* paren = strchr(word,'(');
				if (paren) // handle attachment of paren + stuff
				{
					while (*--ptr != '(');
					*paren = 0;
				}

				ptr = SkipWhitespace(ptr);
				uint64 bits = FindPropertyValueByName(word);
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
			if (!more && !noconcept)
			{
                int buildbits = D->internalBits & (BUILD0 | BUILD1 );
				if (!myBot && GetObjectHead(D)  && buildbits && !nowarndupconcept)
				{
					if (language_bits)
					{
						FACT* F = GetVerbHead(D);
						if (FactLanguage(F) == language_bits) WARNSCRIPT((char*)"CONCEPT-3 Concept/topic already defined %s\r\n", conceptName)
					}
					else WARNSCRIPT((char*)"CONCEPT-3 Concept/topic already defined %s\r\n",conceptName)
				}
				if (HasBotMember(D, myBot) && GetObjectHead(D) &&  !nowarndupconcept)
                {
					WARNSCRIPT((char*)"CONCEPT-3 Concept/topic already defined %s\r\n", conceptName)
                }
            }
			AddInternalFlag(D,(unsigned int)build);
			if (duplicate) AddInternalFlag(D, CONCEPT_DUPLICATES);
			else AddInternalFlag(D, NO_CONCEPT_DUPLICATES);
		}

		// read the keywords zone of the concept
		switch(*word) //   THE MEAT OF CONCEPT DEFINITIONS
		{
			case '(':  case '[':	// start keyword list
				if (parenLevel) BADSCRIPT((char*)"CONCEPT-5 Cannot use [ or ( within a keyword list for %s\r\n",conceptName);
				parenLevel++;
				isConcept = emoticon;
				break;
			case ')': case ']':		// end keyword list
				--parenLevel;
				if (parenLevel < 0) BADSCRIPT((char*)"CONCEPT-6 Missing ( for concept definition %s\r\n",conceptName)
				break;
			default: 
				 ptr = ReadKeyword(word,ptr,notted,quoted,concept,type,ignoreSpell,build,duplicate,startOnly,endOnly,emoticon);
		}
		if (parenLevel == 0) break;

	}
	if (parenLevel) BADSCRIPT((char*)"CONCEPT-7 Failure to give closing ( in concept %s\r\n",conceptName)
	isConcept = false;

	return ptr;
}

static void ReadTopicFile(char* name,uint64 buildid) //   read contents of a topic file (.top or .tbl)
{	
    convertTabs = true;
    tableinput = NULL;
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
			WARNSCRIPT((char*)"Missing topic file %s\r\n",name) 
			++missingFiles;
		}
		return;
	}
    char word[MAX_WORD_SIZE];
    *readBuffer = 0; // insure no carryover from elsewhere
    ReadNextSystemToken(NULL, NULL, word, false, false); // flush cache
    build &= -1 ^ FROM_FILE; // remove any flag indicating it came as a direct file, not from a directory listing

	Log(USERLOG,"\r\n----Reading file %s   %s\r\n",currentFilename, scopeBotName);
	char map[MAX_WORD_SIZE];
	char file[MAX_WORD_SIZE];
	GetCurrentDir(file, MAX_WORD_SIZE);
	sprintf(map,"%s/%s",file,name);
	char* find = map;
	while ((find = strchr(find,'\\'))) *find = '/';
	AddMap((char*)"file:", map, &mapTopicFileCount);
    if (mapFileJson) fprintf(mapFileJson, (char*)",\r\n\"items\" : [\r\n");
    mapItemCount = 0;
    mapRuleCount = 0;

	//   if error occurs lower down, flush to here
	patternContext = false;
	char* ptr = "";
    int buffercount = bufferIndex;
	int frameindex = globalDepth;
	if (setjmp(scriptJump[++jumpIndex]))
	{
		bufferIndex = buffercount;
        ptr = FlushToTopLevel(in, frameindex,0);
	}
	while (ALWAYS) 
	{
		isConcept = false;
		ptr = ReadNextSystemToken(in,ptr,word,false); //   eat tokens (should all be top level)
		if (!*word) break;						//   no more tokens found

		currentFunctionDefinition = NULL; //   can be set by ReadTable or ReadMacro
		if (!stricmp(word,(char*)":quit"))
			break;
				
		if (*word == ':' && word[1] && !strstr(readBuffer, "^eval"))		// testing command not near an eval
		{
			char output[MAX_WORD_SIZE];
			DoCommand(readBuffer, output);
			*readBuffer = 0;
			*ptr = 0;
		}
		else if (!stricmp(word, (char*)"concept:")) ptr = ReadConcept(ptr, in, build);
		else if (!stricmp(word, (char*)"query:")) ptr = ReadQuery(ptr, in, build);
		else if (!stricmp(word, (char*)"word:")) ptr = ReadWord(ptr, in, build);
		else if (!stricmp(word, (char*)"replace:")) ptr = ReadReplace(ptr, in, build);
		else if (!stricmp(word, (char*)"ignorespell:")) ptr = ReadIgnoreSpell(ptr, in, build);
		else if (!stricmp(word, (char*)"prefer:")) ptr = ReadPrefer(ptr, in, build);
		else if (!stricmp(word, (char*)"canon:")) ptr = ReadCanon(ptr, in, build);
		else if (!stricmp(word, (char*)"topic:"))  ptr = ReadTopic(ptr, in, build);
		else if (!stricmp(word, (char*)"plan:"))  ptr = ReadPlan(ptr, in, build);
		else if (!stricmp(word, (char*)"language:"))
		{
			ptr = ReadNextSystemToken(in, ptr, word, false);
			if (multidict)
			{
				SetLanguage(word);
				Log(USERLOG, "\r\n>>Setting language %s\r\n", word);
			}
		}
		else if (!stricmp(word, (char*)"bot:"))
		{
			globalBotScope = false; // lasts for this file
			ptr = ReadBot(ptr);
		}
		else if (!strnicmp(word, (char*)"debug:", 6))
		{
			DebugCode("");
		}
		else if (!stricmp(word,(char*)"table:")) ptr = ReadTable(ptr,in,build,false);
		else if (!stricmp(word,(char*)"rename:")) ptr = ReadRename(ptr,in,build);
		else if (!stricmp(word,(char*)"describe:")) ptr = ReadDescribe(ptr,in,build);
		else if (!stricmp(word, (char*)"patternMacro:") || !stricmp(word, (char*)"outputMacro:") || !stricmp(word, (char*)"dualMacro:") || !stricmp(word, (char*)"tableMacro:"))
		{
			char* data = AllocateBuffer();
			ptr = ReadMacro(ptr, in, word, build,data);
			FreeBuffer();
		}
		else BADSCRIPT((char*)"FILE-1 Unknown top-level declaration %s in %s\r\n",word,name)
	}
	FClose(in); // this should be the only such, not fclose.
    if (mapFileJson) fprintf(mapFileJson, (char*)"\r\n]\r\n}");
	--jumpIndex;
	if (hasHighChar)
	{
		WARNSCRIPT((char*)"File %s has no utf8 BOM but has character>127 - extended Ansi changed to normal Ascii line '%c' %x %s\r\n", name, hasHighChar,hasHighChar,readBuffer) // should have been utf 8 or have no high data.
	}
	if (!globalBotScope) // restore any local change from this file
	{
		myBot = 0;
		*scopeBotName = 0;
	}
}

static void DoubleCheckDefinition(unsigned int build,char* topicfolder,char* baseName)
{
	uint64 oldbot = myBot;
    HEAPREF list = undefinedCallThreadList;
	while (list)
	{
        uint64 functionNamex;
        uint64 filenamex;
        uint64 linex;
        list = UnpackHeapval(list, functionNamex, filenamex,linex);
		if (!functionNamex) 
			continue; // reference check canceled

        char* functionData = (char*)functionNamex;
		myBot = *(uint64*)functionData;
        char* filename = (char*)filenamex;
        char* line = (char*) linex;
        int args = functionData[8] ;
		strcpy(currentFilename, filename);
		currentFileLine = (int)(uint64)line;
		char* fn = functionData + 9;
		char* botname = functionData + 10 + strlen(fn);
        WORDP D = FindWord(fn);
		WORDP fnword = NULL;

		// allow function calls indirect off variables
		if (!D  && fn[1] != USERVAR_PREFIX) 
		{
			WARNSCRIPT((char*)"Undefined function %s in bot %s\r\n", fn, botname)
			char filename[SMALL_WORD_SIZE];
			sprintf(filename, (char*)"%s/BUILD%s/macros%s.txt", topicfolder, baseName, baseName);
			FILE* out = FopenUTF8WriteAppend(filename);
			fprintf(out, (char*)"%s %d %s\r\n", fn, args,botname);
			fclose(out); // dont use Fclose
		}
	}

	list = undefinedConceptThreadList;
	while (list)
	{
		uint64 conceptNamex;
		uint64 filenamex;
		uint64 linex;
		list = UnpackHeapval(list, conceptNamex, filenamex, linex);
		if (!conceptNamex)
			continue; // reference check canceled
		char* conceptData = (char*)conceptNamex;
		myBot = *(uint64*)conceptData;
		char* filename = (char*)filenamex;
		char* line = (char*)linex;
		strcpy(currentFilename, filename);
		currentFileLine = (int)(uint64)line;
		char* concept = conceptData + 9;
		char* botname = conceptData + 10 + strlen(concept);

		WORDP D = FindWord((char*)concept);
		if (D && D->internalBits & TOPIC) continue;
		WARNSCRIPT((char*)"Undefined concept %s in bot %s\r\n", concept, botname)
	}
	myBot = oldbot;
}

static void DoubleCheckReuse()
{
	char file[200];
	sprintf(file,"%s/missingLabel.txt", topicfolder);
	FILE* in = FopenReadWritten(file);
	if (!in) return;

	char label[MAX_WORD_SIZE];
    char bothead[MAX_WORD_SIZE];
    while (ReadALine(readBuffer,in) >= 0)
	{
		char *ptr = ReadCompiledWord(readBuffer, label);		// topic + label
		if (*label == '~')
		{
			MakeLowerCase(label);
			char* dot = strchr(label, '.');
			if (dot) MakeUpperCase(dot + 1);
		}
		ptr = ReadCompiledWord(ptr, bothead);				// from file
        MakeUpperCase(bothead);
        ptr = ReadCompiledWord(ptr,tmpWord); // filename				// from file
		while (!strstr(tmpWord,".top")) ptr = ReadCompiledWord(ptr, tmpWord); // extra bot names
		int line;
		ptr = ReadInt(ptr,line); // line number	
        // from line
        char labelx[MAX_WORD_SIZE];
        sprintf(labelx, "%s-%s", label, bothead);
        WORDP D = FindWord(labelx);
        if (!D) // cant find as bot specific, check for general
        {
            sprintf(labelx, "%s-*", label);
            D = FindWord(labelx);
        }

		if (!D) 
            WARNSCRIPT((char*)"Missing label %s for reuse in bot %s in File: %s Line: %d \r\n",label, bothead, tmpWord,line)
	}
	fclose(in); // dont use Fclose
	remove(file);
}

static void InsureSafeSpellcheck(char* word, bool dictionaryBuild)
{ // all pieces of a multiword keyword need to avoid spell changes
	// there may be a problem if the master key becomes universal and its pieces do not
	if (!word || !*word) return;
	// Spellcheck should not harm keywords or components of keywords. Insure some mark exists.
	// Spellcheck can adjust case without causing recognition damage.
	WORDP X = FindWord(word, 0, LOWERCASE_LOOKUP);
	if (X && (X->properties & TAG_TEST || X->systemFlags & PATTERN_WORD)) return;
	WORDP Y = FindWord(word, 0, UPPERCASE_LOOKUP);
	if (Y && (Y->properties & TAG_TEST || Y->systemFlags & PATTERN_WORD)) return;

	// protect lower case form ONLY if is multiword header
	if (strchr(word,'_'))
	{
		char data[MAX_WORD_SIZE];
		MakeLowerCopy(data, word);
		WORDP Z;
		size_t len = strlen(data);
		if (data[len - 1] == 's') Z = StoreWord(data, AS_IS); // dont force uppercase on plurals like Cousins
		else Z = StoreWord(word, AS_IS);
		if (Z)
		{
			AddSystemFlag(Z, PATTERN_WORD);
			AddWordItem(Z, dictionaryBuild);
		}
	}
	else // protect in whatever case it is
	{
		WORDP  Z = StoreWord(word, AS_IS);
		AddSystemFlag(Z, PATTERN_WORD);
		AddWordItem(Z, dictionaryBuild);
	}
}

static size_t WriteCMore(FACT* F, char*&word,FILE* out,size_t lineSize,uint64 build)
{
	char wordcopy[MAX_WORD_SIZE];
	strcpy(wordcopy, word);
	char* tilde = strchr(wordcopy+1, '~');
	char* tick = strchr(wordcopy, '`'); // bot id marker
	if (tick) *tick = 0; //back to original word before bot added
	unsigned int langbits = language_bits;
	if (tilde) // use given language
	{
		language_bits = atoi(tilde + 2) << LANGUAGE_SHIFT;
		*tilde = 0; //back to original word before language added
	}
	// insure whole word safe from spell check
	InsureSafeSpellcheck(wordcopy, build); // protect whole word
	char* sep = strchr(word, '_');
	size_t wlen = strlen(word) + 1;
	strcat(word, " "); // space separator
	fwrite(word, 1, wlen, out);
    lineSize += wlen;
    if (lineSize > 500) // avoid long lines
    {
        fprintf(out, (char*)" %s", (char*)"\r\n    ");
        lineSize = 0;
    }
    *word = 0;
	language_bits = langbits;
	return lineSize;
}

static char* MakeToken(char* input, char* word)
{
	strcpy(word, input + 1);
	size_t len = strlen(word);
	word[len - 1] = ' ';			// remove trailing quote
	ForceUnderscores(word);
	return word;
}

static size_t WriteExclude(FILE* out, FACT* F, char* word, size_t lineSize, uint64 build)
{
	WORDP E = Meaning2Word(F->subject);
	AddBeenHere(E);

	char* dict = strchr(word + 1, '~'); // has a wordnet attribute on it
	if (dict) //  full wordnet word reference
	{
		if (E->inferMark != inferMark) SetTriedMeaning(E, 0);
		E->inferMark = inferMark;
		if (dict)
		{
			unsigned int which = atoi(dict + 1);
			if (which) // given a meaning index, mark it
			{
				uint64 offset = 1ull << which;
				SetTriedMeaning(E, GetTriedMeaning(E) | offset);
			}
		}
	}
	char* key = WriteMeaning(F->subject, true);
	char* u = key;
	if (*key != '`' && *key != '"') while ((u = strchr(u, ' '))) *u = '_';// keys must be composites, not spaced words

	if (*E->word == '"') word = MakeToken(E->word, word);// change string to std token
	else if (F->flags & ORIGINAL_ONLY) sprintf(word, (char*)"!'%s ", key);
	else if (F->flags & RAWCASE_ONLY) sprintf(word, (char*)"!!'%s ", key);
	else sprintf(word, (char*)"!%s ", key);

	lineSize = WriteCMore(F, word, out, lineSize, build);
	KillFact(F);
	return lineSize;
}

static size_t WriteMember(FILE* out,FACT* F,char* word,size_t lineSize, uint64 build)
{
	WORDP D = Meaning2Word(F->subject);
	AddBeenHere(D);
	char* member = WriteMeaning(F->subject, true);
	char* u = member;
	while ((u = strchr(u, ' '))) *u = '_';  // cannot be freestanding, must be _ or quoted or backquoted
	if (*D->word == '"') word = MakeToken(D->word, word); // change string to std token
	else if (F->flags & ORIGINAL_ONLY) sprintf(word, (char*)"'%s", member);
	else if (F->flags & RAWCASE_ONLY) sprintf(word, (char*)"''%s", member);
	else sprintf(word, (char*)"%s", member);
	
	// generate header words in correct language as well (universal as needed)
	//GetHeaderWord(word); // trigger any header word in correct language

	char* dict = strchr(word + 1, '~'); // has a wordnet attribute on it
	if (*word == '~' || dict) // concept or full wordnet word reference
	{
		if (D->inferMark != inferMark) SetTriedMeaning(D, 0);
		D->inferMark = inferMark;
		if (dict)
		{
			unsigned int which = atoi(dict + 1);
			if (which) // given a meaning indD, mark it
			{
				uint64 offset = 1ull << which;
				SetTriedMeaning(D, GetTriedMeaning(D) | offset);
			}
		}
	}
	uint64 botid = F->botBits;
	char* lang = strstr(word+1, "~l");  // will be after any botid
	if (botid)// put bot id before any language id
	{
		char number[100];
		char language[100];
		*language = 0;
		if (lang)
		{
			strcpy(language, lang);
			*lang = 0;
		}
		sprintf(number, "`%s", PrintU64(botid));
		strcat(word, number); 
		strcat(word, language);
	}
	// write it out- this INVERTS the order now and when read back in, will be reestablished correctly 
	// but dictionary storage locations will be inverted
	return WriteCMore(F, word, out, lineSize, build);
}

static void WriteConcepts(WORDP D, uint64 build) // do last, so dictionary words max already named for *.bin form
{ // this will leave dict words dirty with BEEN_HERE bits
    char* name = D->word;
	if (*name != '~' || !(D->internalBits & build)) return; // not a topic or concept or not defined this build
	RemoveInternalFlag(D,(BUILD0|BUILD1));
	// write out keywords 
	FILE* out = NULL;
	char filename[SMALL_WORD_SIZE];
	sprintf(filename,(char*)"%s/BUILD%s/keywords%s.txt", topicfolder,baseName,baseName);
	out = FopenUTF8WriteAppend(filename);
	fprintf(out,(D->internalBits & TOPIC) ? (char*)"T%s " : (char*)"   %s ", D->word);
	uint64 properties = D->properties;	
	uint64 bit = START_BIT;
	while (properties && bit)
	{
		if (properties & bit && !(bit & AS_IS))
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
		// dont write this out in keywords see FAKE_NOCONCEPTLIST - some of these go in DICTn file
		if (properties & bit && !(bit & (PATTERN_WORD|NOCONCEPTLIST|MARKED_WORD)))
		{
			char* name = FindSystemNameByValue(bit);
			properties ^= bit;
			fprintf(out,(char*)"%s ",name);
		}
		bit >>= 1;
	}
	if (D->internalBits & FAKE_NOCONCEPTLIST) fprintf(out,(char*)"%s",(char*)"NOCONCEPTLIST ");
	if (D->internalBits & UPPERCASE_MATCH) fprintf(out,(char*)"%s",(char*)"UPPERCASE_MATCH ");
	if (D->internalBits & CONCEPT_DUPLICATES) fprintf(out, (char*)"%s", (char*)"DUPLICATE ");
	else fprintf(out, (char*)"%s", (char*)"NODUPLICATE ");
	int n = 10;
	seeAllFacts = true; // do for all bots at once in all languages

    FACT* E = GetObjectNondeadHead(D);
    while (E)
    {
        if (E->verb == Mmember && E->flags & START_ONLY)
        {
            fprintf(out, (char*)"%s", (char*)"START_ONLY ");
        }
        if (E->verb == Mmember && E->flags & END_ONLY)
        {
            fprintf(out, (char*)"%s", (char*)"END_ONLY ");
        }
        if (E->verb == Mmember && E->flags & (END_ONLY | START_ONLY)) break;

        if (--n == 0) break; // should have found by now
        E = GetObjectNondeadNext(E);
    }
    fprintf(out, (char*)"%s", (char*)"( ");

	size_t lineSize = 0;
	NextInferMark();
	char* buffer = AllocateStack(NULL, maxBufferSize);
	unsigned int oldlanguage = language_bits;

	//  write out simple excludes only
	//We want them as most recent facts to find, so we stop checking when we run out
	MEANING MconceptPattern = MakeMeaning(FindWord("conceptpattern")); // does not have to be found
	FACT* F = GetObjectNondeadHead(D);
	if (F)
	{
		while (F)
		{
			WORDP E = Meaning2Word(F->subject);
			if (build == BUILD1 && !(F->flags & FACTBUILD1)) { ; } // defined by earlier level
			else if (F->verb == MconceptPattern) { ; }
			else if (F->verb == Mexclude && *E->word != '~') //  only relevant simple word facts
			{
				lineSize = WriteExclude(out, F, buffer, lineSize, build);
			}

			F = GetObjectNondeadNext(F);
		}
	}

	// now do set excludes
	F = GetObjectNondeadHead(D);
	if (F)
	{
		while (F)
		{
			WORDP D = Meaning2Word(F->subject);
			if (build == BUILD1 && !(F->flags & FACTBUILD1)) { ; } // defined by earlier level
			else if (F->verb == Mexclude && *D->word == '~') // the only relevant concept exclude facts
			{
				lineSize = WriteExclude(out, F, buffer, lineSize, build);
			}
			F = GetObjectNondeadNext(F);
		}
	}

	// write out set members here, dont write out excludes here 
	F = GetObjectNondeadHead(D);
	if (F)
	{
		while (F)
		{
			if (build == BUILD1 && !(F->flags & FACTBUILD1)) { ; } // defined by earlier level
			else if (F->verb == Mmember)
			{
				lineSize = WriteMember(out, F, buffer, lineSize, build);
				KillFact(F);
			}
			F = GetObjectNondeadNext(F);
		}
	}

	ReleaseStack(buffer);
	fprintf(out, (char*)"%s", ")\r\n");
	fclose(out); // dont use Fclose
	seeAllFacts = false;
}

static void WriteDictionaryChange(FILE* dictout, unsigned int build)
{
	// Note that topic labels (topic.name) and pattern words  will not get written
	
	// All words in the dictionary were written out with language from the dictionaries before.
	// Loading this level may introduce new words with language. Or it can generate
	// UNIVERSAL words, which may make all other same words per language become SUPERCEDED.
	// We write out newly superceded flagged words but not prior ones.
	// Superceding means copying all facts of the old language oriented word to use the
	// new word (killing the old facts). We will not write out the old facts.

	FILE* in = NULL;
	char file[SMALL_WORD_SIZE];
	int layer = 0;
	if (build == BUILD0) 	layer = 0;
	else if (build == BUILD1) layer = 1;
	sprintf(file, (char*)"%s/prebuild%c.bin", tmpfolder,layer + '0');
	in = FopenReadWritten(file);
	if (!in)
	{
		ReportBug((char*)"prebuild bin not found");
		return;
	}
	unsigned int check = Read32(in); // version stamp
	if (check != CHECKSTAMP) 
		ReportBug("Fatal: checkstamp writedictchanged");
	seeAllFacts = true;
	for (WORDP D = dictionaryBase + 1; D < dictionaryFree; ++D)
	{
		if (!D->word) continue; // not a real entry
		D->internalBits &= -1 ^ BEEN_HERE;
		D->systemFlags &= -1 ^ MARKED_WORD;
		uint64 oldproperties,oldflags;
		unsigned int oldintbits,oldlength,newlength, oldheader;
		uint64 newproperties = oldproperties = D->properties;
		uint64 newflags = oldflags = D->systemFlags;
		unsigned int newheader = oldheader = GETMULTIWORDHEADER(D);
		unsigned int newintbits = oldintbits = D->internalBits;
		oldlength = newlength = D->length;
		bool deleted = SUPERCEDED(D);
		unsigned int newsubstituteindex = 0;
		if (D->systemFlags & HAS_SUBSTITUTE && D->w.substitutes)
		{
			newsubstituteindex = Word2Index(D->w.substitutes);
			// if zero means substitute is a full delete
		}
		else if (D->internalBits & CONDITIONAL_IDIOM && D->w.conditionalIdiom)
		{
			newsubstituteindex = Word2Index(D->w.substitutes);
		}

		unsigned int oldsubstituteindex = 0;
		bool changed = true; // default is new words
		bool changedForeign = false;
		bool newlyAdded = false;
		if (D < dictionaryPreBuild[layer]) // word preexisted this level, so see if it changed
		{
			unsigned int offset = (unsigned int)(D - dictionaryBase);
			int result = fread(&oldintbits,1,4,in);
			if (result != 4) // ran out of old words
			{
				break;
			}
			if (oldintbits != offset)
			{
				(*printer)((char*)"%s", (char*)"Bad dictionary change test\r\n");
				myexit("bad dict", 4);
			}
			fread(&oldproperties,1,8,in);
			fread(&oldflags,1,8,in);
			fread(&oldintbits,1,4,in); //old internal
			fread(&oldheader, 1, 1, in); // multiword substitute header
			fread(&oldlength,1,4,in); 
			fread(&oldsubstituteindex, 1, 4, in); // index of word we substitute to
			unsigned int index = 0;
			fread(&index, 1, 4, in);
			if (IsQuery(D) && index)
			{
				char query[1000];
				fread(&query, 1, index, in);
				 // we dont check for change - not allowed
			}
			unsigned char junk;
			fread(&junk,1,1,in); // 0 marker
			WORDP foreignFlags[50];
			if (junk == 76)
			{
				fread(&foreignFlags, languageCount, sizeof(WORDP), in);
				fread(&junk, 1, 1, in); // 0 marker
			}
			if (junk != 77)
			{
				(*printer)((char*)"%s", (char*)"out of dictionary change data2?\r\n"); // multiword header 
				myexit("build fail old data");
			}
			// old word, no changes 
			if (D->foreignFlags && !memcmp(D->foreignFlags,&foreignFlags,languageCount * sizeof(WORDP)))
			{
				changedForeign = true;
			}
			else if (newproperties == oldproperties && newflags == oldflags && newintbits == oldintbits && newlength == oldlength
				&& oldsubstituteindex == newsubstituteindex && newheader == oldheader)
				continue; // unchanged old
		}
		else newlyAdded = true; // word came from this build layer
	
		if (deleted && newlyAdded)
				continue; // newly deleted, dont need to write

		if (IsQuery(D) && D->internalBits & build )
		{
			fprintf(dictout, (char*)"+query `%s` `%s` \r\n", D->word, D->w.userValue); // query defn , not a rename
			continue;
		}
		// scripts may not alter their properties and they are language independent
		// if (*D->word == '~' && !( D->systemFlags & NOCONCEPTLIST) ) continue;		// dont write topic names or concept names, let keywords do that and  no variables
		if ((*D->word == '_' || *D->word == '@' || *D->word == '#' ) && D->internalBits & RENAMED) 
		{
			if (!newlyAdded) continue;	// written out before
		}
		// old word has no characteristics OR facts
		else if (!newlyAdded && (oldproperties == newproperties && oldflags == newflags && oldintbits == newintbits && oldlength == newlength) && 
			oldsubstituteindex == newsubstituteindex && !GetSubjectNondeadHead(D) && !GetVerbNondeadHead(D) && !GetObjectNondeadHead(D)) continue; // ignore pattern words, etc EXCEPT when field of a fact or with different sys or prop

		//not checking words without ~  if the ONLY change is an existing word got made into a concept, dont write it out anymore
		{
			char word[MAX_WORD_SIZE];
			char change = (SUPERCEDED(D)) ? '-' : '^';
			if (multidict && GET_LANGUAGE_INDEX(D))  sprintf(word, (char*)"%c `%s~l%u` ", change, D->word, GET_LANGUAGE_INDEX(D));
			else sprintf(word, (char*)"%c `%s` ", change, D->word);
			if (SUPERCEDED(D)) fprintf(dictout, "`%s`\r\n", word);
			else
			{
				char p1[20];
				char p2[20];
				char s1[20];
				char s2[20];
				char i1[20];
				if (newproperties == oldproperties && !newlyAdded) // inherit from before
				{
					strcpy(p1, "@ ");
					*p2 = 0;
				}
				else if (!newproperties)
				{
					strcpy(p1, "0 ");
					*p2 = 0;

				}
				else
				{
					sprintf(p1, "x%08x ", (unsigned int)(D->properties >> 32));
					sprintf(p2, "%08x    ", (unsigned int)(D->properties & 0x00000000ffffffff));
				}

				if (newflags == oldflags && !newlyAdded) // inherit from before
				{
					strcpy(s1, "@ ");
					*s2 = 0;
				}
				else if (!newflags)
				{
					strcpy(s1, "0 ");
					*s2 = 0;
				}
				else if (newflags == PATTERN_WORD)
				{
					strcpy(s1, "P ");
					*s2 = 0;
				}
				else
				{
					sprintf(s1, "x%08x ", (unsigned int)(D->systemFlags >> 32));
					sprintf(s2, "%08x    ", (unsigned int)(D->systemFlags & 0x00000000ffffffff));
				}
				
				if (newintbits == oldintbits && !newlyAdded) // inherit from before
					strcpy(i1, "@ ");
				else if (!newintbits) strcpy(i1, "0 ");
				else sprintf(i1, "x%08x ", newintbits);

				char parse[40];
				if (D->parseBits == 0) strcpy(parse, "0");
				else sprintf(parse, "x%08x ", D->parseBits);

				char actualsub[MAX_WORD_SIZE];
				strcpy(actualsub, "``");
				if (newsubstituteindex)
				{
					sprintf(actualsub, "`%s`", Index2Word(newsubstituteindex)->word);
				}

				char header[20];
				*header = 0;
				if (newheader)
					sprintf(header, "%u", newheader);

				char bigheader[MAX_WORD_SIZE];
				*bigheader = 0;
				if (changedForeign)
				{
					char* ptr = bigheader;
					strcpy(ptr, "FF ");
					ptr += 3;
					for (unsigned int i = 0; i < languageCount; ++i)
					{
						sprintf(ptr, "%u ",Word2Index(D->foreignFlags[i]));
						ptr += strlen(ptr);
					}
				}

				fprintf(dictout, "%s  %s  %s  %s  %s  %s  %s   %s %s %s\r\n", word, p1, p2, s1, s2, i1,parse, actualsub,header,bigheader);
			}
		}
	}
	fclose(in); // dont use Fclose
    seeAllFacts = false;
}

static void WriteExtendedFacts(FILE* factout,FILE* dictout,FILE* varout, unsigned int build)
{
	if (!factout || !dictout || !varout) return;
	fprintf(dictout, "%d %s\r\n", CHECKSTAMPRAW,language_list); 
	fprintf(factout, "%d %s\r\n", CHECKSTAMPRAW,language_list);
	fprintf(varout, "%d\r\n", CHECKSTAMPRAW);
	seeAllFacts = true;
	
	// save global bot vars to file
	char* buffer = AllocateBuffer();
	bool oldshared = shared;
	shared = false;
	char* ptr = WriteUserVariables(buffer,false,true,NULL);
	shared = oldshared;
	fwrite(buffer,ptr-buffer,1,varout);
	FreeBuffer();

	char buildchar = (build == BUILD0) ? '0' : '1';
	WriteDictionaryChange(dictout,build);
	seeAllFacts = true;
	// we only write out new facts created this level. One cannot change old facts
    if (build == BUILD0) WriteFacts(factout,factsPreBuild[LAYER_0]);
	else if (build == BUILD1) WriteFacts(factout,factsPreBuild[LAYER_1]);
	if (multidict)
	{
		fwrite("--dead\r\n", 8, 1, factout);
		WriteDeadFacts(factout); // word converting to universal may kill old facts.
		fwrite("--lang\r\n", 8, 1, factout);
		WriteLanguageAdjustedFacts(factout); // guy who became universal, his facts may change language to universal
	}
	if (factout) fclose(factout);
    
	seeAllFacts = false;
}

static void DumpErrors()
{
	if (errorIndex) Log(ECHOUSERLOG,(char*)"\r\n ERROR SUMMARY: \r\n");
	for (unsigned int i = 0; i < errorIndex; ++i) Log(ECHOUSERLOG,(char*)"  %s",errors[i]);
}

static void DumpWarnings()
{
	if (warnIndex) Log(USERLOG,"\r\nWARNING SUMMARY: \r\n");
	*currentFilename = 0;
	for (unsigned int i = 0; i < warnIndex; ++i) 
	{
		if (strstr(warnings[i],(char*)"is not a known word")) {}
		else if (strstr(warnings[i],(char*)" changes ")) {}
		else if (strstr(warnings[i],(char*)"is unknown as a word")) {}
		else if (strstr(warnings[i],(char*)"in opposite case")){}
		else if (strstr(warnings[i],(char*)"a function call")){}
        else if (strstr(warnings[i], (char*)"multiple spellings")) {}
        else Log(USERLOG,"  %s\r\n",warnings[i]);
	}
}

static void EmptyVerify(char* name, uint64 junk)
{
	char* x = strstr(name,(char*)"-b");
	if (!x) return;
	char c = (buildID == BUILD0) ? '0' : '1';
	if (x[2] == c) unlink(name);
}

static int CompileCleanup(char* output,uint64 oldtokenControl, unsigned int  build)
{
	if (build == BUILD1)
	{
		for (unsigned int i = 0; i < undefinedFunctionIndex; ++i)
		{
			WORDP D = undefinedFunction[i];
			if (!(D->internalBits & IS_OUTPUT_MACRO))
			{
				WARNSCRIPT("*** Function used in Layer 0 never defined here in Layer 1: %s\r\n", D->word);
			}
		}
	}


	EndScriptCompiler();
	build0Requested = build0Requested = false;
	buildID = 0;
	int resultcode = 0;
	numberOfTopics = 0;
	tokenControl = oldtokenControl;
	currentRuleOutputBase = currentOutputBase = ourMainOutputBuffer;
	compiling = NOT_COMPILING;
	jumpIndex = -1;
	testOutput = output; // allow summary to go out the server
	if (hasErrors)
	{
		EraseTopicFiles(build, baseName);
		DumpErrors();
		if (missingFiles) Log(ECHOUSERLOG, (char*)"%d topic files were missing.\r\n", missingFiles);
		Log(ECHOUSERLOG, (char*)"\r\n%d errors - press Enter to quit. Then fix and try again.\r\n", hasErrors);
		if (!server && !commandLineCompile) ReadALine(readBuffer, stdin);
		resultcode = 4; // error
	}
	else if (hasWarnings)
	{
		DumpWarnings();
		if (missingFiles) Log(USERLOG,"%d topic files were missing.\r\n", missingFiles);
		Log(USERLOG,"%d serious warnings, %d function warnings, %d spelling warnings, %d case warnings, %d substitution warnings\r\n    ", hasWarnings - badword - substitutes - cases, functionCall, badword, cases, substitutes);
	}
	else
	{
		if (missingFiles) Log(ECHOUSERLOG, (char*)"%d topic files were missing.\r\n", missingFiles);
		Log(ECHOUSERLOG, (char*)"No errors or warnings\r\n\r\n");
	}
	Log(ECHOUSERLOG, (char*)"\r\n\r\nFinished compile\r\n\r\n");

	Rebegin(0, argc, argv); // reload dict and layers as needed

	echo = true;
	if (userlogFile)
	{
		fclose(userlogFile);
		userlogFile = NULL;
	}
	return resultcode;
}

void InitBuild(unsigned int build)
{
	conditionalCompiledIndex = 0;
	build0jid = 0;
	build1jid = 0;
	buildtransientjid = 0;
	languageadjustedfactsList = 0;
	myBot = 0;	// default
	deadfactsList = userVariableThreadList = kernelVariableThreadList = botVariableThreadList = NULL;
	buildId = build;
	*scopeBotName = 0;

	ClearWordMaps(); // :build
	char filex[200];
	sprintf(filex, "%s/missingSets.txt", topicfolder);
	remove(filex); // precautionary
	sprintf(filex, "%s/missingLabel.txt", topicfolder);
	remove(filex);

	if (buildId == BUILD0) 	EraseTopicBin(BUILD1, "1"); // rebuilding underneath layer 1

	MakeDirectory((char*)"TOPIC");
	if (buildId == BUILD0) MakeDirectory((char*)"TOPIC/BUILD0");
	else if (buildId == BUILD1) MakeDirectory((char*)"TOPIC/BUILD1");

}

int ReadTopicFiles(char* name,unsigned int build,int spell)
{
	currentBuild = build;
	propertyRedefines = NULL;
	flagsRedefines = NULL;
    char filename[SMALL_WORD_SIZE];
    nospellcheck = false;
	isDescribe = false;
	*scopeBotName = 0;
	myBot = 0;
	globalBotScope = false;
	if (build == BUILD1) 
	{
		strcpy(baseName,(char*)"1");
		char dir[200];
		sprintf(dir,"%s/BUILD1", topicfolder);
		MakeDirectory(dir);
	}
	else 
	{
		char dir[200];
		sprintf(dir,"%s/BUILD0", topicfolder);
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

	EraseTopicFiles(build,baseName);
	char file[SMALL_WORD_SIZE];
	sprintf(file,(char*)"%s/missingLabel.txt", topicfolder);
	remove(file);
	sprintf(file,(char*)"%s/missingSets.txt", topicfolder);
	remove(file);

	WalkDirectory((char*)"VERIFY",EmptyVerify,0,false); // clear verification of this level

	compiling = FULL_COMPILE;
	errorIndex = warnIndex = hasWarnings = hasErrors =  0;
	substitutes = cases = functionCall = badword = 0;

	sprintf(filename,(char*)"%s/BUILD%s/map%s.txt", topicfolder,baseName,baseName);
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

    sprintf(filename,(char*)"%s/BUILD%s/map%s.json", topicfolder,baseName,baseName);
    mapFileJson = FopenBinaryWrite(filename); // no UTF if want to be parsed by jsonopen
    fprintf(mapFileJson, "[\r\n");
    mapTopicFileCount = 0;
    mapItemCount = 0;
    mapRuleCount = 0;

	// init the script output file
	sprintf(filename,(char*)"%s/BUILD%s/script%s.txt", topicfolder,baseName,baseName);
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
	StartScriptCompiler(true); // read topic files?
	jumpIndex = -1;
	if (setjmp(scriptJump[++jumpIndex]))  // base of a compile is 0
	{
		return CompileCleanup(output,oldtokenControl, build);
	}

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
			Log(USERLOG,"\r\n>>Reading folder %s\r\n",word);
            bool recurse = word[len - 2] == '/';
            if (recurse) word[len - 2] = 0;
            WalkDirectory(word,ReadTopicFile,build,recurse); // read all files in folder (top level)
			Log(USERLOG,"\r\n<<end folder %s\r\n",word);
		}
		else if (*word == ':' && word[1]) DoCommand(readBuffer,output); // testing command
		else if (!stricmp(word, (char*)"language:"))
		{
			at = ReadNextSystemToken(in, at, word, false);
			if (multidict)
			{
				SetLanguage(word);
				Log(USERLOG, "\r\n>>Setting language %s\r\n", word);
			}
		}
		else ReadTopicFile(word,build|FROM_FILE); // was explicitly named
	}
	if (in) fclose(in); 
	fclose(mapFile); 
    fprintf(mapFileJson, "]\r\n");
    fclose(mapFileJson);
	StartFile((char*)"Post compilation Verification");
    nospellcheck = false;

	SetLanguage("UNIVERSAL"); // see all dict and fact values

	// verify errors across all files
	DoubleCheckSetOrTopic();	//   prove all sets/topics he used were defined
	DoubleCheckReuse();		// see if jump labels are defined
	DoubleCheckDefinition(build,topicfolder, baseName);
	*currentFilename = 0;
	if (*duplicateTopicName)  WARNSCRIPT((char*)"At least one duplicate topic name, i.e., %s, which may intended if bot restrictions differ or concept collision.\r\n",duplicateTopicName)
	WalkDictionary(ClearBeenHere,0);

	// write out compiled data

	//   write how many topics were found (for when we preload during normal startups)
	sprintf(filename,(char*)"%s/BUILD%s/script%s.txt", topicfolder,baseName,baseName);
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
		sprintf(filename,(char*)"%s/BUILD%s/plans%s.txt", topicfolder,baseName,baseName);
		out = FopenUTF8WriteAppend(filename,(char*)"rb+");
		if (out)
		{
			char word[MAX_WORD_SIZE];
			fseek(out,0,SEEK_SET);
			sprintf(word,(char*)"%05u",hasPlans);
			fwrite(word,1,5 * sizeof(char),out);
			fclose(out); // dont use FClose
		}
	}

	// we delay writing out keywords til now, allowing multiple accumulation across tables and concepts
	WalkDictionary(WriteConcepts, build);
	WalkDictionary(ClearBeenHere, 0);

	// dump variables, dictionary changes, topic facts
	sprintf(filename,(char*)"%s/BUILD%s/facts%s.txt", topicfolder,baseName,baseName);
	char filename1[MAX_WORD_SIZE];
	sprintf(filename1,(char*)"%s/BUILD%s/dict%s.txt", topicfolder,baseName,baseName);
	char filename2[MAX_WORD_SIZE];
	sprintf(filename2, (char*)"%s/BUILD%s/variables%s.txt", topicfolder, baseName, baseName);
	FILE* dictout = FopenUTF8Write(filename1);
	FILE* factout = FopenUTF8Write(filename);
	FILE* varout = FopenUTF8Write(filename2);
	WriteExtendedFacts(factout,dictout, varout, build);
    fclose(dictout); // dont use FClose
	fclose(varout); // dont use FClose
	// FClose(factout); closed from within writeextendedfacts
	FreeOutputBuffer();

	return CompileCleanup(output,oldtokenControl, build);
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

#endif
