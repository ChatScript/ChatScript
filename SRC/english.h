#ifndef ENGLISH_H
#define ENGLISH_H
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


#ifdef TREETAGGER
void InitTreeTagger(char* params);
void MarkChunk();
void TreeTagger();
bool MatchTag(char* tag,unsigned int i);
char* GetTag(unsigned int i);
extern bool treetaggerfail;
#endif

#define MAINLEVEL 1

#define MAX_CLAUSES 50
extern unsigned int ambiguousWords;
extern uint64 posTiming;
extern bool parseLimited;
extern unsigned char quotationInProgress;
extern unsigned int roleIndex;
extern unsigned int needRoles[MAX_CLAUSES]; 
extern unsigned char subjectStack[MAX_CLAUSES];
extern unsigned char verbStack[MAX_CLAUSES];
extern char* usedTrace;
extern int nWordForms;
extern WORDP wordForms[50];
extern WORDP negateWordForms[50];
extern int nNegateWordForms;
extern unsigned int usedWordIndex;
extern uint64 usedType;
void ShowForm(char* word);
void DecodeTag(char* buffer, uint64 type, uint64 tie,uint64 originalbits);
bool IsDate(char* original);
uint64 GetPosData(unsigned  int at, char* original,WORDP &revise,WORDP &entry,WORDP &canonical,uint64 &sysflags,uint64 &cansysflags, bool firstTry = true,bool nogenerate = false, unsigned int start = 0);
char* GetAdjectiveBase(char* word,bool nonew);
char* GetAdverbBase(char* word,bool nonew);
char* GetPastTense(char* word);
char* GetPastParticiple(char* word);
char* GetPresentParticiple(char* word);
void CheckParseLimit(char* input);
char* GetThirdPerson(char* word);
char* GetPresent(char* word);
char* GetInfinitive(char* word,bool nonew);
char* GetSingularNoun(char* word,bool initial,bool nonew);
char* GetPluralNoun(char* noun,char* plu);
char* GetAdjectiveMore(char* word);
char* GetAdjectiveMost(char* word);
char* GetAdverbMore(char* word);
char* GetAdverbMost(char* word);
void SetSentenceTense(unsigned int start,unsigned  int end);
WORDP SuffixAdjust(char* word, int lenword, char* suffix, int lensuffix,uint64 bits = 0);
uint64 ProbableAdjective(char* original, unsigned int len,uint64& expectedBase);
uint64 ProbableAdverb(char* original, unsigned int len,uint64& expectedBase);
uint64 ProbableNoun(char* original,unsigned int len);
uint64 ProbableVerb(char* original,unsigned int len);
bool IsDeterminedNoun(unsigned int i,unsigned int& det);

#endif
