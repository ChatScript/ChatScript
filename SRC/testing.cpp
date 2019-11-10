#include "common.h"

extern int ignoreRule;
static bool down_is = true;

static int downcount = 0;
static int volleyCounter = 0;
static int relevantVolleyCounter = 0;

bool VerifyAuthorization(FILE* in) //   is he allowed to use :commands
{
	if (overrideAuthorization) return true; // commanded from script

	char buffer[MAX_WORD_SIZE];
	if ( *authorizations == '1') // command line does not authorize
	{
		if (in) FClose(in);
		return false;
	}

	//  check command line params
	char* at = authorizations;
	char word[MAX_WORD_SIZE];
	if (at) // command line given
	{
		if (*at == '"') ++at;
		while (*at)
		{
			at = ReadCompiledWord(at,word);
			size_t len = strlen(word);
			if (word[len-1] == '"') word[len-1] = 0;
			if (!stricmp(word,(char*)"all") || !stricmp(word,callerIP) || (*word == 'L' && word[1] == '_' && !stricmp(word+2,loginID))) //   allowed by IP or L_loginname
			{
				if (in) FClose(in);
				return true;
			}
		}
	}

	if (!in) return (*authorizations) ? false : true;			//   no restriction file

	bool result = false;
	while (ReadALine(buffer,in) >= 0 )
    {
		ReadCompiledWord(buffer,word);
		if (!stricmp(word,(char*)"all") || !stricmp(word,callerIP) || (*word == 'L' && word[1] == '_' && !stricmp(word+2,loginID)) || (*word == 'P' && word[1] == '_' && strstr(loginID, word + 2))) //   allowed by IP or L_loginname or P_loginname
		{ 
			result = true;
			break;
		}
	}
	FClose(in);
	return result;
}


#ifndef DISCARDTESTING
static void C_Trace(char* input);
static void C_DoInternal(char* input,bool internal);
static int nooob = 0;
static int lineLimit = 0; // in abstract report lines that are longer than this...
static WORDP topLevel = 0;
static unsigned int err = 0;
static int inLine = 0;
static int inDepth = 0;
static unsigned int filesSeen; 
static 	char directory[MAX_WORD_SIZE];
static int itemcount = 0;
static char* abstractBuffer;
static int longLines;
static uint64 verifyToken;
static bool isTracing = false;
static bool isTiming = false;
static WORDP dictUsedG;
static FACT* factUsedG;
static char* textUsedG;
static int trials;
static int nextRule = 0;
static bool stepMatch = false;
static bool keepname = false;

static short int stepOver[MAX_GLOBAL];
static char stepOut[MAX_GLOBAL];
static void TrimIt(char* name, uint64 flag);

#define ABSTRACT_SPELL 1
#define ABSTRACT_SET_MEMBER 2
#define ABSTRACT_CANONICAL 4
#define ABSTRACT_PRETTY 8
#define ABSTRACT_VP 16
#define ABSTRACT_NOCODE 32
#define ABSTRACT_STORY 64
#define ABSTRACT_RESPONDER 128
#define ABSTRACT_RESTRICTIONS (ABSTRACT_SPELL|ABSTRACT_SET_MEMBER|ABSTRACT_CANONICAL|ABSTRACT_VP )
#define ABSTRACT_COVERAGE 256

static bool fromScript = false;

static void ShowTrace(unsigned int bits, bool original);
static void ShowTiming(unsigned int bits, bool original);

// prototypes
static bool DumpOne(WORDP S,int all,int depth,bool shown);
static int CountDown(MEANING T,int all,int depth,unsigned int baseStamp);
static void C_Retry(char* input);

static MEANING* meaningList; // list of meanings from :concepts
static MEANING* meaningLimit; // end of meaninglistp

#include <map>
using namespace std;
std::map <const char*, int> statistics; // statistics data

////////////////////////////////////////////////////////
/// UTILITY ROUTINES
////////////////////////////////////////////////////////

int CountSet(WORDP D,unsigned int baseStamp) //   full recursive referencing
{
	if (!D) return 0;
	if (D->inferMark == inferMark) return 0;
	D->inferMark = inferMark;
	int count = 0;
	FACT* F = GetObjectNondeadHead(D);
	FACT* G;
	while (F) //   do all atomic members of it
	{
		G = F;
		F = GetObjectNondeadNext(F);
		WORDP S = Meaning2Word(G->subject);
		if (!(G->verb == Mmember)  || G->flags & FACTDEAD) continue;
		if (*S->word == '~' ) continue;
		else if (Meaning2Index(G->subject)) count += CountDown(GetMaster(G->subject),-1,-2,baseStamp); //   word~2 reference is a synset header -- follow IS_A submembers
		else //   simple atomic member -- or POS specificiation
		{
			if (S->inferMark <= baseStamp) //   count once
			{
				S->inferMark = inferMark;
				++count;
			}
		}
	}
	F = GetObjectNondeadHead(D);
	while (F) //   do all set members of it
	{
		G = F;
		F = GetObjectNondeadNext(F);
		WORDP S = Meaning2Word(G->subject);
		if (!(G->verb == Mmember)  || G->flags & FACTDEAD) continue;
		if (*S->word == '~')  count += CountSet(S,baseStamp);
	}
	return count;
}

static int CountDown(MEANING T,int all,int depth,unsigned int baseStamp)
{ //  T is a synset header
	T &= -1 ^ SYNSET_MARKER;

	if (all == 5) return 0;
	int count = 0;

	//   show each word in synset
    WORDP D = Meaning2Word(T);
	WORDP baseWord = D;
	unsigned int index = Meaning2Index(T);
	unsigned int baseIndex = index;

	// walk the master list of synonyms at this level
	bool shown = false;
	while (ALWAYS) 
    {
		MEANING next = GetMeaning(D,index);
		if (D->inferMark != inferMark) 
		{
			if (D->inferMark <= baseStamp) ++count;
			D->inferMark = inferMark;	
			if (depth >= 0 ) shown |= DumpOne(D,all,depth,shown); //   display it
		}
		D = Meaning2Word(next);
		index = Meaning2Index(next);
		if (D == baseWord && index == baseIndex) break; // back at start of loop
    }

	//   down go down to next level synset from this one
	FACT* F = GetObjectNondeadHead(T); 
	while (F)
	{
		if (F->verb ==  Mis && F->object == T) count += CountDown(F->subject,all,(depth == -2) ? -2 : (depth+1),baseStamp);
		F = GetObjectNondeadNext(F);
	}
	return count;
}

static void Indent(int count,bool nonumber)
{
	if (!nonumber) Log(STDUSERLOG,(char*)"%d.",count);
	while (count--) Log(STDUSERLOG,(char*)"    ");
}

static bool DumpOne(WORDP S,int all,int depth,bool shown)
{
	bool did = false;
	if (all) 
	{
			if ( all == 3) return false;
			if (itemcount == 0 && all != 2) Indent(depth,shown);
			unsigned char* data = GetWhereInSentence(S);
			if (all == 1) 
			{
				if (!data)
				{
					data = (unsigned char*) AllocateWhereInSentence(S);
					if (!data) return false;
					*data = 0;
					data[1] = 0;
				}
				if (++data[1] == 0) ++data[0];
			}
			if (all == 1 && *data && (data[0] || data[1] > 1)) Log(STDUSERLOG,(char*)"+%s  ",S->word); //   multiple occurences
			else  //   first occurence of word
			{
				if (all == 1 && !(S->systemFlags & VERB_DIRECTOBJECT)) //   generate a list of intransitive verbs
				{
					FILE* out = FopenUTF8WriteAppend((char*)"intransitive.txt");
					fprintf(out,(char*)"%s 1\r\n",S->word);
					FClose(out);
				}
				if (all == 1 && (S->systemFlags & VERB_INDIRECTOBJECT)) //   generate a list of dual transitive verbs
				{
					FILE* out = FopenUTF8WriteAppend((char*)"intransitive.txt");
					fprintf(out,(char*)"%s 2\r\n",S->word);
					FClose(out);
				}
				Log(STDUSERLOG,(char*)"%s  ",S->word);
			}
			++itemcount;
			if (itemcount == 10 && all != 2)
			{
				Log(STDUSERLOG,(char*)"\r\n");
				itemcount = 0;
			}
			did = true;
	}
	return did;
}

static void MarkExclude(WORDP D)
{
	FACT* F = GetObjectNondeadHead(D);
	while (F)
	{
		if (F->verb == Mexclude) Meaning2Word(F->subject)->inferMark = inferMark;
		else if (F->verb == Mmember) break;
		F = GetObjectNondeadNext(F);
	}
}

/////////////////////////////////////////////
/// TESTING
/////////////////////////////////////////////

static void C_AutoReply(char* input)
{
	regression = 1;
	strcpy(oktest,input);
	if (!*oktest)  regression =  false;
	if (*oktest) Log(STDUSERLOG,(char*)"Auto input set to %s\r\n",oktest);
}  

static void MarkUp(WORDP D) // mark all that can be seen from here going up as member
{	
	if (D->inferMark == inferMark) return;
	D->inferMark = inferMark;
	FACT* F = GetSubjectNondeadHead(D);
	while (F)
	{
		if (ValidMemberFact(F))
		{
			WORDP E = Meaning2Word(F->object);
			MarkUp(E);
		}
  
		F = GetSubjectNondeadNext(F);
	}
}

static void C_Common(char* input)
{
	char word[MAX_WORD_SIZE];
	char word1[MAX_WORD_SIZE];
	WORDP D;
	char* ptr = ReadCompiledWord(input,word);
	ptr = ReadCompiledWord(ptr,word1);
	if (!*word1) 
	{
		Log(STDUSERLOG, "You need to supply at least 2 words.\r\n");
		return;
	}
	while (input)
	{
		input = ReadCompiledWord(input,word);
		D = FindWord(word);
		if (!D) 
		{
			Log(STDUSERLOG, "%s is an unknown word\r\n",word);
			return;
		}
		ReadCompiledWord(input,word1); // read ahead 1
		if (!*word1) break; //we are on the last word
		FACT* F = GetSubjectNondeadHead(D);
		NextInferMark();
		while (F)
		{
			TraceFact(F);
			if (F->verb == Mmember && (!myBot || !F->botBits || F->botBits & myBot))
				MarkUp(Meaning2Word(F->object)); // mark all on this path as seen
			F = GetSubjectNondeadNext(F);
		}
	}
	WORDP words[10000];
	WORDP found[10000];
	NextInferMark();
	unsigned int index = 0;
	unsigned int at = 0;
	unsigned int foundIndex = 0;
	words[index++] = D;	// goes onto stack
	words[index++] = 0; // end of a level
	int level = 0;
	FACT* F;
	while (at < index)
	{
		D = words[at++];	// next one from queue
		if (D == 0)
		{
			if (at != index) words[index++] = 0;
			++level;
			found[foundIndex++] = 0; // mark the level
			continue;
		}
		F = GetSubjectNondeadHead(D);
		while (F)
		{
			if (ValidMemberFact(F))
			{
				D = Meaning2Word(F->object);
				if (D->inferMark == (inferMark - 1)) found[foundIndex++] = D;
				if (D->inferMark < inferMark) 
				{
					D->inferMark = inferMark;
					words[index++] = D;
				}
			}
			F = GetSubjectNondeadNext(F);
		}	
	}
	Log(STDUSERLOG,(char*)"Concept intersection:\r\n");
	level = 1;
	bool header = false;
	for (unsigned int i = 0; i < foundIndex; ++i)
	{
		if (found[i] == 0) 
		{
			Log(STDUSERLOG,(char*)"\r\n");
			++level;
			header = false;
		}
		else 
		{
			if (!header) Log(STDUSERLOG,(char*)"%d. ",level);
			header = true;
			Log(STDUSERLOG,(char*)"%s ",found[i]->word);
		}
	}
	Log(STDUSERLOG,(char*)"\r\n");
}  

static void C_NoReact(char* input)
{
	noReact = !noReact;
	Log(STDUSERLOG,(char*)"Noreact = %d\r\n",noReact);
} 

static void C_POS(char* input)
{
    if (!*input) prepareMode = (prepareMode == POS_MODE) ? NO_MODE : POS_MODE;
    else
    {
        unsigned int oldtrace = trace;
        unsigned int oldtiming = timing;
        uint64 oldTokenControl = tokenControl;

        char word[MAX_WORD_SIZE];
        char* at = ReadCompiledWord(input, word);
        if (!stricmp(word, (char*)"PENN"))
        {
            input = at;
            tokenControl = STRICT_CASING | DO_ESSENTIALS | DO_PARSE | DO_CONTRACTIONS | NO_HYPHEN_END | NO_COLON_END | NO_SEMICOLON_END | TOKEN_AS_IS;
        }
        else
        {
            char* token = GetUserVariable((char*)"$cs_token");
            int64 f;
            ReadInt64(token, f);
            if (f == 0) f = DO_ESSENTIALS | DO_PARSE | DO_CONTRACTIONS | NO_HYPHEN_END | NO_COLON_END | NO_SEMICOLON_END | TOKEN_AS_IS;
            tokenControl = f;
        }
        trace = (unsigned int)-1;
        tmpPrepareMode = POS_MODE;
        quotationInProgress = 0;
        PrepareSentence(input, true, true);
        tmpPrepareMode = NO_MODE;
        tokenControl = oldTokenControl;
        trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
        timing = (modifiedTiming) ? modifiedTimingVal : oldtiming;
    }
}

static void C_Prepare(char* input)
{
    uint64 oldToken = tokenControl;
    input = SkipWhitespace(input);
    static bool prepass = true;
    char word[MAX_WORD_SIZE];
    if (*input == USERVAR_PREFIX) // set token control to this
    {
        char* ptr = ReadCompiledWord(input, word);
        char* value = GetUserVariable(word);
        if (value && *value)
        {
            input = ptr;
            int64 val64 = 0;
            ReadInt64(value, val64);
            tokenControl = val64;
        }
    }
    input = SkipWhitespace(input);
    if (!strnicmp(input, (char*)"NOPREPASS", 9) || !strnicmp(input, (char*)"PREPASS", 7))
    {
        prepass = strnicmp(input, (char*)"NOPREPASS", 9) ? true : false;
        input = ReadCompiledWord(input, word);
    }

    if (!*input) prepareMode = (prepareMode == PREPARE_MODE) ? NO_MODE : PREPARE_MODE;
    else
    {
        char prepassTopic[MAX_WORD_SIZE];
        strcpy(prepassTopic, GetUserVariable((char*)"$cs_prepass"));
        unsigned int oldtrace = trace;
        unsigned int oldtiming = timing;
        nextInput = input;
        bool oobstart = (*nextInput == '[');
        while (*nextInput)
        {
            prepareMode = PREPARE_MODE;
            if (*prepassTopic) Log(STDUSERLOG, (char*)"Prepass: %s\r\n", prepass ? (char*)"ON" : (char*)"OFF");
            PrepareSentence(nextInput, true, true, false, oobstart);
            oobstart = false;
            prepareMode = NO_MODE;
            if (prepass && PrepassSentence(prepassTopic)) continue;
        }
        trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
        timing = (modifiedTiming) ? modifiedTimingVal : oldtiming;
    }
    tokenControl = oldToken;
}

static void C_Spellit(char* input)
{
	echo = true;
	spellTrace = true;
	input = SkipWhitespace(input);
	nextInput = input;
	prepareMode = TOKENIZE_MODE;
	while (*nextInput) PrepareSentence(nextInput, true, true, false, false);
	prepareMode = NO_MODE;
	echo = false;
	spellTrace = false;
}

static void C_Tokenize(char* input)
{
    uint64 oldToken = tokenControl;
    input = SkipWhitespace(input);
    char word[MAX_WORD_SIZE];
    if (*input == USERVAR_PREFIX) // set token control to this
    {
        char* ptr = ReadCompiledWord(input, word);
        char* value = GetUserVariable(word);
        if (value && *value)
        {
            input = ptr;
            int64 val64 = 0;
            ReadInt64(value, val64);
            tokenControl = val64;
        }
    }
    input = SkipWhitespace(input);
    if (!*input) prepareMode = (prepareMode == TOKENIZE_MODE) ? NO_MODE : TOKENIZE_MODE;
    else
    {
        unsigned int oldtrace = trace;
        unsigned int oldtiming = timing;
        nextInput = input;
        bool oobstart = (*nextInput == '[');
        while (*nextInput)
        {
            prepareMode = TOKENIZE_MODE;
            PrepareSentence(nextInput, true, true, false, oobstart);
            oobstart = false;
            prepareMode = NO_MODE;
        }
        trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
        timing = (modifiedTiming) ? modifiedTimingVal : oldtiming;
    }
    tokenControl = oldToken;
}

static void MemorizeRegress(char* input)
{
	char word[MAX_WORD_SIZE];
	input = ReadCompiledWord(input,word);  // file to read
	char outputfile[MAX_WORD_SIZE];
	ReadCompiledWord(input,outputfile); // file to write
	FILE* in = FopenReadNormal(word); // source full name given
	char file[SMALL_WORD_SIZE];
	if (!in)
	{
		char* txt = strstr(word,(char*)".txt");
		if (txt) *txt = 0;
		sprintf(file,(char*)"%s/log-%s.txt",users,word); // presume only login given, go find full file
		in = FopenReadNormal(file); // source
	}
	if (!in) Log(STDUSERLOG,(char*)"Couldn't find %s\r\n",file);
	else  
	{
		FILE* out = NULL;
		if (*outputfile) out = FopenUTF8Write(outputfile);
		if (!out)
		{
			char fname[200];
			sprintf(fname, "%s/regress.txt", tmp);
			strcpy(outputfile,fname);
			out = FopenUTF8Write(outputfile);
			if (!out)
			{
				(*printer)((char*)"cannot open %s\r\n",outputfile);
				return;
			}
		}
		char* at;
		bool start = true;
		while (ReadALine(readBuffer,in)  >= 0) // read log file
		{
			if (!*readBuffer) continue;
			size_t len;
			// Start: user:fa bot:patient1a ip: rand:247 (~introductions) 0 ==> Hello Doctor  When:Mar23'14-20:06:23 Version:4.2 Build0: Build1:Mar23'14-20:04:11 0:Mar23'14-20:06:18 F:0 P:0 Why:~introductions.0.0.~control.6.0 
			// Respond: user:fa bot:patient1a ip: (~introductions) 1  fa ==> I'm afraid I don't understand that question.  When:Mar23'14-20:06:30 Why:~control.10.0=MAINCONTROL 
			// fields are: type, user, bot, ip, {rand}, (resulting topic), current volley id, user input, ==> bot output, when: {version/build1/build0/f:/p:) followed by why: rule tags for each issued output.
		
			char kind[MAX_WORD_SIZE];
			ReadCompiledWord(readBuffer,kind);

			if (strcmp(kind,(char*)"Respond:") && strcmp(kind,(char*)"Start:") ) continue; // abnormal line? like a ^log entry.

			// normal volley
			char user[MAX_WORD_SIZE];
			char* ptr = strstr(readBuffer,(char*)"user:") + 5;
			ptr = ReadCompiledWord(ptr,user);

			unsigned int volley;
			char* endtopic = strchr(ptr,')');
			volley = atoi(endtopic+2);

			int rand = -1;
			char* randptr = strstr(ptr,(char*)"rand:");
			if (randptr) rand = atoi(randptr+5);
	
			// confirm legit startup
			if (start == true)
			{
				if (volley || *kind != 'S') 
				{
					Log(STDUSERLOG,(char*)"Log file must begin with Start at turn 0, not turn %d\r\n",volley);
					FClose(in);
					return;
				}
				start = false;
			}

			char bot[MAX_WORD_SIZE];
			ptr = strstr(readBuffer,(char*)"bot:") + 4;
			ptr = ReadCompiledWord(ptr,bot); 

			char topic[MAX_WORD_SIZE];
			at = strchr(ptr,'(') + 1;
			*endtopic = 0;
			at = ReadCompiledWord(at,topic);	
		
			char junk[MAX_WORD_SIZE];
			at = endtopic + 2;
			at = ReadCompiledWord(at,junk) - 1; // point at blank
			char* input = at; // now points to user input start

			char* output = strstr(at,(char*)" ==> ");
			if (!output) continue;
			*output = 0;	// end of input
			input = SkipWhitespace(input);
			output += 5;
			output = SkipWhitespace(output);  // start of output

			char* why = strstr(output,(char*)"Why:");
			char* end = strstr(output,(char*)" When:");
			if (end) *end = 0; // end of output
			if (rand != -1) fprintf(out,(char*)"%s user:%s bot:%s rand:%d (%s) %d ==> %s\r\n", kind, user, bot,rand,topic,volley,output);
			else fprintf(out,(char*)"%s user:%s bot:%s (%s) %d %s ==> %s\r\n", kind, user, bot,topic,volley,input,output);

			if (!why) continue;
			
			why += 4;
			end = strchr(why+1,'~'); // 2nd rule via if there is one
			if (end) *end = 0;
			int topicid;
			int id;
			char* verify = GetVerify(why,topicid,id);
            if (!verify) continue;
			char* rule = GetRule(topicid,id);		// the rule we want to test
            if (!rule) continue;
            char label[MAX_WORD_SIZE];
			char pattern[MAX_WORD_SIZE];
			char outputdata[MAX_WORD_SIZE];
			char* output1 = GetPattern(rule,label,pattern);
			len = strlen(output1);
			if (len > 50) len = 50;
			strncpy(outputdata,output1,len);
			while (outputdata[len-1] == ' ') --len;
			outputdata[len] = 0;
			pattern[50] = 0;	 // limit it
			fprintf(out,(char*)"  verify: %s rule:%s kind:%c label:%s pattern: %s output: %s \r\n",verify, why,*rule,label,pattern,outputdata);

			if (!end)
			{
				fprintf(out,(char*)"%s",(char*)"  verify2: \r\n");
				continue;
			}

			*end = '~';
			rule = GetRule(topicid,id);		// the rule we want to test
			output1 = GetPattern(rule,label,pattern);
			len = strlen(output1);
			if (len > 50) len = 50;
			strncpy(outputdata,output1,len);
			outputdata[len] = 0;
			pattern[50] = 0;	 // limit it
			verify = GetVerify(end,topicid,id);
			fprintf(out,(char*)"  verify2: %s rule:%s kind:%c label:%s pattern: %s output: %s \r\n",verify, end,*rule,label,pattern,outputdata);
		}
		FClose(out);
		FClose(in);
		Log(STDUSERLOG,(char*)"Regression file %s created\r\n",outputfile);
	}
}

static void VerifyRegress(char* file)
{
	char word[MAX_WORD_SIZE];
	char* at = ReadCompiledWord(file,word);
	bool silent = false;
    bool exitrequest = false;
    if (!stricmp(word, (char*)"exit") || !stricmp(word, (char*)"batch"))
    {
        file = at;
        exitrequest = true;
        at = ReadCompiledWord(file, word);
    }
    if (!stricmp(word,(char*)"terse"))
	{
		file = at;
		silent = true;
        at = ReadCompiledWord(file, word);
	}
    if (!stricmp(word, (char*)"exit"))
    {
        file = at;
        exitrequest = true;
    }
    FILE* in = FopenReadNormal(file); // source
	if (!in)
	{
		(*printer)((char*)"No regression data found for %s\r\n",file);
		return;
	}
	sprintf(logFilename,(char*)"%s/tmpregresslog.txt",users); // user log goes here so we can regenerate a new regression file if we need one
	FILE* out = FopenUTF8Write(logFilename); // make a new log file to do this in.
	FClose(out);

	int olduserlog = userLog;
	userLog = LOGGING_SET;
	unsigned int changed = 0;
	bool modified = false;
	bool oldecho = echo;
	echo = false;
	unsigned int count = 0;
	prepareMode = REGRESS_MODE;
	size_t len;
	unsigned int minorchange = 0;
	char* verifyinfo = AllocateBuffer();
	unsigned int volley = 0;
	char* myBuffer = AllocateBuffer();
	regression = REGRESS_REGRESSION;
	char* holdmain = mainOutputBuffer;
	while (ReadALine(myBuffer,in) >= 0 ) // read regression file
	{
		//Start: user:fd bot:rose rand:553 volley:0 topic:~hello input:  output: Good morning.  
		//	verify:  rule:~hello.1.0. kind:t label: pattern: ( !$olduser =8%input=0 =7%hour<12 )  output: $olduser = 1 Good morning. $begintime = %fulltime  
		//	verify2:  rule:~submain_control.5.0  kind:u label: pattern: ( =8%input<%userfirstline )  output: $repeatstart = %userfirstline + 10 ^gambit ( ~hell 
		//Respond: user:fd bot:rose volley:1 topic:~hellold input: what is your name output: My name is Rose. What's yours?  
		//	verify:  rule:~hello.4.0=MYNAME. kind:t label:MYNAME pattern: ( )  output: My name is Rose. $begintime = %fulltime ^^if ( ! $ 
		//	verify2: what is your name?  rule:~physical_self.101.0=TELLNAME  kind:u label:TELLNAME pattern: ( ![ him my her them ] << what your [ name moniker output: ^reuse ( ~hello.myname ) `01b u: ( !my what * you  
		if (!*myBuffer) continue;
		char* ptr = SkipWhitespace(myBuffer);
		if (*ptr == '#' || !*ptr) continue;
		if (strstr(ptr,(char*)":quit")) break; // assume done
		if (strstr(ptr,(char*)":trace"))
		{
			trace = (trace == -1) ? 0 : -1;
			echo = (trace != 0);
			continue;
		}

		char user[MAX_WORD_SIZE];
		char* u = strstr(ptr,(char*)"user:");
		if (!u) 
		{
            FreeBuffer();
            FreeBuffer();
            ReportBug("Inconsistent regression file");
			return;
		}
		u += 5;
		ptr = ReadCompiledWord(u,user);
		
		char bot[MAX_WORD_SIZE];
		u = strstr(myBuffer,(char*)"bot:") + 4;
		ptr = ReadCompiledWord(u,bot);	
		
		ptr = strchr(ptr,'(');
		char topic[MAX_WORD_SIZE];
		char* end = strchr(ptr,')');
		*end = 0;
		ptr = ReadCompiledWord(ptr + 1,topic);	// now points to volley
		*end = ')';

		char volleyid[MAX_WORD_SIZE];
		ptr = ReadCompiledWord(ptr+1,volleyid); // volley id
		char* vinput = ptr;
		char* oldsaid = strstr(vinput,(char*)"==> ");
		char actualOutput[MAX_WORD_SIZE];
		strcpy(actualOutput,oldsaid + 4);
		*oldsaid = 0;
		if (*(oldsaid-1) == ' ') *(oldsaid-1) = 0;
		if (*(oldsaid-2) == ' ') *(oldsaid-2) = 0;
		oldsaid += 4;
		oldsaid = TrimSpaces(oldsaid,true);

		// EXECUTE the input choice
		char* buffer = AllocateBuffer();
		mainOutputBuffer = buffer;

		// bot login
		strcpy(computerID,bot);
		*computerIDwSpace = ' ';
		MakeLowerCopy(computerIDwSpace+1,computerID);
		strcat(computerIDwSpace,(char*)" ");

		strcpy(loginID,user); // user login

		if (*myBuffer == 'S' || myBuffer[3] == 'S' ) // start it  - 1st line may have utf8 marker
		{
			int depth = globalDepth; // reset clears depth, but we are still in process so need to restore it
			int turn = atoi(volleyid);
			if (turn == 0) ResetUser(vinput); // force reset
			globalDepth = depth;
			*vinput = 0;
			userFirstLine = volleyCount+1;
			*readBuffer = 0;
			nextInput = vinput;
			ProcessInput("");
			FinishVolley(vinput,buffer,NULL);
			char* revised = Purify(buffer);
			if (revised != buffer) strcpy(buffer,revised);
			TrimSpaces(buffer,false);
			if (!responseIndex)
			{
				Log(ECHOSTDUSERLOG,(char*)"*** No response to startup\r\n");
			}
		}
		else if (*myBuffer == 'R' || myBuffer[3] == 'R' )// respond - 1st line may have utf8 marker
		{
			int depth = globalDepth; // reset clears depth, but we are still in process so need to restore it
			++volley;
			ReadUserData();
			char myinput[MAX_WORD_SIZE];
			strcpy(myinput,vinput);
			globalDepth = depth;
			ProcessInput(myinput);
			FinishVolley(myinput,buffer,NULL);
			char* revised = Purify(buffer);
			if (revised != buffer) strcpy(buffer,revised);
			TrimSpaces(buffer,false);
			if (!responseIndex)
			{
				Log(ECHOSTDUSERLOG,(char*)"*** No response to user input\r\n");
			}
		}
		else
		{
			Log(STDUSERLOG,(char*)"Bad regression file lineup %s\r\n",myBuffer);
			continue;
		}
		++count;

		// now get verification data
		ReadALine(readBuffer,in);
		strcpy(verifyinfo,readBuffer); // copy so we can debug seeing original data
		//	verify:  rule:~hello.1.0. kind:t label: pattern: ( !$olduser =8%input=0 =7%hour<12 )  output: $olduser = 1 Good morning. $begintime = %fulltime  
		char* vverify = strstr(verifyinfo,(char*)"verify: ") + 8;
		ptr = strstr(vverify,(char*)"rule:");
		*ptr = 0; // end of verification level
		ptr += 5;
		char vtag1[MAX_WORD_SIZE];
		ptr = ReadCompiledWord(ptr,vtag1);
		ptr = strstr(ptr,(char*)"kind:")+5;
		char vkind = *ptr;
		char* vlabel = strstr(ptr,(char*)"label:") + 6;
		char* vpattern = strstr(vlabel,(char*)"pattern: ");
		*--vpattern = 0;
		vpattern += 10;
		char* equal = strchr(vtag1,'=');
		if (equal) *equal = 0;	// remove label from tag

		char* voldoutputcode = strstr(vpattern,(char*)"output: "); // what it said then.
		*--voldoutputcode = 0;
		voldoutputcode += 9;
		len = strlen(voldoutputcode);
		while (voldoutputcode[len-1] == ' ') --len;
		voldoutputcode[len] = 0;
		char* at = strchr(voldoutputcode,'`');
		if (at) *at = 0;

		bool sametag = false;
		bool sameruletype  = false;
		bool samelabel = false;;
		bool samepattern = false;;
		bool sameoutput = false;; // used same rule
		char tag[MAX_WORD_SIZE];
		int id;
		int topicid;
		char label[MAX_WORD_SIZE];
		char pattern[MAX_WORD_SIZE];
		char outputdata[MAX_WORD_SIZE];
		for (int i = 0; i < responseIndex; ++i) // use last said as topicid (we usually prefix emotions and other things)
		{
			// get actual results
			unsigned int order = responseOrder[i];
			strcpy(tag,GetTopicName(responseData[order].topic));
			strcat(tag,responseData[order].id);
			GetVerify(tag,topicid,id);
			char* rule = GetRule(topicid,id);		// the rule we want to test
			char* newoutputcode = GetPattern(rule,label,pattern);
			size_t len = strlen(newoutputcode);
			if (len > 50) len = 50;
			strncpy(outputdata,newoutputcode,len);
			while (outputdata[len-1] == ' ') --len;
			outputdata[len] = 0;
			char* close = strchr(outputdata,'`');
			if (close) *close = 0;	// end rule

			pattern[50] = 0;	 // limit it
			if (!sametag) sametag = !strnicmp(tag,vtag1,strlen(vtag1));
			if (!sameruletype) sameruletype  = vkind == *rule;
			if (!samelabel) samelabel = *label && !stricmp(label,vlabel);
			if (!samepattern) samepattern = !stricmp(pattern,vpattern);
			if (!sameoutput) sameoutput = !stricmp(outputdata,voldoutputcode); // used same rule
		}

		bool samesaid = !stricmp(oldsaid,buffer);
		if (!samesaid && strstr(oldsaid,buffer)) samesaid = true; // see if we subsume what was said
		char changes[MAX_WORD_SIZE];
		*changes = 0;
		if (!sametag) strcat(changes,(char*)"Tag, ");
		if (!sameruletype) strcat(changes,(char*)"Rule type, ");
		if (!samelabel && (*label || *vlabel)) strcat(changes,(char*)"Label, ");
		if (!samepattern) strcat(changes,(char*)"Pattern, ");
		if (!sameoutput) strcat(changes,(char*)"Output, "); // we only care if outputdata changed, not what was said which might be random
				
		ReadALine(readBuffer,in); // verify2 info
		char verify2info[MAX_WORD_SIZE];
		strcpy(verify2info,readBuffer); // copy so we can debug seeing original data
		//	verify:  rule:~hello.1.0. kind:t label: pattern: ( !$olduser =8%input=0 =7%hour<12 )  output: $olduser = 1 Good morning. $begintime = %fulltime  
		char* vverify2 = strstr(verify2info,(char*)"verify2: ") + 8;
		ptr = strstr(vverify2,(char*)"rule:");
		if (ptr) *ptr = 0; // end of verification level

		if (sametag && sameruletype && samepattern && sameoutput){;} // matches rule id, output, kind, pattern - perfect match
		else if (sameoutput || samepattern || samelabel || samesaid)  
		{
			if (!sametag && samesaid) ++minorchange;
			else if (silent) {;}
			else if (!sametag) Log(ECHOSTDUSERLOG,(char*)"        Volley %d input %s changed tag. Was: %s  is: %s\r\n",volley,vinput,vtag1,tag);
			else Log(ECHOSTDUSERLOG,(char*)"    Volley %d input %s is intact. %s changed\r\n",volley,vinput,changes);
			if (!samesaid && !silent)
			{
				Log(ECHOSTDUSERLOG,(char*)"            Old said: %s\r\n",oldsaid);
				Log(ECHOSTDUSERLOG,(char*)"            Now says: %s\r\n\r\n",buffer);
			}
			modified = true;
		}
		else 
		{
			Log(ECHOSTDUSERLOG,(char*)"*** Volley %d input %s - changed radically. old:  %s now: %s\r\n",volley,vinput, vtag1, tag);
			if (!samesaid)
			{
				if (*SkipWhitespace(vverify) || *SkipWhitespace(vverify2)) Log(ECHOSTDUSERLOG,(char*)"            Old verify: %s  +  %s\r\n",vverify,vverify2);
				Log(ECHOSTDUSERLOG,(char*)"            Old said: %s  -  %s pattern: %s",oldsaid,vlabel,vpattern);
				int oldtopic;
				int oldid;
				GetVerify(vtag1,oldtopic,oldid);
				TraceSample(oldtopic,oldid,ECHOSTDUSERLOG);
				Log(ECHOSTDUSERLOG,(char*)"\r\n");
				Log(ECHOSTDUSERLOG,(char*)"            Now says: %s   - %s pattern: %s  ",buffer,label,pattern);
				TraceSample(topicid,id,ECHOSTDUSERLOG);
				Log(ECHOSTDUSERLOG,(char*)"\r\n\r\n");
			}			
			++changed;
		}
	}
	userLog = olduserlog;
	FClose(in);
	echo = oldecho;
	mainOutputBuffer = holdmain;
	prepareMode = NO_MODE;
	regression = NO_REGRESSION;
	// shall we revise the regression file?
	if (changed) Log(ECHOSTDUSERLOG,(char*)"There were %d rules which changed radically of %d inputs.\r\n",changed,count);
	if (minorchange) Log(ECHOSTDUSERLOG,(char*)"There were %d rules which changed tag.\r\n",minorchange);
			
	if (changed || modified || minorchange)
	{
        if (exitrequest) exit(1); // fail batch job

		(*printer)((char*)"%s",(char*)"\nRegression has changed. Do you want to update regression to the current results? Only \"yes\" will do so: ");
		ReadALine(readBuffer,stdin);
		if (!stricmp(readBuffer,(char*)"yes"))
		{
			char fdo[MAX_WORD_SIZE];
			sprintf(fdo,(char*)"%s/tmpregresslog.txt %s",users,file);
			MemorizeRegress(fdo);
		}
	}
    else
    {
        if (exitrequest) exit(0); // succes batch job
        (*printer)((char*)"%s", (char*)"Regression passed.\r\n");
    }
    FreeBuffer();
    FreeBuffer();
    FreeBuffer();
}

static void C_Regress(char* input)
{
	char word[MAX_WORD_SIZE];
	char* xxptr = ReadCompiledWord(input,word);
	if (!strnicmp(input,(char*)"init ",5)) MemorizeRegress(input+5);
	else VerifyRegress(input);
}

static void C_Source(char* input)
{
	char word[MAX_WORD_SIZE];
	multiuser = false;
	char* ptr = ReadCompiledWord(input,word);
	if (!stricmp(word, "multiuser")) // not functional yet
	{
		multiuser = true;
		ptr = ReadCompiledWord(ptr, word);
	}
	FILE* in = FopenReadNormal(word); // source
	if (in) sourceFile = in;
	else Log(STDUSERLOG,(char*)"No such source file: %s\r\n",word);
	SetUserVariable((char*)"$$document",word);
	ReadCompiledWord(ptr,word);
	echoSource = NO_SOURCE_ECHO;
	if (!stricmp(word,(char*)"echo")) echoSource = SOURCE_ECHO_USER;
	else if (!stricmp(word,(char*)"internal"))  echoSource = SOURCE_ECHO_LOG;
	
	sourceStart = ElapsedMilliseconds();
	sourceTokens = 0;
	sourceLines = 0;
} 

static void ReadNextDocument(char* name,uint64 value) // ReadDocument(inBuffer,sourceFile) called eventually to read document
{
	FILE* in = FopenReadNormal(name); // source
	if (in) sourceFile = in;
	else 
	{
		Log(STDUSERLOG,(char*)"No such document file: %s\r\n",name);
		return;
	}
	docSentenceCount = 0;
	sourceStart = ElapsedMilliseconds();
	sourceTokens = 0;
	sourceLines = 0;
	readingDocument = true;
	SetBaseMemory();
	inputSentenceCount = 0;
	docVolleyStartTime = ElapsedMilliseconds(); // time limit control
	tokenCount = 0;
	InitJSONNames(); // reset indices for this volley
	ClearVolleyWordMaps();
	ResetEncryptTags();
	ResetToPreUser(); // back to empty state before any user
	ReadNewUser();	// read user info back in so we can continue (a form of garbage collection)
	ShowStats(true);

	SetUserVariable((char*)"$$document",name);
	if (!trace) echo = false;
	*mainOutputBuffer = 0;
	documentMode = true;
	randIndex =  oldRandIndex;
	OnceCode((char*)" ",(char*)"~document_pre"); // just once per document
	randIndex =  oldRandIndex;

	// read the file
	while (sourceFile != stdin && !quitting) ProcessInputFile();
	
	if (docstats) 
	{
		bool oldecho = echo;
		echo = true;

		unsigned int diff = (unsigned int) (ElapsedMilliseconds() - docVolleyStartTime);
		unsigned int mspl = diff/inputSentenceCount;
		double fract = (double)(diff/1000.0); // part of seccond
		double time = (double)(tokenCount/fract);
	
		unsigned int seconds = (diff/1000);
		diff -= (seconds * diff);
		if (diff >= 500) ++seconds;
		unsigned int minutes = seconds/60;
		seconds -= (minutes * 60);
		unsigned int hours = minutes/60;
		minutes -= (hours * 60);

		Log(STDUSERLOG,(char*)"\r\nDocument Read: %d sentences (%d tokens) in %d hours %d minutes %d seconds\r\n",inputSentenceCount,tokenCount, hours,minutes,seconds);
		Log(STDUSERLOG,(char*)"%d ms/sentence or %f token/s\r\n",mspl,time);
		
		unsigned int dictUsed = dictionaryFree - dictUsedG;
		unsigned int factUsed = factFree - factUsedG;
		unsigned int textUsed = (textUsedG - heapFree) / 1000;
		uint64 dictAvail =  maxDictEntries-(dictionaryFree-dictionaryBase);
		unsigned int factAvail = factEnd-factFree;
		unsigned int textAvail = (heapFree- (char*)dictionaryFree) / 1000;
		Log(STDUSERLOG,(char*)"\r\nUsed- dict:%d fact:%d text:%dkb   Free- dict:%d fact:%d  text:%dkb\r\n",dictUsed,factUsed,textUsed,(unsigned int)dictAvail,factAvail,textAvail);

		echo = oldecho;
	}
	// do post process on document
	postProcessing = 1;
	FinishVolley((char*)" ",mainOutputBuffer,(char*)"~document_post"); // per document post process and will write out stuff  and reset user memory and ...
	postProcessing = 0;
	if (*mainOutputBuffer) (*printer)((char*)"%s\r\n",UTF2ExtendedAscii(mainOutputBuffer));
	documentMode = false;
	readingDocument = false;
}

static void C_Document(char* input)
{
	dictUsedG = dictionaryFree; // track memory use
	factUsedG = factFree;
	textUsedG = heapFree;
	docSampleRate = 0;
	documentBuffer = AllocateBuffer();
	baseBufferIndex = bufferIndex; // lock this in
	*documentBuffer = 0;
	++baseBufferIndex; // system will reset  buffers each sentence to include ours
	char name[MAX_WORD_SIZE];
	char* ptr = ReadCompiledWord(input,name);
	char attrib[MAX_WORD_SIZE];
	echoSource = NO_SOURCE_ECHO;
	docstats = false;
	bool docquit = false;
	while (ptr)
	{
		ptr = ReadCompiledWord(ptr,attrib);
		if (!*attrib) break;
		char fname[200];
		sprintf(fname, "%s/out.txt",tmp);
		if (!stricmp(attrib,(char*)"single")) singleSource = true; // one line at a time, regardless of inability to find a complete sentence
		else if (!stricmp(attrib,(char*)"echo")) docOut = FopenUTF8Write(fname); // clear it
		else if (!stricmp(attrib,(char*)"stats")) docstats = true;
		else if (!stricmp(attrib,(char*)"quit")) docquit = true;
		else if (!stricmp(attrib,(char*)"sample"))
		{
			ptr = ReadCompiledWord(ptr,attrib);
			docSampleRate = docSample = atoi(attrib);
		}
	}
	
	MemoryMarkCode(NULL);

	OnceCode((char*)"$cs_control_pre");

	size_t len = strlen(name);
	if (name[len-1] == '/') WalkDirectory(name,ReadNextDocument, 0,false);
	else ReadNextDocument(name,0);
	echo = false;
	
	postProcessing = 1; // dedicate buffer to alternate use
	documentBuffer = 0;
	FinishVolley((char*)" ",mainOutputBuffer,NULL); // bots post processing step
	FreeBuffer(); // release document buffer
	baseBufferIndex = bufferIndex; // lock this in
	--baseBufferIndex;
	postProcessing = 0;
	
	if (docOut) // end echoing
	{
		FClose(docOut);
		docOut = NULL;
	}

	if (docquit) exit(0);
} 

static void DoAssigns(char* ptr)  // find variable assignments
{
	char var[MAX_WORD_SIZE];
	char* dollar;
	char* percent;
	char* underscore;
	char* at;
	char* first = 0;
	while (ptr) // do all user variables
	{
		at = NULL;
		char* spot = ptr;
		char* d = ptr;
		dollar = NULL;
		while ( (d = strchr(d,USERVAR_PREFIX))) // find potential variable, not money
		{
			if (IsDigit(d[1])) ++d;
			else
			{
				dollar = d;
				break;
			}
		}
		percent = strchr(ptr,'%');
		underscore = strchr(ptr,'_');
		if (dollar) at = dollar;
		else if (percent) at = percent;
		else while ((underscore = strchr(spot,'_')))  // may not be real yet, might be like New_year's_eve 
		{
			if (IsDigit(underscore[1]) && *(underscore-1) != '@') 
			{
				at = underscore;
				break;
			}
			else spot = underscore + 1;
		}
		if (!at) break;
	
		if (percent && percent < at) at = percent;
		if (underscore && underscore < at) at = underscore;
		
		// at is the soonest assignment
		char* eq = strchr(at,'='); // has an assignment
		if (!eq) break; // no assignment
		char c = *eq;
		*eq-- = 0; // break off lhs
		ReadCompiledWord(at,var);
		*eq = c;  // change  $current=1  into $current = 1 moving operator and giving space after

		if (eq[2] == '=') 
		{
			eq[1] = eq[2];
			eq[2] = ' '; // 2 char operator
		}
		else eq[1] = ' '; // single char operator

		if (!first) first = at; // start of all
		FunctionResult result;
		char* buffer = AllocateBuffer();
        char* period = strchr(var, '.');
        char* bracket = strchr(var, '[');
        if (bracket < period && bracket) period = NULL;
        if (period < bracket && period) bracket = NULL;
        if (period) // need transient object
        {
            MEANING objectName = GetUniqueJsonComposite((char*)"jo-t");
            *period = 0;
            SetUserVariable(var, Meaning2Word(objectName)->word);
            *period = '.';
        }
        else if (bracket) // need transient object
        {
            MEANING objectName = GetUniqueJsonComposite((char*)"ja-t");
            *bracket = 0;
            SetUserVariable(var, Meaning2Word(objectName)->word);
            *bracket = '[';
        }
		ptr = PerformAssignment(var,eq,buffer,result);
		FreeBuffer();
		if (ptr) memset(at,' ',ptr-at);
	}

	// do all positional locators
	char* posn = ptr;
	while ((posn = strchr(posn,'@')))
	{
		if (*(posn-1) == ' ' && posn[1] == '_' && IsDigit(posn[2]))
		{
			if (!first) first = posn; // end with this test
			int matchvar = atoi(posn+2);
			// at is the soonest assignment
			char* eq = strchr(posn,'='); // has an assignment
			if (!eq) break; // no assignment
			int pos = atoi(eq+1); 
			wildcardPosition[matchvar] = pos | WILDENDSHIFT(pos);	// do the assignment
		}
		++posn;
	}

	// do all word memberships
	char* mem = ptr;
	while ((mem = strchr(mem,'?'))) 
	{
		if (mem[1] == '~')
		{
			char* at1 = mem;
			while (*--at1 != ' ');
			char var[MAX_WORD_SIZE];
			*mem = 0;
			ReadCompiledWord(at1+1,var);
			if (!first) first = at1; // end with this test
			char set[MAX_WORD_SIZE];
			ReadCompiledWord(mem+1,set);
			MEANING memx = MakeMeaning(StoreWord((char*)"member"));
			CreateFact(MakeMeaning(StoreWord(var)),memx,MakeMeaning(StoreWord(set)),FACTTRANSIENT);
			*mem = '?';
		}
		++mem;
	}
	
	// do all positional marks  2+=xxx 
	mem = ptr;
	while ((mem = strstr(mem,(char*)"+="))) 
	{
		if (IsDigit(*(mem-1)))
		{
			char* at1 = mem;
			while (*--at1 != ' ')
			{
				if (!IsDigit(*(mem-1))) break; // not a number
			}
			/// NOT COMPLETE CODE
			if (*at1 == ' ')
			{
				char var[MAX_WORD_SIZE];
				*mem = 0;
				ReadCompiledWord(at1,var);
				if (!first) first = at1; // end with this test
				char set[MAX_WORD_SIZE];
				ReadCompiledWord(mem+1,set); // the mark to make
				*mem = '+';
			}
		}
		++mem;
	}
	if (first)  *first = 0; // remove all assignments
}

static void C_TestPattern(char* input)
{ // pattern, input, optional var assigns  -  (drink)  Do you like drink? %date = 1
#ifndef DISCARDSCRIPTCOMPILER
    if (*input != '(' && *input != '~')
    {
        Log(STDUSERLOG, (char*)"Bad test pattern");
        return;
    }

    char data[MAX_WORD_SIZE];
    char* pack = data;
    strcpy(readBuffer, input);

    char label[MAX_WORD_SIZE];
    char* ptr = ReadCompiledWord(readBuffer, label);
    int topic = currentTopicID;
    bool fulllabel = false;
    int id = 0;
    bool crosstopic = false;
    if (*label == '~') // named an existing rule
    {
        char* rule;
        char* dot = strchr(label, '.');
        if (!dot)
        {
            Log(STDUSERLOG, (char*)" %s rule lacks dot\r\n", label);
            return;
        }
        if (dot && IsDigit(dot[1])) rule = GetRuleTag(topic, id, label);
        else rule = GetLabelledRule(topic, label, (char*)"", fulllabel, crosstopic, id, currentTopicID);
        if (!rule)
        {
            Log(STDUSERLOG, (char*)" %s rule not found\r\n", label);
            return;
        }
        GetPattern(rule, NULL, data);
    }
    else
    {
        int buffercount = bufferIndex;
        if (setjmp(scriptJump[++jumpIndex])) // return on script compiler error
        {
            bufferIndex = buffercount;
            --jumpIndex;
            return;
        }
        bool inited = StartScriptCompiler(false,true);
        ReadNextSystemToken(NULL, NULL, data, false, false); // flush cache
        ptr = ReadPattern(readBuffer, NULL, pack, false, false); // swallows the pattern
        if (inited) EndScriptCompiler();
    }

    //   var assign?
    DoAssigns(ptr);
    if (!*ptr) return; //   forget example sentence

    char prepassTopic[MAX_WORD_SIZE];
    strcpy(prepassTopic, GetUserVariable((char*)"$cs_prepass"));
    PrepareSentence(ptr, true, true);
    PrepassSentence(prepassTopic);

    wildcardIndex = 0;
    int junk1;
    int oldtrace = trace;
    trace |= TRACE_PATTERN;
    int oldtiming = timing;
    timing |= TIME_PATTERN;
    int uppercasem = 0;
    int whenmatched = 0;
    SetContext(true);
    char* buffer = AllocateBuffer();
    char oldmark[MAX_SENTENCE_LENGTH];
    memcpy(oldmark, unmarked, MAX_SENTENCE_LENGTH);
    bool result = Match(buffer, data + 2, 0, 0, (char*)"(", 1, 0, junk1, junk1, uppercasem, whenmatched, 0, 0);
    memcpy(unmarked, oldmark,  MAX_SENTENCE_LENGTH);
    FreeBuffer();

    SetContext(false);
    trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
    timing = (modifiedTiming) ? modifiedTimingVal : oldtiming;
    if (result)
    {
        Log(STDUSERLOG, (char*)" Matched\r\n");
        if (wildcardIndex)
        {
            Log(STDUSERLOG, (char*)" wildcards: (");
            for (int i = 0; i < wildcardIndex; ++i)
            {
                if (*wildcardOriginalText[i]) Log(STDUSERLOG, (char*)"_%d=%s / %s (%d-%d) ", i, wildcardOriginalText[i], wildcardCanonicalText[i], wildcardPosition[i] & 0x0000ffff, wildcardPosition[i] >> 16);
                else Log(STDUSERLOG, (char*)"_%d=null (%d-%d) ", i, wildcardPosition[i] & 0x0000ffff, wildcardPosition[i] >> 16);
            }
        }
        Log(STDUSERLOG, (char*)"\r\n");
    }
    else
    {
        Log(STDUSERLOG, (char*)" Failed\r\n    Original Input: %s", mainInputBuffer);
        Log(STDUSERLOG, (char*)"                Adjusted Input: ");
        for (int i = 1; i <= wordCount; ++i) Log(STDUSERLOG, (char*)"%s ", wordStarts[i]);
        Log(STDUSERLOG, (char*)"\r\n    Canonical Input: ");
        for (int i = 1; i <= wordCount; ++i) Log(STDUSERLOG, (char*)"%s ", wordCanonical[i]);
        Log(STDUSERLOG, (char*)"\r\n");
    }
    --jumpIndex;
#endif
}

static void GambitTestTopic(char* topic)
{
	int topicID = FindTopicIDByName(topic);
	if (!topicID) 
	{
		Log(STDUSERLOG,(char*)"topic not found %s\r\n",topic);
		return;
	}
	if (GetTopicFlags(topicID) & TOPIC_NOGAMBITS) return;
	int  oldRegression = regression;
	regression = NORMAL_REGRESSION;

	char* data = GetTopicData(topicID);
	char* output = AllocateBuffer();
	int ruleID = -1;
	while (data)
	{
		char* rule = data;
		++ruleID;
		int id = 0;
		data = FindNextRule(NEXTTOPLEVEL,data,id);
		if (*rule != GAMBIT && *rule != RANDOM_GAMBIT) continue; // not a gambit

		// get the output
		rule  = GetPattern(rule,NULL,NULL);
		char* end = strchr(rule,ENDUNIT);  // will not be a useful output as blanks will become underscores, but can do ^reuse() to execute it
		*end = 0;
		strcpy(output,rule);
		*end = ENDUNIT;
		char* q = strchr(output,'?');
		if (!q) continue;	 // not a question
		q[1] = 0; // ignore any following.
		char* at = q;
		while (--at > output) // is there a question before this
		{
			if ((*at == '.' || *at == '!') && at[1] == ' ') break;// end of a sentence?
		}
		if (at != output) output = at+1;

		//  perform varible setup, do assigns, and prepare matching context
		ResetToPreUser();
		KillShare();
		ReadNewUser();   
		char prepassTopic[MAX_WORD_SIZE];
		strcpy(prepassTopic,GetUserVariable((char*)"$cs_prepass"));
		volleyCount = 1;
		strcpy(currentInput,output);	//   this is what we respond to, literally.
		nextInput = output;
		OnceCode((char*)"$cs_control_pre");
		while (ALWAYS)
		{
			PrepareSentence(output,true,true);
			if (!PrepassSentence(prepassTopic)) break;
		}
		AddPendingTopic(topicID); // ResetToPreUser clears pendingTopicIndex
		responseIndex = 0;
		Reply();
		if (pendingTopicIndex && pendingTopicList[pendingTopicIndex-1] == topicID){;}
		else if (!responseIndex || responseData[0].topic != topicID )
		{
			Log(STDTRACETABLOG,(char*)"Not answering own question in topic %d %s.%d.%d: %s => %s %s \r\n\r\n",++err,topic,TOPLEVELID(ruleID),REJOINDERID(ruleID),output,GetTopicName(responseData[0].topic),responseData[0].response);
		}
	}
	FreeBuffer();
	regression = oldRegression;
}

static void C_TestTopic(char* input)
{
	char word[MAX_WORD_SIZE];
	input = ReadCompiledWord(input,word);
	char prepassTopic[MAX_WORD_SIZE];
	strcpy(prepassTopic,GetUserVariable((char*)"$cs_prepass"));
	while (ALWAYS)
	{
		PrepareSentence(nextInput,true,true);	
		if (!PrepassSentence(prepassTopic)) break;
	}
	int topic = FindTopicIDByName(word);
	if (!topic)  return;
    CALLFRAME* frame = ChangeDepth(1,word);
    int pushed =  PushTopic(topic);
	if (pushed < 0) 
	{
		ChangeDepth(-1,word);
		return;
	}
	ClearUserVariableSetFlags();
	AllocateOutputBuffer();
	PerformTopic(0,currentOutputBase); //   ACTIVE handle - 0 is good result
	FreeOutputBuffer();
	ChangeDepth(-1,word);
    for (int i = 0; i < responseIndex; ++i) Log(STDUSERLOG,(char*)"%s\r\n", responseData[responseOrder[i]].response);
	ShowChangedVariables();
}

static void VerifyAccess(char* topic, char kind, char* prepassTopic) // prove patterns match comment example, kind is o for outside, r for rule, t for topic, s for samples
{
    bool testKeyword = kind == 'k';
    bool testPattern = kind == 'p';
    bool testBlocking = kind == 'b';
    bool testSample = kind == 's' || kind == 'S';
    if (kind == 'a' || !kind) testKeyword = testPattern = testBlocking = true;
    int topicID = FindTopicIDByName(topic, true);
    if (!topicID)
    {
        (*printer)((char*)"%s not found\r\n", topic);
        return;
    }
    WORDP topicWord = FindWord(GetTopicName(topicID)); // must find it
    topic = topicWord->word;

    int flags = GetTopicFlags(topicID);
    if (flags & TOPIC_BLOCKED) return;

    if (testKeyword) 	// has no keyword into here so dont test keyword access
    {
        FACT* F = GetObjectNondeadHead(topicWord);
        while (F)
        {
			if (ValidMemberFact(F))  break;
            F = GetObjectNondeadNext(F);
        }
        if (!F)  testKeyword = false;
    }

    if (GetTopicFlags(topicID) & (TOPIC_RANDOM | TOPIC_NOBLOCKING)) testBlocking = false;
    if (GetTopicFlags(topicID) & (TOPIC_RANDOM | TOPIC_NOKEYS)) testKeyword = false;
    if (GetTopicFlags(topicID) & TOPIC_NOPATTERNS) testPattern = false;
    if (GetTopicFlags(topicID) & TOPIC_NOSAMPLES) testSample = false;

    WORDP D = FindWord(topic);
    char c = (D->internalBits & BUILD0) ? '0' : '1';
    char name[100];
    char* tname = (*topic == '~') ? (topic + 1) : topic;
    if (duplicateCount) sprintf(name, (char*)"VERIFY/%s-b%c.%d.txt", tname, duplicateCount, c);
    else sprintf(name, (char*)"VERIFY/%s-b%c.txt", tname, c);
    FILE* in = FopenReadWritten(name);
    if (!in)
    {
        (*printer)((char*)"No %s verification data\r\n", name);
        return;
    }

    unsigned int oldtrace = trace;
    trace = 0;
    unsigned int oldtiming = timing;
    timing = 0;
    Log(STDUSERLOG, (char*)"VERIFYING %s ......\r\n", topic);
    char* copyBuffer = AllocateBuffer();
    char* primaryBuffer = AllocateBuffer();
    char junk[MAX_WORD_SIZE];
    // process verification data
    while (ReadALine(readBuffer, in) >= 0)
    {
        if (bufferIndex > 6) return;

        if (!strnicmp(readBuffer, (char*)":trace", 6))
        {
            trace = atoi(readBuffer + 11);
            continue;
        }
        if (!strnicmp(readBuffer, (char*)":time", 5))
        {
            timing = atoi(readBuffer + 10);
            continue;
        }
        if (!strnicmp(readBuffer, (char*)":exit", 5)) myexit((char*)":exit requested");
        bool failTest = false;
        strcpy(primaryBuffer, readBuffer);

        // read tag of rule to apply input to
        int verifyRuleID;
        char* dot = GetRuleIDFromText(primaryBuffer, verifyRuleID);
        if (!dot) return;
        char* rule = GetRule(topicID, verifyRuleID);					// the rule we want to test
        char* topLevelRule = GetRule(topicID, TOPLEVELID(verifyRuleID));	// the top level rule (if a rejoinder)
        char* rejoinderTop = NULL;
        int rejoinderTopID = 0;
        if (!rule) break;
        if (rule && rule != topLevelRule) // its a rejoinder, find the start of the rejoinder area
        {
            rejoinderTopID = verifyRuleID;
            char* at = rule; // start at the given rejoinder
            while (*at >= *rule && REJOINDERID(rejoinderTopID)) // stop if drop below our level or go to top level rule
            {
                at = RuleBefore(topicID, at);
                rejoinderTopID -= ONE_REJOINDER;
            }
            rejoinderTopID += ONE_REJOINDER; // now move back down to 1st of our level
            rejoinderTop = FindNextRule(NEXTRULE, at, rejoinderTopID);
        }

        // the comment headers are:
        // #!x  - description header for :abstract
        // #!!F  - expect to fail RULE
        // #!!T - expect to fail TOPIC (be masked by earlier rule)
        // #!!P - dont test patterns
        // #!!K - dont test keywords
        // #!!S - dont run sample
        *junk = junk[1] = junk[2] = 0;
        char* test = strchr(primaryBuffer, '!') + 1;	// the input sentence (skipping offset and #! marker)
        if (*test != ' ') test = ReadCompiledWord(test, junk); // things to not test
        MakeLowerCase(junk);
        if (*junk == 'x') continue;  // only used for :abstract

        bool wantFailBlocking = false;
        bool wantFailKeyword = false;
        bool wantNoPattern = false;
        bool wantNoSample = false;
        bool wantFailMatch = false;
        // SUPPRESS testing this rule for this
        if (strchr(junk, 'b') || strchr(junk, 'B')) wantNoSample = wantFailBlocking = true;
        if (strchr(junk, 'k') || strchr(junk, 'K')) wantNoSample = wantFailKeyword = true;
        if (strchr(junk, 's') || strchr(junk, 'S')) wantNoSample = true;
        if (strchr(junk, 'p') || strchr(junk, 'P')) wantNoPattern = true;

        if (strchr(junk, 'f') || strchr(junk, 'F')) wantFailMatch = true;

        //   test pattern on THIS rule

        //  perform varible setup, do assigns, and prepare matching context
        ResetToPreUser();
        KillShare();
        ReadUserData();
        if (verifyToken != 0) tokenControl = verifyToken;
        volleyCount = 1;
        if (testSample) OnceCode((char*)"$cs_control_pre");

        DefineSystemVariables(); // clear system variables to default
        DoAssigns(test); // kills test start where any defines are
        strcpy(copyBuffer, test);
        strcpy(currentInput, test);	//   this is what we respond to, literally.
        nextInput = test;
        while (ALWAYS)
        {
            PrepareSentence(nextInput, true, true);
            if (!PrepassSentence(prepassTopic)) break;
        }
        currentTopicID = topicID;
        strcpy(test, copyBuffer); // sentence prep may have altered test data and we might want to redo it
        AddPendingTopic(topicID); // ResetToPreUser clears pendingTopicIndex

        char label[MAX_WORD_SIZE];
        char pattern[MAX_WORD_SIZE];
        char* topLevelOutput = GetOutputCopy(topLevelRule);
        GetPattern(rule, label, pattern);
        if (!*pattern)
        {
            ReportBug((char*)"No pattern here? %s %s\r\n", topic, rule)
                continue;
        }

        bool result;
        if (testKeyword && !wantFailKeyword && !TopLevelGambit(rule) && TopLevelRule(rule))  // perform keyword test on sample input sentence
        {
            int pStart = 0;
            int pEnd = 0;
            if (!GetNextSpot(topicWord, 0, pStart, pEnd)) // not findable topic
            {
                Query((char*)"direct_v", (char*)"?", (char*)"idiom", topic, (unsigned int)-1, (char*)"?", (char*)"?", (char*)"?", (char*)"?");  // get all idioms that can trigger this topic
                unsigned int i = FACTSET_COUNT(0);
                while (i > 0)
                {
                    FACT* F = factSet[0][i];
                    WORDP pattern = Meaning2Word(F->subject);
                    strcpy(callArgumentList[callArgumentBase + 1], pattern->word);
                    *callArgumentList[callArgumentBase + 2] = 0; // dumy argument 1
                    if (MatchCode(junk) == 0) break;
                    --i;
                }

                if (i == 0)
                {
                    Log(STDTRACETABLOG, (char*)"%d Missing keyword %s.%d.%d <= %s\r\n", ++err, topic, TOPLEVELID(verifyRuleID), REJOINDERID(verifyRuleID), test);
                    failTest = true;
                }
            }
        }

        //   inside the pattern, test this rule
        if (testPattern && !wantNoPattern) // not blocking pattern testing
        {
            ++trials;
            if (*rule == '?' && !(tokenFlags & QUESTIONMARK)) result = 0; // cannot match
            else if (*rule == 's' && tokenFlags & QUESTIONMARK) result = 0; // cannot match
            else
            {
                SetContext(true); // force all contexts to be valid
                result = RuleTest(rule);
                SetContext(false);
            }
            bool unexpected = (!result && !wantFailMatch) || (result && wantFailMatch);
            if (unexpected)
            {
                char label[MAX_WORD_SIZE];
                GetLabel(rule, label);
                if (wantFailMatch) Log(STDTRACETABLOG, (char*)"Pattern matched inappropriately %d %s.%d.%d: %s => %c: %s %s\r\n    Adjusted Input: ", ++err, topic, TOPLEVELID(verifyRuleID), REJOINDERID(verifyRuleID), test, *rule, label, pattern);
                else Log(STDTRACETABLOG, (char*)"Pattern failed to match %d %s.%d.%d: %s => %c: %s %s\r\n    Adjusted Input: ", ++err, topic, TOPLEVELID(verifyRuleID), REJOINDERID(verifyRuleID), test, *rule, label, pattern);
                for (int i = 1; i <= wordCount; ++i) Log(STDUSERLOG, (char*)"%s ", wordStarts[i]);
                Log(STDUSERLOG, (char*)"\r\n    Canonical Input: ");
                for (int i = 1; i <= wordCount; ++i) Log(STDUSERLOG, (char*)"%s ", wordCanonical[i]);
                Log(STDUSERLOG, (char*)"\r\n\r\n");
                failTest = true;

                // redo with tracing on if selected so we can watch it fail
                if (oldtrace || oldtiming)
                {
                    trace = oldtrace;
                    timing = oldtiming;
                    nextInput = test;
                    while (ALWAYS)
                    {
                        PrepareSentence(nextInput, true, true);
                        if (!PrepassSentence(prepassTopic)) break; // user input revise and resubmit?  -- COULD generate output and set rejoinders
                    }
                    strcpy(test, copyBuffer); // sentence prep may have altered test data and we might want to redo it
                    SetContext(true);
                    RuleTest(rule);
                    SetContext(false);
                    trace = 0;
                    timing = 0;
                    Log(STDTRACETABLOG, "\r\n:testpattern %s %s\r \n \r\n", pattern, test);
                }
                continue;
            }
        }

        if (testBlocking && !wantFailBlocking && !TopLevelGambit(rule)) // check for blocking
        {
            char* data;
            char* output = NULL;
            int id = 0;
            char pattern[MAX_WORD_SIZE];
            if (TopLevelRule(rule)) // test all top level rules in topic BEFORE this one
            {
                data = GetTopicData(topicID);
                char label[MAX_WORD_SIZE];
                while (data && data < rule)
                {
                    if (*data == GAMBIT || *data == RANDOM_GAMBIT); // no data gambits
                    else if (*rule == STATEMENT && *data == QUESTION); // no mismatch type
                    else if (*rule == QUESTION && *data == STATEMENT); // no mismatch type
                    else
                    {
                        output = GetPattern(data, label, pattern);
                        if (!*pattern) break;
                        bool result;
                        uint64 oldflags = tokenFlags;
                        SetContext(true);
                        //if (*rule == STATEMENT) tokenFlags &= -1 ^ QUESTIONMARK; // cant be question
                        //else if (*rule == QUESTION) tokenFlags |= -QUESTIONMARK; // can be question
                        if (pattern[2] == ')' || pattern[2] == '*') result = false; // universal match patterns are PRESUMED not to be blocking. they obviously obscure anything
                        else result = RuleTest(data);// past the paren
                        SetContext(false);
                        if (result)	break; // he matched, blocking us
                        tokenFlags = oldflags;
                    }
                    data = FindNextRule(NEXTTOPLEVEL, data, id);
                }
            }
            else  // rejoinder matching 
            {
                data = rejoinderTop;
                id = rejoinderTopID;
                output = " "; // rejoinders block each other whether or not they generate output,
                              // except BUG that sequence does not.
                while (data < rule)
                {
                    if (*data == *rule)// all rules of this same level and before us
                    {
                        SetContext(true);
                        bool result = RuleTest(data); // past the paren
                        SetContext(false);
                        if (result)	break; // he matched, blocking us
                    }
                    data = FindNextRule(NEXTRULE, data, id);
                }
            }

            if (data && data < rule && !strstr(pattern, (char*)"^incontext")) // earlier rule matches same pattern and is not context sensitive
            {
                // prove not a simple () (*) (!?)  (?) etc
                char* t = pattern + 2; // start AFTER the ( 
                while (ALWAYS)
                {
                    t = ReadCompiledWord(t, junk);
                    if (!stricmp(junk, (char*)"!") || !stricmp(junk, (char*)"*") || !stricmp(junk, (char*)"?")) continue;
                    break;
                }
                if (*junk == 0 || *junk == ')') continue;	// presumed end of pattern

                                                            // prove it may output something - all matching rejoinders automatically mask if occur sooner
                if (!Rejoinder(data)) // top level units that dont generate output dont actually mask.
                {
                    char word[MAX_WORD_SIZE];
                    while (*output && *output != ENDUNIT)
                    {
                        output = ReadCompiledWord(output, word);
                        if ((IsAlphaUTF8(*word)) && FindWord(word))
                            break; // possible problem
                    }
                }
                if (!output || *output == ENDUNIT) continue;	// no text output found

                if (REJOINDERID(id)) Log(STDTRACETABLOG, (char*)"Blocking %d Rejoinder %d.%d ", ++err, TOPLEVELID(id), REJOINDERID(id));
                else  Log(STDTRACETABLOG, (char*)"Blocking %d TopLevel %d.%d ", ++err, TOPLEVELID(id), REJOINDERID(id));
                TraceSample(topicID, id, STDTRACETABLOG);
                Log(STDUSERLOG, (char*)"   %s\r\n", ShowRule(data));
                Log(STDTRACETABLOG, (char*)"    blocks %d.%d %s\r\n    given: %s\r\n\r\n", TOPLEVELID(verifyRuleID), REJOINDERID(verifyRuleID), ShowRule(rule), test);
                failTest = true;
            }
        }
        if (testSample && !wantNoSample && !failTest  && TopLevelRule(rule)) // check for sample
        {
            // force bot restriction match
            char oldcomputer[MAX_WORD_SIZE];
            strcpy(oldcomputer, computerIDwSpace);
            if (TI(topicID)->topicRestriction)
            {
                strcpy(computerIDwSpace, TI(topicID)->topicRestriction);
                char* space = strchr(computerIDwSpace + 3, ' ');
                space[1] = 0;
            }

            // prepare for contextfree start
            ClearPendingTopics();
            responseIndex = 0;
            currentTopicID = 0;
            topicIndex = 0;
            outputRejoinderTopic = NO_REJOINDER;
            if (oldtrace) trace = oldtrace;

            sampleTopic = topicID;
            sampleRule = verifyRuleID;
            Reply();
            bool foundSample = sampleTopic == 0;	// it was found and canceled.
            sampleTopic = sampleRule = 0;

            trace = 0;
            if (foundSample) { ; }
            else if (!responseIndex || responseData[0].topic != topicID)
            {
                bool bad = true;
                char* end = strchr(rule, ENDUNIT);
                if (end) *end = 0;
                if (strstr(rule, (char*)"^gambit")) // we dont expect it to answer in this topic unless '~' is argument
                {
                    char * at = strstr(rule, (char*)"^gambit ( ") + 10;
                    if (at[0] == '~' && at[1] == ' ') bad = false;
                }
                else if (strstr(rule, (char*)"^respond") || strstr(rule, (char*)"^end") || strstr(rule, (char*)"^fail")) bad = false; // we dont expect it to answer here
                else if (strstr(rule, (char*)"^reuse"))
                {
                    char * at = strstr(rule, (char*)"^reuse");
                    char* paren = strchr(at, ')');
                    *paren = 0;
                    if (strchr(at, '.')) bad = false; // we dont expect it to answer here
                    *paren = ')';
                }
                if (bad)
                {
                    int gotid;
                    char* junk = GetRuleIDFromText(responseData[0].id, gotid);
                    unsigned int replytopic = responseData[0].topic;
                    char gotrule[300];
                    strcpy(gotrule, ShowRule(GetRule(replytopic, gotid)));

                    // via info
                    unsigned int replytopic2 = 0;
                    int gotid2 = 0;
                    char* gotrule2 = NULL;
                    if (*junk)
                    {
                        replytopic2 = atoi(junk + 1);
                        GetRuleIDFromText(junk + 1, gotid2);
                        gotrule2 = ShowRule(GetRule(replytopic2, gotid2));
                    }

                    char wantrule[MAX_WORD_SIZE];
                    strcpy(wantrule, ShowRule(rule));
                    Log(STDTRACETABLOG, (char*)"Bad sample topic: %d  (%s.%d.%d)   %s  =>   %s\r\n", ++err, topic, TOPLEVELID(verifyRuleID), REJOINDERID(verifyRuleID), test, responseData[0].response);
                    Log(STDTRACETABLOG, (char*)"   want: %s.%d.%d %s\r\n", GetTopicName(topicID), TOPLEVELID(verifyRuleID), REJOINDERID(verifyRuleID), wantrule );
                    Log(STDTRACETABLOG, (char*)"   got : %s%s %s\r\n", GetTopicName(replytopic), responseData[0].id, gotrule);
                    if (gotrule2) Log(STDTRACETABLOG, (char*)"    via %s.%d.%d: %s", GetTopicName(replytopic2), TOPLEVELID(gotid2), REJOINDERID(gotid2), gotrule2);
                    Log(STDUSERLOG, (char*)"\r\n\r\n");
                }
                if (end) *end = ENDUNIT;
            }
            else if (kind != 'S') // also check rule bad - topic was same but might be gambit or reuse
            {
                int id;
                char* after = GetRuleIDFromText(responseData[0].id, id);
                int reuseid = -1;
                if (*after == '.') // there is redirect rule leading to us
                {
                    GetRuleIDFromText(after + 1, reuseid);
                }
                if (id == verifyRuleID || (reuseid >= 0 && TOPLEVELID(reuseid) == (unsigned int)verifyRuleID)) { ; } // we match
                else if (TOPLEVELID(id) == (unsigned int)verifyRuleID && !strstr(topLevelOutput, (char*)"refine")) { ; } // we matched top level and are not looking for refinement
                else
                {
                    char* gotrule = GetRule(topicID, id);
                    char* end = strchr(rule, ENDUNIT);
                    bool bad = true;
                    *end = 0; // limit rule to this one only

                              // via info
                    unsigned int replytopic2 = 0;
                    int gotid2 = 0;
                    char gotrule2[300];
                    *gotrule2 = 0;
                    if (*after)
                    {
                        replytopic2 = atoi(after + 1);
                        GetRuleIDFromText(after + 1, gotid2);
                        strcpy(gotrule2, ShowRule(GetRule(replytopic2, gotid2)));
                    }

                    if (strstr(rule, (char*)"^gambit")) bad = false; // we dont expect it to answer here
                    else if (strstr(rule, (char*)"^respond")) bad = false; // we dont expect it to answer here
                    else if (strstr(rule, (char*)"^reuse")) // we dont expect it to answer here but we should have run label
                    {
                        char label[MAX_WORD_SIZE];
                        GetLabel(gotrule, label);
                        char want[MAX_WORD_SIZE];
                        char* at = strstr(rule, (char*)"^reuse ( ") + 9;
                        ReadCompiledWord(at, want);
                        if (!strcmp(want, label)) bad = false;
                    }
                    else if (strstr(rule, (char*)"^fail") || strstr(rule, (char*)"^end")) bad = false; // we dont expect it to answer here
                    if (bad)
                    {
                        char tmp[MAX_WORD_SIZE];
                        strcpy(tmp, ShowRule(rule));
                        Log(STDTRACETABLOG, (char*)"Bad sample rule %d %s  For input: %s ==> %s\r\n", ++err, topic, test, responseData[0].response);
                        Log(STDTRACETABLOG, (char*)"    want - %s.%d.%d %s\r\n",
                            GetTopicName(topicID),TOPLEVELID(verifyRuleID), REJOINDERID(verifyRuleID), tmp);
                        Log(STDTRACETABLOG, (char*)"    got - %s%s = > %s", GetTopicName(topicID),responseData[0].id, ShowRule(gotrule));
                            if (*gotrule2) Log(STDUSERLOG, (char*)"\r\n   via %s.%d.%d: %s", GetTopicName(replytopic2), TOPLEVELID(gotid2), REJOINDERID(gotid2), gotrule2);
                        Log(STDUSERLOG, (char*)"\r\n\r\n");
                    }
                    *end = ENDUNIT;
                }
            }
            strcpy(computerIDwSpace, oldcomputer);
        }
    }
    FClose(in);
    RemovePendingTopic(topicID);
    FreeBuffer(); // copyBuffer
    FreeBuffer(); // primaryBuffer
    trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
    timing = (modifiedTiming) ? modifiedTimingVal : oldtiming;
}

static void VerifyAllTopics(char kind,char* prepassTopic,char* topic)
{
	size_t len = 0;
	char* x = strchr(topic,'*');
	if (x) len = x - topic ;
	for (int i = 1; i <= numberOfTopics; ++i) 
	{
		if (!*GetTopicName(i)) continue;
		if (len && strnicmp(GetTopicName(i),topic,len)) continue;
		VerifyAccess(GetTopicName(i),kind,prepassTopic);
	}
}

static void AllGambitTests(char* topic)
{
	size_t len = 0;
	char* x = strchr(topic,'*');
	if (x) len = x - topic;
	for (int i = 1; i <= numberOfTopics; ++i) 
	{
		if (!*GetTopicName(i)) continue;
		if (len && strnicmp(GetTopicName(i),topic,len)) continue;
		GambitTestTopic(GetTopicName(i));
	}
}

static void C_Verify(char* input)
{
	char word[MAX_WORD_SIZE];
	char topic[MAX_WORD_SIZE];
	char tokens[MAX_WORD_SIZE];
	trials = 0;
	*topic = 0;
	err = 0;
	char* ptr = SkipWhitespace(input);
	// :verify    or    :verify blocking   or  :verify blocking ~family   or  :verify ~family or :verify sample
	if (*ptr == USERVAR_PREFIX) // tokenize this way
	{
		ptr = ReadCompiledWord(ptr,tokens);
		char* value  = GetUserVariable(tokens);
		int64 flags = 0;
		ReadInt64(value,flags);
		verifyToken = flags;
	}
	else verifyToken = 0; // we dont believe this.
	if (*ptr == '~') ptr = ReadCompiledWord(ptr,topic);  // topic specifier given
	char type = 0;
	while (ptr && *ptr)
	{
		ptr = ReadCompiledWord(ptr,word);
		if (!strnicmp(word,(char*)"pattern",7)) type = 'p';
		else if (!stricmp(word,(char*)"all")) type = 'a';
		else if (!strnicmp(word,(char*)"gambit",6)) type = 'g';
		else if (!strnicmp(word,(char*)"block",5))  type = 'b';
		else if (!strnicmp(word,(char*)"keyword",7)) type = 'k';
		else if (!strnicmp(word,(char*)"sample",6))
		{
			if (!strnicmp(word,(char*)"sampletopic",11)) type = 'S'; // bad topic only
			else type = 's'; // bad topics and rules
		}
		else if (!*topic) // topic name given without ~
		{
			topic[0] = '~';
			strcpy(topic+1,word);
		}
	}
	
	if (type != 'g')
	{
		char prepassTopic[MAX_WORD_SIZE];
		strcpy(prepassTopic,GetUserVariable((char*)"$cs_prepass"));
		if (*topic == '~' && !strchr(topic,'*')) VerifyAccess(topic,type,prepassTopic);
		else VerifyAllTopics(type,prepassTopic,topic);
	}

	// now do gambit tests
	if (type == 'g' || type == 'a')
	{
		if (*topic == '~'  && !strchr(topic,'*')) GambitTestTopic(topic);
		else AllGambitTests(topic);
	}
	Log(STDUSERLOG,(char*)"%d verify findings of %d trials.\r\n",err,trials);
	ResetToPreUser();
}

bool stanfordParser = false;

static void PennWrite(char* name,uint64 flags)
{
	FILE* out = (FILE*)flags;
	FILE* in = fopen(name,(char*)"rb");
	if (!in) 
	{
		(*printer)((char*)"missing %s\r\n",name);
		return;
	}
	bool content = false;
	char* buffer = AllocateBuffer();
	*buffer = 0;
	char* ptr = buffer;
	bool pendingDone = false;
	bool openQuote = false;
	while (ReadALine(readBuffer,in,maxBufferSize,true) >= 0) // read lines, returning empties as well
	{
		char word[MAX_WORD_SIZE];
		ReadCompiledWord(readBuffer,word);
		if (!*word && !stanfordParser) // empty line always separates sentences from Pennbank
		{
			if (content)
			{
				*ptr = 0;
				fprintf(out,(char*)"%s\r\n",buffer);
				ptr = buffer;
				*ptr = 0;
				content = false;
			}
			continue;
		}
		char* at = readBuffer;
		while (at && *at)
		{
			at = ReadCompiledWord(at,word);
			if (pendingDone) // saw a closing, aim to close it if not quote close
			{
				if (*word == '\'' && word[1] == '\'' && word[2] == '/' && openQuote) // close quote 
				{
					strcpy(word,(char*)"\"/\"");
					openQuote = false;
					strcat(ptr,word);
					ptr += strlen(ptr);
					strcat(ptr,(char*)" ");
					++ptr;
					*word = 0;
				}	

				*ptr = 0;
				fprintf(out,(char*)"%s\r\n",buffer);
				ptr = buffer;
				*ptr = 0;
				content = false;
				pendingDone = false;
				if (!*word) continue; // closed quote around this
			}

			if (*word == '`' && word[1] == '`' && word[2] == '/') // open quote 
			{
				strcpy(word,(char*)"\"/\"");
				openQuote = true;
			}
			if (*word == '\'' && word[1] == '\'' && word[2] == '/') // close quote 
			{
				strcpy(word,(char*)"\"/\"");
				openQuote = false;
			}			
			if (*word == '[' || *word == ']') continue;	// ignore this
			if (*word == '=' && word[1] == '=') // ignore ======================================
			{
				if (content)
				{
					*ptr = 0;
					fprintf(out,(char*)"%s\r\n",buffer);
					ptr = buffer;
					*ptr = 0;
					content = false;
				}
				continue; 
			}
			strcat(ptr,word);
			ptr += strlen(ptr);
			strcat(ptr,(char*)" ");
			++ptr;

			if (!content && !stanfordParser)
			{
				if (IsLowerCase(*word)) Log(STDUSERLOG,(char*)"LOWER START? %s in %s \r\n",readBuffer,name);
			}
			content = true;

			if (stanfordParser && (*word == '.' || *word == '?' || *word == '!')) // sentences using stanford parser will end with punctuation UNLESS have quote after that
			{
				pendingDone = true;
			}
		}
	}
	if (content)
	{
		*ptr = 0;
		fprintf(out,(char*)"%s\r\n",buffer);
	}
	FClose(in);
	FreeBuffer();
}

static void C_PennFormat(char* file)
{
	bool rough = true;
	char indir[MAX_WORD_SIZE];
	file = ReadCompiledWord(file,indir); // where source is
	char word[MAX_WORD_SIZE];
	file = ReadCompiledWord(file,word); // where source is
	char outfile[MAX_WORD_SIZE];
	sprintf(outfile,(char*)"REGRESS/PENNTAGS/%s.txt",word); // where output is

	if (!strnicmp(file,(char*)"stanford",8)) stanfordParser = true; // sentences end with . or ! or ?
	FILE* out = FopenUTF8Write(outfile);
	if (!out) return;
	WalkDirectory(indir,PennWrite,(uint64)out,false);
	FClose(out);
}

static void ShowFailCount(WORDP D,uint64 junk)
{
	if (!(D->internalBits & DELETED_MARK)) return;
	Log(STDUSERLOG,(char*)"%s:%d  ",D->word,D->w.planArgCount);
	D->internalBits ^= DELETED_MARK;
	D->w.planArgCount = 0;
}

static void C_TrimDown(char* file)
{
	FILE* out = FopenUTF8Write("tmp/tmp.txt");
	FILE* in = FopenReadOnly(file);
	if (!in)
	{
		Log(STDUSERLOG, (char*)"No such file %s\r\n", file);
		return;
	}
	char word[MAX_WORD_SIZE];
	while (ReadALine(readBuffer, in) >= 0)
	{
		char* at = readBuffer;
		while (*at)
		{
			at = ReadCompiledWord(at, word);
			if (!*word || *word == '~') continue;
			char* tilde = strchr(word+1, '~');
			if (tilde) *tilde = 0;
			fprintf(out, "%s\r\n",word);
		}
	}
	fclose(out);
	fclose(in);
	(*printer)("done\r\n");
}

static void C_CheckList(char* file)
{
	FILE* out = FopenUTF8Write("tmp/tmp.txt");
	FILE* in = FopenReadOnly(file);
	if (!in)
	{
		Log(STDUSERLOG, (char*)"No such file %s\r\n", file);
		return;
	}
	char word[MAX_WORD_SIZE];
	while (ReadALine(readBuffer, in) >= 0)
	{
		char* at = readBuffer;
		while (*at)
		{
			at = ReadCompiledWord(at, word);
			WORDP D = FindWord(word);
			if (D && D->properties & VERB) 
				fprintf(out, "%s\r\n", word);
		}
	}
	fclose(out);
	fclose(in);
	(*printer)("done\r\n");
}

static void C_JA2Starts(char* file)
{
	FILE* out = FopenUTF8Write("tmp/tmp.txt");
	FILE* in = FopenReadOnly(file);
	if (!in)
	{
		Log(STDUSERLOG, (char*)"No such file %s\r\n", file);
		return;
	}
    int n = 0;
    char old[MAX_WORD_SIZE * 10];
    char copy[MAX_WORD_SIZE * 10];
	while (fgets(readBuffer, 10000000, in) != NULL)
	{
		if (!*readBuffer) break;
        strcpy(copy, readBuffer);
        ++n;
		//VA-51382515	legal	family		I have a friend who has a half brother. His father won't let him see his brother. Does my friend have any rights in this matter?
		char* at = strchr(readBuffer, '\t'); // to cat
		if (!at)
		{
			(*printer)("not found delimiter %s\r\n", readBuffer);
			return;
		}
		at = strchr(at+1, '\t'); // to spec
        if (!at) continue;
		at = strchr(at + 1, '\t'); // to loc
        if (!at) continue;
#ifndef LOCATIONIFY
        memmove(at + 1, at, strlen(at)+1);
        *at = '\t';
#endif
        at = strchr(at + 1, '\t'); // to input
        if (!at) continue;
        char* end = strchr(at + 1, '\t'); // to output
        if (!end) strcat(at, "\t");
        else
        {
            char* at = end + 1; // start of output, remove any quotes
            while ((at = strchr(at, '"'))) 
                *at = ' ';
        }
        
        strncpy(old, readBuffer, (MAX_WORD_SIZE * 10) - 3);
        old[(MAX_WORD_SIZE*10) - 3] = 0;
		fprintf(out, "%s", old);
		*readBuffer = 0;
	}
	fclose(out);
	fclose(in);
}

static void C_RemoveCRLF(char* file)
{
    FILE* out = FopenUTF8Write("tmp/tmp.txt");
    FILE* in = FopenReadOnly(file);
    if (!in)
    {
        Log(STDUSERLOG, (char*)"No such file %s\r\n", file);
        return;
    }
    while (fgets(readBuffer, 10000000, in) != NULL)
    {
        char* end;
        int x = 0; // debug break
        while ((end = strchr(readBuffer, '\n'))) *end = ' ';
        while ((end = strchr(readBuffer, '\r'))) *end = ' ';
        fprintf(out, "%s\r\n", readBuffer);
    }
    fclose(out);
    fclose(in);
}

static void C_JA2Source(char* file)
{
    FILE* out = FopenUTF8Write("tmp/tmp.txt");
    FILE* in = FopenReadOnly(file);
    if (!in)
    {
        Log(STDUSERLOG, (char*)"No such file %s\r\n", file);
        return;
    }
    int n = 0;
    char id[MAX_WORD_SIZE];
    char category[MAX_WORD_SIZE];
    char specialty[MAX_WORD_SIZE];
    char location[MAX_WORD_SIZE];
    char input[MAX_WORD_SIZE * 10];
    char copy[MAX_WORD_SIZE * 10];
    while (fgets(readBuffer, 10000000, in) != NULL)
    {
        if (!*readBuffer) break;
        strcpy(copy, readBuffer);
        ++n;
        //VA-51382515	legal	family	alberta 	I have a friend who has a half brother. His father won't let him see his brother. Does my friend have any rights in this matter?
        char* at = strchr(readBuffer, '\t'); // to category
        if (!at)
        {
            (*printer)("not found delimiter %s\r\n", readBuffer);
            return;
        }

        *at = 0;
        strcpy(id, readBuffer);
        char* start = at + 1;
        at = strchr(start, '\t'); // to specialty
        *at = 0;
        strcpy(category, start);
        start = at + 1; 
        at = strchr(start, '\t'); // to loc or beyond
        *at = 0;
        strcpy(specialty, start); 
        *location = 0;
#ifdef LOCALIZE
        start = at + 1; 
        at = strchr(start, '\t'); // to input
        *at = 0;
        strcpy(location, start);
#endif
        strcpy(input, at + 1);

        if (!*specialty) strcpy(specialty, "NULL");
        if (!*location) strcpy(location, "NULL");

        fprintf(out, "[ category: %s specialty: %s location: %s id: %s ]\r\n", category, specialty, location,id);
        fprintf(out, "%s", input);
    }
    fclose(out);
    fclose(in);
}

static void C_PennDecode(char* file)
{
	FILE* in = FopenReadOnly(file); 
	if (!in)
	{
		Log(STDUSERLOG, (char*)"No such file %s\r\n", file);
		return;
	}
	char token[MAX_WORD_SIZE];
	bool start = false;
	while (ReadALine(readBuffer, in) >= 0)
	{
		if (!*readBuffer) continue;
		char* ptr = SkipWhitespace(readBuffer);
		while (*ptr)
		{
			ptr = SkipWhitespace(ptr);
			if (!*ptr) continue;
			char word[MAX_WORD_SIZE];
			if (*ptr == '(')
			{
				start = true;
				++ptr;
			}
			else if (*ptr == ')')
			{
				++ptr;
			}
			else
			{
				ptr = ReadCompiledWord(ptr, word);
				char* end = word + strlen(word);
				while (*--end == ')');
				*++end = 0;
				if (start) strcpy(token, word); // pos tag
				else
				{
					Log(STDUSERLOG, "%s/%s ", word,token);
					if (*token == '.') 
						Log(STDUSERLOG, "\r\n");
				}
				start = false;
			}
		}
	}
	fclose(in);
}

static void C_PennMatch(char* file)
{
	bool rough = true;
	bool tt = false;
	char word[MAX_WORD_SIZE];
	bool raw = false;
	bool ambig = false;
	bool showUsed = false;
	unsigned int ambigLocation = 0;
	char filename[SMALL_WORD_SIZE];
	strcpy(filename,(char*)"REGRESS/PENNTAGS/penn.txt"); // tedtalks
	sprintf(filename, "REGRESS/PENNTAGS/%s.txt",file);
	uint64 startTime = ElapsedMilliseconds(); 
	int sentenceLengthLimit = 0;
	usedTrace = AllocateBuffer();
	unsigned int line = 0;
	bool reveal = false;

reloop:
	FILE* in = FopenReadOnly(filename); // REGRESS/PENNTAGS/
	if (!in) 
	{
		Log(STDUSERLOG,(char*)"No such file %s\r\n",filename);
		return;
	}
	while (*file) // " ambig 1"  or "raw -10" limit to 10 length  or "raw 15 to do sentence 15"
	{
		file = ReadCompiledWord(file,word);
		if (!stricmp(word,(char*)"raw")) raw = true; // original rule-based pos results not lost anything?
		else if (!stricmp(word,(char*)"ambig")) ambig = true; // show ambiguous sentences  - ambig 3
		else if (!stricmp(word,(char*)"show")) 
		{
			showUsed = true; // show sentences where rule matched badly
			echo = true;
			raw = true;
		}
		else if (!stricmp(word,(char*)"reveal")) 
		{
			reveal = true; // show sentences where rule matched
			echo = true;
			raw = true;
		}
		else if (!stricmp(word,(char*)"rule"))
		{
			raw = true;
			ignoreRule = 0;
		}
		else if (IsDigit(*word) || *word == '-') 
		{
			int x;
			ReadInt(word,x);
			if (ambig) ambigLocation = x;
			else sentenceLengthLimit =  x;
			if (sentenceLengthLimit > 0)
			{
				line = sentenceLengthLimit;
				sentenceLengthLimit = 0;
				trace = (unsigned int)-1;
				echo = true;
			}
			else sentenceLengthLimit = - sentenceLengthLimit;
		}
		else sprintf(filename,(char*)"REGRESS/PENNTAGS/%s.txt",word);
	}

	char* buffer = AllocateBuffer();
	char tags[MAX_SENTENCE_LENGTH][20];
	char tokens[MAX_SENTENCE_LENGTH][100];
	char mytags[MAX_SENTENCE_LENGTH][200];
	char prior[MAX_WORD_SIZE];
	int len;
	int right = 0;
	unsigned int total = 0;
	unsigned int sentences = 0;
	quotationInProgress = 0;	
	prepareMode = PENN_MODE;
	unsigned int totalAmbigs = 0;
	unsigned int ambigItems = 0;
	unsigned int parseOK = 0;
	unsigned int parseBad = 0;
	unsigned int ambigSentences = 0;
	UnlockLayer(LAYER_BOOT); // unlock it to add stuff
	StoreWord((char*)"NN");
	StoreWord((char*)"NNS");
	StoreWord((char*)"NNP");
	StoreWord((char*)"NNPS");
	StoreWord((char*)"IN ",AS_IS);
	StoreWord((char*)"PDT");
	StoreWord((char*)"POS");
	StoreWord((char*)"CC");
	StoreWord((char*)"JJ");
	StoreWord((char*)"JJR");
	StoreWord((char*)"JJS");
	StoreWord((char*)"RB");
	StoreWord((char*)"RBR");
	StoreWord((char*)"RBS");
	StoreWord((char*)"MD");
	StoreWord((char*)"RP");
	StoreWord((char*)"DT");
	StoreWord((char*)"PRP$");
	StoreWord((char*)"PRP");
	StoreWord((char*)"VB");
	StoreWord((char*)"VBD");
	StoreWord((char*)"VBG");
	StoreWord((char*)"VBN");
	StoreWord((char*)"VBP");
	StoreWord((char*)"VBZ");
	StoreWord((char*)"WDT");
	StoreWord((char*)"WP");
	StoreWord((char*)"WP$");
	StoreWord((char*)"WRB");
	StoreWord((char*)"CD");
	StoreWord((char*)"EX");
	StoreWord((char*)"FW");
	StoreWord((char*)"SYM"); // includes $
	LockLayer(true);
	ambiguousWords = 0;

	FILE* oldin = NULL;

	while (ReadALine(readBuffer,in)  >= 0 || oldin)
	{
		if (!*readBuffer && !readBuffer[2]) // continue from nested call
		{
			FClose(in);
			in = oldin;
			oldin = NULL;
			continue;
		}
		if (line && currentFileLine != line) continue;
		*usedTrace = 0;
		usedWordIndex = 0;

		char* at = buffer;
		*at = 0;
		char word[MAX_WORD_SIZE];
		char* ptr = SkipWhitespace(readBuffer);
		if (!*ptr || *ptr == '#') continue;
		if (!strnicmp(ptr,(char*)":exit",5)) break;
		if (!strnicmp(ptr,(char*)":include ",9))
		{
			if (oldin) 
			{
				Log(STDUSERLOG,(char*)"Bad include");
				return;
			}
			oldin = in;
			in = FopenReadOnly(ptr+9);  // :include
			if (!in) 
			{
				Log(STDUSERLOG,(char*)"include failed %s\r\n",ptr+9);
				in = oldin;
				oldin = NULL;
			}
			continue;
		}
		if (*ptr == ':') 
		{
			char output[MAX_WORD_SIZE];
			DoCommand(ptr,output);
			continue;
		}
		len = 0;
		int matchedquote = 0;
		bool first = true;
		char* originalPtr = ptr;
		while (ptr && *ptr)
		{
			ptr = ReadCompiledWord(ptr,word);
			if (!*word) break;
			char* sep = strrchr(word,'/'); // find last one (there might be \/  when they actually want token
			if (!sep)
			{
				(*printer)((char*)"Failed %s\r\n",readBuffer);
				break;
			}
			*sep = 0;
			++len;

			// recode \/ and its ilk
			char word1[MAX_WORD_SIZE];
			strcpy(word1,word);
			char* sep1;
			while ((sep1 = strchr(word1,'\\'))) memmove(sep1,sep1+1,strlen(sep1)+1);

			strcpy(tokens[len],word1);
			if (!stricmp(word1,(char*)"'s") && !stricmp((char*)"POS",sep+1) && *(at-1) == ' ') *--at = 0;// possessive vs 's as "is"
			
			if (!stricmp(word1,(char*)"-LRB-")) strcat(at,(char*)"(");
			else if (!stricmp(word1,(char*)"-RRB-")) strcat(at,(char*)")");
			else if (!stricmp(word1,(char*)"-LSB-")) strcat(at,(char*)"[");
			else if (!stricmp(word1,(char*)"-RSB-")) strcat(at,(char*)"]");
			else if (!stricmp(word1,(char*)"-LCB-")) strcat(at,(char*)"{");
			else if (!stricmp(word1,(char*)"-RCB-")) strcat(at,(char*)"}");
			else if (*word1 == '`' && word1[1] == '`') 
			{
				strcat(at,(char*)"\"");  // open quote
				matchedquote |= 1;
			}
			else if (*word1 == '\'' && word1[1] == '\'') 
			{
				if (first) 
					Log(STDUSERLOG,(char*)"Closing quote at start: %d %s \r\n",currentFileLine,originalPtr);
				strcat(at,(char*)"\""); // close quote
				matchedquote |= 2;
			}
			else strcat(at,word1);

			at += strlen(at);
			strcat(at++,(char*)" ");
			strcpy(tags[len],sep+1); // what we expect
			first = false;
		}
		if (matchedquote == 1 && !showUsed)
		{
	//		Log(STDUSERLOG,(char*)"No closing quote: %d %s \r\n",currentFileLine,buffer);
		}
		if (matchedquote == 2 && !showUsed)
		{
		//	Log(STDUSERLOG,(char*)"No opening quote: %d %s \r\n",currentFileLine,buffer);
		}
		if (len == 0) continue; // on to next
		*at = 0;

		// test this sentence
		char* answer1;
		tokenControl = STRICT_CASING | DO_ESSENTIALS | DO_POSTAG | DO_CONTRACTIONS | NO_HYPHEN_END | NO_COLON_END | NO_SEMICOLON_END | TOKEN_AS_IS;
		if (!raw && !ambig && !showUsed) tokenControl |= DO_PARSE;
		ReturnToAfterLayer(LAYER_BOOT, false);
		PrepareSentence(buffer,true,true);
		if (sentenceLengthLimit && (int)wordCount != sentenceLengthLimit) continue; // looking for easy sentences to fix
		int actualLen = len;
		if (*tags[len] == '.' || *tags[len] == '?' || *tags[len] == '!') 
		{
			++right;  // end punctuation is always right
			--actualLen;
		}
		total += len;
		++sentences;
		answer1 = DumpAnalysis(1,wordCount,posValues,(char*)"Tagged POS",false,true); // to debug at
		bool bad;
		if (!showUsed) for (int i = 1; i <= wordCount; ++i) 
		{
			int loc = i;
			if (i == 1 && *wordStarts[i] == '"') ++loc; // ignore quote
			bad = false;
			if (bitCounts[i] != 1)
			{
				bad = true;
				totalAmbigs += bitCounts[i]; // total ambiguity choices
				++ambigItems;
			}
			if (ambig && bad && (!ambigLocation || loc == (int)ambigLocation) )
				Log(STDUSERLOG,(char*)"** AMBIG %d: line: %d %s\r\n",++ambigSentences,currentFileLine,answer1);
		}

		char* xxhold = answer1; // for debugging
		char* answer2 = strchr(answer1,':') + 1;
		int a = 0;
		while (answer2 && *answer2)
		{
			char* close = strstr(answer2,(char*)")  ");
			if (!close) break;
			close[1] = 0;
			strcpy(mytags[++a],answer2);
			close[1] =  ' ';
			answer2 = close+3;
		}
		if (answer1) strcpy(mytags[++a],answer2); // any remnant
		int oldRight = right;
		if ((a-1) != wordCount && !showUsed)
		{
			Log(STDUSERLOG,(char*)"Tag MisCount: %d instead of %d %s \r\n",a,wordCount,buffer);
			while (++a <= wordCount) *mytags[a] = 0;
		}

		if (actualLen != wordCount && !showUsed ) 
		{
			Log(STDUSERLOG,(char*)"MisCount: %d %s \r\n",currentFileLine,buffer);
		}
		strcpy(prior,buffer);
		logged = false;
		if (!reveal) for (int i = 1; i <= wordCount; ++i) // match off the pos values we understand. all others are wrong by definition
		{
retry:
			int ok = right;
			char* sep = strchr(tags[i],'|');
			char* xxoriginalWord = wordStarts[i];
			if (sep) *sep = 0;
		
			if (bitCounts[i] != 1 && (tokenControl & DO_PARSE) == DO_PARSE  ) // did not solve even when parsed
			{
				Log(STDUSERLOG,(char*)"Parse result- Ambiguous %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			} 
#ifdef TREETAGGER
			else if (tt && MatchTag(tags[i], i)) ++right; // match treetagger tag?
			else if (tt && (!showUsed || usedWordIndex == i))
			{
				Log(STDUSERLOG, (char*)"** Bad TTtag %s (%s) word %d(%s) line %d:  %s\r\n", GetTag(i), tags[i], i, wordStarts[i], currentFileLine, buffer);
			}
#endif
			else if (ignoreRule != -1 && !stricmp(wordStarts[i],(char*)"than")) ++right; // special against rule mode
			else if (posValues[i-1] & IDIOM) ++right; // we will PRESUME we are right - he did a lot of harm -- they say of is prep. we say the phrase is adjective
			else if (!stricmp(tags[i],(char*)"-LRB-"))
			{
				if (*wordStarts[i] == '(') ++right;
				else if (!showUsed || usedWordIndex == i) Log(STDUSERLOG,(char*)"** Bad left paren %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"-RRB-"))
			{
				if (*wordStarts[i] == ')') ++right;
				else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad right paren %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"-LSB-"))
			{
				if (*wordStarts[i] == '[') ++right;
				else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad left square bracket %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"-RSB-"))
			{
				if (*wordStarts[i] == ']') ++right;
				else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad right square bracket %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"-LCB-"))
			{
				if (*wordStarts[i] == '{') ++right;
				else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad left curly bracket %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"-RCB-"))
			{
				if (*wordStarts[i] == '}') ++right;
				else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad right curly bracket %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (posValues[i] & IDIOM) ++right;
			else if (!stricmp(tags[i],(char*)"TO")) ++right;	// always correct
			else if (!stricmp(tags[i],(char*)"NN")) 
			{
				if (rough && posValues[i] & NOUN) ++right;
				else if (posValues[i] & (NOUN_SINGULAR | ADJECTIVE_NOUN | NOUN_NUMBER)  && allOriginalWordBits[i] & (NOUN_SINGULAR|NOUN_NUMBER)) ++right;
				else if (posValues[i] & ADJECTIVE_PARTICIPLE && allOriginalWordBits[i] & NOUN_GERUND) ++right; // *drinking straws
				else if (posValues[i] & NOUN_SINGULAR) ++right; // they doubtless dont know it should be lower case
				else if (posValues[i] & ADJECTIVE_NORMAL && allOriginalWordBits[i] & NOUN_SINGULAR) ++right;  //"*expert aim"
				else if (posValues[i] & NOUN_PROPER_SINGULAR) ++right; // "*Pill bugs are good"
				else if (posValues[i] == PARTICLE) ++right; // "take *care of"
				else if (posValues[i-1] == IDIOM) ++right; // "by the *time I got here, she left"
				else if (posValues[i] & NOUN_GERUND && allOriginalWordBits[i] & NOUN_SINGULAR) ++right; // "*spitting is good"
				else if (posValues[i] & PRONOUN_BITS) ++right; // someone, anyone, etc
				else if (!showUsed || (usedWordIndex == i && usedType & (NOUN_SINGULAR|ADJECTIVE_NOUN)))  Log(STDUSERLOG,(char*)"** Bad NN (singular) %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"NNS")) 
			{
				if (rough && posValues[i] & NOUN) ++right; 
				else if (posValues[i] & (NOUN_PLURAL| ADJECTIVE_NOUN)  && allOriginalWordBits[i] & NOUN_PLURAL) ++right;
				else if (posValues[i] & NOUN_NUMBER && canSysFlags[i] & MODEL_NUMBER) ++right; // model numbers
				else if (posValues[i] & NOUN_PROPER_PLURAL) ++right; // they get it wrong
				else if (posValues[i] & PRONOUN_BITS) ++right; // others
				else if (posValues[i] & NOUN_NUMBER) ++right; // 1920s
				else if (posValues[i] & NOUN_SINGULAR && ( allOriginalWordBits[i] &  NOUN_PLURAL || lcSysFlags[i] & NOUN_NODETERMINER)) ++right;
				else if (!showUsed || (usedWordIndex == i && usedType & (NOUN_PLURAL|ADJECTIVE_NOUN)))  Log(STDUSERLOG,(char*)"** Bad NNS (plural) %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"NNP")) // proper singular
			{
				uint64 val;
				if (rough && posValues[i] & NOUN) ++right;
				else if (posValues[i] & (NOUN_PROPER_SINGULAR | ADJECTIVE_NOUN) && allOriginalWordBits[i] & (NOUN_PROPER_SINGULAR | NOUN_SINGULAR)) ++right;
				else if (posValues[i] & NOUN_PROPER_PLURAL && allOriginalWordBits[i] & NOUN_PROPER_SINGULAR) ++right; // we just picked the other side
				else if (posValues[i] & NOUN_NUMBER && canSysFlags[i] & MODEL_NUMBER) ++right; // model numbers
				else if (posValues[i] & NOUN_NUMBER && IsRomanNumeral(wordStarts[i],val)) ++right; //  roman numerals
				else if (posValues[i] & ADJECTIVE_NORMAL && IsUpperCase(*wordStarts[i])) ++right; // things like French can be adjective or noun, we often call them adjectives instead of adjective_noun
				else if (posValues[i] & ADJECTIVE_NORMAL && allOriginalWordBits[i] & NOUN_PROPER_SINGULAR) ++right; // things like French can be adjective or noun, we often call them adjectives instead of adjective_noun
				else if (posValues[i] & NOUN_PROPER_PLURAL) ++right; // if it ended in s like Atomos.
				else if (posValues[i] & NOUN_SINGULAR) ++right; // "Bear had to eat a lot in raw mode
				else if (!showUsed || (usedWordIndex == i && usedType & (NOUN_PROPER_SINGULAR|ADJECTIVE_NOUN)))  Log(STDUSERLOG,(char*)"** Bad NNP (propersingular) %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"NNPS"))  // proper plural
			{
				if (rough && posValues[i] & NOUN) ++right;
				else if (posValues[i] & (NOUN_PROPER_PLURAL| ADJECTIVE_NOUN) && allOriginalWordBits[i] & NOUN_PROPER_PLURAL) ++right;
				else if (posValues[i] & NOUN_PROPER_SINGULAR && allOriginalWordBits[i] & NOUN_PROPER_PLURAL) ++right; // we just picked the other side
				else if (posValues[i] & NOUN_PROPER_SINGULAR) ++right; // confusion like Mercedes which is singualr
				else if (posValues[i] & NOUN_PLURAL) ++right;
				else if (!showUsed ||  (usedWordIndex == i && usedType & NOUN_PROPER_PLURAL))  Log(STDUSERLOG,(char*)"** Bad NNPS (properplural) %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"IN")) 
			{
				if (posValues[i] & (CONJUNCTION_SUBORDINATE|PREPOSITION)) ++right;
				else if (posValues[i] & IDIOM && allOriginalWordBits[i] & PREPOSITION) ++right; // "a couple *of days"
				else if (posValues[i] & PARTICLE && allOriginalWordBits[i] & PREPOSITION) ++right; 
				else if (!showUsed ||  (usedWordIndex == i && usedType & (CONJUNCTION_SUBORDINATE|PREPOSITION)))  
					Log(STDUSERLOG,(char*)"** Bad IN %s word %d(%s) line %d: %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"PDT")) 
			{
				if (posValues[i] & PREDETERMINER) ++right;
				else if (posValues[i] & DETERMINER) ++right; // close enough
				else if (!showUsed || (usedWordIndex == i && usedType & DETERMINER_BITS))  Log(STDUSERLOG,(char*)"** Bad PDT %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"POS")) 
			{
				if (posValues[i] & POSSESSIVE) ++right;
				else if (!showUsed || (usedWordIndex == i && usedType & POSSESSIVE))  Log(STDUSERLOG,(char*)"** Bad POS %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,mytags[i],buffer);
			}
			else if (!stricmp(tags[i],(char*)"LS")) // bullet point
			{
				if (posValues[i] & NOUN_NUMBER) ++right;
				else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad LS %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"CC")) 
			{
				if (posValues[i] & CONJUNCTION_COORDINATE) ++right;
				else if (!showUsed || (usedWordIndex == i && usedType & CONJUNCTION_COORDINATE))  Log(STDUSERLOG,(char*)"** Bad CC %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"JJ")) 
			{
				if (posValues[i] & (ADJECTIVE_NORMAL|NOUN_NUMBER)) ++right;
				else if (posValues[i] & ADJECTIVE_NUMBER && !(allOriginalWordBits[i] & (MORE_FORM|MOST_FORM))) ++right;
				else if (posValues[i] & NOUN_SINGULAR && allOriginalWordBits[i] & NOUN_GERUND  && allOriginalWordBits[i] & ADJECTIVE_NORMAL) ++right; // " *melting point" 
				else if (posValues[i] & NOUN_PROPER_SINGULAR) ++right; // " *Western boots" 
				else if (posValues[i] & VERB_PAST_PARTICIPLE) ++right;
				else if (posValues[i] & (ADJECTIVE_PARTICIPLE | ADJECTIVE_NOUN)) ++right; // "I am *tired"  "*pill bugs eat"
				else if (posValues[i] & NOUN_GERUND ) ++right; // "he got me *moving"
				else if (posValues[i] & NOUN_ADJECTIVE ) ++right; // "he got me *moving"
				else if (posValues[i] & NOUN_SINGULAR && posValues[i+1] & (ADJECTIVE_BITS|NOUN_BITS) && (tokenControl & DO_PARSE) != DO_PARSE ) ++right; // "the *bank teller" when using RAW mode or "*money_market mutual funds"
				else if (posValues[i] & (PREDETERMINER|DETERMINER)) ++right; // "*Other people"  "of *such stature"
				else if (posValues[i] & ADVERB && allOriginalWordBits[i] & ADJECTIVE_NORMAL && posValues[i+1] & ADJECTIVE_BITS) ++right; // we consider them adverbs
				else if (posValues[i] & PARTICLE) ++right; // "take it for *granted"
				else if (posValues[i] & VERB_PRESENT_PARTICIPLE && allOriginalWordBits[i] & ADJECTIVE_BITS) ++right;	// she is *willing to go"
				else if (!showUsed || (usedWordIndex == i && usedType & ADJECTIVE_BITS))  Log(STDUSERLOG,(char*)"** Bad JJ %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"JJR")) 
			{
				if (originalLower[i]  && posValues[i] & ADJECTIVE_NORMAL && allOriginalWordBits[i] & MORE_FORM) ++right;
				else if ( posValues[i] & DETERMINER && allOriginalWordBits[i] & MORE_FORM) ++right;
				else if (!showUsed || (usedWordIndex == i && usedType & ADJECTIVE_BITS))  Log(STDUSERLOG,(char*)"** Bad JJR %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"JJS")) 
			{
				if (originalLower[i] && posValues[i] & ADJECTIVE_NORMAL && allOriginalWordBits[i] & MOST_FORM) ++right;
				else if ( posValues[i] & DETERMINER && allOriginalWordBits[i] & MOST_FORM) ++right;
				else if (!showUsed || (usedWordIndex == i && usedType & ADJECTIVE_BITS))  Log(STDUSERLOG,(char*)"** Bad JJS %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"RB")) 
			{
				if (posValues[i] & ADVERB) ++right;
				else  if (posValues[i] & PARTICLE && allOriginalWordBits[i] & ADVERB)  ++right;
				else if (!showUsed || (usedWordIndex == i && usedType & ADVERB))  Log(STDUSERLOG,(char*)"** Bad RB %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"RBR")) 
			{
				if (posValues[i] & ADVERB && allOriginalWordBits[i] & MORE_FORM) ++right;
				else if (!showUsed ||  (usedWordIndex == i && usedType & ADVERB))  Log(STDUSERLOG,(char*)"** Bad RBR %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"RBS")) 
			{
				if (posValues[i] & ADVERB && allOriginalWordBits[i] & MOST_FORM) ++right;
				else if (!showUsed ||  (usedWordIndex == i && usedType & ADVERB))  Log(STDUSERLOG,(char*)"** Bad RBS %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
		    }
			else if (!stricmp(tags[i],(char*)"UH")) 
			{
				if (posValues[i] & INTERJECTION) ++right;
				else if (wordCount == 1) ++right;	// anything COULD be...
				else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad UH %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
		    }
			else if (!stricmp(tags[i],(char*)"MD")) 
			{
				if (posValues[i] & AUX_VERB) ++right;
				else if (!showUsed ||  (usedWordIndex == i && usedType & AUX_VERB_TENSES))  Log(STDUSERLOG,(char*)"** Bad MD %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"RP")) 
			{
				if (posValues[i] & PARTICLE) ++right;
				else if (posValues[i] & ADVERB) ++right; // who can say if ideomatic particle verb or adverb.... 
				else if (!showUsed || (usedWordIndex == i && usedType & PARTICLE))  Log(STDUSERLOG,(char*)"** Bad RP %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"DT")) 
			{
				if (posValues[i] & DETERMINER_BITS) ++right; // a an the // my her their our your
				else if (posValues[i] & ADJECTIVE_NUMBER) ++right;	// all numbers to us as adjectives might be considered determiners ?????
				else if (posValues[i] & ADJECTIVE_NORMAL && posValues[i-1] == IDIOM) ++right; // "none of *the honey"
				else if (!stricmp(wordStarts[i],(char*)"this") && posValues[i] & PRONOUN_BITS) ++right; 
				else if (!stricmp(wordStarts[i],(char*)"that") && posValues[i] & PRONOUN_BITS) ++right;
				else if (!stricmp(wordStarts[i],(char*)"those") && posValues[i] & PRONOUN_BITS) ++right;
				else if (posValues[i] & ADVERB && posValues[i+1] & PREPOSITION) ++right; // "he walked *all by himself"
				//else if (!stricmp(wordStarts[i],(char*)"every") || !stricmp(wordStarts[i],(char*)"no") || !stricmp(wordStarts[i],(char*)"another")
				//	 || !stricmp(wordStarts[i],(char*)"any") || !stricmp(wordStarts[i],(char*)"some")
				else if (!showUsed || (usedWordIndex == i && usedType & DETERMINER_BITS))   
					Log(STDUSERLOG,(char*)"** Bad DT %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"PRP$")) 
			{
				if (posValues[i] & PRONOUN_POSSESSIVE) ++right;
				else if (!showUsed ||  (usedWordIndex == i && usedType & PRONOUN_POSSESSIVE))  Log(STDUSERLOG,(char*)"** Bad PRP$ %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"PRP")) 
			{
				if (posValues[i] & (PRONOUN_BITS)) ++right;
				else if (!showUsed ||  (usedWordIndex == i && usedType & PRONOUN_BITS))  Log(STDUSERLOG,(char*)"** Bad PRP %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"VB")) // infinitive
			{
				if (rough && posValues[i] & VERB) ++right;
				else if (posValues[i] & (NOUN_INFINITIVE|VERB_INFINITIVE)) ++right;
				else if (posValues[i] & AUX_VERB && allOriginalWordBits[i] &  VERB_INFINITIVE) ++right;  // includes our modals 
				else if (!showUsed ||  (usedWordIndex == i && usedType & (NOUN_INFINITIVE|VERB_INFINITIVE)))  
					Log(STDUSERLOG,(char*)"** Bad VB (infinitive) %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"VBD")) // past
			{
				if (rough && posValues[i] & VERB) ++right;
				else if (posValues[i] & VERB_PAST || (posValues[i] & AUX_VERB &&  allOriginalWordBits[i] &  VERB_PAST) ) ++right;  // includes our modals that can have this tense as verbs
				else if (posValues[i] & VERB_PAST_PARTICIPLE && allOriginalWordBits[i] & VERB_PAST) ++right; 
				else if (!showUsed || (usedWordIndex == i && usedType & VERB_PAST))  Log(STDUSERLOG,(char*)"** Bad VBD (past) %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"VBG"))  // gerund present participle
			{
				if (rough && posValues[i] & VERB) ++right;
				else if (allOriginalWordBits[i] & (VERB_PRESENT_PARTICIPLE|NOUN_GERUND)) ++right;  // includes our modals that can have this tense as verbs
				else if (!showUsed || (usedWordIndex == i && usedType & (VERB_PRESENT_PARTICIPLE|NOUN_GERUND)))   Log(STDUSERLOG,(char*)"** Bad VBG (present participle) %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
		    }
			else if (!stricmp(tags[i],(char*)"VBN")) // past particple
			{
				if (rough && posValues[i] & VERB) ++right;
				else if (posValues[i] & VERB_PAST_PARTICIPLE || ( posValues[i] & AUX_VERB && allOriginalWordBits[i] & (VERB_PAST_PARTICIPLE|VERB_PRESENT_PARTICIPLE))) ++right;  // includes our modals that can have this tense as verbs - he has said
				else if (posValues[i] & (ADJECTIVE_PARTICIPLE|ADJECTIVE_NORMAL|NOUN_ADJECTIVE) && allOriginalWordBits[i] & VERB_PAST_PARTICIPLE) ++right;
				else if (!showUsed || (usedWordIndex == i && usedType & VERB_PAST_PARTICIPLE))   Log(STDUSERLOG,(char*)"** Bad VBN (past participle) %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"VBP")) // present
			{
				if (rough && posValues[i] & VERB) ++right;
				else if (posValues[i] & VERB_PRESENT || (posValues[i] & AUX_VERB && allOriginalWordBits[i]  &  VERB_PRESENT)) ++right;  // includes our modals that can have this tense as verbs
				else if (!showUsed || (usedWordIndex == i && usedType & VERB_PRESENT))  Log(STDUSERLOG,(char*)"** Bad VBP (present) %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"VBZ")) // 3ps
			{
				if (rough && posValues[i] & VERB) ++right;
				else if (posValues[i] & VERB_PRESENT_3PS || (posValues[i] & AUX_VERB && allOriginalWordBits[i]  &  VERB_PRESENT_3PS)) ++right; // includes our modals that can have this tense as verbs
				else if (!showUsed || (usedWordIndex == i && usedType & VERB_PRESENT_3PS))  Log(STDUSERLOG,(char*)"** Bad VBZ (present_3ps) %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"WDT")) 
			{
				if (!stricmp(wordStarts[i],(char*)"that") || !stricmp(wordStarts[i],(char*)"what") ||!stricmp(wordStarts[i],(char*)"whatever") ||!stricmp(wordStarts[i],(char*)"which") ||!stricmp(wordStarts[i],(char*)"whichever"))
				{ 
					if (posValues[i] & (DETERMINER|PRONOUN_BITS|CONJUNCTION_SUBORDINATE)) ++right; // what dog is that
					else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad WDT %s word %d(%s) line %d: %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
				}
				else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad WDT %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"WP")) 
			{
				// that may be WDT
				if ( !stricmp(wordStarts[i],(char*)"what") || !stricmp(wordStarts[i],(char*)"who") || !stricmp(wordStarts[i],(char*)"whom"))
				{ // that whatever which WDT - whatsoever RB -  whosoever NN
					if (posValues[i] & (PRONOUN_BITS|CONJUNCTION_SUBORDINATE | DETERMINER | PREDETERMINER)) ++right; // what is that
					else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad WP %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
				}
				else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad WP %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"WP$")) 
			{
				if (!stricmp(wordStarts[i],(char*)"whose"))
				{
					if (posValues[i] & (PRONOUN_POSSESSIVE | DETERMINER)) ++right; // whose dog is that -- do we do both? or only one?
					else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad WP$ %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
				}
				else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad WP$ %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"WRB")) 
			{
				if (!stricmp(wordStarts[i],(char*)"how")  ||!stricmp(wordStarts[i],(char*)"whenever") ||!stricmp(wordStarts[i],(char*)"when") ||!stricmp(wordStarts[i],(char*)"where")
					||!stricmp(wordStarts[i],(char*)"whereby")||!stricmp(wordStarts[i],(char*)"wherein")||!stricmp(wordStarts[i],(char*)"why"))
				{
					// the ONLY exception is  "when" meaning "if" should be IN.  
					++right; 
					// however, whence, wherever, whereof are NOT wrb?
				}
				else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad WRB %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!IsAlphaUTF8(*tags[i]) || !stricmp(tags[i],(char*)"SYM") ) 
			{
				++right;	// all punctuation must be right
			}
			else if (!stricmp(tags[i],(char*)"CD")) 
			{
				if (posValues[i] & (NOUN_NUMBER | ADJECTIVE_NUMBER)) ++right;
				else if (posValues[i] & NOUN_PLURAL && IsDigit(*wordStarts[i])) ++right; // 1960s
				else if (posValues[i] & NOUN_PROPER_SINGULAR && FindWord(wordStarts[i],0,LOWERCASE_LOOKUP) && FindWord(wordStarts[i],0,LOWERCASE_LOOKUP)->properties & NOUN_NUMBER) ++right;
				else if (allOriginalWordBits[i] & NOUN_NUMBER) ++right;  // one as pronoun sometimes
				else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad CD %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"EX")) 
			{
				if (posValues[i]  &  THERE_EXISTENTIAL) ++right;
				else if (!showUsed ||  (usedWordIndex == i && usedType & THERE_EXISTENTIAL))  Log(STDUSERLOG,(char*)"** Bad EX %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!stricmp(tags[i],(char*)"FW")) 
			{
				if (strstr(mytags[i],(char*)"unknown-word") || allOriginalWordBits[i] & FOREIGN_WORD) ++right;
				else if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad FW %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}
			else if (!sep) if (!showUsed || usedWordIndex == i)  Log(STDUSERLOG,(char*)"** Bad Unknown tag word %d(%s) line %d: %s %s\r\n",i,wordStarts[i],currentFileLine,tags[i],buffer);

			// composite choices
			if (sep && right == ok) // didn't match it yet
			{
				memmove(tags[i],sep+1,strlen(sep+1) + 1);
				goto retry;
			}

			if (stricmp(tags[i],(char*)"RP") && posValues[i] & PARTICLE && bitCounts[i] == 1 && right == ok) // things we thought were particles that werent counted as right
			{
				int at = i;
				while (--at >= 1 && !(posValues[at] & (VERB_BITS|NOUN_INFINITIVE|NOUN_GERUND|ADJECTIVE_PARTICIPLE))){;} // find verb linked across any sentence fragment
				if (!(posValues[at] & (VERB_BITS|NOUN_INFINITIVE|NOUN_GERUND|ADJECTIVE_PARTICIPLE))) // should NOT HAPPEN - we MUST find or particle option would have been removed
				{
					 if (!showUsed || usedWordIndex == i) Log(STDUSERLOG,(char*)"** Faulty RP Cannot find verb before particle %s word %d(%s) line %d:  %s\r\n",mytags[i],i,wordStarts[i],currentFileLine,buffer);
				}
				char word[MAX_WORD_SIZE];
				*word = 0;

				// FIRST, assume they are contiguous
				strcat(word,wordCanonical[at]);
				strcat(word,(char*)"_");
				while (++at <= i)
				{
					strcat(word,wordStarts[at]);
					if (at != i) strcat(word,(char*)"_");
				}
				WORDP X = FindWord(word);
				if (X) 
				{
					if (X->systemFlags & VERB_DIRECTOBJECT) strcat(word,(char*)" directobj ");
					if (X->systemFlags & VERB_NOOBJECT) strcat(word,(char*)" noobj ");
				}
				else // assume they are discontiguous
				{
					*word = 0;
					strcat(word,wordCanonical[at]);
					strcat(word,(char*)"_");
					strcat(word,wordStarts[i]);
				}
				 if (!showUsed || usedWordIndex == i) Log(STDUSERLOG,(char*)"** Faulty RP %s %s word %d(%s) line %d:  %s\r\n",word,mytags[i],i,wordStarts[i],currentFileLine,buffer);
			}		

			// it was considered wrong
			if (ok == right)
			{
				WORDP X = FindWord( stricmp(tags[i],(char*)"IN") ? tags[i] : (char*)"IN ");
				if (X)
				{
					X->w.planArgCount++;
					X->internalBits |= DELETED_MARK;
				}
			}
		}
		if (showUsed && *usedTrace && logged) {
			Log(STDUSERLOG,(char*)"** USED: line: %d %s\r\n",currentFileLine,answer1);
		}
		if (reveal && *usedTrace) {
			Log(STDUSERLOG,(char*)"** USED: line: %d %s\r\n",currentFileLine,answer1);
			continue;
		}
		if ((tokenControl & DO_PARSE) == DO_PARSE ) 
		{
			if ((right-oldRight) != wordCount){;} // pos is bad so parse is by definition bad
			else if (tokenFlags & FAULTY_PARSE && !(tokenFlags & NOT_SENTENCE)) 
			{
				Log(STDUSERLOG,(char*)"** Faulty parse %d words line %d: %s\r\n",wordCount,currentFileLine,buffer);
				++parseBad;
			}
			else ++parseOK;

			// verify plurality and determined
			unsigned int subject = 0;
			for (int i = startSentence; i <= endSentence; ++i)
			{
				if (roles[i] & (SUBJECT2|MAINSUBJECT)) subject = i;
				if (roles[i] & (VERB2|MAINVERB))
				{
					if (subject && posValues[subject] == NOUN_SINGULAR && posValues[i] == VERB_PRESENT && !(lcSysFlags[i] & NOUN_NODETERMINER)) Log(STDUSERLOG,(char*)"*** Warning singular noun %s to plural verb %s in %s\r\n",wordStarts[subject],wordStarts[i],buffer);
					if (subject && posValues[subject] == NOUN_PLURAL && posValues[i] == VERB_PRESENT_3PS) Log(STDUSERLOG,(char*)"*** Warning plural noun %s to singular verb %s in %s\r\n",wordStarts[subject],wordStarts[i],buffer);
					if (subject && posValues[subject] == PRONOUN_BITS && posValues[i] == VERB_PRESENT && !(lcSysFlags[i] & PRONOUN_SINGULAR)) Log(STDUSERLOG,(char*)"*** Warning singular pronoun %s to plural verb %s in %s\r\n",wordStarts[subject],wordStarts[i],buffer);
					if (subject && posValues[subject] == PRONOUN_BITS && posValues[i] == VERB_PRESENT_3PS && lcSysFlags[i] & PRONOUN_SINGULAR) Log(STDUSERLOG,(char*)"*** Warning plural pronoun %s to singular verb %s in %s\r\n",wordStarts[subject],wordStarts[i],buffer);
					subject = 0;
				}
				if (roles[i] & (SUBJECT2|MAINSUBJECT|OBJECT2|MAINOBJECT|INDIRECTOBJECT2|MAININDIRECTOBJECT) && posValues[i] & NOUN_SINGULAR && originalLower[i] && !(originalLower[i]->properties & PRONOUN_BITS) &&  !(lcSysFlags[i] & NOUN_NODETERMINER) )
				{
					int det;
					if (!IsDeterminedNoun(i,det)) Log(STDUSERLOG,(char*)"   *** Warning undetermined noun %s in %s\r\n",wordStarts[i],buffer);
				}
			}
		}
	}
	FClose(in);
	FreeBuffer();
	if (ignoreRule >= 0 && ignoreRule < (int) tagRuleCount) 
	{
		if (total-right)
		{
			Log(STDUSERLOG,(char*)"Rule fail: %s\r\n",comments[ignoreRule]);
		}
		++ignoreRule;
		goto reloop;
	}
	double percentRight = ((double)right * 100) /total;
	double percent = ((double)parseBad * 100) /sentences;
	int val1 = (int)percent;
	percent = ((double)ambigItems * 100) /ambiguousWords;
	int val2 = (int) percent;
	percent = ((double)ambiguousWords * 100) /total;
	int val3 = (int) percent;

	unsigned long timediff =  (unsigned long)( ElapsedMilliseconds() - startTime); 
	unsigned int tokensec = (timediff) ? ((total * 1000) / timediff) : 0; 
	FreeBuffer();

	Log(STDUSERLOG, (char*)"\r\nambigWords:%d  wrong:%d  notWrong:%d total:%d",
		ambigItems, total - right, right, total);
	Log(STDUSERLOG, (char*)" percent:%f sentences:%d parsed:%d parseBad:%d badSentencePercent:%d initialAmbiguousWords:%d percentambigLeft:%d initialAmbigpercent:%d\r\n\r\n",
		(float)percentRight, sentences, parseOK, parseBad, val1, ambiguousWords, val2, val3);
	if (!raw && !ambig) Log(STDUSERLOG,(char*)"parsed:%d parseBad:%d badSentenceRate:%d initialAmbiguousWords:%d percentambigLeft:%d initialAmbigpercent:%d\r\n\r\n",parseOK,parseBad,val1,ambiguousWords,val2,val3);
	WalkDictionary(ShowFailCount,0);
	Log(STDUSERLOG,(char*)"\r\n");
	ignoreRule = -1;
	if (line) 
	{ 
		trace = 0; 
		echo = false;
	}
	ReturnToAfterLayer(LAYER_BOOT, false);
}

static void C_PennNoun(char* file)
{
	char word[MAX_WORD_SIZE];
	file = ReadCompiledWord(file,word);
	char filename[SMALL_WORD_SIZE];
	if (*word) sprintf(filename,(char*)"REGRESS/PENNTAGS/%s.txt",word);
	else strcpy(filename,(char*)"REGRESS/PENNTAGS/penn.txt");  // REGRESS/PENTAGS
	FILE* in = FopenReadOnly(filename);
	if (!in) return;
	char* buffer = AllocateBuffer();
	char tags[MAX_SENTENCE_LENGTH][20];
	char tokens[MAX_SENTENCE_LENGTH][100];
	unsigned int len;
	while (ReadALine(readBuffer,in) >= 0)
	{
		char* at = buffer;
		*at = 0;
		char word[MAX_WORD_SIZE];
		char* ptr = SkipWhitespace(readBuffer);
		if (!*ptr || *ptr == '#') continue;
		len = 0;
		while (ptr && *ptr)
		{
			ptr = ReadCompiledWord(ptr,word);
			if (!*word) break;
			char* sep = strrchr(word,'/'); // find last one (there might be \/  when they actually want token
			if (!sep)
			{
				(*printer)((char*)"Failed %s\r\n",readBuffer);
				break;
			}
			*sep = 0;
			++len;

			// recode quotes (opening and closing)
			if (*word == '`' && word[1] == '`') strcpy(word,(char*)"\"");
			if (*word == '\'' && word[1] == '\'') strcpy(word,(char*)"\"");
			// recode \/ and its ilk
			char word1[MAX_WORD_SIZE];
			strcpy(word1,word);
			char* sep1;
			while ((sep1 = strchr(word1,'\\'))) memmove(sep1,sep1+1,strlen(sep1)+1);

			strcpy(tokens[len],word1);
			strcat(at,word1);
			at += strlen(at);
			strcat(at++,(char*)" ");
			strcpy(tags[len],sep+1); // what we expect
		}
		if (len == 0) continue; // on to next

		*at = 0;
		for (unsigned int i = 1; i <= len; ++i) // match off the pos values we understand. all others are wrong by definition
		{
			char* sep = strchr(tags[i],'|');
			if (sep) *sep = 0;

			if (!stricmp(tags[i],(char*)"NN")) // found a noun, look backwards...
			{
				if (!strnicmp(tags[i+1],(char*)"NN",2)) continue;	 // noun follows us. he must be determined instead
				for (unsigned int x = i-1; x >= 1; --x)
				{
					if (!stricmp(tokens[x],(char*)",(char*)")) break;	 // immediately after comma may be appositive "Bob, dog of my dreams
					if (!stricmp(tokens[x],(char*)"of")) break;	 // can say of xxx always as in type of dog
					if (!stricmp(tags[x],(char*)"CC")) break;	// assume guy before is determeined
					if (!stricmp(tags[x],(char*)"DT")) break;	// it is determined
					if (!stricmp(tags[x],(char*)"POS")) break;	// is owned
					if (tags[x][0] == 'N') continue;	// it is joined noun.
					if (*tags[x] == 'J') continue; // adj
					if (!stricmp(tags[x],(char*)"PRP$")) break; // word after conjunct
					WORDP D = FindWord(tokens[i]);
					if (IsUpperCase(*tokens[i])) break; // actually not NN
					if (D && !IsAlphaUTF8(*D->word)) break;	 // not a normal word
					if (D && D->systemFlags & NOUN_NODETERMINER)
						break;
					Log(STDUSERLOG,(char*)"%s: %s %s  %s\r\n",tokens[i],tags[x],tokens[x], buffer); // unxpected
					break;
				}
			}
		}
	}
	FClose(in);
	FreeBuffer();
}

static void C_VerifyPos(char* file)
{
	if (!*file) file = "REGRESS/postest.txt";
	FILE* in = FopenReadOnly(file);  // REGRESS/postest
	if (!in) return;

	uint64 start = ElapsedMilliseconds();
	
	prepareMode = POSVERIFY_MODE;
	uint64 oldtokencontrol = tokenControl;
	tokenControl =  DO_PARSE | DO_ESSENTIALS| DO_CONTRACTIONS | DO_BRITISH  | STRICT_CASING | DO_NUMBER_MERGE | DO_PROPERNAME_MERGE; 
	unsigned int tokens = 0;
	unsigned int count = 0;
	unsigned int fail = 0;
	char sentence[MAX_WORD_SIZE];
	while (ReadALine(readBuffer,in) >= 0)
	{
		char* ptr =  SkipWhitespace(readBuffer);
		if (!strnicmp(ptr,(char*)"#END",4)) break;
		if (!*ptr || *ptr == '#') continue;
		// debug command
		if (*ptr == ':' && IsAlphaUTF8(ptr[1]))
		{
			char output[MAX_WORD_SIZE];
			DoCommand(ptr,output);
			continue;
		}
		ReturnToAfterLayer(LAYER_1,false); // dont let dictionary tamper affect this. A problem with ANY multiple sentence input...
	
		++count;
		strcpy(sentence,ptr);
		PrepareSentence(sentence,true,true);
		tokens += wordCount;
		char parseForm[MAX_WORD_SIZE * 5];
		*parseForm = 0;
		char liveParse[MAX_WORD_SIZE * 5];
		*liveParse = 0;
		strcpy(liveParse,DumpAnalysis(1,wordCount,posValues,(char*)"Parsed POS",false,true));
		TrimSpaces(liveParse,false);
		while (ReadALine(readBuffer,in) >= 0)
		{
			char* start = SkipWhitespace(readBuffer);
			if (!*start || *start == '#') continue;
			if (!strnicmp(start,(char*)"Parsed",6)) 
			{
				strcpy(parseForm,TrimSpaces(start,false)); 
				break;
			}
		}

		if (strcmp(parseForm,liveParse))
		{
			size_t i;
			for (i = 0; i < strlen(parseForm); ++i)
			{
				if (parseForm[i] != liveParse[i]) break;
				if (!parseForm[i] || !liveParse[i]) break;
			}
			while (i && parseForm[--i] != '(');	// find start backwards
			if (i) --i;
			while (i && parseForm[--i] != ' ');	
			char hold[BIG_WORD_SIZE];
			strcpy(hold,parseForm+i);
			parseForm[i] = 0;
			strcat(parseForm,(char*)"\r\n--> ");
			strcat(parseForm,hold);
			char hold1[BIG_WORD_SIZE];
			*hold1 = 0;
			size_t len = strlen(liveParse);
			if ( len > i) strcpy(hold1,liveParse+i);
			liveParse[i] = 0;
			strcat(liveParse,(char*)"\r\n--> ");
			strcat(liveParse,hold1);
			Log(STDUSERLOG,(char*)"\r\nMismatch at %d: %s\r\n",count,sentence);
			Log(STDUSERLOG,(char*)"          got: %s\r\n",liveParse);
			Log(STDUSERLOG,(char*)"         want: %s\r\n",parseForm);
			int oldtrace = trace;
			trace |= TRACE_POS;
			PrepareSentence(sentence,true,true);
			trace = oldtrace;
			++fail;
		}
	}

	FClose(in);

	Log(STDUSERLOG,(char*)"%d sentences tested, %d failed doing %d tokens in %d ms\r\n",count,fail,tokens, (int)(ElapsedMilliseconds() - start));
	tokenControl = oldtokencontrol;
	prepareMode = NO_MODE; 
}

static void C_TimePos(char* file) // how many wps for pos tagging
{
	if (!*file) file = "RAWDICT/postiming.txt";
	FILE* in = fopen(file,(char*)"rb");
	if (!in) return;
	prepareMode = POSTIME_MODE;
	uint64 oldtokencontrol = tokenControl;
	tokenControl = DO_PARSE | DO_SUBSTITUTE_SYSTEM  | DO_NUMBER_MERGE | DO_PROPERNAME_MERGE ;
	posTiming = 0;
	unsigned int words = 0;
	while (ReadALine(readBuffer,in) >= 0)
	{
		char* ptr =  SkipWhitespace(readBuffer);
		if (!*ptr || *ptr == '#') continue;
		if (!strnicmp(ptr,(char*)"Tagged",6)) continue; 
		PrepareSentence(ptr,true,true);
		words += wordCount;
	}

	FClose(in);
	double wps = (double)words / ((double)posTiming/(double)1000.0);
	Log(STDUSERLOG,(char*)"%d words tagged in %d ms wps: %d.\r\n",words,posTiming, (unsigned int) wps);
	tokenControl = oldtokencontrol;
	prepareMode = NO_MODE; 
}

static void C_VerifySpell(char* file) // test spell checker against a file of entries  wrong-spell rightspell like livedata/spellfix.txt
{ 
    if (!*file) file = "LIVEDATA/ENGLISH/SUBSTITUTES/spellfix.txt";
	FILE* in = fopen(file,(char*)"rb");
	if (!in) return;
	unsigned int right = 0;
	unsigned int wrong = 0;
    unsigned int unfix = 0;
	while (ReadALine(readBuffer,in) >= 0)
	{
		// pull out the wrong and right words
		char wrongWord[MAX_WORD_SIZE];
		char rightWord[MAX_WORD_SIZE];
		char* ptr = SkipWhitespace(readBuffer);
		if (*ptr == 0 || *ptr == '#' || *ptr == '<' || *ptr == '\'' || IsDigit(*ptr)) continue; // unusual stuff
		ptr = ReadCompiledWord(ptr,wrongWord);
		if (strchr(wrongWord,'>') || strchr(wrongWord,'.') || strchr(wrongWord,',')) continue;  // unusual stuff
		ReadCompiledWord(ptr,rightWord);
		if (!*rightWord || strchr(rightWord,'+') || *rightWord == '~'  || *rightWord == '%') continue;  // unusual stuff
        int diff = strlen(wrongWord) - strlen(rightWord);
        diff *= diff;
        if (diff > 1) continue;

		WORDP D = FindWord(wrongWord);
		if (D && D->properties & (PART_OF_SPEECH|FOREIGN_WORD)) // already has a meaning
		{
			Log(STDUSERLOG,(char*)"%s already in dictionary\r\n",wrongWord);
			continue;
		}
  
		char* fix = SpellFix(wrongWord,1,PART_OF_SPEECH); 
		if (fix && !strcmp(fix,rightWord)) ++right;
        else if (!fix)
        {
//          Log(STDUSERLOG, (char*)"%s wanted %s but went unfixed\r\n", wrongWord, rightWord);
            ++unfix;
        }
        else
		{
			Log(STDUSERLOG,(char*)"Bad fix:\r\n    %s wrong\r\n    %s right\r\n    %s fix %s\r\n",wrongWord,rightWord,fix, multichoice ? "ambigchoice" : "");
			++wrong;
		}
	}

	FClose(in);
	Log(STDUSERLOG,(char*)"Right:%d  Wrong:%d Unfix:%d\r\n",right,wrong,unfix);
}

static void VerifySubstitutes1(WORDP D, uint64 unused)
{
	if (!(D->internalBits & HAS_SUBSTITUTE)) return;

	char expectedText[MAX_WORD_SIZE];
	char resultText[MAX_WORD_SIZE];
	*readBuffer = 0;
	int n;

	//   see if word has start or end markers. Remove them.
	bool start = false;
	if (*D->word == '<')
	{
		start = true;
		n = BurstWord(D->word+1);
	}
	else n = BurstWord(D->word);
	bool end = false;
	char* last = GetBurstWord(n-1);
	size_t len = strlen(last);
	if (last[len-1] == '>')
	{
		end = true;
		last[len-1] = 0;
	}

	//   now composite an example, taking into account start and end markers
	int i;
	if (!start) strcat(readBuffer,(char*)"x ");	//   so match is not at start
	for (i = 0; i < n; ++i)
	{
		strcat(readBuffer,GetBurstWord(i));
		strcat(readBuffer,(char*)" ");
	}
	if (!end) strcat(readBuffer,(char*)"x "); //   so match is not at end

	//   generate what it results in
	PrepareSentence(readBuffer,true,true);

	*resultText = 0;
	if (!end) --wordCount;	//   remove the trailing x
	for (i = 1; i <= wordCount; ++i) //   recompose what tokenize got
	{
		if (!start && i == 1) continue;	//   remove the leading x
		strcat(resultText,wordStarts[i]);
		strcat(resultText,(char*)" ");
	}

	WORDP S = GetSubstitute(D);
	if (!S && wordCount == 0) return;	//   erased just fine
	if (!S) Log(STDUSERLOG,(char*)"Substitute failed: %s didn't erase itself, got %s\r\n",D->word,resultText);
	else
	{
		strcpy(expectedText,S->word);
		strcat(expectedText,(char*)" ");	//   add the trailing blank we get from concats above
		char* at;
		while ((at = strchr(expectedText,'+'))) *at = ' '; //   break up answer
		if (!stricmp(resultText,expectedText)) return;	//   got what was expected
		Log(STDUSERLOG,(char*)"Substitute failed: %s got %s not %s\r\n",D->word,resultText,expectedText);
	}
}

static void C_VerifySubstitutes(char* ptr) //   see if substitutes work...
{
	WalkDictionary(VerifySubstitutes1);
}

static bool StripEmbedded(char* word,char* &ptr,char* original,char* ref,char* notref,bool &control,bool keep)
{// strip embedded ref
	if (strstr(word,notref)) // contains an end
	{
		char* at = strstr(original,notref);
		if (at == original) // current token starts with it, end mode
		{
			ptr = original + strlen(notref); // skip over it to try again
			control = false;
		}
		else // separate old from new break
		{
			memmove(at+1,at,strlen(at)+1); // make room to separate off token
			*at = ' ';
			ptr = original; // try it again picking off old token then the ender
		}
		return true;
	}
	else if (control) return !keep; // continuing coverage
	else if (strstr(word,ref)) // starter coverage
	{
		char* at = strstr(original,ref); // find where it begins
		if (at == original) // start is it, ignoring or keeping hereafter til later close
		{
			control = !keep;
			ptr = original + strlen(ref);
			return true;
		}
		else // separate old from new break
		{
			memmove(at+1,at,strlen(at)+1); // make room to separate off token
			*at = ' ';
			ptr = original;
		}
		return true;
	}
	else return false;
}

static bool FlushEmbedded(char* & ptr,char* ref,char* notref,unsigned int &control)
{// strip embedded ref
	char* start;
	char* end;
	size_t endlen = strlen(notref);
	size_t startlen = strlen(ref);
	bool closedsqui = false;
	while ((end = strstr(ptr,notref))) // ending coverage exists on input?
	{
		*end = 0;	// trucate for a moment
		char* last = NULL;
		start = ptr - startlen;
		while ((start = strstr(start+startlen,ref)))  last = start; // find last before
		if (last) // here is the corresponding start
		{
			memmove(last,end+endlen,strlen(end+endlen-1)); // erase all content between
			if (*ref == '{' ) closedsqui = true;
			continue;
		}

		// we have an end from a prior one to close off
		if (control)
		{
			ptr = end + endlen; // skip over it to try again
			if (*ref == '{' ) closedsqui = true;
			--control;
		}
		else break; // unknown close
	}

	if ((start = strstr(ptr,ref)) != NULL) // starter coverage exists in input but has no end for it
	{
		// has start not closed must wait but may have useful stuff before it! 
		if (ptr != start) // useful stuff before it?
		{
			*start = 0;
			return false;		// allow processing of this
		}
		return true; // line is garbage
	}
	if (closedsqui && *ptr == ';') // some {{}} have ; after them for no reason?   ayn rand article for example after her pronounciation
		++ptr;
	return (control != 0 || *ptr == 0); // line is ok as it stand or we must flush it
}

static void C_WikiText(char* ptr)
{ // fromfile directory, size
	char file[SMALL_WORD_SIZE];
	char directoryout[MAX_WORD_SIZE];
	int size = 100000;
	bool split = false;
	ptr = ReadCompiledWord(ptr,file);
	ptr = ReadCompiledWord(ptr,directoryout);
	size_t len = strlen(directoryout);
	if (directoryout[len-1] == '/' || directoryout[len-1] == '\\') directoryout[len-1] = 0;	// remove ending /
	if (IsDigit(*ptr))
	{
		ptr = ReadInt(ptr,size);	 // 1kb chunks
		size *=  1000;
		if (!stricmp(ptr,(char*)"split")) split = true;
	}
	unsigned char bulletchar[5];
	bulletchar[0] = 0xe2; bulletchar[1] = 0x80; bulletchar[2] = 0xa2; bulletchar[3] = 0;  // � 

	unsigned int id = 0;
	char outfile[MAX_WORD_SIZE];
	sprintf(outfile,(char*)"%s/file%d.txt",directoryout,id); // the output file
	FILE* out = FopenUTF8Write(outfile);
	if (!out)
	{
		Log(STDUSERLOG,(char*)"Cannot open directory %s\r\n",directoryout);
		return;
	}
	else Log(STDUSERLOG,(char*)"Starting file: %s\r\n", outfile);

	FILE* in = NULL;
	unsigned int inputid = 0;
	bool inputdirectory = true;
	char word[MAX_WORD_SIZE];
	bool page = false;
	unsigned int countSquiggle = 0;
	bool text = false;
	bool title = false;
	char titlename[MAX_WORD_SIZE];
	char content[MAX_WORD_SIZE];
	char heading[MAX_WORD_SIZE];
	unsigned int lines = 0;
	bool pendingtextclose = false;
	int table = 0;
	bool header = true;
	bool superscript = false;
	bool subscript = false;
	bool italic = false;
	bool center = false;
	bool bold = false;
	bool bullet = false;
	bool killheading = false;
	unsigned int citex = 0;
	unsigned int includex = 0;
	unsigned int galleryx = 0;
	unsigned int mathx = 0;
	unsigned int prex = 0;
	unsigned int span = 0;
	unsigned int squ = 0;
	len = 0;
	while (inputdirectory)
	{
		size_t lent = strlen(file);
		if (file[lent-1] != '/') // read 1 file
		{
			in = FopenReadNormal(file); // WIKITEXT
			inputdirectory = false;
			if (!in) 
			{
				Log(STDUSERLOG,(char*)"No such file %s\r\n",file);
				break;
			}
		}
		else // read multiple files
		{
			char filex[MAX_WORD_SIZE];
			sprintf(filex,(char*)"%sfile%d.txt",file,inputid);
			in = FopenReadNormal(filex); // WIKITEXT
			if (!in) break; // end of files in directory
			Log(STDUSERLOG,(char*)"Reading %s\r\n",filex);
			++inputid;
		}

		bool paragraph = false;
		int len1;
	while ((len1 = ReadALine(readBuffer,in)) >= 0)
	{	
		if (!strncmp(readBuffer,(char*)"xxmarkxx",8)) // debug marker during testing
		{
			continue;
		}
		++lines;
		if (split)
		{
			if (strstr(readBuffer,(char*)"<page")) // skip to start of a page
			{
				if ((int)len > size) // start in new file, this file is getting too big
				{
					FClose(out);
					++id;
					sprintf(outfile,(char*)"%s/file%d.txt",directoryout,id); // the output file
					out = FopenUTF8Write(outfile);
					len = 0;
					Log(STDUSERLOG,(char*)"Starting file: %s\r\n", outfile);
				}
			}
			fprintf(out,(char*)"%s\r\n",readBuffer);
			len += strlen(readBuffer) + 2;
			continue;
		}
		if (len1 > 40000) // machine generated garbage
		{
			text = page = false;
			continue;
		}

		if (text && *content) 	// MAYBE NOT
		{
			fprintf(out,(char*)"%s\r\n",content);
			len += strlen(content) + 2;
			*content = 0;
		}
		
		char* at;
		char* ptr = SkipWhitespace(readBuffer);
		if (*ptr == 0) // NOTHING there-  paragraph boundary?
		{
			if (text) // during text
			{
				paragraph = true;
			}
			continue;
		}

		at = strstr(readBuffer,(char*)"See also");
		if (at && (at-readBuffer) < 10) continue;	 // ignore see also

		//
		// reformat special web characters
		//

		while ((at = strstr(ptr,(char*)"&lt;")))
		{
			memmove(at+1,at+4,strlen(at+3));
			at[0] = '<';
		}
		while ((at = strstr(ptr,(char*)"&gt;")))
		{
			memmove(at+1,at+4,strlen(at+3));
			at[0] = '>';
		}

		while ((at = strstr(ptr,(char*)"{{spaced ndash}}")))
		{
			memmove(at+3,at+16,strlen(at+15));
			at[0] = ' ';
			at[1] = '-';
			at[2] = ' ';
		}
			
		while ((at = strstr(ptr,(char*)"&lsquot;")))// convert '
		{
			memmove(at+1,at+7,strlen(at+6));
			*at = '"';
		}
				
		while ((at = strstr(ptr,(char*)"&rsquot;")))// convert '
		{
			memmove(at+1,at+7,strlen(at+6));
			*at = '"';
		}
	
		while ((at = strstr(ptr,(char*)"&quot;")))// convert quotes
		{
			memmove(at+1,at+6,strlen(at+5));
			*at = '"';
		}
		
		while ((at = strstr(ptr,(char*)"&ldquo;")))// convert left quotes
		{
			memmove(at+1,at+8,strlen(at+7));
			*at = '"';
		}
		
		while ((at = strstr(ptr,(char*)"&rdquo;")))// convert right quotes
		{
			memmove(at+1,at+8,strlen(at+7));
			*at = '"';
		}
	
		while ((at = strstr(ptr,(char*)"&laquo;")))// convert left foreign quotes
		{
			memmove(at+1,at+8,strlen(at+7));
			*at = '"';
		}
			
		while ((at = strstr(ptr,(char*)"&raquo;")))// convert right foreign quotes
		{
			memmove(at+1,at+8,strlen(at+7));
			*at = '"';
		}

		while ((at = strstr(ptr,(char*)"&amp;ndash;")))
		{
			memmove(at+3,at+11,strlen(at+10));
			*at = ' ';
			at[1] = '-';
			at[2] = ' ';
		}

		while ((at = strstr(ptr,(char*)"&amp;"))) // preserve &
		{
			memmove(at+1,at+5,strlen(at+4));
			*at = '&';
		}
		while ((at = strstr(ptr,(char*)"&nbsp;"))) // kill nonbreaking space
		{
			memmove(at+1,at+6,strlen(at+5));
			*at = ' ';
		}

		//
		// handle chunks
		//

		// any line with more than 2 bullets is some kind of index line, ignore it
		char* linker = readBuffer-1;
		unsigned int n = 0;
		while ((linker = strstr(linker+1,(char*)bulletchar))) ++n;
		if (n > 2) continue;


		while ((at = strstr(ptr,(char*)"<!--"))) // kill off private notes  <!-- Attention!  -->
		{
			char* end = strstr(at,(char*)"-->");
			if (end) memmove(at,end+3,strlen(end+2));
			else break;
		}

		// * [[aberation]] ([[aberration]])  ignore dictionary pretenses  - and really short lines
		if (*ptr == '*')
		{
			char* next = ReadCompiledWord(ptr,word);
			if (*word == '[')
			{
				char junk[MAX_WORD_SIZE];
				 next = ReadCompiledWord(next,junk);
				 if (*junk == '(') continue;	// just ignore this
				 next = ReadCompiledWord(next,junk);
				 next = ReadCompiledWord(next,junk);
				 if (!*junk) continue; 
			}
		}

		while (*ptr == ':') ++ptr; // skip over tab marks- BUG--  they may indicate stand alone lines in a line item. we'd want to use periods at end of line

		if (*ptr == '*') // denote a bullet item
		{
			if (text && *content) 	
			{
				fprintf(out,(char*)"%s\r\n",content);
				len += strlen(content) + 2;
				*content = 0;
			}
			bullet = true;
			while (*ptr == '*') ++ptr; 
		}

		// header lines
		if (*ptr == '=' && ptr[1] == '=')
		{
			killheading = false;
			span = citex = squ = prex = mathx = galleryx = includex = 0; // header closes anything we overlooked
			while (*ptr == '=') ++ptr; // skip to end of start
			char* end = strchr(ptr,'=');
			if (end) *end = 0; 
			size_t x = strlen(ptr);
			if (x > (MAX_WORD_SIZE-3)) x = MAX_WORD_SIZE-3;
			strncpy(heading,ptr,x);
			heading[x] = 0;

			if (*content) // close out old content
			{
				fprintf(out,(char*)"%s\r\n",content);
				*content = 0;
			}
			if (strstr(heading,(char*)"Sources") || strstr(heading,(char*)"sources") || strstr(heading,(char*)"urther reading") || strstr(heading,(char*)"ditions") || strstr(heading,(char*)"ebsites") || strstr(heading,(char*)"ibliography") || strstr(heading,(char*)"eferences") || strstr(heading,(char*)"xternal link"))// ignore sections listing other websites
			{
				killheading = true;
				*heading = 0;
			}
			continue;
		}

		if (strstr(ptr,(char*)"<page")) // skip to start of a page
		{
			ptr = strstr(ptr,(char*)"<page");
			
			if ((int)len > size) // start in new file, this file is getting too big
			{
				char outfile[MAX_WORD_SIZE];
				FClose(out);
				++id;
				sprintf(outfile,(char*)"%s/file%d.txt",directoryout,id); // the output file
				out = FopenUTF8Write(outfile);
				len = 0;
				Log(STDUSERLOG,(char*)"Starting file: %s\r\n", outfile);
			}
			span = citex = includex = galleryx = mathx = prex = squ = 0;
			paragraph = false;
			*content = 0;
			*titlename = 0;
			*heading = 0;

			page = true;
			killheading = false;
			title = false;
			countSquiggle = 0;
			text = false;
			header = false;
			bullet = false;
			pendingtextclose = false;
			table = 0;
			superscript = false;
			subscript = false;
			italic = false;
			center = false;
			bold = false;
		}
		
		if (!span && !squ && !prex && !mathx && !galleryx && !includex && FlushEmbedded(ptr,(char*)"<ref",(char*)"</ref>",citex)) continue;
		if (!span && !squ && !prex && !mathx && !galleryx && FlushEmbedded(ptr,(char*)"<includeonly",(char*)"</includeonly>",includex)) continue;
		if (!span && !squ && !prex && !mathx && FlushEmbedded(ptr,(char*)"<gallery",(char*)"</gallery>",galleryx)) continue;
		if (!span && !squ && !prex && FlushEmbedded(ptr,(char*)"<math",(char*)"</math>",mathx)) continue; 
		if (!squ && !span && FlushEmbedded(ptr,(char*)"<pre",(char*)"</pre>",prex)) continue;  // bug - what is this
		if (!squ && FlushEmbedded(ptr,(char*)"<span",(char*)"</span>",span)) continue;  
		if (FlushEmbedded(ptr,(char*)"{{",(char*)"}}",squ)) 
		{
			continue;  //  wikimedia templates
		}
		
		unsigned int oldlen = 0;
		char* oldoriginal = 0;

		while (ptr && *ptr)
		{
			ptr = SkipWhitespace(ptr);
			while (*ptr == '#' || *ptr == ':' || *ptr == ';' ) {++ptr;} // skip these

			char* original = ptr;	// before this iteration
			unsigned int lexn = strlen(ptr);
			if (oldoriginal == ptr && oldlen == lexn) // no progress
			{
				text = false;
				break;
			}
			oldoriginal = ptr;
			oldlen = lexn;
			ptr = ReadCompiledWord(ptr,word,true);

			// page end (article end)
			char* end = strstr(word,(char*)"</page>");
			if (end) 
			{
				page = false;
				title = false;
				text = false;
				*end = 0;
			}

			// titles
			end = strstr(word,(char*)"<title");
			if (end  && page) 
			{
				killheading = false;
				countSquiggle = 0;
				text = false;
				header = false;
				bullet = false;
				pendingtextclose = false;
				table = 0;
				superscript = false;
				subscript = false;
				italic = false;
				center = false;
				bold = false;
				title = true;
				paragraph = false;
				*titlename = 0;
				end = strchr(end,'>');
				if (end) memmove(word,end+1,strlen(end));
			}
			end = strstr(word,(char*)"</title>");
			if (end  && page) 
			{
				title = false;
				*end = 0;
				char* at1 = SkipWhitespace(word);
				if (*at1)
				{
					size_t lenx = strlen(at1) + 2;
					if ((strlen(titlename) + lenx) < (MAX_WORD_SIZE-3))
					{
						strcat(titlename,(char*)" "); // any leading piece adds to title
						strcat(titlename,at1);
					}
				}
				if (strchr(titlename,':') || strchr(titlename,'/')) // discard unusual articles, like: Wikipedia:AutoWikiBrowser/Typos
				{
					// Log(STDUSERLOG,(char*)"Discarding page about %s\r\n",titlename);
					*titlename = 0;
					page = false;
				}
				if (strstr(titlename,(char*)"disambiguation"))
				{
					// Log(STDUSERLOG,(char*)"Discarding disambiguation page about %s\r\n",titlename);
					*titlename = 0;
					page = false;
				}
			}
			// title substance
			if (title)
			{
				char* at1 = SkipWhitespace(word);
				if (*at1)
				{
					size_t lenx = strlen(at1) + 1;
					if ((strlen(titlename) + lenx) < (MAX_WORD_SIZE-3))
					{
						strcat(titlename,(char*)" "); // any leading piece adds to title
						strcat(titlename,at1);
					}
				}
				if (strlen(titlename) > 500) title = false;
				continue;
			}

			// text zones
			if (pendingtextclose)
			{
				char* e = strchr(original,'>');
				if (!e) continue;
				pendingtextclose = false;
				text = true;
				ptr = e + 1;
				continue;
			}
			end = strstr(word,(char*)"<text");
			if (end && page) // cant initiate text unless we saw a page and approved of any title.
			{
				paragraph = true;
				page = true;
				countSquiggle = 0;
				title = false;
				header = false;
				bullet = false;
				pendingtextclose = false;
				table = 0;
				superscript = false;
				subscript = false;
				italic = false;
				center = false;
				bold = false;

				*content = 0;
				end = strchr(end,'>');
				if (!end)
				{
					pendingtextclose = true;
					continue;
				}
				text = true; 
				memmove(word,end+1,strlen(end));
			}
			end = strstr(word,(char*)"</text>");
			if (!end && strstr(original,(char*)"-- interwiki --")) // cross language wiki links at end before close
			{
				text = false;
				*ptr = 0;
				end = SkipWhitespace(content);
				if (*end) 
				{
					fprintf(out,(char*)"%s\r\n",content);
					len += strlen(content) + 2;
				}
				*content = 0;	
				continue;
			}
			if (end  && text) 
			{
				if (end != word) // separate token from other stuff which might be kept
				{
					*end = 0;
					ptr = original;
					end =  strstr(ptr,(char*)"</text>");
					memmove(end+1,end,strlen(end)+1);
					*end = ' ';
					ptr = end + 1; // resume at end but accept stuff here

				}
				else // end at start
				{
					ptr = original;
					end = strstr(ptr,(char*)"</text>");
					ptr = end + 7;
					text = false;
					end = SkipWhitespace(content);
					if (*end) 
					{
						fprintf(out,(char*)"%s\r\n",content);
						len += strlen(content) + 2;
					}
					*content = 0;	
					continue;
				}
			}
			if (!text || killheading) continue; // either this heading's text is unacceptable or we aren't doing txt right now
			// process the text
			
			char* squiggle = strchr(word,'{');
			if (squiggle)
			{
				*squiggle = 0; // truncate word here
				char* endsquiggle = strchr(original,'{');
				ptr = endsquiggle + 1;
				if (squiggle == word) // is at start of token, absorb count and try again
				{
					countSquiggle += 1;
					continue;
				}
				else memmove(endsquiggle+1,endsquiggle,strlen(endsquiggle)+1); // is middle of token, separate it for later review
			}
			squiggle = strchr(word,'}');
			if (squiggle)
			{
				*squiggle = 0; // truncate word here
				char* endsquiggle = strchr(original,'}');
				ptr = endsquiggle + 1;
				if (squiggle == word) // is at start of token, absorb count and try again
				{
					countSquiggle -= 1;
					continue;
				}
				else memmove(endsquiggle+1,endsquiggle,strlen(endsquiggle)+1); // is middle of token, separate it for later review
			}
			if (countSquiggle) 
				continue;	 // ignore junk within squiggles
		
			char* start = word; 
			
			// handle anchors
			if (*start == '<' && start[1] == 'a' && start[2] == ' ') //<a href="/wiki/Month" title="Month">month</a>
			{
				char* end = strchr(start,'>');
				if (end) ptr = end + 1;
				else ptr = strchr(ptr,'>');

				continue;
			}
			char* at = strstr(start,(char*)"</");
			if (at) *at = 0;

			if (*start == '-' && start[1] == '-') continue; // line

			// strip embedded italic/bold format 
			char* format = strstr(word,(char*)"''");
			if (format)
			{
				format = strstr(original,(char*)"''");
				char* endstart = format;
				while (*++endstart == '\''){;}
				memmove(format,endstart,strlen(endstart)+1); 
				ptr = original;
				continue; // try again having removed the marker
			}
			if (StripEmbedded(word,ptr,original,(char*)"<sup",(char*)"</sup>",superscript,true)) continue; 
			if (StripEmbedded(word,ptr,original,(char*)"<sub",(char*)"</sub>",subscript,true)) continue; 
			if (StripEmbedded(word,ptr,original,(char*)"<i",(char*)"</i>",italic,true)) continue; 
			if (StripEmbedded(word,ptr,original,(char*)"<center",(char*)"</center>",center,true)) continue; 
			if (StripEmbedded(word,ptr,original,(char*)"<b",(char*)"</b>",bold,true)) continue; 

			// strip embedded  links  [[ ]] and web link [http:...xxx ]
			char* link = strstr(word,(char*)"[[");
			if (!link) link = strstr(word,(char*)"[http");
			if (link)
			{
				if (link[1] == '[') link = strstr(original,(char*)"[[");
				else 
					link = strstr(original,(char*)"[http");

				// special category stuff
				// [[Category:English mathematicians|Turing, Alan]]
				if (!strnicmp(link,(char*)"[[category:",11))
				{
					char* end = strstr(link,(char*)"]]");
					if (end)
					{
						*end = 0;
						char* vert = strchr(link,'|');
						if (vert) *vert = 0;
						fprintf(out,(char*)"    [category]  %s \r\n",link+11);
						ptr = end+2;
						continue;
					}
				}

				// scan forward
				char* at = link + 1; // at the 2nd [
				// skip over http link
				char junk[MAX_WORD_SIZE];
				if (*at != '[') // http link
				{
					at = ReadCompiledWord(at,junk) - 1; // skip over http content
					if (strchr(junk,']')) at = strchr(link,']') - 1; // link is the only content and close was attached
				}
				unsigned int bracketCounter = 1; 
				char* endlink = 0;
				bool hadcolon = false;
				char hold[MAX_WORD_SIZE * 4];
				char* holdstart = hold;
				char* holdptr = hold;
				bool opensquiggle = false;
				bool image = !strnicmp(at+1,(char*)"image:",6);
				while (*++at)
				{
					if ((holdptr-hold) > ((MAX_WORD_SIZE * 4) - 2)) break; // something isnt right
					*holdptr++ = *at; // copy content
					if (*at == '{') opensquiggle = true;
					if (*at == '|') // discard all prior info
					{
						holdptr = holdstart;
						*holdptr = 0;
						hadcolon = false;
						continue;
					}
					if (*at == ':') 
					{
						hadcolon = true;
						continue;
					}
					if (!strnicmp(at,(char*)"<ref",4)) // internal ref
					{
						char* end = strstr(at,(char*)"</ref>");
						if (end)
						{
							--holdptr;
							at = end + 6;
							continue;
						}
					}
					
					if (*at == '}' && !opensquiggle && at[1] == '}') // revise as close ] as they made a typo
					{
						*at = ']';
						at[1] = ']';
					}
					if (*at == '[' && at[1] == '[')  // nested stuff to be displayed
					{
						--holdptr;	// remove hold on this
						*holdptr++ = ' '; // space it
						holdstart = holdptr;
						++bracketCounter;
						++at;
					}
					else if (*at == ']' ) // in case they dont close it correctly, accept a single close
					{
						--holdptr;	// remove hold on this
						--bracketCounter;
						endlink = at;
						if (at[1] == ']') ++at;  // points at last ]
						if (bracketCounter && (at-link) > 200 && !image) // image text can last a while
							bracketCounter = 0;	// wrong? force end
					}
					if (bracketCounter == 0) // true end of area found (may have nested brackets)
					{
						*endlink = 0;	// the close mark is done
						if (hadcolon) holdptr = hold; // drop all content
						break; // balanced out
					}
				}
				ptr =  (*at) ? (at+1) : at; // assume failure to close like: [[except [[kelsey grammar]]
				if (image) holdptr = hold;	// cancel all findings
				if (holdptr != hold)
				{
					if (*ptr == '\'' && ptr[1] == '\'') // closing italics
					{
						while (*++ptr == '\''){;}
					}
					else while (IsAlphaUTF8OrDigit(*ptr) || *ptr == '\'' ) *holdptr++ = *ptr++; // copy any leftover like [[hit]]s
					*holdptr++ = ' '; // close it off
					*holdptr = 0;
					if (strlen(hold) > (MAX_WORD_SIZE-3)) hold[MAX_WORD_SIZE-3] = 0;
					strcpy(word,hold);	 // declare all this to be displayed
				}
				else continue; // no text
			}

			if (*start == '&') // convert other special web character by removal
			{
				end = strchr(start,';');
				if (end) memmove(start,end+1,strlen(end));
			}
			if (!strnicmp(start,(char*)"ndash;",6) || !strnicmp(start,(char*)"mdash;",6)) // badly formed constant removal
			{
				end = strchr(start,';');
				if (end) memmove(start,end+1,strlen(end));
			}
			if (!*start) continue; // no text remaining

			if (!stricmp(start,(char*)"redirect")) // ignore a redirect body
			{
				text = false;
				continue;
			}

			if (*titlename) // drop title now
			{
				if (strstr(content,(char*)"may refer to")) // disambiguation
				{
					text = false;
					continue;
				}

				fprintf(out,(char*)"\r\n[title] %s\r\n  ",titlename);
				*titlename = 0;
			}

			// actual text now
			if (*heading) // only show heading if text materializes
			{
				fprintf(out,(char*)"  [heading] %s\r\n",heading);
				*heading = 0;
				paragraph = true;
			}
			if (bullet) // drop the bullet now that we found text
			{
				fprintf(out,(char*)"%s",(char*)"    [*] ");
				paragraph = false;
				bullet = false;
			}
			size_t lenx = strlen(start);
			if (lenx > (MAX_WORD_SIZE-3)) // limit
			{
				lenx = MAX_WORD_SIZE -3;
				start[lenx] = 0;
			}
			if (*content && (strlen(content) + lenx) > (MAX_WORD_SIZE - 100)) // dont accumulate, dump the old text, getting too big
			{
				fprintf(out,(char*)"%s\r\n",content);
				*content = 0;
			}
			if (paragraph)
			{
				paragraph = false;
				strcat(content,(char*)"[p] ");
			}
			strcat(content,(char*)" ");
			strcat(content,start);
		}
	}
	FClose(in);

	} // end MAIN loop
	FClose(out);
}

/////////////////////////////////////////////////////
/// SYSTEM CONTROL COMMANDS
/////////////////////////////////////////////////////

static void C_Bot(char* name)
{
	char word[MAX_WORD_SIZE];
	name = ReadCompiledWord(name,word);
	MakeLowerCopy(computerID,word);
	strcpy(computerIDwSpace+1,computerID);
	strcat(computerIDwSpace,(char*)" "); // trailing space
	if (shared) return; // pool doesnt require , just direct changeover since shared has the primary current context of all bots
	wasCommand = BEGINANEW;	// make system save revised user file
}

static void C_Build(char* input)
{
	conditionalCompiledIndex = 0;
#ifndef DISCARDSCRIPTCOMPILER
	int64 oldbotid = myBot;
	echo = true;
    bool olduserlog = userLog;
    userLog = true;
	myBot = 0;	// default
	mystart(input);
    kernelVariableThreadList = botVariableThreadList = NULL;
    char oldlogin[MAX_WORD_SIZE];
	char oldbot[MAX_WORD_SIZE];
	char oldbotspace[MAX_WORD_SIZE];
	char oldloginname[MAX_WORD_SIZE];
	strcpy(oldlogin,loginID);
	strcpy(oldbot,computerID);
	strcpy(oldbotspace,computerIDwSpace);
	strcpy(oldloginname,loginName);
	char file[SMALL_WORD_SIZE];
	char control[MAX_WORD_SIZE];
	ClearWordMaps(); // :build
	input = ReadCompiledWord(input,file);
	input = SkipWhitespace(input);
	int spell = PATTERN_SPELL;
	bool reset = false;
	trace = 0;
	grade = 0;
	while (*input) 
	{
		input = ReadCompiledWord(input,control);
		if (!stricmp(control,(char*)"nospell")) spell = NO_SPELL;
        else if (!stricmp(control, (char*)"quiet")) echo = false;
        else if (!stricmp(control,(char*)"nosubstitution")) spell = NO_SUBSTITUTE_WARNING;
		else if (!stricmp(control,(char*)"outputspell")) spell = OUTPUT_SPELL;
		else if (!stricmp(control,(char*)"gradek")) { grade = KINDERGARTEN; spell = OUTPUT_SPELL;}
		else if (!stricmp(control,(char*)"grade2")) { grade = (KINDERGARTEN|GRADE1_2); spell = OUTPUT_SPELL;}
		else if (!stricmp(control,(char*)"grade4")) { grade = (KINDERGARTEN|GRADE1_2|GRADE3_4); spell = OUTPUT_SPELL;}
		else if (!stricmp(control,(char*)"grade6")) { grade = (KINDERGARTEN|GRADE1_2|GRADE3_4|GRADE5_6); spell = OUTPUT_SPELL;}
		else if (!stricmp(control,(char*)"reset")) reset = true;
		else if (!stricmp(control,(char*)"keys"))
		{
			char fname[200];
			sprintf(fname, "%s/keys.txt", tmp);
			remove(fname);
			spell = NOTE_KEYWORDS;
		}
		else if (*control == '#')  // allowed conditional elements
		{
			strcpy(conditionalCompile[conditionalCompiledIndex++],control);
			if (conditionalCompiledIndex == MAX_CONDITIONALS)  conditionalCompiledIndex--;
		}

	}
	size_t len = strlen(file);
	if (!*file) Log(STDUSERLOG,(char*)"missing build label");
	else
	{
		sprintf(logFilename,(char*)"%s/build%s_log.txt",users,file); //   all data logged here by default
		FILE* in = FopenUTF8Write(logFilename);
		FClose(in);
		Log(STDUSERLOG,(char*)"ChatScript Version %s  compiled %s\r\n",version,compileDate);
		char word[MAX_WORD_SIZE];
		sprintf(word,(char*)"files%s.txt",file);
		if (file[len-1] == '0') buildId = BUILD0;
		else if  (file[len-1] == '2') buildId = BUILD2;
		else buildId = BUILD1; // global so SaveCanon can work
		char file[200];
		sprintf(file,"%s/missingSets.txt",topic);
		remove(file); // precautionary
		sprintf(file,"%s/missingLabel.txt",topic);
		remove(file);
		MakeDirectory((char*)"TOPIC");
		if (buildId == BUILD0) MakeDirectory((char*)"TOPIC/BUILD0");
		else if (buildId == BUILD1) MakeDirectory((char*)"TOPIC/BUILD1");
		else if (buildId == BUILD2) MakeDirectory((char*)"TOPIC/BUILD2");
		userVariableThreadList = NULL;
		*scopeBotName = 0; 
		ReadTopicFiles(word, buildId, spell);

		if (!stricmp(computerID,(char*)"anonymous")) *computerID = 0;	// use default
		ClearPendingTopics(); // flush in case topic ids change or go away
		ClearVolleyWordMaps();
		PartiallyCloseSystem();
		CreateSystem();
#ifdef PRIVATE_CODE
		PrivateRestart(); // must come AFTER any mongo/postgress init (to allow encrypt/decrypt override)
#endif
		systemReset = (reset) ? 2 : 1;
	}
	// refresh current user data lost when we rebooted the system
	strcpy(loginID,oldlogin);
	strcpy(computerID,oldbot);
	strcpy(computerIDwSpace,oldbotspace);
	strcpy(loginName,oldloginname);
	trace &= -1 ^ TRACE_SCRIPT;
	myBot = oldbotid;
    userLog = olduserlog;

#endif
}  

static void C_Quit(char* input)
{
	// allow local bots to continue if in boot
	if (server || currentBeforeLayer != LAYER_BOOT)
	{
		Log(STDUSERLOG, (char*)"Exiting ChatScript via Quit\r\n");
		quitting = true;
	}
}

static void C_Restart(char* input)
{
	static char arg0[200];
	static char arg1[200];
	static char arg2[200];
	static char arg3[200];
	static char* arglist[5];
	arglist[1] = arg1;
	arglist[2] = arg2;
	arglist[3] = arg3;
	arglist[4] = arg0;
	char word[MAX_WORD_SIZE];
	char* hold = ReadCompiledWord(input,word);
	assignedLogin = true;
	if (!stricmp(word,"erase"))
	{
		input = hold;
		pendingUserReset = true;
	}
	if (*input) // change params
	{
		argv = arglist;
		argc = 1;
		while (*input && argc < 5)
		{
			input = ReadCompiledWord(input,arglist[argc++]);
		}
	}

	wasCommand = RESTART;
}

static void C_User(char* username)
{
	// fake a login of user
	strcpy(loginID,username);
	sprintf(logFilename,(char*)"%s/%slog-%s.txt",users,GetUserPath(loginID),loginID); // user log goes here
	wasCommand = BEGINANEW;	// make system save revised user file
}

static void Rephrase(char* buffer,char* out) // rewrite script to human understandable
{
	char* at = strchr(buffer,'`');
	if (at) *at = 0; // end of rule marker
	char* hat = buffer-1;

	// if rule, remove accel at start
	if (TopLevelRule(buffer) || Rejoinder(buffer))
	{
		char label[MAX_LABEL_SIZE];
		char* ptr = GetLabel(buffer,label); // now at pattern if there is one
		if (ptr) memmove(buffer+3,ptr,strlen(ptr)+1);
	}

	// if loop end 
	if (*buffer == ')' && buffer[1] == ' ')
	{
		memmove(buffer,buffer+8,strlen(buffer+8)+1); // ) space,jump3, space, {, space
	}
    CALLFRAME* frame = GetCallFrame(globalDepth);
	while ((hat = strchr(hat+1,'^')))
	{
		if (IsDigit(hat[1])) // convert fnvar to original name
		{
			int index = atoi(hat+1);
			char var[200];
			char* list = (char*) ((frame->display) ? (frame->definition + 2) : NULL);	// skip ( and space xxxx
			if (list) for (int i = 0; i < 15; ++i)
			{
				list = ReadCompiledWord(list,var); 
				if (i == index) break;
			}
			strcpy(out,hat);
			char* end = out;
			while (IsDigit(*++end));
			strcpy(hat,var);
			strcat(hat,end);	// spliced replacement in
		}
		else if (!strnicmp(hat,"^if ",4)) 
		{
			memmove(hat+4,hat+7,strlen(hat+7)+1); // remove accelerator ^if xxx(
		}
	}
}

static void C_Debug(char* x)
{
	if (!server && debugCall) (*debugCall)(x, false);
}

static void C_ClearLog(char* x)
{
	remove(logFilename);
}

///////////////////////////////////////////////
/// SERVER COMMANDS
///////////////////////////////////////////////

static void C_Crash(char* x)
{
	int xx = 0;
	xx = 1 / xx;
}

static void C_Flush(char* x)
{
	FlushCache();
}


///////////////////////////////////////////////////
/// WORD INFORMATION
///////////////////////////////////////////////////

static void DrawSynsets(MEANING M)
{
	unsigned int index = Meaning2Index(M);
	WORDP D = Meaning2Word(M);
	unsigned int limit =  GetMeaningCount(D);
	if (!down_is) limit = 0;
	if (!limit)
	{
		MEANING T = MakeMeaning(D,0);
        ++downcount;
		Log(STDUSERLOG,(char*)" %s ",WriteMeaning(T,true)); // simple member
	}
	for (unsigned int i = 1; i <= limit; ++i)
	{
		if (index && i != index) continue;
		MEANING at = GetMeanings(D)[i];
		unsigned int n = 0;
		MEANING T = MakeMeaning(D,i);
		Log(STDUSERLOG,(char*)"%s ",WriteMeaning(T,true)); // simple member
		while ((at &= (INDEX_BITS|MEANING_BASE)) != (T & (INDEX_BITS|MEANING_BASE))) // find the correct ptr to return. The marked ptr means OUR dict entry is master, not that the ptr points to.
		{
			Log(STDUSERLOG,(char*)"%s ",WriteMeaning(at,true)); // simple member
			WORDP X = Meaning2Word(at);
			unsigned int ind = Meaning2Index(at);
			at = GetMeanings(X)[ind];
			if (++n >= 20) break; // force an end arbitrarily
		}
	}
//	if (down_is) Log(STDUSERLOG, (char*)"\r\n");
}

static void DrawDownHierarchy(MEANING T,unsigned int depth,unsigned int limit,bool sets)
{
	if (sets) limit = 1000;
    if (depth >= limit || !T) return;
    WORDP D = Meaning2Word(T);
	if (D->inferMark == inferMark) return;	

	D->inferMark = inferMark;
    unsigned int index = Meaning2Index(T);
    unsigned int size = GetMeaningCount(D);
    if (!size) size = 1; 
	if (*D->word == '~') // show set members
	{
		MarkExclude(D);

		FACT* F = GetObjectNondeadHead(D);
		unsigned int i = 0;
		while (F)
		{
			if (ValidMemberFact(F))
			{
				if (TraceHierarchyTest(trace)) TraceFact(F);
				MEANING M = F->subject;
				WORDP S = Meaning2Word(M);
				if (S->inferMark != inferMark)
				{
					if (*S->word == '~' && (depth + 1) < limit) // expand to lower level
					{
						Log(STDUSERLOG,(char*)"\r\n");
						for (unsigned int j = 0; j < (depth*2); ++j) Log(STDUSERLOG,(char*)" "); // depth inclusive because tabbing for next level
						Log(STDUSERLOG,(char*)"%s ",WriteMeaning(M)); // simple member
						DrawDownHierarchy(M,depth+1,limit,sets);
						Log(STDUSERLOG,(char*)"\r\n");
						for (unsigned int j = 0; j < (depth*2); ++j) Log(STDUSERLOG,(char*)" ");
					}
					else DrawSynsets(M);
					if ( ++i >= 5)
					{
						Log(STDUSERLOG,(char*)"\r\n");
						for (unsigned int j = 0; j < (depth*2); ++j) Log(STDUSERLOG,(char*)" ");
						Log(STDUSERLOG, (char*)" ");
						i = 0;
					}
				}
			}
			F = GetObjectNondeadNext(F);
		}
		return;
	}

    if (down_is || index) for (unsigned int k = 1; k <= size; ++k) //   for each meaning of this dictionary word
    {
        if (index)
		{
			if (k != index) continue; //   not all, just one specific meaning
			T = GetMaster(GetMeaning(D,k)); 
		}
		else 
		{
			if (GetMeaningCount(D)) T = GetMaster(GetMeaning(D,k));
			else T = MakeMeaning(D); //   did not request a specific meaning, look at each in turn
		}

        //   for the current T meaning
		char* gloss = GetGloss(Meaning2Word(T),Meaning2Index(T));
		if (!gloss) gloss = "";
        if (depth++ == 0 && size)  Log(STDUSERLOG,(char*)"\r\n<%s.%d => %s (%s)\r\n",D->word,k,WriteMeaning(T),gloss); //   header for this top level meaning is OUR entry and MASTER
        int l = 0;
        while (++l) //   find the children of the meaning of T
        {
			MEANING child = (limit >= 1) ? FindChild(T,l) : 0; //   only headers sought
            if (!child) break;
			if (sets) //   no normal words, just a set hierarchy
			{
				WORDP D = Meaning2Word(child);
				if (*D->word != '~') continue;
			}

			 //   child and all syn names of child
            for (unsigned int j = 0; j <= (depth*2); ++j) Log(STDUSERLOG,(char*)" "); 
   			gloss = GetGloss(Meaning2Word(child),Meaning2Index(child));
			if (!gloss) gloss = "";
			Log(STDUSERLOG,(char*)"%d. ",depth);
			DrawSynsets(child);
			Log(STDUSERLOG, (char*)"(%s)\r\n", gloss);

			// below child master
			DrawDownHierarchy(child,depth,limit,sets);
        } //   end of children for this value
        --depth;
    }
}

static void DumpConceptPath(MEANING T) // once you are IN a set, the path can be this
{
	int k = 0;
	while (++k)
	{
		MEANING parent = FindSetParent(T,k); //   next set we are member of
		if (!parent)  break;

		WORDP D = Meaning2Word(parent);	// topic or concept
		FACT* F = GetObjectNondeadHead(D);
		while (F)
		{
			if (F->verb == Mexclude)
			{
				WORDP E = Meaning2Word(F->subject);
				if (E->inferMark == inferMark) break;
			}
			else if (F->verb == Mmember) break;
			F = GetObjectNondeadNext(F);
		}
		if (F) continue;	// exclusion in effect
		WORDP E = Meaning2Word(parent);
		if (E->inferMark != inferMark) 
		{
			E->inferMark = inferMark;
			*meaningLimit++ = parent;
		}
	}
}

static void ShowConcepts(MEANING T)
{
 	MEANING parent;
	unsigned int count;
	WORDP E = Meaning2Word(T);
	unsigned int index = Meaning2Index(T);
    if (*E->word != '~' && index == 0)  // at a base word
	{
		DumpConceptPath(T); // what is it a member of direclty

		//   then do concepts based on this word...
		unsigned int size = GetMeaningCount(E);
		if (!size) size = 1;	//   always at least 1, itself
		//   immediate sets of this base
		for  (unsigned int k = 1; k <= size; ++k)
		{
			//   get meaningptr spot facts are stored (synset head)
			if (!GetMeaningCount(E) ) T = MakeMeaning(E);	//   a generic since we have no meanings
			else 
			{
				if (GetMeaning(E,k) & SYNSET_MARKER) T = MakeMeaning(E,k); // we are master
				else T = GetMaster(GetMeaning(E,k)) | GETTYPERESTRICTION(GetMeaning(E,k)); 
			}
			DumpConceptPath(T); 
		}

		//   up one wordnet hierarchy based on each meaning
		for  (unsigned int k = 1; k <= size; ++k)
		{
			//   get meaningptr spot facts are stored (synset head)
			if (!GetMeaningCount(E) ) T = MakeMeaning(E);	//   a generic since we have no meanings
			else 
			{
				if (GetMeaning(E,k) & SYNSET_MARKER) T = MakeMeaning(E,k); // we are master
				else T = GetMaster(GetMeaning(E,k)) | GETTYPERESTRICTION(GetMeaning(E,k)); 
			}
			count = 0;
			while ((parent =  FindSynsetParent(T,count++))) ShowConcepts(parent); // immediate wordnet hierarchy
		}
	}
	else if (index != 0) //    always synset nodes above the base
	{
		count = 0;
		while ((parent =  FindSynsetParent(T,count++))) DumpConceptPath(parent); // sets of next parent level up
		count = 0;
		while ((parent =  FindSynsetParent(T,count++))) ShowConcepts(parent); // and follow next parent level up
	}
	else  DumpConceptPath(T); // track this synset to the next level
}

static void C_Concepts(char* input)
{
	char word[MAX_WORD_SIZE];
	ReadCompiledWord(input,word);
	MEANING M = ReadMeaning(word,false);
	if (!M) return;
	M = GetMaster(M);
	Log(STDUSERLOG,(char*)"%s: ",word);
	NextInferMark();

	meaningList = (MEANING*) AllocateBuffer();
	meaningLimit = meaningList;

	// check substitutes
	WORDP D = Meaning2Word(M);
	if (D->internalBits & HAS_SUBSTITUTE)
	{
		D = GetSubstitute(D);
		if (D && *D->word == '~')  *meaningLimit++ = MakeMeaning(D); 
	}

	char alter[MAX_WORD_SIZE];
	sprintf(alter,(char*)"<%s",word);
	D = FindWord(alter);
	if (D && D->internalBits & HAS_SUBSTITUTE)
	{
		D = GetSubstitute(D);
		if (D && *D->word == '~')   *meaningLimit++ = MakeMeaning(D); 
	}
	
	sprintf(alter,(char*)"<%s>",word);
	D = FindWord(alter);
	if (D && D->internalBits & HAS_SUBSTITUTE)
	{
		D = GetSubstitute(D);
		if (D &&*D->word == '~')   *meaningLimit++ = MakeMeaning(D); 
	}

	 *meaningLimit++ = M;

	// check concepts and topics
	while (meaningList < meaningLimit) 
	{
		WORDP E = Meaning2Word(*meaningList);
		if (*E->word == '~') Log(STDUSERLOG,(E->internalBits & TOPIC) ? (char*) "T%s " : (char*) "%s ",E->word);
		ShowConcepts(*meaningList++);
	}
	Log(STDUSERLOG,(char*)"\r\n");

	FreeBuffer();
}

static void C_Down(char* input)
{
	char word[MAX_WORD_SIZE];
	input = ReadCompiledWord(input,word);
	if (*word == '~') down_is = false; // concept set not wordnet
	input = SkipWhitespace(input);
    int limit = atoi(input);
    if (!limit) limit = 1; //   top 2 level only (so we can see if it has a hierarchy)
	input = SkipWhitespace(input);
	NextInferMark();
	MEANING M = ReadMeaning(word,false);
	M = GetMaster(M);
    downcount = 0;
	Log(STDUSERLOG, (char*)"   1. %s\r\n",word);
    DrawDownHierarchy(M,1,limit+1,!stricmp(input,(char*)"sets"));
	Log(STDUSERLOG,(char*)"\r\n Count=%d\r\n",downcount);
	down_is = true;
}

static void FindXWord(WORDP D, uint64 pattern)
{
	if (D->word && MatchesPattern(D->word,(char*) pattern)) Log(STDUSERLOG,(char*)"%s\r\n",D->word);
}

static void C_FindWords(char* input)
{
	WalkDictionary(FindXWord,(uint64) input);
}

static bool TestSetPath(MEANING T,unsigned int depth) // once you are IN a set, the path can be this
{
	WORDP D = Meaning2Word(T);
	if (D->inferMark == inferMark || depth > 100) return false;
	D->inferMark = inferMark;
	int k = 0;
	while (++k)
	{
		MEANING parent = FindSetParent(T,k); //   next set we are member of
		if (!parent)  break;
		WORDP D = Meaning2Word(parent);	// topic or concept
		if (trace & TRACE_PATTERN) Log(STDUSERLOG,(char*)"%s\r\n",D->word);
		if (D == topLevel) return true;
		if (TestSetPath(parent,depth+1)) return true; // follow up depth first
	}
	return false;
}

static bool TestUpHierarchy(MEANING T,int depth)
{
    if (!T) return false;

    WORDP E = Meaning2Word(T);
	if (E == topLevel) return true;
	unsigned int index = Meaning2Index(T);
    if (depth == 0)  
	{
		if (TestSetPath(T,depth)) return true;	
		if (*E->word == '~') return false;	// not a word

		//   then do concepts based on this word...
		unsigned int size = GetMeaningCount(E);
		if (!size) size = 1;	//   always at least 1, itself

		//   draw wordnet hierarchy based on each meaning
		for  (unsigned int k = 1; k <= size; ++k)
		{
			if (index && k != index) continue; //   not all, just correct meaning

			//   get meaningptr spot facts are stored (synset head)
			if (!GetMeaningCount(E) ) T = MakeMeaning(E);	//   a generic since we have no meanings
			else 
			{
				if (GetMeaning(E,k) & SYNSET_MARKER) T = MakeMeaning(E,k); // we are master
				else T = GetMaster(GetMeaning(E,k)); 
			}
			if (TestSetPath(T,depth)) return true;
			unsigned int count = 0;
			MEANING parent;
			while ((parent = FindSynsetParent(T,count++)))
			{
				//   walk wordnet hierarchy
				if (TestSetPath(parent,depth)) return true;
				if (TestUpHierarchy(parent,depth+1)) return true; //   we find out what sets PARENT is in (will be none- bug)
			}
		}
	}
	else //    always synset nodes
	{
		E->inferMark = inferMark; // came this way
		unsigned int count = 0;
		MEANING parent;
		while ((parent = FindSynsetParent(T,count++)))
		{
			//   walk wordnet hierarchy
			if (TestSetPath(parent,depth)) return true;
			if (TestUpHierarchy(parent,depth+1)) return true; //   we find out what sets PARENT is in (will be none- bug)
		}
	}
	return false;
}

static void TestSet(WORDP D,uint64 flags)
{
	if (!(D->properties & flags) || !(D->systemFlags & AGE_LEARNED)) return; // only want simple words to be tested
	if (D->properties & NOUN_ABSTRACT) return; // not abstract
	MEANING M = MakeMeaning(D);
	NextInferMark();
	if (TestUpHierarchy(M,0)) return;
	Log(STDUSERLOG,(char*)"%s\r\n",D->word);
}

static void C_Nonset(char* buffer) // NOUN ~sizes
{
	char type[MAX_WORD_SIZE];
	buffer = ReadCompiledWord(buffer,type);
	uint64 kind  = FindValueByName(type);
	if (!kind) return;
	WORDP D = FindWord(buffer);
	topLevel = D;
	WalkDictionary(TestSet,kind);
}

static void C_HasFlag(char* buffer)
{
	bool notflag = false;
	char type[MAX_WORD_SIZE];
	buffer = ReadCompiledWord(buffer,type);
	WORDP D = FindWord(type); // name of set
	buffer = SkipWhitespace(buffer);
	if (*buffer == '!')
	{
		notflag = true;
		++buffer;
	}
	buffer = ReadCompiledWord(buffer,type);
	uint64 flag  = FindSystemValueByName(type); // flag to find or !find
	FACT* F = GetObjectNondeadHead(D);
	while (F)
	{
		if (ValidMemberFact(F))
		{
			WORDP S = Meaning2Word(F->subject);
			if (S->systemFlags & flag)
			{
				if (!notflag) Log(STDUSERLOG,(char*)"%s has %s\r\n",S->word,type);
			}
			else
			{
				if (notflag) Log(STDUSERLOG,(char*)"%s lacks %s\r\n",S->word,type);
			}
		}
		F = GetObjectNondeadNext(F);
	}
}

static bool HitTest(WORDP D, WORDP set) // can we hit this
{
	if (D->inferMark == inferMark) return false;	// been here already
	D->inferMark = inferMark;
	FACT* F = GetSubjectNondeadHead(D);
	while (F)
	{
		if (ValidMemberFact(F))
		{
			WORDP E = Meaning2Word(F->object);
			if (E == set) return true;
			if (*E->word == '~') 
			{
				if (HitTest(E,set)) return true;
			}
		}
		F = GetSubjectNondeadNext(F);
	}

	return false;
}

static void C_Overlap(char* buffer)
{
	char set1[MAX_WORD_SIZE];
	char set2[MAX_WORD_SIZE];
	buffer = ReadCompiledWord(buffer,set1);
	WORDP E = FindWord(set1);
	if (!E || E->word[0] != '~')
	{
		(*printer)((char*)"no such set %s\r\n",set1);
		return;
	}
	buffer = ReadCompiledWord(buffer,set2);
	WORDP D = FindWord(set2);
	if (!D || D->word[0] != '~')
	{
		(*printer)((char*)"no such set %s\r\n",set2);
		return;
	}
	Log(STDUSERLOG,(char*)"These members of %s are also in %s:\r\n",set1,set2);

	// walk members of set1, seeing if they intersect set2
	FACT* F = GetObjectNondeadHead(E);
	while (F)
	{
		E = Meaning2Word(F->subject);
		if (ValidMemberFact(F) && *E->word != '~') // see if word is member of set2
		{
			NextInferMark();
			if (HitTest(E,D)) Log(STDUSERLOG,(char*)"%s\r\n",E->word);
		}
		F = GetObjectNondeadNext(F);
	}

}

static bool DumpSetPath(MEANING T,unsigned int depth) // once you are IN a set, the path can be this
{
	int k = 0;
	while (++k)
	{
 		MEANING parent = FindSetParent(T,k); //   next set we are member of
		if (!parent)  break;

		WORDP D = Meaning2Word(parent);	// topic or concept
		FACT* F = GetObjectNondeadHead(D);
		while (F)
		{
			if (F->verb == Mexclude)
			{
				WORDP E = Meaning2Word(F->subject);
				if (E->inferMark == inferMark) break;
			}
			else if (F->verb == Mmember) break;
			F = GetObjectNondeadNext(F);
		}
		if (F) continue;	// exclusion in effect

 		WORDP E = Meaning2Word(parent);
		if (E->inferMark == inferMark) continue;
		E->inferMark = inferMark;
        Log(STDUSERLOG,(char*)"    ");
		for (unsigned int j = 0; j < depth; ++j) Log(STDUSERLOG,(char*)"   "); 
		if (E->internalBits & TOPIC) Log(STDUSERLOG,(char*)"T%s \r\n",WriteMeaning(parent)); 
		else Log(STDUSERLOG,(char*)"%s \r\n",WriteMeaning(parent)); 
		if (!DumpSetPath(parent,depth+1)) return false; // follow up depth first
	}
	return true;
}

static bool DumpUpHierarchy(MEANING T,int depth)
{
    if (!T) return true;

    WORDP E = Meaning2Word(T);
	if (E->inferMark == inferMark) return true;	
	E->inferMark = inferMark; // came this way
	unsigned int restrict = GETTYPERESTRICTION(T);
	unsigned int index = Meaning2Index(T);
    if (depth == 0)  
	{
		Log(STDUSERLOG,(char*)"\r\nFor %s:\r\n",E->word); 
		Log(STDUSERLOG,(char*)" Set hierarchy:\r\n"); 

		if (!DumpSetPath(T,depth)) return false;	
		if (*E->word == '~') return true;	// we are done, it is not a word

		//   then do concepts based on this word...
		unsigned int meaningCount = GetMeaningCount(E);
		unsigned int size = meaningCount;
		if (!size) size = 1;	//   always at least 1, itself
		Log(STDUSERLOG,(char*)" Wordnet hierarchy:\r\n"); 

		//   draw wordnet hierarchy based on each meaning
		for  (unsigned int k = 1; k <= size; ++k)
		{
			if (index && k != index) continue; //   not all, just correct meaning
			MEANING M = (meaningCount) ? GetMeaning(E,k) : 0;
			if (restrict && !(GETTYPERESTRICTION(M) & restrict)) continue; // not valid meaning

			//   get meaningptr spot facts are stored (synset head)
			if (!GetMeaningCount(E) ) T = MakeMeaning(E);	//   a generic since we have no meanings
			else 
			{
				if (GetMeaning(E,k) & SYNSET_MARKER) T = MakeMeaning(E,k) | GETTYPERESTRICTION(GetMeaning(E,k)); // we are master
				else T = GetMaster(GetMeaning(E,k)) | GETTYPERESTRICTION(GetMeaning(E,k)); 
			}
			WORDP D1 = Meaning2Word(T);
			unsigned int restrict = GETTYPERESTRICTION(T);
			Log(STDUSERLOG,(char*)"  ");
			Log(STDUSERLOG,(char*)"%s~%d:",E->word,k);
			if (restrict & NOUN) Log(STDUSERLOG,(char*)"N   ");
			else if (restrict & VERB) Log(STDUSERLOG,(char*)"V   ");
			else if (restrict & ADJECTIVE) Log(STDUSERLOG,(char*)"Adj ");
			else if (restrict & ADVERB) Log(STDUSERLOG,(char*)"Adv ");
			else if (restrict & PREPOSITION) Log(STDUSERLOG,(char*)"Prep ");
			char* gloss = GetGloss(D1,Meaning2Index(T));
			if (gloss) Log(STDUSERLOG,(char*)" (%s) ",gloss);
			Log(STDUSERLOG,(char*)"\r\n"); 
		
			if (!DumpSetPath(T,depth)) return false;
			unsigned int count = 0;
			MEANING parent;
			while ((parent =  FindSynsetParent(T,count++)))
			{
				//   walk wordnet hierarchy
				WORDP P = Meaning2Word(parent);
				Log(STDUSERLOG,(char*)"   ");
				Log(STDUSERLOG,(char*)" is %s ",WriteMeaning(parent)); //   we show the immediate parent
				char* gloss = GetGloss(P,Meaning2Index(parent));
				if (gloss) Log(STDUSERLOG,(char*)" (%s) ",gloss);
				Log(STDUSERLOG,(char*)"\r\n"); 
				if (!DumpSetPath(parent,depth)) return false;
				if (!DumpUpHierarchy(parent,depth+1)) return false; //   we find out what sets PARENT is in (will be none- bug)
			}
		}
	}
	else //    always synset nodes
	{
		unsigned int count = 0;
		MEANING parent;
		while ((parent =  FindSynsetParent(T,count++)))
		{
			//   walk wordnet hierarchy
			WORDP P = Meaning2Word(parent);
			unsigned int index = Meaning2Index(parent);
			Log(STDUSERLOG,(char*)"   ");
			for (int j = 0; j < depth; ++j) Log(STDUSERLOG,(char*)"   "); 
			Log(STDUSERLOG,(char*)" is %s",WriteMeaning(parent)); //   we show the immediate parent
			char* gloss = GetGloss(P,index);
			if (gloss) Log(STDUSERLOG,(char*)" (%s) ",gloss);
			Log(STDUSERLOG,(char*)"\r\n");
			if (!DumpSetPath(parent,depth)) return false;
			if (!DumpUpHierarchy(parent,depth+1)) return false; //   we find out what sets PARENT is in (will be none- bug)
		}
	}
	return true;
}

static void C_Up(char* input)
{
 	char word[MAX_WORD_SIZE];
	NextInferMark();
	ReadCompiledWord(input,word);
	MEANING M = ReadMeaning(word,false);
	M = GetMaster(M);
	DumpUpHierarchy(M,0);
}

static void C_MixedCase(char* input)
{
	int n = 0;
	for (WORDP D = dictionaryBase+1; D < dictionaryFree; ++D) 
	{
		if (!D->word) continue;
		if (D->internalBits & HAS_SUBSTITUTE) continue; // not real
		char* apos = strchr(D->word,'\'');
		if (apos && !(strchr(D->word,'_') || strchr(D->word,' ')) && D > dictionaryPreBuild[LAYER_0] ) 
		{
			Log(ECHOSTDUSERLOG," has apostrophe %s\r\n",D->word);
		}
		if (D->word && D->internalBits & UPPERCASE_HASH)
		{
			WORDP E = FindWord(D->word,SECONDARY_CASE_ALLOWED);
			if (E && (E > dictionaryPreBuild[LAYER_0] || D > dictionaryPreBuild[LAYER_0] ))
			{
				if (E->internalBits & HAS_SUBSTITUTE) continue; // not real
				Log(ECHOSTDUSERLOG,"%s",D->word);
				char* loc;
				if (D <= dictionaryPreBuild[LAYER_0]) loc = "w";
				else if (D->internalBits & BUILD0) loc = "0";
				else if (D->internalBits & BUILD1) loc = "1";
				else loc = "?";
				Log(ECHOSTDUSERLOG," (%s)",loc);
				Log(ECHOSTDUSERLOG,"    %s",E->word);
				if (E <= dictionaryPreBuild[LAYER_0]) loc = "w";
				else if (E->internalBits & BUILD0) loc = "0";
				else if (E->internalBits & BUILD1) loc = "1";
				else loc = "?";
				Log(ECHOSTDUSERLOG," (%s)",loc);
				Log(ECHOSTDUSERLOG,"\r\n");
				++n;
			}
		}
	}
	Log(ECHOSTDUSERLOG,"There are %d entries\r\n",n);
}


static void C_DualUpper(char* input)
{
	int n = 0;
	inferMark = NextInferMark();
	int index = 0;
	WORDP list[50];
	for (WORDP D = dictionaryBase + 1; D < dictionaryFree; ++D)
	{
		if (!D->word) continue;
		if (D->internalBits & HAS_SUBSTITUTE) continue; // not real
		char* apos = strchr(D->word, '\'');
		if (apos && !(strchr(D->word, '_') || strchr(D->word, ' ')) && D > dictionaryPreBuild[LAYER_0])
		{
			Log(ECHOSTDUSERLOG, " has apostrophe %s\r\n", D->word);
		}
		if (D->word && D->internalBits & UPPERCASE_HASH)
		{
			index = 0;
			unsigned int hash = (D->hash % maxHashBuckets) + 1; // mod by the size of the table
			WORDP E = Index2Word(hashbuckets[hash]);
			while (E != dictionaryBase)
			{
				if (D->hash == E->hash && E->length == D->length && !stricmp(D->word, E->word)) // they match independent of case- 
				{
					list[index++] = E;
				}
				E = dictionaryBase + GETNEXTNODE(E);
			}
			if (index > 1)
			{
				for (int i = 0; i < index; ++i)
				{
					if (i == 2) Log(ECHOSTDUSERLOG, "  ***  ");
					Log(ECHOSTDUSERLOG, " %s  ", list[i]->word);
					list[i]->inferMark = inferMark;
				}
				Log(ECHOSTDUSERLOG, "\r\n");
			}
		}
	}
}

static void C_Word(char* input)
{
	char word[MAX_WORD_SIZE];
	while(ALWAYS)
	{
		input = ReadCompiledWord(input,word);
		if (!*word) break;
		input = SkipWhitespace(input);
		int limit= 0;
		if (IsDigit(*input))
		{
            limit = atoi(word);
            input = ReadCompiledWord(input,word);
		}
		if (*word == '"')
		{
			size_t len = strlen(word);
			word[len-1] = 0;
			memmove(word,word+1,len);
		}
		if (*word == '#')
		{
			uint64 n = FindValueByName(word+1);
			if (n) 
#ifdef WIN32
				Log(STDUSERLOG,(char*)"Value: %I64u  %llx\r\n",n,n);
#else
				Log(STDUSERLOG,(char*)"Value: %llu  %llx\r\n",n,n); 
#endif
			n = FindSystemValueByName(word+1);
			if (n) 
#ifdef WIN32
				Log(STDUSERLOG,(char*)"System: %I64u %llx\r\n",n,n);
#else
				Log(STDUSERLOG,(char*)"System: %llu  %llx\r\n",n,n); 
#endif
			n = FindParseValueByName(word+1);
			if (n) 
#ifdef WIN32
				Log(STDUSERLOG,(char*)"Parse: %I64u  %llx\r\n",n,n);
#else
				Log(STDUSERLOG,(char*)"Parse: %llu  %llx\r\n",n,n); 
#endif
			n = FindMiscValueByName(word+1);
			if (n) 
#ifdef WIN32
				Log(STDUSERLOG,(char*)"Misc: %I64u  %llx\r\n",n,n);
#else
				Log(STDUSERLOG,(char*)"Misc: %llu  %llz\r\n",n,n); 
#endif
		}
		else DumpDictionaryEntry(word,limit);  
	}
} 	

static void WordDump(WORDP D,uint64 flags)
{
	if (D->properties & NOUN &&
		D->properties & VERB &&
		D->systemFlags & NOUN)
		Log(STDUSERLOG,(char*)"%s %d\r\n",D->word,GetMeaningCount(D));
}

static void C_TimeLog(char* input)
{
    FILE* in = FopenReadOnly(input);
    if (!in)
    {
        (*printer)("no such file");
        return;
    }

    int count[64];
    int time[64];
    int min[64];
    int max[64];
	memset(count, 0, sizeof(count));
	memset(time, 0, sizeof(time));
	memset(max, 0, sizeof(max));
	for (int i = 0; i < 64; ++i) min[i] = 100000;
	char bot[64][100];
	memset(bot, 0, sizeof(bot));

    while (ReadALine(readBuffer, in) >= 0)
    {
		char botname[1000];
		char* botptr = strstr(readBuffer, "bot:");
		int botbit = 0;
		if (botptr)
		{
			ReadCompiledWord(botptr + 4, botname);
			int i;
			for (i = 0; i < 64; ++i)
			{
				if (!bot[i][0]) // not allocated yet
				{
					strcpy(bot[i], botname);
                    botbit = i;
					break;
				}
				if (!stricmp(bot[i], botname))
				{
                    botbit = i;
					break;
				}
			}
		}
        char* timeinfo = strstr(readBuffer, "ms Why");
        if (timeinfo)
        {
            ++count[botbit];
            while (IsDigit(*--timeinfo)); 
            int decode = atoi(timeinfo + 1);
            if (decode > max[botbit]) max[botbit] = decode;
            if (decode < min[botbit]) min[botbit] = decode;
            time[botbit] += decode;
        }
    }
    fclose(in);
	for (int i = 0; i < 64; ++i)
	{
		if (!bot[i][0]) break; 
		int avg = time[i] / count[i];
		Log(STDUSERLOG, "%s Lines %d min %d  max %d avg %d\r\n",bot[i], count[i], min[i], max[i], avg);
	}
}

static void C_VerifySentence(char* input)
{
	(*printer)((char*)"\r\nMarkings: \r\n");
	for (int i = 1; i <= wordCount; ++i)
	{
		(*printer)((char*)"%s",wordStarts[i]);
		int start = derivationIndex[i] >> 8;
		int end = derivationIndex[i] & 0x00ff;
		if (start == end && wordStarts[i] == derivationSentence[start]) {;} // unchanged from original
		else // it came from somewhere else
		{
			int startindex = derivationIndex[i] >> 8;
			int endindex = derivationIndex[i] & 0x00Ff;
			(*printer)((char*)"(");
			for (int j = startindex; j <= endindex; ++j)
			{
				if (j != startindex) (*printer)((char*)" ");
				(*printer)("%s",derivationSentence[j]);
			}
			(*printer)((char*)")");
		}
		(*printer)((char*)" %s ",wordCanonical[i]);
	
		char* invert[10000];
		int index = 0;
		HEAPREF list = concepts[i];
		while (list)
		{
            uint64 D;
            list = UnpackHeapval(list, D,discard);
			invert[index++] = ((WORDP)D)->word;
		}

		list = topics[i];
		while (list)
		{
            uint64 D;
            list = UnpackHeapval(list, D, discard);
			invert[index++] = ((WORDP)D)->word;
		}
		while (index--) (*printer)("%s, ",invert[index]);
		(*printer)("\r\n\r\n");
	}
}

#include <map>
using namespace std;
extern std::map <WORDP, int> countData; // per volley index into heap space
static void C_CountWords(char* input)
{
    if (!stricmp(input, "all"))
    {
        C_CountWords("car");
        C_CountWords("computer");
        C_CountWords("legal");
        C_CountWords("appraisals");
        C_CountWords("consumer_electronics");
        C_CountWords("business_and_finance_homework");
        C_CountWords("home_improvement");
        C_CountWords("medical");
        C_CountWords("boat");
        C_CountWords("tax");
        C_CountWords("heavy_equipment");
        C_CountWords("general");
        C_CountWords("engineering");
        C_CountWords("homework");
        return;
    }
    char filename[MAX_WORD_SIZE];
    sprintf(filename, "mlcat/%s.txt", input);
    FILE* in = FopenReadOnly(filename);
    if (!in)
    {
        (*printer)("no such file %s\r\n",input);
        return;
    }
    while (ReadALine(readBuffer, in) >= 0)
    {
        char word[MAX_WORD_SIZE];
        char* ptr = readBuffer;
        while (1)
        {
            ptr = ReadCompiledWord(ptr,word);
            if (!*word) break;
            WORDP D = StoreWord(word);
            countData[D] += 1;
        }
    }
    fclose(in);
    sprintf(filename, "mlcatcount/%s.txt", input);
    FILE* out = FopenUTF8Write(filename);
    // show content:
    for (std::map<WORDP, int>::iterator it = countData.begin(); it != countData.end(); ++it)
    {
        fprintf(out,"%6.6d %s \r\n",it->second,it->first->word);
    }
    fclose(out);
    countData.clear();
    printf("done %s\r\n",input);
}

static void C_CountCat1(char* input) // mark count of cats involved
{
    char filename[MAX_WORD_SIZE];
    sprintf(filename, "mlcatcount/%s.txt", input);
    FILE* in = FopenReadOnly(filename);
    if (!in)
    {
        (*printer)("no such file %s\r\n", input);
        return;
    }
    while (ReadALine(readBuffer, in) >= 0) // read count and word per line
    {
        char word[MAX_WORD_SIZE];
        char* ptr = readBuffer;
        ptr = ReadCompiledWord(ptr, word);
        if (!*word) break;
        int count = atoi(word); // how many times this word in cat 
        ptr = ReadCompiledWord(ptr, word); // the word
        WORDP D = StoreWord(word);
        if (count >= 1) D->counter += 1; // how many files have it significantly
//        if (count > D->counter1) D->counter1 = count; // max
    }
    fclose(in);
}

static void C_CountCat2(char* input)
{
    // discard words going to 4 or more cats with 10 or more counts
    char filename[MAX_WORD_SIZE];
    sprintf(filename, "mlcatcount/%s.txt", input);
    FILE* in = FopenReadOnly(filename);
    if (!in) return;

    sprintf(filename, "mlcatcount/good-%s.txt", input);
    FILE* outgood = FopenUTF8Write(filename);
    sprintf(filename, "mlcatcount/bad-%s.txt", input);
    FILE* outbad = FopenUTF8Write(filename);
    sprintf(filename, "mlcatcount/unique-%s.txt", input);
    FILE* outunique = FopenUTF8Write(filename);
    int accept = 0;
    int reject = 0;
    int unique = 1;
    while (ReadALine(readBuffer, in) >= 0) // read count and word per line
    {
        char word[MAX_WORD_SIZE];
        char* ptr = readBuffer;
        ptr = ReadCompiledWord(ptr, word);
        if (!*word) break;
        int count = atoi(word); // how many times this word in cat 
        ptr = ReadCompiledWord(ptr, word); // the word
        WORDP D = StoreWord(word);
        bool weak = false;
//        if ((count * 5) < D->counter1 && D->counter1 > 100) weak = true;
        if (D->internalBits & UPPERCASE_HASH) weak = true;
        if (!(D->properties & (VERB|NOUN))) weak = true;

        if (weak || D->counter >= 10 || *word == '~' || IsDigitWord(word)) // not enough discrimination
        {
            ++reject;
            fprintf(outbad, "%d %6.6d %s\r\n", D->counter,count, word);
        }
        else
        {
            ++accept;
            fprintf(outgood, "%d %6.6d %s\r\n", D->counter, count, word);
            if (D->counter == 1)
            {
                fprintf(outunique, "%d %6.6d %s\r\n", D->counter, count, word);
                ++unique;
            }
        }
    }
    fprintf(outgood, "%d\r\n", accept);
    fprintf(outbad, "%d\r\n", reject);
    fprintf(outunique, "%d\r\n", unique);

    fclose(in);
    fclose(outgood);
    fclose(outbad);
    fclose(outunique);
}

static void C_CountCat(char* input)
{
    // read words, count how many cats they are in
    C_CountCat1("car");
    C_CountCat1("computer");
    C_CountCat1("legal");
    C_CountCat1("appraisals");
    C_CountCat1("consumer_electronics");
    C_CountCat1("business_and_finance_homework");
    C_CountCat1("home_improvement");
    C_CountCat1("medical");
    C_CountCat1("boat");
    C_CountCat1("tax");
    C_CountCat1("heavy_equipment");
    C_CountCat1("general");
    C_CountCat1("engineering");
    C_CountCat1("homework");

    // read words, sort by good or bad keys
    C_CountCat2("car");
    C_CountCat2("computer");
    C_CountCat2("legal");
    C_CountCat2("appraisals");
    C_CountCat2("consumer_electronics");
    C_CountCat2("business_and_finance_homework");
    C_CountCat2("home_improvement");
    C_CountCat2("medical");
    C_CountCat2("boat");
    C_CountCat2("tax");
    C_CountCat2("heavy_equipment");
    C_CountCat2("general");
    C_CountCat2("engineering");
    C_CountCat2("homework");

    printf("done %s\r\n", input);
}


static void C_WordDump(char* input)
{
	WalkDictionary(WordDump,0);

#ifdef JUNK
	WORDP D = FindWord(input);
	if (!D) 
	{
		Log(STDUSERLOG,(char*)"No such set %s\r\n",input);
		return;
	}
	FACT* F = GetObjectNondeadHead(D);
	while (F)
	{
		if (ValidMemberFact(F))
		{
			if (D->systemFlags & VERB_TAKES_VERBINFINITIVE)
				Log(STDUSERLOG,(char*)"redundant %s\r\n",D->word);
		}
		F = GetObjectNondeadNext(F);
	}
#endif
} 	

//////////////////////////////////////////////////
/// SYSTEM INFO
/////////////////////////////////////////////////

static void FindConceptWord(WORDP D, uint64 pattern)
{
	char* prefix = (char*) pattern;
	if (D->internalBits & CONCEPT && !(D->internalBits & TOPIC))
	{
		if (!*prefix) Log(STDUSERLOG,(char*)"%s\r\n",D->word);
		else if ( MatchesPattern(D->word,prefix)) Log(STDUSERLOG,(char*)"%s\r\n",D->word);
	}
}

static void C_Context(char* input)
{
	int i = contextIndex;
	while (ALWAYS)
	{
		if (--i == -1) i = MAX_RECENT - 1;
		if ( i == (int)contextIndex) break;
		if (InContext(topicContext[i], labelContext[i])) Log(STDUSERLOG,(char*)"%s: %s %d\r\n",GetTopicName(topicContext[i]),labelContext[i],inputContext[i]);
	}
	Log(STDUSERLOG,(char*)"end of contexts");
}

static void C_ConceptList(char* input)
{
	WalkDictionary(FindConceptWord,(uint64) input);
}

static void C_Commands(char* x)
{
	int i = 0;
	CommandInfo *routine;
	while ((routine = &commandSet[++i]) && routine->word) Log(STDUSERLOG,(char*)"%s - %s\r\n",routine->word,routine->comment); // linear search
}

static void C_Definition(char* x)
{
	char name[MAX_WORD_SIZE];
	ReadCompiledWord(x,name);
	WORDP D = FindWord(name);
	if (!D || !(D->internalBits & FUNCTION_NAME)) Log(STDUSERLOG,(char*)"No such name\r\n");
	else if ((D->internalBits & FUNCTION_BITS) == IS_PLAN_MACRO) Log(STDUSERLOG,(char*)"Plan macro\r\n");
	else if (D->x.codeIndex && (D->internalBits & FUNCTION_BITS) != IS_TABLE_MACRO) Log(STDUSERLOG,(char*)"Engine API function\r\n");
	else if ((D->internalBits & FUNCTION_BITS) == IS_OUTPUT_MACRO) Log(STDUSERLOG,(char*)"output macro: %s\r\n",GetDefinition(D)); 
	else Log(STDUSERLOG,(char*)"pattern macro: %s\r\n",GetDefinition(D)); // skip arg count
}

static void DumpMatchVariables()
{
	for (unsigned int i = 0; i <=  MAX_WILDCARDS; ++i)
	{
		Log(STDUSERLOG,(char*)"_%d (%d-%d) =  %s (%s)\r\n",i,WILDCARD_START(wildcardPosition[i]),WILDCARD_END(wildcardPosition[i]),wildcardOriginalText[i],wildcardCanonicalText[i]);  // spot wild cards can be stored
	}
}

static void C_Variables(char* input)
{
	if (!stricmp(input,(char*)"system")) DumpSystemVariables();
    else if (!stricmp(input, (char*)"bot")) DumpUserVariables(false);
    else if (!stricmp(input,(char*)"user")) DumpUserVariables(true);
	else if (!stricmp(input,(char*)"match")) DumpMatchVariables(); 
	else // all
	{
		DumpUserVariables(true);
		DumpSystemVariables();
		DumpMatchVariables(); 
		Log(STDUSERLOG,(char*)"Max Buffers used %d\r\n",maxBufferUsed);
		Log(STDUSERLOG,(char*)"%s\r\n",ShowPendingTopics());
	}
} 	

static void C_Functions(char* input)
{
	DumpFunctions();
}

static void C_Identify(char* input)
{
	IdentifyCode(input);
	Log(STDUSERLOG,(char*)"%s",input);
}

static void ShowMacro(WORDP D,uint64 junk)
{
	if (!(D->internalBits & FUNCTION_NAME)) {;} // not a function or plan
	else if ((D->internalBits & FUNCTION_BITS) == IS_PLAN_MACRO) Log(STDUSERLOG,(char*)"plan: %s (%d)\r\n",D->word,D->w.planArgCount);
	else if (D->x.codeIndex) {;} //is system function (when not plan)
	else if (D->internalBits & IS_PATTERN_MACRO && D->internalBits & IS_OUTPUT_MACRO) Log(STDUSERLOG,(char*)"dualmacro: %s (%d)\r\n",D->word,MACRO_ARGUMENT_COUNT(GetDefinition(D)));
	else if (D->internalBits & IS_PATTERN_MACRO) Log(STDUSERLOG,(char*)"patternmacro: %s (%d)\r\n",D->word,MACRO_ARGUMENT_COUNT(GetDefinition(D)));
	else if (D->internalBits & IS_OUTPUT_MACRO) 	Log(STDUSERLOG,(char*)"outputmacro: %s (%d)\r\n",D->word,MACRO_ARGUMENT_COUNT(GetDefinition(D)));
	else if (D->internalBits & IS_PLAN_MACRO) Log(STDUSERLOG,(char*)"tablemacro: %s (%d)\r\n",D->word,MACRO_ARGUMENT_COUNT(GetDefinition(D)));
}

static void C_Macros(char* input)
{
	WalkDictionary(ShowMacro,0);
}

static void ShowQuery(WORDP D,uint64 junk)
{
	if (D->internalBits & QUERY_KIND) 
	{
		if (D->internalBits & BUILD0) Log(STDUSERLOG,(char*)"BUILD0 ");
		if (D->internalBits & BUILD1) Log(STDUSERLOG,(char*)"BUILD1 ");
		if (D->internalBits & BUILD2) Log(STDUSERLOG,(char*)"BUILD2 ");
		Log(STDUSERLOG,(char*)"Query: %s \"%s\"\r\n",D->word,D->w.userValue);
	}
}

static void C_Queries(char* input)
{
	WalkDictionary(ShowQuery,0);
}

static void ClearTracedFunction(WORDP D,uint64 junk)
{
	if (D->internalBits & FUNCTION_BITS && D->internalBits & MACRO_TRACE) D->internalBits ^= MACRO_TRACE;
}

static void TimeFunction(WORDP D, int bits, int mode) 
{
	unsigned int i;
	for (i = 0; i < tracedFunctionsIndex; ++i)
	{
		if (tracedFunctionsList[i] == D) break;
	}
	if (i >= tracedFunctionsIndex) tracedFunctionsList[tracedFunctionsIndex++] = D;
	if (mode > 0) D->internalBits |= bits;
	else if (mode < 0) D->internalBits ^= bits;
	else D->internalBits &= -1 ^ bits;

}

static void TraceFunction(WORDP D, int bits)
{
	unsigned int i;
	for (i = 0; i < tracedFunctionsIndex; ++i)
	{
		if (tracedFunctionsList[i] == D) break;
	}
	if (i >= tracedFunctionsIndex) tracedFunctionsList[tracedFunctionsIndex++] = D;
	D->inferMark = bits;
	D->internalBits ^= MACRO_TRACE;
}

static void TimedFunction(WORDP D, uint64 junk) // functions
{
	if (D->internalBits & FUNCTION_BITS) {
		if ((D->internalBits & FN_TIME_BITS) == MACRO_TIME) Log(ECHOSTDUSERLOG, (char*)"%s: on\r\n", D->word);
		if ((D->internalBits & FN_TIME_BITS) == (MACRO_TIME | NOTIME_FN)) Log(ECHOSTDUSERLOG, (char*)"%s: on off\r\n", D->word);
		if ((D->internalBits & FN_TIME_BITS) == NOTIME_FN) Log(ECHOSTDUSERLOG, (char*)"%s: off\r\n", D->word);
	
		if (D->internalBits & FN_TIME_BITS) isTracing = true;
	}
}

static void ClearTimedFunction(WORDP D, uint64 junk)
{
	if (D->internalBits & FUNCTION_BITS && D->internalBits & MACRO_TIME) D->internalBits ^= MACRO_TIME;
}

static void C_TimedFunctions(char* input)
{
	WalkDictionary(TimedFunction, 0);
}

static void TracedTopic(WORDP D,uint64 style)
{
	if (D->internalBits & TOPIC) 
	{
		int topic = FindTopicIDByName(D->word);
		topicBlock* block = TI(topic);
		if (block->topicDebug) 
		{
			Log(ECHOSTDUSERLOG,(char*)"%s",D->word);
			if (style == 0) Log(ECHOSTDUSERLOG, (char*)"  0x%x", block->topicDebug);
			Log(ECHOSTDUSERLOG, (char*)"\r\n");
			if (style == 1) ShowTrace(block->topicDebug, false);
			isTracing = true;
		}
		if (D->internalBits & NOTRACE_TOPIC) {
			Log(ECHOSTDUSERLOG, (char*)"%s:", D->word);
			if (style == 1) Log(ECHOSTDUSERLOG, (char*)"\r\n");
			Log(ECHOSTDUSERLOG, (char*)"  Not tracing\r\n");
		}
	}
}

static void TracedFunction(WORDP D,uint64 style)
{
	if (D->internalBits & MACRO_TRACE) 
	{
		if (D->inferMark) 
		{
			Log(ECHOSTDUSERLOG,(char*)"%s",D->word);
			if (style == 0) Log(ECHOSTDUSERLOG, (char*)"  0x%x", D->inferMark);
			Log(ECHOSTDUSERLOG, (char*)"\r\n");
			if (style == 1) ShowTrace(D->inferMark, false);
			isTracing = true;
		}
		if (D->internalBits & NOTRACE_FN) {
			Log(ECHOSTDUSERLOG, (char*)"%s:", D->word);
			if (style == 1) Log(ECHOSTDUSERLOG, (char*)"\r\n");
			Log(ECHOSTDUSERLOG, (char*)"  Not tracing\r\n");
		}
	}
}

static void C_TracedFunctions(char* input)
{
	WalkDictionary(TracedFunction, 0);
}

static void ClearTracedTopic(WORDP D,uint64 junk)
{
	if (D->internalBits & TOPIC) 
	{
		int topicid = FindTopicIDByName(D->word);
		topicBlock* block = TI(topicid);
		block->topicDebug = 0;
	}
}

static void C_TracedTopics(char* input)
{
	WalkDictionary(TracedTopic,0);
}

static void TimedTopic(WORDP D, uint64 style)
{
	if (D->internalBits & TOPIC)
	{
		int topicid = FindTopicIDByName(D->word);
		topicBlock* block = TI(topicid);
		if (block->topicTiming)
		{
			Log(ECHOSTDUSERLOG, (char*)"%s:", D->word);
			if (style == 0) Log(ECHOSTDUSERLOG, (char*)"  0x%x", block->topicTiming);
			Log(ECHOSTDUSERLOG, (char*)"\r\n");
			if (style == 1) ShowTiming(block->topicTiming, false);
			isTiming = true;
		}
		if (D->internalBits & NOTIME_TOPIC) {
			Log(ECHOSTDUSERLOG, (char*)"%s:", D->word);
			if (style == 1) Log(ECHOSTDUSERLOG, (char*)"\r\n");
			Log(ECHOSTDUSERLOG, (char*)"  Not timing\r\n");
		}
	}
}

static void ClearTimedTopic(WORDP D, uint64 junk)
{
	if (D->internalBits & TOPIC)
	{
		int topicid = FindTopicIDByName(D->word);
		topicBlock* block = TI(topicid);
		block->topicTiming = 0;
	}
}

static void C_TimedTopics(char* input)
{
	WalkDictionary(TimedTopic,0);
}

void C_MemStats(char* input)
{
	unsigned int factUsedMemKB = ( factFree-factBase) * sizeof(FACT) / 1000;
	unsigned int dictUsedMemKB = ( dictionaryFree-dictionaryBase) * sizeof(WORDENTRY) / 1000;
	// dictfree shares text space
	unsigned int textUsedMemKB = ( heapBase-heapFree)  / 1000;
	unsigned int bufferMemKB = (maxBufferLimit * maxBufferSize) / 1000;
	
	unsigned int used =  factUsedMemKB + dictUsedMemKB + textUsedMemKB + bufferMemKB;
	used +=  (userTopicStoreSize + userTableSize) /1000;

	char buf[MAX_WORD_SIZE];
	strcpy(buf,StdIntOutput(factFree-factBase));
	Log(STDUSERLOG,(char*)"Used: words %s (%dkb) facts %s (%dkb) heap %dkb buffers %d overflowBuffers %d max Output used %d\r\n",
		StdIntOutput(dictionaryFree-dictionaryBase), 
		dictUsedMemKB,
		buf,
		factUsedMemKB,
		textUsedMemKB,
		bufferIndex,overflowIndex,maxOutputUsed);

	unsigned int factFreeMemKB = ( factEnd-factFree) * sizeof(FACT) / 1000;
	unsigned int textFreeMemKB = ( heapFree- heapEnd) / 1000;
	Log(STDUSERLOG,(char*)"Free:  fact %dKb stack/heap %dKB\r\n",factFreeMemKB,textFreeMemKB);
	Log(STDUSERLOG,(char*)"MinReleaseStackGap %dMB minHeapAvailable %dMB maxBuffers used %d of %d  maxglobaldepth %d\r\n\r\n",maxReleaseStackGap/1000000,minHeapAvailable/1000000,maxBufferUsed,maxBufferLimit,maxGlobalSeen);
}

static void C_Who(char*input)
{
#ifdef WIN32
    Log(STDUSERLOG, (char*)"%s talking to %s %I64u\r\n", loginID, computerID,myBot);
#else
    Log(STDUSERLOG, (char*)"%s talking to %s %llu\r\n", loginID, computerID,myBot);
#endif
}

//////////////////////////////////////////////////////////
//// COMMAND SYSTEM
//////////////////////////////////////////////////////////

void InitCommandSystem() // set dictionary to match builtin functions
{
}

TestMode Command(char* input,char* output,bool scripted)
{
	char word[MAX_WORD_SIZE];
	*word = 0;
	fromScript = scripted;
	bool oldecho = echo;
	if (!scripted) echo = true;	// see outputs sent to log file on console also
	static bool commandsAllowed = true;		// local suppression
	
	if (!commandsAllowed && !stricmp(input,(char*)":commands on")) 
	{
		commandsAllowed =  true;
		if (!scripted) Log(STDUSERLOG,(char*)":commands enabled\r\n");
		echo = oldecho;
		return COMMANDED;
	}

	if (!stricmp(input,(char*)":commands off"))
	{
		commandsAllowed = false;
		if (!scripted) Log(STDUSERLOG,(char*)":commands disabled\r\n");
		echo = oldecho;
		return COMMANDED;
	}
	if (!commandsAllowed) 
	{
		echo = oldecho;
		return FAILCOMMAND;
	}

	int i = 0;
	CommandInfo *routine = NULL;
	input = SkipWhitespace(input);
	while ((routine = &commandSet[++i]) && routine->word) 
	{
		size_t len = strlen(routine->word);
		if (!strnicmp(routine->word,input,len) && !IsLegalNameCharacter(input[len])) break;
		if (input[1] == routine->word[1] && input[2] == routine->word[len-1] && !IsAlphaUTF8(input[3])) break; // 2 char abbrev, not unique
	}
	if (routine->word) 
	{
		CommandInfo* info;
		info = &commandSet[i];
		input = SkipWhitespace(input+strlen(info->word));
		char* data = AllocateBuffer();
		strcpy(data,input);
		TrimSpaces(data,false); // safe from change
		wasCommand = COMMANDED;
		testOutput = output;
		if (output) *output = 0;
		(*info->fn)(data);
		testOutput = NULL;
		FreeBuffer();
		if (strcmp(info->word,(char*)":echo") && prepareMode == NO_MODE && !(trace & TRACE_TREETAGGER)) echo = oldecho;
		if (scripted && strcmp(info->word,(char*)":echo")) echo = oldecho;
		return wasCommand;
	}
	echo = oldecho;
	return FAILCOMMAND; 
}

//////////////////////////////////////////////////////////
//// TOPIC INFO
//////////////////////////////////////////////////////////

void C_Gambits(char* buffer)
{
	buffer = SkipWhitespace(buffer);
	int topicid = FindTopicIDByName(buffer);
	if (!topicid)
	{
		Log(STDUSERLOG,(char*)"No such topic %s\r\n",buffer);
		return;
	}
	
	char* base = GetTopicData(topicid);
	int ruleID = 0;
	topicBlock* block = TI(topicid);
	unsigned int* map = block->gambitTag;
	ruleID = *map;
	unsigned int* indices =  block->ruleOffset;
	unsigned int n = 0;
	while (ruleID != NOMORERULES)
	{
		char* ptr = base + indices[ruleID]; // the gambit 
		char* end = strchr(ptr,ENDUNIT);
		*end = 0;
		++n;
		char label[MAX_WORD_SIZE];
		char pattern[MAX_WORD_SIZE];
		char* output = GetPattern( ptr,label,pattern);
		if (strlen(pattern) == 4) *pattern = 0;
		if (*label) strcat(label,(char*)":");
		if (!UsableRule(topicid,ruleID)) Log(STDUSERLOG,(char*)"- %d %s %s    %s\r\n",n,label,output,pattern);
		else Log(STDUSERLOG,(char*)"%d  %s %s    %s\r\n",n,label,output,pattern);
		*end = ENDUNIT;
		ruleID = *++map;
	}
}

void C_Pending(char* buffer)
{
	Log(STDUSERLOG,(char*)"Pending topics: %s\r\n", ShowPendingTopics());
}

static int functionCounter;
static void CountConcept(WORDP D, uint64 count)
{
	if (D->internalBits & CONCEPT && !(D->internalBits & TOPIC))
	{
		unsigned int* ctr = (unsigned int*) count;
		++*ctr;
	}

    if (D->word && *D->word == '^' && D->internalBits & FUNCTION_BITS) 
    {
        if (D->x.codeIndex && !(D->internalBits & (IS_PLAN_MACRO | IS_TABLE_MACRO))) {}// system function 
        else ++functionCounter;
    }
}

static bool EmptyReuse(char* output, int topicid)
{
	if (!strnicmp(output,(char*)"^reuse ((char*)",8) && !strchr(output,'.')) // dont care about cross topic jumps
	{
		char label[MAX_WORD_SIZE];
		ReadCompiledWord(output+8,label);
		bool fulllabel = false;
		bool crosstopic = false;
		int id;
		char* rule = GetLabelledRule(topicid,label,(char*)"1",fulllabel,crosstopic,id, topicid);
		if (rule)
		{
			char pattern[MAX_WORD_SIZE];
			char* output1 = SkipWhitespace(GetPattern(rule,label,pattern));
			return (*output1 == ENDUNIT);
		}
	}
	return false;
}

static void C_TopicStats(char* input)
{
	unsigned int totalgambits = 0;
	unsigned int totalresponders = 0;
	unsigned int totalrejoinders = 0;
	unsigned int totalquestions = 0;
	unsigned int totalstatements = 0;
	unsigned int totalempties = 0;
	unsigned int totaldual = 0;
	unsigned int conceptCount = 0;
    functionCounter = 0;
	bool normal = false;
	if (!stricmp(input,(char*)"normal")) // show only normal topics
	{
		normal = true;
		*input = 0;
	}
	WalkDictionary(CountConcept,(uint64) &conceptCount); // also counts functions
	int topicCount = 0;

	size_t len = 0;
	char* x = strchr(input,'*');
	if (x) len = x - input;
	else if (*input == '~') len = strlen(input);

	for (int i = 1; i <= numberOfTopics; ++i) 
	{
		if (len && strnicmp(GetTopicName(i),input,len)) continue;
		char* name = GetTopicName(i);
		char* data = GetTopicData(i);
		unsigned int flags = GetTopicFlags(i);
		if (flags & TOPIC_SYSTEM && normal) continue;
		++topicCount;
		unsigned int gambits = 0;
		unsigned int responders = 0;
		unsigned int rejoinders = 0;
		unsigned int empties = 0;
		int id = 0;
		while (data && *data)
		{
			char label[MAX_WORD_SIZE];
			char pattern[MAX_WORD_SIZE];
			char* output = SkipWhitespace(GetPattern(data,label,pattern));
			bool norule = EmptyReuse(output,i);
			if (!*output || *output == ENDUNIT || norule) 
			{
				if (*data < 'a' ||*data > 'q') 
					++empties; // we dont care if rejoinder is empty.
				else  ++rejoinders;
			}
			else if (TopLevelGambit(data)) ++gambits;
			else if (TopLevelRule(data)) ++responders;
			else ++rejoinders;

			if (*data == QUESTION) ++totalquestions;
			else if (*data == STATEMENT) ++totalstatements;
			else if (*data == STATEMENT_QUESTION) ++totaldual;

			data = FindNextRule(NEXTRULE,data,id);
		}
		totalgambits += gambits;
		totalresponders += responders;
		totalrejoinders += rejoinders;
		totalempties += empties;
		Log(STDUSERLOG,(char*)"    %s     gambits %d responders %d rejoinders %d empties %d\r\n", name,gambits,responders,rejoinders,empties);
	}
	unsigned int totalrules = totalgambits + totalresponders + totalrejoinders;
	Log(STDUSERLOG,(char*)"Concepts %d Topics %d Functions %d rules %d empties %d\r\n  gambits %d  responders %d (?: %d s: %d  u: %d) rejoinders %d  \r\n",conceptCount,topicCount,functionCounter,totalrules,totalempties,totalgambits,totalresponders,totalquestions,totalstatements,totaldual,totalrejoinders);
}

static void MarkExcludeIt(WORDP D) // mark all members to exclude
{
    FACT* F = GetObjectHead(D);
    MEANING memx = MakeMeaning(StoreWord((char*)"member"));
    while (F)
    {
        if (F->verb == memx)
        {
            WORDP X = Meaning2Word(F->subject);
            if (X->inferMark != inferMark)
            {
                X->inferMark = inferMark;
                if (*X->word == '~') MarkExcludeIt(X);
            }
        }
        F = GetObjectNext(F);
    }
}

static int follown;
static FILE* outptr;

static void FollowIt(WORDP D)
{
    FACT* F = GetObjectHead(D);
    MEANING memx = MakeMeaning(StoreWord((char*)"member"));
    while (F)
    {
        if (F->verb == memx)
        {
            WORDP X = Meaning2Word(F->subject);
            if (X->inferMark != inferMark)
            {
                X->inferMark = inferMark;
                if (*X->word == '~') FollowIt(X);
                else
                {
                    ++follown;
                    fprintf(outptr, "%s\r\n", X->word);
                }

            }
        }
        F = GetObjectNext(F);
    }
}

static void C_Ingestlog(char* input)
{
    FILE* in = FopenReadOnly(input);
    int n = 0;
    char word[MAX_WORD_SIZE];
    char user[MAX_WORD_SIZE];
    char bot[MAX_WORD_SIZE]; 
    char* message;
    uint64 starttime = ElapsedMilliseconds();
    if (!in)
    {
        (*printer)("no such file");
        return;
    }
/*
Start: user:37224444 bot:Pearl ip:184.106.28.86 (~introductions) 0 ==> [alarm=0|loopback=0|why=~introductions.1.0.~control.3.0 ~introductions.1.0.~control.3.0 ~introductions.4.0=ASKTOPIC.~introductions.1.0 ~introductions.1.0.~control.3.0 |v=33.0 ] Hello, I'm Pearl Wilson, the Expert's Assistant on JustAnswer.   How can I help?  When:May02 18:24:25 5ms Version:8.0 Build0:Apr27'18-11:12:01 Build1:Apr27'18-07:00:21 Why:~xpostprocess.5.0.~control.9.0 ~introductions.1.0.~control.3.0 ~introductions.1.0.~control.3.0 ~introductions.4.0=ASKTOPIC.~introductions.1.0 ~introductions.1.0.~control.3.0
ServerPre: pid: 18682 37224444 (Pearl) [reset type: FunnelQuestion category: consumer_electronics partner: 1 source: sip DeviceCategory: desktop] May 02 18:24:25 2018
Respond: user:37224444 bot:Pearl ip:184.106.28.86 (~consumer_electronics_expert) 0 [reset type: FunnelQuestion category: consumer_electronics partner: 1 source: sip DeviceCategory: desktop]  ==> [assistanttitle=Technician's Assistant|assistantname=Pearl Wilson|why=~consumer_electronics_expert.491.0=GENERAL_START.~handle_oob.12.0=GIVEN_CATEGORY |v=33.0 ] Welcome! How can I help with your electronics question?  When:May02 18:24:25 8ms Why:~xpostprocess.5.0.~control.9.0 ~consumer_electronics_expert.491.0=GENERAL_START.~handle_oob.12.0=GIVEN_CATEGORY
*/
    while (ReadALine(readBuffer, in) >= 0)
    {
        if (!*readBuffer) continue;
        char* ptr;
        char* userp = strstr(readBuffer, "user:");
        if (!userp) continue;
        ReadCompiledWord(userp + 5, user);
        char* botp = strstr(readBuffer, "bot:");
        if (!botp) continue;
        if (botp[4] == ' ') // default bot
        {
            *bot = 0;
        }
        else ReadCompiledWord(botp + 4, bot);
        char* oob = strchr(readBuffer, '[');
        char* when = strstr(readBuffer, "When:");
        char* output = strstr(readBuffer, "==>");
        ptr = ReadCompiledWord(readBuffer, word);

        if (!stricmp(word, "start:"))
        {
            message = "";
        }
        else if (!stricmp(word, "respond:"))
        {
            message = oob;
            *output = 0;
        }
        else continue;
        printf("%d\r\n", n++);
        uint64 starttime = ElapsedMilliseconds();
        PerformChat(user, bot, message, "x", ourMainOutputBuffer);
        LogChat(starttime, user, bot, "x", volleyCount, message, ourMainOutputBuffer);
    }
    fclose(in);
    uint64 endtime = ElapsedMilliseconds();
    long diff = (long) ( endtime - starttime);
    long msPerMessage = diff / n; 
    diff -= 1000;
    printf("Did %d messages in %ld seconds or %d ms per message", n,diff, msPerMessage);
}

static void C_AllMembers(char* input)
{
    MEANING memx = MakeMeaning(StoreWord((char*)"member"));
    char concept[MAX_WORD_SIZE];
    outptr = FopenUTF8Write("TMP/tmp.txt");
    char* ptr = ReadCompiledWord(input,concept);
    WORDP D = FindWord(concept);
    WORDP* members = (WORDP*)malloc(150000);
    WORDP* base = members;
    follown = 0;
    if (!D)
    {
        printf("%s not found\r\n",concept);
        return;
    }

    // mark excluded
    char* more = ptr;
    char excludeSet[MAX_WORD_SIZE];
    NextInferMark();
    while (more && *more)
    {
        more = ReadCompiledWord(more, excludeSet);
        WORDP C = FindWord(excludeSet);
        if (!C)
        {
            printf("missing concept %s\r\n", excludeSet);
            return;
        }
        MarkExcludeIt(C); // all members of this get marks
    }

    FollowIt(D);
    fclose(outptr);
    printf("%d entries now in tmp/tmp.txt\r\n", follown);
}

static void C_Dedupe(char* input)
{
	FILE* in = FopenReadOnly(input);
	char fname[200];
	char* charx = strrchr(input, '/');
	if (charx) *charx = 0; 
	sprintf(fname, "%s/%s", tmp,input);
	if (!in)
	{
		(*printer)("no such file");
		return;
	}
	FILE* out = FopenUTF8Write(fname);
	while (ReadALine(readBuffer, in) >= 0)
	{
		if (!*readBuffer) continue;
        char* line = TrimSpaces(readBuffer); // remove leading/trailing spaces
		char* comment = line;
		while ((comment = strchr(comment, '#')))
		{
			if (comment[1] == ' ' && *(comment - 1) == ' ')
			{
				*(comment - 1) = 0;
				break;
			}
			comment += 1;
		}
		WORDP D = StoreWord(readBuffer,AS_IS);
		if (D->internalBits & FAKE_NOCONCEPTLIST) continue;
		D->internalBits |= FAKE_NOCONCEPTLIST;
		fprintf(out, "%s\r\n", readBuffer);
	}
	(*printer)("done\r\n");
	fclose(out);
	fclose(in);
}

#define VAR_GLBLPERMANENT 1
#define VAR_GLBLTRANSIENT 2
#define VAR_LOCAL 4
char* FindVar(char* ptr, FILE* out, int use)
{
    char* begin = ptr;
    char* var;
    char name[MAX_WORD_SIZE];
    if (use & (VAR_GLBLTRANSIENT| VAR_LOCAL| VAR_GLBLPERMANENT)) 
        while ((var = strchr(ptr, '$')))
    {
        char* start = var;
        ptr = start + 2; // start after any $_ or $$ for next var
        if (IsDigit(start[1])) continue;    // $5  currency or accelerator?
        if (start[1] == '$' || start[1] == '_') var += 2;
        
        if (start[1] == '_' && !(use & VAR_LOCAL)) continue;
        if (start[1] == '$' && !(use & VAR_GLBLTRANSIENT)) continue;
        if (start[1] != '$' && start[1] != '_' && !(use & VAR_GLBLPERMANENT)) continue;
        char* end = start;
        int brackets = 0;
        while (IsLegalNameCharacter(*++end) || *end == '.' || *end == '$' || *end == '\\' || *end == '[' || (*end == ']' && brackets))
        {
            if (*end == '[') ++brackets;
            else if (*end == ']') --brackets;
        }
        strncpy(name, start, end - start);
        name[end - start] = 0;
        ptr = end;
        if (!name[1]) continue;
        // $xxx:=y  or   $xxx_=_ or $xxx_+=
        if (end[1] == '=')
        {
            if (*end == ':' || *end == '=' || *end == '<' || *end == '>' || *end == '&' ||
                *end == '&' || *end == '*' || *end == '+' || *end == '-' || *end == '/' ||
                *end == ' ' || end[2] == ' ') fprintf(out, "%s +\r\n", name);
        }
        else if (end[1] != '=' && end[2] == '=') fprintf(out, "%s +\r\n", name);
        else if (end[2] == '=' && end[1] != '=') fprintf(out, "%s +\r\n", name);
        else fprintf(out, "%s\r\n", name);
    }

    return ptr + strlen(ptr);
}

static void C_ListVariables(char* input)
{
    char fname[200];
    int use = 0;
    if (strstr(input, "local")) use |= VAR_LOCAL;
    if (strstr(input, "global")) use |= VAR_GLBLPERMANENT | VAR_GLBLTRANSIENT;
    if (strstr(input, "permanent")) use |= VAR_GLBLPERMANENT ;
    if (strstr(input, "transient")) use |= VAR_GLBLTRANSIENT;
    if (strstr(input, "all")) use |=  VAR_GLBLPERMANENT | VAR_GLBLTRANSIENT;
    if (use == 0) use = -1;

    sprintf(fname, "%s/variables.txt", tmp);
    FILE* out = FopenUTF8Write(fname);

    for (int i = 1; i <= numberOfTopics; ++i)
    {
        char* ptr = GetTopicData(i);
        while (ptr && *ptr) //   find all choices-- layout is like "t: xxx () yyy"  or   "u: () yyy"  or   "t: this is text" -- there is only 1 space before useful label or data
        {
            char* end = strchr(ptr, '`');
            if (!end) break;
            *end = 0;
            ptr = strchr(ptr, ':') + 1; // after rule type
            FindVar(ptr, out,use);
            *end = '`';
            ptr = end + 1;
        }
    }

    FILE* in = FopenReadOnly("TOPIC/BUILD1/macros1.txt");
    if (in)
    {
        while (ReadALine(readBuffer, in) >= 0) //   ^showfavorite O 2 _0 = ^0 _1 = ^1 ^reuse (~xfave FAVE ) 
        {
            char* ptr = strchr(readBuffer, ')' + 1);
            while (ptr && *ptr ) ptr = FindVar(ptr, out,use);
        }
        fclose(in);
    }
   // ^pearl_31_14_bot o 0 0 A() $cs_token = 70368744177664 | 4096 | 255 | 2048 | 24576 | 256 $std_cstoken = $cs_token $cartoken = $cs_token $userprompt = ^"%user: >" $botprompt = ^"PEARL: " $homepage = 1 $cs_utcoffset = -7 ^ addtopic(~homepage) $cs_control_main = ~control $cs_prepass = ~xpreprocess $cs_control_post = ~xpostprocess $cs_userfactlimit = 1000 $cs_botid = 4 `
    in = FopenReadOnly("TOPIC/BUILD1/macros0.txt");
    if (in)
    {
        while (ReadALine(readBuffer, in) >= 0) //   ^showfavorite O 2 _0 = ^0 _1 = ^1 ^reuse (~xfave FAVE ) 
        {
            char* ptr = strchr(readBuffer, ')' + 1);
            while (ptr && *ptr) ptr = FindVar(ptr, out,use);
        }
        fclose(in);
    }


    fclose(out);
    printf("done\r\n");
}

static void C_TopicDump(char* input)
{
	char fname[200];
	sprintf(fname, "%s/tmp.txt",tmp);
	FILE* out = FopenUTF8Write(fname);
	size_t len = 0;
	char* x = strchr(input,'*');
	if (x) len = x - input;
	else if (*input == '~') len = strlen(input);

	for (int i = 1; i <= numberOfTopics; ++i) 
	{
		char* name = GetTopicName(i);
		if (!*name) continue;
		if (len && strnicmp(name,input,len)) continue;
		topicBlock* block = TI(i);
		fprintf(out,(char*)"topic: %s %s Bot: %s\r\n",name,DisplayTopicFlags(i),block->topicRestriction ? block->topicRestriction : (char*)"all ");
		// dump keywords
		WORDP D = FindWord(name);
		FACT* F = GetObjectNondeadHead(MakeMeaning(D));
		fprintf(out,(char*)"%s",(char*)"Keywords: ");
		while (F)
		{
			if (ValidMemberFact(F))  fprintf(out,(char*)"%s ",Meaning2Word(F->subject)->word);
			F = GetObjectNondeadNext(F);
		}
		fprintf(out,(char*)"%s",(char*)" Rules: \r\n");
		// dump rules
		char* data = GetTopicData(i);
		int id = 0;
		while (data && *data)
		{
			char* end = strchr(data,ENDUNIT);
			*end = 0;
			fprintf(out,(char*)"%s%c\r\n",data-JUMP_OFFSET,ENDUNIT);
			*end = ENDUNIT;
			data = FindNextRule(NEXTRULE,data,id);
		}
		fprintf(out,(char*)"%s",(char*)"000 x\r\n"); // end of topic
	}
	FClose(out);
	Log(STDUSERLOG,(char*)"Done.\r\n");
}

static bool shownItem;

static void TrackFactsUp(MEANING T,FACT* G,WORDP base) //   show what matches up in unmarked topics
{ 
    if (!T) return;
	WORDP D = Meaning2Word(T);
	unsigned int index = Meaning2Index(T);
	unsigned int flags = GETTYPERESTRICTION(T);
	if (!flags) flags = ESSENTIAL_FLAGS;
	if (D->internalBits & TOPIC) // is in some other topic
	{
		if (D->inferMark == inferMark) return;
		D->inferMark = inferMark;
		flags = GetTopicFlags(FindTopicIDByName(D->word));
		if (flags & TOPIC_SYSTEM) return;	// dont report system intersects
		char* word = AllocateStack(NULL, maxBufferSize);
		if (!shownItem)
		{
			shownItem = true;
			Log(STDUSERLOG,(char*)"  %s: ",base->word);
		}
		if (!G){;}
		else if (Meaning2Word(G->subject) == base) sprintf(word,(char*)" %s ",D->word);
		else sprintf(word,(char*)" %s(%s)",D->word,WriteMeaning(G->subject));
		Log(STDUSERLOG,(char*)"%s ",word);
		ReleaseStack(word);
		return;	
	}
	else if (D->internalBits & CONCEPT)
	{
		if (D->inferMark == inferMark) return;	// already marked
		D->inferMark = inferMark;
	}
	FACT* F = GetSubjectNondeadHead(D); 
	while (F) 
	{
		
		WORDP object = Meaning2Word(F->object);
		if ((ValidMemberFact(F) || F->verb == Mis) && object->inferMark != inferMark)
		{
			unsigned int restrict = GETTYPERESTRICTION(F->subject);
			if (restrict) // type restricted member
			{
				if (!( restrict & flags ))
				{
					F = GetSubjectNondeadNext(F);
					continue;
				}
			}

			//  meaning restriction 
			if (index == Meaning2Index(F->subject)) // match generic or specific 
			{
				WORDP E = Meaning2Word(F->subject);
				if (*E->word == '~') TrackFactsUp(F->object,G,base);
				else TrackFactsUp(F->object,F,base);
			}
		}
		F = GetSubjectNondeadNext(F);
	}
}

static void TabInset(unsigned int depth,bool eol)
{
	if (eol) Log(STDUSERLOG,(char*)"\r\n");
	for (unsigned int i = 0; i < depth; ++i) Log(STDUSERLOG,(char*)"  ");
}

static void TrackFactsDown(MEANING M,FACT* F,unsigned int depth,size_t& length,bool display) // look at each keyword of this set
{
	WORDP D = Meaning2Word(M);
	if (D->inferMark == inferMark) return;	// already marked
	D->inferMark = inferMark;
	if (shownItem)
	{
		shownItem = false;
		Log(STDUSERLOG,(char*)"\r\n");
	}
	if (*D->word == '~')  // its a set or topic-- nest and do the set
	{
		if (display)
		{
			if ( length != depth)  TabInset(depth,true);
			// header
			Log(STDUSERLOG,(char*)"%s\r\n",D->word);
			// indent 
			TabInset(depth+2,true);
			length = depth + 2;
		}
		else TrackFactsUp(M,F,D); // what is it a member of
	    // concept keywords
		FACT* G = GetObjectNondeadHead(D);
		while (G)
		{
			if (G->verb == Mmember)	TrackFactsDown(G->subject,G,depth+2,length,display); // what is a member of this concept
			G = GetObjectNondeadNext(G);
		}
		if (display)
		{
			TabInset(depth,true); // end of concept keywords // restore indent 
			length = depth * 2;
		}
	}
	else // displaying a word of a set
	{
		unsigned int index = Meaning2Index(M);
		if (display)
		{
			char word[MAX_WORD_SIZE];
			if (!index)	sprintf(word,(char*)"%s ",D->word);
			else sprintf(word,(char*)"%s~%d ",D->word,index);
			Log(STDUSERLOG,(char*)"%s",word);
			size_t wlen = strlen(word)  + 1;
			length += wlen;
			while (wlen < 20) // force each word to be 20 wide
			{
				Log(STDUSERLOG,(char*)" ");
				++wlen;
				++length;
			}
			if (length > 120) // avoid long lines
			{
				TabInset(depth,true);
				length = depth * 2;
			}
		}
		else if (index) // need to propogate down - but might be huge-- dont display
		{
			int l = 0;
			M = GetMaster(M); // master meaning
			while (++l) //   find the children of the meaning of T
			{
				MEANING child = FindChild(M,l);
				if (!child) break;
				TrackFactsDown(child,F,depth+2,length,false);
			} //   end of children for this value
		}
		else
		{
			FACT* G = GetSubjectNondeadHead(D); // who comes from this word
			while (G)
			{
				if (G->verb == Mmember)	TrackFactsUp(G->object,G,D); 
				G = GetSubjectNondeadNext(G);
			}
			unsigned int size = GetMeaningCount(D); // all meanings up
			for  (unsigned int k = 1; k <= size; ++k)
			{
				MEANING M1 = GetMeaning(D,k);
				TrackFactsUp(M1,F,D); // anyone else refers to this meaning?
				MEANING parent = FindSetParent(M1,0); //   next set we are member of
				TrackFactsUp(parent,F,D);
			}
		}
	}
}

static void C_Topics(char* input)
{
	PrepareSentence(input,true,true);	
	impliedSet = 0;
	KeywordTopicsCode(NULL);
	for (unsigned int i = 1; i <=  FACTSET_COUNT(0); ++i)
	{
		FACT* F = factSet[0][i];
		WORDP D = Meaning2Word(F->subject);
		WORDP N = Meaning2Word(F->object);
		int topic = FindTopicIDByName(D->word);
        char* name = GetTopicName(topic);
		Log(STDUSERLOG,(char*)"%s (%s) : ",name,N->word);
        //   look at references for this topic
        int start = -1;
		int startPosition = 0;
		int endPosition = 0;
        while (GetIthSpot(D,++start, startPosition,endPosition)) // find matches in sentence
        {
            // value of match of this topic in this sentence
            for (int k = startPosition; k <= endPosition; ++k) 
			{
				if (k != startPosition) Log(STDUSERLOG,(char*)"_");
				Log(STDUSERLOG,(char*)"%s",wordStarts[k]);
			}
			Log(STDUSERLOG,(char*)" ");
		}
		Log(STDUSERLOG,(char*)"\r\n");
	}
	impliedSet = ALREADY_HANDLED;
	
}

static void C_TopicInfo(char* input)
{
	char word[MAX_WORD_SIZE];
	char* ptr = ReadCompiledWord(input,word);
	if (*word == '~' && word[1] == 0) 
	{
		if (inputRejoinderTopic == NO_REJOINDER) return;
		strcpy(word,GetTopicName(inputRejoinderTopic));
		input = ptr;
	}
	else if (*word == '~')  input = ptr;

	size_t len = 0;
	char* x = strchr(word,'*');
	if (x) len = x - word;
	else if (*word == '~') len = strlen(word);

	for (int topicid = 1; topicid <= numberOfTopics; ++topicid) 
	{
		char* tname = GetTopicName(topicid);
		if (!*tname) continue;
		if (len && strnicmp(tname,word,len)) continue;
		topicBlock* block = TI(topicid);

		WORDP D = FindWord(tname);
		Log(STDUSERLOG,(char*)"\r\nTopic: %s  ",D->word);
		int rejoinderOffset = -1;
		if ((int)topicid == inputRejoinderTopic) rejoinderOffset = inputRejoinderRuleID;
		bool used = true;
		bool available = true;
		bool rejoinder = false;
		bool gambit = false;
		bool responder = false;
		bool keys = false;
		bool overlap = false;
		bool all = false;
		if (!*input) all = keys = overlap = gambit = responder = rejoinder = true; // show it all
		ptr = input;
		while (*ptr)
		{
			ptr = ReadCompiledWord(ptr,word); // restriction
			if (!*word) break;
			if (!stricmp(word,(char*)"used")) available = false;
			else if (!stricmp(word,(char*)"available")) used = false;

			else if (!stricmp(word,(char*)"rejoinder")) rejoinder = true;
			else if (!stricmp(word,(char*)"gambit")) gambit = true;
			else if (!stricmp(word,(char*)"responder")) responder = true;
			else if (!stricmp(word,(char*)"all")) rejoinder = gambit = responder = true;

			else if (!stricmp(word,(char*)"keys")) keys = true;
			else if (!stricmp(word,(char*)"overlap")) overlap = true;
		}
		if (!gambit && !responder && !rejoinder) used = available = false;
		if (all) 
		{
			Log(STDUSERLOG,(char*)"%s",DisplayTopicFlags(topicid));
			Log(STDUSERLOG,(char*)"Bot: %s\r\n",block->topicRestriction ? block->topicRestriction : (char*)"all");
			if (block->topicLastGambitted == 0 && block->topicLastRespondered == 0 && block->topicLastRejoindered == 0) Log(STDUSERLOG,(char*)"  Seen: never visited");
			else Log(STDUSERLOG,(char*)"  Seen: last gambit %d   last rejoinder %d  lastresponder\r\n", block->topicLastGambitted,block->topicLastRespondered,block->topicLastRejoindered);
		}

		if (keys) // display all keys (execpt recursive wordnet)
		{
			Log(STDUSERLOG,(char*)"\r\n  Keys:\r\n");
			NextInferMark();
			MarkExclude(D);
			FACT* F = GetObjectNondeadHead(D);
			size_t length = 2;
			Log(STDUSERLOG,(char*)"    ");
			while (F)
			{
				shownItem = false;
				if (ValidMemberFact(F)) TrackFactsDown(F->subject,F,1,length,true);
				F = GetObjectNondeadNext(F);
			}
		}
		shownItem = false;
		if (overlap)
		{
			if (GetObjectNondeadHead(D)) Log(STDUSERLOG,(char*)"\r\n");
			FACT* F = GetObjectNondeadHead(D);
			NextInferMark();
			D->inferMark = inferMark;
			MarkExclude(D);
			size_t length = 2;
			bool started = false;
			while (F)
			{
				if (ValidMemberFact(F))
				{
					if (!started)
					{
						Log(STDUSERLOG,(char*)"\r\nKey Overlap: %s\r\n",D->word);
						started = true;
					}
					if (shownItem) 
					{
						Log(STDUSERLOG,(char*)"\r\n");
						shownItem = false;
					}
					TrackFactsDown(F->subject,F,1,length,false); 
				}
				F = GetObjectNondeadNext(F);
			}
		}

		if ((used || available) && !gambit && !rejoinder && !responder) rejoinder = gambit = responder = true;

		unsigned int gambits = 0;
		unsigned int statements = 0;
		unsigned int questions = 0;
		unsigned int dual = 0;
		unsigned int rejoinders = 0;

		int id = 0;
		char* name = GetTopicName(topicid);
		char* data = GetTopicData(topicid);
		bool access = true;
		Log(STDUSERLOG,(char*)"\r\n  Rules:\r\n");
		while (data && *data) // walk data
		{
			char* rule = ShowRule(data);
			if (*data == GAMBIT || *data == RANDOM_GAMBIT) ++gambits;
			else if (*data == QUESTION) ++questions;
			else if (*data == STATEMENT) ++statements;
			else if (*data == STATEMENT_QUESTION) ++dual;
			else  ++rejoinders;
			if (TopLevelRule(data))
			{
				access = UsableRule(topicid,id);
				if ((*data == GAMBIT || *data == RANDOM_GAMBIT) && !gambit) access = false;
				else if ((*data == QUESTION || *data == STATEMENT_QUESTION || *data == STATEMENT) && !responder) access = false;
				else if (!access) // no access exists
				{
					if (used) 
					{
						Log(STDUSERLOG,(char*)"  - %d(%d) %s\r\n",id,block->ruleOffset[id],rule);
						access = true;
					}
				}
				else // rule is accessible
				{
					if (available) Log(STDUSERLOG,(char*)"    %d(%d) %s\r\n",id,block->ruleOffset[id],rule);
					else access = false;
				}
			}
			else if (rejoinder) // inherits prior access
			{
				if (access)
				{
					unsigned int depth = *rule - 'a';
					while (depth--) Log(STDUSERLOG,(char*)"    "); // indent appropriately
					if (id == rejoinderOffset) Log(STDUSERLOG,(char*)"  ***  (%d) %s\r\n",REJOINDERID(id),rule); // current rejoinder
					else Log(STDUSERLOG,(char*)"       (%d) %s\r\n",REJOINDERID(id),rule);
				}
			}
			data = FindNextRule(NEXTRULE,data,id);
		}
		if (all) Log(STDUSERLOG,(char*)"  gambits: %d  responders: %d (?:%d s:%d u:%d)  rejoinders: %d\r\n", gambits,statements+questions+dual,statements, questions, dual,rejoinders);
	}
}

static void LoadDescriptions (char* file)
{
	FILE* in = FopenReadWritten(file);
	if (!in) return;
	char name[MAX_WORD_SIZE];
	char describe[MAX_WORD_SIZE];
	WORDP lock = dictionaryLocked;
	dictionaryLocked = 0; 

	while (ReadALine(readBuffer,in) >= 0 ) 
	{
		char *ptr = ReadCompiledWord(readBuffer,name);
		if (!*name) continue;
		ReadCompiledWord(ptr,describe);
		WORDP D = StoreWord(name);
		WORDP E = StoreWord(describe);
		AddInternalFlag(D,DEFINES);
		D->inferMark = MakeMeaning(E); 
	}
	dictionaryLocked = lock;
	FClose(in);
}

static void FreeDescriptions(WORDP D, uint64 junk)
{
	if (D->internalBits & DEFINES)
	{
		if (*D->word == USERVAR_PREFIX  || *D->word == '~' ) 
		{
			D->internalBits ^= DEFINES;
			D->inferMark = 0;
		}
	}
}

static void ListMacro(WORDP D, uint64 junk)
{
	if (*D->word == '^' ) 
	{
		unsigned int count = FACTSET_COUNT(3);
		factSet[3][++count] = CreateFact(MakeMeaning(D),MakeMeaning(StoreWord((char*)":list")),MakeMeaning(StoreWord((char*)":list")),FACTTRANSIENT|FACTDUPLICATE);
		if (count >= MAX_FIND) --count;
		SET_FACTSET_COUNT(3,count);  
	}
}

static void ListTopic(WORDP D, uint64 junk)
{
	if (*D->word == '~' && D->internalBits & TOPIC ) 
	{
		unsigned int count = FACTSET_COUNT(4);
		factSet[4][++count] = CreateFact(MakeMeaning(D),MakeMeaning(StoreWord((char*)":list")),MakeMeaning(StoreWord((char*)":list")),FACTTRANSIENT|FACTDUPLICATE);
		if (count >= MAX_FIND) --count;
		SET_FACTSET_COUNT(4,count);  
	}
}

static void C_List(char* input)
{
	bool all = false;
	bool sorted = true;
	char item[MAX_WORD_SIZE];
	char* ptr = ReadCompiledWord(input,item);
	if (!stricmp(item,(char*)"unsorted")) 
	{
		sorted = false;
		input = ptr;
	}
	else ptr = input;
	ptr = SkipWhitespace(ptr);	
	if (!*ptr) all = true;
	char word[MAX_WORD_SIZE];
	unsigned int count = 0;
	MEANING verb = MakeMeaning(StoreWord((char*)":list"));
	if (all || strchr(input,USERVAR_PREFIX)) // do permanent user variables
	{
		NextInferMark();
		for (int topicid = 1; topicid <= numberOfTopics; ++topicid) 
		{
			if (!*GetTopicName(topicid)) continue;
			int id = 0;
			char* data = GetTopicData(topicid);
			while (data && *data) // walk data
			{
				data = strstr(data,(char*)"$");
				if (!data) continue;
				data = ReadCompiledWord(data,word);
				if (!word[1] || (word[1] == LOCALVAR_PREFIX ||  word[1] == TRANSIENTVAR_PREFIX || IsDigit(word[1]))) continue; // ignore temp vars, $, and money
				char* at = word;
				while (*++at && (IsAlphaUTF8(*at) || IsDigit(*at) || *at == '-' || *at == '_'));
				*at = 0;
				WORDP D = StoreWord(word);
				if (D->inferMark == inferMark) continue;
				D->inferMark = inferMark;
				factSet[0][++count] = CreateFact(MakeMeaning(D),verb,verb,FACTTRANSIENT|FACTDUPLICATE);
				if (count >= MAX_FIND) --count;
			}
		}
		SET_FACTSET_COUNT(0,count);  
		if (sorted) SortFacts((char*)"@0subject",true);
	}
	if (all || strchr(input,'@'))
	{
		count = 0;
		for (unsigned int i = 0; i <= MAX_FIND_SETS; ++i)
		{
			char word[MAX_WORD_SIZE];
			sprintf(word,(char*)"@%d",i);
			factSet[1][++count] = CreateFact(MakeMeaning(StoreWord(word)),verb,verb,FACTTRANSIENT|FACTDUPLICATE);
		}
		SET_FACTSET_COUNT(1,count);  
	}

	if (all || strchr(input,'_')) // match variables
	{
		count = 0;
		for (unsigned int i = 0; i <= MAX_WILDCARDS; ++i)
		{
			char word[MAX_WORD_SIZE];
			sprintf(word,(char*)"_%d",i);
			factSet[2][++count] = CreateFact(MakeMeaning(StoreWord(word)),verb,verb,FACTTRANSIENT|FACTDUPLICATE);
		}
		SET_FACTSET_COUNT(2,count);  
	}
	
	if (all || strchr(input,'^')) 
	{
		SET_FACTSET_COUNT(3,0);  
		WalkDictionary(ListMacro,0);
		if (sorted) SortFacts((char*)"@3subject",true);

	}
	if (all || strchr(input,'~')) 
	{
		SET_FACTSET_COUNT(4,0);  
		WalkDictionary(ListTopic,0);
		if (sorted) SortFacts((char*)"@4subject",true);
	}
	char file[200];
	sprintf(file,"%s/BUILD0/describe0.txt",topic);
	LoadDescriptions(file);
	sprintf(file,"%s/BUILD1/describe1.txt",topic);
	LoadDescriptions(file);
	if (all || strchr(input,USERVAR_PREFIX))
	{
		count = FACTSET_COUNT(0);
		Log(STDUSERLOG,(char*)"User Variables:\r\n");
		for (unsigned int i = 1; i <= count; ++i)
		{
			WORDP D = Meaning2Word(factSet[0][i]->subject);
			if (D->internalBits & DEFINES) 
			{
				Log(STDUSERLOG,(char*)"    %s %s\r\n",D->word, Meaning2Word(D->inferMark)->word);
				D->internalBits ^= DEFINES;
			}
			else Log(STDUSERLOG,(char*)"    %s\r\n",D->word);
		}
	}

	if (all || strchr(input,'@'))
	{
		count = FACTSET_COUNT(1);
		Log(STDUSERLOG,(char*)"Fact Sets:\r\n");
		for (unsigned int i = 1; i <= count; ++i)
		{
			WORDP D = Meaning2Word(factSet[1][i]->subject);
			if (D->internalBits & DEFINES)
			{
				Log(STDUSERLOG,(char*)"    %s ",D->word);
				if ((setControl & (uint64) (1ull << i))) Log(STDUSERLOG,(char*)" SAVED ");
				Log(STDUSERLOG,(char*)" %s\r\n",Meaning2Word(D->inferMark)->word);
				D->internalBits ^= DEFINES;
				D->inferMark = 0;
			}
		}
	}

	if (all || strchr(input,'_'))
	{
		count = FACTSET_COUNT(2);
		Log(STDUSERLOG,(char*)"Match Variables:\r\n");
		for (unsigned int i = 1; i <= count; ++i)
		{
			WORDP D = Meaning2Word(factSet[2][i]->subject);
			if (D->internalBits & DEFINES)
			{
				D->internalBits ^= DEFINES;
				Log(STDUSERLOG,(char*)"    %s %s\r\n",D->word, Meaning2Word(D->inferMark)->word);
				D->inferMark = 0;
			}
		}
	}

	if (all || strchr(input,'^')) 
	{
		count = FACTSET_COUNT(3);
		Log(STDUSERLOG,(char*)"User Macros:\r\n");
		for (unsigned int i = 1; i <= count; ++i)
		{
			WORDP D = Meaning2Word(factSet[3][i]->subject);
			if (D->internalBits & DEFINES) 
			{
				Log(STDUSERLOG,(char*)"    %s %s\r\n",D->word, Meaning2Word(D->inferMark)->word);
				D->internalBits ^= DEFINES;
				D->inferMark = 0;
			}
		}
	}

	if (all || strchr(input,'~')) 
	{
		count = FACTSET_COUNT(4);
		Log(STDUSERLOG,(char*)"Topics:\r\n");
		for (unsigned int i = 1; i <= count; ++i)
		{
			WORDP D = Meaning2Word(factSet[4][i]->subject);
			if (D->internalBits & DEFINES) 
			{
				Log(STDUSERLOG,(char*)"    %s %s\r\n",D->word, Meaning2Word(D->inferMark)->word);
				D->internalBits ^= DEFINES;
				D->inferMark = 0;
			}
		}
	}

	WalkDictionary(FreeDescriptions,0);
}

static void C_Where(char* input)
{
	int topic = FindTopicIDByName(input);
	if (topic)	Log(STDUSERLOG,(char*)"%s is from %s\r\n",input,GetTopicFile(topic));
}

//////////////////////////////////////////////////////////
//// FACT INFO
//////////////////////////////////////////////////////////

static void C_AllFacts(char* input)
{
	char fname[200];
	sprintf(fname, "%s/facts.txt", tmp);
	WriteFacts(FopenUTF8Write(fname),factBase);
}

static void C_Facts(char* input)
{
	char word[MAX_WORD_SIZE];
	char* ptr = ReadCompiledWord(input,word);
	FACT* G = NULL;
	WORDP D = NULL;
	unsigned int index = 0;
	FACT* F;
	if (*word == '(') // actual fact
	{
		char arg1[MAX_WORD_SIZE];
		char arg2[MAX_WORD_SIZE];
		char arg3[MAX_WORD_SIZE];
		ptr -= (strlen(word)-1) + 1;
		ptr = ReadCompiledWord(ptr,arg1);
		ptr = ReadCompiledWord(ptr,arg2);
		ptr = ReadCompiledWord(ptr,arg3);
		size_t len = strlen(arg3);
		if (arg3[len-1] == ')') arg3[len-1] = 0;	// remove trailing )
		G = FindFact(ReadMeaning(arg1,false),ReadMeaning(arg2,false),ReadMeaning(arg3,false),0); 
		if (!G) 
		{
			Log(STDUSERLOG,(char*)"No such facts\r\n");
			return;
		}
	}
	else if (*word == '@') // in a fact set
	{
		int set = GetSetID(word);
		if (set == ILLEGAL_FACTSET)
		{
			Log(STDUSERLOG,(char*)"Illegal fact set %s\r\n",word);
			return;
		}
		Log(STDUSERLOG,(char*)"Fact set %s: %d facts\r\n",word,FACTSET_COUNT(set));
		for (unsigned int i = 1; i <= FACTSET_COUNT(set); ++i)
		{
			TraceFact(factSet[set][i]);
		}
		return;
	}
	else
	{
		MEANING M = ReadMeaning(word,false);
		index = Meaning2Index(M);
		if (!M)
		{
			Log(STDUSERLOG,(char*)"No such meaning exists\r\n");
			return;
		}
		D = Meaning2Word(M);

	}
	F = (G) ? GetSubjectNondeadHead(G) :  GetSubjectNondeadHead(D);
	while (F)
	{
		if (index && F->subject != index) {;}
		else TraceFact(F);
		F = GetSubjectNondeadNext(F);
	}	
	F = (G) ? GetVerbNondeadHead(G) :  GetVerbNondeadHead(D);
	while (F)
	{
		if (index && F->verb != index)  {;}
		else TraceFact(F);
		F = GetVerbNondeadNext(F);
	}
	F = (G) ? GetObjectNondeadHead(G) :  GetObjectNondeadHead(D);
	while (F)
	{
		if (index && F->object != index)  {;}
		else TraceFact(F);
		F = GetObjectNondeadNext(F);
	}
}

static char* WriteFactFlags(FACT* F)
{
	char* buffer = AllocateBuffer();
	if (F->flags & FACTATTRIBUTE) strcat(buffer,"FACTATTRIBUTE ");
	if (F->flags & MARKED_FACT) strcat(buffer,"MARKED_FACT ");
	if (F->flags & ITERATOR_FACT) strcat(buffer,"ITERATOR_FACT ");
	if (F->flags & MARKED_FACT2) strcat(buffer,"MARKED_FACT2 ");
	if (F->flags & FACTDEAD) strcat(buffer,"FACTDEAD ");
	if (F->flags & FACTTRANSIENT) strcat(buffer,"FACTTRANSIENT ");
	if (F->flags & FACTAUTODELETE) strcat(buffer,"AUTODELETE ");
	if (F->flags & ORIGINAL_ONLY) strcat(buffer,"ORIGINAL_ONLY ");
	if (F->flags & FACTBUILD2) strcat(buffer,"FACTBUILD2 ");
	if (F->flags & FACTBUILD1) strcat(buffer,"FACTBUILD1 ");
	if (F->flags & USER_FLAG3) strcat(buffer,"USER_FLAG3 ");
	if (F->flags & USER_FLAG2) strcat(buffer,"USER_FLAG2 ");
	if (F->flags & USER_FLAG1) strcat(buffer,"USER_FLAG1 ");
    if (F->flags & OVERRIDE_MEMBER_FACT) strcat(buffer, "OVERRIDE_MEMBER_FACT ");
	// unused 0x00004000 0x00008000
	if (F->flags & JSON_OBJECT_FACT) strcat(buffer,"JSON_OBJECT_FACT ");
	if (F->flags & JSON_ARRAY_FACT) strcat(buffer,"JSON_ARRAY_FACT ");
	if (F->flags & JSON_ARRAY_VALUE) strcat(buffer,"JSON_ARRAY_VALUE ");
	if (F->flags & JSON_OBJECT_VALUE) strcat(buffer,"JSON_OBJECT_VALUE ");
	if (F->flags & JSON_STRING_VALUE) strcat(buffer,"JSON_STRING_VALUE ");
	if (F->flags & JSON_PRIMITIVE_VALUE) strcat(buffer,"JSON_PRIMITIVE_VALUE ");
	if (F->flags & FACTSUBJECT) strcat(buffer,"FACTSUBJECT ");
	if (F->flags & FACTVERB) strcat(buffer,"FACTVERB ");
	if (F->flags & FACTOBJECT) strcat(buffer,"FACTOBJECT ");
	if (F->flags & FACTDUPLICATE) strcat(buffer,"FACTDUPLICATE ");
	FreeBuffer();
	return buffer;
}
	
static void C_UserFacts(char* input)
{
	if (!factLocked) return; // no user facts yet
	char* buffer = AllocateBuffer();
	for (unsigned int i = 0; i <= MAX_FIND_SETS; ++i) 
    {
		if (!(setControl &  (1ULL << i))) continue; // purely transient stuff
		unsigned int count = FACTSET_COUNT(i);
		if (!count) continue;
		// save this set
		Log(STDUSERLOG,(char*)"Set %d[%d]\r\n",i,count); 
        for (unsigned int j = 1; j <= count; ++j)
		{
			char* fact = WriteFact(factSet[i][j],false,buffer,false,false);
			Log(STDUSERLOG, "%s  # %d %s\r\n",fact, Fact2Index(factSet[i][j]),WriteFactFlags(factSet[i][j]));
		}
    }
	FreeBuffer();
	FACT* F = factLocked;
	unsigned int count = 0;
	while (++F <= factFree)
	{
		char* word = AllocateBuffer();
		++count;
		char* fact = WriteFact(F,false,word,false,false);
		Log(STDUSERLOG,(char*)"%s  # %d %s\r\n",fact,Fact2Index(F), WriteFactFlags(F));
		FreeBuffer();
	}
	Log(STDUSERLOG,(char*)"user facts: %d\r\n",count);
}

//////////////////////////////////////////////////////////
//// DEBUGGING COMMANDS
//////////////////////////////////////////////////////////

static void C_DoInternal(char* input,bool internal)
{
	SAVEOLDCONTEXT()
	if (!internal)
	{
		++volleyCount;
		responseIndex = 0;	// clear out data (having left time for :why to work)
		AddHumanUsed((char*)":do");
		AddRepeatable(0);
		OnceCode((char*)"$cs_control_pre");
	}
	currentRule = 0;
	currentRuleID = 0;
	currentRuleTopic =  currentTopicID = 0;
	char* data = AllocateBuffer();
	char* out = data;
	char* answer = AllocateBuffer();
	FunctionResult result = NOPROBLEM_BIT;
#ifndef DISCARDSCRIPTCOMPILER
	hasErrors = 0;
	bool inited = StartScriptCompiler();
	ReadOutput(false,false,input, NULL,out,NULL,NULL,NULL);
	if (inited) EndScriptCompiler();
	if (hasErrors) Log(STDUSERLOG,(char*)"\r\nScript errors prevent execution.");
	else 
	{
		FreshOutput(data,answer,result);
		if (trace & TRACE_OUTPUT) Log(STDUSERLOG,(char*)"   result: %s  output: %s\r\n",ResultCode(result),answer);
		if (!internal) AddResponse(answer,responseControl);
		else (*printer)("    %s\r\n",answer);
	}
#else
	Log(STDUSERLOG,(char*)"Script compiler not installed.");
#endif
	FreeBuffer();
	FreeBuffer();
	RESTOREOLDCONTEXT()
	if (internal){;}
	else if (result == RESTART_BIT) wasCommand = RESTART;
	else wasCommand = OUTPUTASGIVEN; // save results to user file
}

static void C_Do(char* input)
{
	C_DoInternal(input,false);
}

static void C_Silent(char* input)
{
	silent = !silent;
}

static void C_Retry(char* input)
{
	if (server && !serverRetryOK) return;

	char file[MAX_WORD_SIZE];
	char name[MAX_WORD_SIZE];
	bool traceit = false;
	if (!strnicmp(input, "trace", 5))
	{
		traceit = true;
		input += 6;
	}
	char word[MAX_WORD_SIZE];
	ResetToPreUser();
    ResetFunctionSystem();
    ResetSentence();
	char which[20];
	*which = 0;
	*name = 0;
	if (!strnicmp(SkipWhitespace(mainInputBuffer), (char*)":redo", 5) && redo)
	{
		char* at = ReadCompiledWord(input, word); // input is after turn
		if (IsDigit(*word)) // retry depth
		{
			input = at;
			unsigned int n = atoi(word);
			sprintf(which, (char*)"%d", n);
			if (!*at)
			{
				Log(STDUSERLOG, (char*)"You must supply input to go back. changing to :retry \r\n");
				*which = 0; // ordinary retry
			}
		}
		else if (!stricmp(word, "FILE"))
		{
			input = ReadCompiledWord(at, name);
			if (shared) return;	// shared subsitution not supported yet
		}
	}
	input = SkipWhitespace(input);
	if (!*input) strcpy(mainInputBuffer, revertBuffer);
	else strcpy(mainInputBuffer, input);
	if (!server) (*printer)((char*)"Retrying with: %s\r\n", mainInputBuffer);

	// get main user file name
	sprintf(file, (char*)"%s/topic_%s_%s.txt", users, loginID, computerID);
	if (stricmp(language, "english")) sprintf(file, (char*)"%s/topic_%s_%s_%s.txt", users, loginID, computerID, language);

	if (!*name) // if not overridden, use default backup data
	{
		sprintf(name, (char*)"%s/backup%s-%s_%s.txt", tmp, which, loginID, computerID);
		if (stricmp(language, "english")) sprintf(name, (char*)"%s/backup%s-%s_%s_%s.txt", tmp, which, loginID, computerID, language);
	}
	CopyFile2File(file, name, false);
	char* buffer = FindUserCache(file); //  (does not trigger a read, assumes it has it in core)
	if (buffer) FreeUserCache(); // erase cache of user so it reads revised disk file

	// get shared file
	if (shared)
	{
		sprintf(file, (char*)"%s/topic_%s_%s.txt", users, loginID, (char*)"share");
		if (stricmp(language, "english")) sprintf(file, (char*)"%s/topic_%s_%s_%s.txt", users, loginID, language, (char*)"share");

		sprintf(name, (char*)"%s/backup%s-share-%s_%s.txt", tmp, which, loginID, computerID);
		if (stricmp(language, "english")) sprintf(name, (char*)"%s/backup%s-share-%s_%s_%s.txt", tmp, which, loginID, computerID, language);
		CopyFile2File(file, name, false);
		buffer = FindUserCache(file); //  (does not trigger a read, assumes it has it in core)
		if (buffer) FreeUserCache(); // erase cache of user so it reads revised disk file
	}

	// load user from refreshed files
	char oldc;
	int oldCurrentLine;
	int BOMvalue = -1; // get prior value
	BOMAccess(BOMvalue, oldc, oldCurrentLine); // copy out prior file access and reinit user file access
	ReadUserData();
	BOMAccess(BOMvalue, oldc, oldCurrentLine);
	if (traceit) 
	{
		trace = -1; 
		echo = true;
	}
}

static void C_Redo(char* input)
{
	C_Retry(input);
}

static void C_Log(char* input)
{
	Log(STDUSERLOG,(char*)"Log: %s\r\n",input);
}

static void C_Skip(char* buffer)
{
	int topic = GetPendingTopicUnchanged();
	if (!topic) 
	{
		Log(STDUSERLOG,(char*)"No pending topic\r\n");
		return;
	}
	topicBlock* block = TI(topic);
	unsigned int* offsets = block->ruleOffset;
	int n = atoi(SkipWhitespace(buffer));
	unsigned int* map = block->gambitTag;
	unsigned int ruleID = *map;
	char * rule = NULL;
	char* data = GetTopicData(topic);  
	while (ruleID != NOMORERULES)
	{
		rule = data + offsets[ruleID];
		if (TopLevelGambit(rule) && UsableRule(topic,ruleID) && --n == 0) SetRuleDisableMark(topic, ruleID);
		ruleID = *++map;
	}
	if (ruleID != NOMORERULES) Log(STDUSERLOG,(char*)"Next gambit of %s is: %s...\r\n",GetTopicName(topic),ShowRule(GetRule(topic,ruleID)));
	WriteUserData(0,false);
}

static void C_Show(char* input)
{
	char word[MAX_WORD_SIZE];
	input = ReadCompiledWord(input,word);
	char value[MAX_WORD_SIZE];
	*value = 0;
	bool set = atoi(value) ? true : false;
	if (*input) ReadCompiledWord(input,value);
	if (!stricmp(word,(char*)"all"))
	{
		if (*value) all = set;
		else all = !all;
		Log(STDUSERLOG,(char*)"All set to %d\n",all);
	}
	else if (!stricmp(word,(char*)"oob"))
	{
		if (*value) oob = set;
		else oob = !oob;
		Log(STDUSERLOG,(char*)" oob set to %d\r\n",oob);
	}
	else if (!stricmp(word,(char*)"newline"))
	{
		if (*value) newline = set;
		else newline = !newline;
		Log(STDUSERLOG,(char*)" newline set to %d\n",newline);
	}
	else if (!stricmp(word,(char*)"depth"))
	{
		if (*value) showDepth = set;
		else showDepth = !showDepth;
		Log(STDUSERLOG,(char*)" showDepth set to %d\n",showDepth);
	}
	else if (!stricmp(word,(char*)"echo"))
	{
		if (*value) echo = set;
		else echo = !echo;
		Log(STDUSERLOG,(char*)" echo set to %d\n",echo);
	}
	else if (!stricmp(word,(char*)"echoserver"))
	{
		if (*value) echoServer = set;
		else echoServer = !echoServer;
		Log(STDUSERLOG,(char*)" echoServer set to %d\n",echoServer);
	}
	else if (!stricmp(word,(char*)"input"))
	{
		showInput = !showInput;
		Log(STDUSERLOG,(char*)" input set to %d\n",showInput);
	}
	else if (!stricmp(word,(char*)"reject"))
	{
		showReject = !showReject;
		Log(STDUSERLOG,(char*)" reject set to %d\n",showReject);
	}
	else if (!stricmp(word,(char*)"memory"))
	{
		showmem = !showmem;
		Log(STDUSERLOG,(char*)" showmem set to %d\n",showmem);
	}	
	else if (!stricmp(word,(char*)"mark"))
	{
		showMark = !showMark;
		Log(STDUSERLOG,(char*)" showMark set to %d\n",showMark);
	}
	else if (!stricmp(word,(char*)"number"))
	{
		if (*value) autonumber = set;
		else autonumber = !autonumber;
		Log(STDUSERLOG,(char*)" autonumber set to %d\n",autonumber);
	}
	else if (!stricmp(word,(char*)"pos"))
	{
		if (*value) shortPos = set;
		else shortPos = !shortPos;
		Log(STDUSERLOG,(char*)" Pos set to %d\n",shortPos);
	}
	else if (!stricmp(word,(char*)"serverLog"))
	{
		if (*value) serverLog = set;
		else serverLog = !serverLog;
		Log(STDUSERLOG,(char*)" serverLog set to %d\n",serverLog);
	}
	else if (!stricmp(word,(char*)"stats"))
	{
		ruleCount = 0;
		if (*value) stats = set;
		else stats = !stats;
		Log(STDUSERLOG,(char*)" stats set to %d\n",stats);
	}
	else if (!stricmp(word,(char*)"topic"))
	{
		if (*value) showTopic = set;
		else showTopic = !showTopic;
		Log(STDUSERLOG,(char*)" topic set to %d\n",showTopic);
	}
	else if (!stricmp(word,(char*)"topics"))
	{
		if (*value) showTopics = set;
		else showTopics = !showTopics;
		Log(STDUSERLOG,(char*)" topics set to %d\n",showTopics);
	}
	else if (!stricmp(word,(char*)"why"))
	{
		if (*value) showWhy = set;
		else showWhy = !showWhy;
		Log(STDUSERLOG,(char*)" why set to %d\n",showWhy);
	}
} 

static void TraceTopicFunction(WORDP D, uint64 data)
{
	if (D->internalBits & TOPIC && D->word[0] ==  '~') TracedTopic(D, data);
	else if (D->word[0] == '^' && (D->internalBits & FN_TRACE_BITS)) TracedFunction(D, data);
}

static void TimingTopicFunction(WORDP D, uint64 data)
{
	if (D->internalBits & TOPIC && D->word[0] == '~') TimedTopic(D, data);
	else if (D->word[0] == '^' && (D->internalBits & FN_TIME_BITS)) TimedFunction(D, data);
}

static void ShowTrace(unsigned int bits, bool original)
{
	unsigned int general = (TRACE_VARIABLE|TRACE_MATCH|TRACE_FLOW|TRACE_ECHO);
	unsigned int mild = (TRACE_OUTPUT| TRACE_INPUT | TRACE_PREPARE|TRACE_PATTERN);
	unsigned int deep = (TRACE_ALWAYS|TRACE_JSON|TRACE_TOPIC|TRACE_FACT|TRACE_SAMPLE|TRACE_INFER|TRACE_HIERARCHY|TRACE_SUBSTITUTE|TRACE_VARIABLESET|TRACE_QUERY|TRACE_USER|TRACE_USERFACT| TRACE_TREETAGGER | TRACE_POS| TRACE_TCP|TRACE_USERFN|TRACE_USERCACHE|TRACE_SQL|TRACE_LABEL);

	// general
	if (bits & general) 
	{
		if (!original) Log(ECHOSTDUSERLOG, (char*)"  ");
		Log(ECHOSTDUSERLOG,(char*)"Enabled simple: ");
		if (bits & TRACE_ECHO) Log(ECHOSTDUSERLOG,(char*)"echo ");
		if (bits & TRACE_MATCH) Log(ECHOSTDUSERLOG,(char*)"match ");
		if (bits & TRACE_FLOW) Log(ECHOSTDUSERLOG,(char*)"ruleflow ");
		if (bits & TRACE_VARIABLE) Log(ECHOSTDUSERLOG,(char*)"variables ");
		Log(ECHOSTDUSERLOG,(char*)"\r\n");
	}

	// mild detail
	if (bits & mild) 
	{
		if (!original) Log(ECHOSTDUSERLOG, (char*)"  ");
		Log(ECHOSTDUSERLOG,(char*)"Enabled mild detail: ");
		if (bits & TRACE_OUTPUT) Log(ECHOSTDUSERLOG,(char*)"output ");
		if (bits & TRACE_PATTERN) Log(ECHOSTDUSERLOG,(char*)"pattern ");
		if (bits & TRACE_INPUT) Log(ECHOSTDUSERLOG, (char*)"input ");
		if (bits & TRACE_PREPARE) Log(ECHOSTDUSERLOG,(char*)"prepare ");
		Log(ECHOSTDUSERLOG,(char*)"\r\n");
	}
	// deep detail
	if (bits & deep) 
	{
		if (!original) Log(ECHOSTDUSERLOG, (char*)"  ");
		Log(ECHOSTDUSERLOG,(char*)"Enabled deep detail: ");
		if (bits & TRACE_ALWAYS) Log(ECHOSTDUSERLOG,(char*)"always ");
		if (bits & TRACE_FACT) Log(ECHOSTDUSERLOG,(char*)"fact ");
		if (TraceHierarchyTest(bits)) Log(ECHOSTDUSERLOG,(char*)"hierarchy ");
		if (bits & TRACE_INFER) Log(ECHOSTDUSERLOG,(char*)"infer ");
		if (bits & TRACE_JSON) Log(ECHOSTDUSERLOG,(char*)"json ");
		if (bits & TRACE_LABEL) Log(ECHOSTDUSERLOG,(char*)"label ");
		if (bits & TRACE_USERFN) Log(ECHOSTDUSERLOG,(char*)"macro ");
		if (bits & TRACE_POS) Log(ECHOSTDUSERLOG,(char*)"pos ");
		if (bits & TRACE_QUERY) Log(ECHOSTDUSERLOG,(char*)"query ");
		if (bits & TRACE_SAMPLE) Log(ECHOSTDUSERLOG,(char*)"sample ");
		if (bits & TRACE_SQL) Log(ECHOSTDUSERLOG,(char*)"sql ");
		if (bits & TRACE_SUBSTITUTE) Log(ECHOSTDUSERLOG,(char*)"substitute ");
		if (bits & TRACE_TCP) Log(ECHOSTDUSERLOG,(char*)"tcp ");
		if (bits & TRACE_TOPIC) Log(ECHOSTDUSERLOG,(char*)"topic ");
		if (bits & TRACE_USER) Log(ECHOSTDUSERLOG,(char*)"user ");
		if (bits & TRACE_USERFACT) Log(ECHOSTDUSERLOG,(char*)"userfact ");
		if (bits & TRACE_TREETAGGER) Log(ECHOSTDUSERLOG, (char*)"treetagger ");
		if (bits & TRACE_USERCACHE) Log(ECHOSTDUSERLOG,(char*)"usercache ");
		if (bits & TRACE_VARIABLESET) Log(ECHOSTDUSERLOG,(char*)"varassign ");
		Log(ECHOSTDUSERLOG,(char*)"\r\n");
	}

	// general
	if ((bits & general) != general) 
	{
		if (!original) Log(ECHOSTDUSERLOG, (char*)"  ");
		Log(ECHOSTDUSERLOG,(char*)"Disabled simple: ");
		if (!(bits & TRACE_ECHO)) Log(ECHOSTDUSERLOG,(char*)"echo ");
		if (!(bits & TRACE_MATCH)) Log(ECHOSTDUSERLOG,(char*)"match ");
		if (!(bits & TRACE_FLOW)) Log(ECHOSTDUSERLOG, (char*)"ruleflow ");
		if (!(bits & TRACE_VARIABLE)) Log(ECHOSTDUSERLOG,(char*)"variables ");
		Log(ECHOSTDUSERLOG,(char*)"\r\n");
	}

	// mild detail
	if ((bits & mild) != mild) 
	{
		if (!original) Log(ECHOSTDUSERLOG, (char*)"  ");
		Log(ECHOSTDUSERLOG,(char*)"Disabled mild detail: ");
		if (!(bits & TRACE_OUTPUT)) Log(ECHOSTDUSERLOG,(char*)"output ");
		if (!(bits & TRACE_PATTERN)) Log(ECHOSTDUSERLOG,(char*)"pattern ");
		if (!(bits & TRACE_INPUT)) Log(ECHOSTDUSERLOG,(char*)"input ");
		if (!(bits & TRACE_PREPARE)) Log(ECHOSTDUSERLOG, (char*)"prepare ");
		Log(ECHOSTDUSERLOG,(char*)"\r\n");
	}

	// deep detail
	if ((bits & deep) != deep)
	{
		if (!original) Log(ECHOSTDUSERLOG, (char*)"  ");
		Log(ECHOSTDUSERLOG,(char*)"Disabled deep detail: ");
		if (!(bits & TRACE_ALWAYS)) Log(ECHOSTDUSERLOG,(char*)"always ");
		if (!(bits & TRACE_FACT)) Log(ECHOSTDUSERLOG,(char*)"fact ");
		if (!TraceHierarchyTest(bits)) Log(ECHOSTDUSERLOG,(char*)"hierarchy ");
		if (!(bits & TRACE_INFER)) Log(ECHOSTDUSERLOG,(char*)"infer ");
		if (!(bits & TRACE_JSON)) Log(ECHOSTDUSERLOG,(char*)"json ");
		if (!(bits & TRACE_LABEL)) Log(ECHOSTDUSERLOG,(char*)"label ");
		if (!(bits & TRACE_USERFN)) Log(ECHOSTDUSERLOG,(char*)"macro ");
		if (!(bits & TRACE_POS)) Log(ECHOSTDUSERLOG,(char*)"pos ");
		if (!(bits & TRACE_QUERY)) Log(ECHOSTDUSERLOG,(char*)"query ");
		if (!(bits & TRACE_SAMPLE)) Log(ECHOSTDUSERLOG,(char*)"sample ");
		if (!(bits & TRACE_SQL)) Log(ECHOSTDUSERLOG,(char*)"sql ");
		if (!(bits & TRACE_SUBSTITUTE)) Log(ECHOSTDUSERLOG,(char*)"substitute ");
		if (!(bits & TRACE_TCP)) Log(ECHOSTDUSERLOG,(char*)"tcp ");
		if (!(bits & TRACE_TOPIC)) Log(ECHOSTDUSERLOG,(char*)"topic ");
		if (!(bits & TRACE_USER)) Log(ECHOSTDUSERLOG,(char*)"user ");
		if (!(bits & TRACE_USERFACT)) Log(ECHOSTDUSERLOG,(char*)"userfact ");
		if (!(bits & TRACE_TREETAGGER)) Log(ECHOSTDUSERLOG, (char*)"treetagger ");
		if (!(bits & TRACE_USERCACHE)) Log(ECHOSTDUSERLOG,(char*)"usercache ");
		if (!(bits & TRACE_VARIABLESET)) Log(ECHOSTDUSERLOG,(char*)"varassign ");
		Log(ECHOSTDUSERLOG,(char*)"\r\n");
	}
	if (original) WalkDictionary(TraceTopicFunction,1);
}

static void ShowTiming(unsigned int bits, bool original)
{
	unsigned int general = TRACE_FLOW;
	unsigned int mild = (TRACE_INPUT|TRACE_PREPARE | TRACE_PATTERN);
	unsigned int deep = (TRACE_ALWAYS | TRACE_JSON | TRACE_TOPIC | TRACE_QUERY | TRACE_USER | TRACE_USERFACT| TRACE_TREETAGGER | TRACE_TCP | TRACE_USERFN | TRACE_USERCACHE | TRACE_SQL);

	// general
	if (bits & general)
	{
		if (!original) Log(ECHOSTDUSERLOG, (char*)"  ");
		Log(ECHOSTDUSERLOG, (char*)"Enabled simple: ");
		if (bits & TRACE_FLOW) Log(ECHOSTDUSERLOG, (char*)"ruleflow ");
		Log(ECHOSTDUSERLOG, (char*)"\r\n");
	}

	// mild detail
	if (bits & mild)
	{
		if (!original) Log(ECHOSTDUSERLOG, (char*)"  ");
		Log(ECHOSTDUSERLOG, (char*)"Enabled mild detail: ");
		if (bits & TRACE_PATTERN) Log(ECHOSTDUSERLOG, (char*)"pattern ");
		if (bits & TRACE_INPUT) Log(ECHOSTDUSERLOG, (char*)"input ");
		if (bits & TRACE_PREPARE) Log(ECHOSTDUSERLOG, (char*)"prepare ");
		Log(ECHOSTDUSERLOG, (char*)"\r\n");
	}
	// deep detail
	if (bits & deep)
	{
		if (!original) Log(ECHOSTDUSERLOG, (char*)"  ");
		Log(ECHOSTDUSERLOG, (char*)"Enabled deep detail: ");
		if (bits & TRACE_ALWAYS) Log(ECHOSTDUSERLOG, (char*)"always ");
		if (bits & TRACE_JSON) Log(ECHOSTDUSERLOG, (char*)"json ");
		if (bits & TRACE_USERFN) Log(ECHOSTDUSERLOG, (char*)"macro ");
		if (bits & TRACE_QUERY) Log(ECHOSTDUSERLOG, (char*)"query ");
		if (bits & TRACE_SQL) Log(ECHOSTDUSERLOG, (char*)"sql ");
		if (bits & TRACE_TCP) Log(ECHOSTDUSERLOG, (char*)"tcp ");
		if (bits & TRACE_TOPIC) Log(ECHOSTDUSERLOG, (char*)"topic ");
		if (bits & TRACE_USER) Log(ECHOSTDUSERLOG, (char*)"user ");
		if (bits & TRACE_USERFACT) Log(ECHOSTDUSERLOG, (char*)"userfact ");
		if (bits & TRACE_TREETAGGER) Log(ECHOSTDUSERLOG, (char*)"treetagger ");
		if (bits & TRACE_USERCACHE) Log(ECHOSTDUSERLOG, (char*)"usercache ");
		Log(ECHOSTDUSERLOG, (char*)"\r\n");
	}

	// general
	if ((bits & general) != general)
	{
		if (!original) Log(ECHOSTDUSERLOG, (char*)"  ");
		Log(ECHOSTDUSERLOG, (char*)"Disabled simple: ");
		if (!(bits & TRACE_FLOW)) Log(ECHOSTDUSERLOG, (char*)"ruleflow ");
		Log(ECHOSTDUSERLOG, (char*)"\r\n");
	}

	// mild detail
	if ((bits & mild) != mild)
	{
		if (!original) Log(ECHOSTDUSERLOG, (char*)"  ");
		Log(ECHOSTDUSERLOG, (char*)"Disabled mild detail: ");
		if (!(bits & TRACE_PATTERN)) Log(ECHOSTDUSERLOG, (char*)"pattern ");
		if (!(bits & TRACE_INPUT)) Log(ECHOSTDUSERLOG, (char*)"input ");
		if (!(bits & TRACE_PREPARE)) Log(ECHOSTDUSERLOG, (char*)"prepare ");
		Log(ECHOSTDUSERLOG, (char*)"\r\n");
	}

	// deep detail
	if ((bits & deep) != deep)
	{
		if (!original) Log(ECHOSTDUSERLOG, (char*)"  ");
		Log(ECHOSTDUSERLOG, (char*)"Disabled deep detail: ");
		if (!(bits & TRACE_ALWAYS)) Log(ECHOSTDUSERLOG, (char*)"always ");
		if (!(bits & TRACE_JSON)) Log(ECHOSTDUSERLOG, (char*)"json ");
		if (!(bits & TRACE_USERFN)) Log(ECHOSTDUSERLOG, (char*)"macro ");
		if (!(bits & TRACE_QUERY)) Log(ECHOSTDUSERLOG, (char*)"query ");
		if (!(bits & TRACE_SQL)) Log(ECHOSTDUSERLOG, (char*)"sql ");
		if (!(bits & TRACE_TCP)) Log(ECHOSTDUSERLOG, (char*)"tcp ");
		if (!(bits & TRACE_TOPIC)) Log(ECHOSTDUSERLOG, (char*)"topic ");
		if (!(bits & TRACE_USER)) Log(ECHOSTDUSERLOG, (char*)"user ");
		if (!(bits & TRACE_USERFACT)) Log(ECHOSTDUSERLOG, (char*)"userfact ");
		if (!(bits & TRACE_TREETAGGER)) Log(ECHOSTDUSERLOG, (char*)"treetagger ");
		if (!(bits & TRACE_USERCACHE)) Log(ECHOSTDUSERLOG, (char*)"usercache ");
		Log(ECHOSTDUSERLOG, (char*)"\r\n");
	}
	if (original) WalkDictionary(TimingTopicFunction,1);
}

static void C_Say(char* input)
{
	AddResponse(input,responseControl);
	wasCommand = OUTPUTASGIVEN;
}

static void NoTraceTime(char* input, unsigned int topicflag, unsigned int macroflag, char* cmd)
{
	char word[MAX_WORD_SIZE];
	unsigned int mode = 1;
	while (input) 
	{
		input = ReadCompiledWord(input,word); // if using in a table, use closer "end" if you are using named flags
		if (!*word) break;
		input = SkipWhitespace(input);
		if (*word != '~' && *word != '^')
		{
			if (!stricmp(word, (char*)"on")) mode = 1;
			else if (!stricmp(word,(char*)"off")) mode = 0;
			else Log(STDUSERLOG,(char*)"Bad :%s request %s\r\n",cmd,word);
		}
		else
		{
			unsigned int flag;
			WORDP D = StoreWord(word);

			if (D->word[0] == '~') flag = topicflag;
			else if (D->word[0] == '^') flag = macroflag;
			else continue;

			if (mode == 1) TimeFunction(D,flag, 1);
			else TimeFunction(D,flag, 0);

			if (!fromScript) Log(STDUSERLOG, (char*)" %s %s %s\r\n", cmd, word, (mode == 1) ? (char*)"on" : (char*)"off");
		}
	}
}

static void C_NoTrace(char* input)
{
	NoTraceTime(input, NOTRACE_TOPIC, NOTRACE_FN, (char*)"notrace");
}

static void C_NoTime(char* input)
{
	NoTraceTime(input, NOTIME_TOPIC, NOTIME_FN, (char*)"notime");
}

static void C_Trace(char* input)
{
	char word[MAX_WORD_SIZE];
	isTracing = false;
	int priorTrace = trace;
	unsigned int flags = trace;
	input = SkipWhitespace(input);
	if (!*input) 
	{
		ShowTrace(trace,true);
		return;
	}
	ReadCompiledWord(input,word);
	if (!stricmp(word,(char*)"none")) // turn off all topics and macros as well
	{
		WalkDictionary(ClearTracedFunction,0);
		WalkDictionary(ClearTracedTopic,0);
		traceUniversal = 0;
	}
	
	if (!strnicmp(input,"factcreate",10)) // tracing on 
	{
		input = ReadCompiledWord(input+11,word); 
		strcpy(traceSubject,word);
		if (!stricmp(word,"null")) *traceSubject = 0;
		input = ReadCompiledWord(input,word); 
		strcpy(traceVerb,word);
		if (!stricmp(word,"null")) *traceVerb = 0;
		input = ReadCompiledWord(input,word); 
		strcpy(traceObject,word);
		if (!stricmp(word,"null")) *traceObject = 0;
	}
	bool noecho = false;
	while (input) 
	{
		input = ReadCompiledWord(input,word); // if using trace in a table, use closer "end" if you are using named flags
		if (!*word || *word == '`') break; // ran out of rule or user input
		input = SkipWhitespace(input);
		if (*word == '+') // add this flag is the default
		{
			if (word[1]) memmove(word,word+1,strlen(word));
			else continue;
		}
		
		if (!stricmp(word,(char*)"all") ) flags = ((unsigned int)-1) ^ TRACE_ALWAYS;
		else if (!stricmp(word,(char*)"none")) flags = 0;
		else if (*word == '-') // remove this flag
		{
			if (!word[1]) input = ReadCompiledWord(input,word);
			else memmove(word,word+1,strlen(word));
			if (!stricmp(word,(char*)"notthis")) flags &= -1 ^ TRACE_NOT_THIS_TOPIC;
			if (!stricmp(word,(char*)"match")) flags &= -1 ^ TRACE_MATCH;
			else if (!stricmp(word,(char*)"variables")) flags &= -1 ^ TRACE_VARIABLE; 
			else if (!stricmp(word,(char*)"simple")) flags &= -1 ^ (TRACE_MATCH|TRACE_VARIABLE); 
			else if (!stricmp(word,(char*)"input")) flags &= -1 ^ TRACE_INPUT;
			else if (!stricmp(word,(char*)"ruleflow")) flags &= -1 ^ TRACE_FLOW;
			else if (!stricmp(word,(char*)"echo")) 
			{
				flags &= -1 ^ TRACE_ECHO;
				noecho = true;
			}
			else if (!stricmp(word, (char*)"input")) flags &= -1 ^ TRACE_INPUT;
			else if (!stricmp(word,(char*)"prepare")) flags &= -1 ^ (TRACE_PREPARE|TRACE_INPUT);
			else if (!stricmp(word,(char*)"output")) flags &= -1 ^ TRACE_OUTPUT;
			else if (!stricmp(word,(char*)"pattern")) flags &= -1 ^ TRACE_PATTERN;
			else if (!stricmp(word,(char*)"mild")) flags &= -1 ^ (TRACE_INPUT|TRACE_PREPARE|TRACE_OUTPUT|TRACE_PATTERN); 

			else if (!stricmp(word,(char*)"infer")) flags &= -1 ^ TRACE_INFER;
			else if (!stricmp(word,(char*)"sample")) flags &= -1 ^ TRACE_SAMPLE;
			else if (!stricmp(word,(char*)"substitute")) flags &= -1 ^ TRACE_SUBSTITUTE;
			else if (!stricmp(word,(char*)"hierarchy")) flags &= -1 ^ TRACE_HIERARCHY;
			else if (!stricmp(word,(char*)"fact")) flags &= -1 ^  TRACE_FACT;
			else if (!stricmp(word,(char*)"varassign")) flags &= -1 ^  TRACE_VARIABLESET;
			else if (!stricmp(word,(char*)"query")) flags &= -1 ^  TRACE_QUERY;
			else if (!stricmp(word,(char*)"user")) flags &= -1 ^  TRACE_USER;
			else if (!stricmp(word,(char*)"userfact")) flags &= -1 ^  TRACE_USERFACT;
			else if (!stricmp(word, (char*)"treetagger")) flags &= -1 ^ TRACE_TREETAGGER;
			else if (!stricmp(word,(char*)"pos")) flags &= -1 ^  TRACE_POS;
			else if (!stricmp(word,(char*)"tcp")) flags &= -1 ^  TRACE_TCP;
			else if (!stricmp(word,(char*)"json")) flags &= -1 ^  TRACE_JSON;
			else if (!stricmp(word,(char*)"macro")) flags &= -1 ^  TRACE_USERFN;
			else if (!stricmp(word,(char*)"usercache")) flags &= -1 ^  TRACE_USERCACHE;
			else if (!stricmp(word,(char*)"sql")) flags &= -1 ^  TRACE_SQL;
			else if (!stricmp(word,(char*)"label")) flags &= -1 ^  TRACE_LABEL;
			else if (!stricmp(word,(char*)"topic")) flags &= -1 ^  TRACE_TOPIC;
			else if (!stricmp(word,(char*)"deep")) flags &= -1 ^ (TRACE_JSON|TRACE_TOPIC|TRACE_FLOW|TRACE_INPUT|TRACE_USERFN|TRACE_SAMPLE|TRACE_INFER|TRACE_SUBSTITUTE|TRACE_HIERARCHY| TRACE_FACT| TRACE_VARIABLESET| TRACE_QUERY| TRACE_USER|TRACE_USERFACT|TRACE_TREETAGGER|TRACE_POS|TRACE_TCP|TRACE_USERCACHE|TRACE_SQL|TRACE_LABEL); 
			else if (!stricmp(word,(char*)"always")) flags &= -1 ^  TRACE_ALWAYS;
		}
		else if (IsNumberStarter(*word)  && !IsAlphaUTF8(word[1]))
		{
			ReadInt(word,*(int*)&flags);
			break; // there wont be morez flags -- want :trace -1 in a table to be safe from reading the rest
		}
		else if (!stricmp(word,(char*)"spell")) flags |= TRACE_SPELLING; // isolated
		else if (!stricmp(word,(char*)"match")) flags |= TRACE_MATCH;
		else if (!stricmp(word,(char*)"variables")) flags |= TRACE_VARIABLE; 
		else if (!stricmp(word,(char*)"simple")) flags |= (TRACE_MATCH|TRACE_VARIABLE); 
		else if (!stricmp(word,(char*)"input")) flags |= TRACE_INPUT;
		else if (!stricmp(word,(char*)"ruleflow")) flags |= TRACE_FLOW;
		else if (!stricmp(word,(char*)"echo")) flags |= TRACE_ECHO;

		else if (!stricmp(word, (char*)"input")) flags |= TRACE_INPUT;
		else if (!stricmp(word,(char*)"prepare")) flags |= TRACE_INPUT|TRACE_PREPARE;
		else if (!stricmp(word,(char*)"output")) flags |= TRACE_OUTPUT;
		else if (!stricmp(word,(char*)"pattern")) flags |= TRACE_PATTERN;
		else if (!stricmp(word,(char*)"mild")) flags |= (TRACE_INPUT|TRACE_PREPARE|TRACE_OUTPUT|TRACE_PATTERN); 

		else if (!stricmp(word,(char*)"infer")) flags |= TRACE_INFER;
		else if (!stricmp(word,(char*)"sample")) flags |= TRACE_SAMPLE;
		else if (!stricmp(word,(char*)"substitute")) flags |= TRACE_SUBSTITUTE;
		else if (!stricmp(word,(char*)"hierarchy")) flags |= TRACE_HIERARCHY;
		else if (!stricmp(word,(char*)"fact")) flags |= TRACE_FACT;
		else if (!stricmp(word,(char*)"varassign")) flags |= TRACE_VARIABLESET;
		else if (!stricmp(word,(char*)"query")) flags |= TRACE_QUERY;
		else if (!stricmp(word,(char*)"user")) flags |= TRACE_USER;
		else if (!stricmp(word,(char*)"userfact")) flags |= TRACE_USERFACT;
		else if (!stricmp(word, (char*)"treetagger")) flags |= TRACE_TREETAGGER;
		else if (!stricmp(word,(char*)"pos")) flags |= TRACE_POS;
		else if (!stricmp(word,(char*)"tcp")) flags |= TRACE_TCP;
		else if (!stricmp(word,(char*)"json")) flags |= TRACE_JSON;
		else if (!stricmp(word,(char*)"macro")) flags |= TRACE_USERFN;
		else if (!stricmp(word,(char*)"usercache")) flags |= TRACE_USERCACHE;
		else if (!stricmp(word,(char*)"sql")) flags |= TRACE_SQL;
		else if (!stricmp(word,(char*)"label")) flags |= TRACE_LABEL;
		else if (!stricmp(word,(char*)"topic")) flags |= TRACE_TOPIC;
		else if (!stricmp(word,(char*)"deep")) flags |= (TRACE_JSON|TRACE_TOPIC|TRACE_FLOW|TRACE_INPUT|TRACE_USERFN|TRACE_SAMPLE|TRACE_INFER|TRACE_SUBSTITUTE|TRACE_HIERARCHY| TRACE_FACT| TRACE_VARIABLESET| TRACE_QUERY| TRACE_USER|TRACE_USERFACT|TRACE_TREETAGGER|TRACE_POS|TRACE_TCP|TRACE_USERCACHE|TRACE_SQL|TRACE_LABEL); 
		else if (!stricmp(word,(char*)"notthis")) flags |=  TRACE_NOT_THIS_TOPIC;
		else if (!stricmp(word,(char*)"always")) flags |= TRACE_ALWAYS;

		else if (!stricmp(word,(char*)"0") || !stricmp(word,(char*)"clear")) flags = 0;
		else if (!stricmp(word,(char*)"end")) break; // safe end

		else if (*word == '!') // NOT tracing a topic or function
		{
			if (word[1]) memmove(word,word+1,strlen(word));  // ! jammed against topic, separate it
			else input = ReadCompiledWord(input,word);
			WORDP D = FindWord(word);
			if (D->word[0] == '~') {
				D->internalBits |= NOTRACE_TOPIC;
				SetTopicDebugMark(FindTopicIDByName(word), 0); // clear any explicit trace on it
			}
		}
		else if (*word == '^')
		{
			WORDP FN = FindWord(word);
			if (FN) 
			{
				int val = (flags) ? flags : -1;
				if (FN->internalBits & MACRO_TRACE) val = 0; // turn off
				TraceFunction(FN,val);
				if (!fromScript) Log(ECHOSTDUSERLOG,(char*)" tracing %s %s %d 0x%08x\n",word, (FN->internalBits & MACRO_TRACE) ? (char*)"on" : (char*)"off",val,val);
				flags = priorTrace; // restore what came before
			}
			else Log(ECHOSTDUSERLOG,(char*)"No such function %s\r\n",word);
		}
		else if (!stricmp(word, "universal"))
		{
			traceUniversal = flags & (-1 ^ TRACE_ECHO);
			flags = priorTrace;
		}
		else if (*word == USERVAR_PREFIX)
		{
			WORDP D = StoreWord(word);
			D->internalBits ^= MACRO_TRACE;
			PrepareVariableChange(D,"",false);
			if (!fromScript)
			{
				Log(ECHOSTDUSERLOG,(char*)" tracing %s %s\r\n",word, (D->internalBits & MACRO_TRACE) ? (char*)"on" : (char*)"off");
			}
		}
		else if (*word == '~') // tracing a topic or rule by label
		{
			char* period = strchr(word,'.');
			if (period) *period = 0;
			int topic = FindTopicIDByName(word);
			if (topic == 0) Log(ECHOSTDUSERLOG,(char*)"No such topic %s\r\n",word);
			else if (!period) 
			{
				if (flags & TRACE_ON) flags ^= TRACE_ON;
				if (!TI(topic)->topicDebug && !flags) SetTopicDebugMark(topic,(unsigned int)(-1 ^ TRACE_ALWAYS)); // default all
				else if (flags) SetTopicDebugMark(topic,flags); // just those named previously
				else SetTopicDebugMark(topic,0); // disable
				flags = priorTrace; // restore what came before
			}
			else if (IsAlphaUTF8(period[1])) // find ALL labelled statement and mark them
			{
				int id = 0;
				char* which = GetTopicData(topic);
				bool found = false;
				while (which && *which && (which = FindNextLabel(topic,period+1,which,id,true)))
				{
					SetDebugRuleMark(topic,id);
					found = true;
					which = FindNextRule(NEXTRULE,which,id);
				}
				if (!found)  Log(STDUSERLOG,(char*)"cannot find %s.%s\r\n",word,period+1);
			}
			else if (IsDigit(period[1]))// did he use number notation?
			{
				int id = 0;
				*period = '.';
				char* rule = GetRuleTag(topic,id,word);
				if (rule) SetDebugRuleMark(topic,id);
				else Log(ECHOSTDUSERLOG,(char*)"cannot find %s.%s\r\n",word,period+1);
			}
		}
	}
	trace = flags;
	if (!fromScript) // do not show things automatically is
	{
		WalkDictionary(TracedFunction,1);
		WalkDictionary(TracedTopic,1);
		if (AreDebugMarksSet()) isTracing = true; // some rule is marked to trace
		if (trace && trace != TRACE_ON) 
		{
			trace |= TRACE_ON | TRACE_ECHO;
			if (noecho) trace ^= TRACE_ECHO;	// requested to be off
			isTracing = true;
			ShowTrace(trace,true);
		}
		else trace = 0;
		if (isTracing)  
		{
			trace |= TRACE_ON | TRACE_ECHO;
			if (noecho) trace ^= TRACE_ECHO;	// requested to be off
		}
		Log(ECHOSTDUSERLOG,(char*)" trace = %d (0x%x)\r\n",trace,trace);
		if (traceUniversal) Log(ECHOSTDUSERLOG, (char*)" universal =  0x%x\r\n", traceUniversal);
	}	
	wasCommand = TRACECMD; // save results to user file
    modifiedTraceVal = trace;
    modifiedTrace = true;
    idetrace = trace;
}

static void C_Time(char* input)
{
	char word[MAX_WORD_SIZE];
	isTiming = false;
	unsigned int flags = timing;
	input = SkipWhitespace(input);
	if (!*input)
	{
		ShowTiming(timing,true);
		return;
	}
	ReadCompiledWord(input,word);
	if (!stricmp(word,(char*)"none")) // turn off all topics and macros as well
	{
		WalkDictionary(ClearTimedFunction,0);
		WalkDictionary(ClearTimedTopic,0);
	}
	
	while (input)
	{
		input = ReadCompiledWord(input,word); // if using time in a table, use closer "end" if you are using named flags
		if (!*word) break;
		input = SkipWhitespace(input);
		if (*word == '+') // add this flag is the default
		{
			if (word[1]) memmove(word, word + 1, strlen(word));
			else continue;
		}

		if (!stricmp(word, (char*)"all")) flags = ((unsigned int)-1) ^ TIME_ALWAYS;
		else if (!stricmp(word, (char*)"none")) flags = 0;
		else if (*word == '-') // remove this flag
		{
			if (!word[1]) input = ReadCompiledWord(input, word);
			else memmove(word, word + 1, strlen(word));
			if (!stricmp(word, (char*)"notthis")) flags &= -1 ^ TIME_NOT_THIS_TOPIC;
			if (!stricmp(word, (char*)"ruleflow")) flags &= -1 ^ TIME_FLOW;

			if (!stricmp(word, (char*)"prepare")) flags &= -1 ^ TIME_PREPARE;
			else if (!stricmp(word, (char*)"pattern")) flags &= -1 ^ TIME_PATTERN;
			else if (!stricmp(word, (char*)"mild")) flags &= -1 ^ (TIME_PREPARE | TIME_PATTERN);

			else if (!stricmp(word, (char*)"query")) flags &= -1 ^ TIME_QUERY;
			else if (!stricmp(word, (char*)"user")) flags &= -1 ^ TIME_USER;
			else if (!stricmp(word, (char*)"tcp")) flags &= -1 ^ TIME_TCP;
			else if (!stricmp(word, (char*)"json")) flags &= -1 ^ TIME_JSON;
			else if (!stricmp(word, (char*)"macro")) flags &= -1 ^ TIME_USERFN;
			else if (!stricmp(word, (char*)"usercache")) flags &= -1 ^ TIME_USERCACHE;
			else if (!stricmp(word, (char*)"sql")) flags &= -1 ^ TIME_SQL;
			else if (!stricmp(word, (char*)"topic")) flags &= -1 ^ TIME_TOPIC;
			else if (!stricmp(word, (char*)"deep")) flags &= -1 ^ (TIME_JSON | TIME_TOPIC | TIME_USERFN | TIME_QUERY | TIME_USER | TIME_TCP | TIME_USERCACHE | TIME_SQL);
			else if (!stricmp(word, (char*)"always")) flags &= -1 ^ TIME_ALWAYS;
		}
		else if (IsNumberStarter(*word)  && !IsAlphaUTF8(word[1]))
		{
			ReadInt(word, *(int*)&flags);
			break; // there wont be morez flags -- want :time -1 in a table to be safe from reading the rest
		}
		else if (!stricmp(word, (char*)"ruleflow")) flags |= TIME_FLOW;

		else if (!stricmp(word, (char*)"prepare")) flags |= TIME_PREPARE;
		else if (!stricmp(word, (char*)"pattern")) flags |= TIME_PATTERN;
		else if (!stricmp(word, (char*)"mild")) flags |= (TIME_PREPARE | TIME_PATTERN);

		else if (!stricmp(word, (char*)"query")) flags |= TIME_QUERY;
		else if (!stricmp(word, (char*)"user")) flags |= TIME_USER;
		else if (!stricmp(word, (char*)"tcp")) flags |= TIME_TCP;
		else if (!stricmp(word, (char*)"json")) flags |= TIME_JSON;
		else if (!stricmp(word, (char*)"macro")) flags |= TIME_USERFN;
		else if (!stricmp(word, (char*)"usercache")) flags |= TIME_USERCACHE;
		else if (!stricmp(word, (char*)"sql")) flags |= TIME_SQL;
		else if (!stricmp(word, (char*)"topic")) flags |= TIME_TOPIC;
		else if (!stricmp(word, (char*)"deep")) flags |= (TIME_JSON | TIME_TOPIC | TIME_USERFN | TIME_QUERY | TIME_USER | TIME_TCP | TIME_USERCACHE | TIME_SQL);
		else if (!stricmp(word, (char*)"notthis")) flags |= TIME_NOT_THIS_TOPIC;
		else if (!stricmp(word, (char*)"always")) flags |= TIME_ALWAYS;

		else if (!stricmp(word, (char*)"0") || !stricmp(word, (char*)"clear")) flags = 0;
		else if (!stricmp(word, (char*)"end")) break; // safe end

		else if (*word == '!') // NOT timing a topic 
		{
			if (word[1]) memmove(word, word + 1, strlen(word));  // ! jammed against topic, separate it
			else input = ReadCompiledWord(input, word);
			WORDP D = FindWord(word);
			if (D->word[0] == '~') {
				D->internalBits |= NOTIME_TOPIC;
				SetTopicTimingMark(FindTopicIDByName(word), 0); // clear any explicit timing on it
			}
		}
		else if (*word == '^')
		{
			WORDP FN = FindWord(word);
			if (FN)
			{
				TimeFunction(FN,MACRO_TIME, -1);
				if (!fromScript)
				{
					Log(ECHOSTDUSERLOG, (char*)" timing %s %s\r\n", word, (FN->internalBits & MACRO_TIME) ? (char*)"on" : (char*)"off");
				}
			}
			else Log(ECHOSTDUSERLOG, (char*)"No such function %s\r\n", word);
		}
		else if (*word == '~') // timing a topic or rule by label
		{
			char* period = strchr(word, '.');
			if (period) *period = 0;
			int topic = FindTopicIDByName(word);
			if (topic == 0) Log(ECHOSTDUSERLOG, (char*)"No such topic %s\r\n", word);
			else if (!period)
			{
				if (flags & TIME_ON) flags ^= TIME_ON;
				if (!TI(topic)->topicTiming && !flags) SetTopicTimingMark(topic, (unsigned int)-1); // default all
				else if (flags) SetTopicTimingMark(topic, flags); // just those named previously
				else SetTopicTimingMark(topic, 0); // disable
				flags = 0;
			}
			else if (IsAlphaUTF8(period[1])) // find ALL labelled statement and mark them
			{
				int id = 0;
				char* which = GetTopicData(topic);
				bool found = false;
				while (which && *which && (which = FindNextLabel(topic, period + 1, which, id, true)))
				{
					SetTimingRuleMark(topic, id);
					found = true;
					which = FindNextRule(NEXTRULE, which, id);
				}
				if (!found)  Log(ECHOSTDUSERLOG, (char*)"cannot find %s.%s\r\n", word, period + 1);
			}
			else if (IsDigit(period[1]))// did he use number notation?
			{
				int id = 0;
				*period = '.';
				char* rule = GetRuleTag(topic, id, word);
				if (rule)
				{
					SetTimingRuleMark(topic, id);
				}
				else Log(ECHOSTDUSERLOG, (char*)"cannot find %s.%s\r\n", word, period + 1);
			}
		}
	}
	timing = flags;
	if (!fromScript) // do not show things automatically 
	{
		if (timing && timing != TIME_ON)
		{
			isTiming = true;
			ShowTiming(timing, true);
		}
		else timing = 0;
		WalkDictionary(TimedFunction,1);
		WalkDictionary(TimedTopic,1);
		if (AreTimingMarksSet()) isTiming = true; // some rule is marked to time
		if (isTiming)
		{
			timing |= TIME_ON;
		}
		Log(ECHOSTDUSERLOG, (char*)" time = %d (0x%x)\n", timing, timing);
	}
	wasCommand = TRACECMD; // save results to user file
    modifiedTimingVal = timing;
    modifiedTiming = true;
}

void C_Authorize(char* buffer)
{
	overrideAuthorization = (*buffer) ? true : false;
}

void C_Why(char* buffer)
{
	for (int i = 0;  i < responseIndex; ++i)
	{
		unsigned int order = responseOrder[i];
		int topic = responseData[order].topic;
		int id;
		char* rest = GetRuleIDFromText(responseData[order].id,id);
		Log(STDUSERLOG,(char*)"%s.%d.%d  %s\r\n",GetTopicName(topic), TOPLEVELID(id), REJOINDERID(id),ShowRule(GetRule(topic,id)));
		if (*rest) // format will be ~topic.3.0.5.3.3  where last 3 are the via rule info
		{
			topic = atoi(rest+1);
			rest = GetRuleIDFromText(rest+1,id);
			Log(STDUSERLOG,(char*)" via %s.%d.%d %s\r\n",GetTopicName(topic), TOPLEVELID(id), REJOINDERID(id),ShowRule(GetRule(topic,id)));
		}
	}
}

//////////////////////////////////////////////////////////
//// MISC COMMANDS
//////////////////////////////////////////////////////////

static void CleanIt(char* word,uint64 junk) // remove cr from source lines for Linux
{
	FILE* in = fopen(word,(char*)"rb");
	if (!in) 
	{
		(*printer)((char*)"missing %s\r\n",word);
		return;
	}
	fseek (in, 0, SEEK_END);
    size_t size = ftell(in);
	char* buf = (char*) malloc(size+2); // enough to hold the file

	fseek (in, 0, SEEK_SET);
	unsigned int val = (unsigned int) fread(buf,1,size,in);
	FClose(in);
	if (val != size)
	{
		free(buf);
		return;
	}
	buf[size] = 0;	// force an end

	// now overwrite file with proper trimming
	FILE* out = FopenUTF8Write(word);
	for (unsigned int i = 0; i < size; ++i)
	{
		if (buf[i] != '\r' && buf[i] != 26) fwrite(buf+i,1,1,out);	// remove cr and ^Z
	}
	if (buf[size-1] != '\n') fwrite((char*)"\n",1,1,out); // force ending line feed
	FClose(out);
	free(buf);
}

static void C_ExtraTopic(char* input) // topicdump will create a file in TMP/tmp.txt
{
	char fname[200];
	sprintf(fname, "%s/tmp.txt", tmp);
	FILE* in = fopen(fname,(char*)"rb");
	if (!in) 
	{
		(*printer)((char*)"missing %s/tmp.txt\r\n",tmp);
		return;
	}
	fseek (in, 0, SEEK_END);
    size_t size = ftell(in);
	fseek (in, 0, SEEK_SET);
 	extraTopicData = (char*)malloc(size+2); // enough to hold the file
	char* at = extraTopicData;
	maxFileLine = currentFileLine = 0; // prepare for BOM
	while(ReadALine(at,in,size) >= 0) {at += strlen(at);} // join all lines
	// clearly end the topic data
	strcpy(at,(char*)"``");
	(*printer)("%s",(char*)"Extra topic read\r\n");
}

static void C_Clean(char* word) // remove CR for LINUX
{
	WalkDirectory((char*)"src",CleanIt,0,false);
}

#ifndef DISCARDPOSTGRES
static void C_EndPGUser(char* word)
{
	PGUserFilesCloseCode();
}
#endif

static void BuildDummyConcept(WORDP D,uint64 junk)
{
	if (*D->word == '~') 
		CreateFact(MakeMeaning(D),Mmember,MakeMeaning(StoreWord((char*)"~a_dummy")), FACTTRANSIENT);
}

static void SortConcept(WORDP D,uint64 junk)
{
	if (*D->word == '~')
		Sortit(D->word,(int)junk); // 1 will be for 1line, otherwise take multiline as needed
}

static void Translate(char* msg,char* to, char* apikey)
{
	char url[MAX_WORD_SIZE];
	char* trandata = AllocateBuffer();
	sprintf(url,"https://translation.googleapis.com/language/translate/v2?target=%s&format=html&source=en&key=%s",
		to,apikey );
	sprintf(trandata, "{\"q\":\"%s\"}", msg);
	char* header = "";
	ARGUMENT(1) = "transient direct";
	ARGUMENT(2) = "post";
	ARGUMENT(3) = url;
	ARGUMENT(4) = trandata;
	ARGUMENT(5) = header;
	ARGUMENT(6) = "";
	*msg = 0;
#ifndef DISCARDJSONOPEN
	JSONOpenCode(msg);
#else
	http_response = 500;
#endif
	FreeBuffer();

	char* at = strstr(msg,"translatedText");
	if (http_response != 200 || !at)  ReportBug("FATAL: translate failure %s\r\n",msg);
	at += 4 + 14; // start of answer past the quote
	char word1[100];
	char phrase[MAX_WORD_SIZE];
	int numwords = 0;
	bool notphrase = false;
	*phrase = 0;
	while (at = ReadCompiledWord(at, word1))
	{
		if (!*word1) break;
		char *ptr = strstr(word1,"\\u003cbr/\\u003e"); // every word/phrase will have a trailing separator, note < and > are in utf-8
		if (ptr)
		{
			*ptr = 0; // found the end of a translation
			if (notphrase)
			{
				strcpy(msg, "!");
				msg++;
			}
			if (*phrase) 
			{
				// had previously started a phrase, so copy it over as a quoted string
				if (*word1)
				{
					strcat(phrase, word1);
					numwords++;
				}
				if (numwords > 1)
				{
					strcpy(msg, "\"");
					msg++;
					strcat(phrase, "\"");
				}
				strcpy(msg, phrase);
				msg += strlen(phrase);
				*phrase = 0;
			}
			else
			{
				// single word translation
				strcpy(msg, word1);
				msg += strlen(word1);
			}
			*msg++ = ' ';
			numwords = 0;
			notphrase = false;
		}
		else
		{
			// not reached the end of the phrase, so just save this word for now
			numwords++;
			if (!*phrase && *word1 == '!')
			{
				notphrase = true;
				if (*(word1 + 1)) strcat(phrase, word1 + 1);
			}
			else
			{
				if (numwords > 1) strcat(phrase, " ");
				strcat(phrase, word1);
			}
		}
	}
	*msg = 0;
}

static void C_TranslateConcept(char* input) // give the language & filename to write
{
#ifdef INFORMATION
	1. first use :sortconcept to get data into concepts.top 
	2. use the apikey command line parameter to name a google translate account
	3. :translateconcept german filename  (or some other supported language)
#endif
	char language[100];
	input = ReadCompiledWord(input,language);
	char* source = "concepts.top";
	if (*language == '~') // single concept named
	{
		WORDP X = FindWord(language);
		if (!X)
		{
			(*printer)("No such concept %s\r\n", language);
			return;
		}
		FClose(FopenUTF8Write((char*)"cset.txt"));
		Sortit(X->word, 1); /// 1 will be for 1line, otherwise take multiline as needed
		input = ReadCompiledWord(input, language);
		source = "cset.txt";
		if (!*language) return; // just sort it. no translate
	}

	MakeUpperCase(language);
	char* into = NULL;
	if (!stricmp(language,"ARABIC")) into = "ar";
	else if (!stricmp(language,"BENGALI")) into = "bn";
	else if (!stricmp(language,"CHINESE")) into = "zh";
	else if (!stricmp(language,"CZECH")) into = "cs";
	else if (!stricmp(language,"DUTCH")) into = "nl";
	else if (!stricmp(language,"FINNISH")) into = "fi";
	else if (!stricmp(language,"FRENCH")) into = "fr";
	else if (!stricmp(language,"GERMAN")) into = "de";
	else if (!stricmp(language,"GREEK")) into = "el";
	else if (!stricmp(language,"HINDI")) into = "hi";
	else if (!stricmp(language,"HUNGARIAN")) into = "hu";
	else if (!stricmp(language,"ITALIAN")) into = "it";
	else if (!stricmp(language,"JAPANESE")) into = "ja";
	else if (!stricmp(language,"KOREAN")) into = "ko";
	else if (!stricmp(language,"MALAY")) into = "ms";
	else if (!stricmp(language,"NORWEGIAN")) into = "no";
	else if (!stricmp(language,"PUNJABI")) into = "pa";
	else if (!stricmp(language,"POLISH")) into = "pl";
	else if (!stricmp(language,"PORTUGUESE")) into = "pt";
	else if (!stricmp(language,"RUSSIAN")) into = "ru";
	else if (!stricmp(language,"SPANISH")) into = "es";
	else if (!stricmp(language,"SWEDISH")) into = "sv";
	else if (!stricmp(language,"TELUGU")) into = "te";

	FILE* in = FopenReadOnly(source);
	if (!in)
	{
		(*printer)("No such file\r\n");
		return;
	}
	char* output = AllocateBuffer();
	char* outputforeign = AllocateBuffer();
	char* translation = AllocateBuffer();
	char name[MAX_WORD_SIZE];  // make large enough to handle long string of ####'s
	char filex[200];
	*filex = 0;
	ReadCompiledWord(input,filex);
	if (*filex) sprintf(name,"%s",filex); 
	else sprintf(name,"%s_concepts.top",language); 
	FILE* out = FopenUTF8Write(name);
	sprintf(name,"rawdict/%s_extravocabulary.txt",language);
	FILE* vocab = FopenUTF8WriteAppend(name);
	char conceptname[MAX_WORD_SIZE];
	while (ReadALine(readBuffer,in) >= 0)
	{
		*output = 0;
		*conceptname = 0;
		*outputforeign = 0;
		*translation = 0;
		char* ptr = readBuffer;
		ptr = ReadCompiledWord(ptr,name);	// concept:
		if (*name == '#') continue;	 // commented out
		if (stricmp(name, "concept:")) continue;
		char *at;
		bool found = false;
		char *lastword;
		lastword = ptr;
		while ((ptr = ReadCompiledWord(ptr,name))) // get header
		{
			if (!*name) break;
			if ((at = strchr(name, '(')))
			{
				*at = 0;
				ptr = strchr(lastword,'(') + 1;
				if (!*name) break;
				found = true;
			}
			if (!*output)
			{
				WORDP D = FindWord(name); // find this 
				if (D && !(D->internalBits & (BUILD1|BUILD0))) break; // needs to not be from base system
				strcpy(conceptname,name);
			}
			MakeUpperCase(name);
			strcat(output,name);
			strcat(output," ");
			if (found) break;
			lastword = ptr;
		}
		if (!*output) continue; // poor start
		strcat(output, "( ");

		found = false;
		while ((ptr = ReadCompiledWord(ptr,name))) // get content
		{
			if (!*name || *name == '#')
			{
				// reached end of line before the closing parenthesis, or found a comment
				if (ReadALine(readBuffer, in) < 0) break;
				ptr = readBuffer;
				continue;
			}
			if ((at = strchr(name, ')')))
			{
				*at = 0;
				if (!*name) break;
				found = true;
			}
			if (*name == '~' || (*name == '!' && *(name+1) == '~') ) // maintain concept hierarchies
			{
				strcat(output,name);
				strcat(output," ");
				if (found) break;
				continue;
			}
			if ((at = strchr(name + 1, '~'))) *at = 0;

			// need to treat each word/phrase distinctly as translation might be to a number of words
			char* at;
			while ((at = strchr(name, '_'))) 
			{ 
				if (*(at+1) == '\'') // collapse possessive 
				{
					size_t x = strlen(name) - (at - name);
					memmove(at, at + 1, x);
				}
				else
				{
					*at = ' ';  // use space instead of underscore
				}
			}
			at = name + strlen(name) - 1;
			// remove quotes because it interferes with how Google translates a phrase
			if (*name == '\"')
			{
				*at-- = 0;
				memmove(name, name + 1, strlen(name));
			}
			strcat(outputforeign, name);
			if (*at == '.') // put a space after an abbreviation so Google behaves
			{
				strcat(outputforeign, " ");
			}
			strcat(outputforeign, "<br/>");// add our own html break to indicate end of word/phrase
			if (found) break;
		}

		// translate and dump the result
		if (strlen(outputforeign) >= maxBufferSize)
			ReportBug("translate too big");
		strcat(output,"     "); // to be clear

		// google takes only 5000 char at a time. leave room for urlencoding
		char* start = outputforeign;
		while (*start)
		{
			size_t len = strlen(start);
			if (len > 3500)
			{
				char* at1 = start-1;
				char* lasthole = NULL;
				while (++at1)
				{
					if ((at1-start) > 3500) break; // accept last hole
					else if (!strncmp(at1,"<br/>",5)) lasthole = at + 1;
				}
				strncpy(translation,start,lasthole-start);
				translation[lasthole-start] = 0;
				start = lasthole + 1;
				Translate(translation,into,apikey); // de fr it es
				strcat(output,translation);
				strcat(output," ");
				size_t l = strlen(output);
				at1 = NULL; // debug hook
			}
			else // direct use or final chunk
			{
				if (start[len-1] == ' ') start[len-1] = 0;	// remove trailing blank
				if (*start) 
                    Translate(start,into,apikey); // de fr it es
				strcat(output,start);
				break;
			}
		}
		if (strlen(output) >= maxBufferSize) ReportBug("translate too big");
		if (strnicmp(output,"~a_dummy",8))
		{
			fprintf(out,"#%s concept: %s )\r\n",language,output);
			(*printer)("%s\r\n",output);

			// augmented vocab
			bool keywords = false;
			char* ptr = output;
			while (ptr && *ptr)
			{
				char word[MAX_WORD_SIZE];
				ptr = ReadCompiledWord(ptr,word);
				if (*word == '(') keywords = true;
				if (*word == '(' || *word == ')' || strchr(word,'~')) continue;
				if (keywords) fprintf(vocab,"%s\r\n",word);
			}
		}
	}
	fclose(out);
	fclose(in);
	fclose(vocab);
	FreeBuffer();
	FreeBuffer();
	FreeBuffer();
	(*printer)("translation of %s complete\r\n",input);
}

static void C_SortConcept(char* input)
{
#ifdef INFORMATION
	To get concepts in a file sorted alphabetically(both by concept and within), do
		0. empty TOPICS
		0. :build concept0
		1. : sortconcept x		-- builds one concept per line and sorts the file by concept name  outputs to concepts.top
		2. take the contents of concepts.top that was written at top level and replace the original file in ONTOLOGY, erase TOPICS
		3. : build concept0
		4. : sortconcept			-- maps concepts neatly onto multiple lines
		5. take the contents of cset.txt and replace the original file

		If you are trying to get level 1 concepts from a bot, then
		1. delete BUILD0
		2. : build botname
		3. if you use boot startup, set a command line parameter to "noboot"
		4. : sortconcept x  -- one concept per line
#endif
	WORDP D = StoreWord((char*)"~a_dummy", AS_IS);
	char which[100];
	int64 flags;
	input = ReadCompiledWord(input, which);
	input = ReadInt64(input, flags);
	if (*which != '~') // do all concepts and tie to dummy
	{
		WalkDictionary(BuildDummyConcept, 0); // stores names of concepts on dummy concept, to lock position in dictionary. later not, will be read in
		AddInternalFlag(D, BUILD0 | CONCEPT);
	}

	FClose(FopenUTF8Write((char*)"cset.txt"));
	if (!flags) // hide this on second pass
	{
		D = FindWord((char*)"~a_dummy");
		RemoveInternalFlag(D, BUILD0);
	}
	if (*which == '~')
	{
		D = FindWord(which);
		if (D) SortConcept(D, flags);
		else (*printer)("No such concept %s\r\n", which);
	}
	else
	{
		WalkDictionary(SortConcept, flags); // 0 (1 per line) or 1 (spread per concept)
		if (flags) system((char*)"sort /rec 63000 c:/chatscript/cset.txt >concepts.top");
		else system((char*)"copy c:/chatscript/cset.txt >concepts.top");
	}
}

//////////////////////////////////////////////////////////
//// ANALYTICS
//////////////////////////////////////////////////////////

static void DisplayTables(char* topicid)
{
	char args[MAX_WORD_SIZE];
	sprintf(args,(char*)"( %s )", topicid);
	Callback(FindWord(GetUserVariable((char*)"$cs_abstract")),args,false,true);
}

static void DoHeader(int count,char* basic,FILE* in,int id,unsigned int spelling,int ruleid, int rejoinderid,int topicID)
{
	if (*abstractBuffer == 0) 	// no more verification data for this topic
	{
		// display header
		if (!lineLimit)	
		{
			TabInset(count,false);
            char label[MAX_WORD_SIZE];
            sprintf(label, (char*)"R-%s-%s.%d.%d", computerID, GetTopicName(topicID), ruleid, rejoinderid);
            WORDP D = FindWord(label, AS_IS);
            if (D) Log(STDUSERLOG, (char*)"(%d) ", statistics[D->word] = statistics[D->word]);
            Log(STDUSERLOG, (char*)"%s", basic);
		}
		return;
	}

	// get verification matching input -- ~abortion.0.0 #! I am against abortion.
	static int readID = 0;
	static char* test = NULL;
	static char type = 0;
	if ((unsigned char)*abstractBuffer == 1) readID = -1; // read 1st line of topic data
retry:
	while (readID == -1 || TOPLEVELID(id) > TOPLEVELID(readID) ||  ( TOPLEVELID(id) == TOPLEVELID(readID) && REJOINDERID(id) > REJOINDERID(readID)  )) // flush reads until get 1st good one
	{
		if (ReadALine(abstractBuffer,in) < 0) break;	// no more verifcation data
		char* dot = strchr(abstractBuffer,'.');
		char* dot1 = strchr(dot+1,'.');
		readID = MAKE_REJOINDERID(atoi(dot1+1)) + atoi(dot+1); // the id pending
		test = strchr(abstractBuffer,'#');
		type = test[2];
		if (!(spelling & ABSTRACT_PRETTY)) test += 2;
		if ((type == 'x' || type == 'X') && *test != ' ' && ((TOPLEVELID(id) > TOPLEVELID(readID)) ||  (TOPLEVELID(id) == TOPLEVELID(readID) && REJOINDERID(id) > REJOINDERID(readID)) )) // global topic comment, dump it immediately and keep going
		{
			Log(STDUSERLOG,(char*)"\r\n%s\r\n",test+1); 
			readID = -1;
		}
	}

	if (test && (type == 'x' || type == 'X') && *test != ' ' && readID == id) // global topic comment for current match
	{
		Log(STDUSERLOG,(char*)"\r\n%s\r\n",test+1); 
		readID = -1;
		goto retry;
	}

	// since we have sample input, kill pattern
	if (id == readID && *basic != ' ' && !(spelling & ABSTRACT_PRETTY)) 
	{
		unsigned int offset = 2;
		while (basic[offset] && basic[offset] != '(') ++offset; // find end of blank space before pattern.
		if (basic[offset]) basic[--offset] = 0;  
	}

	// display header
	if (spelling & ABSTRACT_PRETTY && id == readID)  
	{
		TabInset(count,false);
		Log(STDUSERLOG,(char*)"%s\r\n",test);
	}
	if (!lineLimit)	
	{
		TabInset(count,false);

        char label[MAX_WORD_SIZE];
        sprintf(label, (char*)"R-%s-%s.%d.%d", computerID, GetTopicName(topicID),ruleid,rejoinderid);
        WORDP D = FindWord(label, AS_IS);
        if (D) Log(STDUSERLOG, (char*)"(%d) ", statistics[D->word] = statistics[D->word]);
		Log(STDUSERLOG,(char*)"%s",basic); 
	}

	// display verify as pattern
	if (id == readID && !lineLimit && !(spelling & ABSTRACT_PRETTY)) 
	{
		Log(STDUSERLOG,(char*)" %s =>   ",test);
	}
}

static void TopicHeader(int spelling, int topicID)
{
    if (spelling & ABSTRACT_PRETTY)
    {
        int count = 0;
        char label[MAX_WORD_SIZE];
        sprintf(label, (char*)"T-%s-%s", computerID, GetTopicName(topicID));
        WORDP E = FindWord(label, AS_IS);
        if (E) count = statistics[E->word];

        unsigned int lineSize = 0;
        if (count) Log(STDUSERLOG, (char*)"\r\n(%d) TOPIC: %s", count,GetTopicName(topicID));
        else Log(STDUSERLOG, (char*)"\r\nTOPIC: %s", GetTopicName(topicID));

        unsigned int flags = GetTopicFlags(topicID);
        if (flags & TOPIC_SYSTEM) Log(STDUSERLOG, " SYSTEM");
        if (flags & TOPIC_KEEP) Log(STDUSERLOG, " KEEP");
        if (flags & TOPIC_REPEAT) Log(STDUSERLOG, " REPEAT");
        if (flags & TOPIC_RANDOM) Log(STDUSERLOG, " RANDOM");
        if (flags & TOPIC_NOSTAY) Log(STDUSERLOG, " NOSTAY");
        if (flags & TOPIC_PRIORITY) Log(STDUSERLOG, " PRIORITY");
        if (flags & TOPIC_LOWPRIORITY) Log(STDUSERLOG, " DEPRIORITIZE");
        if (flags & TOPIC_NOBLOCKING) Log(STDUSERLOG, " NOBLOCKING");
        if (flags & TOPIC_NOPATTERNS) Log(STDUSERLOG, " NOPATTERNS");
        if (flags & TOPIC_NOGAMBITS) Log(STDUSERLOG, " NOGAMBITS");
        if (flags & TOPIC_NOSAMPLES) Log(STDUSERLOG, " NOSAMPLES");
        if (flags & TOPIC_NOKEYS) Log(STDUSERLOG, " NOKEYS");
        if (flags & TOPIC_SAFE) Log(STDUSERLOG, " SAFE");
        if (flags & TOPIC_SHARE) Log(STDUSERLOG, " SHARE");
        Log(STDUSERLOG, " (");
        WORDP D = FindWord(GetTopicName(topicID));
        FACT* F = GetObjectNondeadHead(D);
        char* word = AllocateStack(NULL, maxBufferSize);
        while (F)  // keywords of topic
        {
            if (ValidMemberFact(F) || F->verb == Mexclude)
            {
                if (F->flags & ORIGINAL_ONLY) sprintf(word, (char*)"'%s ", WriteMeaning(F->subject));
                else sprintf(word, (char*)"%s ", WriteMeaning(F->subject));
                if (F->verb == Mexclude) Log(STDUSERLOG, (char*)"!");
                size_t wlen = strlen(word);
                lineSize += wlen;
                Log(STDUSERLOG, "%s", word);
                if (lineSize > 500) // avoid long lines
                {
                    Log(STDUSERLOG, (char*)"\r\n     ");
                    lineSize = 0;
                }
            }
            F = GetObjectNondeadNext(F);
        }
        ReleaseStack(word);
        Log(STDUSERLOG, (char*)")\r\n\r\n");
    }
    else
    {
        Log(STDUSERLOG, (char*)"\r\n****** TOPIC: %s", GetTopicName(topicID));

        char label[MAX_WORD_SIZE];
        sprintf(label, (char*)"T-%s-%s", computerID, GetTopicName(topicID));
        WORDP D = FindWord(label, AS_IS);
        if (D && statistics[D->word]) // from spelling == 10 coverage
        {
            Log(STDUSERLOG, (char*)" (%d) ", statistics[D->word]);
        }
        topicBlock* block = TI(topicID);
        if (block && block->topicRestriction) Log(STDUSERLOG, (char*)"  restricted to: %s\r\n", block->topicRestriction);
        Log(STDUSERLOG, (char*)"\r\n");
    }
}
static void DisplayTopic(char* name,int topicID,int spelling)
{
	if (!topicID) topicID = FindTopicIDByName(name,true);
	if (!topicID) return;
	char* rule = GetTopicData(topicID); 
	if (!rule) return;
	if (spelling & ABSTRACT_STORY && !GAMBIT_MAX(TI(topicID)->topicMaxRule)) return; // has no gambits
	
	*abstractBuffer = 1;	// buffer started for new topic
    TopicHeader(spelling,topicID);
	WORDP D = FindWord(name);
	char word[MAX_WORD_SIZE];
	char fname[MAX_WORD_SIZE];
	sprintf(fname,(char*)"VERIFY/%s-b%c.txt",name+1,(D->internalBits & BUILD0) ? '0' : '1');
	FILE* in = FopenReadWritten(fname);

	bool preprint;
	char* old = NULL;
	char* buffer = AllocateBuffer();
	char* tmpBuffer = AllocateBuffer();
	char label[MAX_WORD_SIZE];
	char* pattern = AllocateBuffer();
	char* basic = AllocateBuffer();
	int id = 0;
	char bodyKind[100];
    int ruleid = 0;
    int rejoinderid = 0;

	while (rule && *rule) // for each rule
	{
		preprint = false;
		char* output = GetPattern(rule,label,pattern,true);
		char* end = strchr(output,'`');
		bool norule = EmptyReuse(output,topicID);
		if (!*output || *output == ENDUNIT || norule) 
		{
			rule = FindNextRule(NEXTRULE,rule,id);
            if (TopLevelRule(rule))
            {
                ++ruleid = 0;
                rejoinderid = 0;
            }
            else ++rejoinderid;
			continue;
		}
		if (spelling & ABSTRACT_STORY)
		{
			char* topLevelRule = GetRule(topicID,TOPLEVELID(id));	// the top level rule (if a rejoinder)
			if (TopLevelQuestion (topLevelRule) || TopLevelStatement(topLevelRule)) 
			{
				rule = FindNextRule(NEXTRULE,rule,id);
				continue;
			}
		}
		if (spelling & ABSTRACT_RESPONDER)
		{
			char* topLevelRule = GetRule(topicID,TOPLEVELID(id));	// the top level rule (if a rejoinder)
			if (!TopLevelQuestion (topLevelRule) && !TopLevelStatement(topLevelRule)) 
			{
				rule = FindNextRule(NEXTRULE,rule,id);
				continue;
			}
		}

		if (spelling & ABSTRACT_VP)
		{
			char* end1 = strchr(output,ENDUNIT);
			*end1 = 0;
			if (*rule == QUESTION || *rule == STATEMENT_QUESTION)
			{
				if (!*label && strstr(output,(char*)"factanswer")) Log(STDUSERLOG,(char*)"No label for: %s %s\r\n",pattern,output);
			}
			*end1 = ENDUNIT;
			rule = FindNextRule(NEXTRULE,rule,id);
			continue;
		}
		if (spelling & ABSTRACT_PRETTY) // revise pattern for cannonical
		{
			*tmpBuffer = 0;
			char word[MAX_WORD_SIZE];
			char* pbase = pattern;
			//if (*label) 
			//{
				//strcat(tmpBuffer,label);
				//strcat(tmpBuffer,(char*)" ");
			//}
			while (pbase && *pbase)
			{
				pbase = ReadCompiledWord(pbase,word);
				if (IsAlphaUTF8(word[0]) && strchr(word,'_') && spelling & ABSTRACT_PRETTY ) // is it a word or a phrase
				{
					WORDP X = FindWord(word);
					if (X && X->properties & PART_OF_SPEECH) {;} // known word
					else // make it a phrase
					{
						Convert2Blanks(word);
						strcat(word,(char*)"\"");	// closing quote
						memmove(word+1,word,strlen(word)+1);
						*word = '"';
					}
				}
				if (IsAlphaUTF8(word[0]) && spelling & ABSTRACT_CANONICAL) // could be made canonical
				{
					WORDP entry, canonical;
					uint64 sysflags = 0;
					uint64 cansysflags = 0;
					WORDP revise;
					GetPosData(-1,word,revise,entry,canonical,sysflags,cansysflags);
					if (canonical)
					{
						// if canonical is upper and entry is lower, dont show canonical
						if (entry && canonical->internalBits & UPPERCASE_HASH && !(entry->internalBits & UPPERCASE_HASH)) {;}
						else if (!stricmp(canonical->word,(char*)"unknown-word")) {;}
						else strcpy(word,canonical->word);
					}
				}
				strcat(tmpBuffer,word);
				strcat(tmpBuffer,(char*)" ");
			}
			strcpy(pattern,tmpBuffer);
		}

		// std rule header
		unsigned int kind = *rule;
		basic[0] = (unsigned char)kind;
		basic[1] = ':';
		basic[2] = ' ';
		basic[3] = 0;
		if (*label)
		{
			strcat(basic,label);
			strcat(basic,(char*)"  ");
		}
		int choiceNest = 0;
		char* choiceStart = NULL;
		unsigned int choiceCharacters = 0;

		// revise comparison patterns
		if (*pattern)
		{
			char* compare = pattern;
			while (ALWAYS)
			{
				char* compare1 = strstr(compare,(char*)" !="); // hunt for comparison operators
				compare = strstr(compare,(char*)" =");
				if (compare1)
				{
					if (compare1 < compare || !compare) compare = compare1;
				}
				if (!compare) break;
				if (*++compare != '=') ++compare; // negated compare
				if (compare[1] != ' ') memmove(compare,compare+2,strlen(compare+1));// remove header and accelerator of comparison
			}
			if ((kind == 't' || kind == 'r') && *pattern == '(' && pattern[1] == ' ' && pattern[2] == ')') *pattern = 0;	// there is no pattern really for this gambit
			else
			{
				strcat(basic,pattern);
				if (!(spelling & ABSTRACT_PRETTY)) strcat(basic,(char*)" => ");
			}
		}
		
		// now determine the output
		unsigned int indent = Rejoinder(rule) ? ((*rule - 'a' + 1) * 2) : 0; 
		char* outputPtr = buffer;
		*outputPtr = 0;
		bool badspell = false;
		int hasBody = 0;
		end = strchr(output,ENDUNIT);
 		*end = 0;
		bool badWord = false;
		bool multipleOutput = false;
		int level = 0;
		char levelMark[1000];
		levelMark[0] = 0;
		char* prior = "";
		char* prior2 = "";
        if (spelling & ABSTRACT_NOCODE) output = NULL;
		while (output && *output && *output != ' ') // read output until end of rule
		{
			if (spelling & ABSTRACT_PRETTY) // line by line neatened output
			{
				prior2 = prior;
				prior = output;
				output = ReadDisplayOutput(output,word);
                if (!stricmp(word, "^if ")) 
                    output += ACCELLSIZE; // skip accelerator into if
                if (!stricmp(word, "else ") && *output != '(')
                    output += ACCELLSIZE; // skip accelerator into else

                if (!*word) break;	// nothing left
				if (*word == '}') 
				{
					--level;
					if (level < 0) 
						level = 0;
				}
				// for ^if testing zone, remove accelerator
				if (word[3] == '{' && word[4] == ' ' && !word[5]) strcpy(word,(char*)"{ ");
				if (*word != '^' && *word != '$' && word[0] && word[1] && word[2] && (word[3] == ' ' || word[3] == '(') && !word[4]) // possible accelerator
				{
					if (!strnicmp(prior2,(char*)"^^if",4)) continue;	// ignore accelerator after iftest to skip to next test
					if (!strnicmp(prior2,(char*)"^^loop",6)) continue;	// ignore accelerator at start of loop to skip
                    if (!strnicmp(prior2, (char*)"^^jsonloop", 10)) continue;	// ignore accelerator at start of loop to skip
                    if (!strnicmp(prior2,(char*)"} ",2) && levelMark[level+1] == 'i') continue;	 // ignore jump after if branch to end of whole if
                    if (!strnicmp(prior2, (char*)") ", 2) && levelMark[level + 1] == 'i') continue;	 // ignore jump after if branch to end of whole if
                }
				
				if (multipleOutput) for (unsigned int j = 0; j < (indent + (level * 2) + 4); ++j) 
				{
					sprintf(outputPtr,(char*)"%s",(char*)"  ");
					outputPtr += 2;
				}
				if (*word == '^' && word[1] == '^') memmove(word,word+1,strlen(word));	// ^^if and ^^loop make normal user written
				sprintf(outputPtr,(char*)"%s\r\n",word); // abstract puts lf after EACH item
				outputPtr += strlen(outputPtr);
                multipleOutput = true;
				if (*word == '{' ) 
				{
					++level;
					levelMark[level] = 0;
					if (!strnicmp(prior2,(char*)"^^if",4)) levelMark[level] = 'i'; // is an if level
				}
				continue;
			}

			output = ReadCompiledWord(output,word);
			if (!*word) break; 
			if (*word == '+') break;	// skip assignment
			next:
			switch(*word)
			{
			case '[': // choice area, with optional label
				++choiceNest;
				choiceStart = outputPtr;
				sprintf(outputPtr,(char*)"%s ",word);
				outputPtr += strlen(outputPtr);
				output = ReadCompiledWord(output,word);
				if (word[1] == ':' && !word[2]) // jump label
				{
					sprintf(outputPtr,(char*)"%s ",word); 
					outputPtr += strlen(outputPtr);
					output = ReadCompiledWord(output,word);
				}
				goto next;
			case ']':
				if (--choiceNest == 0)
				{ 
					int len =  outputPtr - choiceStart; // size of [] 
					strcpy(outputPtr++,(char*)"]");
					if (!spelling && len >= lineLimit && len && lineLimit) Log(STDUSERLOG,(char*)"(%d) %s\r\n",len,choiceStart);
					choiceCharacters += len; 
				}
				break;
			case ')':
				if (preprint) //   closing preprint call
				{
					preprint = false;
					continue;
				}
				break;
			case '}':
				if (hasBody)
				{
					if (bodyKind[hasBody] == 'i') // if
					{
						output = strchr(output,' ');
						if (output) ++output; // skip end sizing jump rule
					}
					--hasBody;
					continue;
				}
				break;
			case USERVAR_PREFIX:
			{
				if (IsDigit(word[1])) break; // money $
				char next[MAX_WORD_SIZE];
				ReadCompiledWord(output, next);
				if (!IsComparator(next))
				{
					sprintf(outputPtr, (char*)"%s ", word);
					outputPtr += strlen(outputPtr);
					continue;
				}
			}
				// flow into these other variables
			case SYSVAR_PREFIX: case '_': case '@': // match variable or set variable
				if (*output == '=' || output[1] == '=') // assignment
				{
					output = ReadCompiledWord(output,word); // assign op
					output = ReadCompiledWord(output,word); // rhs item
					if (*word == '^' && *output == '(') output = BalanceParen(output+1,true,false); // rhs function call
					while (IsArithmeticOperator(output)) // arithmetic with assignment
					{
						output = ReadCompiledWord(output,word); // op
						output = ReadCompiledWord(output,word);  // next rhs item
						if (*word == '^' && *output == '(') output = BalanceParen(output+1,true,false); // rhs function call
					}
					continue;
				}
				break;
			case '^': // function call or argument
				if (!stricmp(word,(char*)"^preprint") || !stricmp(word,(char*)"^print") || !stricmp(word,(char*)"^insertprint") || !stricmp(word,(char*)"^postprint")) // show content
				{
					output = ReadCompiledWord(output,word);
					preprint = true;
					continue;
				}
				else if (!stricmp(word,(char*)"^reuse") || !stricmp(word,(char*)"^gambit") || !stricmp(word,(char*)"^respond")|| !stricmp(word,(char*)"^refine"))
				{
					output -= strlen(word) + 1;
					char* end = strchr(output,')');
					char c = end[1];
					end[1] = 0;
					strncpy(outputPtr,output,strlen(output));
					outputPtr += end - output + 1;
					*outputPtr = 0;
					end[1] = c;
					output = end + 1;
				}
				else if ((!stricmp(word,(char*)"^^if") || !stricmp(word,(char*)"^^loop") || !stricmp(word, (char*)"^^jsonloop")) && *output == '(')
				{
					++hasBody;
					bodyKind[hasBody] = word[2]; // i or l
					output = strchr(output,'{') + 2;
					continue;
				}
				else if (word[1] == '\'' || word[1] == '"')
				{
					sprintf(outputPtr, (char*)"%s ", word);
					outputPtr += strlen(outputPtr);
				}
				else if (*output == '(') output = BalanceParen(output+1,true,false); //  end call
				break;
			case ':':  // shouldnt be label inside []
				break;
			case '=': // assignment
				old = outputPtr;
				while (*old && *--old && *old != ' '); // find LHS of assignment
				if (*old == ' ') // erase left hand of assignment
				{
					outputPtr = old + 1;
					if (*outputPtr == USERVAR_PREFIX || *outputPtr == '_' || *outputPtr == '@' || *outputPtr == SYSVAR_PREFIX) *outputPtr = 0;
				}
				if (*output != '^') output = ReadCompiledWord(output,word);	// swallow next when not a function call
				break;
			case '~': 
				break;
			case '\\':
				if (word[1] == '"')
				{
					sprintf(outputPtr,(char*)"%s ",word+1);
					outputPtr += strlen(outputPtr);
				}
				else 
				{
					sprintf(outputPtr,(char*)"%s ",word);
					outputPtr += strlen(outputPtr);
				}
				break;
			default: // ordinary words usually
				if (!stricmp(word,(char*)"else") && (*output == '(' || *output =='{')) //  else {}  OR else if () {}
				{
					++hasBody;
					bodyKind[hasBody] = 'i'; // if
					if (*output != '{') output = strchr(output,'{');
					output += 2;
					continue;
				}
				else 
				{
					bool wrong = false;
					if (spelling == ABSTRACT_SET_MEMBER && word[1])
					{
						D = FindWord(word,0,LOWERCASE_LOOKUP);
						if (!D || D->inferMark != inferMark) D = FindWord(word,0,UPPERCASE_LOOKUP);
						if (D && D->inferMark == inferMark) badWord = true;
					}
					char copy[MAX_WORD_SIZE];
					MakeLowerCopy(copy,(*word == '\'') ? (word+1) : word);
					if (spelling & ABSTRACT_SPELL && word[1]) // ignore punctuation
					{
						if (*word == '*' || IsDigit(*word)) continue;	 // ignore wildcards, numbers, or jump zones
						size_t len = strlen(copy);
						if (copy[len-1] == '{') continue;  // an IF jump zone
						while (len-- && IsPunctuation(copy[len])) copy[len] = word[len] = 0;  // remove trailing punctuation.
						char* apostrophe = strchr(copy,'\'');
						D = FindWord(copy);
						if (!D || !(D->properties & PART_OF_SPEECH)) 
						{
							if (!D || !(D->internalBits & HAS_SUBSTITUTE)) // not known, try for sentence head one
							{
								char word[MAX_WORD_SIZE];
								sprintf(word,(char*)"<%s",copy);
								D = FindWord(word);
							}
							if (!D) D = FindWord(copy,0,UPPERCASE_LOOKUP);
						}
						if (D && (D->properties & PART_OF_SPEECH || D->internalBits & HAS_SUBSTITUTE)){;} //  we know this word
						else if (D && D->internalBits & QUERY_KIND) {;} // a query
						else if (IsUrl(copy,0) || apostrophe || copy[0] == '_' || copy[0] == USERVAR_PREFIX || copy[0] == SYSVAR_PREFIX || copy[0] == '@' || copy[0] == '"') {;} 
						else if (!FindCanonical( copy, 1,true)) wrong = badspell = true;
					}
					if (wrong) 
						Log(STDUSERLOG,(char*)"%s\r\n",word);
					else sprintf(outputPtr,(char*)"%s ",word);
					outputPtr += strlen(outputPtr);
				}
			}
		} 
		
		*end = ENDUNIT; // restore data
		char* finish = buffer + strlen(buffer) - 1;
		if (*finish == ' ') *finish = 0;

		// we have the output, what to do with it

		if (spelling & ABSTRACT_SET_MEMBER && !badWord) *buffer = 0; // only do lines with censored words, showing context
		if (spelling & ABSTRACT_SPELL) *buffer = 0; // only do lines with bad spelling
		bool headit = false;
		size_t len = strlen(buffer);
		if (lineLimit) // check for sections between \n that are too long.
		{
			char* esc;
			char* at = buffer;
			while ((esc = strchr(at,'\\')))
			{
				if (esc[1] != 'n') at = esc+2;
				else
				{
					len = at - esc - 1;
					char wordx[MAX_WORD_SIZE];
					ReadCompiledWord(at,wordx);
					size_t wsize = strlen(wordx);
					if (wordx[wsize-1] == ':') len -= wsize + 1;		// remove speaker flag
					if ((int)len > lineLimit) headit = true;
					at = esc+2;
				}
			}
			len = strlen(at);
			if (choiceCharacters) len -= choiceCharacters - 1; // dont zero out len
			char word[MAX_WORD_SIZE];
			ReadCompiledWord(at,word);
			size_t wsize = strlen(word);
			if (word[wsize-1] == ':') len -= wsize + 1;		// remove speaker flag
			if ((int)len > lineLimit) 
			{
				headit = true;
				longLines++;
			}
		}
		else headit = true;

		if (headit && !(spelling & ABSTRACT_RESTRICTIONS)) 
		{
			DoHeader(indent,basic,in,id,spelling,ruleid, rejoinderid, topicID);
			if (lineLimit) // check for sections between \n that are too long.
			{
				char* esc;
				char* at = buffer;
				char* start = at;
				while ((esc = strchr(at,'\\')))
				{
					if (esc[1] != 'n') at = esc+1;
					else
					{
						*esc = 0;
						len = strlen(start);
						char word[MAX_WORD_SIZE];
						ReadCompiledWord(at,word);
						size_t wsize = strlen(word);
						if (word[wsize-1] == ':') 
						{
							len -= wsize + 1;		// remove speaker flag and space
							start += wsize + 1;
						}
						if (choiceCharacters) len -= choiceCharacters - 1; // dont zero out len
						if ((int)len > lineLimit) Log(STDUSERLOG,(char*)"(%d) %s\r\n",len,start);
						choiceCharacters = 0;
						*esc = '\\';
						start = at = esc + 3; // skip \n and space
					}
				}
				len = strlen(at);
				char wordx[MAX_WORD_SIZE];
				ReadCompiledWord(at,wordx);
				size_t wsize = strlen(wordx);
				if (wordx[wsize-1] == ':') len -= wsize + 1;		// remove speaker flag
				if (choiceCharacters) len -= choiceCharacters - 1; // dont zero out len
				if ((int)len > lineLimit) Log(STDUSERLOG,(char*)"(%d) %s\r\n",len,at);
				choiceCharacters = 0;
			}
			else 
			{
				// convert \n to linefeeds
				char* lf;
				unsigned int gap = 2;
				if (*basic == 'a') gap = 6;
				while ((lf = strstr(buffer,(char*)"\\n")))
				{
					*lf = '\r';
					lf[1] = '\n';
					memmove(lf+2 + gap,lf+2,strlen(lf+2)+1);
					for (unsigned int i = 0; i < gap; ++i) lf[2+i] = ' ';
				}
				Log(STDUSERLOG,(char*)"%s\r\n",buffer);
			}
		}
		*end = ENDUNIT;
		rule = FindNextRule(NEXTRULE,rule,id);
        if (TopLevelRule(rule))
        {
            ++ruleid;
            rejoinderid = 0;
        }
        else ++rejoinderid;
	}
	FClose(in);
	FreeBuffer();
	FreeBuffer();
    FreeBuffer();
    FreeBuffer();
}

static void MarkDownHierarchy(MEANING T)
{
    if (!T) return;
    WORDP D = Meaning2Word(T);
	if (D->inferMark == inferMark) return;	
	D->inferMark = inferMark;

	if (*D->word == '~') // follow members of set
	{
		FACT* F = GetObjectNondeadHead(D);
		while (F)
		{
			if (ValidMemberFact(F))
			{
				MEANING M = F->subject;
				WORDP S = Meaning2Word(M);
				if (S->inferMark != inferMark) MarkDownHierarchy(M);
			}
			F = GetObjectNondeadNext(F);
		}
	}
}

static void C_Coverage(char* input)
{
	char fname[200];
	sprintf(fname, "%s/coverage.txt", tmp);
	FILE* out = FopenUTF8Write(fname);
	if (!out) return;
	for (int i = 1; i <= numberOfTopics; ++i) 
	{
		char* name = GetTopicName(i);
		char* data = GetTopicData(i);
		int id = 0;
		while (data && *data)
		{
			char label[MAX_WORD_SIZE];
			char pattern[MAX_WORD_SIZE];
			GetPattern(data,label,pattern,true);
			if (data[2] != ' ') // write out used ones
			{
				int referenceValue = (unsigned char)data[2]; // centered around 32 which is blank
				if (referenceValue == 31) referenceValue = 254;
				else if (referenceValue > 32) referenceValue -= 32; // counted
				else referenceValue = (0xff - 32) + referenceValue;	// counting up
				fprintf(out,"%s.%d.%d %d %s\r\n",name,TOPLEVELID(id),REJOINDERID(id),referenceValue,label);
			}
			data = FindNextRule(NEXTRULE,data,id);
		}
	}
	fclose(out);
}

static void C_ShowCoverage(char* input)
{
	char fname[200];
	sprintf(fname, "%s/coverage.txt", tmp);
	FILE* in = FopenReadOnly(fname);
	if (!in) return;
	sprintf(fname, "%s/coverageresult.txt", tmp);
	FILE* out = FopenUTF8Write(fname);
	if (!out) return;
	maxFileLine = currentFileLine = 0;
	ReadALine(readBuffer,in); // get a data
	char topictag[MAX_WORD_SIZE];
	char* ptr = ReadCompiledWord(readBuffer,topictag);
	for (int i = 1; i <= numberOfTopics; ++i) 
	{
		char* name = GetTopicName(i);
		fprintf(out,"\r\ntopic: %s\r\n",name);
		char* data = GetTopicData(i);
		int id = 0;
		while (data && *data)
		{
			char label[MAX_WORD_SIZE];
			char pattern[MAX_WORD_SIZE];
			char* output = GetPattern(data,label,pattern,true);
			pattern[40] = 0;
			char tag[MAX_WORD_SIZE];
			sprintf(tag,"%s.%d.%d",name,TOPLEVELID(id),REJOINDERID(id));
			char c = output[40];
			output[40] = 0;
			if (*data >= 'a' && *data <= 'q')
			{
				int tab = *data - 'a' + 1;
				while (tab--) fprintf(out,"  ");
			}
			if (!stricmp(tag,topictag)) // we have data
			{
				char count[MAX_WORD_SIZE];
				ReadCompiledWord(ptr,count);
				fprintf(out,"%3d   %c: %s %s %s\r\n",atoi(count),*data,label,pattern,output);
				ReadALine(readBuffer,in); // get  next data
				ptr = ReadCompiledWord(readBuffer,topictag);
			}
			else fprintf(out,"      %c: %s %s %s\r\n",*data,label,pattern,output);
			output[40] = c;
			data = FindNextRule(NEXTRULE,data,id);
		}
	}
	fclose(out);
	fclose(in);
}

static void C_Abstract(char* input)
{
	int spelling = 0;
    volleyCounter = 0;
    relevantVolleyCounter = 0;
	lineLimit = 0;
	longLines = 0;
	if (IsDigit(*input)) input = ReadInt(input,lineLimit); // line length limit
	char word[MAX_WORD_SIZE];
	input = SkipWhitespace(input);
	while (input && *input)
	{
		if (*input == '~') break;
		input = ReadCompiledWord(input,word);
		if (!stricmp(word,(char*)"spell")) spelling |= ABSTRACT_SPELL;
		else if (!stricmp(word,(char*)"censor"))
		{
			spelling |= ABSTRACT_SET_MEMBER;
			input = ReadCompiledWord(input,word);
			NextInferMark();
			MarkDownHierarchy(MakeMeaning(StoreWord(word)));
		}
		else if (!stricmp(word,(char*)"canon")) spelling |= ABSTRACT_CANONICAL | ABSTRACT_PRETTY;
		else if (!stricmp(word,(char*)"pretty")) spelling |= ABSTRACT_PRETTY;
		else if (!stricmp(word,(char*)"vp")) spelling |= ABSTRACT_VP;
		else if (!stricmp(word,(char*)"story")) spelling |= ABSTRACT_STORY;
		else if (!stricmp(word,(char*)"responder")) spelling |= ABSTRACT_RESPONDER;
		else if (!stricmp(word,(char*)"nocode")) spelling |= ABSTRACT_NOCODE;
        else if (!stricmp(word, (char*)"coverage"))
        {
            spelling |= ABSTRACT_COVERAGE | ABSTRACT_PRETTY;
            input = SkipWhitespace(input);
            input = ReadCompiledWord(input, word);
        }
        else break; // not recognized, might be filename
        input = SkipWhitespace(input);
	}
	input = SkipWhitespace(input);

    if (spelling & ABSTRACT_COVERAGE)
    {
        TrimIt(word, 10); // mark usage of log
        Log(STDUSERLOG, (char*)"Volleys processed: %d Relevant: %d \r\n", volleyCounter, relevantVolleyCounter);
    }
	abstractBuffer = AllocateBuffer();

	size_t len = 0;
	char* x = strchr(input,'*');
	if (x) len = x - input;
	else if (*input == '~') len = strlen(input);

	// get topic if given
	if (*input == '~' && !x)
	{
		char word[MAX_WORD_SIZE];
		while (input && *input)
		{
			input = ReadCompiledWord(input,word);
 			DisplayTopic(word,0,spelling);
			DisplayTables(word);
		}
	}
    // named a file to display all topics from
	else if (*input && *input != '~' && !(spelling & ABSTRACT_COVERAGE)) // from topic file
	{
		char filename[SMALL_WORD_SIZE];
		ReadCompiledWord(input,filename);
		for (int i = 1; i <= numberOfTopics; ++i) 
		{
 			if (!stricmp(GetTopicFile(i),filename)) DisplayTopic(GetTopicName(i),0,spelling);
		}
	}
	// otherwise do all
	else
	{
		for (int i = 1; i <= numberOfTopics; ++i) 
		{
			if (!*GetTopicName(i)) continue;
            if (*input == '~' && stricmp(input, GetTopicName(i))) continue; // only do named topic
            topicBlock* block = TI(i);
            char label[MAX_WORD_SIZE];
            if (spelling & ABSTRACT_COVERAGE)
            {
                if (true || block->topicRestriction)
                {
                    sprintf(label, (char*)"T-%s-%s", computerID, GetTopicName(i));
                    WORDP D = FindWord(label, AS_IS);
                    if (!D) continue; // not used in coverage
                }
                if (block->topicRestriction && !strstr(block->topicRestriction, computerIDwSpace)) continue;
            }

			if (len && strnicmp(GetTopicName(i),input,len)) continue;
			DisplayTopic(GetTopicName(i),i,spelling);
		}
		DisplayTables((char*)"*");
	}
	FreeBuffer();
	if (lineLimit) Log(STDUSERLOG,(char*)"%d lines were over length %d\r\n",longLines,lineLimit);
}

static void C_Diff(char* input)
{
	char file1[MAX_WORD_SIZE];
	char file2[MAX_WORD_SIZE];
	input = ReadCompiledWord(input,file1);
	input = ReadCompiledWord(input,file2);
	input = SkipWhitespace(input);
	char separator = *input;
	FILE* in1 = FopenReadWritten(file1);
	if (!in1) 
	{
		Log(STDUSERLOG,(char*)"%s does not exist\r\n",file1);
		return;
	}
	FILE* in2 = FopenReadWritten(file2);
	if (!in2) 
	{
		Log(STDUSERLOG,(char*)"%s does not exist\r\n",file2);
		FClose(in1);
		return;
	}
	char name[MAX_WORD_SIZE];
	sprintf(name,(char*)"%s/diff.txt",logs);
	FILE* out = FopenUTF8Write(name);
	char* buf1 = AllocateBuffer();
	char* buf2 = AllocateBuffer();
	unsigned int n = 0;
	unsigned int err1 = 0;
	while (ALWAYS)
	{
		++n;
		if (!fgets(buf1,maxBufferSize,in1)) 
		{
			if (fgets(buf2,maxBufferSize,in2)) 
			{
				Log(STDUSERLOG,(char*)"2nd file has more at line %d: %s\r\n",n,buf2);
				fprintf(out,(char*)"2nd file has more at line %d: %s\r\n",n,buf2);
				++err1;
			}
			break;
		}
		if (!fgets(buf2,maxBufferSize,in2)) 
		{
			++err1;
			Log(STDUSERLOG,(char*)"1st file has more at line %d: %s\r\n",n,buf1);
			fprintf(out,(char*)"1st file has more at line %d: %s\r\n",n,buf1);
			break;
		}
		// dont show the input after this
		char* sep1 = strchr(buf1,separator);
		if (sep1) *sep1 = 0;
		char* sep2 = strchr(buf2,separator);
		if (sep2) *sep2 = 0;

		// clean up white space
		char* start1 = buf1;
		while (*start1 == '\r' || *start1 == '\n' || *start1 == ' ' || *start1 == '\t') ++start1; // legal
		size_t len1 = strlen(start1);
		while (len1 && (start1[len1-1] == '\r' || start1[len1-1] == '\n' || start1[len1-1] == ' ' || start1[len1-1] == '\t')) --len1; // legal

		char* start2 = buf2;
		while (*start2 == '\r' || *start2 == '\n' || *start2 == ' ' || *start2 == '\t') ++start2; // legal
		size_t len2 = strlen(start2);
		while (len2 && (start2[len2-1] == '\r' || start2[len2-1] == '\n' || start2[len2-1] == ' ' || start2[len2-1] == '\t')) --len2; // legal
	
		if (len1 != len2 || strncmp(start1,start2,len1)) 
		{
			if (sep1) *sep1 = ':';
			if (sep2) *sep2 = ':';
			Log(STDUSERLOG,(char*)"%5d<<    %s\r\n",n,start1);
			Log(STDUSERLOG,(char*)"     >>    %s\r\n",start2);
			fprintf(out,(char*)"%5d<<    %s\r\n",n,start1);
			fprintf(out,(char*)"     >>    %s\r\n",start2);
		++err1;
		}
	}
	FreeBuffer();
	FreeBuffer();
	FClose(in2);
	FClose(in1);
	fprintf(out,(char*)"For %s vs %s -  %d lines differ.\r\n",file1,file2,err1);
	Log(STDUSERLOG,(char*)"For %s vs %s - %d lines differ.\r\n",file1,file2,err1);
	FClose(out);
}

static void IndentDisplay(char* one, char* two,char* display)
{
	char* original = display;
	bool start = true;
	size_t len = strlen(one);
	while (len)
	{
		if (!start) 
		{
			strcpy(display,(char*)"\r\n");
			display += 2;
		}
		else start = false;
		if (len < 120) 
		{
			strcpy(display,one);
			break;
		}
		unsigned int i = 120;
		while (one[i] && one[i] != ' ') ++i;
		if (one[i]) ++i;
		memmove(display,one,i);
		len -= i;
		display += i;
		*display = 0;
		one += i;
	}

	display += strlen(display);
	len = strlen(two);
	while (len)
	{
		if (len == 1 && two[0] == ' ') break;	 // dont bother
		strcpy(display,(char*)"\r\n    ");
		display += 6;
		if (len < 120) 
		{
			strcpy(display,two);
			break;
		}
		unsigned int i = 120;
		while (two[i] && two[i] != ' ') ++i;
		if (two[i]) ++i;
		memmove(display,two,i);
		len -= i;
		display += i;
		*display = 0;
		two += i;
	}
	strcat(display,(char*)"\r\n\r\n");
}

static void C_StripLog(char* file)
{
	char name[MAX_WORD_SIZE];
	bool all = true;
	if (!strnicmp(file, "oneuser",7))
	{
		all = false;
		file = ReadCompiledWord(file, name);
	}
	sprintf(name, "%s/tmp.txt", tmp);
	FILE* in = fopen(file, "rb");
	if (!in)
	{
		(*printer)("file not found");
		return;
	}
	FILE* out = FopenUTF8Write(name);

	char word[MAX_WORD_SIZE];
	char user[MAX_WORD_SIZE];
	while (ReadALine(readBuffer, in) >= 0)
	{
		char* ptr = SkipWhitespace(readBuffer);
		if (!*ptr) continue;
		char* at = strstr(ptr, "ServerPre:") ;  // ServerPre: pid: 2820 20300655 (pearl) msg Jul 26 19 : 13 : 14 2017
		if (!at) continue;
		at = ReadCompiledWord(at + 11, word); // pid label
		at = ReadCompiledWord(at, word); // pid 
		at = ReadCompiledWord(at, user); 
		at = strchr(at, '(');
		char* end = strchr(at, ')'); // bot
		size_t len = strlen(end + 2) - 21; // skip ) and space and account for space and date
		*end = 0; // terminate botname
		end[len+2] = 0; // trim off space and date
		if (!all) fprintf(out, "%s\r\n", end + 2);
		else if (len) fprintf(out, "%s %s %s\r\n", user, at+1, end+2);
		else fprintf(out, "%s %s\r\n", user, at + 1);
	}
	fclose(in);
	fclose(out);
	(*printer)("done\r\n");
}

static void C_Medtable(char* file)
{
    FILE* in = fopen("t.txt", "rb");
    if (!in)
    {
        (*printer)("file not found");
        return;
    }
    char name[MAX_WORD_SIZE];
    sprintf(name, "%s/tmp.txt", tmp);
    FILE* out = FopenUTF8Write(name);
    // format is  phrase / phrase
    char word[MAX_WORD_SIZE];
    char composite[MAX_WORD_SIZE];
    char answer[MAX_WORD_SIZE];
    while (ReadALine(readBuffer, in) >= 0)
    {
        *composite = 0;
        char* ptr = SkipWhitespace(readBuffer);
        while (*ptr)
        {
            if (!*ptr) continue;
            ptr = ReadCompiledWord(ptr, word);
            if (!*composite) strcpy(composite, word);
            else if (*word == '/') // separator
            {
                if (strchr(composite, ' ') || strchr(composite, '\'')) sprintf(answer, "\"%s\"", composite);
                else strcpy(answer, composite);
                fprintf(out, "%s\r\n", answer);
                *composite = 0;
            }
            else
            {
                strcat(composite, " ");
                strcat(composite, word);
            }
        }
         if (*composite)
         {
            if (strchr(composite, ' ') || strchr(composite, '\'')) sprintf(answer, "\"%s\"",composite);
            else strcpy(answer, composite);
            fprintf(out, "%s\r\n", answer);
         }
     }
    fclose(in);
    fclose(out);
    (*printer)("done\r\n");
}

static void C_RewriteConverse(char* file) // single line test inputs
{
    char name[MAX_WORD_SIZE];
    char* output = AllocateBuffer();
    sprintf(name, "%s/tmp.txt", tmp);
    FILE* in = fopen(file, "rb");
    if (!in)
    {
        (*printer)("file not found");
        return;
    }
    FILE* out = FopenUTF8Write(name);
    char user[100];
    *user = 0;
    while (fgets(readBuffer, 10000, in) != NULL)
    {// VAChat_SKEY	VAMessage_SKEY	RowInsertTime	CategoryName	SubCategoryName	LocationName	Message	BotResponse	chatQuestionsPosted

         char* chaid = strchr(readBuffer, '\t'); 
        *chaid = 0; // user id at readbuffer
        char* cat = strchr(chaid + 1, '\t');
        cat = chaid + 1;
        char* spec = strchr(cat + 1, '\t');
        *spec++ = 0;
        char* loc = strchr(spec, '\t');
        *loc++ = 0;
        char* input = strchr(loc, '\t');
        *input++ = 0;
        char* endinput = strchr(input + 1, '\r');
        *endinput = 0;

        if (stricmp(user, readBuffer)) // change of user
        {
            if (!stricmp(loc, "Null"))  sprintf(output, "%s\t\t[ category: %s specialty: %s ]\r\n", readBuffer, cat, spec);
            else sprintf(output, "%s\t\t[ category: %s specialty: %s location: %s ]\r\n", readBuffer, cat, spec, loc);
            fprintf(out, "%s", output);
            strcpy(user,readBuffer);
        }
        sprintf(output, "%s\t\t%s\r\n", readBuffer, input);
        fprintf(out, "%s", output);
    }
    fclose(in);
    fclose(out);
    FreeBuffer();
    (*printer)("done\r\n");
}

static void C_RewriteToTsv(char* file) // single line to tsv format
{
    int uid = 0;
    char* output = AllocateBuffer();
    char name[MAX_WORD_SIZE];
    sprintf(name, "%s.top", file);
    FILE* in = fopen(name, "rb");
    if (!in)
    {
        (*printer)("file not found");
        return;
    }

    sprintf(name, "%s/%s.tsv", tmp,file);
    FILE* out = FopenUTF8Write(name);
    char word[MAX_WORD_SIZE];
    while (fgets(readBuffer, 10000, in) != NULL)
    {
        char* ptr = readBuffer;
        size_t len = strlen(ptr);
        if (ptr[len - 1] == '\n') ptr[--len] = 0;
        if (ptr[len - 1] == '\r') ptr[--len] = 0;
        char* outptr = output;
        *outptr = 0;
        while (*ptr)
        {
            ptr = SkipWhitespace(ptr);
            if (!*ptr) break;
            char* start = ptr;
            if (*ptr == '"')  // swallowed quoted thingy
            {
                while (*++ptr)
                {
                    if (*ptr == '\\') ++ptr; // swallow escaped thingy
                    else if (*ptr == '"')
                    {
                        ++ptr;
                        break;
                    }
                }
                strncpy(word, start, ptr - start);
                word[ptr - start] = 0;
            }
            else if (*ptr == '(')
            {
                strcpy(word, "(");
                ++ptr;
            }
            else if (*ptr == ')')
            {
                strcpy(word, ")");
                ++ptr;
            }
            else if (*ptr == '#')
            {
                strcpy(word, ptr);
                *ptr = 0;
            }
            else
            {
                while (!IsWhiteSpace(*++ptr) && *ptr != '(' && *ptr != ')');
                strncpy(word, start, ptr - start);
                word[ptr - start ] = 0;
            }
   

            strcpy(outptr, word);
            strcat(outptr, "\t");
            outptr += strlen(outptr);
        }
        *--outptr = 0; // remove trailing tab
        fprintf(out, "%s\r\n",output);
    }
    fclose(in);
    fclose(out);
    FreeBuffer();
    (*printer)("done\r\n");
}

static void C_NewDiff(char* input)
{
	char file1[MAX_WORD_SIZE];
	char file2[MAX_WORD_SIZE];
	input = ReadCompiledWord(input, file1);
	ReadCompiledWord(input, file2);
	FILE* in1 = fopen(file1, "rb");
	if (!in1)
	{
		(*printer)("file not found");
		return;
	}
	FILE* in2 = fopen(file2, "rb");
	if (!in2)
	{
		(*printer)("file not found");
		return;
	}
	char name[MAX_WORD_SIZE];
	sprintf(name, "%s/matched.txt", tmp);
	FILE* match = FopenUTF8Write(name);
	sprintf(name, "%s/unmatched.txt", tmp);
	FILE* unmatch = FopenUTF8Write(name);

	char* buf1 = AllocateBuffer();
	char* buf2 = AllocateBuffer();
	char prior1[MAX_WORD_SIZE];
	char prior2[MAX_WORD_SIZE];
	while (1)
	{
		char user1[MAX_WORD_SIZE];
		char user2[MAX_WORD_SIZE];
		if (fgets(buf1, 10000000, in1) == NULL) break;
		if (fgets(buf2, 10000000, in2) == NULL) break;
		ReadCompiledWord(buf1, user1);
		ReadCompiledWord(buf2, user2);
		if (stricmp(user1, user2)) // users differ
		{
			if (stricmp(user1, prior1)) // went to new user, make user2 catch up
			{
				while (ReadALine(buf2, in2) < 0)
				{
					ReadCompiledWord(buf2, user2);
					if (!stricmp(user2, user1)) break;
				}
			}
			else if (stricmp(user2, prior2)) // went to new user, make user1 catch up
			{
				ReadCompiledWord(buf1, user1);
				if (!stricmp(user2, user1)) break;
			}
		}
		strcpy(prior1, user1);
		strcpy(prior2, user2);
		TrimSpaces(buf1);
		TrimSpaces(buf2);

		// check for rule match...
		char* t = strchr(buf1, '\t'); // 1st rule ptrof
		if (!t) 
			continue;
		t = strchr(t+1, '\t'); // 2nd rule ptr
		t = strchr(t+1, '\t'); // 3rd rule ptr
		*t = 0;
		char* hold = t;
		t = strchr(buf2, '\t'); // 1st rule ptr
		if (!t) 
			continue;
		t = strchr(t+1, '\t'); // 2nd rule ptr
		t = strchr(t+1, '\t'); // 3rd rule ptr
		*t = 0;
		if (strcmp(buf1, buf2) && !strstr(buf2,"CONFUSION_REJOINDER"))
		{
			*hold = '\t';
			*t = '\t';
			fprintf(unmatch, "%s\r\n%s\r\n\r\n", buf1, buf2);
		}
		else
		{
			*hold = '\t';
			*t = '\t';
			fprintf(match, "%s  |  %s\r\n", buf1, buf2);
		}
	}
	FreeBuffer();
	FreeBuffer();
	fclose(in1);
	fclose(in2);
	fclose(match);
	fclose(unmatch);
}

static void C_MergeLines(char* file)
{
   FILE* in = fopen(file, "rb");
    if (!in)
    {
        (*printer)("file not found");
        return;
    }
	char name[MAX_WORD_SIZE];
	sprintf(name, "%s/tmp.txt", tmp);
	FILE* out = FopenUTF8Write(name);
    char word[MAX_WORD_SIZE];
    char priorword[MAX_WORD_SIZE];
    *priorword = 0;
    int count = 0;
    while (ReadALine(readBuffer, in) >= 0)
    {
        char* ptr = SkipWhitespace(readBuffer);
        if (!*ptr) continue;
        ReadCompiledWord(ptr, word); // get word header
        if (!*word) continue;
        if (!*priorword)
        {
            strcpy(priorword, word);
            count = 1;
        }
        else if (stricmp(word, priorword)) // changing word
        {
            fprintf(out, "%06d   %s\r\n", count, priorword);
            strcpy(priorword, word);
            count = 1;
        }
        else ++count;
    }
    fclose(in);
    fclose(out);
    (*printer)("done\r\n");
}

static void C_QuoteLines(char* file)
{
	char name[MAX_WORD_SIZE];
	sprintf(name, "%s/tmp.txt", tmp);
	FILE* in = fopen(file, "rb");
	if (!in)
	{
		(*printer)("file not found");
		return;
	}
	FILE* out = FopenUTF8Write(name);
	char word[MAX_WORD_SIZE];
	word[0] = '"';
    while (ReadALine(readBuffer, in) >= 0)
    {
        char* ptr = SkipWhitespace(readBuffer);
        if (!*ptr) continue;
		strcpy(word+1, ptr);
		size_t len = strlen(word);
		if (word[len - 1] == ' ') --len;
		word[len] = '"';
		word[len+1] = 0;
		fprintf(out, "%s\r\n", word);
	}
	fclose(in);
	fclose(out);
	(*printer)("done\r\n");
}

static void BuildForeign(char* input)
{
	char word[MAX_WORD_SIZE];
	char lang[MAX_WORD_SIZE];
	input = ReadCompiledWord(input,lang);
	MakeUpperCase(lang);
	strcpy(language, lang);

	UseDictionaryFile(NULL);
	InitFacts();
	setMaxHashBuckets = true;
	InitDictionary();
	setMaxHashBuckets = false;
	InitStackHeap();
	InitCache();

	InitFactWords();
	AcquireDefines((char*)"SRC/dictionarySystem.h"); //   get dictionary defines (must occur before loop that decodes properties into sets (below)

	char name[MAX_WORD_SIZE];
	sprintf(name, "DICT/%s", language);
	MakeDirectory(name);
	ClearDictionaryFiles();
	ClearWordMaps(); // build foreign
	ReadForeign();

	sprintf(name,"treetagger/%s_rawwords.txt",language);
	FILE* in = fopen(name,"rb");
	if (!in)
	{
		(*printer)("file not found");
		return;
	}

	// format is  word  `lemma` POSes  -- can be multiple lemmas like `lemma`lemma`` one data for each part of speech, and lemmas can be |  joined also.
	while (ReadALine(readBuffer,in) >= 0)
	{
		char* ptr = ReadTokenMass(readBuffer, word);
		if (!*word) continue;
		if ((unsigned char) word[0] ==  0xEF && (unsigned char) word[1] == 0xBB && (unsigned char) word[2] == 0xBF) continue; // UTF8 BOM
		// word might be a phrase
		char* startlemma = strchr(ptr, '`'); // find start of lemma data
		if (!startlemma) continue;

		char* base = SkipWhitespace(readBuffer);
		*(startlemma - 1) = 0;
		strcpy(word, base);
		*(startlemma - 1) = ' ';

		char* close = strrchr(startlemma+1, '`');
		if (!close) continue; // no lemma?
		*(close) = 0; // ends in ``
		if (!strcmp(startlemma+1, word)) *startlemma = 0; // cancel  as it is the same as its sole lemma
		*close = '`';

		char pos[MAX_WORD_SIZE];
		uint64 flags = AS_IS;
		pos[0] = '_';
		char* p = close + 1;
		while (1)
		{
			p = ReadCompiledWord(p, pos+1);
			if (!pos[1]) break;
			WORDP D = FindWord(pos);
			if (D && D->properties) 
				flags |= D->properties;
		}
		WORDP D = StoreWord(word, flags);
		if (GetCanonical(D)) (*printer)("Already have canonical for %s\r\n", D->word);
		else if (*startlemma) SetCanonical(D, MakeMeaning(StoreWord(startlemma,AS_IS)));
	}
	fclose(in);
	WalkDictionary(WriteDictionary);
	myexit("end foreign build");
}

static void Stats(char* bot,char* why)
{
    if (stricmp(bot, computerID)) // only do this bot
        return;
    ++relevantVolleyCounter;
    while (*why == '~') // do each tag
    {
        char tag[MAX_WORD_SIZE];
        why = ReadCompiledWord(why, tag); // get tag  which is topic.x.y=name or topic.x.y<topic.a.b (reuse) and optional label which is whytag
        char* dot;
        dot = strchr(tag, '.'); // split of primary topic from rest of tag
        int topicidx = 0;
        *dot = 0;
        strcpy(topic, tag); // get the primary topic of the tag
        *dot = '.';
        char label[MAX_WORD_SIZE];
        sprintf(label, (char*)"T-%s-%s",bot,topic);
        WORDP D = StoreWord(label, AS_IS);
        statistics[D->word] = statistics[D->word] + 1; // tally hits on topic itself
        dot = strchr(dot + 1, '.'); // 2nd dot
        while (IsDigit(*++dot)) { ; }
        char c = *dot;
        *dot = 0;
        sprintf(label, (char*)"R-%s-%s",bot, tag);
        D = StoreWord(label, AS_IS);
        statistics[D->word] = statistics[D->word] + 1; // tally hits on topic itself
        *dot = c;

        char* rest = strchr(dot, '~'); // 2ndary rule?
        if (rest) // look at indirection rule
        {
            char* dot1 = strchr(rest, '.');
            dot1 = strchr(dot1 + 1, '.'); // 2nd dot
            while (IsDigit(*++dot1)) { ; }
            char c1 = *dot1;
            *dot1 = 0;
            sprintf(label, (char*)"R-%s-%s",bot, rest);
            D = StoreWord(label, AS_IS);
            statistics[D->word] = statistics[D->word] + 1; // tally hits on topic itself
            *dot1 = c1;
        }
    }
}

static void TrimIt(char* name,uint64 flag) 
{
	//  0 = user->bot
	//  1 = bot->user
	//  2 = topic user-bot
	//  3 = topic bot->user last sentence
	//  4 = indent human
	//  5 = indent bot
	//  6 = user only
	//  7 = tags verify user-bot
	//  8 = topic indent bot
	//  9 = generate user log files from system log
	// 10 = generate statistics from logs
	// 11 = timeline of user->bot
	// 12 = rule user -> bot

	FILE* in = FopenReadWritten(name);
	if (!in) return;
	char* format = ((filesSeen % 100000) == 0) ? (char*)"+\r\n" : (char*) ".";
	if ((++filesSeen % 1000) == 0) (*printer)("%s",format); // mark progress thru files
    char* prior = AllocateBuffer();

	bool header = false;
	char fname[200];
	if (keepname)
	{
		char* end = strrchr(name, '/');
		if (end) name = end + 1;
		sprintf(fname, "%s/%s.txt", tmp,name);
	}
	else sprintf(fname, "%s/tmp.txt", tmp);
	FILE* out = FopenUTF8WriteAppend(fname);
	if (!out) return;
    char* display = AllocateBuffer();
    char file[SMALL_WORD_SIZE];
	*file = 0;
	*prior = 0;
	char* at;
    char* copy = AllocateBuffer();

	while (ReadALine(readBuffer,in) >= 0 ) 
	{
		size_t len = strlen(readBuffer);
   		if (!len) continue;
		strcpy(copy,readBuffer);

        // fields in log file are: type, user, bot, ip, rand, resulting topic, (current volley id),  input,  output, dateinfo (when), possible f:,  followed by rule tags for each issued output.
        //Start: user:dat bot : pearl ip : rand:2366 (~introductions) 0 == >[why = ~introductions.1.0.~control.6.0 | v = 36.0]`Hello, I'm Pearl Wilson, the Expert's Assistant on JustAnswer.How can I help ? When : Sep14'18-12:02:49 Why:~xpostprocess.4.0=OOBRESULT.~control.4.0 ~introductions.1.0.~control.6.0  Version:8.5 Build0:Sep10'18 - 07 : 32 : 40 Build1 : Sep13'18-13:55:30 0:Sep14'18 - 12 : 02 : 49 F : 0 P : 0
        //Respond : user : dat bot : pearl ip : () 1   test me == >[alarm = 0 | loopback = 0 | why = ~phase2.8.0 = MAINCONTROL.~phase2.7.0 | v = 36.0]`What can I say ? When : Sep14'18-12:04:48 Why:~xpostprocess.4.0=OOBRESULT.~control.10.3 ~phase2.8.0=MAINCONTROL.~phase2.7.0   F:4932 

		// fields in regress file are: Start: user:test bot:rose rand:3089 (resulting topic), volleyid,  input:  output: Good morning. 
		// Respond: user:test bot:rose (resulting topic), volleyid input output 

		if (strncmp(readBuffer,(char*)"Respond:",8) && strncmp(readBuffer,(char*)"Start:",6)) continue; // not a normal line?

		// normal volley

		char user[MAX_WORD_SIZE];
		char* ptr = strstr(readBuffer,(char*)"user:") + 5;
		if (*ptr == ' ') strcpy(user,"anonymous");
		else ptr = ReadCompiledWord(ptr,user);

		char bot[MAX_WORD_SIZE];
        if (ptr[4] == ' ')
        {
            strcpy(bot, computerID); // default bot
        }
		else ptr = ReadCompiledWord(ptr+4,bot); // skip "bot:"

		char ip[MAX_WORD_SIZE];
		*ip = 0;
		at = strchr(ptr,'(');
		if (!at) continue;
		*at++ = 0;
		char* end = strchr(at,')'); 
		if (!end) continue;
		*end = 0; // terminate around topic removing )
		char topic[MAX_WORD_SIZE];
		ReadCompiledWord(at,topic);	
	
		char* ipp = strstr(ptr,(char*)"ip:");
		if (ipp) ptr = ReadCompiledWord(ipp+3,ipp);

		char volley[MAX_WORD_SIZE];
		at = ReadCompiledWord(end+1,volley); 
		char* input = SkipWhitespace(at); // now points to user input start
		char* output = strstr(at,(char*)"==> ");
		if (!output) continue;
        ++volleyCounter;
		*output++ = 0;	// end of input
		*output = 0;	// end of input just in case of null input
		output += 3;
		output = SkipWhitespace(output);  // start of output

		char* when = strstr(output,(char*)"When:");
		char* why = "";
		if (when) 
		{
			*when = 0;	// end of output
			when += 5; // start of datestamp

			why = strstr(when,(char*)"Why:");
			if (why) // why may not exist (like postprocess output and gesture output)
			{
				*why = 0;	// end of datestamp
				why += 4;  // beginnings of whys
			}
			else why = "";
		}

		// remove our internal reply markers
		char* x = output;
		while ((x = strchr(x,ENDUNIT))) *x++ = ' ';

		// now show the data
		display[0] = 0;

		// trim any oob
		if (nooob & 1) input = SkipOOB(input);
		if (nooob & 2) output = SkipOOB(output);

        if (!strncmp(readBuffer, (char*)"Start:", 6))
        {
            if (flag == 9)
            {
                char file[SMALL_WORD_SIZE];
                sprintf(file, (char*)"%s/log-%s.txt", users, user);
                FILE* out1 = FopenUTF8WriteAppend(file);
                fprintf(out1, (char*)"%s\r\n", copy);
                FClose(out1);
            }
            // other things you could do with start line here
            if (flag == 10)  Stats(bot,why); 
            continue;
        }

		if (flag == 0) sprintf(display,(char*)"\r\n%s   =>   %s\r\n",input,output); //  showing both as pair, user first
		else if (flag == 1)  sprintf(display,(char*)"\r\n%s   =>   %s\r\n",prior,input);  // pair, bot first
		else if ( flag == 2) sprintf(display,(char*)"\r\n%s %s   =>   %s\r\n",topic,input,output); //  showing both as pair, user first, with topic of response
		else if ( flag == 3) sprintf(display,(char*)"%s %s   =>   %s\r\n",topic,prior,input); // topic bot user
 		else if ( flag == 4) IndentDisplay(output,input,display);
		else if ( flag == 5) IndentDisplay(input,output,display);
 		else if ( flag == 6) sprintf(display,(char*)"%s\r\n",input); // user input only
		else if ( flag == 7) // figure out matching verify
		{
			char tag[MAX_WORD_SIZE];
			char* whyTag = why;
			bool start = true;
			char* atOutput = output;
			if (*whyTag != '~') // had no main motivation
			{
				fprintf(out,(char*)"?-?   %s => %s\r\n",input,atOutput); //  showing both as pair, user first, with tag of response and verify input reference
			}
			while (*whyTag == '~') // do each tag
			{
				whyTag = ReadCompiledWord(whyTag,tag); // get tag  which is topic.x.y=name or topic.x.y<topic.a.b (reuse) and optional label which is whytag
				char* separation = strchr(atOutput,'|');
				char* rule;
				if (separation) *separation = 0; // block out rest of output for a moment
				char* dot;
				dot = strchr(tag,'.'); // split of primary topic from rest of tag
				int topicidx = 0;
				*dot = 0;
				strcpy(topic,tag); // get the primary topic of the tag
				topicidx = FindTopicIDByName(topic,true);
				*dot = '.';
				int id;
				char* rest = GetRuleIDFromText(dot,id); // will point to null, or label, or tag topic
				char verify[MAX_WORD_SIZE];
				strcpy(verify,GetVerify(tag,topicidx, id));
				
				// impacts of indirection
				char* reuseRule = NULL;
				rest = strchr(rest,'~'); // topic start
				if (rest) // look at indirection rule
				{
					dot = strchr(rest,'.');
					*dot = 0;
					int reusetopicid = FindTopicIDByName(rest);
					*dot = '.';
					int reuseid = -1;
					GetRuleIDFromText(dot,reuseid);
					bool updatedVerify = false;
					if (!*verify) 
					{
						strcpy(verify,GetVerify(rest,reusetopicid, reuseid));
						updatedVerify = true;
					}
					reuseRule = GetRule(reusetopicid,reuseid); // the reused rule
					if (!reuseRule || !strstr(reuseRule,(char*)"^reuse")) 
					{
						if (updatedVerify) *verify = 0;  // CANCEL not a reuse - have NO verify at all
						reuseRule = 0;
					}
	//				else rule = GetRule(topicidx,id);	// THIS RULE GETS SHOWN
				}

				rule =  (reuseRule) ?  reuseRule : GetRule(topicidx,id); // show the rule whose pattern matched
				char pattern[MAX_WORD_SIZE];
				*pattern = 0;
				if (rule) GetPattern(rule,NULL,pattern,true);
				else rule = "-";
				if (!*pattern) strcpy(pattern,(char*)"()"); // gambits for example
				if (start) start = false;
				else fprintf(out,(char*)"%s",(char*)"    ");
				fprintf(out,(char*)"%s|\"%s\"|%c: %s|%s|%s\r\n",tag,verify,*rule,pattern,input,atOutput); //  showing both as pair, user first, with tag of response and verify input reference
				if (separation) atOutput = separation + 1; // next output
			}
			*display = 0;
		}
		else if ( flag == 8) sprintf(display,(char*)"%s\r\n\t(%s) %s\r\n",input,topic,output); // 2liner, indented computer   + topic
		else if ( flag == 9) // build user logs
		{
			char file1[SMALL_WORD_SIZE];
			sprintf(file1,(char*)"%s/log-%s.txt",users,user);
			FILE* out1 = FopenUTF8WriteAppend(file1);
			fprintf(out1,(char*)"%s\r\n",copy);
			FClose(out1);
			continue;
		}
		else if (flag == 10) // build stats
		{
            Stats(bot,why);
		}
		else if (flag == 11)  sprintf(display, (char*)"\r\n%s  %s\r\n\t%s\r\n", when, input, output); //  timeline, user first
		else if (flag == 12 || flag == 13) // 12 skips initial oob reason  13 doesnt
		{
			char tag[MAX_WORD_SIZE];
			char* whyTag = why;
			bool start = true;
			char* atOutput = output;
			if (*whyTag != '~') { ; }
			whyTag = ReadCompiledWord(whyTag, tag); // get tag  which is topic.x.y=name or topic.x.y<topic.a.b (reuse) and optional label which is whytag
			
			if (flag == 13) whyTag = tag;
			char* separation = strchr(whyTag+1, '~');
			if (separation) *separation = 0; // block out rest of output for a moment
			int topicidx = 0;
			int id;
			char* rule = GetRuleTag(topicidx, id, whyTag);
			char label[MAX_WORD_SIZE];
			*label = 0;
			GetLabel(rule, label);

			if (*output == '[') // skip OOB data
			{
				int count = 1;
				while (*++output)
				{
					if (*output == '[') ++count;
					else if (*output == ']')
					{
						--count;
						if (!count)
						{
							++output;
							break;
						}
					}
				}
			}

			sprintf(display, (char*)"%s %s => %s\r\n", label,input, output); //  rule then showing both as pair, user first
		}
		if (*display) 
		{
			if (!header) 
			{
				header = true;
				char* type = " ";
				if ( flag == 0) type = "user->bot";
				else if (flag == 1) type = "bot->user";
				else if ( flag == 2) type = "topic user->bot";
				else if ( flag == 3) type = "topic bot->user";
				else if ( flag == 4) type = "indent bot";
				else if ( flag == 5) type = "indent human";
				else if ( flag == 6) type = "user only";
				else if ( flag == 7) type = "tags verify user->bot";
				else if ( flag == 8) type = "indent bot + topic";
				else if ( flag == 11) type = "timeline";
				char* last = strrchr(name,'/');
				if (last) name = last;
				fprintf(out,(char*)"\r\n# ----------------- %s   %s\r\n",name,type);
			}
			fprintf(out,(char*)"%s",display);
		}

		strcpy(prior,output); // what bot said previously
	}
    FClose(in);
    FClose(out);
    FreeBuffer();
    FreeBuffer();
    FreeBuffer();
    Log(STDUSERLOG,(char*)"Trim %s complete\r\n",name);
}

static void C_Trim(char* input) // create simple file of user chat from directory
{   
 	char word[MAX_WORD_SIZE];
	char file[SMALL_WORD_SIZE];
	keepname = false;
	char* original = input;
	*file = 0;
	input = ReadCompiledWord(input,word);
	if (*word == '"') // multiparam
	{
		if (strstr(word, "keepname")) keepname = true;
		input = ReadCompiledWord(input, word);
	}
	filesSeen = 0;
	if (!strnicmp((char*)"log-",word,4)) // is just a user file name
	{
		*directory = 0;	
		if (strstr(word,(char*)"txt")) sprintf(file,(char*)"%s/%s",users,word);
		else sprintf(file,(char*)"%s/%s.txt",users,word);
		input = ReadCompiledWord(input,word);
	}
	else if (strstr(word,(char*)".txt")) // is a full file name
	{
		*directory = 0;	
		strcpy(file,word);
		input = ReadCompiledWord(input,word);
	}
	else if (IsAlphaUTF8(*word)) // directory name or simple user name
	{
		*directory = 0;	
		sprintf(file,(char*)"%s/log-%s.txt",users,word);
		FILE* x = FopenReadWritten(file);
		if (x) FClose(x); // see if file exists. if not, then its a directory name
		else 
		{
			strcpy(directory,word);
			*file = 0;
		}
		input = ReadCompiledWord(input,word);
	}
	else strcpy(directory,logs);

	unsigned int flag = 0;
	if (!stricmp(word,(char*)"bot2user")) flag = 1;
	else if (!stricmp(word,(char*)"useronly") || !stricmp(word,(char*)"humanonly")) flag = 6;
	else if (!stricmp(word,(char*)"indenthuman")) flag = 4;
	else if (!stricmp(word,(char*)"indentbot")) flag = 5;
	else if (!stricmp(word,(char*)"usersfromsystem")) flag = 9;
	else if (!stricmp(word,(char*)"statistics")) flag = 10;
	else if (!stricmp(word,(char*)"timeline")) flag = 11;
	else flag = atoi(word);

	input = ReadCompiledWord(input,word);
	if (!stricmp(word, "nooob")) nooob = 2;  // original was output only
	else nooob = atoi(word);

	char fname[200];
	sprintf(fname, "%s/tmp.txt", tmp);
	FILE* out = FopenUTF8Write(fname);
	fprintf(out,(char*)"# %s\r\n",original);
	Log(STDUSERLOG,(char*)"# %s\r\n",input);
	FClose(out);

	if (!*file) WalkDirectory(directory,TrimIt,flag,false);
	else TrimIt(file,flag);
	(*printer)("%s",(char*)"\r\n");
}

CommandInfo commandSet[] = // NEW
{//  actual command names must be lower case 
	{ (char*)"",0,(char*)""},
	
	{ (char*)"\r\n---- Debugging commands",0,(char*)""}, 
	{ (char*)":do",C_Do,(char*)"Execute the arguments as an output stream, e.g., invoke a function, set variables, etc"},  
	{ (char*)":silent",C_Silent,(char*)"toggle silent - dont show outputs"}, 
	{ (char*)":log",C_Log,(char*)"dump message into log file"}, 
	{ (char*)":noreact",C_NoReact,(char*)"Disable replying to input"}, 
	{ (char*)":notime",C_NoTime,(char*)"Toggle notiming during this topic"},
	{ (char*)":notrace",C_NoTrace,(char*)"Toggle notracing during this topic" },
	{ (char*)":redo",C_Redo,(char*)"Back up to turn n and try replacement user input"},
	{ (char*)":retry",C_Retry,(char*)"Back up and try replacement user input or just redo last sentence"}, 
	{ (char*)":say",C_Say,(char*)"Make chatbot say this line"}, 
	{ (char*)":skip",C_Skip,(char*)"Erase next n gambits"}, 
	{ (char*)":show",C_Show,(char*)"All, Input, Mark, Number, Pos, Stats, Topic, Topics, Why, Reject, Newlines"},
	{ (char*)":time",C_Time,(char*)"Set timing variable (all none prepare match ruleflow pattern query json macro user usercache sql tcp topic)" },
	{ (char*)":trace",C_Trace,(char*)"Set trace variable (all none basic prepare match output pattern infer query substitute hierarchy fact control topic pos)"},
	{ (char*)":why",C_Why,(char*)"Show rules causing most recent output"}, 
	{ (char*)":authorize",C_Authorize,(char*)"Flip authorization for all debug commands"},

	{ (char*)"\r\n---- Fact info",0,(char*)""}, 
	{ (char*)":allfacts",C_AllFacts,(char*)"Write all facts to TMP/facts.tmp"}, 
	{ (char*)":facts",C_Facts,(char*)"Display all facts with given word or meaning or fact set"}, 
	{ (char*)":userfacts",C_UserFacts,(char*)"Display current user facts"}, 

	{ (char*)"\r\n---- Topic info",0,(char*)""}, 
	{ (char*)":gambits",C_Gambits,(char*)"Show gambit sequence of topic"}, 
	{ (char*)":pending",C_Pending,(char*)"Show current pending topics list"}, 
	{ (char*)":topicstats",C_TopicStats,(char*)"Show stats on all topics or named topic or NORMAL for non-system topics"},
	{ (char*)":topicinfo",C_TopicInfo,(char*)"Show information on a topic"}, 
	{ (char*)":topics",C_Topics,(char*)"Show topics that given input resonates with"}, 
	{ (char*)":where",C_Where,(char*)"What file is the given topic in"}, 
	
	{ (char*)"\r\n---- System info",0,(char*)""},  
	{ (char*)":commands",C_Commands,(char*)"Show all :commands"}, 
	{ (char*)":context",C_Context,(char*)"Display current context labels"}, 
	{ (char*)":conceptlist",C_ConceptList,(char*)"Show all concepts- or with argument show concepts starting with argument"}, 
	{ (char*)":definition",C_Definition,(char*)"Show code of macro named"},
	{ (char*)":directories",C_Directories,(char*)"Show current directories"},
	{ (char*)":functions",C_Functions,(char*)"List all defined system ^functions"},
	{ (char*)":identify",C_Identify,(char*)"Give version data on this CS"},
	{ (char*)":macros",C_Macros,(char*)"List all user-defined ^macros and plans"},
	{ (char*)":memstats",C_MemStats,(char*)"Show memory allocations"},
	{ (char*)":list",C_List,(char*)"$ (variables) @ (factsets)  _  (match variables)   ^ (macros)   ~ (topics&concepts)"}, 
	{ (char*)":queries",C_Queries,(char*)"List all defined queries"},
	{ (char*)":timedfunctions",C_TimedFunctions,(char*)"List all user defined macros currently being timed" },
	{ (char*)":timedtopics",C_TimedTopics,(char*)"List all topics currently being timed" },
	{ (char*)":tracedfunctions",C_TracedFunctions,(char*)"List all user defined macros currently being traced"},
	{ (char*)":tracedtopics",C_TracedTopics,(char*)"List all topics currently being traced"},
	{ (char*)":variables",C_Variables,(char*)"Display current bot/user/sysytem/match/all variables"}, 
	{ (char*)":who",C_Who,(char*)"show current login/computer pair"}, 
		
	{ (char*)"\r\n---- Word information",0,(char*)""}, 
	{ (char*)":down",C_Down,(char*)"Show wordnet items inheriting from here or concept members"},  
	{ (char*)":concepts",C_Concepts,(char*)"Show concepts triggered by this word"},  
	{ (char*)":findwords",C_FindWords,(char*)"show words matching pattern of letters and *"},
	{ (char*)":hasflag",C_HasFlag,(char*)"List words of given set having or !having some system flag"}, 
	{ (char*)":nonset",C_Nonset,(char*)"List words of POS type not encompassed by given set"}, 
	{ (char*)":overlap",C_Overlap,(char*)"Direct members of set x that are also in set y somehow"}, 
	{ (char*)":up",C_Up,(char*)"Display concept structure above a word"}, 
	{ (char*)":word",C_Word,(char*)"Display information about given word"}, 
	{ (char*)":mixedcase",C_MixedCase,(char*)"List words occurring in both cases, not part of WordNet"}, 
	{ (char*)":dualupper",C_DualUpper,(char*)"List words occurring in multiple uppercase forms, not part of WordNet" },

	{ (char*)"\r\n---- System Control commands",0,(char*)""}, 
	{ (char*)":build",C_Build,(char*)"Compile a script - filename {nospell,outputspell,reset}"}, 
	{ (char*)":bot",C_Bot,(char*)"Change to this bot"},  
	{ (char*)":clearlog",C_ClearLog,(char*)"Erase user log file" },
	{ (char*)":crash",C_Crash,(char*)"Simulate a server crash"},
	{ (char*)":debug",C_Debug,(char*)"Initiate debugger"}, 
	{ (char*)":flush",C_Flush,(char*)"Flush server cached user data to files"}, 
	{ (char*)":quit",C_Quit,(char*)"Exit ChatScript"}, 
	{ (char*)":reset",ResetUser,(char*)"Start user all over again, flushing his history"}, 
	{ (char*)":restart",C_Restart,(char*)"Restart Chatscript"}, 
	{ (char*)":user",C_User,(char*)"Change to named user, not new conversation"}, 

	{ (char*)"\r\n---- Script Testing",0,(char*)""},  
	{ (char*)":autoreply",C_AutoReply,(char*)"[OK,Why] use one of those as all input."}, 
	{ (char*)":common",C_Common,(char*)"What concepts have the two words in common."},
	{ (char*)":spellit",C_Spellit,(char*)"Show spelling correction traces" },
	{ (char*)":tokenize",C_Tokenize,(char*)"Show results of tokenization" },
	{ (char*)":prepare",C_Prepare,(char*)"Show results of tokenization, tagging, and marking on a sentence"},
	{ (char*)":regress",C_Regress,(char*)"create or test a regression file"}, 
	{ (char*)":source",C_Source,(char*)"Switch input to named file"}, 
	{ (char*)":testpattern",C_TestPattern,(char*)"See if a pattern works with an input."}, 
	{ (char*)":testtopic",C_TestTopic,(char*)"Try named topic responders on input"}, 
	{ (char*)":verify",C_Verify,(char*)"Given test type & topic, test that rules are accessible. Tests: pattern (default), blocking(default), keyword(default), sample, gambit, all."},  

	{ (char*)"\r\n---- Document Processing",0,(char*)""},  
	{ (char*)":document",C_Document,(char*)"Switch input to named file/directory as a document {single, echo}"}, 
	{ (char*)":wikitext",C_WikiText,(char*)"read wiki xml and write plaintext"},

	{ (char*)"\r\n---- Analytics",0,(char*)""}, 
	{ (char*)":abstract",C_Abstract,(char*)"Display overview of ChatScript topics"}, 
	{ (char*)":coverage",C_Coverage,(char*)"Save execution coverage of ChatScript rules"}, 
	{ (char*)":showcoverage",C_ShowCoverage,(char*)"Display execution coverage of ChatScript rules"}, 
	{ (char*)":diff",C_Diff,(char*)"match 2 files and report lines that differ"}, 
	{ (char*)":trim",C_Trim,(char*)"Strip excess off chatlog file to make simple file TMP/tmp.txt"}, 
	
    { (char*)":removecrlf",C_RemoveCRLF,(char*)"echo file  to TMP/tmp.txt with no crlf " },
    { (char*)":ja2source",C_JA2Source,(char*)"JA echo csv  to TMP/tmp.txt reformatting to source " },
    { (char*)":ja2starts",C_JA2Starts,(char*)"JA echo file to TMP/tmp.txt minus short line in column" },
	{ (char*)":trimdown",C_TrimDown,(char*)"echo  to TMP/tmp.txt single word entries from a :down list" },
	{ (char*)":checklist",C_CheckList,(char*)"echo  to TMP/tmp.txt words in file list that can be verbs" },

	{ (char*)"\r\n---- internal support",0,(char*)""}, 
    { (char*)":allmembers",C_AllMembers,(char*)"show all members recursive, excluding when members of named concepts" },
    { (char*)":ingestlog",C_Ingestlog,(char*)"execute a log file as source" },
    { (char*)":dedupe",C_Dedupe,(char*)"echo input file to TMP/tmp.txt without duplicate lines)" },
	{ (char*)":topicdump",C_TopicDump,(char*)"Dump topic data suitable for inclusion as extra topics into TMP/tmp.txt (:extratopic or PerformChatGivenTopic)"},
    { (char*)":listvariables",C_ListVariables,(char*)"List variables into tmp/variables.txt" },
    { (char*)":builddict",BuildDictionary,(char*)" basic, layer0, layer1, foreign, or wordnet are options instead of default full"},
	{ (char*)":buildforeign",BuildForeign,(char*)"regenerate foreign language dictionary given name of language"}, 
	{ (char*)":clean",C_Clean,(char*)"Convert source files to NL instead of CR/LF for unix"},
    { (char*)":medtable",C_Medtable,(char*)"Read lines from file, add quotes around them, write to tmp/tmp.txt" },
    { (char*)":rewriteconverse",C_RewriteConverse,(char*)"Read lines from file, reformat as conversation,  write to tmp/tmp.txt" },
    { (char*)":rewrite2tsv",C_RewriteToTsv,(char*)"Read lines from file, reformat as tsv,  write to same file with tsv sufficx" },
    { (char*)":quotelines",C_QuoteLines,(char*)"Read lines from file, add quotes around them, write to tmp/tmp.txt" },
	{ (char*)":newdiff",C_NewDiff,(char*)"Read lines from paired file, note diff lines into tmp/match.txt and tmp/unmatch.txt" },
	{ (char*)":mergelines",C_MergeLines,(char*)"Read lines from sorted file, rewrite only 1 instence of 1st words to tmp/tmp.txt" },
    { (char*)":striplog",C_StripLog,(char*)"Read lines from a server log file, reducing them to normal inputs for :source, write to tmp/tmp.txt" },
#ifndef DISCARDPOSTGRES
	{ (char*)":endpguser",C_EndPGUser,(char*)"Switch from postgres user topic to file system"},
#endif
	{ (char*)":extratopic",C_ExtraTopic,(char*)"given topic name and file as output from :topicdump, build in core topic and use it thereafter"},
	{ (char*)":pennformat",C_PennFormat,(char*)"rewrite penn tagfile (eg as output from stanford) as one liners"}, 
	{ (char*)":penndecode",C_PennDecode,(char*)"FILE convert penn parse to simple tag format" },
	{ (char*)":pennmatch",C_PennMatch,(char*)"FILE {raw ambig} compare penn file against internal result"},
	{ (char*)":pennnoun",C_PennNoun,(char*)"locate mass nouns in pennbank"}, 
	{ (char*)":pos",C_POS,(char*)"Show results of tokenization and tagging"},  
	{ (char*)":sortconcept",C_SortConcept,(char*)"Prepare concept file alphabetically"}, 
	{ (char*)":translateconcept",C_TranslateConcept,(char*)"take"}, 
	{ (char*)":timepos",C_TimePos,(char*)"compute wps average to prepare inputs"},
	{ (char*)":verifypos",C_VerifyPos,(char*)"Regress pos-tagging using default REGRESS/postest.txt file or named file"},
	{ (char*)":verifyspell",C_VerifySpell,(char*)"Regress spell checker against file"}, 
	{ (char*)":verifysubstitutes",C_VerifySubstitutes,(char*)"Regress test substitutes of all kinds"}, 
	{ (char*)":worddump",C_WordDump,(char*)"show words via hardcoded test"}, 
	{ (char*)":verifySentence",C_VerifySentence,(char*)"verification data"}, 
    { (char*)":timelog",C_TimeLog,(char*)"avg min and max of a log named" },
    { (char*)":countwords",C_CountWords,(char*)"generate counts of words listed in file" },
    { (char*)":countcat",C_CountCat,(char*)"generate counts of words listed in file" },

#ifdef PRIVATE_CODE
#include "../privatecode/privatetestingtable.cpp"
#endif
	{0,0,(char*)""},	
};

void SortTopic(WORDP D,uint64* junk)
{
	if (!(D->internalBits & (BUILD0|BUILD1|BUILD2))) return; // ignore internal system topics (of which there are none)
	if (D->internalBits & TOPIC) Sortit(D->word,(int)(long long)junk); // 1 will be for 1line, otherwise take multiline as needed
}

void SortTopic0(WORDP D,uint64 junk)
{
	if (!(D->internalBits & (BUILD0|BUILD1|BUILD2))) return; // ignore internal system concepts
	if (!(D->internalBits & TOPIC)) return;
	CreateFact(MakeMeaning(D),Mmember,MakeMeaning(StoreWord((char*)"~_dummy",AS_IS)));
}

typedef std::vector<char*>::const_iterator  citer;

static bool myFunction (char* i,char* j) 
{ 
	if (*i == '~' && *j != '~') return true; // all concepts come first
	if (*i != '~' && *j == '~') return false;
	return stricmp(i,j) < 0; 
}

static bool myInverseFunction (char* i,char* j) 
{ 
	if (*i == '~' && *j != '~') return true; // all concepts come first
	if (*i != '~' && *j == '~') return false;
	return stricmp(i,j) > 0; 
}

void Sortit(char* name,int oneline)   
{
	WORDP D = FindWord(name,0);
	if (!D) return;
	FILE* out = FopenUTF8WriteAppend((char*)"cset.txt");

	char* word = AllocateStack(NULL, maxBufferSize);
	MakeUpperCopy(word,name);
	int cat = FindTopicIDByName(name); // note if its a category, need to dump its flags also

	// get the concept members 
	std::vector<char*> mylist;
	FACT* F = GetObjectNondeadHead(D);
	bool duplicate = false;
	while (F)
	{
        if (ValidMemberFact(F) || F->verb == Mexclude)
		{
			strcpy(word,WriteMeaning(F->subject));
			if (*word == '~') MakeUpperCase(word); // cap all concepts and topics
			WORDP E = StoreWord(word);
			if (F->verb == Mexclude) AddInternalFlag(E,BEEN_HERE); // exclude notation
			mylist.push_back(E->word);
			if (F->flags & FACTDUPLICATE) duplicate = true;	// we allow duplicate facts, DONT SORT THIS
		}
		F = GetObjectNondeadNext(F);
	}
	ReleaseStack(word);

	// sort the member list, but do special concept reversed so comes in proper to flood dictionary in correct order
	if (!duplicate) std::sort(mylist.begin(), mylist.end(),!stricmp(name,(char*)"~a_dummy") ? myInverseFunction : myFunction);

	// dump the concept lists
	bool drop = false;
	bool close = false;
	char* buffer = AllocateBuffer();
	*buffer = 0;
	for (citer it = mylist.begin(), end = mylist.end(); it < end; ++it)   
	{	  
		if (close) 
		{
			fprintf(out,(char*)"%s\r\n",buffer);
			*buffer = 0;
			close = false;
			for (unsigned int j = 1; j <= 10; ++j) strcat(buffer,(char*)" ");
		}
		if (!drop) // put out the header
		{
			sprintf(buffer, (D->internalBits & TOPIC) ? (char*) "topic: %s " : (char*) "concept: %s ",name);
			drop = true;
			if (cat)
			{
				int flags = GetTopicFlags(cat);
                if (flags & TOPIC_LOWPRIORITY) strcat(buffer,(char*)"deprioritize ");
                if (flags & TOPIC_NOBLOCKING) strcat(buffer,(char*)"noblocking ");
				if (flags & TOPIC_NOKEYS) strcat(buffer,(char*)"nokeys ");
				if (flags & TOPIC_NOPATTERNS) strcat(buffer,(char*)"nopatterns ");
				if (flags & TOPIC_NOSAMPLES) strcat(buffer,(char*)"nosamples ");
				if (flags & TOPIC_NOGAMBITS) strcat(buffer,(char*)"nogambits ");
	            if (flags & TOPIC_KEEP) strcat(buffer,(char*)"keep ");
                if (flags & TOPIC_NOSTAY) strcat(buffer,(char*)"nostay ");
                if (flags & TOPIC_PRIORITY) strcat(buffer,(char*)"priority ");
                if (flags & TOPIC_RANDOM) strcat(buffer,(char*)"random ");
                if (flags & TOPIC_REPEAT) strcat(buffer,(char*)"repeat ");
				if (flags & TOPIC_SAFE) strcat(buffer,(char*)"safe ");
 				if (flags & TOPIC_SHARE) strcat(buffer,(char*)" share");
                if (flags & TOPIC_SYSTEM) strcat(buffer,(char*)"system ");
			}
			else
			{
				uint64 properties = D->properties;
				uint64 bit = START_BIT;	
				while (bit)
				{
					if (properties & bit ) sprintf(buffer + strlen(buffer),(char*)"%s ",FindNameByValue(bit));
					bit >>= 1;
				}
				properties = D->systemFlags;
				bit = START_BIT;
				while (bit)
				{
					if (properties & bit) sprintf(buffer + strlen(buffer),(char*)"%s ",FindSystemNameByValue(bit));
					bit >>= 1;
				}
			}
			strcat(buffer,(char*)"(");
		}
		char* b = buffer + strlen(buffer);
		WORDP G = FindWord(*it);
		if (G->internalBits & BEEN_HERE) // marked for exclude
		{
			G->internalBits ^= BEEN_HERE;
			sprintf(b,(char*)"!%s ",*it ); 
		}
		else sprintf(b,(char*)"%s ",*it ); 
		if (strlen(buffer) > 180 && !oneline) close = true;
	}
	if (drop) fprintf(out,(char*)"%s)\r\n",buffer);
	FreeBuffer();
	FClose(out);
}

#endif

///// ALWAYS AVAILABLE

TestMode DoCommand(char* input,char* output,bool authorize)
{
#ifndef DISCARDTESTING
	if (server && authorize && !VerifyAuthorization(FopenReadOnly((char*)"authorizedIP.txt")))  // authorizedIP
	{
		Log(SERVERLOG,(char*)"Command %s issued but not authorized\r\n",input);
		return FAILCOMMAND;
	}
	if (authorize) Log(STDUSERLOG,(char*)"Command: %s\r\n",input);
	*currentFilename = 0;
	char* ptr = NULL;
#ifndef DISCARDSCRIPTCOMPILER
	char* hold = newBuffer;
    oldBuffer = NULL; // we must be top level
	bool inited = StartScriptCompiler();
#endif
	ReadNextSystemToken(NULL,ptr,NULL,false,false);		// flush any pending data in input cache
#ifndef DISCARDSCRIPTCOMPILER
	if (inited) EndScriptCompiler();
	newBuffer = hold;
#endif
	if (strnicmp(input,(char*)":why",4)) responseIndex = 0;
	return Command(input,output,!authorize); 
#else
	if (server) Log(SERVERLOG,(char*)"Command %s issued but testing is discarded\r\n",input);
	return COMMANDED;
#endif
}
