#ifndef _VARIABLESYSTEMH_
#define _VARIABLESYSTEMH_
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

#define ALREADY_HANDLED -1
#define MAX_MATCHVAR_SIZE 20000

#define ILLEGAL_MATCHVARIABLE -1

#define MAX_WILDCARDS 30  // _0 ... _30 inclusive
#define WILDCARD_START(x) (x & 0x0000ffff)
#define WILDCARD_END(x) ( x >> 16)
#define WILDCARD_END_ONLY(x) ( (x >> 16) & REMOVE_SUBJECT) 
#define WILDENDSHIFT(x) (x << 16)
extern  unsigned int modifiedTraceVal;
extern bool	modifiedTrace;
extern unsigned int modifiedTimingVal;
extern bool modifiedTiming;
extern HEAPREF variableChangedThreadlist;
extern char* nullLocal;
extern char* nullGlobal;
extern bool legacyMatch;

extern  int wildcardIndex;
extern char wildcardOriginalText[MAX_WILDCARDS+1][MAX_MATCHVAR_SIZE+1];  //   spot wild cards can be stored
extern char wildcardCanonicalText[MAX_WILDCARDS+1][MAX_MATCHVAR_SIZE+1];  //   spot wild cards can be stored
extern unsigned int wildcardPosition[MAX_WILDCARDS+1]; //   spot it started and ended in sentence (16bit end 16bit start)
extern int impliedSet;
extern int impliedWild;
extern char impliedOp;
extern unsigned int tracedFunctionsIndex;
extern WORDP tracedFunctionsList[MAX_TRACED_FUNCTIONS];
extern char wildcardSeparator[2];
extern bool wildcardSeparatorGiven;
extern HEAPREF userVariableThreadList;

extern HEAPREF kernelVariableThreadList;
// wildcard accessors
char* GetwildcardText(unsigned int i, bool canon);
void SetWildCard(char* value,char* canonicalVale,const char* index,unsigned int position);
void SetWildCardGiven(unsigned int start, unsigned int end, bool inpattern, int index,MARKDATA* hitdata = NULL);
void SetWildCardIndexStart(int);
int GetWildcardID(char* x);
void SetAPIVariable(WORDP D, char* value);
void ReadVariables(const char* name);
void InitBotVariables(int argc, char** argv);

// Variables loaded from bot (topic system)
void ReestablishBotVariables();
void NoteBotVariables();
void InitVariableSystem();
void SetWildCardGivenValue(char* original, char* canonical,unsigned int start, unsigned int end, int index,MARKDATA* hitdata = NULL);
void OpenBotVariables();
void CloseBotVariables();
void SetBotVariable(char* word);

// debug data
void ShowChangedVariables();
void DumpUserVariables(bool all);
void SetWildCardNull();
void PrepareVariableChange(WORDP D,char* word,bool init);

// user variable accessors
void ClearUserVariableSetFlags();
void ClearUserVariables(char* above = 0);
void MigrateUserVariables();
void RecoverUserVariables();
char* GetUserVariable(const char* word, bool nojson = false);
void SetUserVariable(const char* var, char* word, bool assignment = false,bool reuse = false);
FunctionResult Add2UserVariable(char* var, char* word,char* op,char* originalArg);

char* PerformAssignment(char* word,char* ptr,char* buffer,FunctionResult& result,bool nojson = false);


#endif
