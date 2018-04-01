#ifndef _EXECUTE
#define _EXECUTE


#ifdef INFORMATION
Copyright (C)2011-2018 by Bruce Wilcox

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

#define FAILCODES ( RESTART_BIT | FAILINPUT_BIT | FAILTOPIC_BIT | FAILLOOP_BIT | FAILRULE_BIT | FAILSENTENCE_BIT | RETRYSENTENCE_BIT | RETRYTOPIC_BIT | UNDEFINED_FUNCTION | RETRYINPUT_BIT )
#define SUCCESSCODES ( ENDINPUT_BIT | ENDSENTENCE_BIT | NEXTLOOP_BIT | ENDTOPIC_BIT | ENDRULE_BIT | ENDCALL_BIT | ENDLOOP_BIT)
#define ENDCODES ( FAILCODES | SUCCESSCODES )
#define RESULTBEYONDTOPIC ( RESTART_BIT | FAILSENTENCE_BIT | ENDSENTENCE_BIT | RETRYSENTENCE_BIT | ENDINPUT_BIT | FAILINPUT_BIT | RETRYINPUT_BIT )

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
};

//   argument data for system calls
extern TestMode wasCommand;

#define MAX_ARG_LIST 200
#define MAX_CALL_DEPTH 400
extern char* codeStart;
extern int rulesExecuted;
extern char* realCode;
extern unsigned int callIndex;
extern WORDP callStack[MAX_CALL_DEPTH];
extern int callArgumentBases[MAX_CALL_DEPTH];    // arguments to functions
extern unsigned int callArgumentIndex;
extern int maxGlobalSeen;
extern long http_response;
extern char* currentFunctionName;
extern unsigned int savedSentences;

#define MAX_ARG_LIMIT 31 // max args to a call -- limit using 2 bit (COMPILE/KEEP_QUOTES) per arg for table mapping behavior
extern unsigned int currentIterator;
extern char* fnOutput;

extern char lastInputSubstitution[INPUT_BUFFER_SIZE];
extern int globalDepth;
#ifdef WIN32
FunctionResult InitWinsock();
#endif

FunctionResult RunJavaScript(char* definition, char* buffer,unsigned int args);
void DeletePermanentJavaScript();
void DeleteTransientJavaScript();
unsigned int MACRO_ARGUMENT_COUNT(unsigned char* defn);
void DebugConcepts(int list, int wordindex);

//   argument data for user calls
char* InitDisplay(char* list);
void RestoreDisplay(char** base, char* list);
extern SystemFunctionInfo systemFunctionSet[];
extern bool planning;
extern bool nobacktrack;
CALLFRAME* GetCallFrame(int depth);
FunctionResult MemoryMarkCode(char* buffer);
char* GetArgOfMacro(int i, char* buffer, int limit);
FunctionResult MemoryFreeCode(char* buffer);
unsigned char* FindAppropriateDefinition(WORDP D, FunctionResult& result);
void ResetReuseSafety();
unsigned char* GetDefinition(WORDP D);
void InitFunctionSystem(); 
char* DoFunction(char* name, char* ptr, char* buffer,FunctionResult &result);
void DumpFunctions();
unsigned int Callback(WORDP D,char* arguments,bool boot,bool mainoutput = false);
void ResetFunctionSystem();
void SaveMark(char* buffer,unsigned int iterator);
FunctionResult RegularReuse(int topic, int id, char* rule,char* buffer,char* arg3,bool crosstopic);

FunctionResult KeywordTopicsCode(char* buffer);
void SetBaseMemory();
void ResetBaseMemory();

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
FunctionResult DebugCode(char* buffer);
FunctionResult IdentifyCode(char* buffer);
FunctionResult MatchCode(char* buffer);
#endif
