#ifndef _TOKENSYSTEMH_
#define _TOKENSYSTEMH_
#include "common.h"

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

#define FUNDAMENTAL_VERB 1
#define FUNDAMENTAL_OBJECT 2
#define FUNDAMENTAL_SUBJECT 4
#define FUNDAMENTAL_SUBJECT_OBJECT 8

extern uint64 tokenFlags;  
extern char* wordStarts[MAX_SENTENCE_LENGTH];
extern bool capState[MAX_SENTENCE_LENGTH];	
extern unsigned int wordCount;
#define MAX_BURST 400
extern char burstWords[MAX_BURST][MAX_WORD_SIZE];
extern int inputNest;
extern int hasFundamentalMeanings;
extern int actualTokenCount;
bool ReplaceWords(char* why,int i, int oldlength,int newlength,char** tokens);
int BurstWord(const char* word, int contractionStyle = 0);
char* GetBurstWord(unsigned int n);
char* JoinWords(unsigned int n,bool output = false,char* buffer = NULL);
void ProcessSplitUnderscores();
WORDP ApostropheBreak(char* aword);
char* Tokenize(char* input, unsigned int& count,char** words, char* separators, bool all = false,bool oobstart = false);
int ValidPeriodToken(char* start, char* end, char next,char next2);
char* ReadTokenMass(char* ptr, char* word);
void ProcessSubstitutes();
FunctionResult GetDerivationText(int start, int end, char* buffer);
void ProcessCompositeNumber();
void ProcessCompositeDate();
void ProperNameMerge();
bool DateZone(unsigned int i, unsigned  int& start, unsigned int& end);
bool ParseTime(char* ptr, char** minute, char** meridiem);
char* FindTimeMeridiem(char* ptr, int len = 0);
unsigned int TransformCount(char* dictword, unsigned int inputLen, char* inputSet, uint64 min);
void ResetTokenSystem();
void DumpTokenControls(uint64 val);
void DumpResponseControls(uint64 val);
void DumpTokenFlags(char* msg);
char* find_closest_jp_period(char* input); /* for testing */
#endif
