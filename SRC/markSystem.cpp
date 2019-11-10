// markSystem.cpp - annotates the dictionary with what words/concepts are active in the current sentence

#include "common.h"

#ifdef INFORMATION

For every word in a sentence, the word knows it can be found somewhere in the sentence, and there is a 64-bit field of where it can be found in that sentence.
The field is in a hashmap and NOT in the dictionary word, where it would take up excessive memory.

Adjectives occur before nouns EXCEPT:
	1. object complement (with some special verbs)
	2. adjective participle (sometimes before and sometimes after)

In a pattern, an author can request:
	1. a simple word like bottle
	2. a form of a simple word non-canonicalized like bottled or apostrophe bottle
	3. a WordNet concept like bottle~1 
	4. a set like ~dead or :dead

For #1 "bottle", the system should chase all upward all sets of the word itself, and all
WordNet parents of the synset it belongs to and all sets those are in. 

Marking should be done for the original and canonical forms of the word.

For #2 "bottled", the system should only chase the original form.

For #3 "bottle~1", this means all words BELOW this in the wordnet hierarchy not including the word
"bottle" itself. This, in turn, means all words below the particular synset head it corresponds to
and so instead becomes a reference to the synset head: (char*)"0173335n" or some such.

For #4 "~dead", this means all words encompassed by the set ~dead, not including the word ~dead.

So each word in an input sentence is scanned for marking. 
the actual word gets to see what sets it is in directly. 
Thereafter the system chases up the synset hierarchy fanning out to sets marked from synset nodes.

#endif

#define GENERIC_MEANING 0  // not a specific meaning of the word

int maxRefSentence = (((MAX_XREF_SENTENCE  * REF_ELEMENTS) + 3) / 4) * 4; // start+end offsets for this many entries + alignment slop
int uppercaseFind = -1; // unknown
static bool failFired = false;
bool trustpos = false;
int marklimit = 0;
static STACKREF wordlist = NULL;
static HEAPREF pendingConceptList = NULL;
static int MarkSetPath(int depth, int exactWord, MEANING M, int start, int end, unsigned int level, bool canonical); //   walks set hierarchy
// mark debug tracing
bool showMark = false;
static unsigned int markLength = 0; // prevent long lines in mark listing trace
#define MARK_LINE_LIMIT 80
int upperCount, lowerCount;
ExternalTaggerFunction externalPostagger = NULL;
char unmarked[MAX_SENTENCE_LENGTH]; // can completely disable a word from mark recognition

bool RemoveMatchValue(WORDP D, int position)
{
    int access = GetAccess(D);
    if (!access) return false;
    bool changed = false;
    unsigned char* data = (unsigned char*)  (Index2Heap(access) + 8);	// skip over 64bit tried by meaning field
    unsigned char* tried = NULL;
    bool didmod = false;
	for (int i = 0; i < maxRefSentence; i += REF_ELEMENTS)
	{
		if (data[i] == position) 
		{
            didmod = true;
            if (!changed)// protect by moving data to new area so restoresentence is safe
            {
                int newaccess = CopyWhereInSentence(access);
                tried = (unsigned char*)(Index2Heap(newaccess) + 8);
                changed = true;
                SetTried(D, newaccess);
            }
			memmove(tried +i, tried +i+ REF_ELEMENTS,(maxRefSentence - i - REF_ELEMENTS));
			break;
		}
	}
    return didmod;
}

static int WhereWordHitWithData(WORDP D, int start,unsigned char* data)
{
	if (data) for (int i = 0; i < maxRefSentence; i += REF_ELEMENTS)
	{
		if (data[i] >= start)
		{
			if (data[i] == start) return data[i + 1]; // return end of it
			else break; // cannot match later - end of data will be 0xff for all
		}
	}
	return 0;
}

static int WhereWordHit(WORDP D, int start)
{
	unsigned char* data = GetWhereInSentence(D);
	if (data) for (int i = 0; i < maxRefSentence; i += REF_ELEMENTS)
	{
		if (data[i] >= start)
		{
			if (data[i] == start) return data[i + 1]; // return end of it
			else break; // cannot match later
		}
	}
	return 0;
}

bool MarkWordHit(int depth, int exactWord, WORDP D, int meaningIndex, int start, int end)
{	//   keep closest to start at bottom, when run out, drop later ones 
    if (!D || !D->word) return false;
	if (start > wordCount) 
	{
		ReportBug((char*)"save position is too big")
		return false;
	}
	if (end > wordCount) end = wordCount;

	// been here before?
	unsigned char* data = GetWhereInSentence(D); // has 2 hidden int fields before this point
	if (!data)  data = (unsigned char*)AllocateWhereInSentence(D);
	if (!data) return false; // allocate failure
	int whereHitEnd = WhereWordHitWithData(D, start,data); // word in sentence index

	if (*D->word != '~') // real word, not a concept
	{
		uint64 meaningBit = 1ull << meaningIndex; // convert index into bit
		if (whereHitEnd < end) SetTriedMeaningWithData(D, GENERIC_MEANING,(unsigned int*)data); 
		uint64 triedBits = GetTriedMeaning(D);
		if (meaningBit & triedBits) return false; // did this meaning already
		SetTriedMeaningWithData(D, triedBits | meaningBit, (unsigned int*)data); // update more
	}
	else if (whereHitEnd >= end) return false; // no need since already covering concept in this area
	if (++marklimit > 5000)
	{
		if (!failFired) ReportBug("Mark limit hit")
		failFired = true;
		return false;
	}

	// diff < 0 means peering INSIDE a multiword token before last word
	// we label END as the word before it (so we can still see next word) and START as the actual multiword token
	bool added = false;
	for (int i = 0; i < maxRefSentence; i += REF_ELEMENTS)
	{
		unsigned char begin = data[i];
		if (begin == 0 || (begin > wordCount && begin != 0xff)) // CANNOT BE TRUE
		{
			static bool did = false; // report ONLY once
			if (!did) ReportBug((char*)"illegal whereref for %s at %d\r\n",D->word,volleyCount);
			did = true;
			return false;
		}

		if (begin < start) continue; // skip over it
		else if (begin == start) // we have already marked this somewhat
		{
			if (end > data[i+1]) added = true; // prefer the longer match
		}
		else // insert here
		{
			memmove(data+i+ REF_ELEMENTS,data+i,maxRefSentence - i - REF_ELEMENTS); // create a hole for entry
			data[i] = (unsigned char)start;
			added = true;
		}
		if (added)
		{
			data[i + 1] = (unsigned char)end;
			data[i + 2] = (unsigned char)(exactWord >> 24);
			data[i + 3] = (unsigned char)(exactWord >> 16);
			data[i + 4] = (unsigned char)((exactWord >> 8) & 0xff);
			data[i + 5] = (unsigned char)exactWord;
		}
		break; // have location
	}
	if (added)
	{
		if (*D->word == '~')// track the actual sets done matching start word location (good for verbs, not so good for nouns)
		{
			if (!(D->internalBits & TOPIC)) Add2ConceptTopicList(concepts, D, start, end, false); // DOESNT need to be be marked as concept
			else Add2ConceptTopicList(topics, D, start, end, false);
		}
		if ((trace & (TRACE_PREPARE | TRACE_HIERARCHY) || prepareMode == PREPARE_MODE || showMark) && (D->word[0] != '~' || !IsDigit(D->word[1])))
		{
			markLength += D->length;
			if (markLength > MARK_LINE_LIMIT)
			{
				markLength = 0;
				Log(STDUSERLOG, (char*)"\r\n");
				Log(STDTRACETABLOG, (char*)"");
			}
			while (depth-- >= 0) Log((showMark) ? ECHOSTDUSERLOG : STDUSERLOG, (char*)"  ");
			char which[20];
			*which = 0;
			which[1] = 0;
			if (exactWord && D->internalBits & UPPERCASE_HASH) which[0] = '^';
			Log((showMark) ? ECHOSTDUSERLOG : STDUSERLOG, (D->internalBits & TOPIC) ? (char*)"+T%s%s " : (char*)" +%s%s", D->word, which);
			Log((showMark) ? ECHOSTDUSERLOG : STDUSERLOG, (char*)"(%d-%d)\r\n", start, end);
			markLength = 0;
		}
	}
    return added;
}

unsigned int GetIthSpot(WORDP D, int i, int& start, int& end)
{
    if (!D) return 0; //   not in sentence
    unsigned char* data = GetWhereInSentence(D);
    if (!data) return 0;
    i *= REF_ELEMENTS;
    if (i >= maxRefSentence) return 0; // at end
    start = data[i];
    if (start == 0xff) return 0;
    end = data[i + 1];
    if (end > wordCount)
    {
        static bool did = false;
        if (!did) ReportBug((char*)"Getith out of range %s at %d\r\n", D->word, volleyCount);
        did = true;
    }
    return start;
}

unsigned int GetNextSpot(WORDP D, int start, int &startPosition, int& endPosition, bool reverse, int legalgap)
{//   spot can be 1-31,  range can be 0-7 -- 7 means its a string, set last marker back before start so can rescan
    //   BUG - we should note if match is literal or canonical, so can handle that easily during match eg
    //   '~shapes matches square but not squares (whereas currently literal fails because it is not ~shapes
    if (!D) return 0; //   not in sentence
    unsigned char* data = GetWhereInSentence(D);
    if (!data) return 0;
    uppercaseFind = -1;
    int i;
    startPosition = 0;
    for (i = 0; i < maxRefSentence; i += REF_ELEMENTS)
    {
        unsigned char at = data[i];
        unsigned char end = data[i + 1];
        if ((at > wordCount && at != 0xff) || (end > wordCount && end != 0xff))
        {
            static bool did = false;
            if (!did) ReportBug((char*)"Getith out of range %s at %d\r\n", D->word, volleyCount);
            did = true;
            return 0;	// CANNOT BE TRUE
        }
        if (unmarked[at])
        {
            int xx = 0;
        }
        else if (reverse)
        {
            if (at < start) // valid. but starts far from where we are
            {
                startPosition = at; // bug fix backward gaps as well
                endPosition = end;
                uppercaseFind = (data[i + 2] << 24) | (data[i + 3] << 16) | (data[i + 4] << 8) | (data[i + 5]);
                continue; // find the CLOSEST without going over
            }
            else if (at >= start) break;
        }
        else if (at > start)
        {
            if (at == 0xff) return 0; // end of data going forward
            if (legalgap)
            {
                if ((at - start) > legalgap) return 0; // too far away and optional
            }
            startPosition = at;
            endPosition = end;
            uppercaseFind = (data[i + 2] << 24) | (data[i + 3] << 16) | (data[i + 4] << 8) | (data[i + 5]);
            return startPosition;
        }
    }
    if (reverse) return startPosition; // we have a closest or we dont
    return 0;
}

static void TraceHierarchy(FACT* F,char* msg)
{
    if (TraceHierarchyTest(trace))
    {
        char word[MAX_WORD_SIZE];
        char* fact = WriteFact(F, false, word); // just so we can see it
        unsigned int hold = globalDepth;
        globalDepth = 4 + 1;
        if (!msg) Log(STDTRACETABLOG, (char*)"%s\r\n", fact); // \r\n
        else Log(STDTRACETABLOG, (char*)"%s (%s)\r\n", fact,msg); // \r\n
        globalDepth = hold;
    }
}

static void AddPendingConcept(FACT* F, unsigned int start, unsigned int end)
{
    pendingConceptList = AllocateHeapval(pendingConceptList, (uint64)F, start, end);
    TraceHierarchy(F,"delayed");
}

static bool ProcessPendingFact(FACT* F, int start, int end)
{
    WORDP O = Meaning2Word(F->object);
    if (WhereWordHit(O, start) >= (int)end) return true; // already marked this set
                                                    // FACT wanted to map subject to concept object, but it had an exclude on a set that needed completion
    int depth = 4;
    WORDP S = Meaning2Word(F->subject);
    int startPosition, endPosition;
    if (GetNextSpot(S, start - 1, startPosition, endPosition) && startPosition == start && endPosition == end)
    {
        TraceHierarchy(F,"");
        return true; // not allowed to proceed
    }
    return false; // unmarked. we dont know
}

static void ProcessPendingConcepts()
{
    // (~setmember  exclude ~set)
    // has subject been marked at this position, if so, we cannot trigger concept for this position
    if (!pendingConceptList) return;

    HEAPREF startList = pendingConceptList; // F start|end
    HEAPREF begin = startList;
    bool changed = false;
    while (1) 
    {
        if (!startList) // we will be cycling list trying to get changes until no list or no changes
        {
            if (!changed) break; // no improvement
            startList = begin;
            changed = false;
        }

        uint64 start;
        uint64 end;
        uint64* currentEntry = (uint64*) startList;
        uint64 Fx;
        startList = UnpackHeapval(startList, Fx, start,end);
        FACT* F = (FACT*)Fx;
        WORDP concept = NULL; // set we want to trigger
        while (F) // will be NULL if we have already finished with it
        {
            // expect exit here because writing a concept with NO members is nuts
            if (F->verb != Mexclude) break; // ran out of set restrictions 

            // before we can trigger this set membership
            concept = Meaning2Word(F->object);
            if (ProcessPendingFact(F, start, end)) // failed 
            {
                currentEntry[1] = 0; // kill fact use
                changed = true;
            }
            F = GetObjectNondeadNext(F);
        }

        // now flow path of this set upwards since all excludes have been considered
        if (F && F->verb != Mexclude)
        {
            TraceHierarchy(F,"resume");
            if (MarkWordHit(4, false, concept, 0, start, end)) // new ref added
            {
                if (MarkSetPath(4 + 1, false, F->object, start, end, 4 + 1, false) != -1) changed = true; // someone marked
            }
        }
    }

    // activate anything not already deactivated now
    while (pendingConceptList) // one entry per pending set that had excludes
    {
        bool exact = false;
        bool canonical = false;
        uint64 Fx;
        uint64 start;
        uint64 end;
        pendingConceptList = UnpackHeapval(pendingConceptList, Fx,start,end);
        if (!Fx) continue; 

        FACT* F = (FACT*)Fx;
        WORDP E = (F) ? Meaning2Word(F->object) : NULL;
        // mark all members of the link
        TraceHierarchy(F,"defaulting");
        if (MarkWordHit(4, false, E, 0, start, end)) // new ref added
        {
           MarkSetPath(4 + 1, false, F->object, start, end, 4 + 1, false); 
        }
    }
}

static int MarkSetPath(int depth,int exactWord,MEANING M, int start, int end, unsigned int level, bool canonical) //   walks set hierarchy
{//   travels up concept/class sets only, though might start out on a synset node or a regular word
    unsigned int flags = GETTYPERESTRICTION(M);
	if (!flags) flags = BASIC_POS; // what POS we allow from Meaning
	WORDP D = Meaning2Word(M);
	unsigned int index = Meaning2Index(M); // always 0 for a synset or set
	// check for any repeated accesses of this synset or set or word
	uint64 offset = 1ull << index;
	int result = NOPROBLEM_BIT;

	FACT* H = GetSubjectNondeadHead(D);  // thisword/concept member y
	while (H)
	{
		FACT* F = H;
		H = GetSubjectNondeadNext(H);
		if (F->verb != Mmember) continue;
		
		// ~concept members and word equivalent
        if (trace & TRACE_HIERARCHY) TraceHierarchy(F,"");
        WORDP concept = Meaning2Word(F->object);
        if (concept->internalBits & OVERRIDE_CONCEPT) // override by ^testpattern, is this legal fact?
        {
			if (!(F->flags & OVERRIDE_MEMBER_FACT)) continue; // pretend he doesnt exist
        }

        // if subject has type restriction, it must pass
		unsigned int restrict = GETTYPERESTRICTION(F->subject );
		if (!restrict && index) restrict = GETTYPERESTRICTION(GetMeaning(D,index)); // new (may be unneeded)
 
		// reasons we cant use this fact
        // for true interjection, END_ONLY can mean wordCount or next word is , or - 
		bool block = false;
        if (canonical && F->flags & ORIGINAL_ONLY) { block = true; } // incoming is not original words and must be
		else if (restrict && !(restrict & flags)) { block = true; } // type restriction in effect for this concept member
		else if (F->flags & (START_ONLY | END_ONLY))
		{
			if (F->flags & START_ONLY && start != 1) { block = true; }  // must begin the sentence
			else if (F->flags & END_ONLY && end != wordCount && !(F->flags & START_ONLY)) { block = true; }  // must begin the sentence
			else if (F->flags & (START_ONLY | END_ONLY) && start == 1)
			{
				if (end == wordCount) { ; }
				else if (*wordStarts[end + 1] == ',') { ; }
				else if (*wordStarts[end + 1] == '-') { ; }
				else block = true;
			}
		}

		int mindex = Meaning2Index(F->subject);
            //   index meaning restriction (0 means all)
            if (!block && index == mindex) // match generic or exact subject 
			{
				bool mark = true;
				// test for word not included in set
				if (index)
				{
					unsigned int pos = GETTYPERESTRICTION(GetMeaning(Meaning2Word(F->subject), index));
					if (!(flags & pos)) 
						mark = false; // we cannot be that meaning because type is wrong
				}

				if (!mark)
				{
					if (trace & TRACE_HIERARCHY) TraceHierarchy(F, "");
				}
				else if (*concept->word == '~' && WhereWordHit(concept, start) >= end) mark = false; // already marked this set
				else  //does set has some members it does not want
				{
					FACT* G = GetObjectNondeadHead(concept);
					while (G)
					{
                        // all simple excludes will be first
                        // all set excludes will be second
                        // actual values of set will be third
                        // User can defeat that by runtime addition of set members. Too bad for now.
                        // so technically we could free up the HAS_EXCLUDE bit
                        if (G->verb == Mexclude) // see if this is marked for this position, if so, DONT trigger topic
						{
							WORDP S = Meaning2Word(G->subject); // what to exclude
							int startPosition,endPosition;
                            // need to test for original only as well - BUG
							if (GetNextSpot(S,start-1,startPosition,endPosition) && startPosition == start && endPosition == end)
							{
                                if (trace & TRACE_HIERARCHY) TraceHierarchy(F,"");
                                mark = false;
								break;
							}
                            // if we see a concept exclude, regular ones already passed
                            if (*S->word == '~') // concept exclusion - status unknown
                            {
                                AddPendingConcept(G, start, end); // must review later
                                mark = false; // dont mark now
                                break; // revisit all exclusions later
                            }
						}
                        else if (G->verb == Mmember) // ran out of excludes
                        {
                            mark = true; 
                            break;
                        }
						G = GetObjectNondeadNext(G);
					}
				}

				if (mark)
				{
					if (MarkWordHit(depth, exactWord, concept, index,start, end)) // new ref added
					{
						if (MarkSetPath(depth+1, exactWord, F->object, start, end, level + 1, canonical) != -1) result = 1; // someone marked
					}
				}
			}
			else if (!index && mindex) // we are all meanings (limited by pos use) and he is a specific meaning
			{
				WORDP H = Meaning2Word(F->subject);
				MEANING M1 = GetMeaning(H, mindex);
				unsigned int pos = GETTYPERESTRICTION(M1);
				if (flags & pos) //  && start == end   wont work if spanning multiple words revised due to "to fish" noun infinitive
				{
					if (MarkWordHit(depth, exactWord, Meaning2Word(F->object), Meaning2Index(F->object),start, end)) // new ref added
					{
						if (MarkSetPath(depth+1, exactWord, F->object, start, end, level + 1, canonical) != -1) result = 1; // someone marked
					}
				}
			}
            else if (false && index && !mindex) // he accepts  all meanings and we are a specific meaning
            {
                    WORDP H = Meaning2Word(F->subject);
                    MEANING M1 = GetMeaning(H, index);
                    unsigned int pos = GETTYPERESTRICTION(M1);
                    if (flags & pos) //  && start == end   wont work if spanning multiple words revised due to "to fish" noun infinitive
                    {
                            if (MarkWordHit(depth, exactWord, Meaning2Word(F->object), Meaning2Index(F->object), start, end)) // new ref added
                            {
                                 if (MarkSetPath(depth + 1, exactWord, F->object, start, end, level + 1, canonical) != -1) result = 1; // someone marked
                             }
                  }
            }
	}
	return result;
}

static void RiseUp(int depth, int exactWord,MEANING M,unsigned int start, unsigned int end,unsigned int level,bool canonical) //   walk wordnet hierarchy above a synset node
{	// M is always a synset head 
	M &= -1 ^ SYNSET_MARKER;
	unsigned int index = Meaning2Index(M);
	WORDP D = Meaning2Word(M);
	char word[MAX_WORD_SIZE];
	sprintf(word,(char*)"%s~%d",D->word,index); // some meaning is directly referenced?
	MarkWordHit(depth, exactWord, FindWord(word),0,start,end); // direct reference in a pattern

	// now spread and rise up
	if (MarkSetPath(depth, exactWord,M,start,end,level,canonical) == -1) return; // did the path already
	FACT* F = GetSubjectNondeadHead(D); 
	while (F)
	{
		if (F->verb == Mis && (index == 0 || F->subject == M)) RiseUp(depth+1,exactWord,F->object,start,end,level+1,canonical); // allowed up
		F = GetSubjectNondeadNext(F);
	}
}

static void MarkAllMeaningAndImplications(int depth, MEANING M, int start, int end, bool canonical, bool sequence, bool once)
{ // M is always a word or sequence from a sentence
	// but if uppercase, there may be multiple forms, so handle all
	if (!M) return;
	WORDP D = Meaning2Word(M);
	bool hasUpperCharacters;
	bool hasUTF8Characters;
	unsigned int len = D->length;
	uint64 fullhash = Hashit((unsigned char*)D->word, len, hasUpperCharacters, hasUTF8Characters); //   sets hasUpperCharacters and hasUTF8Characters 
	unsigned int hash = (fullhash % maxHashBuckets); // mod by the size of the table
	int uindex = 0;											 //   lowercase bucket
	WORDP X = dictionaryBase + hashbuckets[hash + 1];
	while (X != dictionaryBase) // all entries matching in upper case bucket
	{
		if (fullhash == X->hash && X->length == len && !StricmpUTF(D->word, X->word, len))
		{
			MEANING M1 = M;
			int windex = Word2Index(X);
			if ((M & MEANING_BASE) != (unsigned int)windex) // alternate spelling
			{
				M1 = windex | (M & TYPE_RESTRICTION); // no idea what meaning, go generic
			}
			MarkMeaningAndImplications(depth, windex, M1, start, end, canonical, sequence, once);
		}
		X = dictionaryBase + GETNEXTNODE(X);
	}
}

void MarkMeaningAndImplications(int depth, int exactWord,MEANING M,int start, int end,bool canonical,bool sequence,bool once) 
{ // M is always a word or sequence from a sentence
    if (!M) return;
	WORDP D = Meaning2Word(M);
	if (D->properties & NOUN_TITLE_OF_WORK && canonical) return; // accidental canonical match of a title. not intended
																 // We want to avoid wandering fact relationships for meanings we have already scanned.
	if (!exactWord)
	{
		if (D->internalBits & UPPERCASE_HASH) MarkAllMeaningAndImplications(depth,  M, start, end, canonical, sequence, once);
		else if (*D->word == '~') exactWord = (canonical) ? 0x01000000 : 0; // dont use this word as the match
		else exactWord = (canonical) ? 0x01000000 : 0;
	}
																 // We mark words/phrases  and concepts and words/concepts implied by them.
	// We mark words by meaning (63) + generic. They always have a fixed size match.
	// We mark concepts by size match at a start position. You might match 1 word or several in a row.
	// For match variable retrieval we want the longest match at a position.
	// Because we can come in here with a general word with and without a type restriction,
	// we have scan out from the word because we cant mark the different ways we scanned before,
	int index = Meaning2Index(M);
	int whereHit = WhereWordHit(D, start);
	unsigned int restrict = GETTYPERESTRICTION(M);
	unsigned int size = GetMeaningCount(D);
	if (size == 0)
	{
		M = MakeMeaning(D); // remove restriction
		restrict = 0;
	}
	if (*D->word == '~')
	{
		if (whereHit >= end) return; // already have best concept storage
	}
	else
	{
		if (whereHit < end) SetTriedMeaning(D, 0); // found nothing at this index, insure nothing to start
	}
    
    // we mark word hit before using MarkSetPath, so that exclude is supported
    // words we dont know we dont bother marking
    if (!once || D->properties & (PART_OF_SPEECH | NOUN_TITLE_OF_WORK | NOUN_HUMAN) || D->systemFlags & PATTERN_WORD || D->internalBits &  CONCEPT)
    {
        MarkWordHit(depth, exactWord, D, 0, start, end);
        MarkSetPath(depth + 2, exactWord, M, start, end, 0, canonical); // generic membership of this word all the way to top
    }
    // we dont mark random junk discovered, only significant sequences
    else if (sequence) // phrase is not a pattern word, maybe it goes to some concepts
    {
        FACT* F = GetSubjectNondeadHead(D);
        while (F)
        {
            // mark sequence if someone cared about it
			if (ValidMemberFact(F))  // ~concept members and word equivalent
            {
                MarkWordHit(depth, exactWord, D, 0, start, end); // we found something to relate to, so mark us 
                MarkSetPath(depth + 2, exactWord, M, start, end, 0, canonical); // generic membership of this word all the way to top
                break;
            }
            F = GetSubjectNext(F);
        }
    }
 
	// check for POS restricted forms of this word
	char word[MAX_WORD_SIZE];
	if (*D->word != '~' && !once) // words, not concepts
	{
		if (restrict & NOUN && !(posValues[start] & NOUN_INFINITIVE)) // BUG- this wont work up the ontology, only at the root of what the script requests - doesnt accept "I like to *fish" as a noun, so wont refer to the animal
		{
			sprintf(word, (char*)"%s~n", D->word);
			MarkWordHit(depth, exactWord, FindWord(word, 0, PRIMARY_CASE_ALLOWED), 0, start, end); // direct reference in a pattern
        }
		if ((restrict & VERB) || posValues[start] & NOUN_INFINITIVE)// accepts "I like to *swim as not a verb meaning" 
		{
			sprintf(word, (char*)"%s~v", D->word);
			MarkWordHit(depth, exactWord, FindWord(word, 0, PRIMARY_CASE_ALLOWED), 0, start, end); // direct reference in a pattern
        }
		if (restrict & ADJECTIVE) // and adverb
		{
			sprintf(word, (char*)"%s~a", D->word);
			MarkWordHit(depth, exactWord, FindWord(word, 0, PRIMARY_CASE_ALLOWED), 0, start, end); // direct reference in a pattern
        }
	}

	//   now follow out the allowed synset hierarchies 
	if (!restrict) restrict = ESSENTIAL_FLAGS & finalPosValues[end]; // unmarked ptrs can rise all branches compatible with final values - end of a multiword (idiom or to-infintiive) is always the posvalued one

	if (!once && *D->word != '~') for  (unsigned int k = 1; k <= size; ++k) 
	{
		M = GetMeaning(D,k); // it is a flagged meaning unless it self points
		if (!(GETTYPERESTRICTION(M) & restrict)) continue;	// cannot match type

		// walk the synset words and see if any want vague concept matching like dog~~
		MEANING T = M; // starts with basic meaning
		unsigned int n = (index && (int)k != index) ? (unsigned int)80 : (unsigned int) 0;	// only on this meaning or all synset meanings 
		while (n < 50) // insure not infinite loop
		{
			WORDP X = Meaning2Word(T);
			unsigned int ind = Meaning2Index(T);
			sprintf(word,(char*)"%s~~",X->word);
			MarkWordHit(depth, exactWord, FindWord(word, 0, PRIMARY_CASE_ALLOWED),0,start,end); // direct reference in a pattern
			if (!ind) break;	// has no meaning index
			MEANING* meanings = GetMeanings(X);
			if (!meanings) break;
			T = meanings[ind];
			if (!T) break;
			if ((T & MEANING_BASE) == (M & MEANING_BASE)) break; // end of loop
			++n;
		}

		M = (M & SYNSET_MARKER) ? MakeMeaning(D,k) : GetMaster(M); // we are the master itself or we go get the master
		RiseUp(depth+1, exactWord,M,start,end,0,canonical); // allowed meaning pos (self ptrs need not rise up)
	}
}

static void HuntMatch(int canonical, char* word,bool strict,int start, int end, unsigned int& usetrace)
{
	WORDP set[20];
	WORDP D;
	int oldtrace = trace;
    // if user typed upper case specifically, trust him
    if (start == end && start != 1 && IsUpperCase(word[0]))
    {
        if (!IsUpperCase(word[1])) strict = true;
    }
    int i = GetWords(word,set,strict); // words in any case and with mixed underscore and spaces
	while (i) 
	{
		D = set[--i];

		// dont redo effort
		if (D->internalBits & BEEN_HERE) continue;	// huntmatch already covered this
		D->internalBits |= BEEN_HERE;
        wordlist = AllocateStackval(wordlist, (uint64) D,NULL);
		// markallimplied matches words with part of speech. This doesnt
		if (!(D->systemFlags & PATTERN_WORD) && !(D->properties & PART_OF_SPEECH) && !(D->internalBits & UPPERCASE_HASH)) // given no flag reason to use, see if concept member
		{
			FACT* F = GetSubjectHead(D); // is it a part of some concept? Or a direct wor
			while (F)
			{
				if (ValidMemberFact(F)) break; // legal fact for us
				F = GetSubjectNext(F);
			}
			if (!F) continue;
		}
        // not allowed to detect uppercase when user input 
        // is form is conjugated (fitted != Fit) except
        // for Plural noun
        if (start == end && IsUpperCase(*D->word) && canonical == 2){ ; }
        else if (start == end && IsUpperCase(*D->word) &&
            stricmp(D->word, wordStarts[start]))
        {
           // size_t len = strlen(wordStarts[start]);
            //if (wordStarts[start][len-1] != 's') // too uncommon, dont want fits to match
                continue;
        }
        if (canonical == 2) canonical = 0;
		trace = (D->subjectHead || D->systemFlags & PATTERN_WORD || D->properties & PART_OF_SPEECH)  ? usetrace : 0; // being a subject head means belongs to some set. being a marked word means used as a keyword
		if ((*D->word == 'I' || *D->word == 'i'  ) && !D->word[1]){;} // dont follow out this I or i word
		else  MarkMeaningAndImplications(0, 0,MakeMeaning(D),start,end, (bool)canonical,true);
	}
	trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
}

static void SetSequenceStamp() //   mark words in sequence, original and canonical (but not mixed) - detects proper name potential up to 5 words  - and does discontiguous phrasal verbs
{// words are always fully generic, never restricted by meaning or postag
	// these use underscores
	char* rawbuffer = AllocateStack(NULL, maxBufferSize);
	char* originalbuffer = AllocateStack(NULL, maxBufferSize); // includes typos
	char* canonbuffer = AllocateStack(NULL, maxBufferSize);
	wordlist = NULL;
    char* limit = GetUserVariable("$cs_sequence");
    int sequenceLimit = (*limit) ? atoi(limit) : SEQUENCE_LIMIT;
	unsigned int oldtrace = trace;
	unsigned int usetrace = trace;
	if (trace & TRACE_PREPARE || prepareMode == PREPARE_MODE) 
	{
		Log(STDUSERLOG,(char*)"\r\nSequences:\r\n");
		usetrace = (unsigned int) -1;
		if (oldtrace && !(oldtrace & TRACE_ECHO)) usetrace ^= TRACE_ECHO;
	}
	uint64 logbasecount = logCount; // see if we logged anything

	//   consider all sets of up to 5-in-a-row 
	for (int i = startSentence; i <= (int)endSentence; ++i)
	{
		marklimit = 0; // per word scan limit
		while (wordlist)
		{
            uint64 D;
            wordlist = UnpackHeapval(wordlist, D, discard);
			((WORDP)D)->internalBits ^= BEEN_HERE;
		}

		if (!IsAlphaUTF8OrDigit(*wordStarts[i]) ) continue; // we only composite words, not punctuation or quoted stuff
		if (IsDate(wordStarts[i])) continue;// 1 word date, caught later
		// check for dates
		int start,end;
		if (DateZone(i,start,end) && i != wordCount)
		{
			int at = start - 1;
			*rawbuffer = 0;
			while (++at <= end)
			{
				if (!stricmp(wordStarts[at],(char*)"of")) continue;	 // skip of
				strcat(rawbuffer,wordStarts[at]);
				if (at != end) strcat(rawbuffer,(char*)"_");
			}
			StoreWord(rawbuffer,NOUN|NOUN_PROPER_SINGULAR);
			MarkMeaningAndImplications(0, 0,MakeMeaning(FindWord((char*)"~dateinfo")),start,end,false,true);
			i = end;
			continue;
		}
        else if ((i + 4) <= wordCount && IsDigitWord(wordStarts[i], numberStyle) && // multiword date
            IsDigitWord(wordStarts[i + 2], numberStyle) &&
            IsDigitWord(wordStarts[i + 4], numberStyle))
        {
            int val = atoi(wordStarts[i + 2]); // must be legal 1 - 31

            char sep = *wordStarts[i + 1];
            if (*wordStarts[i + 3] == sep && val >= 1 && val <= 31 && 
                (sep == '-' || sep == '/' || sep == '.') )
            {
                char word[MAX_WORD_SIZE];
                strcpy(word, wordStarts[i]); // force month first
                strcat(word, wordStarts[i + 1]);
                strcat(word, wordStarts[i + 2]);
                strcat(word, wordStarts[i + 3]);
                strcat(word, wordStarts[i + 4]);
                WORDP D = StoreWord(word, NOUN | NOUN_PROPER_SINGULAR);
                MarkMeaningAndImplications(0, 0, MakeMeaning(FindWord((char*)"~dateinfo")), i, i+4, false, true);
            }
        }
		//   set base phrase
		strcpy(rawbuffer,wordStarts[i]);
		strcpy(canonbuffer,wordCanonical[i]);
		*originalbuffer = 0;
		start = derivationIndex[i] >> 8; // from here
		end = derivationIndex[i] & 0x00ff;  // to here
		for (int j = start; j <= end; ++j)
		{
			if (!derivationSentence[j]) break; // in case sentence is empty
			strcat(originalbuffer,derivationSentence[j]);
			if ( j != end) strcat(originalbuffer,"_");
		}
		
		// scan interesting initial words (spaced, underscored, capitalized) but we need to recognize bots in lower case, so try all cases here as well
        if (!trustpos) // the base words would already be scanned and marked by pos
        {
            HuntMatch(0, rawbuffer, (tokenControl & STRICT_CASING) ? true : false, i, i, usetrace);
            HuntMatch(1, canonbuffer, (tokenControl & STRICT_CASING) ? true : false, i, i, usetrace);
            HuntMatch(2, originalbuffer, (tokenControl & STRICT_CASING) ? true : false, i, i, usetrace); // using start and end have derivation issues when multiple replaces happen
        }
		//   fan out for addon pieces
		int k = 0;
		int index = 0;
		while ((++k + i) <= endSentence)
		{
			strcat(rawbuffer,(char*)"_");
			strcat(rawbuffer,wordStarts[i+k]);

			strcat(canonbuffer,(char*)"_");
			strcat(canonbuffer,wordCanonical[i+k]);

			strcat(originalbuffer,(char*)"_");
			start = derivationIndex[i+k] >> 8; // from here
			end = derivationIndex[i+k] & 0x00ff;  // to here
			for (int j = start; j <= end; ++j)
			{
				if (!derivationSentence[j]) break; // in case sentence is empty
				strcat(originalbuffer,derivationSentence[j]);
				if ( j != end)  strcat(originalbuffer,"_");
			}

			// we  composite anything, not just words, in case they made a typo
			HuntMatch(false,rawbuffer,(tokenControl & STRICT_CASING) ? true : false,i,i+k,usetrace);
			HuntMatch(true,canonbuffer,(tokenControl & STRICT_CASING) ? true : false,i,i+k,usetrace);
			HuntMatch(false,originalbuffer,(tokenControl & STRICT_CASING) ? true : false,i,i+k,usetrace);
			if (logCount != logbasecount && usetrace)  Log(STDUSERLOG,(char*)"\r\n"); // if we logged something, separate
			if (++index >= sequenceLimit) break; //   up thru 5 words in a phrase
			logbasecount = logCount;
		}
	}
	
	// mark disjoint particle verbs as whole
	for (int i = wordCount; i >= 1; --i)
	{
		marklimit = 0; // per word scan limit
		if (!(posValues[i] & PARTICLE)) continue;
		// find the particle
		unsigned int at = i;
		while (posValues[--at] & PARTICLE){;}	// back up thru contiguous particles
		if (posValues[at] & (VERB_BITS|NOUN_INFINITIVE|NOUN_GERUND|ADJECTIVE_PARTICIPLE)) continue;	// its all contiguous

		char canonical[MAX_WORD_SIZE];
		char original[MAX_WORD_SIZE];
		*canonical = 0;
		*original = 0;
		while (at && !(posValues[--at] & (VERB_BITS|NOUN_INFINITIVE|NOUN_GERUND|ADJECTIVE_PARTICIPLE))){;} // find the verb
		if (!at) continue; // failed to find match  "in his early work *on von neuman...
		strcpy(original,wordStarts[at]);
		strcpy(canonical, wordCanonical[at]);
		unsigned int end = i;
		i = at; // resume out loop later from here
		while (++at <= end)
		{
			if (posValues[at] & (VERB_BITS|PARTICLE|NOUN_INFINITIVE|NOUN_GERUND|ADJECTIVE_PARTICIPLE))
			{
				if (*original) 
				{
					strcat(original,(char*)"_");
					strcat(canonical,(char*)"_");
				}
				strcat(original,wordStarts[at]);
				strcat(canonical, wordCanonical[at]);
			}
		}

		// storeword instead of findword because we normally dont store keyword phrases in dictionary
		WORDP D = FindWord(original,0,LOWERCASE_LOOKUP); 
		if (D)
		{
			trace = usetrace; // being a subject head means belongs to some set. being a marked word means used as a keyword
			MarkMeaningAndImplications(0, 0,MakeMeaning(D),i,i,false,false);
		}
		if (stricmp(original,canonical)) // they are different
		{
			D = FindWord(canonical,0,LOWERCASE_LOOKUP);
			if (D) 
			{
				trace = usetrace;
				MarkMeaningAndImplications(0, 0,MakeMeaning(D),i,i,true,false);
			}
		}
	}
	if (trace & TRACE_PATTERN || prepareMode == PREPARE_MODE) Log(STDUSERLOG,(char*)"\r\n"); // if we logged something, separate

	while (wordlist)
	{
        uint64 D;
        wordlist = UnpackHeapval(wordlist, D, discard);
		((WORDP)D)->internalBits ^= BEEN_HERE;
	}

#ifdef TREETAGGER
	MarkChunk();
#endif

	trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
	ReleaseStack(rawbuffer); // short term
}

static void StdMark(MEANING M, unsigned int start, unsigned int end, bool canonical) 
{
	if (!M) return;
	WORDP D = Meaning2Word(M);
	MarkMeaningAndImplications(0,0,M,start,end,canonical);		//   the basic word
	if (IsModelNumber(D->word) && !canonical) MarkMeaningAndImplications(0,0, MakeMeaning(StoreWord("~modelnumber")), start, end, false);
	if (D->systemFlags & TIMEWORD && !(D->properties & PREPOSITION)) MarkMeaningAndImplications(0, 0,MakeMeaning(Dtime),start,end);
}

static STACKREF BuildConceptList(int field,int verb)
{
    STACKREF reflist = NULL;
    if (field == 0)
    {
        // command sentence with implied "you" as subject
        if (verb && verb < 3 && posValues[verb] & VERB_INFINITIVE)
        {
            WORDP D = StoreWord("you");
            reflist = AllocateStackval(reflist, (uint64)D->word);
        }
        return 0;
    }
    HEAPREF list = concepts[field];
    while (list)
    { //Meaning, Next 
        uint64 word;
        list = UnpackHeapval(list, word, discard, discard);
        reflist = AllocateStackval(reflist, word);
    }

    list = topics[field];
    while (list)
    {
        uint64 word;
        list = UnpackStackval(list, word);
        reflist = AllocateStackval(reflist, word);
    }
    
    reflist = AllocateStackval(reflist, (uint64)wordStarts[field]);
    
    if (stricmp(wordCanonical[field], wordStarts[field]))
    {
        reflist = AllocateStackval(reflist, (uint64)wordCanonical[field]);
    }

    return reflist;
}

static void ProcessWordLoop(STACKREF verblist, int verb, STACKREF subjectlist, int subject, STACKREF objectlist, int object, int base)
{  
    STACKREF slist = subjectlist;
    STACKREF olist = objectlist;
    char format[MAX_WORD_SIZE];
    while (verblist)
    {
        uint64 verbword;
        verblist = UnpackStackval(verblist, verbword);
        sprintf(format, "|%s|", (char*)verbword);
        WORDP D = FindWord(format, 0, LOWERCASE_LOOKUP);
        if (!D) continue;   // no one cares

        if (MarkWordHit(4, false, D, 0, verb, verb)) // new ref added
        {
           MarkSetPath(4 + 1, false, MakeMeaning(D), verb, verb, 4 + 1, false);
        }
        if (slist && hasFundamentalMeanings & FUNDAMENTAL_SUBJECT)
        {
           // subject + verb
           subjectlist = slist;
           while (subjectlist)
           {
               uint64 subjectword;
               subjectlist = UnpackStackval(subjectlist, subjectword);
                sprintf(format, "%s|%s|", (char*)subjectword,(char*)verbword );
                WORDP D = FindWord(format, 0);
                if (!D) continue;   // no one cares

                if (MarkWordHit(4, false, D, 0, verb, verb)) // new ref added
                {
                    MarkSetPath(4 + 1, false, MakeMeaning(D), verb, verb, 4 + 1, false);
                }
                // subject + verb + object
                if (olist && hasFundamentalMeanings & FUNDAMENTAL_SUBJECT_OBJECT)
                {
                    objectlist = olist;
                    while (objectlist)
                    {
                        uint64 objectword;
                        objectlist = UnpackStackval(objectlist, objectword);
                        sprintf(format, "%s|%s|%s",(char*) subjectword,(char*)verbword, (char*)objectword);
                        WORDP D = FindWord(format, 0);
                        if (D && MarkWordHit(4, false, D, 0, verb, verb)) // new ref added
                        {
                            MarkSetPath(4 + 1, false, MakeMeaning(D), verb, verb, 4 + 1, false);
                        }
                    }
                }
            }
            // verb + object
            if (olist && hasFundamentalMeanings & FUNDAMENTAL_OBJECT)
            {
                objectlist = olist;
                while (objectlist)
                {
                   uint64 objectword;
                    objectlist = UnpackStackval(objectlist, objectword);
                    sprintf(format, "|%s|%s", (char*)verbword, (char*)objectword);
                    WORDP D = FindWord(format, 0);
                    if (!D) continue;   // no one cares

                    if (MarkWordHit(4, false, D, 0, verb, verb)) // new ref added
                    {
                        MarkSetPath(4 + 1, false, MakeMeaning(D), verb, verb, 4 + 1, false);
                    }
                }
            }
        }
    }
}

static void ProcessSentenceConcepts(int subject, int verb, int object)
{
    char* beginAllocation = AllocateStack(NULL, 4, false, 4);
    
    STACKREF subjectlist = (subject && hasFundamentalMeanings & FUNDAMENTAL_SUBJECT) ? BuildConceptList(subject,verb) : 0;
    STACKREF verblist = BuildConceptList(verb,0);
    STACKREF objectlist = (subject && hasFundamentalMeanings & FUNDAMENTAL_OBJECT) ? BuildConceptList(object,0) : 0;

    ProcessWordLoop(verblist, verb, subjectlist, subject, objectlist, object, verb);

    ReleaseStack(beginAllocation);
}

static void MarkFundamentalMeaning()
{
    if (trace & TRACE_PREPARE || prepareMode == PREPARE_MODE)
    {
        Log(STDUSERLOG, (char*)"Fundamental Meanings:\r\n");
    }
    int subject = 0;
    int object = 0;
    int verb = 0;
    for (int i = 1; i <= wordCount; ++i)
    {
        if (roles[i] & MAINVERB)
        {
            verb = i;
            int j = i;

            if (roles[i] & PASSIVE_VERB) // find subject as object
            { // he was attacked
                while (j-- > 0)
                {
                    if (roles[j] & MAINSUBJECT)
                    {
                        object = j;
                        break;
                    }
                }
                j = i;
                while (++j <= wordCount)
                {
                    // you were attacked by a monster
                    if (!stricmp(wordStarts[j], "by"))
                    {
                        while (++j <= wordCount)
                        {
                            if (roles[j] & OBJECT2)
                            {
                                subject = j;
                                break;
                            }
                        }
                        break;
                    }
                }
            }
            // active voice. we either have a subject or implied command (at 1)
            else // object is object
            {
                while (++j <= wordCount)
                {
                    if (roles[j] & MAINOBJECT)
                    {
                        object = j;
                        break;
                    }
                }
                j = i;
                while (--j > 0) // find subject
                {
                    if (roles[j] & MAINSUBJECT)
                    {
                        subject = j;
                        break;
                    }
                }
            }
            break;
        }
    }
    if (verb)
    {
        ProcessSentenceConcepts(subject, verb, object);
        
        ProcessPendingConcepts();
    }
}

void MarkAllImpliedWords()
{
	int i;
    pendingConceptList = NULL;
	for (i = 1; i <= wordCount; ++i)  capState[i] = IsUpperCase(*wordStarts[i]); // note cap state
	failFired = false;
	TagIt(); // pos tag and maybe parse

	if ( prepareMode == POS_MODE || tmpPrepareMode == POS_MODE || prepareMode == PENN_MODE || prepareMode == POSVERIFY_MODE  || prepareMode == POSTIME_MODE ) 
	{
		return;
	}
    if (trace & TRACE_PREPARE || prepareMode == PREPARE_MODE) Log(STDUSERLOG,(char*)"\r\nConcepts: \r\n");
 	if (showMark)  Log(ECHOSTDUSERLOG,(char*)"----------------\r\n");
	markLength = 0;
					 
	//   now mark every word in all seen
    for (i = 1; i <= wordCount; ++i) //   mark that we have found this word, either in original or canonical form
    {
		marklimit = 0; // per word scan limit
		if (i == startSentence && upperCount > 10 && lowerCount < 5) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord("~shout")),i,i);
		char* original =  wordStarts[i];
		if (!*original)
			continue;	// ignore this
		if (!wordCanonical[i] || !*wordCanonical[i]) wordCanonical[i] = original; // in case failure below

		if (showMark) Log(ECHOSTDUSERLOG,(char*)"\r\n");

 		if (trace  & (TRACE_HIERARCHY | TRACE_PREPARE) || prepareMode == PREPARE_MODE) Log(STDUSERLOG,(char*)"\r\n%d: %s (raw):\r\n",i,original);
		uint64 flags = posValues[i];
		WORDP D = originalLower[i] ? originalLower[i] : originalUpper[i]; // one of them MUST have been set
		if (!D) D = StoreWord(original); // just so we can't fail later
		
		// put back non-tagger generalized forms of bits
		if (flags & NOUN_BITS) flags |= NOUN;
		if (flags & (VERB_BITS | NOUN_INFINITIVE| NOUN_GERUND)) flags |= VERB;
		if (flags & ADJECTIVE_BITS) flags |= ADJECTIVE  | (allOriginalWordBits[i] & (MORE_FORM|MOST_FORM));
		if (flags & NOUN_ADJECTIVE) flags |=  (allOriginalWordBits[i] & (MORE_FORM|MOST_FORM)) | ADJECTIVE_NORMAL | ADJECTIVE; // what actress is the *prettiest  -- be NOUN OR ADJECTIVE
		if (flags & ADVERB) flags |= ADVERB |  (allOriginalWordBits[i] & (MORE_FORM|MOST_FORM));
		if (D->properties & CURRENCY)  flags |= CURRENCY;
		if (D->systemFlags & ORDINAL) 
		{
			flags |= PLACE_NUMBER;
			AddParseBits(D,QUANTITY_NOUN);
		}
		if (!stricmp(wordCanonical[i],(char*)"be"))
		{
			if (!stricmp(original,(char*)"am") || !stricmp(original,(char*)"was")) flags |= SINGULAR_PERSON;
		}
		if (flags & NOUN_INFINITIVE && !(flags & NOUN_SINGULAR)) // transcribe back to verb only, leaving noun_infinitive status and not verb tense status
		{
			flags &= -1 ^ NOUN; // but still a noun_infinitive
			flags |= VERB;
		}
		finalPosValues[i] = flags; // these are what we finally decided were correct pos flags from tagger
		
		if (wordStarts[i][1] && (wordStarts[i][1] == ':' || wordStarts[i][2] == ':')) // time info 1:30 or 11:30
		{
			if (originalLower[i] && IsDigit(wordStarts[i][0]) && IsDigit(wordStarts[i][3])) 
			{
				AddSystemFlag(D,ACTUAL_TIME);
			}
		}

        WORDP OL = originalLower[i];
        WORDP CL = canonicalLower[i];
        WORDP OU = originalUpper[i];
        WORDP CU = canonicalUpper[i];
        // if (!CL && !CU && wordCanonical[i]) CL = StoreWord(wordCanonical[i]);

        if (!CU && original[1]) // dont convert single letters to upper case "a" if it hasnt already decided its not a determiner
        {
            CU = FindWord(original, 0, UPPERCASE_LOOKUP);	// try to find an upper to go with it, in case we can use that, but not as a human name
            if (OU) { ; } // it was originally uppercase or there is no lower case meaning
            else if (finalPosValues[i] & IDIOM) { ; }	// keep if idiom
            else if (CU && CU->properties & (NOUN_FIRSTNAME | NOUN_HUMAN)) CU = NULL;	// remove accidental names 
            else if (CU && !CU->properties && !(CU->systemFlags & PATTERN_WORD)) CU = NULL; // there is no use for this (maybe only a sequence head)
        }
        if (!(finalPosValues[i] & (NOUN_BITS | ADJECTIVE_NOUN | IDIOM)))
            CU = OU = NULL;	// cannot be upper case

        // WE need to mark the words themselves, before any inferred sets 
        // because user may use EXCLUDE on an inferred set

		// both twitter usernames and hashtags are alphanumberic or _
		// https://help.twitter.com/en/managing-your-account/twitter-username-rules
		// https://www.hashtags.org/featured/what-characters-can-a-hashtag-include/
		if ((*wordStarts[i] == '@' || *wordStarts[i] == '#')  && strlen(wordStarts[i]) > 2)
		{
			char* ptr = wordStarts[i];
			bool hasAlpha = false;
			bool hasFirstAlpha = IsAlphaUTF8(*(ptr+1));
			while (*++ptr)
			{
				if (!IsDigit(*ptr) && !IsAlphaUTF8(*ptr) && *ptr != '_') break;
				if (IsAlphaUTF8(*ptr)) hasAlpha = true;
			}
			if (!*ptr && hasAlpha) 
			{
				if (*wordStarts[i] == '@') MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord("~twitter_name")),i,i);
				if (*wordStarts[i] == '#' && hasFirstAlpha) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord("~hashtag_label")),i,i);
			}
		}

		// detect a filename
		if (IsFileName(wordStarts[i])) {
			MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~filename")), i, i, false);
		}

		// detect acronym
		char* ptrx = wordStarts[i];
		while (*++ptrx)
		{
			if (!IsUpperCase(*ptrx) && *ptrx != '&' ) break;
		}
		if (!*ptrx && wordStarts[i][1])
		{
			bool ok = true;
			if (wordStarts[i - 1] && IsUpperCase(wordStarts[i - 1][0])) ok = false;
			if (wordStarts[i + 1] && IsUpperCase(wordStarts[i + 1][0])) ok = false;
			if (ok) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~capacronym")), i, i);
		}
		// mark general number property -- (datezone can be marked noun_proper_singular) // adjective noun January 18, 2017 9:00 am
			
		if (finalPosValues[i] & (ADJECTIVE_NOUN | NOUN_PROPER_SINGULAR))  // a date can become an idiom, marking it as a proper noun and not a number
		{
			if (IsDigit(*wordStarts[i]) && IsDigit(wordStarts[i][1])  && IsDigit(wordStarts[i][2]) && IsDigit(wordStarts[i][3])  && !wordStarts[i][4]) MarkMeaningAndImplications(0, 0,MakeMeaning(FindWord((char*)"~yearnumber")),i,i);
		}
		if (IsDate(wordStarts[i]))
		{
			MarkMeaningAndImplications(0, 0, MakeMeaning(FindWord((char*)"~dateinfo")), i, i, false, false);
			MarkMeaningAndImplications(0, 0, MakeMeaning(FindWord((char*)"~formatteddate")), i, i, false, false);
		}

		int number = IsNumber(wordStarts[i], numberStyle);
		if (number && number != NOT_A_NUMBER)
		{
			if (!wordCanonical[i][1] || !wordCanonical[i][2]) // 2 digit or 1 digit
			{
				int n = atoi(wordCanonical[i]);
				if (n > 0 && n < 32 && *wordStarts[i] != '$') MarkMeaningAndImplications(0, 0, MakeMeaning(FindWord((char*)"~daynumber")), i, i);
			}

			if (IsDigit(*wordStarts[i]) && IsDigit(wordStarts[i][1])  && IsDigit(wordStarts[i][2]) && IsDigit(wordStarts[i][3])  && !wordStarts[i][4]) MarkMeaningAndImplications(0, 0,MakeMeaning(FindWord((char*)"~yearnumber")),i,i);
	
			MarkMeaningAndImplications(0,0,Mnumber,i,i); 

			// let's mark kind of number also
			if (strchr(wordCanonical[i], '.')) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~float")), i, i, true);
			else
			{
				MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~integer")), i, i, true);
				if (*wordStarts[i] != '-') MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~positiveInteger")), i, i, true);
				else MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~negativeinteger")), i, i, true);
			}

			//   handle finding fractions as 3 token sequence  mark as placenumber 
			if (i < wordCount && *wordStarts[i+1] == '/' && wordStarts[i+1][1] == 0 && IsDigitWord(wordStarts[i+2], numberStyle) )
			{
				MarkMeaningAndImplications(0, 0,MakeMeaning(Dplacenumber),i,i);
				if (trace & TRACE_PREPARE || prepareMode == PREPARE_MODE) Log(STDUSERLOG,(char*)"=%s/%s \r\n",wordStarts[i],wordStarts[i+2]);
			}
			else if (IsPlaceNumber(wordStarts[i],numberStyle)) // finalPosValues[i] & (NOUN_NUMBER | ADJECTIVE_NUMBER) 
			{
				MarkMeaningAndImplications(0,0,MakeMeaning(Dplacenumber),i,i);  
			}
			// special temperature property
			char c = GetTemperatureLetter(original);
			if (c)
			{
				if (c == 'F') MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~fahrenheit")),i,i);
				else if (c == 'C') MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~celsius")),i,i);
				else if (c == 'K')  MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~kelvin")),i,i);
				char number1[MAX_WORD_SIZE];
				sprintf(number1,(char*)"%d",atoi(original));
				WORDP canon =  StoreWord(number1,(NOUN_NUMBER | ADJECTIVE_NUMBER));
				if (canon) wordCanonical[i] = canon->word;
			}

			// special currency property
			char* number1;
			unsigned char* currency = GetCurrency((unsigned char*) wordStarts[i],number1); 
			if (currency) 
			{
				MarkMeaningAndImplications(0, 0,Mmoney,i,i);
				char* set = IsTextCurrency((char*)currency,NULL);
				if (set) // should not fail
				{
					MEANING M = MakeMeaning(FindWord(set));
					MarkMeaningAndImplications(0, 0, M, i, i);
				}
			}
		}
		else if (IsNumber(wordStarts[i], numberStyle) == WORD_NUMBER)
		{
			MarkMeaningAndImplications(0, 0, Mnumber, i, i);
		}
		if (FindTopicIDByName(wordStarts[i])) MarkMeaningAndImplications(0,0,MakeMeaning(Dtopic),i,i);
		
        if (CL && CL == DunknownWord) // allow unknown proper names to be marked unknown
		{
			MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord(original)), i, i);		// allowed word
			MarkMeaningAndImplications(0, 0,MakeMeaning(Dunknown),i,i); // unknown word
		}

		// note "bank teller" we want bank to have recognizion of its noun meaning in concepts - must do FIRST as noun, since adjective value is abnormal
		unsigned int restriction = (unsigned int)(finalPosValues[i] & BASIC_POS);
		if (finalPosValues[i] & ADJECTIVE_NOUN) 
		{
			StdMark(MakeTypedMeaning(OL,0,NOUN), i, i,false); //  mark word as a noun
		}
		else 
		{
			if (!OL && !OU) OL = StoreWord(original);
			StdMark(MakeTypedMeaning(OL,0,restriction), i, i,false);
		}

        if (trace & TRACE_PREPARE || prepareMode == PREPARE_MODE) Log(STDUSERLOG,(char*)" // "); //   close original meanings lowercase

		markLength = 0;
		if (IS_NEW_WORD(OU) && (OL || CL)) {;} // uppercase original was unknown and we have lower case forms, ignore upper.
		else 
		{
			if (finalPosValues[i] & ADJECTIVE_NOUN) StdMark(MakeTypedMeaning(OU,0,NOUN), i, i,false); //  mark word as a noun first, adjective is not normal
			else StdMark(MakeTypedMeaning(OU,0,restriction), i, i,false);
       	}
		
		if (CL) wordCanonical[i] = CL->word; //    original meanings lowercase
		else if (!wordCanonical[i]) wordCanonical[i] = (CU) ? CU->word : (char*)"";

		if (trace & TRACE_PREPARE || prepareMode == PREPARE_MODE) 
		{
			Log(STDUSERLOG,(char*)"\r\n%d: %s (canonical): ", i,wordCanonical[i] ); //    original meanings lowercase
		}

		//   canonical word
		if (finalPosValues[i] & ADJECTIVE_BITS && allOriginalWordBits[i] & (VERB_PRESENT_PARTICIPLE|VERB_PAST_PARTICIPLE)) // see if adj is verb as canonical base - "ing and ed" forms
		{
			StdMark(MakeTypedMeaning(CL,0,VERB), i, i,true);
		}
		else if (finalPosValues[i] & (NOUN_GERUND|NOUN_INFINITIVE))
		{
			StdMark(MakeTypedMeaning(CL,0,VERB), i, i,true);
		}
 		else if (finalPosValues[i] & ADJECTIVE_NOUN)
		{
			StdMark(MakeTypedMeaning(CL,0,NOUN), i, i,true);
			StdMark(MakeTypedMeaning(CU,0,NOUN), i, i,true);
		}
		else StdMark(MakeTypedMeaning(CL,0, (unsigned int)(finalPosValues[i] & BASIC_POS)), i, i,true);

 		markLength = 0;
	    if (trace & TRACE_PREPARE || prepareMode == PREPARE_MODE) Log(STDUSERLOG,(char*)" // "); //   close canonical form lowercase
 		
		// mark upper case canonical 
		StdMark(MakeTypedMeaning(CU,0, NOUN), i, i,true);

        // mark ancillary stuff
        MarkMeaningAndImplications(0, 0, MakeMeaning(wordTag[i]), i, i); // may do nothing
        MarkTags(i);
        MarkMeaningAndImplications(0, 0, MakeMeaning(wordRole[i]), i, i); // may do nothing
#ifndef DISCARDPARSER
        MarkRoles(i);
#endif

		if (trace & TRACE_PREPARE || prepareMode == PREPARE_MODE) Log(STDUSERLOG,(char*)" "); //   close canonical form uppercase
		markLength = 0;
	
        //   peer into multiword expressions  (noncanonical), in case user is emphasizing something so we dont lose the basic match on words
        //   accept both upper and lower case forms . 
		// But DONT peer into something proper like "Moby Dick"
		unsigned int  n = BurstWord(wordStarts[i]); // peering INSIDE a single token....
		WORDP E;
		if (tokenControl & NO_WITHIN || n == 1);  // dont peek within hypenated words 
        else if (finalPosValues[i] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL)) // mark first and last word, if they are recognized words
		{
			char* w = GetBurstWord(0);
			WORDP D1 = FindWord(w);
			w = GetBurstWord(n-1);
			if (D1 && allOriginalWordBits[i] & NOUN_HUMAN ) MarkMeaningAndImplications(0, 0,MakeMeaning(D1),i,i); // allow first name recognition with human names
			
			WORDP D2 = FindWord(w,0, LOWERCASE_LOOKUP);
			if (D2 && (D2->properties & (NOUN|VERB|ADJECTIVE|ADVERB) || D->systemFlags & PATTERN_WORD)) MarkMeaningAndImplications(0,0,MakeMeaning(D2),i,i); // allow final word as in "Bill Gates" "United States of America" , 
			D2 = FindWord(w, 0, UPPERCASE_LOOKUP);
			if (D2 && (D2->properties & (NOUN | VERB | ADJECTIVE | ADVERB) || D->systemFlags & PATTERN_WORD)) MarkMeaningAndImplications(0, 0, MakeMeaning(D2), i, i); // allow final word as in "Bill Gates" "United States of America" , 
		}
        else if (n >= 2 && n <= 4) //   longer than 4 is not emphasis, its a sentence - we do not peer into titles
        {
			static char words[5][MAX_WORD_SIZE];
			unsigned int k;
			for (k = 0; k < n; ++k) strcpy(words[k],GetBurstWord(k)); // need local copy since burstwords might be called again..

            for (unsigned int m = n-1; m < n; ++m) // just last word since common form  "bank teller"
            {
  				unsigned int prior = (m == (n-1)) ? i : (i-1); //   -1  marks its word match INSIDE a string before the last word, allow it to see last word still
                E = FindWord(words[m],0,LOWERCASE_LOOKUP); 
                if (E) StdMark(MakeMeaning(E),i,prior,false);
           }
        }

		// now look on either side of a hypenated word
		char* hypen = strchr(wordStarts[i],'-');
		if (!number && hypen && hypen != wordStarts[i] && hypen[1]) 
		{
			MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord(hypen)),i,i); // post form -colored
			char word[MAX_WORD_SIZE];
			strcpy(word,wordStarts[i]);
			word[hypen+1-wordStarts[i]] = 0;
			MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord(word)),i,i); // pre form  light-
		}
		
		D = (CL) ? CL : CU; //   best recognition
		if (!D) D = StoreWord(original); // just so we can't fail later
		char* last;
		if (!(tokenControl & NO_WITHIN) && D->properties & NOUN && !(D->internalBits & UPPERCASE_HASH) && (last = strrchr(D->word,'_')) && finalPosValues[i] & NOUN)
            StdMark(MakeMeaning(FindWord(last+1,0)), i, i,true); //   composite noun, store last word as referenced also

		// ALL Foreign words detectable by utf8 char
		D = (OL) ? OL : OU;
		if (!D) D = StoreWord(original); // just so we can't fail later
		if (D->internalBits & UTF8) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~utf8")),i,i);
		if (D->internalBits & UPPERCASE_HASH && D->length > 1 && !stricmp(language,"english"))  MarkMeaningAndImplications(0, 0,MakeMeaning(Dpropername),i,i);  // historical - internal is uppercase

        if (trace & TRACE_PREPARE || prepareMode == PREPARE_MODE) Log(STDUSERLOG,(char*)"\r\n");

		D = FindWord(wordStarts[i]);
		if (*wordStarts[i] == 'I' && !wordStarts[i][1]) {;} // ignore "I"
		else if (D && D->internalBits & UPPERCASE_HASH && tokenControl & MARK_LOWER)
		{
			char word[MAX_WORD_SIZE];
			MakeLowerCopy(word,D->word);
			if (trace & TRACE_PREPARE || prepareMode == PREPARE_MODE) Log(STDUSERLOG,(char*)"%d: %s (lower): ", i,word ); //    original meanings lowercase
			D = StoreWord(word);
			StdMark(MakeMeaning(D), i, i,true);
			if (trace & TRACE_PREPARE || prepareMode == PREPARE_MODE) Log(STDUSERLOG,(char*)"\r\n");
		}
        ProcessPendingConcepts();
    }
 
	//   check for repeat input by user - but only if more than 2 words or are unknown (we dont mind yes, ok, etc repeated)
	//   track how many repeats, for escalating response
	unsigned int sentenceLength = endSentence - startSentence + 1;
	bool notbrief = (sentenceLength > 2);
	if (sentenceLength == 1 && !FindWord(wordStarts[startSentence])) notbrief = true;
    unsigned int counter = 0;
    if (notbrief && humanSaidIndex) for (int j = 0; j < (int)(humanSaidIndex-1); ++j)
    {
        if (strlen(humanSaid[j]) > 5 && !stricmp(humanSaid[humanSaidIndex-1],humanSaid[j])) //   he repeats himself
        {
            ++counter;
            char buf[100];
			strcpy(buf,(char*)"~repeatinput");
			buf[12] = (char)('0' + counter);
			buf[13] = 0;
 			MarkMeaningAndImplications(0, 0,MakeMeaning(FindWord(buf,0,PRIMARY_CASE_ALLOWED)),1,1); //   you can see how many times
        }
    }

	//   now see if he is repeating stuff I said
	counter = 0;
    if (sentenceLength > 2) for (int j = 0; j < (int)chatbotSaidIndex; ++j)
    {
        if (humanSaidIndex && strlen(chatbotSaid[j]) > 5 && !stricmp(humanSaid[humanSaidIndex-1],chatbotSaid[j])) //   he repeats me
        {
			if (counter < sentenceLength) ++counter;
			MarkMeaningAndImplications(0, 0,MakeMeaning(FindWord((char*)"~repeatme",0,PRIMARY_CASE_ALLOWED)),counter,counter); //   you can see how many times
        }
    }

    //   handle phrases now
	markLength = 0;
    SetSequenceStamp(); //   sequences of words
    ProcessPendingConcepts();

    if (hasFundamentalMeanings) MarkFundamentalMeaning();

    ExecuteConceptPatterns(); // now use concept patterns
}
