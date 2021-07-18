#ifndef _MARKSYSTEMH_
#define _MARKSYSTEMH_
#ifdef INFORMATION
Copyright (C)2011-2021 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif
typedef void (*ExternalTaggerFunction)();
#define SEQUENCE_LIMIT 5		// max number of add-on words in a row to hit on as an entry
#define MAX_XREF_SENTENCE 50	// number of places a word can hit to in sentence
#define REF_ELEMENT_SIZE 8 // bytes per reference start+end+ exactdata:4 + extra + unused
#define  MAXREFSENTENCE_BYTES (MAX_XREF_SENTENCE * REF_ELEMENT_SIZE) 

#define EXACTNOTSET 0
#define EXACTUSEDUP -1
#define DONTUSEEXACT -2
#define REMOVE_SUBJECT 0x00ff

#define RAW 0			// user typed these letters (case insensitive)
#define RAWCASE 1 // user typed these letters (case sensitive)
#define FIXED 2 // user input after spellfixes etc
#define CANONICAL 3 // canonical derived from FIXED

extern int verbwordx;
extern int marklimit;
extern ExternalTaggerFunction externalPostagger;
extern char respondLevel;
extern std::map <WORDP, int> triedData; // per volley index into heap space

extern char unmarked[MAX_SENTENCE_LENGTH];
extern bool showMark;
extern int uppercaseFind;
extern HEAPREF concepts[MAX_SENTENCE_LENGTH];
extern HEAPREF topics[MAX_SENTENCE_LENGTH];
extern int upperCount, lowerCount;
unsigned int GetNextSpot(WORDP D,int start,int& startx,int& endx,bool reverse = false,int gap = 0);
unsigned int GetIthSpot(WORDP D,int i,int& start,int& end);
bool MarkWordHit(int depth,MEANING ucase,WORDP D, int index, int start,int end);
void MarkMeaningAndImplications(int depth,MEANING ucase,MEANING M,int start,int end,int kind = FIXED, bool sequence = false, bool once = false);
bool RemoveMatchValue(WORDP D, int position);
unsigned int IsMarkedItem(WORDP D, int start, int end);
void MarkAllImpliedWords(bool limitnlp);
bool HasMarks(int start);
MEANING EncodeConceptMember(char* word, int& flags);
char* DumpAnalysis(int start, int end, uint64 posValues[MAX_SENTENCE_LENGTH],const char* label,bool original,bool roles);
#endif
