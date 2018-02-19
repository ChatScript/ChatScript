#ifndef _SCRIPTCOMPILEH_
#define _SCRIPTCOMPILEH_
#ifdef INFORMATION
Copyright (C)2011-2018 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif

#define PATTERN_SPELL 1	// test pattern words for validity
#define OUTPUT_SPELL 2	// test output words for validity
#define NOTE_KEYWORDS 4 // track keywords used
#define NO_SUBSTITUTE_WARNING 8 // dont warn about substitutions
#define NO_SPELL 16		// test nothing

#define ARGSETLIMIT 40 // ^0...^39
extern unsigned int buildID; // build 0 or build 1
extern char* newBuffer;
extern bool compiling;
extern char botheader[MAX_WORD_SIZE];
extern bool patternContext;
extern uint64 grade;
extern char* lastDeprecation;
extern unsigned int buildId;

void ScriptError();
void EraseTopicFiles(unsigned int build,char* name);
void InitScriptSystem();
void SaveCanon(char* word, char* canon);

char* ReadDisplayOutput(char* ptr,char* buffer);
  
#ifndef DISCARDSCRIPTCOMPILER
int ReadTopicFiles(char* name,unsigned int build, int spell);
char* ReadPattern(char* ptr, FILE* in, char* &data,bool macro,bool ifstatement, bool livecall = false);
char* ReadOutput(bool optionalBrace,bool nested,char* ptr, FILE* in,char* &data,char* rejoinders,char* existingRead = NULL,WORDP call = NULL, bool choice = false);
char* CompileString(char* ptr);
void ScriptWarn();
void EndScriptCompiler();
bool StartScriptCompiler();
#define WARNSCRIPT(...) {if (compiling) {ScriptWarn(); Log(STDTRACELOG, __VA_ARGS__);} } // readpattern calls from functions should not issue warnings
#else
#define WARNSCRIPT(...) {Log(STDTRACELOG, __VA_ARGS__); } // readpattern calls from functions should not issue warnings
#endif

// ALWAYS AVAILABLE
extern unsigned int hasErrors;

void AddWarning(char* buffer);
void AddError(char* buffer);
char* ReadNextSystemToken(FILE* in,char* ptr, char* word, bool separateUnderscore=true,bool peek=false);
char* ReadSystemToken(char* ptr, char* word, bool separateUnderscore=true);

#define BADSCRIPT(...) {ScriptError(); Log((compiling) ? BADSCRIPTLOG : STDTRACELOG , __VA_ARGS__); JumpBack();}

#endif
