#include "common.h" 
#include "evserver.h"
int pendingUserLimit = 0;
std::atomic<int> userQueue(0);
char* version = "14.1";
char overflowMessage[400];
int overflowLen = 0;
const char* buildString = "compiled " __DATE__ ", " __TIME__ ".";
char sourceInput[200];
int cs_qsize = 0;
static int python = 0;
char repairinput[20];
unsigned int argcx;
char** argvx;
bool integrationTest = false;
char websocketclient[200];
bool nophrases = false;
char* repairat = NULL;
char baseLanguage[50];
static char* retryInput;
char* tracebuffer; // preallocated buffer for possible api tracing
int volleyFile = 0;
unsigned long startLogDelay = 0;
int forcedRandom = -1;
char* releaseBotVar = NULL;
char rootdir[MAX_WORD_SIZE];
char incomingDir[MAX_WORD_SIZE];
FILE* userInitFile;
unsigned int parseLimit = 0;
bool jabugpatch = false;
bool fastload = true;
int hadAuthCode = 0;
bool blockapitrace = false;
static bool crnl_safe = false;
int externalTagger = 0;
char defaultbot[100];
char serverlogauthcode[30];
uint64 chatstarted = 0;
char websocketparams[300];
char websocketmessage[300];
char websocketbot[300];
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
char* originalUserMessage = NULL;
bool debugcommand = false;
int traceUniversal;
int debugLevel = 0;
PRINTER printer = printf;
uint64 startNLTime;
bool retrycheat;
unsigned int idetrace = (unsigned int)-1;
int outputlevel = 0;
uint64 timedeployed = 0;
// parameters
unsigned int argc;
char** argv;
char* configFile = "cs_init.txt";	// can set config params
char* configFile2 = "cs_initmore.txt";	// can set config params
char* configLines[MAX_WORD_SIZE];	// can set config params

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
unsigned int tokenCount;						// for document 
bool callback = false;						// when input is callback,alarm,loopback, dont want to enable :retry for that...
uint64 timerLimit = 0;						// limit time per volley
int timerCheckRate = 0;					// how often to check calls for time
uint64 volleyStartTime = 0;
char forkCount[10];
int timerCheckInstance = 0;
char hostname[100];
bool logline = false;
static char* tsvInputs[15];
char tsvpriorlogin[MAX_WORD_SIZE];
unsigned int tsvIndex;
unsigned int tsvMessageField;
unsigned int tsvLoginField;
unsigned int tsvSpeakerField;
unsigned int tsvCount;
unsigned int tsvSkip;
char speaker[100];
FILE* tsvFile = NULL;
bool nosuchbotrestart = false; // restart if no such bot
char* derivationSentence[MAX_SENTENCE_LENGTH];
char derivationSeparator[MAX_SENTENCE_LENGTH];
unsigned int derivationLength;
char authorizations[200];	// for allowing debug commands
char* ourMainInputBuffer = NULL;				// user input buffer - ptr to primaryInputBuffer
char* ourMainOutputBuffer = NULL;				// main user output buffer
char* readBuffer;								// main scratch reading buffer (files, etc)
bool readingDocument = false;
bool redo = false; // retry backwards any amount
bool oobExists = false;
bool docstats = false;
unsigned int docSentenceCount = 0;
char* rawSentenceCopy; // current raw sentence
char* evsrv_arg = NULL;

unsigned short int derivationIndex[MAX_SENTENCE_LENGTH];

bool scriptOverrideAuthorization = false;

clock_t  startSystem;						// time chatscript started
unsigned int choiceCount = 0;
int always = 1;									// just something to stop the complaint about a loop based on a constant
bool errorOnMissingElse = false;

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
RESPONSE responseData[MAX_RESPONSE_SENTENCES + 1];
unsigned char responseOrder[MAX_RESPONSE_SENTENCES + 1];
int responseIndex;

int inputSentenceCount;				// which sentence of volley is this
static void HandleReBoot(WORDP boot, bool reboot);

///////////////////////////////////////////////
/// SYSTEM STARTUP AND SHUTDOWN
///////////////////////////////////////////////

void HandlePermanentBuffers(bool init)
{
	if (init)
	{
		if (!fullInputLimit) fullInputLimit = maxBufferSize * 2;
		readBuffer = mymalloc(fullInputLimit); // need to be full, to read a log file potentially
		if (!readBuffer) myexit("FATAL: buffer allocation fail");
		*readBuffer = 0;
		currentRuleOutputBase = currentOutputBase = readBuffer; // a default needed for handle boot

		tracebuffer = mymalloc(TESTPATTERN_TRACE_SIZE);
		lastInputSubstitution = mymalloc(maxBufferSize);
		rawSentenceCopy = mymalloc(maxBufferSize);
		revertBuffer = mymalloc(maxBufferSize);

#ifndef DLL
		ourMainInputBuffer = mymalloc(fullInputLimit);  // precaution for overflow
		ourMainOutputBuffer = mymalloc(outputsize);
#else
		ourMainOutputBuffer = revertBuffer; // for boot call to function on startup potentially
#endif
		currentOutputLimit = outputsize;

		// need allocatable buffers for things that run ahead like servers and such.
		maxBufferSize = (maxBufferSize + 63);
		maxBufferSize &= 0xffffffc0; // force 64 bit align on size  
		unsigned int total = maxBufferLimit * maxBufferSize;
		buffers = mymalloc(total); // have it around already for messages
		if (!buffers)
		{
			(*printer)((char*)"%s", (char*)"cannot allocate buffer space");
			myexit((char*)"FATAL: cannot allocate buffer space");
		}
		bufferIndex = 0;
		baseBufferIndex = bufferIndex;
        malloc_jp_token_buffers();
		malloc_mssql_buffer();
	}
	else
	{
		CloseBuffers();
	}
}

void InitStandalone()
{
	startSystem = CPU_TICKS;
	*currentFilename = 0;	//   no files are open (if logging bugs)
	tokenControl = 0;
	outputlevel = 0;
	*computerID = 0; // default bot
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
	if (traceboot) trace = (unsigned int)-1;
	else
	{
		FILE* in = FopenReadOnly("traceboot.txt"); // per external file created, enable server log on boot
		if (in)
		{
			trace = (unsigned int)-1;
			FClose(in);
		}
	}
	int olduserlog = userLog;
	if (trace) userLog = FILE_LOG | PRE_LOG;
	if (*bootcmd) DoCommand(bootcmd, NULL, false); // run this command  before CSBOOT is run. eg to trace boot process

	if (!volleyboot) UnlockLayer(LAYER_BOOT); // unlock it to add stuff
	else 	currentBeforeLayer = LAYER_BOOT; // be in user layer (before is boot layer)
	FACT* F = lastFactUsed; // note facts allocated before running boot
	Callback(boot, (char*)"()", true, true); // do before world is locked
	if (ourMainOutputBuffer) *ourMainOutputBuffer = 0; // remove any boot message
	myBot = 0;	// restore fact owner to generic all
	   // cs_reboot may call boot to reload layer, this flag
	   // rebooting might be wrong...
	if (!volleyboot)
	{
		RelocateDeadBootFacts(F);
		CloseBotVariables();
	}
	else
	{
		ReturnToAfterLayer(LAYER_BOOT, false);  // dict/fact/strings reverted and any extra topic loaded info  (but CSBoot process NOT lost)
	}

	userLog = olduserlog;
	trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
	timing = (modifiedTiming) ? modifiedTimingVal : oldtiming;
	stackFree = stackStart; // drop any possible stack used
	rebooting = false;
}

unsigned int CountWordsInBuckets(unsigned int& unused, unsigned int* depthcount, int limit)
{
	memset(depthcount, 0, sizeof(int) * limit);
	unused = 0;
	int words = 0;
	for (unsigned long i = 0; i <= maxHashBuckets; ++i)
	{
		int n = 0;
		if (!hashbuckets[i]) ++unused;
		else
		{
			WORDP D = Index2Word(hashbuckets[i]);
			while (D)
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

static void ShowMemoryUsage()
{
	char data[MAX_WORD_SIZE];
	int dictcount = (unsigned int)(dictionaryFree - dictionaryBase);
	unsigned long dictm = dictcount * sizeof(WORDENTRY);
	unsigned long dictfree = (unsigned int)(maxDictEntries - dictcount);
	unsigned long dictFreeMemMB = (unsigned int)((dictfree * sizeof(WORDENTRY)) / 100000);
	unsigned int factcount = lastFactUsed - factBase;
	unsigned int factUsedMemMB = (unsigned int)((factcount * sizeof(FACT)) / 1000000);
	unsigned int factFreeMemMB = (unsigned int)(((factEnd - lastFactUsed) * sizeof(FACT)) / 1000000);
	unsigned int dictUsedMemMB = (unsigned int)(((dictionaryFree - dictionaryBase) * sizeof(WORDENTRY)) / 1000000);
	// dictfree shares text space
	unsigned int textUsedMemMB = (unsigned int)((heapBase - heapFree) / 1000000);
	unsigned int textFreeMemMB = (unsigned int)((heapFree - heapEnd) / 1000000);

	unsigned int bufferMemMB = (maxBufferLimit * maxBufferSize) / 1000000;
	unsigned int cacheMB = (userTopicStoreSize + userTableSize) / 1000000;
	unsigned int usedMB = factUsedMemMB + dictUsedMemMB + textUsedMemMB + bufferMemMB;
	usedMB += (userTopicStoreSize + userTableSize) / 1000000;
	usedMB += bufferMemMB + cacheMB;
	unsigned int freeMB = factFreeMemMB + textFreeMemMB + dictFreeMemMB;

	unsigned int bytes = (tagRuleCount * MAX_TAG_FIELDS * sizeof(uint64)) / 1000;
	usedMB += bytes / 1000000;
	char buf2[MAX_WORD_SIZE];
	char buf1[MAX_WORD_SIZE];
	char buf[MAX_WORD_SIZE];
	strcpy(buf, StdIntOutput(factcount));
	strcpy(buf2, StdIntOutput(textFreeMemMB));
	unsigned int depthcount[MAX_HASH_DEPTH];
	unsigned int unused;
	unsigned int words = CountWordsInBuckets(unused, depthcount, MAX_HASH_DEPTH);

	char depthmsg[8000];
	int x = MAX_HASH_DEPTH;
	const char* full = (depthcount[MAX_HASH_DEPTH - 1]) ? "+" : "";
	while (!depthcount[--x]); // find last used slot
	char* at = depthmsg;
	*at = 0;
	for (int i = 1; i <= x; ++i)
	{
		sprintf(at, "%u.", depthcount[i]);
		at += strlen(at);
	}
	sprintf(at, "-%d", x);

	sprintf(data, (char*)"Used %uMB: dictentries %s (%uMb) fact %s (%uMb) heap %uMb buffer %uMb usercache %uMb\r\n",
		usedMB,
		StdIntOutput(dictcount),
		dictUsedMemMB,
		buf,
		factUsedMemMB,
		textUsedMemMB, bufferMemMB,
		cacheMB);
	if (server) Log(SERVERLOG, "%s", data);
	else (*printer)((char*)"%s", data);

	sprintf(data, (char*)"hashbuckets: %lu (unused %u) depths %s\r\n", maxHashBuckets, unused, depthmsg);
	if (server) Log(SERVERLOG, "%s", data);
	else (*printer)((char*)"%s", data);

	char factmb[500];
	unsigned long factm = factcount * sizeof(FACT);
	strcpy(factmb, StdIntOutput((int)(factm / 1000000))); // unused facts
	strcpy(buf, StdIntOutput(factcount)); // unused facts
	strcpy(buf1, StdIntOutput(textFreeMemMB)); // unused text
	strcpy(buf2, StdIntOutput(freeMB));
	sprintf(data, (char*)"Free %sMB: dict %s(%zuMb) fact %s(%sMb) stack/heap %sMB \r\n\r\n",
		buf2, StdIntOutput(dictfree), dictFreeMemMB, buf, factmb, buf1);
	if (server) Log(SERVERLOG, "%s", data);
	else (*printer)((char*)"%s", data);
}

static void PerformBoot()
{
	buildbootjid = 0;
	builduserjid = 0;
	UnlockLayer(LAYER_BOOT); // unlock it to add stuff
	HandleReBoot(FindWord((char*)"^csboot"), false);// run script on startup of system. data it generates will also be layer 1 data
	LockLayer();
	currentBeforeLayer = LAYER_BOOT;
}

void CreateSystem()
{
	timedeployed = ElapsedMilliseconds();
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
		buffers = mymalloc(total); // have it around already for messages
		if (!buffers)
		{
			(*printer)((char*)"%s", (char*)"cannot allocate buffer space");
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
	sprintf(data, (char*)"ChatScript %s %s pid:%d os:%ld-bit %s compiled: %s", kind, version, pid, (long int)(sizeof(char*) * 8), os, compileDate);
	strcat(data, (char*)" rootdir:");
	strcat(data, rootdir);
	strcat(data, (char*)" host:");
	strcat(data, hostname);
	strcat(data, (char*)"\r\n");
	Log(SERVERLOG, "%s%s", (server) ? "Server " : "", data);

	int oldtrace = trace; // in case trace turned on by default
	trace = 0;
	*oktest = 0;

	sprintf(data, (char*)"Params:   dict:%lu  fact:%lu text:%lukb hash:%lu \r\n", (unsigned long)maxDictEntries, (unsigned long)maxFacts, (unsigned long)(maxHeapBytes / 1000), (unsigned long)maxHashBuckets);
	if (server) Log(SERVERLOG, "%s", data);
	else (*printer)((char*)"%s", data);
	sprintf(data, (char*)"          buffer:%ux%ukb ucache:%ux%ukb fcache:%ux%ukb userfacts:%u outputlimit:%u loglimit:%u\r\n", (unsigned int)maxBufferLimit, (unsigned int)(maxBufferSize / 1000), (unsigned int)userCacheCount, (unsigned int)(userCacheSize / 1000), (unsigned int)fileCacheCount, (unsigned int)(fileCacheSize / 1000), (unsigned int)userFactCount,
		outputsize, logsize);
	if (server) Log(SERVERLOG, "%s", data);
	else (*printer)((char*)"%s", data);

	sprintf(data, (char*)"Libraries: ");
#ifndef DISCARDJSONOPEN
	char data1[100];
	sprintf(data1, (char*)"curl:%s, ", CurlVersion());
	strcat(data, data1);
#endif
#ifndef DISCARD_JAPANESE
	strcat(data, "Mecab-Japanese");
#endif
	strcat(data, "\r\n");
	if (server) Log(SERVERLOG, "%s", data);
	else (*printer)((char*)"%s", data);

	// in case
	MakeDirectory(topicfolder);
	char name[MAX_WORD_SIZE];
	sprintf(name, "%s/BUILD0", topicfolder);
	MakeDirectory(name);
	sprintf(name, "%s/BUILD1", topicfolder);
	MakeDirectory(name);

	originalUserInput = NULL;
	currentInput = NULL;

	LoadSystem(0,argc,argv);		
	// leaves current layer as BOOT (2)

	strcpy(dbparams, mssqlparams); // do regardless so can use dummy codes
#ifndef DISCARDMONGO
	strcpy(dbparams, mongodbparams);
#endif
#ifndef DISCARDPOSTGRES
	strcpy(dbparams, postgresparams);
#endif
#ifndef DISCARDMYSQL
	strcpy(dbparams, mysqlparams);
#endif
	trace = oldtrace; // allow boot tracing
	PerformBoot();
	InitSpellCheck(); // after boot vocabulary added

	ShowMemoryUsage();
	
#ifdef DISCARDSERVER 
	(*printer)((char*)"    Server disabled.\r\n");
#endif
#ifdef DISCARDSCRIPTCOMPILER
	if (server) Log(SERVERLOG, "    Script compiler disabled.\r\n");
	else (*printer)((char*)"    Script compiler disabled.\r\n");
#endif
#ifdef DISCARDTESTING
	if (server) Log(SERVERLOG, "    Testing disabled.\r\n");
	else (*printer)((char*)"    Testing disabled.\r\n");
#else
	* callerIP = 0;
	if (server && VerifyAuthorization(FopenReadOnly((char*)"authorizedIP.txt"))) // authorizedIP
	{
		Log(SERVERLOG, "    *** Server WIDE OPEN to :command use.\r\n");
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
	if (externalPostagger == TreeTagger)
	{
		sprintf(route, (char*)"    TreeTagger access enabled\r\n");
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
	(*printer)((char*)"%s", (char*)"\r\n");
	loading = false;
	printf("System created in %u ms\r\n", (unsigned int)(ElapsedMilliseconds() - timedeployed));
}

void LoadSystem(unsigned int limit,unsigned int argc, char** argv)
{//   reset the basic system 
	myBot = 0; // facts loaded here are universal
	InitFacts(); // malloc space
	InitStackHeap(); // malloc space
	InitDictionary(); // malloc space
	
	LoadDictionary(heapFree);
	InitUserCache(); // malloc space
	InitFunctionSystem(); // modify dictionary entries
	InitScriptSystem();
	InitVariableSystem();
	InitSystemVariables();
	InitLogs();

	SetLanguage("universal");
	InitUniversalTextUtilities(); // also part of basic system before a build
	WalkLanguages(InitTextUtilitiesByLanguage); // also part of basic system before a build
	WalkLanguages(ReadLiveData); // below layer 0
	if (multidict) SetLanguage("English"); // return to std base
	printf("Languages: %s\r\n", language_list);

#ifdef TREETAGGER
	// We will be reserving some working space for treetagger on the heap
	// so make sure it is tucked away where it cannot be touched
   // NOT IN DEBUGGER WILL FORCE reloads of layers
	InitTreeTagger(treetaggerParams);
#endif
#ifndef DISCARD_JAPANESE
	printf("Loaded Mecab kanji tagger\r\n");
#endif

	WordnetLockDictionary();

	char* lang = language_list;
	lang = GetNextLanguage(lang);
	SetLanguage(lang);
	*currentFilename = 0;
	*debugEntry = 0;

	InitTopicSystem(limit);		// dictionary reverts to wordnet zone

	InitBotVariables(argc, argv);
#ifndef DISCARD_PYTHON
	if (python) Py_Initialize();
#endif
}

void Rebegin(unsigned int buildId,unsigned int argc, char** argv)
{
	CloseLogs();
	FreeAllUserCaches(); // user system
	CloseDictionary();	// dictionary system but not its memory
	CloseFacts();		// fact system

	LoadSystem(buildId,argc,argv);

	PerformBoot();
	InitSpellCheck(); // after boot vocabulary added
}

static void ProcessArgument(char* arg)
{
#ifndef NOARGTRACE
	int old = serverLog;
	serverLog = FILE_LOG;
	Log(SERVERLOG, "    %s\r\n", arg);
	serverLog = old;

#endif
	if (!strnicmp(arg, (char*)"trace", 5) && stricmp(arg,(char*)"traceboot"))
	{
		if (arg[5] == '=') trace = atoi(arg + 6);
		else if (arg[5] == 'b') {}
		else trace = (unsigned int)-1;
	}
	else if (!strnicmp(arg, (char*)"language=", 9))
	{
		strcpy(current_language, arg + 9);
		MakeUpperCase(current_language);
		strcpy(language_list, current_language);

		max_language = LANGUAGE_1; // for now
		max_fact_language = FACTLANGUAGE_1; // for now
		languageCount = 1;
		char* x = strchr(current_language, ','); // is a multi list
		if (x)
		{
			multidict = true;
			*x = 0; // default 1st one
			// how many languages are we loading
			x = language_list - 1;
			while ((x = strchr(++x, ','))) ++languageCount;
		}

		strcpy(baseLanguage, current_language);
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
		if (!SetDirectory(arg + 4)) (*printer)((char*)"unable to change to %s\r\n", arg + 4);
	}
	else if (!strnicmp(arg, (char*)"source=", 7))  CopyParam(sourceInput, arg + 7);
	else if (!strnicmp(arg, (char*)"login=", 6))  CopyParam(loginID, arg + 6);
	else if (!strnicmp(arg, (char*)"output=", 7)) outputLength = atoi(arg + 7);
	else if (!strnicmp(arg, (char*)"save=", 5))
	{
		volleyLimit = atoi(arg + 5);
		if (volleyLimit > 255) volleyLimit = 255; // cant store higher
	}

	// memory sizings
	else if (!strnicmp(arg, (char*)"hash=", 5))
	{
		maxHashBuckets = atoi(arg + 5); // size of hash
		setMaxHashBuckets = true;
	}
	else if (!strnicmp(arg, (char*)"dict=", 5)) maxDictEntries = atoi(arg + 5); // how many dict words allowed
	else if (!strnicmp(arg, (char*)"fact=", 5)) maxFacts = atoi(arg + 5);  // fact entries
	else if (!strnicmp(arg, (char*)"text=", 5)) maxHeapBytes = atoi(arg + 5) * 1000; // stack and heap bytes in pages
	else if (!strnicmp(arg, (char*)"debuglevel=", 11)) debugLevel = atoi(arg + 11);  // log level entries
	else if (!strnicmp(arg, (char*)"cache=", 6)) // value of 10x0 means never save user data
	{
		userCacheSize = atoi(arg + 6) * 1000;
		char* number = strchr(arg + 6, 'x');
		if (number) userCacheCount = atoi(number + 1);
	}
	else if (!strnicmp(arg, "nophrases", 9)) nophrases = true;
	else if (!strnicmp(arg, "random=", 7)) forcedRandom = atoi(arg + 7);
	else if (!strnicmp(arg, "legacymatch=", 12)) legacyMatch = atoi(arg+12);
	else if (!strnicmp(arg, (char*)"filecache=", 10)) // read only cache of files
	{
		fileCacheSize = atoi(arg + 10) * 1000;
		char* number = strchr(arg + 10, 'x');
		if (number) fileCacheCount = atoi(number + 1);
	}
	else if (!strnicmp(arg, (char*)"nofastload", 10)) fastload = false; // how many user facts allowed
	else if (!strnicmp(arg, (char*)"userfacts=", 10)) userFactCount = atoi(arg + 10); // how many user facts allowed
	else if (!stricmp(arg, (char*)"redo")) redo = true; // enable redo
	else if (!strnicmp(arg, (char*)"authorize=", 10)) CopyParam(authorizations, arg + 10); // whitelist debug commands
	else if (!stricmp(arg, (char*)"nodebug")) *authorizations = '1';
	else if (!strnicmp(arg, (char*)"users=", 6)) CopyParam(usersfolder, arg + 6);
	else if (!strnicmp(arg, (char*)"logs=", 5)) CopyParam(logsfolder, arg + 5);
	else if (!strnicmp(arg, (char*)"topic=", 6)) CopyParam(topicfolder, arg + 6);
	else if (!strnicmp(arg, (char*)"tmp=", 4)) CopyParam(tmpfolder, arg + 4);
	else if (!strnicmp(arg, (char*)"buildfiles=", 11)) CopyParam(buildfiles, arg + 11);
	else if (!strnicmp(arg, (char*)"private=", 8)) privateParams = arg + 8;
	else if (!strnicmp(arg, (char*)"treetagger", 10))
	{
		if (arg[10] == '=') MakeUpperCopy(treetaggerParams, arg + 11);
		else CopyParam(treetaggerParams, "1");
	}
	else if (!strnicmp(arg, (char*)"encrypt=", 8)) CopyParam(encryptParams, arg + 8);
	else if (!strnicmp(arg, (char*)"decrypt=", 8)) CopyParam(decryptParams, arg + 8);
	else if (!stricmp(arg, (char*)"noautoreload")) autoreload = false;
	else if (!strnicmp(arg, (char*)"livedata=", 9))
	{
		CopyParam(livedataFolder, arg + 9);
		sprintf(systemFolder, (char*)"%s/SYSTEM", arg + 9);
		sprintf(languageFolder, (char*)"%s/%s", arg + 9, current_language);
	}
	else if (!strnicmp(arg, (char*)"nosuchbotrestart=", 17))
	{
		if (!stricmp(arg + 17, "true")) nosuchbotrestart = true;
		else nosuchbotrestart = false;
	}
	else if (!strnicmp(arg, (char*)"parseLimit=", 11))  parseLimit = atoi(arg + 11);
	else if (!strnicmp(arg, (char*)"system=", 7))  CopyParam(systemFolder, arg + 7);
	else if (!strnicmp(arg, (char*)"english=", 8))  CopyParam(languageFolder, arg + 8);
	else if (!strnicmp(arg, (char*)"db=", 3))  db = atoi(arg + 3);
	else if (!strnicmp(arg, (char*)"gzip=", 5))  gzip = atoi(arg + 5);
	else if (!strnicmp(arg, (char*)"python=", 7))  python = atoi(arg + 7);
	else if (!strnicmp(arg, (char*)"syslogstr=", 10)) CopyParam(syslogstr, arg + 10);
#ifndef DISCARDMYSQL
	else if (!strnicmp(arg, (char*)"mysql=", 6)) CopyParam(mysqlparams, arg + 6, 300);
#endif
	else if (!strnicmp(arg, (char*)"mssql=", 6)) CopyParam(mssqlparams, arg + 6, 300); // always here
#ifndef DISCARDPOSTGRES
	else if (!strnicmp(arg, (char*)"pguser=", 7)) CopyParam(postgresparams, arg + 7, 300);
	// Postgres Override SQL
	else if (!strnicmp(arg, (char*)"pguserread=", 11))
	{
		CopyParam(postgresuserread, arg + 11);
		pguserread = postgresuserread;
	}
	else if (!strnicmp(arg, (char*)"pguserinsert=", 13))
	{
		CopyParam(postgresuserinsert, arg + 13);
		pguserinsert = postgresuserinsert;
	}
	else if (!strnicmp(arg, (char*)"pguserupdate=", 13))
	{
		CopyParam(postgresuserupdate, arg + 13);
		pguserupdate = postgresuserupdate;
	}
#endif
#ifndef DISCARDWEBSOCKET
	else if (!strnicmp(arg, (char*)"websocket=", 10))
	{
		CopyParam(websocketparams, arg + 10, 300);
		server = true;
	}
	else if (!strnicmp(arg, (char*)"websocketbot=", 13)) strcpy(websocketbot, arg + 13);
	else if (!strnicmp(arg, (char*)"websocketmessage=", 17))  CopyParam(websocketmessage, arg + 17, 300);
#endif
#ifndef DISCARDMONGO
	else if (!strnicmp(arg, (char*)"mongo=", 6))
	{
		CopyParam(mongodbparams, arg + 6, MONGO_DBPARAM_SIZE);
	}
#endif
#ifndef DISCARDCLIENT
	else if (!strnicmp(arg, (char*)"client=", 7)) // client=1.2.3.4:1024  or  client=localhost:1024
	{
		server = false;
		client = true;
		char buffer[MAX_WORD_SIZE];
		CopyParam(serverIP, arg + 7);

		char* portVal = strchr(serverIP, ':');
		if (portVal)
		{
			*portVal = 0;
			port = atoi(portVal + 1);
		}

		if (!*loginID)
		{
			(*printer)((char*)"%s", (char*)"\r\nEnter client user name: ");
			ReadALine(buffer, stdin);
			(*printer)((char*)"%s", (char*)"\r\n");
			Client(buffer);
		}
		else Client(loginID);
		exit(0);
	}
#endif
	if (!strnicmp(arg, "userLimit=", 10))
		pendingUserLimit = atoi(arg + 10);
	if (!strnicmp(arg, "overflowMessage=",16))
	{
		strcpy(overflowMessage, arg + 17); // skip quote
		char* end = strchr(overflowMessage, '"');
		if (end) *end = 0;
		overflowLen = strlen(overflowMessage);
	}
	else if (!stricmp(arg, (char*)"nouserlog")) userLog = 0;
	else if (!strnicmp(arg, "userlogging=", 12))
	{
		userLog = NO_LOG;
		if (strstr(arg, "none")) userLog = NO_LOG;
		if (strstr(arg, "file")) userLog |= FILE_LOG;
		if (strstr(arg, "stdout")) userLog |= STDOUT_LOG;
		if (strstr(arg, "stderr")) userLog |= STDERR_LOG;
		if (strstr(arg, "prelog")) userLog |= PRE_LOG;
	}
	else if (!strnicmp(arg, "userlog", 7))
	{
		userLog = FILE_LOG;
		if (arg[7] == '=')
		{
			if (IsDigit(arg[8])) userLog = atoi(arg + 8);
			else  userLog = NO_LOG;
		}
	}
	else if (!stricmp(arg, "nopatterndata")) nopatterndata = true;
	else if (!stricmp(arg, "jabugpatch")) jabugpatch = true;
	else if (!strnicmp(arg, "autorestartdelay=", 17)) autorestartdelay = atoi(arg + 17);
	else if (!strnicmp(arg, "jmeter=", 7)) CopyParam(jmeter, arg + 7);
	else if (!stricmp(arg, (char*)"dieonwritefail")) dieonwritefail = true;
	else if (strstr(arg, (char*)"crnl_safe")) crnl_safe = true;
	else if (!strnicmp(arg, (char*)"blockapitrace=", 14)) blockapitrace = atoi(arg + 14);
	else if (!strnicmp(arg, (char*)"hidefromlog=", 12)) CopyParam(hide, arg + 12);
	else if (!strnicmp(arg, "serverlogauthcode=", 18))  CopyParam(serverlogauthcode, arg + 18);
	else if (!stricmp(arg, "noserverlog")) serverLog = NO_LOG;
	else if (!stricmp(arg, "pseudoserver")) pseudoServer = true;
	else if (!strnicmp(arg, "deployloggingdelay=",19)) startLogDelay = atoi(arg+19);
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
		if (arg[9] == '=' && IsDigit(arg[10])) serverLog = atoi(arg + 10);
		else serverLog = FILE_LOG | PRE_LOG;
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
	else if (!strnicmp(arg, "buglog", 6))
	{
		bugLog = FILE_LOG;
		if (arg[6] == '=')
		{
			if (IsDigit(arg[7])) bugLog |= atoi(arg + 7);
			else bugLog = NO_LOG;
		}
	}
	else if (!stricmp(arg, "jaErrorOnMissingElse")) errorOnMissingElse = true;
#ifndef DISCARDSERVER
	else if (!stricmp(arg, "serverretry")) serverRetryOK = true;
	else if (!stricmp(arg, "local")) server = false; // local standalone
	else if (!strnicmp(arg, (char*)"timelog=", 8)) timeLog = atoi(arg + 8);
	else if (!stricmp(arg, (char*)"serverctrlz")) serverctrlz = 1;
	else if (!strnicmp(arg, (char*)"port=", 5))  // be a server
	{
		port = atoi(arg + 5); // accept a port=
		server = true;
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
		sprintf(serverLogfileName, (char*)"%s/serverlog%u.txt", logsfolder, port); // DEFAULT LOG
		sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%u.txt", logsfolder, port);
#endif
	}
#ifdef EVSERVER
	else if (!strnicmp(arg, "fork=", 5))
	{
		forkcount = atoi(arg + 5);
		sprintf(forkCount, (char*)"evsrv:fork=%d", forkcount);
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
	else if (!strnicmp(arg, (char*)"interface=", 10)) interfaceKind = string(arg + 10); // specify interface
#endif
}

void ProcessArguments(int xargc, char* xargv[])
{
	for (int i = 1; i < xargc; ++i) ProcessArgument(xargv[i]);
}


static void ProcessConfigLines() {
	for (int i = 0; i < configLinesLength; i++) {
		ProcessArgument(TrimSpaces(configLines[i], true));
	}
}

static size_t ConfigCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

static void LoadconfigFromUrl(char* configUrl, char** configUrlHeaders, int headerCount) {
#ifndef DISCARDJSONOPEN 
	InitCurl();
	// use curl handle specifically for this processing
	CURL* req = curl_easy_init();
	string response_string;
	curl_easy_setopt(req, CURLOPT_CUSTOMREQUEST, "GET");
	curl_easy_setopt(req, CURLOPT_URL, configUrl);
	struct curl_slist* headers = NULL;
	for (int i = 0; i < headerCount; i++) headers = curl_slist_append(headers, configUrlHeaders[i]);
	headers = curl_slist_append(headers, "Accept: text/plain");
	headers = curl_slist_append(headers, "Cache-Control: no-cache");
	curl_easy_setopt(req, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(req, CURLOPT_WRITEFUNCTION, ConfigCallback);
	curl_easy_setopt(req, CURLOPT_WRITEDATA, &response_string);
	CURLcode ret = curl_easy_perform(req);
	char word[MAX_WORD_SIZE] = { '\0' };
	int wordcount = 0;
	for (unsigned int i = 0; i < response_string.size(); i++) {
		if (response_string[i] == '\n' || (response_string[i] == '\\' && response_string[i + 1] == 'n')) {
			configLines[configLinesLength] = mymalloc(sizeof(char) * MAX_WORD_SIZE);
			strcpy(configLines[configLinesLength++], TrimSpaces(word, true));
			for (int ti = 0; ti < MAX_WORD_SIZE; ti++)  word[ti] = '\0';
			wordcount = 0;
			if (!(response_string[i] == '\n')) i++;
		}
		else  word[wordcount++] = response_string[i];
	}
	curl_easy_cleanup(req);
	ProcessConfigLines();
#endif
}

static void ReadConfig()
{
	// various locations of config data
	for (unsigned int i = 1; i < argc; ++i) // essentials
	{
		char* configHeader;
		if (!strnicmp(argv[i], (char*)"config=", 7)) configFile = argv[i] + 7;
		if (!strnicmp(argv[i], (char*)"config2=", 7)) configFile2 = argv[i] + 8;
		if (!strnicmp(argv[i], (char*)"configUrl=", 10)) configUrl = argv[i] + 10;
		if (!strnicmp(argv[i], (char*)"configHeader=", 13))
		{
			configHeader = argv[i] + 13;
			configHeaders[headerCount] = mymalloc(sizeof(char) * strlen(configHeader) + 1);
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
		FClose(in);
	}
	in = FopenReadOnly(configFile2);
	if (in)
	{
		while (ReadALine(buffer, in, MAX_WORD_SIZE) >= 0)
		{
			ProcessArgument(TrimSpaces(buffer, true));
		}
		FClose(in);
	}
	sprintf(buffer, "cs_init%s", current_language);
	in = FopenReadOnly(buffer);
	if (in)
	{
		while (ReadALine(buffer, in, MAX_WORD_SIZE) >= 0)
		{
			ProcessArgument(TrimSpaces(buffer, true));
		}
		FClose(in);
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
	* mongodbparams = 0;
#endif
#ifndef DISCARDMYSQL
	* mysqlparams = 0;
#endif
#ifndef DISCARDMICROSOFTSQL
	* mssqlparams = 0;
#endif
#ifndef DISCARDPOSTGRES
	* postgresparams = 0;
#endif
#ifdef TREETAGGER
	* treetaggerParams = 0;
#endif
	* authorizations = 0;
	*loginID = 0;
}

static void SetDefaultLogsAndFolders()
{
	strcpy(usersfolder, (char*)"USERS");
	strcpy(logsfolder, (char*)"LOGS");
	strcpy(topicfolder, (char*)"TOPIC");
	strcpy(tmpfolder, (char*)"TMP");
	strcpy(current_language, (char*)"ENGLISH");
	strcpy(language_list, (char*)"ENGLISH");

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
	if (filesystemOverride == MYSQLFILES) sprintf(route, "    %s enabled. FileSystem routed MySql\r\n", MySQLVersion());
	else sprintf(route, "    %s enabled.\r\n", MySQLVersion());
#endif
#ifndef DISCARDMICROSOFTSQL
	if (usesql && *mssqlparams)
	{
		MsSqlUserFilesCode(mssqlparams);
	}
	if (filesystemOverride == MICROSOFTSQLFILES) sprintf(route, "    %s enabled. FileSystem routed MicrosoftSql\r\n", MsSqlVersion());
	else sprintf(route, "    %s enabled.\r\n", MsSqlVersion());
#endif
#ifndef DISCARDPOSTGRES
	if (usesql && *postgresparams)
	{
		PGInitUserFilesCode(postgresparams);
	}
	if (filesystemOverride == POSTGRESFILES) sprintf(route, "    %s enabled. FileSystem routed postgress\r\n", PostgresVersion());
	else sprintf(route, "    %s enabled.\r\n", PostgresVersion());
#endif
#ifndef DISCARDMONGO
	if (usesql && *mongodbparams)
	{
		MongoSystemInit(mongodbparams);
	}
	if (filesystemOverride == MONGOFILES) sprintf(route, "    %s enabled. FileSystem routed to MongoDB\r\n", MongoVersion());
	else sprintf(route, "    %s enabled.\r\n", MongoVersion());
#endif

	if (*route)
	{
		if (server) Log(SERVERLOG, route);
		else (*printer)(route);
	}
}

static void CommandLineBuild(unsigned int buildid,int i)
{
	char c = (buildid == BUILD0) ? '0' : '1';
	sprintf(logFilename, (char*)"%s/build%c_log.txt", usersfolder, c);
	FILE* in = FopenUTF8Write(logFilename);
	FClose(in);

	commandLineCompile = true;
	Rebegin(buildid, argc, argv); // reload dict and layers as needed
	InitBuild(buildid);

	// NO_Spell because there is no value in giving warning messages about spellings
	int result = ReadTopicFiles(argv[i] + 7, buildid, NO_SPELL);
	char msg[100];
	sprintf(msg, "build%c complete", c);
	myexit(msg, result);
}

unsigned int InitSystem(int argcx, char* argvx[], char* unchangedPath, char* readablePath, char* writeablePath, USERFILESYSTEM* userfiles, DEBUGAPI infn, DEBUGAPI outfn)
{ // this work mostly only happens on first startup, not on a restart

	GetCurrentDir(incomingDir, MAX_WORD_SIZE);
	sprintf(serverLogfileName, (char*)"LOGS/serverlog1024.txt");
	bool rootgiven = false;
	strcpy(websocketclient, "websocketbot");
	FILE* in;
	for (int i = 1; i < argcx; ++i)
	{
		if (!strnicmp(argvx[i], "root=", 5))
		{
			SetDirectory((char*)argvx[i] + 5);
			rootgiven = true;
		}
	}
	if (unchangedPath && *unchangedPath && !rootgiven) // presume this is our root
	{ // for DLL/sharedobject calls
		SetDirectory((char*)unchangedPath);
	}
	else
	{
		in = FopenStaticReadOnly((char*)"SRC/dictionarySystem.h"); // SRC/dictionarySystem.h
		if (!in) // if we are not at top level, try going up a level
		{
			if (!SetDirectory((char*)"..")) // move from BINARIES to top level
				myexit((char*)"FATAL unable to change up");
	}
		else FClose(in);
}
	GetCurrentDir(rootdir, MAX_WORD_SIZE);

	*externalBugLog = 0;
	*serverlogauthcode = 0;
	*websocketparams = 0;
	*websocketmessage = 0;
	*jmeter = 0;
	*buildflags = 0;
	GetPrimaryIP(myip);
	strcpy(baseLanguage, "ENGLISH"); // if no language given
	ClearGlobals();
	SetDefaultLogsAndFolders();
	memset(&userFileSystem, 0, sizeof(userFileSystem));
	InitFileSystem(unchangedPath, readablePath, writeablePath);
	if (userfiles) memcpy((void*)&userFileSystem, userfiles, sizeof(userFileSystem));

	// now memorize cs main directory
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
	ProcessArguments(argc, argv);
	MakeDirectory(tmpfolder);

	sprintf(logFilename, (char*)"%s/log%u.txt", logsfolder, port); // DEFAULT LOG
#ifdef LINUX
	if (forkcount > 1) {
		sprintf(serverLogfileName, (char*)"%s/serverlog%u-%u.txt", logsfolder, port, getpid());
		sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%u-%u.txt", logsfolder, port, getpid());
	}
	else
	{
		sprintf(serverLogfileName, (char*)"%s/serverlog%u.txt", logsfolder, port);
		sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%u.txt", logsfolder, port);
	}
#else
	sprintf(serverLogfileName, (char*)"%s/serverlog%u.txt", logsfolder, port); // DEFAULT LOG
	sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%u.txt", logsfolder, port);
#endif
	sprintf(languageFolder, "LIVEDATA/%s", current_language); // default directory for dynamic stuff

	if (redo) autonumber = true;

	int oldserverlog = serverLog;
	serverLog = FILE_LOG;

#ifdef USESIGNALHANDLER
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

	for (unsigned int i = 1; i < argc; ++i) // before createsystem to avoid loading unneeded layers
	{
		if (!strnicmp(argv[i], (char*)"build0=", 7)) build0Requested = true;
		if (!strnicmp(argv[i], (char*)"build1=", 7)) build1Requested = true;
	}

	CreateSystem();

	// Potentially use external databases for the filesystem
	OpenExternalDatabase();

	// system is ready.

	for (unsigned int i = 1; i < argc; ++i) // now for immediate action arguments
	{
#ifndef DISCARDSCRIPTCOMPILER
		if (!strnicmp(argv[i], (char*)"build0=", 7))
		{
			CommandLineBuild(BUILD0,i);
		}
		if (!strnicmp(argv[i], (char*)"build1=", 7))
		{
			CommandLineBuild(BUILD1,i);
		}
#endif
#ifndef DISCARDTESTING
		if (!strnicmp(argv[i], (char*)"debug=", 6))
		{
			char* ptr = SkipWhitespace(argv[i] + 6);
			commandLineCompile = true;
			if (*ptr == ':') DoCommand(ptr, ourMainOutputBuffer);
			myexit((char*)"test complete");
		}
#endif
	}

	// evserver init AFTER db init so all children inherit the one db connection
	if (server)
	{
		struct tm ptm;
#ifndef EVSERVER
		Log(ECHOSERVERLOG, "\r\n\r\n======== Began server %s compiled: %s on host: %s port: %d at %s serverlog:%d userlog: %d\r\n", version, compileDate, hostname, port, GetTimeInfo(&ptm, true), oldserverlog, userLog);
#else
		Log(ECHOSERVERLOG, "\r\n\r\n======== Began EV server pid %d %s compiled: %s on host: %s port: %d at %s serverlog:%d userlog: %d\r\n", getpid(), version, compileDate, hostname, port, GetTimeInfo(&ptm, true), oldserverlog, userLog);
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
	ResetToPreUser();
#ifdef NOMAIN
	RestoreCallingDirectory(); // if dll called it
#endif
	return 0;
}

void PartiallyCloseSystem(bool keepalien) // server data (queues, databases, etc) remain available
{
	build0Requested = build1Requested = false;
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
	static bool closed = false;
	if (closed) return; // already doing it
	closed = true;

	PartiallyCloseSystem();
#ifndef DISCARD_PYTHON
	if (python) Py_FinalizeEx();
#endif
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
	FreeServerLog();
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

	if (ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}
#endif

int FindOOBEnd(unsigned int start)
{
	unsigned int begin = start;
	if (*wordStarts[start] == OOB_START)
	{
		while (++start < wordCount && *wordStarts[start] != OOB_END); // find a close
		return (wordStarts[start] && *wordStarts[start] == OOB_END) ? (start + 1) : begin;
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
			if (*at == '"' && *(at - 1) != '\\') quote = !quote;
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
			char* atptr = strstr(ptr, (char*)"loopback="); // loopback is reset after every user output
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
			atptr = strstr(ptr, (char*)"callback="); // call back is canceled if user gets there first
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
			atptr = strstr(ptr, (char*)"alarm="); // alarm stays pending until it launches
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
			if (!oob) memmove(output, end + 1, strlen(end)); // delete oob data so not printed to user
		}
	}
}

bool ProcessInputDelays(char* buffer, bool hitkey)
{
	// override input for a callback
	if (callBackDelay || loopBackDelay || alarmDelay) // want control back to chatbot when user has nothing
	{
		uint64 milli = ElapsedMilliseconds();
		if (!hitkey && (sourceFile == stdin || !sourceFile))
		{
			if (loopBackDelay && milli > (uint64)loopBackTime)
			{
				strcpy(buffer, (char*)"[ loopback ]");
				(*printer)((char*)"%s", (char*)"\r\n");
			}
			else if (callBackDelay && milli > (uint64)callBackTime)
			{
				strcpy(buffer, (char*)"[ callback ]");
				(*printer)((char*)"%s", (char*)"\r\n");
				callBackDelay = 0; // used up
			}
			else if (alarmDelay && milli > (uint64)alarmTime)
			{
				alarmDelay = 0;
				strcpy(buffer, (char*)"[ alarm ]");
				(*printer)((char*)"%s", (char*)"\r\n");
			}
			else return true; // nonblocking check for input
		}
		if (alarmDelay && milli > (uint64)alarmTime) // even if he hits a key, alarm has priority
		{
			alarmDelay = 0;
			strcpy(buffer, (char*)"[ alarm ]");
			(*printer)((char*)"%s", (char*)"\r\n");
		}
	}
	return false;
}

char* ReviseOutput(char* out, char* prefix)
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

void ExecuteVolleyFile(FILE* sourceFile)
{
	int actualSize = maxBufferSize;
	char* oldinput = ourMainInputBuffer;
	if (sourceFile != stdin)
	{
		fseek(sourceFile, 0, SEEK_END);
		actualSize = (int)ftell(sourceFile);
		actualSize += (actualSize / 5);
		fseek(sourceFile, 0, SEEK_SET);
		ourMainInputBuffer = mymalloc(actualSize); // room for file + spaces at end of lines
	}
	char* ptr = ourMainInputBuffer;
	while (fgets(ptr, actualSize, sourceFile))
	{
		if (HasUTF8BOM(ptr))  memmove(ptr, ptr + 3, strlen(ptr + 2));// UTF8 BOM
		char* x = TrimSpaces(ptr);
		if (!*x && sourceFile != stdin) continue;
		size_t len = strlen(ptr);
		if (ptr[len - 1] == '\n') ptr[--len] = 0;
		if (ptr[len - 1] == '\r') ptr[--len] = 0;
		if (!*ptr && sourceFile == stdin) break; // std in can end for a round
		if (ptr[len - 1] == '-' && IsAlphaUTF8(ptr[len - 2]))  --len;  // word break, remove it
		else ptr[len++] = ' '; // separate words from different lines
		ptr += len; // ignore blank lines
	}
	*ptr = 0;
	if (sourceFile != stdin) FClose(sourceFile);
	
	callStartTime = ElapsedMilliseconds();
	PerformChat(loginID, computerID, ourMainInputBuffer, NULL, ourMainOutputBuffer);
	printf("Volley Complete: %s\r\n", ourMainOutputBuffer);
	if (sourceFile != stdin) myfree(ourMainInputBuffer);

	ourMainInputBuffer = oldinput;
	sourceFile = stdin;
}

static void SeparateData(char* input)
{
	static bool inited = false;
	if (!inited)
	{
		inited = true;
		for (int i = 0; i < 15; ++i) tsvInputs[i] = mymalloc(20000);
	}

	// input is id, category, specialty, message, speaker, rating
	char* tab;
	tsvIndex = 0;
	while ((tab = strchr(input, '\t'))) 
	{
		if (tsvIndex == 3)
		{ // tsv file is allowing unescaped tabs in user input, patch
			char* customer = strstr(input, "\tCustomer");
			if (customer) tab = customer;
			char* professional = strstr(input, "\tProfessional");
			if (professional) tab = professional;
		}
			
		*tab = 0;
		strcpy(tsvInputs[tsvIndex++] ,input);
		input = tab + 1;
	}
	char* end = strchr(input, '\r');
	if (end) *end = 0;
	end = strchr(input, '\n');
	if (end) *end = 0;
	strcpy(tsvInputs[tsvIndex++] ,input);
	*tsvInputs[tsvIndex] = 0;
}

static char* GetTSVLine()
{
	if (fgets(ourMainInputBuffer, fullInputLimit - 100, tsvFile) == NULL) return NULL;
	size_t x = strlen(ourMainInputBuffer);
	SeparateData(ourMainInputBuffer);

	// ID	Category	Sub	Message	Author	Star	
	strcpy(loginID, tsvInputs[tsvLoginField]);
	strcpy(ourMainInputBuffer, tsvInputs[tsvMessageField]);
	strcpy(speaker, tsvInputs[tsvSpeakerField]);

	return ourMainInputBuffer;
}

static bool UseTSVInput()
{
	while (1) // maybe we skip over some data
	{
		strcpy(tsvpriorlogin, loginID);
		char skipID[MAX_WORD_SIZE];

		// get next volley
		if (GetTSVLine() == NULL)
		{
			return false;
		}

		// ignore 1st n
		while(tsvSkip)
		{
			--tsvSkip;
			strcpy(skipID, loginID);
			while (GetTSVLine() != NULL && !stricmp(loginID, skipID)) {}
		}

		// 1st startup
		if (!*tsvpriorlogin)
		{
			// skip 1st line which comes from pearl conversation
			if (GetTSVLine() == NULL)
			{
				*ourMainInputBuffer = 0;
				return false;
			}
		}

		// new user?
		else if (*tsvpriorlogin && stricmp(loginID, tsvpriorlogin)) // changinge users - ignore pearl data 1st volley
		{
			char copy[MAX_WORD_SIZE];
			strcpy(copy, loginID);
			strcpy(loginID, tsvpriorlogin);

			Login(tsvpriorlogin, "", "", "");
			ResetMode buildReset = MILD_RESET;
			GetUserData(buildReset, "");

			FunctionResult result = InternalCall("^GambitCode", GambitCode, "~cs_tsvsource", (char*)"", NULL, currentOutputBase);
			
			strcpy(loginID, copy);
			ResetToPreUser();

			if (--tsvCount <= 0)
			{
				*ourMainInputBuffer = 0;
				return false;
			}

			// skip 1st line which comes from pearl conversation
			if (GetTSVLine() == NULL)
			{
				*ourMainInputBuffer = 0;
				return false;
			}
		}

		// are we ignoring lines from Professional?
		if (!stricmp(speaker, "Professional")) continue;

		// we have next input
		break;
	}
	return true;
}

bool GetInput()
{
	/*
	* Simple inputs are single lines to the current user.
	* 
	* Similarly, one can :source  filename to switch inputs of single lines to the current user.
	* 
	* :tsvsource handles single lines that specificy what user to use and
	* can pass along additional data. When we change users, it executes a closure topic.
	* 
	* DocumentMode is yet another input where lines all come from a document and are all
	* treated as a single volley.
	*/

	// start out with input
	if (ourMainInputBuffer[1]) { ; } // callback in progress, data put into buffer, read his input later, but will be out of date
	else if (documentMode)
	{
		if (!ReadDocument(ourMainInputBuffer + 1, sourceFile)) return true;
	}
	else // normal input reading
	{
		if (sourceFile == stdin && !volleyFile) // user-based input std input
		{
			if (ide) return true; // only are here because input is entered
			else if (ReadALine(ourMainInputBuffer + 1, sourceFile, fullInputLimit - 100,
				false, true, false)
				< 0) 
				return true; // end of input
		}
		else if (tsvFile)
		{
			if (!UseTSVInput()) // ran dry
			{
				FClose(tsvFile); 
				*speaker = 0;
				tsvFile = NULL;
				sourceFile = stdin;
				GetInput();
				printf("\r\nResuming std input\r\n");
				return false; 
			}
		}

		// file based input
		else
		{
			if (volleyFile) 
			{
				ExecuteVolleyFile(sourceFile); 
				return false;
			}

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
				memmove(ourMainInputBuffer + 1, ptr, strlen(ptr) + 1);
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
	char* expect = NULL;
	while (ALWAYS)
	{
		if (*oktest) // self test using OK or WHY as input
		{
			(*printer)((char*)"%s\r\n    ", UTF2ExtendedAscii(ourMainOutputBuffer));
			strcpy(ourMainInputBuffer, oktest);
		}
		else if (quitting) 
			return;
		else if (buildReset)
		{
			(*printer)((char*)"%s\r\n", UTF2ExtendedAscii(ourMainOutputBuffer));
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
			if (!*ourMainInputBuffer)
			{
				if (allNoStay) strcpy(loginID, priorLogin);
				break;     // failed to get input
			}
		}

		// we have input, process it
		callStartTime = ElapsedMilliseconds();
		if (!server && extraTopicData)
		{
			turn = PerformChatGivenTopic(loginID, computerID, ourMainInputBuffer, NULL, ourMainOutputBuffer, extraTopicData);
			myfree(extraTopicData);
			extraTopicData = NULL;
		}
		else
		{
			originalUserInput = ourMainInputBuffer;
			if (integrationTest)
			{
				char* flip = ourMainInputBuffer;
				while ((flip = FindJMSeparator(flip, ','))) *flip = '\t';

				char* ptr = ourMainInputBuffer;
				{
					char* input = AllocateBuffer();
					char* output = AllocateBuffer();
					// RunTest, Suite, Comment, Name, Input, Response, , , , , , , ,
					ptr = ReadTabField(ptr, input); // runtest
					if (stricmp(input, "yes"))
					{
						ourMainInputBuffer[1] = 0; // move on to next inpuit
						continue;
					}

					ptr = ReadTabField(ptr, input); // suite
					ptr = ReadTabField(ptr, input); // comment
					ptr = ReadTabField(ptr, input); // name
					ptr = ReadTabField(ptr, input); // input
					ptr = ReadTabField(ptr, output); // output
					strcpy(ourMainInputBuffer, input+1);
					char* end = strrchr(ourMainInputBuffer, '"');
					*end = 0; // remove trailing quote of field
					size_t l = strlen(ourMainInputBuffer);
					expect = ourMainInputBuffer + l + 1;
					strcpy(expect, output+2); // hide in input buffer again
					end = strrchr(expect, '"');
					end -= 2;
					*end = 0; // remove trailing "}"  of field
					if (*(end - 1) == ' ') *(end - 1) = 0; // trailing space?
					FreeBuffer();
					FreeBuffer();
				}
			}

			turn = PerformChat(loginID, computerID, ourMainInputBuffer, NULL, ourMainOutputBuffer); // no ip
		
			if (expect && integrationTest)
			{
				char* actual = GetActual(ourMainOutputBuffer);
				char* x  = strchr(actual, ':');
				if (x && x[1] == ' ') memmove(x + 1, x + 2, strlen(x + 1)); // our json adds space after : 

				if (strcmp(actual, expect))
				{
					printf("input: %s\r\n", ourMainInputBuffer);
					char* e1 = expect;
					char* a1 = actual;
					while (*e1++ == *a1++ && *e1);
					printf("input: %s\r\n%s  expected\r\n%s  actual\r\n%s  diff expected\r\n%s  diff actual\r\n", ourMainInputBuffer, expect,actual,e1 -1, a1 -1);
				}
				else
				{
					int xx = 0;
				}
				expect = NULL;
			}
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
		printf("Sourcefile Time used %ld ms for %u sentences %u tokens.\r\n", diff, sourceLines, sourceTokens);
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
	callStartTime = ElapsedMilliseconds();
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

void ResetToPreUser(bool saveBuffers) // prepare for multiple sentences being processed - data lingers over multiple sentences
{
	currentMemoryFact = NULL;
	testpatterninput = NULL;
	postProcessing = false;
	stackFree = stackStart; // drop any possible stack used
	csapicall = NO_API_CALL;
	if (!saveBuffers) bufferIndex = baseBufferIndex; // return to default basic buffers used so far, in case of crash and recovery
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

void ComputeWhy(char* buffer, int n)
{
	strcpy(buffer, (char*)"Why:");
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
		char* more = GetRuleIDFromText(responseData[order].id, id); // has no label in text
		char* rule = GetRule(topicid, id);
		GetLabel(rule, label);
		char c = *more;
		*more = 0;
		sprintf(buffer, (char*)"%s%s", GetTopicName(topicid), responseData[order].id); // topic and rule 
		buffer += strlen(buffer);
		if (*label)
		{
			sprintf(buffer, (char*)"=%s", label); // label if any
			buffer += strlen(buffer);
		}
		*more = c;

		// was there a relay
		if (*more)
		{
			topicid = atoi(more + 1); // topic number
			more = strchr(more + 1, '.'); // r top level + rejoinder
			char* dotinfo = more;
			more = GetRuleIDFromText(more, id);
			char* ruleptr = GetRule(topicid, id);
			GetLabel(ruleptr, label);
			sprintf(buffer, (char*)".%s%s", GetTopicName(topicid), dotinfo); // topic and rule 
			buffer += strlen(buffer);
			if (*label)
			{
				sprintf(buffer, (char*)"=%s", label); // label if any
				buffer += strlen(buffer);
			}
		}
		strcpy(buffer, (char*)" ");
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
			unsigned int count;
			char* starts[MAX_SENTENCE_LENGTH];
			memset(starts, 0, sizeof(char*) * MAX_SENTENCE_LENGTH);
			ptr = Tokenize(ptr, count, (char**)starts, NULL, false);   //   only used to locate end of sentence but can also affect tokenFlags (no longer care)
			char c = *ptr; // is there another sentence after this?
			char c1 = 0;
			if (c)
			{
				c1 = *(ptr - 1);
				*(ptr - 1) = 0; // kill the separator 
			}

			//   save sentences as facts
			char* copy = InfiniteStack(limit, "FactizeResult"); // localized
			char* out = copy;
			char* at = old - 1;
			while (*++at) // copy message and alter some stuff like space or cr lf
			{
				if (*at == '\r' || *at == '\n' || *at == '\t') *out++ = ' '; // ignore special characters in fact
				else *out++ = *at;
			}
			*out = 0;
			CompleteBindStack();
			if ((out - copy) > 2) // we did copy something, make a fact of it
			{
				char name[MAX_WORD_SIZE];
				sprintf(name, (char*)"%s.%s", GetTopicName(responseData[order].topic), responseData[order].id);
				CreateFact(MakeMeaning(StoreWord(copy, AS_IS)), Mchatoutput, MakeMeaning(StoreWord(name)), FACTTRANSIENT);
			}
			if (c) *(ptr - 1) = c1;
			ReleaseStack(copy);
		}
	}
	tokenControl = control;
	FreeBuffer();
}

static void ConcatResult(char* result, char* limit)
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
			AddBotUsed(reply, strlen(reply));
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
		if (start != result)  *result++ = ENDUNIT; // add separating item from last unit for log detection
		strcpy(result, data);
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
		unsigned int oldtrace = trace;
		if (!postprocess) trace = 0; // :show postprocess 0
		OnceCode((char*)"$cs_control_post", postvalue);
		trace = oldtrace;
		--outputNest;
		postProcessing = false;

		char* at = output;
		if (autonumber)
		{
			sprintf(at, (char*)"%u: ", volleyCount);
			at += strlen(at);
		}
		ConcatResult(at, output + limit - 100); // save space for after data
		char* val = GetUserVariable("$cs_outputlimit", false);
		if (*val && !csapicall)
		{
			int lim = atoi(val);
			int size = strlen(at);
			if (size > lim) ReportBug("INFO: OutputSize exceeded %d > %d for %s\n", size, lim, at);
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

		if (userLog && prepareMode != POS_MODE && prepareMode != PREPARE_MODE && prepareMode != TOKENIZE_MODE)
		{
			unsigned int lapsedMilliseconds = (unsigned int)(ElapsedMilliseconds() - volleyStartTime);
			char time15[MAX_WORD_SIZE];
			sprintf(time15, (char*)" F:%ums ", lapsedMilliseconds);
			char* nl = (LogEndedCleanly()) ? (char*)"" : (char*)"\r\n";
			char* poutput = Purify(output);
			char* userInput = NULL;
			char endInput = 0;
			char* userOutput = NULL;
			char endOutput = 0;
			if (strstr(hide, "botmessage")) {
				// allow tracing of OOB but thats all
				userOutput = BalanceParen(poutput, false, false);
				if (userOutput) {
					endOutput = *userOutput;
					*userOutput = 0;
				}
			}
			if (originalUserInput && *originalUserInput) {
				if (strstr(hide, "usermessage")) {
					// allow tracing of OOB but thats all
					userInput = BalanceParen(originalUserInput, false, false);
					if (userInput) {
						endInput = *userInput;
						*userInput = 0;
					}
				}
			}

			if (*GetUserVariable("$cs_showtime", false))
				printf("%s\r\n", time15);

			if (userInput) *userInput = endInput;
			if (userOutput) *userOutput = endOutput;
			if (shortPos)
			{
				Log(USERLOG, "%s", DumpAnalysis(1, wordCount, posValues, (char*)"Tagged POS", false, true));
				Log(USERLOG, "\r\n");
			}
		}

		// now convert output separators between rule outputs to space from ' for user display result (log has ', user sees nothing extra)
		if (prepareMode != REGRESS_MODE && responseIndex > 1) // api calls only have single response, and can use ` safely in data
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

int PerformChatGivenTopic(char* user, char* usee, char* incoming, char* ip, char* output, char* topicData)
{
	CreateFakeTopics(topicData);
	originalUserInput = incoming;
	int answer = PerformChat(user, usee, incoming, ip, output);
	ReleaseFakeTopics();
	return answer;
}

void EmergencyResetUser()
{
	globalDepth = 0;
	responseIndex = 0;
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
			if ((!at[1] || !at[2]) && *starttoken == '"' && at != starttoken) // no global quotes, (may have space at end)
			{
				*at = ' ';
				*starttoken = ' ';
			}
		}
		if (*at == ' ' && !quote) // proper token separator
		{
			if ((at - starttoken) > (MAX_WORD_SIZE - 1)) // trouble - token will be too big, drop excess
			{
				ReportBug("INFO: User input token size too big %d vs %d for ==> %s ", (at - starttoken), MAX_WORD_SIZE, starttoken);
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
		ReportBug("INFO: User input token size too big %d vs %d for ==> %s ", (at - starttoken), MAX_WORD_SIZE, starttoken);
			starttoken[MAX_WORD_SIZE - 10] = 0;
	}
}

static int Crashed(char* output, bool reloading, char* ip)
{
	if (crashBack) myexit((char*)"continued crash exit"); // if calling crashfunction crashes
	lastCrashTime = ElapsedMilliseconds();
	crashBack = true;
	if (reloading) myexit((char*)"crash on reloading"); // user input crashed us immediately again. give up
	char* topicName = GetUserVariable("$cs_crash", false);
	char* varmsg = GetUserVariable("$cs_crashmsg", false);
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
	char* sizing = GetUserVariable("$cs_inputlimit", false);
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

int PerformChat(char* user, char* usee, char* incomingmessage, char* ip, char* output) // returns volleycount or 0 if command done or -1 PENDING_RESTART
{ //   primary entrypoint for chatbot -- null incoming treated as conversation start.
	if (HasUTF8BOM(incomingmessage)) incomingmessage += 3; // skip UTF8 BOM (from file)
	incomingmessage = SkipWhitespace(incomingmessage);
	currentRuleOutputBase = currentOutputBase = ourMainOutputBuffer = output;
	output[0] = 0;
	ClearSupplementalInput();
	retrycheat = false;
	if (!strnicmp(incomingmessage, "cheat retry ", 12))
	{
		memmove(incomingmessage, "     :", 6);
		retrycheat = true;
	}
	else if (!strnicmp(incomingmessage, "cheat rever ", 12))
	{
		memmove(incomingmessage, ":retry      ", 12);
		retrycheat = true;
	}
	
	http_response = 0;
	// dont start immediately if requested after a crash
	if (crashBack && autorestartdelay) 
	{
		uint64 delay = ElapsedMilliseconds() - lastCrashTime;
		if (delay < (uint64)autorestartdelay)
		{
			return 0;
		}
	}
	
#ifdef NOMAIN
	callStartTime = ElapsedMilliseconds(); // not thru our control (either server or standalone)
#endif
	GetCurrentDir(incomingDir, MAX_WORD_SIZE);
	char* timecheat = GetUserVariable("$FakeTimeOffset");
	if (*timecheat) callStartTime -= atoi(timecheat);
	*repairinput = 0;
	softRestart = false;
	timeout = false;
	SetDirectory(rootdir); // an outside caller cant be trusted
	currentInput = "";
	buildtransientjid = 0;
	builduserjid = 0;
	char* safeInput = NULL; // will become base of stack allocation for input buffer

	CheckForOutputAPI(incomingmessage); // set flag if is such a call -- alters what  PurifyInput does later 
	incomingmessage = SkipWhitespace(incomingmessage); 
	originalUserInput = incomingmessage; // a global of entire input

	if (servertrace) trace = (unsigned int) -1; // force tracing always
	volleyStartTime = ElapsedMilliseconds(); // time limit control
	startNLTime = ElapsedMilliseconds();
	holdServerLog = serverLog; // global remember so we can restore against local change
	holdUserLog = userLog;
	char holdLanguage[100];
	strcpy(holdLanguage, current_language);
	chatstarted = ElapsedMilliseconds(); // for timing stats
	debugcommand = false;
	ResetToPreUser();
	InitJson();
	InitStats();
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
	static HOOKPTR fns = FindHookFunction((char*)"PerformChatArguments");
	static HOOKPTR fne = FindHookFunction((char*)"EndChat");
	if (fns)
	{
		((PerformChatArgumentsHOOKFN)fns)(user, usee, incomingmessage);
}
#endif

	LoggingCheats(incomingmessage);

	if (!ip) ip = "";
	bool fakeContinue = Login(user, usee, ip, incomingmessage); //   get the participants names, now in caller and callee

	if (!crnl_safe) Clear_CRNL(originalUserInput); // we dont allow returns and newlines to damage logs
	Prelog(caller, callee, originalUserInput);

	SetLanguage(baseLanguage);
	SetUserVariable("$cs_language", baseLanguage);

	FILE* in = fopen("restart.txt", (char*)"rb"); // per external file created, enable server log
	if (in) // trigger restart
	{
		restartBack = true;
		FClose(in);
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
		restartBack = crashBack = false;
		restartfromdeath = true;
	}

#ifdef LINUX
	if (sigsetjmp(crashJump, 1)) // if crashes come back to here
#else
	if (setjmp(crashJump)) // if crashes, come back to here
#endif
	{
		int count = Crashed(output, reloading, ip);
		RESTORE_LOGGING();
#ifdef PRIVATE_CODE
		if (fne) ((EndChatHOOKFN)fne)();   // finish the chat
#endif
		RestoreCallingDirectory();
		serverLog = holdServerLog; // global remember so we can restore against local change
		userLog = holdUserLog ;
		return count;
	}
#ifndef HARDCRASH
#ifdef WIN32
	__try  // catch crashes in windows
#else
	try 
#endif
#endif
{
		if (!strnicmp(incomingmessage, ":reset:", 7))
		{
			buildReset = FULL_RESET;
			incomingmessage += 7;
		}

		uint64 oldtoken = tokenControl;
		parseLimited = false;

		GetUserData(buildReset, originalUserInput); // load user data to have $cs_token control over purifyinput (now lost)
		if (priortrace) trace = priortrace;

		// prelog done before purify, to know what log what we got exactly, now we change
		// now see if we should ingest oob directly rather than waiting for tokenize
		bool directoob = false;
		if (*incomingmessage == '[' && !stricmp(GetUserVariable("$cs_directfromoob"), "true"))
		{
			uint64 oldcontrol = tokenControl;
			tokenControl |= JSON_DIRECT_FROM_OOB;
			directoob = true;
			loading = true; // block fact language assign, so oob json visible to all languages
			unsigned int count = 0;
			char* starts[256];
			char separators[MAX_SENTENCE_LENGTH];
			// strip off oob into single token
			char* ptr = Tokenize(incomingmessage, count, starts, separators, false, true);
			loading = false; 
			// we have oob message and enough write to patch into it, so now message is oob + rest
			if (count == 3 && ptr > incomingmessage + 16)
			{
				char* before = ptr - 15;
				repairat = before;
				strncpy(repairinput, before, 15);
				sprintf(before, "[%s]", starts[2]);
				char* end = before + strlen(before);
				while (end < ptr) *end++ = ' '; // blank til real message
				incomingmessage = before; // new input start, purify only user message itself
		}
			tokenControl = oldcontrol;
		}

		// clean up input, with a copy of the input on stack and perhaps in json facts
		char* limit;
		size_t size;
		safeInput = InfiniteStack(limit, "PerformChat"); // dynamically allocate a copy of the input we can change
		originalUserMessage = PurifyInput(incomingmessage, safeInput, size, INPUT_PURIFY); // change out special or illegal characters
		CompleteBindStack(size);
		if (!originalUserMessage) originalUserMessage = safeInput; // all user message
		originalUserMessage = SkipWhitespace(originalUserMessage); // for sure its user input
		CheckParseLimit(originalUserMessage);
		LimitUserInput(originalUserMessage);
		ValidateUserInput(originalUserMessage); // insure no large tokensize other than strings
		
		char* first = originalUserMessage;

#ifndef DISCARDTESTING
#ifndef NOMAIN
		if (!server && !volleyFile && !(*first == ':' && IsAlphaUTF8(first[1]))) strcpy(revertBuffer, first); // for a possible revert
#endif
#endif

		bool hadIncoming = *originalUserMessage != 0 || documentMode;
		if (!*computerID && *originalUserMessage != ':') ReportBug("No computer id before user load?");

			// else documentMode
			if (fakeContinue)
			{
				RESTORE_LOGGING();
#ifdef PRIVATE_CODE
				if (fne) ((EndChatHOOKFN)fne)();   // finish the chat
#endif
				RestoreCallingDirectory();
				ReleaseStack(safeInput);
				return volleyCount;
			}
		if (!*computerID && (!originalUserMessage || *originalUserMessage != ':'))  // a  command will be allowed to execute independent of bot- ":build" works to create a bot
		{
			strcpy(output, (char*)"No such bot.\r\n");
			char* fact = "no default fact";
			WORDP D = FindWord((char*)"defaultbot",0,LOWERCASE_LOOKUP);
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
#ifdef PRIVATE_CODE
			if (fne) ((EndChatHOOKFN)fne)();   // finish the chat
#endif
			RestoreCallingDirectory();
			ReleaseStack(safeInput);
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
			userFirstLine = volleyCount + 1;
			responseIndex = 0;
		}

		AddInput(safeInput, 0, true);

		ok = ProcessInput();
		if (ok <= 0)
		{
			if (userlogFile)
			{
				FClose(userlogFile);
				userlogFile = NULL;
			}
			RESTORE_LOGGING();
#ifdef PRIVATE_CODE
			if (fne) ((EndChatHOOKFN)fne)();   // finish the chat
#endif
			RestoreCallingDirectory();
			serverLog = holdServerLog; // global remember so we can restore against local change
			userLog = holdUserLog;
			ReleaseStack(safeInput);
			return ok; // command processed
		}

		if (!server) // refresh prompts from a loaded bot since mainloop happens before user is loaded
		{
			WORDP dBotPrefix = FindWord((char*)"$botprompt");
			strcpy(botPrefix, (dBotPrefix && dBotPrefix->w.userValue) ? dBotPrefix->w.userValue : (char*)"");
			WORDP dUserPrefix = FindWord((char*)"$userprompt");
			strcpy(userPrefix, (dUserPrefix && dUserPrefix->w.userValue) ? dUserPrefix->w.userValue : (char*)"");
		}

		// compute response and hide additional information after it about why
		FinishVolley(output, NULL, outputsize); // use original input main buffer, so :user and :bot can cancel for a restart of concerasation

		LogChat(startNLTime, user, usee, ip, volleyCount, ourMainInputBuffer, output, 0);

		if (parseLimited) tokenControl = oldtoken;

		if (softRestart)
		{
			ReturnBeforeBootLayer(); // erase it
			buildbootjid = 0;
			char* buffer = AllocateBuffer();
			strcpy(buffer, ourMainOutputBuffer);
			Callback(FindWord("^csboot"), (char*)"()", true, true); // do before world is locked
			NoteBotVariables(); // convert user variables read into bot variables in boot layer
			LockLayer();
			strcpy(ourMainOutputBuffer,buffer); // remove any boot message
			FreeBuffer();
			myBot = 0;	// restore fact owner to generic all
		}

#ifndef DISCARDJAVASCRIPT
		DeleteTransientJavaScript(); // unload context if there
#endif

		if (*GetUserVariable("$cs_summary", false))
		{
			chatstarted = ElapsedMilliseconds() - chatstarted;
			printf("Summary-  Prepare: %d  Reply: %d  Finish: %d\r\n", (int)preparationtime, (int)replytime, (int)chatstarted);
		}
		RESTORE_LOGGING();
#ifdef PRIVATE_CODE
		if (fne) ((EndChatHOOKFN)fne)();   // finish the chat
#endif
	} // end of try
#ifndef HARDCRASH
#ifdef WIN32
	__except (true)
#else
	catch (...)
#endif
	{
		RESTORE_LOGGING();
#ifdef PRIVATE_CODE
		if (fne) ((EndChatHOOKFN)fne)();   // finish the chat
#endif
		char word[MAX_WORD_SIZE];
		sprintf(word, (char*)"Try/catch");
		Log(SERVERLOG, word);
		ReportBug("fatal %s", word);
#ifdef LINUX
		siglongjmp(crashJump, 1);
#else
		longjmp(crashJump, 1);
#endif
	}
#endif // hardcrash

	if (safeInput) ReleaseStack(safeInput);

	if (userlogFile)
	{
		FClose(userlogFile);
		userlogFile = NULL;
	}
	RestoreCallingDirectory();
	serverLog = holdServerLog; // global remember so we can restore against local change
	userLog = holdUserLog;
	return volleyCount;
}

FunctionResult Reply()
{
	int depth = globalDepth;
	callback = (wordCount > 1 && *wordStarts[1] == OOB_START && (!stricmp(wordStarts[2], (char*)"callback") || !stricmp(wordStarts[2], (char*)"alarm") || !stricmp(wordStarts[2], (char*)"loopback"))); // dont write temp save
	withinLoop = 0;
	choiceCount = 0;
	ResetOutput();
	ResetTopicReply();
	ResetReuseSafety();
	if (trace & TRACE_OUTPUT)
	{
		Log(USERLOG, "\r\n\r\nReply to: ");
		for (unsigned int i = 1; i <= wordCount; ++i) Log(USERLOG, "%s ", wordStarts[i]);
		Log(USERLOG, "\r\n  Pending topics: %s\r\n", ShowPendingTopics());
	}
	FunctionResult result = NOPROBLEM_BIT;
#ifndef NOMAIN
	char* labelbreak = strstr(originalUserInput, "#!!#"); // input src verify data header  ~AI_en_US $language_context = en_US 
	if (labelbreak) 
	{
		char vlabel[MAX_WORD_SIZE];
		char* ptr = ReadCompiledWord(originalUserInput, vlabel);
		if (!stricmp(vlabel,"verify"))
		{
			char* before = ptr;
			char var[MAX_WORD_SIZE];
			ptr = ReadCompiledWord(ptr, var);
			if (*var == '$')
			{
				ptr = ReadCompiledWord(ptr, vlabel);
				if (*vlabel == '=') // assign value to user variable
				{
					ptr = ReadCompiledWord(ptr, vlabel);
					if (trace) Log(USERLOG, "VERIFY set %s to %s\r\n", var, vlabel);
					if (volleyCount > 1) --volleyCount; // make not at startup
					else volleyCount = 2;
					SetUserVariable(var, vlabel);
					strcpy(currentOutputBase, "OK"); // ack verification request
					AddResponse(currentOutputBase, 0);
					return ENDINPUT_BIT;
				}
			}
			else if (*var == '%')
			{
				ptr = ReadCompiledWord(ptr, vlabel);
				if (*vlabel == '=') // assign value to system variable (or . to clear)
				{
					ptr = ReadCompiledWord(ptr, vlabel);
					if (trace) Log(USERLOG, "VERIFY set %s to %s\r\n", var, vlabel);
					if (volleyCount > 1) --volleyCount; // make not at startup
					else volleyCount = 2;
					SystemVariable(var, vlabel);
					strcpy(currentOutputBase, "OK"); // ack verification request
					AddResponse(currentOutputBase, 0);
					return ENDINPUT_BIT;
				}
			}
			else if (*var == '^') // execute function
			{
				*labelbreak = 0;
				char* paren = strchr(before, '(');
				memmove(paren + 3, paren+1, strlen(paren) ); // 0,1,2 moved
				*paren = ' ';  // space before paren before 
				paren[1] = '(';  // (
				paren[2] = ' ';  // space after

				 paren = strchr(before, ')');
				memmove(paren + 1, paren, strlen(paren) + 1);
				*paren = ' ';
				if (volleyCount > 1) --volleyCount; // make not at startup
				else volleyCount = 2;
				Output(before, currentOutputBase, result, 0);
				strcpy(currentOutputBase, "OK"); // ack verification request
				AddResponse(currentOutputBase, 0);
				return ENDINPUT_BIT;
			}
			else if (!stricmp(var, "trace"))
			{
				ptr = ReadCompiledWord(ptr, vlabel);
				if (*vlabel == '=') // set trace valu
				{
					ptr = ReadCompiledWord(ptr, vlabel);
					if (trace) Log(USERLOG, "VERIFY set %s to %s\r\n", var, vlabel);
					volleyCount = 2; // make not at startup
					if (!strcmp(vlabel, "none")) trace = 0;
					else trace = (unsigned int)-1;
					strcpy(currentOutputBase, "OK"); // ack verification request
					AddResponse(currentOutputBase, 0);
					return ENDINPUT_BIT;
				}
			}

		}
		// add see if context assignment is happening
	}
#endif

	sentenceLimit = SENTENCES_LIMIT;
	char* sentencelim = GetUserVariable("$cs_sentences_limit", false);
	if (*sentencelim) sentenceLimit = atoi(sentencelim);
	char* topicName = GetUserVariable("$cs_control_main", false);
	if (trace & TRACE_FLOW) Log(USERLOG, "0 Ctrl:%s\r\n", topicName);
	howTopic = (tokenFlags & QUESTIONMARK) ? (char*)"question" : (char*)"statement";
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

	ProcessArguments(argc, argv);
	strcpy(us, loginID);

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
		PerformChat(us, computerID, initialInput, callerIP, ourMainOutputBuffer); // this autofrees buffer
	}
	else
	{
		struct tm ptm;
		Log(USERLOG, "System restarted %s\r\n", GetTimeInfo(&ptm, true)); // shows user requesting restart.
#ifdef EVSERVER
		// new server fork log file
		Log(ECHOSERVERLOG, "\r\n\r\n======== Restarted EV server pid %d %s compiled %s on host %s port %d at %s serverlog:%d userlog: %d\r\n", getpid(), version, compileDate, hostname, port, GetTimeInfo(&ptm, true), serverLog, userLog);
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
		char alternate[1000];
		strcpy(alternate, at);
		bool reset = false;
		if (!strnicmp(alternate, ":reset", 6))
		{
			reset = true;
			char* intercept = GetUserVariable("$cs_beforereset", false);
			if (intercept) Callback(FindWord(intercept), "()", false); // call script function first
		}
		TestMode commanded = DoCommand(at, ourMainOutputBuffer);
		if (commanded != FAILCOMMAND && server && !*ourMainOutputBuffer) strcat(ourMainOutputBuffer, at); // return what we didnt fail at

		// reset rejoinders to ignore this interruption
		outputRejoinderRuleID = inputRejoinderRuleID;
		outputRejoinderTopic = inputRejoinderTopic;
		if (!strnicmp(alternate, (char*)":retry", 6) || !strnicmp(at, (char*)":redo", 5))
		{
			outputRejoinderTopic = outputRejoinderRuleID = NO_REJOINDER; // but a redo must ignore pending
			AddInput(originalUserInput, 0, true);  // bug!
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
			BOMAccess(BOMvalue, oldc, oldCurrentLine); // copy out prior file access and reinit user file access
			ResetToPreUser();	// back to empty state before user
			FreeAllUserCaches();
			ReadNewUser();		//   now bring in user state again (in case we changed user)
			BOMAccess(BOMvalue, oldc, oldCurrentLine); // restore old BOM values
			userFirstLine = volleyCount + 1;
			*readBuffer = 0;
			if (reset)
			{
				char* intercept = GetUserVariable("$cs_afterreset", false);
				if (intercept) Callback(FindWord(intercept), "()", false); // call script function after
			}
			AddInput(" ", 0, true);
			ProcessInput();
			return 2;
		}
		else if (commanded == COMMANDED)
		{
			ResetToPreUser(); // back to empty state before any user
			// normally we are done
			// But if user input is a retry sentence, we patched the input to continue
			if (!retrycheat) return false;
		}
		else if (commanded == OUTPUTASGIVEN)
			return true;
		else if (commanded == TRACECMD || commanded == LANGUAGECMD)
		{
			WriteUserData(time(0), true); // writes out data in case of tracing variables
			ResetToPreUser(); // back to empty state before any user
			return false;
		}
		// otherwise FAILCOMMAND
	}
#endif

	if (!documentMode)
	{
		responseIndex = 0;	// clear out data (having left time for :why to work)
		FunctionResult result = OnceCode((char*)"$cs_control_pre");
		randIndex = oldRandIndex;
		if (result != NOPROBLEM_BIT)
		{
			return 0; // abort run
		}
	}
	char* input = SkipWhitespace(GetNextInput());
	if (input && *input) ++volleyCount;
	int oldVolleyCount = volleyCount;
	bool startConversation = !input;
loopback:
	inputNest = 0; // all normal user input to start with
	lastInputSubstitution[0] = 0;
	if (trace & TRACE_OUTPUT) Log(USERLOG, "\r\n\r\nInput: %d to %s: %s \r\n", volleyCount, computerID, input);

	if (input && !strncmp(input, (char*)". ", 2)) input += 2;	//   maybe separator after ? or !

	//   process input now
	char prepassTopic[MAX_WORD_SIZE];
	strcpy(prepassTopic, GetUserVariable("$cs_prepass", false));
	if (!documentMode)  AddHumanUsed(input);
	sentenceloopcount = 0;

	while ((((input = GetNextInput()) && input && *input) || startConversation) && (volleyFile || sentenceloopcount < sentenceLimit)) // loop on user input sentences
	{
		if (input && *input == '#' && input[1] == '!') 
			break; // testing label
		sentenceOverflow = !volleyFile && (sentenceloopcount + 1) >= sentenceLimit; // is this the last one we will do
		topicIndex = currentTopicID = 0; // precaution
		FunctionResult result = DoOneSentence(input, prepassTopic, sentenceloopcount >= (sentenceLimit - 1));
		if (result == RESTART_BIT)
		{
			pendingRestart = true;
			if (pendingUserReset) // erase user file?
			{
				ResetToPreUser(); // back to empty state before any user
				ReadNewUser();
				WriteUserData(time(0), false);
				ResetToPreUser(); // back to empty state before any user
			}
			return PENDING_RESTART;	// nothing more can be done here.
		}
		else if (result == FAILSENTENCE_BIT) // usually done by substituting a new input
		{
			inputRejoinderTopic = inputRetryRejoinderTopic;
			inputRejoinderRuleID = inputRetryRejoinderRuleID;
		}
		else if (result == RETRYINPUT_BIT)
		{
			// for retry INPUT
			inputRejoinderTopic = inputRetryRejoinderTopic;
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
	howTopic = (tokenFlags & QUESTIONMARK) ? (char*)"question" : (char*)"statement";
	if (prepassTopic && *prepassTopic)
	{
		int topicid = FindTopicIDByName(prepassTopic);
		if (topicid && !(GetTopicFlags(topicid) & TOPIC_BLOCKED))
		{
			if (trace & TRACE_FLOW) Log(USERLOG, "0 ***Ctrl:%s\r\n", prepassTopic);
			CALLFRAME* frame = ChangeDepth(1, prepassTopic);
			int pushed = PushTopic(topicid);
			if (pushed < 0)
			{
				ChangeDepth(-1, prepassTopic);
				return false;
			}
			AllocateOutputBuffer();
			ResetReuseSafety();
			uint64 oldflags = tokenFlags;
			FunctionResult result = PerformTopic(0, currentOutputBase);
			FreeOutputBuffer();
			ChangeDepth(-1, prepassTopic);
			if (pushed) PopTopic();
			//   subtopic ending is not a failure.
			if (result & (RESTART_BIT | ENDSENTENCE_BIT | FAILSENTENCE_BIT | ENDINPUT_BIT | FAILINPUT_BIT))
			{
				if (result & ENDINPUT_BIT) ClearSupplementalInput();
				--inputSentenceCount; // abort this input
				return true;
			}
			if (prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE || trace & (TRACE_INPUT | TRACE_POS) || prepareMode == POS_MODE || (prepareMode == PENN_MODE && trace & TRACE_POS))
			{
				if (tokenFlags != oldflags) DumpTokenFlags((char*)"After prepass"); // show revised from prepass
			}
		}
	}
	return false;
}

FunctionResult DoOneSentence(char* input, char* prepassTopic, bool atlimit)
{
	if (!input || !*input) return NOPROBLEM_BIT;

	ambiguousWords = 0;

	bool oldecho = echo;
	bool changedEcho = true;
	if (prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)  changedEcho = echo = true;

	//    generate reply by lookup first
	unsigned int retried = 0;
	sentenceRetryRejoinderTopic = inputRejoinderTopic;
	sentenceRetryRejoinderRuleID = inputRejoinderRuleID;
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
	bool oobstart = (input && *input == '[');

	bool prepare = true;
#ifndef NOMAIN
	if (strstr(originalUserInput,"VERIFY")) prepare = false;
#endif

	if (prepare)
	{
		PrepareSentence(input, true, true, false, oobstart); // user input.. sets nextinput up to continue
		if (atlimit) sentenceOverflow = true;
		if (changedEcho) echo = oldecho;
		if (PrepassSentence(prepassTopic)) return NOPROBLEM_BIT; // user input revise and resubmit?  -- COULD generate output and set rejoinders
	}
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
		for (unsigned int i = 1; i <= FACTSET_COUNT(0); ++i)
		{
			FACT* F = factSet[0][i];
			WORDP D = Meaning2Word(F->subject);
			WORDP N = Meaning2Word(F->object);
			int topicid = FindTopicIDByName(D->word);
			char* name = GetTopicName(topicid);
			Log(USERLOG, "%s (%s) : (char*)", name, N->word);
			//   look at references for this topic
			int startx = -1;
			unsigned int startPosition = 0;
			unsigned int endPosition = 0;
			while (GetIthSpot(D, ++startx, startPosition, endPosition)) // find matches in sentence
			{
				// value of match of this topic in this sentence
				for (unsigned int k = startPosition; k <= endPosition; ++k)
				{
					if (k != startPosition) Log(USERLOG, "_");
					Log(USERLOG, "%s", wordStarts[k]);
				}
				Log(USERLOG, " ");
			}
			Log(USERLOG, "\r\n");
		}
		impliedSet = ALREADY_HANDLED;
		if (changedEcho) echo = oldecho;
	}

	if (noReact) return NOPROBLEM_BIT;

	chatstarted = ElapsedMilliseconds() - chatstarted;
	preparationtime += chatstarted;
	chatstarted = ElapsedMilliseconds();

	FunctionResult result = Reply();

	chatstarted = ElapsedMilliseconds() - chatstarted;
	replytime += chatstarted;
	chatstarted = ElapsedMilliseconds();

	if (result & RETRYSENTENCE_BIT && retried < MAX_RETRIES)
	{
		inputRejoinderTopic = sentenceRetryRejoinderTopic;
		inputRejoinderRuleID = sentenceRetryRejoinderRuleID;

		++retried;	 // allow  retry -- issues with this
		--inputSentenceCount;
		sentenceRetry = true;
		input = retryInput;
		goto retry; // try input again -- maybe we changed token controls...
	}
	if (result & FAILSENTENCE_BIT)  --inputSentenceCount;
	if (result == ENDINPUT_BIT) ClearSupplementalInput(); // end future input

	return result;
}

FunctionResult OnceCode(const char* var, char* function) //   run before doing any of his input
{
	// cannot drop stack because may be executing from ^reset(USER)
	// stackFree = stackStart; // drop any possible stack used
	withinLoop = 0;
	topicIndex = currentTopicID = 0;
	char* name = (!function || !*function) ? GetUserVariable(var, false) : function;
	if (trace & (TRACE_MATCH | TRACE_PREPARE) && CheckTopicTrace())
	{
		Log(USERLOG, "\r\n0 ***Ctrl:%s \r\n", var);
	}
	FunctionResult result = NOPROBLEM_BIT;
	int olddepth = globalDepth;

	int topicid = FindTopicIDByName(name);
	if (!topicid) return NOPROBLEM_BIT;
	ResetReuseSafety();
	CALLFRAME* frame = ChangeDepth(1, name);

	int pushed = PushTopic(topicid);
	if (pushed < 0)
	{
		ChangeDepth(-1, name);
		return NOPROBLEM_BIT;
	}

	// prove it has gambits
	topicBlock* block = TI(topicid);
	if (BlockedBotAccess(topicid) || GAMBIT_MAX(block->topicMaxRule) == 0)
	{
		char word[MAX_WORD_SIZE];
		sprintf(word, "There are no gambits in topic %s for %s or topic is blocked for this bot.", GetTopicName(topicid), var);
		AddResponse(word, 0);
		ChangeDepth(-1, name);
		return NOPROBLEM_BIT;
	}
	ruleErased = false;
	howTopic = (char*)"gambit";
	
	result = PerformTopic(GAMBIT, currentOutputBase);
	ChangeDepth(-1, name);

	if (pushed) PopTopic();
	if (topicIndex) ReportBug((char*)"INFO: topics still stacked");
	if (globalDepth != olddepth)
		ReportBug((char*)"INFO: Once code %s global depth not back", name);
	topicIndex = currentTopicID = 0; // precaution
	return result;
}

void AddHumanUsed(const char* reply)
{
	if (!reply || !*reply || volleyFile) return;
	if (humanSaidIndex >= MAX_USED) humanSaidIndex = 0; // chop back //  overflow is unlikely but if he inputs >10 sentences at once, could
	unsigned int i = humanSaidIndex++;
	*humanSaid[i] = ' ';
	reply = SkipOOB((char*)reply);
	size_t len = strlen(reply);
	if (len >= SAID_LIMIT) // too long to save in user file
	{
		strncpy(humanSaid[i] + 1, reply, SAID_LIMIT);
		humanSaid[i][SAID_LIMIT] = 0;
	}
	else strcpy(humanSaid[i] + 1, reply);
}

void AddBotUsed(const char* reply, unsigned int len)
{
	if (chatbotSaidIndex >= MAX_USED || volleyFile) chatbotSaidIndex = 0; // overflow is unlikely but if he could  input >10 sentences at once
	unsigned int i = chatbotSaidIndex++;
	*chatbotSaid[i] = ' ';
	reply = SkipOOB((char*)reply);
	len = strlen(reply);
	if (len >= SAID_LIMIT) // too long to save fully in user file
	{
		strncpy(chatbotSaid[i] + 1, reply, SAID_LIMIT);
		chatbotSaid[i][SAID_LIMIT] = 0;
	}
	else strcpy(chatbotSaid[i] + 1, reply);
}

bool HasAlreadySaid(char* msg)
{
	if (!*msg) return true;
	if (!currentRule || Repeatable(currentRule) || GetTopicFlags(currentTopicID) & TOPIC_REPEAT) return false;
	msg = TrimSpaces(msg);
	size_t actual = strlen(msg);
	for (int i = 0; i < chatbotSaidIndex; ++i) // said in previous recent  volleys
	{
		size_t len = strlen(chatbotSaid[i] + 1);
		if (actual > (SAID_LIMIT - 3)) actual = len;
		if (!strnicmp(msg, chatbotSaid[i] + 1, actual + 1)) return true; // actual sentence is indented one (flag for end reads in column 0)
	}
	for (int i = 0; i < responseIndex; ++i) // already said this turn?
	{
		char* says = SkipOOB(responseData[i].response);
		size_t len = strlen(says);
		if (actual > (SAID_LIMIT - 3)) actual = len;
		if (!strnicmp(msg, says, actual + 1)) return true;
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
	multichoice = false;
	responseData[responseIndex].response = AllocateHeap(msg, 0);
	responseData[responseIndex].responseInputIndex = inputSentenceCount; // which sentence of the input was this in response to
	responseData[responseIndex].topic = currentTopicID; // what topic wrote this
	sprintf(responseData[responseIndex].id, (char*)".%u.%u", TOPLEVELID(currentRuleID), REJOINDERID(currentRuleID)); // what rule wrote this
	if (currentReuseID != -1) // if rule was referral reuse (local) , add in who did that
	{
		sprintf(responseData[responseIndex].id + strlen(responseData[responseIndex].id), (char*)".%d.%u.%u", currentReuseTopic, TOPLEVELID(currentReuseID), REJOINDERID(currentReuseID));
	}
	responseOrder[responseIndex] = (unsigned char)responseIndex;
	responseIndex++;
	if (responseIndex == MAX_RESPONSE_SENTENCES) --responseIndex;

	// now mark rule as used up if we can since it generated text
	SetErase(true); // top level rules can erase whenever they say something

	if (showWhy) Log(ECHOUSERLOG, (char*)"\r\n  => %s %s %d.%d  %s\r\n", (!UsableRule(currentTopicID, currentRuleID)) ? (char*)"-" : (char*)"", GetTopicName(currentTopicID, false), TOPLEVELID(currentRuleID), REJOINDERID(currentRuleID), ShowRule(currentRule));
}

char* DoOutputAdjustments(char* msg, unsigned int control, char*& buffer, char* limit)
{
	size_t len = strlen(msg);
	if ((buffer + len) > limit)
	{
		strncpy(buffer, msg, 1000); // 5000 is safestringlimit
		strcpy(buffer + 1000, (char*)" ... "); //   prevent trouble
		ReportBug((char*)"INFO: response too big %s...", buffer);
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
	if (!msg) return true;
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
	char* at = DoOutputAdjustments(msg, control, buffer, limit);
	if (!*at) {} // we only have oob?
	else if (all || HasAlreadySaid(at)) // dont really do this, either because it is a repeat or because we want to see all possible answers
	{
		if (all) Log(ECHOUSERLOG, (char*)"Choice %d: %s  why:%s %d.%d %s\r\n\r\n", ++choiceCount, at, GetTopicName(currentTopicID, false), TOPLEVELID(currentRuleID), REJOINDERID(currentRuleID), ShowRule(currentRule));
		else if (trace & TRACE_OUTPUT) Log(USERLOG, "Rejected: %s already said in %s\r\n", buffer, GetTopicName(currentTopicID, false));
		else if (showReject) Log(ECHOUSERLOG, (char*)"Rejected: %s already said in %s\r\n", buffer, GetTopicName(currentTopicID, false));
		ReleaseInfiniteStack();
		return false;
	}
	if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, "Message: %s\r\n", buffer);

	SaveResponse(buffer);
	ReleaseInfiniteStack();
	if (debugMessage) (*debugMessage)(buffer); // debugger inform

	return true;
}

void NLPipeline(int mytrace)
{
	char* original[MAX_SENTENCE_LENGTH];
	if (mytrace & TRACE_INPUT || prepareMode) memcpy(original + 1, wordStarts + 1, wordCount * sizeof(char*));	// replicate for test
	unsigned int originalCount = wordCount;
	if (tokenControl & (DO_SUBSTITUTE_SYSTEM | DO_PRIVATE))
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
		if (mytrace & TRACE_INPUT || prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)
		{
			int changed = 0;
			if (wordCount != originalCount) changed = true;
			for (unsigned int i = 1; i <= wordCount; ++i)
			{
				if (original[i] != wordStarts[i])
				{
					changed = i;
					break;
				}
			}
			if (changed)
			{
				Log(USERLOG, "Substituted (");
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
				Log(USERLOG, ") into: ");
				for (unsigned int i = 1; i <= wordCount; ++i) Log(USERLOG, "%s  ", wordStarts[i]);
				Log(USERLOG, "\r\n");
				memcpy(original + 1, wordStarts + 1, wordCount * sizeof(char*));	// replicate for test
			}
			originalCount = wordCount;
		}
	}

	// if 1st token is an interjection DO NOT allow this to be a question
	if (wordCount && wordStarts[1] && *wordStarts[1] == '~' && !(tokenControl & NO_INFER_QUESTION))
		tokenFlags &= -1 ^ QUESTIONMARK;

	// special lowercasing of 1st word if it COULD be AUXVERB and is followed by pronoun - avoid DO and Will and other confusions
	if (wordCount > 1 && IsUpperCase(*wordStarts[1]))
	{
		WORDP X = FindWord(wordStarts[1], 0, LOWERCASE_LOOKUP);
		if (X && X->properties & AUX_VERB)
		{
			WORDP Y = FindWord(wordStarts[2]);
			if (Y && Y->properties & PRONOUN_BITS) wordStarts[1] = X->word;
		}
	}

	if (mytrace & TRACE_INPUT || prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)
	{
		int changed = 0;
		if (wordCount != originalCount) changed = true;
		for (unsigned j = 1; j <= wordCount; ++j) if (original[j] != wordStarts[j]) changed = j;
		if (changed)
		{
			if (tokenFlags & DO_PROPERNAME_MERGE) Log(USERLOG, "Name-");
			if (tokenFlags & DO_NUMBER_MERGE) Log(USERLOG, "Number-");
			if (tokenFlags & DO_DATE_MERGE) Log(USERLOG, "Date-");
			Log(USERLOG, "merged: ");
			for (unsigned i = 1; i <= wordCount; ++i) Log(USERLOG, "%s  ", wordStarts[i]);
			Log(USERLOG, "\r\n");
			memcpy(original + 1, wordStarts + 1, wordCount * sizeof(char*));	// replicate for test
			originalCount = wordCount;
		}
	}

	// spell check unless 1st word is already a known interjection. Will become standalone sentence
	if (tokenControl & DO_SPELLCHECK && wordCount && *wordStarts[1] != '~')
	{
		if (SpellCheckSentence()) tokenFlags |= DO_SPELLCHECK;
		if (*GetUserVariable("$$cs_badspell", false)) return;
		if (spellTrace) {}
		else if (mytrace & TRACE_INPUT || prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)
		{
			int changed = 0;
			if (wordCount != originalCount) changed = 1;
			for (unsigned i = 1; i <= wordCount; ++i)
			{
				if (original[i] != wordStarts[i])
				{
					original[i] = wordStarts[i];
					changed = i;
				}
			}
			if (changed)
			{
				Log(USERLOG, "Spelling changed into: ");
				for (unsigned i = 1; i <= wordCount; ++i) Log(USERLOG, "%s  ", wordStarts[i]);
				Log(USERLOG, "\r\n");
			}
		}
		originalCount = wordCount;

		// resubstitute after spellcheck did something
		if (tokenFlags & DO_SPELLCHECK && tokenControl & (DO_SUBSTITUTE_SYSTEM | DO_PRIVATE))  ProcessSubstitutes();

		if (spellTrace) {}
		else if (mytrace & TRACE_INPUT || prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)
		{
			int changed = 0;
			if (wordCount != originalCount) changed = true;
			for (unsigned i = 1; i <= wordCount; ++i) if (original[i] != wordStarts[i]) changed = i;
			if (changed)
			{
				Log(USERLOG, "Substitution changed into: ");
				for (unsigned i = 1; i <= wordCount; ++i) Log(USERLOG, "%s  ", wordStarts[i]);
				Log(USERLOG, "\r\n");
			}
		}
	}
	if (tokenControl & DO_PROPERNAME_MERGE && wordCount)  ProperNameMerge();
	if (tokenControl & DO_DATE_MERGE && wordCount)  ProcessCompositeDate();
	if (tokenControl & DO_NUMBER_MERGE && wordCount)  ProcessCompositeNumber(); //   numbers AFTER titles, so they dont change a title
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
		if (start == end && !strcmp(wordStarts[i], derivationSentence[start])) { ; } // unchanged from original
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
	Log(USERLOG, "\r\n%s TokenControl: ", current_language);
	DumpTokenControls(tokenControl);
	Log(USERLOG, "\r\n");
	if (tokenFlags & USERINPUT) Log(USERLOG, "Original User Input: %s\r\n", input);
	else Log(USERLOG, "Original Chatbot Output: %s\r\n", input);
	Log(USERLOG, "Tokenized into: ");
	for (unsigned int i = 1; i <= wordCount; ++i) Log(FORCESTAYUSERLOG, (char*)"%s  ", wordStarts[i]);
	Log(FORCESTAYUSERLOG, (char*)"\r\n");
}

static void FixCasing()
{
	bool lowercase = false;
	for (unsigned int i = 1; i <= wordCount; ++i)
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
		for (unsigned int i = 1; i <= wordCount; ++i)
		{
			char* word = wordStarts[i];
			char myword[MAX_WORD_SIZE];
			MakeLowerCopy(myword, word);
			WORDP D = StoreWord(myword);
			wordStarts[i] = D->word;
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

static void SplitInterjection(bool analyze)
{
	unsigned int i;
			// formulate an input insertion
		char* buffer = AllocateBuffer(); // needs to be able to hold all input queued
		*buffer = 0;
		unsigned int more = (derivationIndex[1] & 0x000f) + 1; // at end
		if (more <= derivationLength && *derivationSentence[more] == ',') // swallow comma into original
		{
			++more;	// dont add comma onto input
			derivationIndex[1]++;	// extend end onto comma
		}
		for (i = more; i <= derivationLength; ++i) // rest of data after input.
		{
			if (i == 1) strncat(buffer, &derivationSeparator[0], 1);
			strcat(buffer, derivationSentence[i]);
			strncat(buffer, &derivationSeparator[i], 1);
		}

		// what the rest of the input had as punctuation OR insure it does have it
		char* end = buffer + strlen(buffer);
		char* tail = end - 1;
		while (tail > buffer && *tail == ' ') --tail;
		if (tokenFlags & QUESTIONMARK && *tail != '?') strcat(tail, (char*)"? ");
		else if (tokenFlags & EXCLAMATIONMARK && *tail != '!') strcat(tail, (char*)"! ");
		else if (*tail != '.') strcat(tail, (char*)". ");

		if (!analyze)
		{
			EraseCurrentInput();
			AddInput(buffer, 0);
		}
		FreeBuffer();
		wordCount = 1; // truncate current analysis to just the interjection
		tokenFlags |= DO_INTERJECTION_SPLITTING;
}

void PrepareSentence(char* input, bool mark, bool user, bool analyze, bool oobstart, bool atlimit)
{ // nlp processing
    unsigned int mytrace = trace;
    uint64 start_time = ElapsedMilliseconds();
    int diff = 0;
    if (prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE) mytrace = 0;
    ResetSentence();
    ResetTokenSystem();
    char separators[MAX_SENTENCE_LENGTH];
    memset(separators, 0, sizeof(char) * MAX_SENTENCE_LENGTH);
    
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
        ReportBug("INFO: Missing dq in API input field");
    }
    
    tokenFlags |= (user) ? USERINPUT : 0; // remove any question mark
    ptr = SkipWhitespace(ptr); // not needed now?
    char startc = *ptr;
    char* start = ptr;
    loading = true; // block fact language assign, so oob json visible to all languages
    char* xy = ptr;
    if (!oobstart) while ((xy = strchr(ptr, '`'))) *xy = ' '; // get rid of internal reserved use
    uint64 start_time_tokenize = ElapsedMilliseconds();
	ptr = Tokenize(ptr, wordCount, wordStarts, separators, false, oobstart);
	diff = (int)(ElapsedMilliseconds() - start_time_tokenize);
    TrackTime((char*)"Tokenize",diff);
    loading = false;
    SetContinuationInput(ptr);
    retryInput = AllocateHeap(start,ptr-start); // what we are doing now (for retry use)
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
    for (unsigned int i = 1; i <= wordCount; ++i)   // see about SHOUTing
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
    for (unsigned int i = 1; i <= wordCount; ++i)
    {
        derivationIndex[i] = (unsigned short)((i << 8) | i); // track where substitutions come from
        derivationSentence[i] = wordStarts[i];
    }
    memcpy(derivationSeparator, separators, (wordCount + 1) * sizeof(char));
    derivationLength = wordCount;
    derivationSentence[wordCount + 1] = NULL;
    derivationSeparator[wordCount + 1] = 0;
    
    if (oobPossible && wordCount && *wordStarts[1] == '[' && !wordStarts[1][1] && *wordStarts[wordCount] == ']' && !wordStarts[wordCount][1])
    {
        oobPossible = false; // no more for now
        oobExists = true;
    }
    else oobExists = false;
    
    if (tokenControl & ONLY_LOWERCASE && !oobExists) // force lower case
    {
        for (unsigned int i = FindOOBEnd(1); i <= wordCount; ++i)
        {
            if (wordStarts[i][0] != 'I' || wordStarts[i][1]) MakeLowerCase(wordStarts[i]);
        }
    }
    
    if (!oobExists) FixCasing();
    
    if (mytrace & TRACE_INPUT || prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE)
    {
        TraceTokenization(input);
    }
    
    char* bad = GetUserVariable("$$cs_badspell", false); // spelling says this user is a mess
    char* timelimit = GetUserVariable("$cs_analyzelimit");
    bool timeover = false;
    if (*timelimit && !oobExists)
    {
        uint64 used = ElapsedMilliseconds() - callStartTime;
        if (used >= atoi(timelimit))
        {
            timeover = true;  // enforce time limit for nl- too long in q
            char* logit = GetUserVariable("$cs_analyzelimitlog");
            if (*logit && *logit != '0') ReportBug("Limiting analysis");
        }
    }
    if (!oobExists && !timeover && !*bad)
    {
        uint64 start_time_nlp = ElapsedMilliseconds();
        NLPipeline(mytrace);
        diff = (int)(ElapsedMilliseconds() - start_time_nlp);
        TrackTime((char*)"NLPipeline",diff);
        bad = GetUserVariable("$$cs_badspell", false); // possibly set by NLPipeline
    }
    else // need wordcanonical to be valid (nonzero)  for any savesentence call
    {
        marklimit = 0;
        for (unsigned int i = 1; i <= wordCount; ++i)
        {
            wordCanonical[i] = wordStarts[i];
            // even oob has possible pattern match, so mark its components
            MarkWordHit(0, EXACTNOTSET, StoreWord(wordStarts[i], AS_IS), 0, i, i);
        }
    }
    if (!oobExists && tokenControl & DO_INTERJECTION_SPLITTING && wordCount > 1 && *wordStarts[1] == '~') // interjection. handle as own sentence
        SplitInterjection(analyze);
    
    // copy the raw sentence original input that this sentence used
    *rawSentenceCopy = 0;
    char* atword = rawSentenceCopy;
    unsigned int reach = 0;
    for (unsigned i = 1; i <= wordCount; ++i)
    {
        unsigned int start = derivationIndex[i] >> 8;
        unsigned int end = derivationIndex[i] & 0x00ff;
        if (start > reach)
        {
            reach = end;
            for (unsigned int j = start; j <= reach; ++j)
            {
                if (j == 1) strncat(atword, &derivationSeparator[0], 1);
                strcpy(atword, derivationSentence[j]);
                if (j < derivationLength) strncat(atword, &derivationSeparator[j], 1);
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
        Log(ECHOUSERLOG, (char*)"  => ");
        for (unsigned i = 1; i <= wordCount; ++i) Log(USERLOG, "%s  ", wordStarts[i]);
        Log(ECHOUSERLOG, (char*)"\r\n");
    }
    
    wordStarts[0] = wordStarts[wordCount + 1] = AllocateHeap((char*)""); // visible end of data in debug display
    wordStarts[wordCount + 2] = 0;
    
    if (!oobExists && mark && wordCount)
    {
        bool limitnlp = (*bad || timeover || actualTokenCount == REAL_SENTENCE_WORD_LIMIT) ? true : false;

        uint64 start_time_tag = ElapsedMilliseconds();
        TagIt(limitnlp); // pos tag and maybe parse
        diff = (int)(ElapsedMilliseconds() - start_time_tag);
        TrackTime((char*)"PosTag",diff);

        uint64 start_time_mark = ElapsedMilliseconds();
        MarkAllImpliedWords(limitnlp);
        diff = (int)(ElapsedMilliseconds() - start_time_mark);
        TrackTime((char*)"MarkWords",diff);
        SetSentenceTense(1, wordCount); // english, mostly, some foreign
    }
    if (prepareMode == PREPARE_MODE || prepareMode == TOKENIZE_MODE || trace & (TRACE_POS | TRACE_PREPARE) || prepareMode == POS_MODE || (prepareMode == PENN_MODE && trace & TRACE_POS)) DumpTokenFlags((char*)"After parse");
    diff = (int)(ElapsedMilliseconds() - start_time);
    TrackTime((char*)"PrepareSentence",diff);
    if (timing & TIME_PREPARE) {
        if (timing & TIME_ALWAYS || diff > 0)
        {
            char hold[10];
            strncpy(hold, input + 50, 4);
            strcpy(input + 50, "..."); // avoid long inputs
            Log(STDTIMELOG, (char*)"Prepare %s time: %d ms\r\n", input, diff);
            strncpy(input + 50, hold, 4);
        }
    }
}

#ifndef NOMAIN
int main(int argc, char* argv[])
{
	if (InitSystem(argc, argv)) myexit((char*)"failed to load memory");
	if (!server)
	{
		quitting = false; // allow local bots to continue regardless
		MainLoop();
	}
	else if (quitting) { ; } // boot load requests quit
#ifndef DISCARDWEBSOCKET
	else if (*websocketparams) WebSocketClient(websocketparams, websocketmessage);
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
