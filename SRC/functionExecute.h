#ifndef _EXECUTE
#define _EXECUTE


#ifdef INFORMATION
Copyright (C)2011-2024 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif

#include "common.h"

#define MAX_DISPLAY 2500
#define LCLVARDATA_PREFIX '`'

#define FAILCODES ( FAILTOPRULE_BIT | RESTART_BIT | FAILINPUT_BIT | FAILTOPIC_BIT | FAILLOOP_BIT | FAILRULE_BIT | FAILSENTENCE_BIT | RETRYSENTENCE_BIT | RETRYTOPIC_BIT | UNDEFINED_FUNCTION | RETRYINPUT_BIT )
#define SUCCESSCODES ( ENDTOPRULE_BIT |  ENDINPUT_BIT | ENDSENTENCE_BIT | NEXTLOOP_BIT | ENDTOPIC_BIT | ENDRULE_BIT | ENDCALL_BIT | ENDLOOP_BIT)
#define ENDCODES ( FAILCODES | SUCCESSCODES )
#define RESULTBEYONDTOPIC ( RESTART_BIT | FAILSENTENCE_BIT | ENDSENTENCE_BIT | RETRYSENTENCE_BIT | ENDINPUT_BIT | FAILINPUT_BIT | RETRYINPUT_BIT )
#define RETRYCODES (RETRYRULE_BIT | RETRYTOPRULE_BIT | RETRYTOPIC_BIT | RETRYSENTENCE_BIT | RETRYINPUT_BIT)
typedef FunctionResult (*EXECUTEPTR)(char* buffer);

#define SAMELINE 2


typedef struct SystemFunctionInfo 
{
	const char* word;					//   dictionary word entry
	EXECUTEPTR fn;				//   function to use to get it
	int argumentCount;			//   how many callArgumentList it takes
	int	properties;				//   non-zero means does its own argument tracing
	const char* comment;
} SystemFunctionInfo;

//   special argeval codes
#define VARIABLE_ARG_COUNT -1	// evaled
#define STREAM_ARG -2
#define UNEVALED -3		// split but not evaled
#define NOEVAL_ARG1 0x00000100
#define NOEVAL_ARG2 0x00000200
#define NOEVAL_ARG3 0x00000400
#define NOEVAL_ARG4 0x00000800
#define NOEVAL_ARG5 0x00001000
// values 1-9 are normal argument count (evaled)
// bit 0x10 means args are split but NOT evaled, allowing receiver to eval individual ones on demand
// bits 0x100 and above are to specify specific args not to eval
			
// codes returned from :command
enum TestMode {
	FAILCOMMAND = 0,
	COMMANDED = 1,
	NOPROCESS = 2,
	BEGINANEW = 3,
	OUTPUTASGIVEN = 4,
	RESTART = 5,
	TRACECMD = 6,
	LANGUAGECMD = 7
};

#define MAX_ARG_LIMIT 31 // max args to a call -- limit using 2 bit (COMPILE/KEEP_QUOTES) per arg for table mapping behavior

typedef struct CALLFRAME
{
    char* label;
    char* rule;
    char** display; // where display archive is saved
    char* definition; // actual definition chosen to execute: (locals) + code
    WORDP name; // basic name
    char* code; // code after locals
    char* oldRule;
    char* heapstart;
    char* arguments[MAX_ARG_LIMIT + 1];
    uint64 index; // debug data for crashes
    unsigned int n_arguments; // how many arguments function has
    CALLFRAME* currentFNVarFrame; // where ^n variables are gotten
    CALLFRAME* currentCallFrame; // where current executing code gets its arguments
    unsigned int outputlevel;
    unsigned int oldRuleID;
    unsigned int oldTopic;
    unsigned int oldRuleTopic;
    unsigned int memindex;
    unsigned int heapDepth;
    union {
        FunctionResult result;
        int ownvalue;
    }x;

}CALLFRAME;

enum APICall {
    NO_API_CALL = 0,
    COMPILE_PATTERN = 1,
    COMPILE_OUTPUT = 2,
    TEST_PATTERN = 3,
    TEST_OUTPUT = 4
};
//   argument data for system calls
extern TestMode wasCommand;
extern char* traceTestPatternBuffer;
extern int tracepatterndata;
extern bool directAnalyze;
extern CALLFRAME* frameList[MAX_GLOBAL];

extern HEAPREF patternwordthread;
extern HEAPREF memoryMarkThreadList;
extern HEAPREF memoryVariableThreadList;
extern HEAPREF memoryVariableChangesThreadList;
#define MAX_ARG_LIST 200
#define MAX_CALL_DEPTH 400
extern char* rawtestpatterninput;
extern char testpatternlabel[100];
extern bool softRestart;
extern APICall csapicall;
extern char* style;
extern int rulesExecuted;
extern char* traceTestPattern;
extern bool testpatternblocksave;
extern char* realCode;
extern int maxGlobalSeen;
extern long http_response;
extern char lastcurltime[100];
extern char* currentFunctionName;
extern HEAPREF savedSentencesThreadList;
extern char* testpatterninput;
extern int testPatternIndex;

extern unsigned int currentIterator;
extern char* fnOutput;
extern bool allowBootKill;

extern char* lastInputSubstitution;
extern int globalDepth;

#ifndef DISCARDWEBSOCKET
void WebSocketClient(char* url,char* startmessage);
FunctionResult WebsocketCloseCode(char* buffer);
#endif

#ifdef WIN32
FunctionResult InitWinsock();
#endif

CALLFRAME* ChangeDepth(int value, const char* where, bool nostackCutboack = false, char* code = NULL);
FunctionResult GambitCode(char* buffer);
FunctionResult RunJavaScript(char* definition, char* buffer,unsigned int args);
void DeletePermanentJavaScript();
void DeleteTransientJavaScript();
void ShowChangedVariablesList();
HEAPREF MakeFunctionDefinition(char* data);
unsigned int MACRO_ARGUMENT_COUNT(unsigned char* defn);
FunctionResult FindRuleCode1(char* buffer, char* word);
int GetFnArgCount(char* func,int& flags);
bool NeedNL(MEANING patterns);
//   argument data for user calls
char* InitDisplay(char* list);
void RestoreDisplay(char** base, char* list);
extern SystemFunctionInfo systemFunctionSet[];
extern bool planning;
extern bool nobacktrack;
extern CALLFRAME* currentFNVarFrame;
extern CALLFRAME* currentCallFrame;
CALLFRAME* GetCallFrame(int depth);
FunctionResult MemoryMarkCode(char* buffer);
char* GetArgOfMacro(CALLFRAME* frame,int i, char* buffer, int limit);
FunctionResult MemoryFreeCode(char* buffer);
unsigned char* FindAppropriateDefinition(WORDP D, FunctionResult& result,bool finddefn = false);
void ResetReuseSafety();
unsigned char* GetDefinition(WORDP D);
void InitFunctionSystem(); 
char* DoFunction(char* name, char* ptr, char* buffer,FunctionResult &result);
void DumpFunctions();
unsigned int Callback(WORDP D,char* arguments,bool boot,bool mainoutput = false);
void SaveMark(char* buffer,unsigned int iterator);
FunctionResult RegularReuse(int topic, int id, char* rule,char* buffer,char* arg3,bool crosstopic);
void UpdateTrace(char* value);
FunctionResult InternalCall(char* name, EXECUTEPTR fn, char* arg1, char* arg2, char* arg3, char* buffer, char* arg4 = NULL, char* arg5 = NULL);

FunctionResult KeywordTopicsCode(char* buffer);
void SetBaseMemory();

// utilities
FACT* AddToList(FACT* newlist,FACT* oldfact,GetNextFact getnext,SetNextFact setnext);
FACT* DeleteFromList(FACT* oldlist,FACT* oldfact,GetNextFact getnext,SetNextFact setnext);
FunctionResult ReviseFactCode(char* buffer);
FunctionResult ReviseFact1Code(char* buffer, bool arrayallowed = false);
FunctionResult LengthCode(char* buffer);
char* ResultCode(FunctionResult result);
FunctionResult FLR(char* buffer,char* which);
void ResetUser(char* input);
bool RuleTest(char* rule);
extern int debugValue;
FunctionResult DebugCode(char* buffer);
FunctionResult IdentifyCode(char* buffer);
FunctionResult MatchCode(char* buffer);
FunctionResult CompileOutputCode(char* buffer); // for unit tests
#endif
