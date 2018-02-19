#ifndef _FACTSYSTEMH_
#define _FACTSYSTEMH_
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

#define MAX_FACT_NODES 800000 

typedef struct FACT 
{  
	unsigned int flags;		

 	FACTOID_OR_MEANING subjectHead;	
	FACTOID_OR_MEANING verbHead;		
	FACTOID_OR_MEANING objectHead;		

    FACTOID_OR_MEANING subjectNext;	
 	FACTOID_OR_MEANING verbNext;		
    FACTOID_OR_MEANING objectNext;  	

	MEANING subject;		
	MEANING verb;			
    MEANING object;	

	uint64 botBits;		// which bots can access this fact (up to 64)
} FACT;
extern int worstFactFree;
extern FACT* factBase;		//   start of fact space
extern FACT* factEnd;		//   end of fact space
extern FACT* currentFact;	//   most recent fact found or created
extern FACT* factsPreBuild[NUMBER_OF_LAYERS+1];	//   end of build0 facts (start of build1 facts)
extern FACT* factFree;		//   end of facts - most recent fact allocated (ready for next allocation)

// pre-reserved verb types
extern MEANING Mmember;
extern MEANING Mexclude;
extern MEANING Mis;

extern size_t maxFacts;		// allocation limit of facts
extern uint64 myBot;
void SortFacts(char* set, int alpha, int setpass = -1);

// fact index accessing
FACTOID Fact2Index(FACT* F);
FACT* Index2Fact(FACTOID e);
inline FACTOID currentFactIndex() { return (currentFact) ? (FACTOID)((currentFact - factBase)) : 0;}
FACT* FactTextIndex2Fact(char* word);
void RipFacts(FACT* F,WORDP dictbase);
void WeaveFacts(FACT* F);

// fact system startup and shutdown
void InitFacts();
void CloseFacts();
void ResetFactSystem(FACT* locked);
void InitFactWords();
void AutoKillFact(MEANING M);

// fact creation and destruction
FACT* FindFact(FACTOID_OR_MEANING subject, FACTOID_OR_MEANING verb, FACTOID_OR_MEANING object, unsigned int properties = 0);
FACT* CreateFact(FACTOID_OR_MEANING subject,FACTOID_OR_MEANING verb, FACTOID_OR_MEANING object,unsigned int properties = 0);
FACT* CreateFastFact(FACTOID_OR_MEANING subject, FACTOID_OR_MEANING verb, FACTOID_OR_MEANING object, unsigned int properties);
void KillFact(FACT* F,bool jsonrecurse = true, bool autoreviseArray = true);
FACT* SpecialFact(FACTOID_OR_MEANING verb, FACTOID_OR_MEANING object,unsigned int flags);
void FreeFact(FACT* F);
char* GetSetEnd(char* x);

// fact reading and writing
char* ReadField(char* ptr,char* &field,char fieldkind,unsigned int& flags);
char* EatFact(char* ptr,char* buffer,unsigned int flags = 0,bool attribute = false);
FACT* ReadFact(char* &ptr,unsigned int build);
void ReadFacts(const char* name,const char* layer,unsigned int build,bool user = false);
char* WriteFact(FACT* F,bool comments,char* buffer,bool ignoreDead = false,bool eol = false);
void WriteFacts(FILE* out,FACT* from,int flags = 0);
bool ReadBinaryFacts(FILE* in);
void WriteBinaryFacts(FILE* out,FACT* F);
void ClearUserFacts();
extern char traceSubject[100];
extern char traceVerb[100];
extern char traceObject[100];

// factset information
char* GetSetType(char* x);
int GetSetID(char* x);
bool GetSetMod(char* x);
unsigned int AddFact(unsigned int set, FACT* F);
FunctionResult ExportJson(char* name, char* jsonitem, char* append);

// reading and writing facts to file
bool ExportFacts(char* name, int set,char* append);
bool ImportFacts(char* buffer,char* name, char* store, char* erase, char* transient);

// debugging
void TraceFact(FACT* F,bool ignoreDead = true);

// field access

inline FACT* GetSubjectHead(FACT* F) {return Index2Fact(F->subjectHead);}
inline FACT* GetVerbHead(FACT* F) {return Index2Fact(F->verbHead);}
inline FACT* GetObjectHead(FACT* F) {return Index2Fact(F->objectHead);}

FACT* GetSubjectNondeadHead(FACT* F);
FACT* GetVerbNondeadHead(FACT* F);
FACT* GetObjectNondeadHead(FACT* F);

inline void SetSubjectHead(FACT* F, FACT* value){ F->subjectHead = Fact2Index(value);}
inline void SetVerbHead(FACT* F, FACT* value) {F->verbHead = Fact2Index(value);}
inline void SetObjectHead(FACT* F, FACT* value){ F->objectHead = Fact2Index(value);}

typedef FACT* (*GetNextFact)(FACT* F);
typedef void (*SetNextFact)(FACT* F,FACT* value);

FACT* GetSubjectNondeadNext(FACT* F,bool jsonaccept = true);
FACT* GetVerbNondeadNext(FACT* F);
FACT* GetObjectNondeadNext(FACT* F) ;
FACT* GetSubjectNext(FACT* F);
FACT* GetVerbNext(FACT* F);
FACT* GetObjectNext(FACT* F) ;
void SetSubjectNext(FACT* F, FACT* value);
void SetVerbNext(FACT* F, FACT* value);
void SetObjectNext(FACT* F, FACT* value);

inline FACT* GetSubjectHead(WORDP D) {return Index2Fact(D->subjectHead);}
inline FACT* GetVerbHead(WORDP D) {return Index2Fact(D->verbHead);}
inline FACT* GetObjectHead(WORDP D)  {return Index2Fact(D->objectHead);}

FACT* GetSubjectNondeadHead(WORDP D,bool jsonaccept = true);
FACT* GetVerbNondeadHead(WORDP D);
FACT* GetObjectNondeadHead(WORDP D);
inline FACT* GetSubjectNondeadHead(MEANING M) {return GetSubjectNondeadHead(Meaning2Word(M));}
inline FACT* GetVerbNondeadHead(MEANING M) {return GetVerbNondeadHead(Meaning2Word(M));}
inline FACT* GetObjectNondeadHead(MEANING M)  {return GetObjectNondeadHead(Meaning2Word(M));}

inline void SetSubjectHead(WORDP D, FACT* value) {D->subjectHead = Fact2Index(value);}
inline void SetVerbHead(WORDP D, FACT* value) {D->verbHead = Fact2Index(value);}
inline void SetObjectHead(WORDP D, FACT* value) {D->objectHead = Fact2Index(value);}

inline FACT* GetSubjectHead(MEANING M) {return GetSubjectHead(Meaning2Word(M));}
inline FACT* GetVerbHead(MEANING M) {return GetVerbHead(Meaning2Word(M));}
inline FACT* GetObjectHead(MEANING M)  {return GetObjectHead(Meaning2Word(M));}
inline void SetSubjectHead(MEANING M, FACT* value) {SetSubjectHead(Meaning2Word(M),value);}
inline void SetVerbHead(MEANING M, FACT* value) {SetVerbHead(Meaning2Word(M),value);}
inline void SetObjectHead(MEANING M, FACT* value) {SetObjectHead(Meaning2Word(M),value);}

#endif
