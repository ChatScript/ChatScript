
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
static bool blockshow = false;
FACT* factFreeList = NULL;
FACT* currentMemoryFact = NULL;
int worstlastFactUsed = 1000000;
char traceSubject[100];
bool allowBootKill = false;
char traceVerb[100];
char traceObject[100];
bool bootFacts = false;
bool recordBoot = false;
uint64 allowedBots = 0xffffffffffffffffULL; // default fact visibility restriction is all bots
uint64 myBot = 0;							// default fact creation restriction
size_t maxFacts = MAX_FACT_NODES;	// how many facts we can create at max
HEAPREF factThreadList = NULL;
static STACKREF stackfactThreadList = NULL;
bool factsExhausted = false;
FACT* factBase = NULL;			// start of all facts
FACT* factEnd = NULL;			// end of all facts
FACT* factsPreBuild[NUMBER_OF_LAYERS+1];		// on last fact of build0 facts, start of build1 facts
FACT* lastFactUsed = NULL;			// lastFactUsed is a fact in use (increment before use as a free fact)
FACT* currentFact = NULL;		// current fact found or created
static bool verifyFacts = false;

//   values of verbs to compare against
MEANING Mmember;				// represents concept sets
MEANING Mis;					// represents wordnet hierarchy
MEANING Mexclude;				// represents restriction of word not allowed in set (blocking inheritance)
MEANING Mremapfact;		// api marking remaping data
bool seeAllFacts = false;
FACT* Index2Fact(FACTOID e)
{ 
	FACT* F = NULL;
	if (e)
	{
		F =  e + factBase;
 	}
	return F;
}

static void VerifyField(FACT* F, MEANING field, unsigned int offset)
{ // find this fact linked in thru field given
	if (field == 1 || !verifyFacts) return; // member is huge, dont try to validate

	WORDP D = Meaning2Word(field);
	FACTOID* linkfields = &D->subjectHead;
	linkfields += offset;
	FACTOID x = *linkfields; // start of list on D for this
	FACT* G = Index2Fact(x);
	int limit = 300000; // beware of some common fields which have large numbers 
	bool debugshow = false;
	while (G && --limit)
	{
		if (debugshow) TraceFact(G);
		if (G == F) return;	// found it
		FACTOID* y = (&G->subjectNext) + offset;
		G = Index2Fact(*y);
	}
	bool old = blockshow;
	blockshow = true;
	TraceFact(F);
	blockshow = old;
	if (!limit) 
		ReportBug("Circular field data");
	else ReportBug("Fact not woven correctly");
}

static void VerifyFact(FACT* F)
{
	if (!verifyFacts) return;

	// prove dict entries can find this fact and that is has no loops
	char* subject = "";
	char* verb = "";
	char* object = "";
	if (!(F->flags & FACTSUBJECT))
	{
		VerifyField(F, F->subject, 0);
		subject = Meaning2Word(F->subject)->word;
	}
	else
	{
		FACT* G = factBase + F->subject;
		VerifyFact(G);
	}
	if (!(F->flags & FACTVERB))
	{
		VerifyField(F, F->verb, 1);
		verb = Meaning2Word(F->verb)->word;
	}
	else
	{
		FACT* G = factBase + F->verb;
		VerifyFact(G);
	}
	if (!(F->flags & FACTOBJECT))
	{
		VerifyField(F, F->object, 2);
		object = Meaning2Word(F->object)->word;
	}
	else
	{
		FACT* G = factBase + F->object;
		VerifyFact(G);
	}
	unsigned int l = F->flags & FACTLANGUAGEBITS;
	unsigned int factl = GetFullFactLanguage(F);
	if ( l != factl ) // they differ? 
	{
		blockshow = true;
		TraceFact(F);
		blockshow = false;
		Log(ECHOUSERLOG, " fact bad validatecreatefas %d %08x  %d \r\n",
			F - factBase, F->flags, l);
		F->flags &= -1 ^ FACTLANGUAGEBITS;
		F->flags |= factl;
	}
}

void C_Fact(char* word)
{
	char name[MAX_WORD_SIZE];
	char field[MAX_WORD_SIZE];
	word = ReadToken(word, name);
	word = ReadToken(word, field);
	WORDP set[GETWORDSLIMIT];
	seeAllFacts = true;
	int n= GetWords(name, set, true, false);
	FACT* F;
	int i;
	if (!n) printf("%s not found\r\n", name);
	else
	{
		Log(USERLOG, "----\r\n");
		if (*field == 's' || *field == 'S' || *field == 'a' || !*field)
		{
			for (i = 0; i < n; ++i)
			{
				F = GetSubjectHead(set[i]);
				while (F)
				{
					TraceFact(F, false);
					F = GetSubjectNext(F);
				}
			}
		}
		Log(USERLOG, "----\r\n");
		if (*field == 'v' || *field == 'V' ||  *field == 'a' || !*field)
		{
			for (i = 0; i < n; ++i)
			{
				F = GetVerbHead(set[i]);
				while (F)
				{
					TraceFact(F, false);
					F = GetVerbNext(F);
				}
			}
		}
		Log(USERLOG, "----\r\n");
		if (*field == 'o' || *field == 'O' || *field == 'a' || !*field)
		{
			for (i = 0; i < n; ++i)
			{
				F = GetObjectHead(set[i]);
				while (F)
				{
					TraceFact(F, false);
					F = GetObjectNext(F);
				}
			}
		}
	}
	seeAllFacts = false;
}

void VerifyFacts()
{
	FACT* F = factBase;
	while (++F <= lastFactUsed)
	{
		VerifyFact(F);
	}
}
bool showit = false;

bool AcceptableFact(FACT* F)
{
	if (!F || F->flags & FACTDEAD) return false;
	if (showit) TraceFact(F);
	if (seeAllFacts) return true;

	// if ownership flags exist (layer0 or layer1) and we have different ownership.
	if (myBot) // if we are not universal, see if we can see this fact
	{
		if (F->botBits && !(F->botBits & myBot)) return false;
	}
	
	// fact is not from overridden collection during api testpattern call
	if (csapicall == TEST_PATTERN && F->verb == Mmember &&
		Meaning2Word(F->object)->internalBits & OVERRIDE_CONCEPT &&
		!(F->flags & OVERRIDE_MEMBER_FACT)) return false; 

	// if language flags exist, confirm those
	unsigned int factlanguage = (F->flags & FACTLANGUAGEBITS) << LANGUAGE_SHIFT;
	// if we are in universal language, we can see all facts
	// if a fact is in universal language, it can be seen by any language 
	if (!factlanguage ||  !language_bits || factlanguage == language_bits) return true;

	return false;
}

FACT* GetSubjectNext(FACT* F) { return Index2Fact(F->subjectNext);}
FACT* GetVerbNext(FACT* F) {return Index2Fact(F->verbNext);}
FACT* GetObjectNext(FACT* F) {return Index2Fact(F->objectNext);}

FACT* GetSubjectNondeadNext(FACT* F) 
{ 
	FACT* G = F;
	while (F) 
	{
		F =  Index2Fact(F->subjectNext);
		if (F && AcceptableFact(F))
			break;
	}
	if (F == G)
	{
		ReportBug("Infinite loop in GetSubjectNondeadNext ");
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
		if (F && AcceptableFact(F))
			break;
	}
	if (F == G)
	{
		ReportBug("Infinite loop in GetVerbNondeadNext ");
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
		if (F && AcceptableFact(F))
			break;
	}
	if (F == G)
	{
		ReportBug("Infinite loop in GetObjectNondeadNext ");
		F = NULL;
	}	
	return F;
}

FACT* GetSubjectNondeadHead(FACT* F) 
{
	F = Index2Fact(F->subjectHead);
	while (F) 
	{
		if (AcceptableFact(F)) break;
		F =  Index2Fact(F->subjectNext);
	}
	return F;
}

FACT* GetVerbNondeadHead(FACT* F) 
{
	F = Index2Fact(F->verbHead);
	while (F ) 
	{
		if (AcceptableFact(F)) break;
		F =  Index2Fact(F->verbNext);
	}
	return F;
}

FACT* GetObjectNondeadHead(FACT* F) 
{
	F = Index2Fact(F->objectHead);
	while (F ) 
	{
		if (AcceptableFact(F)) break;
		F =  Index2Fact(F->objectNext);
	}
	return F;
}

FACT* GetSubjectNondeadHead(WORDP D) // able to get special marker empty fact for json units
{
	if (!D) return NULL;
	FACT* F = Index2Fact(D->subjectHead);
	while (F) 
	{
		if (AcceptableFact(F)) break;
		F =  Index2Fact(F->subjectNext);
	}
	return F;
}

FACT* GetVerbNondeadHead(WORDP D) 
{
	if (!D) return NULL;
	FACT* F = Index2Fact(D->verbHead);
	while (F ) 
	{
		if (AcceptableFact(F)) break;
		F =  Index2Fact(F->verbNext);
	}
	return F;
}

FACT* GetObjectNondeadHead(WORDP D)  
{
	if (!D) return NULL;
	FACT* F = Index2Fact(D->objectHead);
	while (F) 
	{
		if (AcceptableFact(F)) break;
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
	return (n <= (unsigned int)(lastFactUsed-factBase)) ? Index2Fact(n) : NULL;
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

FACT* EarliestObjectFact(MEANING M) // earliest fact is last in list
{
	FACT* F = GetObjectNondeadHead(M); // most recent fact
	FACT* G = F;
	while (F)
	{
		F = GetObjectNondeadNext(F);
		if (F) G = F;
	}
	return G;
}
FACT* EarliestFact(MEANING M) // earliest fact is last in list
{
	FACT* F = GetSubjectNondeadHead(M); // most recent fact
	FACT* G = F;
	while (F)
	{
		F = GetSubjectNondeadNext(F);
		if (F) G = F;
	}
	return G;
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
{	if (!F) return;
	char* word = AllocateBuffer(); 
	Log(FORCESTAYUSERLOG,"FactIndex: %d x%08x: %s\r\n",Fact2Index(F),F,WriteFact(F,false,word,ignoreDead,false,true));
	Log(FORCESTAYUSERLOG,"");
	FreeBuffer();
} 

void ClearUserFacts()
{
	for (int i = 0; i <= MAX_FIND_SETS; ++i) SET_FACTSET_COUNT(i, 0);
	while (lastFactUsed > factLocked)  FreeFact(lastFactUsed--); //   erase new facts
}

void InitFacts()
{
	*traceSubject = 0;
	*traceVerb = 0;
	*traceObject = 0;

	if ( factBase == NULL) 
	{
		factBase = (FACT*) mymalloc(maxFacts * sizeof(FACT)); // only on 1st startup, not on reload
		memset(factBase, 0, sizeof(FACT) * maxFacts); // not strictly necessary
		if ( factBase == NULL)
		{
			(*printer)((char*)"%s",(char*)"failed to allocate fact space\r\n");
			myexit((char*)"FATAL failed to get fact space");
		}
	}
    lastFactUsed = factBase; // 1st fact is always meaningless
	factEnd = factBase + maxFacts;
}

void InitFactWords()
{
	//   special internal fact markers (always 1st 3 words)
	Mmember = MakeMeaning(StoreWord((char*)"member",AS_IS));
	Mexclude = MakeMeaning(StoreWord((char*)"exclude", AS_IS));
	Mis = MakeMeaning(StoreWord((char*)"is", AS_IS));
	Mremapfact = MakeMeaning(StoreWord("api-remap", AS_IS));
	StoreWord("null", AS_IS);
}

void CloseFacts()
{
    myfree(factBase);
	factBase = 0;
}

void FreeFact(FACT* F)
{ //   most recent facts are always at the top of any xref list. Can only free facts sequentially backwards.
	if (!F) return;
	if (!F->subject) // unindexed fact recording a fact delete that must be undeleted
	{
		if (!F->object) // death of fact fact (verb but no object)
		{
			F = Index2Fact(F->verb);
			if (F) F->flags &= -1 ^ FACTDEAD;
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
	if (++lastFactUsed == factEnd) 
	{
		--lastFactUsed;
        if (loading || compiling) ReportBug((char*)"FATAL:  out of fact space at %d", Fact2Index(lastFactUsed));
        ReportBug((char*)"FATAL: out of fact space at %d",Fact2Index(lastFactUsed));
		(*printer)((char*)"%s",(char*)"out of fact space");
		return lastFactUsed; // dont return null because we dont want to crash anywhere
	}
	//   init the basics
	memset(lastFactUsed,0,sizeof(FACT));
	lastFactUsed->verb = verb;
	lastFactUsed->object = object;
	lastFactUsed->flags = FACTTRANSIENT | FACTDEAD | flags;	// cannot be written or kept
	return lastFactUsed;
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
	FACT* base = lastFactUsed++;
	while (--lastFactUsed > F) // mark all still used new dict words
	{
		if (!(F->flags & FACTDEAD))
		{
			MarkDictWord(Meaning2Word(lastFactUsed->subject), dictBase);
			MarkDictWord(Meaning2Word(lastFactUsed->verb), dictBase);
			MarkDictWord(Meaning2Word(lastFactUsed->object), dictBase);
		}
	}
	lastFactUsed = base + 1;
	while (--lastFactUsed > F)
	{
		FreeFact(lastFactUsed); // remove all links from dict
		if (F->flags & FACTDEAD) // release 
		{
			ReleaseDictWord(Meaning2Word(lastFactUsed->subject), dictBase);
			ReleaseDictWord(Meaning2Word(lastFactUsed->verb), dictBase);
			ReleaseDictWord(Meaning2Word(lastFactUsed->object), dictBase);
		}
	}
	lastFactUsed = base;
}

FACT* WeaveFact(FACT* current)
{
	if (current->subject == 0) return current; // dummy fact

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
    if (F->subject) UnweaveFactSubject(F);
    if (F->verb) UnweaveFactVerb(F);
    if (F->object) UnweaveFactObject(F);
}

void WeaveFacts(FACT* F)
{
    --F;
	while (++F <= lastFactUsed) WeaveFact(F);
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
		Log(USERLOG,"Kill: ");
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
	while (lastFactUsed > locked) FreeFact(lastFactUsed--); // restore to end of basic facts
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

FACT* FindFact(FACTOID_OR_MEANING subject, FACTOID_OR_MEANING verb, FACTOID_OR_MEANING object, unsigned int properties,uint64 bot)
{
    FACT* F;
	FACT* G;
	if (!subject || !verb || !object) return NULL;
	if (properties & FACTDUPLICATE) 
		return NULL;	// can never find this since we are allowed to duplicate and we do NOT want to share any existing fact
	unsigned int wantlanguage = properties &  (-1 ^ FACTLANGUAGEBITS); // implied by the actual fields

    //   see if  fact already exists. if so, just return it 
	if (properties & FACTSUBJECT)
	{
		F  = Index2Fact(subject);
		G = GetSubjectHead(F);
		while (G)
		{
			bool allowedBot;
			if (!bot || !G->botBits) allowedBot = true; // either bot or fact is universal
			else allowedBot = (G->botBits == bot);

			bool allowedLanguage;
			unsigned int  language = G->flags & (-1 ^ FACTLANGUAGEBITS);
			if (!language || !wantlanguage) allowedLanguage = true; // bot or desire is universal
			else allowedLanguage = (wantlanguage == language);
			
			if (G->verb == verb &&  G->object == object &&
				allowedLanguage  && allowedBot && !(G->flags & FACTDEAD)) return G;
			G  = GetSubjectNext(G);
		}
		return NULL;
	}
	else if (properties & FACTVERB)
	{
		F  = Index2Fact(verb);
		G = GetVerbHead(F);
		while (G)
		{
			bool allowedBot;
			if (!bot || !G->botBits) allowedBot = true; // either bot or fact is universal
			else allowedBot = (G->botBits == bot);

			bool allowedLanguage;
			unsigned int  language = G->flags & (-1 ^ FACTLANGUAGEBITS);
			if (!language || !wantlanguage) allowedLanguage = true; // bot or desire is universal
			else allowedLanguage = (wantlanguage == language);
			
			if (G->subject == subject && G->object == object &&
				allowedLanguage && allowedBot && !(G->flags & FACTDEAD)) return G;
			G  = GetVerbNext(G);
		}
		return NULL;
	}   
 	else if (properties & FACTOBJECT)
	{
		F  = Index2Fact(object);
		G = GetObjectHead(F);
		while (G)
		{
			bool allowedBot;
			if (!bot || !G->botBits) allowedBot = true; // either bot or fact is universal
			else allowedBot = (G->botBits == bot);

			bool allowedLanguage;
			unsigned int  language = G->flags & (-1 ^ FACTLANGUAGEBITS);
			if (!language || !wantlanguage) allowedLanguage = true; // bot or desire is universal
			else allowedLanguage = (wantlanguage == language);

			if (G->subject == subject && G->verb == verb &&
			allowedBot && allowedLanguage && !(G->flags & FACTDEAD)) return G;
			G  = GetObjectNext(G);
		}
		return NULL;
	}
  	//   simple FACT* based on dictionary entries
	F = GetSubjectHead(Meaning2Word(subject));
    while (F)
    {
		bool allowedBot;
		if (!bot || !F->botBits) allowedBot = true; // either bot or fact is universal
		else allowedBot = (F->botBits == bot);

		bool allowedLanguage;
		unsigned int  language = F->flags & (-1 ^ FACTLANGUAGEBITS);
		if (!language || !wantlanguage) allowedLanguage = true; // bot or desire is universal
		else allowedLanguage = (wantlanguage == language);

		if ( F->subject == subject &&  F->verb ==  verb && F->object == object &&
			allowedBot && allowedLanguage && !(F->flags & FACTDEAD))  
			return F;
		F = GetSubjectNext(F);
    }
    return NULL;
}

FACT* CreateFact(FACTOID_OR_MEANING subject, FACTOID_OR_MEANING verb, FACTOID_OR_MEANING object, unsigned int properties)
{
	currentFact = NULL; 
	if (!subject || !object || !verb)
	{
		if (!subject) 
			ReportBug((char*)"Missing subject field in fact create at line %d of %s",currentFileLine,currentFilename);
		if (!verb) 
			ReportBug((char*)"Missing verb field in fact create at line %d of %s",currentFileLine,currentFilename);
		if (!object) 
			ReportBug((char*)"Missing object field in fact create at line %d of %s",currentFileLine,currentFilename);
		return NULL;
	}

	//   get correct field values
    WORDP s = (properties & FACTSUBJECT) ? NULL : Meaning2Word(subject);
    WORDP v = (properties & FACTVERB) ? NULL : Meaning2Word(verb);
	WORDP o = (properties & FACTOBJECT) ? NULL : Meaning2Word(object);
	if (s && !s->word)
	{
		ReportBug((char*)"bad choice in fact subject");
		return NULL;
	}
	if (v && !v->word)
	{
		ReportBug((char*)"bad choice in fact verb");
		return NULL;
	}
	if (o && !o->word)
	{
		ReportBug((char*)"bad choice in fact object");
		return NULL;
	}

	// convert any restricted meaning
	MEANING other;
	char* word = nullptr;
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
	currentFact =  (properties & FACTDUPLICATE) ? NULL : FindFact(subject,verb,object,properties, myBot);
	if (trace & TRACE_FACT && currentFact && CheckTopicTrace())  Log(USERLOG," Found %d ", Fact2Index(currentFact));
	if (currentFact) return currentFact;

	currentFact = CreateFastFact(subject,verb,object,properties,myBot);
	currentFact->flags |= GetFullFactLanguage(currentFact);
	if (trace & TRACE_FACT && currentFact && CheckTopicTrace())  
	{
        TraceFact(currentFact, false); // precise trace for JSON
	}

	if (verifyFacts)
	{
		VerifyFact(currentFact);
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

	char* buffer = GetFileBuffer();
	char* base = buffer;

	unsigned int count = FACTSET_COUNT(set);
	for (unsigned int i = 1; i <= count; ++i)
	{
		FACT* F = factSet[set][i];
		if (!F) strcpy(buffer,(char*)"null ");
		else if ( !(F->flags & FACTDEAD))
		{
			unsigned int original = F->flags;
			F->flags &= -1 ^ FACTTRANSIENT;	// dont pass transient flag out
			WriteFact(F,false,buffer,false,true);
			F->flags = original;
		}
		buffer += strlen(buffer);
	}

	FILE* out;
    if (isKnownFileType(name)) out = userFileSystem.userCreate(name); // user ltm file
	else out = (append && !stricmp(append,"append")) ? FopenUTF8WriteAppend(name) : FopenUTF8Write(name);
	if (out)
	{
        if (isKnownFileType(name))
		{
			EncryptableFileWrite(base,1,(buffer-base),out,ltmEncrypt,"LTM"); 
			userFileSystem.userClose(out);
		}
		else
		{
			fprintf(out,(char*)"%s",base);
			FClose(out);
		}
	}
	FreeFileBuffer();
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
    if (isKnownFileType(name)) out = userFileSystem.userCreate(name); // user ltm file
	else out = (append && !stricmp(append,"append")) ? FopenUTF8WriteAppend(name) : FopenUTF8Write(name);
	if (!out) return FAILRULE_BIT;

	char* buffer = GetFileBuffer(); // filesize limit
	ExportJson1(jsonitem, buffer);
    bool isWriteFailure = false;
    if (isKnownFileType(name))
	{
		size_t wrote = EncryptableFileWrite(buffer,1,strlen(buffer),out,ltmEncrypt,"LTM");
		userFileSystem.userClose(out);
        if (!wrote) isWriteFailure = true;
	}
	else
	{
		fprintf(out,(char*)"%s",buffer);
		FClose(out);
	}
	FreeFileBuffer();
	return (isWriteFailure ? FAILRULE_BIT : NOPROBLEM_BIT);
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
		sprintf(buffer,(char*)"%u",currentFactIndex() ); //   created OR e found instead of created
	}
	else  ptr = GetCommandArg(ptr,buffer,result,OUTPUT_FACTREAD); //   subject
	ptr = SkipWhitespace(ptr); // could be user-formateed, dont trust
	if (!ptr) return NULL;
	if (result & ENDCODES) return ptr;

	if (!*buffer) 
	{
		if (compiling)
			BADSCRIPT((char*)"FACT-1 Missing subject for fact create %s",readBuffer)
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
		sprintf(buffer,(char*)"%u",currentFactIndex() );
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
		if (compiling) BADSCRIPT((char*)"FACT-2 Missing verb for fact create %s",readBuffer)
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
		sprintf(buffer,(char*)"%u",currentFactIndex() );
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

	 char* sword = word;
	 char* vword = word1;
	 char* oword = word2;
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
		subject =  MakeMeaning(StoreWord(word, AS_IS),0);
	}
	else subject = MakeMeaning(StoreWord(word, AS_IS), 0);

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
		verb =  MakeMeaning(StoreWord(word1, AS_IS),0);
	}
	else verb = MakeMeaning(StoreWord(word1, AS_IS), 0);

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
		object =  MakeMeaning(StoreWord(word2, AS_IS),0);
	}
	else object = MakeMeaning(StoreWord(word2, AS_IS), 0);

	if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG,"%s %s %s %08x) = ",word,word1,word2,flags);
	
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
					char* wordx = AllocateBuffer();
					WriteFact(F,false,wordx,false,true);
					Log(USERLOG,"Fact created is not an attribute. There already exists %s",wordx); 
					(*printer)((char*)"Fact created is not an attribute. There already exists %s",wordx); 
					currentFact = F;
                    FreeBuffer();
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

bool ImportFacts(char* buffer, char* name, char* set, char* erase, char* transient)
{ // if no set is given, return the subject of the 1st fact we read (Json entity name)
	int store = -1;
	if (*set && *set != '"') // able to pass empty string
	{
		store = GetSetID(set);
		if (store == ILLEGAL_FACTSET) return false;
		SET_FACTSET_COUNT(store, 0);
	}
	if (*name == '"')
	{
		++name;
		size_t len = strlen(name);
		if (name[len - 1] == '"') name[len - 1] = 0;
	}

	bool exists = false;
	if (!stricmp(erase, (char*)"exist") || !stricmp(transient, (char*)"exist")) exists = true;

	// could this file be cached
	bool cacheFile = false;
	if (!stricmp(erase, (char*)"cache") || !stricmp(transient, (char*)"cache")) cacheFile = true;

	char* filebuffer = 0;
	size_t readit = 0;

	// check to see if we already have the document
	filebuffer = FindFileCache(name, !exists);  // don't record hit of an existance check
	if (filebuffer)
	{
		filebuffer += strlen(name) + 2; // skip over the filename
		readit = strlen(filebuffer);
	}
	if (!filebuffer || !*filebuffer)
	{
		// not found in cache, so have to read the file
		FILE* in;
		bool knownFileType = isKnownFileType(name);

		if (knownFileType) in = userFileSystem.userOpen(name);
		else in = FopenReadWritten(name);
		if (!in) return false;

		filebuffer = GetFileBuffer();
		char* ptr = filebuffer;
		if (cacheFile)
		{
			// add filename to start of buffer so that it can be found in the future
			sprintf(filebuffer, "%s\r\n", name);
			filebuffer += strlen(name) + 2;
		}

		if (knownFileType)
		{
			readit = DecryptableFileRead(filebuffer, 1, userCacheSize, in, ltmEncrypt, "LTM");    // LTM file read, read it all in, including BOM
			userFileSystem.userClose(in);
		}
		else
		{
			readit = fread(filebuffer, 1, userCacheSize, in);
			fclose(in);
		}
		filebuffer[readit] = 0;
		filebuffer[readit + 1] = 0; // insure fully closed off

		// save a copy of the file in the cache
		if (cacheFile) Cache(ptr, strlen(ptr));
	}

	// allow for simple existence check
	if (exists)
	{
		FreeFileBuffer();
		if (readit > 0) return true;
		return false;
	}

	// set bom
	maxFileLine = currentFileLine = 0;
	BOM = BOMSET; // will be utf8
	userRecordSourceBuffer = filebuffer;

	unsigned int flags = 0;
	if (!stricmp(erase, (char*)"transient") || !stricmp(transient, (char*)"transient")) flags |= FACTTRANSIENT; // make facts transient
	if (!stricmp(erase, (char*)"boot") || !stricmp(transient, (char*)"boot")) flags |= FACTBOOT; // make facts migrate to the boot layer
	while (ReadALine(readBuffer, 0) >= 0)
	{
		if (*readBuffer == 0 || *readBuffer == '#') continue; //   empty or comment
		char word[MAX_WORD_SIZE];
		ReadCompiledWord(readBuffer, word);
		if (!stricmp(word, (char*)"null"))
		{
			if (store >= 0) AddFact(store, NULL);
		}
		else if (*word == '(')
		{
			char* ptr = readBuffer;
			FACT* G = ReadFact(ptr, 0);
			if (!G)
			{
				ReportBug("Import data: %s", readBuffer);
				continue;
			}
			G->flags |= flags;
			if (store >= 0) AddFact(store, G);
			else if (!*buffer) strcpy(buffer, Meaning2Word(G->subject)->word); // to return the name
		}
	}
	FreeFileBuffer();
	userRecordSourceBuffer = NULL;

	if (!stricmp(erase, (char*)"erase") || !stricmp(transient, (char*)"erase")) remove(name); // erase file after reading
	if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, "[%d] => ", FACTSET_COUNT(store));
	currentFact = NULL; // we have used up the facts
	return true;
}

int FindEarliestMember(WORDP concept, char* word)
{ // need to reverse all members
	char* limit;
	FACT** stack = (FACT**)InfiniteStack(limit, "findearliestmember");
	FACT* F = GetObjectNondeadHead(concept);
	int index = 0;
	while (F) // stack object key data
	{
		if (F->verb == Mmember) stack[index++] = F;
		F = GetObjectNondeadNext(F);
	}
	ReleaseInfiniteStack();
	MEANING M = MakeMeaning(FindWord(word, 0, PRIMARY_CASE_ALLOWED));
	unsigned int count = index - 1;
	while (index--)
	{
		F = stack[index];
		char* name = Meaning2Word(F->subject)->word;
		if (F->verb == Mmember && (F->subject & MEANING_BASE) == M)
			return count - index;
	}
	return -1;
}

int FindRecentMember(WORDP concept,char* word)
{
	unsigned int count = 0;
	MEANING M = MakeMeaning(FindWord(word, 0, PRIMARY_CASE_ALLOWED));
	FACT* F = GetObjectNondeadHead(concept);
	while (F)
	{
		if (F->verb == Mmember && (F->subject & MEANING_BASE) == M) return count;
		++count;
		F = GetObjectNondeadNext(F);
	}
	return -1; // not found
}

WORDP NthEarliestMember(WORDP concept, int n)
{ // need to reverse all members
	char* limit;
	FACT** stack = (FACT**)InfiniteStack(limit, "nthearliestmember");
	FACT* F = GetObjectNondeadHead(concept);
	int index = 0;
	while (F) // stack object key data
	{
		char* word = Meaning2Word(F->subject)->word;
		if (F->verb == Mmember) stack[index++] = F;
		F = GetObjectNondeadNext(F);
	}
	ReleaseInfiniteStack();
	
	++n;
	while (--index >= 0)
	{
		if (--n == 0) return Meaning2Word(stack[index]->subject);
	}
	return NULL;
}

WORDP NthRecentMember(WORDP concept, int n)
{
	unsigned int count = 0;
	FACT* F = GetObjectNondeadHead(concept);
	while (F)
	{
		if (F->verb == Mmember) --n;
		if (n < 0) return Meaning2Word(F->subject);
		F = GetObjectNondeadNext(F); 
	}
	return NULL; // not found
}

void AdjustFactLanguage(FACT* F,MEANING M, unsigned int field)
{ // changes language on existing facts of entry being made universal
	unsigned oldlang = F->flags & FACTLANGUAGEBITS;
	unsigned int newlang = GetFullFactLanguage(F);
	if (oldlang == newlang) return; // no change
	unsigned int flags = F->flags & (-1 ^ FACTLANGUAGEBITS);
	F->flags = flags; // now universalized

	// only aim to write out facts from a prior build layer that changed
	if (currentBuild == BUILD0 && F >= factsPreBuild[LAYER_0]);
	else if (currentBuild == BUILD1 && F >= factsPreBuild[LAYER_1]);
	else languageadjustedfactsList = AllocateHeapval(HV1_FACTI|HV2_INT| HV3_MEAN, languageadjustedfactsList, Fact2Index(F), F->flags, M);
}

void LostFact(FACT* F) // facts lost when a dict node becomes universal 
{
	// only mark facts from a prior build layer
	if (currentBuild == BUILD0 && F >= factsPreBuild[LAYER_0] ) return;
	else if (currentBuild == BUILD1 && F >= factsPreBuild[LAYER_1]) return;

	// here we write out indications of facts from earlier level
	deadfactsList = AllocateHeapval(HV1_FACTI|HV2_INT, deadfactsList, Fact2Index(F), F->flags);
	F->flags |= FACTDEAD;
}

FILE* WriteFacts(FILE* out,FACT* F) //   write out from here to end
{ 
	if (!out) return out;

    char* word = AllocateBuffer();
    while (++F <= lastFactUsed) 
	{
		if (!(F->flags & (FACTTRANSIENT | FACTDEAD)))
		{
			char* f = WriteFact(F, true, word, false, true);
			fprintf(out, (char*)"%s", f);
		}
	}
    FreeBuffer();
	return out;
}

void WriteDeadFacts(FILE* out)
{
	uint64 val1, val2, val3;
	while (deadfactsList)
	{
		deadfactsList = UnpackHeapval(deadfactsList, val1, val2, val3);
		unsigned int index = (unsigned int)val1;
		FACT* F = Index2Fact(index);
		// val1 is already fact index
		char subject[100];
		char verb[100];
		char object[100];
		strcpy(subject, WordWithLanguage(Meaning2Word(F->subject)));
		strcpy(verb, WordWithLanguage(Meaning2Word(F->verb)));
		strcpy(object, WordWithLanguage(Meaning2Word(F->object)));
		fprintf(out, " -f %u (%s %s %s) x%08x %u\r\n", index,subject,verb,object,F->flags,(unsigned int) F->botBits);
	}
}

void WriteLanguageAdjustedFacts(FILE* out)
{
	uint64 val1, val2, val3;
	while (languageadjustedfactsList)
	{
		languageadjustedfactsList = UnpackHeapval(languageadjustedfactsList, val1, val2, val3);
		unsigned int index = (unsigned int)val1;
		FACT* F = Index2Fact(index);
		char subject[100];
		char verb[100];
		char object[100];
		strcpy(subject, WordWithLanguage(Meaning2Word(F->subject)));
		strcpy(verb, WordWithLanguage(Meaning2Word(F->verb)));
		strcpy(object, WordWithLanguage(Meaning2Word(F->object)));
		fprintf(out, " -fl %u (%s %s %s x%08x)  %u\r\n", index, subject, verb, object, F->flags, (unsigned int)F->botBits);
	}
}
void WriteBinaryFacts(FILE* out,FACT* F,unsigned int beforefacts) //   write out from after here to thru end 
{ 
	if (!out) return;
	FACT* base = F + 1;
	unsigned int count =  (lastFactUsed - F) + 1; 
	FACT* verify = lastFactUsed + 1;
	verify->subjectHead = (FACTOID)CHECKSTAMP;
	verify->subject = Word2Index(dictionaryFree); // how large is dict
	verify->verb = WORDLENGTH((dictionaryFree - 1));
	verify->verbHead = (FACTOID)count;
	verify->object = Fact2Index(base); // what was start of fact append
	verify->flags = Word2Index(dictionaryFree );
	unsigned int keybase = (keywordBase) ? Word2Index(keywordBase) : 0;
	verify->objectHead = keybase;
	verify->botBits = (uint64)beforefacts;
	fwrite(base, sizeof(FACT), count, out); // block of actual facts + verify
	FClose(out);
}

FACT* CreateFastFact(FACTOID_OR_MEANING subject, FACTOID_OR_MEANING verb, FACTOID_OR_MEANING object, unsigned int properties,uint64 bot)
{
	WORDP Z = Meaning2Word(subject);
	//   get correct field values
	// DICTIONARY should never be build with any but simple meanings and Mis
	// No fact meaning should ever have a synset marker on it. And member facts may have type restrictions on them
	//   allocate a fact
    int n = factEnd - lastFactUsed;
    if (n < worstlastFactUsed) worstlastFactUsed = n;
	if (++lastFactUsed == factEnd) 
	{
		--lastFactUsed;
        if (loading || compiling) ReportBug((char*)"FATAL:  out of fact space at %d", Fact2Index(lastFactUsed));
        ReportBug((char*)"out of fact space at %d",Fact2Index(lastFactUsed));
		(*printer)((char*)"%s",(char*)"out of fact space");
        factsExhausted = true;
		return NULL;
	}

	// get available fact
	currentFact = lastFactUsed;
	if (factFreeList) // reused facts but NOT past a memory mark
	{
		if (!memoryMarkThreadList || currentMemoryFact < factFreeList)
		{
			currentFact = factFreeList;
			factFreeList = Index2Fact(factFreeList->subject);
		}
	}

	//   init the basics
	memset(currentFact,0,sizeof(FACT));
	currentFact->subject = subject;
	currentFact->verb = verb; 
	currentFact->object = object;
	currentFact->flags = properties;
	currentFact->botBits = bot; // this fact is visible to these bot ids (0 or -1 means all)
	currentFact = WeaveFact(currentFact); 
	if (!currentFact) // weave failed, why?
	{
		--lastFactUsed;
		return NULL;
	}
	if (planning) currentFact->flags |= FACTTRANSIENT;
	if (currentFact->flags & FACTBOOT) 
		bootFacts = true;
	if (trace & TRACE_FACT && CheckTopicTrace())
	{
		char* limit;
		char* buffer = InfiniteStack(limit,"CreateFastFact"); // fact might be big, cant use mere WORD_SIZE
		buffer = WriteFact(currentFact,false,buffer,true,false,true);
		Log( (globalDepth > 1) ? USERLOG : USERLOG,(char*)"create %s\r\n",buffer);
		ReleaseInfiniteStack();
	}	
	return currentFact;
}

int ReadBinaryFacts(FILE* in,bool dictionary,unsigned int beforefact) //   read binary facts
{ 
	if (!in) return 0; // no file
	fseek(in, 0, SEEK_END);
	unsigned long size = ftell(in); // bytes
	fseek(in, 0, SEEK_SET);
	size_t fsize = sizeof(FACT); 
	int factcount = size / fsize;
	FACT* base = lastFactUsed+1;
	size_t count = fread((void*)(base), fsize, factcount , in);
	FClose(in);
	if (count != 0)
	{
		FACT* verify = (FACT*)(base + count - 1);
		if (verify->subjectHead != (FACTOID)CHECKSTAMP)
			return 1; // old format
		if (verify->subject != Word2Index(dictionaryFree))
			return 1; // dictionary size wrong
		if (verify->verb != WORDLENGTH((dictionaryFree-1)))
			return 1; // last entry differs in length of name
		if (verify->verbHead != (FACTOID)count)
			return 1; // old format
		if (verify->object != Fact2Index(base)) 
			return 1; // start of fact append is wrong
		unsigned int keybase = (keywordBase) ? Word2Index(keywordBase) : 0;
		if (verify->objectHead != keybase)
			return 1; 
		if (verify->botBits != beforefact)
		{
			return 1;
		}
		lastFactUsed = verify - 1;
		return 2; // good load
	}
	else return 0; // not in new format (old fact.bin in dictionary)
}

static char* WriteField(MEANING& T, uint64 flags,char* buffer,bool ignoreDead, bool displayonly, bool jsonString) // uses no additional memory
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
		ReportBug((char*)"Missing fact field");
		*buffer++ = '?';
	}
    else 
	{
		WORDP D = Meaning2Word(T);
		if (!D)
		{
			*buffer = 0;
			return buffer; //   cancels print
		}
		if (SUPERCEDED(D))
		{
			unsigned int oldlang = language_bits;
			language_bits = LANGUAGE_UNIVERSAL; // go universal
			D = FindWord(D->word);
			if (!D)
			{
				*buffer = 0;
				return buffer; //   cancels print
			}
			T &= -1 ^ MEANING_BASE; // keep upper bits
			T |= MakeMeaning(D);
			language_bits = oldlang;
		}
		if ( D->internalBits & (INTERNAL_MARK|DELETED_MARK) && !ignoreDead) // a deleted field
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
		bool embeddedspace = !quoted && (strchr(answer,' ')  || strchr(answer,'(') || strchr(answer, '"')  || strchr(answer,')')); // does this need protection? blanks or function call maybe
		bool safe = true; 
		if (!*answer || strchr(answer,'\\') || strchr(answer,'/')) safe = false;
        if (quoted && jsonString) safe = false;
        
		// json must protect: " \ /  nl cr tab  we are not currently protecting bs ff
		if ((embeddedbacktick || !safe) && !displayonly) // uses our own marker and can escape data
		{
			if (embeddedbacktick) strcpy(buffer,(char*)"`*^");
			else strcpy(buffer,(char*)"`"); // has blanks or paren or just starts with ", use internal string notation
			buffer += strlen(buffer);
            bool normal = quoted && jsonString ? true : displayonly;
			AddEscapes(buffer,D->word,normal,currentOutputLimit - (buffer - currentOutputBase)); // facts are not normal
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

unsigned int GetFullFactLanguage(FACT* F)
{
	if (!F) return 0;

	// a fact can only be composed of fields of the same language (some may be of UNIVERSAL)
	WORDP SUBJ,VERBX, OBJ;
	SUBJ = Meaning2Word(F->subject);
	VERBX = Meaning2Word(F->verb);
	OBJ = Meaning2Word(F->object);
	int flang, vlang, olang;
	flang = (F->flags & FACTSUBJECT) ?  GetFullFactLanguage(Index2Fact(F->subject)) : (SUBJ->internalBits & LANGUAGE_BITS);
	if (SUBJ->internalBits & UNIVERSAL_WORD) flang = LANGUAGE_UNIVERSAL;
	vlang = (F->flags & FACTVERB) ? GetFullFactLanguage(Index2Fact(F->verb)) : (VERBX->internalBits & LANGUAGE_BITS);
	if (VERBX->internalBits & UNIVERSAL_WORD) vlang = LANGUAGE_UNIVERSAL;
	olang  = (F->flags & FACTOBJECT) ? GetFullFactLanguage(Index2Fact(F->object)) :( OBJ->internalBits & LANGUAGE_BITS);
	if (OBJ->internalBits & UNIVERSAL_WORD) olang = LANGUAGE_UNIVERSAL;
	unsigned int languagebits = flang | vlang | olang;
	languagebits >>= LANGUAGE_SHIFT;
	return languagebits;
}

static void ValidFieldLanguage(MEANING& M)
{
	if (!M || !verifyFacts)  return;
	WORDP D = Meaning2Word(M);
	if (SUPERCEDED(D))
	{
		D = FindWord(D->word); // should get universal
		if (D)
		{
			unsigned int bits = M & (-1 ^ MEANING_BASE);
			M = bits | MakeMeaning(D);
		}
		else
		{
			int xx = 0; // didnt find?
		}
	}
	unsigned int lang = (D->internalBits & LANGUAGE_BITS) ;
	if (lang && language_bits != lang) // nonuniversal word in fact of wrong language
	{
		int xx = 0; 
	}
}

char* WriteFact(FACT* F,bool comment,char* buffer,bool ignoreDead,bool eol,bool displayonly) // uses no extra memory
{ //   if fact is junk, return the null string
	char* start = buffer;
	*buffer = 0;
	if (!blockshow && verifyFacts)
	{
		VerifyFact(F);
	}
	if (!F || !F->subject) return start; // never write special facts out
	if (F->flags & FACTDEAD && !compiling ) 
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

	unsigned int index = Fact2Index(F);
	unsigned int oldlanguagebits = language_bits;
	language_bits = (F->flags & FACTLANGUAGEBITS) << LANGUAGE_SHIFT;
	WORDP XX = Meaning2Word(F->subject);
	WORDP YY = Meaning2Word(F->verb);
	WORDP ZZ = Meaning2Word(F->object);
	if (!(F->flags & FACTSUBJECT)) ValidFieldLanguage(F->subject);
	if (!(F->flags & FACTVERB)) ValidFieldLanguage(F->verb);
	if (!(F->flags & FACTOBJECT)) ValidFieldLanguage(F->object);

	//   do subject
	char* base = buffer;
	buffer = WriteField(F->subject,F->flags & FACTSUBJECT,buffer,ignoreDead,displayonly,false);
	if (base == buffer ) 
	{
		*start = 0;
		return start; //    word itself was removed from dictionary
	}
	base = buffer;
	buffer = WriteField(F->verb,F->flags & FACTVERB,buffer,ignoreDead, displayonly,false);
	if (base == buffer ) 
	{
		*start = 0;
		return start; //    word itself was removed from dictionary
	}

	base = buffer;
	int autoflag = F->flags & FACTOBJECT;
    bool jsonString = F->flags & JSON_STRING_VALUE ? true : false;
	MEANING X = F->object;
	if (F->flags & FACTAUTODELETE && F->flags & (JSON_OBJECT_FACT| JSON_ARRAY_FACT)) 
	{
		autoflag = FACTOBJECT;
		X = atoi(Meaning2Word(F->object)->word);
	}
	buffer = WriteField(X,autoflag,buffer,ignoreDead, displayonly, jsonString);
	if (base == buffer ) 
	{
		*start = 0;
		return start; //    word itself was removed from dictionary
	}

	//   add properties
    unsigned int flags = F->flags & (-1 ^ (MARKED_FACT | MARKED_FACT2));
   // BUG flags &= (-1 ^ FACTLANGUAGEBITS);
	// BUG if (multidict) flags |= GetFullFactLanguage(F); // fields may change to universal
    if (flags || F->botBits)
	{
        sprintf(buffer,(char*)"x%08x ",flags);  // dont show markers
        buffer += strlen(buffer);
	}

	// add bot owner flags
	if (F->botBits)
	{
		strcpy(buffer, PrintU64(F->botBits));
		buffer += strlen(buffer);
		*buffer++ = ' '; // insure space after
	}

	//   close fact
	*buffer++ = ')';
	*buffer = 0;
	if (eol) strcat(buffer,(char*)"\r\n");

	language_bits = oldlanguagebits;
	return start;
}

char* ReadField(char* ptr,char* field,char fieldkind, unsigned int& flags)
{
	if (*ptr == '(')
	{
		FACT* G = ReadFact(ptr, 0);
		if (fieldkind == 's') flags |= FACTSUBJECT;
		else if (fieldkind == 'v') flags |= FACTVERB;
		else if (fieldkind == 'o') flags |= FACTOBJECT;
		if (!G)
		{
			ReportBug((char*)"Missing fact field");
			return NULL;
		}
		sprintf(field,(char*)"%u",Fact2Index(G)); 
	}
	else if (*ptr == ENDUNIT) // internal string token (fact read)
	{
		char* end;
		char* start;
		if (ptr[1] == '*' && ptr[2] == '^') // they use our ` in their field - we use `*^%s`*^ --- BUG ?? Do we even believe we allow them access to ` any more?
		{
			start = end = ptr+3;
			while ((end = strchr(end,'^')))
			{
				if (*(end-1) == '*' && *(end-2) == '`') break;  // found our close
				++end;
			}
			if (!end)
			{
				end = strchr(ptr+3,ENDUNIT);
				if(!*end){
					ReportBug("No end found");
					return NULL;
				}
				*end = 0;
				start = ptr + 1;
			}
			else
			{
				*(end-2) = 0; // terminate string with null
			}
		}
		else
		{
			end = strchr(ptr+1,ENDUNIT); // find corresponding end
			start = ptr + 1;
			if (!end)
			{
				ReportBug("No end found");
				return NULL;
			}
			*end = 0;
		}

		CopyRemoveEscapes(field,start,90000);	// we only remove ones we added
		return end+2; // point AFTER the space after the closer
	}
    else  ptr = ReadToken(ptr,field); // no escaping or anything weird needed
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
    ptr = ReadToken(ptr,word);
    if (*word == '0') return 0; 
	unsigned int flags = 0;
	if (build == BUILD1) flags |= FACTBUILD1;
	char* subjectname = AllocateBuffer(); 
	ptr = ReadField(ptr,subjectname,'s',flags);
    char* verbname = AllocateBuffer();
	ptr = ReadField(ptr,verbname,'v',flags);
    char* objectname = AllocateBuffer();
	ptr = ReadField(ptr,objectname,'o',flags);
    if (!ptr) return 0;
    
    //   handle the flags on the fact
    uint64 properties = 0;
	bool bad = false;
	bool response = false;
    if (!*ptr || *ptr == ')' ); // end of fact
	else ptr = ReadFlags(ptr,properties,bad,response,true);
	flags |= (unsigned int) properties;
	unsigned int oldlanguage = language_bits;
	unsigned int factlang = flags & FACTLANGUAGEBITS;
	if (multidict && factlang) language_bits = factlang << LANGUAGE_SHIFT;
	else if (!multidict && factlang > max_fact_language)
	{
		FreeBuffer();
		FreeBuffer();
		FreeBuffer();
		return NULL; // not using these facts
	}
	unsigned int language = 0;
    if (flags & FACTSUBJECT) subject = (MEANING) atoi(subjectname);
	else
	{
		subject = ReadMeaning(subjectname, true, true);
		if (subject)
		{
			WORDP D = Meaning2Word(subject);
			if (!(D->internalBits & UNIVERSAL_WORD)) language |= D->internalBits & LANGUAGE_BITS;
		}
	}
	if (flags & FACTVERB) verb = (MEANING) atoi(verbname);
	else
	{
		verb = ReadMeaning(verbname, true, true);
		if (verb)
		{
			WORDP D = Meaning2Word(verb);
			if (!(D->internalBits & UNIVERSAL_WORD)) language |= Meaning2Word(verb)->internalBits & LANGUAGE_BITS;
		}
	}
	if (flags & FACTOBJECT) 
	{
		if (flags & FACTAUTODELETE && flags & (JSON_OBJECT_FACT| JSON_ARRAY_FACT)) 
		{
			object = ReadMeaning(objectname,true,true);
			flags ^= FACTOBJECT;
		}
		else
		{
			object = (MEANING)atoi(objectname);
			if (object)
			{
				WORDP D = Meaning2Word(object);
				if (!(D->internalBits & UNIVERSAL_WORD)) language |= Meaning2Word(object)->internalBits & LANGUAGE_BITS;
			}
		}
	}
    else if (!*objectname && flags & (JSON_OBJECT_FACT | JSON_STRING_VALUE))
    {
        object = MakeMeaning(StoreWord(objectname, AS_IS)); // obj.key = ""
    }
	else
	{
		object = ReadMeaning(objectname, true, true);
		if (object)
		{
			WORDP D = Meaning2Word(object); 
			if (!(D->internalBits & UNIVERSAL_WORD)) language |= Meaning2Word(object)->internalBits & LANGUAGE_BITS;
		}
	}
	if (multidict && language && !(flags & FACTLANGUAGEBITS)) flags |= language >> LANGUAGE_SHIFT;
	FreeBuffer();
	FreeBuffer();
	FreeBuffer();
	uint64 oldbot = myBot;
	myBot = 0; // no owner by default unless read in by fact
	if (*ptr && *ptr != ')') ptr = ReadInt64(ptr,(int64&)myBot); // read bot bits

    FACT* F = FindFact(subject, verb,object,flags);
    if (!F)   F = CreateFact(subject,verb,object,flags); 
	
	if (*ptr == ')') ++ptr;	// skip over ending )
	ptr = SkipWhitespace(ptr);
	myBot = oldbot;
	language_bits = oldlanguage;
    return F;
}

bool ReadFacts(const char* name,const char* layer,int build,bool user) //   a facts file may have dictionary augmentations and variable also
{
	// build of -1 is wordnet data
	bool wordnet = false;
	if (build == -1)
	{
		wordnet = true;
		build = 0;
	}
  	char word[MAX_WORD_SIZE];
	sprintf(word, "%s/%s", topicfolder, name);
	if (layer) sprintf(word, "%s/%s", topicfolder, name); // from topic load
	else strcpy(word, name); // from dictionary load
	FILE* in = (user) ? FopenReadWritten(word) : FopenReadOnly(word); //  TOPIC fact/DICT files
	if (!in && layer)
	{
		sprintf(word, "%s/BUILD%s/%s", topicfolder, layer, name);
		in = (user) ? FopenReadWritten(word) : FopenReadOnly(word);
	}
	if (!in) return true;
	char wordx[MAX_WORD_SIZE];
	char data[500];
	uint64 val;

	StartFile(name);
	if (build >= 0 && !wordnet) // only for TOPIC facts, not dict facts or livedata facts
	{
		ReadALine(readBuffer, in); // checkstamp
		int stamp = atoi(readBuffer);
		if (stamp != CHECKSTAMPRAW)
		{
			FClose(in);
			EraseTopicFiles(0, "0");
			EraseTopicFiles(1, "1");
			printf("Erase your TOPIC folder, rerun CS and recompile your bot. Data formats have changed\r\n");
			ReportBug("FATAL: Erase your TOPIC folder, rerun CS and recompile your bot. Data formats have changed\r\n");
		}
		char* ptr = strchr(readBuffer, ' '); // new field languages required?
		if (ptr && strnicmp(language_list, ptr+1, strlen(ptr+1)))
		{
			printf("FATAL: languages=%s but build requires %s. Alter your languages= init or  Erase your TOPIC folder, rerun CS and recompile your bot.", language_list, ptr);
			ReportBug("FATAL: languages=%s but build requires %s. Alter your languages= init or  Erase your TOPIC folder, rerun CS and recompile your bot.", language_list, ptr);
		}
	}

	int oldlanguage = language_bits;
    while (ReadALine(readBuffer, in) >= 0)
    {
		language_bits = oldlanguage;
		char* ptr = ReadToken(readBuffer, word);
		if (*word == 0 || (*word == '#' && word[1] != '#')); //   empty or comment
		else if (*word == '-' && word[1] == '-') continue; // section separator
		else if (*word == '+' && word[1] == 'q') // defining a private query
		{
			ptr = ReadToken(ptr, word); // name
			WORDP D = StoreWord(word,AS_IS);
			if (monitorChange) AddWordItem(D, false);
			AddInternalFlag(D, (unsigned int)(QUERY_KIND | build));
			ReadToken(ptr, word);
			D->w.userValue = AllocateHeap(word + 1);
			continue;
		}
		else if (*word == '-' && word[1] == 'f' && word[2] == 'l') // change fact language
		{
			ptr = ReadToken(ptr, word);
			unsigned int index = atoi(word);
			FACT* F = Index2Fact(index);
			if (F > lastFactUsed) // level 1 being loaded on a level 0 that changed
			{
				FClose(in);
				EraseTopicFiles(BUILD1, "1");
				EraseTopicBin(BUILD1, (char*)"1");
				char buf[MAX_WORD_SIZE];
				strcpy(buf, StdIntOutput(lastFactUsed - factBase));
				ReportBug((char*)"FATAL: Illegal fact index %d last:%s  Level 0 changed. Recompile your level 1 file. ", index, buf);
			}
			if (F)
			{
				unsigned int flags = F->flags & (-1 ^ FACTLANGUAGEBITS);
				flags |= GetFullFactLanguage(F);
				F->flags = flags;
			}
			else
			{
				FClose(in);
				ReportBug("fact missing %s %s", word, ptr);
				return false;
			}
		}
		else if (*word == '-' && word[1] == 'f' && word[2] == 0) // delete fact
		{
			ptr = ReadToken(ptr, word);
			unsigned int index = atoi(word);
			FACT* F = Index2Fact(index);
			if (F) F->flags |= FACTDEAD;
		}
		else if (*word == '-') // delete dict entry
		{
			ptr = ReadToken(ptr, word);
			unsigned int l  = LanguageRestrict(word);
			if (l) language_bits = l;
			WORDP D = FindWord(word, 0, PRIMARY_CASE_ALLOWED, true); // from this specific language
			if (D) KillWord(D,false); // supercede  dictionary entry
			else
			{
				int xx = 0; // already dead?
			}
		}
		else if (*word == '^' || *word == '+') // add/modify  dictionary entry or raw dict fact
		{
		    bool newWord = false;
			char* at = ReadToken(readBuffer+2,wordx); //   start at valid token past space
			
			unsigned int l = LanguageRestrict(wordx);
			if (l) language_bits = l;
			WORDP D = FindWord(wordx, 0, PRIMARY_CASE_ALLOWED,true);
			if (!D || strcmp(D->word,wordx)) // alternate capitalization?
			{
				D = StoreWord(wordx,AS_IS);
                newWord = true;
				AddInternalFlag(D,(unsigned int)build);
			}
			AddWordItem(D, false); 
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
				if (sign) AddInternalFlag(D,CONSTANT_IS_NEGATIVE);
			}
			else if (wordnet ) 
				ReadDictionaryFlags(D,at);
			else // from script file
			{
				val = 0;
				if (!D) StoreWord(wordx,AS_IS);
				// get properties
				at = ReadToken(at, data);
				if (*data == '0') D->properties = 0;
				else if (*data == '@') {;}
				else
				{
					ReadHex(data, val);
					D->properties = val << 32;
					at = ReadToken(at, data);
					ReadHex(data, val);
					D->properties |= val;
				}
				// get systemflags
				at = ReadToken(at, data);
				if (*data == '0') D->systemFlags = 0;
				else if (*data == '@') {;}
				else if (*data == 'P') { D->systemFlags = PATTERN_WORD; }
				else
				{
					ReadHex(data, val);
					D->systemFlags = val << 32;
					at = ReadToken(at, data);
					ReadHex(data, val);
					D->systemFlags |= val;
				}
				// get intbits
				at = ReadToken(at, data);
				if (*data == '0') D->internalBits = 0;
				else if (*data == '@') {;}
				else
				{
					ReadHex(data, val);
					D->internalBits = (unsigned int)val;
				}
				at = ReadToken(at, data);
				ReadHex(data,val);
				D->parseBits = (unsigned int) val;

				at = ReadToken(at, data);
				if (*data == 0) { ; }// no change to subsitutions
				else if (D->systemFlags & HAS_SUBSTITUTE)
				{
					WORDP S = StoreWord(data, AS_IS);
					D->w.substitutes = S;
				}
				else if (D->internalBits & CONDITIONAL_IDIOM)
				{
					WORDP S = StoreWord(data, AS_IS);
					D->w.conditionalIdiom = S;
				}
				else // shouldnt happen
				{
					int xx = 0;
				}
				// get header count
				at = ReadToken(at, data);
				if (*data)
				{
					unsigned int n = atoi(data);
					SETMULTIWORDHEADER(D, n);
				}
				// FF
				at = SkipWhitespace(at);
				if (*at == 'F' && at[1] == 'F')
				{
					if (!D->foreignFlags) D->foreignFlags = (WORDP*)AllocateHeap(NULL, languageCount, sizeof(WORDP), true);
					for (unsigned int i = 0; i < languageCount; ++i)
					{
						int data;
						at = ReadInt(at, data);
						D->foreignFlags[i]  = Index2Word(data);
					}
				}

			}
            if (monitorChange && D && (newWord || D->internalBits & BIT_CHANGED)) AddWordItem(D, false);
		}
		else if (*word == USERVAR_PREFIX) // old style compile puts variable init in fact file
		{
			char* eq = strchr(word, '=');
			if (!eq) ReportBug((char*)"Bad fact file user var assignment %s", word);
			else
			{
				*eq = 0;
				SetUserVariable(word, eq + 1);

				// put into variables file for next round proper behavior
				


				WORDP D = FindWord(word);
				if (monitorChange)
				{
					AddWordItem(D, false);
					D->internalBits |= BIT_CHANGED;
				}
			}
		}
        else 
		{
			char* xptr = readBuffer;
			FACT* F = ReadFact(xptr,build); // will write on top of ptr... must not be readBuffer variable
			if (!F) continue; // language restrict may have killed it
			if (monitorChange && F)
			{
				WORDP X;
				if (!(F->flags & FACTSUBJECT))
				{
					X = Meaning2Word(F->subject);
					AddWordItem(X, false);
					X->internalBits |= BIT_CHANGED;
				}
				if (!(F->flags & FACTVERB)) 
				{
					X = Meaning2Word(F->verb);
					AddWordItem(X, false);
					X->internalBits |= BIT_CHANGED;
				}
				if (!(F->flags & FACTOBJECT)) 
				{
					X = Meaning2Word(F->object);
					AddWordItem(X, false);
					X->internalBits |= BIT_CHANGED;
				}
			}
		}
    }
   FClose(in);
   language_bits =  oldlanguage;
   return true;
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

static STACKREF PutBootVariables()
{
    STACKREF newThread = NULL;

    while (botVariableThreadList)
    {
        uint64 Dx;
        size_t size = 0;
        
        botVariableThreadList = UnpackHeapval(botVariableThreadList, Dx, discard, discard);
        WORDP D = (WORDP)Dx;
        
        size = strlen(D->word) + 1;
        if (D->w.userValue) size += strlen(D->w.userValue);
        size += 8; // eos + align

        uint64* data = (uint64*)AllocateStack(NULL, size, false, 8);
        memset(data, 0, size);
        
        char* var = (char*)data;
        strcpy(var, D->word);
        var += strlen(D->word) + 1;
        if (D->w.userValue) strcpy(var, D->w.userValue);

        newThread = AllocateStackval(newThread, (uint64)data);
    }

    return newThread;
}

static void RestoreBootVariables(STACKREF thread)
{
    while (thread)
    {
        uint64 data;
        thread = UnpackStackval(thread, data);

        char* name = (char*)data;
        char* value = name + strlen(name) + 1;

        WORDP D = StoreWord(name, AS_IS);
        if (*value)
        {
            D->w.userValue = AllocateHeap(value);
        }

        D->internalBits |= (unsigned int)BOTVAR; // mark it
        botVariableThreadList = AllocateHeapval(HV1_WORDP|HV2_STRING,botVariableThreadList, (uint64)D, (uint64)D->w.userValue, (uint64)0);
   }
}

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
    WORDP D = StoreFlaggedWord(name, properties, sysflags);
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
    while (factThreadList) // heap list
    {
        uint64 val;
        factThreadList = UnpackHeapval(factThreadList, val,discard,discard);
        FACT* F = Index2Fact((unsigned int)val);
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
    stackfactThreadList = newThread; // now have stack-based copy of needed data
}

void RedoSystemFactFields() // see NoteBotFacts for inverse
{
    while (stackfactThreadList) 
    {
        uint64 Fx;
        uint64 changed;
        uint64 datax;
        stackfactThreadList = UnpackStackval(stackfactThreadList, Fx, changed, datax);
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
	base += (sizeof(uint64) * 2);
	strcpy(base, D->word);
	uint64 align = (uint64)(base + strlen(base) + 1 + 7);  // length + terminator + align
	align &= 0xFFFFFFFFFFFFFFf8ULL;
	return (char*)align;
}

static char* GetBlob(char* base, WORDP& D, unsigned int flags)
{
	uint64* val = (uint64*)base;
	uint64 prop = *val++;
	uint64 sys = *val++;
	char* name = (char*)val;
	if (flags & (JSON_ARRAY_FACT | JSON_OBJECT_FACT)) prop |= AS_IS;
	D = StoreFlaggedWord(name, prop, sys);
	uint64 align = (uint64)(name + strlen(name) + 1 + 7);  // length + terminator + align
	align &= 0xFFFFFFFFFFFFFFf8ULL;
	return (char*)align;
}

void MigrateFactsToBoot(FACT* endUserFacts, FACT* endBootFacts)
{ // transfer facts from user layer to boot layer

    // if we purged stuff from boot, then we need to fix end of boot
    FACT* G = factsPreBuild[LAYER_USER]; // last fact in boot
    while (!G->subject) --G;
    lastFactUsed = factsPreBuild[LAYER_USER] = G;

    if (!bootFacts && !factThreadList) return;

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
    char* blobs = AllocateBuffer("MigrateFactsToBoot"); // use buffers to temporarily stash words
    char* buffersEnd = buffers + (maxBufferSize * maxBufferLimit) - (3 * MAX_WORD_SIZE); // don't get too close to the real end
    char* startblobs = blobs;
    // dictionaryFree has already been reset to boot layer
    // will need to adjust those words that were part of the user layer
    FACT* bootFactStart = factsPreBuild[LAYER_BOOT];
    WORDP bootDictStart = dictionaryPreBuild[LAYER_BOOT];
   
    // facts will be simple, but perhaps involve dictionary entries that need moving
    F = bootFactStart;
    while (++F <= endUserFacts) // note dictionary name and data before we lose them to reuse
    {
        // if fact is moving and involves dictionary entries past boot, we need to replicate those temporarily
        if ((F->flags & FACTBOOT || F <= endBootFacts) && !(F->flags & (FACTDEAD | FACTTRANSIENT)))
        {
            WORDP SD = Meaning2Word(F->subject);
            if (SD >= bootDictStart) blobs = PutBlob(SD, blobs);
            WORDP VD = Meaning2Word(F->verb);
            if (VD >= bootDictStart) blobs = PutBlob(VD, blobs);
            WORDP OD = Meaning2Word(F->object);
            if (OD >= bootDictStart) blobs = PutBlob(OD, blobs);

            if (blobs >= buffersEnd) myexit((char*)"FATAL Out of buffer space in MigratefactsToBoot");
        }
    }
    
    // stash the boot layer variables in the stack
    STACKREF bootVariables = PutBootVariables();
    
    char* startdictblobs = blobs;
    WORDP D = dictionaryPreBuild[LAYER_BOOT] - 1; // end word of layer_1
    WORDP endBootWord = dictionaryPreBuild[LAYER_USER] - 1; // last word of boot layer
    while (++D <= endBootWord) // walk all words of boot layer
    {
        unsigned int hash = (D->hash % maxHashBuckets);
        if (D->internalBits & UPPERCASE_HASH) ++hash;
        if (D->internalBits & BOTVAR) continue;
        if (!(D->internalBits & DELETED_MARK))
        {
            blobs = PutBlob(D, blobs);
        }
    }
    char* enddictblobs = blobs;

    NoteBotFacts(); // store changes to system facts on stack as well
    
    // how many actual buffers have been used?
    // need to know and adjust in case other functions, i.e. StoreWord() allocate buffers
    unsigned int bufferIndexStart = bufferIndex;
    unsigned int numBuffers = ((blobs - startblobs) / maxBufferSize) + 1;
    bufferIndex += numBuffers;
    
    // reset the dictionary
    DictionaryRelease(dictionaryPreBuild[LAYER_BOOT], heapPreBuild[LAYER_BOOT]);
    
    // now rebuild facts and dictionary items for real into boot layer
    RestoreBootVariables(bootVariables);
    
    blobs = startdictblobs;
    while (blobs < enddictblobs)
    {
		WORDP E;
        blobs = GetBlob(blobs, E, 0);
    }


    blobs = startblobs;
    F = bootFactStart;
    while (++F <= endUserFacts) 
    {
        // facts will be simple, but perhaps involve dictionary entries that need moving
        if ((F->flags & FACTBOOT || F <= endBootFacts) && !(F->flags & (FACTDEAD | FACTTRANSIENT)))
        {
            // get all major data from this fact, dangling in user space
            unsigned int flags = F->flags;
            flags &= -1 ^ (MARKED_FACT | MARKED_FACT2 | FACTTRANSIENT | FACTBOOT);
            WORDP SD = Meaning2Word(F->subject);
            if (SD >= bootDictStart) blobs = GetBlob(blobs, SD, flags);
            WORDP VD = Meaning2Word(F->verb);
            if (VD >= bootDictStart) blobs = GetBlob(blobs, VD, flags);
            WORDP OD = Meaning2Word(F->object);
            if (OD >= bootDictStart) blobs = GetBlob(blobs, OD, flags);
            uint64 botbits = F->botBits;

            FACT* X;
            if (F <= endBootFacts)
            {
                UnweaveFact(F);
                X = F;
            }
            else
            {
                X = ++lastFactUsed; // allocate new fact (may even be this fact!)
                memset(X, 0, sizeof(FACT)); // botbits = 0 (all can see)
                X->botBits = botbits;
                X->flags = flags;
            }
            X->subject = MakeMeaning(SD);
            X->verb = MakeMeaning(VD);
            X->object = MakeMeaning(OD);
            WeaveFact(X); // interlace to dictionary

            if (bootwrite)
            {
                WriteFact(X, false, buffer, false, true);
                fprintf(bootwrite, "%s", buffer);
            }
        }
    }
    RedoSystemFactFields();
    bufferIndex = bufferIndexStart;
    FreeBuffer();  // blobs

    // need to cause the prebuild file to be regenerated with the new words
    LockLayer();

    if (buffer)
    {
        FreeBuffer("MigrateFactsToBoot");
        FClose(bootwrite);
    }
    bootFacts = false;
}

void ModBaseFact(FACT* F) // anyone changing fields of facts calls this
{
    if (F > factLocked) return; // dont care about user layer facts
    WORDP userdictstart = dictionaryPreBuild[LAYER_BOOT + 1];
    if (Meaning2Word(F->subject) >= userdictstart) {}
    else if (Meaning2Word(F->verb) >= userdictstart) {}
    else if (Meaning2Word(F->object) >= userdictstart) {}
    else return; // nothing from user space here 
    factThreadList = AllocateHeapval(HV1_FACTI,factThreadList, (uint64)Fact2Index(F));
}


