// infer.cpp - handles queries into fact data

#include "common.h"

#define MAX_PARENTS 9980
#define MAX_QUEUE 10000
#define FOLLOW_LIMIT 50

static MEANING parents[MAX_PARENTS+20];	// nodes above where we are now
static int parentIndex = 0;		// add into parents at here
static int parentWalk = 0;		// retrieve from parents starting here. when reach parentIndex you have run out
static unsigned int currentBaseInfer;	// start of reserved marks
static unsigned int currentFreeInfer;	// marks before this are reserved

unsigned int inferMark = 0;				// primary "been-here" mark for all inferencing and tree traversals
static unsigned int saveMark = 0;		// tertiary mark - used in zone 1 control
static unsigned int ignoremark = 0;		// mark on entries to ignore
static WORDP fact = 0;

#define ORIGINALWORD 0x00000001
#define NORMAL		 0x00000002    //   class and set are a group (recursive)
#define QUOTED		 0x00000004   //   class and set are simple words
#define PREMARK		 0X00000008   //   marking but save word not meaning so wont use as scan
#define NOQUEUE		 0X00000010   //   just marked with mark, not queued
#define QUEUE		 0X00000020	//    add to q
#define NOTOPIC		 0X00000040	//   dont follow topic/set names
#define BLOCKMEANING 0X00000080
#define FACTTYPE	 0X00000100
#define FINDTOPIC	 0X00000200
#define UPDICTIONARY 0X00000400
#define USERFACTS	 0X00000800
#define SYSTEMFACTS	 0X00001000
#define FINDCONCEPT	 0X00002000
// 4 8 unused
#define USE_ORIGINAL_SUBJECT			0x00010000 // use subject as fact source
#define USE_ORIGINAL_OBJECT				0x00020000 // use object as fact source
#define RICCOCHET_USING_SUBJECT			0x00040000 
#define RICCOCHET_USING_OBJECT			0x00080000
#define RICCOCHET_BITS ( USE_ORIGINAL_SUBJECT | USE_ORIGINAL_OBJECT | RICCOCHET_USING_SUBJECT | RICCOCHET_USING_OBJECT )

// queued entries pending scanning
static MEANING queue[MAX_QUEUE+20]; 
static unsigned int queueIndex;

// answers from inferences go in these sets
FACT* factSet[MAX_FIND_SETS+2][MAX_FIND+1]; 
int	  factFlags[MAX_FIND+1];
int	  factIndex[MAX_FIND+1];
unsigned int factSetNext[MAX_FIND_SETS+1];		// when walking a set over time, which index to continue from

static void AddSet2Scan(unsigned int flags,WORDP D,int depth);

unsigned int NextInferMark() // set up for a new inference
{
    return ++inferMark;
}

FACT* IsConceptMember(WORDP D)
{
	if (!D) return NULL;
	FACT* F = GetSubjectNondeadHead(D);
	while (F)
	{
		if (F->verb == Mmember) return F;	// is a concept member so it is ok
		F = GetSubjectNondeadNext(F);
	}
	return NULL;
}

static bool IsExcluded(WORDP set,WORDP item)
{
	if (!(set->internalBits & HAS_EXCLUDE)) return false;
	FACT* F = GetObjectNondeadHead(set);
	while (F)
	{
		if (F->verb == Mexclude && Meaning2Word(F->subject) == item) break;
		F = GetObjectNondeadNext(F);
	}
	return (F) ? true : false;
}

static bool SetContains1(MEANING set,MEANING M, unsigned int depth) 
{
	if (!M || !set) return false;

	// the word
	WORDP D = Meaning2Word(M);
	unsigned int index = Meaning2Index(M);
	D->inferMark = inferMark;
	FACT* F = GetSubjectNondeadHead(D);

	WORDP D1 = Meaning2Word(set);

	// we walk up the tree from the word and see if it runs into D1, the set.
	if (depth == 0) 
	{
		SetFactBack(D,0); 
		if (trace & TRACE_INFER && CheckTopicTrace()) Log(STDTRACELOG,(char*)" %s ? %s ",D->word,D1->word);
	}
	unsigned int counter = 20000;
	while (F && --counter)
	{
		if (index != 0 && F->subject != M); // fact doesnt apply
		else if (F->verb == Mmember) 
		{
			// if this topic or concept has exclusions, check to see if this is a marked exclusion
			bool blocked = false;
			WORDP object = Meaning2Word(F->object);
			if (object->internalBits & HAS_EXCLUDE) 
			{
				FACT* G = GetObjectNondeadHead(object);
				while (G && !blocked)
				{
					if (G->verb == Mexclude && Meaning2Word(G->subject)->inferMark == inferMark) blocked = true;
					else G = GetObjectNondeadNext(G);
				}
			}

			// since this is not a marked exclusion, we can say it is a member
			if (F->object == set && !blocked) 
			{
				if (trace & TRACE_INFER && CheckTopicTrace()) // show the path from set back to word
				{
					Log(STDTRACELOG,(char*)"\r\n");
					Log(STDTRACETABLOG,(char*)"within: %s ",D1->word);
					WORDP path = Meaning2Word(F->subject); 
					while (path)
					{
						Log(STDTRACELOG,(char*)" %s ",path->word);
						FACT* prior = Index2Fact(GetFactBack(path));
						path = (prior) ? Meaning2Word(prior->subject) : 0;
					}
					Log(STDTRACELOG,(char*)"\r\n");
					Log(STDTRACETABLOG,(char*)"");
				}

				return true;
			}
			if (!blocked && object->inferMark != inferMark)
			{
				SetFactBack(object,Fact2Index(F));
				if (SetContains1(set,F->object,depth + 1)) return true;
			}
		}
		else if (F->verb == Mis) // a link up the wordnet ontology
		{
			if (F->object == set)  return true;
			WORDP object = Meaning2Word(F->object);
			if (object->inferMark != inferMark)
			{
				SetFactBack(object,Fact2Index(F));
				if (SetContains1(set,F->object,depth + 1)) return true;
			}
		}
		F = GetSubjectNondeadNext(F);
	}
	if (trace & TRACE_INFER && depth == 0 && CheckTopicTrace())
	{
		Log(STDTRACELOG,(char*)" not within ");
	}
	return false;
}

bool SetContains(MEANING set,MEANING M) 
{
	bool answer = SetContains1(set,M,0);
	ClearBacktracks();
	return answer;
}

static bool AllowedMember(FACT* F, unsigned int i,unsigned int is,unsigned int index)
{
	if (trace & TRACE_INFER && CheckTopicTrace())  TraceFact(F);
    unsigned int localIndex = Meaning2Index(F->subject);
	unsigned int pos = GetMeaningType(F->subject);
	bool bad = false;
	if (!i && pos ) 
	{
            if (pos & VERB && is & NOUN)  
            {
				bad = true;
            }
            else if (pos & NOUN && is & VERB)  
            {
				bad = true;
            }
            else if ((pos & ADJECTIVE || localIndex & ADVERB) && is & (NOUN|VERB))  
            {
				bad = false;
            }
 	}
	else if (index && pos && pos != index) bad = true;
	return !bad;
}

static void QueryFacts(WORDP original, WORDP D,unsigned int index,unsigned int store,char* kind,MEANING A)
{
    if (!D || D->inferMark == inferMark) return;
    D->inferMark = inferMark; 
    FACT* F;
    FACT* G = GetSubjectNondeadHead(D); 
    unsigned int count = 20000;
    unsigned int restriction = 0;
	if (kind) //   limitation on translation of word as member of a set
	{
		if (!strnicmp(kind,(char*)"subject",7)) restriction = NOUN;
		else if (!strnicmp(kind,(char*)"verb",4)) restriction = VERB;
		else if (!strnicmp(kind,(char*)"object",7)) restriction = NOUN;
	}
	while (G)
    {
        F = G;
        G = GetSubjectNondeadNext(G);
        if (trace & TRACE_INFER  && CheckTopicTrace()) TraceFact(F);

        if (!--count) 
		{
			ReportBug((char*)"matchfacts infinite loop")
			break;
		}
        uint64 flags = F->flags;
        unsigned int fromindex = Meaning2Index(F->subject);
        if (fromindex == index || !fromindex); //   we allow penguins to go up to bird, then use unnamed bird to go to ~topic
        else if (index ) continue;  //   not following this path
        else if (flags & ORIGINALWORD) continue; //   you must match exactly- generic not allowed to match specific wordnet meaning- hierarchy BELOW only

		if (F->verb == Mmember  && !AllowedMember(F,0,restriction,0)) continue; // POS doesn't match
        if (F->verb == Mmember && !(flags & ORIGINALWORD))
        {
            WORDP object = Meaning2Word(F->object);
            if (object->inferMark != inferMark) 
            {
				if (*object->word == '~') // set, not a word association
                {
					if (IsExcluded(object,original)) continue; // explicitly excluded from this set

					if (object->internalBits & TOPIC)
					{
						int topic = FindTopicIDByName(object->word);
						if (topic && !(GetTopicFlags(topic) & TOPIC_SYSTEM) && HasGambits(topic)) AddFact(store,CreateFact(MakeMeaning(original,0),A,MakeMeaning(object,0),FACTTRANSIENT));
					}
                }
                QueryFacts(original,object,0,store,kind,A);
            }
         }
    }
}

FunctionResult QueryTopicsOf(char* word,unsigned int store,char* kind) // find topics referred to by word
{
	SET_FACTSET_COUNT(store,0);
    NextInferMark(); 
    WORDP D = FindWord(word,0);
    QueryFacts(D,D,0,store,kind,MakeMeaning(FindWord((char*)"a")));
    if (trace & TRACE_INFER && CheckTopicTrace()) Log(STDTRACELOG,(char*)"QueryTopics: %s %d ",word,FACTSET_COUNT(store));
	impliedSet = ALREADY_HANDLED;
	return  NOPROBLEM_BIT;
}

static bool AddWord2Scan(int flags,MEANING M,MEANING from,int depth,unsigned int type) // mark (and maybe queue) this word + implied wordnet up hierarchy + auto-equivalences
{
    if (queueIndex >= MAX_QUEUE || !M) return false; 
	if (type && !(M & type) && GETTYPERESTRICTION(M)) return false;	// not valid type restriction

	// mark word or abandon marking
    WORDP D = Meaning2Word(M);
	unsigned int index = Meaning2Index(M);
	if (D->inferMark == saveMark || (ignoremark &&  D->inferMark == ignoremark)) return false; // marked with a current mark
	if (depth > FOLLOW_LIMIT)
	{
		ReportBug((char*)"Exceeding follow limit %s\r\n",D->word)
		return false;	
	}

	// concept set has exclusions, so if excluded is already marked, do not allow this topic to be marked
	if (D->internalBits & HAS_EXCLUDE)
	{
		FACT* G = GetObjectNondeadHead(D);
		while (G)
		{
			if (G->verb == Mexclude && Meaning2Word(G->subject)->inferMark == saveMark) return false;
			G = GetObjectNondeadNext(G);
		}
	}

	D->inferMark = saveMark; 
	if (flags & QUEUE) queue[queueIndex++] = M;

	if (trace & TRACE_QUERY && CheckTopicTrace()) 
	{
		static char last[1000];
		if (from)
		{
			char* mean = WriteMeaning(from);
			if (stricmp(last,mean))
			{
				Log(STDTRACELOG,(char*)"\r\n");
				Log(STDTRACETABLOG,(char*)"(%s=>) ",mean);
				if (strlen(mean) > 999) ReportBug("Scan insert > 1000")
				else strcpy(last,mean);
			}
		}
		Log(STDTRACELOG,(flags & QUEUE) ? (char*)" %s+" : " %s. ",WriteMeaning(M));
	}

    // auto check all equivalences as well
    FACT* F = GetSubjectNondeadHead(D);
    while (F)
    {
        if (F->verb == Mmember) // can be member of an ordinary word (like USA member United_States_of_America), creates equivalence
		{
			WORDP D = Meaning2Word(F->object);
			if (*D->word != '~') AddWord2Scan(flags,F->object,F->subject,depth+1,type); // member is not to a set, but to a word. So it's an equivalence
		}
        F = GetSubjectNondeadNext(F);
    }
	
	// and if item is generic, all synsets
	if (index == 0 && !(flags & ORIGINALWORD))
	{
		unsigned int count = GetMeaningCount(D);
		for (unsigned int i = 1; i <= count; ++i) AddWord2Scan(flags,GetMeaning(D,i),M,depth+1,type);
	}

	return true;
}

static bool AddWordOnly(int flags,char* word,unsigned int type) // mark (and maybe queue) this word 
{
    if (queueIndex >= MAX_QUEUE || !*word) return false; 

	// mark word or abandon marking
    WORDP D = StoreWord(word,AS_IS);
	if (D->inferMark == saveMark) return false; // marked with a current mark
	D->inferMark = saveMark; 
	if (flags & QUEUE) queue[queueIndex++] = MakeMeaning(D);

	if (trace & TRACE_QUERY && CheckTopicTrace())  Log(STDTRACELOG,(flags & QUEUE) ? (char*)" %s+" : (char*)" %s. ",D->word);

	return true;
}

static void AddWordOrSet2Scan(unsigned int how, char* word,int depth)
{
	++depth;
	if (!(how & ORIGINALWORD) && *word == '~' && word[1]) //   recursive on set and all its members
    {
		WORDP D = FindWord(word,0);
		if (D)
		{
			if (how & NOTOPIC && D->internalBits & TOPIC) {;} 
			else if (AddWord2Scan(how, MakeMeaning(D,0),0,depth,0)) AddSet2Scan(how,D,depth);  //   mark the original name and then follow its members
		}
	}
	else 
	{
		if (*word == '\'')
		{
			// WORDP D = FindWord(word);
			//if (!D) 
			++word; // but dont harm 'tween_decks which is natural
		}
		AddWord2Scan(how, ReadMeaning(word, true, true), 0, depth, 0);
	}
}

static void AddSet2Scan(unsigned int how,WORDP D,int depth)
{
	++depth;
	FACT* F = GetObjectNondeadHead(D);
	while (F)
	{
		if (F->verb == Mmember) AddWordOrSet2Scan(how | (F->flags & ORIGINALWORD),Meaning2Word(F->subject)->word,depth);
		F = GetObjectNondeadNext(F);
	}
}

// used by query setup - scans noun hierarchies upwards for inference
static void ScanHierarchy(MEANING T,int savemark,unsigned int flowmark,bool up,unsigned int flag, unsigned int type)
{
	if (!T) return;
	if (trace & TRACE_QUERY && CheckTopicTrace()) Log(STDTRACELOG,(char*)"\r\nHierarchy: (%s=>) ",WriteMeaning(T));
	if (!AddWord2Scan(flag,T,0,0,type)) return;

	parentIndex = parentWalk  = 0;
	parents[parentIndex++] = T;

	WORDP A = Meaning2Word(T);

	int index = Meaning2Index(T);
	int start = 1;

	//   find its wordnet ontological meanings, they then link synset head to synset head
	//   automatically store matching synset heads to this also -- THIS APPLIES ONLY TO THE ORIGINAL WORDP
	MEANING* onto = GetMeaningsFromMeaning(T);
	unsigned int size = GetMeaningCount(A);
	if (!up || flag & BLOCKMEANING ) size = 0;//   we are a specific meaning already, so are the synset head of something - or are going down
	else if (index) start= size = index;	//   do JUST this one
	else if (!size) size = 1; //   even if no ontology, do it once for 
	for  (unsigned int k = start; k <= size; ++k) // for each meaning of this word, mark its synset heads
	{
		MEANING T1;
		if (GetMeaningCount(A)) 
		{
			T1 = (MEANING)(ulong_t)onto[k]; //   this is the synset ptr.
			if (T1 & SYNSET_MARKER) T1 = MakeMeaning(A,k) | SYNSET_MARKER;
			else T1 = GetMaster(T1);

			if (type && !(T1 & type)) continue;	
			if (! AddWord2Scan(flag,T1,T,0,type)) continue; //   either already marked OR to be ignored
			parents[parentIndex++] = T1;	
		}
	}

	while (parentWalk < parentIndex) //   walk up its chains in stages
	{
		T = parents[parentWalk++];
		if (!T) continue;
		if (parentIndex > MAX_PARENTS) break;	//   overflow may happen. give up
		WORDP D = Meaning2Word(T);
		unsigned int index = Meaning2Index(T);
		
		// now follow facts of the synset head itself or the word itself.
		FACT* F = GetSubjectNondeadHead(D);
		while (F)
		{
			WORDP verb = Meaning2Word(F->verb); 
			FACT* G = F;
			if (trace & TRACE_QUERY && CheckTopicTrace()) TraceFact(F);
			F = GetSubjectNondeadNext(F);
			if (verb->inferMark !=  flowmark) continue;

			//   if the incoming ptr is generic, it can follow out any generic or pos_generic reference.
			//   It cannot follow out a specific reference of a particular meaning.
			//   An incoming non-generic ptr is always specific (never pos_generic) and can only match specific exact.
			if (index && T !=  G->subject) continue; //   generic can run all meanings out of here
			MEANING x = G->object; 
			if (type && GETTYPERESTRICTION(G->subject )  && !(type & GETTYPERESTRICTION(G->subject ))) continue;  // fact has bad type restriction on subject
			if (!AddWord2Scan(flag,x,G->subject,0,type)) continue;	//   either already marked OR to be ignored
			parents[parentIndex++] = x;
		}
	}

	if (trace & TRACE_QUERY && CheckTopicTrace()) Log(STDTRACELOG,(char*)"\r\n");
}

static bool Riccochet(unsigned int baseFlags, FACT* G,int set,unsigned int limit,unsigned int rmarks,unsigned int rmarkv, unsigned int rmarko)
{// use two fields to select a third. Then look at facts of that third to find a matching verb.
	FACT* F;
	WORDP D1;
	if (G->flags & (FACTSUBJECT|FACTOBJECT)) // we cant get here if the wrong field is a fact.
	{
		D1 = fact;
		F = (baseFlags & USE_ORIGINAL_SUBJECT) ? Index2Fact(G->subject) : Index2Fact(G->object);
	}
	else
	{
		D1 = (baseFlags & USE_ORIGINAL_SUBJECT) ? Meaning2Word(G->subject) : Meaning2Word(G->object);
		F  = (baseFlags & RICCOCHET_USING_SUBJECT) ? GetSubjectNondeadHead(D1) : GetObjectNondeadHead(D1);
	}
	if (trace & TRACE_QUERY && CheckTopicTrace()) 
	{
		WORDP S = (G->flags & FACTSUBJECT) ? fact : Meaning2Word(G->subject);
		WORDP V = (G->flags & FACTVERB) ? fact : Meaning2Word(G->verb);
		WORDP O = (G->flags & FACTOBJECT) ? fact : Meaning2Word(G->object);
		char* use = (baseFlags & RICCOCHET_USING_SUBJECT) ? (char*) "subjectfield" : (char*) "objectfield";
		if (baseFlags & USE_ORIGINAL_SUBJECT) Log(STDTRACELOG,(char*)"Riccochet incoming (%s %s %s) via subject %s using %s\r\n",S->word,V->word,O->word,D1->word,use);
		else Log(STDTRACELOG,(char*)"Riccochet incoming (%s %s %s) via object %s using %s\r\n",S->word,V->word,O->word,D1->word,use);
	}

	// walk all facts at node testnig for riccochet
	while (F)	 // walk_of_S3
	{
		if (trace & TRACE_QUERY && CheckTopicTrace()) TraceFact(F);
		FACT* I = F; 
		if (D1 == fact) F = NULL; // only the 1 main fact
		else F = (baseFlags & RICCOCHET_USING_SUBJECT)  ? GetSubjectNondeadNext(F) : GetObjectNondeadNext(F);
		if (I->flags & FACTDEAD) continue;	 // cannot use this

		// reasons this fact is no good
		if (I->flags & MARKED_FACT) continue; // already seen this answer
		if ((baseFlags & SYSTEMFACTS && I > factLocked) || (baseFlags & USERFACTS && I <= factLocked) ) continue; // restricted by owner of fact
		if (rmarks && (I->flags & FACTSUBJECT || Meaning2Word(I->subject)->inferMark != rmarks)) continue; // mark must match
		if (rmarkv && (I->flags & FACTVERB || Meaning2Word(I->verb)->inferMark != rmarkv)) continue; // mark must match
		if (rmarko && (I->flags & FACTOBJECT || Meaning2Word(I->object)->inferMark != rmarko)) continue; // mark must match

		I->flags |= MARKED_FACT;
		AddFact(set,I); 
		if (trace & TRACE_QUERY && CheckTopicTrace()) 
		{
			Log(STDTRACELOG,(char*)"    Found:");
			TraceFact(I);
		}
		if (FACTSET_COUNT(set) >= limit) return false;
	}
	return true;
}

static bool ConceptPropogateTest(MEANING M,unsigned int mark,unsigned int depth)  // is this meaning ultimately in a marked set
{
	if (!depth) 
	{
		NextInferMark();
		if (trace & TRACE_QUERY && CheckTopicTrace())  Log(STDTRACELOG,(char*)"\r\n ~propogate: ");
	}
	
	FACT* F = GetSubjectNondeadHead(M);
	while (F)
	{
		if (F->verb == Mmember && F->subject == M)
		{
			MEANING O = F->object;
			WORDP D = Meaning2Word(O);
			if (trace & TRACE_QUERY && CheckTopicTrace())  Log(STDTRACELOG,(char*)" %d->%s ",depth,D->word);
 
			if (D->inferMark == mark) return true; // this is what we seek
			if (D->inferMark != inferMark)// not already visited this pass or is marked for current query for other use
			{
				if (D->inferMark < currentBaseInfer || D->inferMark >= currentFreeInfer) D->inferMark = inferMark; // safely set seen this (efficiency)
				if (*D->word == '~' && ConceptPropogateTest(O,mark,depth+1)) return true;  // NOT setting been here- and dont follow fake members ( word member word)
			}
		}
		F = GetSubjectNondeadNext(F);
	}
	return false;
}

unsigned int Query(char* kind, char* subjectword, char* verbword, char* objectword, unsigned int count, char* fromset, char* toset, char* propogate, char* match)
{
	int store = GetSetID(toset);
	if (store == ILLEGAL_FACTSET) store = 0;
	if (trace & TRACE_QUERY && CheckTopicTrace()) Log(STDTRACETABLOG,(char*)"QUERY: @%d %s ",store,kind);
	WORDP C = FindWord(kind,0);
	if (!C || !(C->internalBits & QUERY_KIND)) 
	{
		ReportBug((char*)"Illegal query name: %s",kind)
		return 0;
	}
	char copy[MAX_WORD_SIZE];	// hold the actual control value, so we can overwrite it
	char* control = NULL;
	if (C->w.userValue && *C->w.userValue) 
	{	
		strcpy(copy,C->w.userValue);
		control = copy;
	}
	else 
	{
		ReportBug((char*)"query control lacks data %s",kind)
		return 0;
	}

	// get correct forms of arguments - _ is an empty argument, but legal
	char word[MAX_WORD_SIZE];
	int n;
	if (!strchr(subjectword,' ')) // anything with a natural space in it should be left alone
	{
	    n = BurstWord(subjectword);
		if (n > 1) strcpy(subjectword,JoinWords(n,false));
	}
  	if (!strchr(verbword,' ')) // anything with a natural space in it should be left alone
	{
		n = BurstWord(verbword);
		if (n > 1) strcpy(verbword,JoinWords(n,false));
	}
    if (!strchr(objectword,' ')) // anything with a natural space in it should be left alone
	{	
		n = BurstWord(objectword);
		if (n > 1) strcpy(objectword,JoinWords(n,false));
	}
	if (!strchr(match,' ')) // anything with a natural space in it should be left alone
	{	
		n = BurstWord(match);
		if (n > 1) strcpy(match,JoinWords(n,false));
	}
	if (!strchr(propogate,' ')) // anything with a natural space in it should be left alone
	{	
		n = BurstWord(propogate);
		if (n > 1) strcpy(propogate,JoinWords(n,false));
	}
	if (trace & TRACE_QUERY && CheckTopicTrace()) 
	{
		Log(STDTRACETABLOG,(char*)" control: %s  s/v/o:[%s %s %s] count:%d ",control,subjectword,verbword,objectword,count);   
		if (*fromset != '?') Log(STDTRACELOG,(char*)"fromset:%s ",fromset);   
		if (*toset != '?') Log(STDTRACELOG,(char*)"toset:%s ",toset);   
		if (*propogate != '?') Log(STDTRACELOG,(char*)"propogate:%s",propogate);   
		if (*match != '?') Log(STDTRACELOG,(char*)"match:%s",match);   
		Log(STDTRACELOG,(char*)"\r\n");   
		Log(STDTRACETABLOG,(char*)"");
	}

	//   handle what sets are involved
	if (impliedOp == 0 || impliedOp == '=')	SET_FACTSET_COUNT(store,0); // auto kill content
	else if (impliedOp != '+') 
	{
		SET_FACTSET_COUNT(store,0); // we dont support other ops like -=
		return 0; 
	}
	int from = GetSetID(fromset);
	if (from == ILLEGAL_FACTSET) from = 0;
	unsigned int baseFlags = 0;

	if (!stricmp(fromset,(char*)"user")) baseFlags |= USERFACTS;
	if (!stricmp(fromset,(char*)"system")) baseFlags |= SYSTEMFACTS;

	queueIndex = 0; 
	ignoremark = 0;
	unsigned int baseMark = inferMark; //   offsets of this value
	currentBaseInfer = baseMark + 1;	// start of used marks

	//   process initialization
nextsearch:  //   can do multiple searches, thought they have the same basemark so can be used across searchs (or not) up to 9 marks

#ifdef INFORMATION
# first segment describes what to initially mark and initially queue for processing (sources of facts)
#	Values:
#		1..9  = set global tag to this label - 0 means turn off global tag
#		i =  use argument tag on words to ignore during a  tag or queue operation 
#			Next char is tag label. 0 means no ignoremark
#		s/v/o/p/m/~set/tick-word  = use subject/verb/object/progogate/match/factset argument as item to process or use named set or given word 
#	    S/V/O  choice is a fact id
#			This is automatically marked using the current mark and is followed by 
#				q  = queue items (sets will follow to all members recursively and wordnet identities will propogate up)
#				Q  = queue this exact word only
#				t  = tag (no queue) items
#				e  = expandtag (no queue) (any set gets all things below it tagged)
#				h  = tag propogation from base (such propogation might be large)
#					1ST char after h is mark on verbs to propogate thru
#					2nd char is t or q (for tag or mark/queue)
#					3rd char (< >) after h is whether to propogate up from left/subject to object or down/right from object to subject when propogating
#		n = implied all topics marked to ignore on object
#		f = use given facts in from as items to process -- f@n  means use this set
#			This is followed by 
#			s/v/o/f	= use corresponding field of fact or entire fact
#		the value to process will be marked AND may or may not get stored, depending on following flag being q or m
#endif

	//   ZONE 1 - mark and queue setups
	char myset[10];
	int baseOffset = 0; //   facts come from this side, and go out the verb or other side
	char* choice;
	char* at;
	int qMark = 0;
	int mark = 0;
	int whichset = 0;
	fact = StoreWord((char*)"fact");// for foreign languages insure word is there

	char maxmark = '0'; // deepest mark user has used
	if (trace & TRACE_QUERY && CheckTopicTrace()) 
	{
		// convert all _ and periods to spaces for easier viewing
		char* underscore;
		while ((underscore = strchr(control,'_'))) *underscore = ' ';

		char* colon = strchr(control,':');
		if (colon) *colon = 0;
		Log(STDTRACELOG,(char*)"@@@ Control1 mark/queue: %s\r\n",control);
		if (colon) *colon = ':';
		Log(STDTRACETABLOG,(char*)"");
	}
	--control;
	bool facttype = false;
	while (*++control && *control != ':' )
	{
		choice = NULL;
		switch(*control)
		{
			case '_': case '.': case ' ': // does nothing per se
				continue;	
			case '0':  // means NO savemark
				saveMark = 0; 
				continue;
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': //   set current marks
				saveMark = baseMark + *control - '0';
				if (*control > maxmark) maxmark = *control;
				continue;
			case 'n':  // ignore all member facts involving topic as object
				if (trace & TRACE_QUERY && CheckTopicTrace()) Log(STDTRACELOG,(char*)" ignore all member facts w topics as objects ");
				++control; 
				baseFlags |= NOTOPIC;
				continue;
			case '~': case '\'': //   use named set or named word
				choice = control; 
				at = strchr(control,'.'); //   set name ends with _ or . or space (generated)
				if (!at) at = strchr(control,'_');
				if (!at) at = strchr(control,' ');
				if (!at) 
				{
					ReportBug((char*)"Couldnt find end of name %s in control",control)
					return 0; // couldn't find end of name
				}
				*at = 0;
				control = at;	// skip past to end
				break;
			case 'i': 
				++control;
				if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)" ignore #%c results ", *control);
				ignoremark = (*control == '0') ? 0 : (baseMark + (*control - '0'));
				break;
			case 's': 
				choice = subjectword; 
				mark = 0; 
				break;
			case 'S':
				choice = subjectword;
				mark = 0;
				facttype = true;
				break;
			case 'v': // automatically quote verbs. NEVER let them wander
				if (*verbword == '\'') strcpy(word,verbword);
				else
				{
					*word = '\''; 
					strcpy(word+1,verbword);
				}
				choice = word; 
				mark = 1; 
				break;
			case 'V':
				choice = verbword;
				mark = 1;
				facttype = true;
				break;
			case 'o': 
				choice = objectword; 
				mark = 2; 
				break;
			case 'O':
				choice = objectword;
				mark = 2;
				facttype = true;
				break;
			case 'p': 
				choice = propogate; 
				break;
			case 'm': 
				choice = match; 
				break;
			case 'f':  //   we have incoming facts to use 
				whichset = (control[1] == '@') ? GetSetID(++control) : from; // only allowed sets 1-9
				if (whichset == ILLEGAL_FACTSET) return 0;
				sprintf(myset,"@%d",whichset);
				choice = myset;
				break;
			default: 
				ReportBug((char*)"Bad control code for query init %s(%s) %s",C->word,C->w.userValue,control)
				return 0;

		}
		if (choice) // we have something to follow
		{
			++control; // now see flags on the choice
			unsigned int flags = baseFlags;
			// dont treat 'tween_decks as an originalword request
			if (choice[0] == '\'' ) //  && !IsAlphaUTF8(choice[1]))  //dont expand this beyond its first leve --   $$tmp would come in with its value, which if set would fan out.  '$$tmp gets just its value
			{
				flags |= ORIGINALWORD;
				++choice;
			}
			if (choice[0] == '^') // replace the function arg
			{
				strcpy(word,FNVAR(choice+1));
				choice = word;
			}
			// dynamic choices
			if (choice[0] == '_') 
			{
				int wild = GetWildcardID(choice);
				if (wild == ILLEGAL_MATCHVARIABLE){;}
				else if (flags != 0)
				{
					choice = wildcardOriginalText[wild];
					if (*choice != '~') flags = 0; // '_0 treated as normal word unquoted (original meaning) unless its a set, then it must not expand instead
				}
				else choice = wildcardCanonicalText[wild];
			}
			else if (choice[0] == USERVAR_PREFIX && choice[1]) choice = GetUserVariable(choice);
			else if (choice[0] == SYSVAR_PREFIX && choice[1]) choice = SystemVariable(choice,NULL);
			else if (choice[0] == '@' )
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace())  
				{
					Log(STDTRACELOG,(char*)"\r\n");
					Log(STDTRACETABLOG,(char*)"FactField: %c(%d) ",saveMark-baseMark+'0',saveMark);
				}
				choice = NULL;
				for (unsigned int j = 1; j <= FACTSET_COUNT(whichset); ++j) 
				{
					FACT* F = factSet[whichset][j];
					if (F->flags & FACTDEAD) continue;	 // cannot use this

					MEANING M;
					if (*control == 'f') //   whole fact can be queued. It cannot be marked as on the queue
					{
						queue[queueIndex++] = Fact2Index(F);
						continue;
					}
					else if (*control == 's') M = F->subject; 
					else if (*control == 'v') M = F->verb; 
					else if (*control == 'o') M = F->object; 
					else 
					{
						ReportBug((char*)"bad control for query %s(%s) %s",C->word,C->w.userValue,control)
						return 0;
					}
					if (trace & TRACE_QUERY  && CheckTopicTrace())  Log(STDTRACELOG,(char*)" %s ",WriteMeaning(M));
					AddWord2Scan((control[1] == 'q') ? (QUEUE|flags) : flags,M,0,0,0);
				}
				continue;
			}
			else if (facttype) // choice is a fact
			{
				if (!IsDigit(*choice)) return 0; // illegal fact reference
				unsigned int f = atoi(choice);
				if (atoi(choice) > (int) Fact2Index(factFree)) return 0;	// beyond legal range
				// we can q it but we dont mark it....
				queue[queueIndex++] = f;
				baseFlags |= FACTTYPE; 
				facttype = false;
				continue;
			}
			if (choice[0] == '\\')  // accept this unchanged
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)" raw ");
				++choice;
			}

			// for non-factset values of choice
			char buf[1000];
			if (trace & TRACE_QUERY  && CheckTopicTrace()) sprintf(buf,(char*)"%s #%c(%d)",choice,saveMark-baseMark+'0',saveMark);
			if (*control == 'q') 
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) 
				{
					Log(STDTRACELOG,(char*)"\r\n");
					Log(STDTRACETABLOG,(char*)"Tag+Queue: %s ",buf);
				}
				qMark = saveMark;	//   if we q more later, use this mark by default
				if (*choice) AddWordOrSet2Scan(QUEUE|flags,choice,0); //   mark and queue items
			}
			else if (*control == 'Q') 
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) 
				{
					Log(STDTRACELOG,(char*)"\r\n");
					Log(STDTRACETABLOG,(char*)"Tag+QueueWord: %s ",buf);
				}
				qMark = saveMark;	//   if we q more later, use this mark by default
				if (*choice) AddWordOnly(QUEUE|flags,choice,0); //   mark and queue item
			}
			else  if (*control == 't') 
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) 
				{
					Log(STDTRACELOG,(char*)"\r\n");
					Log(STDTRACETABLOG,(char*)"Tag: %s ",buf);
					if (flags & ORIGINALWORD) Log(STDTRACELOG,(char*)" don't expand ");
				}
				if (!*choice);
				else if (*choice == '\'') AddWord2Scan(flags, ReadMeaning(choice+1,true,true),0,0,0); // ignore unneeded quote
				else AddWord2Scan(flags, ReadMeaning(choice,true,true),0,0,0);
			}
			else  if (*control == 'T') // tag and dont follow
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) 
				{
					Log(STDTRACELOG,(char*)"\r\n");
					Log(STDTRACETABLOG,(char*)"Tag: %s ",buf);
					if (flags & ORIGINALWORD) Log(STDTRACELOG,(char*)" don't expand ");
				}
				qMark = saveMark;	//   if we q more later, use this mark by default
				if (*choice) AddWordOnly(flags,choice,0); //   mark and queue item
			}
			else  if (*control == 'e') 
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) 
				{
					Log(STDTRACELOG,(char*)"\r\n");
					Log(STDTRACETABLOG,(char*)" ExpandTag: %s ",buf);
				}
				if (*choice) AddWordOrSet2Scan(flags,choice,0); // tag but dont queue
			}
			else if (*control == '<' || *control == '>') //   chase hierarchy (exclude VERB hierarchy-- we infer on nouns)
			{ //   callArgumentList are:  flowverbs,  queue or mark, 
				char kind = *control; //< or > 
				int flows = baseMark + *++control - '0'; // mark for flow
				unsigned int flag = (*++control == 'q') ? QUEUE : 0;
				if (trace & TRACE_QUERY  && CheckTopicTrace()) 
				{
					if (flag) Log(STDTRACELOG,(char*)" Tag+Queue Propogate %c ",kind);
					else Log(STDTRACELOG,(char*)" Tag Propogate %c ",kind);
				}
				// if (flag & QUEUE) flag |= BLOCKMEANING;
				// mark subject 0 and object 2 are nouns, 1 is verb
				if (*choice) ScanHierarchy(ReadMeaning(choice,true,true),saveMark,flows,kind == '<',flag, (mark != 1) ? NOUN : VERB);
			}
			else 
			{
				ReportBug((char*)"bad follow argument %s",control)
				return 0;
			}
			if (trace & TRACE_QUERY  && CheckTopicTrace()) 
			{
				Log(STDTRACELOG,(char*)"\r\n");
				Log(STDTRACETABLOG,(char*)"");
			}
		}
		else if (baseFlags & USERFACTS) // transfer over user flags
		{
			unsigned int total = factFree - factLocked;
			if (total >= MAX_QUEUE) total = MAX_QUEUE - 1;
			FACT* F = factFree - total;
			while (++F <= factFree) queue[queueIndex++] = Fact2Index(F);
			baseFlags |= FACTTYPE; 
		}
	}
	inferMark += maxmark - '0';	//   update to use up marks we have involved
	currentFreeInfer = inferMark;
	ignoremark = 0;				//   require be restated if a matching requirement

	//   ZONE 2 - how to use contents of the queue
	//   given items in queue, what field from a queued entry to use facts from 
	if (*control) control = SkipWhitespace(control+1); // skip over the : and past any white space
	if (!strncmp(control,(char*)"queue",5))  control = SkipWhitespace(control+5);  // just a label defining the field
	if (trace & TRACE_QUERY  && CheckTopicTrace()) 
	{
		char* colon = strchr(control+1,':');
		if (colon) *colon = 0;
		if (control[1]) 
		{
			Log(STDTRACELOG,(char*)"@@@ Control2 queue use: %s\r\n",control+1);
			Log(STDTRACETABLOG,(char*)"");
		}
		if (colon) *colon = ':';
	}
	if (*control && --control) while (*++control && *control != ':' )
	{
		switch (*control)
		{
		case '_': case '.':  case ' ': // does nothing per se
			continue;
		case 's': 
			baseOffset = 0; 
			break;
		case 'v': 
			baseOffset = 1; 
			break;
		case 'o': 
			baseOffset = 2; 
			break;
		case 'f': 
			baseFlags |= FACTTYPE; 
			break; //   queued items are facts instead of meaning
		case 'e':
			baseOffset = 3;
			break;
		default:
			ReportBug((char*)"Bad control code for #2 (queue test) %s(%s) %s",C->word,C->w.userValue,control)
			return 0;
		}
	}

	whichset = store;

	//   ZONE 3 - how to detect facts we can return as answers and where they go
	//   set marks to compare on (test criteria for saving an answer)
	bool sentences = false;
	bool sentencev = false;
	bool sentenceo = false;
	unsigned int marks = 0, markv = 0, marko = 0;
	unsigned int markns = 0, marknv = 0, markno = 0;
	unsigned int rmarks = 0, rmarkv = 0, rmarko = 0;
	unsigned int intersectMark = 0, propogateVerb = 0;
	unsigned int systemFlags = 0;
	unsigned int ultimateSubjectMember = 0;
	unsigned int ultimateVerbMember = 0;
	unsigned int ultimateObjectMember = 0;
	unsigned int factflags = 0;
	saveMark = qMark;	//   default q value is what we used before
	if (*control) control = SkipWhitespace(control+1); // skip over the : and past any white space
	if (!strncmp(control,(char*)"match",5)) control = SkipWhitespace(control + 5);  // just a comment label defining what the field does
	if (trace & TRACE_QUERY && *control && CheckTopicTrace()) 
	{
		char* colon = strchr(control+1,':');
		if (colon) *colon = 0;
		if (control[1]) 
		{
			Log(STDTRACELOG,(char*)"@@@ Control3 match requirements: %s\r\n",control+1); // if there is data
			Log(STDTRACETABLOG,(char*)"");
		}
		if (colon) *colon = ':';
	}
	bool noSystemFlag = false;
	if (*control && --control) while (*++control && *control != ':' )
	{
		switch (*control)
		{
		case '_': case '.':  case ' ':  // does nothing per se
			continue;
		case '!': //   do NOT match this
			++control;
			if (*control == 's')  
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)" don't match subjects #%c ",*control);
				markns = baseMark + (*++control - '0');
			}
			if (*control == 'v')  
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)" don't match verbs #%c ",*control);
				marknv = baseMark + (*++control - '0');
			}
			if (*control == 'o')  
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)" don't match objects #%c ",*control);
				markno = baseMark + (*++control - '0');
			}
			if (*control == 'n')
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)" don't match concepts with noconceptlist marking ");
				noSystemFlag = true;
				systemFlags = NOCONCEPTLIST;
			}
			break;
		case 'x': // field after has sentence mark
			++control;
			if (*control == 's')  sentences = true; 
			else if (*control == 'v')   sentencev = true; 
			else if (*control == 'o')   sentenceo = true; 
			break;

			//   normal tests of fact fields
		case 's': 
			marks = baseMark + (*++control - '0'); 
			if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)" subject must be #%c ",*control);
			break;
		case 'v': 
			markv = baseMark + (*++control - '0'); 
			if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)" verb must be #%c ",*control);
			break;
		case 'o': 
			marko = baseMark + (*++control - '0'); 
			if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)" object must be #%c ",*control);
			break;
			
		//   dont pay attention to this value during search (opposite the baseOffset)
		case 'i': 
			++control;
			if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)" ignore results with #%c ",*control);
			ignoremark = (*control == '0') ? 0 : (baseMark + (*control - '0'));
			break;
		//   future queuing uses this mark (hopefully same as original queued)
		case 'q': 
			saveMark = baseMark + (*++control - '0');  
			break;
		case '~': 
			intersectMark = baseMark + (*++control - '0'); //   label to intersect to in propogation
			break;
		case 'n':
			systemFlags |= NOCONCEPTLIST;
			break;
		case 't': 
			baseFlags |= FINDTOPIC;
			break;
		case 'c': 
			baseFlags |= FINDCONCEPT;
			break;
		case '<':  case '>': 
			if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)" propogate on verb #%c ",*control);
			propogateVerb = baseMark + (*++control - '0'); //   label of verbs to propogate on
			break;
		case '@': //   where to put answers (default is store)
			if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)" store facts in @%c",*control);
			whichset = *++control - '0';
			break;
		case '^':
			baseFlags |= UPDICTIONARY;
			break;	// rise up dominant dictionary meaning 
		case 'f': // has this flag from match
			factflags = atoi(match);
			break;
		case 'm': 
			++control;
			if (*control == 'o') ultimateObjectMember = baseMark + (*++control - '0'); 
			else if (*control == 'v') ultimateVerbMember = baseMark + (*++control - '0'); 
			else if (*control == 's') ultimateSubjectMember = baseMark + (*++control - '0'); 
			if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)" object must ultimately be member of set marked #%c ",*control);
			break;
		default: 
			ReportBug((char*)"Bad control code for Zone 3 test %s(%s) %s",C->word,C->w.userValue,control)
			return 0;
		}
	}

	//   ZONE 4- how to migrate around the graph and save new queue entries
	//   now examine riccochet OR other propogation controls (if any)
	//   May say to match another field, and when it matches store X on queue
	if (*control) control = SkipWhitespace(control+1); // skip over the : and past any white space
	if (!strncmp(control,(char*)"walk",4)) control = SkipWhitespace(control +4);
	if (trace & TRACE_QUERY && *control && CheckTopicTrace()) 
	{
		char* colon = strchr(control+1,':');
		if (colon) *colon = 0;
		if (control[1]) 
		{
			Log(STDTRACELOG,(char*)"@@@ Control4 riccochet: %s\r\n",control+1);
			Log(STDTRACETABLOG,(char*)"");
		}
		if (colon) *colon = ':';
	}
	if (*control && --control) while (*++control && *control != '|')
	{
		switch (*control)
		{
		case '_':  case '.': case ' ': // does nothing per se
			continue;
		//   tests on riccochet fields
		case 'S': 
			if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)"Riccochet on Subject #%c ",*control);
			rmarks = baseMark + (*++control - '0'); 
			break;
		case 'V': 
			if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)"Riccochet on Verb #%c ",*control);
			rmarkv = baseMark + (*++control - '0'); 
			break;
		case 'O': 
			if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)"Riccochet on Object #%c ",*control);
			rmarko = baseMark + (*++control - '0'); 
			break;

		//   fields to access as next element from a NORMAL fact - 1st reference is base fact, second is riccochet fact
		case  's': //   MEANING offsets into a fact to get to subject,verb,object
			baseFlags |=  (baseFlags & (USE_ORIGINAL_SUBJECT|USE_ORIGINAL_OBJECT)) ? RICCOCHET_USING_SUBJECT : USE_ORIGINAL_SUBJECT;
			break; 
		case  'v':
			ReportBug((char*)"bad riccochet field")
			return 0; 
		case  'o': 
			baseFlags |=  (baseFlags & (USE_ORIGINAL_SUBJECT|USE_ORIGINAL_OBJECT)) ? RICCOCHET_USING_OBJECT : USE_ORIGINAL_OBJECT;
			break;
		default: 
			ReportBug((char*)"Bad control code for Zone 4 test %s(%s) %s",C->word,C->w.userValue,control)
			return 0;
		}
	}
	if (trace & TRACE_QUERY  && CheckTopicTrace()) 
	{
		Log(STDTRACELOG,(char*)"Start processing loop\r\n");
		Log(STDTRACETABLOG,(char*)"");
	}
	
	//   now perform the query
	FACT* F;
	unsigned int  scanIndex = 0;
	int pStart,pEnd;
	while (scanIndex < queueIndex)
	{
		MEANING next = queue[scanIndex++];
		next &= SIMPLEMEANING; // no type bits, just word ref
        unsigned int index;

		//   get node which has fact list on it
		if (baseFlags & FACTTYPE) // q data is facts
		{
			F = Index2Fact(next);
			if (baseOffset == 0) F = GetSubjectNondeadHead(F);
			else if (baseOffset == 1) F = GetVerbNondeadHead(F);
			else if (baseOffset == 2) F = GetObjectNondeadHead(F);
			index = 0;
		}
		else // q data is meanings
		{
			WORDP D = Meaning2Word((ulong_t)next);
			if (baseOffset == 0) F = GetSubjectNondeadHead(D);
			else if (baseOffset == 1) F = GetVerbNondeadHead(D);
			else  F = GetObjectNondeadHead(D);
			index = Meaning2Index(next);
		}

		bool once = false;
		while (F)
		{
			if (trace & TRACE_QUERY  && CheckTopicTrace()) TraceFact(F,true);

			//   prepare for next fact to walk
			FACT* G = F;

			MEANING INCOMING;
			MEANING OUTGOING;
			// what fields do we process by default
			if (baseOffset == 0) 
			{
				F = GetSubjectNondeadNext(F);
				INCOMING = G->subject;
				OUTGOING = G->object;
			}
			else if (baseOffset == 1)  
			{
				F = GetVerbNondeadNext(F);
				INCOMING = G->verb;
				OUTGOING = G->object;
			}
			else if (baseOffset == 2)
			{
				F = GetObjectNondeadNext(F);
				INCOMING = G->object;
				OUTGOING = G->subject;
			}
			else // baseOffset == 3 when using user facts as the initial base
			{
				INCOMING = G->subject;
				OUTGOING = G->object;
			}
			if (G->flags & FACTDEAD) continue;	 // cannot use this
			if (baseFlags & USERFACTS && G <= factLocked) continue; // restricted by kind of fact
			if (baseFlags & SYSTEMFACTS && G > factLocked) continue;

			INCOMING &= SIMPLEMEANING;
			OUTGOING &= SIMPLEMEANING;
			//   is this fact based on what we were checking for? (we store all specific instances on the general dictionary)
			if (index && INCOMING != next) continue;
			WORDP OTHER = Meaning2Word(OUTGOING);

			WORDP S = (G->flags & FACTSUBJECT) ? fact : Meaning2Word(G->subject);
			WORDP V = (G->flags & FACTVERB) ? fact : Meaning2Word(G->verb);
			WORDP O = (G->flags & FACTOBJECT) ? fact : Meaning2Word(G->object);

			//   if this is part of ignore set, ignore it (not good if came via verb BUG)
			if (ignoremark && OTHER->inferMark == ignoremark ) 
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)"ignore ");
				continue;
			}
			// pay no attention to topic facts
			if (baseFlags & NOTOPIC && O->internalBits & TOPIC)
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) Log(STDTRACELOG,(char*)"notopic ");
				continue;
			}
			
			bool match = true;

			// follow dictionary path?
			if (baseFlags & UPDICTIONARY && G->verb == Mis && !once)
			{
				once = true;
				if (AddWord2Scan(QUEUE,OUTGOING,INCOMING,0,0)){;} // add object onto queue
				continue;
			}

			//   field validation fails on some field?
			if (marks && S->inferMark != marks)  match = false;
			else if (markns && S->inferMark == markns)  match = false;
			if (markv && V->inferMark != markv) match = false;
			else if (marknv && V->inferMark == marknv)  match = false;
			if (marko && O->inferMark != marko)  match = false;
			else if (markno && O->inferMark == markno)  match = false;
			if (sentences && !GetNextSpot(S,0,pStart,pEnd)) match = false;
			if (sentencev && !GetNextSpot(V,0,pStart,pEnd)) match = false;
			if (sentenceo && !GetNextSpot(O,0,pStart,pEnd)) match = false;
			if (factflags && !(G->flags & factflags)) 
				match = false; // lacks appropriate fact flags
			if (intersectMark && OTHER->inferMark != intersectMark) match = false;// we have not reached requested intersection
			if (ultimateSubjectMember && !(ConceptPropogateTest(G->object,ultimateSubjectMember,0))) match = false;	 // object is not a member of designated set
			if (ultimateVerbMember && !(ConceptPropogateTest(G->object,ultimateVerbMember,0))) match = false;	 // object is not a member of designated set
			if (ultimateObjectMember && !(ConceptPropogateTest(G->object,ultimateObjectMember,0))) match = false;	 // object is not a member of designated set
			if (systemFlags && match && !noSystemFlag)
			{
				if (!(OTHER->systemFlags & systemFlags)) match = false;	// dont go here
			}
			else if (systemFlags && match && noSystemFlag)
			{
				if (OTHER->systemFlags & systemFlags) match = false;	// dont go here
			}

			//   if search is riccochet, we now walk facts of riccochet target
			if (match && baseFlags & RICCOCHET_BITS )
			{
				if (!Riccochet(baseFlags,G,whichset,count,rmarks,rmarkv,rmarko))
				{
					scanIndex = queueIndex; //   end outer loop 
					F = NULL;
				}
			} // end riccochet
			else if (match && !(G->flags & MARKED_FACT) && !(baseFlags & (FINDTOPIC | FINDCONCEPT))) //   find unique fact -- it was not rejected by anything
			{
				G->flags |= MARKED_FACT;
				AddFact(whichset,G);
				if (trace & TRACE_QUERY && CheckTopicTrace() ) 
				{
					Log(STDTRACELOG,(char*)"    Found:");
					TraceFact(G);
				}
				if (FACTSET_COUNT(whichset) >= count) 
				{
					scanIndex = queueIndex; //   end outer loop 
					F = NULL;
					break;
				}
				if (count == 1 && intersectMark) // create backtract in next set
				{
					int set1 = whichset + 1;
					if (set1 > MAX_FIND_SETS) set1 = 0;	// wrap around end
					WORDP D = Meaning2Word(INCOMING);
					unsigned int count = 0;
					while (D)
					{
						factSet[set1][++count] = CreateFact(INCOMING,MakeMeaning(D),OUTGOING,FACTTRANSIENT);
						D = Meaning2Word(GetFactBack(D));
						OUTGOING = INCOMING;
						INCOMING = MakeMeaning(D);
					}
					SET_FACTSET_COUNT(set1,count);
				}
			}

			//   if propogation is enabled, queue appropriate choices
			if (match && baseFlags & FINDTOPIC && OTHER->internalBits & TOPIC) // supposed to find a topic
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) 
				{
					Log(STDTRACELOG,(char*)"\r\n propogate ");
					Log(STDTRACETABLOG,(char*)"");
				}
				AddFact(whichset,G); 
				if (trace & TRACE_QUERY  && CheckTopicTrace()) 
				{
					Log(STDTRACELOG,(char*)"    Found:");
					TraceFact(G);
				}
				if (FACTSET_COUNT(whichset) >= count) 
				{
					scanIndex = queueIndex; //   end outer loop 
					F = NULL;
					break;
				}
				if (G->flags & MARKED_FACT) {;} // stored this fact, stop propogation
				else if (AddWord2Scan(QUEUE,OUTGOING,INCOMING,0,0)) SetFactBack(OTHER,INCOMING);  // add object onto queue and provide traceback
				G->flags |= MARKED_FACT;
			}
			else if (match && baseFlags & FINDCONCEPT && OTHER->internalBits & CONCEPT && !(OTHER->internalBits & TOPIC)) // supposed to find a concept
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) 
				{
					Log(STDTRACELOG,(char*)"\r\n propogate ");
					Log(STDTRACETABLOG,(char*)"");
				}
				AddFact(whichset,G); 
				if (trace & TRACE_QUERY  && CheckTopicTrace()) 
				{
					Log(STDTRACELOG,(char*)"    Found:");
					TraceFact(G);
				}
				if (FACTSET_COUNT(whichset) >= count) 
				{
					scanIndex = queueIndex; //   end outer loop 
					F = NULL;
					break;
				}
				if (G->flags & MARKED_FACT) {;} // stored this fact, stop propogation
				else if (AddWord2Scan(QUEUE,OUTGOING,INCOMING,0,0)) SetFactBack(OTHER,INCOMING);  // add object onto queue and provide traceback
				G->flags |= MARKED_FACT;
			}
			else if (!match && propogateVerb && V->inferMark == propogateVerb) // this is not a fact to check, this is a fact to propogate on
			{
				if (trace & TRACE_QUERY  && CheckTopicTrace()) 
				{
					Log(STDTRACELOG,(char*)"\r\n propogate ");
					Log(STDTRACETABLOG,(char*)"");
				}
				if (G->flags & MARKED_FACT) {;} // stored this fact, stop propogation
				else if (AddWord2Scan(QUEUE,OUTGOING,INCOMING,0,0)) SetFactBack(OTHER,INCOMING);  // add object onto queue and provide traceback
				if (trace & TRACE_QUERY  && CheckTopicTrace()) 
				{
					Log(STDTRACELOG,(char*)"\r\n");
					Log(STDTRACETABLOG,(char*)"");
				}
			}
			if (baseOffset == 3) break; // resume with next fact in q rather than any chaining
		} // end loop on facts
	} // end loop on scan queue
	
	//   clear marks for duplicates
	unsigned int counter = FACTSET_COUNT(whichset);
	for (unsigned int i = 1; i <= counter; ++i) factSet[whichset][i]->flags &= -1 ^ MARKED_FACT;
	factSetNext[store] = 0;
    if (trace & TRACE_QUERY && CheckTopicTrace()) 
	{
		char word[MAX_WORD_SIZE];
		if (counter) Log(STDTRACETABLOG,(char*)" result: @%d[%d] e.g. %s\r\n",whichset,counter,WriteFact(factSet[whichset][1],false,word));
		else Log(STDTRACETABLOG,(char*)" result: @%d none found \r\n",whichset);
		Log(STDTRACETABLOG,(char*)"");
	}
	ClearBacktracks();

	if (*control++ == '|') goto nextsearch; //   chained search, do the next
	return counter;
}
