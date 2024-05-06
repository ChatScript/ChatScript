#ifndef _MARKSYSTEMH_
#define _MARKSYSTEMH_
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
typedef void (*ExternalTaggerFunction)();
#define SEQUENCE_LIMIT 6		// max number of add-on words in a row to hit on as an entry

#define EXACTNOTSET 0
#define EXACTUSEDUP -1
#define DONTUSEEXACT -2
#define REMOVE_SUBJECT 0x00ff

#define RAW 0			// user typed these letters (case insensitive)
#define RAWCASE 1 // user typed these letters (case sensitive)
#define FIXED 2 // user input after spellfixes etc
#define CANONICAL 3 // canonical derived from FIXED

#define END_OF_REFERENCES 0xff

/////////////// TriedData information
#define MAX_XREF_SENTENCE REAL_SENTENCE_WORD_LIMIT	// number of places a word can hit to in sentence
#define REF_ELEMENT_SIZE 8 // bytes per reference start+end+ exactdata:4 + extra + unused
#define  MAXREFSENTENCE_BYTES (MAX_XREF_SENTENCE * REF_ELEMENT_SIZE) 
#define TRIEDDATA_WORDSIZE ((sizeof(uint64)/4)  + ((MAXREFSENTENCE_BYTES + 3) / sizeof(int)))
//  64bit tried by meaning field (aligned) + sentencerefs (8 bytes each x 50)
#define REFSTRIEDDATA_WORDSIZE (TRIEDDATA_WORDSIZE - 2 )
extern std::map <WORDP, HEAPINDEX> triedData; // per volley index into heap space

typedef struct MARKDATA //   a dictionary entry  - starred items are written to the dictionary
  // if you edit this, you may need to change ReadBinaryEntry and WriteBinaryEntry
{
	unsigned int start; // first contiguous word position in sentence;
	unsigned int end; // last contiguous word position in sentence;
	MEANING word; // the actual matched word/phrase (correct case)
	unsigned int disjoint; // outlier before or after word
	unsigned int kind; // canonical, original, whatever
} MARKDATA;


extern int verbwordx;
extern int marklimit;
extern ExternalTaggerFunction externalPostagger;
extern char respondLevel;

extern char unmarked[MAX_SENTENCE_LENGTH];
extern char oldunmarked[MAX_SENTENCE_LENGTH];
extern bool showMark;
extern HEAPREF concepts[MAX_SENTENCE_LENGTH];
extern HEAPREF topics[MAX_SENTENCE_LENGTH];
extern int upperCount, lowerCount;
unsigned int GetNextSpot(WORDP D,int start,bool reverse = false,unsigned int gap = 0,MARKDATA* hitdata = NULL);
unsigned int GetIthSpot(WORDP D,int i, unsigned int& start,unsigned int& end);
bool MarkWordHit(int depth, MEANING ucase, WORDP D, int index, unsigned int start, unsigned int end, unsigned int prefix = 0, unsigned int kind = 5);
void ShowMarkData(char* word); 
void MarkMeaningAndImplications(int depth,MEANING ucase,MEANING M,int start,int end,int kind = FIXED, bool sequence = false, bool once = false,int prequel = 0);
void HuntMatch(int kind, char* word,bool strict,int start, int end, unsigned int& usetrace,unsigned int restriction = 0);
bool RemoveMatchValue(WORDP D, int position);
unsigned int IsMarkedItem(WORDP D, int start, int end);
void MarkAllImpliedWords(bool limitnlp);
bool HasMarks(int start);
MEANING EncodeConceptMember(char* word, int& flags);
char* DumpAnalysis(unsigned int start, unsigned  int end, uint64 posValues[MAX_SENTENCE_LENGTH],const char* label,bool original,bool roles);
#endif
