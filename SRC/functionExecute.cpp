#include "common.h"

#ifdef INFORMATION

Function calls all run through DoFunction().

A function call can either be to a system routine or a user routine.

User routines are like C macros, executed in the context of the caller, so the argument
are never evaluated prior to the call. If you evaluated an argument during the mustering,
you could get bad answers.Consider:
One has a function : ^ foo(^ arg1^ arg2) ^ arg2^ arg1
And one has a call^ foo(($val = 1) $val)
This SHOULD look like inline code : $val  $val = 1
But evaluation at argument time would alter the value of $val and pass THAT as ^ arg2.Wrong.

System routines are proper functions, whose callArgumentList may or may not be evaluated.
The description of a system routine tells
how many callArgumentList it expectsand in what way.Routines that set variables always pass
that designator as the first(unevaluated) argumentand all the rest are evaluated callArgumentList.

The following argument passing is supported
1. Evaluated - each argument is evaluated and stored(except for a storage argument).
If the routine takes optional callArgumentList these are already also evaluatedand stored,
and the argument after the last actual argument is a null string.
2. STREAM_ARG - the entire argument stream is passed unevaled as a single argument,
allowing the routine to handle processing them itself.

All calls have a context of "executingBase" which is the start of the rule causing this
evaluation. All calls are passed a "buffer" which is spot in the currentOutputBase it
should write any answers.

Anytime a single argument is expected, one can pass a whole slew of them by making
them into a stream, encasing them with().The parens will be strippedand the
entire mess passed unevaluated.This makes it analogous to STREAM_ARG, but the latter
requires no excess parens to delimit it.

All calls create a CALLFRAME. These contain, among other data, the argument values being
passed to the function. The variable currentCallFrame is always the one for the currently executing
function and has currentCallFrame->arguments array of the arguments (1-based). 

User functions have named arguments, either $_ local variables or ^xxx read-write variables.
Function variables in a function signature like ^myfunc(^arg1 ^arg2 $_arg3) are compiled into 
indexed references like ^1 and ^2 into the frame's argument list. Those references refer to the 
arguments in the call frame of the user function even though other system functions may have been 
invoked underneath. Hense all function variable references translate using the frame pointed to by
currentFNVarFrame.

Memory management :
Incoming variables to functions have their data copied into stack space pointed at by the call frame's
argument list. This memory frees automaticall when the call frame is popped on function completion.
'
Regular heap space is used to hold permanent data(e.g.variable assignment for non - local variables.

#endif
#define ENCODE_MARKER 123
#define END_TEXT_CHUNK '`'
#define END_TEXT_CHUNK_STRING "``"

#define SIZELIM 200
#define MAX_TOPIC_KEYS 5000
#define PLANMARK -1
#define RULEMARK -2

#define MAX_TEST_PATTERN 10000
#define NO_WINNER 10000

static char testoutputbacktrace[MAX_WORD_SIZE];
static int testoutputbacktracecount = 0;

static char* nullArguments[MAX_ARG_LIMIT + 1] =
{ "","","","","","","","",   "","","","","","","","",  "","","","","","","","",  "","","","","","","","" };

APICall csapicall = NO_API_CALL;
HEAPREF patternwordthread = NULL;
char* fnOutput = NULL;
static bool changedNL = false;
char* rawtestpatterninput = NULL;
bool directAnalyze = false;
static char lognames[MAX_LOG_NAMES][200];
static char* codeStart = NULL;
char* realCode = NULL;
static char testoutputFail[200];
int rulesExecuted = 0;
HEAPREF memoryMarkThreadList = NULL;
HEAPREF memoryVariableThreadList = NULL;
HEAPREF memoryVariableChangesThreadList = NULL;
bool planning = false;
int debugValue = 0;
char* testpatterninput = NULL;
char testpatternlabel[100];

char* style = NULL;
#define MAX_REUSE_SAFETY 100
static int reuseIndex = 0;
char* traceTestPatternBuffer = NULL; // where api traces get stored
int testPatternIndex = -1;
static char* traceBase = NULL;
static int traceIndex = 0;
static char* reuseSafety[MAX_REUSE_SAFETY + 1];
static int reuseSafetyCount[MAX_REUSE_SAFETY + 1];
static char* months[] = { (char*)"January",(char*)"February",(char*)"March",(char*)"April",(char*)"May",(char*)"June",(char*)"July",(char*)"August",(char*)"September",(char*)"October",(char*)"November",(char*)"December" };
static char* days[] = { (char*)"Sunday",(char*)"Monday",(char*)"Tuesday",(char*)"Wednesday",(char*)"Thursday",(char*)"Friday",(char*)"Saturday" };
long http_response = 0;
char lastcurltime[100] = { '\0' };
int globalDepth = 0;
int maxGlobalSeen = 0;
char* stringPlanBase = 0;
char* backtrackPoint = 0;		// plan code backtrace data
unsigned int currentIterator = 0;		// next value of iterator
HEAPREF savedSentencesThreadList = NULL;

bool softRestart = false;
bool backtrackable = false;
char* lastInputSubstitution = NULL;
TestMode wasCommand; // special result passed back from some commands to control chatscript

static unsigned int spellSet;			// place to store word-facts on words spelled per a pattern
int tracepatterndata = 0;
static unsigned int internalConceptIndex = 0;
static int rhymeSet;
CALLFRAME* currentFNVarFrame = NULL; // argument list access for interpreting ^0
CALLFRAME* currentCallFrame = NULL; // argument list access for current function

char* currentPlanBuffer;


#include <map>
using namespace std;
extern std::map <WORDP, HEAPINDEX> triedData; // per volley index into heap space
typedef std::map<WORDP, double> wordScore_t;

//////////////////////////////////////////////////////////
/// BASIC FUNCTION CODE
//////////////////////////////////////////////////////////
unsigned int callindex = 0;

CALLFRAME* ChangeDepth(int value, const char* name, bool nostackCutback, char* code)
{
	if (value == 0) // secondary call from userfunction. frame is now valid
	{
		CALLFRAME* frame = frameList[globalDepth];
		if (showDepth) Log(USERLOG, "same depth %u %s \r\n", globalDepth, name);
		if (name) frame->label = (char*)name; // override
		if (code) frame->code = code;
#ifndef DISCARDTESTING
		if (debugCall) (*debugCall)(frame->label, true);
#endif
		return frame;
	}
	else if (value < 0) // leaving depth
	{
		bool abort = false;
		CALLFRAME* frame = frameList[globalDepth];
		frameList[globalDepth] = NULL;

#ifndef DISCARDTESTING
		if (globalDepth && *name && debugCall)
		{
			char* result = (*debugCall)((char*)name, false);
			if (result) abort = true;
		}
#endif
		if (frame->memindex != bufferIndex)
		{
			ReportBug((char*)"INFO: depth %d not closing bufferindex correctly at %s bufferindex now %d was %d\r\n", globalDepth, name, bufferIndex, frame->memindex);
			bufferIndex = frame->memindex; // recover release
		}
		frame->name = NULL;
		frame->rule = NULL;
		frame->label = NULL;
		currentRuleID = frame->oldRuleID;
		currentTopicID = frame->oldTopic;
		currentRuleTopic = frame->oldRuleTopic;
		currentRule = frame->oldRule;
		currentFNVarFrame = frame->currentFNVarFrame;
		currentCallFrame = frame->currentCallFrame;
		// engine functions that are streams should not destroy potential local adjustments
		if (!nostackCutback)
			stackFree = (char*)frame; // deallocoate ARGUMENT space
		if (showDepth) Log(USERLOG, "-depth %d %s bufferindex %d heapused:%d\r\n", globalDepth, name, bufferIndex, (int)(heapBase - heapFree));
		globalDepth += value;
		if (globalDepth < 0) { ReportBug((char*)"bad global depth in %s", name); globalDepth = 0; }
		return (abort) ? (CALLFRAME*)1 : NULL; // abort code
	}
	else // value > 0
	{
		++callindex; 
		stackFree = (char*)(((uint64)stackFree + 7) & 0xFFFFFFFFFFFFFFF8ULL);
		CALLFRAME* frame = (CALLFRAME*)stackFree;
		stackFree += sizeof(CALLFRAME);
		memset(frame, 0, sizeof(CALLFRAME));
		
		frame->label = (char*)name;
		frame->index = callindex; // debugging
		frame->code = code;
		frame->heapstart = heapFree;
		frame->rule = (currentRule) ? currentRule : (char*)"";
		memcpy(&frame->arguments, nullArguments, sizeof(nullArguments));
		frame->outputlevel = outputlevel + 1;
		frame->oldRuleID = currentRuleID;
		frame->oldTopic = currentTopicID;
		frame->oldRuleTopic = currentRuleTopic;
		frame->currentFNVarFrame = currentFNVarFrame; // inherit to pass along
		frame->currentCallFrame = currentCallFrame;
		currentCallFrame = frame;
		frame->oldRule = currentRule;
		frame->memindex = bufferIndex;
		frame->heapDepth = heapBase - heapFree;

		globalDepth += value;

		if (showDepth) Log(USERLOG, "+depth %d %s bufferindex %d heapused: %d stackused:%d gap:%d\r\n", globalDepth, name, bufferIndex, (int)(heapBase - heapFree), (int)(stackFree - stackStart), (int)(heapFree - stackFree));
		frameList[globalDepth] = frame; // define argument start space - release back to here on exit
#ifndef DISCARDTESTING
		if (globalDepth && *name != '*' && debugCall) (*debugCall)((char*)name, true);
#endif
		if (globalDepth >= (MAX_GLOBAL - 1)) ReportBug((char*)"FATAL: globaldepth too deep at %s\r\n", name);
		if (globalDepth > maxGlobalSeen) maxGlobalSeen = globalDepth;
		return frame;
	}
}

FunctionResult SetLanguageCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	MakeUpperCase(arg1);
	return SetLanguage(arg1) ? NOPROBLEM_BIT : FAILRULE_BIT;
}

void InitFunctionSystem() // register all functions
{
	unsigned int k = 0;
	SystemFunctionInfo* fn;
	while ((fn = &systemFunctionSet[++k]) && fn->word)
	{
		if (*fn->word == '^') // not a header
		{
			WORDP D = StoreWord((char*)fn->word, 0);
			AddInternalFlag(D, FUNCTION_NAME);
			D->x.codeIndex = (unsigned short)k;
		}
	}

	for (unsigned int i = 0; i < MAX_LOG_NAMES; ++i) // for ^log function
	{
		lognames[i][0] = 0;
		logfiles[i] = NULL;
	}

	oldunmarked[255] = 0;	// global unmarking has nothing for ^marking functions
}

unsigned char* GetDefinition(WORDP D)
{
	unsigned char* defn = D->w.fndefinition; // raw definition w link
	if (!defn) return NULL;
	return defn + 4;	 // skip link field, leaves botid, flags , count
}

unsigned int MACRO_ARGUMENT_COUNT(unsigned char* defn) // 0e(
{// botid flags count ( ...
	if (!defn || !*defn) return 0;
	while (IsDigit(*++defn)) { ; } // skip space then botid 
	while (IsDigit(*++defn)) { ; } // skip macro arguments descriptor
	unsigned char c = (unsigned char)*++defn;
	return (c >= 'a') ? (c - 'a' + 15) : (c - 'A');
}

char* InitDisplay(char* list)
{
	char word[MAX_WORD_SIZE];
	list += 2;	// skip ( and space
	while (1)
	{
		list = ReadCompiledWord((char*)list, word);
		if (*word == ')') break;  // end of display is signaled by )
		if (*word == USERVAR_PREFIX)
		{
			//    printf("saving %s for %d %s\r\n", word, globalDepth, GetCallFrame(globalDepth)->label);
			if (!AllocateStackSlot(word)) return 0;
		}
	}
	return list;
}

void RestoreDisplay(char** base, char* list)
{
	char word[MAX_WORD_SIZE];
	list += 2;	// skip ( and space
	char** slot = base; // display table starts here
	while (1)
	{
		list = ReadCompiledWord(list, word);
		if (*word == ')') break;
		if (*word == USERVAR_PREFIX)
		{
			// printf("restoring %s for %d %s\r\n", word, globalDepth, GetCallFrame(globalDepth)->label);
			slot = RestoreStackSlot(word, slot);
		}
	}
}

#ifdef WIN32
#define MAKEWORDX(a, b)      ((unsigned short)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((unsigned short)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
FunctionResult InitWinsock()
{
	static bool first = true;
	if (first) // prevent DB close from closing WSAStartup to improve performance
	{
		first = false;
		WSADATA wsaData;
		unsigned short wVersionRequested = MAKEWORDX(2, 0);              //   Request WinSock v2.0
		if (WSAStartup(wVersionRequested, &wsaData) != 0)
		{
			if (trace & TRACE_SQL && CheckTopicTrace()) Log(USERLOG, "WSAStartup failed\r\n");
			return FAILRULE_BIT;
		}
	}
	return NOPROBLEM_BIT;
}
#endif


static char* GetPossibleFunctionArgument(char* arg, char* word)
{
	char* ptr = ReadCompiledWord(arg, word);
	if (*word == '^' && IsDigit(word[1]))
	{
		char* value = FNVAR(word);
		if (*value == LCLVARDATA_PREFIX && value[1] == LCLVARDATA_PREFIX)
			value += 2; // already evaled data  -- but bug remains
		strcpy(word, value);
	}
	// this gets us what was passed by name, $xxx.hi will be a json ref.
	return ptr;
}

FunctionResult JavascriptArgEval(unsigned int index, char* buffer)
{
	FunctionResult result;
	char argNum[10];
	sprintf(argNum, "^%u", index+1);
	char* arg = FNVAR(argNum);
	GetCommandArg(arg, buffer, result, OUTPUT_UNTOUCHEDSTRING);
	return result;
}

char* SaveBacktrack(int id)  // --defunct call
{
	// save: id, oldbacktrack point, currentfact, current dict,    HEAPINDEX?  
	char* mark = AllocateHeap(NULL, 4, sizeof(int), false);
	if (!mark) return NULL;
	int* i = (int*)mark;
	i[0] = id;										// 1st int is a backtrack label - plan (-1) or rule (other)
	i[1] = (int)(stringPlanBase - backtrackPoint);	// 2nd is old backtrack point value
	i[2] = Fact2Index(lastFactUsed);					// 4th is fact base 
	i[3] = Word2Index(dictionaryFree);				// 5th is word base (this entry is NOT used)
	return backtrackPoint = mark;
}

unsigned char* FindAppropriateDefinition(WORDP D, FunctionResult & result, bool findright)
{
	int64 botid;
	unsigned char* defn = D->w.fndefinition; // raw definition w link
	if (!defn) return NULL;

	unsigned char* allaccess = NULL;
	unsigned int link = (defn[0] << 24) + (defn[1] << 16) + (defn[2] << 8) + (defn[3]);
	defn += 4; // skip jump 
	defn = (unsigned char*)ReadInt64((char*)defn, botid);
	if (botid == 0) allaccess = defn; // default access 
	while (!(myBot & botid) && (findright || !compiling) && link) // wrong bot, but have link to another bots can share code
	{
		if (!botid) allaccess = defn;
		defn = (unsigned char*)Index2Heap(link); // next defn
		link = (defn[0] << 24) + (defn[1] << 16) + (defn[2] << 8) + (defn[3]);
		defn += 4; // skip jump
		defn = (unsigned char*)ReadInt64((char*)defn, botid);
	}
	if (!botid && !myBot) { ; } // matches
	else if (compiling && !findright) { ; } // during compilation most recent always matches
	else if (!(myBot & botid)) defn = allaccess;
	result = (!defn) ? FAILRULE_BIT : NOPROBLEM_BIT;
	return defn; // point at (
}

int GetFnArgCount(char* func, int& flags)
{
	int args = 0;
	if (*func == '^') // how many arguments does function accept
	{
		size_t len = strlen(func);
		if (func[len - 1] == ' ') func[--len] = 0;  // trailing blank, remove
		WORDP D = FindWord(func, len, LOWERCASE_LOOKUP);
		if (D && D->internalBits & FUNCTION_NAME)
		{
			FunctionResult fnresult;
			char* definition = (char*)FindAppropriateDefinition(D, fnresult);
			if (definition)
			{
				definition = ReadInt((char*)definition, flags);
				unsigned char c = (unsigned char)*definition;
				args = (c >= 'a') ? (c - 'a' + 15) : (c - 'A'); // expected args, leaving us at ( args ...
			}
		}
	}
	return args;
}

static char* FlushMark() // throw away this backtrack point, maybe reclaim its heap space
{
	if (!backtrackPoint) return NULL;
	// we are keeping facts and variable changes, so we cannot reassign the string free space back because it may be in use.
	if (backtrackPoint == heapFree) ResetHeapFree(backtrackPoint + (4 * sizeof(int)));
	int* i = (int*)backtrackPoint;
	return backtrackPoint = stringPlanBase - i[1];
}

static void RestoreMark()
{	// undo all changes
	if (!backtrackPoint) return;
	int* i = ((int*)backtrackPoint); // skip id

	// revert facts
	FACT* oldF = Index2Fact(i[2]);
	while (lastFactUsed > oldF) FreeFact(lastFactUsed--); // undo facts to start
	// revert dict entries
	WORDP oldD = Index2Word(i[3]);

	// trim dead facts at ends of sets
	for (unsigned int store = 0; store <= MAX_FIND_SETS; ++store)
	{
		unsigned int count = FACTSET_COUNT(store) + 1;
		while (--count >= 1)
		{
			if (!(factSet[store][count]->flags & FACTDEAD)) break; // stop having found a live fact
		}
		if (count) SET_FACTSET_COUNT(store, count); // new end
	}
	DictionaryRelease(oldD, backtrackPoint);
	backtrackPoint = stringPlanBase - i[1];
}

static void StoreCompileErrors(MEANING M)
{
	if (errorIndex)
	{
		MEANING M1 = GetUniqueJsonComposite((char*)"ja-", FACTTRANSIENT);
		WORDP field = StoreWord("errors", AS_IS);
		unsigned int flags = JSON_OBJECT_FACT | FACTTRANSIENT | JSON_ARRAY_VALUE;
		CreateFact(M, MakeMeaning(field), M1, flags);
		flags = JSON_ARRAY_FACT | FACTTRANSIENT | JSON_STRING_VALUE;
		for (unsigned int i = 0; i < errorIndex; ++i)
		{
			char index[MAX_WORD_SIZE];
			sprintf(index, "%u", i);
			field = StoreWord(index, AS_IS);
			char* errmsg = errors[i];
			while (*errmsg == '.' || *errmsg == ',') ++errmsg; // skip tabbing data
			size_t len = strlen(errmsg);
			while (errmsg[len - 1] == '\n' || errmsg[len - 1] == '\r') errmsg[--len] = 0;
			WORDP err = StoreWord(errmsg, AS_IS);
			CreateFact(M1, MakeMeaning(field), MakeMeaning(err), flags);
		}
	}
}

void RefreshMark()
{	// undo all changes but leave rule mark in place
	if (!backtrackPoint) return;
	int* i = (int*)backtrackPoint; // point past id, backtrack 

	// revert facts
	FACT* oldF = Index2Fact(i[2]);
	while (lastFactUsed > oldF) FreeFact(lastFactUsed--); // undo facts to start
	// revert dict entries
	WORDP oldD = Index2Word(i[3]);

	// trim dead facts at ends of sets
	for (unsigned int store = 0; store <= MAX_FIND_SETS; ++store)
	{
		unsigned int count = FACTSET_COUNT(store) + 1;
		while (--count >= 1)
		{
			if (!(factSet[store][count]->flags & FACTDEAD)) break; // stop having found a live fact
		}
		if (count) SET_FACTSET_COUNT(store, count); // new end
	}

	DictionaryRelease(oldD, backtrackPoint);
}

static void UpdatePlanBuffer()
{
	size_t len = strlen(currentPlanBuffer);
	if (len) // we have output, prep next output
	{
		currentPlanBuffer += len;	// cumulative output into buffer
		*++currentPlanBuffer = ' '; // add a space
		currentPlanBuffer[1] = 0;
	}
}

static int WildStartPosition(char* arg)
{
	int x = GetWildcardID(arg);
	if (x == ILLEGAL_MATCHVARIABLE) return ILLEGAL_MATCHVARIABLE;
	unsigned int n = WILDCARD_START(wildcardPosition[x]);
	if (n == 0 || n > wordCount) n = atoi(wildcardCanonicalText[x]);
	if (n == 0 || n > wordCount) n = 1;
	return n;
}

static int WildEndPosition(char* arg)
{
	int x = GetWildcardID(arg);
	if (x == ILLEGAL_MATCHVARIABLE) return ILLEGAL_MATCHVARIABLE;
	unsigned int n = WILDCARD_END_ONLY(wildcardPosition[x]);
	if (n == 0 || n > wordCount) n = atoi(wildcardCanonicalText[x]);
	if (n == 0 || n > wordCount) n = 1;
	return n;
}

static FunctionResult PlanCode(WORDP plan, char* buffer)
{  // failing to find a responder is not failure.
#ifdef INFORMATION

	A plan sets a recover point for backtrackingand clears it one way or another when it exits.
		A rule sets a backpoint only if it finds some place to backtrack.The rule will clear that point one way or another when it finishes.

		Undoable changes to variables are handled by creating special facts.
#endif

		if (trace & (TRACE_MATCH | TRACE_PATTERN) && CheckTopicTrace()) Log(USERLOG, "\r\n\r\nPlan: %s ", plan->word);
	WORDP originalPlan = plan;
	bool oldplan = planning;
	bool oldbacktrackable = backtrackable;
	char* oldbacktrackPoint = backtrackPoint;
	char* oldStringPlanBase = stringPlanBase;
	stringPlanBase = heapFree;
	backtrackPoint = heapFree;
	backtrackable = false;
	unsigned int oldWithinLoop = withinLoop;
	withinLoop = 0;
	planning = true;
	int holdd = globalDepth;
	char* oldCurrentPlanBuffer = currentPlanBuffer;

	unsigned int tindex = topicIndex;
	FunctionResult result = NOPROBLEM_BIT;

	ChangeDepth(1, originalPlan->word); // plancode

	// where future plans will increment naming
	char name[MAX_WORD_SIZE];
	strcpy(name, plan->word);
	char* end = name + WORDLENGTH(plan);
	*end = '.';
	*++end = 0;

	unsigned int n = 0;
	while (result == NOPROBLEM_BIT) // loop on plans to use
	{
		*buffer = 0;
		currentPlanBuffer = buffer;	  // where we are in buffer writing across rules of a plan
		int topicid = plan->x.topicIndex;
		if (!topicid)
		{
			result = FAILRULE_BIT;
			break;
		}
		int pushed = PushTopic(topicid);  // sets currentTopicID
		if (pushed < 0)
		{
			result = FAILRULE_BIT;
			break;
		}
		char* xxplanMark = SaveBacktrack(PLANMARK); // base of changes the plan has made  -- defunct call
		char* base = GetTopicData(topicid);
		int ruleID = 0;
		currentRuleTopic = currentTopicID;
		currentRule = base;
		currentRuleID = ruleID;
		char* ruleMark = NULL;

		bool fail = false;

		CALLFRAME* frame = ChangeDepth(1, GetTopicName(currentTopicID)); // plancode
		char* locals = GetTopicLocals(currentTopicID);
		bool updateDisplay = locals && DifferentTopicContext(-1, currentTopicID);
		frame->display = (char**)stackFree;
		if (updateDisplay && !InitDisplay(locals)) fail = true;

		while (!fail && base && *base) //   loop on rules of topic
		{
			currentRule = base;
			ruleMark = SaveBacktrack(RULEMARK); // allows rule to be completely undone if it fails  -- defunct call
			backtrackable = false;
			result = TestRule(ruleID, base, currentPlanBuffer); // do rule at base
			if (!result || (result & ENDTOPIC_BIT)) // rule didnt fail
			{
				UpdatePlanBuffer();	// keep any results
				if (result & ENDTOPIC_BIT) break; // no more rules are needed
			}
			else if (backtrackable)  // rule failed 
			{
				while (backtrackable)
				{
					if (trace & (TRACE_MATCH | TRACE_PATTERN) && CheckTopicTrace()) Log(USERLOG, "Backtrack \r\n");
					*currentPlanBuffer = 0;
					RefreshMark(); // undo all of rule, but leave undo marker in place
					backtrackable = false;
					result = DoOutput(currentPlanBuffer, currentRule, currentRuleID); // redo the rule per normal
					if (!result || result & ENDTOPIC_BIT) break; // rule didnt fail
				}
				if (result & ENDTOPIC_BIT) break; // rule succeeded eventually
			}
			FlushMark();  // cannot revert changes after this
			base = FindNextRule(NEXTTOPLEVEL, base, ruleID);
		}

		if (updateDisplay) RestoreDisplay(frame->display, locals);
		ChangeDepth(-1, frame->label); // plan

		if (backtrackPoint == ruleMark) FlushMark(); // discard rule undo
		if (trace & (TRACE_MATCH | TRACE_PATTERN) && CheckTopicTrace())
		{
			char* xname = GetTopicName(currentTopicID);
			if (*xname == '^') Log(USERLOG, "Result: %s Plan: %s \r\n", ResultCode(result), name);
			else Log(USERLOG, "Result: %s Topic: %s \r\n", ResultCode(result), xname);
		}
		if (pushed) PopTopic();
		if (result & ENDTOPIC_BIT)
		{
			FlushMark(); // drop our access to this space, we are as done as we can get on this rule
			break;	// we SUCCEEDED, the plan is done
		}
		//   flush any deeper stack back to spot we started
		if (result & FAILCODES) topicIndex = tindex;
		//   or remove topics we matched on so we become the new master path
		RestoreMark(); // undo failed plan
		sprintf(end, (char*)"%u", ++n);
		plan = FindWord(name);
		result = (!plan) ? FAILRULE_BIT : NOPROBLEM_BIT;
		if (!result && trace & (TRACE_MATCH | TRACE_PATTERN) && CheckTopicTrace()) Log(USERLOG, "NextPlan %s\r\n", name);
	}

	if (globalDepth != holdd) ReportBug((char*)"PlanCode didn't balance");
	ChangeDepth(-1, originalPlan->word); // plancode

	if (*currentPlanBuffer == ' ') *currentPlanBuffer = 0; // remove trailing space

	// revert to callers environment
	planning = oldplan;
	currentPlanBuffer = oldCurrentPlanBuffer;
	withinLoop = oldWithinLoop;
	backtrackable = oldbacktrackable;
	stringPlanBase = oldStringPlanBase;
	backtrackPoint = oldbacktrackPoint;
	result = (FunctionResult)(result & (-1 ^ (ENDTOPIC_BIT | ENDRULE_BIT)));
	return result; // these are swallowed
}

static char* SystemCall(char* buffer, char* ptr, CALLFRAME * frame, FunctionResult & result, bool& streamArg)
{
	WORDP D = frame->name;
	bool showtrace = ((trace & (TRACE_OUTPUT | TRACE_USERFN)) || (D->internalBits & MACRO_TRACE)) && !(D->internalBits & NOTRACE_FN) && CheckTopicTrace();
	if (showtrace) 	Log(USERLOG, "%s(", D->word);
	unsigned int nArguments = 0;
	int callArgumentIndex = 0;
	char* paren = ptr;
	ptr = SkipWhitespace(ptr + 1); // aim to next major thing after ( 
	SystemFunctionInfo* info = &systemFunctionSet[D->x.codeIndex];
	char* start = ptr;
	int flags = 0x00000100;	// do we leave this unevaled?
	while (ptr && *ptr != ')' && *ptr != ENDUNIT) // read arguments
	{
		char word[MAX_WORD_SIZE];
		*word = 0;
		if (info->argumentCount != STREAM_ARG) // break them up
		{
			if (info->argumentCount == UNEVALED)
			{
				ptr = ReadCompiledWordOrCall(ptr, buffer);
				strcpy(word, buffer);
			}
			// unevaled counted arg
			else if (info->argumentCount != VARIABLE_ARG_COUNT && info->argumentCount & (flags << nArguments))
			{
				ptr = ReadCompiledWord(ptr, buffer);
				strcpy(word, buffer);
			}
			else // VARIABLE ARG OR COUNTED ARG
			{
				ReadCompiledWord(ptr, word);
				ptr = GetCommandArg(ptr, buffer, result, OUTPUT_UNTOUCHEDSTRING);
			}
			frame->arguments[++callArgumentIndex] = AllocateStack(buffer);
			ptr = SkipWhitespace(ptr);
			if (frame->arguments[callArgumentIndex] && frame->arguments[callArgumentIndex][0] == USERVAR_PREFIX &&
				strstr(frame->arguments[callArgumentIndex], "$_")) // NOT by reference but by value somewhere in the chain
			{
				frame->arguments[callArgumentIndex] = AllocateStack(GetUserVariable(frame->arguments[callArgumentIndex], false)); // pass by unmarked value - no one will try to store thru it
			}
			if (showtrace)
			{
				if (!strcmp(word, frame->arguments[callArgumentIndex]))
				{
					if (nArguments) Log(USERLOG, " %s", word);
					else Log(USERLOG, "%s", word);
				}
				else
				{
					if (nArguments) Log(USERLOG, " %s`%s`", word, frame->arguments[callArgumentIndex]);
					else Log(USERLOG, "%s`%s`", word, frame->arguments[callArgumentIndex]);
				}
			}
			++nArguments;
			if (result != NOPROBLEM_BIT)
			{
				if (showtrace) Log(USERLOG, "FAILED argument\r\n");
				return ptr;  // arg failed
			}
		}
		else // swallow unevaled arg stream
		{
			ptr = BalanceParen(paren, false, false);  // start after (, point after closing ) if one can, to next token - it may point 2 after )  or it may point 1 after )
			char* beforeendparen = ptr--; // higher level will move past this closing paren
			while (*--beforeendparen != ')') { ; } // back up to closing
			size_t len = beforeendparen - start; // length of argument bytes not including paren, and end up after paren
			if (len >= maxBufferSize) // bad pointer from balance paren
			{
				ReportBug("BalanceParen failed");
				result = FAILRULE_BIT;
				return ptr;
			}
			while (start[len - 1] == ' ') --len; // dont want trailing blanks
			frame->arguments[++callArgumentIndex] = AllocateStack(start, len);
			streamArg = true;
			if (showtrace)
			{
				if (len < 100)
				{
					char* end = strchr(start, ')');
					if (end) *end = 0;
					Log(USERLOG, start);
					if (end) *end = ')';
				}
				else Log(USERLOG, "...");
			}
		}
		if (!frame->arguments[callArgumentIndex])
		{ // only need to check once, at end of attempting it
			ReportBug("FATAL: Stack space exhausted %s", D->word);
			result = FAILRULE_BIT;
			return ptr;
		}
		if (info->argumentCount == STREAM_ARG) break; // end of arguments
		ptr = SkipWhitespace(ptr);
	}
	frame->n_arguments = callArgumentIndex;

	if (showtrace)
	{
		if (info->properties != SAMELINE) Log(USERLOG, ")\r\n");
		else Log(USERLOG, ") "); // result will be on same line with no additional step
	}
	*buffer = 0; // remove any leftover argument data
	fnOutput = start; // for text debugger
	adjustIndent++;
	if (result & ENDCODES); // failed during argument processing
	else result = (*info->fn)(buffer);
	return ptr;
}

char* GetArgOfMacro(CALLFRAME* frame, int i, char* buffer, int limit)
{
	char* x = frame->arguments[i];
	if (*x == USERVAR_PREFIX)
	{
		strncpy(buffer, GetUserVariable(x, false), limit);
		x = buffer;
	}
	else if (*x == '_' && IsDigit(x[1]))
	{
		int id = GetWildcardID(x);
		if (id >= 0)
		{
			strncpy(buffer, wildcardOriginalText[id], limit);
			x = buffer;
		}
	}
	else if (*x == '\'' && x[1] == '_' && IsDigit(x[2]))
	{
		int id = GetWildcardID(x + 1);
		if (id >= 0)
		{
			strncpy(buffer, wildcardCanonicalText[id], limit);
			x = buffer;
		}
	}
	else if (*x == LCLVARDATA_PREFIX) x += 2; // skip ``
	return x;
}

static void UnbindVariables(HEAPREF list) // undo all variable changes
{
	while (list)
	{
		uint64 variable;
		uint64 value;
		uint64 internalbits;
		list = UnpackHeapval(list, variable, value, internalbits);
		//Log(STDUSERLOG,"UnbindVariables: %s reset to %s \r\n", ((WORDP)variable)->word, (char*)value);
		((WORDP)variable)->w.userValue = (char*)value;
		((WORDP)variable)->internalBits = (unsigned int)internalbits;
	}
}

static void BindVariables(CALLFRAME * frame, FunctionResult & result, char* buffer, bool showtrace, char* originalArgs)
{
	if (!frame->display) return;

	char var[MAX_WORD_SIZE];
	char* list = (char*)(frame->definition + 2);	// skip ( and space xxxx
	char* val;
	for (int i = 1; i <= (int)frame->n_arguments; ++i)
	{
		list = ReadCompiledWord(list, var);
		val = frame->arguments[i]; // constants wont have `` in front of them
		if (*var == USERVAR_PREFIX) // var not currency
		{
			WORDP arg = FindWord(var);
			if (!arg) continue;	// should never happen
			if (*val == USERVAR_PREFIX && !IsDigit(val[1])) val = GetUserVariable(val, false); // not currency
			else if (*val == SYSVAR_PREFIX)
			{
				Output(val, buffer, result, 0);
				val = AllocateStack(buffer, 0, true);
				*buffer = 0;
			}
			else if (val[0] == ENDUNIT && val[1] == ENDUNIT) val += 2; // skip over noeval marker
			else if (val[0] == '\'' && val[1] == '_' && IsDigit(val[2]))
			{
				int id = GetWildcardID(val + 1);
				if (id >= 0) val = AllocateStack(wildcardOriginalText[id], 0, true);
				else val = AllocateStack("");
			}
			else if (val[0] == '_' && IsDigit(val[1]))
			{
				int id = GetWildcardID(val);
				if (id >= 0) val = AllocateStack(wildcardCanonicalText[id], 0, true);
				else val = AllocateStack("");
			}
			else if (*val && *(val - 1) != '`') val = AllocateStack(val, 0, true);
			arg->w.userValue = val;
#ifndef DISCARDTESTING
			if (debugVar) (*debugVar)(arg->word, arg->w.userValue);
#endif
		}
		else // ^var
		{
		}
		if (showtrace && originalArgs) //  variable args may run out of display values
		{
			Log(USERLOG, " ");
			char* end = strchr(originalArgs, '|');
			if (end) *end = 0;
			if (strcmp(originalArgs, val)) Log(USERLOG, "%s`%s`", originalArgs, val);
			else Log(USERLOG, "%s", originalArgs);
			if (end) originalArgs = end + 1;
			else originalArgs = NULL;
		}
	}
	if (showtrace) Log(USERLOG, ")\r\n");
}

static char* InvokeUser(char*& buffer, char* ptr, FunctionResult & result, CALLFRAME * frame,
	unsigned int& expectedArgCount, int givenArgCount, char*& startRawArg, bool showtrace, char* originalArgs)
{
	WORDP D = frame->name;

	// handle any display variables -- MUST BE DONE AFTER computing arguments and before saving them onto vars
	if ((D->internalBits & FUNCTION_BITS) != IS_PLAN_MACRO && frame->definition && frame->definition[0] == '(')
	{
		frame->display = (char**)stackFree;
		frame->code = InitDisplay((char*)frame->definition); // will return 0 if runs out of heap space
		if (!frame->code) result = FAILRULE_BIT;
	}

	// now if expectedArgCount are local vars instead of ^, set them up (we have protected them by now)
	if (D->internalBits & MACRO_TRACE && !(D->internalBits & NOTRACE_FN)) trace = (unsigned int)D->inferMark;

	BindVariables(frame, result, buffer, showtrace, originalArgs);
	FreeBuffer(); // allocated by caller

	*buffer = 0; // remove any leftover argument data

	//   run the definition
	adjustIndent += 3; // call level and then invocation level
	if (D->internalBits & NOTRACE_FN) trace = 0;

	char* paren = strchr(frame->label, '(');
	*paren = '{';
	paren[1] = '}';
	if (result & ENDCODES)
	{
		ChangeDepth(0, NULL);
	}
	else if (frame->definition && (D->internalBits & FUNCTION_BITS) == IS_PLAN_MACRO)
	{
		frame->n_arguments = D->w.planArgCount;
		ChangeDepth(0, NULL);
		result = PlanCode(D, buffer); // run a plan
	}
#ifndef DISCARDJAVASCRIPT
	else if (frame->code && *frame->code == '*' && !strncmp((char*)frame->code, "*JavaScript", 11))
	{
		ChangeDepth(0, NULL);
		result = RunJavaScript((char*)frame->code + 11, buffer, expectedArgCount); // point at space after label
	}
#endif
	else if (frame->definition)
	{
		unsigned int flags = OUTPUT_FNDEFINITION;
		if (!(D->internalBits & IS_OUTPUT_MACRO)) flags |= OUTPUT_NOTREALBUFFER;// if we are outputmacro, we are merely extending an existing buffer
		Output((char*)frame->code, buffer, result, flags);
	}
	fnOutput = buffer; // for text debugger

	// undo any display variables
	if (frame->display) RestoreDisplay(frame->display, frame->definition);

	if (result & ENDCALL_BIT) result = (FunctionResult)(result ^ ENDCALL_BIT); // terminated user call 
	return ptr;
}

CALLFRAME* GetCallFrame(int depth)
{
	if (depth <= 0) return NULL;
	if (depth > globalDepth) return NULL;
	return frameList[depth];
}

static char* UserCall(char* buffer, char* ptr, CALLFRAME * frame, FunctionResult & result)
{
	WORDP D = frame->name;
	char* originalbuffer = buffer;
	unsigned int nArguments = 0;
	ptr = SkipWhitespace(ptr + 1); // aim to next major thing after ( 
	unsigned int argflags = 0;
	unsigned int expectedArgCount  = 0;
	bool showtrace = ((trace & (TRACE_OUTPUT | TRACE_USERFN)) || (D->internalBits & MACRO_TRACE)) && !(D->internalBits & NOTRACE_FN) && CheckTopicTrace();
	if ((D->internalBits & FUNCTION_BITS) == IS_PLAN_MACRO) expectedArgCount = D->w.planArgCount;
	else if (!D->w.fndefinition)
	{
		ReportBug((char*)"INFO: Missing function definition for %s\r\n", D->word);
		result = FAILRULE_BIT;
	}
	else
	{
		frame->definition = (char*)FindAppropriateDefinition(D, result, true);

		if (frame->definition)
		{
			int flags;
			frame->definition = ReadInt((char*)frame->definition, flags);
			argflags = flags;
			unsigned char c = (unsigned char)*frame->definition++;
			expectedArgCount = (c >= 'a') ? (c - 'a' + 15) : (c - 'A'); // expected args, leaving us at ( args ...
		}
		else result = FAILRULE_BIT;
	}
	frame->n_arguments = expectedArgCount;

	if (showtrace) Log(USERLOG, "calling %s(", D->word);

	// now process arguments
	char* startRawArg = ptr;
	char* definition = frame->definition;
	char* originalArgs = AllocateBuffer("usercall");
	int callArgumentIndex = 0;
	while (definition && ptr && *ptr && *ptr != ')') //   ptr is after opening (and before an arg but may have white space
	{
		if (currentRule == NULL) //   this is a table function- DONT EVAL ITS ARGUMENTS AND... keep quoted item intact
		{
			ptr = ReadCompiledWord(ptr, buffer); // return dq args as is
#ifndef DISCARDSCRIPTCOMPILER
			if (compiling && ptr == NULL) BADSCRIPT((char*)"TABLE-11 Arguments to %s ran out", D->word)
#endif
		}
		else
		{
			bool stripQuotes = (argflags & (1 << nArguments)) ? 1 : 0; // want to use quotes 
			// arguments to user functions are not evaluated, they will be used, in place, in the function.
			// EXCEPT evaluation of ^arguments must be immediate to maintain current context- both ^arg and ^"xxx" stuff
			if (showtrace)
			{
				char word[MAX_WORD_SIZE];
				ReadCompiledWord(ptr, word);
				strcat(originalArgs, word);
				strcat(originalArgs, "|");
			}
			ptr = ReadArgument(ptr, buffer, result); //   ptr returns on next significant char
			if (*buffer == '"' && stripQuotes)
			{
				size_t len = strlen(buffer);
				if (buffer[len - 1] == '"')
				{
					buffer[len - 1] = 0;
					memmove(buffer, buffer + 1, strlen(buffer));
				}
				// and purify internal \" to simple quote
				char* x = buffer;
				while ((x = strchr(x, '\\')))
				{
					if (x[1] == '"') memmove(x, x + 1, strlen(x)); // remove 
				}
			}
		}
		if (*buffer == 0) strcpy(buffer, (char*)"null");// no argument found - caller had null data in argument

		//   within a function, seeing function argument as an argument 
		//   switch to incoming arg now, later callArgumentBase will be wrong
		if (*buffer == '^' && IsDigit(buffer[1])) strcpy(buffer, FNVAR(buffer));
		if (*buffer == '"' && buffer[1] == ENDUNIT) // internal special quoted item, remove markers.
		{
			size_t len = strlen(buffer);
			if (buffer[len - 2] == ENDUNIT)
			{
				buffer[len - 2] = 0;
				memmove(buffer, buffer + 2, len - 1);
			}
		}
		if (*buffer == FUNCTIONSTRING && (buffer[1] == '"' || buffer[1] == '\''))
		{
			char* buf = AllocateStack(NULL, maxBufferSize, false); // initial form expected small
			size_t len = strlen(buffer);
			if (len >= (maxBufferSize - 2)) // overflow our intent
			{
				ReportBug("FormatString > MAX maxBufferSize SIZE");
				ReleaseStack(buf);
				FreeBuffer();
				result = FAILRULE_BIT;
				return ptr; // ReleaseStack(buf) also happens when ChangeDepth finishes
			}
			strcpy(buf, buffer);
			ReformatString(buf[1], buf + 2, buffer, result, 0);
			ReleaseStack(buf);
		}
		if (buffer[0] == USERVAR_PREFIX && strstr(buffer, "$_"))
		{
			char* data = GetUserVariable(buffer, false) - 2;
			if (buffer[1] == LOCALVAR_PREFIX && buffer[2] == LOCALVAR_PREFIX) // internal variable from jsonreadcsv
			{
				frame->arguments[++callArgumentIndex] = data; // shared data ptr
			}
			else
			{
				strcpy(buffer, data); // NOT by reference but by marked value
				if (!stricmp(buffer, (char*)"null")) *buffer = 0;	 // pass NOTHING as the value
				frame->arguments[++callArgumentIndex] = AllocateStack(buffer);
			}
		}
		else
		{
			if (!stricmp(buffer, (char*)"null")) *buffer = 0;	 // pass NOTHING as the value
			frame->arguments[++callArgumentIndex] = AllocateStack(buffer);
		}

		ptr = SkipWhitespace(ptr);
	} // end of argument processing

	// currentFNVarFrame remains visible to access ^0 type variables even when other frames which are system calls get added
	currentFNVarFrame = frame;
	*originalbuffer = 0;
	ptr = InvokeUser(buffer, ptr, result, frame, expectedArgCount, callArgumentIndex,startRawArg, showtrace, originalArgs); // originalArgs buffer freed by child
	// buffer allocated above freed in InvokeUser
	return ptr;
}

FunctionResult InternalCall(char* name, EXECUTEPTR fn, char* arg1, char* arg2, char* arg3, char* buffer, char* arg4, char* arg5)
{
	char namx[150];
	if (*name == '\'') ++name;  // quoted needed to survive compiler
	sprintf(namx, "%s()", name);
	CALLFRAME* frame = ChangeDepth(1, namx, false, NULL);
	frame->name = StoreWord(name, AS_IS);

	unsigned callArgumentIndex = 0;
	frame->arguments[++callArgumentIndex] = AllocateStack(arg1);
	if (arg2) frame->arguments[++callArgumentIndex] = AllocateStack(arg2);
	if (arg3) frame->arguments[++callArgumentIndex] = AllocateStack(arg3);
	if (arg4) frame->arguments[++callArgumentIndex] = AllocateStack(arg4);
	if (arg5) frame->arguments[++callArgumentIndex] = AllocateStack(arg5);
	frame->n_arguments = callArgumentIndex;

	FunctionResult result;
	if (fn) result = (*fn)(buffer);
	else
	{
		WORDP D = FindWord(name);
		*buffer = 0;
		frame->definition = (char*)FindAppropriateDefinition(D, result, true);
		if (frame->definition)
		{
			int flags;
			frame->definition = ReadInt((char*)frame->definition, flags) + 1;
		}

		// handle any display variables -- MUST BE DONE AFTER computing arguments and before saving them onto vars
		if (frame->definition && frame->definition[0] == '(')
		{
			frame->display = (char**)stackFree;
			frame->code = InitDisplay((char*)frame->definition); // will return 0 if runs out of heap space
			if (!frame->code) result = FAILRULE_BIT;
		}
		BindVariables(frame, result, buffer, false, NULL);

		ChangeDepth(0, NULL, false);
		unsigned int flags = OUTPUT_FNDEFINITION | OUTPUT_NOTREALBUFFER;
		Output((char*)frame->code, buffer, result, flags);
		fnOutput = buffer; // for text debugger

						   // undo any display variables
		if (frame->display) RestoreDisplay(frame->display, frame->definition);

		if (result & ENDCALL_BIT) result = (FunctionResult)(result ^ ENDCALL_BIT); // terminated user call 
	}

	frame->x.result = result;
	if (ChangeDepth(-1, name, (bool)NULL)) result = FAILINPUT_BIT; // stack is released by frame release
	return result;
}

static bool TimerFail()
{
	if (timerCheckInstance == TIMEOUT_INSTANCE) return true;
	
	if (++timerCheckInstance >= timerCheckRate)
	{
			timerCheckInstance = 0;
			if ((ElapsedMilliseconds() - volleyStartTime) >= timerLimit)
			{
				timerCheckInstance = TIMEOUT_INSTANCE;	// force it to fail all the time
				return true;
		}
	}
	return false;
}

static void FNResultTrace(CALLFRAME* frame,FunctionResult result,char* buffer, SystemFunctionInfo* info)
{
		// make a short description for which call this is, if we can
		char word[MAX_WORD_SIZE];
		*word = 0;
		// if (callArgumentIndex > (unsigned int)(frame->varBaseIndex + 1))
		{
			strncpy(word, frame->arguments[1], 40);
			if (*word == '$') strncpy(word, GetUserVariable(word, false), 40);
			word[40] = 0;
		}
		if (result == NOPROBLEM_BIT || result == ENDCALL_BIT)
		{
			if (info && info->properties == SAMELINE) Log(USERLOG, "  value: `%s`", buffer);
			else Log(USERLOG, "%s() value: `%s`", frame->label, buffer);
		}
		else
		{
			if (info && info->properties == SAMELINE) Log(USERLOG, "status %s", ResultCode(result));
			else Log(USERLOG, "%s() status %s", frame->label, ResultCode(result));
		}
		ReadCompiledWord(buffer, word);
		if (IsValidJSONName(word))
		{
			int count = 0;
			char* limit;
			char* buf = InfiniteStack(limit, "DoFunction");
			FACT* F = GetSubjectNondeadHead(FindWord(word));
			while (F)
			{
				if (++count <= 2) Log(USERLOG, " e.g. %s,", WriteFact(F, false, buf, true, false));
				else break;
				F = GetSubjectNondeadNext(F);
			}
			ReleaseInfiniteStack();
			Log(USERLOG, " - size %d facts", count);
		}
	Log(USERLOG, "\r\n");
}

char* DoFunction(char* name, char* ptr, char* buffer, FunctionResult & result) // DoCall(
{
	WORDP D = FindWord(name, 0, LOWERCASE_LOOKUP);
	if (!D || !(D->internalBits & FUNCTION_NAME))
	{
		if (compiling) BADSCRIPT("Attempting to call undefined function %s from table code", name)
		else if (csapicall == TEST_OUTPUT || csapicall == TEST_PATTERN) sprintf(testoutputFail, "Calling undefined function %s", name); // fail quietly
		result = UNDEFINED_FUNCTION;
		return ptr;
	}
	ptr = SkipWhitespace(ptr);
	if (*ptr != '(') // should have been arg list
	{
		result = FAILRULE_BIT;
		return ptr;
	}

	if (timerLimit && TimerFail()) // check for violating time restriction
	{
		result = FAILINPUT_BIT;
		return ptr;
	}

	uint64 start_time = ElapsedMilliseconds();
	*buffer = 0;
	if (stricmp(name, "^substitute")) impliedIf = ALREADY_HANDLED;	// we all allow immediate if context to pass thru here safely
	result = NOPROBLEM_BIT;

	// restorable context
	int oldimpliedIf = impliedIf;
	char* oldcode = codeStart;
	char* oldRealCode = realCode;
	int oldtrace = trace;
	int oldtiming = timing;
	int oldindent = adjustIndent;

	SystemFunctionInfo* info = NULL;
	bool systemcall = (D->x.codeIndex && !(D->internalBits & (IS_PLAN_MACRO | IS_TABLE_MACRO))); // system function --   but IS_TABLE_MACRO distinguishes  but PLAN also has a topicindex which is a codeindex
	bool streamArg = false;
	char namx[150];
	sprintf(namx, "%s()", name);
	CALLFRAME* frame = ChangeDepth(1, namx, false, (systemcall) ? NULL : (ptr + 2));
	frame->name = D;

	--adjustIndent; // show call args at same level, until execution

	if (systemcall) // system function --   but IS_TABLE_MACRO distinguishes  but PLAN also has a topicindex which is a codeindex
	{
		ptr = SystemCall(buffer, ptr, frame, result, streamArg);
	}
	else //   user function (plan macro, inputmacro, outputmacro, tablemacro)) , eg  ^call (_10 ^2 it ^call2 (3 ) )  spot each token has 1 space separator 
	{
		ptr = UserCall(buffer, ptr, frame, result);
	} // end user function

	if ((result == FAILRULE_BIT || result == FAILTOPRULE_BIT) && csapicall == TEST_OUTPUT && ++testoutputbacktracecount < 5)
	{
		strcat(testoutputbacktrace, name);
		strcat(testoutputbacktrace, " called by ");
	}

	// things like ^notrace(_$x = 5) should not endanger the variable in outer scope

	frame->x.result = result;
	if (ChangeDepth(-1, name, true)) result = FAILINPUT_BIT; 
	bool showtrace = ((trace & TRACE_OUTPUT || D->internalBits & MACRO_TRACE || (trace & TRACE_USERFN && frame->definition)) && CheckTopicTrace());
	if (showtrace)
	{
		if (systemcall) // system function --   but IS_TABLE_MACRO distinguishes  but PLAN also has a topicindex which is a codeindex
		{
			info = &systemFunctionSet[D->x.codeIndex];
		}
		FNResultTrace(frame, result, buffer, info);
	}

	// currently only user functions- NOTRACE printouts would be boring
	int diff = (int)(ElapsedMilliseconds() - start_time);
	TrackTime(D, diff);
	if (frame->definition && (timing & TIME_USERFN || D->internalBits & MACRO_TIME) && !(D->internalBits & NOTIME_FN) && CheckTopicTime()) 
	{
		if (diff > 0 || timing & TIME_ALWAYS)
		{
			// make a short description for which call this is, if we can
			char word[MAX_WORD_SIZE];
			*word = 0;
			strncpy(word, frame->arguments[1], 40);
			if (*word == '$') strncpy(word, GetUserVariable(word, false), 40);
			word[40] = 0;

			Log(STDTIMELOG, (char*)"%s(%s) time: %d ms\r\n", name, word, diff);
		}
	}

	codeStart = oldcode;
	impliedIf = oldimpliedIf;
	realCode = oldRealCode;
	if (!streamArg) stackFree = (char*)frame; // deallocoate ARGUMENT space

	trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
	timing = (modifiedTiming) ? modifiedTimingVal : oldtiming;
	adjustIndent = oldindent;

	if (ptr && *ptr == ')') // skip ) and space if there is one...
	{
		if (ptr[1] == ' ') return ptr + 2; // if this is a pattern comparison, this will NOT be a space, but will be a comparison op instead missing it
		return ptr + 1;	// ptr to the end unit
	}
	else return ptr;
}

void DumpFunctions()
{
	unsigned int k = 0;
	SystemFunctionInfo* fn;
	while ((fn = &systemFunctionSet[++k]) && fn->word)
	{
		if (*fn->word != '^') Log(USERLOG, "%s\r\n", fn->word);
		else Log(USERLOG, "%s - %s\r\n", fn->word, fn->comment);
	}
}

//////////////////////////////////////////////////////////
/// FUNCTION UTILITIES
//////////////////////////////////////////////////////////

char* ResultCode(FunctionResult result)
{
	char* ans = "OK";
	if (result & ENDCALL_BIT) ans = "ENDCALL";
	else if (result & ENDRULE_BIT) ans = "ENDRULE";
	else if (result & FAILRULE_BIT) ans = "FAILRULE";
	else if (result & RETRYRULE_BIT) ans = "RETRYRULE";
	else if (result & RETRYTOPRULE_BIT) ans = "RETRYTOPRULE";

	else if (result & ENDTOPIC_BIT) ans = "ENDTOPIC";
	else if (result & FAILTOPIC_BIT) ans = "FAILTOPIC";
	else if (result & RETRYTOPIC_BIT) ans = "RETRYTOPIC";

	else if (result & ENDSENTENCE_BIT) ans = "ENDSENTENCE";
	else if (result & FAILSENTENCE_BIT) ans = "FAILSENTENCE";
	else if (result & RETRYSENTENCE_BIT) ans = "RETRYSENTENCE";

	else if (result & RETRYINPUT_BIT) ans = "RETRYINPUT";
	else if (result & ENDINPUT_BIT) ans = "ENDINPUT";
	else if (result & FAILINPUT_BIT) ans = "FAILINPUT";
	else if (result & FAILMATCH_BIT) ans = "FAILMATCH";
	else if (result == NOPROBLEM_BIT) ans = "NOPROBLEM";
	else if (result == FAILLOOP_BIT) ans = "FAILLOOP";
	else if (result == ENDLOOP_BIT) ans = "ENDLOOP";
	else if (result & UNDEFINED_FUNCTION) ans = "UNDEFINED_FUNCTION";
	return ans;
};


static unsigned int ComputeSyllables(char* word)
{
	char copy[MAX_WORD_SIZE];
	MakeLowerCopy(copy, word);
	size_t len = strlen(copy);
	if (len <= 2) return 1;

	char* ptr = copy - 1;
	unsigned int vowels = 0;
	int series = 0;
	// count number of vowels
	// subtract 1 for each diphthong or triphthong in word
	// Does the word end with "le" or "les?" Add 1 only if the letter before the "le" is a consonant.
	while (*++ptr)
	{
		if (IsVowel(*ptr))
		{
			if (ptr == copy)
			{
				if (IsVowel(*ptr)) ++vowels;
				continue;
			}
			if (!IsVowel(*(ptr - 1)))
			{
				if (len > 2 && *ptr == 'e' && !ptr[1] && IsVowel(*(ptr - 2))) {}// silent e
				else ++vowels; // presume vowels in sequence are one sound
			}
			if (*ptr == 'y' && IsVowel(ptr[1])) ++vowels; // dont merge vowel y in player
			if (*(ptr - 1) == 'e' && *ptr == 'o') ++vowels; // preinvolve reoccupy
			// ia and io not dipthongs  
			if (*(ptr - 1) == 'i')
			{
				if (*ptr == 'a' && ptr[1] == 'n' && !ptr[2]) {} // australian
				else if (*ptr == 'a') ++vowels; // hiatus
				else if (*ptr == 'o' && ptr[1] != 'n') ++vowels; // not placation but curious
			}
			if (len > 2 && *(ptr - 2) == 'i' && *(ptr - 1) == 'o' && *ptr == 'u' && ptr[2] == 's') ++vowels; // ficticious
			if (*(ptr - 1) == 'i' && *ptr == 'e' && ptr[1] == 'r') ++vowels; // flier
			if (ptr[1] == 'r')
			{
				if (*ptr == 'u' && *(ptr - 1) == 'o') ++vowels; // hour
				else if (*ptr == 'i' && ptr[2] == 'e') ++vowels; // ireland  fire
				else if (*(ptr - 1) == 'o' && *ptr == 'i') ++vowels; // choir
			}
			char c = *(ptr - 1);
			if (!IsVowel(c) && *ptr == 'e' && ptr[1] == 'd' && !ptr[2])  // ed ending
			{
				// dont sound the ed dending unless D, T, or ((consonant other than R) + R) or (consonant + L).
				if (c == 'd' || c == 't') {}
				else if (len > 2 && c == 'r' && *(ptr - 2) != 'r') {}
				else if (len > 2 && c == 'l' && !IsVowel(*(ptr - 2))) {}
				else --vowels; // walked (dont count ed)
			}
			if (!IsVowel(c) && *ptr == 'e' && ptr[1] == 's' && !ptr[2]) // es ending
			{
				// dont sound the es  unless ...
				if (c == 'c' || c == 'g' || c == 'x' || c == 's' || c == 'z') {}
				else if (len > 2 && c == 'l' && !IsVowel(*(ptr - 2))) {}
				else --vowels; // walked (dont count ed)
			}
		}
	}
	// trailing le and les, only count the e if consonant before
	if (len > 3 && !stricmp(copy + len - 3, "les"))
	{
		if (IsVowel(copy[len - 4])) --vowels;
	}
	if (len > 2 && !stricmp(copy + len - 2, "le"))
	{
		if (IsVowel(copy[len - 3])) --vowels;
	}

	return vowels;
}

static void RecurseMembers(char* concept)
{
	WORDP D = FindWord(concept);
	FACT* F = GetObjectNondeadHead(D);
	while (F)
	{
		if (F->verb == Mmember)
		{
			WORDP E = Meaning2Word(F->subject);
			if (!(E->internalBits & BEEN_HERE))
			{
				if (*E->word == '~') RecurseMembers(E->word);
				else
				{
					int index = FACTSET_COUNT(impliedSet) + 1;
					if (index == MAX_FIND) return; // abort at limit
					factSet[impliedSet][index] = F;
					SET_FACTSET_COUNT(impliedSet, index);
				}
				E->internalBits |= BEEN_HERE;
			}
		}
		F = GetObjectNondeadNext(F);
	}
}

static FunctionResult AllMembersCode(char* buffer)
{
	SET_FACTSET_COUNT(impliedSet, 0);
	char* set = ARGUMENT(1);
	if (*set != '~') return FAILRULE_BIT;
	RecurseMembers(set);
	for (int i = FACTSET_COUNT(impliedSet); i >= 1; --i)
	{
		WORDP D = Meaning2Word(factSet[impliedSet][i]->subject);
		D->internalBits &= -1 ^ BEEN_HERE;
	}
	impliedSet = ALREADY_HANDLED;
	return NOPROBLEM_BIT;
}

static FunctionResult RandomMember(char* buffer, char* answer)
{
#ifdef INFORMATION
	returns a random member of a set or class

		returns FAILRULE if a bad set is given.

		The value is recursive.If the random member chosen is a set or class, the
		link is followedand a random member from the next level is chosen, and so on.
		If the value is a wordnet reference it goes lower until it cant go any lower.

#endif
		MEANING* members = (MEANING*)AllocateBuffer();
	int limit = maxBufferSize / sizeof(MEANING);
loop:
	WORDP D = FindWord(answer);
	if (!D)
	{
		FreeBuffer();
		return FAILRULE_BIT;
	}
	int count = 0;
	FACT* F = GetObjectNondeadHead(D);
	while (F && count < limit)
	{
		if (F->verb == Mmember)  members[count++] = F->subject;
		F = GetObjectNondeadNext(F);
	}
	if (!count)
	{
		FreeBuffer();
		return FAILRULE_BIT; //   none found
	}

	//   pick one at random
	while (ALWAYS)
	{
		MEANING M = members[random(count)];
		M = GetMaster(M);
		D = Meaning2Word(M);
		unsigned int index = Meaning2Index(M);
		answer = D->word;
		if (*answer == '~') goto loop; //   member is a subset or class, get from it instead

		else if (index) // go down hierarchy until you cant and use that
		{
			F = GetObjectNondeadHead(D); // he is the general, get a specific
			count = 0;
			while (F && count < limit)
			{
				if (F->verb == Mis && Meaning2Index(F->object) == index) members[count++] = F->subject;
				F = GetObjectNondeadNext(F);
			}
			if (count) continue; // select from there
			// we are a bottom meaning
			strcpy(buffer, D->word);
			FreeBuffer();
			return NOPROBLEM_BIT;
		}
		else break;
	}

	if (*answer == '<') ++answer; //   interjections have < in front
	strcpy(buffer, answer);
	FreeBuffer();
	return NOPROBLEM_BIT;
}

FunctionResult FLR(char* buffer, char* which)
{
	int store;
	*buffer = 0;
	char* limit;
	char* word = InfiniteStack(limit, "FLR");
	bool keep = false;
	char* ptr = GetPossibleFunctionArgument(ARGUMENT(1), word);
	store = GetSetID(word);
	char type = *GetSetType(word);
	ReleaseInfiniteStack();

	if (!strnicmp(ptr, (char*)"KEEP", 4)) keep = true;
	if (store == ILLEGAL_FACTSET) return FAILRULE_BIT;
	unsigned int count = FACTSET_COUNT(store);
	if (!count)
	{
		if (impliedWild != ALREADY_HANDLED)
		{
			SetWildCardIndexStart(impliedWild); // cannot damage these for a single value
			SetWildCard((char*)"", (char*)"", 0, 0); // value
		}
		impliedWild = ALREADY_HANDLED;
		return ENDRULE_BIT; //   terminates but does not cancel output
	}

	if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, "%s[%d] => ", word, count);

	if (!withinLoop && planning && (*which != 'n') && *GetTopicName(currentTopicID) == '^' && !backtrackable) backtrackable = true;

	// pick fact
	unsigned int item;
	if (*which == 'l') item = count; //   last
	else if (*which == 'f') item = 1; //   first
	else if (*which == 'n') // next
	{
		item = ++factSetNext[store];
		if (count < item) return FAILRULE_BIT;
	}
	else if (*which == 'r') item = random(count) + 1;    // random
	else // specific index
	{
		keep = true;
		item = atoi(which);
		if (count < item || item == 0) return FAILRULE_BIT;
	}
	FACT* F = factSet[store][item];

	// remove fact from set, but next does not alter set
	if (*which != 'n' && !keep)
	{
		SET_FACTSET_COUNT(store, (count - 1));
		memmove(&factSet[store][item], &factSet[store][item + 1], sizeof(FACT*) * (count - item));
	}

	// transfer fact pieces appropriately
	MEANING Mfirst = 0;
	MEANING Mlast = 0;
	uint64 factSubject = 0;
	uint64 factObject = 0;
	if (trace & TRACE_INFER && CheckTopicTrace()) TraceFact(F);
	if (type == 'f') // want entire fact as index
	{
		if (impliedSet == ALREADY_HANDLED) sprintf(buffer, (char*)"%u", Fact2Index(F));
		else AddFact(impliedSet, F);
	}
	else if (type == 's') // want subject
	{
		if (!F) strcpy(buffer, (char*)"null");
		else
		{
			MEANING M = F->subject;
			if (F->flags & FACTSUBJECT) sprintf(buffer, (char*)"%u", M);
			else sprintf(buffer, (char*)"%s", Meaning2Word(M)->word);
		}
	}
	else if (type == 'v') // want verb
	{
		if (!F) strcpy(buffer, (char*)"null");
		else
		{
			MEANING M = F->verb;
			if (F->flags & FACTVERB) sprintf(buffer, (char*)"%u", M);
			else sprintf(buffer, (char*)"%s", Meaning2Word(M)->word);
		}
	}
	else if (type == 'o') // want object
	{
		if (!F) strcpy(buffer, (char*)"null");
		else
		{
			MEANING M = F->object;
			if (F->flags & FACTOBJECT) sprintf(buffer, (char*)"%u", M);
			else sprintf(buffer, (char*)"%s", Meaning2Word(M)->word);
		}
	}
	else if (type == 'a' || type == '+' || type == ' ' || !type || type == 'r') // want all, subject first
	{
		if (!F) Mfirst = Mlast = 0xffffffff;
		else
		{
			Mfirst = F->subject;
			factSubject = F->flags & FACTSUBJECT;
			Mlast = F->object;
			factObject = F->flags & FACTOBJECT;
		}
	}
	else // want all, object first
	{
		if (!F) Mfirst = Mlast = 0xffffffff;
		else
		{
			Mlast = F->subject;
			factObject = F->flags & FACTSUBJECT;
			Mfirst = F->object;
			factSubject = F->flags & FACTOBJECT;
		}
	}
	if (Mfirst) // spread
	{
		char factID[100];
		char* piece;
		if (Mfirst == 0xffffffff) piece = "null";
		else if (factSubject)
		{
			sprintf(factID, (char*)"%u", Mfirst);
			piece = factID;
		}
		else if (type == 'r') piece = WriteMeaning(Mfirst, false);
		else piece = Meaning2Word(Mfirst)->word;

		// _wildcard can take all, otherwise you get just a field
		// for variables. not legal for sets
		if (!F) strcpy(buffer, (char*)"null");
		else if (impliedWild == ALREADY_HANDLED) strcpy(buffer, piece);
		else
		{
			SetWildCardIndexStart(impliedWild);
			if (trace & TRACE_INFER && CheckTopicTrace()) Log(USERLOG, " _%d=%s ", impliedWild, piece);
			SetWildCard(piece, piece, 0, 0);

			//   verb 
			if (Mfirst == 0xffffffff) piece = "null";
			else
			{
				MEANING M = F->verb;
				if (F->flags & FACTVERB)
				{
					sprintf(factID, (char*)"%u", M);
					piece = factID;
				}
				else if (type == 'r') piece = WriteMeaning(M, false);
				else piece = Meaning2Word(M)->word;
			}
			if (trace & TRACE_INFER && CheckTopicTrace()) Log(USERLOG, " _%d=%s ", impliedWild + 1, piece);
			SetWildCard(piece, piece, 0, 0);

			//   object
			if (Mfirst == 0xffffffff) piece = "null";
			else if (factObject)
			{
				sprintf(factID, (char*)"%u", Mlast);
				piece = factID;
			}
			else if (type == 'r') piece = WriteMeaning(Mlast, false);
			else piece = Meaning2Word(Mlast)->word;
			if (trace & TRACE_INFER && CheckTopicTrace()) Log(USERLOG, " _%d=%s ", impliedWild + 2, piece);
			SetWildCard(piece, piece, 0, 0);

			if (type == 'a' && F) // all include flags on fact
			{
				sprintf(tmpWord, (char*)"0x%08x", F->flags);
				SetWildCard(tmpWord, tmpWord, 0, 0);
			}
		}
		impliedSet = impliedWild = ALREADY_HANDLED; // we spread the values out
	}
	if (trace & TRACE_OUTPUT && *buffer && CheckTopicTrace()) Log(USERLOG, " %s  ", buffer);
	return NOPROBLEM_BIT;
}

bool RuleTest(char* data) // see if pattern matches
{
	char pattern[MAX_WORD_SIZE];
	GetPattern(data, NULL, pattern);
	wildcardIndex = 0;
	int matched = 0;
	char* buffer = AllocateStack(NULL, maxBufferSize); // not expecting big data from function call in match
	char oldmark[MAX_SENTENCE_LENGTH];
	memcpy(oldmark, unmarked, MAX_SENTENCE_LENGTH);
	MARKDATA hitdata;
	hitdata.start = 0;
	bool answer = Match(pattern + 2,0,hitdata, 1, 0, matched, '('); // start past the opening paren
	memcpy(unmarked, oldmark, MAX_SENTENCE_LENGTH);
	ReleaseStack(buffer);
	ShowMatchResult(answer ? NOPROBLEM_BIT : FAILRULE_BIT, pattern + 2, NULL, 0);
	return answer;
}

unsigned int Callback(WORDP D, char* arguments, bool boot, bool mainoutput)
{
	if (!D || !(D->internalBits & FUNCTION_NAME)) return FAILRULE_BIT;
	unsigned int oldtrace = trace;
	unsigned int oldtiming = timing;
	if (!boot) trace = timing = 0;
	char args[MAX_WORD_SIZE];
	strcpy(args, arguments);
	FunctionResult result;
	if (!mainoutput) AllocateOutputBuffer();
	DoFunction(D->word, args, currentOutputBase, result);
	if (*currentOutputBase && boot)
	{
		(*printer)("Boot response: %s\r\n", currentOutputBase);
		if (server) Log(SERVERLOG, "Boot response: %s\r\n", currentOutputBase);
	}
	if (!mainoutput) FreeOutputBuffer();
	trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
	timing = (modifiedTiming) ? modifiedTimingVal : oldtiming;
	return result;
}

void ResetUser(char* input)
{
	if (globalDepth) // in midst of execution, being safe
	{
		inputCounter = 0;
		totalCounter = 0;
		inputSentenceCount = 0;
		ReadNewUser();
		userFirstLine = 1;
		return;
	}
	SAVEOLDCONTEXT()
	ResetToPreUser();	// back to empty state before user
	KillShare();
	ReadNewUser();
	userFirstLine = 1;
	responseIndex = 0;
	wasCommand = BEGINANEW;
	*input = 0;
	*originalUserInput = 0;
	RESTOREOLDCONTEXT()
}

//////////////////////////////////////////////////////////
/// TOPIC FUNCTIONS
//////////////////////////////////////////////////////////

static FunctionResult AddTopicCode(char* buffer)
{
	AddPendingTopic(FindTopicIDByName(ARGUMENT(1))); // does not fail, just may not become pending
	return NOPROBLEM_BIT;
}

static FunctionResult ClearTopicsCode(char* buffer)
{
	ClearPendingTopics();
	return NOPROBLEM_BIT;
}

static FunctionResult CountTopicCode(char* buffer)
{
	int topicid = FindTopicIDByName(ARGUMENT(1));
	if (BlockedBotAccess(topicid)) return FAILRULE_BIT;
	topicBlock* block = TI(topicid);

	char* name = ARGUMENT(2);
	if (!strnicmp(name, (char*)"gambit", 6)) sprintf(buffer, (char*)"%d", GAMBIT_MAX(block->topicMaxRule));
	else if (!strnicmp(name, (char*)"responder", 9)) sprintf(buffer, (char*)"%d", RULE_MAX(block->topicMaxRule) - GAMBIT_MAX(block->topicMaxRule));
	else if (!strnicmp(name, (char*)"rule", 4)) sprintf(buffer, (char*)"%d", RULE_MAX(block->topicMaxRule));
	else if (!stricmp(name, (char*)"used")) sprintf(buffer, (char*)"%d", TopicUsedCount(topicid));
	else if (!stricmp(name, (char*)"available") || !stricmp(name, (char*)"availablegambits"))
	{
		unsigned int count = 0;
		unsigned int* map = block->gambitTag;
		unsigned int gambitID = *map;
		while (gambitID != NOMORERULES)
		{
			if (UsableRule(topicid, gambitID)) ++count;
			gambitID = *++map;
		}
		sprintf(buffer, (char*)"%u", count);
	}
	else if (!stricmp(name, (char*)"availableresponders"))
	{
		unsigned int count = 0;
		unsigned int* map = block->responderTag;
		unsigned int responderID = *map;
		while (responderID != NOMORERULES)
		{
			if (UsableRule(topicid, responderID)) ++count;
			responderID = *++map;
		}
		sprintf(buffer, (char*)"%u", count);
	}
	else return FAILRULE_BIT;
	return NOPROBLEM_BIT;
}

FunctionResult GambitCode(char* buffer)
{
	char arguments[MAX_ARG_LIMIT + 1][200];
	if (planning) return FAILRULE_BIT;	// cannot call from planner
	if (all) return FAILRULE_BIT; // dont generate gambits when doing all
	bool fail = false;
	unsigned int i;
	for (i = 1; i <= MAX_ARG_LIMIT; ++i)
	{
		char* a = ARGUMENT(i);
		if (!*a) break;
		size_t len = strlen(a);
		if (len > SIZELIM)
		{
			ReportBug((char*)"respond code size limit exceeded to %d for %s\r\n", len, a);
			return FAILRULE_BIT;
		}
		strcpy(arguments[i], a);
		if (!stricmp(a, (char*)"FAIL") && !*ARGUMENT(i + 1))
		{
			fail = true;
			*arguments[i] = 0;
		}
	}
	--adjustIndent; // stay at call level
	*arguments[i] = 0;
	char* oldhow = howTopic;
	howTopic = "gambit";

	int oldIndex = responseIndex;
	FunctionResult result = NOPROBLEM_BIT;
	int oldreuseid = currentReuseID;
	int oldreusetopic = currentReuseTopic;
	for (i = 1; i <= MAX_ARG_LIMIT; ++i)
	{
		// gambit(PENDING) means from interesting stack  
		// gambit(~name) means use named topic 
		// gambit(~) means current topic we are within now
		// gambit (word) means topic with that keyword
		char* word = arguments[i];
		if ((oldIndex < responseIndex && result == NOPROBLEM_BIT) || result != NOPROBLEM_BIT || *word == 0) break; // generated an answer or failed or ran out
		oldIndex = responseIndex; // in case answer generated but topic claims failure

		//  if "~", get current topic name to use for gambits
		int topicid;

		currentReuseID = currentRuleID; // LOCAL reuse
		currentReuseTopic = currentRuleTopic;
		SAVEOLDCONTEXT()

			if (!stricmp(word, (char*)"pending")) // pick topic from pending stack
			{
				unsigned int stack[MAX_TOPIC_STACK + 1];
				memcpy(stack, pendingTopicList, pendingTopicIndex * sizeof(unsigned int)); // copy stack
				int oldPendingIndex = pendingTopicIndex;
				while (oldPendingIndex) // walk topics, most recent first
				{
					topicid = stack[--oldPendingIndex];
					char* xname = GetTopicName(currentTopicID); // just for debugging
					if (TopicInUse(topicid) == -1) continue;
					CALLFRAME* frame = ChangeDepth(1, xname);
					currentTopicID = topicid;
					FunctionResult myresult = PerformTopic(GAMBIT, buffer);
					ChangeDepth(-1, xname);
					if (myresult & RESULTBEYONDTOPIC)
					{
						result = myresult;
						break;
					}
					if (responseIndex > oldIndex)
					{
						result = NOPROBLEM_BIT;
						break;
					}
				}
			}

		// do topic by name
			else if (*word == '~') //current topic
			{
				topicid = FindTopicIDByName(word);
				if (topicid && !(GetTopicFlags(topicid) & TOPIC_BLOCKED))
				{
					CALLFRAME* frame = ChangeDepth(1, word);
					int pushed = PushTopic(topicid);
					if (pushed < 0) result = FAILRULE_BIT;
					else
					{
						result = PerformTopic(GAMBIT, buffer);
						if (pushed) PopTopic();
					}
					ChangeDepth(-1, word);
				}
			}

		// do topic by keyword
			else
			{
				WORDP D = FindWord(word);
				FACT* F = NULL;
				if (!D) result = NOPROBLEM_BIT;
				else  F = GetSubjectNondeadHead(D);
				while (F) // find topics word is a direct member of
				{
					if (F->verb == Mmember)
					{
						WORDP E = Meaning2Word(F->object);
						if (E->internalBits & TOPIC)
						{
							int xtopic = FindTopicIDByName(E->word);
							if (xtopic && !(GetTopicFlags(xtopic) & (TOPIC_BLOCKED | TOPIC_SYSTEM | TOPIC_NOGAMBITS)))
							{
								CALLFRAME* frame = ChangeDepth(1, E->word);
								int pushed = PushTopic(xtopic);
								if (pushed < 0)
								{
									result = FAILRULE_BIT;
									ChangeDepth(-1, E->word);
									break;
								}
								result = PerformTopic(GAMBIT, buffer);
								ChangeDepth(-1, E->word);
								if (pushed) PopTopic();
								if (result & RESULTBEYONDTOPIC) break;
								if (responseIndex > oldIndex)
								{
									result = NOPROBLEM_BIT;
									break;
								}
							}
						}
					}
					F = GetSubjectNondeadNext(F);
				}
			}
		RESTOREOLDCONTEXT() // this is where we were

	}
	if (fail && responseIndex <= oldIndex)  result = FAILRULE_BIT; // report failure
	currentReuseID = oldreuseid;
	currentReuseTopic = oldreusetopic;
	howTopic = oldhow;
	++adjustIndent;

	return result;
}

static FunctionResult GetVerifyCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	int topicid;
	int id;
	char* verify = GetVerify(arg1, topicid, id); //  ~topic.#.#=LABEL<~topic.#.#  is a maximally complete why
	if (verify) strcpy(buffer, verify);
	return NOPROBLEM_BIT;
}

static FunctionResult GetRuleCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	char* arg2 = ARGUMENT(2);
	char* arg3 = ARGUMENT(3);
	int topicid = currentTopicID;
	int id;
	char* rule;
	bool fulllabel = false;
	bool crosstopic = false;
	char* dot = strchr(arg2, '.');
	if (dot && IsDigit(dot[1])) rule = GetRuleTag(topicid, id, arg2);
	else rule = GetLabelledRule(topicid, arg2, arg3, fulllabel, crosstopic, id, currentTopicID);
	if (!rule) return FAILRULE_BIT;
	if (!stricmp(arg1, (char*)"tag")) sprintf(buffer, (char*)"%s.%u.%u", GetTopicName(topicid), TOPLEVELID(id), REJOINDERID(id));
	else if (!stricmp(arg1, (char*)"topic")) strcpy(buffer, GetTopicName(topicid));
	else if (!stricmp(arg1, (char*)"label"))
	{
		if (csapicall == TEST_PATTERN && *testpatternlabel)  strcpy(buffer, testpatternlabel);
		else GetLabel(rule, buffer);
	}
	else if (!stricmp(arg1, (char*)"type")) sprintf(buffer, (char*)"%c", *rule);
	else if (!stricmp(arg1, (char*)"pattern")) // use pattern notation so it can work with ^match and will not be harmed stored as a variable
	{
		*buffer = '"';
		buffer[1] = 0;
		GetPattern(rule, NULL, buffer + 1, true);
		if (!buffer[1]) *buffer = 0;
		else strcat(buffer, (char*)"\"");
	}
	else if (!stricmp(arg1, (char*)"usable")) strcpy(buffer, (UsableRule(topicid, id)) ? (char*)"1" : (char*)"");
	else // output
	{
		rule = GetPattern(rule, NULL, NULL);
		char* end = strchr(rule, ENDUNIT);  // will not be a useful output as blanks will become underscores, but can do ^reuse() to execute it
		*end = 0;
		strcpy(buffer, rule);
		*end = ENDUNIT;
	}
	if (trace & TRACE_OUTPUT && CheckTopicTrace())
	{
		char word[MAX_WORD_SIZE];
		strncpy(word, buffer, 50);
		word[50] = 0;
		Log(USERLOG, " %s ", word);
	}
	return NOPROBLEM_BIT;
}

static FunctionResult HasGambitCode(char* buffer)
{
	// hasgambit(~topic) means does it have any unused gambits
	// hasgambit(~topic last) means is last gambit unused
	// hasgambit(~topic any) means does it have gambits used or unused
	char* name = ARGUMENT(1);
	int topicid = FindTopicIDByName(name);
	if (!topicid) return FAILRULE_BIT;
	topicBlock* block = TI(topicid);

	unsigned int gambits = GAMBIT_MAX(block->topicMaxRule);   // total gambits of topic
	if (!gambits) return FAILRULE_BIT;

	char* arg = ARGUMENT(2);
	if (!stricmp(arg, (char*)"last")) return UsableRule(topicid, block->gambitTag[gambits - 1]) ? NOPROBLEM_BIT : FAILRULE_BIT; // is last gambit unused
	else if (!stricmp(arg, (char*)"any")) return NOPROBLEM_BIT;
	else return (HasGambits(topicid) < 1) ? FAILRULE_BIT : NOPROBLEM_BIT;
}

static FunctionResult KeepCode(char* buffer)
{
	if (planning) return FAILRULE_BIT;

	AddKeep(currentRule);
	return NOPROBLEM_BIT;
}

static FunctionResult LastUsedCode(char* buffer)
{
	char* name = ARGUMENT(1);
	char* what = ARGUMENT(2);
	int topicid = FindTopicIDByName(name);
	if (!topicid)  return FAILRULE_BIT;
	topicBlock* block = TI(topicid);

	if (!stricmp(what, (char*)"gambit")) sprintf(buffer, (char*)"%d", block->topicLastGambitted);
	else if (!stricmp(what, (char*)"responder")) sprintf(buffer, (char*)"%d", block->topicLastRespondered);
	else if (!stricmp(what, (char*)"rejoinder")) sprintf(buffer, (char*)"%d", block->topicLastRejoindered);
	else // any 
	{
		int last = block->topicLastRejoindered;
		if (block->topicLastRespondered > last) last = block->topicLastRespondered;
		if (block->topicLastGambitted > last) last = block->topicLastGambitted;
		sprintf(buffer, (char*)"%d", last);
	}
	return NOPROBLEM_BIT;
}

static FunctionResult PopTopicCode(char* buffer) // reconsider BUG
{
	char* arg1 = ARGUMENT(1);
	if (*arg1 == '~') RemovePendingTopic(FindTopicIDByName(arg1)); // current topic may continue executing
	else if (!*arg1) // make current topic not interesting AND quit it
	{
		RemovePendingTopic(currentTopicID);
		return ENDTOPIC_BIT;
	}
	else return FAILRULE_BIT;
	return NOPROBLEM_BIT;
}

static FunctionResult DoRefine(char* buffer, char* arg1, bool fail, bool doall)
{
	FunctionResult result = NOPROBLEM_BIT;
	char* rule;
	int id = currentRuleID;
	int topicid = currentTopicID;
	int originalTopic = topicid;
	char level = currentRule ? *currentRule : 'a';

	if (!*arg1)
	{
		// of course if there were NO rejoinders this would be in error
		rule = currentRule; // default continue AFTER the current rule
		level = TopLevelRule(currentRule) ? 'a' : (*currentRule + 1);
	}
	else // designated
	{
		bool fulllabel = false;
		bool crosstopic = false;
		char* dot = strchr(arg1, '.');
		if (dot && IsDigit(dot[1])) rule = GetRuleTag(topicid, id, arg1);
		else rule = GetLabelledRule(topicid, arg1, (char*)"", fulllabel, crosstopic, id, currentTopicID);
		level = TopLevelRule(rule) ? 'a' : (*rule + 1);
	}

	if (!rule) return FAILRULE_BIT;

	// change context now
	SAVEOLDCONTEXT()
		currentRuleTopic = currentTopicID = topicid;
	currentRuleID = id;
	currentRule = FindNextRule(NEXTRULE, rule, id);
	CALLFRAME* oldframe = GetCallFrame(globalDepth);
	oldframe->display = (char**)stackFree;
	char* locals = GetTopicLocals(currentTopicID);
	bool updateDisplay = locals && DifferentTopicContext(0, currentTopicID);
	unsigned int oldTrace = EstablishTopicTrace();
	unsigned int oldTiming = EstablishTopicTiming();

	bool failed = false;

	if (updateDisplay && !InitDisplay(locals)) failed = true;

	while (currentRule && level == *currentRule) // try all choices
	{
		result = TestRule(id, currentRule, buffer, true);
		if (result == FAILTOPRULE_BIT || result == ENDTOPRULE_BIT) break;
		else if (doall && result != NOPROBLEM_BIT && result != FAILMATCH_BIT && result != ENDRULE_BIT) break; // failure
		else if (!doall && result != FAILMATCH_BIT && result != FAILRULE_BIT) break;
		else result = NOPROBLEM_BIT;

		while (currentRule && *currentRule)
		{
			currentRule = FindNextRule(NEXTRULE, currentRule, id);
			if (currentRule && (*currentRule <= level || !Rejoinder(currentRule))) break;	// matches our level OR is earlier than it (end of a zone like refine of a: into b: zone)
		}
	}
	if (outputRejoinderRuleID == NO_REJOINDER) outputRejoinderRuleID = BLOCKED_REJOINDER; // refine values exist instead of real rejoinders, dont let calling rule do set rejoinder
	if (result == RETRYTOPIC_BIT) // retry the topic of THIS rejoinder, not of the caller
	{
		if (originalTopic != currentTopicID) // it was not us, so need to gambit or rejoinder on this topic that we are in now
		{
			result = InternalCall("^GambitCode", GambitCode, GetTopicName(currentTopicID), (char*)"", NULL, buffer);
		}
	}
	if (updateDisplay && !failed) RestoreDisplay(oldframe->display, locals);

	// finding none does not fail unless told to fail
	if (fail && (!currentRule || level != *currentRule)) result = FAILRULE_BIT;

	RESTOREOLDCONTEXT()

		trace = (modifiedTrace) ? modifiedTraceVal : oldTrace;
	timing = (modifiedTiming) ? modifiedTimingVal : oldTiming;
	return result;
}

static FunctionResult RefineCode(char* buffer)
{
	int i = 1;
	bool fail = false;
	if (!stricmp(ARGUMENT(1), (char*)"FAIL"))
	{
		fail = true;
		++i; // promote any 2nd argument
	}
	FunctionResult result = DoRefine(buffer, ARGUMENT(i), fail, false);
	bool toprule = *currentRule == 's' || *currentRule == 'u' || *currentRule == '?';
	if (result == FAILTOPRULE_BIT && toprule) result = FAILRULE_BIT;
	else if (result == ENDTOPRULE_BIT && toprule) result = ENDRULE_BIT;
	return result;
}

static FunctionResult SequenceCode(char* buffer)
{
	FunctionResult result = DoRefine(buffer, ARGUMENT(1), false, true);
	bool toprule = *currentRule == 's' || *currentRule == 'u' || *currentRule == '?';
	if (result == FAILTOPRULE_BIT && toprule) result = FAILRULE_BIT;
	else if (result == ENDTOPRULE_BIT && toprule) result = ENDRULE_BIT;
	return result;
}

static FunctionResult RejoinderCode(char* buffer)
{
	if (allNoStay) return NOPROBLEM_BIT;// dont do rejoinders when using :verify
	if (postProcessing)
	{
		ReportBug((char*)"Not legal to use ^rejoinder in postprocessing");
		return FAILRULE_BIT;
	}
	if (!unusedRejoinder)
	{
		if (trace & TRACE_TOPIC && CheckTopicTrace()) Log(USERLOG, " disabled rejoinder\r\n\r\n");
		return NOPROBLEM_BIT; //   an earlier response handled this
	}
	unsigned int responseCanceled = responseIndex;
	unsigned int originalResponse = responseIndex;
	char* tag = ARGUMENT(1);
	if (*tag) // 
	{
		int topicid = currentTopicID;
		bool fulllabel = false;
		bool crosstopic = false;
		char* rule;
		char* dot = strchr(tag, '.');
		int id;
		if (dot && IsDigit(dot[1])) rule = GetRuleTag(topicid, id, tag);
		else rule = GetLabelledRule(topicid, tag, (char*)"", fulllabel, crosstopic, id, currentTopicID);
		if (!rule) return FAILRULE_BIT; // unable to find labelled rule 
		// named rule BEFORE rejoinder. FindNextRule will point to 1st rejoinder
		char level = TopLevelRule(rule) ? 'a' : (*rule + 1); // default rejoinder level
		int actualid = id;
		char* ptr = FindNextRule(NEXTRULE, rule, id); 
		if (!ptr || *ptr != level) return FAILRULE_BIT;		// not found

		inputRejoinderRuleID = actualid;
		inputRejoinderTopic = topicid;
	}

	if (inputRejoinderRuleID == NO_REJOINDER)
	{
		if (trace & TRACE_PATTERN && CheckTopicTrace()) Log(USERLOG, "  rejoinder not set ");
		return NOPROBLEM_BIT; // not a failure, just nothing done
	}

	//   we last made a QUESTIONWORD or statement, can his reply be expected for that? 
	FunctionResult result = NOPROBLEM_BIT;
	int pushed = PushTopic(inputRejoinderTopic);
	if (pushed < 0) return FAILRULE_BIT;

	int actualRejoinder = inputRejoinderRuleID + ONE_REJOINDER; // now in rejoinder area

	// get rule after base
	char* ptr = GetRule(inputRejoinderTopic, actualRejoinder);
	if (!ptr)
	{
		if (trace & TRACE_TOPIC && CheckTopicTrace()) Log(USERLOG, " no rejoinder data for topic %s %d.%d\r\n\r\n", GetTopicName(currentTopicID), TOPLEVELID(inputRejoinderRuleID), REJOINDERID(inputRejoinderRuleID));
		if (pushed) PopTopic();
		return result;
	}

	unsigned int oldtrace = EstablishTopicTrace();
	unsigned int oldtiming = EstablishTopicTiming();
	if (trace & TRACE_TOPIC && CheckTopicTrace())
	{
		char label[MAX_WORD_SIZE];
		*label = 0;
		if (*ptr == 'a') // simple 1st level
		{
			*label = '(';
			char* top = GetRule(inputRejoinderTopic, TOPLEVELID(actualRejoinder));
			GetLabel(top, label + 1);
			if (label[1]) strcat(label, ")");
			else *label = 0;
		}
		Log(USERLOG, "  try rejoinder for: %s %d.%d%s", GetTopicName(currentTopicID), TOPLEVELID(inputRejoinderRuleID), REJOINDERID(inputRejoinderRuleID+1), label);
	}

	char level;
	level = *ptr; //   what marks this level
	while (ptr && *ptr) //   loop will search for a level answer it can use
	{
		if (TopLevelRule(ptr)) break; // failed to find rejoinder
		else if (*ptr < level) break;  // end of local choices
		else if (*ptr == level) // check rejoinder
		{
			result = TestRule(actualRejoinder, ptr, buffer);
			if (result == FAILMATCH_BIT || result == FAILTOPRULE_BIT) result = FAILRULE_BIT; // convert 
			if (result == NOPROBLEM_BIT) // we found a match
			{
				unusedRejoinder = false;
				break;
			}
			if (result & (RESTART_BIT | RETRYTOPIC_BIT | RETRYSENTENCE_BIT | FAILTOPIC_BIT | ENDTOPIC_BIT | FAILSENTENCE_BIT | ENDSENTENCE_BIT | ENDINPUT_BIT | RETRYINPUT_BIT | FAILINPUT_BIT)) break;
			if (responseCanceled != responseIndex &&
				result & FAILRULE_BIT)
			{
				responseCanceled = responseIndex;
			}
		}
		ptr = FindNextRule(NEXTRULE, ptr, actualRejoinder); //   wrong or failed responder, swallow this subresponder whole
	}
	if (result == RETRYTOPIC_BIT) // retry the topic of THIS rejoinder, not of the caller who is likely just the control script topic
	{
		if (pushed) // it was not us, so need to gambit or rejoinder on this topic that we are in now
		{
			result = InternalCall("^GambitCode", GambitCode, GetTopicName(currentTopicID), (char*)"", NULL, buffer);
		}
	}
	if (pushed) PopTopic();

	if (inputSentenceCount > 1) // this is the 2nd sentence that failed, give up
	{
		if (trace & TRACE_FLOW) Log(USERLOG, "Clearing input rejoinder on 2nd sentence");
		inputRejoinderRuleID = NO_REJOINDER;
		unusedRejoinder = false;
	}
	trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
	timing = (modifiedTiming) ? modifiedTimingVal : oldtiming;
	// any fresh response generated that gets canceled means we fail
	if (responseCanceled != originalResponse && responseCanceled == responseIndex)
		return FAILRULE_BIT;
	return  result;
}

static FunctionResult RespondCode(char* buffer)
{  // failing to find a responder is not failure unless ask it to be
	char arguments[MAX_ARG_LIMIT + 1][SIZELIM];
	if (planning) return FAILRULE_BIT;	// cannot call from planner
	bool fail = false;
	bool testing = false;
	int oldindent = adjustIndent;

	// if a last argument exists (FAIL) then return failure code if doesnt generate output to user
	unsigned int  i;
	for (i = 1; i <= MAX_ARG_LIMIT; ++i)
	{
		char* a = ARGUMENT(i);
		if (!*a) break; // end
		size_t len = strlen(a);
		if (len > SIZELIM)
		{
			ReportBug((char*)"respond code size limit exceeded to %d for %s\r\n", len, a);
			return FAILRULE_BIT;
		}
		strcpy(arguments[i], a);
		if (!stricmp(a, (char*)"FAIL") && !*ARGUMENT(i + 1))
		{
			fail = true;
			*arguments[i] = 0;
		}
		if (!stricmp(a, (char*)"TEST") && !*ARGUMENT(i + 1)) // last argument
		{
			testing = true;
			*arguments[i] = 0;
		}

	}
	*arguments[i] = 0;
	int oldIndex = responseIndex;
	FunctionResult result = NOPROBLEM_BIT;
	int oldreuseid = currentReuseID;
	int oldreusetopic = currentReuseTopic;
	currentReuseID = currentRuleID; // LOCAL reuse
	currentReuseTopic = currentRuleTopic;
	SAVEOLDCONTEXT()
		char* oldhow = howTopic;
	howTopic = (tokenFlags & QUESTIONMARK) ? (char*)"question" : (char*)"statement";

	for (i = 1; i <= MAX_ARG_LIMIT; ++i)
	{
		// respond(PENDING) means from interesting stack  
		// respond(~name) means use named topic 
		// respond(~) means current topic we are within now
		// respond (word) means topic with that keyword
		char* name = arguments[i];
		if ((oldIndex < responseIndex && result == NOPROBLEM_BIT) || result != NOPROBLEM_BIT || *name == 0) break; // generated an answer or failed or ran out
		oldIndex = responseIndex; // in case answer generated but topic claims failure
		if (trace & TRACE_TOPIC) Log(USERLOG, "respond trying %s\r\n", name);
		char* rule = NULL;
		char* dot = strchr(name, '.'); // tagged?
		if (dot) *dot = 0;
		int topicid = FindTopicIDByName(name); // handles ~ and fully named topics
		if (dot) *dot = '.';
		int id = 0;
		if (dot) // find tagged rule
		{
			bool fulllabel = false;
			bool crosstopic = false;
			if (IsDigit(dot[1])) rule = GetRuleTag(topicid, id, name); // numbered rule
			else rule = GetLabelledRule(topicid, name, (char*)"", fulllabel, crosstopic, id, topicid); // labelled rule
			if (!rule) return FAILRULE_BIT; // unable to find labelled rule 
		}

		// respond(PENDING) means from interesting stack  
		// respond(~name) means use named topic 
		// respond(~) means current topic we are within now
		// respond(word) means topic with that keyword
		oldIndex = responseIndex;
		result = FAILRULE_BIT;

		if (!stricmp(name, (char*)"pending")) // pick topic from pending stack
		{
			unsigned int stack[MAX_TOPIC_STACK + 1];
			memcpy(stack, pendingTopicList, pendingTopicIndex * sizeof(unsigned int)); // copy stack
			int oldPendingIndex = pendingTopicIndex;
			while (oldPendingIndex) // walk topics, most recent first
			{
				int xtopic = stack[--oldPendingIndex];
				char* xname = GetTopicName(currentTopicID); // just for debugging
				if (TopicInUse(topicid) == -1) continue;
				CALLFRAME* frame = ChangeDepth(1, xname);
				currentTopicID = xtopic;
				FunctionResult myresult = PerformTopic(0, buffer);
				ChangeDepth(-1, xname);
				if (myresult & RESULTBEYONDTOPIC)
				{
					result = myresult;
					break;
				}
				if (responseIndex > oldIndex)
				{
					result = myresult;
					break; // we got something
				}
			}
		}

		// do topic by name
		else if (*name == '~')
		{
			if (dot) *dot = 0;
			topicid = FindTopicIDByName(name);
			if (dot) *dot = '.';
			if (!topicid)  return FAILRULE_BIT;
			if (GetTopicFlags(topicid) & TOPIC_BLOCKED)  continue;
			CALLFRAME* frame = ChangeDepth(1, name);
			int pushed = PushTopic(topicid);
			if (pushed < 0)
			{
				ChangeDepth(-1, name);
				return FAILRULE_BIT;
			}
			if (testing) hypotheticalMatch = true;
			result = PerformTopic(0, buffer, rule, id);
			hypotheticalMatch = false;
			ChangeDepth(-1, name);
			if (pushed) PopTopic();

			AddKeep(currentRule);  //   do not allow responders to erase his nest call whether or not he succeeds  BUG ???
			result = (FunctionResult)(result & (-1 ^ (ENDTOPIC_BIT | ENDRULE_BIT))); // these are swallowed
		}

		// do topic by keyword
		else
		{
			WORDP D = FindWord(name);
			FACT* F = NULL;
			if (!D) result = NOPROBLEM_BIT;
			else  F = GetSubjectNondeadHead(D);
			while (F) // find topics word is a direct member of
			{
				if (F->verb == Mmember)
				{
					WORDP E = Meaning2Word(F->object);
					if (E->internalBits & TOPIC)
					{
						int xtopic = FindTopicIDByName(E->word);
						if (xtopic && !(GetTopicFlags(xtopic) & (TOPIC_BLOCKED | TOPIC_SYSTEM | TOPIC_NOGAMBITS)))
						{
							CALLFRAME* frame = ChangeDepth(1, E->word);
							int pushed = PushTopic(xtopic);
							if (pushed < 0)
							{
								result = FAILRULE_BIT;
								ChangeDepth(-1, E->word);
								break;
							}
							result = PerformTopic(0, buffer);
							ChangeDepth(-1, E->word);
							if (pushed) PopTopic();
							if (result & RESULTBEYONDTOPIC) break;
							if (responseIndex > oldIndex)
							{
								result = NOPROBLEM_BIT;
								break;
							}
						}
					}
				}
				F = GetSubjectNondeadNext(F);
			}

			AddKeep(currentRule);  //   do not allow responders to erase his nest call whether or not he succeeds  BUG ???
			result = (FunctionResult)(result & (-1 ^ (ENDTOPIC_BIT | ENDRULE_BIT))); // these are swallowed
		}
		if (result & RESULTBEYONDTOPIC) break;
	}
	RESTOREOLDCONTEXT()
		currentReuseID = oldreuseid;
	currentReuseTopic = oldreusetopic;
	howTopic = oldhow;
	if (adjustIndent != oldindent)
	{
		adjustIndent = oldindent;
	}

	if (fail && responseIndex <= oldIndex && result == NOPROBLEM_BIT)  result = FAILRULE_BIT; // report failure
	return result;
}

void ResetReuseSafety()
{
	memset(reuseSafety, 0, sizeof(reuseSafety));
	memset(reuseSafetyCount, 0, sizeof(reuseSafetyCount));
	reuseIndex = 0;
}

FunctionResult RegularReuse(int topicid, int id, char* rule, char* buffer, char* arg3, bool crosstopic)
{
	bool found = false;
	int locator = 0;
	for (locator = 0; locator < reuseIndex; ++locator)
	{
		if (reuseSafety[locator] == rule)
		{
			found = true;
			if (++reuseSafetyCount[locator] > 10)
			{
				char c = rule[30];
				rule[30] = 0;
				ReportBug((char*)"INFO: Recursive reuse %s", rule);
				rule[30] = c;
				return FAILRULE_BIT;
			}
			else break;
		}
	}
	if (!found)
	{
		if (reuseIndex == (MAX_REUSE_SAFETY - 1)) reuseIndex = 0;
		locator = reuseIndex++;
		reuseSafetyCount[locator] = 1;
		reuseSafety[locator] = rule;
	}

	int oldreuseid = currentReuseID;
	unsigned int oldreusetopic = currentReuseTopic;

	currentReuseID = currentRuleID; // LOCAL reuse
	currentReuseTopic = currentRuleTopic;

	// execute rule 
	SAVEOLDCONTEXT()

		currentRule = rule;
	currentRuleID = id;
	currentRuleTopic = currentTopicID = topicid;
	CALLFRAME* oldframe = GetCallFrame(globalDepth);
	oldframe->display = (char**)stackFree;

	unsigned int oldTrace = EstablishTopicTrace();
	unsigned int oldTiming = EstablishTopicTiming();
	char* locals = GetTopicLocals(currentTopicID);
	bool updateDisplay = locals && DifferentTopicContext(0, currentTopicID);
	int holdindex = responseIndex;
	FunctionResult result = FAILRULE_BIT;

	bool failed = false;

	if (updateDisplay && !InitDisplay(locals)) failed = true;

	if (!failed) result = ProcessRuleOutput(currentRule, currentRuleID, buffer);

	if (updateDisplay && !failed) RestoreDisplay(oldframe->display, locals);

	if (crosstopic && responseIndex > holdindex) AddPendingTopic(topicid); // restore caller topic as interesting

	RESTOREOLDCONTEXT()
		currentReuseID = oldreuseid;
	currentReuseTopic = oldreusetopic;

	trace = (modifiedTrace) ? modifiedTraceVal : oldTrace;
	timing = (modifiedTiming) ? modifiedTimingVal : oldTiming;

	if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, ""); //   restore index from lower level
	if (!result && holdindex == responseIndex && !stricmp(arg3, (char*)"FAIL")) return FAILRULE_BIT; // user wants notification of failure
	if (--reuseSafetyCount[locator] == 0) // decrement use count, no need to track any more if goes to 0
	{
		reuseSafetyCount[locator] = reuseSafetyCount[--reuseIndex];
		reuseSafety[locator] = reuseSafety[reuseIndex];
	}
	return result;
}

static FunctionResult ReuseCode(char* buffer)
{
	int id = 0;
	char* arg1 = ARGUMENT(1); // label of rule or topic.label
	if (!*arg1) return FAILRULE_BIT;
	CALLFRAME* frame = GetCallFrame(globalDepth);
	int topicid = currentTopicID;
	bool fulllabel = false;
	bool crosstopic = false;
	char* arg2 = ARGUMENT(2); // optional- if there not allowed to use erased rules
	char* arg3 = ARGUMENT(3); // possible fail value
	if (!stricmp(arg2, (char*)"FAIL")) // make it 3rd argument if it exists
	{
		char* val = ARGUMENT(2);
		frame->arguments[2]= ARGUMENT(3);
		frame->arguments[3] = val;
	}

	char* rule;
	char* dot = strchr(arg1, '.');
	if (dot && IsDigit(dot[1])) rule = GetRuleTag(topicid, id, arg1);
	else rule = GetLabelledRule(topicid, arg1, arg2, fulllabel, crosstopic, id, currentTopicID);
	if (!rule) return FAILRULE_BIT; // unable to find labelled rule that is available 

	return RegularReuse(topicid, id, rule, buffer, arg3, crosstopic);
}

static FunctionResult AvailableCode(char* buffer)
{
	int id = 0;
	char* arg1 = ARGUMENT(1); // label of rule or topic.label
	if (!*arg1) return FAILRULE_BIT;

	int topicid = currentTopicID;
	bool fulllabel = false;
	bool crosstopic = false;
	char* rule;
	char* dot = strchr(arg1, '.');
	if (dot && IsDigit(dot[1])) rule = GetRuleTag(topicid, id, arg1);
	else rule = GetLabelledRule(topicid, arg1, (char*)"", fulllabel, crosstopic, id, currentTopicID);
	if (!rule) return FAILRULE_BIT; // unable to find labelled rule 
	unsigned int result = UsableRule(topicid, id);
	if (!result && !stricmp(ARGUMENT(2), (char*)"FAIL")) return FAILRULE_BIT; // user wants notification of failure
	sprintf(buffer, (char*)"%u", result);
	return NOPROBLEM_BIT;
}

static FunctionResult SetRejoinderCode(char* buffer)
{ // types: input, output, copy are obsolete
	if (planning) return NOPROBLEM_BIT; // canot rejoinder inside a plan
	bool input = false;
	bool output = true;
	char* tag = ARGUMENT(1); // kind of rejoinder or only 1 arg
	char* where = ARGUMENT(2); // to where if 2 args given
	if (!stricmp(where, (char*)"null")) where = ""; // clear to null
	
	if (!*ARGUMENT(2)) { where = tag; } // single arg given
	else if (!stricmp(tag, (char*)"input"))
	{
		input = true;
		output = false;
	}
	else if (!stricmp(tag, (char*)"output") || !stricmp(tag, (char*)"copy")) { ; }
	else return FAILRULE_BIT;

	if (!*where && output) // disable rejoinder
	{
		outputRejoinderRuleID = outputRejoinderTopic = NO_REJOINDER;
		return NOPROBLEM_BIT;
	}
	if (!*where && input) // disable rejoinder
	{
		inputRejoinderRuleID = inputRejoinderTopic = NO_REJOINDER;
		return NOPROBLEM_BIT;
	}

	int topicid = currentTopicID;
	bool fulllabel = false;
	bool crosstopic = false;
	char* rule;
	char* dot = strchr(where, '.');
	int id;
	if (dot && IsDigit(dot[1])) rule = GetRuleTag(topicid, id, where);
	else rule = GetLabelledRule(topicid, where, (char*)"", fulllabel, crosstopic, id, currentTopicID);
	if (!rule) return FAILRULE_BIT; // unable to find labelled rule 

	char label[MAX_WORD_SIZE];
	*label = '(';
	GetLabel(rule, label + 1);
	strcat(label, ")");
	if (*label == ')') *label = 0;

	// when you give us a rule, we  use AFTER it (1st rejoinder)
	// confirm the request was feasible
	int newid = id;
	char level = TopLevelRule(rule) ? 'a' : (*rule + 1); // default rejoinder level
	char* ptr = FindNextRule(NEXTRULE, rule, newid);
	if (!ptr || *ptr != level) return FAILRULE_BIT;		// not found

	if (input)
	{
		inputRetryRejoinderRuleID = inputRejoinderRuleID = id;
		inputRetryRejoinderTopic = inputRejoinderTopic = topicid;
	}
	else 
	{
		outputRejoinderRuleID = id;
		outputRejoinderTopic = topicid;
	}
	if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, "  **set  rejoinder at %s.%d.%d%s\r\n", GetTopicName(topicid), TOPLEVELID(id), REJOINDERID(id), label);
	return NOPROBLEM_BIT;
}

static FunctionResult TopicFlagsCode(char* buffer)
{
	sprintf(buffer, (char*)"%d", GetTopicFlags(FindTopicIDByName(ARGUMENT(1))));
	return NOPROBLEM_BIT;
}

//////////////////////////////////////////////////////////
/// TOPIC LISTS
//////////////////////////////////////////////////////////
void SET_FACTSET_COUNT(int x, int y)
{
	factSet[x][0] = (FACT*)((uint64)y);
	if (!y) factSetNext[x] = 0; // all count resets require next scan reset
}

static FunctionResult GetTopicsWithGambitsCode(char* buffer)
{
	unsigned int store = (impliedSet == ALREADY_HANDLED) ? 0 : impliedSet;
	SET_FACTSET_COUNT(store, 0);
	*buffer = 0;

	for (unsigned int topicid = 1; topicid <= numberOfTopics; ++topicid)
	{
		if (topicid == (unsigned int)currentTopicID || HasGambits(topicid) <= 0) continue;
		if (GetTopicFlags(topicid) & TOPIC_NOGAMBITS) continue;	// dont use this
		char* name = GetTopicName(topicid);
		if (!*name) continue; // deactivated by faketopic
		MEANING T = MakeMeaning(StoreWord(name));
		FACT* F = CreateFact(T, MgambitTopics, T, FACTTRANSIENT | FACTDUPLICATE);
		AddFact(store, F);
	}
	if (impliedSet == ALREADY_HANDLED && FACTSET_COUNT(store) == 0) return FAILRULE_BIT;
	impliedSet = impliedWild = ALREADY_HANDLED;
	currentFact = NULL;
	return NOPROBLEM_BIT;
}

static int OrderTopics(unsigned short topicList[MAX_TOPIC_KEYS], unsigned int matches[MAX_TOPIC_KEYS]) // find other topics that use keywords
{
	bool newpass = topicList[1] != 0;
	unsigned int max = 2;
	unsigned int index = 0;
	unsigned int i;
	char currentTopic[MAX_WORD_SIZE];
	GetActiveTopicName(currentTopic); // current topic, not system or nostay.
	int baseid = FindTopicIDByName(currentTopic);

	//  value on each topic
	for (i = 1; i <= numberOfTopics; ++i) // value 0 means we havent computed it yet. Value 1 means it has been erased.
	{
		if (i == (unsigned int)baseid || BlockedBotAccess(i)) continue;

		char* name = GetTopicName(i);
		if (!*name) continue; // hidden topic
		unsigned int val = topicList[i];
		if (!val) //   compute match value
		{
			char word[MAX_WORD_SIZE];
			strcpy(word, name);
			char* dot = strchr(word + 1, DUPLICATETOPICSEPARATOR);
			if (dot) *dot = 0;	// use base name of the topic, not topic family name.
			WORDP D = FindWord(word); //   go look up the ~word for it
			if (!D) continue; // topic not found -- shouldnt happen

			// Note- once we have found an initial match for a topic name, we dont want to match that name again...
			// E.g., we have a specific topic for a bot, and later a general one that matches all bots. We dont want that later one processed.
			if (D->inferMark == inferMark) continue;	// already processed a topic of this name
			D->inferMark = inferMark;

			//   look at references for this topic
			int start = -1;
			unsigned int startPosition = 0;
			unsigned int endPosition = 0;
			while (GetIthSpot(D, ++start, startPosition, endPosition)) // find matches in sentence
			{
				// value of match of this topic in this sentence
				for (unsigned int k = startPosition; k <= endPosition; ++k)
				{
					if (trace & TRACE_PATTERN && CheckTopicTrace()) Log(USERLOG, "%s->%s ", wordStarts[k], word);
					val += 10 + strlen(wordStarts[k]);   // each hit gets credit 10 and length of word as subcredit
					if (!stricmp(wordStarts[k], word + 1) || (wordCanonical[k] && !stricmp(wordCanonical[k], word + 1))) val += 20; //  exact hit on topic name
				}
				if (endPosition < startPosition) // phrase subcomponent
				{
					if (trace & TRACE_PATTERN && CheckTopicTrace())  Log(USERLOG, "%s->%s", wordStarts[startPosition], word);
					val += 10;
				}
			}

			//   Priority modifiers

			char priority = ' ';
			if (GetTopicFlags(i) & TOPIC_PRIORITY && val)
			{
				priority = '+';
				val *= 3; //  raise its value
			}
			else if (GetTopicFlags(i) & TOPIC_LOWPRIORITY && val)
			{
				priority = '-';
				val /= 3; // lower its value
			}

			topicList[i] = (unsigned short)(val + 1); //  1 means we did compute it, beyond that is value
			if (trace & TRACE_PATTERN && val > 1 && CheckTopicTrace()) Log(USERLOG, "%c(%d) ", priority, topicList[i]);
		} //   close if

		if (val >= max) //  find all best
		{
			if (val > max) // new high value
			{
				max = val;
				index = 0;
			}
			matches[++index] = i;
		}
	}
	if (trace & TRACE_PATTERN && newpass && CheckTopicTrace()) Log(USERLOG, "\r\n");
	matches[0] = max;
	return index;
}

FunctionResult KeywordTopicsCode(char* buffer)
{	//   find  topics best matching his input words - never FAILS but can return 0 items stored
	unsigned short topicList[MAX_TOPIC_KEYS];
	memset(topicList, 0, MAX_TOPIC_KEYS * sizeof(short));

	int set = (impliedSet == ALREADY_HANDLED) ? 0 : impliedSet;
	SET_FACTSET_COUNT(set, 0);

	bool onlyGambits = (!stricmp(ARGUMENT(1), (char*)"gambit"));

	//   now consider topics in priority order
	unsigned int index;
	unsigned int matches[MAX_TOPIC_KEYS];
	NextInferMark();
	while ((index = OrderTopics(topicList, matches))) //   finds all at this level. 1st call evals topics. other calls just retrieve.
	{
		//   see if equally valued topics found are feasible, if so, return one chosen at random
		while (index) // items are 1-based
		{
			unsigned int which = random(index) + 1;
			int topicid = matches[which];
			topicList[topicid] = 1;
			matches[which] = matches[index--]; // swap equally valued end back to fill in position

			unsigned int flags = GetTopicFlags(topicid);
			if (onlyGambits && (flags & TOPIC_SYSTEM || !HasGambits(topicid))) continue;

			char word[MAX_WORD_SIZE];
			strcpy(word, GetTopicName(topicid, true));
			if (impliedSet == ALREADY_HANDLED) // just want one
			{
				strcpy(buffer, word);
				break;
			}

			char value[100];
			sprintf(value, (char*)"%u", matches[0]);
			MEANING M = MakeMeaning(StoreWord(word));
			AddFact(set, CreateFact(M, Mkeywordtopics, MakeMeaning(StoreWord(value)), FACTTRANSIENT | FACTDUPLICATE));
		}
	}
	if (impliedSet == ALREADY_HANDLED && FACTSET_COUNT(set) == 0) return FAILRULE_BIT;
	impliedSet = ALREADY_HANDLED;
	currentFact = NULL;
	return NOPROBLEM_BIT;
}

static FunctionResult PendingTopicsCode(char* buffer)
{
	int set = GetSetID(ARGUMENT(1));
	if (set == ILLEGAL_FACTSET) set = impliedSet;
	if (set == ILLEGAL_FACTSET) return FAILRULE_BIT;
	PendingTopics(set);
	impliedSet = ILLEGAL_FACTSET;
	return NOPROBLEM_BIT;
}

static FunctionResult QueryTopicsCode(char* buffer)
{
	if (impliedSet == ALREADY_HANDLED) // not in assignment
	{
		QueryTopicsOf(ARGUMENT(1), 0, NULL);
		return (FACTSET_COUNT(0)) ? NOPROBLEM_BIT : FAILRULE_BIT;
	}
	return QueryTopicsOf(ARGUMENT(1), impliedSet, NULL);
}

//////////////////////////////////////////////////////////
/// MARKINGS
//////////////////////////////////////////////////////////

static FunctionResult MarkCode(char* buffer)
{
	// argument3 might be a text string "SINGLE". Past that,
	// argument1 is a word or ~set or missing entirely
	// mark()  flip off generic unmarks 
	// argument 2 is a location designator or * or missing entirely
	// mark(word) enables mark at location 1 
	// mark(word _xxx) enable word mark at start location of _xxx variable 
	// mark(word  1) enables mark at specified location if within range of input
	// mark(* whatever) turns on any turned off global marking at the range of the given location
	char* ptr = ARGUMENT(1);
	changedNL = true;
	FunctionResult result;
	char word[MAX_WORD_SIZE];
	bool single = false;
	ptr = ReadShortCommandArg(ptr, word, result); // what is being marked
	if (result & ENDCODES) return result;
	
	if (!*word) // mark() remove all generic unmarks
	{
		if (!oldunmarked[255]) // cache current disables
		{
			memcpy(oldunmarked, unmarked, MAX_SENTENCE_LENGTH);
			oldunmarked[255] = 1;
		}
		memset(unmarked, 0, MAX_SENTENCE_LENGTH); // clear all mark suppression
#ifndef DISCARDTESTING
		if (debugMark)
		{
			(*debugMark)("+all", "all");
		}
#endif
		return NOPROBLEM_BIT;
	}

    // get location
    unsigned int startPosition = wordCount;
	unsigned int endPosition = 1;
	char var2[MAX_WORD_SIZE];
	if (*ptr == '^') // convert function or function arg to value
	{
		ptr = GetPossibleFunctionArgument(ptr, var2); // get the request
		char word2[MAX_WORD_SIZE];
		if (!IsDigit(*var2) && *var2 != '_') // numbers and match variables do not eval here
		{
			strcpy(word2, var2);
			if (!*word2) return NOPROBLEM_BIT;
			GetCommandArg(word2, var2, result, 0); // should be small
		}
	}
	else ptr = ReadCompiledWord(ptr, var2);
	if (IsDigit(*var2) || *var2 == '_');  // the locator, leave it unevaled as number or match var
	else if (*var2)
	{
		strcpy(buffer, var2);
		GetCommandArg(buffer, var2, result, 0); // evaluate the locator as a number presumably
	}
	if (*var2 == USERVAR_PREFIX) strcpy(var2, GetUserVariable(var2, false));

	char flag[MAX_WORD_SIZE];
	ReadCompiledWord(ptr, flag);
	bool doall = true;
	if (*flag == USERVAR_PREFIX) strcpy(flag, GetUserVariable(flag, false));
	if (!stricmp(flag, "ALL")) doall = true;
	else if (!stricmp(flag, "ONE")) doall = false;
	*buffer = 0;

	if (!*var2 || *var2 == ')')
    {
        if (*word == '*') // mark(*)
        {
            startPosition = 1;
            endPosition = wordCount;
        }
        else startPosition = endPosition = 1;
    }
	else if (IsDigit(*var2))
	{
		int val = atoi(var2);
		startPosition = val & 0x0000ffff;
		endPosition = val >> 16;
	}
	else if (*var2 == '_') //  wildcard position designator
	{
		startPosition = WILDCARD_START(wildcardPosition[GetWildcardID(var2)]); // the match location
		endPosition = WILDCARD_END_ONLY(wildcardPosition[GetWildcardID(var2)]);
	}
	else return FAILRULE_BIT;

	if (endPosition == 0) endPosition = startPosition; // SHARED
	if (startPosition < 1 || startPosition > wordCount) return NOPROBLEM_BIT; // fail silently
	if (endPosition > wordCount)  endPosition = wordCount;

	if (*word == '*') // enable all - mark (* _0)
	{
		if (trace & TRACE_OUTPUT) Log(USERLOG, "mark * ");
		unsigned int i;
		for (i = startPosition; i <= endPosition; ++i)
		{
			if (showMark) Log(ECHOUSERLOG, (char*)"Mark * %d(%s)\r\n", i, wordStarts[i]);
			if (trace & TRACE_FLOW) Log(USERLOG, "%d(%s) ", i, wordStarts[i]);
			unmarked[i] = 0;
		}
		for ( i = 1; i <= MAX_SENTENCE_LENGTH; ++i)
		{
			if (unmarked[i]) break; // still some
		}
		if (i > MAX_SENTENCE_LENGTH) unmarked[0] = 0; // flag showing we have no unmarked
#ifndef DISCARDTESTING
		if (debugMark)
		{
			char posn[MAX_WORD_SIZE];
			sprintf(posn, "+%u-%u", startPosition, endPosition);
			(*debugMark)(posn, "*");
		}
#endif
		if (trace & TRACE_FLOW) Log(USERLOG, "\r\n");
		return NOPROBLEM_BIT;
	}
	if ((strstr(ptr, "SINGLE") || strstr(ptr, "single"))) single = true;

	// might be rawcase only
	int kind = FIXED;
	char* at = word;
	if (*at == '\'')
	{
		at++;
		if (*at == '\'')
		{
			at++;
			kind = RAWCASE;
		}
	}
	// Mark specific thing 
	WORDP D = StoreWord(at);
	MEANING M = MakeMeaning(D);
	if (*D->word != '~') Add2ConceptTopicList(concepts, D, startPosition, endPosition, true); // add ordinary word to concept list directly as WordHit will not store anything but concepts
	if (showMark || (trace & TRACE_PREPARE)) Log(ECHOUSERLOG, (char*)"Mark %s: \r\n", D->word);
	if (trace & TRACE_OUTPUT) Log(USERLOG, "mark %s...%s  @word %s %d-%d ", wordStarts[startPosition], wordStarts[endPosition], D->word, startPosition, endPosition);
	if (doall) MarkMeaningAndImplications(0, 0, M, startPosition, endPosition, kind, false, single);
	else
	{
		MarkWordHit(0, EXACTNOTSET, D, 0, startPosition, endPosition);
		FACT* F = GetSubjectNondeadHead(D);
		while (F)
		{
			if (F->verb == Mmember)
			{
				WORDP X = Meaning2Word(F->object);
				if (*(X->word) == '~' && IsDigit(X->word[1])) // internal concept
					MarkWordHit(0, EXACTNOTSET, X, 0, startPosition, endPosition);
			}
			F = GetSubjectNondeadNext(F);
		}
	}
	if (showMark) Log(ECHOUSERLOG, (char*)"------\r\n");
#ifndef DISCARDTESTING
	if (debugMark)
	{
		char posn[MAX_WORD_SIZE];
		sprintf(posn, "+%u-%u", startPosition, endPosition);
		(*debugMark)(posn, D->word);
	}
#endif

	return NOPROBLEM_BIT;
}

static FunctionResult MarkedCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	if (*ARGUMENT(1) == USERVAR_PREFIX)  // indirect thru variable
	{
		char* at = GetUserVariable(ARGUMENT(1), false);
		if (at) arg1 = at;
	}

	WORDP D = FindWord(arg1);
	if (!D) return FAILRULE_BIT;
	if (!GetNextSpot(D, 0)) return FAILRULE_BIT;
	strcpy(buffer, (char*)"1");
	return NOPROBLEM_BIT;
}

static FunctionResult PositionCode(char* buffer)
{
	char* ptr = ARGUMENT(1);
	FunctionResult result;
	char word[MAX_WORD_SIZE];
	ptr = ReadShortCommandArg(ptr, word, result); // start or end
	if (result & ENDCODES) return result;

	ptr = GetPossibleFunctionArgument(ptr, buffer);  // the _ var
	if (*buffer == '\'') memmove(buffer, buffer + 1, strlen(buffer));
	if (*buffer == '_') //  wildcard position designator
	{
		unsigned int wild = wildcardPosition[GetWildcardID(buffer)];
		unsigned int start = WILDCARD_START(wild);
		unsigned int end = WILDCARD_END_ONLY(wild);
		unsigned int subject = WILDCARD_END(wild) >> 8;
		if (!stricmp(word, (char*)"start")) sprintf(buffer, (char*)"%u", start);  // the match location
		else if (!stricmp(word, (char*)"end")) sprintf(buffer, (char*)"%u", end);
		else if (!stricmp(word, (char*)"both")) sprintf(buffer, (char*)"%u", start | (end << 16));
		else if (!stricmp(word, (char*)"subject"))
		{
			if (!subject) return FAILRULE_BIT; // not a fundamental meaning match
			if (subject == 255) subject = 0; // none
			sprintf(buffer, (char*)"%u", subject);
		}
		else if (!stricmp(word, (char*)"verb"))
		{
			if (!subject) return FAILRULE_BIT; // not a fundamental meaning match
			sprintf(buffer, (char*)"%u", start);
		}
		else if (!stricmp(word, (char*)"object"))
		{
			if (!subject) return FAILRULE_BIT; // not a fundamental meaning match
			if (end == start) end = 0;
			sprintf(buffer, (char*)"%u", end);
		}
		else return FAILRULE_BIT;
	}
	else return FAILRULE_BIT;
	return NOPROBLEM_BIT;
}

static FunctionResult SetPositionCode(char* buffer)
{
	char* ptr = ARGUMENT(1);
	char word[MAX_WORD_SIZE];

	if (*ptr == '_') // set match var position
	{
		ptr = ReadCompiledWord(ptr, word);
		int n = GetWildcardID(word);
		if (n < 0) return FAILRULE_BIT;

		char startw[MAX_WORD_SIZE];
		FunctionResult result;

		ptr = SkipWhitespace((char*)ptr);
		char* ptr2 = ptr;

		unsigned int start = 0, end = 0;
		if (*ptr == '_') // set from other wildcard
		{
			ptr = ReadCompiledWord(ptr, startw);
			int n2 = GetWildcardID(startw);
			if (n2 < 0) return FAILRULE_BIT;

			start = WILDCARD_START(wildcardPosition[n2]);
			if (start > 0)
			{
				end = WILDCARD_END(wildcardPosition[n2]);

				char mode[MAX_WORD_SIZE];
				ptr = ReadShortCommandArg(ptr, mode, result); // based on the orginal size?
				if (!stricmp(mode, "original"))
				{
					int originalStart = derivationIndex[start] >> 8; // from here
					int originalEnd = (derivationIndex[end] & 0x00ff);  // to here including here  The end may be beyond
					start = 0;
					end = 0;
					for (unsigned int i = 1; i <= wordCount; ++i) // leaf thru actual words to find ranges covered
					{
						// actual word is from this range in original
						int a = derivationIndex[i] >> 8; // from here
						int b = (derivationIndex[i] & 0x00ff);  // to here including here  The end may be beyond wordCount if words have been deducted by now

						if (a <= originalEnd && b >= originalStart) // our desired range (start - end) overlaps the word range (a - b)
						{
							if (!start) start = i;  // starting actual word in range
							end = i;     // maximal ending actual word in range
						}
					}
				}
			}
		}

		if (start == 0 && end == 0)
		{
			ptr = ptr2;
			ptr = ReadShortCommandArg(ptr, startw, result); // what is being marked
			if (result != NOPROBLEM_BIT) return result;
			start = atoi(startw);
			char endw[MAX_WORD_SIZE];
			ptr = ReadShortCommandArg(ptr, endw, result); // what is being marked
			if (result != NOPROBLEM_BIT) return result;
			end = atoi(endw);
		}
		if (start >= (wordCount + 1)) start = wordCount + 1;
		if (start <= 0) start = 0;

		if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, "_%d = %d...%d ", n, start, end);

		if (end <= 0) end = 0;
		if ((end & REMOVE_SUBJECT) >= (wordCount + 1)) end = wordCount + 1;
		wildcardPosition[n] = start | WILDENDSHIFT(end);
	}
	else return FAILRULE_BIT;// set GLOBAL position -- unused at present
	return NOPROBLEM_BIT;
}

static FunctionResult CapitalizedCode(char* buffer)
{
	if (IsDigit(*ARGUMENT(1)))
	{
		unsigned int n = atoi(ARGUMENT(1));
		if (n == 0 || n > wordCount) return FAILRULE_BIT;
		strcpy(buffer, (capState[n]) ? (char*)"1" : (char*)"0");
	}
	else if (IsAlphaUTF8(*ARGUMENT(1))) strcpy(buffer, (IsUpperCase(*ARGUMENT(1))) ? (char*)"1" : (char*)"0");
	else return FAILRULE_BIT;
	return NOPROBLEM_BIT;
}

static FunctionResult RoleCode(char* buffer)
{
	unsigned int n = 0;
	FunctionResult result = NOPROBLEM_BIT;
	char* arg = ARGUMENT(1);
	if (*arg == '\'') memmove(arg, arg + 1, strlen(arg));
	if (IsDigit(*arg))
	{
		n = atoi(arg);
	}
	else if (*arg == '_') n = WildStartPosition(arg);
	else if (*arg == USERVAR_PREFIX) n = atoi(GetUserVariable(arg, false));
	else if (*arg == '^')
	{
		ReadArgument(arg, buffer, result);
		n = atoi(buffer);
	}
	else return FAILRULE_BIT;
	if (n == 0 || n > wordCount) return FAILRULE_BIT;
	sprintf(buffer, (char*)"%u", (unsigned int)roles[n]);
	return result;
}

static char* tokenValues[] = {
	(char*)"DO_ESSENTIALS",(char*)"DO_SUBSTITUTES",(char*)"DO_CONTRACTIONS",(char*)"DO_INTERJECTIONS",(char*)"DO_BRITISH", "DO_SPELLING", "DO_TEXTING", "DO_NOISE",
	(char*)"DO_PRIVATE",(char*)"DO_NUMBER_MERGE",(char*)"DO_PROPERNAME_MERGE",(char*)"DO_SPELLCHECK",(char*)"DO_INTERJECTION_SPLITTING",(char*)"DO_POSTAG/PRESENT",(char*)"DO_PARSE/PAST",(char*)"NO_IMPERATIVE/FUTURE",
	(char*)"NO_WITHIN/PRESENT_PERFECT",(char*)"DO_DATE_MERGE/CONTINUOUS",(char*)"NO_SENTENCE_END/PERFECT",(char*)"NO_INFER_QUESTION/PASSIVE",
	(char*)"NO_HYPHEN_END",(char*)"NO_COLON_END",(char*)"NO_SEMICOLON_END",(char*)"STRICT_CASING",
	(char*)"ONLY_LOWERCASE",(char*)"TOKEN_AS_IS",(char*)"SPLIT_QUOTE",(char*)"LEAVE_QUOTE",(char*)"IMPLIED_SUBJECT",(char*)"QUESTIONMARK",(char*)"EXCLAMATIONMARK",(char*)"PERIODMARK",
	(char*)"USERINPUT",(char*)"COMMANDMARK",(char*)"IMPLIED_YOU",(char*)"FOREIGN_TOKENS",(char*)"FAULTY_PARSE",(char*)"QUOTATION",(char*)"NOT_SENTENCE",
	(char*)"NO_PROPER_SPELLCHECK",(char*)"NO_LOWERCASE_PROPER_MERGE",(char*)"DO_SPLIT_UNDERSCORE",(char*)"MARK_LOWER",(char*)"NO_FIX_UTF",(char*)"NO_CONDITIONAL_IDIOM",
	(char*)"",(char*)"JSON_DIRECT_FROM_OOB"
};

static FunctionResult DecodeInputTokenCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	int64 n;
	ReadInt64(arg1, n);
	uint64 bit = 1;
	int index = 0;
	int numTokens = sizeof(tokenValues) / sizeof(tokenValues[0]); // bit of crash protection
	while (n)
	{
		if (bit & n)
		{
			strcat(buffer, tokenValues[index]);
			n ^= bit;
			if (n > 0) strcat(buffer, (char*)", ");
		}
		bit <<= 1;
		++index;
		if (index >= numTokens) break;
	}
	return NOPROBLEM_BIT;
}

static FunctionResult DecodePosCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	char* arg = ARGUMENT(2);
	int64 n;
	ReadInt64(arg, n);
	if (!stricmp(arg1, (char*)"pos")) DecodeTag(buffer, n, 0, 0);
	else strcpy(buffer, GetRole(n));
	return NOPROBLEM_BIT;
}

static FunctionResult PartOfSpeechCode(char* buffer)
{
	FunctionResult result = NOPROBLEM_BIT;
	unsigned int n = 0;
	char* arg = ARGUMENT(1);
	if (*arg == '\'') memmove(arg, arg + 1, strlen(arg));
	if (IsDigit(*arg))
	{
		n = atoi(arg);
		if (n == 0 || n > wordCount) return FAILRULE_BIT;
		strcpy(buffer, (capState[n]) ? (char*)"1" : (char*)"0");
	}
	else if (*arg == '_')  n = WildStartPosition(arg);
	else if (*arg == '^')
	{
		ReadArgument(arg, buffer, result);
		n = atoi(buffer);
	}
	else if (*arg == USERVAR_PREFIX) n = atoi(GetUserVariable(arg, false));
	else return FAILRULE_BIT;
	uint64 pos = finalPosValues[n];
	if (pos & (AUX_VERB | ADJECTIVE_PARTICIPLE)) pos |= allOriginalWordBits[n] & VERB_BITS; // supllementatal data
	else if (pos & ADJECTIVE_NORMAL && allOriginalWordBits[n] & ADJECTIVE_PARTICIPLE) pos |= allOriginalWordBits[n] & VERB_BITS; // supllementatal data
	else if (pos & ADJECTIVE_NOUN) pos |= allOriginalWordBits[n] & NORMAL_NOUN_BITS;
	strcpy(buffer, PrintU64(pos));
	return result;
}

static FunctionResult KeepHistoryCode(char* buffer)
{
	int count = atoi(ARGUMENT(2));
	if (count >= (MAX_USED - 1)) count = MAX_USED - 1;
	if (!stricmp(ARGUMENT(1), (char*)"BOT"))
	{
		if (count == 0) *chatbotSaid[0] = 0;
		if (count < chatbotSaidIndex)  chatbotSaidIndex = count;
	}
	if (!stricmp(ARGUMENT(1), (char*)"USER"))
	{
		if (count == 0)  *humanSaid[0] = 0;
		if (count < humanSaidIndex) humanSaidIndex = count;
	}

	return NOPROBLEM_BIT;
}

static FunctionResult GetTagCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	unsigned int n = atoi(arg1);
	if (n < 1 || n > wordCount) return FAILRULE_BIT;
	strcpy(buffer, wordTag[n]->word);
	return NOPROBLEM_BIT;
}

static FunctionResult SetTagCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	unsigned int n = atoi(arg1);
	if (n < 1 || n > wordCount) return FAILRULE_BIT;
	char* arg2 = ARGUMENT(2);
	if (*arg2 != '~') return FAILRULE_BIT;

	changedNL = true;

#ifdef TREETAGGER
	if (wordTag[n])
	{
		char oldtag[MAX_WORD_SIZE];
		strcpy(oldtag, wordTag[n]->word);
		*oldtag = '_';
		WORDP Y = FindWord(oldtag);
		if (Y) posValues[n] ^= Y->properties;  // remove old pos tag values
	}
#endif	
	wordTag[n] = StoreWord(arg2);
#ifdef TREETAGGER
	* arg2 = '_';
	WORDP X = FindWord(arg2);
	if (X) posValues[n] |= X->properties; // new english pos tag references
#endif
	return NOPROBLEM_BIT;
}

static FunctionResult WordInfoCode(char* buffer)
{
	char* word = ARGUMENT(1);
	WORDP D = FindWord(word);
	if (!D) return FAILRULE_BIT;

	WORDP entry = NULL;
	WORDP canonical = NULL;
	uint64 xflags = 0;
	WORDP revise;
	uint64 cansysflags = 0;
	uint64 inferredProperties = GetPosData((unsigned int)-1, word, revise, entry, canonical, xflags, cansysflags);
	char* revised = (entry) ? entry->word : (char*)"";
	if (D->systemFlags & HAS_SUBSTITUTE)
	{
		if (GetSubstitute(D))	sprintf(buffer, "SUBSTITUTION->%s ", GetSubstitute(D)->word);
		else strcpy(buffer, "self-erases");
	}
	strcat(buffer, "properties:");
	buffer += strlen(buffer);
	uint64 bit = START_BIT;
	while (bit)
	{
		if (D->properties & bit)
		{
			sprintf(buffer, "%s ", FindNameByValue(bit));
			buffer += strlen(buffer);
		}
		bit >>= 1;
	}
	sprintf(buffer, "  systemflags:");
	buffer += strlen(buffer);
	bit = START_BIT;
	while (bit)
	{
		if (D->systemFlags & bit)
		{
			sprintf(buffer, "%s ", FindNameByValue(bit));
			buffer += strlen(buffer);
		}
		bit >>= 1;
	}
	sprintf(buffer, "   InternalBits: 0x%x\r\n", D->internalBits);
	buffer += strlen(buffer);

	return NOPROBLEM_BIT;
}

static FunctionResult SetRoleCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	unsigned int n = atoi(arg1);
	if (n < 1 || n > wordCount) return FAILRULE_BIT;
	char* arg2 = ARGUMENT(2);
	if (*arg2 != '~') return FAILRULE_BIT;

	changedNL = true;

	wordRole[n] = StoreWord(arg2);
	return NOPROBLEM_BIT;
}

static FunctionResult SetOriginalCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	unsigned int n = atoi(arg1);
	if (n < 1 || n > wordCount) return FAILRULE_BIT;

	changedNL = true;

	char* arg2 = ARGUMENT(2);
	wordStarts[n] = StoreWord(arg2)->word;
	return NOPROBLEM_BIT;
}

static FunctionResult SetCanonCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	unsigned int n = atoi(arg1);
	if (n < 1 || n > wordCount) return FAILRULE_BIT;

	changedNL = true;
	char* arg2 = ARGUMENT(2);
	WORDP D = StoreWord(arg2, AS_IS);
	wordCanonical[n] = D->word;
	canonicalWordp[n] = D;

	return NOPROBLEM_BIT;
}

static FunctionResult AddWordAtCode(char* buffer) // word canonical location
{
	if (wordCount >= REAL_SENTENCE_WORD_LIMIT) return FAILRULE_BIT;
	changedNL = true;

	char* ptr = ARGUMENT(1);
	char word[MAX_WORD_SIZE];
	char canon[MAX_WORD_SIZE];
	FunctionResult result;

	ptr = ReadShortCommandArg(ptr, word, result); // set
	if (result & ENDCODES) return result;
	ptr = ReadShortCommandArg(ptr, canon, result);
	if (result & ENDCODES) return result;

	// get location
	unsigned int startPosition = wordCount;
	if (*ptr == '^')
	{
		ptr = GetPossibleFunctionArgument(ptr, buffer); // pass thru or convert
		char word2[MAX_WORD_SIZE];
		if (!IsDigit(*buffer) && *buffer != '_')
		{
			strcpy(word2, buffer);
			GetCommandArg(word2, buffer, result, 0); // should be small
		}
	}
	if (IsDigit(*ptr) || *ptr == '_') ptr = ReadCompiledWord(ptr, buffer);  // the locator, leave it unevaled as number or match var
	else if (*ptr == USERVAR_PREFIX)
	{
		ptr = ReadCompiledWord(ptr, buffer);
		strcpy(buffer, GetUserVariable(buffer, false));
	}
	else if (*ptr) ptr = GetCommandArg(ptr, buffer, result, 0); // evaluate the locator as a number presumably

	if (IsDigit(*buffer))
	{
		int val = atoi(buffer);
		startPosition = val & 0x0000ffff;
	}
	else if (*buffer == '_')
	{
		startPosition = WILDCARD_START(wildcardPosition[GetWildcardID(buffer)]); // the match location
	}
	else
	{
		*buffer = 0;
		return FAILRULE_BIT;
	}
	if (startPosition < 1 || startPosition > wordCount || wordCount == REAL_SENTENCE_WORD_LIMIT)
	{
		*buffer = 0;
		return NOPROBLEM_BIT;	// fail silently
	}

	// make hole for word
	memmove(wordStarts + startPosition + 1, wordStarts + startPosition, (wordCount - startPosition + 1) * sizeof(char*));
	wordStarts[startPosition] = AllocateHeap(word, 0);

	memmove(wordCanonical + startPosition + 1, wordCanonical + startPosition, (wordCount - startPosition + 1) * sizeof(char*));
	wordCanonical[startPosition] = AllocateHeap(canon, 0);

	memmove(finalPosValues + startPosition + 1, finalPosValues + startPosition, (wordCount - startPosition + 1) * sizeof(uint64));
	finalPosValues[startPosition] = 0; // total unknown pos data
	memmove(derivationIndex + startPosition + 1, derivationIndex + startPosition, (wordCount - startPosition + 1) * sizeof(unsigned short));
	derivationIndex[startPosition] = derivationIndex[startPosition - 1]; // tag onto prior
	memmove(unmarked + startPosition + 1, unmarked + startPosition, (wordCount - startPosition + 1) * sizeof(char));
	unmarked[startPosition] = unmarked[startPosition - 1]; // tag onto prior;
	++wordCount;
	*buffer = 0;
	return NOPROBLEM_BIT;
}

static FunctionResult ReplaceWordCode(char* buffer)
{
	changedNL = true;
	char* ptr = ARGUMENT(1);
	char word[MAX_WORD_SIZE];
	FunctionResult result;
	ptr = ReadShortCommandArg(ptr, word, result); // set
	if (result & ENDCODES) return result;

	// get location
	unsigned int startPosition = wordCount;
	if (*ptr == '^')
	{
		ptr = GetPossibleFunctionArgument(ptr, buffer); // pass thru or convert
		char word2[MAX_WORD_SIZE];
		if (!IsDigit(*buffer) && *buffer != '_')
		{
			strcpy(word2, buffer);
			GetCommandArg(word2, buffer, result, 0); // should be small
		}
	}
	if (IsDigit(*ptr) || *ptr == '_') ptr = ReadCompiledWord(ptr, buffer);  // the locator, leave it unevaled as number or match var
	else if (*ptr == USERVAR_PREFIX)
	{
		ptr = ReadCompiledWord(ptr, buffer);
		strcpy(buffer, GetUserVariable(buffer, false));
	}
	else if (*ptr) ptr = GetCommandArg(ptr, buffer, result, 0); // evaluate the locator as a number presumably

	if (IsDigit(*buffer))
	{
		int val = atoi(buffer);
		startPosition = val & 0x0000ffff;
	}
	else if (*buffer == '_')
	{
		startPosition = WILDCARD_START(wildcardPosition[GetWildcardID(buffer)]); // the match location
	}
	else
	{
		*buffer = 0;
		return FAILRULE_BIT;
	}
	if (startPosition < 1 || startPosition > wordCount)
	{
		*buffer = 0;
		return NOPROBLEM_BIT;	// fail silently
	}
	wordCanonical[startPosition] = wordStarts[startPosition] = AllocateHeap(word, 0);
	*buffer = 0;
	return NOPROBLEM_BIT;
}

static FunctionResult UnmarkCode(char* buffer)
{
	// unmark() // disable global unmarks
	// unmark(*) // global unmark all words individually
	// unmark(* 4)	 // global unmark this location
	// unmark(* _location) // global unmark range
	// unmark(word 4)
	// unmark(word _location)
	// unmark(word all)
	// unmakr(@ _0)
	changedNL = true;

	char* ptr = ARGUMENT(1);
	char arg1[MAX_WORD_SIZE];
	char arg2[MAX_WORD_SIZE];
	FunctionResult result;
	ptr = ReadShortCommandArg(ptr, arg1, result);// set

	if (!*arg1) // unmark() reenables generic unmarking
	{
		if (oldunmarked[255]) // merge state back if have cached
		{
			memcpy(unmarked, oldunmarked, MAX_SENTENCE_LENGTH);
			oldunmarked[255] = 0;
		}
#ifndef DISCARDTESTING
		if (debugMark) (*debugMark)("-revert", "*");
#endif
		return NOPROBLEM_BIT;
	}

	if (result & ENDCODES) return result;
	ptr = ReadCompiledWord(ptr, arg2);

	// get location
	unsigned int startPosition = wordCount;
	unsigned int endPosition = 1;
	if (*arg2 == '^')
	{
		GetPossibleFunctionArgument(arg2, buffer); // pass thru or convert
		char word2[MAX_WORD_SIZE];
		if (!IsDigit(*buffer) && *buffer != '_')
		{
			strcpy(word2, buffer);
			if (!*word2)
				return NOPROBLEM_BIT; // just decline no place w/o error
			GetCommandArg(word2, buffer, result, 0); // should be small
		}
		strcpy(arg2, buffer);
	}
	if (IsDigit(*arg2) || *arg2 == '_');  // the locator, leave it unevaled as number or match var
	else if (*arg2 == USERVAR_PREFIX)
	{
		strcpy(arg2, GetUserVariable(arg2, false));
	}
	else if (*arg2)
	{
		GetCommandArg(arg2, buffer, result, 0); // evaluate the locator as a number presumably
		strcpy(arg2, buffer);
	}
	if (!*arg2)
	{
		if (*arg1 == '*') // unmark(*)
		{
			startPosition = 1;
			endPosition = wordCount;
		}
		else startPosition = endPosition = 1;
	}
	else if (IsDigit(*arg2))
	{
		int val = atoi(arg2);
		startPosition = val & 0x0000ffff;
		endPosition = val >> 16;
	}
	else if (*arg2 == '_')
	{
		startPosition = WILDCARD_START(wildcardPosition[GetWildcardID(arg2)]); // the match location
		endPosition = WILDCARD_END_ONLY(wildcardPosition[GetWildcardID(arg2)]);
	}
	else if (!stricmp(arg2, (char*)"all")) // remove ALL references anywhere of this
	{
		// BUG does not remove internal hidden concepts
		WORDP D = FindWord(arg1); //   set or word to unmark
		if (!D) return FAILRULE_BIT;
		ClearWordWhere(D, -1);
		*buffer = 0;
#ifndef DISCARDTESTING
		if (debugMark) (*debugMark)("-all", "all");
#endif
		return NOPROBLEM_BIT;
	}
	else
	{
		*buffer = 0;
		return FAILRULE_BIT;
	}

	if (endPosition == 0) endPosition = startPosition; // SHARED
	if (startPosition < 1 || startPosition > wordCount)
	{
		*buffer = 0;
		return NOPROBLEM_BIT;	// fail silently
	}
	if (endPosition > wordCount) endPosition = wordCount;

	if (*arg1 == '@' && !arg1[1]) // remove all references to match here
	{
		for (unsigned int i = startPosition; i <= endPosition; ++i)
		{
			if (trace & TRACE_OUTPUT) Log(USERLOG, "unmark @ %d(%s) ", i, wordStarts[i]);
			ClearWhereAt(i);
		}
	}
	else if (*arg1 == '*' && !arg1[1]) // set unmark EVERYTHING in range 
	{
		if (trace & TRACE_OUTPUT) Log(USERLOG, "unmark * words: ");
		for (unsigned int i = startPosition; i <= endPosition; ++i)
		{
			if (trace & TRACE_OUTPUT) Log(USERLOG, "%d(%s) ", i, wordStarts[i]);
			unmarked[i] = 1;
		}
		unmarked[0] = 1; // flag that we have some unmarking
#ifndef DISCARDTESTING
		if (debugMark)
		{
			char posn[MAX_WORD_SIZE];
			sprintf(posn, "-%u-%u", startPosition, endPosition);
			(*debugMark)(posn, "*");
		}
#endif
		if (trace & TRACE_OUTPUT) Log(USERLOG, "\r\n");
	}
	else
	{
		WORDP D = FindWord(arg1); //   set or word to unmark at specific location
		if (D)
		{
			for (unsigned int i = startPosition; i <= endPosition; ++i)
			{
				if (trace & TRACE_OUTPUT) Log(USERLOG, "unmark %s %d(%s)\r\n", D->word, i, wordStarts[i]);
				if (RemoveMatchValue(D, i))
				{
					RemoveConceptTopic(concepts, D, i);
					RemoveConceptTopic(topics, D, i);
					FACT* F = GetSubjectNondeadHead(D);
					while (F)
					{
						if (F->verb == Mmember)
						{
							WORDP X = Meaning2Word(F->object);
							if (*(X->word) == '~' && IsDigit(X->word[1])) // internal concept
							{
								RemoveMatchValue(X, i);
								if (trace & TRACE_OUTPUT) Log(USERLOG, "unmark %s %d(%s)\r\n", X->word, i, wordStarts[i]);
							}
						}
						F = GetSubjectNondeadNext(F);
					}
				}
			}
#ifndef DISCARDTESTING
			if (debugMark)
			{
				char posn[MAX_WORD_SIZE];
				sprintf(posn, "-%u-%u", startPosition, endPosition);
				(*debugMark)(posn, arg1);
			}
#endif
		}
	}
	*buffer = 0;
	return NOPROBLEM_BIT;
}

//////////////////////////////////////////////////////////
/// INPUT ROUTINES
//////////////////////////////////////////////////////////

static FunctionResult OriginalCode(char* buffer)
{
	if (csapicall == TEST_OUTPUT) return FAILRULE_BIT; // no input or NL data is visible

	char* arg = ARGUMENT(1);
	char kind[MAX_WORD_SIZE];
	ReadCompiledWord(arg, kind);
	if (!stricmp(kind, "rawuser"))
	{
		char* input = (csapicall != TEST_PATTERN) ? originalUserInput : rawtestpatterninput;
		size_t l = strlen(input);
		unsigned int size = (buffer - currentOutputBase) + l;
		if (size >= currentOutputLimit) strcpy(buffer, "**OriginalCode OVERFLOW**");
		else strcpy(buffer, input); // may blow the buffer
		return NOPROBLEM_BIT;
	}

	if (*arg == '\'') ++arg;
	unsigned int start = 0, end = 0;
	if (*arg == '^') strcpy(arg, FNVAR(arg));
	else if (*arg == USERVAR_PREFIX) strcpy(arg, GetUserVariable(arg, false));

	if (*arg == '_')
	{
		int x = GetWildcardID(arg);
		if (x == ILLEGAL_MATCHVARIABLE) return FAILRULE_BIT;
		start = WILDCARD_START(wildcardPosition[x]);
		end = WILDCARD_END_ONLY(wildcardPosition[x]);
	}
	else if (IsDigit(*arg))
	{
		start = atoi(arg);
		end = WILDCARD_END(start);
		start = WILDCARD_START(start);
	}
	else return FAILRULE_BIT;

	if (start >= (wordCount + 1)) start = wordCount + 1;
	if (start <= 0) start = 0;
	if (end <= 0) end = 0;
	if (end >= (wordCount + 1)) end = wordCount;

	return GetDerivationText(start, end, buffer);
}

static FunctionResult ActualInputRangeCode(char* buffer)
{ // for range of original input words, get the actual input range
	unsigned int start = atoi(ARGUMENT(1));
	unsigned int end = atoi(ARGUMENT(2));
	if (start < 0 || start > derivationLength) return FAILRULE_BIT;
	if (end < 0 || end > derivationLength || end < start) return FAILRULE_BIT;

	// set derivation data on original words of user before we do substitution
	unsigned int first = 0;
	unsigned int last = 0;
	for (unsigned int i = 1; i <= wordCount; ++i) // leaf thru actual words to find ranges covered
	{
		// actual word is from this range in original
		unsigned int a = derivationIndex[i] >> 8; // from here
		unsigned int b = (derivationIndex[i] & 0x00ff);  // to here including here  The end may be beyond wordCount if words have been deducted by now

		if (a <= end && b >= start) // our desired range (start - end) overlaps the word range (a - b)
		{
			if (!first) first = i;  // starting actual word in range
			last = i;	 // maximal ending actual word in range
		}
	}
	sprintf(buffer, "%u", (first << 8) + last);
	return NOPROBLEM_BIT;
}

static FunctionResult OriginalInputRangeCode(char* buffer)
{ // for range of actual input words, get the original input range
	unsigned int start = atoi(ARGUMENT(1));
	unsigned int end = atoi(ARGUMENT(2));
	if (start < 0 || start > wordCount) return FAILRULE_BIT;
	if (end < 0 || end > wordCount || end < start) return FAILRULE_BIT;

	// set derivation data on original words of user before we do substitution
	// actual word is from this range in original
	int a = derivationIndex[start] >> 8; // from here
	int b = (derivationIndex[end] & 0x00ff);  // to here including here  The end may be beyond wordCount if words have been deducted by now
	sprintf(buffer, "%d", (a << 8) + b);
	return NOPROBLEM_BIT;
}

static FunctionResult InputCode(char* buffer)
{      // when supplying multiple sentences, must do them in last first order
	if (inputCounter++ > 5)
		return FAILRULE_BIT;// limit per sentence reply
	if (totalCounter++ > 15)
		return FAILRULE_BIT; // limit per input from user
	if (!volleyFile && (sentenceloopcount + 1) >= sentenceLimit)
		return FAILRULE_BIT; // supplying new input will exceed limit, so decline

	if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, "\r\n");
	FunctionResult result;
	char* word = ARGUMENT(1);
	Output(word, buffer, result);
	if (strlen(buffer) >= (maxBufferSize - 100)) buffer[maxBufferSize - 100] = 0;	// might be smaller buffer
	Convert2Blanks(buffer); // break apart underscored words

	// put possessives back together.
	char* at = buffer;
	while ((at = strstr(at, (char*)" '")))
	{
		if (buffer[1] == 's') memmove(at, at + 1, strlen(at));
		if (buffer[1] == ' ' || buffer[1] == ',' || buffer[1] == '.' || buffer[1] == ';' || buffer[1] == '?' || buffer[1] == ':') memmove(at, at + 1, strlen(at));
		++at;
	}

	if (!strcmp(lastInputSubstitution, buffer) && *buffer)
	{
		*buffer = 0;
		return FAILRULE_BIT; // same result as before, apparently looping
	}
	if (showInput) Log(ECHOUSERLOG, (char*)"^input: %s\r\n", buffer);
	else if (trace & TRACE_FLOW) Log(USERLOG, "^input given: %s\r\n", buffer);
	if (!AddInput(buffer, 1))
	{
		*buffer = 0;
		return FAILRULE_BIT;
	}
	strcpy(lastInputSubstitution, buffer);
	*buffer = 0;
	return NOPROBLEM_BIT;
}

static FunctionResult RemoveTokenFlagsCode(char* buffer)
{
	int64 flags;
	ReadInt64(ARGUMENT(1), flags);
	tokenFlags &= -1 ^ flags;
	return NOPROBLEM_BIT;
}

static FunctionResult SetTokenFlagsCode(char* buffer)
{
	int64 flags;
	ReadInt64(ARGUMENT(1), flags);
	tokenFlags |= flags;
	return NOPROBLEM_BIT;
}

static FunctionResult WordAtIndexCode(char* buffer)
{
	bool canonical = !stricmp(ARGUMENT(1), "canonical");
	char* arg2 = ARGUMENT(2);
	unsigned int start, end;
	if (*arg2 == '"')
	{
		int index = GetWildcardID(arg2 + 1);
		if (index == ILLEGAL_MATCHVARIABLE) return FAILRULE_BIT;
		start = WILDCARD_START(wildcardPosition[index]);
		end = WILDCARD_END_ONLY(wildcardPosition[index]);
	}
	else
	{
		start = end = atoi(arg2);
		char* arg3 = ARGUMENT(3);
		if (*arg3) end = atoi(arg3);
	}
	if (start < 1 || start > wordCount) return FAILRULE_BIT;
	bool started = false;
	for (unsigned int i = start; i <= end; ++i)
	{
		if (i < 1 || i > wordCount) continue;	// ignore off end
		if (unmarked[i]) continue; // ignore words masked
		char* word = wordStarts[i];
		if (started) strcat(buffer, wildcardSeparator);
		else started = true;
		if (!canonical || !wordCanonical[i]) strcat(buffer, word);
		else strcat(buffer, wordCanonical[i]);
	}
	return NOPROBLEM_BIT;
}

static FunctionResult IsNormalWordCode(char* buffer)
{
	bool letters = false;
	char* arg1 = ARGUMENT(1);
	if (!stricmp(arg1, "letteronly"))
	{
		arg1 = ARGUMENT(2);
		letters = true;
	}
	unsigned char* at = (unsigned char*)(arg1 - 1);
	while (*++at)
	{
		if (letters && !IsAlphaUTF8(*at)) return FAILRULE_BIT;
		else if (!letters && !IsAlphaUTF8OrDigit(*at) && *at != '_' &&
			*at != '\'' && *at != '-' && *at < 0x80)
			return FAILRULE_BIT;
	}
	if (*(at - 1) == '-' || *(at - 1) == '_') return FAILRULE_BIT;
	return NOPROBLEM_BIT;
}

static FunctionResult SetWildcardIndexCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	int index = GetWildcardID(arg1);
	if (index == ILLEGAL_MATCHVARIABLE) return FAILRULE_BIT;
	SetWildCardIndexStart(index); // start here
	return NOPROBLEM_BIT;
}

//////////////////////////////////////////////////////////
/// NUMBER FUNCTIONS
//////////////////////////////////////////////////////////

static FunctionResult ComputeCode(char* buffer)
{
	int64 value = NOT_A_NUMBER;
	char* arg1 = ARGUMENT(1);
	char* op = ARGUMENT(2);
	char* arg2 = ARGUMENT(3);

	// move to float
	if (strchr(arg1, '.') || strchr(arg2, '.') || !stricmp(op, (char*)"root") || !stricmp(op, (char*)"square_root"))

	{
	FLOAT_X:
		double fvalue = (double)NOT_A_NUMBER;
		double number1 = (strchr(arg1, '.')) ? Convert2Double(arg1) : (double)Convert2Integer(arg1);
		double number2 = (strchr(arg2, '.')) ? Convert2Double(arg2) : (double)Convert2Integer(arg2);
		//   we must test case insenstive because arg2 might be capitalized (like add and ADD for attention deficit disorder)
		if (*op == '+' || !stricmp(op, (char*)"plus") || !stricmp(op, (char*)"add") || !stricmp(op, (char*)"and")) fvalue = number1 + number2;
		else if (!stricmp(op, (char*)"minus") || !stricmp(op, (char*)"subtract") || !stricmp(op, (char*)"deduct") || !stricmp(op, (char*)"take away") || *op == '-') fvalue = number1 - number2;
		else if (!stricmp(op, (char*)"x") || !stricmp(op, (char*)"times") || !stricmp(op, (char*)"multiply") || *op == '*') fvalue = number1 * number2;
		else if (!stricmp(op, (char*)"divide") || !stricmp(op, (char*)"quotient") || *op == '/')
		{
			if (number2 == 0.0)
			{
				strcpy(buffer, (char*)"infinity");
				return NOPROBLEM_BIT;
			}
			else fvalue = number1 / number2;
		}
		else if (!stricmp(op, (char*)"remainder") || !stricmp(op, (char*)"modulo") || !stricmp(op, (char*)"mod") || *op == '%')
		{
			return FAILRULE_BIT;
		}
		else if (!stricmp(op, (char*)"random"))
		{
			ReportBug((char*)"illegal random op in float");
			return FAILRULE_BIT;
		}
		else if (!stricmp(op, (char*)"root") || !stricmp(op, (char*)"square_root")) fvalue = (double)sqrt(number1);
		else if (!stricmp(op, (char*)"^") || !stricmp(op, (char*)"^^") || !stricmp(op, (char*)"power") || !stricmp(op, (char*)"exponent"))
		{
			int power = (int)Convert2Integer(arg2);
			if (power >= 1)
			{
				fvalue = number1;
				while (--power) fvalue *= number1;
			}
			else if (power == 0) fvalue = 1;
			else return FAILRULE_BIT;
		}
		if (fvalue != NOT_A_NUMBER)
		{
			long x = (long)fvalue;

			if ((double)x == fvalue)  strcpy(buffer, PrintU64(x));
			else WriteFloat(buffer, fvalue);
		}
		else sprintf(buffer, (char*)"%s", (char*)" ?");
	}
	else //   integer
	{
		int64 value1 = Convert2Integer(arg1);
		int64 value2 = Convert2Integer(arg2);
		if (*op == '+' || !stricmp(op, (char*)"add") || !stricmp(op, (char*)"and") || !stricmp(op, (char*)"plus")) value = value1 + value2;
		else if (*op == '-' || !stricmp(op, (char*)"deduct") || !stricmp(op, (char*)"minus") || !stricmp(op, (char*)"sub") || !stricmp(op, (char*)"subtract") || !stricmp(op, (char*)"take_away")) value = value1 - value2;
		else if (*op == '*' || !stricmp(op, (char*)"x") || !stricmp(op, (char*)"multiply") || !stricmp(op, (char*)"times")) value = value1 * value2;
		else if (*op == '%' || !stricmp(op, (char*)"mod") || !stricmp(op, (char*)"modulo") || !stricmp(op, (char*)"remainder"))
		{
			int64 v1 = value1;
			if (value1 < 0) value1 *= -1;
			value = value1 % value2;
			if (v1 < 0 && value) value = value2 - value;
		}
		else if (!stricmp(op, (char*)"random")) value = random((unsigned int)(value2 - value1)) + value1;
		else if (*op == '<' && op[1] == '<')  value = value1 << value2;  // BUT FLAWED if shift >= 32
		else if (*op == '>' && op[1] == '>') value = value1 >> value2;
		else if (*op == '/' || !stricmp(op, (char*)"divide"))
		{
			if (value2 == 0)
			{
				strcpy(buffer, (char*)"infinity");
				return NOPROBLEM_BIT;
			}
			value = value1 / value2;
			if ((value * value2) != value1) goto FLOAT_X; // needs float
		}
		else if (*op == '^' || !stricmp(op, (char*)"exponent") || !stricmp(op, (char*)"power"))
		{
			if (value2 >= 1)
			{
				value = value1;
				while (--value2) value *= value1;
			}
			else if (value2 == 0) value = 1;
			else return FAILRULE_BIT;
		}
		if (value == NOT_A_NUMBER) strcpy(buffer, (char*)"?");
		else strcpy(buffer, PrintU64(value));
	}
	return NOPROBLEM_BIT;
}

static FunctionResult IsNumberCode(char* buffer)
{
	return IsDigitWord(ARGUMENT(1), AMERICAN_NUMBERS, true) ? NOPROBLEM_BIT : FAILRULE_BIT;
}

static FunctionResult TimeFromSecondsCode(char* buffer)
{
	int64 seconds;
	char* word = ARGUMENT(1);
	ReadInt64(word, seconds);
	time_t sec = (time_t)seconds;

	// optional 2nd arg changes time zone as offset
	int sign = 1;
	char* offset = ARGUMENT(2);
	if (*offset == '+') ++offset;
	else if (*offset == '-')
	{
		sign = -1;
		++offset;
	}
	short int offsetHours = (short int)(atoi(offset) * sign);
	seconds += offsetHours * 60 * 60; // hours offset

	// convert to text string in whatever timezone the raw is in.
	myctime(&sec, buffer);
	*strchr(buffer, '\n') = 0; // erase newline at end

	return NOPROBLEM_BIT;
}

static FunctionResult TimeInfoFromSecondsCode(char* buffer)
{
	int64 seconds;
	char* word = ARGUMENT(1);
	ReadInt64(word, seconds);
	time_t sec = (time_t)seconds;
	struct tm time;
	mylocaltime((time_t*)&sec, &time);
	if (impliedWild != ALREADY_HANDLED)
	{
		SetWildCardIndexStart(impliedWild); //   start of wildcards to spawn
		impliedWild = ALREADY_HANDLED;
	}
	else SetWildCardIndexStart(0); //   start of wildcards to spawn
	char value[MAX_WORD_SIZE];
	sprintf(value, (char*)"%d", time.tm_sec);
	SetWildCard(value, value, 0, 0);
	sprintf(value, (char*)"%d", time.tm_min);
	SetWildCard(value, value, 0, 0);
	sprintf(value, (char*)"%d", time.tm_hour);
	SetWildCard(value, value, 0, 0);
	sprintf(value, (char*)"%d", time.tm_mday);
	SetWildCard(value, value, 0, 0);
	sprintf(value, (char*)"%s", months[time.tm_mon]); // january = 0
	SetWildCard(value, value, 0, 0);
	sprintf(value, (char*)"%d", time.tm_year + 1900);
	SetWildCard(value, value, 0, 0);
	sprintf(value, (char*)"%s", days[time.tm_wday]); // sunday = 0
	SetWildCard(value, value, 0, 0);
	sprintf(value, (char*)"%d", time.tm_mon); // january = 0
	SetWildCard(value, value, 0, 0);
	sprintf(value, (char*)"%d", time.tm_wday); // sunday = 0
	SetWildCard(value, value, 0, 0);

	return NOPROBLEM_BIT;
}

static FunctionResult StatsCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	bool echo = (!stricmp(ARGUMENT(2), "echo")) ? true : false;
	if (echo) userLog = FILE_LOG;
	uint64 endtime = ElapsedMilliseconds();

	if (!stricmp(arg1, "facts"))
	{
		sprintf(buffer, "%d", (int)(factEnd - lastFactUsed));
		if (echo) Log(ECHOUSERLOG, "facts: %s\r\n", buffer);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, "text"))
	{
		sprintf(buffer, "%d", (int)(heapFree - stackFree));
		if (echo) Log(ECHOUSERLOG, "text: %s\r\n", buffer);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, "dict"))
	{
		int index = Word2Index(dictionaryFree);
		int dictfree = (int)(maxDictEntries - index);
		sprintf(buffer, "%d words", dictfree);
		if (echo) Log(ECHOUSERLOG, "dict: %s words\r\n", buffer);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, "hashbuckets"))
	{
		unsigned int depthcount[100];
		unsigned int unused;
		unsigned int words = CountWordsInBuckets(unused, depthcount, 100);

		char depthmsg[1000];
		int x = 100;
		while (!depthcount[--x]); // find last used slot
		char* at = depthmsg;
		*at = 0;
		for (int i = 1; i <= x; ++i)
		{
			sprintf(at, "%u.", depthcount[i]);
			at += strlen(at);
		}
		sprintf(buffer, "%s", depthmsg);
		if (echo) Log(ECHOUSERLOG, "hashbuckets %s\r\n", buffer);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, "unusedhashbuckets"))
	{
		unsigned int depthcount[100];
		unsigned int unused;
		unsigned int words = CountWordsInBuckets(unused, depthcount, 100);
		sprintf(buffer, "%u", unused);
		if (echo) Log(ECHOUSERLOG, "unusedhashbuckets %s\r\n", buffer);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, "patternevaluation"))
	{
		sprintf(buffer, "%u", patternEvaluationCount);
		if (echo) Log(ECHOUSERLOG, "patterns: %s\r\n", buffer);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, "timesummary"))
	{
		unsigned int store = (impliedSet == ALREADY_HANDLED) ? 0 : impliedSet;
		SET_FACTSET_COUNT(store, 0);

		for (std::map<WORDP, uint64>::iterator it = timeSummary.begin(); it != timeSummary.end(); ++it)
		{
			WORDP D = it->first;
			unsigned int count = it->second >> TIMESUMMARY_COUNT_OFFSET;
			unsigned int time = it->second & TIMESUMMARY_TIME;

			unsigned int flags = FACTTRANSIENT | FACTDUPLICATE;
			if (D->internalBits & TOPIC) flags |= USER_FLAG1;
			bool systemcall = (D->x.codeIndex && !(D->internalBits & (IS_PLAN_MACRO | IS_TABLE_MACRO))); // system function
			if (D->internalBits & FUNCTION_NAME) flags |= (systemcall ? USER_FLAG2 : USER_FLAG3);

			WORDP C = StoreIntWord(count);
			WORDP T = StoreIntWord(time);
			if (echo) Log(ECHOUSERLOG, "%s #:%s ms:%s\r\n", D->word, C->word, T->word);
			FACT* F = CreateFact(MakeMeaning(D), MakeMeaning(C), MakeMeaning(T), flags);
			AddFact(store, F);
		}
		if (echo) Log(ECHOUSERLOG, "NL time:ms:%d\r\n", (int)(endtime - startNLTime));
		impliedSet = ALREADY_HANDLED;
		return NOPROBLEM_BIT;
	}

	return FAILRULE_BIT;
}

static FunctionResult TimeToSecondsCode(char* buffer)
{
	struct tm timeinfo;
	time_t rawtime;
	time(&rawtime);
	mylocaltime(&rawtime, &timeinfo); // get daylight savings value in

	/*
	tm_sec	int	seconds after the minute	0-61*
	tm_min	int	minutes after the hour	0-59
	tm_hour	int	hours since midnight	0-23
	tm_mday	int	day of the month	1-31
	tm_mon	int	months since January	0-11
	tm_year	int	years since 1900
	tm_wday	int	days since Sunday	0-6
	tm_yday	int	days since January 1	0-365
	tm_isdst	int	Daylight Saving Time flag
	*/
	//   Www Mmm dd hh:mm:ss yyyy Where Www is the weekday, Mmm the month in letters, dd the day of the month, hh:mm:ss the time, and yyyy the year. Sat May 20 15:21:51 2000
	timeinfo.tm_wday = timeinfo.tm_yday = 0;
	char* seconds = ARGUMENT(1);
	if (*seconds == '-') return FAILRULE_BIT;
	ReadInt(seconds, timeinfo.tm_sec);
	if (timeinfo.tm_sec > 61) return FAILRULE_BIT; // leap seconds allowed
	char* minutes = ARGUMENT(2);
	if (*minutes == '-') return FAILRULE_BIT;
	timeinfo.tm_min = atoi(minutes);
	char* hours = ARGUMENT(3);
	if (*hours == '-') return FAILRULE_BIT;
	timeinfo.tm_hour = atoi(hours);
	char* day = ARGUMENT(4);
	if (!IsDigit(*day)) return FAILRULE_BIT;
	timeinfo.tm_mday = atoi(day);
	if (timeinfo.tm_mday == 0) return FAILRULE_BIT; // day must be 1 or higher

	char* month = ARGUMENT(5);
	if (IsDigit(*month)) timeinfo.tm_mon = atoi(month) - 1;	 // 0 based
	else if (!strnicmp(month, (char*)"jan", 3)) timeinfo.tm_mon = 0;
	else if (!strnicmp(month, (char*)"feb", 3)) timeinfo.tm_mon = 1;
	else if (!strnicmp(month, (char*)"mar", 3)) timeinfo.tm_mon = 2;
	else if (!strnicmp(month, (char*)"apr", 3)) timeinfo.tm_mon = 3;
	else if (!strnicmp(month, (char*)"may", 3)) timeinfo.tm_mon = 4;
	else if (!strnicmp(month, (char*)"jun", 3)) timeinfo.tm_mon = 5;
	else if (!strnicmp(month, (char*)"jul", 3)) timeinfo.tm_mon = 6;
	else if (!strnicmp(month, (char*)"aug", 3)) timeinfo.tm_mon = 7;
	else if (!strnicmp(month, (char*)"sep", 3)) timeinfo.tm_mon = 8;
	else if (!strnicmp(month, (char*)"oct", 3)) timeinfo.tm_mon = 9;
	else if (!strnicmp(month, (char*)"nov", 3)) timeinfo.tm_mon = 10;
	else if (!strnicmp(month, (char*)"dec", 3)) timeinfo.tm_mon = 11;
	char* year = ARGUMENT(6);
	if (*year == '-') return FAILRULE_BIT;
	timeinfo.tm_year = atoi(year); // years since 1900
	if (timeinfo.tm_year < 1970 || timeinfo.tm_year > 2100) return FAILRULE_BIT; // unacceptable year
	timeinfo.tm_year -= 1900;	// pass in as years since 1900
	char* daylightsavings = ARGUMENT(7);
	if (*daylightsavings == '1' || *daylightsavings == 't' || *daylightsavings == 'T') timeinfo.tm_isdst = 1;
	else if (*daylightsavings == '0' || *daylightsavings == 'f' || *daylightsavings == 'F') timeinfo.tm_isdst = 0;

	time_t time = mktime(&timeinfo);
	if (time == -1) return FAILRULE_BIT;
	strcpy(buffer, PrintU64(time));
	return NOPROBLEM_BIT;
}
//////////////////////////////////////////////////////////
/// DEBUG FUNCTIONS
//////////////////////////////////////////////////////////
static FunctionResult ReportBugCode(char* buffer)
{
	char* data = ARGUMENT(1);
	ReportBug(data);
	return NOPROBLEM_BIT;
}

static FunctionResult LogCode(char* buffer)
{
	char* stream = ARGUMENT(1);
	uint64 flags;
	bool bad = false;
	bool response = false;
	stream = ReadFlags(stream, flags, bad, response); // try for flags
	if (flags && *stream == ')') ++stream; // skip end of flags
	char name[MAX_WORD_SIZE];
	*name = 0;
	FunctionResult result;
	bool keep = false;
	char* fname = NULL;
	if (!strnicmp(stream, (char*)"CLOSE", 5))
	{
		stream = ReadCompiledWord(stream, name); // close
		stream = ReadShortCommandArg(stream, name, result, OUTPUT_NOQUOTES); // name of file
		if (*name == '"')
		{
			size_t len = strlen(name);
			name[len - 1] = 0;	// remove trailing "
		}
		fname = (*name == '"') ? (name + 1) : name;
		for (unsigned int i = 0; i < MAX_LOG_NAMES; ++i)
		{
			if (!stricmp(fname, lognames[i])) // found already open
			{
				FClose(logfiles[i]);
				logfiles[i] = NULL;
				*lognames[i] = 0;
				return NOPROBLEM_BIT;
			}
		}
		return FAILRULE_BIT;
	}

	// preopen a file
	if (!strnicmp(stream, (char*)"FILE ", 5) || !strnicmp(stream, (char*)"OPEN ", 5)) // write data to this file
	{
		stream = ReadCompiledWord(stream, name); // FILE or OPEN
		if (!stricmp(name, (char*)"OPEN")) keep = true;
		stream = ReadShortCommandArg(stream, name, result, OUTPUT_NOQUOTES); // name of file
		if (*name == '"')
		{
			size_t len = strlen(name);
			name[len - 1] = 0;	// remove trailing "
		}
		fname = (*name == '"') ? (name + 1) : name;
		if (!strnicmp(stream, (char*)"NEW", 3)) // start with a clean file
		{
			char junk[MAX_WORD_SIZE];
			stream = ReadCompiledWord(stream, junk);
			FILE* out = FopenUTF8Write(fname);
			if (out) FClose(out);
			else return FAILRULE_BIT;
		}
	}

	char item[MAX_WORD_SIZE];
	char* at = ReadCompiledWord(stream, item);
	int channel = FORCESTAYUSERLOG;
	if (!stricmp(item, "USERLOG"))
	{
		channel = FORCESTAYUSERLOG;
		stream = at;
	}
	else	if (!stricmp(item, "SERVERLOG"))
	{
		channel = SERVERLOG;
		stream = at;
	}

	++outputNest;
	WORDP lock = dictionaryLocked;
	dictionaryLocked = (WORDP)22; // allow format string to work even while compiling a table
	FreshOutput(stream, buffer, result, (unsigned int)flags, MAX_BUFFER_SIZE);
	--outputNest;
	dictionaryLocked = lock;

	if (!(flags & RESPONSE_NOCONVERTSPECIAL)) ConvertNL(buffer); // convert \n \r \t to ascii chars

	if (fname)
	{
		FILE* out = NULL;
		bool cached = false;
		for (unsigned int i = 0; i < MAX_LOG_NAMES; ++i)
		{
			if (!stricmp(fname, lognames[i])) // found already open
			{
				out = logfiles[i];
				cached = true;
				break;
			}
		}
		if (!out) // not cached
		{
			out = FopenUTF8WriteAppend(fname);
			if (keep)
			{
				for (unsigned int i = 0; i < MAX_LOG_NAMES; ++i) // try to cache it
				{
					if (!logfiles[i]) // found already open
					{
						logfiles[i] = out;
						strcpy(lognames[i], fname);
						cached = true;
						break;
					}
				}
			}
		}
		if (out)
		{
			fwrite(buffer, 1, strlen(buffer), out);
			if (!cached) FClose(out);
		}
		else return FAILRULE_BIT;
	}
	else Log(channel, (char*)"%s", buffer);
	if (trace & TRACE_OUTPUT && CheckTopicTrace())
		Log(USERLOG, (char*)"^log: %s", buffer);
	if (flags & OUTPUT_ECHO && !echo) (*printer)((char*)"%s", buffer);
	*buffer = 0;
	return NOPROBLEM_BIT;
}


//////////////////////////////////////////////////////////
/// OUTPUT FUNCTIONS
//////////////////////////////////////////////////////////

static FunctionResult FormatCode(char* buffer)
{
	char* arg1 = ARGUMENT(1); // int64 or double 
	char* arg2 = ARGUMENT(2); // the format string
	size_t len = strlen(arg2);
	if (*arg2 == '"')
	{
		memmove(arg2, arg2 + 1, len);
		arg2[len - 2] = 0;
	}
	char* arg3 = ARGUMENT(3); // the value

	double fvalue;
	int value;
	if (!stricmp(arg1, "float") || !stricmp(arg1, "double")) // input is float
	{
		fvalue = Convert2Double(arg3);
		if (strchr(arg2, 'd') || strchr(arg2, 'u') || strchr(arg2, 'i')
			|| strchr(arg2, 'x') || strchr(arg2, 'X'))
		{
			int64 x = (int64)fvalue;
			sprintf(buffer, arg2, (int)x);
		}
		else if (strstr(arg2, "ll")) // covers ull also
		{
			int64 x = (int64)fvalue;
			sprintf(buffer, arg2, x);
		}
		else sprintf(buffer, arg2, fvalue); // sprintf doesnt not take float, it autoexpands to double
	}
	else if (!stricmp(arg1, "integer") || !stricmp(arg1, "int")) // input is a flavor of int
	{
		int64 intvalue = atoi64(arg3); // if they lie about incoming type so what
		bool minus = false;
		if (intvalue < 0)
		{
			minus = true;
			intvalue *= -1;
		}
		value = (int)intvalue;
		if (minus)
		{
			intvalue *= -1;
			value *= -1;
		}
		if (strstr(arg2, "ll")) sprintf(buffer, arg2, intvalue);
		else sprintf(buffer, arg2, value);
	}
	else return FAILRULE_BIT;
	return NOPROBLEM_BIT;
}

static FunctionResult FlushOutputCode(char* buffer)
{
	if (planning) return FAILRULE_BIT;
	if (!AddResponse(currentRuleOutputBase, responseControl)) return FAILRULE_BIT;
	return NOPROBLEM_BIT;
}

static FunctionResult InsertOutput(char* stream, char* buffer, int index)
{
	// add at end, then alter order
	FunctionResult result;
	Output(stream, buffer, result, 0);
	if (AddResponse(buffer, responseControl))
	{
		memmove(&responseOrder[index + 1], &responseOrder[index], responseIndex - index); // shift order out 1
		responseOrder[index] = (unsigned char)(responseIndex - 1);
	}
	else result = FAILRULE_BIT;
	return result;
}

static FunctionResult InsertPrintCode(char* buffer)
{
	if (planning) return FAILRULE_BIT;
	char* stream = ARGUMENT(1);
	uint64 flags;
	bool bad = false;
	bool response = false;
	stream = ReadFlags(stream, flags, bad, response); // try for flags
	if (flags) ++stream; // skip end of flags
	FunctionResult result;
	char beforeIndex[MAX_WORD_SIZE];
	stream = ReadShortCommandArg(stream, beforeIndex, result);
	int index = 0;

	if (*beforeIndex == '~') // put before 1st reference to this topic
	{
		int topicid = FindTopicIDByName(beforeIndex);
		for (int i = responseIndex - 1; i > 0; --i)
		{
			if (topicid == responseData[responseOrder[i]].topic) index = responseOrder[i];
		}
	}
	else if (IsDigit(*beforeIndex)) // numeric index he gives must be 1 based, eg before %response 
	{
		index = atoi(beforeIndex);
		if (index <= 0 || index > (int)responseIndex) return FAILRULE_BIT;
		index = responseOrder[index - 1]; // the current location of the output
	}
	return InsertOutput(stream, buffer, index);
}

static FunctionResult PrintCode(char* buffer)
{
	if (planning) return FAILRULE_BIT;
	char* stream = ARGUMENT(1);
	char flag[MAX_WORD_SIZE];
	bool doprintf = false;
	bool log = false;
	char* x = ReadCompiledWord(stream, flag);
	if (!stricmp(flag, "stdout"))
	{
		stream = x;
		doprintf = true;
		x = ReadCompiledWord(stream, flag);
	}
	if (!stricmp(flag, "log"))
	{
		stream = x;
		log = true;
	}

	uint64 flags;
	bool bad = false;
	bool response = false;
	stream = ReadFlags(stream, flags, bad, response); // try for flags
	if ((flags || response) && *stream == ')') ++stream; // skip end of flags

	FunctionResult result;
	Output(stream, buffer, result, (unsigned int)flags);
	if (doprintf && buffer)
	{
		if (trace) Log(USERLOG, "print=`%s`", buffer);
		char* find = buffer;
		while ((find = strstr(find, "\\r")))
		{
			memmove(find, find + 1, strlen(find));
			*find = '\r';
		}
		find = buffer;
		while ((find = strstr(find, "\\n"))) strncpy(find, "\n ", 2);
		printf("%s ", buffer);
		if (echo && log) Log(ECHOUSERLOG, "%s", buffer);
		else if (log) Log(USERLOG, "%s", buffer);
		else printf("%s", buffer);
		*buffer = 0;
	}
	else  if (!(flags & OUTPUT_RETURNVALUE_ONLY) && !AddResponse(buffer, response ? (unsigned int)flags : responseControl)) result = FAILRULE_BIT;
	return result;
}

static FunctionResult PrePrintCode(char* buffer)
{
	if (planning) return FAILRULE_BIT;
	char* stream = ARGUMENT(1);
	uint64 flags;
	bool bad = false;
	bool response = false;
	stream = ReadFlags(stream, flags, bad, response); // try for flags
	if (flags) ++stream; // skip end of flags

	return InsertOutput(stream, buffer, 0);
}

static FunctionResult RepeatCode(char* buffer)
{
	if (postProcessing)
	{
		ReportBug((char*)"INFO: Illegal to use ^Repeat during postprocessing");
		return FAILRULE_BIT;
	}
	AddRepeatable(currentRule); // local repeats allowed this volley
	return NOPROBLEM_BIT;
}

static FunctionResult SetPronounCode(char* buffer)
{
	// argument1 is a word to use
	// mark(word _xxx) enable word mark at location of _xxx variable
	char* ptr = ARGUMENT(1);
	if (!*ptr) return FAILRULE_BIT;

	FunctionResult result;
	char word[MAX_WORD_SIZE];
	ptr = ReadShortCommandArg(ptr, word, result); // what is being marked
	if (result & ENDCODES) return result;
	if (!*word) return FAILRULE_BIT; // missing arg

	char word1[MAX_WORD_SIZE];
	ptr = ReadCompiledWord(ptr, word1);  // the locator

	unsigned int startPosition;
	unsigned int endPosition;
	if (!*word1 || *word1 == ')') startPosition = endPosition = 1; // default mark  (ran out or hit end paren of call
	else if (IsDigit(*word1)) endPosition = startPosition = atoi(word1); // named number as index
	else if (*word1 == '_') //  wildcard position designator
	{
		int position = wildcardPosition[GetWildcardID(word1)];
		startPosition = WILDCARD_START(position); // the match location
		endPosition = WILDCARD_END_ONLY(position);
	}
	else return FAILRULE_BIT;

	if (startPosition < 1) startPosition = 1;
	if (startPosition > wordCount)  startPosition = wordCount;
	WORDP D = StoreWord(word);
	MarkMeaningAndImplications(0, 0, MakeMeaning(D), startPosition, startPosition);

	WORDP entry;
	WORDP canonical;
	uint64 sysflags = 0;
	uint64 cansysflags = 0;
	WORDP revise;
	GetPosData((unsigned int)-1, word, revise, entry, canonical, sysflags, cansysflags, false); // NOT first try
	wordStarts[startPosition] = D->word;
	wordCanonical[startPosition] = (canonical) ? canonical->word : D->word;
	if (!wordCanonical[startPosition]) wordCanonical[startPosition] = D->word;

	uint64 bits = D->properties & (NOUN_PROPERTIES | NOUN_BITS | NOUN_INFINITIVE | LOWERCASE_TITLE);
	allOriginalWordBits[startPosition] = bits;
	finalPosValues[startPosition] = bits;
	MarkTags(startPosition);

	return NOPROBLEM_BIT;
}

//////////////////////////////////////////////////
/// OUTPUT ACCESS
//////////////////////////////////////////////////

static FunctionResult LastSaidCode(char* buffer)
{
	if (chatbotSaidIndex) sprintf(buffer, (char*)"%s", chatbotSaid[chatbotSaidIndex - 1]);
	return NOPROBLEM_BIT;
}

static FunctionResult SetResponseCode(char* buffer)
{
	int index = atoi(ARGUMENT(1)) - 1;
	if (index >= responseIndex || index < 0) return FAILRULE_BIT;
	char* data = ARGUMENT(2);
	if (*data == '"')
	{
		++data;
		size_t len = strlen(data);
		if (data[len - 1] == '"') data[len - 1] = 0;
	}
	if (!stricmp(data, "null")) *data = 0;
	responseData[responseOrder[index]].response = AllocateHeap(data, 0);
	return NOPROBLEM_BIT;
}

static FunctionResult ResponseCode(char* buffer)
{
	int index = atoi(ARGUMENT(1)) - 1;
	if (index >= responseIndex || index < 0) return FAILRULE_BIT;
	strcpy(buffer, responseData[responseOrder[index]].response);
	return NOPROBLEM_BIT;
}

static FunctionResult ResponsePatternCode(char* buffer)
{
	int index = atoi(ARGUMENT(1)) - 1;
	if (index >= responseIndex || index < 0) return FAILRULE_BIT;
	char* end = responseData[responseOrder[index]].patternchoice;
	int nest = 0;
	if (!end) return NOPROBLEM_BIT; // there is no pattern 
	--end;
	while (*++end)
	{
		if (!strnicmp(end, "[", 1) || !strnicmp(end, "(", 1) ||
			!strnicmp(end, "{", 1) || !strnicmp(end, "<<", 2))
			++nest;
		else if (!strnicmp(end, "]", 1) || !strnicmp(end, ")", 1) ||
			!strnicmp(end, "}", 1) || !strnicmp(end, ">>", 2))
			--nest;
		if (*end == ' ' && nest == 0) break;
	}
	char* start = responseData[responseOrder[index]].patternchoice;
	strncpy(buffer, start, end - start);
	buffer[end - start] = 0;
	return NOPROBLEM_BIT;
}

static FunctionResult ResponseQuestionCode(char* buffer)
{
	int index = atoi(ARGUMENT(1)) - 1; // which response (1 based)
	if (index >= responseIndex || index < 0) return FAILRULE_BIT;
	char* ptr = TrimSpaces(responseData[responseOrder[index]].response, false);
	strcpy(buffer, (ptr[strlen(ptr) - 1] == '?') ? (char*)"1" : (char*)"");
	return NOPROBLEM_BIT;
}

static FunctionResult ResponseRuleIDCode(char* buffer)
{
	int index = atoi(ARGUMENT(1)) - 1; // they say 1, we use 0
	if (index >= responseIndex) return FAILRULE_BIT;
	ComputeWhy(buffer, index);
	return NOPROBLEM_BIT;
}

//////////////////////////////////////////////////////////
/// POSTPROCESSING FUNCTIONS
//////////////////////////////////////////////////////////

static FunctionResult TokenizeCode(char* buffer)
{
	if (impliedSet != ALREADY_HANDLED) SET_FACTSET_COUNT(impliedSet, 0);
	char* limit;
	int index = 0;
	char* ptr = ARGUMENT(1);
	bool word = false;
	bool raw = true;
	bool seen = false;
	bool purify = false;
	if (strstr(ptr, "raw") || strstr(ptr, "RAW")) { raw = true; seen = true; }
	if (strstr(ptr, "full") || strstr(ptr, "FULL")) { raw = false; seen = true; }
	if (strstr(ptr, "word") || strstr(ptr, "WORD")) { word = true; seen = true; }
	if (strstr(ptr, "purify") || strstr(ptr, "PURIFY")) { purify = true; seen = true; }
	if (strstr(ptr, "sentence") || strstr(ptr, "SENTENCE")) { word = false; seen = true; }
	if (seen) ptr = ARGUMENT(2);

	if (purify)
	{
		size_t size = strlen(ptr);
		PurifyInput(ptr, ptr, size, INPUT_PURIFY); // change out special or illegal characters
	}

	MEANING M = MakeMeaning(StoreWord("^tokenize"));
	// each sentence becomes a transient fact (possibly duplicate)
	while (ptr && *ptr) // find sentences of response
	{
		char* old = ptr;
		unsigned int count;
		char* starts[MAX_SENTENCE_LENGTH];
		memset(starts, 0, sizeof(char*) * MAX_SENTENCE_LENGTH);
		char separators[MAX_SENTENCE_LENGTH];
		memset(separators, 0, sizeof(char) * MAX_SENTENCE_LENGTH);
		ptr = SkipWhitespace(ptr);
		ptr = Tokenize(ptr, count, (char**)starts, separators, false);   //   only used to locate end of sentence but can also affect tokenFlags (no longer care)
		if (!raw)
		{
			wordCount = count;
			memcpy(wordStarts, starts, sizeof(starts));
			NLPipeline(trace);// use preprocesses
		}
		if (word)
		{
			for (unsigned int i = 1; i <= count; ++i)
			{
				MEANING subject = MakeMeaning(StoreWord(starts[i], AS_IS));
				FACT* F = CreateFact(subject, M, M, FACTTRANSIENT | FACTDUPLICATE);
				if (impliedSet != ALREADY_HANDLED) factSet[impliedSet][++index] = F;
			}
		}
		else
		{
			//   save sentences as facts
			char* copy = InfiniteStack(limit, "FactizeResult"); // localized
			char* at = copy;
			for (unsigned int i = 1; i <= count; ++i)
			{
				strcpy(at, starts[i]);
				strncat(at, &separators[i], 1);
				at += strlen(at);
			}
			*at = 0;
			CompleteBindStack();
			FACT* F = CreateFact(MakeMeaning(StoreWord(copy, AS_IS)), M, M, FACTTRANSIENT | FACTDUPLICATE);
			if (impliedSet != ALREADY_HANDLED) factSet[impliedSet][++index] = F;
			ReleaseStack(copy);
		}
	}
	if (impliedSet != ALREADY_HANDLED)
	{
		SET_FACTSET_COUNT(impliedSet, index);
		impliedSet = ALREADY_HANDLED;
	}
	currentFact = NULL;
	return NOPROBLEM_BIT;
}


static FunctionResult PostPrintBeforeCode(char* buffer) // only works if post processing
{
	if (!postProcessing)
	{
		ReportBug((char*)"INFO: Cannot use ^PostPrintBefore except in postprocessing");
		return FAILRULE_BIT;
	}

	char* stream = ARGUMENT(1);
	uint64 flags;
	bool bad = false;
	bool response = false;
	stream = ReadFlags(stream, flags, bad, response); // try for flags
	if (flags) ++stream; // skip end of flags

	FunctionResult result;
	Output(stream, buffer, result, (unsigned int)flags);

	// prepend output 
	if (AddResponse(buffer, responseControl))
	{
		memmove(&responseOrder[0 + 1], &responseOrder[0], responseIndex - 0); // shift order out 1
		responseOrder[0] = (unsigned char)(responseIndex - 1);
	}

	*buffer = 0;
	return result;
}

static FunctionResult PostPrintAfterCode(char* buffer) // only works if post processing
{
	if (!postProcessing)
	{
		ReportBug((char*)"INFO: Cannot use ^PostProcessPrintAfter except in postprocessing");
		return FAILRULE_BIT;
	}

	char* stream = ARGUMENT(1);
	uint64 flags;
	bool bad = false;
	bool response = false;
	stream = ReadFlags(stream, flags, bad, response); // try for flags
	if (flags) ++stream; // skip end of flags

	FunctionResult result;
	Output(stream, buffer, result, (unsigned int)flags);

	// postpend output 
	AddResponse(buffer, responseControl);
	*buffer = 0;
	return result;
}

static FunctionResult SleepCode(char* buffer)
{
	int milliseconds = atoi(ARGUMENT(1));
#ifdef WIN32
	Sleep(milliseconds);
#else
	usleep(milliseconds * 1000);
#endif 
	return NOPROBLEM_BIT;
}

static FunctionResult RebootCode(char* buffer)
{ // script request to reload boot data
	if (!rebooting) return FAILRULE_BIT; // only legal when per volley boot requests
	ReturnToAfterLayer(LAYER_1, false);  // dict/fact/strings reverted and any extra topic loaded info  (but CSBoot process NOT lost)
	UnlockLayer(LAYER_BOOT);
	rebooting = false;	// we turn it off
	return NOPROBLEM_BIT;
}

static FunctionResult DummyCode(char* buffer)
{
	return FAILRULE_BIT;
}

static FunctionResult RulematchesCode(char* buffer)
{
	char* arg = ARGUMENT(1);
	if (!stricmp(arg, "on")) noteRulesMatching = true;
	else if (!stricmp(arg, "off")) noteRulesMatching = false;
	else if (!stricmp(arg, "show"))
	{
		HEAPREF list = rulematches;
		while (list)
		{
			uint64 top, name, rejoinder;
			list = UnpackHeapval(list, name, top, rejoinder);
			char* topic = (char*)name;
			sprintf(buffer, "%s.%d.%d ", topic, (int)top, (int)rejoinder);
			buffer += strlen(buffer);
		}
	}
	else return FAILRULE_BIT;
	return NOPROBLEM_BIT;
}

static FunctionResult EnvironmentCode(char* buffer)
{
	const char* env_p = std::getenv(ARGUMENT(1));
	if (!env_p || !*env_p) return FAILRULE_BIT;
	strcpy(buffer, env_p);
	return NOPROBLEM_BIT;
}

//////////////////////////////////////////////////////////
/// COMMAND FUNCTIONS
//////////////////////////////////////////////////////////

static FunctionResult CommandCode(char* buffer)
{
	char* arg = ARGUMENT(1);
	char arg1[MAX_WORD_SIZE];
	arg = GetPossibleFunctionArgument(arg, arg1); // pass thru or convert
	FunctionResult result;
	GetCommandArg(arg, buffer, result, OUTPUT_NOCOMMANUMBER | ASSIGNMENT);
	if (result != NOPROBLEM_BIT) return result;

	if (*arg1 == USERVAR_PREFIX) strcpy(arg1, GetUserVariable(arg1, false));
	else if (*arg1 == '_') strcpy(arg1, GetwildcardText(GetWildcardID(arg1), true));

	DoCommand(arg1, NULL, false);
	return NOPROBLEM_BIT;
}

FunctionResult IdentifyCode(char* buffer)
{
	char* os;
#ifdef WIN32
	os = "Windows";
#elif IOS
	os = "IOS";
#elif __MACH__
	os = "MACH";
#else
	os = "LINUX";
#endif
	sprintf(buffer, (char*)"ChatScript Version %s  %ld bit %s compiled %s\r\n", version, (long int)(sizeof(char*) * 8), os, compileDate);

	return NOPROBLEM_BIT;
}

FunctionResult DebugCode(char* buffer)
{
	return NOPROBLEM_BIT;
}

FunctionResult SpellCheckCode(char* buffer)
{
	WORDP choices[4000];
	unsigned int index = 0;
	int min = 35; // allow 2 changes as needed

	char* xarg = ARGUMENT(1); // input string
	char* dict = ARGUMENT(2);   // json array
	char* tol = ARGUMENT(3);  // spell tolerance - % of word length or # of characters
	if (!*xarg || !*dict) return FAILRULE_BIT;
	if (!IsValidJSONName(dict, 'a')) return FAILRULE_BIT;
	WORDP dictionary = FindWord(dict);
	if (!dictionary) return FAILRULE_BIT;
	float tolerance = -1;
	float spellFactor = 17.5;   // spell fix uses 35 as meaning two words can be fixed
	if (*tol && IsInteger(tol, false)) min = (int)(spellFactor * atoi(tol));
	else tolerance = (*tol ? (float)atof(tol) : (float)-1);
	FACT* F;
	WORDINFO realWordData;
	char* output = buffer;
	*output = 0;

	// loop thru tokens of input
	char originalWord[MAX_WORD_SIZE];
	char* ptr = xarg;
	char lcword[MAX_WORD_SIZE];
	while (ptr && *ptr) // walk thru sentence
	{
		ptr = ReadCompiledWord(ptr, originalWord);
		WORDP D = FindWord(originalWord, 0, PRIMARY_CASE_ALLOWED);
		index = 0;
		MakeLowerCopy(lcword, originalWord);
		ComputeWordData(lcword, &realWordData);

		int wordMin = (tolerance > 0 ? (int)(strlen(originalWord) * tolerance * spellFactor) : min);

		F = GetSubjectHead(dictionary);
		while (F)
		{
			D = Meaning2Word(F->object);
			CheckWord(originalWord, realWordData, D, choices, index, wordMin);
			if (index > 3997) break;
			F = GetSubjectNext(F);
		}
		if (index)  strcpy(output, choices[0]->word); // pick first guess, no tiebreaks
		else strcpy(output, originalWord);
		output += strlen(output);
		*output++ = ' ';
		for (unsigned int j = 0; j < index; ++j) RemoveInternalFlag(choices[j], BEEN_HERE);
	}
	if (output > buffer) *--output = 0;
	return NOPROBLEM_BIT;
}

static FunctionResult ReturnCode(char* buffer)
{
	FunctionResult result;
	Output(ARGUMENT(1), buffer, result, OUTPUT_NOCOMMANUMBER);
	if (result != NOPROBLEM_BIT) return result;
	if (!stricmp(buffer, "null")) *buffer = 0;	// treat null as null
	return ENDCALL_BIT;
}

static FunctionResult EndCode(char* buffer)
{
	char* word = ARGUMENT(1);
	if (!stricmp(word, (char*)"RULE")) return ENDRULE_BIT;
	if (!stricmp(word, (char*)"TOPRULE")) return ENDRULE_BIT;
	if (!stricmp(word, (char*)"LOOP")) return ENDLOOP_BIT;
	if (!stricmp(word, (char*)"TOPIC") || !stricmp(word, (char*)"PLAN")) return ENDTOPIC_BIT;
	if (!stricmp(word, (char*)"SENTENCE")) return ENDSENTENCE_BIT;
	if (!stricmp(word, (char*)"INPUT")) return ENDINPUT_BIT;
	if (!stricmp(word, (char*)"CALL")) return ENDCALL_BIT;
	return FAILRULE_BIT;
}

static FunctionResult EvalCode(char* buffer) //  ??? needed with output eval instead?
{
	FunctionResult result;
	char* stream = ARGUMENT(1);
	uint64 flags;
	bool bad = false;
	bool response = false;
	stream = ReadFlags(stream, flags, bad, response); // try for flags
	if (flags && *stream == ')') ++stream; // skip end of flags
	Output(stream, buffer, result, (unsigned int)flags);
	return result;
}

static FunctionResult FailCode(char* buffer)
{
	char* word = ARGUMENT(1);
	if (!stricmp(word, (char*)"LOOP")) return FAILRULE_BIT;
	if (!stricmp(word, (char*)"RULE")) return FAILRULE_BIT;
	if (!stricmp(word, (char*)"TOPRULE"))
		return FAILTOPRULE_BIT;
	if (!stricmp(word, (char*)"TOPIC") || !stricmp(word, (char*)"PLAN"))
	{
		RemovePendingTopic(currentTopicID);
		return FAILTOPIC_BIT;
	}
	if (!stricmp(word, (char*)"SENTENCE")) return FAILSENTENCE_BIT;
	if (!stricmp(word, (char*)"INPUT")) return FAILINPUT_BIT;
	return FAILRULE_BIT;
}

FunctionResult MatchesCode(char* buffer)
{
	if (nopatterndata) return FAILRULE_BIT;
	GetPatternData(buffer);
	return NOPROBLEM_BIT;
}

FunctionResult MatchCode(char* buffer)
{
	char word[MAX_WORD_SIZE];
	char word1[MAX_WORD_SIZE];
	char* at = ReadCompiledWord(ARGUMENT(1), word1);
	if (!*word1) return FAILRULE_BIT;
	bool functionString = false;
	if (*word1 == USERVAR_PREFIX && !*at) strcpy(word, GetUserVariable(word1, false)); //   solitary user var, decode it  eg match($var)
	else if (*word1 == '_' && !*at) strcpy(word, wildcardCanonicalText[GetWildcardID(word1)]); //   solitary user var, decode it  eg match($var)
	else if (*word1 == '@' && !*at)
	{
		FunctionResult result;
		ReadShortCommandArg(word1, word, result, OUTPUT_NOQUOTES);
		if (result != NOPROBLEM_BIT) return result;
	}
	else strcpy(word, ARGUMENT(1)); // otherwise it is what to say (like from idiom table)

	// could already be compiled
	if (word[0] == FUNCTIONSTRING && word[1] == '(')
	{
		memmove(word, word + 1, strlen(word));
		functionString = true;
	}
	// compiled string (rather than format string)
	else if (word[1] == FUNCTIONSTRING && word[0] == '"' && word[2] == ':')
	{
		memmove(word, word + 3, strlen(word));
		functionString = true;
	}
	SAVESYSTEMSTATE()

	WORDP X = FindWord(word);
	char* pack = AllocateBuffer();
	char* base = pack;
	if (*word == '~' && (!X || !(X->internalBits & TOPIC)) && strchr(word, '.')) // named an existing rule 
	{
		char* rule;
		bool fulllabel = false;
		int id = 0;
		SAVEOLDCONTEXT();
		bool crosstopic = false;
		char* dot = strchr(word, '.');
		if (!dot) { FreeBuffer();  return FAILRULE_BIT; }
		if (dot && IsDigit(dot[1])) rule = GetRuleTag(currentTopicID, id, word);
		else rule = GetLabelledRule(currentTopicID, word, (char*)"", fulllabel, crosstopic, id, currentTopicID);
		RESTOREOLDCONTEXT()
			if (!rule) { FreeBuffer();  RESTORESYSTEMSTATE() return FAILRULE_BIT; }
		if (!GetPattern(rule, NULL, pack, false, maxBufferSize)) // out of space
		{
			FreeBuffer();
			RESTORESYSTEMSTATE()
				return FAILRULE_BIT;
		}
		++base; // ignore starting paren
	}
	else
	{
		char* ptr = word;
		if (*word)
		{
			size_t len = strlen(word);
			strcpy(word + len, (char*)" )"); // insure it has a closing paren (even if it has);
			if (*word == '"')
			{
				word[len - 1] = ' '; // change closing " to space
				++ptr;	// skip opening "
				if (*ptr == FUNCTIONSTRING) ++ptr; // bypass the mark  "^" is compiled string
												   // now purify of any internal \" " marked strings
				char* x = strchr(ptr, '\\');
				while (x)
				{
					if (x[1] == '"') memmove(x, x + 1, strlen(x));	// remove escape
					x = strchr(x + 1, '\\'); // next?
				}
			}
		}
		if (*ptr == '|') // compilepattern keyword spellcheck protector
		{
			char* at1 = strchr(ptr + 1, '|');
			char* paren = strchr(ptr + 1, '(');
			if (at1 && paren < at1 && paren) at1 = paren - 1; // its missing |?
			if (at1) ptr = at1 + 1;
		}
		// skip compiled string mark, unless this really is a function call
		if (*ptr == FUNCTIONSTRING)
		{
			char token[MAX_WORD_SIZE];
			ReadNextSystemToken(NULL, ptr, token, true, true);
			WORDP D = FindWord(token, 0, LOWERCASE_LOOKUP);
			if (!D || !(D->internalBits & FUNCTION_NAME)) ++ptr; //   not a recognized call
		}
		while (*ptr == ' ') ++ptr;	// prepare for start
		if (*ptr != '(') // insure it has a pattern start
		{
			memmove(ptr + 1, ptr, strlen(ptr) + 1);
			*ptr = '(';
		}

		at = pack;
		*at = 0;
		base = ptr;	// do the best you can, may not be laid out properly

		int buffercount = bufferIndex;
		if (functionString) {} // the compiled pattern
#ifndef DISCARDSCRIPTCOMPILER 
		else if (setjmp(scriptJump[++jumpIndex])) // return on script compiler error
		{
			//globalDepth = oldDepth;
			bufferIndex = buffercount;
			EndScriptCompiler();
			--jumpIndex;
			FreeBuffer();
			RESTORESYSTEMSTATE()
				return FAILRULE_BIT;
		}
		else
		{
			char junk[MAX_WORD_SIZE];
			StartScriptCompiler(false);
			ReadNextSystemToken(NULL, NULL, junk, false, false); // flush cache
			ReadPattern(ptr, NULL, at, false, false); // compile the pattern
			EndScriptCompiler();
			strcat(at, (char*)" )");
			at = pack; // for debug retry
			base = pack; // the compiled pattern
			--jumpIndex;
		}
#endif
	}
	if (!*base) { FreeBuffer();  RESTORESYSTEMSTATE() return FAILRULE_BIT; }	// NO DATA?
	if (*base == '(') ++base;		// skip opening paren of a pattern
	if (*base == ' ') ++base;		// skip opening space of a pattern

	int start = 0;
	int end = 0;
	bool retried = false;
	MARKDATA hitdata;
retry:
	int matched = 0;
	wildcardIndex = 0;  //   reset wildcard allocation on top-level pattern match
	char oldmark[MAX_SENTENCE_LENGTH];
	memcpy(oldmark, unmarked, MAX_SENTENCE_LENGTH);
	hitdata.start = start;
	bool match = Match(base, 0, hitdata,1, 0,  matched, '(' ) != 0;  //   skip paren and treat as NOT at start depth, dont reset wildcards- if it does match, wildcards are bound
	start = hitdata.start;
	end = hitdata.end;
	if (match && patternRetry)
	{
		if (start > MAX_SENTENCE_LENGTH || start < 0)
		{
			start = 0; // never matched internal words - is at infinite start -- WHY allow this?
		}
		else
		{
			retried = true;
			goto retry;
		}
	}

	memcpy(unmarked, oldmark, MAX_SENTENCE_LENGTH);
	ShowMatchResult(!match ? FAILRULE_BIT : NOPROBLEM_BIT, base, NULL, 0);
	FreeBuffer();
	RESTORESYSTEMSTATE()
		if (!match && patternRetry) return NOPROBLEM_BIT;
	if (!match) return FAILRULE_BIT;

	char data[10];
	sprintf(data, "%d", start);
	SetUserVariable("$$csmatch_start", data);
	sprintf(data, "%d", end);
	SetUserVariable("$$csmatch_end", data);
	return NOPROBLEM_BIT;
}

static FunctionResult NoRejoinderCode(char* buffer)
{
	norejoinder = true;
	return NOPROBLEM_BIT;
}

static FunctionResult TraceCode(char* buffer)
{
	unsigned int oldtrace = trace;
	trace = (unsigned int)-1;
	FunctionResult result;
	*buffer = 0;
	char* arg1 = ARGUMENT(1);
	Output(arg1, buffer, result);
	trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
	return result;
}

static FunctionResult NoTraceCode(char* buffer)
{
	unsigned int oldtrace = trace;
	if (trace & TRACE_ALWAYS && (trace & (-1 ^ TRACE_ALWAYS)))	trace = TRACE_ON; // allow specific topic and function traces
	else trace = 0;
	FunctionResult result;
	*buffer = 0;
	char* arg1 = ARGUMENT(1);
	Output(arg1, buffer, result);
	trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
	return result;
}

static WORDP RestoreWord(unsigned int index)
{
	return 	Index2Word(index);
}

static unsigned int SaveWord(WORDP D)
{
	return Word2Index(D); 
}

static bool HasTriedData(HEAPINDEX index)
{
	unsigned char* data = (unsigned char*)Index2Heap(index) + 8;
	if (*data == END_OF_REFERENCES) return false;
	else return true;
}

static unsigned int* RestoreTriedData(WORDP key, unsigned int*& memory)
{
	unsigned int* data = (unsigned int*)AllocateHeap(NULL, TRIEDDATA_WORDSIZE, 4, false);
	if (!data) return NULL;

	// 1st 2 words are the 64bit int
	*data++ = *--memory;
	*data++ = *--memory;
	unsigned char* refs = (unsigned char*)data; // write onto here
	for (int i = 0; i < MAX_XREF_SENTENCE; ++i) // each 8 byte ref is start,end,extra,unused, 4byte exact,
	{
		// hardcoded! 1st save is 4 char  2nd save is dict offset
		memmove(refs, --memory, REF_ELEMENT_SIZE / 2);
		unsigned int index = *--memory;
		if (index) index = Word2Index(RestoreWord(index));
		memmove(refs + 4, (char*)&index, REF_ELEMENT_SIZE / 2);
		unsigned char at = *refs;
		refs += REF_ELEMENT_SIZE;
		if (at == END_OF_REFERENCES)  break; // end of unit marker
	}
	triedData[key] = Heap2Index((char*)data);
	return data;
}

HEAPREF RestoreList(unsigned int*& memory)
{
	HEAPREF list = NULL;
	unsigned int ref;
	while ((ref = *--memory) != 0)
	{
		WORDP D = RestoreWord(ref);
		list = AllocateHeapval(HV1_WORDP,list, (uint64)D, 0, 0); // concepts[i] and topics[i] lists of word indices
	}

	return list;
}

static FunctionResult InternalRestoreSentence(unsigned int* start, unsigned int* endit)
{
	ClearWhereInSentence();
	unsigned int* memory = endit;
	unsigned int checkb = *--memory; // marker 123
	if (checkb != ENCODE_MARKER)
	{
		ReportBug("RestoreSentence mismatch top");
		return FAILRULE_BIT;
	}

	--memory; // not used

	uint64 x = *--memory;
	uint64 y = *--memory;
	tokenFlags = (x << 32) | y;

	wordCount = *--memory;
	unsigned int val = *--memory;
	moreToCome = val & 1;
	moreToComeQuestion = val & 2;

	for (unsigned int i = 1; i <= wordCount; ++i)
	{
		WORDP D = RestoreWord(*--memory);
		wordStarts[i] = D->word;
		WORDP E = RestoreWord(*--memory);
		wordCanonical[i] = E->word;
		x = *--memory;
		y = *--memory;
		finalPosValues[i] = (x << 32) | y;
		x = *--memory;
		y = *--memory;
		allOriginalWordBits[i] = (x << 32) | y;
		x = *--memory;
		y = *--memory;
		roles[i] = (x << 32) | y;
		wordTag[i] = RestoreWord(*--memory);
	}
	checkb = *--memory; // marker 123
	if (checkb != ENCODE_MARKER)
	{
		ReportBug("RestoreSentence mismatch top");
		return FAILRULE_BIT;
	}
	// store derivation data
	derivationLength = *--memory;
	for (unsigned int i = 1; i <= derivationLength; ++i)
	{
		derivationSentence[i] = RestoreWord(*--memory)->word;
	}
	checkb = *--memory; // marker 123
	if (checkb != ENCODE_MARKER)
	{
		ReportBug("RestoreSentence mismatch top");
		return FAILRULE_BIT;
	}
	int hold = endit - memory;
	memory -= MAX_SENTENCE_LENGTH / sizeof(int);
	memmove(derivationSeparator, memory, MAX_SENTENCE_LENGTH * sizeof(char));
	hold = endit - memory;
	memory -= (MAX_SENTENCE_LENGTH * sizeof(short int)) / sizeof(int);
	memmove(derivationIndex, memory, (MAX_SENTENCE_LENGTH * sizeof(short int)));
	hold = endit - memory;
	checkb = *--memory; // marker 123
	if (checkb != ENCODE_MARKER)
	{
		ReportBug("RestoreSentence mismatch top");
		return FAILRULE_BIT;
	}
	int xy = (wordCount + 1);
	memory -= xy;
	memcpy(phrases, memory, xy * sizeof(int));
	memory -= xy;
	memcpy(verbals, memory, xy * sizeof(int));
	memory -= xy;
	memcpy(clauses, memory, xy * sizeof(int));
	memory -= (xy + 3) / 4; // byte oriented
	memmove(unmarked, memory, xy); // clear all mark suppression
	memory -= (xy + 3) / 4; // byte oriented
	memmove(crossReference, memory, xy);
	memory -= (xy + 3) / 4; // byte oriented
	memmove(indirectObjectRef, memory, xy);
	memory -= (xy + 3) / 4; // byte oriented
	memmove(objectRef, memory, xy);
	memory -= (xy + 3) / 4; // byte oriented
	memmove(complementRef, memory, xy);

	unsigned int checkd = *--memory;
	if (checkd != ENCODE_MARKER)
	{
		ReportBug("RestoreSentence mismatch top");
		return FAILRULE_BIT;
	}
	// save tried data
	int counter = *--memory;
	for (int i = 0; i < counter; ++i)
	{
		WORDP key = RestoreWord(*--memory);
		triedData[key] = *--memory; // heap data index
	}
	checkd = *--memory;
	if (checkb != ENCODE_MARKER)
	{
		ReportBug("RestoreSentence mismatch top");
		return FAILRULE_BIT;
	}
	for (unsigned int i = 1; i <= wordCount; ++i)
	{
		// concepts and topics lists are threaded links in heap space. [0] is the concept/topic, [1] is next link
		// that list will be valid later on restore
		concepts[i] = Index2Heap(*--memory);
		topics[i] = Index2Heap(*--memory);
	}

	unsigned int check = *--memory;
	if (check != ENCODE_MARKER)
	{
		ReportBug("RestoreSentence mismatch bottom");
		return FAILRULE_BIT; // ran out
	}

	if (trace & TRACE_FLOW)
	{
		Log(USERLOG, "Restore Sentence: ");
		for (unsigned int i = 1; i <= wordCount; ++i) Log(USERLOG, "%s ", wordStarts[i]);
		Log(USERLOG, "\r\n");
	}

	return NOPROBLEM_BIT;
}

static unsigned char GetNextData(unsigned char*& data, unsigned char*& pending)
{
	static unsigned char hold[26 * 52 + 4];
	static int index = 0;

	if (!data || *data == 0) // end of stacked data
	{
		data = pending;
		pending = NULL;
		if (!data || *data == '.') return '.';
	}
	if (*data == '.')
	{
		return 0; // partial leftover is a bug!
	}
	if (*data == '*')  // ab
	{
		int n = 1; // zone
		for (int i = 0; i < 2; ++i) hold[i] = '0'; // each 0 is 2 hex digits
		hold[2] = 0; // end of data
		pending = data + 1;
		data = hold;
	}
	else if (*data >= 'a' && *data <= 'z') // uncompress 0's
	{
		int n = (*data - 'a') * 52; // zone
		if (*++data >= 'a' && *data <= 'z') n += *data - 'a';
		else n += *data - 'A' + 26;
		for (int i = 0; i < (n * 2); ++i) hold[i] = '0'; // each 0 is 2 hex digits
		hold[n * 2] = 0; // end of data
		pending = data + 1;
		data = hold;
	}
	else if (*data == '<') // uncompress 0xff's
	{
		int n = 0; // zone
		if (*++data >= 'a' && *data <= 'z') n += *data - 'a';
		else n += *data - 'A' + 26;
		for (int i = 0; i < (n * 2); ++i) hold[i] = 'F'; // each n is 2 hex digits
		hold[n * 2] = 0; // end of data
		pending = data + 1;
		data = hold;
	}
	else index++;
	unsigned char c = (*data <= '9') ? (*data++ - '0') : (*data++ - 'A' + 10);
	return c;
}

FunctionResult RestoreSentence(WORDP  name)
{
	HEAPREF list = savedSentencesThreadList;
	uint64 data = 0;
	uint64 endit = 0;
	uint64 E = 0;
	while (list && E != (uint64)name) // find the sentence referred to
	{
		list = UnpackHeapval(list, E, data, endit);
	}
	if (E != (uint64)name) return FAILRULE_BIT;

	return InternalRestoreSentence((unsigned int*)data, (unsigned int*)endit);

}

static FunctionResult EncodeSentenceData(unsigned int* start, unsigned int* endit, char* buffer)
{
	if (*(endit - 1) != ENCODE_MARKER) return FAILRULE_BIT; // bad data

	size_t len = (char*)endit - (char*)start;
	int unitsSaved = len; // in bytes
	char* startBuffer = buffer;
	char* basedata = (char*)start;
	buffer += 8; // ascii size data in future

	// copy wordsaves character data
	char* endwords = strchr(basedata, END_TEXT_CHUNK); // end marker for preliminary word list
	if (!endwords)
	{
		ReportBug("missing preliminary words");
		return FAILRULE_BIT;
	}
	else
	{
		if (endwords != basedata) endwords = strstr(endwords, END_TEXT_CHUNK_STRING) + 1; // find last string and marker
		while (*++endwords == '@'); // skip align data
		size_t ll = endwords - basedata;
		strncpy(buffer, basedata, ll);
		buffer += ll;
		*buffer = 0;
	}

	// now the main binary save data
	unsigned int* membase = (unsigned int*)endwords;
	if (membase[0] != ENCODE_MARKER)
	{
		ReportBug("encode sentence bad data");
		return FAILRULE_BIT;
	}
	static int index = 0;
	unsigned char* data = ((unsigned char*)membase) - 1;
	while (++data < (unsigned char*)endit) // now write out the binary save info mostly
	{
		if (*data == 0) // compress 0's, each represents 2 hex digit 0's
		{
			int hold = index;
			int n = 1;
			int limit = (26 * 52);  // cant use hex A-F in encoding
			while (--limit && ++data < (unsigned char*)endit && *data == 0)
			{
				++n;  // how many more 0 in a row
			}
			index += n * 2;
			--data; // redo tail
			int blocks = n / 52; // how many blocks - each block maps 52 choices 
			int myindex = n - (blocks * 52);  // using lowercase and uppercase alpha for index
			if (blocks == 0 && myindex == 1) // ab
			{
				*buffer++ = '*';
			}
			else
			{
				*buffer++ = (char)('a' + blocks); // save block index  a-z values
				if (myindex < 26) *buffer++ = (unsigned char)('a' + myindex);
				else *buffer++ = (unsigned char)('A' + (myindex - 26));
			}
		}
		else if (*data == 0xff) // compress f's, each represents 2 hex digit f's
		{
			int hold = index;
			int n = 1;
			int limit = (26 * 52);  // cant use hex A-F in encoding
			while (--limit && ++data < (unsigned char*)endit && *data == 0xff)
			{
				++n;  // how many more 0 in a row
			}
			index += n * 2;
			--data; // redo tail
			int blocks = n / 52; // how many blocks - each block maps 52 choices 
			if (blocks)
			{
				ReportBug("compression of 0xff failed");
				return FAILRULE_BIT;
			}
			*buffer++ = '<'; // fmarker
			int myindex = n;  // using lowercase and uppercase alpha for index
			if (myindex < 26) *buffer++ = (unsigned char)('a' + myindex);
			else *buffer++ = (unsigned char)('A' + (myindex - 26));
		}
		else // 1 byte normal is 2 hex digits 0-F
		{
			*buffer++ = toHex[*data >> 4];
			*buffer++ = toHex[*data & 0x0f];
		}
	}
	*buffer++ = '.';
	*buffer = 0;
	size_t n = buffer - (startBuffer + 8);
	sprintf((char*)startBuffer, "%07zu", n); // size of actual data (including wordnames)
	startBuffer[7] = '-';
	return NOPROBLEM_BIT;
}

static FunctionResult RestoreSentenceCode(char* buffer)
{
	if (documentMode) return FAILRULE_BIT;
	char* arg1 = ARGUMENT(1);
	if (!*arg1) return FAILRULE_BIT;	// need an id
	WORDP D = FindWord(arg1);
	if (!D) return FAILRULE_BIT;
	return RestoreSentence(D);
}

unsigned int GetWordIndex(char* word, char* savewordbuffer)
{
	WORDP D = StoreWord(word, AS_IS);
	return  SaveWord(D);
}

static FunctionResult InternalSaveSentence(char* name)
{
	if (!wordCount) return NOPROBLEM_BIT;

	WORDP D;

	unsigned int* startmem = (unsigned int*)AllocateStack(NULL, 400000, false, 64);  // bottom of NL space
	unsigned int* memory = (unsigned int*)(startmem + 400000); // top of NL space (moveable)
	unsigned int* endit = (unsigned int*)memory;

	*--memory = ENCODE_MARKER;
	*--memory = 0; // unused
	unsigned int x = tokenFlags >> 32;
	unsigned int y = tokenFlags & 0x00000000ffffffff;
	*--memory = x;
	*--memory = y;
	*--memory = wordCount;
	*--memory = (moreToCome) ? 1 : 0;
	if (moreToComeQuestion) *memory |= 2;
	for (unsigned int i = 1; i <= wordCount; ++i)
	{
		*--memory = SaveWord(StoreWord(wordStarts[i], AS_IS));
		*--memory = SaveWord(StoreWord(wordCanonical[i], AS_IS));
		// note we have not saved any properties or systemflags set by script
		*--memory = finalPosValues[i] >> 32;
		*--memory = finalPosValues[i] & 0x00000000ffffffff;
		*--memory = allOriginalWordBits[i] >> 32;
		*--memory = allOriginalWordBits[i] & 0x00000000ffffffff;
		*--memory = roles[i] >> 32;
		*--memory = roles[i] & 0x00000000ffffffff;
		// BUG WARNING concepts and topics are not safe in GC if sentence saved
		// after mark

		*--memory = SaveWord(wordTag[i]);
	}
	*--memory = ENCODE_MARKER;

	// store derivation data
	*--memory = derivationLength; // int
	for (unsigned int i = 1; i <= derivationLength; ++i) *--memory = SaveWord(StoreWord(derivationSentence[i], AS_IS));
	*--memory = ENCODE_MARKER;
	memory -= MAX_SENTENCE_LENGTH / sizeof(int); // as bytes
	memmove(memory, derivationSeparator, MAX_SENTENCE_LENGTH * sizeof(char));
	memory -= (MAX_SENTENCE_LENGTH * sizeof(short int)) / sizeof(int);  // as shorts
	memmove(memory, derivationIndex, (MAX_SENTENCE_LENGTH * sizeof(short int)));
	*--memory = ENCODE_MARKER;

	int xz = (wordCount + 1); // include 0 position
	memory -= xz;
	memcpy(memory, phrases, xz * sizeof(int));
	memory -= xz;
	memcpy(memory, verbals, xz * sizeof(int));
	memory -= xz;
	memcpy(memory, clauses, xz * sizeof(int));
	int yy = xz + 3;
	memory -= yy / 4; // bytes
	memmove(memory, unmarked, xz); // clear all mark suppression
	memory -= yy / 4;
	memmove(memory, crossReference, xz);
	memory -= yy / 4;
	memmove(memory, indirectObjectRef, xz);
	memory -= yy / 4;
	memmove(memory, objectRef, xz);
	memory -= yy / 4;
	memmove(memory, complementRef, xz);

	*--memory = ENCODE_MARKER;
	// save tried data
	unsigned int* counterLocation = --memory; // reserve counter
	int counter = 0;
	for (std::map<WORDP, HEAPINDEX>::iterator it = triedData.begin(); it != triedData.end(); ++it)
	{
		HEAPINDEX ref = it->second;
		// only save actual marks, otherwise potentail for zombie marks to be restored
		if (HasTriedData(ref))
		{
			++counter;
			*--memory = SaveWord(it->first); // key
			*--memory = ref; // heap data index
		}
	}
	*--memory = ENCODE_MARKER;

	*counterLocation = counter; // backfill with number to do
	for (unsigned int i = 1; i <= wordCount; ++i)
	{
		// concepts and topics lists are threaded links in heap space. [0] is the concept/topic, [1] is next link
		// that list will be valid later on restore
		*--memory = Heap2Index(concepts[i]);
		*--memory = Heap2Index(topics[i]);
	}
	*--memory = ENCODE_MARKER;

	// merge to a heap blob
	int size = ((char*)endit - (char*)memory);
	if (memory < startmem)
	{
		ReportBug("SaveSentence overrun");
		return FAILRULE_BIT;
	}
	char* heap = AllocateHeap(NULL, size );
	memcpy(heap , (char*)memory, size); // copy in binary data
	unsigned int* endval = (unsigned int*)(heap + size); //corresponds to endit
	ReleaseStack((char*)startmem);

	// save on named list of sentences for local use
	D = StoreWord(name, AS_IS);
	savedSentencesThreadList = AllocateHeapval(HV1_WORDP|HV2_STRING|HV3_STRING,savedSentencesThreadList, (uint64)D, (uint64)heap, (uint64)endval);

	if (trace & TRACE_FLOW)
	{
		char* buf = AllocateBuffer("savesentence");
		char* ptr = buf;
		for (unsigned int i = 1; i <= wordCount; ++i)
		{
			strcpy(ptr, wordStarts[i]);
			ptr += strlen(ptr);
			*ptr++ = ' ';
			*ptr = 0;
		}
		Log(USERLOG, "Savesentence %s:  %s\r\n", name, buf);
		FreeBuffer();
	}

	return NOPROBLEM_BIT;
}

static FunctionResult SaveSentenceCode(char* buffer)
{
	if (documentMode) return FAILRULE_BIT;
	char* arg1 = ARGUMENT(1);
	if (!*arg1) return FAILRULE_BIT;	// need an id
	if (wordCount && !wordStarts[1]) return FAILRULE_BIT;
	return InternalSaveSentence(arg1);
}

static FunctionResult IfCode(char* buffer)
{
	return FAILRULE_BIT;	// shouldnt come thru here
}

static FunctionResult LoopCode(char* buffer)
{
	return FAILRULE_BIT;	// shouldnt come thru here
}


static FunctionResult NoFailCode(char* buffer)
{
	char word[MAX_WORD_SIZE];
	char* ptr = ReadCompiledWord(ARGUMENT(1), word);
	FunctionResult result;
	Output(ptr, buffer, result);
	if (result == RESTART_BIT) return result;
	else if (!stricmp(word, (char*)"RULE")) return (FunctionResult)(result & (RESTART_BIT | ENDTOPIC_BIT | FAILTOPIC_BIT | RETRYTOPIC_BIT | ENDSENTENCE_BIT | FAILSENTENCE_BIT | ENDINPUT_BIT | RETRYSENTENCE_BIT | ENDCALL_BIT));
	else if (!stricmp(word, (char*)"TOPRULE")) return (FunctionResult)(result & (RESTART_BIT | ENDTOPIC_BIT | FAILTOPIC_BIT | RETRYTOPIC_BIT | ENDSENTENCE_BIT | FAILSENTENCE_BIT | ENDINPUT_BIT | RETRYSENTENCE_BIT | ENDCALL_BIT));
	else if (!stricmp(word, (char*)"TOPIC")) return (FunctionResult)(result & (RESTART_BIT | ENDSENTENCE_BIT | FAILSENTENCE_BIT | RETRYSENTENCE_BIT | ENDINPUT_BIT | RETRYINPUT_BIT | ENDCALL_BIT));
	else if (!stricmp(word, (char*)"LOOP")) return (FunctionResult)(result & (RESTART_BIT | ENDTOPIC_BIT | FAILTOPIC_BIT | RETRYTOPIC_BIT | ENDSENTENCE_BIT | FAILSENTENCE_BIT | RETRYSENTENCE_BIT | ENDINPUT_BIT | RETRYINPUT_BIT | ENDCALL_BIT));
	else if (!stricmp(word, (char*)"SENTENCE") || !stricmp(word, (char*)"INPUT")) return NOPROBLEM_BIT;
	return FAILRULE_BIT; // not a legal choice
}

static FunctionResult NotNullCode(char* buffer)
{
	FunctionResult result;
	Output(ARGUMENT(1), buffer, result);
	if (*buffer) *buffer = 0;
	else return FAILRULE_BIT;
	return NOPROBLEM_BIT;
}

static FunctionResult ResultCode(char* buffer)
{
	FunctionResult result;
	Output(ARGUMENT(1), buffer, result);
	if (result == RESTART_BIT) return result;
	*buffer = 0;
	strcpy(buffer, ResultCode(result));
	return NOPROBLEM_BIT;
}

static FunctionResult RetryCode(char* buffer)
{
	char* arg = ARGUMENT(1);
	if (!stricmp(arg, (char*)"TOPIC"))  return RETRYTOPIC_BIT;
	if (!stricmp(arg, (char*)"sentence"))  return RETRYSENTENCE_BIT;
	if (!stricmp(arg, (char*)"input"))
		return RETRYINPUT_BIT;
	if (!stricmp(arg, (char*)"toprule"))
	{
		return RETRYTOPRULE_BIT;
	}
	return RETRYRULE_BIT;
}

static WORDP memoryDictBase = 0;
static char* memoryTextBase = 0;
static FACT* memoryFactBase = 0;

FunctionResult MemoryMarkCode(char* buffer)
{
	memoryMarkThreadList = AllocateHeapval(HV1_STRING|HV2_WORDP|HV3_FACTI,memoryMarkThreadList,
		(uint64)heapFree, (uint64)dictionaryFree, (uint64)Fact2Index(lastFactUsed)); 
	// start a new thread list of variable changes
	memoryVariableThreadList = AllocateHeapval(HV1_STRING,memoryVariableThreadList,
		(uint64)memoryVariableChangesThreadList, (uint64)0, (uint64)0);
	memoryVariableChangesThreadList = NULL;
	return NOPROBLEM_BIT;
}

void SetBaseMemory()
{
	memoryTextBase = heapFree;
	memoryDictBase = dictionaryFree;
	memoryFactBase = lastFactUsed;
}

static void ClearHeapRef2Mark(HEAPREF & ref, HEAPREF memory)
{
	// heap is allocated downwards so need to bring a reference to a heapval list back to a memory mark
	while (ref > (HEAPREF)0 && ref < memory)
	{
		ref = UnpackHeapval(ref, discard, discard);
	}
}

FunctionResult MemoryFreeCode(char* buffer)
{
	if (!memoryMarkThreadList) return FAILRULE_BIT;

	// copy pass data out of area to be cleared
	char* arg1 = ARGUMENT(1);
	char* copy = NULL;
	char word[MAX_WORD_SIZE];
	ReadCompiledWord(arg1, word);
	bool json = false;
	if (IsValidJSONName(word))
	{
		WORDP D = FindWord(word);
		if (!D) return FAILRULE_BIT;
		NextInferMark();
		unsigned int limit = jwritesize(D, 1, false) + 1000;  // bit of extra
		copy = AllocateStack(NULL, limit);
        NextInferMark();
		jwrite(copy, copy, D, 1, false, limit); // it is subject field
		json = true;
	}
	else if (*word) copy = AllocateStack(word);

	memset(wordStarts, 0, sizeof(char*) * MAX_SENTENCE_LENGTH);
	ClearWhereInSentence();

	uint64 memory;
	uint64 dict;
	uint64 fact;
	memoryMarkThreadList = UnpackHeapval(memoryMarkThreadList, memory, dict, fact);
	currentMemoryFact = (FACT*)Index2Fact((unsigned int)fact);
	ClearHeapRef2Mark(savedSentencesThreadList, (HEAPREF)memory);
	ClearHeapRef2Mark(userVariableThreadList, (HEAPREF)memory);
	//extern HEAPREF kernelVariableThreadList; before user layer
	for (int i = 1; i < MAX_SENTENCE_LENGTH; ++i)
	{
		ClearHeapRef2Mark(concepts[i], (HEAPREF)memory);
		ClearHeapRef2Mark(topics[i], (HEAPREF)memory);
	}

	// now cut back
	ClearUserVariables((HEAPREF)memory); // reset any above and delete from list but leave alone ones below
	ResetFactSystem(currentMemoryFact);// empties all fact sets and releases facts above marker
	DictionaryRelease((WORDP)dict, (HEAPREF)memory); // word & text

	// restore old variable values
	UnbindVariables(memoryVariableChangesThreadList);

	uint64 variableChanges;
	memoryVariableThreadList = UnpackHeapval(memoryVariableThreadList, variableChanges, discard, discard);
	memoryVariableChangesThreadList = (HEAPREF)variableChanges;

    ResetTokenSystem();
    ClearHeapRef2Mark(matchedWordsList, (HEAPREF)memory);

	if (copy && *copy)
	{
		if (json)
		{
			char wordx[MAX_WORD_SIZE];
			uint64 oldbot = myBot;
			myBot = 0; // universal access to this transient json
			InternalCall("^JSONParseCode", JSONParseCode, (char*)"TRANSIENT SAFE", copy, NULL, wordx);
			myBot = oldbot;
			strcpy(buffer, wordx);
		}
		else strcpy(buffer, copy);
		ReleaseStack(copy);
	}
	return NOPROBLEM_BIT;
}

static STACKREF MoveListToStack(HEAPREF list)
{
	// save property lists
	STACKREF tmplist = 0;
	while (list)
	{
		uint64 val1;
		uint64 val2;
		list = UnpackHeapval(list, val1, val2, discard);

		tmplist = AllocateStackval(tmplist, val1, val2, 0);
	}
	return tmplist;
}

#pragma warning(disable: 4068)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static HEAPREF MoveListToHeap(unsigned int descriptor,STACKREF list)
{
	HEAPREF tmplist = 0;
	while (list)
	{
		uint64 val1;
		uint64 val2;
		list = UnpackStackval(list, val1, val2, discard);
		tmplist = AllocateHeapval(descriptor,tmplist, val1, val2);
	}
	return tmplist;
}

#pragma GCC diagnostic pop

FunctionResult MemoryGCCode(char* buffer)
{
	if (currentBeforeLayer < LAYER_USER) return FAILRULE_BIT;
	// validate no facts are facts of facts for now
	FACT* F = factsPreBuild[LAYER_USER];
	while (++F <= lastFactUsed)
	{
		if (F->flags & (FACTSUBJECT | FACTVERB | FACTOBJECT)) return FAILRULE_BIT;
	}

	char* stackbase = AllocateStack(NULL, 1);

	// clear sentence data references, analyze is no longer possible
	savedSentencesThreadList = NULL;
	ResetTokenSystem();
	ClearWhereInSentence(); // clears lists concepts and topics.
	propertyRedefines = NULL;	// property changes on locked dictionary entries
	flagsRedefines = NULL; // system flag changes on locked dictionary entries
	for (unsigned int i = 0; i <= MAX_FIND_SETS; ++i) SET_FACTSET_COUNT(i, 0); // empty all factsets
	// BUG??? all HEAPINDEXS need to be relocated to stack???
																			   // userVariableThreadList   botVariableThreadList(cannot be wrong unless done during boot)
	// we ASSUME no facts are referred to by index on variables. 
	MigrateUserVariables();
	// dont have to account for locals, they are already in stack space
	// remove connections from facts to words.
	// If new word is only referred to by a dead fact, kill its text reference
	RipFacts(factsPreBuild[LAYER_USER], dictionaryPreBuild[LAYER_USER]); // unweave all facts back to marker

	// compress living facts
	F = factsPreBuild[LAYER_USER];
	FACT* openSlot = NULL;
	while (++F <= lastFactUsed)
	{
		if (F->flags & FACTDEAD) // need to move something to here
		{
			if (!openSlot) openSlot = F;
		}
		else if (openSlot) // we need to move living fact to overwrite dead fact
		{
			memmove(openSlot, F, sizeof(FACT));
			F->flags |= FACTDEAD;
			++openSlot;
			if (!(openSlot->flags & FACTDEAD)) openSlot = F;
		}
	}
	if (openSlot) lastFactUsed = openSlot; // the new free fact

	// erase dictionary living marks
	WORDP at = dictionaryFree;
	while (--at > dictionaryPreBuild[LAYER_USER])
	{
		at->internalBits &= -1 ^ BEEN_HERE;
		if (InHeap(at->word)) // true unless we erased it or it doesnt have a name
		{
			at->word = AllocateStack(at->word, 0);
		}
		if (*at->word == '$' && InHeap(at->word))
		{
			at->w.userValue = AllocateStack(at->w.userValue, 0);
		}
	}

	WeaveFacts(factsPreBuild[LAYER_USER]); // weave them back from marker

	// save property and systemflags lists
	STACKREF tmppropertylist = MoveListToStack(propertyRedefines);
	STACKREF tmpsystemflagslist = MoveListToStack(flagsRedefines);

	FlipResponses(); // save messages to user
	ResetHeapFree(heapPreBuild[LAYER_USER]); // chop back heap
	FlipResponses(); // restore messages to user

	// restore property and systemflags lists
	propertyRedefines = MoveListToStack(tmppropertylist);
	flagsRedefines = MoveListToStack(tmpsystemflagslist);

	// migrate stack dictionary words back to heap
	at = dictionaryFree;
	RecoverUserVariables(); // restore vars back to heap
	while (--at > dictionaryPreBuild[LAYER_USER])
	{
		at->internalBits &= -1 ^ BEEN_HERE;
		if (InStack(at->word)) // true unless we erased it or it doesnt have a name
		{
			at->word = AllocateHeap(at->word, 0);
		}
		if (*at->word == '$' && at->word[1] != '_' &&
			InStack(at->w.userValue))
		{
			at->w.userValue = AllocateHeap(at->w.userValue, 0);
		}
	}

	ReleaseStack(stackbase);
	return NOPROBLEM_BIT;
}

FunctionResult ClearContextCode(char* buffer)
{
	contextIndex = 0;
	return NOPROBLEM_BIT;
}


FunctionResult AddContextCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	int topicid = (!strcmp(arg1, (char*)"~")) ? currentTopicID : FindTopicIDByName(arg1);
	if (!topicid) return FAILRULE_BIT;
	AddContext(topicid, ARGUMENT(2));
	return NOPROBLEM_BIT;
}

FunctionResult BugCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	if (compiling && !strstr(arg1, "reportbug")) BADSCRIPT(arg1)
	else ReportBug(arg1);
	return NOPROBLEM_BIT;
}

FunctionResult ChangeBotCode(char* buffer)
{
	char* name = ARGUMENT(1);
	uint64 id = (uint64)atoi64(ARGUMENT(2));
	myBot = id;
	MakeLowerCopy(computerID, name);
	strcpy(computerIDwSpace + 1, computerID);
	strcat(computerIDwSpace, (char*)" "); // trailing space
	return NOPROBLEM_BIT;
}

FunctionResult InContextCode(char* buffer)
{
	int when = atoi(ARGUMENT(2)); // null == 0
	char* arg = ARGUMENT(1);
	char* dot = strchr(arg, '.');
	int topicid = currentTopicID;
	if (dot)
	{
		*dot = 0;
		topicid = FindTopicIDByName(arg);
		arg = dot + 1;
	}
	int turn = (int) InContext(topicid, arg);
	if (turn == 0) return FAILRULE_BIT;
	if (when && turn > when) return FAILRULE_BIT;

	sprintf(buffer, (char*)"%d", turn);
	return NOPROBLEM_BIT;
}

FunctionResult ArgumentCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	char* arg2 = ARGUMENT(2);
	if (!IsDigit(*arg1)) return FAILRULE_BIT;	// must be number index within some call
	if (*arg2 == '\'') ++arg2;
	int depth = globalDepth + 1;
	CALLFRAME* frame = GetCallFrame(depth - 1);
	while (--depth > 0 && *arg2)
	{
		frame = GetCallFrame(depth);
		if (!stricmp(frame->label, arg2)) break;
	}
	if (depth <= 0) return FAILRULE_BIT;	// could not find requested topic
	while (*frame->label != '^' || !frame->display)
	{
		--depth;
		frame = GetCallFrame(depth);
	}
	if (depth <= 0) return FAILRULE_BIT;

	unsigned int arg = frame->n_arguments;
	unsigned int requestedArg = atoi(arg1);
	if (requestedArg == 0) return NOPROBLEM_BIT;  // just checking to see if caller exists
	if (requestedArg > arg) return FAILRULE_BIT;	// not a legal arg value, too high
	char* limited;
	char* x = InfiniteStack(limited, "ArgumentCode");
	strcpy(buffer, GetArgOfMacro(frame,requestedArg, x, maxBufferSize));
	ReleaseInfiniteStack();
	return NOPROBLEM_BIT;
}

FunctionResult CallstackCode(char* buffer)
{
	if (*ARGUMENT(1) != '@') return FAILRULE_BIT;
	int n = GetSetID(ARGUMENT(1));
	if (n == ILLEGAL_FACTSET) return FAILRULE_BIT;

	int framecnt = 0;
	for (int i = 1; i < globalDepth; ++i) // dont include the call to callstack
	{
		CALLFRAME* frame = GetCallFrame(i);
		if (*(frame->label)) {
			if (framecnt >= MAX_FIND) return FAILRULE_BIT;
			MEANING M = MakeMeaning(StoreWord(frame->label));
			factSet[n][++framecnt] = CreateFact(M, MakeMeaning(StoreWord("callstack")), M, FACTTRANSIENT);
		}
	}
	SET_FACTSET_COUNT(n, framecnt);
	return NOPROBLEM_BIT;
}

static FunctionResult ClearMatchCode(char* buffer)
{
	for (int i = 0; i < MAX_WILDCARDS; ++i)
	{
		wildcardOriginalText[i][0] = 0;  //   spot wild cards can be stored
		wildcardCanonicalText[i][0] = 0;  //   spot wild cards can be stored
		wildcardPosition[i] = 0; //   spot it started and ended in sentence (16bit end 16bit start)
	}

	return NOPROBLEM_BIT;
}

//////////////////////////////////////////////////////////
/// DATABASE FUNCTIONS
//////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
/// WORD MANIPULATION
//////////////////////////////////////////////////////////
static FunctionResult SynsetCode(char* buffer)
{
	int set = impliedSet;
	if (impliedSet == ALREADY_HANDLED) set = 0;
	SET_FACTSET_COUNT(set, 0);
	impliedSet = ALREADY_HANDLED;
	bool master = false;
	bool up = false;
	char* word = ARGUMENT(1);   // simple word or synset meaning
	if (!stricmp(word, "synsetmaster"))
	{
		master = true;
		word = ARGUMENT(2);
	}
	else if (!stricmp(word, "synsetup"))
	{
		up = true;
		word = ARGUMENT(2);
	}
	MEANING M = MakeMeaning(StoreWord("^synset", AS_IS));
	char* tilde = strchr(word, '~');
	WORDP D;
	if (tilde) *tilde = 0;
	else if (master || up) return FAILRULE_BIT;
	D = FindWord(word);
	if (!D) return NOPROBLEM_BIT;
	int count = GetMeaningCount(D);
	if (!count) return NOPROBLEM_BIT;
	FACT* F;

	if (!tilde)
	{
		for (int i = 1; i <= count; ++i)
		{
			F = CreateFact(MakeMeaning(D, i), M, M, FACTTRANSIENT);
			AddFact(set, F);
		}
	}
	else
	{
		int index = atoi(tilde + 1);
		if (index > count) return FAILRULE_BIT; /// meaning out of range
		MEANING old = MakeMeaning(D, index);  // the one he gave us
		WORDP E = Meaning2Word(old);
		if (master || up)
		{
			MEANING X = GetMaster(old);
			if (up)
			{
				F = GetSubjectNondeadHead(X);
				while (F)
				{
					if (F->verb == Mis && F->subject == X) // up inheritance link of synset
					{
						X = F->object;
						break;
					}
					F = GetSubjectNondeadNext(F);
				}
				if (!F) return NOPROBLEM_BIT; // has no up (at top)
			}
			sprintf(buffer, "%s~%u", Meaning2Word(X)->word, Meaning2Index(X));
			return NOPROBLEM_BIT;
		}

		MEANING at = GetMeanings(D)[index] & (-1 ^ (TYPE_RESTRICTION | SYNSET_MARKER));  // synset ptr to next one
		unsigned int n = 0;
		while (true) // find the correct ptr to return. The marked ptr means OUR dict entry is master, not that the ptr points to.
		{
			F = CreateFact(at, M, M, FACTTRANSIENT);
			AddFact(set, F);
			if (at == old) break; // returned to meaning loop start
			D = Meaning2Word(at);
			index = Meaning2Index(at);
			at = GetMeanings(D)[index] & (-1 ^ (TYPE_RESTRICTION | SYNSET_MARKER));
			if (++n >= 40) break; // force an end arbitrarily
		}
	}
	currentFact = NULL;
	return NOPROBLEM_BIT;
}

static FunctionResult BurstCode(char* buffer) //   take value and break into facts of burst-items as subjects
{//   ^burst(^cause : )   1: data source 2: burst character  optional 0th argument is WORDCOUNT
 // burst(^ITEM letterdigit)
 //   prepare spot to store pieces
	MEANING verb;
	MEANING object;
	if (impliedWild != ALREADY_HANDLED)  SetWildCardIndexStart(impliedWild); //   start of wildcards to spawn
	object = verb = Mburst;
	*buffer = 0;
	bool count = false;
	char* ptr = ARGUMENT(1);  //   how/what to burst
	char* ptr2 = ARGUMENT(2); //   what to burst
	char* ptr3 = ARGUMENT(3); //   burst on
	if (*ptr == '"') // if a quoted string, remove the quotes
	{
		++ptr;
		size_t len = strlen(ptr);
		if (ptr[len - 1] == '"') ptr[len - 1] = 0;
	}
	bool once = false;
	CALLFRAME* frame = GetCallFrame(globalDepth);
	if (*ptr2 || *ptr3)
	{
		if (!stricmp(ptr, (char*)"count"))
		{
			count = true;
			frame->arguments[1] = frame->arguments[2];
			ptr = frame->arguments[1];
			frame->arguments[2] = frame->arguments[3];
			frame->arguments[3] = frame->arguments[4];
			frame->arguments[4] = frame->arguments[5];
		}
		else if (!stricmp(ptr, (char*)"once"))
		{
			once = true;
			frame->arguments[1] = frame->arguments[2];
			ptr = frame->arguments[1];
			frame->arguments[2] = frame->arguments[3];
			frame->arguments[3] = frame->arguments[42];
			frame->arguments[4] = frame->arguments[5];
		}
		else if (!stricmp(ptr, (char*)"allwords"))
		{
			// just consume the first argument
			frame->arguments[1] = frame->arguments[2];
			ptr = frame->arguments[1];
			frame->arguments[2] = frame->arguments[3];
			frame->arguments[3] = frame->arguments[4];
			frame->arguments[4] = frame->arguments[5];
		}
	}

	unsigned int counter = 0;
	if (count && *ptr) counter = 1; // there is at least one.
	char* arg1 = ARGUMENT(1);
	bool tracing = false;
	if (trace & TRACE_FLOW)
	{
		WORDP D = FindWord("^burst");
		if (D->internalBits & MACRO_TRACE || trace & TRACE_FACT) tracing = true;
	}
	if (tracing) Log(USERLOG, "  ");

	//   get string to search for. If quoted, remove the quotes
	char* scan = ARGUMENT(2);	//   how to burst
	char* scan1 = scan;
	bool digitsplit = false;
	if (!stricmp(scan, "digitsplit")) digitsplit = true;

	if (!*scan || !*scan1) // scan1 test just to suppress compiler warning
	{
		scan = " "; // default is space AND OR _
		scan1 = "_";
	}

	if (*ptr == '"') // if a quoted string, remove the quotes
	{
		++ptr;
		size_t len = strlen(ptr);
		if (ptr[len - 1] == '"') ptr[len - 1] = 0;
	}

	if (digitsplit)
	{
		char digits[MAX_WORD_SIZE];
		char* atdigit = digits;
		char letters[MAX_WORD_SIZE];
		if (IsDigit(*arg1) || (IsSign(*arg1) && IsDigit(arg1[1]))) // number first
		{
			if (IsSign(*arg1)) *atdigit++ = *arg1++;
			while (IsDigit(*arg1) || *arg1 == '.' || *arg1 == ',') *atdigit++ = *arg1++;
			*atdigit = 0;
			strcpy(letters, arg1);
		}
		else // number last
		{
			size_t len = strlen(arg1);
			while (IsDigit(arg1[--len]) || arg1[len] == '.' || arg1[len] == ',');
			if (IsSign(arg1[len])) --len;
			strcpy(digits, arg1 + len + 1);
			arg1[len + 1] = 0;
			strcpy(letters, arg1);
		}
		if (impliedWild != ALREADY_HANDLED)
		{
			SetWildCard(letters, letters, 0, 0);
			SetWildCard(digits, digits, 0, 0);
			impliedWild = ALREADY_HANDLED;
		}
		else if (impliedSet != ALREADY_HANDLED)
		{
			MEANING T = MakeMeaning(StoreWord(letters, AS_IS));
			FACT* F = CreateFact(T, verb, object, FACTTRANSIENT | FACTDUPLICATE);
			AddFact(impliedSet, F);
			T = MakeMeaning(StoreWord(digits, AS_IS));
			F = CreateFact(T, verb, object, FACTTRANSIENT | FACTDUPLICATE);
			AddFact(impliedSet, F);
			currentFact = NULL;
			impliedSet = ALREADY_HANDLED;
		}
		else sprintf(buffer, "%s %s", letters, digits);
		return NOPROBLEM_BIT;
	}

	if (*scan == '"') // if a quoted string as burst char, remove the quotes
	{
		++scan;
		size_t len = strlen(scan);
		if (scan[len - 1] == '"') scan[len - 1] = 0;
		if (*scan == 0) // burst into characters
		{
			--ptr; //   what to burst
			char word[MAX_WORD_SIZE];
			word[1] = 0;
			if (impliedSet != ALREADY_HANDLED) SET_FACTSET_COUNT(impliedSet, 0);
			while (*++ptr && ptr[1]) // leave rest for end
			{
				unsigned int charLen = UTFCharSize(ptr);
				strncpy(word, ptr, charLen);
				word[charLen] = 0;
				ptr += charLen - 1;
				if (tracing) Log(USERLOG, "     [%d]: %s\r\n", counter, word);
				++counter;
				//   store piece before scan marker
				if (impliedWild != ALREADY_HANDLED)  SetWildCard(word, word, 0, 0);
				else if (impliedSet != ALREADY_HANDLED)
				{
					MEANING T = MakeMeaning(StoreWord(word, AS_IS));
					FACT* F = CreateFact(T, verb, object, FACTTRANSIENT | FACTDUPLICATE);
					AddFact(impliedSet, F);
				}
				else if (!count) //   dump straight to output buffer, first piece only
				{
					strcpy(buffer, word);
					break;
				}
				if (once) break;
			}
			if (count) sprintf(buffer, (char*)"%u", counter); // just doing count
			if (impliedWild != ALREADY_HANDLED)
			{
				SetWildCard(ptr, ptr, 0, 0);
				SetWildCard((char*)"", (char*)"", 0, 0); // clear next one
			}
			else if (impliedSet != ALREADY_HANDLED)
			{
				if (*ptr) AddFact(impliedSet, CreateFact(MakeMeaning(StoreWord(ptr, AS_IS)), verb, object, FACTTRANSIENT | FACTDUPLICATE));
			}
			else if (!*buffer) strcpy(buffer, ptr);

			impliedSet = impliedWild = ALREADY_HANDLED;
			currentFact = NULL;
			return NOPROBLEM_BIT;
		}
		scan1 = scan;
	}

	//   loop that splits into pieces and stores them
	if (!scan[1]) // strip off leading and trailing separators, must occur BETWEEN tokens
	{
		while (*ptr == *scan) ++ptr;
		while (*ptr == *scan1) ++ptr;
		char* end = ptr + strlen(ptr) - 1;
		while (*end == *scan) *end-- = 0;
		while (*end == *scan1) *end-- = 0;
	}

	char* hold = strstr(ptr, scan);
	char* hold1 = (scan1) ? strstr(ptr, scan1) : NULL;
	if (!hold) hold = hold1; // only has second
	if (hold1 && hold1 < hold) hold = hold1; // sooner
	size_t scanlen = strlen(scan);

	if (impliedSet != ALREADY_HANDLED) SET_FACTSET_COUNT(impliedSet, 0);
	while (hold)
	{
		*hold = 0;	//   terminate first piece
		if (*ptr == 0) { ; } // null piece - breaking here makes no sense at start
		++counter;
		//   store piece before scan marker
		if (count) { ; }
		else if (impliedWild != ALREADY_HANDLED)  SetWildCard(ptr, ptr, 0, 0);
		else if (impliedSet != ALREADY_HANDLED)
		{
			if (*ptr)
			{
				MEANING T = MakeMeaning(StoreWord(ptr, AS_IS));
				FACT* F = CreateFact(T, verb, object, FACTTRANSIENT | FACTDUPLICATE);
				AddFact(impliedSet, F);
			}
		}
		else //   dump straight to output buffer, first piece only
		{
			strcpy(buffer, ptr);
			break;
		}
		if (tracing)
		{
			if (impliedWild != ALREADY_HANDLED) Log(USERLOG, "     [%d] _%d=> %s\r\n", counter, wildcardIndex - 1, ptr);
			else Log(USERLOG, "     [%d] %s\r\n", counter, ptr);
		}

		ptr = hold + scanlen; //   ptr after scan marker
		if (!*ptr) hold = 0;
		while (*ptr)
		{
			hold = strstr(ptr, scan);
			hold1 = (scan1) ? strstr(ptr, scan1) : NULL;
			if (!hold) hold = hold1; // only has second
			if (hold1 && hold1 < hold) hold = hold1; // sooner
			if (hold == ptr && !scan[1]) ++ptr;// there is an excess of single splits here
			else break;
		}

		if (once) break;
	}

	//   assign the last piece
	char result[MAX_WORD_SIZE];
	if (count) // just doing count
	{
		sprintf(result, (char*)"%u", counter);
		ptr = result;
	}
	if (impliedWild != ALREADY_HANDLED)
	{
		SetWildCard(ptr, ptr, 0, 0);
		SetWildCard((char*)"", (char*)"", 0, 0); // clear next one
	}
	else if (impliedSet != ALREADY_HANDLED)
	{
		if (*ptr) AddFact(impliedSet, CreateFact(MakeMeaning(StoreWord(ptr, AS_IS)), verb, object, FACTTRANSIENT | FACTDUPLICATE));
	}
	else if (!*buffer) strcpy(buffer, ptr);

	if (tracing)
	{
		if (impliedWild != ALREADY_HANDLED) Log(USERLOG, "    [ %d]: _%d=> %s ", counter, wildcardIndex - 2, ptr);
		else Log(USERLOG, "     [%d]: %s ", counter, ptr);
	}
	impliedSet = impliedWild = ALREADY_HANDLED;	//   we did the assignment
	currentFact = NULL; // should not advertise any created facts
	return NOPROBLEM_BIT;
}

static FunctionResult CanonCode(char* buffer)
{
#ifndef DISCARDSCRIPTCOMPILER
	if (!compiling) return FAILRULE_BIT;	 // only during script processing
	WriteCanon(ARGUMENT(1), ARGUMENT(2));
	return NOPROBLEM_BIT;
#else
	return FAILRULE_BIT;
#endif
}

static FunctionResult FlagsCode(char* buffer)
{
	WORDP D = FindWord(ARGUMENT(1));
	if (!D) return FAILRULE_BIT;
	strcpy(buffer, PrintU64(D->systemFlags));
	return NOPROBLEM_BIT;
}

static FunctionResult UppercaseCode(char* buffer)
{
	strcpy(buffer, (IsUpperCase(ARGUMENT(1)[0])) ? (char*)"1" : (char*)"0");
	return NOPROBLEM_BIT;
}

static FunctionResult PropertiesCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	WORDP D = FindWord(arg1, 0, PRIMARY_CASE_ALLOWED);
	if (!D) return FAILRULE_BIT;
	if (IS_NEW_WORD(D) && *ARGUMENT(2)) return FAILRULE_BIT; // optional second argument NO_NEW
	strcpy(buffer, PrintU64(D->properties));
	return NOPROBLEM_BIT;
}

static FunctionResult LayerCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	WORDP D = FindWord(arg1);
	if (!D) return FAILRULE_BIT;
	if (D->internalBits & BUILD0) strcpy(buffer, "0");
	else if (D->internalBits & BUILD1) strcpy(buffer, "1");
	else if (IS_NEW_WORD(D)) strcpy(buffer, "user");
	else strcpy(buffer, "wordnet");
	return NOPROBLEM_BIT;
}

static char* NextWord(char* ptr, WORDP & D, bool canon)
{
	char word[MAX_WORD_SIZE];
	ptr = ReadCompiledWord(ptr, word);
	MakeLowerCase(word);
	if (canon)
	{
		WORDP entry, canonical;
		uint64 sysflags = 0;
		uint64 cansysflags = 0;
		WORDP revise;
		char* oldheap = heapFree;
		WORDP olddict = dictionaryFree;
		GetPosData((unsigned int)-1, word, revise, entry, canonical, sysflags, cansysflags, true, true);
		if (canonical) strcpy(word, canonical->word);
		else if (entry) strcpy(word, entry->word);
		// revert any allocation made to dictionary for new word
		if (!monitorDictChanges) DictionaryRelease(olddict, oldheap);
	}
	MakeLowerCase(word);
	D = strlen(word) > 0 ? StoreWord(word) : NULL;
	return ptr;
}

static FunctionResult IntersectWordsCode(char* buffer)
{
	unsigned int store = (impliedSet == ALREADY_HANDLED) ? 0 : impliedSet;
	SET_FACTSET_COUNT(store, 0);
	WORDP words[2000];
	int index = 0;

	char* arg1 = ARGUMENT(1);
	if (*arg1 == '"')
	{
		size_t len = strlen(arg1);
		if (arg1[len - 1] == '"')
		{
			arg1[len - 1] = 0;
			++arg1;
		}
	}
	Convert2Blanks(arg1);
	char* at = arg1;
	while ((at = strchr(at, '~'))) *at = ' ';

	char* arg2 = ARGUMENT(2);
	if (*arg2 == '"')
	{
		size_t len = strlen(arg2);
		if (arg2[len - 1] == '"')
		{
			arg2[len - 1] = 0;
			++arg2;
		}
	}
	Convert2Blanks(arg2);
	at = arg2;
	while ((at = strchr(at, '~'))) *at = ' ';

	bool canon = (!stricmp(ARGUMENT(3), (char*)"canonical"));
	WORDP D;
	while (*arg1)
	{
		arg1 = NextWord(arg1, D, canon);
		if (D)
		{
			D->internalBits |= INTERNAL_MARK;
			words[++index] = D;
		}
	}

	unsigned int count = 0;
	if (index)
	{
		while (*arg2)
		{
			arg2 = NextWord(arg2, D, canon);
			if (D && D->internalBits & INTERNAL_MARK)
			{
				FACT* old = lastFactUsed;
				FACT* F = CreateFact(MakeMeaning(D), Mintersect, MakeMeaning(D), FACTTRANSIENT | FACTDUPLICATE);
				if (old != lastFactUsed)
				{
					AddFact(store, F);
					count = 1;
				}
			}
		}

		while (index) RemoveInternalFlag(words[index--], INTERNAL_MARK);
	}

	if (impliedSet == ALREADY_HANDLED && count == 0) return FAILRULE_BIT;
	impliedSet = ALREADY_HANDLED;
	return NOPROBLEM_BIT;
}

static FunctionResult JoinCode(char* buffer)
{
	char* original = buffer;
	char* ptr = ARGUMENT(1);
	bool autospace = false;
	if (!strnicmp(ptr, (char*)"AUTOSPACE", 9))
	{
		autospace = true;
		ptr += 10;
	}
	while (ptr)
	{
		char word[MAX_WORD_SIZE];
		char* at = ReadCompiledWord(ptr, word); // individual items will not be big in text
		if (*word == ')' || !*word) break; //   done
		size_t len = strlen(word);
		if (*word == '"' && word[1] == ' ' && word[2] == '"') // pure simple space
		{
			strcpy(buffer, (char*)" ");
			ptr = at;
		}
		else if (*word == '"' && word[1] == FUNCTIONSTRING) // compiled code being joined
		{
			FunctionResult result = NOPROBLEM_BIT;
			ReformatString(*word, word + 2, buffer, result, 0);
			if (result != NOPROBLEM_BIT) return result;
			ptr = at;
		}
		else if (*word == '"' && word[len - 1] == '"')
		{
			word[len - 1] = 0;
			strcpy(buffer, word + 1);
			ptr = at;
		}
		else
		{
			FunctionResult result;
			ptr = GetCommandArg(ptr, buffer, result, 0);
			if (!(result & ENDCODES)) {}
			else return result;
		}
		bool did = *buffer != 0;
		size_t l = strlen(buffer);
		if (*buffer == '"' && l > 2)
		{
			if (buffer[l - 1] == '"')
			{
				buffer[l - 1] = 0;
				memmove(buffer, buffer + 1, l);
				l -= 2;
			}
		}
		buffer += l;
		if (autospace && did) *buffer++ = ' ';
	}
	if (autospace && original != buffer) *--buffer = 0;
	return NOPROBLEM_BIT;
}

static FunctionResult PhraseCode(char* buffer)
{
	char* arg = ARGUMENT(1);
	char posn[MAX_WORD_SIZE];
	char type[MAX_WORD_SIZE];
	bool canonical = false;
	unsigned int n;
	FunctionResult result = NOPROBLEM_BIT;
	arg = ReadCompiledWord(arg, type);
	arg = ReadCompiledWord(arg, posn);
	if (!strnicmp(arg, (char*)"canonical", 9)) canonical = true;
	if (*posn == '\'') memmove(posn, posn + 1, strlen(posn));
	if (IsDigit(*posn))
	{
		n = atoi(posn);
		if (n == 0 || n > wordCount) return FAILRULE_BIT;
	}
	else if (*posn == '_')  n = WildStartPosition(posn);
	else if (*posn == '^')
	{
		ReadArgument(posn, buffer, result);
		n = atoi(buffer);
	}
	else if (*posn == USERVAR_PREFIX) n = atoi(GetUserVariable(posn, false));
	else return FAILRULE_BIT;
	unsigned int i = n;
	if (!stricmp(type, (char*)"noun")) // noun phrase
	{
		if (roles[i] & (MAINOBJECT | MAINSUBJECT) && verbals[i]) // like "to play football"
		{
			unsigned int x = verbals[i];
			if (!x) return FAILRULE_BIT;
			while (i && verbals[--i] & x) { ; }
			while (verbals[++n] & x) { ; };
			--n;
		}
		else if (roles[i] & (MAINOBJECT | MAINSUBJECT) && clauses[i]) // "I like *whatever tastes good"
		{
			unsigned int x = clauses[i];
			if (!x) return FAILRULE_BIT;
			while (i && clauses[--i] & x) { ; }
			while (clauses[++n] & x) { ; };
			--n;
		}
		else while (posValues[--i] & (ADJECTIVE_BITS | DETERMINER_BITS | POSSESSIVE | ADVERB)) { ; }
		// for now ignore , and conjunct coord grabbing like "my fat, luxurious file"
	}
	else if (!stricmp(type, (char*)"preposition")) // prep phrase
	{
		unsigned int x = phrases[n];
		if (!x) return FAILRULE_BIT;
		if (phrases[x] & phrases[startSentence])
		{
			strcat(buffer, wordStarts[startSentence]);
			strcat(buffer, (char*)"_");
			strcat(buffer, wordStarts[n]);
		}
		else while (i && phrases[--i] & x) { ; }
		while (phrases[n + 1] == x) ++n;	// find actual end
	}
	else if (!stricmp(type, (char*)"verbal")) // verbal phrase
	{
		unsigned int x = verbals[n];
		if (!x) return FAILRULE_BIT;
		while (i && verbals[++i] & x) { ; }
		int tmpx = i - 1;
		i = n;
		n = tmpx;
	}
	else if (!stricmp(type, (char*)"adjective")) // complement phrase
	{
		while (posValues[--i] & (ADJECTIVE_BITS | ADVERB)) { ; }
	}
	else return FAILRULE_BIT;
	if (n > wordCount) return FAILRULE_BIT;
	while (++i <= n)
	{
		if (canonical) strcat(buffer, wordCanonical[i]);
		else strcat(buffer, wordStarts[i]);
		if (i != n) strcat(buffer, wildcardSeparator);
	}
	return result;
}

static FunctionResult POSCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	char* arg2 = ARGUMENT(2);
	if (!arg1 || !arg2 || !*arg1) return FAILRULE_BIT;
	if (!*arg2) return NOPROBLEM_BIT;
	char* arg3 = ARGUMENT(3);
	char* arg4 = ARGUMENT(4);
	if (!stricmp(arg1, (char*)"ismodelnumber"))
	{
		if (IsModelNumber(arg2))
		{
			strcpy(buffer, "1");
			return NOPROBLEM_BIT;
		}
		else return FAILRULE_BIT;
	}
	if (!stricmp(arg1, (char*)"isfunction"))
	{
		if (*arg2 != '^')  // add function marker if it lacks one
		{
			memmove(arg2 + 1, arg2, strlen(arg2) + 1);
			*arg2 = '^';
		}
		WORDP D = FindWord(arg2, 0, LOWERCASE_LOOKUP);
		if (D && (D->internalBits & FUNCTION_NAME))
		{
			strcpy(buffer, "1");
			return NOPROBLEM_BIT;
		}
		else return FAILRULE_BIT;
	}
	if (!stricmp(arg1, (char*)"ismixedcase"))
	{
		int bits = 0;
		--arg2;
		while (*++arg2)
		{
			if (IsUpperCase(*arg2)) bits |= 1;
			if (IsLowerCase(*arg2)) bits |= 2;
		}
		if (bits == 3)
		{
			strcpy(buffer, "1");
			return NOPROBLEM_BIT;
		}
		else return FAILRULE_BIT;
	}
	if (!stricmp(arg1, (char*)"isinteger"))
	{
		if (IsInteger(arg2, false))
		{
			strcpy(buffer, "1");
			return NOPROBLEM_BIT;
		}
		else return FAILRULE_BIT;
	}
	if (!stricmp(arg1, (char*)"isuppercase"))
	{
		if (IsUpperCase(*arg2))
		{
			strcpy(buffer, "1");
			return NOPROBLEM_BIT;
		}
		else return FAILRULE_BIT;
	}
	if (!stricmp(arg1, (char*)"preexists"))
	{
		WORDP D = FindWord(arg2, 0, PRIMARY_CASE_ALLOWED);
		if (D && !IS_NEW_WORD(D))
		{
			strcpy(buffer, "1");
			return NOPROBLEM_BIT;
		}
		D = FindWord(arg2, 0, SECONDARY_CASE_ALLOWED);
		if (D && !IS_NEW_WORD(D))
		{
			strcpy(buffer, "1");
			return NOPROBLEM_BIT;
		}
		return FAILRULE_BIT;
	}
	if (!stricmp(arg1, (char*)"isalluppercase"))
	{
		if (IsAllUpper(arg2))
		{
			strcpy(buffer, "1");
			return NOPROBLEM_BIT;
		}
		else return FAILRULE_BIT;
	}
	if (!stricmp(arg1, (char*)"isfloat"))
	{
		if (IsFloat(arg2, arg2 + strlen(arg2)))
		{
			strcpy(buffer, "1");
			return NOPROBLEM_BIT;
		}
		else return FAILRULE_BIT;
	}
	if (!stricmp(arg1, (char*)"raw"))
	{
		int n = atoi(arg2);
		if (n < 1 || n >(int)wordCount) return FAILRULE_BIT;
		strcpy(buffer, wordStarts[n]);
		return NOPROBLEM_BIT;
	}
	if (!stricmp(arg1, (char*)"xref"))
	{
		int n = atoi(arg2);
		if (n < 1 || n >(int)wordCount) return FAILRULE_BIT;
		int xref = 0;
		// word number as xref
		if (!stricmp(arg3, (char*)"crossReference")) xref = crossReference[n];
		if (!stricmp(arg3, (char*)"indirectObject")) xref = indirectObjectRef[n];
		if (!stricmp(arg3, (char*)"object")) xref = objectRef[n];
		if (!stricmp(arg3, (char*)"complement")) xref = complementRef[n];
		// object number as xref
		if (!stricmp(arg3, (char*)"phrase")) xref = phrases[n];
		if (!stricmp(arg3, (char*)"verbal")) xref = verbals[n];
		if (!stricmp(arg3, (char*)"clause")) xref = clauses[n];
		sprintf(buffer, (char*)"%d", xref);
		return NOPROBLEM_BIT;
	}
	if (!stricmp(arg1, (char*)"conjugate"))
	{
		int64 pos;
		ReadInt64(arg3, pos);
		if (pos & (VERB_PRESENT_PARTICIPLE | NOUN_GERUND))
		{
			InternalCall("^POSCode", POSCode, "verb", ARGUMENT(2), "present_participle", buffer);
		}
		else if (pos & VERB_PAST_PARTICIPLE)
		{
			InternalCall("^POSCode", POSCode, "verb", ARGUMENT(2), "past_participle", buffer);
		}
		else if (pos & VERB_PAST)
		{
			if (!stricmp(arg2, (char*)"be") && pos & SINGULAR_PERSON) strcpy(buffer, (char*)"was"); // for 1st and 3rd person singular,  default is were
			else
			{
				InternalCall("^POSCode", POSCode, "verb", ARGUMENT(2), "past", buffer);
			}
		}
		else if (pos & VERB_PRESENT)
		{
			if (!stricmp(arg2, (char*)"be") && pos & SINGULAR_PERSON) strcpy(buffer, (char*)"am"); // default is is. good for are
			else
			{
				InternalCall("^POSCode", POSCode, "verb", ARGUMENT(2), "present", buffer);
			}
		}
		else if (pos & VERB_PRESENT_3PS)
		{
			InternalCall("^POSCode", POSCode, "verb", ARGUMENT(2), "present3ps", buffer);
		}
		else if (pos & NOUN_PLURAL || pos & NOUN_PROPER_PLURAL)
		{
			InternalCall("^POSCode", POSCode, "verb", ARGUMENT(2), "plural", buffer);
		}
		else if (pos & PLACE_NUMBER)
		{
			size_t len = strlen(arg2);
			char c = arg2[len - 1];
			int val = atoi(arg2);
			if ((val % 100) == 11 || (val % 100) == 12 || (val % 100) == 13) sprintf(buffer, (char*)"%dth", val);
			else if (c == '1') sprintf(buffer, (char*)"%sst", arg2);
			else if (c == '2') sprintf(buffer, (char*)"%snd", arg2);
			else if (c == '3') sprintf(buffer, (char*)"%srd", arg2);
			else if (IsDigit(*arg2)) sprintf(buffer, (char*)"%sth", arg2);
			else strcpy(buffer, arg2); // first, second, third etc
		}
		else if (pos & MORE_FORM && pos & ADJECTIVE)
		{
			InternalCall("^POSCode", POSCode, "adjective", ARGUMENT(2), "more", buffer);
		}
		else if (pos & MOST_FORM && pos & ADJECTIVE)
		{
			InternalCall("^POSCode", POSCode, "adjective", ARGUMENT(2), "most", buffer);
		}
		else if (pos & MORE_FORM && pos & ADVERB)
		{
			InternalCall("^POSCode", POSCode, "adverb", ARGUMENT(2), "more", buffer);
		}
		else if (pos & MOST_FORM && pos & ADVERB)
		{
			InternalCall("^POSCode", POSCode, "adverb", ARGUMENT(2), "most", buffer);
		}
		else if (pos & PRONOUN_POSSESSIVE)
		{
			if (!stricmp(arg2, (char*)"he")) strcpy(buffer, (char*)"his"); // currently we keep pronouns as is, but we might use canonical on them
			else if (!stricmp(arg2, (char*)"his")) strcpy(buffer, (char*)"his");
			else if (!stricmp(arg2, (char*)"she")) strcpy(buffer, (char*)"her");
			else if (!stricmp(arg2, (char*)"her")) strcpy(buffer, (char*)"her");
			else if (!stricmp(arg2, (char*)"it")) strcpy(buffer, (char*)"its");
			else if (!stricmp(arg2, (char*)"its")) strcpy(buffer, (char*)"its");
			else if (!stricmp(arg2, (char*)"they")) strcpy(buffer, (char*)"their");
			else if (!stricmp(arg2, (char*)"their")) strcpy(buffer, (char*)"their");
			else if (!stricmp(arg2, (char*)"you")) strcpy(buffer, (char*)"your");
			else if (!stricmp(arg2, (char*)"my")) strcpy(buffer, (char*)"my");
			else if (!stricmp(arg2, (char*)"I")) strcpy(buffer, (char*)"my");
		}
		else if (pos & PRONOUN_OBJECT)
		{
			if (!stricmp(arg2, (char*)"he")) strcpy(buffer, (char*)"him");
			else if (!stricmp(arg2, (char*)"she")) strcpy(buffer, (char*)"her");
			else if (!stricmp(arg2, (char*)"I")) strcpy(buffer, (char*)"me");
			else if (!stricmp(arg2, (char*)"they")) strcpy(buffer, (char*)"them");
			else if (!stricmp(arg2, (char*)"him")) strcpy(buffer, (char*)"him");
			else if (!stricmp(arg2, (char*)"her")) strcpy(buffer, (char*)"her");
			else if (!stricmp(arg2, (char*)"me")) strcpy(buffer, (char*)"me");
			else if (!stricmp(arg2, (char*)"them")) strcpy(buffer, (char*)"them");
			else strcpy(buffer, arg2);
		}
		else strcpy(buffer, arg2);
		return NOPROBLEM_BIT;
	}
	if (!stricmp(arg1, (char*)"syllable"))
	{
		sprintf(buffer, (char*)"%u", ComputeSyllables(arg2));
		return NOPROBLEM_BIT;
	}
	if (!stricmp(arg1, (char*)"substitute"))
	{
		MakeLowerCase(arg2); 
		WORDP D = FindWord(arg2);
		if (!D || !( D->systemFlags & HAS_SUBSTITUTE))
		{
			char* quote = strchr(arg2,'\'');
			if (quote)
			{
				memmove(quote + 1, quote, strlen(quote) + 1);
				*quote = '|'; // use joiner
				D = FindWord(arg2);
			}
		}
		if (D && D->systemFlags & HAS_SUBSTITUTE)
		{
			WORDP X = GetSubstitute(D); //uh - but we would, uh, , buy, .. lollipops
			if (X)
			{
				if (!strcmp(X->word, D->word)) return NOPROBLEM_BIT; // avoid infinite substitute
				char copy[MAX_WORD_SIZE];
				strcpy(copy, X->word);
				char* at = copy;
				while ((at = strchr(at, '+'))) *at = '_';
				if (!strcmp(copy, D->word)) return NOPROBLEM_BIT; // + and ` are synonymous
				uint64 allowed = tokenControl & (DO_SUBSTITUTE_SYSTEM | DO_PRIVATE);
				// there is no token control during build, so all subs are legal to consider
				if (!tokenControl || (allowed & D->internalBits)) strcpy(buffer, X->word);
			}
		}
		return NOPROBLEM_BIT;
	}
	if (!stricmp(arg1, (char*)"hex64"))
	{
		int64 num;
		ReadInt64(arg2, num);
		strcpy(buffer, PrintX64(num));
		return NOPROBLEM_BIT;
	}
	if (!stricmp(arg1, (char*)"hex32"))
	{
		int num;
		ReadInt(arg2, num);
		sprintf(buffer, (char*)"0x%08x", num);
		return NOPROBLEM_BIT;
	}
	if (!stricmp(arg1, (char*)"type"))
	{
		if (*arg2 == '~') strcpy(buffer, (char*)"concept");
		else if (IsDigit(*arg2)) strcpy(buffer, (char*)"number");
		else if (IsAlphaUTF8(*arg2)) strcpy(buffer, (char*)"word");
		else strcpy(buffer, (char*)"unknown");
		return NOPROBLEM_BIT;
	}
	if (!stricmp(arg1, (char*)"common"))
	{
		if (!arg2) return FAILRULE_BIT;
		WORDP D = FindWord(arg2, 0, PRIMARY_CASE_ALLOWED);
		if (!D) return NOPROBLEM_BIT;
		int level = CommonLevel(D->systemFlags);
		sprintf(buffer, (char*)"%d", level);
		return NOPROBLEM_BIT;
	}
	if (!stricmp(arg1, (char*)"grade"))
	{
		if (!arg2) return FAILRULE_BIT;
		WORDP D = FindWord(arg2, 0, PRIMARY_CASE_ALLOWED);
		if (!D) return NOPROBLEM_BIT;
		uint64 age = (D->systemFlags & AGE_LEARNED);
		if (age == KINDERGARTEN) strcpy(buffer, (char*)"0");
		else if (age == GRADE1_2) strcpy(buffer, (char*)"2");
		else if (age == GRADE3_4) strcpy(buffer, (char*)"4");
		else if (age == GRADE5_6) strcpy(buffer, (char*)"6");
		else strcpy(buffer, (char*)"8");
		return NOPROBLEM_BIT;
	}
	if (!stricmp(arg1, (char*)"verb"))
	{
		if (!arg2) return FAILRULE_BIT;
		char word[MAX_WORD_SIZE];
		MakeLowerCopy(word, arg2);
		char* infin = GetInfinitive(word, true);
		if (!infin) infin = word;
		if (!stricmp(arg3, (char*)"present_participle"))
		{
			char* use = GetPresentParticiple(word);
			if (!use) return FAILRULE_BIT;
			strcpy(buffer, use);
		}
		else if (!stricmp(arg3, (char*)"past_participle"))
		{
			char* use = GetPastParticiple(word);
			if (!use) return FAILRULE_BIT;
			strcpy(buffer, use);
		}
		else if (!stricmp(arg3, (char*)"infinitive"))
		{
			char* verb = GetInfinitive(word, true);
			if (!verb) return FAILRULE_BIT;
			strcpy(buffer, verb);
		}
		else if (!stricmp(arg3, (char*)"past"))
		{
			char* past = GetPastTense(infin);
			if (!stricmp(infin, (char*)"be"))
			{
				if (!stricmp(arg4, (char*)"I")) past = "was";
				else  past = "were";
			}
			if (!past) return FAILRULE_BIT;
			strcpy(buffer, past);
		}
		else if (!stricmp(arg3, (char*)"present3ps"))
		{
			char* third = GetThirdPerson(infin);
			if (!third) return FAILRULE_BIT;
			strcpy(buffer, third);
		}
		else if (!stricmp(arg3, (char*)"present"))
		{
			char* third = GetPresent(infin);
			if (!stricmp(infin, (char*)"be"))
			{
				if (!stricmp(arg4, (char*)"I")) third = "am";
				else if (!stricmp(arg4, (char*)"you") || !stricmp(arg4, (char*)"we") || !stricmp(arg4, (char*)"they")) third = "are";
				else  third = "is";
			}
			if (!stricmp(infin, (char*)"do"))
			{
				if (!stricmp(arg4, (char*)"I")) third = "do";
				else if (!stricmp(arg4, (char*)"you") || !stricmp(arg4, (char*)"we") || !stricmp(arg4, (char*)"they")) third = "do";
				else  third = "does";
			}
			if (!third) return FAILRULE_BIT;
			strcpy(buffer, third);
		}
		else if (!stricmp(arg3, (char*)"match"))
		{
			char wo[MAX_WORD_SIZE];
			char* plural = GetPluralNoun(arg4, wo);
			char* verb;
			if (!plural || stricmp(plural, arg4)) // singular noun
			{
				verb = GetThirdPerson(arg2);
				if (verb)  strcpy(buffer, verb);
			}
			else // plural noun
			{
				verb = GetInfinitive(arg2, false);
				if (verb)
				{
					if (!stricmp(verb, (char*)"be")) strcpy(buffer, (char*)"are");
					else strcpy(buffer, verb);
				}
			}
			if (!*buffer) strcpy(buffer, arg2);
		}
		if (IsUpperCase(ARGUMENT(2)[0])) *buffer = GetUppercaseData(*buffer);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"aux")) // (aux do you)
	{
		if (!arg2) return FAILRULE_BIT;
		char word[MAX_WORD_SIZE];
		MakeLowerCopy(word, ARGUMENT(2));
		char* result = word;

		if (!strcmp(arg2, (char*)"do")) //   present tense
		{
			if (strcmp(arg3, (char*)"I") && strcmp(arg3, (char*)"you") && strcmp(arg3, (char*)"we") && strcmp(arg3, (char*)"they")) result = "does";
			else result = "do";
		}
		else if (!strcmp(arg2, (char*)"have"))
		{
			if (strcmp(arg3, (char*)"I") && strcmp(arg3, (char*)"you") && strcmp(arg3, (char*)"we") && strcmp(arg3, (char*)"they")) result = "has";
			else result = "have";
		}
		else if (!strcmp(arg2, (char*)"be"))
		{
			if (!strcmp(arg3, (char*)"I")) result = "am";
			else if (!strcmp(arg3, (char*)"you")) result = "are";
			else if (!strcmp(arg3, (char*)"we") || !strcmp(arg3, (char*)"they"))  result = "are";
			else result = "is";
		}
		else if (!strcmp(arg2, (char*)"was") || !strcmp(arg2, (char*)"were")) //   past tense
		{
			if (!strcmp(arg3, (char*)"I")) result = "was";
			else result = "were";
		}
		else result = arg2;
		strcpy(buffer, result);
		if (IsUpperCase(arg2[0])) *buffer = GetUppercaseData(*buffer);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"pronoun"))
	{
		if (!stricmp(arg3, (char*)"flip"))
		{
			unsigned int n = BurstWord(arg2, 0);
			for (unsigned int i = 0; i < n; ++i)
			{
				char* word = GetBurstWord(i);
				if (!stricmp(word, (char*)"my")) word = "your";
				else if (!stricmp(word, (char*)"your")) word = "my";
				else if (!stricmp(word, (char*)"I")) word = "you";
				else if (!stricmp(word, (char*)"you")) word = "I";
				strcpy(buffer, word);
				buffer += strlen(buffer);
				if (i != (n - 1)) strcpy(buffer, (char*)" ");
				buffer += strlen(buffer);
			}
			return NOPROBLEM_BIT;
		}
	}
	else if (!stricmp(arg1, (char*)"adjective"))
	{
		if (!arg2) return FAILRULE_BIT;
		char word[MAX_WORD_SIZE];
		MakeLowerCopy(word, ARGUMENT(2));
		char* adj = word;
		if (!stricmp(arg3, (char*)"more"))
		{
			char* more = GetAdjectiveMore(adj);
			if (!more) sprintf(buffer, (char*)"more %s", adj);
			else strcpy(buffer, more);
		}
		else if (!stricmp(arg3, (char*)"most"))
		{
			char* most = GetAdjectiveMost(adj);
			if (!most) sprintf(buffer, (char*)"most %s", adj);
			else strcpy(buffer, most);
		}
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"adverb"))
	{
		if (!arg2) return FAILRULE_BIT;
		char word[MAX_WORD_SIZE];
		MakeLowerCopy(word, ARGUMENT(2));
		char* adv = word;
		if (!stricmp(arg3, (char*)"more"))
		{
			char* more = GetAdverbMore(adv);
			if (!more) sprintf(buffer, (char*)"more %s", adv);
			else strcpy(buffer, more);
		}
		else if (!stricmp(arg3, (char*)"most"))
		{
			char* most = GetAdverbMost(adv);
			if (!most) sprintf(buffer, (char*)"most %s", adv);
			else strcpy(buffer, most);
		}
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"noun"))
	{
		if (!stricmp(arg3, (char*)"proper"))
		{
			// we know the word, use its known casing for spelling
			WORDP D = FindWord(arg2, 0, UPPERCASE_LOOKUP);
			if (D)
			{
				strcpy(buffer, D->word);
				return NOPROBLEM_BIT;
			}

			// synthesize appropriate casing
			unsigned int n = BurstWord(arg2);
			for (unsigned int i = 0; i < n; ++i)
			{
				char* word = GetBurstWord(i);
				D = FindWord(word, 0, LOWERCASE_LOOKUP);
				if (D && D->properties & LOWERCASE_TITLE); //   allowable particles and connecting words that can be in lower case
				else *word = GetUppercaseData(*word);
				strcat(buffer, word);
				if (i != (n - 1)) strcat(buffer, (char*)" ");
			}
			return NOPROBLEM_BIT;
		}
		if (!stricmp(arg3, (char*)"lowercaseexist"))	// lower case legal?
		{
			WORDP D = FindWord(arg2, 0, LOWERCASE_LOOKUP);
			return (D) ? NOPROBLEM_BIT : FAILRULE_BIT;
		}
		if (!stricmp(arg3, (char*)"uppercaseexist"))	// upper case legal?
		{
			WORDP D = FindWord(arg2, 0, UPPERCASE_LOOKUP);
			return (D) ? NOPROBLEM_BIT : FAILRULE_BIT;
		}

		char* singularnoun = GetSingularNoun(arg2, true, false);
		if (!singularnoun) singularnoun = arg2; // nonworld like baronne
		char wo[MAX_WORD_SIZE];
		char* plural = GetPluralNoun(singularnoun, wo);
		if (!singularnoun && !plural) return FAILRULE_BIT; // not a noun at all?

		if (!stricmp(arg3, (char*)"singular") || (atoi(arg3) == 1 && !strchr(arg3, '.')) || !stricmp(arg3, (char*)"-1") || !stricmp(arg3, (char*)"one")) // allow number 1 but not float
		{
			if (singularnoun) strcpy(buffer, singularnoun);
			return NOPROBLEM_BIT;
		}
		else if (!stricmp(arg3, (char*)"plural") || IsDigit(arg3[0]) || (*arg3 == '-' && IsDigit(arg3[1]))) // allow number non-one and negative 1
		{
			//   swallow the args. for now we KNOW they are wildcard references
			if (plural) strcpy(buffer, plural);
			return NOPROBLEM_BIT;
		}
		else if (!stricmp(arg3, (char*)"irregular")) // generate a response only if plural is irregular from base (given)
		{
			//   swallow the args. for now we KNOW they are wildcard references
			if (!plural) return NOPROBLEM_BIT;
			size_t len = strlen(singularnoun);
			if (strnicmp(plural, singularnoun, len)) strcpy(buffer, plural); // show plural when base not in it
			return NOPROBLEM_BIT;
		}
	}
	else if (!stricmp(arg1, (char*)"determiner")) //   DETERMINER noun
	{
		size_t len = strlen(arg2);
		if (arg2[len - 1] == 'g' && arg2[len - 2] == 'n' && arg2[len - 3] == 'i' && GetInfinitive(arg2, false)) //   no determiner on gerund
		{
			strcpy(buffer, arg2);
			return NOPROBLEM_BIT;
		}
		//   already has one builtinto the word or phrase
		if (!strnicmp(arg2, (char*)"a_", 2) || !strnicmp(arg2, (char*)"an_", 3) || !strnicmp(arg2, (char*)"the_", 4))
		{
			strcpy(buffer, arg2);
			return NOPROBLEM_BIT;
		}

		char* singularnoun = GetSingularNoun(arg2, true, false);

		//   if a plural word, use no determiner
		if (!singularnoun || stricmp(arg2, singularnoun)) //   if has no singular or isnt same, assume we are plural and add the
		{
			sprintf(buffer, (char*)"the %s", arg2);
			return NOPROBLEM_BIT;
		}

		//   provide the determiner now
		*buffer++ = 'a';
		*buffer = 0;
		if (IsVowel(*arg2) || *arg2 == 'X' || *arg2 == 'x') *buffer++ = 'n'; //   make it "an"
		*buffer++ = ' ';	//   space before the word
		strcpy(buffer, arg2);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"place"))
	{
		int value = (int)Convert2Integer(arg2);
		if ((value % 100 == 11) || (value % 100) == 12 || (value % 100) == 13) sprintf(buffer, (char*)"%dth", value);
		else if ((value % 10) == 1) sprintf(buffer, (char*)"%dst", value);
		else if ((value % 10) == 2) sprintf(buffer, (char*)"%dnd", value);
		else if ((value % 10) == 3) sprintf(buffer, (char*)"%drd", value);
		else sprintf(buffer, (char*)"%dth", value);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"capitalize") || !stricmp(arg1, (char*)"uppercase"))
	{
		strcpy(buffer, arg2);
		*buffer = GetUppercaseData(*buffer);
		char* at = buffer;
		while (*++at)
		{
			if (*at == ' ' || *at == '_' || *at == '-') at[1] = GetUppercaseData(at[1]);
		}

		// some words in titles should be lower case
		int n = BurstWord(buffer, 0); 
		char* start = buffer;
		if (n > 1)
		{
			char* under = strchr(buffer, '_'); // confirm separator type
			*buffer = 0;
			for (int i = 0; i < n; ++i)
			{
				char* hold = burstWords[i];
				if (!hold) break;
				strcpy(buffer, hold);
				if (i > 0 && i < (n-1) )
				{
					WORDP D = FindWord(hold,0,LOWERCASE_LOOKUP);
					// these should be lower case in titles if not 1st word or last word
					// short prepositions or conjunctions or determiners (a an the)
					if (D && D->properties & (DETERMINER | PREPOSITION | CONJUNCTION) && WORDLENGTH(D) < 4)
					{
						buffer = strcpy(buffer, D->word);
					}
				}
				if (i > 0 && buffer[0] == 'D' && buffer[1] == '\'') *buffer = 'd';  // d'
				buffer += strlen(buffer);
				*buffer++ = (under) ? '_' : ' ';
			}
			*--buffer = 0; // trim trailing separtor
		}
	
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"mixcase"))
	{
		strcpy(buffer, arg2);
		char* at = buffer;
		while (*++at) // ignore 1st letter
		{
			if (*at == ' ' || *at == '_' || *at == '-') ++at;   // leave alone first letter after separator
			else at[1] = GetLowercaseData(at[1]); // make it non cap
		}
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"allupper"))
	{
		strcpy(buffer, arg2);
		MakeUpperCase(buffer);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"lowercase"))
	{
		MakeLowerCopy(buffer, arg2);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"canonical"))
	{
		WORDP entry = NULL, canonical = NULL;
		uint64 sysflags = 0;
		uint64 cansysflags = 0;
		WORDP revise;
		if (*arg2)
		{
			if (*arg3)
			{
				if (*arg3 != '~' && stricmp(arg3, (char*)"all")) return FAILRULE_BIT;

				WORDP D = FindWord(arg2);
				if (D)
				{
					WORDP E = RawCanonical(D);
					if (E && *E->word == '`')
					{
						char canons[MAX_WORD_SIZE]; // canonicals separated by `
						char postags[MAX_WORD_SIZE]; // Postags separated by spaces, no ~

						strcpy(canons, E->word);
						char* end = strrchr(canons, '`');
						*++end = 0;

						if (!stricmp(arg3, (char*)"all"))
						{
							*--end = 0;
							char* ptr = canons;
							while ((ptr = strchr(ptr + 1, '`')))
							{
								*ptr = '|';
							}
							strcpy(buffer, canons + 1);
							return NOPROBLEM_BIT;
						}

						strcpy(postags, end + 1);
						char* ptr = postags;

						char tag[MAX_WORD_SIZE];
						int p = -1;
						while (*ptr) {
							p++;
							ptr = ReadCompiledWord(ptr, tag);
							if (!stricmp(tag, arg3 + 1)) {
								// found the postag we are seeking
								ptr = canons;
								for (int i = 0; i < p; ++i) {
									ptr = strchr(++ptr, '`');
								}
								char* canon = ++ptr;
								ptr = strchr(ptr, '`');
								*ptr = 0;

								canonical = FindWord(canon);
								break;
							}
						}
					}
				}
			}
			char* oldheap = heapFree;
			WORDP olddict = dictionaryFree;
			if (!canonical) GetPosData((unsigned int)-1, arg2, revise, entry, canonical, sysflags, cansysflags, true, true);
			// revert any allocation made to dictionary for new word
			if (!monitorDictChanges) DictionaryRelease(olddict, oldheap);
		}
		if (canonical) strcpy(buffer, canonical->word);
		else if (entry) strcpy(buffer, entry->word);
		else strcpy(buffer, arg2);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"isword"))
	{
		--arg2;
		while (IsAlphaUTF8(*++arg2));
		if (IsAlphaUTF8(*arg2))
		{
			strcpy(buffer, "1");
			return NOPROBLEM_BIT;
		}
		else return FAILRULE_BIT;
	}
	else if (!stricmp(arg1, (char*)"integer"))
	{
		strcpy(buffer, arg2);
		char* period = strchr(arg2, '.');
		if (period)
		{
			double val = Convert2Double(arg2);
			*period = 0;
			int64 vali;
			ReadInt64(arg2, vali);
			if ((double)vali == val) strcpy(buffer, arg2);
		}
		return NOPROBLEM_BIT;
	}
	return FAILRULE_BIT;
}

static void RhymeWord(WORDP D, uint64 flag)
{
	if (!(D->properties & (NOUN | VERB | ADJECTIVE | ADVERB | DETERMINER_BITS | PRONOUN_BITS | CONJUNCTION | PREPOSITION | AUX_VERB))) return;
	if (D->word && !IsAlphaUTF8(*D->word)) return;
	if (D->properties & (NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL | NOUN_TITLE_OF_ADDRESS)) return;	// want ordinary words
	if (strchr(D->word, '_') || strchr(D->word, '-')) return; // only normal words and not multi words either

	char* tail = (char*)flag;
	size_t n = strlen(tail);
	size_t len = strlen(D->word);
	if (len <= n) return;  // too short to rhyme
	if (D->word[len - 1] != tail[n - 1] || D->word[len - 2] != tail[n - 2]) return;	// must end the same way for last 2 letters
	if (!stricmp(D->word, tail)) return; // cannot have whole match
	if (!IsVowel(tail[n - 1]) && !IsVowel(tail[n - 2]) && D->word[len - 3] != tail[n - 3]) return;	// if 2 consonant ending, vowel before must match also
	if ((len - n) > 3) return; // should be similar in size

	if (!IsVowel(tail[n - 1]) && IsVowel(tail[n - 2]))// if vowel-consonant ending, then before must match type also
	{
		if (IsVowel(tail[n - 3]) && IsVowel(D->word[len - 3])) { ; }
		else if (!IsVowel(tail[n - 3]) && !IsVowel(D->word[len - 3])) { ; }
		else return;	// if 2 consonant ending, vowel before must match also
	}
	if (!IsVowel(tail[n - 2]) && IsVowel(tail[n - 1])) //if consonant vowel ending, then before must match character before also
	{
		if (tail[n - 3] != D->word[len - 3]) return;
	}

	if (FACTSET_COUNT(rhymeSet) >= 500) return; //   limit
	WORDP E = StoreWord((char*)"1");
	AddFact(spellSet, CreateFact(MakeMeaning(E, 0), MakeMeaning(StoreWord((char*)"word")), MakeMeaning(D, 0), FACTTRANSIENT | FACTDUPLICATE));
}

static FunctionResult RhymeCode(char* buffer)
{
	int set = (impliedSet == ALREADY_HANDLED) ? 0 : impliedSet;
	SET_FACTSET_COUNT(set, 0);
	rhymeSet = set;
	WalkDictionary(RhymeWord, (uint64)ARGUMENT(1));
	if (FACTSET_COUNT(set))
	{
		impliedSet = ALREADY_HANDLED;
		return NOPROBLEM_BIT;
	}

	return FAILRULE_BIT;
}

static FunctionResult  RecordDictionaryChangesCode(char* buffer)
{
	monitorDictChanges = atoi(ARGUMENT(1)) != 0;
	unsigned int index = atoi(ARGUMENT(2));
	unsigned int index1 = atoi(ARGUMENT(3));
	if (monitorDictChanges)
	{
		ongoingDictChanges = (index) ? Index2Heap(index) : NULL;	 // resume old list or start again
		ongoingUniversalDictChanges = (index) ? Index2Heap(index1) : NULL;	 // resume old list or start again
	}

	else sprintf(buffer, "%u %u", Heap2Index(ongoingDictChanges), Heap2Index(ongoingUniversalDictChanges));
	return NOPROBLEM_BIT;
}

static FunctionResult RevertDictionaryChangesCode(char* buffer)
{
	monitorDictChanges = false;
	HEAPREF start = ongoingDictChanges;
	HEAPREF startUniversal = ongoingUniversalDictChanges;
	unsigned int index = atoi(ARGUMENT(1));
	if (index) start = Index2Heap(index);
	if (!start) return FAILRULE_BIT;
	ReverseDictionaryChanges(start);
	ReverseUniversalDictionaryChanges(start);
	sprintf(buffer, "%u", Heap2Index(start)); // give him this list
	return NOPROBLEM_BIT;
}

static FunctionResult  ReplayDictionaryChangesCode(char* buffer)
{
	monitorDictChanges = false;
	unsigned int index = atoi(ARGUMENT(1));
	if (!index) return FAILRULE_BIT;
	HEAPREF start = Index2Heap(index);

	// reverse order

	STACKREF reverse = NULL;
	uint64 val1, val2, val3;
	while (start)
	{
		start = UnpackHeapval(start, val1, val2, val3);
		reverse = AllocateStackval(reverse, val1, val2, val3);
	}

	while (reverse)
	{
		reverse = UnpackStackval(reverse, val1, val2, val3);
		WORDP D = (WORDP)val1;
		if (D)
		{
			D->properties = val2;
			D->systemFlags = val3;
		}
	}
	return NOPROBLEM_BIT;
}
static FunctionResult FindTextCode(char* buffer)
{
	// what to search in
	char* target = ARGUMENT(1);
	if (!*target) return FAILRULE_BIT;

	// find value
	char* find = ARGUMENT(2);
	if (!*find) return FAILRULE_BIT;
	else if (*find == '\\' && find[1] == 't') find = "\t";
	else if (*find == '\\' && find[1] == 'n') find = "\n";
	else if (*find == '\\' && find[1] == '\\' && find[2] == 't') find = "\\t";
	else if (*find == '\\' && find[1] == '\\' && find[2] == 'n') find = "\\n";
	else if (*find == '"')
	{
		size_t len = strlen(find);
		find[len - 1] = 0;
		memmove(find, find + 1, len);
	}

	unsigned int start = atoi(ARGUMENT(3));
	if (start >= UTFStrlen(target)) return FAILRULE_BIT;

	char* buf = AllocateStack(target, 0); // so we dont lose real blanks status
	if (*target != '_' && *find != '_') Convert2Blanks(target); // if we explicitly request or are looking for  _, use it
	if (*find != '_') Convert2Blanks(find); // if we explicitly request _, use it

	if (strstr(ARGUMENT(4), (char*)"insensitive"))
	{
		MakeLowerCase(find);
		MakeLowerCase(target);
	}

	char* found;
	size_t len = UTFStrlen(find);
	unsigned int startPos = UTFPosition(target, start);
	found = strstr(target + startPos, find); // case sensitive
	if (found)
	{
		unsigned int offset = UTFOffset(target, found);
		char word[MAX_WORD_SIZE];
		sprintf(buffer, (char*)"%d", (int)(offset + len)); // where it ended (not part of it)
		sprintf(word, (char*)"%d", (int)offset);
		found = buf + offset;	// change reference frame back to original

		int count = 1;
		char* at = buf - 1; // original input before _ conversion
		bool nonblank = false;
		while (*++at)
		{
			bool quote = false;
			if (*at == '"' && *(at - 1) != '\\') quote = !quote;
			else if (!quote && *at == ' ' && nonblank) // ignore contiguous blanks and string blanks
			{
				++count;	// will be next wor
				nonblank = false;
			}
			if (*at != ' ') nonblank = true;
			if (at >= found) break;	// started with this word
		}

		SetUserVariable((char*)"$$findtext_start", word); // where it started
		sprintf(word, (char*)"%d", count);
		SetUserVariable((char*)"$$findtext_word", word); // where it started as words
	}
	ReleaseStack(buf);
	if (!found)  return FAILRULE_BIT;
	return NOPROBLEM_BIT;
}

static FunctionResult ExtractCode(char* buffer)
{
	// what to search in
	char* target = ARGUMENT(1);
	if (!*target) return FAILRULE_BIT;
	size_t len = UTFStrlen(target);

	bool startFromEnd = false;
	char* arg2 = ARGUMENT(2);
	if (*arg2 == '-') {
		startFromEnd = true;
		++arg2; // remove sign
	}

	char* arg3 = ARGUMENT(3);
	int offset = 0;
	if (*arg3 == '-') offset = -1;
	else if (*arg3 == '+') offset = 1;
	if (offset != 0) ++arg3; // remove sign

	if (!IsDigit(*arg2)) 
		return FAILRULE_BIT;
	if (!IsDigit(*arg3)) 
		return FAILRULE_BIT;

	int start = atoi(arg2);
	int end = atoi(arg3);
	if (startFromEnd) start = len - start;
	if (offset > 0) end = start + end; // moving forwards
	else if (offset < 0)
	{ // moving backwards  -8 -5 means end 8 before end, run backwards 5 char toward start
		// move from start backwards for end length
		int tmpx = end;
		end = start; // ending here
		start = end - tmpx; // starting here
		// move backwards from end - start   for len
		if (start < 0) start = 0;
	}
	if (start >= (int)len) 
		return  FAILRULE_BIT;
	if (end > ((int)len + 1)) end = len + 1;
	if (end < start) 
		return FAILRULE_BIT;
	unsigned int startPos = UTFPosition(target, start);
	unsigned int endPos = UTFPosition(target, end);
	if (endPos < startPos) 
		return FAILRULE_BIT;
	unsigned int numChars = endPos - startPos;
	strncpy(buffer, target + startPos, numChars);
	buffer[numChars] = 0;
	return NOPROBLEM_BIT;
}

static FunctionResult SubstituteCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	bool wordMode = false;
	bool insensitive = false;
	if (*arg1 == '"') ++arg1;
	char word[MAX_WORD_SIZE];
	while (*arg1)
	{
		arg1 = ReadCompiledWord(arg1, word);
		if (!*word) break;
		if (*word == 'c' || *word == 'C') wordMode = false;
		if (*word == 'w' || *word == 'W') wordMode = true;
		if (*word == 'i' || *word == 'I') insensitive = true;
	}
	char* xxoriginal = buffer;	// for debug
								// adjust substitution value
	char* substituteValue = ARGUMENT(4);
	size_t substituteLen = strlen(substituteValue);
	if (substituteLen > 1 && *substituteValue == '"' && substituteValue[substituteLen - 1] == '"') // quoted expression means use the interior of it
	{
		substituteValue[substituteLen - 1] = 0;
		++substituteValue;
		substituteLen -= 2;
	}
	// if (*substituteValue != '_') Convert2Blanks(substituteValue); // if we explicitly request _, use it  but Task_Test will be ruined

	// what to search in, held in local copy because may be made all lowercase
	char* copy = AllocateBuffer();
	*copy = ' '; // protective leading blank for -1 test
	char* arg2 = ARGUMENT(2);
	size_t xlen = strlen(arg2);
	strcpy(copy + 1, arg2);
	char* target = copy + 1;
	// target may be null string

	// find value
	char* find = ARGUMENT(3);
	if (!*find)
	{
		FreeBuffer();
		return FAILRULE_BIT;
	}
	size_t findLen = strlen(find);
	if (findLen > 1 && *find == '"' && find[1] != '"' && find[findLen - 1] == '"') // find of a quoted thing means use interior
	{
		find[findLen - 1] = 0;
		++find;
		findLen -= 2;
	}
	if (findLen == 0)
	{
		FreeBuffer();   /// using buffer where no collision is expected
		return FAILRULE_BIT; // can never make headway
	}
	char* found;
	bool changed = false;
	strcpy(buffer, arg2);
	char* target2 = buffer;
	size_t subslen = strlen(substituteValue);
	if (insensitive)
	{
		MakeLowerCase(find);
		MakeLowerCase(target);  // only changes local searching copy, not output
	}
	char* buf = AllocateBuffer();
	while ((found = strstr(target, find))) // case sensitive
	{
		char* found2 = target2 + (found - target);  // corresponding position in output buffer
		target = found + findLen;  // next place to search from
								   // no partial matches
		if (wordMode)
		{
			char c = found[findLen];
			if (IsAlphaUTF8OrDigit(c) || IsAlphaUTF8OrDigit(*(found - 1))) // skip nonword match
			{
				target2 = found2 + findLen;
				continue;
			}
		}
		changed = true;
		strcpy(buf, found2 + findLen); // preserve what comes after the match
		strcpy(found2, substituteValue);
		strcat(found2, buf);
		target2 = found2 + subslen;
	}

	// check for FAIL request
	FreeBuffer();
	FreeBuffer();
	char* notify = ARGUMENT(5);
	if (*notify || impliedIf != ALREADY_HANDLED) return (changed) ? NOPROBLEM_BIT : FAILRULE_BIT; // if user wants possible failure result
	return NOPROBLEM_BIT;
}

static void SpellOne(WORDP D, uint64 data)
{
	char* match = (char*)data;
	if (FACTSET_COUNT(spellSet) >= 500) return; //   limit

	if (!(D->properties & (NOUN | VERB | ADJECTIVE | ADVERB | DETERMINER_BITS | PRONOUN_BITS | CONJUNCTION | PREPOSITION | AUX_VERB))) return;
	if (D->word && !IsAlphaUTF8(*D->word)) return;
	if (D->properties & (NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL | NOUN_TITLE_OF_ADDRESS)) return;	// want ordinary words
	if (strchr(D->word, '_')) return; // only normal words and not multi words either
	if (MatchesPattern(D->word, match))
	{
		WORDP E = StoreWord((char*)"1");
		AddFact(spellSet, CreateFact(MakeMeaning(E, 0), MakeMeaning(StoreWord((char*)"word")), MakeMeaning(D, 0), FACTTRANSIENT | FACTDUPLICATE));
	}
}

static unsigned int  Spell(char* match, unsigned int set)
{
	char pattern[MAX_WORD_SIZE];
	SET_FACTSET_COUNT(set, 0);
	if (match[1] == '-') match[1] = 0;	// change 4-letter to 4
	MakeLowerCopy(pattern, match);
	spellSet = set;
	WalkDictionary(SpellOne, (uint64)pattern);
	return FACTSET_COUNT(set);
}

static FunctionResult SpellCode(char* buffer) //- locates up to 100 words in dictionary matching pattern and stores them as facts in @0
{
#ifdef INFORMATION
	Fails if no words are found.Words must begin with a letterand be marked as a part of speech
	(noun, verb, adjective, adverb, determiner, pronoun, conjunction, prepostion).

		Not all words are found in the dictionary.The system only stores singular nounsand base
		forms of verbs, adverbs, and adjectives unless it is irregular.

		Pattern is a sequence of characters, with* matching 0 or more characters and .matching
		exactly one.Pattern must cover the entire string.Pattern may be prefixed with a number, which
		indicates how long the word must be.E.g.

		^ spell((char*)"4*")	# retrieves 100 4 - letter words
		^ spell((char*)"3a*")  # retrieves 3 - letter words beginning with "a"
		^ spell((char*)"*ing") # retrieves words ending in "ing"
#endif
		int factset = GetSetID(ARGUMENT(2));
	if (factset == ILLEGAL_FACTSET) factset = 0;
	return (Spell(ARGUMENT(1), factset)) ? NOPROBLEM_BIT : FAILRULE_BIT;
}

static FunctionResult SexedCode(char* buffer)
{
	WORDP D = FindWord(ARGUMENT(1));
	if (!D || !(D->properties & (NOUN_HE | NOUN_SHE))) strcpy(buffer, ARGUMENT(4)); //   it 
	else if (D->properties & NOUN_HE) strcpy(buffer, ARGUMENT(2)); //   he
	else strcpy(buffer, ARGUMENT(3)); //   she
	return NOPROBLEM_BIT;
}

static FunctionResult TallyCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	if (!*arg1) return FAILRULE_BIT;
	WORDP D = StoreWord(arg1);
	char* arg2 = ARGUMENT(2);
	if (!*arg2) sprintf(buffer, (char*)"%d", GetWordValue(D));
	else SetWordValue(D, atoi(arg2));
	return NOPROBLEM_BIT;
}

static FunctionResult WordsCode(char* buffer)
{
	WORDP set[GETWORDSLIMIT];
	char* arg1 = ARGUMENT(1);
	if (!*arg1)	return FAILRULE_BIT;
	if (*arg1 == '"')
	{
		++arg1;
		size_t len = strlen(arg1);
		if (arg1[len - 1] == '"') arg1[len - 1] = 0;	 //remove quotes
	}
	int i = GetWords(arg1, set, false);
	MEANING M = MakeMeaning(StoreWord("words"));
	while (i)
	{
		FACT* F = CreateFact(MakeMeaning(set[--i]), M, M, FACTTRANSIENT);
		if (impliedSet != ALREADY_HANDLED)
		{
			AddFact(impliedSet, F);
		}
	}

	impliedSet = ALREADY_HANDLED;
	return NOPROBLEM_BIT;
}

static bool WordPropogate(MEANING at, MEANING find)
{
	if (at == find) return true;
	WORDP D = Meaning2Word(at);
	if (D->inferMark == inferMark) return false;
	D->inferMark = inferMark;
	FACT* F = GetSubjectHead(D);
	// propogate to find set
	while (F)
	{
		if (F->verb == Mmember && AcceptableFact(F) && WordPropogate(F->object, find)) return true;
		F = GetSubjectNext(F);
	}

	return false;
}

static FunctionResult WordInConceptCode(char* buffer)
{
	WORDP set[GETWORDSLIMIT];
	char* arg1 = ARGUMENT(1);
	if (!*arg1)	return FAILRULE_BIT;
	if (*arg1 == '"')
	{
		++arg1;
		size_t len = strlen(arg1);
		if (arg1[len - 1] == '"') arg1[len - 1] = 0;	 //remove quotes
	}
	char* arg2 = ARGUMENT(2);
	if (*arg2 != '~') return FAILRULE_BIT;
	MEANING find = MakeMeaning(StoreWord(arg2));
	int i = GetWords(arg1, set, false);
	NextInferMark();
	while (i)
	{
		MEANING M = MakeMeaning(set[--i]);
		WORDP D = Meaning2Word(M);
		FACT* F = GetSubjectNondeadHead(D);
		// propogate to find set
		while (F)
		{
			if (F->verb == Mmember && WordPropogate(F->object, find))
			{
				sprintf(buffer, "%s", Meaning2Word(F->subject)->word);
				return NOPROBLEM_BIT;
			}
			F = GetSubjectNondeadNext(F);
		}
	}
	char* arg3 = ARGUMENT(3);
	if (*arg3) return FAILRULE_BIT;

	return NOPROBLEM_BIT;
}

#ifndef DISCARDCOUNTER
static FunctionResult WordCountCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	if (!*arg1)	return FAILRULE_BIT;
	WORDP D = StoreWord(arg1);
	char* arg2 = ARGUMENT(2);
	if (!*arg2) sprintf(buffer, (char*)"%u", D->counter);
	else D->counter = atoi(arg2);
	return NOPROBLEM_BIT;
}
#endif

static char* xbuffer;

static void DWalker(WORDP D, uint64 fn)
{
	if (*D->word == USERVAR_PREFIX || *D->word == ':' || *D->word == '^' || *D->word == '~' || *D->word == SYSVAR_PREFIX || *D->word == ENDUNIT || *D->word == '"') return; // not real stuff
	if (D->internalBits & CONDITIONAL_IDIOM) return;

	if (D->systemFlags & HAS_SUBSTITUTE) return;
	if (D->properties & (PUNCTUATION | COMMA | PAREN | QUOTE)) return; // " will cause a crash
	if (strchr(D->word, ' ')) return;
	FunctionResult result;
	char* function = (char*)fn;
	char word[MAX_WORD_SIZE];
	sprintf(word, (char*)"( %s )", D->word);
	DoFunction(function, word, xbuffer, result);
	xbuffer += strlen(xbuffer);
}

static FunctionResult WalkDictionaryCode(char* buffer)
{
	FunctionResult result;
	xbuffer = buffer;
	char function[MAX_WORD_SIZE];
	char* ptr = ReadFunctionCommandArg(ARGUMENT(1), function, result);
	if (result != NOPROBLEM_BIT) return result;
	WalkDictionary(DWalker, (uint64)function);
	return NOPROBLEM_BIT;
}

static FunctionResult WalkVariablesCode(char* buffer)
{
	FunctionResult result;
	xbuffer = buffer;
	char function[MAX_WORD_SIZE];
	char* ptr = ReadFunctionCommandArg(ARGUMENT(1), function, result);
	if (result != NOPROBLEM_BIT) return result;

	HEAPREF varthread = userVariableThreadList;
	while (varthread)
	{
		uint64 entry;
		varthread = UnpackHeapval(varthread, entry, discard, discard);
		WORDP D = (WORDP)entry;
		if (D->word[1] != TRANSIENTVAR_PREFIX && D->word[1] != LOCALVAR_PREFIX && D->w.userValue) // transients not dumped, nor are NULL values
		{
			char wordx[MAX_WORD_SIZE];
			sprintf(wordx, (char*)"( %s )", D->word);
			*buffer = 0;
			DoFunction(function, wordx, buffer, result);
		}
	}
	return NOPROBLEM_BIT;
}

static FunctionResult WalkTopicsCode(char* buffer)
{
	FunctionResult result;
	xbuffer = buffer;
	char function[MAX_WORD_SIZE];
	char* ptr = ReadFunctionCommandArg(ARGUMENT(1), function, result);
	if (result != NOPROBLEM_BIT) return result;
	WalkTopics(function, buffer);
	return NOPROBLEM_BIT;
}

//////////////////////////////////////////////////////////
/// DICTIONARY
//////////////////////////////////////////////////////////

static FunctionResult GetPropertyCodes(char* who, char* ptr, uint64 & val, uint64 & sysval, unsigned int& internalBits, unsigned int& parseBits)
{
	while (ptr && *ptr)
	{
		char arg[MAX_WORD_SIZE];
		FunctionResult result = NOPROBLEM_BIT;
		if (*ptr == USERVAR_PREFIX) ptr = ReadShortCommandArg(ptr, arg, result, OUTPUT_NOQUOTES);
		else ptr = ReadCompiledWord(ptr, arg);
		if (!*arg || *arg == ')') break;
		if (*arg == '^' && IsDigit(arg[1])) strcpy(arg, FNVAR(arg));
		if (!stricmp(arg, (char*)"CONCEPT"))
		{
			if (*who != '~') return FAILRULE_BIT; // must be a concept name
		}
		else  if (!stricmp(arg, (char*)"HAS_SUBSTITUTE"))
		{
			sysval |= HAS_SUBSTITUTE;
		}

		// fact marks
		else if (IsDigit(arg[0])) ReadInt64(arg, (int64&)sysval);
		else
		{
			uint64 bits = FindPropertyValueByName(arg);
			if (bits) val |= bits;
			else {
				bits = FindSystemValueByName(arg);
				if (!bits)
				{
					bits = FindParseValueByName(arg);
					if (!bits)
					{
						bits = FindMiscValueByName(arg); // goes to sysval
						if (!bits) Log(USERLOG, "Unknown addproperty value %s\r\n", arg);
						else sysval |= bits;
					}
					else parseBits |= bits;
				}
				else sysval |= bits;
			}
		}
	}
	return (!sysval && !val && !internalBits) ? FAILRULE_BIT : NOPROBLEM_BIT;
}

static FunctionResult AddPropertyCode(char* buffer)
{
	char* ptr = ARGUMENT(1);
	char arg1[MAX_WORD_SIZE];
	FunctionResult result = NOPROBLEM_BIT;
	if (*ptr == '@') ptr = ReadCompiledWord(ptr, arg1); // dont eval a set
	else ptr = ReadShortCommandArg(ptr, arg1, result, OUTPUT_NOQUOTES);
	if (result != NOPROBLEM_BIT) return result;
	if (!*arg1) return FAILRULE_BIT;
	WORDP D = NULL;
	int store = 0;
	unsigned int count = 0;
	char arg3 = 0;
	if (*arg1 == '@') // add property to all facts in set either on a field or fact as a whole
	{
		store = GetSetID(arg1);
		if (store == ILLEGAL_FACTSET) return FAILRULE_BIT;
		count = FACTSET_COUNT(store);
		arg3 = *GetSetType(arg1);
	}
 	else  D = StoreWord(arg1, 0); // add property to dictionary word
	uint64 val = 0;
	uint64 sysval = 0;
	unsigned int internalBits = 0;
	unsigned int parseBits = 0;
	result = GetPropertyCodes(arg1, ptr, val, sysval, internalBits, parseBits);
	if (result != NOPROBLEM_BIT) return result;
	if (!compiling)
		dictionaryBitsChanged = true;
	if (D) // add to dictionary entry
	{
		if (val & NOUN_SINGULAR && D->internalBits & UPPERCASE_HASH) //make it right
		{
			val ^= NOUN_SINGULAR;
			val |= NOUN_PROPER_SINGULAR;
		}
		AddProperty(D, val);
		AddSystemFlag(D, sysval);
	}
	else if (*arg1 == '@') // add to all properties of fact set
	{
		for (unsigned int i = 1; i <= count; ++i)
		{
			FACT* F = factSet[store][i];
			if (arg3 == 's') D = Meaning2Word(F->subject);
			else if (arg3 == 'v') D = Meaning2Word(F->verb);
			else if (arg3 == 'o') D = Meaning2Word(F->object);
			else
			{
				// cannot mark a fact as dead if not in current layer
				if (F <= factsPreBuild[currentBeforeLayer] && sysval & FACTDEAD) {
					sysval ^= FACTDEAD;
				}
				F->flags |= sysval;
				if (trace & TRACE_INFER && CheckTopicTrace()) TraceFact(F);
			}
			if (D)
			{
				uint64 val1 = val;
				if (val1 & NOUN_SINGULAR && D->internalBits & UPPERCASE_HASH) //make it right
				{
					val1 ^= NOUN_SINGULAR;
					val1 |= NOUN_PROPER_SINGULAR;
				}
				AddProperty(D, val1);
				AddSystemFlag(D, sysval);
				if (trace & TRACE_INFER && CheckTopicTrace()) Log(USERLOG, " %s\r\n", D->word);
			}
		}
	}
	return NOPROBLEM_BIT;
}

static FunctionResult DefineCode(char* buffer)
{
	char* w = ARGUMENT(1);

	char* tilde = strchr(w, '~');
	if (tilde) *tilde = 0; // remove meaning id for now
	WORDP D = FindWord(w, 0);
	if (!D) return NOPROBLEM_BIT;
	if (tilde)
	{
		int index = atoi(tilde + 1);
		int count = GetMeaningCount(D);
		if (index > count) return FAILRULE_BIT;
		char* gloss = GetGloss(D, index);
		strcpy(buffer, gloss);
		return NOPROBLEM_BIT;
	}

	bool noun = false;
	bool verb = false;
	bool adjective = false;
	bool adverb = false;
	char* which = ARGUMENT(2);
	bool doall = false;
	if (!stricmp(which, (char*)"all"))
	{
		doall = true;
		which = "";
	}
	if (!stricmp(ARGUMENT(3), (char*)"all")) doall = true;

	for (int i = 1; i <= GetMeaningCount(D); ++i)
	{
		MEANING T = GetMaster(GetMeaning(D, i)) | GETTYPERESTRICTION(GetMeaning(D, i));
		int index = Meaning2Index(T);
		WORDP E = Meaning2Word(T);
		char* gloss = GetGloss(E, index);
		unsigned int restrict = GETTYPERESTRICTION(T);
		if (gloss && restrict & NOUN && !noun && (!*which || !stricmp(which, (char*)"NOUN")))
		{
			if (verb) sprintf(buffer, (char*)"As a noun it means %s. ", gloss);
			else sprintf(buffer, (char*)"The noun %s means %s. ", ARGUMENT(1), gloss);
			if (!doall) noun = true;
			else strcat(buffer, (char*)"\n");
			buffer += strlen(buffer);

		}
		else if (gloss && restrict & VERB && !verb && (!*which || !stricmp(which, (char*)"VERB")))
		{
			if (noun) sprintf(buffer, (char*)"As a verb it means %s. ", gloss);
			else sprintf(buffer, (char*)"The verb %s means %s. ", ARGUMENT(1), gloss);
			if (!doall) verb = true;
			else strcat(buffer, (char*)"\n");
			buffer += strlen(buffer);
		}
		else if (gloss && restrict & ADJECTIVE && !noun && !verb && !adjective && (!*which || !stricmp(which, (char*)"ADJECTIVE")))
		{
			sprintf(buffer, (char*)"The adjective %s means %s. ", ARGUMENT(1), gloss);
			if (!doall) adjective = true;
			else strcat(buffer, (char*)"\n");
			buffer += strlen(buffer);
		}
		else if (gloss && restrict & ADVERB && !adverb && !noun && !verb && !adjective && (!*which || !stricmp(which, (char*)"ADVERB")))
		{
			sprintf(buffer, (char*)"The adverb %s means %s. ", ARGUMENT(1), gloss);
			if (!doall) adverb = true;
			else strcat(buffer, (char*)"\n");
			buffer += strlen(buffer);
		}
	}
	return NOPROBLEM_BIT;
}

static void ArgFlags(uint64 & properties, uint64 & flags, unsigned int& internalbits)
{
	char* arg2 = ARGUMENT(2);
	char* arg3 = ARGUMENT(3);
	properties = FindPropertyValueByName(arg2);
	properties |= FindPropertyValueByName(arg3);
	properties |= FindPropertyValueByName(ARGUMENT(4));
	properties |= FindPropertyValueByName(ARGUMENT(5));
	properties |= FindPropertyValueByName(ARGUMENT(6));

	flags = FindSystemValueByName(arg2);
	flags |= FindSystemValueByName(arg3);
	flags |= FindSystemValueByName(ARGUMENT(4));
	flags |= FindSystemValueByName(ARGUMENT(5));
	flags |= FindSystemValueByName(ARGUMENT(6));

	internalbits = 0;
	if (!stricmp(arg2, (char*)"CONCEPT") || !stricmp(arg3, (char*)"CONCEPT") || !stricmp(ARGUMENT(4), (char*)"CONCEPT") ||
		!stricmp(ARGUMENT(5), (char*)"CONCEPT") || !stricmp(ARGUMENT(6), (char*)"CONCEPT"))
	{
		// internalbits |= CONCEPT;  // no need to mark anymore, it just is
	}
	if (!stricmp(arg2, (char*)"TOPIC") || !stricmp(arg3, (char*)"TOPIC") || !stricmp(ARGUMENT(4), (char*)"TOPIC") ||
		!stricmp(ARGUMENT(5), (char*)"TOPIC") || !stricmp(ARGUMENT(6), (char*)"TOPIC"))
	{
		internalbits |= TOPIC;
	}
	if (!stricmp(arg2, (char*)"HAS_SUBSTITUTE") || !stricmp(arg3, (char*)"HAS_SUBSTITUTE") || !stricmp(ARGUMENT(4), (char*)"HAS_SUBSTITUTE") ||
		!stricmp(ARGUMENT(5), (char*)"HAS_SUBSTITUTE") || !stricmp(ARGUMENT(6), (char*)"HAS_SUBSTITUTE"))
	{
		flags |= HAS_SUBSTITUTE;
	}
}

static FunctionResult HasAnyPropertyCode(char* buffer)
{
	WORDP canonical = NULL;
	uint64 dprop;
	uint64 dsys;
	char* arg = ARGUMENT(1);
	WORDP D = FindWord(arg, 0, PRIMARY_CASE_ALLOWED);
	WORDP revise;
	char* oldheap = heapFree;
	WORDP olddict = dictionaryFree;
	if (!D)  GetPosData((unsigned int) -1, arg, revise, D, canonical, dprop, dsys, true, true);  // WARNING- created dict entry if it doesnt exist yet
	else
	{
		dsys = D->systemFlags;
		dprop = D->properties;
	}
	uint64 properties;
	uint64 flags;
	unsigned int internalbits;
	// revert any allocation made to dictionary for new word
	if (!monitorDictChanges) DictionaryRelease(olddict, oldheap);

	ArgFlags(properties, flags, internalbits);
	return (dprop & properties || dsys & flags || D->internalBits & internalbits) ? NOPROBLEM_BIT : FAILRULE_BIT;
}

static FunctionResult HasAllPropertyCode(char* buffer)
{
	WORDP canonical = NULL;
	uint64 dprop = 0;
	uint64 dsys;
	char* arg = ARGUMENT(1);
	WORDP D = FindWord(arg, 0, PRIMARY_CASE_ALLOWED);
	WORDP revise;
	char* oldheap = heapFree;
	WORDP olddict = dictionaryFree;
	if (!D)  GetPosData((unsigned int)-1, arg, revise, D, canonical, dprop, dsys, true, true);
	else
	{
		dsys = D->systemFlags;
		dprop = D->properties;
	}
	uint64 properties;
	uint64 flags;
	unsigned int internalbits;
	ArgFlags(properties, flags, internalbits);
	// revert any allocation made to dictionary for new word
	if (!monitorDictChanges) DictionaryRelease(olddict, oldheap);

	if (!flags && !properties) return FAILRULE_BIT;
	if (D->word[0] == '~' &&  (D->internalBits & TOPIC)) return FAILRULE_BIT;
	return ((dprop & properties) == properties && (dsys & flags) == flags && (D->internalBits & internalbits) == internalbits) ? NOPROBLEM_BIT : FAILRULE_BIT; // has all the bits given
}

static FunctionResult RemovePropertyCode(char* buffer)
{
	char* ptr = ARGUMENT(1);
	char arg1[MAX_WORD_SIZE];
	FunctionResult result = NOPROBLEM_BIT;
	if (*ptr == '@') ptr = ReadCompiledWord(ptr, arg1); // dont eval a set
	else ptr = ReadShortCommandArg(ptr, arg1, result, OUTPUT_NOQUOTES);
	if (result != NOPROBLEM_BIT) return result;
	char arg3 = *GetSetType(arg1);
	if (!*arg1) return FAILRULE_BIT;
	WORDP D = NULL;
	int store = -1;
	unsigned int count = 0;
	if (*arg1 == '@')
	{
		store = GetSetID(arg1);
		if (store == ILLEGAL_FACTSET) return FAILRULE_BIT;
		count = FACTSET_COUNT(store);
	}
	else if (*arg1 == '"')
	{
		size_t len = strlen(arg1);
		if (arg1[len - 1] == '"') arg1[len - 1] = 0;
		D = FindWord(arg1, len - 2);
	}
	else  D = FindWord(arg1, 0);
	if (!D && store == -1) return FAILRULE_BIT;

	uint64 val = 0;
	uint64 sysval = 0;
	unsigned int internalBits = 0;
	unsigned int parseBits = 0;
	result = GetPropertyCodes(arg1, ptr, val, sysval, internalBits, parseBits);
	if (result != NOPROBLEM_BIT) return result;
	if (D) // remove to dictionary entry
	{
		RemoveProperty(D, val);
		RemoveSystemFlag(D, sysval);
		RemoveInternalFlag(D, internalBits);
	}
	else // remove to all properties of set
	{
		for (unsigned int i = 1; i <= count; ++i)
		{
			FACT* F = factSet[store][i];
			if (arg3 == 's') D = Meaning2Word(F->subject);
			else if (arg3 == 'v') D = Meaning2Word(F->verb);
			else if (arg3 == 'o') D = Meaning2Word(F->object);
			else
			{
				F->flags &= -1 ^ sysval; // uses miscflags retrieval
				if (trace & TRACE_INFER && CheckTopicTrace()) TraceFact(F);
			}
			if (D)
			{
				RemoveProperty(D, val);
				RemoveSystemFlag(D, sysval);
				if (trace & TRACE_INFER && CheckTopicTrace()) Log(USERLOG, " %s\r\n", D->word);
			}
		}
	}
	return NOPROBLEM_BIT;
}


//////////////////////////////////////////////////////////
/// MULTIPURPOSE
//////////////////////////////////////////////////////////

static FunctionResult DisableCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	char* arg2 = ARGUMENT(2);
	if (!stricmp(arg1, (char*)"topic"))
	{
		if (!*arg2) return FAILRULE_BIT;
		int id = FindTopicIDByName(ARGUMENT(2));
		if (id)
		{
			if (GetTopicFlags(id) & TOPIC_SYSTEM) return FAILRULE_BIT;
			if (!(GetTopicFlags(id) & TOPIC_BLOCKED)) AddTopicFlag(id, TOPIC_BLOCKED | TOPIC_USED);
			return NOPROBLEM_BIT;
		}
	}
	else if (!stricmp(arg1, (char*)"rule")) // 1st one found
	{
		if (planning) return FAILRULE_BIT;
		int id = 0;
		int topicid = currentTopicID;
		bool fulllabel;
		bool crosstopic;
		char* rule;
		char* dot = strchr(arg2, '.');
		if (*arg2 == '~' && !arg2[1])  // current rule
		{
			rule = currentRule;
			id = currentRuleID;
		}
		else if (dot && IsDigit(dot[1])) rule = GetRuleTag(topicid, id, arg2);
		else rule = GetLabelledRule(topicid, arg2, (char*)"", fulllabel, crosstopic, id, currentTopicID);
		if (!rule) return FAILRULE_BIT;
		SetRuleDisableMark(topicid, id);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"rejoinder") || !stricmp(arg1, (char*)"outputrejoinder"))
	{
		outputRejoinderRuleID = NO_REJOINDER;
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"inputrejoinder"))
	{
		inputRejoinderRuleID = NO_REJOINDER;
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"save"))
	{
		stopUserWrite = true;
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(arg1, (char*)"write"))
	{
		int set = GetSetID(ARGUMENT(1));
		if (set == ILLEGAL_FACTSET) return FAILRULE_BIT;
		setControl &= -1ll ^ (1ull << set);
		return NOPROBLEM_BIT;
	}

	return FAILRULE_BIT;
}

static FunctionResult EnableCode(char* buffer)
{
	char* arg2 = ARGUMENT(2);
	if (!stricmp(ARGUMENT(1), (char*)"topic"))
	{
		//   topic name to enable
		if (!*arg2) return FAILRULE_BIT;
		if (!stricmp(arg2, (char*)"all"))
		{
			for (unsigned int start = 1; start <= numberOfTopics; ++start)
			{

				if (GetTopicFlags(start) & TOPIC_SYSTEM) continue;
				RemoveTopicFlag(start, TOPIC_BLOCKED);
			}
			return NOPROBLEM_BIT;
		}
		int id = FindTopicIDByName(arg2);
		if (!id) return FAILRULE_BIT;
		if (GetTopicFlags(id) & TOPIC_SYSTEM) return FAILRULE_BIT;
		RemoveTopicFlag(id, TOPIC_BLOCKED);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(ARGUMENT(1), (char*)"rule"))
	{
		if (planning) return FAILRULE_BIT;
		int id = 0;
		int topicid = currentTopicID;
		bool fulllabel;
		bool crosstopic;
		char* rule;
		char* dot = strchr(arg2, '.');
		if (dot && IsDigit(dot[1])) rule = GetRuleTag(topicid, id, arg2);
		else rule = GetLabelledRule(topicid, arg2, ARGUMENT(3), fulllabel, crosstopic, id, currentTopicID);
		if (!rule) return FAILRULE_BIT;
		UndoErase(rule, topicid, id);
		AddTopicFlag(topicid, TOPIC_USED);
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(ARGUMENT(1), (char*)"save"))
	{
		stopUserWrite = false;
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(ARGUMENT(1), (char*)"usedrules")) // rules turned off this volley as we went along
	{
		FlushDisabled();
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(ARGUMENT(1), (char*)"write"))
	{
		int set = GetSetID(ARGUMENT(2));
		if (set == ILLEGAL_FACTSET) return FAILRULE_BIT;
		setControl |= (1ull << set);
		return NOPROBLEM_BIT;
	}
	return FAILRULE_BIT;
}

FunctionResult LengthCode(char* buffer)
{
	char* word = ARGUMENT(1);
	if (*word == '@' && IsDigit(word[1])) // how many facts in factset
	{
		int store = GetSetID(word);
		if (store == ILLEGAL_FACTSET) sprintf(buffer, (char*)"%d", (int)strlen(word)); // characters in word
		else
		{
			unsigned int count = FACTSET_COUNT(store);
			sprintf(buffer, (char*)"%u", count);
		}
	}
	else if (*word == '~') // how many top level members in set
	{
		WORDP D = FindWord(word, 0);
		if (!D) return FAILRULE_BIT;
		int count = 0;
		FACT* F = GetObjectNondeadHead(D);
		while (F)
		{
			if (F->verb == Mmember)  ++count;
			F = GetObjectNondeadNext(F);
		}
		sprintf(buffer, (char*)"%d", count);
	}
	else if (IsValidJSONName(word)) // elements in a json array or object
	{
		int count = 0;
		WORDP D = FindWord(word);
		if (D)
		{
			FACT* F = GetSubjectNondeadHead(D);
			while (F)
			{
				++count;
				F = GetSubjectNondeadNext(F);
			}
		}
		sprintf(buffer, (char*)"%d", count);
	}
	else if (!*word) strcpy(buffer, (char*)"0"); // NULL has 0 length (like a null value array)
	else sprintf(buffer, (char*)"%u", UTFStrlen(word)); // characters in word
	return NOPROBLEM_BIT;
}

static FunctionResult NextCode(char* buffer)
{
	char* ptr = ARGUMENT(1); // GAMBIT or RESPONDER or RULE OR FACT or INPUT
	char arg1[MAX_WORD_SIZE];
	char arg2[MAX_WORD_SIZE];
	ptr = ReadCompiledWord(ptr, arg1);
	ReadCompiledWord(ptr, arg2);
	if (stricmp(arg1, (char*)"FACT") || *arg2 != '@') // eval all but FACT @1subjecct
	{
		FunctionResult result;
		GetCommandArg(ptr, buffer, result, OUTPUT_UNTOUCHEDSTRING);
		size_t len = strlen(buffer);
		if (len >= MAX_WORD_SIZE) return FAILRULE_BIT;
		strcpy(arg2, buffer);
	}
	if (!stricmp(arg1, (char*)"LOOP"))
		return NEXTLOOP_BIT;
	if (!stricmp(arg1, (char*)"FACT"))
	{
		CALLFRAME* frame = GetCallFrame(globalDepth);
		frame->arguments[1] = AllocateStack(arg2);
		return FLR(buffer, (char*)"n");
	}
	if (!stricmp(arg1, (char*)"INPUT") || !stricmp(arg1, (char*)"SENTENCE")) // same meaning
	{
		if (NO_API_CALL != csapicall) return FAILRULE_BIT;

		SAVEOLDCONTEXT()
			* buffer = 0;
		while ((ptr = GetNextInput())) // revise inputs until prepass doesnt change them
		{
			if (!ptr) return FAILRULE_BIT;

			PrepareSentence(ptr, true, true);

			if (!wordCount && (GetNextInput() || (responseIndex != 0))) // ignore this input
			{
				RESTOREOLDCONTEXT()
					return NOPROBLEM_BIT;
			}
			if (!PrepassSentence(GetUserVariable("$cs_prepass", false))) break; // it was quiet
		}
		if (!wordCount) return FAILRULE_BIT;
		++inputSentenceCount; //  sentence id of volley has moved on
		RESTOREOLDCONTEXT()
	}
	else  // gambit, responder, rule, REJOINDER
	{
		bool gambit = (*arg1 == 'G' || *arg1 == 'g');
		bool responder = !stricmp(arg1, (char*)"responder");
		bool rejoinder = !stricmp(arg1, (char*)"rejoinder");
		int topicid = currentTopicID;
		int id;
		bool fulllabel = false;
		bool crosstopic = false;
		char* rule;
		char* dot = strchr(arg2, '.');
		if (dot && IsDigit(dot[1])) rule = GetRuleTag(topicid, id, arg2);
		else rule = GetLabelledRule(topicid, arg2, arg2, fulllabel, crosstopic, id, currentTopicID);
		if (!rule) return FAILRULE_BIT; // unable to find labelled rule 

		char* data = rule;
		while (data)
		{
			data = FindNextRule((gambit || responder) ? NEXTTOPLEVEL : NEXTRULE, data, id);
			if (!data || !*data) break;

			if (gambit && TopLevelGambit(data)) break;
			else if (responder && (TopLevelStatement(data) || TopLevelQuestion(data) || TopLevelVoid(data))) break;
			else if (rejoinder && Rejoinder(data)) break;
			else if (rejoinder) return FAILRULE_BIT;	// no more rejoinders
			else if (!gambit && !responder && !rejoinder) break;	// any next rule
		}
		if (!data || !*data) return FAILRULE_BIT;
		sprintf(buffer, (char*)"%s.%u.%u", GetTopicName(topicid), TOPLEVELID(id), REJOINDERID(id));
	}
	return NOPROBLEM_BIT;
}

static FunctionResult FLRCodeR(char* buffer)
{
	char* arg = ARGUMENT(1);
	CALLFRAME* frame = GetCallFrame(globalDepth);
	GetPossibleFunctionArgument(arg, buffer); // pass thru or convert
	arg = frame->arguments[1] = AllocateStack(buffer); // put it back in case it changed
	*buffer = 0;

	if (*arg == USERVAR_PREFIX) arg = GetUserVariable(arg, false);
	else if (*arg == '_') arg = GetwildcardText(GetWildcardID(arg), true);

	if (*arg == '@') return FLR(buffer, (char*)"r");
	else if (*arg == '~')  return RandomMember(buffer, arg);
	else if (IsValidJSONName(arg))
	{
		WORDP X = FindWord(arg);
		FACT* F = (X) ? GetSubjectNondeadHead(X) : NULL;
		int count = 0;
		while (F)
		{
			++count;
			F = GetSubjectNondeadNext(F);
		}
		count = random(count); // which one to pick (0 ..n)
		F = (X) ? GetSubjectNondeadHead(X) : NULL;
		while (F)
		{
			if (count-- == 0) break; // select this one
			F = GetSubjectNondeadNext(F);
		}
		if (!F) return FAILRULE_BIT;
		sprintf(buffer, "%u", Fact2Index(F));
		return NOPROBLEM_BIT;
	}
	else return FAILRULE_BIT;
}

static FunctionResult NthCode(char* buffer)
{
	char* arg = ARGUMENT(1);
	char arg1[MAX_WORD_SIZE];
	arg = GetPossibleFunctionArgument(arg, arg1); // pass thru or convert
	FunctionResult result;
	GetCommandArg(arg, buffer, result, OUTPUT_NOCOMMANUMBER | ASSIGNMENT);
	if (result != NOPROBLEM_BIT) return result;

	if (*arg1 == USERVAR_PREFIX) strcpy(arg1, GetUserVariable(arg1, false));
	else if (*arg1 == '_') strcpy(arg1, GetwildcardText(GetWildcardID(arg1), true));

	int count = 0;
	int n = atoi(buffer); // which one

	if (*arg1 == '~') // nth member of set, counting from 0
	{
		WORDP D = FindWord(arg1);
		if (!multidict) D = NthRecentMember(D, n);
		else D = NthEarliestMember(D, n);
		if (!D) return FAILRULE_BIT; // not found
		strcpy(buffer, D->word);
		return NOPROBLEM_BIT;
	}
	else if (IsValidJSONName(arg1))
	{
		WORDP D = FindWord(arg1);
		FACT* F = GetSubjectNondeadHead(D);
		while (F) // back to front order, need to invert, count how many
		{
			++count;
			F = GetSubjectNondeadNext(F);
		}
		if (count <= n) return FAILRULE_BIT; // not enough or bad count
		F = GetSubjectNondeadHead(D);
		while (F && ++n < count) F = GetSubjectNondeadNext(F); // back to front order, need to invert
		if (F) sprintf(buffer, "%u", Fact2Index(F));
		return NOPROBLEM_BIT;
	}

	CALLFRAME* frame = GetCallFrame(globalDepth);
	frame->arguments[1] = AllocateStack(arg1); // put it back in case it changed
	char arg2[MAX_WORD_SIZE];
	strcpy(arg2, buffer); // should not be big
	if (*arg1 == '@') return FLR(buffer, arg2);
	else return FAILRULE_BIT;
}

static FunctionResult ResetCode(char* buffer)
{
	char* word = ARGUMENT(1);
	if (!stricmp(word, (char*)"USER"))
	{
		if (planning) return FAILRULE_BIT;
		int depth = globalDepth; // reset clears depth, but we are still in process so need to restore it
		ResetUser(buffer);
		globalDepth = depth;
		*buffer = 0;
		unsigned int index = bufferIndex;
		char* s = stackFree;
		AddInput(buffer, 0, true);
		ProcessInput();
		if (s != stackFree)
			stackFree = s;
		if (index != bufferIndex)
		{
			bufferIndex = index;
		}

#ifndef DISCARDTESTING
		wasCommand = COMMANDED;	// lie so system will save revised user file
#endif
		return ENDINPUT_BIT;
	}
	else if (!stricmp(word, (char*)"HISTORY")) //    
	{
		chatbotSaidIndex = humanSaidIndex = 0;
		*humanSaid[humanSaidIndex] = 0;
		*chatbotSaid[chatbotSaidIndex] = 0;
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(word, (char*)"FACTS")) // delete permanent user facts
	{
		FACT* F = lastFactUsed + 1;
		while (--F > factLocked)
		{
			if (!(F->flags & FACTTRANSIENT)) F->flags |= FACTDEAD;
		}
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(word, (char*)"VARIABLES"))
	{
		HEAPREF varthread = userVariableThreadList;
		while (varthread)
		{
			uint64 entry;
			varthread = UnpackHeapval(varthread, entry, discard, discard);
			WORDP D = (WORDP)entry;
			if (D->word[1] != TRANSIENTVAR_PREFIX && D->word[1] != LOCALVAR_PREFIX && D->w.userValue) // transients not dumped, nor are NULL values
			{
				D->w.userValue = NULL;
			}
		}
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(word, (char*)"TOPIC"))
	{
		word = ARGUMENT(2);
		int topicid;
		if (*word == '*' && word[1] == 0) // all topics
		{
			if (!all) ResetTopics();
			ResetContext(); // all context history
			pendingTopicIndex = 0;
		}
		else if ((topicid = FindTopicIDByName(word))) ResetTopic(topicid);
		else return FAILRULE_BIT;
		return NOPROBLEM_BIT;
	}
	else if (!stricmp(word, (char*)"OUTPUT"))
	{
		responseIndex = 0;
		return NOPROBLEM_BIT;
	}
	else if (*word == '@') // reset a fact set for browsing sequentially
	{
		int store = GetSetID(word);
		if (store == ILLEGAL_FACTSET) return FAILRULE_BIT;
		factSetNext[store] = 0;
		if (trace & TRACE_INFER) Log(USERLOG, " @%d[%d] ", store, FACTSET_COUNT(store));
		return NOPROBLEM_BIT;
	}
	return FAILRULE_BIT;
}

//////////////////////////////////////////////////////////
/// EXTERNAL ACCESS
//////////////////////////////////////////////////////////

static FunctionResult ExportCode(char* buffer)
{
	char* set = ARGUMENT(2);
	char* append = ARGUMENT(3); // optional 3rd argument is append or overwrite
	if (*set == '@') return (ExportFacts(ARGUMENT(1), GetSetID(set), append)) ? NOPROBLEM_BIT : FAILRULE_BIT;
	else if (IsValidJSONName(set) || !*set) return ExportJson(ARGUMENT(1), set, append);
	else return FAILRULE_BIT;
}

static FunctionResult ImportCode(char* buffer)
{
	bool oldload = loading;
	loading = true;
	bool ans = ImportFacts(buffer, ARGUMENT(1), ARGUMENT(2), ARGUMENT(3), ARGUMENT(4));
	loading = oldload;
	return ans ? NOPROBLEM_BIT : FAILRULE_BIT;
}

static FunctionResult GetRemoteFileCode(char* buffer)
{
	FunctionResult result = FAILRULE_BIT;
	char name[MAX_WORD_SIZE];
	strcpy(name, ARGUMENT(1));
#ifndef DISCARDMONGO
	char* arg2 = ARGUMENT(2);	// optional specifier - mongo x y z
	if (!stricmp(arg2, "mongo")) // transfer mongo to local
	{
		// get the mongo data.
		if (filesystemOverride != MONGOFILES) // currently not connected
		{
			ARGUMENT(1) = ARGUMENT(3); // set up server init arguments  URL
			ARGUMENT(2) = ARGUMENT(4); // DB
			ARGUMENT(3) = ARGUMENT(5); // Collection
			ARGUMENT(4) = ARGUMENT(6); // what kind of data  topic, ltm, log
			result = MongoInit(NULL); // init filesys collection
			if (result != NOPROBLEM_BIT) return result;
			ARGUMENT(2) = ARGUMENT(4);	// for mongoGetDoc
		} // Otherwise we are using mongo filesystem values
		else ARGUMENT(2) = ARGUMENT(3);
		char* buffer = GetFileBuffer();

		char* dot = strchr(name, '.');
		if (dot) *dot = 0;	 // terminate any suffix, not legal in mongo key
		result = mongoGetDocument(name, buffer, (userCacheSize - MAX_USERNAME), false); // can take argument(2)
		if (dot) *dot = '.';

		if (result != NOPROBLEM_BIT)
		{
			FreeFileBuffer(); // WE CAN still use the data buffer
			return result;
		}
		if (filesystemOverride != MONGOFILES) MongoClose(NULL); // close filesys collection

		// write to local filesystem
		FILE* out = FopenUTF8Write(name);
		if (out)
		{
			fwrite(buffer, 1, strlen(buffer), out); // write out the actual data.
			FClose(out);
		}
		FreeFileBuffer();
		return (out) ? NOPROBLEM_BIT : FAILRULE_BIT;
	}
	else
#endif
		return result;
}

static FunctionResult PopenCode(char* buffer)
{
	char   psBuffer[MAX_WORD_SIZE];
	FILE* pPipe;
	char* arg;
	arg = ARGUMENT(1);

	// convert \" to " within params
	if (*arg == '"') ++arg;
	size_t len = strlen(arg);
	if (arg[len - 1] == '"') arg[len - 1] = 0;
	char* fix = arg;
	while (*fix)
	{
		if (*fix == '\\' && *(fix + 1) == '\"') memmove(fix, fix + 1, strlen(fix) + 1); // remove protective backslash
		fix++;
	}

	// adjust function reference name
	char function[MAX_WORD_SIZE];
	FunctionResult fnresult;
	char* ptr = ReadFunctionCommandArg(ARGUMENT(2), function, fnresult, true);
	if (fnresult != NOPROBLEM_BIT) return fnresult;

#ifdef WIN32
	if ((pPipe = _popen(arg, (char*)"rb")) == NULL) return FAILRULE_BIT; //  "dir *.c /on /p", "rt" 
#else
	if ((pPipe = popen(arg, (char*)"r")) == NULL) return FAILRULE_BIT;
#endif
	psBuffer[0] = '(';
	psBuffer[1] = ' ';
	psBuffer[2] = '"'; // stripable string marker
	psBuffer[3] = ENDUNIT; // stripable string marker
	while (!feof(pPipe))
	{
		psBuffer[4] = 0;
		if (Myfgets(psBuffer + 4, MAX_WORD_SIZE - 5, pPipe) != NULL)
		{
			FunctionResult result = NOPROBLEM_BIT;
			RemoveImpure(psBuffer);
			strcat(psBuffer, (char*)"`\" )"); // trailing quote and ending paren
			if (*function == '^') DoFunction(function, psBuffer, buffer, result);
			buffer += strlen(buffer);
			if (result == UNDEFINED_FUNCTION) result = FAILRULE_BIT;
		}
	}
#ifdef WIN32
	_pclose(pPipe);
#else
	pclose(pPipe);
#endif
	return NOPROBLEM_BIT;
}

static FunctionResult TCPOpenCode(char* buffer)
{
#ifdef INFORMATION
	// POST http://de.sempar.ims.uni-stuttgart.de/parse HTTP/1.1
	// Accept: text/html, application/xhtml+xml, */*
	// Host: de.sempar.ims.uni-stuttgart.de
	// Content-Type: application/x-www-form-urlencoded
	// Content-Length: 31
	//
	// sentence=ich+bin&returnType=rdf

	// e.g.  TCPOpen(POST "http://de.sempar.ims.uni-stuttgart.de/parse" "sentence=ich+bin&returnType=rdf" 'myfunc)
#endif

#ifdef DISCARDTCPOPEN
	char* msg = "tcpopen not available\r\n";
	SetUserVariable((char*)"$$tcpopen_error", msg);	// pass along the error
	Log(USERLOG, msg);
	return FAILRULE_BIT;
#else
	size_t len;
	char* url;
	char directory[MAX_WORD_SIZE];
	char* arg;
	char kind = 0;
	FunctionResult result;
	bool encoded = false;
	if (!stricmp(ARGUMENT(1), (char*)"POST")) kind = 'P';
	else if (!stricmp(ARGUMENT(1), (char*)"GET")) kind = 'G';
	else if (!stricmp(ARGUMENT(1), (char*)"POSTU"))
	{
		kind = 'P';
		encoded = true;
	}
	else if (!stricmp(ARGUMENT(1), (char*)"GETU"))
	{
		kind = 'G';
		encoded = true;
	}
	else
	{
		char* msg = "tcpopen- only POST and GET allowed\r\n";
		SetUserVariable((char*)"$$tcpopen_error", msg);	// pass along the error
		Log(USERLOG, msg);
		return FAILRULE_BIT;
	}

	url = ARGUMENT(2);
	char* dot = strchr(url, '.');
	if (!dot)
	{
		char* msg = "tcpopen- an url was not given\r\n";
		SetUserVariable((char*)"$$tcpopen_error", msg);	// pass along the error
		Log(USERLOG, msg);
		return FAILRULE_BIT;
	}
	char* slash = strchr(dot, '/');
	if (slash)
	{
		*slash = 0; // leave url as is
		strcpy(directory, slash + 1);
	}
	else *directory = 0;

	// convert \" to " within params abd remove any wrapper
	arg = ARGUMENT(3);
	if (*arg == '"') ++arg;
	len = strlen(arg);
	if (arg[len - 1] == '"') arg[len - 1] = 0;
	char* fix;
	while ((fix = strchr(arg, '\\'))) memmove(fix, fix + 1, strlen(fix)); // remove protective backslash

	char originalArg[MAX_WORD_SIZE];
	strcpy(originalArg, arg);

	// url encoding:
	char* ptr = arg - 1;
	if (!encoded) while (*++ptr)
	{
		if (!IsAlphaUTF8(*ptr) && isAlphabeticDigitData[*ptr] != VALIDDIGIT && *ptr != '-' && *ptr != '_' && *ptr != '.' && *ptr != '~' && *ptr != '=' && *ptr != '&')
		{
			if (*ptr == ' ')
			{
				*ptr = '+';
				continue;
			}
			memmove(ptr + 3, ptr + 1, strlen(ptr)); // reserve 2 extra chars
			ptr[1] = toHex[(*ptr >> 4) & 0x0f];
			ptr[2] = toHex[(*ptr & 0x0f)];
			*ptr = '%';
			ptr += 2;
		}
	}

	// adjust function reference name
	char function[MAX_WORD_SIZE];
	FunctionResult fnresult;
	ptr = ReadFunctionCommandArg(ARGUMENT(4), function, fnresult, true);
	if (fnresult != NOPROBLEM_BIT) return fnresult;

	unsigned int portid = 0;
	if (kind == 'P' || kind == 'G') portid = 80;
	else
	{
		char* colon = strchr(url, ':');
		if (colon)
		{
			*colon = 0;
			portid = atoi(colon + 1);
		}
	}
	uint64 start_time = ElapsedMilliseconds();
	int size = 0;
	char* tcpbuffer = AllocateStack(NULL, maxBufferSize);
	char* startContent = tcpbuffer;
	try
	{
		if (trace & TRACE_TCP && CheckTopicTrace()) Log(USERLOG, "RAW TCP: %s/%s port:%d %s %s", url, directory, portid, (kind == 'G') ? (char*)"GET" : (char*)"POST", originalArg);
		TCPSocket* sock = new TCPSocket(url, (unsigned short)portid);

		if (kind == 'P')
		{
			if (*directory) sprintf(tcpbuffer, (char*)"POST /%s HTTP/1.0\r\nHost: %s\r\n", directory, url);
			else sprintf(tcpbuffer, (char*)"POST HTTP/1.0\r\nHost: %s\r\n", url);
		}
		else if (kind == 'G') sprintf(tcpbuffer, (char*)"GET /%s?%s HTTP/1.0\r\nHost: %s\r\n", directory, arg, url);
		sock->send(tcpbuffer, strlen(tcpbuffer));
		if (trace & TRACE_TCP && CheckTopicTrace()) Log(USERLOG, "\r\n%s", tcpbuffer);

		if (kind == 'P')
		{
			strcpy(tcpbuffer, (char*)"Content-Type: application/x-www-form-urlencoded\r\nAccept: text/html, application/xhtml+xml, */*\r\nAccept-Charset: utf-8\r\nUser-Agent: Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.2; WOW64; Trident/6.0)\r\n");
			sock->send(tcpbuffer, strlen(tcpbuffer));
			if (trace & TRACE_TCP && CheckTopicTrace()) Log(USERLOG, "%s", tcpbuffer);
			len = strlen(arg);
			sprintf(tcpbuffer, (char*)"Content-Length: %u\r\n\r\n%s\r\n", (unsigned int)len, arg);
		}
		else strcpy(tcpbuffer, (char*)"Content-Type: application/x-www-form-urlencoded\r\nAccept: text/html, application/xhtml+xml, */*\r\nAccept-Charset: utf-8\r\nUser-Agent: Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.2; WOW64; Trident/6.0)\r\n\r\n"); // GET
		sock->send(tcpbuffer, strlen(tcpbuffer));
		if (trace & TRACE_TCP && CheckTopicTrace()) Log(USERLOG, "%s", tcpbuffer);

		unsigned int bytesReceived = 1;              // Bytes read on each recv()
		unsigned int totalBytesReceived = 0;         // Total bytes read
		char* base = tcpbuffer;
		*base = 0;
		bool hasContent = false;
		int allowedBytes = maxBufferSize - 10;
		while (bytesReceived > 0)
		{
			// Receive up to the buffer size bytes from the sender
			bytesReceived = sock->recv(base, allowedBytes);
			allowedBytes -= bytesReceived;
			totalBytesReceived += bytesReceived;
			base += bytesReceived;
			if (!hasContent && (kind == 'P' || kind == 'G')) // std POST/GET http formats
			{
				startContent = strstr(tcpbuffer, (char*)"\r\n\r\n"); // body separator
				if (!startContent) continue; // not found yet
				startContent += 4;

				char* lenheader = strstr(tcpbuffer, (char*)"Content-Length: "); // look for earlier size info
				if (lenheader)
				{
					size = atoi(SkipWhitespace(lenheader + 16)); // size of body
					hasContent = true;
				}
			}
			if (hasContent && (base - startContent) >= size) break;	// we have enough
		}
		delete(sock);
		*base++ = 0;
		*base++ = 0;
		// chatbot replies this
		if (trace & TRACE_TCP && CheckTopicTrace()) Log(USERLOG, "tcp received: %d bytes %s", totalBytesReceived, tcpbuffer);
		if (timing & TIME_TCP && CheckTopicTime())
		{
			int diff = (int)(ElapsedMilliseconds() - start_time);
			if (timing & TIME_ALWAYS || diff > 0) Log(STDTIMELOG, (char*)"TCP open time: %d ms for %s %s/%s\r\n", diff, (kind == 'G') ? (char*)"GET" : (char*)"POST", url, directory);
		}
	}
	catch (SocketException e) {
		char* msg = "tcpopen- failed to connect to server or died in transmission\r\n";
		SetUserVariable((char*)"$$tcpopen_error", msg);	// pass along the error
		Log(USERLOG, msg);
		Log(USERLOG, "failed to connect to server %s %d\r\n", url, portid);
		ReleaseStack(tcpbuffer); // short term
		return FAILRULE_BIT;
	}

	// process http return for validity
	if (kind == 'P' || kind == 'G')
	{
		if (strnicmp(tcpbuffer, (char*)"HTTP", 4))
		{
			char* msg = "tcpopen- no HTTP ack code\r\n";
			SetUserVariable((char*)"$$tcpopen_error", msg);
			Log(USERLOG, msg);
			ReleaseStack(tcpbuffer); // short term
			return FAILRULE_BIT;
		}
		char* space = strchr(tcpbuffer, ' ');
		space = SkipWhitespace(space);	// go to end of whitespace
		if (trace & TRACE_TCP && CheckTopicTrace()) Log(USERLOG, "response: %s", space);
		if (*space != '2')
		{
			char msg[MAX_WORD_SIZE];
			space[5] = 0;
			sprintf(msg, (char*)"tcpopen- HTTP ack code bad %s\r\n", space);
			SetUserVariable((char*)"$$tcpopen_error", msg);
			Log(USERLOG, msg);
			ReleaseStack(tcpbuffer); // short term
			return FAILRULE_BIT;	// failure code of some kind
		}
	}

	userRecordSourceBuffer = startContent;
	char* buf1 = AllocateStack(NULL, maxBufferSize);
	buf1[0] = '(';
	buf1[1] = ' ';
	buf1[2] = '"'; // strippable string marker
	buf1[3] = ENDUNIT; // strippable string marker
	result = NOPROBLEM_BIT;
	while (result == NOPROBLEM_BIT)
	{
		if (ReadALine(buf1 + 4, 0) < 0) break;
		if (!buf1[4]) continue;		// no content
		char* actual = TrimSpaces(buf1);
		strcat(actual, (char*)"`\" )"); // trailing quote and ending paren
		if (*function == '^') DoFunction(function, actual, buffer, result);
		buffer += strlen(buffer);
		if (result == UNDEFINED_FUNCTION)
		{
			char* msg = "tcpopen- no such function to call";
			SetUserVariable((char*)"$$tcpopen_error", msg);
			Log(USERLOG, msg);
			result = FAILRULE_BIT;
		}
		else if (result & FAILCODES)
		{
			char* msg = "tcpopen- function call failed";
			SetUserVariable((char*)"$$tcpopen_error", msg);
			Log(USERLOG, msg);
		}
	}
	userRecordSourceBuffer = NULL;
	ReleaseStack(tcpbuffer); // short term
	return result;
#endif
}

static FunctionResult SystemCode(char* buffer)
{
	char word[MAX_WORD_SIZE];
	*word = 0;
	char* stream = ARGUMENT(1);
	while (stream && *stream)
	{
		FunctionResult result;
		char name[MAX_WORD_SIZE];
		stream = ReadShortCommandArg(stream, name, result, OUTPUT_NOQUOTES); // name of file
		if (*name)
		{
			strcat(word, name);
			strcat(word, (char*)" ");
		}
	}
	if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, "system: %s ", word);
	sprintf(buffer, (char*)"%d", system(word));
	if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, " result: %s r\n", buffer);
	return  NOPROBLEM_BIT;
}

#ifndef DISCARDWEBSOCKET

#include "easywclient/easywsclient.cpp"

#ifdef WIN32
#include <afunix.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#endif

using easywsclient::WebSocket;
static WebSocket::pointer websocket = NULL; // NOT THREAD SAFE
static char* websocketCallbackBuffer = NULL;

static void WebsocketCallback(const std::string & message)
{
	strcpy(websocketCallbackBuffer, message.c_str());
}

static FunctionResult WebsocketInitCode(char* buffer)
{
	if (websocket) return FAILRULE_BIT;

#ifdef _WIN32
	INT rc;
	WSADATA wsaData;
	rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (rc) {
		printf("WSAStartup Failed.\n");
		return FAILRULE_BIT;
	}
#endif

	char* url = ARGUMENT(1);
	websocket = easywsclient::WebSocket::from_url(url);
	if (!websocket) return FAILRULE_BIT;
	return NOPROBLEM_BIT;
}

FunctionResult WebsocketCloseCode(char* buffer)
{
	if (!websocket) return FAILRULE_BIT;
	WebSocket::pointer websocketOld = websocket;
	websocket = NULL;
	websocketOld->close();
	delete websocketOld;
#ifdef _WIN32
	WSACleanup();
#endif
	return NOPROBLEM_BIT;
}

static FunctionResult  WebsocketSendCode(char* buffer)
{
	char* message = ARGUMENT(1);
	if (!websocket || !message || !*message) return FAILRULE_BIT;

	// remote closed us?
	if (websocket->getReadyState() == WebSocket::CLOSING || websocket->getReadyState() == WebSocket::CLOSED)
	{
		WebsocketCloseCode(buffer); // make sure we ourselves are closed
		return FAILRULE_BIT;
	}

	websocket->send(message);
	websocket->poll(0);
	websocketCallbackBuffer = buffer;
	websocket->dispatch(WebsocketCallback);
	return NOPROBLEM_BIT;
}

static FunctionResult WebsocketReceiveCode(char* buffer)
{
	if (!websocket) return FAILRULE_BIT;

	int timeout = atoi(ARGUMENT(1));
	// timeout -1 will block until received. expect that as default, 0 will not block
	uint64 starttime = ElapsedMilliseconds();
	int polllimit = 1;
	while (websocket)
	{
		// remote closed connection?
		if (websocket->getReadyState() == WebSocket::CLOSING || websocket->getReadyState() == WebSocket::CLOSED)
		{
			WebsocketCloseCode(buffer);
			break;
		}

		websocket->poll((polllimit > timeout) ? timeout : polllimit); // milliseconds
		websocketCallbackBuffer = buffer;
		websocket->dispatch(WebsocketCallback);
		if (*buffer) return NOPROBLEM_BIT; // message gotten

		// if you timeout, socket will still accept message on next poll maybe.
		if (!timeout || (ElapsedMilliseconds() - starttime) > timeout) break;
		polllimit <<= 1; // the longer it takes, the longer we will wait til timeout happens
	}
	return FAILRULE_BIT;
}

void WebSocketClient(char* url, char* startmessage)
{
	InternalCall("^WebsocketInit", WebsocketInitCode, url, "", "", ourMainInputBuffer);
	if (websocket)
	{
		printf("Websocket server %s started\r\n", url);
		if (*startmessage) InternalCall("^WebsocketSend", WebsocketSendCode, startmessage, "", "", ourMainInputBuffer);
	}
	else printf("Failed to connect to websocket %s\r\n", url);

	while (websocket)
	{
		InternalCall("^WebsocketReceive", WebsocketReceiveCode, "-1", "", "", ourMainInputBuffer);
		PerformChat("websocket", "websocketbot", ourMainInputBuffer, "0.0.0", ourMainOutputBuffer);
		InternalCall("^WebsocketSend", WebsocketSendCode, ourMainOutputBuffer, "", "", ourMainInputBuffer);
	}
}
#endif

//////////////////////////////////////////////////////////
/// FACTS
//////////////////////////////////////////////////////////

static FunctionResult CreateFactCode(char* buffer)
{
	currentFact = NULL;
	char* arg = SkipWhitespace(ARGUMENT(1));
	char word[MAX_WORD_SIZE];
	char* at = ReadCompiledWord(arg, word);
	char* argcopy;
	if (!*at) // single arg, eval it to get real one....
	{
		FunctionResult result;
		GetCommandArg(arg, buffer, result, OUTPUT_NOQUOTES);
		if (result != NOPROBLEM_BIT) return result;
		at = buffer;
		if (*at == '(') at += 2; // skip paren start as would be done by WriteFact
		argcopy = AllocateStack(at, 0);
	}
	else argcopy = AllocateStack(arg, 0);
	EatFact(argcopy, buffer); // PUTS NOTHING IN THE OUTPUT BUFFER but can be assigned from.
	ReleaseStack(argcopy);
	*buffer = 0;
	return (currentFact) ? NOPROBLEM_BIT : FAILRULE_BIT;
}

FACT* DeleteFromList(FACT * oldlist, FACT * oldfact, GetNextFact getnext, SetNextFact setnext)
{ // olditem can be either a meaning or a factoid. we are allowing for the oldlist fact that has olditem as subject
	FACT* start = oldlist;
	if (oldlist == oldfact) return (*getnext)(oldlist); // fact to delete is list head
	FACT* prior = oldlist;
	while (oldlist)
	{
		FACT* H = (*getnext)(oldlist);
		if (oldlist == oldfact)
		{
			(*setnext)(prior, H); // remove this from list by skip over
			break;
		}
		prior = oldlist;
		oldlist = H;
	}

	return start;
}

FACT* AddToList(FACT * list, FACT * F, GetNextFact getnext, SetNextFact setnext)
{// insert dictionary xref at right place in dictionary list
	FACT* start = list;
	if (start < F) // we will head the list, most recent fact first
	{
		(*setnext)(F, list); // but list store has not been updated yet
		return F;
	}
	FACT* prior = list;
	while (list)
	{
		if (list < F) break; // add fact into list by insert
		prior = list;
		list = (*getnext)(list);
	}
	(*setnext)(F, list);
	if (prior) (*setnext)(prior, F);
	return start;
}

FunctionResult ReviseFact1Code(char* buffer, bool arrayAllowed)
{
	currentFact = NULL;
	char* arg = ARGUMENT(1);
	FACTOID index = atoi(arg);
	FACT* F = Index2Fact(index);
	if (F <= factLocked || F->flags & FACTDEAD) return FAILRULE_BIT; // only user undead facts
	if (F <= factsPreBuild[LAYER_USER])  return FAILRULE_BIT; // may not revise facts built into world
	char* subject = ARGUMENT(2);
	char* verb = ARGUMENT(3);
	char* object = ARGUMENT(4);
	unsigned int flags = F->flags;

	if (stricmp(subject, (char*)"null"))
	{
		if (!arrayAllowed && flags & (JSON_ARRAY_FACT | JSON_OBJECT_FACT)) return FAILRULE_BIT;	// may not change subject of json.. linked possibly from above
		if (flags & FACTSUBJECT)
		{
			unsigned int newsubject = atoi(subject); // find the fact replacing it
			FACT* newfact = Index2Fact(index);
			if (!newfact) return FAILRULE_BIT;
			FACTOID oldsubject = (FACTOID)F->subject;
			FACT* oldfact = Index2Fact(oldsubject);
			if (index != oldsubject)
			{
				SetSubjectHead(oldfact, DeleteFromList(GetSubjectHead(oldfact), F, GetSubjectNext, SetSubjectNext));
				SetSubjectHead(newfact, AddToList(GetSubjectHead(newfact), F, GetSubjectNext, SetSubjectNext));
				F->subject = newsubject;
				ModBaseFact(F);
			}
		}
		else // word replacement
		{
			WORDP oldsubject = Meaning2Word(F->subject);
			WORDP newsubject = StoreWord(subject, AS_IS);
			if (oldsubject != newsubject)
			{
				UnweaveFactSubject(F);
				SetSubjectHead(newsubject, AddToList(GetSubjectHead(newsubject), F, GetSubjectNext, SetSubjectNext));  // dont use nondead
				F->subject = MakeMeaning(newsubject);
				ModBaseFact(F);
			}
		}
	}
	if (stricmp(verb, (char*)"null"))
	{
		if (!arrayAllowed && flags & JSON_ARRAY_FACT) return FAILRULE_BIT;	// may not change index of json array. breaks sequencing
		if (flags & FACTVERB)
		{
			unsigned int newverb = atoi(verb); // find the fact replacing it
			FACT* newfact = Index2Fact(index);
			if (!newfact) return FAILRULE_BIT;
			FACTOID oldverb = (FACTOID)F->verb;
			FACT* oldfact = Index2Fact(oldverb);
			if (index != oldverb)
			{
				FACT* X = DeleteFromList(GetVerbHead(oldfact), F, GetVerbNext, SetVerbNext);  // dont use nondead
				SetVerbHead(oldfact, X);
				SetVerbHead(newfact, AddToList(GetVerbHead(newfact), F, GetVerbNext, SetVerbNext));
				F->verb = newverb;
				ModBaseFact(F);
			}
		}
		else // word replacement
		{
			WORDP oldverb = Meaning2Word(F->verb);
			WORDP newverb = StoreWord(verb, AS_IS);
			if (oldverb != newverb)
			{
				UnweaveFactVerb(F);
				SetVerbHead(newverb, AddToList(GetVerbHead(newverb), F, GetVerbNext, SetVerbNext));
				F->verb = MakeMeaning(newverb);
				ModBaseFact(F);
			}
		}
	}
	if (stricmp(object, (char*)"null"))
	{
		if (flags & FACTOBJECT)
		{
			unsigned int newobject = atoi(object); // find the fact replacing it
			FACT* newfact = Index2Fact(index);
			if (!newfact) return FAILRULE_BIT;
			FACTOID oldobject = (FACTOID)F->object;
			FACT* oldfact = Index2Fact(oldobject);
			if (index != oldobject)
			{
				FACT* X = DeleteFromList(GetObjectHead(oldfact), F, GetObjectNext, SetObjectNext);  // dont use nondead
				SetObjectHead(oldfact, X);
				SetObjectHead(newfact, AddToList(GetObjectHead(newfact), F, GetObjectNext, SetObjectNext));
				F->object = newobject;
				ModBaseFact(F);
			}
		}
		else // word replacement
		{
			WORDP oldObject = Meaning2Word(F->object);
			WORDP newObject = StoreWord(object, AS_IS);
			MEANING value = MakeMeaning(newObject);
			if (flags & JSON_FLAGS)
			{
				flags &= -1 ^ (JSON_PRIMITIVE_VALUE | JSON_STRING_VALUE | JSON_OBJECT_VALUE | JSON_ARRAY_VALUE);
				value = jsonValue(object, flags);
				newObject = Meaning2Word(value);
				F->flags = flags;	// revised for possible new object type
			}

			if (oldObject != newObject)
			{
				UnweaveFactObject(F);
				SetObjectHead(newObject, AddToList(GetObjectHead(newObject), F, GetObjectNext, SetObjectNext));
				F->object = value;
				ModBaseFact(F);
			}
		}
	}

	if (trace & (TRACE_INFER | TRACE_FACT)) TraceFact(F, false);
#ifdef INFORMATION
	As we create facts, older facts(lower index) will be farther down the list.When we erase a fact, we should be at the top of all xref lists.
#endif
		return NOPROBLEM_BIT;
}

FunctionResult ReviseFactCode(char* buffer)
{
	return ReviseFact1Code(buffer); // using default 2nd arg false
}

static void GenerateConceptList(bool tracing, HEAPREF list, int set, char* filter, size_t len,unsigned  int wordindex)
{ // the use of concepts and topics lists is this- script call
	uint64 val;
	MARKDATA hitdata;
	while (list)
	{
		list = UnpackHeapval(list, val, discard, discard);
		WORDP X = (WORDP)val;
		if (X && !(X->systemFlags & NOCONCEPTLIST) && (!len || !strnicmp(X->word, filter, len)))
		{
			// found at this word index, get its correct range
			if (!GetNextSpot(X, wordindex - 1,  false,0,&hitdata)) continue;
			if (wordindex != hitdata.start) continue;	// didnt find it
			int position = (hitdata.start << 8) | hitdata.end;
			char word[MAX_WORD_SIZE];
			sprintf(word, "%d", position);
			FACT* F = CreateFact(MakeMeaning(X), Mconceptlist, MakeMeaning(StoreWord(word)), FACTTRANSIENT);
			int n = FACTSET_COUNT(set);
			int i;
			for (i = n; i > 0; --i)
			{
				if (factSet[set][i] == F) break; // repeated
			}
			if (!i)
			{
				if (tracing) TraceFact(F);
				AddFact(set, F);
			}
		}
	}
}

static FunctionResult ConceptListCode(char* buffer)
{
	int set = (impliedSet == ALREADY_HANDLED) ? 0 : impliedSet;
	SET_FACTSET_COUNT(set, 0);
	unsigned int how = 0;
	char* arg = ARGUMENT(1);
	char word[MAX_WORD_SIZE];
	arg = ReadCompiledWord(arg, word);
	if (!stricmp(word, (char*)"CONCEPT"))
	{
		arg = GetPossibleFunctionArgument(arg, word);
		how = 1;
	}
	else if (!stricmp(word, (char*)"TOPIC"))
	{
		arg = GetPossibleFunctionArgument(arg, word);
		how = 2;
	}
	else if (!stricmp(word, (char*)"BOTH"))
	{
		arg = GetPossibleFunctionArgument(arg, word);
		how = 3;
	}
	else return FAILRULE_BIT;

	FunctionResult result;
	GetCommandArg(arg, buffer, result, OUTPUT_NOCOMMANUMBER | ASSIGNMENT); // possible filter

	unsigned int start = 1;
	unsigned int end = 1;
	if (*word == '\'') memmove(word, word + 1, strlen(word));

	if (*word == '_')
	{
		start = WildStartPosition(word);  //  wildcard position designator
		end = WildEndPosition(word);
	}
	else if (*word == USERVAR_PREFIX) start = end = atoi(GetUserVariable(word, false));  //  user var
	else if (IsDigit(*word)) start = end = atoi(word);
	else if (!*word || !stricmp(word, "null")) end = wordCount; // overall
	else return FAILRULE_BIT;

	if (start < 1 || start > (unsigned int)wordCount) return FAILRULE_BIT;

	bool tracing = false;
	if (trace & TRACE_FLOW)
	{
		WORDP D = FindWord("^conceptList");
		if (D->internalBits & MACRO_TRACE || trace & TRACE_FACT) tracing = true;
	}

	size_t len = strlen(buffer);
	for (unsigned int i = start; i <= end; ++i)
	{
		if (tracing) Log(USERLOG, "@%u %s ", i, wordStarts[i]);
		if (how & 1) GenerateConceptList(tracing, concepts[i], set, buffer, len, i);
		if (how & 2) GenerateConceptList(tracing, topics[i], set, buffer, len, i);
	}
	if (impliedSet == ALREADY_HANDLED && FACTSET_COUNT(set) == 0) return FAILRULE_BIT;
	impliedSet = ALREADY_HANDLED;
	currentFact = NULL;
	*buffer = 0;
	return NOPROBLEM_BIT;
}

static FunctionResult CreateAttributeCode(char* buffer)
{
	currentFact = NULL;
	EatFact(ARGUMENT(1), buffer, 0, true);
	*buffer = 0;
	if (currentFact && !(currentFact->flags & FACTATTRIBUTE)) return FAILINPUT_BIT;	// kill the whole line.
	return (currentFact) ? NOPROBLEM_BIT : FAILRULE_BIT; // fails if pre-existing fact cant be killed because used in fact
}

HEAPREF MakeFunctionDefinition(char* data)
{
	char name[MAX_WORD_SIZE];
	// "name t %d %d%s\r\n"  name  type  mybot  macroflags  argumentcount (args )  code - may have optional display
	data = ReadCompiledWord(data, name);
	WORDP D = StoreWord(name, AS_IS);
	// refuse to redefine system functions
	if (!D->x.codeIndex)
	{
		unsigned int bits = FUNCTION_NAME | IS_OUTPUT_MACRO;
		if (data[0] == 'O') bits |= VARIABLE_ARGS_TABLE;
		// old copy of fn name
		variableChangedThreadlist = AllocateHeapval(HV1_WORDP|HV2_STRING|HV3_INT,variableChangedThreadlist,
			(uint64)D, (uint64)D->w.userValue, (uint64)D->internalBits);// save name
		AddInternalFlag(D, bits);
		char* newdata = AllocateHeap(NULL, strlen(data) + 5); // our own copy distinct from caller with link 0
		strcpy(newdata + 4, data + 2);
		newdata[0] = newdata[1] = newdata[2] = newdata[3] = 0; // no link value
		D->w.fndefinition = (unsigned char*)newdata;
		if (trace & (TRACE_USERFN | TRACE_PATTERN))
		{
			Log(USERLOG, "Defining %s as %s\r\n", D->word, newdata + 4);
		}
	}
	return variableChangedThreadlist;
}

static HEAPREF MakeVariables(MEANING array, bool needNL) // array of objects describing a variable
{
	if (!array) return NULL;

	char buffer[MAX_WORD_SIZE];
	WORDP V = FindWord(Meaning2Word(array)->word); // array of objects (variable name and value)
	FACT* F = GetSubjectHead(V);
	char* holderjson = NULL;
	char* holderoriginal = NULL;
	while (F) // save old values and assign new ones to variables
	{
		FACT* G = GetSubjectNext(F);
		WORDP D = Meaning2Word(F->verb);
		char* varname = D->word;

		char* text = Meaning2Word(F->object)->word;
		if (F->flags & JSON_PRIMITIVE_VALUE && !stricmp(text, "null")) text = "";
		char* base = SkipWhitespace(text);
		if (*base == '{' || *base == '[') // possible json embedded in string
		{
			size_t len = strlen(base);
			while (len > 0 && IsWhiteSpace(base[len - 1])) --len;
			bool matched = false;
			if (len && base[len - 1] == '}' && *base == '{') matched = true; // found end of json
			else if (len && base[len - 1] == ']' && *base == '[') matched = true; // found end of json
			if (matched)
			{
				// treat $holder and $data as the same structure when jabugpatch is on and both have same incoming
				if (jabugpatch && holderoriginal == base && !stricmp(varname, "$data") )
					text = holderjson;
				else
				{
					ParseJson(buffer, base, len, true, NULL);
					text = buffer; // name of structure returned
					if (jabugpatch && !stricmp(varname, "$holder")) // comes 1st from dse
					{
						holderoriginal = base;
						holderjson = text;
					}
				}
			}
		}
		if (trace & TRACE_VARIABLE)
		{
			if (text && !*text) Log(USERLOG, "Incoming variable %s = `empty string` \r\n", varname);
			else Log(USERLOG, "Incoming variable %s = %s \r\n", varname, text);
		}
		if (*varname == '$')
		{
			char name[MAX_WORD_SIZE];
			MakeLowerCopy(name, varname); // variables must be lower case
			WORDP D = StoreWord(name, AS_IS);
			SetUserVariable(D->word, text); // require it be variable or ignore it
			if (name[1] == '^') MakeFunctionDefinition(text);
			if (!stricmp(name, "$language")) strcpy(current_language, text); // change language (eg japanese)
			else if (!stricmp(name, "$cs_language")) strcpy(current_language, text); // change language (eg japanese)
			else SetUserVariable(name, text); // require it be variable or ignore it
		}
		F = GetSubjectNext(F);
	}
	return variableChangedThreadlist;
}

static FunctionResult MakeConceptPattern(char* pattern, MEANING concept, MEANING conceptPattern)
{
	FunctionResult result = NOPROBLEM_BIT;
	char* data = AllocateBuffer();
	char* startData = data;
	*data++ = '^'; // mark as compiled
	*data = 0;
	SAVESYSTEMSTATE()
	char* buffer = AllocateBuffer();

	// try to recover from fatality
	int buffercount = bufferIndex;
	int frameindex = globalDepth;
	if (setjmp(scriptJump[++jumpIndex])) // return on script compiler error
	{
		globalDepth = frameindex;
		bufferIndex = buffercount;
		--jumpIndex;
		*buffer = 0;
		result = FAILRULE_BIT;
	}
	else
	{
		patternwordthread = NULL;
		StartScriptCompiler(false);
		ReadNextSystemToken(NULL, NULL, data, false, false); // flush cache
		*data = 0;
		strcpy(readBuffer, pattern);
		compiling = CONCEPTSTRING_COMPILE;
		currentFileLine = 0;
		supplementalColumn = currentLineColumn = 0;
#ifndef DISCARDSCRIPTCOMPILER
		strcpy(currentFilename, "ConceptPattern");
		ReadPattern(readBuffer, NULL, data, false, false); // swallows the pattern
#endif
		--jumpIndex;
	}
	compiling = NOT_COMPILING;

	EndScriptCompiler();

	WORDP D = StoreWord(startData, AS_IS);
	CreateFact(MakeMeaning(D), conceptPattern, concept, FACTTRANSIENT);
	RESTORESYSTEMSTATE()
		FreeBuffer();
	return result;
}

static HEAPREF AddPOSConcept(char* posname, FACT * members, HEAPREF & protectPOSList)
{
	while (members) //  xxx member ~concept
	{
		MEANING m = members->object;
		WORDP MEMBER = Meaning2Word(m);
		uint64 bit = 0;
		if (!stricmp(posname, "~noun"))
		{
			char* split = strchr(MEMBER->word, '|');
			if (split) *split = 0;
			WORDP D = StoreWord(MEMBER->word, AS_IS);
			if (!(D->properties & NOUN)) bit |= NOUN;
			if (!(D->properties & NOUN_SINGULAR)) bit |= NOUN_SINGULAR;
			if (bit)
			{
				AddProperty(D, bit);
				protectPOSList = AllocateHeapval(HV1_WORDP|HV2_INT,protectPOSList, (uint64)D, bit);
			}
			if (split)
			{
				D = StoreWord(split + 1, AS_IS);
				if (!(D->properties & NOUN)) bit |= NOUN;
				if (!(D->properties & NOUN_PLURAL)) bit |= NOUN_SINGULAR;
				if (bit)
				{
					AddProperty(D, bit);
					protectPOSList = AllocateHeapval(HV1_WORDP|HV2_INT,protectPOSList, (uint64)D, bit);
				}
				*split = '|';
			}
		}
		else
		{
			if (!stricmp(posname, "~verb")) bit |= VERB;
			else if (!stricmp(posname, "~adjective")) bit |= ADJECTIVE;
			else if (!stricmp(posname, "~adverb")) bit |= ADVERB;
			char* word = MEMBER->word;
			while (1)
			{
				char* split = strchr(word + 1, '|');
				if (split) *split = 0;
				WORDP D = StoreWord(word, AS_IS);
				if (!(D->properties & bit))
				{
					AddProperty(D, bit);
					protectPOSList = AllocateHeapval(HV1_WORDP|HV2_INT,protectPOSList, (uint64)D, bit);
				}
				if (split)
				{
					*split = '|';
					word = split + 1;
				}
				else break; // finished all forms
			}
		}
		members = GetSubjectNondeadNext(members);
	}
	return protectPOSList;
}

static FunctionResult MakeConcepts(bool needNL, MEANING array, HEAPREF & subs, HEAPREF & protectPOSList, bool markConcepts)
{
	/*
	"concepts": [  <<- array is this.
	{    "name": "~stove",
	"values" : ["oven", "range", "cooktop"]      },
	{ "name": "~leaky",
	"values" : ["'leak",          "drip",          "spill"] }],
	*/
	FACT* patterns[MAX_FIND + 1];
	int numPatterns = 0;
	FACT* G = GetSubjectHead(array); // list of json objects representing concepts
	while (G) // walk list of concepts and bind new memberships
	{
		WORDP conceptName = NULL;
		FACT* members = NULL;
		FACT* conceptObject = GetObjectHead(G->object);
		while (conceptObject) // walk list of concept objects
		{
			// 2 facts: name and values in any order
			FACT* F = GetSubjectHead(conceptObject->object); // 2 facts, name + array
			if (!stricmp(Meaning2Word(F->verb)->word, "values"))
			{
				members = GetSubjectHead(F->object);
				F = GetSubjectNext(F); // name fact
				if (stricmp(Meaning2Word(F->verb)->word, "name")) return FAILRULE_BIT;
				conceptName = Meaning2Word(F->object);
			}
			else if (!stricmp(Meaning2Word(F->verb)->word, "name"))
			{
				conceptName = Meaning2Word(F->object);
				F = GetSubjectNext(F); // array fact
				if (stricmp(Meaning2Word(F->verb)->word, "values")) return FAILRULE_BIT;
				members = GetSubjectHead(F->object); // joxxx values jaxxx
			}
			else return FAILRULE_BIT;

			char* name = conceptName->word;

			// augment pos attributes instead of create a concept
			if (!stricmp(name, "~noun") || !stricmp(name, "~verb") ||
				!stricmp(name, "~adjective") || !stricmp(name, "~adverb"))
			{
				if (needNL) protectPOSList = AddPOSConcept(name, members, protectPOSList);
				conceptObject = GetObjectNext(conceptObject);
				continue;
			}

			bool spellfix = !stricmp(name, "~replace_spelling"); // replace: equivalent
			conceptName->internalBits |= OVERRIDE_CONCEPT;
			MEANING concept = MakeMeaning(conceptName);
			MEANING conceptPattern = MakeMeaning(StoreWord("conceptPattern", AS_IS));
			while (members) //  xxx member ~concept
			{
				int flags = FACTTRANSIENT | OVERRIDE_MEMBER_FACT | FACTDUPLICATE;
				MEANING m = members->object;

				// member is word, phrase, or pattern
				FACT* pattern = NULL;
				WORDP P = NULL;
				WORDP MEMBER = Meaning2Word(m);
				char* item = MEMBER->word;
				char* safestart = SkipWhitespace(item);
				if (*safestart == '"') safestart = SkipWhitespace(item + 1);
				char* remap = strchr(item, '|');
				if (remap && !remap[1]) remap = 0; 
				if (remap && !spellfix) // word|word is remap ability
				{
					*remap = 0; // will also generate normal member
					WORDP D = StoreWord(item, AS_IS);
					WORDP E = StoreWord(remap + 1, AS_IS);
					CreateFastFact(MakeMeaning(D), Mremapfact, MakeMeaning(E), FACTTRANSIENT);
					m = MakeMeaning(D); // prep for normal member create of item on concept
				}
				if (spellfix) // pairs of words, inverse order
				{
					char* replace = Meaning2Word(m)->word;
					char* split = strchr(replace, '|');
					if (split) // x|y format
					{
						char original[100];
						strcpy(original, replace);
						original[split - replace] = 0;
						subs = SetSubstitute(original, split + 1, 0, DO_PRIVATE, subs);
					}
					else
					{
						members = GetSubjectNext(members);
						if (!members) return FAILRULE_BIT; // BAD formation
						MEANING original = members->object;
						subs = SetSubstitute(Meaning2Word(original)->word, replace, 0, DO_PRIVATE, subs);
					}
				}
				else if (*item == '"' && (*safestart == '(' || safestart[1] == '(')) // pattern in "(   )" w possible leading whitespace
				{
					if (*safestart == '^') ++safestart;
					size_t len = strlen(safestart);
					safestart[len - 1] = 0;
					if (MakeConceptPattern(safestart, concept, conceptPattern) == FAILRULE_BIT) return FAILRULE_BIT;
					safestart[len - 1] = '"';
					pattern = currentFact;
				}
				else if (*safestart == '(' && strchr(item, ')')) // pattern in  ()  -- skip over leading whitespace
				{
					if (MakeConceptPattern(safestart, concept, conceptPattern) == FAILRULE_BIT) return FAILRULE_BIT;
					pattern = currentFact;
				}
				else if (*item == '^' && item[1] == '(') // compiled pattern
				{
					CreateFact(m, conceptPattern, concept, FACTTRANSIENT);
				}
				else // simple member
				{
					if (*item == '\'')
					{
						flags |= ORIGINAL_ONLY;
						m = MakeMeaning(StoreWord(item +1, AS_IS));
					}
					CreateFastFact(m, Mmember, concept, flags);
				}

				if (markConcepts)
				{
					if (P)
					{
						int start = 0;
						MARKDATA hitdata;
						while (GetNextSpot(P, start,false,0,&hitdata))
						{
							MarkMeaningAndImplications(0, 0, concept, hitdata.start, hitdata.end, CANONICAL);
							start = hitdata.start;
						}
					}
					else if (pattern) patterns[numPatterns++] = pattern;
				}

				members = GetSubjectNext(members);
				currentFact = NULL;
			}
			currentFact = 0;
			conceptObject = GetObjectNext(conceptObject);
		}

		G = GetSubjectNext(G);
	}

	// execute any concept patterns
	for (int i = 0; i < numPatterns; ++i)
	{
		ExecuteConceptPatterns(patterns[i]);
	}

	return NOPROBLEM_BIT;
}

static void UnbindConcepts(MEANING array, FACT * newfact, FACT * oldfact, HEAPREF substitutes)
{
	UndoSubstitutes(substitutes);

	FACT* G = GetSubjectHead(array); // list of json objects representing concepts
	while (G) // walk list of concepts and disable overrides
	{
		WORDP conceptName = NULL;
		FACT* members = NULL;
		FACT* conceptObject = GetObjectHead(G->object);
		while (conceptObject) // walk list of concept objects
		{
			// 2 facts: name and values in any order
			FACT* F = GetSubjectHead(conceptObject->object); // 2 facts, name + array
			if (!stricmp(Meaning2Word(F->verb)->word, "values"))
			{
				F = GetSubjectNext(F); // name fact
				conceptName = Meaning2Word(F->object);
			}
			else if (!stricmp(Meaning2Word(F->verb)->word, "name"))
			{
				conceptName = Meaning2Word(F->object);
			}

			conceptName->internalBits &= -1 ^ OVERRIDE_CONCEPT; // safe if they pass the same concept twice
			conceptObject = GetObjectNondeadNext(conceptObject);
		}

		G = GetSubjectNext(G);
	}
	while (newfact > oldfact) KillFact(newfact--, false, false);

	while (heapPatternThread)
	{
		uint64 val1;
		uint64 val2;
		uint64 val3;
		heapPatternThread = UnpackHeapval(heapPatternThread, val1, val2, val3);
		WORDP D = (WORDP)val1;
		RemoveSystemFlag(D, PATTERN_WORD);
	}
}

static void UndoAPICall(HEAPREF changedIncomingVariables,MEANING conceptlist, FACT* newfact, FACT* oldfact, HEAPREF substitutes)
{
	if (conceptlist) UnbindConcepts(conceptlist, newfact, oldfact, substitutes); // kill these facts
	UnbindVariables(changedIncomingVariables);
}

static bool TestPatternAgainstSentence(int sentenceid, int index, char* label, char* pattern, char* buffer)
{
	if (!label) label = "";
	pattern = strchr(pattern, '('); // may have ^ or  []concepts in it or || safewords
	if (!pattern) return false; // no pattern found
	MARKDATA hitdata;

	char* base = pattern;
	int start = 0;
	int end = 0;
	bool retried = false;
	// printf("PATTERN: %s\r\n", HexDisplay(base));

RETRY:
	wildcardIndex = 0;  //   reset wildcard allocation on top-level pattern match
	int whenmatched = 0;
	char oldmark[MAX_SENTENCE_LENGTH];
	memcpy(oldmark, unmarked, MAX_SENTENCE_LENGTH);
	if (trace & TRACE_PATTERN)
	{
		char* buf = AllocateBuffer();
		char* startptr = buf;
		for (unsigned int i = 1; i <= wordCount; ++i)
		{
			sprintf(buf, "%s ", wordStarts[i]);
			buf += strlen(buf);
		}
		Log(USERLOG, "test pattern %d %s against sentence %d \"%s\"\n", index, (*label ? label : pattern), sentenceid, startptr);
		FreeBuffer();
	}
	bool match = false;

	if (*base == '(')
	{
		base += 2;		// skip opening paren of a pattern and space
		hitdata.start = start;
		match = Match(base, 0, hitdata, 1, 0, whenmatched, '(') != 0;  //   skip paren and treat as NOT at start depth, dont reset wildcards- if it does match, wildcards are bound
		start = hitdata.start;
		end = hitdata.end;
		if (trace & TRACE_PATTERN && !traceTestPatternBuffer)
		{
			if (match) Log(USERLOG, "pattern result %d match %s\r\n",  index, label );
			else Log(USERLOG, "pattern result %d fail %s\r\n", index, label);
		}
		if (patternRetry)
		{
			if (end > start) start = end;	// continue from last match location
			if (start > MAX_SENTENCE_LENGTH || start < 0)
			{
				start = 0; // never matched internal words - is at infinite start -- WHY allow this?
			}
			else
			{
				retried = true;
				goto RETRY;
			}
		}
	}
	else // ^if
	{
		FunctionResult result;
		Output(base, buffer, result, 0);
		match = *buffer == '1';
		if (trace & TRACE_PATTERN) Log(USERLOG, "ifmatch: %d from %s", whenmatched, base);
	}

	return match || retried;
}

static wordScore_t ComputeTFIDF(char* text, wordScore_t idfScores, int termCount = 0)
{
	wordScore_t tfidf;

	tfidf.clear();
	if (idfScores.empty()) return tfidf;

	unsigned int totalTerms = termCount;

	// compute TF/IDF for the text based on the words which have an actual score
	wordScore_t::iterator it;

	char word[MAX_WORD_SIZE];
	char wordRange[10];
	char* ptr = text;

	// first count the number of terms each term is in the string (TF)
	while (ALWAYS)
	{
		char* nextTokenStart = SkipWhitespace(ptr);
		ptr = ReadCompiledWord(nextTokenStart, word);
		if (!*word) break;

		nextTokenStart = SkipWhitespace(ptr);
		ptr = ReadCompiledWord(nextTokenStart, wordRange);
		int numWords = atoi(wordRange);

		WORDP D = FindWord(word);
		if (D)
		{
			it = idfScores.find(D);
			if (it == idfScores.end()) continue;

			it = tfidf.find(D);
			if (it == tfidf.end())
			{
				tfidf[D] = (!numWords ? 1 : numWords);
				if (!termCount) totalTerms++;
			}
			else tfidf[D] += numWords;
		}
	}
	if (totalTerms == 0) return tfidf;

	// then compute the tf/idf value
	for (it = tfidf.begin(); it != tfidf.end(); ++it)
	{
		WORDP D = it->first;
		double tf = it->second / totalTerms;
		tfidf[D] = tf * idfScores[D];
	}

	return tfidf;
}

static double VectorNormal(wordScore_t vector)
{
	double normal = 0.0;

	wordScore_t::iterator it;
	for (it = vector.begin(); it != vector.end(); ++it)
	{
		double v = it->second;
		normal += (v * v);
	}
	normal = sqrt(normal);

	return normal;
}

static double CosineSimilarity(wordScore_t inputTFIDF, wordScore_t matchedTFIDF, double inputNorm, double matchedNorm)
{
	double similarity = 0.0;
	double crossProduct = 0.0;

	// compute the cross product of the matched words with the input
	wordScore_t::iterator it;
	for (it = matchedTFIDF.begin(); it != matchedTFIDF.end(); ++it)
	{
		WORDP D = it->first;
		double v = it->second;

		wordScore_t::iterator itp;
		itp = inputTFIDF.find(D);
		if (itp != inputTFIDF.end())
		{
			crossProduct += (v * (itp->second));
		}
	}

	// final cosine simularity calculation
	// https://en.wikipedia.org/wiki/Vector_space_model
	//     sim(d,q) = d . q / ||d||.||q||
	if (crossProduct > 0)
	{
		double normProduct = inputNorm * matchedNorm;
		if (normProduct > 0)
		{
			similarity = crossProduct / normProduct;
		}
	}

	return similarity;
}

static double PatternMatchSimilarity(wordScore_t inputTFIDF, double inputNorm, char* matchedWords, wordScore_t idfScores)
{
	double similarity = 0.0;
	if (strlen(matchedWords) == 0) return similarity;

	// compute tf-idf and norm for the matched words
	wordScore_t matchedTFIDF = ComputeTFIDF(matchedWords, idfScores);
	double matchedNorm = VectorNormal(matchedTFIDF);

	// compute the cosine similarity between the input and the matched words
	similarity = CosineSimilarity(inputTFIDF, matchedTFIDF, inputNorm, matchedNorm);

	return similarity;
}

static void UndoPOS(HEAPREF list)
{
	uint64 E;
	uint64 pos;
	while (list)
	{
		list = UnpackHeapval(list, E, pos);
		RemoveProperty((WORDP)E, pos); // undo changed pos marks
	}
}

static HEAPREF ProtectKeywords(char* pattern, HEAPREF protectedList)
{
	WORDP D;
	char word[MAX_WORD_SIZE];
	if (pattern) // keyword protection zone requested
	{
		if (*pattern++ == '|') while (*pattern && *pattern != '|')
		{
			pattern = ReadCompiledWord(pattern, word);
			pattern = SkipWhitespace(pattern);
			D = StoreWord(word, AS_IS);
			if (!(D->systemFlags & PATTERN_WORD))
			{
				AddSystemFlag(D, PATTERN_WORD);
				protectedList = AllocateHeapval(HV1_WORDP,protectedList, (uint64)D);
			}
		}
	}
	else while (protectedList)
	{
		uint64 E;
		protectedList = UnpackHeapval(protectedList, E, discard);
		RemoveSystemFlag((WORDP)E, PATTERN_WORD); // unprotect new protections
	}

	return protectedList;
}

static void JSONConceptList(HEAPREF list, int wordindex, MEANING array, int& index)
{ // the use of concepts and topics lists is this- script call
	uint64 val;
	char n[20];
	while (list)
	{
		list = UnpackHeapval(list, val, discard, discard);
		WORDP D = (WORDP)val;
		if (D->inferMark != inferMark && stricmp(D->word, "~bwinfo_ignore"))
		{
			sprintf(n, "%d", ++index);
			CreateFact(array, MakeMeaning(StoreWord(n, AS_IS)), MakeMeaning(D), FACTTRANSIENT | JSON_ARRAY_FACT | JSON_STRING_VALUE);
		}
	}
}

static MEANING BWInfo()
{
	char index[20];
	int counter = 0;
	NextInferMark();

	// dont detect these
	WORDP D = FindWord("~bwinfo_ignore");
	if (D)
	{
		FACT* F = GetObjectNondeadHead(D);
		while (F)
		{
			if (F->verb == Mmember) Meaning2Word(F->subject)->inferMark = inferMark;
			F = GetObjectNondeadNext(F);
		}
	}

	// make  objects of each word of sentence
	MEANING object = GetUniqueJsonComposite((char*)"jo-", FACTTRANSIENT);
	for (unsigned int i = 1; i <= wordCount; ++i)
	{
		sprintf(index, "%d", ++counter);
		MEANING C = MakeMeaning(StoreWord(index, AS_IS)); // for array of arrays

		MEANING wordarray = GetUniqueJsonComposite((char*)"ja-", FACTTRANSIENT);
		CreateFact(wordarray, MakeMeaning(StoreWord("0", AS_IS)), MakeMeaning(StoreWord(wordCanonical[i], AS_IS)), JSON_ARRAY_FACT | FACTTRANSIENT | JSON_STRING_VALUE);
		int n = 1;
		JSONConceptList(concepts[i], i, wordarray, n);

		CreateFact(object, MakeMeaning(StoreWord(wordStarts[i])), wordarray, JSON_OBJECT_FACT | FACTTRANSIENT | JSON_ARRAY_VALUE);
	}


	currentFact = NULL;
	return object;
}

static void HandleChangedVariables(MEANING resultobject, unsigned int msgcount, bool bwinfo)
{
	MEANING M = 0;
	// variables set?
	MEANING globals = 0;
	MEANING varresultobject = 0;

	NextInferMark();
	HEAPREF list = variableChangedThreadlist; // global list of variables that changed
	while (list)
	{
		uint64 varx;
		list = UnpackHeapval(list, varx, discard, discard);
		WORDP var = (WORDP)varx;
		if (var->word[1] == '_' || var->word[1] == '$') continue; // SHOULD NOT HAPPEN dont send back transients and locals
		if (var->internalBits & BEEN_HERE) continue; // only most recent value

		var->internalBits |= BEEN_HERE;
		var->internalBits &= -1 ^ VAR_CHANGED;
		if (!globals)
		{
			globals = MakeMeaning(StoreWord("newglobals", AS_IS));
			varresultobject = GetUniqueJsonComposite((char*)"jo-", FACTTRANSIENT);
			CreateFact(resultobject, globals, varresultobject, JSON_OBJECT_FACT | FACTTRANSIENT | JSON_OBJECT_VALUE);
		}
		MEANING name = MakeMeaning(var);
		if (var->w.userValue)
		{
			char* text = var->w.userValue;
			MEANING value;
			if (IsValidJSONName(text)) // write json text
			{
				unsigned int limit = 1000000;
				char* base = AllocateStack(NULL, limit);
				NextInferMark();
				jwrite(base, base, FindWord(text), 1, false, limit); // it is subject field
				value = MakeMeaning(StoreWord(base, AS_IS));
			}
			else  value = MakeMeaning(StoreWord(text, AS_IS));
			CreateFact(varresultobject, name, value, JSON_OBJECT_FACT | FACTTRANSIENT | JSON_STRING_VALUE);
		}
		else // set to null
		{
			MEANING value = MakeMeaning(StoreWord("null", AS_IS));
			CreateFact(varresultobject, name, value, JSON_OBJECT_FACT | FACTTRANSIENT | JSON_PRIMITIVE_VALUE);
		}
	}

	list = variableChangedThreadlist;
	while (list) // disable been here markers
	{
		uint64 var;
		list = UnpackHeapval(list, var, discard, discard);
		((WORDP)var)->internalBits &= -1 ^ BEEN_HERE;
	}
	UnbindVariables(variableChangedThreadlist); // undo changes we made to globals (leaves transients though and pure locals)
	variableChangedThreadlist = NULL;

	if (!globals && bwinfo)
	{
		globals = MakeMeaning(StoreWord("newglobals", AS_IS));
		varresultobject = GetUniqueJsonComposite((char*)"jo-", FACTTRANSIENT);
		CreateFact(resultobject, globals, varresultobject, JSON_OBJECT_FACT | FACTTRANSIENT | JSON_OBJECT_VALUE);
	}
	if (bwinfo)
	{
		MEANING array = BWInfo();
		CreateFact(varresultobject, MakeMeaning(StoreWord("bwinfo", AS_IS)), array, JSON_OBJECT_FACT | FACTTRANSIENT | JSON_ARRAY_VALUE);
	}
	currentFact = NULL;

}

static void ChangeJsonOwner(FACT * F, uint64 data)
{
	F->botBits = data;
	if (trace & TRACE_JSON)
	{
		char* buffer = AllocateBuffer();
		WriteFact(F, false, buffer);
		Log(USERLOG, "purge %s\r\n", buffer);
		FreeBuffer();
	}
}

static FunctionResult PurgeBootCode(char* buffer)
{
	uint64 id = 0;
	uint64 dieSign = 0xffffffffffffffff; // sign to die
	char* arg1 = ARGUMENT(1);
	if (*arg1 == '@') // factset
	{
		int setid = GetSetID(arg1);
		int count = FACTSET_COUNT(setid);
		id = dieSign;
		for (int i = 1; i <= count; ++i)
		{
			FACT* F = factSet[setid][i];
			if (F > factsPreBuild[LAYER_BOOT] && F <= factsPreBuild[LAYER_USER]) F->botBits = id; // mark with sign to die
		}
		SET_FACTSET_COUNT(setid, 0);
	}
	else if (IsValidJSONName(ARGUMENT(1)))
	{
		WORDP D = FindWord(ARGUMENT(1));
		if (!D) return FAILRULE_BIT;
		id = dieSign;
		WalkJsonHierarchy(D, ChangeJsonOwner, id);
		releaseBotVar = D->word; // allow to be overridden
	}
	else if (IsDigit(*arg1)) id = atoi64(ARGUMENT(1)); //entire bot
	if (!id) return FAILRULE_BIT;

	FACT* F = factsPreBuild[LAYER_BOOT]; // end fact of layer_1
	FACT* endBoot = factsPreBuild[LAYER_USER]; // end fact of boot layer
	FACT* tailArea = endBoot + 1; // just outside of end of boot
	while (++F <= endBoot) // walk all facts of boot
	{
		if (F->botBits != id || !F->subject) continue;  // wrong owner or moved fact
		UnweaveFact(F);
		bool dead = true;
		// at this point the dictionary knows nothing of the fact
		// now repack other fact to fill in the hole IF WE CAN
		// Find a valid fact in boot LATER than where we have this hole
		// if we cant find one, then WE are at the end of the area
		while (--tailArea > F) // find someone to move
		{
			if (tailArea->botBits == id || tailArea->flags & FACTDEAD) continue;  // dying owner or already dead

			UnweaveFact(tailArea); // cut from from dictionary referencing
			memcpy(F, tailArea, sizeof(FACT)); // replicate fact lower
			WeaveFact(F); // rebind fact to dictionary

			// now pre-moved fact is dead
			tailArea->subject = tailArea->verb = tailArea->object = 0;
			tailArea->flags |= FACTDEAD;
			// we assume no one has a factref to this fact (or the other)
			// and we are not saved in factset or such
			dead = false;
			break;
		}
		// kill this completely in case we cant overwrite it with valid fact
		if (dead)
		{
			F->subject = F->verb = F->object = 0;
			F->flags |= FACTDEAD;
		}
	}

	// this leaves us with potentially useless dictionary entries.
	// words in the boot layer that have no properties or fact references can go
	// therefore flag words as deletable, they will be actually removed in MigrateFactsToBoot
	NextInferMark();

	// need to keep all the bot variables regardless
	HEAPREF varthread = botVariableThreadList;
	char* value;
	while (varthread)
	{
		uint64 Dx;
		varthread = UnpackHeapval(varthread, Dx, discard, discard);
		WORDP D = (WORDP)Dx;
		D->inferMark = inferMark;
		value = D->w.userValue;
		WORDP E = FindWord(value);
		if (E) E->inferMark = inferMark;  // protects JSON objects that have been created but currently have no facts
	}

	WORDP D = dictionaryPreBuild[LAYER_BOOT] - 1; // last word of layer_1
	WORDP endBootWord = dictionaryPreBuild[LAYER_USER] - 1; // last word of boot layer
	while (++D <= endBootWord) // walk all words of boot layer
	{
		bool keep = false;
		char* word = D->word;
		if (D->properties || D->systemFlags & PATTERN_WORD || D->inferMark == inferMark) keep = true;
		else if (*word == USERVAR_PREFIX && word[1] != LOCALVAR_PREFIX) keep = true;
		else if (GetSubjectNondeadHead(D) || GetVerbNondeadHead(D) || GetObjectNondeadHead(D)) keep = true;

		// nothing interesting on this word, so get rid of it
		if (!keep) D->internalBits |= DELETED_MARK;
	}

	return NOPROBLEM_BIT;
}

static void CreatePatternInternalConcepts(char* pattern)
{
	uint64 oldbot = myBot;
	// input has concept followed by members followed by concept followed by memers
	if (*pattern == '^') ++pattern; // compile pattern mark
	char* patternStart = strchr(pattern, '(');
	if (!patternStart) return; // bad data

	char word[MAX_WORD_SIZE];
	MEANING concept = 0;
	while (1)
	{
		pattern = ReadCompiledWord(pattern, word);
		if (!*word || *word == '(' || *word == '^' || *word == '|') break; // ends at actual pattern or compiled pattern mark
		if (*word == '~') // concept name listed
		{
			char name[MAX_WORD_SIZE];
			sprintf(name, "~3%05u", ++internalConceptIndex);
			concept = MakeMeaning(StoreWord(name, AS_IS));

			// now revise pattern to use new name
			char* x = patternStart;
			while ((x = strstr(x, word))) strncpy(x, name, 7);
		}
		else // keyword for concept listed
		{
			char* key = word;
			int flags = FACTTRANSIENT;
			if (*word == '\'')
			{
				++key;
				flags |= ORIGINAL_ONLY;
			}
			MEANING keyname = MakeMeaning(StoreWord(key, AS_IS));
			CreateFact(keyname, Mmember, concept, flags);
		}
	}
	myBot = oldbot;
}

void UpdateTrace(char* value)
{
	char* at = value - 1;
	int n = 0;
	// trace buffer may be bigger than output buffer. 
	// Presume output not to exceed 12k without us
	char* limit = traceBase + TESTPATTERN_TRACE_SIZE - 12000;
	char* limit1 = traceBase + maxBufferSize - 12000;
	if (limit1 < limit) limit = limit1;
	if (traceTestPatternBuffer > limit)
		return; // too much trace, abandon rest
	bool begun = false;
	while (*++at)
	{
		if (*at == '\r' || *at == '\n' || *at == ' ')
		{
			if (!begun && (*at == ' ' || IsDigit(*at)))
			{
				if (++n <= traceIndex)  continue; // ignore leading whitespace up to globaldepth start
			}
		}
		else begun = true;

		if (*at == '\r') { *traceTestPatternBuffer++ = '\\';  *traceTestPatternBuffer++ = 'r'; }
		else if (*at == '\t') { *traceTestPatternBuffer++ = '\\';  *traceTestPatternBuffer++ = 't'; }
		else if (*at == '\n') { *traceTestPatternBuffer++ = '\\';  *traceTestPatternBuffer++ = 'n'; }
		else  *traceTestPatternBuffer++ = *at;
		*traceTestPatternBuffer = 0;
		if (traceTestPatternBuffer > limit)
			break; // too much trace, abandon rest
	}
}

static char* GetPatternReference(FACT * F, char* label)
{
	*label = 0;
	char* pattern = Meaning2Word(F->object)->word; // may directly be pattern
	if (IsValidJSONName(pattern, 'o')) // composite notation
	{
		pattern = NULL;
		FACT* G = GetSubjectHead(F->object);
		while (G)
		{
			char* verb = Meaning2Word(G->verb)->word;
			if (!stricmp(verb, "pattern")) pattern = Meaning2Word(G->object)->word;
			else if (!stricmp(verb, "label")) strcpy(label, Meaning2Word(G->object)->word);
			G = GetSubjectNext(G);
		}
	}
	return pattern;
}

static bool isOrdinaryWordNotMatchLabel(char c, char* token, char* label)
{
	size_t label_len = strlen(label);
	bool flag = c != '$' && c != '%' && IsAlphaUTF8DigitNumeric(c) &&
		(label_len == 0 || strnicmp(token, label, label_len));
	return flag;
}

bool NeedNL(MEANING patterns)
{
	FACT* F = GetSubjectHead(patterns);
	while (F) // do any patterns imply the need for NL analysis
	{
		// meanwhile, protect keywords listed under pattern protection
		char label[100];
		char* pattern = GetPatternReference(F, label); // may directly be pattern
		if (!pattern)
		{
			F = GetSubjectNondeadNext(F);
			continue;
		}
		if (*pattern == '|')
			return true; // has keywords that need protection (even if just labels)

		char token[MAX_WORD_SIZE];
		char* at = pattern;
		if (*at == '^') ++at; // skip compiled code marker
		int function = 0;
		while (at > (char*)1 && *at)
		{
			at = ReadCompiledWord(at, token);
			char* tok = token;
			char c = *tok;
			while (c == '!') c = *++tok; // skip over not flags
			if (c == ':' || c == '=') //assignment  or comparison (token has entire unit)
			{
				continue; 
			}
			if (c == '^') // function call?
			{
				WORDP X = FindWord(token);
				if (!stricmp(token, "^respond")) return true;	 // uses cs rules, assume it needs data
				if (X && X->internalBits & FUNCTION_NAME) function = 1;
			}
			else if (c == '(' && function) function = 2;
			else if (c == ')' && function == 2) function = 0; // call over (not allowed to nest calls)
			else if (function == 2) {} // ignore all arguments, they are not things to match in sentence
			else if (c == '_' || c == '~' || c == '\'' || c == '"' || c == '?' || (c == '\\' && tok[1] == '!'))
				return true; // memorization or concept or quoting implies nl
			else if (!strnicmp(tok, "%length", 7)) return true;
			// ordinary word, not old fashioned excluder with same name as label
			else if (isOrdinaryWordNotMatchLabel(c, tok, label))
				return true; // normal word to match (or system var might need nl)
		}
		F = GetSubjectNext(F);
	}
	return false;
}

static void ExecuteDebug(MEANING resultobject)
{
	char node[MAX_WORD_SIZE];
	char* ptr = ReadCompiledWord(debugdata, node); // strip node name, rest is uncompiled script

	if (*ptr) // execute this code...
	{
		char* buffer = AllocateBuffer();
		FunctionResult result;
		FreshOutput(ptr, buffer, result, 0, maxBufferSize);
		if (result == NOPROBLEM_BIT)
		{
			MEANING debugM = MakeMeaning(StoreWord("debug", AS_IS));
			CreateFact(resultobject, debugM, MakeMeaning(StoreWord(buffer, AS_IS)), JSON_OBJECT_FACT | FACTTRANSIENT | JSON_STRING_VALUE);
		}
		FreeBuffer();
	}
}

FunctionResult ProtectPatternKeys(FACT* facts[MAX_TEST_PATTERN],bool needNL, MEANING patterns,int& count,
	double patternscores[MAX_TEST_PATTERN],HEAPREF& protectedKey,bool& tracepercent)
{
	count = 0;
	FACT* F = GetSubjectHead(patterns);
	while (F) // pattern objects in inverse order and protect patternwords
	{
		//(ja - t1002 0 jo - t1003 //  jo-t1003 is pattern object OR the pattern
		facts[count++] = F;
		if (count >= MAX_TEST_PATTERN)
		{
			if (trace & TRACE_PATTERN) Log(USERLOG, "Too many patterns, above %d ", MAX_TEST_PATTERN);
			return FAILRULE_BIT; // too many
		}
		// meanwhile, protect keywords listed under pattern protection
		char label[100];
		char* pattern = GetPatternReference(F, label); // may directly be pattern
		if (!pattern) return FAILRULE_BIT; // too many// failed to pass in correctly
		if (strstr(pattern, "%trace_on")) tracepercent = true; // script will control pattern tracing manually
		if (needNL)
		{
			protectedKey = ProtectKeywords(pattern, protectedKey); // does protection with pattern
			CreatePatternInternalConcepts(pattern); // does protection with pattern
		}

		// initial tf/idf score for this pattern
		patternscores[count - 1] = 0;

		F = GetSubjectNext(F);
	}
	return NOPROBLEM_BIT;
}

static int 	StoreTFIDF(int index,double patternscores[MAX_TEST_PATTERN],int  winner,double& bestScore,wordScore_t idfScores,char* ptr, wordScore_t inputTFIDF, double inputNorm)
{
	// calculate a TF score for the pattern tokens
	char* patternWords = AllocateBuffer();

	// splice in the interesting words from the pattern
	char word[MAX_WORD_SIZE];
	char* wordptr = patternWords;
	wordScore_t::iterator it;
	while (ALWAYS)
	{
		char* nextTokenStart = SkipWhitespace(ptr);
		ptr = ReadCompiledWord(nextTokenStart, word);
		if (!*word) break;

		WORDP D = FindWord(word);
		if (D)
		{
			it = idfScores.find(D);
			if (it == idfScores.end()) continue;

			strcat(wordptr, " ");
			++wordptr;
			strcat(wordptr, D->word);
			wordptr += strlen(D->word);
			strcat(wordptr, " 0");  // will only allow each word to be counted once
			wordptr += 2;
		}
	}

	double similarity = PatternMatchSimilarity(inputTFIDF, inputNorm, patternWords, idfScores);
	if (similarity > patternscores[index]) patternscores[index] = similarity;
	FreeBuffer();
	if (similarity > bestScore)
	{
		winner = testPatternIndex; // best similarity
		bestScore = similarity;
	}
	return winner;
}


static double SortTFIDF(wordScore_t idfScores, wordScore_t inputTFIDF)
{
	double inputNorm;
	inputTFIDF.clear();
		char* inputWords = AllocateBuffer();
		*inputWords = 0;
		MARKDATA hitdata;
		for (wordScore_t::iterator it = idfScores.begin(); it != idfScores.end(); ++it)
		{
			WORDP D = it->first;
			int start = 0;
			int numWords = 0;
			while (GetNextSpot(D, start, false, 0, &hitdata))
			{
				// more words => better scores
				numWords += hitdata.end - hitdata.start + 1;
				start = hitdata.start;
			}
			if (numWords > 0)
			{
				char wordData[MAX_WORD_SIZE + 10];
				sprintf(wordData, "%s %d ", D->word, numWords);
				strcat(inputWords, wordData);
			}
		}

		inputTFIDF = ComputeTFIDF(inputWords, idfScores, wordCount);
		inputNorm = VectorNormal(inputTFIDF);
		FreeBuffer();
		return inputNorm;
}

static bool InitTFIDF(wordScore_t idfScores, MEANING idf)
{
		if (!idf)
		{
			if (trace & TRACE_PATTERN) Log(USERLOG, "Missing idf scores ");
			return false;    // must have idf scores for a tf/idf style
		}

		// convert scores to a map for faster and easier processing
		idfScores.clear();
		FACT* F = GetSubjectHead(idf);
		while (F)
		{
			WORDP D = Meaning2Word(F->verb);
			idfScores[D] = Convert2Double(Meaning2Word(F->object)->word);
			F = GetSubjectNext(F);
		}
		return true;
}

static void LimitTrace(MEANING resultobject, char* traceBase, char* traceTestPatternBuffer)
{
		if (*traceBase == 0) strcpy(traceBase, "cs bug- no trace?");
		else
		{
			// how much space is used up by actual answer data
			unsigned int limit = 4000000; // 4 mb 
			char* base = AllocateStack(NULL, limit);

			jwrite(base, base, Meaning2Word(resultobject), 1, false, limit);
			size_t len = strlen(base); // how much size is the output that must be shipped
			size_t bufferlimit = outputsize - len - 1000;   // how much is left over for trace, leave room for misc header stuff

			// limit trace as needed
			size_t tracelen = traceTestPatternBuffer - traceBase;
			if (tracelen >= bufferlimit)  traceBase[bufferlimit] = 0; // truncate trace

			ReleaseStack(base);
		}
}

static int TraceSetup(char* traceit, int priortrace,bool tracepercent)
{
	// Many ways to enable tracing of testpattern...
	// DSE can pass field "trace" which if exists will trigger tracing
	// Scripter pattern may contain %trace_on  which will trigger tracing at that point
	// Input sentence may have bruce-cheat or whatever authorization code is for user input
	// Init file may have "servertrace" enabled as run time param
	int tracepatterndata = 0; // no trace by default
	traceTestPatternBuffer = traceBase = NULL;

#ifdef NOMAIN
	trace = 0;
#else
	if (server || pseudoServer) trace = 0;
#endif
	if (hadAuthCode)
	{
		if (hadAuthCode == 1) trace = TRACE_PATTERN;
		else trace = (unsigned int)(-1 ^ TRACE_POS); // full trace
		tracepatterndata |= 2; // bruce-cheat auth code given

		// persist the user input onto all following calls of this volley
		char test[100];
		strcpy(test, serverlogauthcode);
		if (hadAuthCode == 3) strcat(test, "3");
		else if (hadAuthCode == 2) strcat(test, "2");
		else if (hadAuthCode == 1) strcat(test, "1");
		SetUserVariable("$bwtrace", test); // force testoutputs to also be traced or authorized
	}

	if (servertrace) trace = (unsigned int)-1; // goes into user log if enabled but not to api
	if (traceit && !priortrace) // prior trace allows us local tracing against data saved with tracing on in dse
	{
		trace = TRACE_PATTERN;
		tracepatterndata |= 1; // requested by outside using TRACE field for all patterns
	}
	if (tracepercent)  	tracepatterndata |= 2;

	if (blockapitrace) // param on prod machines prevents accidental %trace_on left in patterns
	{
		trace = 0;
		tracepatterndata = 0;
	}
	else if (tracepatterndata) // trigger saving trace to pass back in output
	{
		traceTestPatternBuffer = traceBase = tracebuffer;
		*tracebuffer = 0;
	}
	return tracepatterndata;
}

static bool PrepareTestPatternInput(char* usermsg,int msgcount,uint64 elapsedMS, char* timelimit)
{
	if (elapsedMS && ElapsedMilliseconds() >= elapsedMS)
	{
		if (*GetUserVariable("$cs_analyzelimitlog")) ReportBug("^testpattern timer lapse %d", atoi(timelimit));
		return false; // time limitation
	}

	EraseCurrentInput(); // we know there is no useful input currently

	char counter[10];
	sprintf(counter, "%d", msgcount); // 1 based
	if (msgcount >= sentenceLimit) return false; // usual internal limit
	sentenceOverflow = (msgcount + 1) >= sentenceLimit;
	SetUserVariable("$$cs_sentencecount", counter);
	char patternName[30];
	sprintf(patternName, "testPattern_%s", counter); // likely unique name
	WORDP name = StoreWord(patternName, AS_IS);
	usermsg = SkipWhitespace(usermsg);

	if (*usermsg == 1)
	{
		moreToCome =  false;
		moreToComeQuestion =  false;
		wordCount = 0;
		return true; // input was empty
	}

	char* in = AllocateBuffer("testpattern");
	strcpy(in, "more ");
	strcat(in, usermsg);
	directAnalyze = true;
	char* buffer = AllocateBuffer("PrepareTestPatternInput");
	InternalCall("^AnalyzeCode", AnalyzeCode, in, NULL, NULL, buffer); // analyze returns leftover input in buffer
	directAnalyze = false;
	strcpy(usermsg, buffer);
	FreeBuffer();
	FreeBuffer();
	moreToCome = (*usermsg) ? true : false;
	moreToComeQuestion = strchr(usermsg, '?') ? true : false;

	char* prepassTopic = GetUserVariable("$cs_prepass", false);
	if (*prepassTopic) PrepassSentence(prepassTopic);

	return true;
}

static FunctionResult TestPatternCode(char* buffer)
{
	char* arg = ARGUMENT(1); // json object
	if (!IsValidJSONName(arg, 'o')) return FAILRULE_BIT;
	WORDP D = FindWord(arg);
	if (!D) return FAILRULE_BIT;

	FunctionResult result = NOPROBLEM_BIT;
	int olduserlog = userLog;
	int oldtrace = trace;
	timeout = false;
	traceTestPatternBuffer = NULL; // where to put trace if authorized
	*testpatternlabel = 0;
	errorIndex = warnIndex = 0;
	traceIndex = globalDepth;
	testPatternIndex = -1;
	wordScore_t idfScores;

	// break out the data passed in via JSON
	MEANING input = 0;
	MEANING patterns = 0;
	MEANING variables = 0;
	MEANING conceptlist = 0;
	style = NULL;
	bool tfidf = false;
	char* traceit = NULL;
	MEANING idf = 0;
	FACT* F = GetSubjectHead(D); // dont test language and death. use all facts
	while (F)
	{
		char* field = Meaning2Word(F->verb)->word;
		if (!stricmp(field, "input")) input = F->object;
		else if (!stricmp(field, "language")) SetLanguage(Meaning2Word(F->object)->word);
		else if (!stricmp(field, "patterns")) patterns = F->object;
		else if (!stricmp(field, "variables")) variables = F->object; // we will ignore these (deprecated)
		else if (!stricmp(field, "concepts")) conceptlist = F->object;
		else if (!stricmp(field, "idf")) idf = F->object;
		else if (!stricmp(field, "trace")) traceit = Meaning2Word(F->object)->word;
		else if (!stricmp(field, "style")) style = Meaning2Word(F->object)->word; // first, all, best, last, tfidf
		else if (trace & TRACE_PATTERN) Log(USERLOG, "Unknown field %s ", field);
		F = GetSubjectNext(F);
	}
	if ((!input && !variables) || !patterns) // we can match against user text or just incoming variables
	{
		if (trace & TRACE_PATTERN)
		{
			if (!input && !variables) Log(USERLOG, "Missing input  or variables");
			if (!patterns) Log(USERLOG, "Missing patterns ");
		}
		return FAILRULE_BIT;    // must have input/variables and patterns
	}

	if (style && !stricmp(style, "tfidf")) tfidf = true;
	if (tfidf && !InitTFIDF(idfScores,  idf)) return FAILRULE_BIT;
	
	bool needNL = NeedNL(patterns); // do patterns suggest we need to do NL analysis?

	csapicall = TEST_PATTERN;

	// can preprocess input, see if we need to change it
	SetUserVariable("$$cs_testpatterninput", Meaning2Word(input)->word, false, true);
	char* value = GetUserVariable("$$cs_testpatterninput"); // the address used
	FunctionResult r = OnceCode("$testpatternpretopic");
	char* val1 = GetUserVariable("$$cs_testpatterninput");
	if (val1 != value) // data was changed
	{
		input = MakeMeaning(StoreWord(val1, AS_IS));
	}

	// meaningful user input
	bool markConcepts = false;
	bool realInput = true;
	WORDP X = (input) ? Meaning2Word(input) : StoreWord(" ", AS_IS);
	rawtestpatterninput = X->word; // global visible for ^original(rawuser)
	char* usermsg = AllocateBuffer();
	parseLimited = false;

	// dont trace fact creation for concepts
	FACT* oldfact = lastFactUsed;
	HEAPREF substitutesList = NULL;
	HEAPREF protectPOSList = NULL;
	
	// do incoming variable assignments which may change language before processing concepts
	HEAPREF changedIncomingVariables = MakeVariables(variables, needNL);
	// prepare for changes in variables during execution
	variableChangedThreadlist = NULL;

	if (needNL)
	{
		if (stricmp(X->word, "null")) strcpy(usermsg, SkipWhitespace(X->word));
		else
		{
			markConcepts = true;
			realInput = false;
		}
		AdjustUTF8(usermsg, usermsg - 1);
		CheckParseLimit(usermsg);
		if (conceptlist)  result = MakeConcepts(needNL, conceptlist, substitutesList, protectPOSList, markConcepts);
	}

	SAVESYSTEMSTATE()

		// define concepts -  [  {name: [] } ]

	FACT* newfact = lastFactUsed; // end of concept zone

	if (result != NOPROBLEM_BIT)
	{
		if (trace & TRACE_PATTERN) Log(USERLOG, "Concept definition bad ");
		RESTORESYSTEMSTATE()
		FreeBuffer(); //usermsg
		UndoAPICall(changedIncomingVariables,conceptlist, newfact, oldfact, substitutesList); 
		patternwordthread = NULL;
		csapicall = NO_API_CALL;
		trace = oldtrace;
		userLog = olduserlog;
		return result;
	}

	if (!*debugdata) strcpy(debugdata, GetUserVariable("$bwdebug")); // if any

	// if no style on call, use global. no global, default to first
	bool styleoncall = (style) ? true : false;
	if (!style) style = (char*)"first"; // default if not given

	// return JSON object with result: index and possible match values in the future
	// get the object
	MEANING resultobject = GetUniqueJsonComposite((char*)"jo-", FACTTRANSIENT);
	D = Meaning2Word(resultobject);

	// since analysis is expensive, normally we analyze a sentence then test all patterns on it
	// test the patterns - presume compiled, not using RETRY on pattern, not returning value

	double patternscores[MAX_TEST_PATTERN];
	internalConceptIndex = 0; // for autolabeling concepts in patterns

	int count = 0;
	HEAPREF protectedKey = NULL;
	FACT* facts[MAX_TEST_PATTERN];
	bool tracepercent = false;
	if (ProtectPatternKeys(facts,needNL,patterns,count,patternscores, protectedKey, tracepercent) == FAILRULE_BIT)
	{
		RESTORESYSTEMSTATE()
		FreeBuffer(); //usermsg
		UndoAPICall(changedIncomingVariables, conceptlist, newfact, oldfact, substitutesList); // kill these facts
		csapicall = NO_API_CALL;
		trace = oldtrace;
		userLog = olduserlog;
		return FAILRULE_BIT; // too many
	}

	int tracepatterndata = TraceSetup(traceit, trace & TRACE_PATTERN, tracepercent);

	int totalPatterns = count;
	int winner = NO_WINNER;
	double bestScore = 0;
	char* pattern = NULL;

	// what pattern nesting depth triggers newline on return
	int oldindex = indentBasis;
	char* var = GetUserVariable("$indentlevel", false);
	if (*var) indentBasis = atoi(var);  // local in call
	else
	{
		var = GetUserVariable("$cs_indentlevel", false);
		if (*var) indentBasis = atoi(var);  // default from init file
		else indentBasis = 3; // default
	}

	if (!strnicmp(usermsg, ":tracepatternlevel ", 19) && AuthorizedCode(NULL) == NOPROBLEM_BIT)
	{
		char depth[100];
		usermsg = ReadCompiledWord(usermsg + 19, depth);
		indentBasis = atoi(depth);
	}
	char* startx = SkipWhitespace(usermsg);
	if (!strnicmp(usermsg, "cheat cs info", 13))
	{
		usermsg += 13;
		char buf[MAX_WORD_SIZE];
		sprintf(buf, "Engine: %s Script1: %s Script2: %s", compileDate, timeStamp[1], timeStamp[0]);
		SetUserVariable("$cs_info", buf);
	}

	bool bwinfo = false;

	char* put = "";
	if (stricmp(X->word, "null")) put = X->word;
	testpatterninput = put;
	// printf("RAW %s\r\n", HexDisplay(put));

	// dynamic debug display system

	int debug = 0;
	if (*debugdata)
	{
		debug = 1; // begin scan
		char* dvar = GetUserVariable("$bwdebug");
		if (!*dvar) SetUserVariable("$bwdebug", debugdata); // pass it onward to testoutput as well
	}
	char patternName[30];

	// see if we have prescans that require special handling
	F = facts[totalPatterns - 1]; // 1st fact
	pattern = GetPatternReference(F, testpatternlabel); // known to return a pattern
	char copy[100];  // command must be 1st signfiicant token
	strncpy(copy, pattern, 50);
	copy[50] = 0;
	char* hasprescan = strstr(copy, "%testpattern-prescan");

	int msgcount = 0;
	int prescanlimit = 0;
	uint64 elapsedMS = 0;
	char* timelimit = GetUserVariable("$cs_analyzelimit");
	// dont use time limit when running local console debugging
#ifdef NO_MAIN
	if (*timelimit && !hadAuthCode) elapsedMS += callStartTime + atoi(timelimit); // how much ms to allow for testpattern analysis
#else
	if (server && *timelimit && !hadAuthCode) elapsedMS += callStartTime + atoi(timelimit); // how much ms to allow for testpattern analysis
#endif

	// show :prepare info
	char* bwin = strstr(usermsg, "bwinfo ");
	if (bwin)
	{
		bwinfo = true;
		strncpy(bwin, "      ", 6);
	}
	
	char* xy = usermsg;
	while ((xy = strchr(usermsg, '`'))) *xy = ' '; // get rid of internal reserved use

	if (hasprescan) // do any prescans - prescans MUST needNL
	{
		if (setjmp(scriptJump[++jumpIndex])) // crash return
		{
			msgcount = 0;
			*buffer = 0;
			result = NOPROBLEM_BIT;
			*usermsg = 0;
		}

		while (*usermsg)
		{
			if (!PrepareTestPatternInput(usermsg, ++msgcount, elapsedMS,timelimit)) break;

			// do all prescans on this sentence
			int limit = totalPatterns;
			int matchIndex = 0;
			while (limit)
			{
				FACT* F = facts[--limit];
				pattern = GetPatternReference(F, testpatternlabel); // known to return a pattern
				char copy[100];  // command must be 1st signfiicant token
				strncpy(copy, pattern, 50);
				copy[50] = 0;
				if (!strstr(copy, "%testpattern-prescan"))
				{
					prescanlimit = totalPatterns - limit - 1; // so we can discard prescans later
					break; // end of prescans on this sentence
				}
		
				if (debug == 1) // see if label is one we want
				{
					if (!strnicmp(debugdata + 1, testpatternlabel, strlen(testpatternlabel)))
					{
						debug = 2; // found what we wanted
						break; // stop now
					}
				}
				if (TestPatternAgainstSentence(msgcount, matchIndex, testpatternlabel, pattern, buffer) && winner == NO_WINNER) winner = matchIndex; // found highest priority match on this sentence
				++matchIndex;
			} // end all prescans against sentence
			char id[20];
			sprintf(id, "testPattern_%d", msgcount);
			InternalSaveSentence(id); // save any changes to sentence as dominant over any prior
		}
		*usermsg = 1; // just to activate next loop on all patterns remaining
		--jumpIndex;
	} 

	// now do all patterns on all sentences except %testpattern-prescan 
	if (setjmp(scriptJump[++jumpIndex])) // return on BADSCRIPT call
	{
		--jumpIndex;
		*buffer = 0;
		result = NOPROBLEM_BIT;
		msgcount = 0;
	}

	if (trace & TRACE_PATTERN)   Log(USERLOG, "testpattern style: %s \r\n", style);

	int oldwinner = winner;
	winner = NO_WINNER;
	int msgcounter = 0;
	if (!*usermsg) *usermsg = 1; // flag for empty message, but we have to run once at least
	while(*usermsg) 
	{
		++msgcounter;

		if (needNL) 
		{
			if (hasprescan ) 
			{
				sprintf(patternName, "testPattern_%d", msgcounter); 
				result = RestoreSentence(StoreWord(patternName));
				if (result == FAILRULE_BIT)
				{
					result = NOPROBLEM_BIT;
					break;
				}
			}
			else
			{
				if (!PrepareTestPatternInput(usermsg, msgcounter, elapsedMS,timelimit)) break;
			}
		}
		char counter[10];
		sprintf(counter, "%d", msgcounter); // 1 based
		SetUserVariable("$$cs_sentencecount", counter);

		// compute a tf/idf and norm for this input from the actual marks
		wordScore_t inputTFIDF;
		double inputNorm = 0;
		if (tfidf) inputNorm = SortTFIDF( idfScores, inputTFIDF);

		// walk array of pattern objects- correctly reverse ordered
		count = totalPatterns;
		int matchIndex = 0;
		while (count--)
		{
			int index = totalPatterns -  count - 1; // 0 to n-1
			if (index < prescanlimit) continue; // already prescanned
			F = facts[count];
			pattern = GetPatternReference(F, testpatternlabel);  // known to return a pattern
	
			// if we have a winner and want earliest/first, dont test later ones
			if (winner != NO_WINNER && (!stricmp(style, "earliest") || !stricmp(style, "first")) && index >= winner) { ; }
			// if we have a winner and want latest/last, dont test earlier ones
			else if (winner != NO_WINNER && (!stricmp(style, "latest") || !stricmp(style, "last")) && index <= winner) { ; }
			else // style is all
			{
				testPatternIndex = index;
				if (debug == 1) // see if label is one we want
				{
					if (!strnicmp(debugdata + 1, testpatternlabel, strlen(testpatternlabel)))
					{
						debug = 2; // found what we wanted
						break; // stop now
					}
				}
				bool match = TestPatternAgainstSentence(msgcounter, index, testpatternlabel, pattern, buffer);
				if (match)
				{
					if (tfidf) winner = StoreTFIDF(index,patternscores, winner, bestScore, idfScores, pattern, inputTFIDF, inputNorm);
					else winner = index; // found highest priority match on this sentence
				}
			}
		} // end msg loop
		if (winner == NO_WINNER) winner = oldwinner; // show prescan match if any
		if (!needNL) break; // dont run patterns more than once if no nl needed

		if (!styleoncall) // style named in pattern itself
		{
			char* xstyle = GetUserVariable("$$cs_testpattern_style", false);
			if (xstyle) style = xstyle;
		}
		if (result != NOPROBLEM_BIT) break;
		if (winner != NO_WINNER && !stricmp(style, "any")) break; // otherwise find best match across all inputs
	}
	--jumpIndex;
	indentBasis = oldindex;

	if (curlFail)
	{
		CreateFact(resultobject, MakeMeaning(StoreWord("jsonopen-status")),
			MakeMeaning(StoreWord("timeout", AS_IS)), JSON_OBJECT_FACT | FACTTRANSIENT | JSON_STRING_VALUE);
	}
	responseIndex = 0;
	StoreCompileErrors(resultobject); // actually execution errors we reported as compile errors

	RESTORESYSTEMSTATE() // implicit freebuffer happens
	FreeBuffer(); //usermsg
	testPatternIndex = -1;

	// now run optional posttopic which can monitor variables and replicate
	OnceCode((char*)"$testpattern_posttopic", NULL);

	// we dont want to trace killing facts
	trace = 0;
	if (needNL)
	{
		ProtectKeywords(NULL, protectedKey); // unprotects as need be
		UndoPOS(protectPOSList);
	}
	HandleChangedVariables(resultobject, msgcount, false);
	UndoAPICall(changedIncomingVariables, conceptlist, newfact, oldfact, substitutesList);

	// store the result index
	unsigned int flags = JSON_OBJECT_FACT | FACTTRANSIENT | JSON_PRIMITIVE_VALUE;
	MEANING match = MakeMeaning(StoreWord("match", AS_IS));
	MEANING testpattern = MakeMeaning(StoreWord("pattern", AS_IS));
	if (winner != NO_WINNER)
	{
		char word1[MAX_WORD_SIZE];
		sprintf(word1, "%d", winner);
		CreateFact(resultobject, match, MakeMeaning(StoreWord(word1, AS_IS)), flags);
		if (trace & TRACE_PATTERN) Log(USERLOG, "result: winner %d", winner);
	}
	else if (bwinfo) CreateFact(resultobject, match, MakeMeaning(StoreWord("0", AS_IS)), flags);
	else
	{
		CreateFact(resultobject, match, MakeMeaning(StoreWord("false", AS_IS)), flags);
		// CreateFact(resultobject, testpattern, MakeMeaning(StoreWord(pattern, AS_IS)), JSON_OBJECT_FACT | FACTTRANSIENT | JSON_STRING_VALUE);
		if (trace & TRACE_PATTERN) Log(USERLOG, "result: no match");
	}

	if (debug == 2) // debug field triggered
	{
		ExecuteDebug(resultobject);
	}

	if (tfidf)
	{
		MEANING scores = MakeMeaning(StoreWord("scores", AS_IS));
		MEANING scoresarray = GetUniqueJsonComposite((char*)"ja-", FACTTRANSIENT);
		CreateFact(resultobject, scores, scoresarray, JSON_OBJECT_FACT | FACTTRANSIENT | JSON_ARRAY_VALUE);

		unsigned int flags = JSON_ARRAY_FACT | FACTTRANSIENT | JSON_PRIMITIVE_VALUE;
		for (unsigned int i = 0; i < (unsigned int)totalPatterns; i++)
		{
			char word1[MAX_WORD_SIZE];
			sprintf(word1, "%u", i);
			char word2[MAX_WORD_SIZE];
			sprintf(word2, "%f", patternscores[i]);
			CreateFact(scoresarray, MakeMeaning(StoreWord(word1, AS_IS)), MakeMeaning(StoreWord(word2, AS_IS)), flags);
		}
	}

	if (tracepatterndata)  // tracing was enabled
	{
		LimitTrace(resultobject, traceBase, traceTestPatternBuffer);
		CreateFact(resultobject, MakeMeaning(StoreWord("trace")),
			MakeMeaning(StoreWord(traceBase, AS_IS)), JSON_OBJECT_FACT | FACTTRANSIENT | JSON_STRING_VALUE);
		
		traceTestPatternBuffer =  traceBase = NULL;
		tracepatterndata = 0;
	}

	sprintf(buffer, "%s", Meaning2Word(resultobject)->word);
	trace = oldtrace;
	userLog = olduserlog;
	currentFact = NULL; // dont pass up changes in facts
	csapicall = NO_API_CALL;
	return result;
}

// preprocess output request so all variables convert japanese . - _  space to ascii

static FunctionResult TestOutputCode(char* buffer)
{
	*testoutputFail = 0;
	*testoutputbacktrace = 0;
	testoutputbacktracecount = 0;
	FunctionResult result = NOPROBLEM_BIT;
	char* arg = ARGUMENT(1); // json object
	if (!IsValidJSONName(arg, 'o')) return FAILRULE_BIT;
	WORDP D = FindWord(arg);
	if (!D) return FAILRULE_BIT;
	heapPatternThread = NULL;
	errorIndex = warnIndex = 0;
	timeout = false;

	// output can have variable references
	// { output: "", variables: [ {name: x, value: y} }
	// returns output generated, possible return variables
	int oldtrace = trace;
	int olduserlog = userLog;

	MEANING output = 0;
	MEANING variables = 0;
	MEANING conceptlist = 0;
	FACT* F = GetSubjectHead(D);
	while (F)
	{
		WORDP field = Meaning2Word(F->verb);
		if (!stricmp(field->word, "output")) output = F->object;
		else if (!stricmp(field->word, "variables")) variables = F->object;
		else if (!stricmp(field->word, "concepts")) conceptlist = F->object;
		else if (!stricmp(field->word, "language")) SetLanguage(Meaning2Word(F->object)->word);
		else
		{
			if (trace & TRACE_OUTPUT) Log(USERLOG, "Unknown field %s ", field->word);
			return FAILRULE_BIT;
		}
		F = GetSubjectNext(F);
	}
	if (!output) return FAILRULE_BIT;    // must have output

	SAVESYSTEMSTATE()
	HEAPREF substitutesList = NULL;
	csapicall = TEST_OUTPUT;

	// do incoming variable assignments
	HEAPREF changedIncomingVariables = MakeVariables(variables, false);
	// prepare for changes in variables during execution
	variableChangedThreadlist = NULL;
	FACT* oldfact = lastFactUsed;
	FACT* newfact = lastFactUsed; // end of concept zone
	HEAPREF junk = NULL;
	if (conceptlist) result = MakeConcepts(false, conceptlist, substitutesList, junk, false);

	// allow tracing in authorized environments by request
	// process each sentence in turn against the patterns
	// first pattern to match ANY input sentence wins
	char* value = GetUserVariable("$tracetestOutput", false);
	if (!value) value = GetUserVariable("$cs_tracetestOutput", false);
	if (hadAuthCode && !blockapitrace)
	{
		trace = (unsigned int)-1;
		userLog = FILE_LOG;
	}
	if (!blockapitrace && !stricmp(value, "1") && AuthorizedCode(NULL) == NOPROBLEM_BIT)
	{
		trace = (unsigned int)-1;
		userLog = FILE_LOG;
	}

	// return JSON object with result: index and possible match values in the future
	// get the object
	MEANING resultobject = GetUniqueJsonComposite((char*)"jo-", FACTTRANSIENT);
	D = Meaning2Word(resultobject);
	int winner = 100000;
	char* check = Meaning2Word(output)->word;

	strcpy(debugdata, GetUserVariable("$bwdebug")); // if any

	int buffercount = bufferIndex;
	int frameindex = globalDepth;

	if (setjmp(scriptJump[++jumpIndex])) // return on script compiler error
	{
		bufferIndex = buffercount;
		*buffer = 0;
		result = FAILRULE_BIT;
	}
	else
	{
		char* traceTestPatternBuffer = NULL;
		if (!blockapitrace && strstr(check, "%trace_on"))
		{
			traceTestPatternBuffer = traceBase = tracebuffer;
			trace = (unsigned int)-1;
			userLog = FILE_LOG;
		}

		// preprocess output request so all variables convert japanese . - _  space to ascii
		Output(check, buffer, result, 0);
		if (traceTestPatternBuffer) trace = 0;
		WORDP D = FindWord("cs_info");
		if (D && D->w.userValue)
		{
			strcat(buffer, " ");
			strcat(buffer, D->w.userValue);
		}
	}
	--jumpIndex;

	if (*debugdata) ExecuteDebug(resultobject);

	// variables set?
	HandleChangedVariables(resultobject, 0, false);

	// store the result text
	if (result == NOPROBLEM_BIT)
	{
		char* zbuffer = AllocateBuffer(); // localized infinite allocation
		char* at = DoOutputAdjustments(buffer, responseControl, zbuffer, zbuffer + maxBufferSize);

		for (int i = 0; i < responseIndex; ++i)
		{
			strcat(zbuffer, responseData[responseOrder[i]].response);
			strcat(zbuffer, " ");
		}

		MEANING match = MakeMeaning(StoreWord("output", AS_IS));
		if (!stricmp(current_language, "JAPANESE") || !stricmp(current_language, "chinese")) // no spaces except in quoted
		{
			char* at = zbuffer - 1;
			bool dq = false;
			while (*++at)
			{
				if (*at == '"') dq = !dq;
				if (*at == ' ' && !dq)
				{
					memmove(at, at + 1, strlen(at));
				}
			}
		}
		if (!*zbuffer) CreateFact(resultobject, match, MakeMeaning(StoreWord("\"\"", AS_IS)), JSON_OBJECT_FACT | FACTTRANSIENT | JSON_STRING_VALUE);
		else CreateFact(resultobject, match, MakeMeaning(StoreWord(zbuffer, AS_IS)), JSON_OBJECT_FACT | FACTTRANSIENT | JSON_STRING_VALUE);
		
		if (trace & TRACE_JSON)
		{
			NextInferMark();
			WORDP D = Meaning2Word(resultobject);
			PushOutputBuffers();
			currentOutputBase = AllocateBuffer();
			currentOutputLimit = maxBufferSize;
			jwritehierarchy(false, false, 2, currentOutputBase, D,JSON_OBJECT_VALUE, 100); // nest of 0 (unspecified) is infinitiy
			Log(ECHOUSERLOG, "\r\nTestOutput Result Object:\r\n%s\r\n\r\n", currentOutputBase);
			PopOutputBuffers();
			FreeBuffer();
		}
		UndoAPICall(changedIncomingVariables, conceptlist, newfact, oldfact, substitutesList); // kill these facts

		FreeBuffer();
	}
	else // store failure
	{
		if (trace & TRACE_OUTPUT) Log(USERLOG, "Code failed execution ");
		if (*testoutputFail) sprintf(errors[errorIndex++], "code execution failed with %s", testoutputFail);
		else sprintf(errors[errorIndex++], "code execution failed with resultcode: %d %s", result, testoutputbacktrace);
	}
	responseIndex = 0;
	StoreCompileErrors(resultobject); // actually execution errors we reported as compile errors

	if (traceBase)
	{
		traceTestPatternBuffer = NULL;
		CreateFact(resultobject, MakeMeaning(StoreWord("trace")),
			MakeMeaning(StoreWord(traceBase, AS_IS)), JSON_OBJECT_FACT | FACTTRANSIENT | JSON_STRING_VALUE);
		traceBase = NULL;
	}
	sprintf(buffer, "%s", Meaning2Word(resultobject)->word);

	RESTORESYSTEMSTATE()
	currentFact = NULL; // dont pass up changes in facts
	trace = oldtrace;
	userLog = olduserlog;
	csapicall = NO_API_CALL;

	return NOPROBLEM_BIT;
}

static void DSEReturnCode(char* startData, char* data, MEANING M, bool pattern)
{
	// store the compiled code
	unsigned int flags = JSON_OBJECT_FACT | FACTTRANSIENT | JSON_STRING_VALUE;
	WORDP field = NULL;
	if (data != startData && !errorIndex) // some code was generated safely
	{
		char* hold = AllocateBuffer();
		char* ptr = hold;
		*hold = 0;
		if (!pattern) {}
		else if (patternwordthread)
		{
			*ptr++ = '|';
			HEAPREF list = patternwordthread;
			while (list)
			{
				uint64 D;
				list = UnpackHeapval(list, D, discard, discard);
				sprintf(ptr, "%s ", ((WORDP)D)->word);
				ptr += strlen(ptr);
			}
			*ptr++ = '|'; // close marker
			*ptr = 0;
		}
		else
		{
			strcpy(ptr, "^%s"); // BUG?
			++ptr;
		}
		size_t len = strlen(startData);
		size_t len1 = data - startData;
		strcpy(ptr, startData);

		WORDP CODE = StoreWord(hold, AS_IS);
		field = StoreWord("code", AS_IS);
		CreateFact(M, MakeMeaning(field), MakeMeaning(CODE), flags);
		FreeBuffer();
	}
	// store any errors
	StoreCompileErrors(M);

	// store any errors
	if (warnIndex)
	{
		MEANING M1 = GetUniqueJsonComposite((char*)"ja-", FACTTRANSIENT);
		field = StoreWord("warnings", AS_IS);
		flags = JSON_OBJECT_FACT | FACTTRANSIENT | JSON_ARRAY_VALUE;
		CreateFact(M, MakeMeaning(field), M1, flags);
		flags = JSON_ARRAY_FACT | FACTTRANSIENT | JSON_STRING_VALUE;
		for (unsigned int i = 0; i < warnIndex; ++i)
		{
			char index[MAX_WORD_SIZE];
			sprintf(index, "%u", i);
			field = StoreWord(index, AS_IS);
			char* warnmsg = warnings[i];
			while (*warnmsg == '.' || *warnmsg == ',') ++warnmsg; // skip tabbing data
			size_t len = strlen(warnmsg);
			while (warnmsg[len - 1] == '\n' || warnmsg[len - 1] == '\r') warnmsg[--len] = 0;
			WORDP warn = StoreWord(warnmsg, AS_IS);
			CreateFact(M1, MakeMeaning(field), MakeMeaning(warn), flags);
		}
	}
}

static FunctionResult CompilePatternCode(char* buffer)
{
	char* data = AllocateBuffer();
	*data = 0;
	char* startData = data;
	heapPatternThread = NULL;
	SAVESYSTEMSTATE()

		FunctionResult result = NOPROBLEM_BIT;
#ifndef DISCARDSCRIPTCOMPILER
	char* arg = ARGUMENT(1); // text string to compile
	arg = SkipWhitespace(arg);
	if (IsValidJSONName(arg, 'o')) // passing as "pattern" and language
	{
		WORDP D = FindWord(arg);
		if (!D) return FAILRULE_BIT;
		FACT* F = GetSubjectHead(D);
		while (F)
		{
			WORDP field = Meaning2Word(F->verb);
			if (!stricmp(field->word, "pattern")) arg = Meaning2Word(F->object)->word;
			else if (!stricmp(field->word, "language")) SetLanguage(Meaning2Word(F->object)->word);
			F = GetSubjectNext(F);
		}
	}

	if (*arg == '"') // remove quotes around the whole thing
	{
		++arg;
		size_t len = strlen(arg);
		if (arg[len - 1] == '"') arg[len - 1] = 0;
	}
	arg = SkipWhitespace(arg);
	unsigned int initiallen = (unsigned int)strlen(arg);
	if (initiallen > (maxBufferSize - 1000)) // pattern too big
	{
		csapicall = NO_API_CALL;
		RESTORESYSTEMSTATE()

			MEANING M1 = GetUniqueJsonComposite((char*)"ja-", FACTTRANSIENT);
		WORDP field = StoreWord("errors", AS_IS);
		unsigned int flags = JSON_OBJECT_FACT | FACTTRANSIENT | JSON_ARRAY_VALUE;
		MEANING M = GetUniqueJsonComposite((char*)"jo-", FACTTRANSIENT);
		CreateFact(M, MakeMeaning(field), M1, flags);
		flags = JSON_ARRAY_FACT | FACTTRANSIENT | JSON_STRING_VALUE;
		char index[MAX_WORD_SIZE];
		sprintf(index, "%d", 1);
		field = StoreWord(index, AS_IS);
		char errmsg[MAX_WORD_SIZE];
		char mydata[100];
		strncpy(mydata, arg, 90);
		mydata[90] = 0;
		sprintf(errmsg, "Pattern data too big, break it up. %u > %u ", initiallen, maxBufferSize);
		size_t len = strlen(errmsg);
		WORDP err = StoreWord(errmsg, AS_IS);
		CreateFact(M1, MakeMeaning(field), MakeMeaning(err), flags);
		DSEReturnCode(startData, data, M, true);
		WORDP D = Meaning2Word(M);
		sprintf(buffer, "%s", D->word);
		FreeBuffer(); // data
		patternwordthread = NULL;
		currentFact = NULL; // we have swallowed all facts
		return NOPROBLEM_BIT;
	}

	// if test or pattern test
	if (*arg != '(' && strncmp(arg, "if", 2) && strncmp(arg, "^if", 3))
	{
		MEANING M1 = GetUniqueJsonComposite((char*)"ja-", FACTTRANSIENT);
		WORDP field = StoreWord("errors", AS_IS);
		unsigned int flags = JSON_OBJECT_FACT | FACTTRANSIENT | JSON_ARRAY_VALUE;
		MEANING M = GetUniqueJsonComposite((char*)"jo-", FACTTRANSIENT);
		CreateFact(M, MakeMeaning(field), M1, flags);
		flags = JSON_ARRAY_FACT | FACTTRANSIENT | JSON_STRING_VALUE;
		char index[MAX_WORD_SIZE];
		sprintf(index, "%d", 1);
		field = StoreWord(index, AS_IS);
		char errmsg[MAX_WORD_SIZE];
		char mydata[100];
		strncpy(mydata, arg, 90);
		mydata[90] = 0;
		sprintf(errmsg, "Pattern missing opening paren --> %s", mydata);
		size_t len = strlen(errmsg);
		WORDP err = StoreWord(errmsg, AS_IS);
		CreateFact(M1, MakeMeaning(field), MakeMeaning(err), flags);
		DSEReturnCode(startData, data, M, true);
		WORDP D = Meaning2Word(M);
		sprintf(buffer, "%s", D->word);
		FreeBuffer(); // data
		patternwordthread = NULL;
		currentFact = NULL; // we have swallowed all facts
		return NOPROBLEM_BIT;
	}
	csapicall = COMPILE_PATTERN;
	bool oldmixed = nomixedcase;
	nomixedcase = true;

	//  =>  {code: xxx, errors: [], warnings: [] }

	// try to recover from fatality
	int buffercount = bufferIndex;
	int frameindex = globalDepth;

	if (setjmp(scriptJump[++jumpIndex])) // return on script compiler error
	{
		bufferIndex = buffercount;
		*buffer = 0;
	}
	else
	{
		patternwordthread = NULL;
		StartScriptCompiler(false);
		ReadNextSystemToken(NULL, NULL, data, false, false); // flush cache
		strcpy(readBuffer, arg);
		linestartpoint = readBuffer;
		compiling = PIECE_COMPILE;
		maxFileLine = currentFileLine = 0;
		supplementalColumn = currentLineColumn = 0;
#ifndef DISCARDSCRIPTCOMPILER
		if (*readBuffer == '(') // pattern test
		{
			*currentFilename = 0;
			supplementalColumn = currentLineColumn = 0;

			// convert japanese space and some punctuation to english for cs script processing
			if (!stricmp(current_language, "japanese") || !stricmp(current_language, "chinese")) ConvertJapanText(readBuffer, true);
			ReadPattern(readBuffer, NULL, data, false, false); // swallows the pattern
		}
		else // if test
		{
			strcpy(currentFilename, "^CompileIf");
			char word[MAX_WORD_SIZE];
			char* ptr1 = ReadCompiledWord(readBuffer, word);
			strcpy(word, "^if");
			strcat(ptr1, " { 1 }");
			ReadIf(word, ptr1, NULL, data, NULL);
		}
#endif
	}
	--jumpIndex;
	compiling = NOT_COMPILING;
	RESTORESYSTEMSTATE()
		EndScriptCompiler();
	// get the object
	MEANING M = GetUniqueJsonComposite((char*)"jo-", FACTTRANSIENT);
	char* at = startData - 1;
	while ((at = strstr(++at, "\\n")))
	{
		*at = ' ';
		at[1] = ' ';
	}

	DSEReturnCode(startData, data, M, true);
	UnbindBeenHere();
	WORDP D = Meaning2Word(M);
	sprintf(buffer, "%s", D->word);
	size_t len = strlen(startData);
	FreeBuffer();
	patternwordthread = NULL;

	currentFact = NULL; // we have swallowed all facts
	nomixedcase = oldmixed;
	csapicall = NO_API_CALL;
#endif
	return result;
}

FunctionResult CompileOutputCode(char* buffer)
{
	char* data = AllocateBuffer();
	*data = 0;
	csapicall = COMPILE_OUTPUT;
	heapPatternThread = NULL;

	SAVESYSTEMSTATE()

		FunctionResult result = NOPROBLEM_BIT;
#ifndef DISCARDSCRIPTCOMPILER
	char* arg = ARGUMENT(1); // text string to compile
	arg = SkipWhitespace(arg);
	if (IsValidJSONName(arg, 'o')) // passing as "output" and language
	{
		WORDP D = FindWord(arg);
		if (!D) return FAILRULE_BIT;
		FACT* F = GetSubjectHead(D);
		while (F)
		{
			WORDP field = Meaning2Word(F->verb);
			if (!stricmp(field->word, "output")) arg = Meaning2Word(F->object)->word;
			else if (!stricmp(field->word, "language")) SetLanguage(Meaning2Word(F->object)->word);
			F = GetSubjectNext(F);
		}
	}
	arg = SkipWhitespace(arg);
	if (*arg == '"') // remove quotes around the whole thing
	{
		++arg;
		size_t len = strlen(arg);
		if (arg[len - 1] == '"') arg[len - 1] = 0;
	}
	arg = SkipWhitespace(arg);

	//  =>  {code: xxx, errors: [], warnings: [] }
	char* startData = data;
	int buffercount = bufferIndex;
	int frameindex = globalDepth;
	if (setjmp(scriptJump[++jumpIndex])) // return on script compiler error
	{
		globalDepth = frameindex;
		bufferIndex = buffercount;
		*buffer = 0;
	}
	else
	{
		patternwordthread = NULL;
		StartScriptCompiler(false);
		ReadNextSystemToken(NULL, NULL, data, false, false); // flush cache
		*data = 0;
		strcpy(readBuffer, arg);

		// protect against \n code
		//char* at = readBuffer - 1;
		//while ((at = strstr(++at, "\\n")))
		//{
		//	*at = ' ';
		//	at[1] = ' ';
		//}

		linestartpoint = readBuffer;
		compiling = PIECE_COMPILE;
		strcpy(currentFilename, "^CompileOutput");
		maxFileLine = currentFileLine = 0;
		supplementalColumn = currentLineColumn = 0;
#ifndef DISCARDSCRIPTCOMPILER
		char rejoinders[256];	//   legal levels a: thru q:
		memset(rejoinders, 0, sizeof(rejoinders));

		if (!stricmp(current_language, "japanese") || !stricmp(current_language, "chinese")) ConvertJapanText(readBuffer, false);
		ReadOutput(false, false, readBuffer, NULL, data, rejoinders, NULL, NULL, false);
#endif
	}
	--jumpIndex;
	compiling = NOT_COMPILING;
	EndScriptCompiler();
	RESTORESYSTEMSTATE()
		// get the object
		MEANING M = GetUniqueJsonComposite((char*)"jo-", FACTTRANSIENT);
	char* at = startData;
	while ((at = strstr(at, "\\n")))
	{
		*at = ' ';
		at[1] = ' ';
	}
	DSEReturnCode(startData, data, M, false);
	UnbindBeenHere();

	WORDP D = Meaning2Word(M);
	sprintf(buffer, "%s", D->word);
	FreeBuffer();

	currentFact = NULL; // we have swallowed all facts

#endif
	csapicall = NO_API_CALL;
	return result;
}

static FunctionResult DeleteCode(char* buffer) //   delete all facts in collection or named fact or named json object/array
{
	char* arg1 = ARGUMENT(1);
	if (!*arg1) return NOPROBLEM_BIT;

	if (!stricmp(ARGUMENT(2), "BOOT")) allowBootKill = true;
	if (IsDigit(*arg1))
	{
		FACT* F = Index2Fact(atoi(arg1));
		if (F) KillFact(F);
	}
	else if (IsValidJSONName(arg1))
	{
		FunctionResult result = JSONDeleteCode(buffer);
		allowBootKill = false;
		return result;
	}
	else if (*arg1 == '~') // delete all facts of concept
	{
		WORDP D = FindWord(arg1);
		FACT* F = GetSubjectHead(D);
		while (F) { KillFact(F); F = GetSubjectNext(F); }
		F = GetVerbHead(D);
		while (F) { KillFact(F); F = GetVerbNext(F); }
		F = GetObjectHead(D);
		while (F) { KillFact(F); F = GetObjectNext(F); }
		return NOPROBLEM_BIT;
	}
	else if (*arg1 == '@') // factset
	{
		int store = GetSetID(arg1);
		if (store == ILLEGAL_FACTSET)
		{
			allowBootKill = false;
			return FAILRULE_BIT;
		}
		unsigned int count = FACTSET_COUNT(store);
		for (unsigned int i = 1; i <= count; ++i)
		{
			FACT* F = factSet[store][i];
			KillFact(F);
		}
	}
	else // deletefile
	{
		char word[MAX_WORD_SIZE];
		ReadCompiledWord(arg1, word);
		int fail = unlink(word);
		return (fail) ? FAILRULE_BIT : NOPROBLEM_BIT;
	}
	allowBootKill = false;

	return NOPROBLEM_BIT;
}

static FunctionResult FlushFactsCode(char* buffer) // delete all facts after this one (presuming sentence failed)
{
	if (planning) return FAILRULE_BIT; // dont allow this in planner

	unsigned int f = atoi(ARGUMENT(1));
	FACT* F = lastFactUsed;
	if (f > Fact2Index(F)) return FAILRULE_BIT;
	while (Fact2Index(F) > f)
	{
		F->flags |= FACTDEAD;	// kill it. dont have to do it recursive (KillFact) because everything that might be using this is already killed by this loop
		--F;
	}
	return NOPROBLEM_BIT;
}

static FunctionResult FieldCode(char* buffer)
{
	FACT* F;
	char* word = ARGUMENT(1);
	if (*word == '@') return FAILRULE_BIT;
	F = FactTextIndex2Fact(word);
	if (!F || F > lastFactUsed) return FAILRULE_BIT;

	char* limit;
	word = InfiniteStack(limit, "FieldCode");
	FunctionResult result = NOPROBLEM_BIT;
	WORDP xxs = Meaning2Word(F->subject); // for debugging
	WORDP xxv = Meaning2Word(F->verb);  // for debugging
	WORDP xxo = Meaning2Word(F->object);  // for debugging
	char* arg2 = ARGUMENT(2);
	if (*arg2 == 's' || *arg2 == 'S')
	{
		if (F->flags & FACTSUBJECT)
		{
			if (*arg2 == 's') sprintf(buffer, (char*)"%u", F->subject);
			else WriteFact(Index2Fact(F->subject), false, buffer, false, false);
		}
		else WriteMeaning(F->subject, false, buffer);
	}
	else if (*arg2 == 'v' || *arg2 == 'V')
	{
		if (F->flags & FACTVERB)
		{
			if (*arg2 == 'v') sprintf(buffer, (char*)"%u", F->verb);
			else WriteFact(Index2Fact(F->verb), false, buffer, false, false);
		}
		else WriteMeaning(F->verb, false, buffer);
	}
	else if (*arg2 == 'o' || *arg2 == 'O')
	{
		if (F->flags & FACTOBJECT)
		{
			if (*arg2 == 'o') sprintf(buffer, (char*)"%u", F->object);
			else WriteFact(Index2Fact(F->object), false, buffer, false, false);
		}
		else WriteMeaning(F->object, false, buffer);
	}
	else if (*arg2 == 'f' || *arg2 == 'F')
	{
		sprintf(buffer, (char*)"%u", F->flags);
	}
	else if (impliedWild == ALREADY_HANDLED)  result = FAILRULE_BIT;	// must spread them
	else if (*arg2 == 'a' || *arg2 == 'A') // all
	{
		SetWildCardIndexStart(impliedWild); //   start of wildcards to spawn
		if (F->flags & FACTSUBJECT)  sprintf(word, (char*)"%u", F->subject);
		else  strcpy(word, Meaning2Word(F->subject)->word);
		if (trace & TRACE_INFER && CheckTopicTrace()) Log(USERLOG, " _%d = %s ", impliedWild, word);
		SetWildCard(word, word, 0, 0);
		if (F->flags & FACTVERB)  sprintf(word, (char*)"%u", F->verb);
		else  strcpy(word, Meaning2Word(F->verb)->word);
		if (trace & TRACE_INFER && CheckTopicTrace()) Log(USERLOG, " _%d = %s ", impliedWild + 1, word);
		SetWildCard(word, word, 0, 0);
		if (F->flags & FACTOBJECT)  sprintf(word, (char*)"%u", F->object);
		else  strcpy(word, Meaning2Word(F->object)->word);
		if (trace & TRACE_INFER && CheckTopicTrace()) Log(USERLOG, " _%d = %s ", impliedWild + 2, word);
		SetWildCard(word, word, 0, 0);
		impliedWild = ALREADY_HANDLED;	//   we did the assignment
	}
	else if (*arg2 == 'r' || *arg2 == 'R') // all raw
	{
		SetWildCardIndexStart(impliedWild); //   start of wildcards to spawn
		if (F->flags & FACTSUBJECT)  sprintf(word, (char*)"%u", F->subject);
		else  WriteMeaning(F->subject, false, word);
		if (trace & TRACE_INFER && CheckTopicTrace()) Log(USERLOG, " _%d = %s ", impliedWild, word);
		SetWildCard(word, word, 0, 0);
		if (F->flags & FACTVERB)  sprintf(word, (char*)"%u", F->verb);
		else WriteMeaning(F->verb, false, word);
		if (trace & TRACE_INFER && CheckTopicTrace()) Log(USERLOG, " _%d = %s ", impliedWild + 1, word);
		SetWildCard(word, word, 0, 0);
		if (F->flags & FACTOBJECT)  sprintf(word, (char*)"%u", F->object);
		else  WriteMeaning(F->object, false, word);
		if (trace & TRACE_INFER && CheckTopicTrace()) Log(USERLOG, " _%d = %s ", impliedWild + 2, word);
		SetWildCard(word, word, 0, 0);
		impliedWild = ALREADY_HANDLED;	//   we did the assignment
	}
	else result = FAILRULE_BIT;
	ReleaseInfiniteStack();
	return result;
}

FunctionResult FindRuleCode1(char* buffer, char* word)
{
	for (unsigned int topicid = 1; topicid <= numberOfTopics; ++topicid)
	{
		char* tname = GetTopicName(topicid);
		if (!*tname) continue;
		char* rule = GetTopicData(topicid);
		int ruleID = 0;
		char label[MAX_WORD_SIZE];
		while (rule)
		{
			GetLabel(rule, label);
			if (!stricmp(label, word))
			{
				sprintf(buffer, "%s.%u.%u", tname, TOPLEVELID(ruleID), REJOINDERID(ruleID));
				return NOPROBLEM_BIT;
			}
			rule = FindNextRule(NEXTRULE, rule, ruleID);
		}

	}
	return FAILRULE_BIT;
}

static FunctionResult FindRuleCode(char* buffer)
{
	char word[MAX_WORD_SIZE];
	strcpy(word, ARGUMENT(1));
	return FindRuleCode1(buffer, word);
}

static FunctionResult FindCode(char* buffer) // given a set, find the ordered position of the 2nd argument in it 
{
	char word[MAX_WORD_SIZE];
	char* arg1 = ARGUMENT(1); // set
	char* arg2 = ARGUMENT(2); // item
	strcpy(word, JoinWords(BurstWord(arg2), false)); //  the value to find
	WORDP D = FindWord(arg1);
	if (*arg1 == '@')
	{
		FACT* F = Index2Fact(atoi(arg2));
		int set = GetSetID(arg1);
		if (set == ILLEGAL_FACTSET) return FAILRULE_BIT;
		unsigned int count = FACTSET_COUNT(set);
		for (unsigned int i = 1; i <= count; ++i)
		{
			if (F == factSet[set][i])
			{
				sprintf(buffer, (char*)"%u", i);
				return NOPROBLEM_BIT;
			}
		}
		return FAILRULE_BIT;
	}
	else if (D && *arg1 == '~')
	{
		WORDP D = FindWord(arg1);
		int n;
		if (!multidict) n = FindRecentMember(D, arg2);
		else n = FindEarliestMember(D, arg2);
		if (n >= 0)
		{
			sprintf(buffer, "%d", n);
			return NOPROBLEM_BIT;
		}
	}

	return FAILRULE_BIT;
}

static FunctionResult FindFactCode(char* buffer) // given a Simple fact
{
	char* arg1 = ARGUMENT(1);
	char* arg2 = ARGUMENT(2);
	char* arg3 = ARGUMENT(3);
	FACT* F = FindFact(ReadMeaning(arg1, false), ReadMeaning(arg2, false), ReadMeaning(arg3, false), 0);
	if (!F) return FAILRULE_BIT;
	sprintf(buffer, (char*)"%u", Fact2Index(F));
	return NOPROBLEM_BIT;
}

static FACT* FindF(MEANING subject, WORDP verb, uint64 marker)
{
	FACT* F = GetSubjectNondeadHead(subject);
	while (F)
	{
		WORDP v = Meaning2Word(F->verb);
		if (v == verb)
		{
			WORDP obj = Meaning2Word(F->object);
			if (marker != MARKED_WORD) // using a fact marking for find
			{
				if (F->flags & marker) return F;
			}
			else if (obj->systemFlags & marker) return F; // can use marked word flag as well
			FACT* G = FindF(F->object, verb, marker);
			if (G) return G;
		}
		F = GetSubjectNondeadNext(F);
	}

	return 0;
}

static FunctionResult FindMarkedFactCode(char* buffer)
{
	WORDP subject = FindWord(ARGUMENT(1));
	if (!subject) return FAILRULE_BIT;
	WORDP verb = FindWord(ARGUMENT(2));
	if (!verb) return FAILRULE_BIT;
	char* mark = ARGUMENT(3);
	int64 marker;
	if (IsDigit(*mark)) ReadInt64(mark, marker);
	else marker = FindPropertyValueByName(mark); // a fact marker like MARKED_FACT  or word systemflag marker like MARKED_WORD
	if (!marker) return FAILRULE_BIT;

	FACT* F = FindF(MakeMeaning(subject), verb, marker);
	if (trace & TRACE_INFER && CheckTopicTrace())
	{
		if (F)
		{
			Log(USERLOG, "FindMarkedFact found ");
			TraceFact(F);
		}
		else Log(USERLOG, "FindMarkedFact not found ");
	}
	if (!F) return FAILRULE_BIT;

	sprintf(buffer, (char*)"%u", Fact2Index(F)); // return index
	return NOPROBLEM_BIT;
}

static FunctionResult FLRCodeF(char* buffer)
{
	return FLR(buffer, (char*)"f");
}

static FunctionResult IntersectFactsCode(char* buffer)
{
	char* word = ARGUMENT(1);
	char from[MAX_WORD_SIZE];
	char to[MAX_WORD_SIZE];
	FunctionResult result;
	word = ReadShortCommandArg(word, from, result, OUTPUT_KEEPQUERYSET);
	word = ReadShortCommandArg(word, to, result, OUTPUT_KEEPQUERYSET);
	int store = (impliedSet == ALREADY_HANDLED) ? 0 : impliedSet;
	SET_FACTSET_COUNT(store, 0);

	WORDP D;
	FACT* F;
	unsigned int usedMark = NextInferMark();
	unsigned int i;
	char toKind = 's';
	int testSet = GetSetID(from);
	if (testSet == ILLEGAL_FACTSET) return FAILRULE_BIT;

	if (*to != '@') // mark word
	{
		D = FindWord(to);
		if (D) D->inferMark = usedMark;
	}
	else //  mark 2nd set word or pieces
	{
		int toset = GetSetID(to);
		if (toset == ILLEGAL_FACTSET) return FAILRULE_BIT;
		toKind = GetLowercaseData(*GetSetType(to)); // s v o null
		unsigned int limit = FACTSET_COUNT(toset);
		for (i = 1; i <= limit; ++i)
		{
			WORDP X;
			F = factSet[toset][i];
			if (!F) continue;
			if (trace & TRACE_INFER && CheckTopicTrace())   TraceFact(F);
			if (toKind == 's') Meaning2Word(F->subject)->inferMark = usedMark;
			else if (toKind == 'v') Meaning2Word(F->verb)->inferMark = usedMark;
			else if (toKind == 'o') Meaning2Word(F->object)->inferMark = usedMark;
			else // mark all pieces
			{
				X = Meaning2Word(F->subject);
				X->inferMark = usedMark;
				X = Meaning2Word(F->verb);
				X->inferMark = usedMark;
				X = Meaning2Word(F->object);
				X->inferMark = usedMark;
				F->flags |= MARKED_FACT;
			}
		}
	}

	// look for all matches of second set
	char fromKind = GetLowercaseData(*GetSetType(from)); // s v o null
	unsigned int limit = FACTSET_COUNT(testSet);
	if (trace & TRACE_INFER) Log(USERLOG, " // ");
	for (i = 1; i <= limit; ++i)
	{
		F = factSet[testSet][i];
		if (!F) continue;
		if (trace & TRACE_INFER && CheckTopicTrace())   TraceFact(F);
		if (fromKind == 's')
		{
			if (Meaning2Word(F->subject)->inferMark == usedMark) AddFact(store, F);
		}
		else if (fromKind == 'v')
		{
			if (Meaning2Word(F->verb)->inferMark == usedMark) AddFact(store, F);
		}
		else if (fromKind == 'o')
		{
			if (Meaning2Word(F->object)->inferMark == usedMark) AddFact(store, F);
		}
		else
		{
			// exact same fact, or a duplicate fact with the same SVO found
			if (toKind != 's' && toKind != 'v' && toKind != 'o')
			{
				if (F->flags & MARKED_FACT ||
					(Meaning2Word(F->subject)->inferMark == usedMark && Meaning2Word(F->verb)->inferMark == usedMark && Meaning2Word(F->object)->inferMark == usedMark)) AddFact(store, F);
			}
			// some piece found
			else if (Meaning2Word(F->subject)->inferMark == usedMark || Meaning2Word(F->verb)->inferMark == usedMark || Meaning2Word(F->object)->inferMark == usedMark) AddFact(store, F);
		}
	}
	unsigned int count = FACTSET_COUNT(store);
	if (trace & TRACE_INFER && CheckTopicTrace())
	{
		Log(USERLOG, "Found %d in IntersectFact\r\n", count);
		for (i = 1; i <= count; ++i) TraceFact(factSet[store][i]);
	}
	if (impliedSet == ALREADY_HANDLED && !count) return FAILRULE_BIT;
	impliedSet = ALREADY_HANDLED;
	return NOPROBLEM_BIT;
}

static FunctionResult UniqueFactsCode(char* buffer)
{
	char* word = ARGUMENT(1);
	char from[MAX_WORD_SIZE];
	char to[MAX_WORD_SIZE];
	FunctionResult result;
	word = ReadShortCommandArg(word, from, result, OUTPUT_KEEPQUERYSET);
	word = ReadShortCommandArg(word, to, result, OUTPUT_KEEPQUERYSET);
	int store = (impliedSet == ALREADY_HANDLED) ? 0 : impliedSet;
	SET_FACTSET_COUNT(store, 0);

	WORDP D;
	FACT* F;
	unsigned int usedMark = NextInferMark();
	unsigned int i;
	char toKind = 's';
	int where = GetSetID(from);
	if (where == ILLEGAL_FACTSET) return FAILRULE_BIT;

	if (*to != '@') // mark word
	{
		D = FindWord(to);
		if (D) D->inferMark = usedMark;
	}
	else //  mark set
	{
		int toset = GetSetID(to);
		if (toset == ILLEGAL_FACTSET) return FAILRULE_BIT;
		toKind = GetLowercaseData(*GetSetType(to)); // s v o null
		unsigned int limit = FACTSET_COUNT(toset);
		for (i = 1; i <= limit; ++i)
		{
			F = factSet[toset][i];
			if (!F) continue;
			if (trace & TRACE_INFER && CheckTopicTrace())   TraceFact(F);
			if (toKind == 's') Meaning2Word(F->subject)->inferMark = usedMark;
			else if (toKind == 'v') Meaning2Word(F->verb)->inferMark = usedMark;
			else if (toKind == 'o') Meaning2Word(F->object)->inferMark = usedMark;
			else // mark all pieces
			{
				D = Meaning2Word(F->subject);
				D->inferMark = usedMark;
				D = Meaning2Word(F->verb);
				D->inferMark = usedMark;
				D = Meaning2Word(F->object);
				D->inferMark = usedMark;
				F->flags |= MARKED_FACT;
			}
		}
	}

	// look for non matches
	char fromKind = GetLowercaseData(*GetSetType(from)); // s v o null
	unsigned int limit = FACTSET_COUNT(where);
	if (trace & TRACE_INFER && CheckTopicTrace()) Log(USERLOG, " // ");
	for (i = 1; i <= limit; ++i)
	{
		F = factSet[where][i];
		if (!F) continue;
		if (trace & TRACE_INFER && CheckTopicTrace())   TraceFact(F);
		if (fromKind == 's' && Meaning2Word(F->subject)->inferMark != usedMark) AddFact(store, F);
		else if (fromKind == 'v' && Meaning2Word(F->verb)->inferMark != usedMark) AddFact(store, F);
		else if (fromKind == 'o' && Meaning2Word(F->object)->inferMark != usedMark) AddFact(store, F);
		else
		{
			// entire fact not found
			if (toKind != 's' && toKind != 'v' && toKind != 'o' && !(F->flags & MARKED_FACT)) AddFact(store, F);
			// some piece found
			else if (Meaning2Word(F->subject)->inferMark != usedMark && Meaning2Word(F->verb)->inferMark != usedMark && Meaning2Word(F->object)->inferMark != usedMark) AddFact(store, F);
		}
	}
	unsigned int count = FACTSET_COUNT(store);
	if (trace & TRACE_INFER && CheckTopicTrace())
	{
		Log(USERLOG, "Found %d in UniqueFacts\r\n", count);
		for (i = 1; i <= count; ++i) TraceFact(factSet[store][i]);
	}
	if (impliedSet == ALREADY_HANDLED && !count) return FAILRULE_BIT;
	impliedSet = ALREADY_HANDLED;
	return NOPROBLEM_BIT;
}

static FunctionResult IteratorCode(char* buffer)
{// ? is std iterator ?? is recursive
	char* arg1 = ARGUMENT(1);
	char* arg2 = ARGUMENT(2);
	char* arg3 = ARGUMENT(3);
	WORDP verb = FindWord(arg2);
	if (!verb) return FAILRULE_BIT;
	MEANING v = MakeMeaning(verb);
	FACT* F;
	WORDP D;
	FACT* holdIterator = NULL;
	if (currentIterator) // this is a return to iteration- either a normal fact or a special fact containing both hieararcy and normal fact data
	{
		F = Index2Fact(currentIterator);
		if (F->flags & ITERATOR_FACT)
		{
			holdIterator = F;
			F = Index2Fact(F->object);
		}
		F = (*arg1 == '?') ? GetObjectNondeadNext(F) : GetSubjectNondeadNext(F);
	}
	else // this is a start of iteration
	{
		if (*arg1 == '?')
		{
			D = FindWord(arg3); // simple word, not meaning
			F = (D) ? GetObjectNondeadHead(D) : NULL;
		}
		else
		{
			D = FindWord(arg1); // simple word, not meaning
			F = (D) ? GetSubjectNondeadHead(D) : NULL;
		}
	}
retry: // level return if any
	while (F)
	{
		if (F->verb == v)
		{
			if (arg1[1] == '?' || arg3[1] == '?') // recursive on concepts?
			{
				MEANING field = (*arg1 == '?') ? F->subject : F->object;
				WORDP E = Meaning2Word(field);
				if (*E->word == '~') // going to nest within
				{
					FACT* G = SpecialFact(holdIterator ? (holdIterator->verb) : 0, Fact2Index(F), ITERATOR_FACT); // remember where we were
					F = (*arg1 == '?') ? GetObjectNondeadHead(E) : GetSubjectNondeadHead(E);
					if (!holdIterator) holdIterator = SpecialFact(Fact2Index(G), Fact2Index(F), ITERATOR_FACT); // we return this as holding current level and prior level tree
					else holdIterator->verb = Fact2Index(G);
					continue;	// resume hunting at lower level
				}
			}
			break; // found one
		}
		F = (*arg1 == '?') ? GetObjectNondeadNext(F) : GetSubjectNondeadNext(F);
	}
	if (!F) // ran dry
	{
		if (holdIterator) // back out of recursive on concepts?
		{
			F = Index2Fact(holdIterator->verb);  // this is a special fact also
			if (!F) return FAILRULE_BIT;		// levels ran dry
			holdIterator->verb = F->verb;		// hold now points higher
			F = Index2Fact(F->object);			// where we were at the higher level
			F = (*arg1 == '?') ? GetObjectNondeadNext(F) : GetSubjectNondeadNext(F);
			goto retry;
		}
		return FAILRULE_BIT;
	}
	MEANING M = (*arg1 == '?') ? F->subject : F->object;
	WriteMeaning(M, false, buffer);
	if (!withinLoop && planning && !backtrackable) backtrackable = true;

	if (holdIterator)
	{
		holdIterator->object = Fact2Index(F); // alter we are pair of hierarchy return and current
		F = holdIterator;
	}
	currentIterator = Fact2Index(F);
	return NOPROBLEM_BIT;
}

static FunctionResult MakeRealCode(char* buffer)
{
	char* arg = ARGUMENT(1);
	if (*arg == '@')
	{
		int set = GetSetID(arg);
		if (set < 0) return FAILRULE_BIT;
		unsigned int count = FACTSET_COUNT(set);
		for (unsigned int i = 1; i <= count; ++i)
		{
			FACT* F = factSet[set][i];
			if (F->flags & FACTTRANSIENT) F->flags ^= FACTTRANSIENT;
		}
	}
	else
	{
		FACT* F = Index2Fact(atoi(arg));
		if (!F) return FAILRULE_BIT;
		FACT* at = lastFactUsed + 1;
		while (--at > F) // user facts
		{
			if (at->flags & FACTTRANSIENT) at->flags ^= FACTTRANSIENT;
		}
	}

	return NOPROBLEM_BIT;
}

static FunctionResult SetFactOwner(char* buffer)
{
	char* arg = ARGUMENT(1);
	int64 botid = atoi64(ARGUMENT(2));
	if (*arg == '@')
	{
		int set = GetSetID(arg);
		if (set < 0) return FAILRULE_BIT;
		unsigned int count = FACTSET_COUNT(set);
		for (unsigned int i = 1; i <= count; ++i)
		{
			FACT* F = factSet[set][i];
			F->botBits = botid;		// which bots can access this fact (up to 64)
		}
	}
	else // all new facts past here
	{
		FACT* F = Index2Fact(atoi(arg));
		if (!F) return FAILRULE_BIT;
		FACT* at = lastFactUsed + 1;
		while (--at > F) // user facts
		{
			F->botBits = botid;		// which bots can access this fact (up to 64)
		}
	}

	return NOPROBLEM_BIT;
}


static FunctionResult FLRCodeL(char* buffer)
{
	return FLR(buffer, (char*)"l");
}

extern int backtrackIndex;

static FunctionResult QueryCode(char* buffer)
{ //   kind, s, v, o, count,  from, to, propogate, mark 
	int count = 0;
	CALLFRAME* frame = GetCallFrame(globalDepth);
	char* ptr = ARGUMENT(1);
	int argcount = 0;
	while (ptr && *ptr) // break apart arguments, but leave any quoted arg UNEVALED.
	{
		argcount++;
		char word[MAX_WORD_SIZE];
		ptr = ReadCompiledWord(ptr, word);
		if (*word != '\'' || word[1] == '_') // quoted var or such but not quoted matchvar
		{
			FunctionResult result = NOPROBLEM_BIT;
			if (*word == '@') frame->arguments[argcount] = AllocateStack(word); // no eval of such
			else
			{
				GetCommandArg(word, buffer, result, 0);
				frame->arguments[argcount] = AllocateStack(buffer);
			}
			if (result != NOPROBLEM_BIT) return result;
		}
		else frame->arguments[argcount] = AllocateStack(word);
	}
	*buffer = 0; // clear off old arg data

	for (int i = argcount + 1; i <= 9; ++i) frame->arguments[i] = (char*)""; // default rest of args to ""
	if (IsDigit(ARGUMENT(5)[0])) ReadInt(ARGUMENT(5), count); // defaults to ? if not given
	if (count == 0) count = (unsigned int)-1; // infinite

	if (argcount < 9) while (++argcount <= 9)  frame->arguments[argcount] = (char*)"?"; //   default rest of calling Arguments
	char set[50];
	char* arg1 = ARGUMENT(1);
	char* subject = ARGUMENT(2);
	char* verb = ARGUMENT(3);
	char* object = ARGUMENT(4);
	char* from = ARGUMENT(6);
	char* to = ARGUMENT(7);
	char* arg8 = ARGUMENT(8);
	char* arg9 = ARGUMENT(9);

	if (impliedSet != ALREADY_HANDLED)
	{
		sprintf(set, (char*)"@%d", impliedSet);
		to = set;
	}

	char splitoff[MAX_WORD_SIZE];
	*splitoff = 0;
	char* at = to;
	if (*at == '@') // get any special assignment data
	{
		while (IsDigit(*++at));
		strcpy(splitoff, at);
	}
	uint64 start_time = ElapsedMilliseconds();

	count = Query(arg1, subject, verb, object, count, from, to, arg8, arg9);

	if (timing & TIME_QUERY && CheckTopicTime())
	{
		int diff = (int)(ElapsedMilliseconds() - start_time);
		if (timing & TIME_ALWAYS || diff > 0) Log(STDTIMELOG, (char*)"Query(%s %s %s %s) time: %d ms\r\n", arg1, subject, verb, object, diff);
	}

	// result was a count. now convert to a fail code
	FunctionResult result;
	if (impliedSet != ALREADY_HANDLED) result = NOPROBLEM_BIT;
	else if (*splitoff) result = NOPROBLEM_BIT;
	else result = (count != 0) ? NOPROBLEM_BIT : FAILRULE_BIT;
	impliedSet = ALREADY_HANDLED;

	// split off?
	if (*splitoff && count != 0)
	{
		int store = GetSetID(to);
		if (store == ILLEGAL_FACTSET) return FAILRULE_BIT;

		FACT* F = factSet[store][1];
		if (*splitoff == 'o' || *splitoff == 'O') sprintf(buffer, "%s", Meaning2Word(F->object)->word);
		else if (*splitoff == 'v' || *splitoff == 'V') sprintf(buffer, "%s", Meaning2Word(F->verb)->word);
		else if (*splitoff == 's' || *splitoff == 'S') sprintf(buffer, "%s", Meaning2Word(F->subject)->word);
		else if (*splitoff == 'f' || *splitoff == 'F') sprintf(buffer, "%u", Fact2Index(F));
		else if (*splitoff == 'a' || *splitoff == 'A') return FAILRULE_BIT;
	}
	return result;
}

static FunctionResult SortCode(char* buffer) // sorts low to high  sort(@factset @chainedfactset
{
	char* arg = ARGUMENT(1); // stream
	char word[MAX_WORD_SIZE];
	char word1[MAX_WORD_SIZE];
	unsigned int sortedSet = MAX_FIND_SETS;
	unsigned int workSet = MAX_FIND_SETS + 1;
	int alpha = 0;
	char* start = arg;
	arg = SkipWhitespace(ReadCompiledWord(arg, word));
	if (!stricmp(word, (char*)"alpha")) // optional alpha
	{
		alpha = 1;
		start = arg;
		arg = ReadCompiledWord(arg, word);
	}
	else if (!stricmp(word, (char*)"age")) // optional age
	{
		alpha = 2;
		start = arg;
		arg = ReadCompiledWord(arg, word);
	}
	if (*word != '@') return FAILRULE_BIT;
	int n = GetSetID(word);
	if (n == ILLEGAL_FACTSET) return FAILRULE_BIT;
	unsigned int count = FACTSET_COUNT(n);
	int startSet = n;
	if (count > 0x0000ffff) return FAILRULE_BIT;	// too many facts to count
	// if chained sets, number the facts of the original
	if (*arg == '@') // remaining sets
	{
		// verify they all have the same count
		char* at = arg;
		while (*at == '@') // sort the others to correspond
		{
			at = SkipWhitespace(ReadCompiledWord(at, word1));
			int a = GetSetID(word1);
			if (a == ILLEGAL_FACTSET) return FAILRULE_BIT;
			if (FACTSET_COUNT(a) != count) return FAILRULE_BIT;
		}
	}
	SET_FACTSET_COUNT(sortedSet, count);
	for (unsigned int i = 1; i <= count; ++i) // mark original set we sort on
	{
		FACT* F = factSet[n][i];
		if (F)
		{
			factFlags[i] = F->flags; // protect flags
			F->flags = i; // index of fact originally
			factSet[sortedSet][i] = F;
		}
		else return FAILRULE_BIT; // null value fact
	}

	SortFacts(word, alpha, sortedSet); // sort copy of the original
	bool once = false;
	while (*start == '@') // sort the others to correspond
	{
		start = SkipWhitespace(ReadCompiledWord(start, word));
		int a = GetSetID(word);
		memcpy(&factSet[workSet], &factSet[a], sizeof(FACT*) * (count + 1)); // duplicate it
		for (unsigned int i = 1; i <= count; ++i)
		{
			// if (!factSet[n][i]) continue;
			unsigned int index = factSet[sortedSet][i]->flags;	// the new index at this position
			factSet[a][i] = factSet[workSet][index];
			if (!once) factIndex[i] = index;
		}
		once = true;
	}
	for (unsigned int i = 1; i <= count; ++i)
	{
		FACT* F = factSet[startSet][i];
		F->flags = factFlags[factIndex[i]];
	}
	if (trace & TRACE_QUERY && CheckTopicTrace() && FACTSET_COUNT(startSet))
	{
		char* wordx = AllocateBuffer();
		Log(USERLOG, "  %d[1]: %s\r\n", startSet, WriteFact(factSet[startSet][1], false, wordx));
		FreeBuffer();
	}
	impliedSet = ALREADY_HANDLED;

	return NOPROBLEM_BIT;
}

static FunctionResult UnduplicateCode(char* buffer)
{
	if (impliedSet == ALREADY_HANDLED) return FAILRULE_BIT;

	int from = GetSetID(ARGUMENT(1));
	if (from == ILLEGAL_FACTSET) return FAILRULE_BIT;
	if (impliedSet == from) return FAILRULE_BIT; // cant do in-place
	int count = FACTSET_COUNT(from);
	SET_FACTSET_COUNT(impliedSet, 0);
	char type = 0;
	char* mod = GetSetType(ARGUMENT(1));
	if ((*mod == 's' || *mod == 'S')) type = 's';
	else if ((*mod == 'v' || *mod == 'V')) type = 'v';
	else if ((*mod == 'o' || *mod == 'O')) type = 'o';

	// copy unmarked facts to to
	int i;
	int newcount = 0;
	for (i = 1; i <= count; ++i)
	{
		FACT* F = factSet[from][i];
		if (!F) continue;
		if (type == 's' && Meaning2Word(F->subject)->internalBits & WORDNET_ID) continue;
		if (type == 'v' && Meaning2Word(F->verb)->internalBits & WORDNET_ID) continue;
		if (type == 'o' && Meaning2Word(F->object)->internalBits & WORDNET_ID) continue;
		if (!(F->flags & MARKED_FACT))
		{
			++newcount;
			AddFact(impliedSet, F);
		}
		F->flags |= MARKED_FACT;
		WORDP D = Meaning2Word(F->subject);
		D->internalBits |= WORDNET_ID;
		D = Meaning2Word(F->verb);
		D->internalBits |= WORDNET_ID;
		D = Meaning2Word(F->object);
		D->internalBits |= WORDNET_ID;
	}

	// erase marks
	SET_FACTSET_COUNT(impliedSet, newcount);
	for (i = 1; i <= newcount; ++i)
	{
		FACT* F = factSet[impliedSet][i];
		F->flags ^= MARKED_FACT;
		WORDP D = Meaning2Word(F->subject);
		D->internalBits &= -1 ^ WORDNET_ID;
		D = Meaning2Word(F->verb);
		D->internalBits &= -1 ^ WORDNET_ID;
		D = Meaning2Word(F->object);
		D->internalBits &= -1 ^ WORDNET_ID;
	}

	if (trace & TRACE_QUERY && CheckTopicTrace())
	{
		char* wordx = AllocateBuffer();
		if (FACTSET_COUNT(impliedSet)) Log(USERLOG, "  %d[1]: %s\r\n", impliedSet, WriteFact(factSet[impliedSet][1], false, wordx));
		FreeBuffer();
	}
	impliedSet = ALREADY_HANDLED;
	return NOPROBLEM_BIT;
}

static FunctionResult UnpackFactRefCode(char* buffer)
{
	if (impliedSet == ALREADY_HANDLED) return FAILRULE_BIT;
	char* arg1 = ARGUMENT(1);
	int from = GetSetID(arg1);
	if (from == ILLEGAL_FACTSET) return FAILRULE_BIT;
	int count = FACTSET_COUNT(from);
	char* type = GetSetType(arg1);
	SET_FACTSET_COUNT(impliedSet, 0);
	FACT* G;
	for (int i = 1; i <= count; ++i)
	{
		FACT* F = factSet[from][i];
		if (!F) continue;
		if (F->flags & FACTSUBJECT && *type != 'v' && *type != 'o')
		{
			G = Index2Fact(F->subject);
			if (trace & TRACE_INFER && CheckTopicTrace()) TraceFact(G);
			AddFact(impliedSet, G);
		}
		if (F->flags & FACTVERB && *type != 's' && *type != 'o')
		{
			G = Index2Fact(F->verb);
			if (trace & TRACE_INFER && CheckTopicTrace()) TraceFact(G);
			AddFact(impliedSet, G);
		}
		if (F->flags & FACTOBJECT && *type != 's' && *type != 'v')
		{
			G = Index2Fact(F->object);
			if (trace & TRACE_INFER && CheckTopicTrace()) TraceFact(G);
			AddFact(impliedSet, G);
		}
	}
	impliedSet = ALREADY_HANDLED;
	currentFact = NULL;
	return NOPROBLEM_BIT;
}


static void JSONCounter(WORDP structure, unsigned int depth)
{
	if (!structure || !depth) return;
	FACT* F = GetSubjectNondeadHead(structure);
	while (F)
	{
		int d = depth;
		while (d--) printf("  "); // tab data
		if (structure->word[1] == 'a')
		{
		}
		else TraceFact(F);
		printf("\r\n");
		if (F->flags & JSON_ARRAY_VALUE)
		{
			JSONCounter(Meaning2Word(F->object), --depth);
		}
		else if (F->flags & JSON_OBJECT_VALUE)
		{
			JSONCounter(Meaning2Word(F->object), depth);
		}
		else
		{
			printf("%s", Meaning2Word(F->object)->word);
		}
		F = GetSubjectNondeadNext(F);
	}
}

static FunctionResult JsonCountCode(char* buffer)
{
	trace = (unsigned int)-1;
	echo = true;
	char* arg1 = ARGUMENT(1);
	unsigned int index = atoi(ARGUMENT(2));
	JSONCounter(FindWord(arg1), index);
	trace = 0;
	echo = false;
	return NOPROBLEM_BIT;
}

static FunctionResult WriteFactCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	unsigned int index = atoi(arg1);
	FACT* F = Index2Fact(index);
	if (!F) return FAILRULE_BIT;
	WriteFact(F, false, buffer, false, false);
	return NOPROBLEM_BIT;
}

static FunctionResult SerializeCode(char* buffer)
{
	int store = GetSetID(ARGUMENT(1));
	if (store == ILLEGAL_FACTSET) return FAILRULE_BIT;
	int n = FACTSET_COUNT(store);
	for (int i = 1; i <= n; ++i)
	{
		FACT* F = factSet[store][i];
		WriteFact(F, false, buffer);
		buffer += strlen(buffer);
		*buffer++ = ' ';
		*buffer = 0;
		unsigned int size = (buffer - currentOutputBase);
		if (size >= (currentOutputLimit - 400)) return FAILRULE_BIT; // too close
	}
	return NOPROBLEM_BIT;
}

static FunctionResult DeserializeCode(char* buffer)
{
	if (impliedSet == ALREADY_HANDLED) return FAILRULE_BIT;
	int store = impliedSet;
	if (store == ILLEGAL_FACTSET) return FAILRULE_BIT;
	SET_FACTSET_COUNT(store, 0);
	char* arg = ARGUMENT(1);
	int n = 0;
	while (arg && *arg)
	{
		arg = SkipWhitespace(arg);
		if (*arg++ != '(') return FAILRULE_BIT;
		arg = EatFact(arg, buffer);
		factSet[store][++n] = currentFact;
	}
	*buffer = 0;
	SET_FACTSET_COUNT(store, n);
	impliedSet = ALREADY_HANDLED;
	return NOPROBLEM_BIT;
}

#ifdef PRIVATE_CODE
#include "../privatecode/privatesrc.cpp"
#endif

SystemFunctionInfo systemFunctionSet[] =
{
	{ (char*)"",0,0,0,(char*)""},

	{ (char*)"\r\n---- Topic",0,0,0,(char*)""},
	{ (char*)"^addtopic",AddTopicCode,1,SAMELINE,(char*)"note a topic as interesting"}, //O
	{ (char*)"^available",AvailableCode,VARIABLE_ARG_COUNT,0,(char*)"is rule still available or has it been disabled"},
	{ (char*)"^cleartopics",ClearTopicsCode,0,SAMELINE,(char*)"remove all interesting topics in queue"},
	{ (char*)"^counttopic",CountTopicCode,2,SAMELINE,(char*)"provide topic and count requested: GAMBIT, AVAILABLE, RULE, USED"},
	{ (char*)"^gambit",GambitCode,VARIABLE_ARG_COUNT,0,(char*)"execute topic in gambit mode, naming ~ ~topicname PENDING or keyword or nothing"},
	{ (char*)"^getverify",GetVerifyCode,1,SAMELINE,(char*)""},
	{ (char*)"^getrule",GetRuleCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"get the requested data (TAG,TYPE,LABEL,PATTERN,OUTPUT,TOPIC,USABLE) for rule tag or label"},
	{ (char*)"^topicflags",TopicFlagsCode,1,SAMELINE,(char*)"Get topic control bits"},
	{ (char*)"^lastused",LastUsedCode,2,SAMELINE,(char*)"Get input count of last topic access - GAMBIT, RESPONDER, REJOINDER, ANY"},
	{ (char*)"^hasgambit",HasGambitCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"name of topic to test for an unexpired gambit, LAST/ANY/"},
	{ (char*)"^keep",KeepCode,0,SAMELINE,(char*)"do not erase rule after use"},
	{ (char*)"^poptopic",PopTopicCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"remove current or named topic from interesting set"},
	{ (char*)"^refine",RefineCode,VARIABLE_ARG_COUNT,0,(char*)"execute continuations until one matches"},
	{ (char*)"^rejoinder",RejoinderCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"try to match a pending rejoinder - not legal in postprocessing"},
	{ (char*)"^respond",RespondCode,VARIABLE_ARG_COUNT,0,(char*)"execute a topic's responders"},
	{ (char*)"^reuse",ReuseCode,VARIABLE_ARG_COUNT,0,(char*)"jump to a rule label or tag and execute output section"},
	{ (char*)"^sequence",SequenceCode,VARIABLE_ARG_COUNT,0,(char*)"execute continuations until one fails in output"},
	{ (char*)"^setrejoinder",SetRejoinderCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"Set rejoinder {INPUT OUTPUT,COPY} mark to this tag"},
	// These can transfer control to another topic: gambit, responder, rejoinder, reuse, refine

		{ (char*)"\r\n---- Topic Lists",0,0,0,(char*)""},
		{ (char*)"^gambittopics",GetTopicsWithGambitsCode,0,SAMELINE,(char*)"get all topics that have usable gambits that are not current topic"},
		{ (char*)"^keywordtopics",KeywordTopicsCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"get facts of topics that cover keywords mentioned in input"},
		{ (char*)"^pendingtopics",PendingTopicsCode,1,SAMELINE,(char*)"return list of currently pending topics as facts in 1st arg"},
		{ (char*)"^querytopics",QueryTopicsCode,1,SAMELINE,(char*)"get topics of which 1st arg is a keyword?"},
		{ (char*)"^walktopics",WalkTopicsCode,1,0,(char*)"call a function on every topic accessible to this bot" },
		{ (char*)"^walkvariables",WalkVariablesCode,1,0,(char*)"call a function on every global user variable" },

		{ (char*)"\r\n---- Marking & Parser Info",0,0,0,(char*)""},
		{ (char*)"^gettag",GetTagCode,1,SAMELINE,(char*)"for word n, return its postag concept" },
		{ (char*)"^mark",MarkCode,STREAM_ARG,SAMELINE,(char*)"mark word/concept in sentence"},
		{ (char*)"^replaceword",ReplaceWordCode,STREAM_ARG,SAMELINE,(char*)"assign word as original in sentence at _ var location" },
		{ (char*)"^addwordAt",AddWordAtCode,STREAM_ARG,SAMELINE,(char*)"Add word/canonical to sentence at _ var location" },
		{ (char*)"^marked",MarkedCode,1,SAMELINE,(char*)"BOOLEAN - is word/concept marked in sentence"},
		{ (char*)"^position",PositionCode,STREAM_ARG,SAMELINE,(char*)"get START or END position of an _ var"},
		{ (char*)"^setposition",SetPositionCode,STREAM_ARG,SAMELINE,(char*)"set absolute match position"},
		{ (char*)"^setpronoun",SetPronounCode,STREAM_ARG,SAMELINE,(char*)"replace pronoun with word"},
		{ (char*)"^unmark",UnmarkCode,STREAM_ARG,SAMELINE,(char*)"remove a mark on a word/concept in the sentence"},
		{ (char*)"^setcanon",SetCanonCode,2,SAMELINE,(char*)"for word n, set its canonical value"},
		{ (char*)"^settag",SetTagCode,2,SAMELINE,(char*)"for word n, set its postag concept"},
		{ (char*)"^setoriginal",SetOriginalCode,2,SAMELINE,(char*)"for word n, set its original value"},
		{ (char*)"^setrole",SetRoleCode,2,SAMELINE,(char*)"for word n, set its parse role value"},
		{ (char*)"^wordinfo",WordInfoCode,1,SAMELINE,(char*)"for word, give info about it"},

		{ (char*)"\r\n---- Input",0,0,0,(char*)""},
		{ (char*)"^tokenize",TokenizeCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"generate facts from sentences of arguments"},
		{ (char*)"^analyze",AnalyzeCode,STREAM_ARG,0,(char*)"Take an output stream and do preparation on it like it was user input"},
		{ (char*)"^capitalized",CapitalizedCode,1,SAMELINE,(char*)"given index of word in sentence return 1 or 0 for whether user capitalized it"},
		{ (char*)"^decodepos",DecodePosCode,2,SAMELINE,(char*)"decode into text the pos bits of given pos (POS) or role (ROLE) "},
		{ (char*)"^decodeinputtoken",DecodeInputTokenCode,1,SAMELINE,(char*)"Display flags of a cs_token or %token value "},
		{ (char*)"^input",InputCode,STREAM_ARG,0,(char*)"submit stream as input immediately after current input"},
		{ (char*)"^original",OriginalCode,STREAM_ARG,0,(char*)"retrieve raw user input corresponding to this match variable"},
		{ (char*)"^actualinputrange",ActualInputRangeCode,2,SAMELINE,(char*)"what range in actual input does this range in original input generate"},
		{ (char*)"^originalinputrange",OriginalInputRangeCode,2,SAMELINE,(char*)"what range in original input does this range in actual input come from"},
		{ (char*)"^partofspeech",PartOfSpeechCode,STREAM_ARG,SAMELINE,(char*)"given index of word in sentence return 64-bit pos data from parsing"},
		{ (char*)"^phrase",PhraseCode,STREAM_ARG,SAMELINE,(char*)"get noun or prep phrase at location, possibly canonical"},
		{ (char*)"^removetokenflags",RemoveTokenFlagsCode,1,SAMELINE,(char*)"remove value from tokenflags"},
		{ (char*)"^role",RoleCode,STREAM_ARG,SAMELINE,(char*)"given index of word in sentence return 32-bit role data from parsing"},
		{ (char*)"^settokenflags",SetTokenFlagsCode,1,SAMELINE,(char*)"add value to tokenflags"},
		{ (char*)"^setwildcardindex",SetWildcardIndexCode,STREAM_ARG,SAMELINE,(char*)"resume wildcard allocation at this number"},
		{ (char*)"^isnormalword",IsNormalWordCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"does word not have unusual characters, optional letteronly arg" },
		{ (char*)"^wordAtIndex",WordAtIndexCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"1st arg original canonical, Get word at index or possible range" },

		{ (char*)"\r\n---- Numbers",0,0,0,(char*)""},
		{ (char*)"^compute",ComputeCode,3,SAMELINE,(char*)"perform a numerical computation"},
		{ (char*)"^isnumber",IsNumberCode,1,SAMELINE,(char*)"is this an integer or double number or currency"},
		{ (char*)"^timefromseconds",TimeFromSecondsCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"given time/date in seconds, return the timeinfo string corresponding to it"},
		{ (char*)"^timeinfofromseconds",TimeInfoFromSecondsCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"given time/date in seconds and indicator of daylight savings or not, returning a sequence of 6 matchvariables (sec min hr date mo yr)"},
		{ (char*)"^timetoseconds",TimeToSecondsCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"given time/date a series of 6 values (sec min hr date mo yr), return the timeinfo string corresponding to it"},
		{ (char*)"^stats",StatsCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"given FACTS, tells how many facts are left, given TEXT tells how much heap is left, given DICT tells how many free entries left" },

		{ (char*)"\r\n---- Debugging",0,0,0,(char*)""},
		{ (char*)"^debug",DebugCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"only useful for debug code breakpoint"},
		{ (char*)"^identify",IdentifyCode,0,SAMELINE,(char*)"report CS version info"},
		{ (char*)"^log",LogCode,STREAM_ARG,0,(char*)"add to logfile"},
		{ (char*)"^reportbug",ReportBugCode,STREAM_ARG,0,(char*)"add to bugs.txt" },

		{ (char*)"\r\n---- Output Generation - not legal in post processing",0,0,0,(char*)""},
		{ (char*)"^flushoutput",FlushOutputCode,0,SAMELINE,(char*)"force existing output out"},
		{ (char*)"^format",FormatCode,3,SAMELINE,(char*)"sprintf equiv: int/integer/float/double formatstring value" },
		{ (char*)"^insertprint",InsertPrintCode,STREAM_ARG,0,(char*)"add output before named responseIndex"},
		{ (char*)"^keephistory",KeepHistoryCode,2,SAMELINE,(char*)"trim history of USER or BOT to number of entries given -- see also $cs_userhistorylimit"},
		{ (char*)"^print",PrintCode,STREAM_ARG,0,(char*)"isolated output message from current stream"},
		{ (char*)"^preprint",PrePrintCode,STREAM_ARG,0,(char*)"add output before existing output"},
		{ (char*)"^repeat",RepeatCode,0,SAMELINE,(char*)"set repeat flag so can repeat output"},
		{ (char*)"^reviseoutput",SetResponseCode,2,0,(char*)"Same as ^SetResponse, takes index and output, replacing output at that index"},

		{ (char*)"\r\n---- Output Access",0,0,0,(char*)""},
		{ (char*)"^lastsaid",LastSaidCode,0,SAMELINE,(char*)"get what chatbot said just before"},
		{ (char*)"^response",ResponseCode,1,SAMELINE,(char*)"raw text for this response, including punctuation"},
		{ (char*)"^responsepattern",ResponsePatternCode,1,SAMELINE,(char*)"[] pattern for this response" },
		{ (char*)"^setresponse",SetResponseCode,2,SAMELINE,(char*)"response id and raw text to replace with" },
		{ (char*)"^responsequestion",ResponseQuestionCode,1,SAMELINE,(char*)"BOOLEAN - 1 if response ends in ?  0 otherwise"},
		{ (char*)"^responseruleid",ResponseRuleIDCode,1,SAMELINE,(char*)"rule tag generating this response"},

		{ (char*)"\r\n---- Postprocessing functions - only available in postprocessing",0,0,0,(char*)""},
		{ (char*)"^postprintbefore",PostPrintBeforeCode,STREAM_ARG,0,(char*)"add to front of output stream"},
		{ (char*)"^postprintafter",PostPrintAfterCode,STREAM_ARG,0,(char*)"add to end of output stream"},

		{ (char*)"\r\n---- Control Flow",0,0,0,(char*)""},
		{ (char*)"^setlanguage", SetLanguageCode, 1, SAMELINE, (char*)"Change to designated language in multilanguage system" },
		{ (char*)"^authorized",AuthorizedCode,0,SAMELINE,(char*)"is current user authorized"},
		{ (char*)"^addcontext",AddContextCode,2,SAMELINE,(char*)"set topic and label as a context"},
		{ (char*)"^bug",BugCode,1,SAMELINE,(char*)"generate a script error during compilation of a table" },
		{ (char*)"^changebot",ChangeBotCode,2,SAMELINE,(char*)"become this bot: name, botid" },

		{ (char*)"^clearcontext",ClearContextCode,2,SAMELINE,(char*)"clear all context"},
		{ (char*)"^findrule",FindRuleCode,1,SAMELINE,(char*)"Given rule label, find rule anywhere in all topics" },
		{ (char*)"^argument",ArgumentCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"returns the calling scope's nth argument (given n and possible fn name)"},
		{ (char*)"^callstack",CallstackCode,1,SAMELINE,(char*)"return callstack facts in named factset"},
		{ (char*)"^clearmatch",ClearMatchCode,0,SAMELINE,(char*)"erase all match variables" },
		{ (char*)"^command",CommandCode,STREAM_ARG,0,(char*)"execute a : command"},
		{ (char*)"^end",EndCode,1,SAMELINE,(char*)"cease current processing thru this level"},
		{ (char*)"^eval",EvalCode,STREAM_ARG,0,(char*)"evaluate stream"},
		{ (char*)"^evaltwice",EvalCode,1,0,(char*)"evaluate stream after evaluation" },
		{ (char*)"^fail",FailCode,1,SAMELINE,(char*)"return a return code of some kind - allowed to erase facts on sentence fail"},
		{ (char*)"^incontext",InContextCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"returns normally if given label or topic.label have output recently else fails"},
		{ (char*)"^match",MatchCode,STREAM_ARG,0,(char*)"Perform given pattern match"},
		{ (char*)"^matches",MatchesCode,0,SAMELINE,(char*)"note the indexes of the words matched by the most recent pattern"},
		{ (char*)"^memorygc",MemoryGCCode,0,SAMELINE,(char*)"try to free up data spaces"},
		{ (char*)"^memoryfree",MemoryFreeCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"release dict,fact,text allocated since last memorymark, optional argument to pass thru" },
		{ (char*)"^memorymark",MemoryMarkCode,0,SAMELINE,(char*)"note memory information for later memory free"},
		{ (char*)"^norejoinder",NoRejoinderCode,0,SAMELINE,(char*)"block assigning rejoinder from this rule"},
		{ (char*)"^nofail",NoFailCode,STREAM_ARG,0,(char*)"execute script but ignore all failures thru some level"},
		{ (char*)"^notnull",NotNullCode,STREAM_ARG,0,(char*)"tests that output of stream argument is not null, fails otherwise"},
		{ (char*)"^result",ResultCode,STREAM_ARG,0,(char*)"executes the stream and returns the result code (never fails) "},
		{ (char*)"^retry",RetryCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"reexecute a rule with a later match or retry  input"},
		{ (char*)"^return",ReturnCode,STREAM_ARG,SAMELINE,(char*)"return this value from current user function call"},
		{ (char*)"^notrace",NoTraceCode,STREAM_ARG,0,(char*)"execute code with trace off (except for topics and functions)"},
		{ (char*)"^trace",TraceCode,STREAM_ARG,0,(char*)"execute code with trace on, even if within ^notrace" },
		{ (char*)"^savesentence",SaveSentenceCode,1,SAMELINE,(char*)"memorize current sentence analysis given label"},
		{ (char*)"^restoresentence",RestoreSentenceCode,1,SAMELINE,(char*)"recover prior saved sentence analysis given label"},
		{ (char*)"^sleep",SleepCode,1,SAMELINE,(char*)"wait n milliseconds"},
		{ (char*)"^if",IfCode,STREAM_ARG,0,(char*)"the if statement"},
		{ (char*)"^loop",LoopCode,STREAM_ARG,0,(char*)"the loop statement"},
		{ (char*)"^jsonloop",LoopCode,STREAM_ARG,0,(char*)"the jsonloop statement" },
		{ (char*)"^environment",EnvironmentCode,1,SAMELINE,(char*)"get os environment variable"},
		{ (char*)"^reboot",RebootCode,0,0,(char*)"viable only during ^cs_reboot, resets boot layer" },
		{ (char*)"^rulematches",RulematchesCode,1,SAMELINE,(char*)"On, Off, List - rules who went to output because pattern matched" },
		{ (char*)"\r\n---- External Databases",0,0,0,(char*)""},

		{ (char*)"\r\n---- Databases",0,0,0,(char*)"" },
	#ifndef DISCARDPOSTGRES
		{ (char*)"^dbinit",DBInitCode,STREAM_ARG,SAMELINE,(char*)"access a postgres database"},
		{ (char*)"^dbclose",DBCloseCode,0,SAMELINE,(char*)"close current postgres database"},
		{ (char*)"^dbexecute",DBExecuteCode,STREAM_ARG,SAMELINE,(char*)"perform postgres transactions"},
	#else
		{ (char*)"^dbinit",DummyCode,STREAM_ARG,SAMELINE,(char*)"access a postgres database - DUMMY" },
		{ (char*)"^dbclose",DummyCode,0,SAMELINE,(char*)"close current postgres database  - DUMMY" },
		{ (char*)"^dbexecute",DummyCode,STREAM_ARG,SAMELINE,(char*)"perform postgres transactions  - DUMMY" },
	#endif
	#ifndef DISCARDMYSQL
		{ (char*)"^mysqlinit",MySQLInitCode,1,SAMELINE,(char*)"access a mysql database" },
		{ (char*)"^mysqlclose",MySQLCloseCode,0,SAMELINE,(char*)"close current mysql database" },
		{ (char*)"^mysqlquery",MySQLQueryCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"perform mysql transactions" },
	#else
		{ (char*)"^mysqlinit",DummyCode,1,SAMELINE,(char*)"access a mysql database - DUMMY" },
		{ (char*)"^mysqlclose",DummyCode,0,SAMELINE,(char*)"close current mysql database - DUMMY" },
		{ (char*)"^mysqlquery",DummyCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"perform mysql transactions - DUMMY" },
	#endif
	#ifndef DISCARDMICROSOFTSQL
		{ (char*)"^mssqlinit",MsSqlScriptInitCode,1,SAMELINE,(char*)"init script db" },
		{ (char*)"^mssqlclose",MsSqlScriptCloseCode,0,SAMELINE,(char*)"close script db" },
		{ (char*)"^mssqlread",MsSqlScriptReadCode,1,SAMELINE,(char*)"entryname" },
		{ (char*)"^mssqlwrite",MsSqlScriptWriteCode,2,SAMELINE,(char*)"entryname & data" },
	#else
		{ (char*)"^mssqlinit",MsSqlScriptDummyInitCode,1,SAMELINE,(char*)"init script db - DUMMY" },
		{ (char*)"^mssqlclose",MsSqlScriptDummyCloseCode,0,SAMELINE,(char*)"close script db - DUMMY" },
		{ (char*)"^mssqlread",MsSqlScriptDummyReadCode,1,SAMELINE,(char*)"script db read - DUMMY" },
		{ (char*)"^mssqlwrite",MsSqlScriptDummyWriteCode,2,SAMELINE,(char*)"script db write - DUMMY" },
	#endif

	#ifndef DISCARDMONGO
		{ (char*)"^mongoinit",MongoInit,3,SAMELINE,(char*)"establish connection to mongo database"},
		{ (char*)"^mongoclose",MongoClose,0,SAMELINE,(char*)"destroy connection to mongo database"},
		{ (char*)"^mongoinsertdocument",mongoInsertDocument,2,SAMELINE,(char*)"mongo upsert document"},
		{ (char*)"^mongodeletedocument",mongoDeleteDocument,VARIABLE_ARG_COUNT,SAMELINE,(char*)"mongo delete document"},
		{ (char*)"^mongofinddocument",mongoFindDocument,VARIABLE_ARG_COUNT,SAMELINE,(char*)"mongo find document"},
	#else
		{ (char*)"^mongoinit",DummyCode,3,SAMELINE,(char*)"establish connection to mongo database - DUMMY" },
		{ (char*)"^mongoclose",DummyCode,0,SAMELINE,(char*)"destroy connection to mongo database - DUMMY" },
		{ (char*)"^mongoinsertdocument",DummyCode,2,SAMELINE,(char*)"mongo upsert document - DUMMY" },
		{ (char*)"^mongodeletedocument",DummyCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"mongo delete document - DUMMY" },
		{ (char*)"^mongofinddocument",DummyCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"mongo find document - DUMMY" },
	#endif

		{ (char*)"\r\n---- Word Manipulation",0,0,0,(char*)""},
		{ (char*)"^addproperty",AddPropertyCode,STREAM_ARG,SAMELINE,(char*)"Add value to dictionary entry properties or systemFlags or facts of factset properties"},
		{ (char*)"^burst",BurstCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"break a string into component words either to facts or matchvars - if 1st arg is count, returns number of words"},
		{ (char*)"^synset",SynsetCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"take word and make facts of its MEANINGs, or take MEANING and make facts of the related MEANINGS" },
		{ (char*)"^canon",CanonCode,2, SAMELINE, "Add word + canon to canon file while compiling a table"},
		{ (char*)"^define",DefineCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"get dictionary gloss of  word"},
		{ (char*)"^extract",ExtractCode,3,SAMELINE,(char*)"pull out text from given string starting at position and ending at position"},
		{ (char*)"^findtext",FindTextCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"locate start position in target of given string starting at offset"},
		{ (char*)"^flags",FlagsCode,1,SAMELINE,(char*)"get flag values of word"},
		{ (char*)"^hasanyproperty",HasAnyPropertyCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"argument 1 has any of property or systemflags of argument2 .. argumentn"},
		{ (char*)"^hasallproperty",HasAllPropertyCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"argument 1 has all of the properties or systemflags of argument2 .. argumentn"},
		{ (char*)"^uppercase",UppercaseCode,1,SAMELINE,(char*)"boolean return 1 if word was entered uppercase, 0 if not"},
		{ (char*)"^layer",LayerCode,1,SAMELINE,(char*)"get layer of where word showed up"},
		{ (char*)"^properties",PropertiesCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"get property values of word"},
		{ (char*)"^intersectwords",IntersectWordsCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"see if words in arg 1 are in arg2"},
		{ (char*)"^join",JoinCode,STREAM_ARG,SAMELINE,(char*)"merge words into one"},
		{ (char*)"^pos",POSCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"compute some part of speech value"},
		{ (char*)"^removeproperty",RemovePropertyCode,STREAM_ARG,SAMELINE,(char*)"remove value to dictionary entry properies or systemFlags or facts of factset properties"},

		{ (char*)"^recordDictionaryChanges",RecordDictionaryChangesCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"starts tracking changes to property and systemflags on words being modified" },
		{ (char*)"^revertDictionaryChanges",RevertDictionaryChangesCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"undoes those changes(turning recording off and if omitted, happens automatically at end of volley" },
		{ (char*)"^replayDictionaryChanges",ReplayDictionaryChangesCode,1,SAMELINE,(char*)" restores changes done by a record and later reverted(allowing you to insert changes not recorded)" },

		{ (char*)"^rhyme",RhymeCode,1,SAMELINE,(char*)"find a rhyming word"},
		{ (char*)"^substitute",SubstituteCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"alter a string by substitution"},
		{ (char*)"^spell",SpellCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"find words matching pattern and store as facts"},
		{ (char*)"^spellcheck",SpellCheckCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"Given tokenized sentence and JSON array of words, return adjusted sentence" },
		{ (char*)"^sexed",SexedCode,4,SAMELINE,(char*)"pick a word based on sex of given word"},
		{ (char*)"^tally",TallyCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"get or set a word value"},
		{ (char*)"^walkdictionary",WalkDictionaryCode,1,0,(char*)"call a function on every word in the dictionary"},
	#ifndef DISCARDCOUNTER
		{ (char*)"^wordcount",WordCountCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"get or set a word count value"},
	#endif
		{ (char*)"^words",WordsCode,1,SAMELINE,(char*)"given a word, get all words in dictionary with that spelling- case and space/_ independent"},
		{ (char*)"^wordinconcept",WordInConceptCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"given a word and a concept (and fail option), get the word in dictionary with that spelling- case and space/_ independent in that concept"},
		{ (char*)"\r\n---- MultiPurpose Functions",0,0,0,(char*)""},
		{ (char*)"^disable",DisableCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"stop a RULE or TOPIC or INPUTREJOINDER or OUTPUTREJOINDER or SAVE"},
		{ (char*)"^enable",EnableCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"allow a rule or topic"},
		{ (char*)"^length",LengthCode,1,SAMELINE,(char*)"counts characters in a word or members of a fact set or top level concept members or elements in json array or object"},
		{ (char*)"^next",NextCode,STREAM_ARG,SAMELINE,(char*)"FACT- walk a factset without erasing it  GAMBIT,RESPONDER,RULE,REJOINDER with tag or label for next one  INPUT to go to next sentence"},
		{ (char*)"^pick",FLRCodeR,STREAM_ARG,SAMELINE,(char*)"randomly select and remove an element from a factset or randomly select from a concept or factid from a JSON object or array"},
		{ (char*)"^reset",ResetCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"reset a topic or all topics or user or pending output back to initial state "},

		{ (char*)"\r\n---- Remote access",0,0,0,(char*)"" },
		{ (char*)"^compilepattern",CompilePatternCode,1,0,(char*)"compile a pattern from JSON data" },
		{ (char*)"^compileoutput",CompileOutputCode,1,0,(char*)"compile output from JSON data" },
		{ (char*)"^testpattern",TestPatternCode,1,0,(char*)"test a pattern from JSON data" },
		{ (char*)"^testoutput",TestOutputCode,1,0,(char*)"test output from JSON data" },

		{ (char*)"\r\n---- Functions on facts",0,0,0,(char*)""},
		{ (char*)"^allmembers",AllMembersCode,STREAM_ARG,SAMELINE,(char*)"return facts of all members direct or indirect of concept given- subject is word" },
		{ (char*)"^conceptlist",ConceptListCode,STREAM_ARG,SAMELINE,(char*)"create facts of the concepts or topics or both triggers by word at position or overall"},
		{ (char*)"^createattribute",CreateAttributeCode,STREAM_ARG,SAMELINE,(char*)"create a triple where the 3rd field is exclusive"},
		{ (char*)"^createfact",CreateFactCode,STREAM_ARG,SAMELINE,(char*)"create a triple"},
		{ (char*)"^delete",DeleteCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"delete all facts in factset or delete named fact or delete  file or delete named json object or array, optionally allow in boot layer as well"},
		{ (char*)"^purgeboot", PurgeBootCode, 1, SAMELINE, "erases data for a botid from boot layer" },
		{ (char*)"^deserialize", DeserializeCode, 1, SAMELINE, "transcribes a string into a factset" },
		{ (char*)"^field",FieldCode,2,SAMELINE,(char*)"get a field of a fact"},
		{ (char*)"^find",FindCode,2,SAMELINE,(char*)"Given set or factset, find ordinal position of item within it"},
		{ (char*)"^findfact",FindFactCode,3,SAMELINE,(char*)"given simple non-facts subject verb object, see if fact exists of it"},
		{ (char*)"^findmarkedfact",FindMarkedFactCode,3,SAMELINE,(char*)"given a subject,a verb, and a mark, return a marked fact that can be found propogating from subject using verb  or null"},
		{ (char*)"^first",FLRCodeF,STREAM_ARG,SAMELINE,(char*)"get first element of a factset and remove it"},
		{ (char*)"^flushfacts",FlushFactsCode,1,SAMELINE,(char*)"erase all facts created after here"},
		{ (char*)"^intersectfacts",IntersectFactsCode,STREAM_ARG,SAMELINE,(char*)"find facts common to two factsets, based on fields"},
		{ (char*)"^iterator",IteratorCode,3,SAMELINE,(char*)"walk facts of some thing"},
		{ (char*)"^makereal",MakeRealCode,1,SAMELINE,(char*)"make all transient facts non-transient either in factset or starting with factid to end"},
		{ (char*)"^setfactowner",SetFactOwner,2,SAMELINE,(char*)"change ownership of all facts either in factset or starting with factid to end" },
		{ (char*)"^nth",NthCode,STREAM_ARG,SAMELINE,(char*)"from factset get nth element (kept) or from set get nth element"},
		{ (char*)"^revisefact",ReviseFactCode,4,SAMELINE,(char*)"revise a triple"},
		{ (char*)"^uniquefacts",UniqueFactsCode,STREAM_ARG,SAMELINE,(char*)"find facts in first set not found in second"},
		{ (char*)"^last",FLRCodeL,STREAM_ARG,SAMELINE,(char*)"get last element of a factset and remove it"},
		{ (char*)"^query",QueryCode,STREAM_ARG,0,(char*)"hunt for fact in fact database"},
		{ (char*)"^serialize", SerializeCode, 1, SAMELINE, "transcribes a factset into a string" },
		{ (char*)"^sort",SortCode,STREAM_ARG,SAMELINE,(char*)"sort facts on named set-field (presumed number) low to high"},
		{ (char*)"^unduplicate",UnduplicateCode,STREAM_ARG,SAMELINE,(char*)"remove duplicate facts"},
		{ (char*)"^unpackfactref",UnpackFactRefCode,1,SAMELINE,(char*)"copy out fields which are facts"},
		{ (char*)"^writefact",WriteFactCode,1,SAMELINE,(char*)"take fact index and print out the fact suitable to be read again"},

		{ (char*)"\r\n---- External Access",0,0,0,(char*)""},
		{ (char*)"^export",ExportCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"write fact set to a file"},
		{ (char*)"^import",ImportCode,4,SAMELINE,(char*)"read fact set from a file"},
		{ (char*)"^system",SystemCode,STREAM_ARG,SAMELINE,(char*)"send command to the operating system"},
		{ (char*)"^popen",PopenCode,2,SAMELINE,(char*)"send command to the operating system and read reply strings"},
		{ (char*)"^tcpopen",TCPOpenCode,4,SAMELINE,(char*)"send command to website and read reply strings"},
		{ (char*)"^getremotefile",GetRemoteFileCode,VARIABLE_ARG_COUNT,SAMELINE,(char*)"retrieve data from external fileserver"},
	#ifndef DISCARDWEBSOCKET
		{ (char*)"^websocketinit",WebsocketInitCode,1,SAMELINE,(char*)"open a websocket" },
		{ (char*)"^websocketclose",WebsocketCloseCode,0,SAMELINE,(char*)"close current websocket" },
		{ (char*)"^websocketsend",WebsocketSendCode,1,SAMELINE,(char*)"send message via socket" },
		{ (char*)"^websocketreceive",WebsocketReceiveCode,1,SAMELINE,(char*)"get message via socket, waiting max argument time" },
	#endif

		{ "\r\n---- JSON Related", 0, 0, 0, "" },
		{"^jsoncount",JsonCountCode,2,SAMELINE,(char*)"show number of array elements in first arrays beyond depth given" },
		{ "^jsonreusekill",JsonReuseKillCode,1,SAMELINE,(char*)"delete json structure and make facts reusable" },
		{ "^jsontext", JSONTextCode, 1, SAMELINE, "given json fact id, outputs quotes around value of field if it needs it" },
		{ "^jsoncopy", JSONCopyCode, VARIABLE_ARG_COUNT, SAMELINE, "given json array or json object, creates a duplicate copy" },
		{ "^jsoncreate", JSONCreateCode, VARIABLE_ARG_COUNT, SAMELINE, "given array or object, creates a new one" },
		{ "^jsondelete", JSONDeleteCode, 1, SAMELINE, "deprecated in favor of ^delete" },
		{ "^jsongather", JSONGatherCode, VARIABLE_ARG_COUNT, SAMELINE, "stores the json facts referred to by the name into a fact set" },
		{ "^jsonkind", JSONKindCode, 1, SAMELINE, "returns object, array, or fails depending on what is passed to it" },
		{ "^jsonstorage", JSONStorageCode, 1, SAMELINE, "returns permanent, transient, boot, or fails depending on what is passed to it" },
		{ "^jsonarraysize", JSONArraySizeCode, 1, SAMELINE, "deprecated in favor of ^length" },
		{ "^jsonarrayinsert", JSONArrayInsertCode, VARIABLE_ARG_COUNT, SAMELINE, "given name of json array fact, adds given  value BEFORE or AFTER the given" },
		{ "^jsonarraydelete", JSONArrayDeleteCode, VARIABLE_ARG_COUNT, SAMELINE, "given name of json array and index of fact to remove, removes it and renumbers all after it down" },
		{ "^jsonobjectinsert", JSONObjectInsertCode, VARIABLE_ARG_COUNT, SAMELINE, "given name of json object, adds given key and value" },
		{ "^jsonparse", JSONParseCode, VARIABLE_ARG_COUNT, 0, "parses the provided string argument to a set of facts accessible from ChatScript code" },
		{ "^jsonparsefile", JSONParseFileCode, VARIABLE_ARG_COUNT, 0, "parses the provided filename to a set of facts accessible from ChatScript code" },
		{ "^jsonformat", JSONFormatCode, 1, SAMELINE, "given a json text string, makes all field names use doublequotes and proper escaping" },
		{ "^jsonpath", JSONPathCode, VARIABLE_ARG_COUNT, 0, "retrieves the json value corresponding to a path and a given fact presumed to be array or object" },
		{ "^jsontree", JSONTreeCode, VARIABLE_ARG_COUNT, 0, "prints the hierarchy represented by the json node to depth if requested" },
		{ "^jsonwrite", JSONWriteCode, VARIABLE_ARG_COUNT, 0, "prints out json string corresponding to the facts of the root name given, optional 1st arg PLAIN" },
		{ "^jsonlabel", JSONLabelCode, 1, SAMELINE, "use label when creating json arrays and objects" },
		{ "^jsonmerge", JSONMergeCode, VARIABLE_ARG_COUNT, SAMELINE, "given two json objects or two json arrays, merge into new one, where conflict, prefer first" },
		{ "^jsonundecodestring", JSONUndecodeStringCode, 1, SAMELINE, "remove escapes from json data" },
#ifndef DISCARDJSONOPEN
		{ "^jsonopen", JSONOpenCode, VARIABLE_ARG_COUNT, SAMELINE, "send command to JSON server and parse reply strings" },
#else
		{ "^jsonopen", DummyCode, VARIABLE_ARG_COUNT, SAMELINE, "send command to JSON server and parse reply strings" },
#endif
		{ "^readfile", JSONReadCSVCode, VARIABLE_ARG_COUNT, SAMELINE, "read tsv file into json array of objects or call function with spread args or call function with any line-oriented file" },
		{ "^jsonreadcsv", JSONReadCSVCode, VARIABLE_ARG_COUNT, SAMELINE, "deprecated name for ^readfile" },



	#ifdef PRIVATE_CODE
		#include "../privatecode/privatetable.cpp"
	#endif

		{0,0,0,0,(char*)""}
};
