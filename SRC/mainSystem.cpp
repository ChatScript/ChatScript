#include "common.h" 
#include "evserver.h"
char* version = "11.5";
char sourceInput[200];
char* releaseBotVar = NULL;
FILE* userInitFile;
unsigned int parseLimit = 0;
bool jabugpatch = false;
bool fastload = true;
bool hadAuthCode;
bool blockapitrace = false;
static bool crnl_safe = false;
int externalTagger = 0;
char defaultbot[100];
char serverlogauthcode[30] ;
uint64 chatstarted = 0;
char websocketparams[300];
char websocketmessage[300];
char jmeter[100];
int sentenceloopcount = 0;
static uint64 lastCrashTime = 0;
int autorestartdelay = 0;
int db = 0;
int gzip = 0;
bool client = false;
bool newuser = false;
bool restartfromdeath = false;
char myip[100];
int stdlogging = 0;
uint64 preparationtime = 0;
uint64 replytime = 0;
unsigned int timeLog = 2147483647;
bool autoreload = true;
bool sentenceOverflow = false;
int sentenceLimit = SENTENCES_LIMIT;
bool loadingUser = false;
bool dieonwritefail = false;
unsigned int inputLimit = 0;
unsigned int fullInputLimit = 0;
char traceuser[500];
char* originalUserInput = NULL;
bool debugcommand = false;
int traceUniversal;
int debugLevel = 0;
PRINTER printer = printf;
uint64 startNLTime;
unsigned int idetrace = (unsigned int) -1;
int outputlevel = 0;
static bool argumentsSeen = false;
// parameters
int argc;
char ** argv;
char* configFile = "cs_init.txt";	// can set config params
char* configFile2 = "cs_initmore.txt";	// can set config params
char* configLines[MAX_WORD_SIZE];	// can set config params
char language[40];							// indicate current language used
char livedataFolder[500];		// where is the livedata folder
char languageFolder[500];		// where is the livedata language folder
char systemFolder[500];		// where is the livedata system folder
char usersfolder[100];
char logsfolder[100];
char topicfolder[100];
static char* privateParams = NULL;
char treetaggerParams[200];
static char encryptParams[300];
static char decryptParams[300];
char buildfiles[100];
char apikey[100];
int forkcount = 1;
char newusername[100] = "";
char  erasename[100] = "csuser_erase";
char buildflags[100] = "";
bool noboot = false;
static bool traceboot = false;
char tmpfolder[100];
static char* configUrl = NULL;
static char* configHeaders[20];
static int headerCount = 0;

int configLinesLength;	// can set config params
bool build0Requested = false;
bool build1Requested = false;
bool servertrace = false;
static int repeatLimit = 0;
bool pendingRestart = false;
bool pendingUserReset = false;
bool rebooting = false;
int outputchoice = -1;
DEBUGAPI debugInput = NULL;
DEBUGAPI debugOutput = NULL;
DEBUGAPI debugEndTurn = NULL;
DEBUGLOOPAPI debugCall = NULL;
DEBUGVARAPI debugVar = NULL;
DEBUGVARAPI debugMark = NULL;
DEBUGAPI debugMessage = NULL;
DEBUGAPI debugAction = NULL;
#define MAX_RETRIES 20
char* revertBuffer;			// copy of user input so can :revert if desired
bool postProcessing = false;						
unsigned int tokenCount;						// for document performc
bool callback = false;						// when input is callback,alarm,loopback, dont want to enable :retry for that...
uint64 timerLimit = 0;						// limit time per volley
int timerCheckRate = 0;					// how often to check calls for time
uint64 volleyStartTime = 0;
char forkCount[10];
int timerCheckInstance = 0;
char hostname[100];
bool logline = false;
bool nosuchbotrestart = false; // restart if no such bot
char* derivationSentence[MAX_SENTENCE_LENGTH];
char derivationSeparator[MAX_SENTENCE_LENGTH];
int derivationLength;
char authorizations[200];	// for allowing debug commands
char* ourMainInputBuffer = NULL;				// user input buffer - ptr to primaryInputBuffer
char* mainInputBuffer;	// exernal buffer calling us
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

unsigned short int derivationIndex[MAX_SENTENCE_LENGTH];

bool scriptOverrideAuthorization = false;

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

char* extraTopicData = NULL;				// supplemental topic set by :extratopic

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
uint64 tokenControl = 0;						// results from tokenization and prepare processing
unsigned int responseControl = ALL_RESPONSES;					// std output behaviors

// general display and flow controls
bool quitting = false;							// intending to exit chatbot
ResetMode buildReset = NO_RESET;		// intending to reload system 
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
	
// outputs generated
RESPONSE responseData[MAX_RESPONSE_SENTENCES+1];
unsigned char responseOrder[MAX_RESPONSE_SENTENCES+1];
int responseIndex;

int inputSentenceCount;				// which sentence of volley is this
static void HandleReBoot(WORDP boot,bool reboot);

///////////////////////////////////////////////
/// SYSTEM STARTUP AND SHUTDOWN
///////////////////////////////////////////////

static void HandlePermanentBuffers(bool init)
{
	if (init)
	{
		if (fullInputLimit == 0) fullInputLimit = maxBufferSize * 2;
		readBuffer = (char*)malloc(fullInputLimit);
		*readBuffer = 0;
		lastInputSubstitution = (char*)malloc(maxBufferSize);
		rawSentenceCopy = (char*)malloc(maxBufferSize);
		revertBuffer = (char*)malloc(maxBufferSize);

		ourMainInputBuffer = (char*)malloc(fullInputLimit);  // precaution for overflow

		currentOutputLimit = outputsize;
		currentRuleOutputBase = currentOutputBase = ourMainOutputBuffer = (char*)malloc(outputsize);

		// need allocatable buffers for things that run ahead like servers and such.
		maxBufferSize = (maxBufferSize + 63);
		maxBufferSize &= 0xffffffc0; // force 64 bit align on size  
		unsigned int total = maxBufferLimit * maxBufferSize;
		buffers = (char*)malloc(total); // have it around already for messages
		if (!buffers)
		{
			(*printer)((char*)"%s", (char*)"cannot allocate buffer space");
			myexit((char*)"FATAL: cannot allocate buffer space");
		}
		bufferIndex = 0;
		baseBufferIndex = bufferIndex;
	}
	else
	{
		free(readBuffer);
		free(lastInputSubstitution);
		free(rawSentenceCopy);
		free(revertBuffer);
		free(ourMainInputBuffer);

		free(ourMainOutputBuffer);
		// free(buffers);  see CloseBuffers()
	}
}

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
        if (server && debugLevel > 0) Log(SERVERLOG,"botvariable: %s = %s\r\n", word, eq + 1);
        else if(debugLevel > 0) (*printer )((char*)"botvariable: %s = %s\r\n", word, eq + 1);
        NoteBotVariables(); // these go into level 1
        LockLayer(false);
        // need to restore the initial markers so that a restart can process them
        *eq = '=';
        *word = 'V';
    }
}

static void RelocateDeadBootFacts(FACT* F)
{
	// move dead/transient facts to end
	FACT* tailArea = lastFactUsed + 1; // last fact
	FACT* G = NULL;
	while (++F <= lastFactUsed)
	{
		if (!(F->flags & (FACTTRANSIENT | FACTDEAD)))
		{
			F->flags |= FACTBUILD1;
			continue;
		}
		else if (!F->subject) continue;  // already moved this one
		UnweaveFact(F);
		bool dead = true;

		if (!G) G = F;
		while (++G < tailArea)
		{
			if ((G->flags & (FACTTRANSIENT | FACTDEAD))) continue;
			if (!G->subject) break;  // already dead
			UnweaveFact(G); // cut from from dictionary referencing
			memcpy(F, G, sizeof(FACT)); // replicate fact lower
			WeaveFact(F); // rebind fact to dictionary
			F->flags |= FACTBUILD1;
			G->flags |= FACTDEAD;
			dead = false;
			break;
		}
		if (dead)
		{
			// kill this completely in case we cant overwrite it with valid fact
			F->subject = F->verb = F->object = 0;
			F->flags |= FACTDEAD;
		}
	}
	while (!lastFactUsed->subject) --lastFactUsed; // reclaim
}

static void HandleReBoot(WORDP boot, bool volleyboot)
{ 
	if (!boot || noboot)
	{// noboot is command line paramenter to block use of boot
		rebooting = false;
		return;
	}
	// boot== ^csboot on startup (reboot==false) vs ^cs_reboot per volley (reboot==true) 
	if (volleyboot) rebooting = true; // global awareness we are rebooting vs system startup
	else // system startup boot
	{
	}
	// run script on startup of system. data it generates will also be layer 1 data
	int oldtrace = trace;
	int oldtiming = timing;
	if (traceboot) trace = (unsigned int) -1;
	if (*bootcmd) DoCommand(bootcmd, NULL, false); // run this command  before CSBOOT is run. eg to trace boot process
		
	if (!volleyboot) UnlockLayer(LAYER_BOOT); // unlock it to add stuff
	else 	currentBeforeLayer = LAYER_BOOT; // be in user layer (before is boot layer)
	FACT* F = lastFactUsed; // note facts allocated before running boot
	Callback(boot, (char*)"()", true, true); // do before world is locked
	*ourMainOutputBuffer = 0; // remove any boot message
	myBot = 0;	// restore fact owner to generic all
       // cs_reboot may call boot to reload layer, this flag
       // rebooting might be wrong...
	if (!volleyboot)
	{	
		RelocateDeadBootFacts(F);
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
	rebooting = false;
}

unsigned int CountWordsInBuckets(unsigned int& unused, unsigned int* depthcount,int limit)
{
	memset(depthcount, 0, sizeof(int)*limit);
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
		words += n;  // n is number of words in this bucket
		if (n && n < limit) 
			depthcount[n] += n; // this many words have a bucket access of n
	}
	return words;
}

static void ReadBotVariable(char* file)
{
	char buffer[5000];
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadOnly(file);
	if (in)
	{
		while (ReadALine(buffer, in, MAX_WORD_SIZE) >= 0)
		{
			ReadCompiledWord(buffer, word);
			if (*word == 'V') SetBotVariable(word); // these are level 1 values
		}
		fclose(in);
	}
}

void CreateSystem()
{
	uint64 starttime = ElapsedMilliseconds();

    tableinput = NULL;
	scriptOverrideAuthorization = false;	// always start safe
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
			myexit((char*)"FATAL: cannot allocate buffer space");
		}
		bufferIndex = 0;
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
	if (server)  Log(SERVERLOG,"Server %s\r\n",data);
	strcat(data,(char*)"\r\n");
    (*printer)((char*)"%s",data);

	int oldtrace = trace; // in case trace turned on by default
	trace = 0;
	*oktest = 0;

	sprintf(data,(char*)"Params:   dict:%lu words fact:%lu facts text:%lukb hash:%lu \r\n",(unsigned long )maxDictEntries,(unsigned long)maxFacts,(unsigned long)(maxHeapBytes/1000),(unsigned long)maxHashBuckets);
	if (server) Log(SERVERLOG,"%s",data);
	else (*printer)((char*)"%s",data);
	sprintf(data,(char*)"          buffer:%ux%ukb cache:%ux%ukb userfacts:%u outputlimit:%u loglimit:%u\r\n",(unsigned int)maxBufferLimit,(unsigned int)(maxBufferSize/1000),(unsigned int)userCacheCount,(unsigned int)(userCacheSize/1000),(unsigned int)userFactCount,
		outputsize,logsize);
	if (server) Log(SERVERLOG,"%s",data);
	else (*printer)((char*)"%s",data);

	// in case
	MakeDirectory(topicfolder);
	char name[MAX_WORD_SIZE];
	sprintf(name, "%s/BUILD0", topicfolder);
	MakeDirectory(name);
	sprintf(name, "%s/BUILD1", topicfolder);
	MakeDirectory(name);

	LoadSystem();			// builds base facts and dictionary (from wordnet)
	*currentFilename = 0;
    *debugEntry = 0;

	char word[MAX_WORD_SIZE];
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
	trace = oldtrace; // allow boot tracing
#ifndef DISCARDMONGO
	strcpy(dbparams, mongodbparams);
#endif
#ifndef DISCARDPOSTGRES
	strcpy(dbparams, postgresparams);
#endif
#ifndef DISCARDMYSQL
	strcpy(dbparams, mysqlparams);
#endif
#ifndef DISCARDMICROSOFTSQL
	strcpy(dbparams, mssqlparams);
#endif
	HandleReBoot(FindWord((char*)"^csboot"),false);// run script on startup of system. data it generates will also be layer 1 data
	LockLayer(true);
    currentBeforeLayer = LAYER_BOOT;
	InitSpellCheck(); // after boot vocabulary added

	unsigned int factUsedMemKB = ( lastFactUsed-factBase) * sizeof(FACT) / 1000;
	unsigned int lastFactUsedMemKB = ( factEnd-lastFactUsed) * sizeof(FACT) / 1000;
	unsigned int dictUsedMemKB = ( dictionaryFree-dictionaryBase) * sizeof(WORDENTRY) / 1000;
	// dictfree shares text space
	unsigned int textUsedMemKB = ( heapBase-heapFree)  / 1000;
	unsigned int textFreeMemKB = ( heapFree- heapEnd) / 1000;

	unsigned int bufferMemKB = (maxBufferLimit * maxBufferSize) / 1000;
	
	unsigned int used =  factUsedMemKB + dictUsedMemKB + textUsedMemKB + bufferMemKB;
	used +=  (userTopicStoreSize + userTableSize) /1000;
	unsigned int free = lastFactUsedMemKB + textFreeMemKB;

	unsigned int bytes = (tagRuleCount * MAX_TAG_FIELDS * sizeof(uint64)) / 1000;
	used += bytes;
	char buf2[MAX_WORD_SIZE];
	char buf1[MAX_WORD_SIZE];
	char buf[MAX_WORD_SIZE];
	strcpy(buf,StdIntOutput(lastFactUsed-factBase));
	strcpy(buf2,StdIntOutput(textFreeMemKB));
	unsigned int depthcount[100];
	unsigned int unused;
	unsigned int words = CountWordsInBuckets(unused, depthcount,100);

	char depthmsg[1000];
	int x = 100;
	while (!depthcount[--x]); // find last used slot
	char* at = depthmsg;
	*at = 0;
	for (int i = 1; i <= x; ++i)
	{
		sprintf(at, "%u.", depthcount[i]);
		at += strlen(at);
	}

	sprintf(data, (char*)"Used %uMB: dict %s (%ukb) hashdepths %s words %u unusedbuckets %u fact %s (%ukb) heap %ukb\r\n",
		used / 1000,
		StdIntOutput(dictionaryFree - dictionaryBase - 1),
		dictUsedMemKB, depthmsg,words,unused,
		buf,
		factUsedMemKB,
		textUsedMemKB);
	if (server) Log(SERVERLOG,"%s",data);
	else (*printer)((char*)"%s",data);

	sprintf(data,(char*)"           buffer (%ukb) cache (%uWkb)\r\n",
		bufferMemKB,
		(userTopicStoreSize + userTableSize)/1000);
	if(server) Log(SERVERLOG,"%s",data);
	else (*printer)((char*)"%s",data);

	strcpy(buf,StdIntOutput(factEnd-lastFactUsed)); // unused facts
	strcpy(buf1,StdIntOutput(textFreeMemKB)); // unused text
	strcpy(buf2,StdIntOutput(free/1000));

	sprintf(data,(char*)"Free %sMB: dict %s fact %s stack/heap %sKB \r\n\r\n",
		buf2,StdIntOutput(((unsigned int)maxDictEntries)-(dictionaryFree-dictionaryBase)),buf,buf1);
	if (server) Log(SERVERLOG,"%s",data);
	else (*printer)((char*)"%s",data);

	trace = oldtrace;
#ifdef DISCARDSERVER 
    (*printer)((char*)"    Server disabled.\r\n");
#endif
#ifdef DISCARDSCRIPTCOMPILER
	if(server) Log(SERVERLOG,"    Script compiler disabled.\r\n");
	else (*printer)((char*)"    Script compiler disabled.\r\n");
#endif
#ifdef DISCARDTESTING
	if(server) Log(SERVERLOG,"    Testing disabled.\r\n");
	else (*printer)((char*)"    Testing disabled.\r\n");
#else
	*callerIP = 0;
	if (server && VerifyAuthorization(FopenReadOnly((char*)"authorizedIP.txt"))) // authorizedIP
	{
		Log(SERVERLOG,"    *** Server WIDE OPEN to :command use.\r\n");
	}
#endif
	char route[100];
	*route = 0;
#ifdef DISCARDJSONOPEN
	sprintf(route, (char*)"%s", (char*)"    JSONOpen access disabled.\r\n");
	if (server) Log(SERVERLOG, route);
	else (*printer)(route);
#endif
#ifdef TREETAGGER
	if (!treetaggerfail)
	{
		sprintf(route, (char*)"    TreeTagger access enabled: %s\r\n", language);
		if (server) Log(SERVERLOG, route);
		else (*printer)(route);
	}
#endif
#ifndef DISCARDMICROSOFTSQL
	if (gzip)
	{
		sprintf(route, (char*)"    Text Compression enabled\r\n");
		if (server) Log(SERVERLOG, route);
		else (*printer)(route);
	}
#endif
    (*printer)((char*)"%s",(char*)"\r\n");
	loading = false;
	printf("System created in %u ms\r\n", (unsigned int)(ElapsedMilliseconds() - starttime));
}

void LoadSystem()
{//   reset the basic system 
    myBot = 0; // facts loaded here are universal
	originalUserInput = NULL;
	InitFacts(); // malloc space
	InitStackHeap(); // malloc space
	InitTextUtilities(); // also part of basic system before a build
	InitDictionary(); // malloc space
	LoadDictionary(heapFree);
	InitTextUtilities1(); // also part of basic system before a build
	InitUserCache(); // malloc space
	InitFunctionSystem(); // modify dictionary entries
	InitScriptSystem();
	InitVariableSystem();
	InitSystemVariables();
	ReadLiveData(); // below layer 0
#ifdef TREETAGGER
	 // We will be reserving some working space for treetagger on the heap
	 // so make sure it is tucked away where it cannot be touched
	// NOT IN DEBUGGER WILL FORCE reloads of layers
	InitTreeTagger(treetaggerParams);
#endif
	WordnetLockDictionary();

	InitTopicSystem();		// dictionary reverts to wordnet zone
	ReadBotVariable(configFile);
	ReadBotVariable(configFile2);
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
#ifndef NOARGTRACE
    (*printer)("    %s\r\n",arg);
#endif
	if (!strnicmp(arg, (char*)"trace", 5))
	{
		if (arg[5] == '=') trace = atoi(arg + 6);
		else trace = (unsigned int)-1;
	}
	else if (!strnicmp(arg, (char*)"language=", 9))
	{
		strcpy(language, arg + 9);
		MakeUpperCase(language);
	}
	else if (!strnicmp(arg, (char*)"buildflags=", 11)) 		CopyParam(buildflags, arg + 11, 100);
	else if (!strnicmp(arg, (char*)"buffer=", 7))  // number of large buffers available  8x80000
	{
		maxBufferLimit = atoi(arg + 7);
		char* size = strchr(arg + 7, 'x');
		if (size) maxBufferSize = atoi(size + 1) * 1000;
		if (maxBufferSize < OUTPUT_BUFFER_SIZE)
		{
			(*printer)((char*)"Buffer cannot be less than OUTPUT_BUFFER_SIZE of %d\r\n", OUTPUT_BUFFER_SIZE);
			myexit((char*)"FATAL buffer size less than output buffer size");
		}
	}
	else if (!strnicmp(arg, (char*)"timer=", 6))
	{
		char* x = strchr(arg + 6, 'x'); // 15000x10
		if (x)
		{
			*x = 0;
			timerCheckRate = atoi(x + 1);
		}
		timerLimit = atoi(arg + 6);
	}
	else if (!stricmp(arg, (char*)"trustpos")) trustpos = true;
	else if (!strnicmp(arg, (char*)"inputlimit=", 11)) inputLimit = atoi(arg + 11);
	else if (!strnicmp(arg, (char*)"fullInputlimit=", 15)) fullInputLimit = atoi(arg + 15);
	else if (!strnicmp(arg, (char*)"debug=", 6)) CopyParam(debugEntry, arg + 6, 1000);
	else if (!strnicmp(arg, "erasename=", 10)) CopyParam(erasename, arg + 10, 100);
	else if (!strnicmp(arg, "cs_new_user=", 12)) CopyParam(newusername, arg + 12, 100);
	else if (!stricmp(arg, "userencrypt")) userEncrypt = true;
	else if (!stricmp(arg, "ltmencrypt")) ltmEncrypt = true;
	else if (!strnicmp(arg, "defaultbot=", 11)) CopyParam(defaultbot, arg + 11, 100);
	else if (!strnicmp(arg, "traceuser=", 10)) CopyParam(traceuser, arg + 10);
	else if (!stricmp(arg, (char*)"noboot")) noboot = true;
	else if (!stricmp(arg, (char*)"noretrybackup")) noretrybackup = true;
	else if (!stricmp(arg, (char*)"traceboot")) traceboot = true;
	else if (!stricmp(arg, (char*)"trace")) trace = (unsigned int)-1; // make trace work on login
	else if (!stricmp(arg, (char*)"time")) timing = (unsigned int)-1 ^ TIME_ALWAYS; // make timing work on login
	else if (!stricmp(arg, (char*)"recordboot")) recordBoot = true;
	else if (!stricmp(arg, (char*)"servertrace")) servertrace = true;
	else if (!strnicmp(arg, (char*)"repeatlimit=", 12)) repeatLimit = atoi(arg + 12);
	else if (!strnicmp(arg, (char*)"apikey=", 7)) CopyParam(apikey, arg + 7);
	else if (!strnicmp(arg, (char*)"logsize=", 8)) logsize = atoi(arg + 8); // bytes avail for log buffer
	else if (!strnicmp(arg, (char*)"loglimit=", 9)) loglimit = atoi(arg + 9); // max mb of log file before change files
	else if (!strnicmp(arg, (char*)"outputsize=", 11)) outputsize = atoi(arg + 11); // bytes avail for output buffer
	else if (!stricmp(arg, (char*)"time")) timing = (unsigned int)-1 ^ TIME_ALWAYS;
	else if (!strnicmp(arg, (char*)"bootcmd=", 8)) CopyParam(bootcmd, arg + 8);
#ifdef WIN32 
	else if (!strnicmp(arg, (char*)"windowsbuglog=", 14)) CopyParam(externalBugLog, arg + 14);
#else
	else if (!strnicmp(arg, (char*)"linuxbuglog=", 12)) CopyParam(externalBugLog, arg + 12);
#endif


	else if (!strnicmp(arg, (char*)"dir=", 4))
	{
#ifdef WIN32
		if (!SetCurrentDirectory(arg + 4)) (*printer)((char*)"unable to change to %s\r\n", arg + 4);
#else
		chdir(arg + 5);
#endif
	}
	else if (!strnicmp(arg, (char*)"source=", 7))  CopyParam(sourceInput, arg + 7);
	else if (!strnicmp(arg,(char*)"login=",6))  CopyParam(loginID,arg+6);
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
	else if (!strnicmp(arg,(char*)"debuglevel=",11)) debugLevel = atoi(arg+11);  // log level entries
	else if (!strnicmp(arg,(char*)"cache=",6)) // value of 10x0 means never save user data
	{
		userCacheSize = atoi(arg+6) * 1000;
		char* number = strchr(arg+6,'x');
		if (number) userCacheCount = atoi(number+1);
	}
	else if (!strnicmp(arg, (char*)"nofastload", 10)) fastload = false; // how many user facts allowed
	else if (!strnicmp(arg,(char*)"userfacts=",10)) userFactCount = atoi(arg+10); // how many user facts allowed
	else if (!stricmp(arg,(char*)"redo")) redo = true; // enable redo
	else if (!strnicmp(arg,(char*)"authorize=",10)) CopyParam(authorizations,arg+10); // whitelist debug commands
	else if (!stricmp(arg,(char*)"nodebug")) *authorizations = '1'; 
	else if (!strnicmp(arg,(char*)"users=",6 )) CopyParam(usersfolder,arg+6);
	else if (!strnicmp(arg,(char*)"logs=",5 )) CopyParam(logsfolder,arg+5);
	else if (!strnicmp(arg,(char*)"topic=",6 )) CopyParam(topicfolder,arg+6);
	else if (!strnicmp(arg,(char*)"tmp=",4 )) CopyParam(tmpfolder,arg+4);
    else if (!strnicmp(arg, (char*)"buildfiles=", 11)) CopyParam(buildfiles, arg + 11);
    else if (!strnicmp(arg,(char*)"private=",8)) privateParams = arg+8;
	else if (!strnicmp(arg, (char*)"treetagger", 10))
	{
		if (arg[10] == '=') CopyParam(treetaggerParams, arg + 11);
		else CopyParam(treetaggerParams, "1");
	}
	else if (!strnicmp(arg,(char*)"encrypt=",8)) CopyParam(encryptParams,arg+8);
	else if (!strnicmp(arg,(char*)"decrypt=",8)) CopyParam(decryptParams,arg+8);
	else if (!stricmp(arg, (char*)"noautoreload")) autoreload = false;
	else if (!strnicmp(arg,(char*)"livedata=",9) ) 
	{
		CopyParam(livedataFolder,arg+9);
		sprintf(systemFolder,(char*)"%s/SYSTEM",arg+9);
		sprintf(languageFolder,(char*)"%s/%s",arg+9,language);
	}
	else if (!strnicmp(arg,(char*)"nosuchbotrestart=",17) ) 
	{
		if (!stricmp(arg+17,"true")) nosuchbotrestart = true;
		else nosuchbotrestart = false;
	}
	else if (!strnicmp(arg, (char*)"parseLimit=", 11))  parseLimit = atoi(arg+11);
	else if (!strnicmp(arg,(char*)"system=",7) )  CopyParam(systemFolder,arg+7);
	else if (!strnicmp(arg,(char*)"english=",8) )  CopyParam(languageFolder,arg+8);
	else if (!strnicmp(arg, (char*)"db=", 3))  db = atoi(arg + 3);
	else if (!strnicmp(arg, (char*)"gzip=", 5))  gzip = atoi(arg + 5);
	else if (!strnicmp(arg, (char*)"syslogstr=", 10)) CopyParam(syslogstr, arg + 10);
#ifndef DISCARDMYSQL
	else if (!strnicmp(arg, (char*)"mysql=", 6)) CopyParam(mysqlparams, arg + 6, 300);
#endif
#ifndef DISCARDMICROSOFTSQL
	else if (!strnicmp(arg, (char*)"mssql=", 6)) CopyParam(mssqlparams, arg + 6, 300);
#endif
#ifndef DISCARDPOSTGRES
	else if (!strnicmp(arg, (char*)"pguser=", 7)) CopyParam(postgresparams, arg + 7, 300);
	// Postgres Override SQL
	else if (!strnicmp(arg,(char*)"pguserread=",11) )
	{
		CopyParam(postgresuserread, arg+11);
		pguserread = postgresuserread;
	}
	else if (!strnicmp(arg,(char*)"pguserinsert=",13) )
	{
	CopyParam(postgresuserinsert, arg+13);
		pguserinsert = postgresuserinsert;
	}
	else if (!strnicmp(arg,(char*)"pguserupdate=",13) )
	{
	CopyParam(postgresuserupdate, arg+13);
		pguserupdate = postgresuserupdate;
	}
#endif
#ifndef DISCARDWEBSOCKET
	else if (!strnicmp(arg, (char*)"websocket=", 10))
	{
		CopyParam(websocketparams, arg + 10,300);
		server = true;
	}
	else if (!strnicmp(arg, (char*)"websocketmessage=", 17))  CopyParam(websocketmessage, arg + 17,300);
#endif
#ifndef DISCARDMONGO
	else if (!strnicmp(arg, (char*)"mongo=", 6))
	{
		CopyParam(mongodbparams, arg + 6, 300);
	}
#endif
#ifndef DISCARDCLIENT
	else if (!strnicmp(arg,(char*)"client=",7)) // client=1.2.3.4:1024  or  client=localhost:1024
	{
		server = false;
		client = true;
		char buffer[MAX_WORD_SIZE];
		CopyParam(serverIP,arg+7);
		
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
		exit(0);
	}  
#endif
	else if (!stricmp(arg, (char*)"nouserlog")) userLog = 0;
	else if (!strnicmp(arg, "userlogging=", 12))
	{
		userLog = NO_LOG;
		if (strstr(arg, "none")) userLog = NO_LOG;
		if (strstr(arg, "file")) userLog |= FILE_LOG;
		if (strstr(arg, "stdout")) userLog |= STDOUT_LOG;
		if (strstr(arg, "stderr")) userLog |= STDERR_LOG;
		if (strstr(arg, "prelog")) userLog |=PRE_LOG;
	}
	else if (!strnicmp(arg, "userlog",7))
	{
		userLog = FILE_LOG;
		if (arg[7] == '=')
		{
			if (IsDigit(arg[8])) userLog = atoi(arg + 8);
			else  userLog = NO_LOG;
		}
	}
	else if (!stricmp(arg, "jabugpatch")) jabugpatch = true;
	else if (!strnicmp(arg, "autorestartdelay=", 17)) autorestartdelay = atoi( arg + 17);
	else if (!strnicmp(arg, "jmeter=", 7)) CopyParam(jmeter, arg + 7);
	else if (!stricmp(arg, (char*)"dieonwritefail")) dieonwritefail = true;
	else if (strstr(arg, (char*)"crnl_safe")) crnl_safe = true;
	else if (!strnicmp(arg, (char*)"blockapitrace=",14)) blockapitrace = atoi(arg+14);
	else if (!strnicmp(arg, (char*)"hidefromlog=", 12)) CopyParam(hide, arg+12);
#ifndef DISCARDSERVER
	else if (!stricmp(arg,"serverretry")) serverRetryOK = true;
	else if (!stricmp(arg,"local")) server = false; // local standalone
	else if (!strnicmp(arg, "serverlogauthcode=", 18))  CopyParam(serverlogauthcode,arg + 18);
	else if (!stricmp(arg, "noserverlog")) serverLog = NO_LOG;
	else if (!stricmp(arg, "pseudoserver")) pseudoServer = true;
	else if (!strnicmp(arg, "serverlogging=", 14))
	{
			serverLog = NO_LOG;
			if (strstr(arg, "none")) serverLog = NO_LOG;
			if (strstr(arg, "file")) serverLog |= FILE_LOG;
			if (strstr(arg, "stdout")) serverLog |= STDOUT_LOG;
			if (strstr(arg, "stderr")) serverLog |= STDERR_LOG;
			if (strstr(arg, "prelog")) serverLog |= PRE_LOG;
	}
	else if (!strnicmp(arg, "serverlog", 9))
	{
		if (arg[9] == '=' && IsDigit(arg[10])) serverLog = atoi(arg + 10) | PRE_LOG;
		else serverLog = FILE_LOG| PRE_LOG;
	}
	else if (!stricmp(arg, "nobuglog")) bugLog = NO_LOG;
	else if (!strnicmp(arg, "buglogging=", 11))
	{
		bugLog = NO_LOG;
		if (strstr(arg, "none")) bugLog = NO_LOG;
		if (strstr(arg, "file")) bugLog |= FILE_LOG;
		if (strstr(arg, "stdout")) bugLog |= STDOUT_LOG;
		if (strstr(arg, "stderr")) bugLog |= STDERR_LOG;
	}
	else if (!strnicmp(arg, "buglog",6))
	{
		bugLog = FILE_LOG;
		if (arg[6] == '=')
		{
			if (IsDigit(arg[7])) bugLog |= atoi(arg + 7); 
			else bugLog = NO_LOG;
		}
	}
	else if (!strnicmp(arg, (char*)"timelog=",8)) timeLog = atoi(arg+8);
	else if (!stricmp(arg, (char*)"noserverprelog")) serverPreLog = false;
	else if (!stricmp(arg,(char*)"serverctrlz")) serverctrlz = 1;
	else if (!strnicmp(arg, (char*)"stdlogging=", 11)) stdlogging = (atoi(arg + 11) != 0) ? true : false; // means server logging should go to std out (enabled server logging)
	else if (!strnicmp(arg,(char*)"port=",5))  // be a server
	{
        port = atoi(arg+5); // accept a port=
		server = true;
#ifdef LINUX
		if (forkcount > 1) {
			sprintf(serverLogfileName, (char*)"%s/serverlog%d-%d.txt", logsfolder, port,getpid());
            sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d-%d.txt", logsfolder, port, getpid());
		}
		else {
			sprintf(serverLogfileName, (char*)"%s/serverlog%d.txt", logsfolder, port); 
            sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d.txt", logsfolder, port);
		}
#else
		sprintf(serverLogfileName,(char*)"%s/serverlog%u.txt", logsfolder,port); // DEFAULT LOG
        sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%u.txt", logsfolder, port);
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
			sprintf(serverLogfileName, (char*)"%s/serverlog%d-%d.txt", logsfolder, port, getpid());
            sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d-%d.txt", logsfolder, port, getpid());
		}
		else {
			sprintf(serverLogfileName, (char*)"%s/serverlog%d.txt", logsfolder, port);
        	sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d.txt", logsfolder, port);
		}
#else
		sprintf(serverLogfileName, (char*)"%s/serverlog%d.txt", logsfolder, port); // DEFAULT LOG
        sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d.txt", logsfolder, port);
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
    // use curl handle specifically for this processing
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
	for (unsigned int i=0; i<response_string.size(); i++){
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
	ProcessConfigLines();
#endif
}

static void ReadConfig()
{
	// various locations of config data
	for (int i = 1; i < argc; ++i) // essentials
	{
		char* configHeader;
		if (!strnicmp(argv[i], (char*)"config=", 7)) configFile = argv[i] + 7;
		if (!strnicmp(argv[i], (char*)"config2=", 7)) configFile2 = argv[i] + 8;
		if (!strnicmp(argv[i], (char*)"configUrl=", 10)) configUrl = argv[i] + 10;
		if (!strnicmp(argv[i], (char*)"configHeader=", 13))
		{
			configHeader = argv[i] + 13;
			configHeaders[headerCount] = (char*)malloc(sizeof(char)*strlen(configHeader) + 1);
			strcpy(configHeaders[headerCount], configHeader);
			configHeaders[headerCount++][strlen(configHeader)] = '\0';
		}
	}

	char buffer[MAX_WORD_SIZE];
	FILE* in = FopenReadOnly(configFile);
	if (in)
	{
		while (ReadALine(buffer, in, MAX_WORD_SIZE) >= 0)
			ProcessArgument(TrimSpaces(buffer, true));
		fclose(in);
	}
	in = FopenReadOnly(configFile2);
	if (in)
	{
		while (ReadALine(buffer, in, MAX_WORD_SIZE) >= 0)
		{
			ProcessArgument(TrimSpaces(buffer, true));
		}
		fclose(in);
	}
	sprintf(buffer, "cs_init%s", language);
	in = FopenReadOnly(buffer);
	if (in)
	{
		while (ReadALine(buffer, in, MAX_WORD_SIZE) >= 0)
		{
			ProcessArgument(TrimSpaces(buffer, true));
		}
		fclose(in);
	}
}

static void ClearGlobals()
{
	for (unsigned int i = 0; i <= MAX_WILDCARDS; ++i)
	{
		*wildcardOriginalText[i] = *wildcardCanonicalText[i] = 0;
		wildcardPosition[i] = 0;
	}
	computerID[0] = 0;
	loginName[0] = loginID[0] = 0;
	*botPrefix = *userPrefix = 0;
	*traceuser = 0;
	*hide = 0;
	*scopeBotName = 0;
	*defaultbot = 0;
	*sourceInput = 0;
	*buildfiles = 0;
	*apikey = 0;
	*bootcmd = 0;
#ifndef DISCARDMONGO
	*mongodbparams = 0;
#endif
#ifndef DISCARDMYSQL
	*mysqlparams = 0;
#endif
#ifndef DISCARDMICROSOFTSQL
	*mssqlparams = 0;
#endif
#ifndef DISCARDPOSTGRES
	*postgresparams = 0;
#endif
#ifdef TREETAGGER
	*treetaggerParams = 0;
#endif
	*authorizations = 0;
	*loginID = 0;
}

static void SetDefaultLogsAndFolders()
{
	strcpy(usersfolder, (char*)"USERS");
	strcpy(logsfolder, (char*)"LOGS");
	strcpy(topicfolder, (char*)"TOPIC");
	strcpy(tmpfolder, (char*)"TMP");
	strcpy(language, (char*)"ENGLISH");

	strcpy(livedataFolder, (char*)"LIVEDATA"); // default directory for dynamic stuff
	strcpy(systemFolder, (char*)"LIVEDATA/SYSTEM"); // default directory for dynamic stuff
}

void OpenExternalDatabase()
{
	// db=0 turns database off everywhere
	// db=1 (default) enables database for servers only
	// db>1 turn database on everywhere
	bool usesql = (server ? (db > 0) : (db > 1));
#ifdef EVSERVER_FORK
	// When there are multiple forks then the parent doesn't open a db connection
	// but does need to remember the db parameters so that a respawned fork can use them
	if (parent_g && no_children_g > 0) usesql = false;
#endif

	char route[100];
	*route = 0;

#ifndef DISCARDMYSQL
	if (usesql && *mysqlparams)
	{
		MySQLUserFilesCode(mysqlparams);
	}
    if (filesystemOverride == MYSQLFILES) sprintf(route, "    MySql enabled. FileSystem routed MySql\r\n");
    else sprintf(route, "    MySql enabled.\r\n");
#endif
#ifndef DISCARDMICROSOFTSQL
	if (usesql && *mssqlparams)
	{
		MsSqlUserFilesCode(mssqlparams);
	}
    if (filesystemOverride == MICROSOFTSQLFILES) sprintf(route, "    MicrosoftSql enabled. FileSystem routed MicrosoftSql\r\n");
    else sprintf(route, "    MicrosoftSql enabled.\r\n");
#endif
#ifndef DISCARDPOSTGRES
	if (usesql && *postgresparams)
	{
		PGInitUserFilesCode(postgresparams);
	}
    if (filesystemOverride == POSTGRESFILES) sprintf(route, "    Postgres enabled. FileSystem routed postgress\r\n");
    else sprintf(route, "    Postgres enabled.\r\n");
#endif
#ifndef DISCARDMONGO
	if (usesql && *mongodbparams)
	{
		MongoSystemInit(mongodbparams);
	}
    if (filesystemOverride == MONGOFILES) sprintf(route, "    Mongo enabled. FileSystem routed to MongoDB\r\n");
    else sprintf(route, "    Mongo enabled.\r\n");
#endif

    if (*route)
    {
        if (server) Log(SERVERLOG, route);
        else (*printer)(route);
    }
}

unsigned int InitSystem(int argcx, char * argvx[],char* unchangedPath, char* readablePath, char* writeablePath, USERFILESYSTEM* userfiles, DEBUGAPI infn, DEBUGAPI outfn)
{ // this work mostly only happens on first startup, not on a restart
	*externalBugLog = 0;
	*serverlogauthcode = 0;
	*websocketparams = 0;
	*websocketmessage = 0;
	*jmeter = 0;
	*buildflags = 0;
	GetPrimaryIP(myip);
	for (int i = 1; i < argcx; ++i)
	{
		if (!strnicmp(argvx[i], "root=", 5))
		{
#ifdef WIN32
			SetCurrentDirectory((char*)argvx[i] + 5);
#else
			chdir((char*)argvx[i] + 5);
#endif
		}
	}

	ClearGlobals();
	SetDefaultLogsAndFolders();
	memset(&userFileSystem, 0, sizeof(userFileSystem));
	InitFileSystem(unchangedPath, readablePath, writeablePath);
	if (userfiles) memcpy((void*)&userFileSystem, userfiles, sizeof(userFileSystem));

	FILE* in = FopenStaticReadOnly((char*)"SRC/dictionarySystem.h"); // SRC/dictionarySystem.h
	if (!in) // if we are not at top level, try going up a level
	{
#ifdef WIN32
		if (!SetCurrentDirectory((char*)"..")) // move from BINARIES to top level
			myexit((char*)"FATAL unable to change up");
#else
		chdir((char*)"..");
#endif
	}
	else FClose(in);

	strcpy(hostname, (char*)"local");
	debugInput = infn;
	debugOutput = outfn;
	argc = argcx;
	argv = argvx;

    ReadConfig();
    if (configUrl != NULL) LoadconfigFromUrl(configUrl, configHeaders, headerCount);

	HandlePermanentBuffers(true);

	quitting = false;
	echo = true;	
	ProcessArguments(argc,argv);
	MakeDirectory(tmpfolder);
	if (argumentsSeen) (*printer)("\r\n");
	argumentsSeen = false;

    sprintf(logFilename,(char*)"%s/log%u.txt",logsfolder,port); // DEFAULT LOG
#ifdef LINUX
	if (forkcount > 1) {
		sprintf(serverLogfileName, (char*)"%s/serverlog%u-%u.txt", logsfolder, port,getpid());
        sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%u-%u.txt", logsfolder, port, getpid());
	}
	else {
		sprintf(serverLogfileName, (char*)"%s/serverlog%u.txt", logsfolder, port);
        sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%u.txt", logsfolder, port);
	}
#else
    sprintf(serverLogfileName,(char*)"%s/serverlog%u.txt",logsfolder,port); // DEFAULT LOG
    sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%u.txt", logsfolder, port);
#endif
	sprintf(languageFolder,"LIVEDATA/%s",language); // default directory for dynamic stuff

	if (redo) autonumber = true;

	int oldserverlog = serverLog;
	serverLog = FILE_LOG;

#ifdef LINUX
	setSignalHandlers();
#endif

#ifndef DISCARDSERVER
	if (server && !*websocketparams)
	{
#ifndef EVSERVER
		GrabPort();
#else
		(*printer)("evcalled pid: %d\r\n", getpid());

		// each fork in evserver will execute the rest from this
		if (evsrv_init(interfaceKind, port, evsrv_arg) < 0)  exit(4); // additional params will apply to each child and they will load data each
#endif
#ifdef WIN32
		PrepareServer();
#endif
	}
#endif

	if (volleyLimit == -1) volleyLimit = DEFAULT_VOLLEY_LIMIT;

	for (int i = 1; i < argc; ++i) // before createsystem to avoid loading unneeded layers
	{
		if (!strnicmp(argv[i], (char*)"build0=", 7)) build0Requested = true;
		if (!strnicmp(argv[i], (char*)"build1=", 7)) build1Requested = true;
	}

	CreateSystem();

    // Potentially use external databases for the filesystem
    OpenExternalDatabase();

	// system is ready.

	for (int i = 1; i < argc; ++i) // now for immediate action arguments
	{
#ifndef DISCARDSCRIPTCOMPILER
		if (!strnicmp(argv[i],(char*)"build0=",7))
		{
			sprintf(logFilename,(char*)"%s/build0_log.txt",usersfolder);
			in = FopenUTF8Write(logFilename);
			FClose(in);
			commandLineCompile = true;
			int result = ReadTopicFiles(argv[i]+7,BUILD0,NO_SPELL);
 			myexit((char*)"build0 complete",result);
		}  
		if (!strnicmp(argv[i],(char*)"build1=",7))
		{
			sprintf(logFilename,(char*)"%s/build1_log.txt",usersfolder);
			in = FopenUTF8Write(logFilename);
			FClose(in);
			commandLineCompile = true;
			int result = ReadTopicFiles(argv[i]+7,BUILD1,NO_SPELL);
 			myexit((char*)"build1 complete",result);
		}  
#endif
#ifndef DISCARDTESTING
		if (!strnicmp(argv[i],(char*)"debug=",6))
		{
			char* ptr = SkipWhitespace(argv[i]+6);
			commandLineCompile = true;
			if (*ptr == ':') DoCommand(ptr,mainOutputBuffer);
 			myexit((char*)"test complete");
		}  
#endif
	}

	// evserver init AFTER db init so all children inherit the one db connection
	if (server)
	{
		struct tm ptm;
#ifndef EVSERVER
		Log(ECHOSERVERLOG, "\r\n\r\n======== Began server %s compiled %s on host %s port %d at %s serverlog:%d userlog: %d\r\n",version,compileDate,hostname,port,GetTimeInfo(&ptm,true),oldserverlog,userLog);
#else
		Log(ECHOSERVERLOG, "\r\n\r\n======== Began EV server pid %d %s compiled %s on host %s port %d at %s serverlog:%d userlog: %d\r\n",getpid(),version,compileDate,hostname,port,GetTimeInfo(&ptm,true),oldserverlog,userLog);
#endif
	}
	serverLog = oldserverlog;

	echo = false;

	InitStandalone();
#ifdef PRIVATE_CODE
    PrivateInit(privateParams);
#endif
	EncryptInit(encryptParams);
	DecryptInit(decryptParams);
	ResetEncryptTags();
	return 0;
}

void PartiallyCloseSystem(bool keepalien) // server data (queues, databases, etc) remain available
{
	if (!keepalien)
	{
		WORDP shutdown = FindWord((char*)"^csshutdown");
		if (shutdown)  Callback(shutdown, (char*)"()", false, true);
#ifndef DISCARDJAVASCRIPT
		DeleteTransientJavaScript(); // unload context if there
		DeletePermanentJavaScript(); // javascript permanent system
#endif
		FreeAllUserCaches(); // user system
		CloseDictionary();	// dictionary system but not its memory
		CloseFacts();		// fact system
	}
}

void CloseSystem()
{
	PartiallyCloseSystem();
	FreeStackHeap();
	FreeDictionary();
	CloseUserCache(); 

	// server remains up on a restart
#ifndef DISCARDWEBSOCKET
	WebsocketCloseCode("");
#endif
#ifndef DISCARDSERVER
	CloseServer();
#endif
	CloseDatabases();
#ifdef PRIVATE_CODE
	PrivateShutdown();  // must come last after any mongo/postgress 
#endif
#ifndef DISCARDJSONOPEN
	CurlShutdown();
#endif
	HandlePermanentBuffers(false);
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

		char* end = (*at == ']') ? at : NULL; // old school top level style
		if (end)
		{
			uint64 milli = ElapsedMilliseconds();
			char* atptr = strstr(ptr,(char*)"loopback="); // loopback is reset after every user output
			if (!atptr) atptr = strstr(ptr, (char*)"loopback\":");
			if (atptr)
			{
				atptr += 8;
				if (*atptr == '=') ++atptr; // old style
				else if (*atptr == '"') atptr += 2; // loopback": json style
				atptr = SkipWhitespace(atptr);
				loopBackDelay = atoi(atptr);
				loopBackTime = milli + loopBackDelay;
			}
            atptr = strstr(ptr,(char*)"callback="); // call back is canceled if user gets there first
			if (!atptr) atptr = strstr(ptr, (char*)"callback\":");
			if (atptr)
			{
				atptr += 8;
				if (*atptr == '=') ++atptr; // old style
				else if (*atptr == '"') atptr += 2; // loopback": json style
				atptr = SkipWhitespace(atptr);
				callBackDelay = atoi(atptr);
				callBackTime = milli + callBackDelay;
			}
            atptr = strstr(ptr,(char*)"alarm="); // alarm stays pending until it launches
			if (!atptr) atptr = strstr(ptr, (char*)"alarm\":");
			if (atptr)
			{
				atptr += 5;
				if (*atptr == '=') ++atptr; // old style
				else if (*atptr == '"') atptr += 2; // loopback": json style
				atptr = SkipWhitespace(atptr);
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
            else if (ReadALine(ourMainInputBuffer + 1, sourceFile, fullInputLimit - 100,
				false, true, true)
			 < 0) return true; // end of input
        }
        // file based input
		else
		{
		REREAD:
			if (ReadALine(ourMainInputBuffer + 1, sourceFile, fullInputLimit - 100, false, true, true) < 0) return true; // end of input
			if (strstr(ourMainInputBuffer + 1, ":exit")) return true;
			// see if respond log entry and trim
			if (strstr(ourMainInputBuffer + 1, "Serverpre:")) logline = true;
			else if (strstr(ourMainInputBuffer + 1, "Respond:"))
			{
				logline = true;
				char* output = strstr(ourMainInputBuffer + 1, "==>");
				if (output) *output = 0; // discard 2nd half
				char* paren = strchr(ourMainInputBuffer + 1, ')');
				char word[MAX_WORD_SIZE];
				char* ptr = ReadCompiledWord(paren + 1, word); // eat turn count
				memmove(ourMainInputBuffer+1, ptr, strlen(ptr) + 1);
			}
			else if (logline || ourMainInputBuffer[1] == '#') goto REREAD; // ignore junk lines
		}
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
		else if (buildReset) 
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
		else
		{
			originalUserInput = ourMainInputBuffer;
			turn = PerformChat(loginID, computerID, ourMainInputBuffer, NULL, ourMainOutputBuffer); // no ip
		}
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
        Log(USERLOG, "Sourcefile Time used %ld ms for %d sentences %d tokens.\r\n", diff, sourceLines, sourceTokens);
		if (documentMode || silent) { ; } // no prompt in document mode
		else if (*userPrefix)
		{
			char prefix[MAX_WORD_SIZE];
			(*printer)((char*)"%s ", ReviseOutput(userPrefix, prefix));
		}
		else (*printer)((char*)"%s", (char*)"   >");
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
	originalUserInput = ourMainInputBuffer; 
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

static void ClearFunctionTracing()
{
	WORDP D;
	while (tracedFunctionsIndex)
	{
		D = tracedFunctionsList[--tracedFunctionsIndex];
		if (D->inferMark && D->internalBits & (FN_TRACE_BITS | NOTRACE_TOPIC | FN_TIME_BITS))
		{
			D->inferMark = 0;	// turn off tracing now
			D->internalBits &= -1 ^ (FN_TRACE_BITS | NOTRACE_TOPIC | FN_TIME_BITS);
		}
	}
}

void ResetToPreUser() // prepare for multiple sentences being processed - data lingers over multiple sentences
{
	postProcessing = false;
	stackFree = stackStart; // drop any possible stack used
	csapicall = NO_API_CALL;
	bufferIndex = baseBufferIndex; // return to default basic buffers used so far, in case of crash and recovery
	jumpIndex = 0;
	patternDepth = adjustIndent = 0;
	ReestablishBotVariables(); // any changes user made to kernel or boot variables will be reset
    ClearUserVariables();
	ResetUserChat();
	FACT* F = factLocked; // start of user-space facts
	ReturnToAfterLayer(LAYER_BOOT, false);  // dict/fact/strings reverted and any extra topic loaded info  (but CSBoot process NOT lost)
	if (migratetop && bootFacts) MigrateFactsToBoot(migratetop, F); // move relevant user facts or changes to bot facts to boot layer
	migratetop = NULL;
	bootFacts = false;
	for (int i = 0; i <= MAX_FIND_SETS; ++i) SET_FACTSET_COUNT(i, 0);

	ResetSentence();
	inputCounter = 0;
	totalCounter = 0;
	ResetTokenSystem();
	fullfloat = false;
	inputSentenceCount = 0;
	ClearFunctionTracing();
	rulematches = NULL;
	ClearVolleyWordMaps();
	ClearHeapThreads(); // volley start
	ResetTopicSystem(false);
	ResetFunctionSystem();
	ResetTopicReply();
	lastheapfree = heapFree;
	currentBeforeLayer = LAYER_USER;
}

void ResetSentence() // read for next sentence to process from raw system level control only
{
	respondLevel = 0; 
	currentRuleID = NO_REJOINDER;	//   current rule id
 	currentRule = 0;				//   current rule being procesed
	currentRuleTopic = -1;
	ruleErased = false;	
	badspellcount = 0;
	actualTokenCount = 0;
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
			ptr = Tokenize(ptr,count,(char**) starts,NULL,false);   //   only used to locate end of sentence but can also affect tokenFlags (no longer care)
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

void FinishVolley(char* output, char* postvalue, int limit)
{
    // massage output going to user
    if (!documentMode)
    {
        if (!(responseControl & RESPONSE_NOFACTUALIZE)) FactizeResult();
        postProcessing = true;
        ++outputNest;
        OnceCode((char*)"$cs_control_post", postvalue);
        --outputNest;
        postProcessing = false;

        char* at = output;
        if (autonumber)
        {
            sprintf(at, (char*)"%u: ", volleyCount);
            at += strlen(at);
        }
        ConcatResult(at, output + limit - 100); // save space for after data
		char* val = GetUserVariable("$cs_outputlimit", false, true);
		if (*val && !csapicall)
		{
			int lim = atoi(val);
			int size = strlen(at);
			if (size > lim) ReportBug("INFO: OutputSize exceeded %d > %d for %s\n", size, lim,at);
		}
        time_t curr = time(0);
        if (regression) curr = 44444444;
        char* when = GetMyTime(curr); // now
        if (originalUserInput && *originalUserInput) strcpy(timePrior, GetMyTime(curr)); // when we did the last volley
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
			unsigned int lapsedMilliseconds = (unsigned int)(ElapsedMilliseconds() - volleyStartTime);
			char time15[MAX_WORD_SIZE];
            sprintf(time15, (char*)" F:%ums ", lapsedMilliseconds);
            char* nl = (LogEndedCleanly()) ? (char*) "" : (char*) "\r\n";
            char* poutput = Purify(output);
            char* userInput = NULL;
            char endInput = 0;
            char* userOutput = NULL;
            char endOutput = 0;
            if (strstr(hide,"botmessage")){
                // allow tracing of OOB but thats all
                userOutput = BalanceParen(poutput, false, false);
                if (userOutput) {
                    endOutput = *userOutput;
                    *userOutput = 0;
                }
            }
            if (originalUserInput && *originalUserInput) {
                if (strstr(hide,"usermessage")){
                    // allow tracing of OOB but thats all
                    userInput = BalanceParen(originalUserInput, false, false);
                    if (userInput) {
                        endInput = *userInput;
                        *userInput = 0;
                    }
                }
            }
	
			if (*GetUserVariable("$cs_showtime", false, true))
				printf("%s\r\n", time15);

            if (userInput) *userInput = endInput;
            if (userOutput) *userOutput = endOutput;
            if (shortPos)
            {
                Log(USERLOG,"%s", DumpAnalysis(1, wordCount, posValues, (char*)"Tagged POS", false, true));
                Log(USERLOG,"\r\n");
            }
        }

        // now convert output separators between rule outputs to space from ' for user display result (log has ', user sees nothing extra)
        if (prepareMode != REGRESS_MODE)
        {
            char* sep = output + 1;
            while ((sep = strchr(sep, ENDUNIT)))
            {
				if (sep[1] == '`') sep += 2; // `` comes from empty field value in fact "" 
                else if (*(sep - 1) == ' ' || *(sep - 1) == '\n') memmove(sep, sep + 1, strlen(sep)); // since prior had space, we can just omit our separator.
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
	originalUserInput = incoming;
	int answer = PerformChat(user,usee,incoming,ip,output);
	ReleaseFakeTopics();
	return answer;
}

void EmergencyResetUser()
{
	globalDepth = 0;
	responseIndex = 0;
	callArgumentIndex = 0;
	ResetBuffers(); 
	ClearUserVariables();
	ClearHeapThreads();
	InitScriptSystem();
	ReleaseInfiniteStack();
	while (lastFactUsed > factsPreBuild[LAYER_USER]) FreeFact(lastFactUsed--); 
	DictionaryRelease(dictionaryPreBuild[LAYER_USER], heapPreBuild[LAYER_USER]);
	stackFree = stackStart; 
}

// WE DO NOT ALLOW USERS TO ENTER CONTROL CHARACTERS.
// INTERNAL STRINGS AND STUFF never have control characters either (/r /t /n converted only on output to user)
// WE internally use /r/n in file stuff for the user topic file.

bool crashBack = false; // are we rejoining from a crash?
bool restartBack = false;
bool crashset = false;

static void ValidateUserInput(char* oobSkipped)
{
	// insure no large tokensize other than strings
	char* starttoken = oobSkipped;
	char* at = starttoken;
	--at;
	char* quote = NULL;
	while (*++at) // scan user input (skipped oob data) for unreasonably long tokenization
	{
		if (*at == '\\') // absorb backslash on user input? is this safe to do?
		{
			++at;
			continue;
		}
		if (*at == '"')
		{
			if (quote) quote = NULL;
			else quote = at;
			if ((!at[1] || !at[2]) && *starttoken == '"') // no global quotes, (may have space at end)
			{
				*at = ' ';
				*starttoken = ' ';
			}
		}
		if (*at == ' ' && !quote) // proper token separator
		{
			if ((at - starttoken) > (MAX_WORD_SIZE - 1)) // trouble - token will be too big, drop excess
			{
				ReportBug("INFO: User input token size too big %d vs %d for ==> %s ", (at - starttoken), MAX_WORD_SIZE, starttoken)
					memmove(starttoken + MAX_WORD_SIZE - 10, at, strlen(at) + 1);
				at = starttoken + MAX_WORD_SIZE - 10;
			}
			starttoken = at + 1;
		}
		else if (quote && (at - starttoken) > (MAX_WORD_SIZE - 1)) // quote not closed soon enough
		{
			at = quote; // rescan tokens
			*quote = '\'';  // revise troublesome freestanding quote to not cause token size issues
			quote = NULL;
		}
	}
	if ((at - starttoken) > (MAX_WORD_SIZE - 1)) // token will be too big at end, drop excess
	{
		ReportBug("INFO: User input token size too big %d vs %d for ==> %s ", (at - starttoken), MAX_WORD_SIZE, starttoken)
			starttoken[MAX_WORD_SIZE - 10] = 0;
	}
}

static int Crashed(char* output,bool reloading,char* ip)
{
	if (crashBack) myexit((char*)"continued crash exit"); // if calling crashfunction crashes
	lastCrashTime = ElapsedMilliseconds();
	crashBack = true;
	if (reloading) myexit((char*)"crash on reloading"); // user input crashed us immediately again. give up
	char* topicName = GetUserVariable("$cs_crash", false, true);
	char* varmsg = GetUserVariable("$cs_crashmsg", false, true);
	if (*topicName == '~')
	{
		EmergencyResetUser(); // insure we can run this function
		FunctionResult result;
		*output = 0;
		howTopic = (char*)"statement";
		int pushed = PushTopic(FindTopicIDByName(topicName));
		if (pushed > 0)
		{
			currentRuleOutputBase = currentOutputBase = output;
			currentOutputLimit = maxBufferSize;
			*currentOutputBase = 0;

			CALLFRAME* frame = ChangeDepth(1, topicName);
			postProcessing = true;
			result = PerformTopic(GAMBIT, currentOutputBase); //   allow control to vary
			PopTopic();
			ChangeDepth(-1, topicName);
			if (responseIndex)
			{
				stopUserWrite = true;
				FinishVolley(currentOutputBase, NULL, outputsize);

				LogChat(startNLTime, caller, callee, ip, volleyCount, ourMainInputBuffer, ourMainOutputBuffer, 0);

				RESTORE_LOGGING();
				return volleyCount;
			}
		}
	}
	else if (*varmsg)
	{
		strcpy(output, varmsg);
		RESTORE_LOGGING();
		return volleyCount;
	}

	myexit((char*)"crash exit");
	return 0; // never gets here
}

static void LimitUserInput(char* at)
{
	unsigned int len1 = strlen(at);
	char* sizing = GetUserVariable("$cs_inputlimit", false, true);
	if (*sizing && len1) // truncate user input based on x:y data - prefer this to the inputLimit truncation
	{
		char* separator = strchr(sizing, ':');
		unsigned int prelimit = atoi(sizing);
		int postlimit = 0;
		if (separator) postlimit = atoi(separator + 1);
		unsigned int total = prelimit + postlimit;
		if (len1 > total) // truncate by sliding or truncation
		{
			if (total == prelimit) at[total] = 0; // truncate after start
			else memmove(at + prelimit, at + len1 - postlimit, postlimit + 1); // merge from end
			len1 = total;
		}
	}
	if (inputLimit && inputLimit <= (int)len1) at[inputLimit] = 0; // relative limit on user component
}

int PerformChat(char* user, char* usee, char* incoming,char* ip,char* output) // returns volleycount or 0 if command done or -1 PENDING_RESTART
{ //   primary entrypoint for chatbot -- null incoming treated as conversation start.
	currentInput = "";
	if (crashBack && autorestartdelay) // dont start immediately if requested after a crash
	{
		uint64 delay = ElapsedMilliseconds() - lastCrashTime;
		if (delay < (uint64)autorestartdelay)
		{
			*output = 0;
			return 0;
		}
	}

	incoming = SkipWhitespace(incoming);
	originalUserInput = incoming;

	volleyStartTime = ElapsedMilliseconds(); // time limit control
	startNLTime = ElapsedMilliseconds();
	holdServerLog = serverLog; // global remember so we can restore against local change
	holdUserLog = userLog;
	chatstarted = ElapsedMilliseconds(); // for timing stats
	debugcommand = false;
	ResetToPreUser();
	InitJson();
	InitStats();
	output[0] = 0;
	output[1] = 0;
	*currentFilename = 0;
	inputNest = 0; // all normal user input to start with
	// protective level 0 callframe
	globalDepth = -1;
	ChangeDepth(1, ""); // never enter debugger on this
	factsExhausted = false;
	pendingUserReset = false;
	myBot = 0;
	if (!documentMode)
	{
		tokenCount = 0;
		InitJSONNames(); // reset indices for this volley
		ClearVolleyWordMaps();
		ResetEncryptTags();

		HandleReBoot(FindWord("^cs_reboot"), true);
	}

#ifdef PRIVATE_CODE
    // Check for private hook function to potentially scan and adjust the input
    static HOOKPTR fn = FindHookFunction((char*)"PerformChatArguments");
    if (fn)
    {
        ((PerformChatArgumentsHOOKFN)fn)(user, usee, incoming);
    }
#endif

	LoggingCheats(incoming);

	if (!ip) ip = "";
	bool fakeContinue = Login(user, usee, ip, originalUserInput); //   get the participants names, now in caller and callee
	if (!crnl_safe) Clear_CRNL(originalUserInput); // we dont allow returns and newlines to damage logs
	Prelog(caller,callee, originalUserInput);

	FILE* in = fopen("restart.txt", (char*)"rb"); // per external file created, enable server log
	if (in) // trigger restart
	{
		restartBack = true;
		fclose(in);
		unlink("restart.txt");
	}
	// dont let user with bad input crash us. Crash next one after so his input wont reenter system
	crashset = true;
	bool reloading = false;
	restartfromdeath = false;
	if (crashBack || restartBack) // this is now the volley after we crashed or requested restart a moment ago
	{
#ifndef DLL
		if (!autoreload && !restartBack) myexit((char*)"delayed crash exit");
#endif
		// attempt autoreload of system and continue
		if (!restartBack) ReportBug("INFO: Autoreload: ");
		reloading = true;
		PartiallyCloseSystem(true);
		CreateSystem();
		originalUserInput = incoming;
		restartBack = crashBack = false;
		restartfromdeath = true;
	}
	
	originalUserInput = incoming;

#ifdef LINUX
	if (sigsetjmp(crashJump,1)) // if crashes come back to here
#else
	if (setjmp(crashJump)) // if crashes, come back to here
#endif
	{
		int count = Crashed(output, reloading,ip);
		RESTORE_LOGGING();
		return count;
	}

#ifdef WIN32
	__try
	{ // catch crashes in windows
#else
	try {
#endif

	if (!strnicmp(incoming, ":reset:", 7))
	{
		buildReset = FULL_RESET ;
		incoming += 7;
	}
	
	uint64 oldtoken = tokenControl; 
	parseLimited = false;

	GetUserData(buildReset, originalUserInput); // load user data to have $cs_token control over purifyinput

	// prelog before purify, to know what we got exactly 
	char* limit;
	size_t size;
	char* safeInput = InfiniteStack(limit, "PerformChat");
	char* oobSkipped = PurifyInput(incoming, safeInput, size, INPUT_PURIFY); // change out special or illegal characters
	CompleteBindStack(size);
	incoming = safeInput;
	if (!oobSkipped) oobSkipped = safeInput; // all user message
	if (parseLimit && strlen(oobSkipped) > parseLimit)
	{
		parseLimited = true;
		tokenControl &= -1 ^ (DO_POSTAG | DO_PARSE | DO_SPELLCHECK);
	}
	char* at = (tokenControl & JSON_DIRECT_FROM_OOB) ? oobSkipped : safeInput;
	LimitUserInput(at);
	mainOutputBuffer = output;

	incoming = SkipWhitespace(oobSkipped);
	ValidateUserInput(incoming); // insure no large tokensize other than strings
	char* first = incoming;

#ifndef DISCARDTESTING
	if (!server && !(*first == ':' && IsAlphaUTF8(first[1]))) strcpy(revertBuffer,first); // for a possible revert
#endif
  
	bool hadIncoming = *incoming != 0 || documentMode;
	if (!*computerID && *incoming != ':') ReportBug("No computer id before user load?")

	// else documentMode
	if (fakeContinue)
	{
		RESTORE_LOGGING();
		return volleyCount;
	}
	if (!*computerID && (!incoming || *incoming != ':'))  // a  command will be allowed to execute independent of bot- ":build" works to create a bot
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
		RESTORE_LOGGING();
		return 0;	// no such bot
	}

	// for retry INPUT
	inputRetryRejoinderTopic = inputRejoinderTopic;
	inputRetryRejoinderRuleID = inputRejoinderRuleID;
	lastInputSubstitution[0] = 0;

	int ok = 1;
    if (originalUserInput && !*originalUserInput && !hadIncoming)  //   begin a conversation - bot goes first
	{
		*readBuffer = 0;
		userFirstLine = volleyCount+1;
		responseIndex = 0;
	}

	AddInput(safeInput,0, true);
	
	ok = ProcessInput();
	if (ok <= 0)
	{
		if (userlogFile)
		{
			fclose(userlogFile);
			userlogFile = NULL;
		}
		RESTORE_LOGGING();
		return ok; // command processed
	}

	if (!server) // refresh prompts from a loaded bot since mainloop happens before user is loaded
	{
		WORDP dBotPrefix = FindWord((char*)"$botprompt");
		strcpy(botPrefix,(dBotPrefix && dBotPrefix->w.userValue) ? dBotPrefix->w.userValue : (char*)"");
		WORDP dUserPrefix = FindWord((char*)"$userprompt");
		strcpy(userPrefix,(dUserPrefix && dUserPrefix->w.userValue) ? dUserPrefix->w.userValue : (char*)"");
	}

	// compute response and hide additional information after it about why
	FinishVolley(output,NULL,outputsize); // use original input main buffer, so :user and :bot can cancel for a restart of concerasation

	LogChat(startNLTime, user, usee, ip, volleyCount, ourMainInputBuffer, output, 0);

	if (parseLimited) tokenControl = oldtoken;

#ifndef DISCARDJAVASCRIPT
	DeleteTransientJavaScript(); // unload context if there
#endif

	if (*GetUserVariable("$cs_summary", false, true))
	{
		chatstarted = ElapsedMilliseconds() - chatstarted;
		printf("Summary-  Prepare: %d  Reply: %d  Finish: %d\r\n", (int)preparationtime, (int)replytime, (int)chatstarted);
	}
	RESTORE_LOGGING();
// end of try
}
#ifdef WIN32
 __except(true)
#else
catch (...)
#endif
{
	RESTORE_LOGGING();
	char word[MAX_WORD_SIZE];
	sprintf(word, (char*)"Try/catch");
	Log(SERVERLOG, word);
	ReportBug("%s", word);
#ifdef LINUX
	siglongjmp(crashJump, 1);
#else
	longjmp(crashJump, 1);
#endif

}

if (userlogFile)
{
	fclose(userlogFile);
	userlogFile = NULL;
}
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
		Log(USERLOG,"\r\n\r\nReply to: ");
		for (int i = 1; i <= wordCount; ++i) Log(USERLOG,"%s ",wordStarts[i]);
		Log(USERLOG,"\r\n  Pending topics: %s\r\n",ShowPendingTopics());
	}
	FunctionResult result = NOPROBLEM_BIT;

	sentenceLimit = SENTENCES_LIMIT;
	char* sentencelim = GetUserVariable("$cs_sentences_limit", false, true);
	if (*sentencelim) sentenceLimit = atoi(sentencelim);
	char* topicName = GetUserVariable("$cs_control_main", false, true);
	if (trace & TRACE_FLOW) Log(USERLOG, "0 Ctrl:%s\r\n", topicName);
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
        ReportBug((char*)"INFO: Reply global depth %d not returned to %d", globalDepth, depth);
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
	PrivateRestart();
#endif

	ProcessArguments(argc,argv);
	strcpy(us,loginID);

    OpenExternalDatabase();

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
		PerformChat(us,computerID,initialInput,callerIP,mainOutputBuffer); // this autofrees buffer
    }
	else 
	{
		struct tm ptm;
		Log(USERLOG,"System restarted %s\r\n",GetTimeInfo(&ptm,true)); // shows user requesting restart.
#ifdef EVSERVER
        // new server fork log file
        Log(ECHOSERVERLOG, "\r\n\r\n======== Restarted EV server pid %d %s compiled %s on host %s port %d at %s serverlog:%d userlog: %d\r\n",getpid(),version,compileDate,hostname,port,GetTimeInfo(&ptm,true),serverLog,userLog);
#endif
	}
	pendingRestart = false;
}

int ProcessInput()
{
	// aim to be able to reset some global data of user
	unsigned int oldInputSentenceCount = inputSentenceCount;
	//   precautionary adjustments

#ifndef DISCARDTESTING
	char* at = GetNextInput(); // was set via addinput from outside
	at = SkipOOB(at);
	if (at && *at == ':' && IsAlphaUTF8(at[1]) && IsAlphaUTF8(at[2]) && !documentMode && !readingDocument) // avoid reacting to :P and other texting idioms
	{
		bool reset = false;
		if (!strnicmp(at,":reset",6)) 
		{
			reset = true;
			char* intercept = GetUserVariable("$cs_beforereset", false, true);
			if (intercept) Callback(FindWord(intercept),"()",false); // call script function first
		}
		TestMode commanded = DoCommand(at,mainOutputBuffer);
		if (commanded != FAILCOMMAND && server && !*mainOutputBuffer) strcat(mainOutputBuffer, at); // return what we didnt fail at
	
		// reset rejoinders to ignore this interruption
		outputRejoinderRuleID = inputRejoinderRuleID; 
 		outputRejoinderTopic = inputRejoinderTopic;
		if (!strnicmp(at,(char*)":retry",6) || !strnicmp(at,(char*)":redo",5))
		{
			outputRejoinderTopic = outputRejoinderRuleID = NO_REJOINDER; // but a redo must ignore pending
			AddInput(originalUserInput, 0,true);  // bug!
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
			userFirstLine = volleyCount+1;
			*readBuffer = 0;
			if (reset) 
			{
				char* intercept = GetUserVariable("$cs_afterreset", false, true);
				if (intercept) Callback(FindWord(intercept),"()",false); // call script function after
			}
			AddInput(" ", 0, true);
			ProcessInput();
			return 2; 
		}
		else if (commanded == COMMANDED ) 
		{
			ResetToPreUser(); // back to empty state before any user
			return false; 
		}
		else if (commanded == OUTPUTASGIVEN) 
			return true; 
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
	char* input = SkipWhitespace(GetNextInput());
	if (input && *input) ++volleyCount;
	int oldVolleyCount = volleyCount;
	bool startConversation = !input;
loopback:
	inputNest = 0; // all normal user input to start with
	lastInputSubstitution[0] = 0;
	if (trace &  TRACE_OUTPUT) Log(USERLOG,"\r\n\r\nInput: %d to %s: %s \r\n",volleyCount,computerID,input);

	if (input && !strncmp(input,(char*)". ",2)) input += 2;	//   maybe separator after ? or !

	//   process input now
	char prepassTopic[MAX_WORD_SIZE];
	strcpy(prepassTopic,GetUserVariable("$cs_prepass", false, true));
	if (!documentMode)  AddHumanUsed(input);
 	sentenceloopcount = 0;
	while ((( (input = GetNextInput())  && input && *input) || startConversation) && sentenceloopcount < sentenceLimit) // loop on user input sentences
	{
		sentenceOverflow = (sentenceloopcount + 1) >= sentenceLimit; // is this the last one we will do
		topicIndex = currentTopicID = 0; // precaution
		FunctionResult result = DoSentence(input,prepassTopic, sentenceloopcount >= (sentenceLimit - 1)); 
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
			RecoverUser(); // regain some initial state as though we had not done some input
			inputSentenceCount = oldInputSentenceCount;
			volleyCount = oldVolleyCount;
			AddInput(input, 0);
			goto loopback;
		}
		char* atptr = input;
		if (atptr && *atptr != '[') ++inputSentenceCount; // dont increment if OOB message
		if (sourceFile && wordCount)
		{
			sourceTokens += wordCount;
			++sourceLines;
		}
		startConversation = false;
		++sentenceloopcount;
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
			if (trace & TRACE_FLOW) Log(USERLOG, "0 ***Ctrl:%s\r\n", prepassTopic);
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
				if (result & ENDINPUT_BIT) ClearSupplementalInput();
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

FunctionResult DoSentence(char* input,char* prepassTopic, bool atlimit)
{
	if (!input || !*input) return NOPROBLEM_BIT;

	ambiguousWords = 0;

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
	char* start = input; // where we read from
	if (trace & TRACE_INPUT)
	{
		char c = start[maxBufferSize - 100];
		start[maxBufferSize - 100] = 0;
		Log(USERLOG, "\r\n\r\nInput: %s\r\n", start);
		start[maxBufferSize - 100] = c;
	}
 	if (trace && sentenceRetry) DumpUserVariables(true); 
	PrepareSentence(input,true,true,false,true); // user input.. sets nextinput up to continue
	if (atlimit) sentenceOverflow = true;

	if (changedEcho) echo = oldecho;
	
    if (PrepassSentence(prepassTopic)) return NOPROBLEM_BIT; // user input revise and resubmit?  -- COULD generate output and set rejoinders
    if (prepareMode == PREPARE_MODE || prepareMode == POS_MODE || tmpPrepareMode == POS_MODE)
    {
        return NOPROBLEM_BIT; // just do prep work, no actual reply
    }
    tokenCount += wordCount;
	sentenceRetry = false;

    if (!wordCount && responseIndex != 0) return NOPROBLEM_BIT; // nothing here and have an answer already. ignore this
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
			Log(USERLOG,"%s (%s) : (char*)",name,N->word);
			//   look at references for this topic
			int startx = -1;
			int startPosition = 0;
			int endPosition = 0;
			while (GetIthSpot(D,++startx, startPosition, endPosition)) // find matches in sentence
			{
				// value of match of this topic in this sentence
				for (int k = startPosition; k <= endPosition; ++k) 
				{
					if (k != startPosition) Log(USERLOG,"_");
					Log(USERLOG,"%s",wordStarts[k]);
				}
				Log(USERLOG," ");
			}
			Log(USERLOG,"\r\n");
		}
		impliedSet = ALREADY_HANDLED;
		if (changedEcho) echo = oldecho;
	}

    if (noReact) return NOPROBLEM_BIT;

	chatstarted = ElapsedMilliseconds() - chatstarted;
	preparationtime += chatstarted;
	chatstarted = ElapsedMilliseconds();

	FunctionResult result =  Reply();

	chatstarted = ElapsedMilliseconds() - chatstarted;
	replytime += chatstarted;
	chatstarted = ElapsedMilliseconds();

	if (result & RETRYSENTENCE_BIT && retried < MAX_RETRIES)
	{
		inputRejoinderTopic  = sentenceRetryRejoinderTopic; 
		inputRejoinderRuleID = sentenceRetryRejoinderRuleID; 

		++retried;	 // allow  retry -- issues with this
		--inputSentenceCount;    
		sentenceRetry = true;
		goto retry; // try input again -- maybe we changed token controls...
	}
	if (result & FAILSENTENCE_BIT)  --inputSentenceCount;
	if (result == ENDINPUT_BIT) ClearSupplementalInput(); // end future input

    return result;
}

void OnceCode(const char* var,char* function) //   run before doing any of his input
{
    // cannot drop stack because may be executing from ^reset(USER)
	// stackFree = stackStart; // drop any possible stack used
	withinLoop = 0;
	callIndex = 0;
	topicIndex = currentTopicID = 0; 
	char* name = (!function || !*function) ? GetUserVariable(var,false,true) : function;
	if (trace & (TRACE_MATCH | TRACE_PREPARE) && CheckTopicTrace())
	{
		Log(USERLOG, "\r\n0 ***Ctrl:%s \r\n",var);
	}
	
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
	
	// prove it has gambits
	topicBlock* block = TI(topicid);
	if (BlockedBotAccess(topicid) || GAMBIT_MAX(block->topicMaxRule) == 0)
	{
		char word[MAX_WORD_SIZE];
		sprintf(word,"There are no gambits in topic %s for %s or topic is blocked for this bot.",GetTopicName(topicid),var);
		AddResponse(word,0);
		ChangeDepth(-1,name);
        return;
	}
	ruleErased = false;	
	howTopic = 	(char*)  "gambit";
	PerformTopic(GAMBIT,currentOutputBase);
	ChangeDepth(-1,name);

	if (pushed) PopTopic();
	if (topicIndex) ReportBug((char*)"INFO: topics still stacked")
	if (globalDepth) 
		ReportBug((char*)"INFO: Once code %s global depth not 0",name);
	topicIndex = currentTopicID = 0; // precaution
}
		
void AddHumanUsed(const char* reply)
{
	if (!reply || !*reply) return;
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
	reply = SkipOOB((char*)reply);
	len = strlen(reply);
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
	sprintf(responseData[responseIndex].id,(char*)".%u.%u",TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID)); // what rule wrote this
	if (currentReuseID != -1) // if rule was referral reuse (local) , add in who did that
	{
		sprintf(responseData[responseIndex].id + strlen(responseData[responseIndex].id),(char*)".%d.%u.%u",currentReuseTopic,TOPLEVELID(currentReuseID),REJOINDERID(currentReuseID));
	}
	responseOrder[responseIndex] = (unsigned char)responseIndex;
	responseIndex++;
	if (responseIndex == MAX_RESPONSE_SENTENCES) --responseIndex;

	// now mark rule as used up if we can since it generated text
	SetErase(true); // top level rules can erase whenever they say something
	
	if (showWhy) Log(ECHOUSERLOG,(char*)"\r\n  => %s %s %d.%d  %s\r\n",(!UsableRule(currentTopicID,currentRuleID)) ? (char*)"-" : (char*)"", GetTopicName(currentTopicID,false),TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID),ShowRule(currentRule));
}

char* DoOutputAdjustments(char* msg, unsigned int control,char* &buffer,char* limit)
{
	size_t len = strlen(msg);
	if ((buffer + len) > limit)
	{
		strncpy(buffer, msg, 1000); // 5000 is safestringlimit
		strcpy(buffer + 1000, (char*)" ... "); //   prevent trouble
		ReportBug((char*)"INFO: response too big %s...", buffer)
	}
	else strcpy(buffer, msg);
	if (buffer[len - 1] == ' ') buffer[len - 1] = 0; // no trailing blank from output autospacing
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
			char* comma = strchr(ptr, ',');
			if (comma && comma != buffer)
			{
				if (*--comma == ' ') memmove(comma, comma + 1, strlen(comma));
				ptr = comma + 2;
			}
			else if (comma) ptr = comma + 1;
			else ptr = 0;
		}
	}
	return at;
}

bool AddResponse(char* msg, unsigned int control)
{
	if (!msg ) return true;
    if (*msg == '`') msg = strrchr(msg, '`') + 1; // this part was already committed
	if (!*msg) return true;
	if (*msg == ' ' && !msg[1]) return true;	// we had concatenated stuff, got erased, but leaves a blank, ignore it
	
	WORDP tapfunction = FindWord("$cs_addresponse");
	if (tapfunction && tapfunction->w.fndefinition)
	{
		char why[MAX_WORD_SIZE];
		sprintf(why, (char*)"%s.%u.%u", GetTopicName(currentTopicID), TOPLEVELID(currentRuleID), REJOINDERID(currentRuleID)); // what rule wrote this
		if (currentReuseID != -1) // if rule was referral reuse (local) , add in who did that
		{
			sprintf(why + strlen(why), (char*)".%s.%u.%u", GetTopicName(currentReuseTopic), TOPLEVELID(currentReuseID), REJOINDERID(currentReuseID));
		}
		char* buf = msg + strlen(msg) + 1;
		FunctionResult result = InternalCall(tapfunction->w.userValue, NULL, msg, why, NULL, buf);
		if (result != NOPROBLEM_BIT) // rejected
		{
			*msg = 0;
			return false;
		}
		memmove(msg, buf, strlen(buf) + 1); // revised response
	}

	char* limit;
	char* buffer = InfiniteStack(limit, "AddResponse"); // localized infinite allocation
	char* at = DoOutputAdjustments(msg, control,buffer,limit);
	if (!*at){} // we only have oob?
    else if (all || HasAlreadySaid(at) ) // dont really do this, either because it is a repeat or because we want to see all possible answers
    {
		if (all) Log(ECHOUSERLOG,(char*)"Choice %d: %s  why:%s %d.%d %s\r\n\r\n",++choiceCount,at,GetTopicName(currentTopicID,false),TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID),ShowRule(currentRule));
        else if (trace & TRACE_OUTPUT) Log(USERLOG,"Rejected: %s already said in %s\r\n",buffer,GetTopicName(currentTopicID,false));
		else if (showReject) Log(ECHOUSERLOG,(char*)"Rejected: %s already said in %s\r\n",buffer,GetTopicName(currentTopicID,false));
 		ReleaseInfiniteStack();
		return false;
    }
    if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG,"Message: %s\r\n",buffer);

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
				Log(USERLOG,"Substituted (");
				if (tokenFlags & DO_ESSENTIALS) Log(USERLOG, "essentials ");
				if (tokenFlags & DO_SUBSTITUTES) Log(USERLOG, "substitutes ");
				if (tokenFlags & DO_CONTRACTIONS) Log(USERLOG, "contractions ");
				if (tokenFlags & DO_INTERJECTIONS) Log(USERLOG, "interjections ");
				if (tokenFlags & DO_BRITISH) Log(USERLOG, "british ");
				if (tokenFlags & DO_SPELLING) Log(USERLOG, "spelling ");
				if (tokenFlags & DO_TEXTING) Log(USERLOG, "texting ");
				if (tokenFlags & DO_NOISE) Log(USERLOG, "noise ");
				if (tokenFlags & DO_PRIVATE) Log(USERLOG, "private ");
				if (tokenFlags & NO_CONDITIONAL_IDIOM) Log(USERLOG, "conditional idiom ");
				Log(USERLOG,") into: ");
				for (int i = 1; i <= wordCount; ++i) Log(USERLOG,"%s  ", wordStarts[i]);
				Log(USERLOG,"\r\n");
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
			if (tokenFlags & DO_PROPERNAME_MERGE) Log(USERLOG,"Name-");
			if (tokenFlags & DO_NUMBER_MERGE) Log(USERLOG,"Number-");
			if (tokenFlags & DO_DATE_MERGE) Log(USERLOG,"Date-");
			Log(USERLOG,"merged: ");
			for (i = 1; i <= wordCount; ++i) Log(USERLOG,"%s  ", wordStarts[i]);
			Log(USERLOG,"\r\n");
			memcpy(original + 1, wordStarts + 1, wordCount * sizeof(char*));	// replicate for test
			originalCount = wordCount;
		}
	}	

	// spell check unless 1st word is already a known interjection. Will become standalone sentence
	if (tokenControl & DO_SPELLCHECK && wordCount && *wordStarts[1] != '~' )
	{
		if (SpellCheckSentence()) tokenFlags |= DO_SPELLCHECK;
		if (*GetUserVariable("$$cs_badspell", false, true)) return;
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
				Log(USERLOG,"Spelling changed into: ");
				for (i = 1; i <= wordCount; ++i) Log(USERLOG,"%s  ", wordStarts[i]);
				Log(USERLOG,"\r\n");
			}
		}
        originalCount = wordCount;

        // resubstitute after spellcheck did something
        if (tokenFlags & DO_SPELLCHECK && tokenControl & (DO_SUBSTITUTE_SYSTEM | DO_PRIVATE))  ProcessSubstitutes();

		if (spellTrace) {}
		else if (mytrace & TRACE_INPUT  || prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)
		{
			int changed = 0;
			if (wordCount != originalCount) changed = true;
			for (i = 1; i <= wordCount; ++i) if (original[i] != wordStarts[i]) changed = i;
			if (changed)
			{
				Log(USERLOG,"Substitution changed into: ");
				for (i = 1; i <= wordCount; ++i) Log(USERLOG,"%s  ", wordStarts[i]);
				Log(USERLOG,"\r\n");
			}
		}
	}
	if (tokenControl & DO_PROPERNAME_MERGE && wordCount )  ProperNameMerge();
	if (tokenControl & DO_DATE_MERGE && wordCount)  ProcessCompositeDate();
	if (tokenControl & DO_NUMBER_MERGE && wordCount )  ProcessCompositeNumber(); //   numbers AFTER titles, so they dont change a title
	if (tokenControl & DO_SPLIT_UNDERSCORE)  ProcessSplitUnderscores();
}

static void TraceDerivation(bool analyze)
{
	if (analyze) Log(USERLOG, "Actual used analyze input: (words:%d more:%d) ", wordCount, moreToCome);
	else Log(USERLOG, "Actual used input words:%d more:%d:    ", wordCount, moreToCome);
	for (unsigned int i = 1; i <= (unsigned int)wordCount; ++i)
	{
		Log(USERLOG, "%s", wordStarts[i]);
		int start = derivationIndex[i] >> 8;
		int end = derivationIndex[i] & 0x00ff;
		if (start == end && wordStarts[i] == derivationSentence[start]) { ; } // unchanged from original
		else // it came from somewhere else
		{
			start = derivationIndex[i] >> 8;
			end = derivationIndex[i] & 0x00Ff;
			Log(USERLOG, "(");
			for (int j = start; j <= end; ++j)
			{
				if (j == start) Log(USERLOG, " ");
				if (j == 1) Log(USERLOG, "%c", derivationSeparator[0]);
				Log(USERLOG, "%s%c", derivationSentence[j], derivationSeparator[j]);
			}
			Log(USERLOG, ")");
		}
		Log(USERLOG, " ");
	}
	Log(USERLOG, "\r\n\r\n");
}

static void TraceTokenization(char* input)
{
	Log(USERLOG, "\r\nTokenControl: ");
	DumpTokenControls(tokenControl);
	Log(USERLOG, "\r\n");
	if (tokenFlags & USERINPUT) Log(USERLOG, "Original User Input: %s\r\n", input);
	else Log(USERLOG, "Original Chatbot Output: %s\r\n", input);
	Log(USERLOG, "Tokenized into: ");
	for (int i = 1; i <= wordCount; ++i) Log(FORCESTAYUSERLOG, (char*)"%s  ", wordStarts[i]);
	Log(FORCESTAYUSERLOG, (char*)"\r\n");
}

void PrepareSentence(char* input,bool mark,bool user, bool analyze,bool oobstart,bool atlimit) 
{
	unsigned int mytrace = trace;
	uint64 start_time = ElapsedMilliseconds();
	if (prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE) mytrace = 0;
	ResetSentence();
	ResetTokenSystem();
    ClearWhereInSentence();
    ClearTriedData();
    char separators[MAX_SENTENCE_LENGTH];
    memset(separators,0,sizeof(char)*MAX_SENTENCE_LENGTH);
    
	char* ptr = input;
	size_t len = strlen(ptr);
	char* nearend = ptr + len - 50;

	// protection from null input bug to testpattern
	char* in = strstr(input, "\"input\": \", \"pattern");
	if (in)
	{
		in += 10;
		memmove(in + 1, in, strlen(in) + 1); // make room to patch
		*in = '"';
		ReportBug("INFO: Missing dq in API input field")
	}

	tokenFlags |= (user) ? USERINPUT : 0; // remove any question mark
	ptr = SkipWhitespace(ptr); // not needed now?
	char startc = *ptr;
    ptr = Tokenize(ptr,wordCount,wordStarts,separators,false,oobstart);
	SetContinuationInput(ptr);
	if (!wordCount && startc) // insure we return something always
	{
		char x[10];
		x[0] = startc;
		x[1] = 0;
		wordStarts[++wordCount] = AllocateHeap(x);	
	}
	actualTokenCount = wordCount;
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
    // separators might use the first character when there is a single quoted word
	for (int i = 1; i <= wordCount; ++i)
	{
		derivationIndex[i] = (unsigned short)((i << 8) | i); // track where substitutions come from
		derivationSentence[i] = wordStarts[i]; 
	}
    memcpy(derivationSeparator,separators,(wordCount+1) * sizeof(char));
	derivationLength = wordCount;
	derivationSentence[wordCount+1] = NULL;
    derivationSeparator[wordCount+1] = 0;

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
					i = j = len + 10000; // len might be BIG (oob data) so make sure beyond it)
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
		TraceTokenization(input);
	}
	
	char* bad = GetUserVariable("$$cs_badspell", false, true); // spelling says this user is a mess
	if (*bad) {}
	else if (!oobExists)
	{
		NLPipeline(mytrace);
		bad = GetUserVariable("$$cs_badspell", false, true); // possibly set by NLPipeline
	}
    else
    {
		marklimit = 0;
		for (int i = 1; i <= wordCount; ++i)
        {
            wordCanonical[i] = wordStarts[i];
            MarkWordHit(0, EXACTNOTSET, StoreWord(wordStarts[i], AS_IS), 0, i, i);
        }
    }
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
            if (i == 1) strncat(buffer,&derivationSeparator[0],1);
			strcat(buffer,derivationSentence[i]);
            strncat(buffer,&derivationSeparator[i],1);
		}

		// what the rest of the input had as punctuation OR insure it does have it
		char* end = buffer + strlen(buffer);
		char* tail = end - 1;
		while (tail > buffer && *tail == ' ') --tail; 
		if (tokenFlags & QUESTIONMARK && *tail != '?') strcat(tail,(char*)"? ");
		else if (tokenFlags & EXCLAMATIONMARK && *tail != '!' ) strcat(tail,(char*)"! ");
		else if (*tail != '.') strcat(tail,(char*)". ");

		if (!analyze) 
		{
			EraseCurrentInput();
			AddInput(buffer, 0);
		}
		FreeBuffer();
		wordCount = 1; // truncate current analysis to just the interjection
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
                if (j == 1) strncat(atword,&derivationSeparator[0],1);
				strcpy(atword, derivationSentence[j]);
				if (j < derivationLength) strncat(atword,&derivationSeparator[j],1);
                atword += strlen(atword);
				*atword = 0;
			}
		}
	}
	
	if (mytrace & TRACE_INPUT || prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)
	{
		TraceDerivation(analyze);
	}

	if (echoSource == SOURCE_ECHO_LOG) 
	{
		Log(ECHOUSERLOG,(char*)"  => ");
		for (i = 1; i <= wordCount; ++i) Log(USERLOG,"%s  ",wordStarts[i]);
		Log(ECHOUSERLOG,(char*)"\r\n");
	}

	wordStarts[0] = wordStarts[wordCount+1] = AllocateHeap((char*)""); // visible end of data in debug display
	wordStarts[wordCount+2] = 0;
    if (!oobExists && mark && wordCount) MarkAllImpliedWords((*bad || actualTokenCount == REAL_SENTENCE_WORD_LIMIT) ? true : false);

	if (prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE || trace & TRACE_POS || prepareMode == POS_MODE || (prepareMode == PENN_MODE && trace & TRACE_POS)) DumpTokenFlags((char*)"After parse");
	if (timing & TIME_PREPARE) {
		int diff = (int)(ElapsedMilliseconds() - start_time);
		if (timing & TIME_ALWAYS || diff > 0) Log(STDTIMELOG, (char*)"Prepare %s time: %d ms\r\n", input, diff);
	}
}

#ifndef NOMAIN
int main(int argc, char * argv[]) 
{
	if (InitSystem(argc,argv)) myexit((char*)"failed to load memory");
    ResetToPreUser();
    if (!server) 
	{
		quitting = false; // allow local bots to continue regardless
		MainLoop();
	}
	else if (quitting) {;} // boot load requests quit
#ifndef DISCARDWEBSOCKET
	else if (*websocketparams) WebSocketClient(websocketparams,websocketmessage);
#endif
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
