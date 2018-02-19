#ifdef INFORMATION
Copyright (C) 2011 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif

#ifndef __TAGGER__
#define __TAGGER__


#define MAX_TAG_FIELDS 4

 enum IgnoreValue{
	NOIGNORE = 0,
	IGNOREALL = 1,
	IGNOREPAREN = 2,
};

#ifdef JUNK
The top 16 bits of dictionary properties are not used in pos-tagging labeling of words
#endif

#define RESULT_SHIFT 48		// for 3 - set only on 1st field 49.50.51
#define OFFSET_SHIFT 48		// for 3 - set only on 2nd field 49.50.51
#define PART1_BIT   (1LL << 48)		// for 1 - set only on 3rd field 49
#define USED_BIT	(1LL << 51)		// for 1 - set only on first field to show rule applied

#define PART2_BIT	( 1LL << 52)	// 1bit -- set on [0] 1st field only 0x0020 000000000000  (this rule is 2nd half of prior rule)
#define KEEP_BIT	( 1LL << 52) 	// for 1 - set only on [1] 2nd field
#define REVERSE_BIT ( 1LL << 52)	// for 1 - set only on [2] 3rd field
#define TRACE_BIT ( 1LL << 52)		// for 1  - set only on [3] 4th field 0x001000000000000000

#define SKIP_OP	( 1LL << 55 )
#define STAY_OP	( 1LL << 56 )
#define NOT_OP	( 1LL << 57 )
#define CONTROL_SHIFT 55 // for 3 flags - set on all pattern fields  9 bits 
#define OP_SHIFT 58		// for 6 operator set on all pattern fields
#define CTRL_SHIFT ( OP_SHIFT - CONTROL_SHIFT )
#define PATTERN_BITS 0x0000FFFFFFFFFFFFULL  // 48 bits 
#define CONTROL_BITS 0xFF80000000000000ULL // 9 control bits  
//0-47  48,49,50,	51,   52,   53,54,	55,56,57,58,59,60,61,62,63 
extern uint64 lcSysFlags[MAX_SENTENCE_LENGTH];
extern uint64 canSysFlags[MAX_SENTENCE_LENGTH];
extern uint64 posValues[MAX_SENTENCE_LENGTH];	
extern  uint64* tags;
extern bool reverseWords;
#define AUXQUESTION 1
#define QWORDQUESTION 2

extern  WORDP wordTag[MAX_SENTENCE_LENGTH]; 
extern  WORDP wordRole[MAX_SENTENCE_LENGTH]; 
extern  char* wordCanonical[MAX_SENTENCE_LENGTH]; //   current sentence tokenization
extern  WORDP originalLower[MAX_SENTENCE_LENGTH];
extern  WORDP originalUpper[MAX_SENTENCE_LENGTH];
extern  WORDP canonicalLower[MAX_SENTENCE_LENGTH];
extern  WORDP canonicalUpper[MAX_SENTENCE_LENGTH];
extern  uint64 finalPosValues[MAX_SENTENCE_LENGTH];
extern  uint64 allOriginalWordBits[MAX_SENTENCE_LENGTH];	// starting full bits about this word (all dict properties) in this word position
extern  uint64 lcSysFlags[MAX_SENTENCE_LENGTH];      // current system tags lowercase in this word position (there are no interesting uppercase flags)
extern  uint64 posValues[MAX_SENTENCE_LENGTH];			// current pos tags in this word position
extern unsigned int lowercaseWords;
extern unsigned int knownWords;
extern char* roleSets[];
extern uint64* dataBuf;
extern char** commentsData;

extern  char** comments;
extern int itAssigned;
extern int theyAssigned;
extern WORDP originalLower[MAX_SENTENCE_LENGTH];
extern WORDP originalUpper[MAX_SENTENCE_LENGTH];
extern  WORDP canonicalLower[MAX_SENTENCE_LENGTH];
extern WORDP canonicalUpper[MAX_SENTENCE_LENGTH];
extern char* wordCanonical[MAX_SENTENCE_LENGTH];	//   canonical form of word 

void TagTest(char* file);
extern unsigned int tagRuleCount;
extern unsigned char bitCounts[MAX_SENTENCE_LENGTH]; 
#define MAX_POS_RULES 1500
void MarkTags(unsigned int i);
char* GetNounPhrase(int i,const char* avoid);
extern int clauses[MAX_SENTENCE_LENGTH];
extern int phrases[MAX_SENTENCE_LENGTH];
extern int verbals[MAX_SENTENCE_LENGTH];
extern unsigned char ignoreWord[MAX_SENTENCE_LENGTH];
extern unsigned char coordinates[MAX_SENTENCE_LENGTH];
extern unsigned char crossReference[MAX_SENTENCE_LENGTH];
extern unsigned char phrasalVerb[MAX_SENTENCE_LENGTH];
extern uint64 roles[MAX_SENTENCE_LENGTH];
extern unsigned int parseFlags[MAX_SENTENCE_LENGTH];
extern unsigned char tried[MAX_SENTENCE_LENGTH];
extern unsigned char objectRef[MAX_SENTENCE_LENGTH];  // link from verb to any main object
extern unsigned char indirectObjectRef[MAX_SENTENCE_LENGTH];  // link from verb to any indirect object
extern unsigned char complementRef[MAX_SENTENCE_LENGTH];  // link from verb to any main object

#ifndef DISCARDPARSER

void MarkRoles(int i);
#endif
void TagInit();
void TagIt();
void ParseSentence(bool &resolved,bool &changed);
void DumpSentence(int start, int end);
char* GetRole(uint64 role);

#endif
