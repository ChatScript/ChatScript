#include "common.h"

#ifdef INFORMATION

Facts are added as layers.You remove facts by unpeeling a layer.You can "delete" a fact merely by
marking it dead.

Layer - 1:  facts resulting from wordnet dictionary(wordnetFacts)
Layer0 : facts resulting from topic system build0
    Layer1 : facts resulting from topic system build1
    Layer2 : facts created by user

    Layer 2 is always unpeeled after an interchange with the chatbot, in preparation for a new user who may chat with a different persona.
    Layers 0 and 1 are unpeeled if you want to restart the topic system to read in new topic data on the fly or rebuild 0.
    Layer 1 is unpeeled for a new build 1.

    Layer - 1 is never unpeeled.If you want to modify the dictionary, you either restart the chatbot entirely
    or patch in data piggy backing on facts(like from the topic system).

    Unpeeling a layer implies you will also reset dictionary / stringspace pointers back to levels at the
    start of the layer since facts may have allocated dictionary and string items.
    This is ReturnToDictionaryFreeze for unpeeling 3 / 4 and ReturnDictionaryToWordNet for unpeeling layer 2.

#endif
int worstFactFree = 1000000;
char traceSubject[100];
bool allowBootKill = false;
char traceVerb[100];
char traceObject[100];
bool bootFacts = false;
bool recordBoot = false;
uint64 allowedBots = 0xffffffffffffffffULL; // default fact visibility restriction is all bots
uint64 myBot = 0;							// default fact creation restriction
size_t maxFacts = MAX_FACT_NODES;	// how many facts we can create at max
HEAPREF factThread = NULL;
static STACKREF stackFactThread = NULL;
bool factsExhausted = false;
FACT* factBase = NULL;			// start of all facts
FACT* factEnd = NULL;			// end of all facts
FACT* factsPreBuild[NUMBER_OF_LAYERS+1];		// on last fact of build0 facts, start of build1 facts
FACT* factFree = NULL;			// factFree is a fact in use (increment before use as a free fact)
FACT* currentFact = NULL;		// current fact found or created

//   values of verbs to compare against
MEANING Mmember;				// represents concept sets
MEANING Mis;					// represents wordnet hierarchy
MEANING Mexclude;				// represents restriction of word not allowed in set (blocking inheritance)
bool seeAllFacts = false;
FACT* Index2Fact(FACTOID e)
{ 
	FACT* F = NULL;
	if (e)
	{
		F =  e + factBase;
		if (F > factFree)
		{
			char buf[MAX_WORD_SIZE];
			strcpy(buf, StdIntOutput(factFree - factBase));
			ReportBug((char*)"Illegal fact index %d last:%s ", e, buf);
			F = NULL;
		}
	}
	return F;
}

static bool UnacceptableFact(FACT* F,bool jsonavoid)
{
	if (!F || F->flags & FACTDEAD) return true;
	// if ownership flags exist (layer0 or layer1) and we have different ownership.
	return (F->botBits && !(F->botBits & myBot) && !seeAllFacts) ? true : false;
}

FACT* GetSubjectNext(FACT* F) { return Index2Fact(F->subjectNext);}
FACT* GetVerbNext(FACT* F) {return Index2Fact(F->verbNext);}
FACT* GetObjectNext(FACT* F) {return Index2Fact(F->objectNext);}

bool ValidMemberFact(FACT* F)
{
	return (F->verb == Mmember && (!myBot || !F->botBits || F->botBits & myBot)); 
}

FACT* GetSubjectNondeadNext(FACT* F,bool jsonaccept) 
{ 
	FACT* G = F;
	while (F) 
	{
		F =  Index2Fact(F->subjectNext);
		if (F && !UnacceptableFact(F,jsonaccept)) break;
	}
	if (F == G)
	{
		ReportBug("Infinite loop in GetSubjectNondeadNext ")
		F = NULL;
	}
	return F;
}

FACT* GetVerbNondeadNext(FACT* F) 
{
	FACT* G = F;
	while (F) 
	{
		F =  Index2Fact(F->verbNext);
		if (F && !UnacceptableFact(F,true)) break;
	}
	if (F == G)
	{
		ReportBug("Infinite loop in GetVerbNondeadNext ")
		F = NULL;
	}
	return F;
}

FACT* GetObjectNondeadNext(FACT* F) 
{
	FACT* G = F;
	while (F) 
	{
		F =  Index2Fact(F->objectNext);
		if (F && !UnacceptableFact(F,true)) break;
	}
	if (F == G)
	{
		ReportBug("Infinite loop in GetObjectNondeadNext ")
		F = NULL;
	}	
	return F;
}

FACT* GetSubjectNondeadHead(FACT* F) 
{
	F = Index2Fact(F->subjectHead);
	while (F && UnacceptableFact(F,true)) 
	{
		F =  Index2Fact(F->subjectNext);
	}
	return F;
}

FACT* GetVerbNondeadHead(FACT* F) 
{
	F = Index2Fact(F->verbHead);
	while (F && UnacceptableFact(F,true)) 
	{
		F =  Index2Fact(F->verbNext);
	}
	return F;
}

FACT* GetObjectNondeadHead(FACT* F) 
{
	F = Index2Fact(F->objectHead);
	while (F && UnacceptableFact(F,true)) 
	{
		F =  Index2Fact(F->objectNext);
	}
	return F;
}

FACT* GetSubjectNondeadHead(WORDP D, bool jsonaccess) // able to get special marker empty fact for json units
{
	if (!D) return NULL;
	FACT* F = Index2Fact(D->subjectHead);
	while (F && UnacceptableFact(F,jsonaccess)) 
	{
		F =  Index2Fact(F->subjectNext);
	}

	return F;
}

FACT* GetVerbNondeadHead(WORDP D) 
{
	if (!D) return NULL;
	FACT* F = Index2Fact(D->verbHead);
	while (F && UnacceptableFact(F,true)) 
	{
		F =  Index2Fact(F->verbNext);
	}
	return F;
}

FACT* GetObjectNondeadHead(WORDP D)  
{
	if (!D) return NULL;
	FACT* F = Index2Fact(D->objectHead);
	while (F && UnacceptableFact(F,true)) 
	{
		F =  Index2Fact(F->objectNext);
	}
	return F;
}


void SetSubjectNext(FACT* F, FACT* value){ F->subjectNext = Fact2Index(value);}
void SetVerbNext(FACT* F, FACT* value) {F->verbNext = Fact2Index(value);}
void SetObjectNext(FACT* F, FACT* value){ F->objectNext = Fact2Index(value);}

FACTOID Fact2Index(FACT* F) 
{ 
	return  (F) ? ((FACTOID)(F - factBase)) : 0;
}

FACT* FactTextIndex2Fact(char* id) // given number word, get corresponding fact
{
	char word[MAX_WORD_SIZE];
	strcpy(word,id);
	char* comma = word;
	while ((comma = strchr(comma,','))) memmove(comma,comma+1,strlen(comma));  // maybe number has commas, like 100,000,300 . remove them
	unsigned int n = atoi(word);
	if (n <= 0) return NULL;
	return (n <= (unsigned int)(factFree-factBase)) ? Index2Fact(n) : NULL;
}

int GetSetID(char* x)
{
	if (*x != '@') return ILLEGAL_FACTSET;
	if (!IsDigit(*++x)) return ILLEGAL_FACTSET; 
	unsigned int n = *x - '0';
	if (IsDigit(*++x)) n = (n * 10) + *x - '0';
	// allow additional characters naming a field
	return (n > MAX_FIND_SETS) ? ILLEGAL_FACTSET : n;
}

bool GetSetMod(char* x)
{
	if (!IsDigit(*++x)) return false; // illegal set
	if (IsDigit(*++x)) ++x;
	if ((*x == 's' || *x == 'S' ) && strnicmp(x,(char*)"subject",7)) return false;
	if ((*x == 'v' || *x == 'V' )&& strnicmp(x,(char*)"verb",4)) return false;
	if ((*x == 'o' || *x == 'O' )&& strnicmp(x,(char*)"object",6)) return false;
	if ((*x == 'a' || *x == 'A' )&& strnicmp(x,(char*)"all",4)) return false;
	if ((*x == 'f'|| *x == 'F' ) && strnicmp(x,(char*)"fact",4)) return false;
	return true;
}

char* GetSetType(char* x)
{ // @13subject returns subject
	if (!x[0] || !x[1] ) return ""; // safety from bad data
	x += 2;
	if (IsDigit(*x)) ++x;
	return x;
}

char* GetSetEnd(char* x)
{ // @adagsubject returns subject
	char* start = x;
	while (IsAlphaUTF8(*++x)){;} // find natural end.
	size_t len = x - start;	 // this long
	if (len >= 9 && !strnicmp(x-7,(char*)"subject",7)) x -= 7;
	else if (len >= 6 && !strnicmp(x-4,(char*)"verb",4)) x -= 4;
	else if (len >= 8 && !strnicmp(x-6,(char*)"object",6)) x -= 6;
	else if (len >= 6 && !strnicmp(x-4,(char*)"fact",4)) x -= 4;
	else if (len >= 5 && !strnicmp(x-3,(char*)"all",3)) x -= 3;

	if (IsDigit(*x)) ++x;
	return x; 
}

void TraceFact(FACT* F,bool ignoreDead)
{
	char* limit;
	char* word = InfiniteStack(limit,"TraceFact"); // short term
	Log(STDUSERLOG,(char*)"%d: %s\r\n",Fact2Index(F),WriteFact(F,false,word,ignoreDead,false,true));
	Log(STDTRACETABLOG,(char*)"");
	ReleaseInfiniteStack();
} 

void ClearUserFacts()
{
	while (factFree > factLocked)  FreeFact(factFree--); //   erase new facts
}

void InitFacts()
{
	*traceSubject = 0;
	*traceVerb = 0;
	*traceObject = 0;

	if ( factBase == 0) 
	{
		factBase = (FACT*) malloc(maxFacts * sizeof(FACT)); // only on 1st startup, not on reload
		if ( factBase == 0)
		{
			(*printer)((char*)"%s",(char*)"failed to allocate fact space\r\n");
			myexit((char*)"failed to get fact space");
		}
	}
	memset(factBase,0,sizeof(FACT) *  maxFacts); // not strictly necessary
    factFree = factBase; // 1st fact is always meaningless
	factEnd = factBase + maxFacts;
}

void InitFactWords()
{
	//   special internal fact markers
	Mmember = MakeMeaning(StoreWord((char*)"member",AS_IS));
	Mexclude = MakeMeaning(StoreWord((char*)"exclude", AS_IS));
	Mis = MakeMeaning(StoreWord((char*)"is", AS_IS));
}

void CloseFacts()
{
    free(factBase);
	factBase = 0;
}

void FreeFact(FACT* F)
{ //   most recent facts are always at the top of any xref list. Can only free facts sequentially backwards.
	if (!F->subject) // unindexed fact recording a fact delete that must be undeleted
	{
		if (!F->object) // death of fact fact (verb but no object)
		{
			F = Index2Fact(F->verb);
			F->flags &= -1 ^ FACTDEAD;
		}
		else if (F->flags & ITERATOR_FACT) {;} // holds iterator backtrack
		else // undo a variable assignment fact (verb and object)
		{
			WORDP D = Meaning2Word(F->verb); // the variable
			unsigned int offset = (unsigned int) F->object;
			D->w.userValue =  (offset == 1) ? NULL : (heapBase + offset);
		}
	}
    else // normal indexed fact
	{
		if (!(F->flags & FACTSUBJECT)) SetSubjectHead(F->subject,GetSubjectNext(F)); // dont use nondead
		else SetSubjectHead(Index2Fact(F->subject),GetSubjectNext(F)); // dont use nondead

		if (!(F->flags & FACTVERB)) SetVerbHead(F->verb,GetVerbNext(F)); // dont use nondead
		else  SetVerbHead(Index2Fact(F->verb),GetVerbNext(F)); // dont use nondead

		if (!(F->flags & FACTOBJECT)) SetObjectHead(F->object,GetObjectNext(F)); // dont use nondead
		else SetObjectHead(Index2Fact(F->object),GetObjectNext(F)); // dont use nondead
	}
 }

unsigned int AddFact(unsigned int set, FACT* F) // fact added to factset
{
	unsigned int count = FACTSET_COUNT(set);
	if (++count > MAX_FIND) --count; 
	factSet[set][count] = F;
	SET_FACTSET_COUNT(set,count);
	return count;
}

FACT* SpecialFact(FACTOID_OR_MEANING verb, FACTOID_OR_MEANING object,unsigned int flags)
{
	//   allocate a fact
	if (++factFree == factEnd) 
	{
		--factFree;
        if (loading || compiling) ReportBug((char*)"FATAL:  out of fact space at %d", Fact2Index(factFree))
        ReportBug((char*)"out of fact space at %d",Fact2Index(factFree))
		(*printer)((char*)"%s",(char*)"out of fact space");
		return factFree; // dont return null because we dont want to crash anywhere
	}
	//   init the basics
	memset(factFree,0,sizeof(FACT));
	factFree->verb = verb;
	factFree->object = object;
	factFree->flags = FACTTRANSIENT | FACTDEAD | flags;	// cannot be written or kept
	return factFree;
}

static void ReleaseDictWord(WORDP D,WORDP dictbase)
{
	char c = *D->word;
	// dont erase interally used words
	if (c == '$' || c == '~' || c == '^' || c == '%' || c == '#') return;
	if (D >= dictbase) // we dont have to protect this
	{
		if (!(D->internalBits & BEEN_HERE) && !GetSubjectNondeadHead(D))
		{
			D->word = " "; // kill off its name
		}
	}
}

static void MarkDictWord(WORDP D, WORDP dictbase)
{ // insure this word is safe from destruction
	if (D >= dictbase) 
		D->internalBits |= BEEN_HERE;
}

void RipFacts(FACT* F,WORDP dictBase)
{
	FACT* base = factFree++;
	while (--factFree > F) // mark all still used new dict words
	{
		if (!(F->flags & FACTDEAD))
		{
			MarkDictWord(Meaning2Word(factFree->subject), dictBase);
			MarkDictWord(Meaning2Word(factFree->verb), dictBase);
			MarkDictWord(Meaning2Word(factFree->object), dictBase);
		}
	}
	factFree = base + 1;
	while (--factFree > F)
	{
		FreeFact(factFree); // remove all links from dict
		if (F->flags & FACTDEAD) // release 
		{
			ReleaseDictWord(Meaning2Word(factFree->subject), dictBase);
			ReleaseDictWord(Meaning2Word(factFree->verb), dictBase);
			ReleaseDictWord(Meaning2Word(factFree->object), dictBase);
		}
	}
	factFree = base;
}

FACT* WeaveFact(FACT* current)
{
    unsigned int properties = current->flags;
	FACT* F;
	WORDP s = (properties & FACTSUBJECT) ? NULL : Meaning2Word(current->subject);
	WORDP v = (properties & FACTVERB) ? NULL : Meaning2Word(current->verb);
	WORDP o = (properties & FACTOBJECT) ? NULL : Meaning2Word(current->object);

	//   crossreference
	if (s)
	{
		SetSubjectNext(current, GetSubjectHead(current->subject)); // dont use nondead
		SetSubjectHead(current->subject, current);
	}
	else
	{
		F = Index2Fact(current->subject);
		if (F)
		{
			SetSubjectNext(current, GetSubjectHead(F));   // dont use nondead
			SetSubjectHead(F, current);
		}
		else return NULL;
	}
	if (v)
	{
		SetVerbNext(current, GetVerbHead(current->verb));  // dont use nondead
		SetVerbHead(current->verb, current);
	}
	else
	{
		F = Index2Fact(current->verb);
		if (F)
		{
			SetVerbNext(current, GetVerbHead(F));  // dont use nondead
			SetVerbHead(F, current);
		}
		else return NULL;
	}
	if (o)
	{
		SetObjectNext(current, GetObjectHead(current->object));  // dont use nondead
		SetObjectHead(current->object, current);
	}
	else
	{
		F = Index2Fact(current->object);
		if (F)
		{
			SetObjectNext(current, GetObjectNext(F));   // dont use nondead
			SetObjectHead(F, current);
		}
		else return NULL;
	}
	return current;
}

void UnweaveFactSubject(FACT* F)
{
    FACT* listHead = DeleteFromList(GetSubjectHead(F->subject), F, GetSubjectNext, SetSubjectNext);  // dont use nondead
    SetSubjectHead(F->subject, listHead); // removed from dict element
}

void UnweaveFactVerb(FACT* F)
{
    FACT* listHead = DeleteFromList(GetVerbHead(F->verb), F, GetVerbNext, SetVerbNext);  // dont use nondead
    SetVerbHead(F->verb, listHead); // removed from dict element
}

void UnweaveFactObject(FACT* F)
{
    FACT* listHead = DeleteFromList(GetObjectHead(F->object), F, GetObjectNext, SetObjectNext);  // dont use nondead
    SetObjectHead(F->object, listHead); // removed from dict element
}

void UnweaveFact(FACT* F)
{// decouple fact from dictionary links to it
    UnweaveFactSubject(F);
    UnweaveFactVerb(F);
    UnweaveFactObject(F);
}

void WeaveFacts(FACT* F)
{
    --F;
	while (++F <= factFree) WeaveFact(F);
}

void AutoKillFact(MEANING M)
{
	int id = atoi(Meaning2Word(M)->word);
	KillFact(Index2Fact(id));
}

void KillFact(FACT* F,bool jsonrecurse, bool autoreviseArray)
{
	if (!F || F->flags & FACTDEAD) return; // already dead
	
	if (F <= factsPreBuild[currentBeforeLayer] && !allowBootKill)
		return;	 // may not kill off facts built into world

	F->flags |= FACTDEAD;

	if (trace & TRACE_FACT && CheckTopicTrace()) 
	{
		Log(STDUSERLOG,(char*)"Kill: ");
		TraceFact(F);
	}
	if (F->flags & FACTAUTODELETE)
	{
		if (F->flags & FACTSUBJECT) AutoKillFact(F->subject);
		if (F->flags & FACTVERB) AutoKillFact(F->verb);
		if (F->flags & FACTOBJECT) AutoKillFact(F->object);
	}

	// recurse on JSON datastructures below if they are being deleted on right side
	if (F->flags & (JSON_ARRAY_VALUE | JSON_OBJECT_VALUE) && jsonrecurse)
	{
		WORDP jsonarray = Meaning2Word(F->object);
		// should it recurse to kill guy refered to?
		// if no other living fact refers to it, you can also kill the referred object/array
		FACT* H = GetObjectNondeadHead(jsonarray);  // facts which link to this
		if (!H) jkillfact(jsonarray);
	}
	if (autoreviseArray && F->flags & JSON_ARRAY_FACT) JsonRenumber(F);// have to renumber this array

	if (planning) SpecialFact(Fact2Index(F),0,0); // save to restore

	// if this fact has facts depending on it, they too must die
	FACT* G = GetSubjectNondeadHead(F);
	while (G)
	{
		KillFact(G);
		G = GetSubjectNondeadNext(G);
	}
	G = GetVerbNondeadHead(F);
	while (G)
	{
		KillFact(G);
		G = GetVerbNondeadNext(G);
	}
	G = GetObjectNondeadHead(F);
	while (G)
	{
		KillFact(G);
		G = GetObjectNondeadNext(G);
	}
}

void ResetFactSystem(FACT* locked)
{
	while (factFree > locked) FreeFact(factFree--); // restore to end of basic facts
	for (unsigned int i = 0; i <= MAX_FIND_SETS; ++i)  // clear any factset with contaminated data
	{
		unsigned int limit = FACTSET_COUNT(i);
		for (unsigned int k = 1; k <= limit; ++k)
		{
			if (factSet[i][k] > locked) 
			{
				SET_FACTSET_COUNT(i, 0); // empty all facts since set is contaminated
				break;
			}
		}
	}
}

FACT* FindFact(FACTOID_OR_MEANING subject, FACTOID_OR_MEANING verb, FACTOID_OR_MEANING object, unsigned int properties)
{
    FACT* F;
	FACT* G;
	if (!subject || !verb || !object) return NULL;
	if (properties & FACTDUPLICATE) 
		return NULL;	// can never find this since we are allowed to duplicate and we do NOT want to share any existing fact

    //   see if  fact already exists. if so, just return it 
	if (properties & FACTSUBJECT)
	{
		F  = Index2Fact(subject);
		G = GetSubjectNondeadHead(F);
		while (G)
		{
			if (G->verb == verb &&  G->object == object && G->flags == properties  && G->botBits == myBot && !(G->flags & FACTDEAD)) return G;
			G  = GetSubjectNondeadNext(G);
		}
		return NULL;
	}
	else if (properties & FACTVERB)
	{
		F  = Index2Fact(verb);
		G = GetVerbNondeadHead(F);
		while (G)
		{
			if (G->subject == subject && G->object == object &&  G->flags == properties  && G->botBits == myBot && !(G->flags & FACTDEAD)) return G;
			G  = GetVerbNondeadNext(G);
		}
		return NULL;
	}   
 	else if (properties & FACTOBJECT)
	{
		F  = Index2Fact(object);
		G = GetObjectNondeadHead(F);
		while (G)
		{
			if (G->subject == subject && G->verb == verb &&  G->flags == properties && G->botBits == myBot && !(G->flags & FACTDEAD)) return G;
			G  = GetObjectNondeadNext(G);
		}
		return NULL;
	}
  	//   simple FACT* based on dictionary entries
	F = GetSubjectNondeadHead(Meaning2Word(subject));
    while (F)
    {
		if ( F->subject == subject &&  F->verb ==  verb && F->object == object && properties == F->flags && F->botBits == myBot && !(F->flags & FACTDEAD))  return F;
		F = GetSubjectNondeadNext(F);
    }
    return NULL;
}

FACT* CreateFact(FACTOID_OR_MEANING subject, FACTOID_OR_MEANING verb, FACTOID_OR_MEANING object, unsigned int properties)
{
	currentFact = NULL; 
	if (!subject || !object || !verb)
	{
		if (!subject) 
			ReportBug((char*)"Missing subject field in fact create at line %d of %s",currentFileLine,currentFilename)
		if (!verb) 
			ReportBug((char*)"Missing verb field in fact create at line %d of %s",currentFileLine,currentFilename)
		if (!object) 
			ReportBug((char*)"Missing object field in fact create at line %d of %s",currentFileLine,currentFilename)
		return NULL;
	}

	//   get correct field values
    WORDP s = (properties & FACTSUBJECT) ? NULL : Meaning2Word(subject);
    WORDP v = (properties & FACTVERB) ? NULL : Meaning2Word(verb);
	WORDP o = (properties & FACTOBJECT) ? NULL : Meaning2Word(object);
    if (s && *s->word == 0)
	{
		ReportBug((char*)"bad choice in fact subject")
		return NULL;
	}
	if (v && *v->word == 0)
	{
		ReportBug((char*)"bad choice in fact verb")
		return NULL;
	}
	if (o && *o->word == 0)
	{
		ReportBug((char*)"bad choice in fact object")
		return NULL;
	}

	// convert any restricted meaning
	MEANING other;
	char* word;
	if (s && (word = s->word) && *word != '"' && strchr(word+1,'~')) // has number or word after it? convert notation
	{
		other = ReadMeaning(word,true,true);
		if (Meaning2Word(other) == s) {;}
		else if (Meaning2Index(other) || GETTYPERESTRICTION(other)) subject = other;
	}
	if (v && (word = v->word) && strchr(word+1,'~')) // has number or word after it? convert notation
	{
		other = ReadMeaning(word,true,true);
		if (Meaning2Word(other) == v) {;}
		else if (Meaning2Index(other) || GETTYPERESTRICTION(other)) verb = other;
	}
	if (o && (word = o->word) &&  strchr(word+1,'~')) // has number or word after it? convert notation
	{
		other = ReadMeaning(word,true,true);
		if (Meaning2Word(other) == o) {;}
		else if (Meaning2Index(other) || GETTYPERESTRICTION(other)) object = other;
	}
	if (*traceSubject)
	{
		if (s && !stricmp(s->word,traceSubject) && *traceVerb && *traceObject)
		{
			DebugCode(NULL);
		}
		else if (s && !stricmp(s->word,traceSubject) && v && !stricmp(s->word,traceVerb) && *traceObject)
		{
			DebugCode(NULL);
		}
		else if (s && !stricmp(s->word,traceSubject) && o && !stricmp(s->word,traceObject) && *traceVerb)
		{
			DebugCode(NULL);
		}
		else if (s && !stricmp(s->word,traceSubject) && v && !stricmp(s->word,traceVerb) && o && !stricmp(s->word,traceObject))
		{
			DebugCode(NULL);
		}
	}
	if (*traceVerb)
	{
		if (v && !stricmp(v->word,traceVerb) && *traceSubject && *traceObject)
		{
			DebugCode(NULL);
		}
		else if (v && !stricmp(s->word,traceVerb) && *traceSubject  && o && !stricmp(s->word,traceObject))
		{
			DebugCode(NULL);
		}
	}
	if (*traceObject)
	{
		if (o &&  !stricmp(o->word,traceObject)  && *traceSubject && *traceVerb)
		{
			DebugCode(NULL);
		}
	}

	if (compiling)
	{
		if (buildId == BUILD1) properties |= FACTBUILD1;
	}

	//   insure fact is unique if requested
	currentFact =  (properties & FACTDUPLICATE) ? NULL : FindFact(subject,verb,object,properties); 
	if (trace & TRACE_FACT && currentFact && CheckTopicTrace())  Log(STDUSERLOG,(char*)" Found %d ", Fact2Index(currentFact));
	if (currentFact) return currentFact;

	currentFact = CreateFastFact(subject,verb,object,properties);
	if (trace & TRACE_FACT && currentFact && CheckTopicTrace())  
	{
        if (trace & TRACE_FACT && trace & TRACE_JSON && !(trace & TRACE_OUTPUT)) 
            TraceFact(currentFact, false); // precise trace for JSON
		Log(STDUSERLOG,(char*)" Created %d\r\n", Fact2Index(currentFact));
		Log(STDTRACETABLOG,(char*)"");
	}

	return  currentFact;
}

bool ExportFacts(char* name, int set,char* append)
{ 
	if (set < 0 || set > MAX_FIND_SETS) return false;
	if ( *name == '"')
	{
		++name;
		size_t len = strlen(name);
		if (name[len-1] == '"') name[len-1] = 0;
	}

	char* buffer = GetFreeCache();
	char* base = buffer;

	char* limit;
	char* word = InfiniteStack(limit,"ExportFacts"); //short term
	unsigned int count = FACTSET_COUNT(set);
	for (unsigned int i = 1; i <= count; ++i)
	{
		FACT* F = factSet[set][i];
		if (!F) strcpy(buffer,(char*)"null ");
		else if ( !(F->flags & FACTDEAD))
		{
			unsigned int original = F->flags;
			F->flags &= -1 ^ FACTTRANSIENT;	// dont pass transient flag out
			strcpy(buffer,WriteFact(F,false,word,false,true));
			F->flags = original;
		}
		buffer += strlen(buffer);
	}
	ReleaseInfiniteStack();

	FILE* out;
	if (strstr(name,"ltm")) out = userFileSystem.userCreate(name); // user ltm file
	else out = (append && !stricmp(append,"append")) ? FopenUTF8WriteAppend(name) : FopenUTF8Write(name);
	if (out)
	{
		if (strstr(name,"ltm")) 
		{
			EncryptableFileWrite(base,1,(buffer-base),out,ltmEncrypt,"LTM"); 
			userFileSystem.userClose(out);
		}
		else
		{
			fprintf(out,(char*)"%s",base);
			fclose(out);
		}
	}
	FreeUserCache();
	return (out) ? true : false;
}

static char* ExportJson1(char* jsonitem, char* buffer)
{
	if (!*jsonitem)
	{
		strcpy(buffer,(char*)"null\r\n");
		return buffer + strlen(buffer);
	}

	WORDP D = FindWord(jsonitem);
	if (!D) return buffer;
	FACT* F = GetSubjectNondeadHead(D);
	while (F)
	{
		if ( !(F->flags & FACTDEAD))
		{
			unsigned int original = F->flags;
			F->flags &= -1 ^ FACTTRANSIENT;	// dont pass transient flag out
			WriteFact(F,false,buffer,false,true);
			buffer += strlen(buffer);
			F->flags = original;
		}
		if (F->flags & (JSON_ARRAY_VALUE|JSON_OBJECT_VALUE)) buffer = ExportJson1(Meaning2Word(F->object)->word,buffer); // on object side of triple

		F = GetSubjectNondeadNext(F);
	}
	return buffer + strlen(buffer);
}

FunctionResult ExportJson(char* name, char* jsonitem, char* append)
{
	WORDP D;
	if (*jsonitem) 
	{
		D = FindWord(jsonitem);
		if (!D) return FAILRULE_BIT;
	}
	if ( *name == '"')
	{
		++name;
		size_t len = strlen(name);
		if (name[len-1] == '"') name[len-1] = 0;
	}

	FILE* out;
	if (strstr(name,"ltm")) out = userFileSystem.userCreate(name); // user ltm file
	else out = (append && !stricmp(append,"append")) ? FopenUTF8WriteAppend(name) : FopenUTF8Write(name);
	if (!out) return FAILRULE_BIT;

	char* buffer = GetFreeCache(); // filesize limit
	ExportJson1(jsonitem, buffer);
	if (strstr(name,"ltm"))
	{
		EncryptableFileWrite(buffer,1,strlen(buffer),out,ltmEncrypt,"LTM"); 
		userFileSystem.userClose(out);
	}
	else
	{
		fprintf(out,(char*)"%s",buffer);
		fclose(out);
	}
	FreeUserCache();
	return NOPROBLEM_BIT;
}

char* EatFact(char* ptr,char* buffer,unsigned int flags,bool attribute)
{
	ptr = SkipWhitespace(ptr); // could be user-formateed, dont trust
	FunctionResult result = NOPROBLEM_BIT;
	//   subject
	if (*ptr == '(') //   nested fact
	{
		ptr = EatFact(ptr+1,buffer); //   returns after the closing paren
		flags |= FACTSUBJECT;
		sprintf(buffer,(char*)"%d",currentFactIndex() ); //   created OR e found instead of created
	}
	else  ptr = GetCommandArg(ptr,buffer,result,OUTPUT_FACTREAD); //   subject
	ptr = SkipWhitespace(ptr); // could be user-formateed, dont trust
	if (!ptr) return NULL;
	if (result & ENDCODES) return ptr;
	if (!*buffer) 
	{
		if (!ptr) return NULL;
		if (compiling) BADSCRIPT((char*)"FACT-1 Missing subject for fact create")
		char* end = strchr(ptr,')');
		return (end) ? (end + 2) : ptr; 
	}
	char* word = AllocateStack(buffer);
	*buffer = 0;

	//verb
	if (*ptr == '(') //   nested fact
	{
		ptr = EatFact(ptr+1,buffer);
		flags |= FACTVERB;
		sprintf(buffer,(char*)"%d",currentFactIndex() );
	}
	else  ptr = GetCommandArg(ptr,buffer,result,OUTPUT_FACTREAD); //verb
	if (!ptr)
	{
		ReleaseStack(word);
		return NULL;
	}
	ptr = SkipWhitespace(ptr); // could be user-formateed, dont trust
	if (result & ENDCODES)
	{
		ReleaseStack(word);
		return ptr;
	}
	if (!*buffer) 
	{
		ReleaseStack(word);
		if (!ptr) return NULL;
		if (compiling) BADSCRIPT((char*)"FACT-2 Missing verb for fact create")
		char* end = strchr(ptr,')');
		return (end) ? (end + 2) : ptr; 
	}
	char* word1 = AllocateStack(buffer);
	*buffer = 0;

	//   object
	if (*ptr == '(') //   nested fact
	{
		ptr = EatFact(ptr+1,buffer);
		flags |= FACTOBJECT;
		sprintf(buffer,(char*)"%d",currentFactIndex() );
	}
	else  ptr = GetCommandArg(ptr,buffer,result,OUTPUT_FACTREAD); 
	ptr = SkipWhitespace(ptr); // could be user-formateed, dont trust
	if (result & ENDCODES)
	{
		ReleaseStack(word);
		return ptr;
	}
	if (!*buffer) 
	{
		ReleaseStack(word);
		if (compiling) BADSCRIPT((char*)"FACT-3 Missing object for fact create - %s",readBuffer)
		if (!ptr) return NULL;
		char* end = strchr(ptr,')');
		return (end) ? (end + 2) : ptr; 
	}
	char* word2 = AllocateStack(buffer);
	*buffer = 0;

	uint64 fullflags;
	bool bad = false;
	bool response = false;
	char strip[MAX_WORD_SIZE];
	char* at = ReadCompiledWord(ptr,strip);
	bool stripquote = false;
	if (!strnicmp(strip,(char*)"STRIP_QUOTE",11)) 
	{
		stripquote = true;
		ptr = at;
	}
	ptr = ReadFlags(ptr,fullflags,bad,response);
	flags |= (unsigned int) fullflags;

	MEANING subject;
	if ( flags & FACTSUBJECT)
	{
		subject = atoi(word);
		if (!subject) subject = 1; // make a phony fact
	}
	else if (stripquote)
	{
		size_t len = strlen(word);
		if (*word == '"' && word[len-1] == '"') 
		{
			word[len-1] = 0;
			memmove(word,word+1,strlen(word));
		}
		subject =  MakeMeaning(StoreWord(word,AS_IS),0);
	}
	else subject =  MakeMeaning(StoreWord(word,AS_IS),0);
	MEANING verb;
	if ( flags & FACTVERB)
	{
		verb = atoi(word1);
		if (!verb) verb = 1; // make a phony fact
	}
	else if (stripquote)
	{
		size_t len = strlen(word1);
		if (*word1 == '"' && word1[len-1] == '"') 
		{
			word1[len-1] = 0;
			memmove(word1,word1+1,strlen(word1));
		}
		verb =  MakeMeaning(StoreWord(word1,AS_IS),0);
	}
	else verb =  MakeMeaning(StoreWord(word1,AS_IS),0);
	MEANING object;
	if ( flags & FACTOBJECT)
	{
		object = atoi(word2);
		if (!object) object = 1; // make a phony fact
	}
	else if (stripquote)
	{
		size_t len = strlen(word2);
		if (*word2 == '"' && word2[len-1] == '"') 
		{
			word2[len-1] = 0;
			memmove(word2,word2+1,strlen(word2));
		}
		object =  MakeMeaning(StoreWord(word2,AS_IS),0);
	}
	else object =  MakeMeaning(StoreWord(word2,AS_IS),0);

	if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(STDUSERLOG,(char*)"%s %s %s %x) = ",word,word1,word2,flags);

	FACT* F = FindFact(subject,verb,object,flags);
	if (!attribute || (F && object == F->object)) {;}  // not making an attribute or already made
	else // remove any facts with same subject and verb, UNlESS part of some other fact
	{
		if (flags & FACTSUBJECT) F = GetSubjectNondeadHead(Index2Fact(subject));
		else F = GetSubjectNondeadHead(subject);
		while (F)
		{
			if (F->subject == subject && F->verb == verb) 
			{
				if (!(F->flags & FACTATTRIBUTE)) // this is a script failure. Should ONLY be an attribute
				{
					char wordx[MAX_WORD_SIZE];
					WriteFact(F,false,wordx,false,true);
					Log(STDUSERLOG,(char*)"Fact created is not an attribute. There already exists %s",wordx); 
					(*printer)((char*)"Fact created is not an attribute. There already exists %s",wordx); 
					currentFact = F;
					return (*ptr) ? (ptr + 2) : ptr; 
				}
				KillFact(F); 
			}
			F = GetSubjectNondeadNext(F);
		}
	}

	F = CreateFact(subject,verb,object,flags);
	if (attribute) 	F->flags |= FACTATTRIBUTE;

	ReleaseStack(word);
	return (*ptr) ? (ptr + 2) : ptr; //   returns after the closing ) if there is one
}

bool ImportFacts(char* buffer,char* name, char* set, char* erase, char* transient)
{ // if no set is given, return the subject of the 1st fact we read (Json entity name)
	int store = -1;
	if (*set && *set != '"') // able to pass empty string
	{
		store = GetSetID(set);
		if (store == ILLEGAL_FACTSET) return false;
		SET_FACTSET_COUNT(store,0);
	}
	if ( *name == '"')
	{
		++name;
		size_t len = strlen(name);
		if (name[len-1] == '"') name[len-1] = 0;
	}
	FILE* in;
	if (strstr(name,"ltm")) in = userFileSystem.userOpen(name);
	else in = FopenReadWritten(name);
	if (!in) return false;

	char* filebuffer = GetFreeCache();
	size_t readit;
	if (strstr(name,"ltm"))
	{
		readit = DecryptableFileRead(filebuffer,1,userCacheSize,in,ltmEncrypt,"LTM");	// LTM file read, read it all in, including BOM
		userFileSystem.userClose(in);
	}
	else
	{
		readit = fread(filebuffer,1,userCacheSize,in);	
		fclose(in);
	}
	filebuffer[readit] = 0;
	filebuffer[readit+1] = 0; // insure fully closed off

	// set bom
	maxFileLine = currentFileLine = 0;
	BOM = BOMSET; // will be utf8
	userRecordSourceBuffer = filebuffer;

	unsigned int flags = 0;
	if (!stricmp(erase,(char*)"transient") || !stricmp(transient,(char*)"transient")) flags |= FACTTRANSIENT; // make facts transient
	while (ReadALine(readBuffer, 0) >= 0)
    {
        if (*readBuffer == 0 || *readBuffer == '#') continue; //   empty or comment
		char word[MAX_WORD_SIZE];
		ReadCompiledWord(readBuffer,word);
		if (!stricmp(word,(char*)"null")) 
		{
			if (store >= 0) AddFact(store,NULL);
		}
		else if (*word == '(')
		{
			char* ptr = readBuffer;
			FACT* G = ReadFact(ptr,0);
			if (!G) 
			{
				ReportBug("Import data: %s",readBuffer);
				continue;
			}
			G->flags |= flags;
			if (store >= 0) AddFact(store,G);
			else if (!*buffer) strcpy(buffer,Meaning2Word(G->subject)->word); // to return the name
		}
	}
	FreeUserCache();
	userRecordSourceBuffer = NULL;

	if (!stricmp(erase,(char*)"erase") || !stricmp(transient,(char*)"erase")) remove(name); // erase file after reading
	if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(STDUSERLOG,(char*)"[%d] => ",FACTSET_COUNT(store));
	currentFact = NULL; // we have used up the facts
	return true;
}

void WriteFacts(FILE* out,FACT* F, int flags) //   write out from here to end
{ 
	if (!out) return;

    char* word = AllocateBuffer();
    while (++F <= factFree) 
	{
		if (!(F->flags & (FACTTRANSIENT|FACTDEAD))) 
		{
            if (UnacceptableFact(F, true)) continue;
			F->flags |= flags; // used to pass along build2 flag
			char* f = WriteFact(F, true, word, false, true);
			fprintf(out,(char*)"%s",f);
			F->flags ^= flags;
		}
	}
    FClose(out);
    FreeBuffer();
}

void WriteBinaryFacts(FILE* out,FACT* F) //   write out from here to end (dictionary entries)
{ 
	if (!out) return;
    while (++F <= factFree) 
	{
		unsigned int index = Fact2Index(F);
		if (F->flags & FACTSUBJECT && F->subject >= index) ReportBug((char*)"subject fact index too high")
		if (F->flags & FACTVERB && F->verb >= index) ReportBug((char*)"verb fact index too high")
		if (F->flags & FACTOBJECT && F->object >= index) ReportBug((char*)"object fact index too high")
		Write32(F->subject,out);
		Write32(F->verb,out);
		Write32(F->object,out);
		Write32(F->flags,out);
        // binary fact writing doesnt write botBits because are dict entries owned by id 0
    }
    FClose(out);
}

FACT* CreateFastFact(FACTOID_OR_MEANING subject, FACTOID_OR_MEANING verb, FACTOID_OR_MEANING object, unsigned int properties)
{
	//   get correct field values
	WORDP s = (properties & FACTSUBJECT) ? NULL : Meaning2Word(subject);
	WORDP v = (properties & FACTVERB) ? NULL : Meaning2Word(verb);
	WORDP o = (properties & FACTOBJECT) ? NULL : Meaning2Word(object);
	// DICTIONARY should never be build with any but simple meanings and Mis
	// No fact meaning should ever have a synset marker on it. And member facts may have type restrictions on them
	if (s && ((subject & (-1 ^ SIMPLEMEANING) && verb == Mis) || subject&SYNSET_MARKER))
	{
		int xx = 0;
	}
	if (o && ((object & (-1 ^ SIMPLEMEANING) && verb == Mis)|| subject&SYNSET_MARKER))
	{
		int xx = 0;
	}

	//   allocate a fact
    int n = factEnd - factFree;
    if (n < worstFactFree) worstFactFree = n;
	if (++factFree == factEnd) 
	{
		--factFree;
        if (loading || compiling) ReportBug((char*)"FATAL:  out of fact space at %d", Fact2Index(factFree))
        ReportBug((char*)"out of fact space at %d",Fact2Index(factFree))
		(*printer)((char*)"%s",(char*)"out of fact space");
        factsExhausted = true;
		return NULL;
	}
	currentFact = factFree;

	//   init the basics
	memset(currentFact,0,sizeof(FACT));
	currentFact->subject = subject;
	currentFact->verb = verb; 
	currentFact->object = object;
	currentFact->flags = properties;
	currentFact->botBits = myBot; // this fact is visible to these bot ids (0 or -1 means all)
	currentFact = WeaveFact(currentFact);
	if (!currentFact)
	{
		--factFree;
		return NULL;
	}

	if (planning) currentFact->flags |= FACTTRANSIENT;

	if (trace & TRACE_FACT && CheckTopicTrace())
	{
		char* limit;
		char* buffer = InfiniteStack(limit,"CreateFastFact"); // fact might be big, cant use mere WORD_SIZE
		buffer = WriteFact(currentFact,false,buffer,true,false,true);
		Log(STDUSERLOG,(char*)"create %s",buffer);
		ReleaseInfiniteStack();
	}	
	return currentFact;
}

bool ReadBinaryFacts(FILE* in) //   read binary facts
{ 
	if (!in) return false;
	while (ALWAYS)
	{
		MEANING subject = Read32(in);
		if (!subject) break;
 		MEANING verb = Read32(in);
 		MEANING object = Read32(in);
  		unsigned int properties = Read32(in);
		CreateFastFact(subject,verb,object,properties);
	}
    FClose(in);
	return true;
}

static char* WriteField(MEANING T, uint64 flags,char* buffer,bool ignoreDead, bool displayonly) // uses no additional memory
{
	char* xxstart = buffer;
	// a field is either a contiguous mass of non-blank tokens, or a user string "xxx" or an internal string `xxx`  (internal removes its ends, user doesnt)
    if (flags ) //   fact reference
    {
		FACT* G = Index2Fact(T);
		if (!*WriteFact(G,false,buffer,ignoreDead, false,displayonly))
		{
			*buffer = 0;
			return buffer;
		}
		buffer += strlen(buffer);
    }
	else if (!T) 
	{
		ReportBug((char*)"Missing fact field")
		*buffer++ = '?';
	}
    else 
	{
		WORDP D = Meaning2Word(T);
		if (D->internalBits & (INTERNAL_MARK|DELETED_MARK) && !ignoreDead) // a deleted field
		{
			*buffer = 0;
			return buffer; //   cancels print
		}
		char* answer = D->word;

		// characteristics of the data
		bool quoted = false;
		if (*answer == '"') // if it starts with ", does it end with "
		{
			size_t len = strlen(answer);
			if (answer[len-1] == '"') quoted = true;
		}
		bool embeddedbacktick = strchr(answer,'`') ? true : false;
		bool embeddedspace = !quoted && (strchr(answer,' ')  || strchr(answer,'(') || strchr(answer,')')); // does this need protection? blanks or function call maybe
		bool safe = true; 
		if (strchr(answer,'\\') || strchr(answer,'/')) safe = false;
		
		// json must protect: " \ /  nl cr tab  we are not currently protecting bs ff
		if ((embeddedbacktick || !safe) && !displayonly) // uses our own marker and can escape data
		{
			if (embeddedbacktick) strcpy(buffer,(char*)"`*^"); 
			else strcpy(buffer,(char*)"`"); // has blanks or paren or just starts with ", use internal string notation
			buffer += strlen(buffer);
			AddEscapes(buffer,D->word,displayonly,currentOutputLimit - (buffer - currentOutputBase)); // facts are not normal
			buffer += strlen(buffer);
			SuffixMeaning(T,buffer, true);
			buffer += strlen(buffer);
			if (embeddedbacktick) strcpy(buffer,(char*)"`*^"); // add closer marker
			else strcpy(buffer,(char*)"`");
		}
		else if (embeddedspace )   // has blanks, use internal string notation
		{
			*buffer++ = '`';
			WriteMeaning(T,true,buffer);
			buffer += strlen(buffer);
			*buffer++ = '`';
			*buffer = 0;
		}
		else WriteMeaning(T,true,buffer); // use normal notation
		buffer += strlen(buffer);
	}
	*buffer++ = ' ';
	*buffer = 0;
	return buffer;
}

char* WriteFact(FACT* F,bool comment,char* buffer,bool ignoreDead,bool eol,bool displayonly) // uses no extra memory
{ //   if fact is junk, return the null string
	char* start = buffer;
	*buffer = 0;
	if (!F || !F->subject) return start; // never write special facts out
	if (F->flags & FACTDEAD) // except for user display THIS shouldnt happen to real fact writes
	{
		if (ignoreDead)
		{
			strcpy(buffer,(char*)"DEAD ");
			buffer += strlen(buffer);
		}
		else 
			return ""; // illegal - only happens with facts (nondead) that refer to dead facts?
	}

	//   fact opener
	*buffer++ = '(';
	*buffer++ = ' ';

	//   do subject
	char* base = buffer;
 	buffer = WriteField(F->subject,F->flags & FACTSUBJECT,buffer,ignoreDead,displayonly);
	if (base == buffer ) 
	{
		*start = 0;
		return start; //    word itself was removed from dictionary
	}
	base = buffer;
	buffer = WriteField(F->verb,F->flags & FACTVERB,buffer,ignoreDead, displayonly);
	if (base == buffer ) 
	{
		*start = 0;
		return start; //    word itself was removed from dictionary
	}

	base = buffer;
	int autoflag = F->flags & FACTOBJECT;
	MEANING X = F->object;
	if (F->flags & FACTAUTODELETE && F->flags & (JSON_OBJECT_FACT| JSON_ARRAY_FACT)) 
	{
		autoflag = FACTOBJECT;
		X = atoi(Meaning2Word(F->object)->word);
	}
	buffer = WriteField(X,autoflag,buffer,ignoreDead, displayonly);
	if (base == buffer ) 
	{
		*start = 0;
		return start; //    word itself was removed from dictionary
	}

	//   add properties
    if (F->flags || F->botBits)  
	{
		sprintf(buffer,(char*)"x%x ",F->flags & (-1 ^ (MARKED_FACT|MARKED_FACT2) ));  // dont show markers
		buffer += strlen(buffer);
	}

	// add bot owner flags
	if (F->botBits)
	{
#ifdef WIN32
		sprintf(buffer,(char*)"%I64u ",F->botBits); 
#else
		sprintf(buffer,(char*)"%llu ",F->botBits); 
#endif
		buffer += strlen(buffer);
	}

	//   close fact
	*buffer++ = ')';
	*buffer = 0;
	if (eol) strcat(buffer,(char*)"\r\n");
	return start;
}

char* ReadField(char* ptr,char* &field,char fieldkind, unsigned int& flags)
{
	field = AllocateStack(NULL, maxBufferSize); // released above
	if (*ptr == '(')
	{
		FACT* G = ReadFact(ptr,(flags & FACTBUILD2) ? BUILD2 : 0);
		if (fieldkind == 's') flags |= FACTSUBJECT;
		else if (fieldkind == 'v') flags |= FACTVERB;
		else if (fieldkind == 'o') flags |= FACTOBJECT;
		if (!G)
		{
			ReportBug((char*)"Missing fact field")
			return NULL;
		}
		sprintf(field,(char*)"%d",Fact2Index(G)); 
	}
	else if (*ptr == ENDUNIT) // internal string token (fact read)
	{
		char* end;
		char* start;
		if (ptr[1] == '*' && ptr[2] == '^') // they use our ` in their field - we use `*^%s`*^
		{
			start = end = ptr+3;
			while ((end = strchr(end,'^')))
			{
				if (*(end-1) == '*' && *(end-2) == '`') break;  // found our close
				++end;
			}
			if (!end)
			{
				ReportBug("No end found");
				return NULL;
			}
			*(end-2) = 0; // terminate string with null
		}
		else
		{
			end = strchr(ptr+1,ENDUNIT); // find corresponding end
			start = ptr + 1;
			if (!*end)
			{
				ReportBug("No end found");
				return NULL;
			}
			*end = 0;
		}
		
		CopyRemoveEscapes(field,start,90000);	// we only remove ones we added
		return end+2; // point AFTER the space after the closer
	}
    else  ptr = ReadCompiledWord(ptr,field,false,false,true); // no escaping or anything weird needed
	if (field[0] == '~') MakeLowerCase(field);	// all concepts/topics are lower case
	return ptr; //   return at new token
}

FACT* ReadFact(char* &ptr, unsigned int build)
{
	char word[MAX_WORD_SIZE];
    MEANING subject = 0;
	MEANING verb = 0;
    MEANING object = 0;
 	//   fact may start indented.  Will start with (or be 0 for a null fact
    ptr = ReadCompiledWord(ptr,word);
    if (*word == '0') return 0; 
	unsigned int flags = 0;
	if (build == BUILD2) flags |= FACTBUILD2;
	else if (build == BUILD1) flags |= FACTBUILD1;
	char* subjectname;
	ptr = ReadField(ptr,subjectname,'s',flags);
    char* verbname;
	ptr = ReadField(ptr,verbname,'v',flags);
    char* objectname;
	ptr = ReadField(ptr,objectname,'o',flags);

	if (!ptr) 
	{
		ReleaseStack(subjectname);
		return NULL;
	}
	
    //   handle the flags on the fact
    uint64 properties = 0;
	bool bad = false;
	bool response = false;
    if (!*ptr || *ptr == ')' ); // end of fact
	else ptr = ReadFlags(ptr,properties,bad,response,true);
	flags |= (unsigned int) properties;

    if (flags & FACTSUBJECT) subject = (MEANING) atoi(subjectname);
    else  subject = ReadMeaning(subjectname,true,true);
	if (flags & FACTVERB) verb = (MEANING) atoi(verbname);
	else  verb = ReadMeaning(verbname,true,true);
	if (flags & FACTOBJECT) 
	{
		if (flags & FACTAUTODELETE && flags & (JSON_OBJECT_FACT| JSON_ARRAY_FACT)) 
		{
			object = ReadMeaning(objectname,true,true);
			flags ^= FACTOBJECT;
		}
		else object = (MEANING) atoi(objectname);
	}
	else  object = ReadMeaning(objectname,true,true);
	ReleaseStack(subjectname);
	uint64 oldbot = myBot;
	myBot = 0; // no owner by default unless read in by fact
	if (*ptr && *ptr != ')') ptr = ReadInt64(ptr,(int64&)myBot); // read bot bits

    FACT* F = FindFact(subject, verb,object,flags);
    if (!F)   F = CreateFact(subject,verb,object,flags); 
	
	if (*ptr == ')') ++ptr;	// skip over ending )
	ptr = SkipWhitespace(ptr);
	myBot = oldbot;
    return F;
}

void ReadFacts(const char* name,const char* layer,unsigned int build,bool user) //   a facts file may have dictionary augmentations and variable also
{
  	char word[MAX_WORD_SIZE];
	if (layer) sprintf(word,"%s/%s",topic,name);
	else strcpy(word,name);
    FILE* in = (user) ? FopenReadWritten(word) : FopenReadOnly(word); //  TOPIC fact/DICT files
	if (!in && layer) 
	{
		sprintf(word,"%s/BUILD%s/%s",topic,layer,name);
		in = FopenReadOnly(word);
	}
	if (!in) return;
	StartFile(name);
    while (ReadALine(readBuffer, in) >= 0)
    {
		char* ptr = ReadCompiledWord(readBuffer,word);
		if (*word == 0 || (*word == '#' && word[1] != '#')); //   empty or comment
		else if (*word == '+') //   dictionary entry
		{
			if (!stricmp(word,(char*)"+query")) // defining a private query
			{
				ptr = ReadCompiledWord(ptr,word); // name
				WORDP D = StoreWord(word);
				AddInternalFlag(D,(unsigned int)(QUERY_KIND|build));
				ReadCompiledWord(ptr,word);
				ptr = strchr(word+1,'"');
				*ptr = 0;
				D->w.userValue = AllocateHeap(word+1);
				continue;
			}

			char wordx[MAX_WORD_SIZE];
			char* at = ReadCompiledWord(readBuffer+2,wordx); //   start at valid token past space
			WORDP D = FindWord(wordx,0,PRIMARY_CASE_ALLOWED);
			if (!D || strcmp(D->word,wordx)) // alternate capitalization?
			{
				D = StoreWord(wordx);
				AddInternalFlag(D,(unsigned int)build);
			}
			bool sign = false;
			if (*at == '-' && IsDigit(at[1])) 
			{
				sign = true;
				++at;
			}
			if (IsDigit(*at)) // # has numeric value, is a rename number
			{
				int64 n;
				ReadInt64(at,n);
				AddProperty(D,(uint64) n);// rename value
				AddInternalFlag(D,RENAMED);
				if (sign) AddSystemFlag(D,CONSTANT_IS_NEGATIVE);
			}
			else ReadDictionaryFlags(D,at);
		}
		else if (*word == USERVAR_PREFIX) // variable
		{
			char* eq = strchr(word,'=');
			if (!eq) ReportBug((char*)"Bad fact file user var assignment %s",word)
			else 
			{
				*eq = 0;
				SetUserVariable(word,eq+1);
			}
		}
        else 
		{
			char* xptr = readBuffer;
			ReadFact(xptr,build); // will write on top of ptr... must not be readBuffer variable
		}
    }
   FClose(in);
}

void SortFacts(char* set, int alpha, int setpass) //   sort low to high ^sort(@1subject) which field we sort on (subject or verb or object)
{
    int n = (setpass >= 0) ? setpass : GetSetID(set);
	if (n == ILLEGAL_FACTSET) return;
	char kind = GetLowercaseData(*GetSetType(set));
	if (!kind) kind = 's';
    int i;
	int size = FACTSET_COUNT(n);
	double floatValues[MAX_FIND+1];
	char* wordValues[MAX_FIND+1];

	// load the actual array to compare
    for (i = 1; i <= size; ++i)
    {
        FACT* F =  factSet[n][i];
		char* word;
		if (kind == 's') word = Meaning2Word(F->subject)->word; 
		else if (kind == 'v') word = Meaning2Word(F->verb)->word; 
		else word = Meaning2Word(F->object)->word;
		if (alpha == 1) wordValues[i] = word;
		else if (alpha == 0) floatValues[i] = Convert2Double(word);
    }

	// do bubble sort 
    bool swap = true;
    while (swap)
    {
        swap = false;
        for (i = 1; i <= size-1; ++i) 
        {
			char* word = wordValues[i];
            double tmpfloat = floatValues[i];
			if (alpha == 1 && strcmp(word,wordValues[i+1]) > 0) {;}
			else if (alpha == 2 && (char*)factSet[n] > (char*)factSet[n+1]) {;}
			else if (alpha == 0 && tmpfloat > floatValues[i+1]) {;}
			else continue;
	        wordValues[i] = wordValues[i+1];
            wordValues[i+1] = word;
            floatValues[i] = floatValues[i+1];
            floatValues[i+1] = tmpfloat;
            FACT* F = factSet[n][i];
			factSet[n][i] = factSet[n][i+1];
			factSet[n][i+1] = F;
			swap = true;
		}
        --size;
    }
}

// Handle changes to system facts done by user

static uint64* WriteDEntry(uint64* data, WORDP D)
{
    *data++ = D->properties;
    *data++ = D->systemFlags;
    char* name = (char*)data;
    size_t len = strlen(D->word) + 1;
    strcpy(name, D->word);
    name += len + 7; // align
    uint64 base = (uint64)name;
    base &= 0xFFFFFFFFFFFFFFF8ULL;  // 8 byte align
    return (uint64*)base;
}

static uint64* ReadFactField(uint64* data, FACT* F, int changed)
{
    uint64 properties = *data++;
    uint64 sysflags = *data++;
    char* name = (char*)data;
    size_t len = strlen(name) + 8;
    WORDP D = StoreWord(name, properties, sysflags);
    uint64 base = (uint64)(name + len);
    base &= 0xFFFFFFFFFFFFFFF8ULL;  // 8 byte align
    if (changed == 4) F->object = MakeMeaning(D);
    else if (changed == 2) F->verb = MakeMeaning(D);
    else  F->subject = MakeMeaning(D);
    return (uint64*)base;
}

void NoteBotFacts() // see RedoSystemFactFields for inverse
{
    // system facts may have had their fields repointed to user dictionary entry
    // Those need to be relocated into boot space
    STACKREF newThread = NULL; // will be in stack instead of heap
    while (factThread) // heap list
    {
        uint64 val;
        factThread = UnpackHeapval(factThread, val,discard,discard);
        FACT* F = (FACT*)val;
        if (!F) continue; // illegal fact reference
        if (F->flags & MARKED_FACT) continue; // only do this once per resulting fact modification

        // is there still something pointing to user space? Maybe was set back again
        int changed = 0; // which fields changed
        int size =0; 
       // write out names and prop and sysflags per word changed
        WORDP D;
        if (Meaning2Word(F->subject) > dictionaryFree)
        {
            changed |= 1;
            D = Meaning2Word(F->subject);
            size += (2 * sizeof(uint64)) + strlen(D->word) + sizeof(uint64); // eos + align
        }
        else if (Meaning2Word(F->verb) > dictionaryFree)
        {
            changed |= 2;
            D = Meaning2Word(F->verb);
            size += (2 * sizeof(uint64)) + strlen(D->word) + sizeof(uint64); // eos + align
        }
        else if (Meaning2Word(F->object) > dictionaryFree)
        {
            changed |= 4;
            D = Meaning2Word(F->object);
            size += (2 * sizeof(uint64)) + strlen(D->word) + sizeof(uint64); // eos + align
        }
        else continue; // nothing from user space here any more

        F->flags |= MARKED_FACT;
        uint64* data = (uint64*)AllocateStack(NULL, size, false, 8);
        newThread = AllocateStackval(newThread, (uint64)F,(uint64)changed,(uint64) data);
        if (changed & 1) data = WriteDEntry(data, Meaning2Word(F->subject));
        if (changed & 2) data = WriteDEntry(data, Meaning2Word(F->verb));
        if (changed & 4) data = WriteDEntry(data, Meaning2Word(F->object));
    }
    stackFactThread = newThread; // now have stack-based copy of needed data
}

void RedoSystemFactFields() // see NoteBotFacts for inverse
{
    while (stackFactThread) 
    {
        uint64 Fx;
        uint64 changed;
        uint64 datax;
        stackFactThread = UnpackStackval(stackFactThread, Fx, changed, datax);
        FACT* F = (FACT*)Fx;
        uint64* data = (uint64*)datax;
        F->flags ^= MARKED_FACT;
        if (changed & 1) data = ReadFactField(data, F, 1);
        if (changed & 2) data = ReadFactField(data, F, 2);
        if (changed & 4) data = ReadFactField(data, F, 4);
    }
}

static char* PutBlob(WORDP D, char* base)
{
    uint64* val = (uint64*)base;
    *val++ = D->properties;
    *val++ = D->systemFlags;
    base += 16;
    strcpy(base, D->word);
    uint64 align = (uint64)(base + strlen(base) + 8);
    align &= 0xFFFFFFFFFFFFFFf8ULL;
    if ((char*)align >= (heapFree - 50)) myexit((char*)"Out of stack space in Putblob \r\n");
    return (char*)align;
}

static char* GetBlob(char* base, WORDP& D)
{
    uint64* val = (uint64*)base;
    uint64 prop = *val++;
    uint64 sys = *val++;
    char* name = (char*)val;
    D = StoreWord(name, prop, sys);
    uint64 align = (uint64)(name + strlen(name) + sizeof(uint64));
    align &= 0xFFFFFFFFFFFFFFf8ULL;
    return (char*)align;
}

void MigrateFactsToBoot(FACT* endUserFacts, FACT* endBootFacts)
{ // transfer facts from user layer to boot layer

    // if we purged stuff from boot, then we need to fix end of boot
    FACT* G = factsPreBuild[LAYER_USER]; // last fact in boot
    while (!G->subject) --G;
    factFree = factsPreBuild[LAYER_USER] = G;

    if (!bootFacts && !factThread) return;

    UnlockLayer(LAYER_BOOT); // unlock it to add stuff
    FILE* bootwrite = NULL;
    char* buffer = NULL;
    if (recordBoot)
    {
        bootwrite = FopenUTF8WriteAppend((char*)"bootfacts.txt"); // augmenting against system restart
        buffer = AllocateBuffer("MigrateFactsToBoot");
    }
    // while facts ordering is safe to walk and overwrite, dictionary items are
    // unreliable in order and the strings must be protected. 
    FACT* F = endBootFacts;
    char* limit;
    char* blobs = InfiniteStack64(limit, "MigrateFactsToBoot");
    char* startblobs = blobs;
    // facts will be simple, but perhaps involve dictionary entries that need moving
    while (++F <= endUserFacts) // note dictionary name and data before we lose them to reuse
    {
        // if fact is moving and involves dictionary entries past boot, we need to replicate those temporarily
        if (F->flags & FACTBOOT && !(F->flags & (FACTDEAD | FACTTRANSIENT)))
        {
            WORDP SD = Meaning2Word(F->subject);
            if (SD > dictionaryFree) blobs = PutBlob(SD, blobs); // from user layer
            WORDP VD = Meaning2Word(F->verb);
            if (VD > dictionaryFree) blobs = PutBlob(VD, blobs);
            WORDP OD = Meaning2Word(F->object);
            if (OD > dictionaryFree) blobs = PutBlob(OD, blobs);
        }
    }
    NoteBotFacts(); // store changes to system facts on stack as well

    // now rebuild facts and dictionary items for real into boot layer
    blobs = startblobs;
    F = endBootFacts;
    while (++F <= endUserFacts) 
    {
        // facts will be simple, but perhaps involve dictionary entries that need moving
        if (F->flags & FACTBOOT && !(F->flags & (FACTDEAD | FACTTRANSIENT)))
        {
            // get all major data from this fact, dangling in user space
            unsigned int flags = F->flags;
            flags &= -1 ^ (MARKED_FACT | MARKED_FACT2 | FACTTRANSIENT | FACTBOOT);
            WORDP SD = Meaning2Word(F->subject);
            if (SD > dictionaryFree) blobs = GetBlob(blobs, SD);
            WORDP VD = Meaning2Word(F->verb);
            if (VD > dictionaryFree) blobs = GetBlob(blobs, VD);
            WORDP OD = Meaning2Word(F->object);
            if (OD > dictionaryFree) blobs = GetBlob(blobs, OD);
            uint64 botbits = F->botBits;

            FACT* X = ++factFree; // allocate new fact (may even be this fact!)
            memset(X, 0, sizeof(FACT)); // botbits = 0 (all can see)
            X->botBits = botbits;
            X->flags = flags;
            X->subject = Word2Index(SD);
            X->verb = Word2Index(VD);
            X->object = Word2Index(OD);
            WeaveFact(X); // interlace to dictionary

            if (bootwrite)
            {
                WriteFact(F, false, buffer, false, true);
                fprintf(bootwrite, "%s", buffer);
            }
        }
    }
    RedoSystemFactFields();
    ReleaseInfiniteStack();

    LockLayer(true);

    if (buffer)
    {
        FreeBuffer("MigrateFactsToBoot");
        fclose(bootwrite);
    }
    bootFacts = false;
}

void ModBaseFact(FACT* F) // anyone changing fields of facts calls this
{
    WORDP userdictstart = dictionaryPreBuild[LAYER_BOOT + 1];
    if (F > factLocked) return; // dont care about user layer facts
    if (Meaning2Word(F->subject) > userdictstart) {}
    else if (Meaning2Word(F->verb) > userdictstart) {}
    else if (Meaning2Word(F->object) > userdictstart) {}
    else return; // nothing from user space here 
    factThread = AllocateHeapval(factThread, (uint64)F, NULL);
}


