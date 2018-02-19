#ifndef _INFERH_
#define _INFERH_
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

#define MAX_FIND 10000
#define MAX_FIND_SETS 20

#define ILLEGAL_FACTSET -1

#define FACTSET_COUNT(x) ( (unsigned int) ( (uintptr_t) factSet[x][0]  ))
void SET_FACTSET_COUNT(int x, int y);

extern FACT* factSet[MAX_FIND_SETS+2][MAX_FIND+1];
extern unsigned int factSetNext[MAX_FIND_SETS+1];
extern int	  factFlags[MAX_FIND+1];
extern int	  factIndex[MAX_FIND+1];
extern unsigned int inferMark;
FACT* IsConceptMember(WORDP D);
unsigned int NextInferMark();
unsigned int Query(char* search, char* subject, char* verbword, char* object, unsigned int count, char* from, char* toset, char* up, char* match);
FunctionResult QueryTopicsOf(char* word,unsigned int store,char* kind);
bool SetContains(MEANING set,MEANING M);
#endif
