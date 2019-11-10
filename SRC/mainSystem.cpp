#include "common.h" 
#include "evserver.h"
char* version = "9.8";
char sourceInput[200];
FILE* userInitFile;
int externalTagger = 0;
char defaultbot[100];
bool loadingUser = false;
bool dieonwritefail = false;
int inputLimit = 0;
char traceuser[500];
int traceUniversal;
PRINTER printer = printf;
unsigned int idetrace = (unsigned int) -1;
int outputlevel = 0;
char* outputCode[MAX_GLOBAL];
static bool argumentsSeen = false;
char* configFile = "cs_init.txt";	// can set config params
char* configLines[MAX_WORD_SIZE];	// can set config params
int configLinesLength;	// can set config params
char language[40];							// indicate current language used
char livedata[500];		// where is the livedata folder
char languageFolder[500];		// where is the livedata language folder
char systemFolder[500];		// where is the livedata system folder
bool noboot = false;
static char* erasename = "csuser_erase";
bool build0Requested = false;
bool build1Requested = false;
bool servertrace = false;
static int repeatLimit = 0;
bool pendingRestart = false;
bool pendingUserReset = false;
bool rebooting = false;
bool assignedLogin = false;
int forkcount = 1;
int outputchoice = -1;
char apikey[100];
DEBUGAPI debugInput = NULL;
DEBUGAPI debugOutput = NULL;
DEBUGAPI debugEndTurn = NULL;
DEBUGLOOPAPI debugCall = NULL;
DEBUGVARAPI debugVar = NULL;
DEBUGVARAPI debugMark = NULL;
DEBUGAPI debugMessage = NULL;
DEBUGAPI debugAction = NULL;
#define MAX_RETRIES 20
uint64 startTimeInfo;							// start time of current volley
char* revertBuffer;			// copy of user input so can :revert if desired
int argc;
char ** argv;
char postProcessing = 0;						// copy of output generated during MAIN control. Postprocessing can prepend to it
unsigned int tokenCount;						// for document performc
bool callback = false;						// when input is callback,alarm,loopback, dont want to enable :retry for that...
int timerLimit = 0;						// limit time per volley
int timerCheckRate = 0;					// how often to check calls for time
uint64 volleyStartTime = 0;
char forkCount[10];
int timerCheckInstance = 0;
static char* privateParams = NULL;
char treetaggerParams[200];
static char encryptParams[300];
static char decryptParams[300];
char hostname[100];
bool nosuchbotrestart = false; // restart if no such bot
char users[100];
char logs[100];
char topic[100];
char tmp[100];
char buildfiles[100];
char* derivationSentence[MAX_SENTENCE_LENGTH];
int derivationLength;
char authorizations[200];	// for allowing debug commands
char* ourMainInputBuffer = NULL;				// user input buffer - ptr to primaryInputBuffer
char* mainInputBuffer;								// user input buffer - ptr to primaryInputBuffer
char* ourMainOutputBuffer;				// main user output buffer
char* mainOutputBuffer;								//
char* readBuffer;								// main scratch reading buffer (files, etc)
bool readingDocument = false;
bool redo = false; // retry backwards any amount
bool oobExists = false;
bool docstats = false;
unsigned int docSentenceCount = 0;
char* rawSentenceCopy; // current raw sentence
char *evsrv_arg = NULL;

unsigned short int derivationIndex[256];

bool overrideAuthorization = false;

clock_t  startSystem;						// time chatscript started
unsigned int choiceCount = 0;
int always = 1;									// just something to stop the complaint about a loop based on a constant

// support for callbacks
uint64 callBackTime = 0;			// one-shot pending time - forces callback with "[callback]" when output was [callback=xxx] when no user submitted yet
uint64 callBackDelay = 0;			// one-shot pending time - forces callback with "[callback]" when output was [callback=xxx] when no user submitted yet
uint64 loopBackTime = 0;	// ongoing callback forces callback with "[loopback]"  when output was "[loopback=xxx]"
uint64 loopBackDelay = 0;	// ongoing callback forces callback with "[loopback]"  when output was "[loopback=xxx]"
uint64 alarmTime = 0;
uint64 alarmDelay = 0;					// one-shot pending time - forces callback with "[alarm]" when output was [callback=xxx] when no user submitted yet
unsigned int outputLength = 100000;		// max output before breaking.

char* extraTopicData = 0;				// supplemental topic set by :extratopic

// server data
#ifdef DISCARDSERVER
bool server = false;
#else
std::string interfaceKind = "0.0.0.0";
#ifdef WIN32
bool server = false;	// default is standalone on Windows
#elif IOS
bool server = true; // default is server on LINUX
#else
bool server = true; // default is server on LINUX
#endif
#endif
unsigned int port = 1024;						// server port
bool commandLineCompile = false;

PrepareMode tmpPrepareMode = NO_MODE;						// controls what processing is done in preparation NO_MODE, POS_MODE, PREPARE_MODE
PrepareMode prepareMode = NO_MODE;						// controls what processing is done in preparation NO_MODE, POS_MODE, PREPARE_MODE
bool noReact = false;

// :source:document data
bool documentMode = false;						// read input as a document not as chat
FILE* sourceFile = NULL;						// file to use for :source
bool multiuser = false;
EchoSource echoSource = NO_SOURCE_ECHO;			// for :source, echo that input to nowhere, user, or log
uint64 sourceStart = 0;					// beginning time of source file
unsigned int sourceTokens = 0;
unsigned int sourceLines = 0;

// status of user input
unsigned int volleyCount = 0;					// which user volley is this
bool moreToComeQuestion = false;				// is there a ? in later sentences
bool moreToCome = false;						// are there more sentences pending
uint64 tokenControl = 0;						// results from tokenization and prepare processing
unsigned int responseControl = ALL_RESPONSES;					// std output behaviors
char* nextInput;
char* copyInput; // ptr to rest of users input after current sentence
static char* oldInputBuffer = NULL;			//  copy of the sentence we are processing
char* currentInput;			// the sentence we are processing  BUG why both

// general display and flow controls
bool quitting = false;							// intending to exit chatbot
int systemReset = 0;							// intending to reload system - 1 (mild reset) 2 (full user reset)
bool autonumber = false;						// display number of volley to user
bool showWhy = false;							// show user rules generating his output
bool showTopic = false;							// show resulting topic on output
bool showTopics = false;						// show all relevant topics
bool showInput = false;							// Log internal input additions
bool showReject = false;						// Log internal repeat rejections additions
bool all = false;								// generate all possible answers to input
int regression = NO_REGRESSION;						// regression testing in progress
unsigned int trace = 0;							// current tracing flags
char debugEntry[1000];
char bootcmd[200];						// current boot tracing flags
unsigned int timing = 0;						// current timing flags
bool shortPos = false;							// display pos results as you go

int inputRetryRejoinderTopic = NO_REJOINDER;				
int inputRetryRejoinderRuleID = NO_REJOINDER;
int sentenceRetryRejoinderTopic = NO_REJOINDER;				
int sentenceRetryRejoinderRuleID = NO_REJOINDER;

char oktest[MAX_WORD_SIZE];						// auto response test
char activeTopic[200];

char respondLevel = 0;							// rejoinder level of a random choice block
static bool oobPossible = false;						// 1st thing could be oob

int inputCounter = 0;							// protecting ^input from cycling
int totalCounter = 0;							// protecting ^input from cycling

char userPrefix[MAX_WORD_SIZE];			// label prefix for user input
char botPrefix[MAX_WORD_SIZE];			// label prefix for bot output

bool unusedRejoinder = true;							// inputRejoinder has been executed, blocking further calls to ^Rejoinder
	
char* inputCopy; // the original input we were given, we will work on this

// outputs generated
RESPONSE responseData[MAX_RESPONSE_SENTENCES+1];
unsigned char responseOrder[MAX_RESPONSE_SENTENCES+1];
int responseIndex;

int inputSentenceCount;				// which sentence of volley is this
static void HandleBoot(WORDP boot,bool reboot);

///////////////////////////////////////////////
/// SYSTEM STARTUP AND SHUTDOWN
///////////////////////////////////////////////

void InitStandalone()
{
	startSystem =  clock()  / CLOCKS_PER_SEC;
	*currentFilename = 0;	//   no files are open (if logging bugs)
	tokenControl = 0;
    outputlevel = 0;
	*computerID = 0; // default bot
}

static void SetBotVariable(char* word)
{
    char* eq = strchr(word, '=');
    if (eq)
    {
        *eq = 0; 
        ReturnToAfterLayer(currentBeforeLayer-1, true);
        *word = USERVAR_PREFIX;
        SetUserVariable(word, eq + 1);
        if (server) Log(SERVERLOG, (char*)"botvariable: %s = %s\r\n", word, eq + 1);
        else (*printer)((char*)"botvariable: %s = %s\r\n", word, eq + 1);
        NoteBotVariables(); // these go into level 1
        LockLayer(false);
    }
}

static void HandleBoot(WORDP boot, bool reboot)
{
    currentBeforeLayer = LAYER_1;
	if (reboot) rebooting = true; // csboot vs csreboot
	if (boot && !noboot) // run script on startup of system. data it generates will also be layer 1 data
	{
		int oldtrace = trace;
		int oldtiming = timing;
		if (*bootcmd)
		{
			char* at = bootcmd;
			if (*bootcmd == '"')
			{
				++at;
				if (at[strlen(at) - 1] == '"')  at[strlen(at) - 1] = 0;
			}
			DoCommand(at, NULL, false);
		}
		if (!reboot) UnlockLayer(LAYER_BOOT); // unlock it to add stuff
		FACT* F = factFree;
		Callback(boot, (char*)"()", true, true); // do before world is locked
		*ourMainOutputBuffer = 0; // remove any boot message
		myBot = 0;	// restore fact owner to generic all
        // cs_reboot may call boot to reload layer, this flag
        // rebooting might be wrong...
		if (!rebooting) 
		{	
            // move dead/transient facts to end
            FACT* tailArea = factFree + 1; // last fact
			while (++F <= factFree)
			{
                if (!(F->flags & (FACTTRANSIENT|FACTDEAD)))
                {
                    F->flags |= FACTBUILD1;
                    continue;
                }
                else if (!F->subject) continue;  // already moved this one
                UnweaveFact(F);
                bool dead = true;

                while (--tailArea > F) // find someone to move
                {
                    if (tailArea->flags & (FACTTRANSIENT | FACTDEAD)) continue;  // dying fact
                    UnweaveFact(tailArea); // cut from from dictionary referencing
                    memcpy(F, tailArea, sizeof(FACT)); // replicate fact lower
                    WeaveFact(F); // rebind fact to dictionary

                    // now pre-moved fact is dead
                    tailArea->subject = tailArea->verb = tailArea->object = 0;
                    tailArea->flags |= FACTDEAD;
                    dead = false;
                    break;
                }
                // kill this completely in case we cant overwrite it with valid fact
                if (dead)
                {
                    F->subject = F->verb = F->object = 0;
                    F->flags |= FACTDEAD;
                }
            }
            while (!factFree->subject) --factFree; // reclaim

			NoteBotVariables(); // convert user variables read into bot variables in boot layer
			LockLayer(false); // rewrite level 2 start data with augmented from script data
		}
        
		else
		{
			ReturnToAfterLayer(LAYER_BOOT, false);  // dict/fact/strings reverted and any extra topic loaded info  (but CSBoot process NOT lost)
		}
		trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
		timing = (modifiedTiming) ? modifiedTimingVal : oldtiming;
		stackFree = stackStart; // drop any possible stack used
	}
	rebooting = false;
}

int CountWordsInBuckets(int& unused, unsigned int* depthcount,int limit)
{
	memset(depthcount, 0, sizeof(int)*1000);
	unused = 0;
	int words = 0;
	for (unsigned long i = 0; i <= maxHashBuckets; ++i)
	{
		int n = 0;
		if (!hashbuckets[i]) ++unused;
		else
		{
			WORDP D = Index2Word(hashbuckets[i]);
			while (D != dictionaryBase)
			{
				++n;
				D = Index2Word(GETNEXTNODE(D));
			}
		}
		words += n;
		if (n < limit) depthcount[n]++;
	}
	return words;
}

void CreateSystem()
{
    tableinput = NULL;
	overrideAuthorization = false;	// always start safe
	loading = true;
	char* os;
	mystart("createsystem");
#ifdef WIN32
	os = "Windows";
#elif IOS
	os = "IOS";
#elif __MACH__
	os = "MACH";
#elif FREEBSD
	os = "FreeBSD";
#else
	os = "LINUX";
#endif

	if (!buffers) // restart asking for new memory allocations
	{
		maxBufferSize = (maxBufferSize + 63);
		maxBufferSize &= 0xffffffC0; // force 64 bit align
		unsigned int total = maxBufferLimit * maxBufferSize;
		buffers = (char*) malloc(total); // have it around already for messages
		if (!buffers)
		{
            (*printer)((char*)"%s",(char*)"cannot allocate buffer space");
			exit(1);
		}
		bufferIndex = 0;
		readBuffer = AllocateBuffer(); // technically if variables are huge, readuserdata would overflow this
		baseBufferIndex = bufferIndex;
	}
	char data[MAX_WORD_SIZE];
	char* kind;
	int pid = 0;
#ifdef EVSERVER
	kind = "EVSERVER";
	pid = getpid();
#elif DEBUG
	kind = "Debug";
#else
	kind = "Release";
#endif
	sprintf(data,(char*)"ChatScript %s Version %s pid: %d %ld bit %s compiled %s",kind,version,pid,(long int)(sizeof(char*) * 8),os,compileDate);
	strcat(data,(char*)" host=");
	strcat(data,hostname);
	if (server)  Log(SERVERLOG,(char*)"Server %s\r\n",data);
	strcat(data,(char*)"\r\n");
    (*printer)((char*)"%s",data);

	int oldtrace = trace; // in case trace turned on by default
	trace = 0;
	*oktest = 0;

	sprintf(data,(char*)"Params:   dict:%ld fact:%ld text:%ldkb hash:%ld \r\n",(long int)maxDictEntries,(long int)maxFacts,(long int)(maxHeapBytes/1000),(long int)maxHashBuckets);
	if (server) Log(SERVERLOG,(char*)"%s",data);
	else (*printer)((char*)"%s",data);
	sprintf(data,(char*)"          buffer:%dx%dkb cache:%dx%dkb userfacts:%d outputlimit:%d loglimit:%d\r\n",(int)maxBufferLimit,(int)(maxBufferSize/1000),(int)userCacheCount,(int)(userCacheSize/1000),(int)userFactCount,
		outputsize,logsize);
	if (server) Log(SERVERLOG,(char*)"%s",data);
	else (*printer)((char*)"%s",data);
	InitScriptSystem();
	InitVariableSystem();
	ReloadSystem();			// builds base facts and dictionary (from wordnet)
	LoadTopicSystem();		// dictionary reverts to wordnet zone
	*currentFilename = 0;
    *debugEntry = 0;
	computerID[0] = 0;
	if (!assignedLogin) loginName[0] = loginID[0] = 0;
	*botPrefix = *userPrefix = 0;

	char word[MAX_WORD_SIZE];
    char buffer[MAX_WORD_SIZE];
    FILE* in = FopenReadOnly(configFile);
    if (in)
    {
        while (ReadALine(buffer, in, MAX_WORD_SIZE) >= 0)
        {
            ReadCompiledWord(buffer, word);
            if (*word == 'V') SetBotVariable(word); // these are level 1 values
        }
        fclose(in);
    }
	for (int i = 1; i < argc; ++i)
	{
		strcpy(word,argv[i]);
		if (*word == '"' && word[1] == 'V') 
		{
			memmove(word,word+1,strlen(word));
			size_t len = strlen(word);
			if (word[len-1] == '"') word[len-1] = 0;
		}
        if (*word == 'V') SetBotVariable(word); // predefined bot variable in level 1
	}
	for (int i=0;i<configLinesLength;i++)
    {
		char* line = configLines[i];
        if (*line == 'V') SetBotVariable(line); // these are level 1 values
	}

	kernelVariableThreadList = botVariableThreadList;
	botVariableThreadList = NULL;
	UnlockLayer(LAYER_BOOT); // unlock it to add stuff
	HandleBoot(FindWord((char*)"^csboot"),false);// run script on startup of system. data it generates will also be layer 1 data
	LockLayer(true);
    currentBeforeLayer = LAYER_BOOT;
	InitSpellCheck(); // after boot vocabulary added

	unsigned int factUsedMemKB = ( factFree-factBase) * sizeof(FACT) / 1000;
	unsigned int factFreeMemKB = ( factEnd-factFree) * sizeof(FACT) / 1000;
	unsigned int dictUsedMemKB = ( dictionaryFree-dictionaryBase) * sizeof(WORDENTRY) / 1000;
	// dictfree shares text space
	unsigned int textUsedMemKB = ( heapBase-heapFree)  / 1000;
	unsigned int textFreeMemKB = ( heapFree- heapEnd) / 1000;

	unsigned int bufferMemKB = (maxBufferLimit * maxBufferSize) / 1000;
	
	unsigned int used =  factUsedMemKB + dictUsedMemKB + textUsedMemKB + bufferMemKB;
	used +=  (userTopicStoreSize + userTableSize) /1000;
	unsigned int free = factFreeMemKB + textFreeMemKB;
	char route[MAX_WORD_SIZE];

	unsigned int bytes = (tagRuleCount * MAX_TAG_FIELDS * sizeof(uint64)) / 1000;
	used += bytes;
	char buf2[MAX_WORD_SIZE];
	char buf1[MAX_WORD_SIZE];
	char buf[MAX_WORD_SIZE];
	strcpy(buf,StdIntOutput(factFree-factBase));
	strcpy(buf2,StdIntOutput(textFreeMemKB));
	unsigned int depthcount[1000];
	int unused;
	int words = CountWordsInBuckets(unused, depthcount,1000);

	char depthmsg[1000];
	int x = 1000;
	while (!depthcount[--x]); // find first used slot
	char* at = depthmsg;
	*at = 0;
	for (int i = 0; i <= x; ++i)
	{
		sprintf(at, "%d.", depthcount[i]);
		at += strlen(at);
	}

	sprintf(data, (char*)"Used %dMB: dict %s (%dkb) hashdepths %s words %d unusedbuckets %d fact %s (%dkb) heap %dkb\r\n",
		used / 1000,
		StdIntOutput(dictionaryFree - dictionaryBase - 1),
		dictUsedMemKB, depthmsg,words,unused,
		buf,
		factUsedMemKB,
		textUsedMemKB);
	if (server) Log(SERVERLOG,(char*)"%s",data);
	else (*printer)((char*)"%s",data);

	sprintf(data,(char*)"           buffer (%dkb) cache (%dkb)\r\n",
		bufferMemKB,
		(userTopicStoreSize + userTableSize)/1000);
	if(server) Log(SERVERLOG,(char*)"%s",data);
	else (*printer)((char*)"%s",data);

	strcpy(buf,StdIntOutput(factEnd-factFree)); // unused facts
	strcpy(buf1,StdIntOutput(textFreeMemKB)); // unused text
	strcpy(buf2,StdIntOutput(free/1000));

	sprintf(data,(char*)"Free %sMB: dict %s fact %s stack/heap %sKB \r\n\r\n",
		buf2,StdIntOutput(((unsigned int)maxDictEntries)-(dictionaryFree-dictionaryBase)),buf,buf1);
	if (server) Log(SERVERLOG,(char*)"%s",data);
	else (*printer)((char*)"%s",data);

	trace = oldtrace;
#ifdef DISCARDSERVER 
    (*printer)((char*)"    Server disabled.\r\n");
#endif
#ifdef DISCARDSCRIPTCOMPILER
	if(server) Log(SERVERLOG,(char*)"    Script compiler disabled.\r\n");
	else (*printer)((char*)"    Script compiler disabled.\r\n");
#endif
#ifdef DISCARDTESTING
	if(server) Log(SERVERLOG,(char*)"    Testing disabled.\r\n");
	else (*printer)((char*)"    Testing disabled.\r\n");
#else
	*callerIP = 0;
	if (!assignedLogin) *loginID = 0;
	if (server && VerifyAuthorization(FopenReadOnly((char*)"authorizedIP.txt"))) // authorizedIP
	{
		Log(SERVERLOG,(char*)"    *** Server WIDE OPEN to :command use.\r\n");
	}
#endif
#ifdef DISCARDJSONOPEN
	sprintf(route, (char*)"%s", (char*)"    JSONOpen access disabled.\r\n");
	if (server) Log(SERVERLOG, route);
	else (*printer)(route);
#endif
#ifdef TREETAGGER
	sprintf(route, (char*)"    TreeTagger access enabled: %s\r\n",language);
	if (server) Log(SERVERLOG, route);
	else (*printer)(route);
#endif
#ifndef DISCARDMONGO
	//if (*mongodbparams) sprintf(route,"    Mongo enabled. FileSystem routed to %s\r\n",mongodbparams);
	if (*mongodbparams) sprintf(route,"    Mongo enabled. FileSystem routed to MongoDB\r\n");
	else sprintf(route,"    Mongo enabled.\r\n"); 
	if (server) Log(SERVERLOG,route);
	else (*printer)(route);
#endif
#ifndef DISCARDPOSTGRES
	// if (*postgresparams) sprintf(route, "    Postgres enabled. FileSystem routed to %s\r\n", postgresparams);
	if (*postgresparams) sprintf(route, "    Postgres enabled. FileSystem routed postgress\r\n");
	else sprintf(route, "    Postgres enabled.\r\n");
	if (server) Log(SERVERLOG, route);
	else (*printer)(route);
#endif
    (*printer)((char*)"%s",(char*)"\r\n");
	loading = false;
}

void ReloadSystem()
{//   reset the basic system through wordnet but before topics
    myBot = 0; // facts loaded here are universal
	InitFacts(); // malloc space
	InitStackHeap(); // malloc space
	InitDictionary(); // malloc space
	InitCache(); // malloc space
	// make sets for the part of speech data
	LoadDictionary();
	InitFunctionSystem();
#ifndef DISCARDTESTING
	InitCommandSystem();
#endif
	ExtendDictionary(); // store labels of concepts onto variables.
	DefineSystemVariables();
	ClearUserVariables();
	AcquirePosMeanings(false); // do not build pos facts (will be in binary) but make vocab available
	if (!ReadBinaryFacts(FopenStaticReadOnly(UseDictionaryFile((char*)"facts.bin"))))  // DICT
	{
		AcquirePosMeanings(true);
		WORDP safeDict = dictionaryFree;
		ReadFacts(UseDictionaryFile((char*)"facts.txt"),NULL,0);
		if ( safeDict != dictionaryFree) myexit((char*)"dict changed on read of facts");
		WriteBinaryFacts(FopenBinaryWrite(UseDictionaryFile((char*)"facts.bin")),factBase);
	}
	char name[MAX_WORD_SIZE];
	sprintf(name,(char*)"%s/%s/systemfacts.txt",livedata,language);
	ReadFacts(name,NULL,0); // part of wordnet, not level 0 build 
	ReadLiveData();  // considered part of basic system before a build
	InitTextUtilities1(); // also part of basic system before a build

#ifdef TREETAGGER
	 // We will be reserving some working space for treetagger on the heap
	 // so make sure it is tucked away where it cannot be touched
		InitTreeTagger(treetaggerParams);
#endif

	WordnetLockDictionary();
}

static void ProcessArgument(char* arg)
{
	if (!argumentsSeen)
	{
		char path[MAX_WORD_SIZE];
		*path = 0;
		GetCurrentDir(path, MAX_WORD_SIZE);
        (*printer)("CommandLine: %s\r\n",path);
		argumentsSeen = true;
	}
    (*printer)("    %s\r\n",arg);
	if (!stricmp(arg,(char*)"trace")) trace = (unsigned int) -1; 
	else if (!strnicmp(arg,(char*)"language=",9)) 
	{
		strcpy(language,arg+9);
		MakeUpperCase(language);
	}
    else if (!strnicmp(arg, (char*)"buffer=", 7))  // number of large buffers available  8x80000
    {
        maxBufferLimit = atoi(arg + 7);
        char* size = strchr(arg + 7, 'x');
        if (size) maxBufferSize = atoi(size + 1) * 1000;
        if (maxBufferSize < OUTPUT_BUFFER_SIZE)
        {
            (*printer)((char*)"Buffer cannot be less than OUTPUT_BUFFER_SIZE of %d\r\n", OUTPUT_BUFFER_SIZE);
            myexit((char*)"buffer size less than output buffer size");
        }
    }
    else if (!stricmp(arg, (char*)"trustpos")) trustpos = true;
    else if (!strnicmp(arg, (char*)"inputlimit=", 11)) inputLimit = atoi(arg+11);
    else if (!strnicmp(arg, (char*)"debug=",6)) strcpy(arg+6,debugEntry);
    else if (!strnicmp(arg, "erasename=", 10)) erasename = arg + 10;
	else if (!stricmp(arg,"userencrypt")) userEncrypt = true;
	else if (!stricmp(arg,"ltmencrypt")) ltmEncrypt = true;
	else if (!strnicmp(arg, "defaultbot=", 11)) strcpy(defaultbot, arg + 11);
	else if (!strnicmp(arg, "traceuser=", 10)) strcpy(traceuser, arg + 10);
	else if (!stricmp(arg, (char*)"noboot")) noboot = true;
    else if (!stricmp(arg, (char*)"recordboot")) recordBoot = true;
	else if (!stricmp(arg, (char*)"servertrace")) servertrace = true;
    else if (!strnicmp(arg, (char*)"crashpath=",10)) strcpy(crashpath,arg+10);
    else if (!strnicmp(arg, (char*)"repeatlimit=",12)) repeatLimit = atoi(arg+12);
    else if (!strnicmp(arg,(char*)"apikey=",7)) strcpy(apikey,arg+7);
	else if (!strnicmp(arg,(char*)"logsize=",8)) logsize = atoi(arg+8); // bytes avail for log buffer
	else if (!strnicmp(arg, (char*)"loglimit=", 9)) loglimit = atoi(arg + 9); // max mb of log file before change files
	else if (!strnicmp(arg,(char*)"outputsize=",11)) outputsize = atoi(arg+11); // bytes avail for output buffer
    else if (!stricmp(arg, (char*)"time")) timing = (unsigned int)-1 ^ TIME_ALWAYS;
	else if (!strnicmp(arg,(char*)"bootcmd=",8)) strcpy(bootcmd,arg+8); 
    else if (!strnicmp(arg, (char*)"authorize", 9)) overrideAuthorization = true;
	else if (!strnicmp(arg,(char*)"dir=",4))
	{
#ifdef WIN32
		if (!SetCurrentDirectory(arg+4)) (*printer)((char*)"unable to change to %s\r\n",arg+4);
#else
		chdir(arg+5);
#endif
	}
	else if (!strnicmp(arg,(char*)"source=",7)) 
	{
		if (arg[7] == '"') 
		{
			strcpy(sourceInput,arg+8);
			size_t len = strlen(sourceInput);
			if (sourceInput[len-1] == '"') sourceInput[len-1] = 0;
		}
		else strcpy(sourceInput,arg+7);
	}
	else if (!strnicmp(arg,(char*)"login=",6)) 
	{
		assignedLogin = true;
		strcpy(loginID,arg+6);
	}
	else if (!strnicmp(arg,(char*)"output=",7)) outputLength = atoi(arg+7);
	else if (!strnicmp(arg,(char*)"save=",5)) 
	{
		volleyLimit = atoi(arg+5);
		if (volleyLimit > 255) volleyLimit = 255; // cant store higher
	}

	// memory sizings
	else if (!strnicmp(arg,(char*)"hash=",5)) 
	{
		maxHashBuckets = atoi(arg+5); // size of hash
		setMaxHashBuckets = true;
	}
	else if (!strnicmp(arg,(char*)"dict=",5)) maxDictEntries = atoi(arg+5); // how many dict words allowed
	else if (!strnicmp(arg,(char*)"fact=",5)) maxFacts = atoi(arg+5);  // fact entries
	else if (!strnicmp(arg,(char*)"text=",5)) maxHeapBytes = atoi(arg+5) * 1000; // stack and heap bytes in pages
	else if (!strnicmp(arg,(char*)"cache=",6)) // value of 10x0 means never save user data
	{
		userCacheSize = atoi(arg+6) * 1000;
		char* number = strchr(arg+6,'x');
		if (number) userCacheCount = atoi(number+1);
	}
	else if (!strnicmp(arg,(char*)"userfacts=",10)) userFactCount = atoi(arg+10); // how many user facts allowed
	else if (!stricmp(arg,(char*)"redo")) redo = true; // enable redo
	else if (!strnicmp(arg,(char*)"authorize=",10)) strcpy(authorizations,arg+10); // whitelist debug commands
	else if (!stricmp(arg,(char*)"nodebug")) *authorizations = '1'; 
	else if (!strnicmp(arg,(char*)"users=",6 )) strcpy(users,arg+6);
	else if (!strnicmp(arg,(char*)"logs=",5 )) strcpy(logs,arg+5);
	else if (!strnicmp(arg,(char*)"topic=",6 )) strcpy(topic,arg+6);
	else if (!strnicmp(arg,(char*)"tmp=",4 )) strcpy(tmp,arg+4);
    else if (!strnicmp(arg, (char*)"buildfiles=", 11)) strcpy(buildfiles, arg + 11);
    else if (!strnicmp(arg,(char*)"private=",8)) privateParams = arg+8;
	else if (!strnicmp(arg, (char*)"treetagger", 10))
	{
		if (arg[10] == '=') strcpy(treetaggerParams, arg + 11);
		else strcpy(treetaggerParams, "1");
	}
	else if (!strnicmp(arg,(char*)"encrypt=",8)) strcpy(encryptParams,arg+8);
	else if (!strnicmp(arg,(char*)"decrypt=",8)) strcpy(decryptParams,arg+8);
	else if (!strnicmp(arg,(char*)"livedata=",9) ) 
	{
		strcpy(livedata,arg+9);
		sprintf(systemFolder,(char*)"%s/SYSTEM",arg+9);
		sprintf(languageFolder,(char*)"%s/%s",arg+9,language);
	}
	else if (!strnicmp(arg,(char*)"nosuchbotrestart=",17) ) 
	{
		if (!stricmp(arg+17,"true")) nosuchbotrestart = true;
		else nosuchbotrestart = false;
	}
	else if (!strnicmp(arg,(char*)"system=",7) )  strcpy(systemFolder,arg+7);
	else if (!strnicmp(arg,(char*)"english=",8) )  strcpy(languageFolder,arg+8);
#ifndef DISCARDPOSTGRES
	else if (!strnicmp(arg,(char*)"pguser=",7) )  strcpy(postgresparams, arg+7);
	// Postgres Override SQL
	else if (!strnicmp(arg,(char*)"pguserread=",11) )
	{
		strcpy(postgresuserread, arg+11);
		pguserread = postgresuserread;
	}
	else if (!strnicmp(arg,(char*)"pguserinsert=",13) )
	{
		strcpy(postgresuserinsert, arg+13);
		pguserinsert = postgresuserinsert;
	}
	else if (!strnicmp(arg,(char*)"pguserupdate=",13) )
	{
		strcpy(postgresuserupdate, arg+13);
		pguserupdate = postgresuserupdate;
	}
#endif
#ifndef DISCARDMYSQL
	else if (!strnicmp(arg,(char*)"mysqlhost=",10) )
	{
		mysqlconf = true;
		strcpy(mysqlhost, arg+10);
	}
	else if (!strnicmp(arg,(char*)"mysqlport=",10) )
	{
		mysqlconf = true;
		unsigned int port = atoi(arg+10);
		mysqlport = port;
	}
	else if (!strnicmp(arg,(char*)"mysqldb=",8) )
	{
		mysqlconf = true;
		strcpy(mysqldb, arg+8);
	}
	else if (!strnicmp(arg,(char*)"mysqluser=",10) )
	{
		mysqlconf = true;
		strcpy(mysqluser, arg+10);
	}
	else if (!strnicmp(arg,(char*)"mysqlpasswd=",12) )
	{
		mysqlconf = true;
		strcpy(mysqlpasswd, arg+12);
	}
	// MySQL Query Override SQL
	else if (!strnicmp(arg,(char*)"mysqluserread=",14) )
	{
		strcpy(my_userread_sql, arg+14);
		mysql_userread = my_userread_sql;
	}
	else if (!strnicmp(arg,(char*)"mysqluserinsert=",16) )
	{
		strcpy(my_userinsert_sql, arg+16);
		mysql_userinsert = my_userinsert_sql;
	}
	else if (!strnicmp(arg,(char*)"mysqluserupdate=",16) )
	{
		strcpy(my_userupdate_sql, arg+16);
		mysql_userupdate = my_userupdate_sql;
	}
#endif
#ifndef DISCARDMONGO
	else if (!strnicmp(arg,(char*)"mongo=",6) )  strcpy(mongodbparams,arg+6);
#endif
#ifndef DISCARDCLIENT
	else if (!strnicmp(arg,(char*)"client=",7)) // client=1.2.3.4:1024  or  client=localhost:1024
	{
		server = false;
		char buffer[MAX_WORD_SIZE];
		strcpy(serverIP,arg+7);
		
		char* portVal = strchr(serverIP,':');
		if ( portVal)
		{
			*portVal = 0;
			port = atoi(portVal+1);
		}

		if (!*loginID)
		{
            (*printer)((char*)"%s",(char*)"\r\nEnter client user name: ");
			ReadALine(buffer,stdin);
            (*printer)((char*)"%s",(char*)"\r\n");
			Client(buffer);
		}
		else Client(loginID);
		myexit((char*)"client ended");
	}  
#endif
	else if (!stricmp(arg,(char*)"userlog")) userLog = true;
	else if (!stricmp(arg,(char*)"nouserlog")) userLog = false;
    else if (!stricmp(arg, (char*)"dieonwritefail")) dieonwritefail = true;
	else if (!strnicmp(arg, (char*)"hidefromlog=", 12))
	{
		char* at = arg + 12;
		if (*at == '"') ++at;
		strcpy(hide, at);
		size_t len = strlen(hide);
		if (hide[len - 1] == '"') hide[len - 1] = 0;
	}
#ifndef DISCARDSERVER
	else if (!stricmp(arg,(char*)"serverretry")) serverRetryOK = true;
	else if (!stricmp(arg,(char*)"local")) server = false; // local standalone
	else if (!stricmp(arg,(char*)"noserverlog")) serverLog = false;
	else if (!stricmp(arg,(char*)"serverlog")) serverLog = true;
	else if (!stricmp(arg,(char*)"noserverprelog")) serverPreLog = false;
	else if (!stricmp(arg,(char*)"serverctrlz")) serverctrlz = 1;
	else if (!strnicmp(arg,(char*)"port=",5))  // be a server
	{
        port = atoi(arg+5); // accept a port=
		server = true;
#ifdef LINUX
		if (forkcount > 1) {
			sprintf(serverLogfileName, (char*)"%s/serverlog%d-%d.txt", logs, port,getpid());
            sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d-%d.txt", logs, port, getpid());
		}
		else {
			sprintf(serverLogfileName, (char*)"%s/serverlog%d.txt", logs, port); 
            sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d.txt", logs, port);
		}
#else
		sprintf(serverLogfileName,(char*)"%s/serverlog%d.txt",logs,port); // DEFAULT LOG
        sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d.txt", logs, port);
#endif
	}
#ifdef EVSERVER
	else if (!strnicmp(arg, "fork=", 5)) 
	{
		forkcount = atoi(arg + 5);
		sprintf(forkCount,(char*)"evsrv:fork=%d", forkcount);
		evsrv_arg = forkCount;
#ifdef LINUX
		if (forkcount > 1) {
			sprintf(serverLogfileName, (char*)"%s/serverlog%d-%d.txt", logs, port, getpid());
            sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d-%d.txt", logs, port, getpid());
		}
		else {
			sprintf(serverLogfileName, (char*)"%s/serverlog%d.txt", logs, port);
        	sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d.txt", logs, port);
		}
#else
		sprintf(serverLogfileName, (char*)"%s/serverlog%d.txt", logs, port); // DEFAULT LOG
        sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d.txt", logs, port);
#endif
	}
#endif
	else if (!strnicmp(arg,(char*)"interface=",10)) interfaceKind = string(arg+10); // specify interface
#endif
}

void ProcessArguments(int xargc, char* xargv[])
{
	for (int i = 1; i < xargc; ++i) ProcessArgument(xargv[i]);
}

 
static void ProcessConfigLines(){
	for (int i=0; i< configLinesLength; i++){
		ProcessArgument(TrimSpaces(configLines[i],true));
	}
}

#ifdef WIN32
#include "curl.h"
#else
#include <curl/curl.h>
#endif

static size_t ConfigCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static void LoadconfigFromUrl(char*configUrl, char**configUrlHeaders, int headerCount){
#ifndef DISCARDJSONOPEN 
    InitCurl();
	CURL *req = curl_easy_init();
	string response_string;
	curl_easy_setopt(req, CURLOPT_CUSTOMREQUEST, "GET");
	curl_easy_setopt(req, CURLOPT_URL, configUrl);
	struct curl_slist *headers = NULL;
	for (int i = 0; i<headerCount; i++)
    {
		headers = curl_slist_append(headers, configUrlHeaders[i]);
	}
	headers = curl_slist_append(headers, "Accept: text/plain");
	headers = curl_slist_append(headers, "Cache-Control: no-cache");
	curl_easy_setopt(req, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(req, CURLOPT_WRITEFUNCTION, ConfigCallback);
	curl_easy_setopt(req, CURLOPT_WRITEDATA, &response_string);
	CURLcode ret = curl_easy_perform(req);
	char word[MAX_WORD_SIZE] = {'\0'};
	int wordcount = 0;
	for (unsigned int i=0; i<=response_string.size(); i++){
		if( response_string[i] == '\n' || (response_string[i] == '\\' && response_string[i+1] == 'n')  ){
			configLines[configLinesLength] = (char*)malloc(sizeof(char)*MAX_WORD_SIZE);
			strcpy(configLines[configLinesLength++],TrimSpaces(word,true));
			for (int ti=0;ti<MAX_WORD_SIZE;ti++){
				word[ti]='\0';
			}
			wordcount=0;
			if(!(response_string[i] == '\n' ))
				i++;
		}
		else{
			word[wordcount++] = response_string[i];
		}
	}
	    curl_easy_cleanup(req);
		CurlShutdown();
	ProcessConfigLines();
#endif
}

static void ReadConfig()
{
	char buffer[MAX_WORD_SIZE];
	FILE* in = FopenReadOnly(configFile);
	if (!in) return;
	while (ReadALine(buffer,in,MAX_WORD_SIZE) >= 0){
		ProcessArgument(TrimSpaces(buffer,true));
	}
	fclose(in);
}

unsigned int InitSystem(int argcx, char * argvx[],char* unchangedPath, char* readablePath, char* writeablePath, USERFILESYSTEM* userfiles, DEBUGAPI infn, DEBUGAPI outfn)
{ // this work mostly only happens on first startup, not on a restart
    for (int i = 0; i <= MAX_WILDCARDS; ++i)
    {
        wildcardOriginalText[i][0] = 0;
        wildcardCanonicalText[i][0] = 0;
        wildcardPosition[i] = 0;
    }
    
    *traceuser = 0;
	*hide = 0;
	*scopeBotName = 0;
	FILE* in = FopenStaticReadOnly((char*)"SRC/dictionarySystem.h"); // SRC/dictionarySystem.h
	if (!in) // if we are not at top level, try going up a level
	{
#ifdef WIN32
		if (!SetCurrentDirectory((char*)"..")) // move from BINARIES to top level
			myexit((char*)"unable to change up\r\n");
#else
		chdir((char*)"..");
#endif
	}
	else FClose(in);
	*defaultbot = 0;
	strcpy(hostname,(char*)"local");
	*sourceInput = 0;
    *buildfiles = 0;
	*apikey = 0;
	*bootcmd = 0;
#ifndef DISCARDMONGO
	*mongodbparams = 0;
#endif
#ifndef DISCARDPOSTGRES
	*postgresparams = 0;
#endif
#ifdef TREETAGGER
	*treetaggerParams = 0;
#endif
	*authorizations = 0;
	debugInput = infn;
	debugOutput = outfn;
	argc = argcx;
	argv = argvx;
	InitFileSystem(unchangedPath,readablePath,writeablePath);
	if (userfiles) memcpy((void*)&userFileSystem,userfiles,sizeof(userFileSystem));
	for (unsigned int i = 0; i <= MAX_WILDCARDS; ++i)
	{
		*wildcardOriginalText[i] =  *wildcardCanonicalText[i]  = 0; 
		wildcardPosition[i] = 0;
	}
	strcpy(users,(char*)"USERS");
	strcpy(logs,(char*)"LOGS");
	strcpy(topic,(char*)"TOPIC");
	strcpy(tmp,(char*)"TMP");

	strcpy(language,(char*)"ENGLISH");

	*loginID = 0;
	char*configUrl = NULL;
	char*configHeaders[20];
	int headerCount = 0;
	for (int i = 1; i < argc; ++i) // essentials
	{
		char* configHeader;
		if (!strnicmp(argv[i],(char*)"config=",7)) configFile = argv[i]+7;
		if (!strnicmp(argv[i],(char*)"configUrl=",10)) configUrl = argv[i]+10;
		if (!strnicmp(argv[i],(char*)"configHeader=",13)) {
			configHeader = argv[i]+13;
			configHeaders[headerCount] = (char*)malloc(sizeof(char)*strlen(configHeader)+1);
			strcpy(configHeaders[headerCount],configHeader);
			configHeaders[headerCount++][strlen(configHeader)] = '\0';
		}
	}
    ReadConfig();

	currentRuleOutputBase = currentOutputBase = ourMainOutputBuffer = (char*)malloc(outputsize);
	currentOutputLimit = outputsize; 

	// need buffers for things that run ahead like servers and such.
	maxBufferSize = (maxBufferSize + 63);
	maxBufferSize &= 0xffffffc0; // force 64 bit align on size  
	unsigned int total = maxBufferLimit * maxBufferSize;
	buffers = (char*) malloc(total); // have it around already for messages
	if (!buffers)
	{
        (*printer)((char*)"%s",(char*)"cannot allocate buffer space");
		return 1;
	}
	bufferIndex = 0;
	readBuffer = AllocateBuffer();
	baseBufferIndex = bufferIndex;

    oldInputBuffer = (char*)malloc(maxBufferSize);
    lastInputSubstitution = (char*)malloc(maxBufferSize);
    inputCopy = (char*)malloc(maxBufferSize);
    rawSentenceCopy = (char*)malloc(maxBufferSize);
    currentInput = (char*)malloc(maxBufferSize);
    revertBuffer = (char*)malloc(maxBufferSize);
    ourMainInputBuffer = (char*)malloc(maxBufferSize * 2);  // precaution for overlow
    copyInput = (char*)malloc(maxBufferSize);

	quitting = false;
	echo = true;	
	if (configUrl != NULL)
		LoadconfigFromUrl(configUrl, configHeaders, headerCount);
	ProcessArguments(argc,argv);
	MakeDirectory(tmp);
	if (argumentsSeen) (*printer)("\r\n");
	argumentsSeen = false;

	InitTextUtilities();
    sprintf(logFilename,(char*)"%s/log%d.txt",logs,port); // DEFAULT LOG
#ifdef LINUX
	if (forkcount > 1) {
		sprintf(serverLogfileName, (char*)"%s/serverlog%d-%d.txt", logs, port,getpid());
        sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d-%d.txt", logs, port, getpid());
	}
	else {
		sprintf(serverLogfileName, (char*)"%s/serverlog%d.txt", logs, port);
        sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d.txt", logs, port);
	}
#else
    sprintf(serverLogfileName,(char*)"%s/serverlog%d.txt",logs,port); // DEFAULT LOG
    sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d.txt", logs, port);
#endif
	
	strcpy(livedata,(char*)"LIVEDATA"); // default directory for dynamic stuff
	strcpy(systemFolder,(char*)"LIVEDATA/SYSTEM"); // default directory for dynamic stuff
	sprintf(languageFolder,"LIVEDATA/%s",language); // default directory for dynamic stuff

	if (redo) autonumber = true;

	// defaults where not specified
	if (userLog == LOGGING_NOT_SET) userLog = 1;	// default ON for user if unspecified
	if (serverLog == LOGGING_NOT_SET) serverLog = 1; // default ON for server if unspecified
	
	int oldserverlog = serverLog;
	serverLog = true;

#ifndef DISCARDSERVER
	if (server)
	{
#ifndef EVSERVER
		GrabPort(); 
#else
        (*printer)("evcalled pid: %d\r\n",getpid());
		if (evsrv_init(interfaceKind, port, evsrv_arg) < 0)  exit(4); // additional params will apply to each child and they will load data each
#endif
#ifdef WIN32
		PrepareServer(); 
#endif
	}
#endif

	if (volleyLimit == -1) volleyLimit = DEFAULT_VOLLEY_LIMIT;
	for (int i = 1; i < argc; ++i)
	{
		if (!strnicmp(argv[i], (char*)"build0=", 7)) build0Requested = true;
		if (!strnicmp(argv[i], (char*)"build1=", 7)) build1Requested = true;
	}

	CreateSystem();
	for (int i = 1; i < argc; ++i)
	{
#ifndef DISCARDSCRIPTCOMPILER
		if (!strnicmp(argv[i],(char*)"build0=",7))
		{
			sprintf(logFilename,(char*)"%s/build0_log.txt",users);
			in = FopenUTF8Write(logFilename);
			FClose(in);
			commandLineCompile = true;
			int result = ReadTopicFiles(argv[i]+7,BUILD0,NO_SPELL);
 			myexit((char*)"build0 complete",result);
		}  
		if (!strnicmp(argv[i],(char*)"build1=",7))
		{
			sprintf(logFilename,(char*)"%s/build1_log.txt",users);
			in = FopenUTF8Write(logFilename);
			FClose(in);
			commandLineCompile = true;
			int result = ReadTopicFiles(argv[i]+7,BUILD1,NO_SPELL);
 			myexit((char*)"build1 complete",result);
		}  
#endif
		if (!strnicmp(argv[i],(char*)"timer=",6))
		{
			char* x = strchr(argv[i]+6,'x'); // 15000x10
			if (x)
			{
				*x = 0;
				timerCheckRate = atoi(x+1);
			}
			timerLimit = atoi(argv[i]+6);
		}
#ifndef DISCARDTESTING
		if (!strnicmp(argv[i],(char*)"debug=",6))
		{
			char* ptr = SkipWhitespace(argv[i]+6);
			commandLineCompile = true;
			if (*ptr == ':') DoCommand(ptr,mainOutputBuffer);
 			myexit((char*)"test complete");
		}  
#endif
		if (!stricmp(argv[i],(char*)"trace")) trace = (unsigned int) -1; // make trace work on login
		if (!stricmp(argv[i], (char*)"time")) timing = (unsigned int)-1 ^ TIME_ALWAYS; // make timing work on login
	}

	if (server)
	{
		struct tm ptm;
#ifndef EVSERVER
		Log(SERVERLOG, "\r\n\r\n======== Began server %s compiled %s on host %s port %d at %s serverlog:%d userlog: %d\r\n",version,compileDate,hostname,port,GetTimeInfo(&ptm,true),oldserverlog,userLog);
        (*printer)((char*)"\r\n\r\n======== Began server %s compiled %s on host %s port %d at %s serverlog:%d userlog: %d\r\n",version,compileDate,hostname,port,GetTimeInfo(&ptm,true),oldserverlog,userLog);
#else
		Log(SERVERLOG, "\r\n\r\n======== Began EV server pid %d %s compiled %s on host %s port %d at %s serverlog:%d userlog: %d\r\n",getpid(),version,compileDate,hostname,port,GetTimeInfo(&ptm,true),oldserverlog,userLog);
        (*printer)((char*)"\r\n\r\n======== Began EV server pid %d  %s compiled %s on host %s port %d at %s serverlog:%d userlog: %d\r\n",getpid(),version,compileDate,hostname,port,GetTimeInfo(&ptm,true),oldserverlog,userLog);
#endif
	}
	serverLog = oldserverlog;

	echo = false;

	InitStandalone();
#ifdef PRIVATE_CODE
    PrivateInit(privateParams);
#endif
#ifndef DISCARDMYSQL
	if (mysqlconf) MySQLUserFilesCode(); //Forked must hook uniquely AFTER forking
#endif
#ifndef DISCARDPOSTGRES
#ifndef EVSERVER
	if (*postgresparams)  PGUserFilesCode(); // unforked process can hook directly. Forked must hook AFTER forking
#endif
#endif
#ifndef DISCARDMONGO
	if (*mongodbparams)  MongoSystemInit(mongodbparams);
#endif

	EncryptInit(encryptParams);
	DecryptInit(decryptParams);
	ResetEncryptTags();
	return 0;
}

void PartiallyCloseSystem() // server data (queues etc) remain available
{
	WORDP shutdown = FindWord((char*)"^csshutdown");
	if (shutdown)  Callback(shutdown,(char*)"()",false,true); 
#ifndef DISCARDJAVASCRIPT
	DeleteTransientJavaScript(); // unload context if there
	DeletePermanentJavaScript(); // javascript permanent system
#endif
	FreeAllUserCaches(); // user system
    CloseDictionary();	// dictionary system
    CloseFacts();		// fact system
	CloseBuffers();		// memory system
#ifndef DISCARDPOSTGRES
	PostgresShutDown();
#endif
#ifndef DISCARDMYSQL
	MySQLserFilesCloseCode();
#endif
#ifndef DISCARDMONGO
	MongoSystemRestart(); // actually just closes files
#endif
}

void CloseSystem()
{
	PartiallyCloseSystem();
	// server remains up on a restart
#ifndef DISCARDSERVER
	CloseServer();
#endif
	// user file rerouting stays up on a restart
#ifndef DISCARDPOSTGRES
	PGUserFilesCloseCode();
#endif
#ifndef DISCARDMONGO
	MongoSystemShutdown();
#endif
#ifdef PRIVATE_CODE
	PrivateShutdown();  // must come last after any mongo/postgress 
#endif
#ifndef DISCARDJSONOPEN
	CurlShutdown();
#endif
	free(logmainbuffer);
	logmainbuffer = NULL;
	free(ourMainOutputBuffer);
	ourMainOutputBuffer = NULL;

    free(oldInputBuffer);
    oldInputBuffer = NULL;
    free(lastInputSubstitution);
    lastInputSubstitution = NULL;
    free(inputCopy);
    inputCopy = NULL;
    free(rawSentenceCopy);
    rawSentenceCopy = NULL;
    free(currentInput);
    currentInput = NULL;
    free(revertBuffer);
    revertBuffer = NULL;
    free(ourMainInputBuffer);
    ourMainInputBuffer = NULL;
    free(copyInput);
    copyInput = NULL;
}

////////////////////////////////////////////////////////
/// INPUT PROCESSING
////////////////////////////////////////////////////////
#ifdef LINUX
	#define LINUXORMAC 1
#endif
#ifdef __MACH__
	#define LINUXORMAC 1
#endif

#ifdef LINUXORMAC
#include <fcntl.h>
int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;
 
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
  ch = getchar();
 
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
 
  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }
 
  return 0;
}
#endif

int FindOOBEnd(int start)
{
    int begin = start;
	if (*wordStarts[start] == OOB_START)
	{
		while (++start < wordCount && *wordStarts[start] != OOB_END); // find a close
		return (wordStarts[start]  && *wordStarts[start] == OOB_END) ? (start + 1) : begin;
	}
	else return 1; // none
}

void ProcessOOB(char* output)
{
	char* ptr = SkipWhitespace(output);
	if (*ptr == OOB_START) // out-of-band data?
	{
		char* at = ptr;
		bool quote = false;
		int bracket = 1;
		while (*++at) // do full balancing
		{
			if (*at == '"' && *(at-1) != '\\') quote = !quote;
			else if (!quote)
			{
				if (*at == '[') ++bracket;
				else if (*at == ']') 
				{
					--bracket;
					if (!bracket) break; // true end
				}
			}
		}

		char* end = (*at == ']') ? at : NULL;
		if (end)
		{
			uint64 milli = ElapsedMilliseconds();
			char* atptr = strstr(ptr,(char*)"loopback="); // loopback is reset after every user output
			if (atptr)
			{
                atptr = SkipWhitespace(atptr +9);
				loopBackDelay = atoi(atptr);
				loopBackTime = milli + loopBackDelay;
			}
            atptr = strstr(ptr,(char*)"callback="); // call back is canceled if user gets there first
			if (atptr)
			{
                atptr = SkipWhitespace(atptr +9);
				callBackDelay = atoi(atptr);
				callBackTime = milli + callBackDelay;
			}
            atptr = strstr(ptr,(char*)"alarm="); // alarm stays pending until it launches
			if (atptr)
			{
				atptr = SkipWhitespace(atptr +6);
				alarmDelay = atoi(atptr);
				alarmTime = milli + alarmDelay;
			}
			if (!oob) memmove(output,end+1,strlen(end)); // delete oob data so not printed to user
		}
	}
}

bool ProcessInputDelays(char* buffer,bool hitkey)
{
	// override input for a callback
	if (callBackDelay || loopBackDelay || alarmDelay) // want control back to chatbot when user has nothing
	{
		uint64 milli =  ElapsedMilliseconds();
		if (!hitkey && (sourceFile == stdin || !sourceFile))
		{
			if (loopBackDelay && milli > (uint64)loopBackTime) 
			{
				strcpy(buffer,(char*)"[ loopback ]");
                (*printer)((char*)"%s",(char*)"\r\n");
			}
			else if (callBackDelay && milli > (uint64)callBackTime) 
			{
				strcpy(buffer,(char*)"[ callback ]");
                (*printer)((char*)"%s",(char*)"\r\n");
				callBackDelay = 0; // used up
			}
			else if (alarmDelay && milli > (uint64)alarmTime)
			{
				alarmDelay = 0;
				strcpy(buffer,(char*)"[ alarm ]");
                (*printer)((char*)"%s",(char*)"\r\n");
			}
			else return true; // nonblocking check for input
		}
		if (alarmDelay && milli > (uint64)alarmTime) // even if he hits a key, alarm has priority
		{
			alarmDelay = 0;
			strcpy(buffer,(char*)"[ alarm ]");
            (*printer)((char*)"%s",(char*)"\r\n");
		}
	}
	return false;
}

char* ReviseOutput(char* out,char* prefix)
{
	strcpy(prefix, out);
	char* at = prefix;
	while ((at = strchr(at, '\\')))
	{
		if (at[1] == 'n')
		{
			memmove(at, at + 1, strlen(at));
			*at = '\n';
		}
		else if (at[1] == 'r')
		{
			memmove(at, at + 1, strlen(at));
			*at = '\r';
		}
		else if (at[1] == 't')
		{
			memmove(at, at + 1, strlen(at));
			*at = '\t';
		}
	}
	return prefix;
}

void EmitOutput()
{
    if ((!documentMode || *ourMainOutputBuffer) && !silent) // if not in doc mode OR we had some output to say - silent when no response
    {
        // output bot response from last round
        char prefix[MAX_WORD_SIZE];
        if (*botPrefix) (*printer)((char*)"%s ", ReviseOutput(botPrefix, prefix));
    }
    if (showTopic)
    {
        GetActiveTopicName(tmpWord); // will show currently the most interesting topic
        (*printer)((char*)"(%s) ", tmpWord);
    }
    callBackDelay = 0; // now turned off after an output
    if (!silent)
    {
        ProcessOOB(ourMainOutputBuffer);
        (*printer)((char*)"%s", UTF2ExtendedAscii(ourMainOutputBuffer));
    }
    if ((!documentMode || *ourMainOutputBuffer) && !silent) (*printer)((char*)"%s", (char*)"\r\n");

    if (showWhy) (*printer)((char*)"%s", (char*)"\r\n"); // line to separate each chunk
                                                         //output user prompt
    if (documentMode || silent) { ; } // no prompt in document mode
    else if (*userPrefix)
    {
        char prefix[MAX_WORD_SIZE];
        (*printer)((char*)"%s ", ReviseOutput(userPrefix, prefix));
    }

    else (*printer)((char*)"%s", (char*)"   >");
}

bool GetInput()
{
    // start out with input

    if (ourMainInputBuffer[1]) { ; } // callback in progress, data put into buffer, read his input later, but will be out of date
    else if (documentMode)
    {
        if (!ReadDocument(ourMainInputBuffer + 1, sourceFile)) return true;
    }
    else // normal input reading
    {
        if (sourceFile == stdin) // user-based input
        {
            if (ide) return true; // only are here because input is entered
            else if (ReadALine(ourMainInputBuffer + 1, sourceFile, maxBufferSize - 100) < 0) return true; // end of input
        }
        // file based input
        else if (ReadALine(ourMainInputBuffer + 1, sourceFile, maxBufferSize - 100) < 0) return true; // end of input
    }
    if (ourMainInputBuffer[1] && !*ourMainInputBuffer) ourMainInputBuffer[0] = ' ';

    if (sourceFile != stdin)
    {
        char word[MAX_WORD_SIZE];
        if (multiuser)
        {
            char user[MAX_WORD_SIZE];
            char bot[MAX_WORD_SIZE];
            char* ptr = ReadCompiledWord(ourMainInputBuffer, user);
            ptr = ReadCompiledWord(ptr, bot);
            memmove(ourMainInputBuffer, ptr, strlen(ptr) + 1);
        }

        ReadCompiledWord(ourMainInputBuffer, word);
        if (!stricmp(word, (char*)":quit") || !stricmp(word, (char*)":exit")) return true;
        if (!stricmp(word, (char*)":debug"))
        {
            DebugCode(ourMainInputBuffer);
            return false;
        }
        if ((!*word && !documentMode) || *word == '#') return false;
        if (echoSource == SOURCE_ECHO_USER) (*printer)((char*)"< %s\r\n", ourMainInputBuffer);
    }
    return true;
}

void ProcessInputFile() // will run any number of inputs on auto, or 1 user input
{
	int turn = 0;
	while (ALWAYS) 
    {
		if (*oktest) // self test using OK or WHY as input
		{
            (*printer)((char*)"%s\r\n    ",UTF2ExtendedAscii(ourMainOutputBuffer));
			strcpy(ourMainInputBuffer,oktest);
		}
		else if (quitting) return; 
		else if (systemReset) 
		{
            (*printer)((char*)"%s\r\n",UTF2ExtendedAscii(ourMainOutputBuffer));
			*computerID = 0;	// default bot
			*ourMainInputBuffer = 0;		// restart conversation
		}
        else
        {
            if (!GetInput())
            {
                *ourMainInputBuffer = 0;
                ourMainInputBuffer[1] = 0;
                continue; // ignore input we got, already processed	
            }
            if (!*ourMainInputBuffer) break;     // failed to get input
        }
		
        // we have input, process it
		if (!server && extraTopicData)
		{
			turn = PerformChatGivenTopic(loginID, computerID, ourMainInputBuffer, NULL, ourMainOutputBuffer, extraTopicData);
			free(extraTopicData);
			extraTopicData = NULL;
		}
		else turn = PerformChat(loginID, computerID, ourMainInputBuffer, NULL, ourMainOutputBuffer); // no ip
		if (turn == PENDING_RESTART)
		{
			ourMainInputBuffer[0] = ourMainInputBuffer[1] = 0;
			Restart();
		}
        EmitOutput();
        if (sourceFile == stdin) break; // do only one
        *ourMainInputBuffer = 0;
        ourMainInputBuffer[1] = 0;
	} // input loop on sourceFile/stdinput

	if (sourceFile != stdin) 
	{
		FClose(sourceFile);  // to get here, must have been a source file that ended - cmdline or user init
		if (userInitFile && sourceFile != userInitFile) 
		{
			sourceFile = userInitFile; 
			userInitFile = NULL;	// dont do it again
            ProcessInputFile();     // now do next file
		}
		else sourceFile = stdin;
        int diff = (int)(ElapsedMilliseconds() - sourceStart);
        Log(STDUSERLOG, "Sourcefile Time used %ld ms for %d sentences %d tokens.\r\n", diff, sourceLines, sourceTokens);
    }
    *ourMainInputBuffer = 0;
    ourMainInputBuffer[1] = 0;
}

void MainLoop() //   local machine loop
{
    char user[MAX_WORD_SIZE];
    *ourMainInputBuffer = 0;
    sourceFile = stdin;
    if (*sourceInput)	sourceFile = FopenReadNormal(sourceInput);
    else if (userInitFile) sourceFile = userInitFile;
    if (!*loginID)
    {
        (*printer)((char*)"%s", (char*)"\r\nEnter user name: ");
        ReadALine(user, stdin);
        (*printer)((char*)"%s", (char*)"\r\n");
        if (*user == '*') // let human go first  -   say "*bruce
        {
            memmove(user, user + 1, strlen(user));
            (*printer)((char*)"%s", (char*)"\r\nEnter starting input: ");
            ReadALine(ourMainInputBuffer, stdin);
            (*printer)((char*)"%s", (char*)"\r\n");
        }
    }
    else strcpy(user, loginID);
    PerformChat(user, computerID, ourMainInputBuffer, NULL, ourMainOutputBuffer); // unknown bot, no input,no ip
    EmitOutput();

    while (!quitting)
    {
        *ourMainInputBuffer = ' '; // leave space at start to confirm NOT a null init message, even if user does only a cr
        ourMainInputBuffer[1] = 0;
        if (loopBackDelay) loopBackTime = ElapsedMilliseconds() + loopBackDelay; // resets every output
        while (ProcessInputDelays(ourMainInputBuffer + 1, KeyReady())) { ; }// use our fake callback input? loop waiting if no user input found
        // if callback generated message, ourMainInputBuffer will 
        ProcessInputFile(); // does all of a file or 1 from user
    }
}

void ResetToPreUser() // prepare for multiple sentences being processed - data lingers over multiple sentences
{
    ReestablishBotVariables(); // any changes user made to a variable will be reset
    ClearVolleyWordMaps();
    ResetUserChat();
    ClearUserVariables();
    ClearUserFacts();
    ResetTopicSystem(true);
    ResetSentence();
    // limitation on how many sentences we can internally resupply
	inputCounter = 0;
	totalCounter = 0;
	itAssigned = theyAssigned = 0;
	ResetTokenSystem();
	fullfloat = false;
    ReturnToAfterLayer(LAYER_BOOT, false);  // dict/fact/strings reverted and any extra topic loaded info  (but CSBoot process NOT lost)
    ClearHeapThreads(); // volley start

    ResetTopicSystem(false);
	ResetUserChat();
	ResetFunctionSystem();
	ResetTopicReply();
	currentBeforeLayer = LAYER_USER;

	inputSentenceCount = 0;
}

void ResetSentence() // read for next sentence to process from raw system level control only
{
	respondLevel = 0; 
	currentRuleID = NO_REJOINDER;	//   current rule id
 	currentRule = 0;				//   current rule being procesed
	currentRuleTopic = -1;
	ruleErased = false;	
}

void ComputeWhy(char* buffer,int n)
{
	strcpy(buffer,(char*)"Why:");
	buffer += 4;
	int start = 0;
	int end = responseIndex;
	if (n >= 0) 
	{
		start = n;
		end = n + 1;
	}
	for (int i = start; i < end; ++i) 
	{
		unsigned int order = responseOrder[i];
		int topicid = responseData[order].topic;
		if (!topicid) continue;
		char label[MAX_WORD_SIZE];
		int id;
		char* more = GetRuleIDFromText(responseData[order].id,id); // has no label in text
		char* rule = GetRule(topicid,id);
		GetLabel(rule,label);
		char c = *more;
		*more = 0;
		sprintf(buffer,(char*)"%s%s",GetTopicName(topicid),responseData[order].id); // topic and rule 
		buffer += strlen(buffer);
		if (*label) 
		{
			sprintf(buffer,(char*)"=%s",label); // label if any
			buffer += strlen(buffer);
		}
		*more = c;
		
		// was there a relay
		if (*more)
		{
			topicid = atoi(more+1); // topic number
			more = strchr(more+1,'.'); // r top level + rejoinder
			char* dotinfo = more;
			more = GetRuleIDFromText(more,id);
			char* ruleptr = GetRule(topicid,id);
			GetLabel(ruleptr,label);
			sprintf(buffer,(char*)".%s%s",GetTopicName(topicid),dotinfo); // topic and rule 
			buffer += strlen(buffer);
			if (*label)
			{
				sprintf(buffer,(char*)"=%s",label); // label if any
				buffer += strlen(buffer);
			}
		}
		strcpy(buffer,(char*)" ");
		buffer += 1;
	}
}

static void FactizeResult() // takes the initial given result
{
	uint64 control = tokenControl;
    tokenControl |= LEAVE_QUOTE;
    tokenControl &= -1 ^ SPLIT_QUOTE;
	char* limit;
    char* buffer = AllocateBuffer();
    // WARNING-- tokenize uses current user settings and
    // SHOULD be more independent -
	for (int i = 0; i < responseIndex; ++i) 
    {
		unsigned int order = responseOrder[i];
        strcpy(buffer, responseData[order].response);
		char* ptr = buffer;
        if (!*ptr) continue;
        if (i == 0) ptr = SkipOOB(ptr); // no facts out of oob data
		
		// each sentence becomes a transient fact
		while (ptr && *ptr) // find sentences of response
		{
			char* old = ptr;
			int count;
			char* starts[MAX_SENTENCE_LENGTH];
			memset(starts,0,sizeof(char*)*MAX_SENTENCE_LENGTH);
			ptr = Tokenize(ptr,count,(char**) starts,false);   //   only used to locate end of sentence but can also affect tokenFlags (no longer care)
			char c = *ptr; // is there another sentence after this?
			char c1 = 0;
			if (c)  
			{
				c1 = *(ptr-1);
				*(ptr-1) = 0; // kill the separator 
			}

			//   save sentences as facts
			char* copy = InfiniteStack(limit,"FactizeResult"); // localized
			char* out = copy;
			char* at = old-1;
			while (*++at) // copy message and alter some stuff like space or cr lf
			{
				if (*at == '\r' || *at == '\n' || *at == '\t') *out++ = ' '; // ignore special characters in fact
				else *out++ = *at;  
			}
			*out = 0;
			CompleteBindStack();
			if ((out-copy) > 2) // we did copy something, make a fact of it
			{
				char name[MAX_WORD_SIZE];
				sprintf(name,(char*)"%s.%s",GetTopicName(responseData[order].topic),responseData[order].id);
				CreateFact(MakeMeaning(StoreWord(copy,AS_IS)),Mchatoutput,MakeMeaning(StoreWord(name)),FACTTRANSIENT);
			}
			if (c) *(ptr-1) = c1;
			ReleaseStack(copy);
		}	
	}
	tokenControl = control;
    FreeBuffer();
}

static void ConcatResult(char* result,char* limit)
{
  	unsigned int oldtrace = trace;
	trace = 0;
	char* start = result;
	result[0] = 0;
	if (timerLimit && timerCheckInstance == TIMEOUT_INSTANCE)
	{
		responseIndex = 0;
		currentRule = 0;
		AddResponse((char*)"Time limit exceeded.", 0);
	}
	for (int i = 0; i < responseIndex; ++i) 
    {
		unsigned int order = responseOrder[i];
		char* reply = responseData[order].response;
        if (*reply) 
		{
			if (i == 0) reply = SkipOOB(reply);
			AddBotUsed(reply,strlen(reply));
		}
    }

	//   now join up all the responses as one output into result
	uint64 control = tokenControl;
	tokenControl |= LEAVE_QUOTE;
	for (int i = 0; i < responseIndex; ++i) 
    {
		unsigned int order = responseOrder[i];
		char* data = responseData[order].response;
        if (!*data) continue;
		size_t len = strlen(data);
		if ((result + len) >= limit) break; // dont overflow
		if (start != result)  *result++  = ENDUNIT; // add separating item from last unit for log detection
		strcpy(result,data);
		result += len;
	}
	trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
	tokenControl = control;
}

void FinishVolley(char* incoming, char* output, char* postvalue, int limit)
{
    // massage output going to user
    if (!documentMode)
    {
        if (!(responseControl & RESPONSE_NOFACTUALIZE)) FactizeResult();
        postProcessing = 1;
        ++outputNest;
        OnceCode((char*)"$cs_control_post", postvalue);
        --outputNest;
        postProcessing = 0;

        char* at = output;
        if (autonumber)
        {
            sprintf(at, (char*)"%d: ", volleyCount);
            at += strlen(at);
        }
        ConcatResult(at, output + limit - 100); // save space for after data

        time_t curr = time(0);
        if (regression) curr = 44444444;
        char* when = GetMyTime(curr); // now
        if (*incoming) strcpy(timePrior, GetMyTime(curr)); // when we did the last volley
                                                           // Log the results
        GetActiveTopicName(activeTopic); // will show currently the most interesting topic

                                         // compute hidden data
        char* buff = AllocateBuffer();
        char* origbuff = buff;
        *buff++ = (char)0xfe; // positive termination
        *buff++ = (char)0xff; // positive termination for servers
        *buff = 0;
        if (responseIndex && regression != NORMAL_REGRESSION) ComputeWhy(buff, -1);

        if (userLog && prepareMode != POS_MODE && prepareMode != PREPARE_MODE  && prepareMode != TOKENIZE_MODE)
        {
            char time15[MAX_WORD_SIZE];
            unsigned int lapsedMilliseconds = (unsigned int)(ElapsedMilliseconds() - volleyStartTime);
            sprintf(time15, (char*)" F:%dms ", lapsedMilliseconds);

            char* nl = (LogEndedCleanly()) ? (char*) "" : (char*) "\r\n";
            char* poutput = Purify(output);
            if (*incoming && regression == NORMAL_REGRESSION) Log(STDUSERLOG, (char*)"%s(%s) %s ==> %s %s\r\n", nl, activeTopic, TrimSpaces(incoming), poutput, origbuff+2); // simpler format for diff
            else if (!*incoming)
            {
                Log(STDUSERLOG, (char*)"%sStart: user:%s bot:%s ip:%s rand:%d (%s) %d ==> %s  When:%s %s Version:%s Build0:%s Build1:%s 0:%s F:%s P:%s\r\n", nl, loginID, computerID, callerIP, randIndex, activeTopic, volleyCount, poutput, when, origbuff+2, version, timeStamp[0], timeStamp[1], timeturn0, timeturn15, timePrior); // conversation start
            }
            else
            {
                Log(STDUSERLOG, (char*)"%sRespond: user:%s bot:%s ip:%s (%s) %d  %s ==> %s  When:%s %s %s\r\n", nl, loginID, computerID, callerIP, activeTopic, volleyCount, incoming, poutput, when, origbuff+2, time15);  // normal volley
            }
            if (shortPos)
            {
                Log(STDUSERLOG, (char*)"%s", DumpAnalysis(1, wordCount, posValues, (char*)"Tagged POS", false, true));
                Log(STDUSERLOG, (char*)"\r\n");
            }
        }

        // now convert output separators between rule outputs to space from ' for user display result (log has ', user sees nothing extra) 
        if (prepareMode != REGRESS_MODE)
        {
            char* sep = output + 1;
            while ((sep = strchr(sep, ENDUNIT)))
            {
                if (*(sep - 1) == ' ' || *(sep - 1) == '\n') memmove(sep, sep + 1, strlen(sep)); // since prior had space, we can just omit our separator.
                else if (!sep[1]) *sep = 0; // show nothing extra on last separator
                else if (sep[1] == ' ' && !sep[2]) *sep = 0; // show nothing extra on last separator w blank after
                else *sep = ' ';
            }
        }
        size_t lenx = strlen(output) + 1;
        strcpy(output + lenx, origbuff); // hidden why
        size_t leny = strlen(output + lenx);
        buff = output + lenx + leny + 1;
        strcpy(buff, activeTopic); // currently the most interesting topic

        FreeBuffer();
        if (debugEndTurn) (*debugEndTurn)(output);
        if (!stopUserWrite) WriteUserData(curr, false);
        else stopUserWrite = false;

        ShowStats(false);
        ResetToPreUser(); // back to empty state before any user
    }
    else
    {
        // Don't do anything after a single line of a document
        if (postProcessing)
        {
            ++outputNest;
            OnceCode((char*)" ", postvalue);
            --outputNest;
        }
        *output = 0;
    }
}
int PerformChatGivenTopic(char* user, char* usee, char* incoming,char* ip,char* output,char* topicData)
{
	CreateFakeTopics(topicData);
	int answer = PerformChat(user,usee,incoming,ip,output);
	ReleaseFakeTopics();
	return answer;
}

static void LimitInput(char* incoming)
{
    // limit sans oob
    if (incoming[0] == '[' || incoming[1] == '[' || incoming[2] == '[')
    {
        char* at = incoming - 1;
        int bracket = 0;
        bool quote = false;
        while (*++at) // input limit applies to user text, not oob
        {
            if (*at == '"' && *(at - 1) != '\\') quote = !quote;
            if (!quote && *at == '[' && *(at - 1) != '\\') ++bracket;
            else if (!quote && *at == ']' && *(at - 1) != '\\')
            {
                --bracket;
                if (!bracket) // closes oob
                {
                    at[inputLimit - 1] = 0;
                    return;
                }
            }
        }
    }
    incoming[inputLimit - 1] = 0; // none or mangled oob 
}

// WE DO NOT ALLOW USERS TO ENTER CONTROL CHARACTERS.
// INTERNAL STRINGS AND STUFF never have control characters either (/r /t /n converted only on output to user)
// WE internally use /r/n in file stuff for the user topic file.

int PerformChat(char* user, char* usee, char* incoming,char* ip,char* output) // returns volleycount or 0 if command done or -1 PENDING_RESTART
{ //   primary entrypoint for chatbot -- null incoming treated as conversation start.
    stackFree = stackStart; // begin fresh
    ResetToPreUser();
	bool resetuser = false;
	if (!strnicmp(incoming, ":reset:", 7))
	{
		resetuser = true;
		incoming += 7;
	}
   
    // protective level 0 callframe
    globalDepth = -1;
    ChangeDepth(1, ""); // never enter debugger on this
    rulesExecuted = 0;
    factsExhausted = false;
    pendingUserReset = false;
	volleyStartTime = ElapsedMilliseconds(); // time limit control
	timerCheckInstance = 0;
	modifiedTraceVal = 0;
	modifiedTrace = false;
	modifiedTimingVal = 0;
	modifiedTiming = false;
    jsonDefaults = 0;
	if (server && servertrace) trace = (unsigned int)-1;
	myBot = 0;
	if (!documentMode) {
		tokenCount = 0;
		InitJSONNames(); // reset indices for this volley
		ClearVolleyWordMaps();
		ResetEncryptTags();

		HandleBoot(FindWord("^cs_reboot"), true);
	}

	bool eraseUser = false;

	// accept a user topic file erase command
	char* x = strstr(incoming, erasename);
	if (x)
	{
		eraseUser = true;
		strncpy(x, erasename, strlen(erasename));
	}
	mainInputBuffer = incoming;
	mainOutputBuffer = output;
	size_t len = strlen(incoming);

    // absolute limit
    if (len >= maxBufferSize) incoming[maxBufferSize - 1] = 0; // chop to legal safe limit
    // relative limit on user
    if (inputLimit && inputLimit <= len) LimitInput(incoming);
    
    char* at = mainInputBuffer;
	if (tokenControl & JSON_DIRECT_FROM_OOB) at = SkipOOB(mainInputBuffer);
	char* startx = at--;
    bool quote = false;
    incoming = SkipWhitespace(incoming);
    while (*++at)
	{
		if (*at < 31) *at = ' ';
        if (*at == '"' && *(at - 1) != '\\')
        {
            if (at[1] == '"') at[1] = ' ';// dont allow doubled quotes
            quote = !quote;
            if ((!at[1] || !at[2]) && *incoming == '"') // no global quotes, (may have space at end)
            {
                *at = ' ';
                *incoming = ' ';
            }
        }
		if (*at == ' ' && !quote) // proper token separator
		{
			if ((at-startx) > (MAX_WORD_SIZE-1)) break; // trouble
			startx = at + 1;
		}
	}
	if ((at-startx) > (MAX_WORD_SIZE-1)) startx[MAX_WORD_SIZE-5] = 0; // trouble - cut him off at the knees
    
	char* first = incoming;
#ifndef DISCARDTESTING
	if (!server && !(*first == ':' && IsAlphaUTF8(first[1]))) strcpy(revertBuffer,first); // for a possible revert
#endif
    output[0] = 0;
	output[1] = 0;
	*currentFilename = 0;
	bufferIndex = baseBufferIndex; // return to default basic buffers used so far, in case of crash and recovery
	inputNest = 0; // all normal user input to start with

    //   case insensitive logins
    static char caller[MAX_WORD_SIZE];
	static char callee[MAX_WORD_SIZE];
	callee[0] = 0;
    caller[0] = 0;
	MakeLowerCopy(callee, usee);
    if (user) 
	{
		strcpy(caller,user);
		//   allowed to name callee as part of caller name
		char* separator = strchr(caller,':');
		if (separator) *separator = 0;
		if (separator && !*usee) MakeLowerCopy(callee,separator+1); // override the bot
		strcpy(loginName,caller); // original name as he typed it

		MakeLowerCopy(caller,caller);
	}
	bool hadIncoming = *incoming != 0 || documentMode;
	while (incoming && *incoming == ' ') ++incoming;	// skip opening blanks

	if (incoming[0] && incoming[1] == '#' && incoming[2] == '!') // naming bot to talk to- and whether to initiate or not - e.g. #!Rosette#my message
	{
		char* next = strchr(incoming+3,'#');
		if (next)
		{
			*next = 0;
			MakeLowerCopy(callee,incoming+3); // override the bot name (including future defaults if such)
			strcpy(incoming+1,next+1);	// the actual message.
			if (!*incoming) incoming = 0;	// login message
		}
	}

	bool fakeContinue = false;
	if (callee[0] == '&') // allow to hook onto existing conversation w/o new start
	{
		*callee = 0;
		fakeContinue = true;
	}
    Login(caller,callee,ip); //   get the participants names
	if (!*computerID && *incoming != ':') ReportBug("No computer id before user load?")

	if (trace & TRACE_MATCH) Log(STDUSERLOG, (char*)"Incoming data- %s | %s | %s\r\n", caller, (*callee) ? callee : (char*)" ", (incoming) ? incoming : (char*)"");

	if (systemReset) // drop old user
	{
		if (systemReset == 2 || resetuser)
		{
			KillShare();
			ReadNewUser(); 
		}
		else
		{
			ReadUserData();		//   now bring in user state
		}
		systemReset = 0;
	}
	else if (eraseUser) ReadNewUser();
	else if (!documentMode) 
	{
		// preserve file status reference across this read use of ReadALine
		int BOMvalue = -1; // get prior value
		char oldc;
		int oldCurrentLine;	
		BOMAccess(BOMvalue, oldc,oldCurrentLine); // copy out prior file access and reinit user file access
		ReadUserData();		//   now bring in user state
		BOMAccess(BOMvalue, oldc,oldCurrentLine); // restore old BOM values
	}
	// else documentMode
	if (fakeContinue) return volleyCount;
	if (!*computerID && *incoming != ':')  // a  command will be allowed to execute independent of bot- ":build" works to create a bot
	{
		strcpy(output,(char*)"No such bot.\r\n");
		char* fact = "no default fact";
		WORDP D = FindWord((char*)"defaultbot");
		if (!D) fact = "defaultbot word not found";
		else
		{
			FACT* F = GetObjectHead(D);
			if (!F)  fact = "defaultbot fact not found";
			else if (F->flags & FACTDEAD) fact = "dead default fact";
			else fact = Meaning2Word(F->subject)->word;
		}

		ReadComputerID(); // presume default bot log file
		CopyUserTopicFile("nosuchbot");
		if (nosuchbotrestart) pendingRestart = true;
		return 0;	// no such bot
	}

	if (!ip) ip = "";
	
	// for retry INPUT
	inputRetryRejoinderTopic = inputRejoinderTopic;
	inputRetryRejoinderRuleID = inputRejoinderRuleID;
	lastInputSubstitution[0] = 0;

	int ok = 1;
    if (!*incoming && !hadIncoming)  //   begin a conversation - bot goes first
	{
		*readBuffer = 0;
		nextInput = output;
		userFirstLine = volleyCount+1;
		*currentInput = 0;
		responseIndex = 0;
	}
	else if (*incoming == '\r' || *incoming == '\n' || !*incoming) // incoming is blank, make it so
	{
		*incoming = ' ';
		incoming[1] = 0;
	}

	// change out special or illegal characters
	char* p = incoming;
	while ((p = strchr(p,ENDUNIT))) *p = '\''; // remove special character
	p = incoming;
	while ((p = strchr(p,'\n'))) *p = ' '; // remove special character
	p = incoming;
	while ((p = strchr(p,'\r'))) *p = ' '; // remove special character
	p = incoming;
	while ((p = strchr(p,'\t'))) *p = ' '; // remove special character 
	p = incoming;
	while ((p = strchr(p, (char)0xC2))) {  // remove non breaking space
		if (p[1] == (char)0xA0) {
			*p++ = ' ';
			*p = ' ';
		}
		++p;
	}
	p = incoming;
	while ((p = strchr(p,(char)-30))) // handle curved quote
	{
		if (p[1] == (char)-128 && p[2] == (char)-103)
		{
			*p = '\'';
			memmove(p+1,p+3,strlen(p+2));
		}
		++p;
	}
	strcpy(copyInput,incoming); // so input trace not contaminated by input revisions -- mainInputBuffer is "incoming"
	ok = ProcessInput(copyInput);
	if (ok <= 0) return ok; // command processed
	
	if (!server) // refresh prompts from a loaded bot since mainloop happens before user is loaded
	{
		WORDP dBotPrefix = FindWord((char*)"$botprompt");
		strcpy(botPrefix,(dBotPrefix && dBotPrefix->w.userValue) ? dBotPrefix->w.userValue : (char*)"");
		WORDP dUserPrefix = FindWord((char*)"$userprompt");
		strcpy(userPrefix,(dUserPrefix && dUserPrefix->w.userValue) ? dUserPrefix->w.userValue : (char*)"");
	}

	// compute response and hide additional information after it about why
	FinishVolley(mainInputBuffer,output,NULL,outputsize); // use original input main buffer, so :user and :bot can cancel for a restart of concerasation
#ifndef DISCARDJAVASCRIPT
	DeleteTransientJavaScript(); // unload context if there
#endif
	return volleyCount;
}

FunctionResult Reply() 
{
    int depth = globalDepth;
	callback =  (wordCount > 1 && *wordStarts[1] == OOB_START && (!stricmp(wordStarts[2],(char*)"callback") || !stricmp(wordStarts[2],(char*)"alarm") || !stricmp(wordStarts[2],(char*)"loopback"))); // dont write temp save
	withinLoop = 0;
	choiceCount = 0;
	callIndex = 0;
	ResetOutput();
	ResetTopicReply();
	ResetReuseSafety();
    if (trace & TRACE_OUTPUT)
	{
		Log(STDUSERLOG,(char*)"\r\n\r\nReply input: ");
		for (int i = 1; i <= wordCount; ++i) Log(STDUSERLOG,(char*)"%s ",wordStarts[i]);
		Log(STDUSERLOG,(char*)"\r\n  Pending topics: %s\r\n",ShowPendingTopics());
	}
	FunctionResult result = NOPROBLEM_BIT;
	char* topicName = GetUserVariable((char*)"$cs_control_main");
	howTopic = 	 (tokenFlags & QUESTIONMARK) ? (char*) "question" : (char*)  "statement";
    CALLFRAME* frame = ChangeDepth(1, topicName);
    int pushed = PushTopic(FindTopicIDByName(topicName));
	if (pushed < 0) result = FAILRULE_BIT;
    else
    {
        result = PerformTopic(0, currentOutputBase); //   allow control to vary
        if (pushed) PopTopic();
    }
    ChangeDepth(-1, topicName); 
    if (globalDepth != depth)
    {
        ReportBug((char*)"Reply global depth %d not returned to %d", globalDepth, depth);
        globalDepth = depth;
    }
    return result;
}

void Restart()
{
	char us[MAX_WORD_SIZE];
	trace = 0;
	ClearUserVariables();
	PartiallyCloseSystem();
	CreateSystem();
	InitStandalone();
#ifdef PRIVATE_CODE
	PrivateRestart(); // must come AFTER any mongo/postgress init (to allow encrypt/decrypt override)
#endif

	ProcessArguments(argc,argv);
	strcpy(us,loginID);

#ifndef DISCARDPOSTGRES
	if (*postgresparams)  PGUserFilesCode();
#endif
#ifndef DISCARDMONGO
	if (*mongodbparams)  MongoSystemInit(mongodbparams);
#endif
#ifdef PRIVATE_CODE
	PrivateInit(privateParams); 
#endif
	EncryptInit(encryptParams);
	DecryptInit(decryptParams);
	ResetEncryptTags();

	if (!server)
	{
		echo = false;
		char* initialInput = AllocateBuffer();
		*initialInput = 0;
		PerformChat(us,computerID,initialInput,callerIP,mainOutputBuffer);
        FreeBuffer();
    }
	else 
	{
		struct tm ptm;
		Log(STDUSERLOG,(char*)"System restarted %s\r\n",GetTimeInfo(&ptm,true)); // shows user requesting restart.
	}
	pendingRestart = false;
}

int ProcessInput(char* input)
{
	startTimeInfo =  ElapsedMilliseconds();
	// aim to be able to reset some global data of user
	unsigned int oldInputSentenceCount = inputSentenceCount;
	//   precautionary adjustments
	strcpy(inputCopy,input);
	char* buffer = inputCopy;
	size_t len = strlen(input);
	if (len >= maxBufferSize) buffer[maxBufferSize -1] = 0;

#ifndef DISCARDTESTING
	char* at = SkipWhitespace(buffer);
	if (*at == '[') // oob is leading
	{
		int n = 1;
		while (n && *++at)
		{
			if (*at == '\\') ++at; // ignore \[ stuff
			else if (*at == '[') ++n;
			else if (*at == ']') --n;
		}
		at = SkipWhitespace(++at);
	}
	
	if (*at == ':' && IsAlphaUTF8(at[1]) && IsAlphaUTF8(at[2]) && !documentMode && !readingDocument) // avoid reacting to :P and other texting idioms
	{
		bool reset = false;
		if (!strnicmp(at,":reset",6)) 
		{
			reset = true;
			char* intercept = GetUserVariable("$cs_beforereset");
			if (intercept) Callback(FindWord(intercept),"()",false); // call script function first
		}
		TestMode commanded = DoCommand(at,mainOutputBuffer);
		// reset rejoinders to ignore this interruption
		outputRejoinderRuleID = inputRejoinderRuleID; 
 		outputRejoinderTopic = inputRejoinderTopic;
		if (!strnicmp(at,(char*)":retry",6) || !strnicmp(at,(char*)":redo",5))
		{
			outputRejoinderTopic = outputRejoinderRuleID = NO_REJOINDER; // but a redo must ignore pending
			strcpy(input,mainInputBuffer);
			strcpy(inputCopy,mainInputBuffer);
			buffer = inputCopy;
		}
		else if (commanded == RESTART)
		{
			pendingRestart = true;
			return PENDING_RESTART;	// nothing more can be done here.
		}
		else if (commanded == BEGINANEW)  
		{ 
			int BOMvalue = -1; // get prior value
			char oldc;
			int oldCurrentLine;	
			BOMAccess(BOMvalue, oldc,oldCurrentLine); // copy out prior file access and reinit user file access
			ResetToPreUser();	// back to empty state before user
			FreeAllUserCaches();
			ReadNewUser();		//   now bring in user state again (in case we changed user)
			BOMAccess(BOMvalue, oldc,oldCurrentLine); // restore old BOM values
			*buffer = *mainInputBuffer = 0; // kill off any input
			userFirstLine = volleyCount+1;
			*readBuffer = 0;
			nextInput = buffer;
			if (reset) 
			{
				char* intercept = GetUserVariable("$cs_afterreset");
				if (intercept) Callback(FindWord(intercept),"()",false); // call script function after
			}
			ProcessInput(buffer);
			return 2; 
		}
		else if (commanded == COMMANDED ) 
		{
			ResetToPreUser(); // back to empty state before any user
			return false; 
		}
		else if (commanded == OUTPUTASGIVEN) return true; 
		else if (commanded == TRACECMD) 
		{
			WriteUserData(time(0),true); // writes out data in case of tracing variables
			ResetToPreUser(); // back to empty state before any user
			return false; 
		}
		// otherwise FAILCOMMAND
	}
#endif
	
	if (!documentMode) 
	{
		responseIndex = 0;	// clear out data (having left time for :why to work)
		OnceCode((char*)"$cs_control_pre");
		if (responseIndex != 0) ReportBug((char*)"Not expecting PRE to generate a response")
		randIndex =  oldRandIndex;
	}
	if (*buffer) ++volleyCount;
	int oldVolleyCount = volleyCount;
	bool startConversation = !*buffer;
loopback:
	inputNest = 0; // all normal user input to start with
	lastInputSubstitution[0] = 0;
	if (trace &  TRACE_OUTPUT) Log(STDUSERLOG,(char*)"\r\n\r\nInput: %d to %s: %s \r\n",volleyCount,computerID,input);
	strcpy(currentInput,input);	//   this is what we respond to, literally.

	if (!strncmp(buffer,(char*)". ",2)) buffer += 2;	//   maybe separator after ? or !

	//   process input now
	char prepassTopic[MAX_WORD_SIZE];
	strcpy(prepassTopic,GetUserVariable((char*)"$cs_prepass"));
	nextInput = buffer;
	if (!documentMode)  AddHumanUsed(buffer);
 	int loopcount = 0;
	while (((nextInput && *nextInput) || startConversation) && loopcount < SENTENCE_LIMIT) // loop on user input sentences
	{
		nextInput = SkipWhitespace(nextInput);
		if (nextInput[0] == INPUTMARKER) // submitted by ^input `` is start,  ` is end
		{
			if (nextInput[1] == INPUTMARKER) ++inputNest;
			else --inputNest;
			nextInput += 2;
			continue;
		}
		topicIndex = currentTopicID = 0; // precaution
		FunctionResult result = DoSentence(prepassTopic,loopcount == (SENTENCE_LIMIT - 1)); // sets nextInput to next piece
		if (result == RESTART_BIT)
		{
			pendingRestart = true;
			if (pendingUserReset) // erase user file?
			{
				ResetToPreUser(); // back to empty state before any user
				ReadNewUser();
				WriteUserData(time(0),false); 
				ResetToPreUser(); // back to empty state before any user
			}
			return PENDING_RESTART;	// nothing more can be done here.
		}
		else if (result == FAILSENTENCE_BIT) // usually done by substituting a new input
		{
			inputRejoinderTopic  = inputRetryRejoinderTopic; 
			inputRejoinderRuleID = inputRetryRejoinderRuleID; 
		}
		else if (result == RETRYINPUT_BIT)
		{
			// for retry INPUT
			inputRejoinderTopic  = inputRetryRejoinderTopic; 
			inputRejoinderRuleID = inputRetryRejoinderRuleID; 
			// CANNOT DO ResetTopicSystem(); // this destroys our saved state!!!
			RecoverUser(); // must be after resettopic system
			// need to recover current topic and pending list
			inputSentenceCount = oldInputSentenceCount;
			volleyCount = oldVolleyCount;
			strcpy(inputCopy,input);
			buffer = inputCopy;
			goto loopback;
		}
		char* atptr = SkipWhitespace(buffer);
		if (*atptr != '[') ++inputSentenceCount; // dont increment if OOB message
		if (sourceFile && wordCount)
		{
			sourceTokens += wordCount;
			++sourceLines;
		}
		startConversation = false;
		++loopcount;
	}
	return true;
}

bool PrepassSentence(char* prepassTopic)
{
	howTopic = 	 (tokenFlags & QUESTIONMARK) ? (char*) "question" : (char*)  "statement";
	if (prepassTopic && *prepassTopic)
	{
		int topicid = FindTopicIDByName(prepassTopic);
		if (topicid && !(GetTopicFlags(topicid) & TOPIC_BLOCKED))
		{
            CALLFRAME* frame = ChangeDepth(1,prepassTopic);
            int pushed =  PushTopic(topicid);
			if (pushed < 0)
			{
				ChangeDepth(-1,prepassTopic);
				return false;
			}
			AllocateOutputBuffer();
			ResetReuseSafety();
			uint64 oldflags = tokenFlags;
			FunctionResult result = PerformTopic(0,currentOutputBase); 
			FreeOutputBuffer();
			ChangeDepth(-1,prepassTopic);
			if (pushed) PopTopic();
            //   subtopic ending is not a failure.
			if (result & (RESTART_BIT|ENDSENTENCE_BIT | FAILSENTENCE_BIT| ENDINPUT_BIT | FAILINPUT_BIT )) 
			{
				if (result & ENDINPUT_BIT) nextInput = "";
				--inputSentenceCount; // abort this input
                return true; 
			}
			if (prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE || trace & (TRACE_INPUT |TRACE_POS) || prepareMode == POS_MODE || (prepareMode == PENN_MODE && trace & TRACE_POS))
			{
				if (tokenFlags != oldflags) DumpTokenFlags((char*)"After prepass"); // show revised from prepass
			}
		}
	}
	return false;
}

void MoreToCome()
{
	// set %more and %morequestion
	moreToCome = moreToComeQuestion = false;
	char* at = nextInput - 1;
	while (*++at)
	{
		if (IsWhiteSpace(*at) || *at == INPUTMARKER) continue;	// ignore this junk
		moreToCome = true;	// there is more input coming
		break;
	}
	moreToComeQuestion = (strchr(nextInput, '?') != 0);
}

FunctionResult DoSentence(char* prepassTopic, bool atlimit)
{
	char* input = AllocateBuffer();  // complete input we received
	int len = strlen(nextInput);
	if (len >= (maxBufferSize)-100) nextInput[maxBufferSize-100] = 0;
	strcpy(input,nextInput);
	ambiguousWords = 0;

	if (all) Log(STDUSERLOG,(char*)"\r\n\r\nInput: %s\r\n",input);
	bool oldecho = echo;
	bool changedEcho = true;
	if (prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)  changedEcho = echo = true;

    //    generate reply by lookup first
	unsigned int retried = 0;
	sentenceRetryRejoinderTopic = inputRejoinderTopic; 
	sentenceRetryRejoinderRuleID =	inputRejoinderRuleID; 
	bool sentenceRetry = false;
	oobPossible = true;

retry:  
	char* start = nextInput; // where we read from
	if (trace & TRACE_INPUT) Log(STDUSERLOG,(char*)"\r\n\r\nInput: %s\r\n",input);
 	if (trace && sentenceRetry) DumpUserVariables(true); 
    ResetFunctionSystem();
	PrepareSentence(nextInput,true,true,false,true); // user input.. sets nextinput up to continue
	nextInput = SkipWhitespace(nextInput);

	if (!atlimit) MoreToCome();

	char nextWord[MAX_WORD_SIZE];
	ReadCompiledWord(nextInput,nextWord);
	WORDP next = FindWord(nextWord);
	if (next && next->properties & QWORD) moreToComeQuestion = true; // assume it will be a question (misses later ones in same input)
	if (changedEcho) echo = oldecho;
	
    if (PrepassSentence(prepassTopic))
    {
        FreeBuffer();
        return NOPROBLEM_BIT; // user input revise and resubmit?  -- COULD generate output and set rejoinders
    }
    if (prepareMode == PREPARE_MODE || prepareMode == POS_MODE || tmpPrepareMode == POS_MODE)
    {
        FreeBuffer();
        return NOPROBLEM_BIT; // just do prep work, no actual reply
    }
    tokenCount += wordCount;
	sentenceRetry = false;

    if (!wordCount && responseIndex != 0)
    {
        FreeBuffer();
        return NOPROBLEM_BIT; // nothing here and have an answer already. ignore this
    }
	if (showTopics)
	{
		changedEcho = echo = true;
		impliedSet = 0;
		KeywordTopicsCode(NULL);
		for (unsigned int i = 1; i <=  FACTSET_COUNT(0); ++i)
		{
			FACT* F = factSet[0][i];
			WORDP D = Meaning2Word(F->subject);
			WORDP N = Meaning2Word(F->object);
			int topicid = FindTopicIDByName(D->word);
			char* name = GetTopicName(topicid);
			Log(STDUSERLOG,(char*)"%s (%s) : (char*)",name,N->word);
			//   look at references for this topic
			int startx = -1;
			int startPosition = 0;
			int endPosition = 0;
			while (GetIthSpot(D,++startx, startPosition, endPosition)) // find matches in sentence
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
		if (changedEcho) echo = oldecho;
	}

    if (noReact)
    {
        FreeBuffer();
        return NOPROBLEM_BIT;
    }
	FunctionResult result =  Reply();
	if (result & RETRYSENTENCE_BIT && retried < MAX_RETRIES) 
	{
		inputRejoinderTopic  = sentenceRetryRejoinderTopic; 
		inputRejoinderRuleID = sentenceRetryRejoinderRuleID; 

		++retried;	 // allow  retry -- issues with this
		--inputSentenceCount;
		char* buf = AllocateStack(NULL,maxBufferSize);
		strcpy(buf,nextInput);	// protect future input
		strcpy(start,oldInputBuffer); // copy back current input
		strcat(start,(char*)" "); 
		strcat(start,buf); // add back future input
		nextInput = start; // retry old input
		sentenceRetry = true;
		ReleaseStack(buf);
		goto retry; // try input again -- maybe we changed token controls...
	}
	if (result & FAILSENTENCE_BIT)  --inputSentenceCount;
	if (result == ENDINPUT_BIT) nextInput = ""; // end future input
    FreeBuffer();
    return result;
}

void OnceCode(const char* var,char* function) //   run before doing any of his input
{
    // cannot drop stack because may be executing from ^reset(USER)
	// stackFree = stackStart; // drop any possible stack used
	withinLoop = 0;
	callIndex = 0;
	topicIndex = currentTopicID = 0; 
	char* name = (!function || !*function) ? GetUserVariable(var) : function;
	int topicid = FindTopicIDByName(name);
	if (!topicid) return;
	ResetReuseSafety();
    CALLFRAME* frame = ChangeDepth(1,name);

	int pushed = PushTopic(topicid);
	if (pushed < 0) 
	{
		ChangeDepth(-1,name);
		return;
	}
	if (trace & (TRACE_MATCH|TRACE_PREPARE) && CheckTopicTrace()) 
	{
		if (!stricmp(var,(char*)"$cs_control_pre")) 
		{
			Log(STDUSERLOG,(char*)"\r\nPrePass\r\n");
		}
		if (!stricmp(var,(char*)"$cs_externaltag")) 
		{
			Log(STDUSERLOG,(char*)"\r\nPosTagging\r\n");
		}
		else 
		{
			Log(STDUSERLOG,(char*)"\r\n\r\nPostPass\r\n");
			Log(STDUSERLOG,(char*)"Pending topics: %s\r\n",ShowPendingTopics());
		}
	}
	
	// prove it has gambits
	topicBlock* block = TI(topicid);
	if (BlockedBotAccess(topicid) || GAMBIT_MAX(block->topicMaxRule) == 0)
	{
		char word[MAX_WORD_SIZE];
		sprintf(word,"There are no gambits in topic %s for %s.",GetTopicName(topicid),var);
		AddResponse(word,0);
		ChangeDepth(-1,name);
        return;
	}
	ruleErased = false;	
	howTopic = 	(char*)  "gambit";
	PerformTopic(GAMBIT,currentOutputBase);
	ChangeDepth(-1,name);

	if (pushed) PopTopic();
	if (topicIndex) ReportBug((char*)"topics still stacked")
	if (globalDepth) 
		ReportBug((char*)"Once code %s global depth not 0",name);
	topicIndex = currentTopicID = 0; // precaution
}
		
void AddHumanUsed(const char* reply)
{
	if (!*reply) return;
	if (humanSaidIndex >= MAX_USED) humanSaidIndex = 0; // chop back //  overflow is unlikely but if he inputs >10 sentences at once, could
    unsigned int i = humanSaidIndex++;
    *humanSaid[i] = ' ';
	reply = SkipOOB((char*)reply);
	size_t len = strlen(reply);
	if (len >= SAID_LIMIT) // too long to save in user file
	{
		strncpy(humanSaid[i]+1,reply,SAID_LIMIT); 
		humanSaid[i][SAID_LIMIT] = 0; 
	}
	else strcpy(humanSaid[i]+1,reply); 
}

void AddBotUsed(const char* reply,unsigned int len)
{
	if (chatbotSaidIndex >= MAX_USED) chatbotSaidIndex = 0; // overflow is unlikely but if he could  input >10 sentences at once
	unsigned int i = chatbotSaidIndex++;
    *chatbotSaid[i] = ' ';
	if (len >= SAID_LIMIT) // too long to save fully in user file
	{
		strncpy(chatbotSaid[i]+1,reply,SAID_LIMIT); 
		chatbotSaid[i][SAID_LIMIT] = 0; 
	}
	else strcpy(chatbotSaid[i]+1,reply); 
}

bool HasAlreadySaid(char* msg)
{
    if (!*msg) return true; 
    if (!currentRule || Repeatable(currentRule) || GetTopicFlags(currentTopicID) & TOPIC_REPEAT) return false;
	msg = TrimSpaces(msg);
	size_t actual = strlen(msg);
    for (int i = 0; i < chatbotSaidIndex; ++i) // said in previous recent  volleys
    {
		size_t len = strlen(chatbotSaid[i]+1);
		if (actual > (SAID_LIMIT-3)) actual = len;
        if (!strnicmp(msg,chatbotSaid[i]+1,actual+1)) return true; // actual sentence is indented one (flag for end reads in column 0)
    }
	for (int i = 0; i  < responseIndex; ++i) // already said this turn?
	{
		char* says = SkipOOB(responseData[i].response);
		size_t len = strlen(says);
		if (actual > (SAID_LIMIT-3)) actual = len;
        if (!strnicmp(msg,says,actual+1)) return true; 
	}
    return false;
}

void FlipResponses()
{
	for (int i = 0; i < responseIndex; ++i)
	{
		if (InHeap(responseData[i].response))
		{
			responseData[i].response = AllocateStack(responseData[i].response, 0);
		}
		else
		{
			responseData[i].response = AllocateHeap(responseData[i].response, 0);
		}
	}
}

static void SaveResponse(char* msg)
{
    responseData[responseIndex].patternchoice = patternchoice;
    multichoice = NULL;
	responseData[responseIndex].response = AllocateHeap(msg,0);
    responseData[responseIndex].responseInputIndex = inputSentenceCount; // which sentence of the input was this in response to
	responseData[responseIndex].topic = currentTopicID; // what topic wrote this
	sprintf(responseData[responseIndex].id,(char*)".%d.%d",TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID)); // what rule wrote this
	if (currentReuseID != -1) // if rule was referral reuse (local) , add in who did that
	{
		sprintf(responseData[responseIndex].id + strlen(responseData[responseIndex].id),(char*)".%d.%d.%d",currentReuseTopic,TOPLEVELID(currentReuseID),REJOINDERID(currentReuseID));
	}
	responseOrder[responseIndex] = (unsigned char)responseIndex;
	responseIndex++;
	if (responseIndex == MAX_RESPONSE_SENTENCES) --responseIndex;

	// now mark rule as used up if we can since it generated text
	SetErase(true); // top level rules can erase whenever they say something
	
	if (showWhy) Log(ECHOSTDUSERLOG,(char*)"\r\n  => %s %s %d.%d  %s\r\n",(!UsableRule(currentTopicID,currentRuleID)) ? (char*)"-" : (char*)"", GetTopicName(currentTopicID,false),TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID),ShowRule(currentRule));
}

char* SkipOOB(char* buffer)
{
    // skip oob data 
    char* noOob = SkipWhitespace(buffer);
    if (*noOob == '[' || *noOob == '{') // TEMPORARY
    {
        int count = 1;
        bool quote = false;
        while (*++noOob)
        {
            if (*noOob == '"' && *(noOob - 1) != '\\') quote = !quote; // ignore within quotes
            if (quote) continue;
            if ((*noOob == '[' || *noOob == '{') && *(noOob - 1) != '\\') ++count;
            if ((*noOob == ']' || *noOob == '}') && *(noOob - 1) != '\\')
            {
                --count;
                if (count == 0)
                {
                    noOob = SkipWhitespace(noOob + 1);	// after the oob end
                    break;
                }
            }
        }
    }
    return noOob;
}

bool AddResponse(char* msg, unsigned int control)
{
	if (!msg ) return true;
    if (*msg == '`') msg = strrchr(msg, '`') + 1; // this part was already committed
	if (!*msg) return true;
    size_t len = strlen(msg);

	char* limit;
	char* buffer = InfiniteStack(limit,"AddResponse"); // localized infinite allocation
 	if ((buffer + len) > limit)
	{
		strncpy(buffer,msg,1000); // 5000 is safestringlimit
		strcpy(buffer+1000,(char*)" ... "); //   prevent trouble
		ReportBug((char*)"response too big %s...",buffer)
	}
    else strcpy(buffer,msg);
    if (buffer[len - 1] == ' ') buffer[len-1] = 0; // no trailing blank from output autospacing
    if (!timerLimit || timerCheckInstance != TIMEOUT_INSTANCE) memset(msg, '`', len); //mark message as sent

	// Do not change any oob data or test for repeat
	char* at = SkipOOB(buffer);
	if (!(control & RESPONSE_NOCONVERTSPECIAL)) ConvertNL(at);
	if (control & RESPONSE_REMOVETILDE) RemoveTilde(at);
	if (control & RESPONSE_ALTERUNDERSCORES)
	{
		Convert2Underscores(at); 
		Convert2Blanks(at);
	}
	if (control & RESPONSE_CURLYQUOTES) ConvertQuotes(at);
	if (control & RESPONSE_UPPERSTART) 	*at = GetUppercaseData(*at);

	//   remove spaces before commas (geofacts often have them in city_,_state)
	if (control & RESPONSE_REMOVESPACEBEFORECOMMA)
	{
		char* ptr = at;
		while (ptr && *ptr)
		{
			char* comma = strchr(ptr,',');
			if (comma && comma != buffer )
			{
				if (*--comma == ' ') memmove(comma,comma+1,strlen(comma));
				ptr = comma+2;
			}
			else if (comma) ptr = comma+1;
			else ptr = 0;
		}
	}
 
	if (!*at){} // we only have oob?
    else if (all || HasAlreadySaid(at) ) // dont really do this, either because it is a repeat or because we want to see all possible answers
    {
		if (all) Log(ECHOSTDUSERLOG,(char*)"Choice %d: %s  why:%s %d.%d %s\r\n\r\n",++choiceCount,at,GetTopicName(currentTopicID,false),TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID),ShowRule(currentRule));
        else if (trace & TRACE_OUTPUT) Log(STDUSERLOG,(char*)"Rejected: %s already said in %s\r\n",buffer,GetTopicName(currentTopicID,false));
		else if (showReject) Log(ECHOSTDUSERLOG,(char*)"Rejected: %s already said in %s\r\n",buffer,GetTopicName(currentTopicID,false));
 		ReleaseInfiniteStack();
		return false;
    }
    if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(STDTRACETABLOG,(char*)"Message: %s\r\n",buffer);

    SaveResponse(buffer);
 	ReleaseInfiniteStack();
    if (debugMessage) (*debugMessage)(buffer); // debugger inform

	return true;
}

void NLPipeline(int mytrace)
{
	char* original[MAX_SENTENCE_LENGTH];
	if (mytrace & TRACE_INPUT  || prepareMode) memcpy(original + 1, wordStarts + 1, wordCount * sizeof(char*));	// replicate for test
	int originalCount = wordCount;
	if (tokenControl & (DO_SUBSTITUTE_SYSTEM | DO_PRIVATE) )
	{
		// test for punctuation not done by substitutes (eg "?\")
		char c = (wordCount) ? *wordStarts[wordCount] : 0;
		if ((c == '?' || c == '!') && wordStarts[wordCount])
		{
			char* tokens[3];
			tokens[1] = AllocateHeap(wordStarts[wordCount], 1, 1);
			ReplaceWords("Remove ?!", wordCount, 1, 1, tokens);
		}

		// test for punctuation badly done at end (eg "?\")
		ProcessSubstitutes();
		if (mytrace & TRACE_INPUT  || prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)
		{
			int changed = 0;
			if (wordCount != originalCount) changed = true;
			for (int i = 1; i <= wordCount; ++i)
			{
				if (original[i] != wordStarts[i])
				{
					changed = i;
					break;
				}
			}
			if (changed)
			{
				Log(STDUSERLOG, (char*)"Substituted (");
				if (tokenFlags & DO_ESSENTIALS) Log(STDUSERLOG, "essentials ");
				if (tokenFlags & DO_SUBSTITUTES) Log(STDUSERLOG, "substitutes ");
				if (tokenFlags & DO_CONTRACTIONS) Log(STDUSERLOG, "contractions ");
				if (tokenFlags & DO_INTERJECTIONS) Log(STDUSERLOG, "interjections ");
				if (tokenFlags & DO_BRITISH) Log(STDUSERLOG, "british ");
				if (tokenFlags & DO_SPELLING) Log(STDUSERLOG, "spelling ");
				if (tokenFlags & DO_TEXTING) Log(STDUSERLOG, "texting ");
				if (tokenFlags & DO_NOISE) Log(STDUSERLOG, "noise ");
				if (tokenFlags & DO_PRIVATE) Log(STDUSERLOG, "private ");
				if (tokenFlags & NO_CONDITIONAL_IDIOM) Log(STDUSERLOG, "conditional idiom ");
				Log(STDUSERLOG, (char*)") into: ");
				for (int i = 1; i <= wordCount; ++i) Log(STDUSERLOG, (char*)"%s  ", wordStarts[i]);
				Log(STDUSERLOG, (char*)"\r\n");
				memcpy(original + 1, wordStarts + 1, wordCount * sizeof(char*));	// replicate for test
			}
			originalCount = wordCount;
		}
	}

	// if 1st token is an interjection DO NOT allow this to be a question
	if (wordCount && wordStarts[1] && *wordStarts[1] == '~' && !(tokenControl & NO_INFER_QUESTION))
		tokenFlags &= -1 ^ QUESTIONMARK;

	// special lowercasing of 1st word if it COULD be AUXVERB and is followed by pronoun - avoid DO and Will and other confusions
	if (wordCount > 1 && IsUpperCase(*wordStarts[1]) )
	{
		WORDP X = FindWord(wordStarts[1], 0, LOWERCASE_LOOKUP);
		if (X && X->properties & AUX_VERB)
		{
			WORDP Y = FindWord(wordStarts[2]);
			if (Y && Y->properties & PRONOUN_BITS) wordStarts[1] = X->word;
		}
	}

	int i, j;
	if (mytrace & TRACE_INPUT  || prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)
	{
		int changed = 0;
		if (wordCount != originalCount) changed = true;
		for (j = 1; j <= wordCount; ++j) if (original[j] != wordStarts[j]) changed = j;
		if (changed)
		{
			if (tokenFlags & DO_PROPERNAME_MERGE) Log(STDUSERLOG, (char*)"Name-");
			if (tokenFlags & DO_NUMBER_MERGE) Log(STDUSERLOG, (char*)"Number-");
			if (tokenFlags & DO_DATE_MERGE) Log(STDUSERLOG, (char*)"Date-");
			Log(STDUSERLOG, (char*)"merged: ");
			for (i = 1; i <= wordCount; ++i) Log(STDUSERLOG, (char*)"%s  ", wordStarts[i]);
			Log(STDUSERLOG, (char*)"\r\n");
			memcpy(original + 1, wordStarts + 1, wordCount * sizeof(char*));	// replicate for test
			originalCount = wordCount;
		}
	}

	// spell check unless 1st word is already a known interjection. Will become standalone sentence
	if (tokenControl & DO_SPELLCHECK && wordCount && *wordStarts[1] != '~' )
	{
		if (SpellCheckSentence()) tokenFlags |= DO_SPELLCHECK;
		if (spellTrace) {}
		else if (mytrace & TRACE_INPUT  || prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)
		{
			int changed = 0;
			if (wordCount != originalCount) changed = true;
			for (i = 1; i <= wordCount; ++i)
			{
				if (original[i] != wordStarts[i])
				{
					original[i] = wordStarts[i];
					changed = i;
				}
			}
			if (changed)
			{
				Log(STDUSERLOG, (char*)"Spelling changed into: ");
				for (i = 1; i <= wordCount; ++i) Log(STDUSERLOG, (char*)"%s  ", wordStarts[i]);
				Log(STDUSERLOG, (char*)"\r\n");
			}
			originalCount = wordCount;
		}

		if (tokenControl & (DO_SUBSTITUTE_SYSTEM | DO_PRIVATE))  ProcessSubstitutes();
		if (spellTrace) {}
		else if (mytrace & TRACE_INPUT  || prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)
		{
			int changed = 0;
			if (wordCount != originalCount) changed = true;
			for (i = 1; i <= wordCount; ++i) if (original[i] != wordStarts[i]) changed = i;
			if (changed)
			{
				Log(STDUSERLOG, (char*)"Substitution changed into: ");
				for (i = 1; i <= wordCount; ++i) Log(STDUSERLOG, (char*)"%s  ", wordStarts[i]);
				Log(STDUSERLOG, (char*)"\r\n");
			}
		}
	}
	if (tokenControl & DO_PROPERNAME_MERGE && wordCount )  ProperNameMerge();
	if (tokenControl & DO_DATE_MERGE && wordCount)  ProcessCompositeDate();
	if (tokenControl & DO_NUMBER_MERGE && wordCount )  ProcessCompositeNumber(); //   numbers AFTER titles, so they dont change a title
	if (tokenControl & DO_SPLIT_UNDERSCORE)  ProcessSplitUnderscores();
}

void PrepareSentence(char* input,bool mark,bool user, bool analyze,bool oobstart,bool atlimit) // set currentInput and nextInput
{
	unsigned int mytrace = trace;
	uint64 start_time = ElapsedMilliseconds();
	if (prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE) mytrace = 0;
	ResetSentence();
	ResetTokenSystem();
    ClearWhereInSentence();
    ClearTriedData();

	char* ptr = input;
	tokenFlags |= (user) ? USERINPUT : 0; // remove any question mark
    ptr = Tokenize(ptr,wordCount,wordStarts,false,oobstart); 
	upperCount = 0;
	lowerCount = 0;
    int repeat = 0;
	for (int i = 1; i <= wordCount; ++i)   // see about SHOUTing
	{
        if (i < wordCount) // junk input?
        {
            if (!strcmp(wordStarts[i], wordStarts[i + 1])) ++repeat;
            else repeat = 0;
        }
        if (repeatLimit && repeat >= repeatLimit)
        {
            *ptr = 0;
            *input = 0;
            wordCount = 2;
        }
		char* at = wordStarts[i] - 1;
		while (*++at)
		{
			if (IsAlphaUTF8(*at))
			{
				if (IsUpperCase(*at)) ++upperCount;
				else ++lowerCount;
			}
		}	
	}

	// set derivation data on original words of user before we do substitution
	for (int i = 1; i <= wordCount; ++i) derivationIndex[i] = (unsigned short)((i << 8) | i); // track where substitutions come from
	memcpy(derivationSentence+1,wordStarts+1,wordCount * sizeof(char*));
	derivationLength = wordCount;
	derivationSentence[wordCount+1] = NULL;

	if (oobPossible && wordCount && *wordStarts[1] == '[' && !wordStarts[1][1] && *wordStarts[wordCount] == ']'  && !wordStarts[wordCount][1]) 
	{
		oobPossible = false; // no more for now
		oobExists = true;
	}
	else oobExists = false;

 	if (tokenControl & ONLY_LOWERCASE && !oobExists) // force lower case
	{
		for (int i = FindOOBEnd(1); i <= wordCount; ++i) 
		{
			if (wordStarts[i][0] != 'I' || wordStarts[i][1]) MakeLowerCase(wordStarts[i]);
		}
	}
	
	// this is the input we currently are processing.
	*oldInputBuffer = 0;
	char* at = oldInputBuffer;
	for (int i = 1; i <= wordCount; ++i)
	{
		strcpy(at,wordStarts[i]);
		at += strlen(at);
		*at++ = ' ';
	}
	*at = 0;

	// check for all uppercase (capslock)
	for (int i = 1; i <= wordCount; ++i)  originalCapState[i] = IsUpperCase(*wordStarts[i]); // note cap state
	bool lowercase = false;
	if (!oobExists)
	{
		for (int i = 1; i <= wordCount; ++i)
		{
			char* word = wordStarts[i];
			size_t len = strlen(word);
			for (int j = 0; j < (int)len; ++j)
			{
				if (IsLowerCase(word[j])) // lower case letter detected?
				{
					lowercase = true;
					i = j = len + 1000; // len might be BIG (oob data) so make sure beyond it)
				}
			}
		}

		if (!lowercase && wordCount > 2) // must have multiple words all in uppercase
		{
			for (int i = 1; i <= wordCount; ++i)
			{
				char* word = wordStarts[i];
				char myword[MAX_WORD_SIZE];
				MakeLowerCopy(myword, word);
				WORDP D = StoreWord(myword);
				wordStarts[i] = D->word;
				originalCapState[i] = false;
			}
		}

		// force Lower case on plural start word which has singular meaning (but for substitutes
		if (wordCount)
		{
			char word[MAX_WORD_SIZE];
			MakeLowerCopy(word, wordStarts[1]);
			size_t len = strlen(word);
			if (strcmp(word, wordStarts[1]) && word[1] && word[len - 1] == 's') // is a different case and seemingly plural
			{
				WORDP O = FindWord(word, len, UPPERCASE_LOOKUP);
				WORDP D = FindWord(word, len, LOWERCASE_LOOKUP);
				if (D && D->properties & PRONOUN_BITS) { ; } // dont consider hers and his as plurals of some noun
				else if (O && O->properties & NOUN) { ; }// we know this noun (like name James)
				else
				{
					char* singular = GetSingularNoun(word, true, false);
					D = FindWord(singular);
					if (D && stricmp(singular, word)) // singular exists different from plural, use lower case form
					{
						D = StoreWord(word); // lower case plural form
						if (D->internalBits & UPPERCASE_HASH) AddProperty(D, NOUN_PROPER_PLURAL | NOUN);
						else AddProperty(D, NOUN_PLURAL | NOUN);
						wordStarts[1] = D->word;
					}
				}
			}
		}
	}
 	if (mytrace & TRACE_INPUT || prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)
	{
		Log(STDUSERLOG,(char*)"TokenControl: ");
		DumpTokenControls(tokenControl);
		Log(STDUSERLOG,(char*)"\r\n\r\n");
		if (tokenFlags & USERINPUT) Log(STDUSERLOG,(char*)"\r\nOriginal User Input: %s\r\n",input);
		else Log(STDUSERLOG,(char*)"\r\nOriginal Chatbot Output: %s\r\n",input);
		Log(STDUSERLOG,(char*)"Tokenized into: ");
		for (int i = 1; i <= wordCount; ++i) Log(STDUSERLOG,(char*)"%s  ",wordStarts[i]);
		Log(STDUSERLOG,(char*)"\r\n");
	}
	
	if (!oobExists) NLPipeline(mytrace);
    else
    {
        for (int i = 1; i <= wordCount; ++i)
        {
            wordCanonical[i] = wordStarts[i];
            MarkWordHit(0, false, StoreWord(wordStarts[i], AS_IS), 0, i, i);
        }
    }
	if (!analyze) nextInput = ptr;	//   allow system to overwrite input here
	int i, j;
	if (!oobExists && tokenControl & DO_INTERJECTION_SPLITTING && wordCount > 1 && *wordStarts[1] == '~') // interjection. handle as own sentence
	{
		// formulate an input insertion
		char* buffer = AllocateBuffer(); // needs to be able to hold all input queued
		*buffer = 0;
		int more = (derivationIndex[1] & 0x000f) + 1; // at end
		if (more <= derivationLength && *derivationSentence[more] == ',') // swallow comma into original
		{
			++more;	// dont add comma onto input
			derivationIndex[1]++;	// extend end onto comma
		}
		for (i = more; i <= derivationLength; ++i) // rest of data after input.
		{
			strcat(buffer,derivationSentence[i]);
			strcat(buffer,(char*)" ");
		}

		// what the rest of the input had as punctuation OR insure it does have it
		char* end = buffer + strlen(buffer);
		char* tail = end - 1;
		while (tail > buffer && *tail == ' ') --tail; 
		if (tokenFlags & QUESTIONMARK && *tail != '?') strcat(tail,(char*)"? (char*)");
		else if (tokenFlags & EXCLAMATIONMARK && *tail != '!' ) strcat(tail,(char*)"! ");
		else if (*tail != '.') strcat(tail,(char*)". ");

		if (!analyze) 
		{
			size_t len = strlen(nextInput);
			size_t len1 = strlen(buffer);
			if ((len + len1 + 10) < maxBufferSize) // safe to swallow
			{
				strcpy(end, nextInput); // a copy of rest of input
				strcpy(nextInput, buffer); // unprocessed user input is here
				ptr = nextInput;
			}
		}
		FreeBuffer();
		wordCount = 1;
		tokenFlags |= DO_INTERJECTION_SPLITTING;
	}
	
	// copy the raw sentence original input that this sentence used
	*rawSentenceCopy = 0;
	char* atword = rawSentenceCopy;
	int reach = 0;
	for (i = 1; i <= wordCount; ++i)
	{
		int start = derivationIndex[i] >> 8;
		int end = derivationIndex[i] & 0x00ff;
		if (start > reach)
		{
			reach = end;
			for (j = start; j <= reach; ++j)
			{
				strcpy(atword, derivationSentence[j]);
				atword += strlen(atword);
				if (j < derivationLength) *atword++ = ' ';
				*atword = 0;
			}
		}
	}
	
	if (mytrace & TRACE_INPUT  || prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)
	{
		Log(STDUSERLOG,(char*)"Actual used input: ");
		for (i = 1; i <= wordCount; ++i) 
		{
			Log(STDUSERLOG,(char*)"%s",wordStarts[i]);
			int start = derivationIndex[i] >> 8;
			int end = derivationIndex[i] & 0x00ff;
			if (start == end && wordStarts[i] == derivationSentence[start]) {;} // unchanged from original
			else // it came from somewhere else
			{
				start = derivationIndex[i] >> 8;
				end = derivationIndex[i] & 0x00Ff;
				Log(STDUSERLOG,(char*)"(");
				for (j = start; j <= end; ++j)
				{
					if (j != start) Log(STDUSERLOG,(char*)" ");
					Log(STDUSERLOG,(char*)"%s",derivationSentence[j]);
				}
				Log(STDUSERLOG,(char*)")");
			}
			Log(STDUSERLOG,(char*)" ");
		}
		Log(STDUSERLOG,(char*)"\r\n\r\n");
	}

	if (echoSource == SOURCE_ECHO_LOG) 
	{
		Log(ECHOSTDUSERLOG,(char*)"  => ");
		for (i = 1; i <= wordCount; ++i) Log(STDUSERLOG,(char*)"%s  ",wordStarts[i]);
		Log(ECHOSTDUSERLOG,(char*)"\r\n");
	}

	wordStarts[0] = wordStarts[wordCount+1] = AllocateHeap((char*)""); // visible end of data in debug display
	wordStarts[wordCount+2] = 0;
    if (!oobExists && mark && wordCount) MarkAllImpliedWords();

	if (prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE || trace & TRACE_POS || prepareMode == POS_MODE || (prepareMode == PENN_MODE && trace & TRACE_POS)) DumpTokenFlags((char*)"After parse");
	if (timing & TIME_PREPARE) {
		int diff = (int)(ElapsedMilliseconds() - start_time);
		if (timing & TIME_ALWAYS || diff > 0) Log(STDTIMELOG, (char*)"Prepare %s time: %d ms\r\n", input, diff);
	}
}

#ifndef NOMAIN
int main(int argc, char * argv[]) 
{
    *crashpath = 0;
	for (int i = 1; i < argc; ++i)
	{
		if (!strnicmp(argv[i],"root=",5)) 
		{
#ifdef WIN32
			SetCurrentDirectory((char*)argv[i]+5);
#else
			chdir((char*)argv[i]+5);
#endif
		}
	}

	if (InitSystem(argc,argv)) myexit((char*)"failed to load memory\r\n");
    ResetToPreUser();
    if (!server) 
	{
		quitting = false; // allow local bots to continue regardless
		MainLoop();
	}
	else if (quitting) {;} // boot load requests quit
#ifndef DISCARDSERVER
    else
    {
#ifdef EVSERVER
        if (evsrv_run() == -1) Log(SERVERLOG, "evsrv_run() returned -1");
#else
        InternetServer();
#endif
    }
#endif
	CloseSystem();
}
#endif
