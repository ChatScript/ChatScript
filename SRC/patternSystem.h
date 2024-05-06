#ifndef _PATTERNH_
#define _PATTERNH_
#ifdef INFORMATION
Copyright (C)2011-2024 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif

#include "common.h"

#define NORETRY 10000

extern unsigned int patternEvaluationCount;    // number of patterns evaluated in this volley
extern bool nopatterndata;
void ExecuteConceptPatterns(FACT* specificPattern = NULL);
bool MatchesPattern(char* word, char* pattern);
void GetPatternData(char* buffer);
void GetPatternMatchedWords(char* buffer);
bool Match(char* ptr, int depth, MARKDATA& hitdata,int rebindable,unsigned int wildcardSelector,
	int& firstmatched,char kind,
	bool reverse = false);
extern int indentBasis;
extern char* patternchoice;
extern HEAPREF heapPatternThread;
extern HEAPREF matchedWordsList;
extern bool patternRetry;
extern bool deeptrace;
extern int patternDepth;
void ShowMatchResult(FunctionResult result, char* rule,char* label,int id = 0);
#endif
