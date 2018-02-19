#ifndef _H
#define _H
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


#ifdef TREETAGGER
void InitTreeTagger(char* params);
void MarkChunk();
bool MatchTag(char* tag,int i);
char* GetTag(int i);
#endif

#define MAINLEVEL 1

#define MAX_CLAUSES 50
extern unsigned int ambiguousWords;
extern uint64 posTiming;
extern unsigned char quotationInProgress;
extern unsigned int roleIndex;
extern unsigned int needRoles[MAX_CLAUSES]; 
extern unsigned char subjectStack[MAX_CLAUSES];
extern unsigned char verbStack[MAX_CLAUSES];
extern char* usedTrace;
extern int usedWordIndex;
extern uint64 usedType;

void SetRole( int i, uint64 role, bool revise = false,  int currentVerb = verbStack[roleIndex]);
void DecodeTag(char* buffer, uint64 type, uint64 tie,uint64 originalbits);
bool IsDate(char* original);
uint64 GetPosData( int at, char* original,WORDP &revise,WORDP &entry,WORDP &canonical,uint64 &sysflags,uint64 &cansysflags, bool firstTry = true,bool nogenerate = false,int start = 0);
char* GetAdjectiveBase(char* word,bool nonew);
char* GetAdverbBase(char* word,bool nonew);
char* GetPastTense(char* word);
char* GetPastParticiple(char* word);
char* GetPresentParticiple(char* word);
char* GetThirdPerson(char* word);
char* GetPresent(char* word);
char* GetInfinitive(char* word,bool nonew);
char* GetSingularNoun(char* word,bool initial,bool nonew);
char* GetPluralNoun(WORDP noun);
char* GetAdjectiveMore(char* word);
char* GetAdjectiveMost(char* word);
char* GetAdverbMore(char* word);
char* GetAdverbMost(char* word);
void SetSentenceTense(int start, int end);
uint64 ProbableAdjective(char* original, unsigned int len,uint64& expectedBase);
uint64 ProbableAdverb(char* original, unsigned int len,uint64& expectedBase);
uint64 ProbableNoun(char* original,unsigned int len);
uint64 ProbableVerb(char* original,unsigned int len);
bool IsDeterminedNoun(int i,int& det);
#endif
