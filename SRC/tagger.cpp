 // tagger.cpp - used for pos tagging

#include "common.h"

unsigned int lowercaseWords;
unsigned int knownWords;
unsigned int tagRuleCount = 0;
uint64* tags = NULL;
char** comments = NULL;
static char* Describe(int i,char* buffer);

WORDP wordTag[MAX_SENTENCE_LENGTH]; 
WORDP wordRole[MAX_SENTENCE_LENGTH];
char* wordCanonical[MAX_SENTENCE_LENGTH]; //   chosen canonical form
WORDP originalLower[MAX_SENTENCE_LENGTH]; // transient during marking
WORDP originalUpper[MAX_SENTENCE_LENGTH]; // transient during marking
WORDP canonicalLower[MAX_SENTENCE_LENGTH]; // transient during marking
WORDP canonicalUpper[MAX_SENTENCE_LENGTH]; // transient during marking
uint64 finalPosValues[MAX_SENTENCE_LENGTH]; // needed during execution
uint64 allOriginalWordBits[MAX_SENTENCE_LENGTH];	// starting pos tags in this word position -- should merge some to finalpos
uint64 lcSysFlags[MAX_SENTENCE_LENGTH];      // transient current system tags lowercase in this word position (there are no interesting uppercase flags)
uint64 posValues[MAX_SENTENCE_LENGTH];			// current pos tags in this word position
uint64 canSysFlags[MAX_SENTENCE_LENGTH];		// canonical sys flags lowercase in this word position 
unsigned int parseFlags[MAX_SENTENCE_LENGTH];
static unsigned char wasDescribed[256];
static unsigned char describeVerbal[100];
static unsigned char describePhrase[100];
static unsigned char describeClause[100];
static int describedVerbals;
static int describedPhrases;
static int describedClauses;

// dynamic cumulative data across assignroles calls
int phrases[MAX_SENTENCE_LENGTH];
int clauses[MAX_SENTENCE_LENGTH];
int verbals[MAX_SENTENCE_LENGTH];
unsigned char ignoreWord[MAX_SENTENCE_LENGTH];
unsigned char coordinates[MAX_SENTENCE_LENGTH]; // for conjunctions
unsigned char crossReference[MAX_SENTENCE_LENGTH]; // object back to spawner,  particle back to verb
unsigned char phrasalVerb[MAX_SENTENCE_LENGTH]; // linking verbs and particles (potential)
uint64 roles[MAX_SENTENCE_LENGTH];
unsigned char tried[MAX_SENTENCE_LENGTH];

unsigned char objectRef[MAX_SENTENCE_LENGTH] ;  // link from verb to any main object ( allow use of 0 and end for holding)
unsigned char indirectObjectRef[MAX_SENTENCE_LENGTH];  // link from verb to any indirect object
unsigned char complementRef[MAX_SENTENCE_LENGTH ];  // link from verb to any 2ndary complement
// also posValues

int itAssigned = 0;
int theyAssigned = 0;

char* GetNounPhrase(int i,const char* avoid)
{
	static char buffer[MAX_WORD_SIZE];
	*buffer = 0;
#ifndef DISCARDPARSER
	if (clauses[i-1] != clauses[i]) // noun is a clause
	{
		int clause = clauses[i];
		int at = i-1;
		while (clauses[++at] & clause)
		{
			char* word = wordStarts[at];							
			if (tokenFlags & USERINPUT) strcat(buffer,word);		
			else if (!stricmp(word,(char*)"my")) strcat(buffer,(char*)"your");	
			else if (!stricmp(word,(char*)"your")) strcat(buffer,(char*)"my");	
			else strcat(buffer,word);
			strcat(buffer,(char*)" ");
		}
		size_t len = strlen(buffer);
		buffer[len-1] = 0;
		return buffer;
	}

	if (posValues[i+1] & (NOUN_INFINITIVE|NOUN_GERUND)) // noun is a verbal 
	{
		int verbal = verbals[i];
		int at = i-1;
		while (verbals[++at] & verbal)
		{
			char* word = wordStarts[at];							
			if (tokenFlags & USERINPUT) strcat(buffer,word);		
			else if (!stricmp(word,(char*)"my")) strcat(buffer,(char*)"your");	
			else if (!stricmp(word,(char*)"your")) strcat(buffer,(char*)"my");	
			else strcat(buffer,word);
			strcat(buffer,(char*)" ");
		}
		size_t len = strlen(buffer);
		buffer[len-1] = 0;
		return buffer;
	}

	if (clauses[i-1] != clauses[i]) return wordStarts[i]; // cannot cross clause boundary
	if (verbals[i-1] != verbals[i]) return wordStarts[i]; // cannot cross verbal boundary
	if (phrases[i-1] != phrases[i]) return wordStarts[i]; // cannot cross phrase boundary

	int start = (int) i; // on the noun
	// NOTE posvalues still has adjectivenoun as adjective.  Finalposvalues has it as a noun.
	while (--start > 0 && posValues[start] & (NOUN_BITS | COMMA | CONJUNCTION_COORDINATE | ADJECTIVE_BITS | DETERMINER | PREDETERMINER | ADVERB | POSSESSIVE | PRONOUN_POSSESSIVE)) 
	{
		if (roles[start] & (MAININDIRECTOBJECT|INDIRECTOBJECT2)) break; // cannot switch to this
		if (posValues[start] & TO_INFINITIVE) break;
		if (posValues[start] & COMMA && !(posValues[start-1] & ADJECTIVE_BITS)) break; // NOT like:  the big, red, tall human
		if (posValues[start] & CONJUNCTION_COORDINATE)
		{
			if ( canonicalLower[start] && strcmp(canonicalLower[start]->word,(char*)"and") && strcmp(canonicalLower[start]->word, (char*)"&")) break;	// not "and"
			if (!(posValues[start-1] & (ADJECTIVE_BITS|COMMA))) break;	// NOT like:  the big, red, and very tall human
			if (posValues[start-1] & COMMA && !(posValues[start-2] & ADJECTIVE_BITS)) break;	// NOT like:  the big, red, and very tall human
		}
		if (posValues[start] & NOUN_GERUND) break; 
		if (posValues[start] & ADVERB && !(posValues[start+1] & ADJECTIVE_BITS)) break;

		WORDP canon = canonicalLower[start];
		WORDP orig = originalLower[start];
		if (orig && (!strcmp((char*)"here",orig->word) || !strcmp((char*)"there",orig->word))) break;
		//if (orig && (!strcmp((char*)"this",orig->word) || !strcmp((char*)"that",orig->word) || !strcmp((char*)"these",orig->word) || !strcmp((char*)"those",orig->word))) break;
		if (canon && canon->properties & PRONOUN_BITS && !strcmp(canon->word,avoid)) break; // avoid recursive pronoun expansions... like "their teeth"
		if (posValues[start] & NOUN_PROPER_SINGULAR) break; // proper singular blocks appostive 
	}
	
	// start is NOT a member
	while (++start <= i)
	{
		char* word = wordStarts[start];							
		if (tokenFlags & USERINPUT) strcat(buffer,word);		
		else if (!stricmp(word,(char*)"my")) strcat(buffer,(char*)"your");	
		else if (!stricmp(word,(char*)"your")) strcat(buffer,(char*)"my");	
		else strcat(buffer,word);
		if (start != i) strcat(buffer,(char*)" ");
	}
#endif
	return buffer;
}

static char* DescribeComponent(int i,char* buffer,char* open, char* close) // verbal or phrase or clause
{
	strcat(buffer,open);
	Describe(i,buffer);
	strcat(buffer,close);
	return buffer;
}

static char* Describe(int i,char* buffer)
{
	if (wasDescribed[i]) 
		return buffer; // protect against infinite xref loops
	wasDescribed[i] = 1;
	// before
	int currentPhrase = phrases[i] & (-1 ^ phrases[i-1]); // only the new bit
	if (!currentPhrase) currentPhrase = phrases[i];
	int currentVerbal = verbals[i] & (-1 ^ verbals[i-1]); // only the new bit
	if (!currentVerbal) currentVerbal = verbals[i];
	int currentClause = clauses[i] & (-1 ^ clauses[i-1]); // only the new bit
	if (!currentClause) currentClause = clauses[i];
	bool found = false;
	char word[MAX_WORD_SIZE];
	for (int j = 1; j < i; ++j) // find things before
	{
		if (ignoreWord[j]) continue;
		if (crossReference[j] == i && posValues[j] & IDIOM)
		{
			strcat(buffer,wordStarts[j]);
			strcat(buffer,(char*)"_");
		}
		else if (crossReference[j] == i && phrases[j] ^ currentPhrase)
		{
			if (!found) strcat(buffer,(char*)" [");
			else  strcat(buffer,(char*)" ");
			found = true;
			++describedPhrases;
			describePhrase[describedPhrases] = (unsigned char)j;
			sprintf(word,(char*)"p%d",describedPhrases);
			strcat(buffer,word);
			strcat(buffer,(char*)" ");
		}
		else if (crossReference[j] == i && verbals[j] ^ currentVerbal)
		{
			if (!found) strcat(buffer,(char*)" [");
			else  strcat(buffer,(char*)" ");
			found = true;
			++describedVerbals;
			describeVerbal[describedVerbals] =  (unsigned char)j;
			sprintf(word,(char*)"v%d",describedVerbals);
			strcat(buffer,word);
			strcat(buffer,(char*)" ");
		}
		else if (crossReference[j] == i && clauses[j] ^ currentClause)
		{
			if (!found) strcat(buffer,(char*)" [");
			else  strcat(buffer,(char*)" ");
			found = true;
			++describedClauses;
			describeClause[describedClauses] =  (unsigned char)j;
			sprintf(word,(char*)"c%d",describedClauses);
			strcat(buffer,word);
			strcat(buffer,(char*)" ");
		}
		else if (crossReference[j] == i && !(roles[j] & (MAINSUBJECT|MAINOBJECT|MAININDIRECTOBJECT)))
		{
			if (roles[j] & OBJECT_COMPLEMENT && posValues[j] & NOUN_BITS && !phrases[j] && !clauses[j] && !verbals[j]) continue;
			if (!found) strcat(buffer,(char*)" [");
			else  strcat(buffer,(char*)" ");
			found = true;
			if (posValues[j] != TO_INFINITIVE) Describe(j,buffer);
			else strcat(buffer,(char*)"to");
		}
	}
	if (found) 
	{
		char* end = buffer + strlen(buffer) - 1;
		if (*end == ' ') *end = 0;
		strcat(buffer,(char*)"]");
	}
	found = false;
	if (!(posValues[i-1] & IDIOM)) strcat(buffer,(char*)" ");

	// the word
	strcat(buffer,wordStarts[i]);
	if (*wordStarts[i] == '"') strcat(buffer,(char*)"...\""); // show omitted quotation

	// after
	for (int j = i+1; j <= wordCount; ++j) // find things after
	{
		if (ignoreWord[j]) continue;
		if (crossReference[j] == i && posValues[j] & PARTICLE)
		{
			strcat(buffer,(char*)"_");
			strcat(buffer,wordStarts[j]);
		}
		else if (crossReference[j] == i && phrases[j] ^ currentPhrase)
		{
			if (!found) strcat(buffer,(char*)" [");
			else  strcat(buffer,(char*)" ");
			found = true;
			++describedPhrases;
			describePhrase[describedPhrases] =  (unsigned char)j;
			sprintf(word,(char*)"p%d",describedPhrases);
			strcat(buffer,word);
			strcat(buffer,(char*)" ");
		}
		else if (crossReference[j] == i && verbals[j] ^ currentVerbal)
		{
			if (!found) strcat(buffer,(char*)" [");
			else  strcat(buffer,(char*)" ");
			found = true;
			++describedVerbals;
			describeVerbal[describedVerbals] =  (unsigned char)j;
			sprintf(word,(char*)"v%d",describedVerbals);
			strcat(buffer,word);
			strcat(buffer,(char*)" ");
		}
		else if (crossReference[j] == i && clauses[j] ^ currentClause)
		{
			if (!found) strcat(buffer,(char*)" [");
			else  strcat(buffer,(char*)" ");
			found = true;
			++describedClauses;
			describeClause[describedClauses] =  (unsigned char)j;
			sprintf(word,(char*)"c%d",describedClauses);
			strcat(buffer,word);
			strcat(buffer,(char*)" ");
		}
		else if (currentPhrase && phrases[j] == currentPhrase && roles[j] & OBJECT2 && crossReference[j] == i)
		{
			strcat(buffer,(char*)" ");
			Describe(j,buffer);
		}
		else if (posValues[i] & TO_INFINITIVE && posValues[j] & (NOUN_INFINITIVE|VERB_INFINITIVE))
		{
			strcat(buffer,(char*)" ");
			Describe(j,buffer);
		}
		else if (crossReference[j] == i && !(roles[j] & (MAINSUBJECT|MAINOBJECT|MAININDIRECTOBJECT)))
		{
			if (roles[j] & OBJECT_COMPLEMENT && posValues[j] & NOUN_BITS && !phrases[j] && !clauses[j] && !verbals[j]) continue;
			if (!found && !(posValues[i] & PREPOSITION)) strcat(buffer,(char*)" [");
			found = true;
			Describe(j,buffer);
			strcat(buffer,(char*)" ");
		}
	}
	if (found) 
	{
		char* end = buffer + strlen(buffer) - 1;
		if (*end == ' ') *end = 0;
		strcat(buffer,(char*)"] ");
	}

	if (coordinates[i] > i) // conjoined
	{
		strcat(buffer,(char*)" + " );
		Describe(coordinates[i],buffer);
	}

	return buffer;
}

static void DescribeUnit( int i, char* buffer, char* msg,int verbal, int clause)
{
	char word[MAX_WORD_SIZE];
	if (i) // adjective object or causal infinitive
	{
		strcat(buffer,msg);
		if (verbals[i] != verbal)  
		{
			++describedVerbals;
			describeVerbal[describedVerbals] =  (unsigned char)i;
			sprintf(word,(char*)"v%d",describedVerbals);
			strcat(buffer,word);
		}
		else if (clauses[i] != clause)
		{
			++describedClauses;
			describeClause[describedClauses] =  (unsigned char)i;
			sprintf(word,(char*)"c%d",describedClauses);
			strcat(buffer,word);
		}
		else Describe(i,buffer);
		strcat(buffer,(char*)" ");
	}
}

void DumpSentence(int start,int end)
{
#ifndef DISCARDPARSER
	int to = end;
	int subject = 0, verb = 0, indirectobject = 0, object = 0,complement = 0;
	int i;
	bool notFound = false;
	char word[MAX_WORD_SIZE];
	describedVerbals = 0;
	describedPhrases = 0;
	describedClauses = 0;
	memset(wasDescribed,0,sizeof(wasDescribed));

	for ( i = start; i <= to; ++i) // main sentence
	{
		if (ignoreWord[i] && *wordStarts[i] != '"') continue;
		if (roles[i] & MAINSUBJECT && !subject) subject = i;
		if (roles[i] & MAINVERB && !verb) verb = i;
		if (roles[i] & OBJECT_COMPLEMENT && posValues[i] & NOUN_BITS && !complement) complement = i;
		if (roles[i] & OBJECT_COMPLEMENT && posValues[i] & (ADJECTIVE_BITS|VERB_INFINITIVE|NOUN_INFINITIVE) && !complement) complement = i;
		if (roles[i] & SUBJECT_COMPLEMENT && !complement) complement = i;
		if (!stricmp(wordStarts[i],(char*)"not")) notFound = true;
		if (roles[i] & SENTENCE_END) 
		{
			to = i;
			break;
		}
		if ((roles[i] & CONJUNCT_KINDS) == CONJUNCT_SENTENCE)
		{
			to = i;
			break;
		}
	}
	char* buffer = AllocateStack(NULL,MAX_BUFFER_SIZE); // local display
	strcat(buffer,(char*)"  MainSentence: ");

	for (i = start; i <= to; ++i)
	{
		if (roles[i] & ADDRESS)
		{
			Describe(i,buffer);
			strcat(buffer,(char*)" :  ");
		}
	}
	
	if (subject) DescribeUnit(subject,buffer, "Subj:",0,0);
	else if (tokenFlags& IMPLIED_YOU) 	strcat(buffer,(char*)"YOU   ");
	
	if (verb) 
	{
		object = objectRef[verb];
		indirectobject = indirectObjectRef[verb];
		strcat(buffer,(char*)"  Verb:");
		if (notFound) strcat(buffer,(char*)"(NOT!) ");
		Describe(verb,buffer);
		strcat(buffer,(char*)"   ");
	}

	if (indirectobject) 
	{
		strcat(buffer,(char*)"  IndirectObject:");
		Describe(indirectobject,buffer);
		strcat(buffer,(char*)"   ");
	}

	DescribeUnit(object,buffer, "  Object:",0,0);
	DescribeUnit(complement,buffer, "Complement:",0,0);

	if (clauses[start]){;}
	else if (!stricmp(wordStarts[start],(char*)"when")) strcat(buffer,(char*)"(:when) ");
	else if (!stricmp(wordStarts[start],(char*)"where")) strcat(buffer,(char*)"(:where) ");
	else if (!stricmp(wordStarts[start],(char*)"why")) strcat(buffer,(char*)"(:why) ");
	else if (!stricmp(wordStarts[start],(char*)"who") && subject != 1 && object != 1) strcat(buffer,(char*)"(:who) ");
	else if (!stricmp(wordStarts[start],(char*)"what") && subject != 1 && object != 1) strcat(buffer,(char*)"(:what) ");
	else if (!stricmp(wordStarts[start],(char*)"how")) strcat(buffer,(char*)"(:how) ");

	if (tokenFlags & QUESTIONMARK)  strcat(buffer,(char*)"? ");

	if (tokenFlags & PAST) strcat(buffer,(char*)" PAST ");
	else if (tokenFlags & FUTURE) strcat(buffer,(char*)" FUTURE ");
	else if (tokenFlags & PRESENT) strcat(buffer,(char*)" PRESENT ");

	if (tokenFlags & PERFECT) strcat(buffer,(char*)"PERFECT ");
	if (tokenFlags & CONTINUOUS) strcat(buffer,(char*)"CONTINUOUS ");

	if (tokenFlags & PASSIVE) strcat(buffer,(char*)" PASSIVE ");
	strcat(buffer,(char*)"\n");
	for (int i = 1; i <= describedPhrases; ++i)
	{
		sprintf(word,(char*)"Phrase %d :",i);
		strcat(buffer,word);
		Describe(describePhrase[i],buffer);
		strcat(buffer,(char*)"\n");
	}
	for (int i = 1; i <= describedVerbals; ++i)
	{
		sprintf(word,(char*)"Verbal %d: (",i);
		strcat(buffer,word);
		int verbal = describeVerbal[i]; 
		int verbalid = verbals[verbal] & (-1 ^ verbals[verbal-1]);
		if (!verbalid) verbalid = verbals[verbal];
		for (int j = i; j <= endSentence; ++j)
		{
			if (!(verbals[j] & verbalid)) continue;
			if (roles[j] & VERB2) 
			{
				strcat(buffer,(char*)"  Verb:");
				Describe(j,buffer); // the verb
				if (indirectObjectRef[j]) 
				{
					strcat(buffer,(char*)"  Indirect: ");
					Describe(indirectObjectRef[j],buffer);
				}
				DescribeUnit(objectRef[j],buffer, "  Direct:",verbalid,0);
				DescribeUnit(complementRef[j],buffer, " Complement:",verbalid,0);
				break;
			}
		}
		strcat(buffer,(char*)"\n");
	}
	for (int i = 1; i <= describedClauses; ++i)
	{
		sprintf(word,(char*)"Clause %d %s : ",i,wordStarts[describeClause[i]]);
		strcat(buffer,word);
		int clause = describeClause[i];
		int clauseid = clauses[clause] & (-1 ^ clauses[clause-1]);
		if (!clauseid) clauseid = clauses[clause];
		for (int j = i; j <= endSentence; ++j)
		{
			if (!(clauses[j] & clauseid)) continue;
			if (roles[j] & SUBJECT2)  
			{
				strcat(buffer,(char*)"  Subject:");
				Describe(j,buffer); // the subject
				strcat(buffer,(char*)"  ");
			}
			if (roles[j] & VERB2)  
			{
				strcat(buffer,(char*)"  Verb:");
				Describe(j,buffer); // the verb
				if (indirectObjectRef[j]) 
				{
					strcat(buffer,(char*)"  IndirectObject:");
					Describe(indirectObjectRef[j],buffer);
				}
				DescribeUnit(objectRef[j],buffer, "  DirectObject:",0,clauseid);
				DescribeUnit(complementRef[j],buffer, "  Complement:",0,clauseid);
			}
		}
		strcat(buffer,(char*)"\n");
	}

	for (int i = start; i <= to; ++i) // show phrases
	{
		if (ignoreWord[i]) continue;
		if (i >= to) continue; // ignore
		if (coordinates[i] && posValues[i] & CONJUNCTION_COORDINATE)
		{
			strcat(buffer,(char*)"\r\n Coordinate ");
			uint64 crole = roles[i] & CONJUNCT_KINDS;
			if (crole == CONJUNCT_NOUN) strcat(buffer,(char*)"Noun: ");
			else if (crole == CONJUNCT_VERB) strcat(buffer,(char*)"Verb: ");
			else if (crole == CONJUNCT_ADJECTIVE) strcat(buffer,(char*)"Adjective: ");
			else if (crole == CONJUNCT_ADVERB) strcat(buffer,(char*)"Adverb: ");
			else if (crole == CONJUNCT_PHRASE) strcat(buffer,(char*)"Phrase: ");
			else strcat(buffer,(char*)"Sentence: ");
			strcat(buffer,wordStarts[i]);
			strcat(buffer,(char*)" (");
			if (coordinates[i] && coordinates[coordinates[i]])
			{
				strcat(buffer,wordStarts[coordinates[coordinates[i]]]); // the before
				strcat(buffer,(char*)"/");
				strcat(buffer,wordStarts[coordinates[i]]); // the after
			}
			strcat(buffer,(char*)") ");
		}
	}

	for (int i = start; i <= to; ++i) // show phrases
	{
		if (ignoreWord[i]) continue;
		if ( phrases[i] && phrases[i] != phrases[i-1] && (i != wordCount || phrases[wordCount] != phrases[1])) 
		{
			if (posValues[i] & (NOUN_BITS|PRONOUN_BITS)) strcat(buffer,(char*)"\r\n  Absolute Phrase: ");
			//else if (posValues[i] & (ADJECTIVE_BITS|ADVERB)) strcat(buffer,(char*)"\r\n  Time Phrase: ");
			else continue;
			if (i == 1 && phrases[wordCount] == phrases[1]) 
			{
				DescribeComponent(wordCount,buffer,(char*)"{","}"); // wrapped?
				strcat(buffer,(char*)" ");
			}
			DescribeComponent(i,buffer,(char*)"{","}"); // wrapped?
			strcat(buffer,(char*)" ");
		}

		if (roles[i] & TAGQUESTION) strcat(buffer,(char*)" TAGQUESTION ");
	}

	Log(STDTRACELOG,(char*)"%s\r\n",buffer);

	ReleaseStack(buffer);
	if (to < end) DumpSentence(to+1,end); // show next piece
#endif
 }
 
char* roleSets[] = 
{
	(char*)"~mainsubject",(char*)"~mainverb",(char*)"~mainobject",(char*)"~mainindirectobject",
	(char*)"~whenunit",(char*)"~whereunit",(char*)"~howunit",(char*)"~whyunit",
	(char*)"~subject2",(char*)"~verb2",(char*)"~object2",(char*)"~indirectobject2",(char*)"~byobject2",(char*)"~ofobject2",
	(char*)"~appositive",(char*)"~adverbial",(char*)"~adjectival",
	(char*)"~subjectcomplement",(char*)"~objectcomplement",(char*)"~address",(char*)"~postnominaladjective",(char*)"~adjectivecomplement",(char*)"~omittedtimeprep",(char*)"~omittedofprep",
	(char*)"~comma_phrase",(char*)"~tagquestion",(char*)"~absolute_phrase",(char*)"~omitted_subject_verb",(char*)"~reflexive",(char*)"~DISTANCE_NOUN_MODIFY_ADVERB",(char*)"~DISTANCE_NOUN_MODIFY_ADJECTIVE",(char*)"~TIME_NOUN_MODIFY_ADVERB",(char*)"~TIME_NOUN_MODIFY_ADJECTIVE",
	(char*)"~conjunct_noun",(char*)"~conjunct_verb",(char*)"~conjunct_adjective",(char*)"~conjunct_adverb",(char*)"~conjunct_phrase",(char*)"~conjunct_clause",(char*)"~conjunct_sentence",
	NULL
};
	
char* GetRole(uint64 role)
{
	static char answer[MAX_WORD_SIZE];
	*answer = 0; 
#ifndef DISCARDPARSER
	if (role & APPOSITIVE) strcat(answer, "APPOSITIVE ");
	if (role & ADVERBIAL) strcat(answer, "ADVERBIAL ");
	if (role & ADJECTIVAL) strcat(answer, "ADJECTIVAL ");  // if it is not specified as adverbial or adjectival, it is used as a noun  -- prep after a noun will be adjectival, otherwize adverbial
	// THey claim a preposition phrase to act as a noun � "During a church service is not a good time to discuss picnic plans" or "In the South Pacific is where I long to be" 
	// but inverted it's - a good time to discuss picnic plans is not during a church service. 

	if ((role & ADVERBIALTYPE) == WHENUNIT) strcat(answer, "WHENUNIT ");	
	if ((role & ADVERBIALTYPE)  == WHEREUNIT) strcat(answer, "WHEREUNIT ");
	if ((role & ADVERBIALTYPE)   == HOWUNIT) strcat(answer, "HOWUNIT "); // how and in what manner
	if ((role & ADVERBIALTYPE)   == WHYUNIT) strcat(answer, "WHYUNIT ");

	if (role & MAINSUBJECT) strcat(answer,(char*)"MAINSUBJECT ");
	if (role & MAINVERB) strcat(answer, "MAINVERB ");
	if (role & MAINOBJECT) strcat(answer, "MAINOBJECT ");
	if (role & MAININDIRECTOBJECT) strcat(answer,  "MAININDIRECTOBJECT ");
	
	if (role & OBJECT2) strcat(answer,  "OBJECT2 ");
	if (role & BYOBJECT2) strcat(answer,  "BYOBJECT2 ");
	if (role & OFOBJECT2) strcat(answer,  "OFOBJECT2 ");
	if (role & SUBJECT2) strcat(answer,  "SUBJECT2 ");
	if (role & VERB2) strcat(answer,  "VERB2 ");
	if (role & INDIRECTOBJECT2) strcat(answer, "INDIRECTOBJECT2 ");
	if (role & OBJECT_COMPLEMENT) strcat(answer,   "OBJECT_COMPLEMENT ");
	if (role & SUBJECT_COMPLEMENT) strcat(answer,   "SUBJECT_COMPLEMENT ");
	
	unsigned int crole = role & CONJUNCT_KINDS;

	if (crole == CONJUNCT_NOUN) strcat(answer, "CONJUNCT_NOUN ");
	else if (crole == CONJUNCT_VERB) strcat(answer, "CONJUNCT_VERB ");
	else if (crole == CONJUNCT_ADJECTIVE) strcat(answer, "CONJUNCT_ADJECTIVE ");
	else if (crole == CONJUNCT_ADVERB) strcat(answer, "CONJUNCT_ADVERB ");
	else if (crole == CONJUNCT_PHRASE) strcat(answer, "CONJUNCT_PHRASE ");
	else if (crole== CONJUNCT_CLAUSE) strcat(answer, "CONJUNCT_CLAUSE ");
	else if (crole == CONJUNCT_SENTENCE) strcat(answer, "CONJUNCT_SENTENCE ");
	
	if (role & POSTNOMINAL_ADJECTIVE) strcat(answer, "POSTNOMINAL_ADJECTIVE ");
	if (role & ADJECTIVE_COMPLEMENT) strcat(answer, "ADJECTIVE_COMPLEMENT ");
	if (role & OMITTED_TIME_PREP) strcat(answer, "OMITTED_TIME_PREP ");
	if (role & DISTANCE_NOUN_MODIFY_ADVERB) strcat(answer, "DISTANCE_NOUN_MODIFY_ADVERB ");
	if (role & DISTANCE_NOUN_MODIFY_ADJECTIVE) strcat(answer, "DISTANCE_NOUN_MODIFY_ADJECTIVE ");
	if (role & TIME_NOUN_MODIFY_ADVERB) strcat(answer, "TIME_NOUN_MODIFY_ADVERB ");
	if (role & TIME_NOUN_MODIFY_ADJECTIVE) strcat(answer, "TIME_NOUN_MODIFY_ADJECTIVE ");
	if (role & OMITTED_OF_PREP) strcat(answer, "OMITTED_OF_PREP ");
	if (role & ADDRESS) strcat(answer, "ADDRESS ");
	if (role & COMMA_PHRASE) strcat(answer, "COMMA_PHRASE ");
	if (role & TAGQUESTION) strcat(answer, "TAGQUESTION ");
	if (role & ABSOLUTE_PHRASE) strcat(answer, "ABSOLUTE_PHRASE ");
	if (role & OMITTED_SUBJECT_VERB) strcat(answer, "OMITTED_SUBJECT_VERB ");

#endif
	return answer;
}
