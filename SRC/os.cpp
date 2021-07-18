#include "common.h"

#ifdef SAFETIME // some time routines are not thread safe (not relevant with EVSERVER)
#include <mutex>
static std::mutex mtx;
#endif 
bool authorize = false;
bool pseudoServer = false;
int loglimit = 0;
bool prelog = false;
FILE* userlogFile = NULL;
char* indents[100];
int ide = 0;
int inputSize =  0;
bool inputLimitHit = false;
bool convertTabs = true;
bool serverLogTemporary = false;
bool idestop = false;
bool idekey = false;
bool inputAvailable = false;
static char encryptUser[200];
static char encryptLTM[200];
char logLastCharacter = 0;
#define MAX_STRING_SPACE 100000000  // transient+heap space 100MB
size_t maxHeapBytes = MAX_STRING_SPACE;
char* heapBase = NULL;					// start of heap space (runs backward)
char* heapFree = NULL;					// current free string ptr
char* stackFree = NULL;
static const char* infiniteCaller = "";
char* stackStart = NULL;
char* heapEnd = NULL;
uint64 discard;
bool infiniteStack = false;
bool infiniteHeap = false; 
bool userEncrypt = false;
bool ltmEncrypt = false;
unsigned long minHeapAvailable;
bool showDepth = false;

char serverLogfileName[1000];				// file to log server to
char dbTimeLogfileName[1000];				// file to log db time to
char externalBugLog[1000];
char logFilename[MAX_WORD_SIZE];			// file to user log to
bool logUpdated = false;					// has logging happened
int holdUserLog;
int holdServerLog;
int userLog = FILE_LOG;				// where do we log user
int serverLog = FILE_LOG;			// where do we log server
int bugLog = FILE_LOG;				// where do we log bugs
char hide[4000];							// dont log these json fields 
unsigned int logsize = MAX_BUFFER_SIZE;              // default
static char* logmainbuffer = NULL;					// where we build a log line
bool serverPreLog = true;					// show what server got BEFORE it works on it

unsigned int outputsize = MAX_BUFFER_SIZE;           // default
bool serverctrlz = false;					// close communication with \0 and ctrlz
bool echo = false;							// show log output onto console as well
bool oob = false;							// show oob data
bool detailpattern = false;
bool silent = false;						// dont display outputs of chat
bool logged = false;
bool showmem = false;
int filesystemOverride = NORMALFILES;
bool inLog = false;
char* testOutput = NULL;					// testing commands output reroute
static char encryptServer[1000];
static char decryptServer[1000];
int adjustIndent = 0;
char* lastheapfree = NULL;

// buffer information
#define MAX_BUFFER_COUNT 80
unsigned int maxReleaseStack = 0;
unsigned int maxReleaseStackGap = 0xffffffff;

unsigned int maxBufferLimit = MAX_BUFFER_COUNT;		// default number of system buffers for AllocateBuffer
unsigned int maxBufferSize = MAX_BUFFER_SIZE;		// default how big std system buffers from AllocateBuffer should be
unsigned int maxBufferUsed = 0;						// worst case buffer use - displayed with :variables
unsigned int bufferIndex = 0;				//   current allocated index into buffers[]  
unsigned baseBufferIndex = 0;				// preallocated buffers at start
char* buffers = 0;							//   collection of output buffers
#define MAX_OVERFLOW_BUFFERS 20
static char* overflowBuffers[MAX_OVERFLOW_BUFFERS];	// malloced extra buffers if base allotment is gone

CALLFRAME* releaseStackDepth[MAX_GLOBAL];				// ReleaseStack at start of depth

static unsigned int overflowLimit = 0;
unsigned int overflowIndex = 0;

USERFILESYSTEM userFileSystem;
static char staticPath[MAX_WORD_SIZE]; // files that never change
static char readPath[MAX_WORD_SIZE];   // readonly files that might be overwritten from outside
static char writePath[MAX_WORD_SIZE];  // files written by app
unsigned int currentFileLine = 0;				// line number in file being read
unsigned int currentLineColumn = 0;				// column number in file being read
unsigned int maxFileLine = 0;				// line number in file being read
unsigned int peekLine = 0;
char currentFilename[MAX_WORD_SIZE];	// name of file being read
std::map <WORDP, uint64> timeSummary;  // per volley time data about functions etc

// error recover 
jmp_buf scriptJump[20];
jmp_buf crashJump;
int jumpIndex = -1;

unsigned int randIndex = 0;
unsigned int oldRandIndex = 0;

char syslogstr[300] = "chatscript"; // header for syslog messages

#ifdef WIN32
#include <conio.h>
#include <direct.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <Winbase.h>
#endif

#ifdef LINUX
#include <syslog.h>
#endif

void Bug()
{
	if (compiling == FULL_COMPILE)
	{
		jumpIndex = 0; // top level of scripting abort
		BADSCRIPT("INFO: Execution error (see LOGS/bugs.txt) - try again.");
	}
}

void TrackTime(char* name, int elapsed)
{
    WORDP D = StoreWord(name);
    if (D) TrackTime(D, elapsed);
}

void TrackTime(WORDP D, int elapsed)
{
    if (!D) return;
    
    unsigned int count = 0;
    unsigned int time = 0;
    
    std::map<WORDP, uint64>::iterator it;
    it = timeSummary.find(D);
    if (it != timeSummary.end())
    {
        count = it->second >> TIMESUMMARY_COUNT_OFFSET;
        time = it->second & TIMESUMMARY_TIME;
    }
    
    count += 1;
    time += elapsed;
    uint64 summary = count;
    summary <<= TIMESUMMARY_COUNT_OFFSET;
    summary |= time;
    timeSummary[D] = summary;
}


/////////////////////////////////////////////////////////
/// KEYBOARD
/////////////////////////////////////////////////////////

bool KeyReady()
{
	if (sourceFile && sourceFile != stdin) return true;
#ifdef WIN32
    if (ide) return idekey;
	return _kbhit() ? true : false;
#else
	bool ready = false;
	struct termios oldSettings, newSettings;
    if (tcgetattr( fileno( stdin ), &oldSettings ) == -1) return false; // could not get terminal attributes
    newSettings = oldSettings;
    newSettings.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr( fileno( stdin ), TCSANOW, &newSettings );    
    fd_set set;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO( &set );
    FD_SET( fileno( stdin ), &set );
    int res = select( fileno( stdin )+1, &set, NULL, NULL, &tv );
	ready = ( res > 0 );
    tcsetattr( fileno( stdin ), TCSANOW, &oldSettings );
	return ready;
#endif
}

/////////////////////////////////////////////////////////
/// EXCEPTION/ERROR
/////////////////////////////////////////////////////////

void SafeLock()
{
#ifdef SAFETIME
	mtx.lock();
#endif
}

void SafeUnlock()
{
#ifdef SAFETIME
	mtx.unlock();
#endif
}

void JumpBack()
{
	if (jumpIndex < 0) return;	// not under handler control
	globalDepth = 0;
	longjmp(scriptJump[jumpIndex], 1);
}

void CloseDatabases(bool restart)
{
	bool usedb = (db == 1 && server) || db == 2;
#ifndef DISCARDPOSTGRES
	if (*postgresparams)
	{
		PostgresScriptShutDown(); // any script connection
		PGUserFilesCloseCode();	// filesystem
	}
#endif
#ifndef DISCARDMONGO
	if (!restart) MongoSystemShutdown();
	else MongoUserFilesClose(); // actually just closes files
#endif
#ifndef DISCARDMYSQL
	if (*mysqlparams) MySQLFullCloseCode(restart);	// filesystem and/or script
#endif
#ifndef  DISCARDMICROSOFTSQL
	if (*mssqlparams && usedb) MsSqlFullCloseCode();	// filesystem and/or script
#endif
}

void myexit(const char* msg, int code)
{	
	int oldcompiling = compiling;
	int oldloading = loading;
	compiling = NOT_COMPILING;
	loading = false;
	traceTestPatternBuffer = NULL;
#ifdef LINUX
	const char* fatal_str = strstr(msg, "FATAL:");
	if (fatal_str != NULL) {
		syslog(LOG_ERR, "%s user: %s bot: %s msg: %s ",
			syslogstr, loginID, computerID, msg);
	}
#endif 
	ReportBug("myexit called with %s  ", msg); // since this does not say FATAL at start, it will not loop back here
	char name[MAX_WORD_SIZE];
	sprintf(name, (char*)"%s/exitlog.txt", logsfolder);
	FILE* out;
	if (stdlogging) out = stderr;
	else out = FopenUTF8WriteAppend(name);
	struct tm ptm;
	if (out)
	{
		fprintf(out, (char*)"%s %d - called myexit at %s\r\n", msg, code, GetTimeInfo(&ptm, true));
		if (!stdlogging) FClose(out);
	}

	if (code != 0 && crashset && !crashBack) // try to recover
	{
#ifdef LINUX
		siglongjmp(crashJump, 1);
#else
		longjmp(crashJump, 1);
#endif
	}

	// non recoverable
	out = NULL;
	if (!oldcompiling && !oldloading)
	{
		if (stdlogging) out = stderr;
		else out = FopenUTF8WriteAppend(name);
		if (out)
		{
			struct tm ptm;
			fprintf(out, (char*)"\r\n%s: caller:%s callee:%s ", GetTimeInfo(&ptm, true), loginID, computerID);
			fprintf(out, (char*)"input:%s \r\n", currentInput );
			BugBacktrace(out);
		}
	}

	if (code == 0 && out) fprintf(out, (char*)"CS exited at %s\r\n", GetTimeInfo(&ptm, true));
	else if (out) fprintf(out, (char*)"CS terminated at %s\r\n", GetTimeInfo(&ptm, true));
	if (!stdlogging && out) FClose(out);
	if (!client) CloseSystem();
	exit((code == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

void mystart(char* msg)
{
	char name[MAX_WORD_SIZE];
    MakeDirectory(logsfolder);
	sprintf(name, (char*)"%s/startlog.txt", logsfolder);
	FILE* out;
	if (stdlogging) out = stdout;
	else out = FopenUTF8WriteAppend(name);
	char word[MAX_WORD_SIZE];
	struct tm ptm;
	sprintf(word, (char*)"System startup %s %s\r\n", msg, GetTimeInfo(&ptm, true));
	if (out && !stdlogging)
	{
		fprintf(out, (char*)"%s", word);
		FClose(out);
	}
	if (server) Log(SERVERLOG, "%s",word);
}


/////////////////////////////////////////////////////////
/// Fatal Error/signal logging
/////////////////////////////////////////////////////////
#ifdef LINUX 
#include <signal.h>
void setSignalHandlers();

void signalHandler( int signalcode ) 
{
	char word[MAX_WORD_SIZE];
	sprintf(word, (char*)"FATAL: Linux Signal code %d", signalcode);

#ifdef PRIVATE_CODE
    // Check for private hook function for additional handling
    static HOOKPTR fn = FindHookFunction((char*)"SignalHandler");
    if (fn)
    {
        char* ptr = word;
        ((SignalHandlerHOOKFN) fn)(signalcode, ptr);
    }
#endif

    myexit(word,1);
}

void setSignalHandlers () 
{
	char word[MAX_WORD_SIZE];
	struct sigaction sa;
	sa.sa_handler = &signalHandler;
	sigfillset(&sa.sa_mask); // Block every signal during the handler
		// Handle relevant signals
		if (sigaction(SIGFPE, &sa, NULL) == -1) {
			sprintf(word, (char*)"Error: cannot handle SIGFPE");
			Log(BUGLOG, word);
			Log(SERVERLOG, word);
		}
		if (sigaction(SIGSEGV, &sa, NULL) == -1) {
			sprintf(word, (char*)"Error: cannot handle SIGSEGV");
            Log(BUGLOG, word);
			Log(SERVERLOG, word);
		}
		if (sigaction(SIGBUS, &sa, NULL) == -1) {
			sprintf(word, (char*)"Error: cannot handle SIGBUS");
			Log(BUGLOG, word);
			Log(SERVERLOG, word);
		}
		if (sigaction(SIGILL, &sa, NULL) == -1) {
			sprintf(word, (char*)"Error: cannot handle SIGILL");
			Log(BUGLOG, word);
			Log(SERVERLOG, word);
		}
		if (sigaction(SIGTRAP, &sa, NULL) == -1) {
			sprintf(word, (char*)"Error: cannot handle SIGTRAP");
			Log(BUGLOG, word);
			Log(SERVERLOG, word);
		}
		if (sigaction(SIGHUP, &sa, NULL) == -1) {
			sprintf(word, (char*)"Error: cannot handle SIGHUP");
			Log(BUGLOG, word);
			Log(SERVERLOG, word);
		}
}

#endif

/////////////////////////////////////////////////////////
/// MEMORY SYSTEM
/////////////////////////////////////////////////////////

void ResetBuffers()
{
	globalDepth = 0;
	bufferIndex = baseBufferIndex;
	memset(releaseStackDepth,0,sizeof(releaseStackDepth)); 
	outputNest = oldOutputIndex = 0;
	currentRuleOutputBase = currentOutputBase = ourMainOutputBuffer;
	currentOutputLimit = outputsize;
}

void CloseBuffers()
{
	while (overflowLimit > 0) 
	{
		free(overflowBuffers[--overflowLimit]);
		overflowBuffers[overflowLimit] = 0;
	}
	free(buffers);
	buffers = 0;
}

FunctionResult AuthorizedCode(char* buffer)
{
	if ((server || pseudoServer ) && !hadAuthCode  && !VerifyAuthorization(FopenReadOnly((char*)"authorizedIP.txt"))) return FAILRULE_BIT; // authorizedIP
	return NOPROBLEM_BIT;
}

void LoggingCheats(char* incoming)
{
	// logging overrides 
	FILE* in = fopen("serverlogging.txt", (char*)"rb"); // per external file created, enable server log
	if (in)
	{
		serverLog |= FILE_LOG | PRE_LOG;
		fclose(in);
	}
	in = fopen("prelogging.txt", (char*)"rb"); // per external file created, enable prelogging
	if (in)
	{
		serverLog |= PRE_LOG;
		userLog |= PRE_LOG;
		prelog = true;
		fclose(in);
	}
	in = fopen("userlogging.txt", (char*)"rb"); // per external file created, enable user log
	if (in)
	{
		userLog |= FILE_LOG | PRE_LOG;
		fclose(in);
	}
	in = fopen("tracelogging.txt", (char*)"rb"); // per external file created, enable user log
	if (in)
	{
		trace = (unsigned int)-1;
		userLog |= FILE_LOG;
		fclose(in);
	}
	char* authcode = NULL;
	if (incoming && *serverlogauthcode) authcode = strstr(incoming, serverlogauthcode);
	if (authcode)// dont process authcode as input from user
	{
		memset(authcode, ' ', strlen(serverlogauthcode)); // hide auth code entirely
		if (!userLog) userLog = FILE_LOG | PRE_LOG;
		if (!serverLog) serverLog = FILE_LOG | PRE_LOG;
		hadAuthCode = true;
	}
	else hadAuthCode = false;
}

char* Myfgets(char* buffer,  int size, FILE* in)
{
	return fgets(buffer, size, in);
		//char* answer = fgets(buffer, size, in);
		//PurifyInput(buffer, true, false); // leave ending crlfs
		//return answer;
}

char* AllocateBuffer(char* name)
{
	if (!buffers) return (char*)""; // IDE before start

	char* buffer = buffers + (maxBufferSize * bufferIndex); 
	if (++bufferIndex >= maxBufferLimit ) // want more than nominally allowed
	{
		if (bufferIndex > (maxBufferLimit+2) || overflowIndex > 20) 
		{
			char word[MAX_WORD_SIZE];
			sprintf(word,(char*)"FATAL: Corrupt bufferIndex %u or overflowIndex %u\r\n",bufferIndex,overflowIndex);
			ReportBug(word);
		}
		--bufferIndex;

		// try to acquire more space, permanently
		if (overflowIndex >= overflowLimit) 
		{
			overflowBuffers[overflowLimit] = (char*) malloc(maxBufferSize);
			if (!overflowBuffers[overflowLimit])  ReportBug((char*)"FATAL: out of buffers\r\n");
			overflowLimit++;
			if (overflowLimit >= MAX_OVERFLOW_BUFFERS) ReportBug((char*)"FATAL: Out of overflow buffers\r\n");
			Log(USERLOG,"Allocated extra buffer %d\r\n",overflowLimit);
		}
		buffer = overflowBuffers[overflowIndex++];
	}
	else if (bufferIndex > maxBufferUsed) maxBufferUsed = bufferIndex;
	if (showmem) Log(USERLOG,"%d Buffer alloc %d %s %s\r\n",globalDepth,bufferIndex,name, releaseStackDepth[globalDepth]->name);
	*buffer++ = 0;	//   prior value
	*buffer++ = 0;	//   prior value
	*buffer = 0;	//   empty string

	return buffer;
}

void FreeBuffer(char* name)
{ // if longjump happens this may not be called. manually restore counts in setjump code
	if (showmem) Log(USERLOG,"%d Buffer free %s %d %s\r\n", globalDepth,name, bufferIndex, releaseStackDepth[globalDepth]->name);
	if (overflowIndex) --overflowIndex; // keep the dynamically allocated memory for now.
	else if (bufferIndex)  --bufferIndex; 
	else ReportBug((char*)"Buffer allocation underflow")
}

void ResetHeapFree(char* val)
{
	lastheapfree = heapFree = val;
}

void InitStackHeap()
{
	size_t size = maxHeapBytes / 64; // stack + heap size
	size = (size * 64) + 64; // 64 bit align both ends
	if (!heapEnd)
	{
		heapEnd = (char*)malloc(size);	// point to end of heap (start of stack)
		if (!heapEnd)
		{
			(*printer)((char*)"Out of  memory space for text space %d\r\n", (int)size);
			ReportBug((char*)"FATAL: Cannot allocate memory space for text %d\r\n", (int)size)
		}
	}
	ResetHeapFree( heapEnd + size); // allocate backwards
	heapBase = heapFree;
	stackStart =  stackFree = heapEnd;
	minHeapAvailable = maxHeapBytes;
	PrepIndent();
}

void FreeStackHeap()
{
	if (heapEnd) free(heapEnd);
	heapEnd = NULL;
}

char* AllocateStack(const char* word, size_t len,bool localvar,int align) // call with (0,len) to get a buffer
{
	if (!stackFree) 
		return NULL; // shouldnt happen

	if (infiniteStack) 
		ReportBug("FATAL: Allocating stack while InfiniteStack in progress from %s\r\n",infiniteCaller);
	if (len == 0)
	{
		if (!word ) return NULL;
		len = strlen(word);
	}
	if (align == 1 || align == 4) // 1 is old true value
	{
		stackFree += 3;
		uint64 x = (uint64)stackFree;
		x &= 0xfffffffffffffffc;
		stackFree = (char*)x;
	}
    else if (align == 8) 
    {
        stackFree += 7;
        uint64 x = (uint64)stackFree;
        x &= 0xfffffffffffffff8;
        stackFree = (char*)x;
    }
	unsigned int avail = heapFree - (stackFree + len + 1);
	if (avail < 5000) // dont get close
    {
		ReportBug((char*)"FATAL: Out of stack space stringSpace:%d ReleaseStackspace:%d \r\n",heapBase-heapFree,stackFree - stackStart);
		return NULL;
    }
	char* answer = stackFree;
	if (localvar) // give hidden data
	{
		*answer = '`';
		answer[1] = '`';
		answer += 2;
	}
	*answer = 0;
	if (word && *word) strncpy(answer, word, len); // dont copy if word is actually an empty string allowed with a nonzero length
	answer[len++] = 0;
	answer[len++] = 0;
	if (localvar) len += 2;
	stackFree += len;
	len = stackFree - stackStart;	// how much stack used are we?
	len = heapBase - heapFree;		// how much heap used are we?
	len = heapFree - stackFree;		// size of gap between
	if (len < maxReleaseStackGap) maxReleaseStackGap = len;
	return answer;
}

void ReleaseStack(char* word)
{
	stackFree = word;
}

bool AllocateStackSlot(char* variable)
{
	WORDP D = StoreWord(variable,AS_IS);

	unsigned int len = sizeof(char*);
    if ((stackFree + len + 1) >= (heapFree - 5000)) // dont get close
    {
		ReportBug((char*)"Out of stack space\r\n")
		return false;
    }
	char* answer = stackFree;
	memcpy(answer,&D->w.userValue,sizeof(char*));
	if (D->word[1] == LOCALVAR_PREFIX) D->w.userValue = NULL; // autoclear local var
	stackFree += sizeof(char*);
	len = heapFree - stackFree;
	if (len < maxReleaseStackGap) maxReleaseStackGap = len;
	return true;
}

char** RestoreStackSlot(char* variable,char** slot)
{
	WORDP D = FindWord(variable);
	if (!D) return slot; // should never happen, we allocate dict entry on save
    memcpy(&D->w.userValue,slot,sizeof(char*));

#ifndef DISCARDTESTING
    if (debugVar) (*debugVar)(variable, D->w.userValue);
#endif
	return ++slot;
}

char* InfiniteHeap(const char* caller)
{
	if (infiniteHeap) ReportBug("FATAL: Allocating InfiniteHeap from %s while one already in progress from %s\r\n", caller, infiniteHeap);
	infiniteCaller = caller;
	infiniteHeap = true;
	uint64 base = (uint64)(heapFree);
	base &= 0xFFFFFFFFFFFFFFF8ULL;
	heapFree = (char*)base;
	return heapFree;
}

char* InfiniteStack(char*& limit,const char* caller)
{
	if (infiniteStack) ReportBug("FATAL: Allocating InfiniteStack from %s while one already in progress from %s\r\n",caller, infiniteCaller);
	infiniteCaller = caller;
	infiniteStack = true;
	limit = heapFree - 5000; // leave safe margin of error
	uint64 base = (uint64) (stackFree+7);
	base &= 0xFFFFFFFFFFFFFFF8ULL;
	return (char*) base; // slop may be lost when allocated finally, but it can be reclaimed later
}

void ReleaseInfiniteStack()
{
	infiniteStack = false;
	infiniteCaller = "";
}

HEAPREF AllocateHeapval(HEAPREF linkval, uint64 val1, uint64 val2, uint64 val3)
{
    uint64* heapval = (uint64*)AllocateHeap(NULL, 5, sizeof(uint64), false);
    heapval[0] = (uint64)linkval;
    heapval[1] = val1;
    heapval[2] = val2;
    heapval[3] = val3;
    return (HEAPREF)heapval;
}

HEAPREF UnpackHeapval(HEAPREF linkval, uint64 & val1, uint64 & val2, uint64 & val3)
{
    uint64* data = (uint64*)linkval;
    val1 = data[1];
    val2 = data[2];
    val3 = data[3];
    return (HEAPREF)data[0];
}

STACKREF AllocateStackval(STACKREF linkval, uint64 val1, uint64 val2, uint64 val3)
{
    uint64* stackval = (uint64*)AllocateStack(NULL, 5 * sizeof(uint64), false,8);
    stackval[0] = (uint64)linkval;
    stackval[1] = val1;
    stackval[2] =  val2;
    stackval[3] =  val3;
    return (STACKREF)stackval;
}

STACKREF UnpackStackval(STACKREF linkval, uint64& val1, uint64& val2, uint64& val3)
{ // factthread requires all 5, writes on its own
    uint64* data = (uint64*) linkval;
    val1 = data[1];
    val2 = data[2];
    val3 = data[3];
    return (STACKREF)data[0];
}

void CompleteBindStack64(int n,char* base)
{
	stackFree =  base + ((n+1) * sizeof(FACT*)); // convert infinite allocation to fixed one given element count
	size_t len = heapFree - stackFree;
	if (len < maxReleaseStackGap) maxReleaseStackGap = len;
	infiniteStack = false;
}

void CompleteBindStack(int used)
{
	if (!used) 	stackFree += strlen(stackFree) + 1; // convert infinite allocation to fixed one
	else stackFree += used;
	size_t len = heapFree - stackFree;
	if (len < maxReleaseStackGap) maxReleaseStackGap = len;
	infiniteStack = false;
}

HEAPREF Index2Heap(HEAPINDEX offset)
{ 
	if (!offset) return NULL;
    char* ptr = heapBase - offset;
    if (ptr < heapFree)
	{
		ReportBug((char*)"INFO: String offset into free space\r\n")
		return NULL;
	}
	if (ptr > heapBase)  
	{
		ReportBug((char*)"INFO: String offset before heap space\r\n")
		return NULL;
	}
	return ptr;
}

bool PreallocateHeap(size_t len) // do we have the space
{
	if (infiniteHeap)
		ReportBug("FATAL: Allocating heap while InfiniteHeap in progress from %s\r\n", infiniteCaller);

	char* used = heapFree - len;
	if (used <= ((char*)stackFree + 2000)) 
	{
		ReportBug("Heap preallocation fails");
		return false;
	}
	return true;
}

bool InHeap(char* ptr)
{
	return (ptr >= heapFree && ptr <= heapBase);
}

bool InStack(char* ptr)
{
	return (ptr < heapFree && ptr >= stackStart);
}

void ShowMemory(char* label)
{
	(*printer)("%s: HeapUsed: %d Gap: %d\r\n", label, heapBase - heapFree,heapFree - stackFree);
}

char* AllocateConstHeap(char* word, size_t len, int bytes, bool clear, bool purelocal)
{
	return AllocateHeap(word, len, bytes, clear, purelocal);
}

void CompleteInfiniteHeap(char* base)
{
	infiniteHeap = false;
	infiniteCaller = "";
	if (base >= lastheapfree) ReportBug("Heap growing backwards")
	size_t len = (heapFree - base);
	lastheapfree = heapFree = (char*)base;
	int nominalLeft = maxHeapBytes - (heapBase - heapFree);
	if ((unsigned long)nominalLeft < minHeapAvailable) minHeapAvailable = nominalLeft;
	char* used = heapFree - len;
	if (used <= ((char*)stackFree + 2000) || nominalLeft < 0)
	ReportBug((char*)"FATAL: Out of permanent heap space\r\n")
}

char* AllocateHeap(const char* word,size_t len,int bytes,bool clear, bool purelocal) // BYTES means size of unit
{ //   string allocation moves BACKWARDS from end of dictionary space (as do meanings)
/* Allocations during setup as :
2 when setting up cs using current dictionary and livedata for extensions (plurals, comparatives, tenses, canonicals) 
3 preserving flags or properties when removing them or adding them while  the dictionary is unlocked - only during builds 
4 reading strings during dictionary setup
5 assigning meanings or glosses or posconditionsfor the dictionary
6 reading in postag information
---
Allocations happen during volley processing as
1 adding a new dictionary word - all the time on user input
2. saving plan backtrack data
3 altering concepts[] and topics[] lists as a result of a mark operation or normal behavior
4. temps information
5 spellcheck adjusted word in sentence list
6 tokenizing words and quoted stuff adjustments
7. assignment onto user variables
8. JSON reading
*/
	if (infiniteHeap)
		ReportBug("FATAL: Allocating stack while InfiniteHeap in progress from %s\r\n", infiniteCaller);
	len *= bytes; // this many units of this size
	if (len == 0)
	{
		if (!word ) return NULL;
		len = strlen(word);
	}
	if (word) ++len;	// null terminate string
	if (purelocal) len += 2;	// for `` prefix
	size_t allocationSize = len;

	//   always allocate in word units
	unsigned int allocate = ((allocationSize + 3) / 4) * 4;
	char* oldHeapFree = heapFree;
	heapFree -= allocate; // heap grows lower, stack grows higher til they collide
	
	uint64 base = (uint64)heapFree;
 	if (bytes > 4) // force 64bit alignment alignment
	{
		base &= 0xFFFFFFFFFFFFFFF8ULL;  // 8 byte align
	}
 	else if (bytes == 4) // force 32bit alignment alignment
	{
		base &= 0xFFFFFFFFFFFFFFFCULL;  // 4 byte align
	}
 	else if (bytes == 2) // force 16bit alignment alignment
	{
		base &= 0xFFFFFFFFFFFFFFFEULL; // 2 byte align
	}
	else if (bytes != 1) 
		ReportBug((char*)"Allocation of bytes is not std unit %d", bytes);
	
	if (heapFree >= lastheapfree)
	{
		ReportBug("Heap growing backwards")
	}
	lastheapfree =  heapFree = (char*)base;
	char* newword =  heapFree;
	int nominalLeft = maxHeapBytes - (heapBase - heapFree);
	if ((unsigned long) nominalLeft < minHeapAvailable) minHeapAvailable = nominalLeft;
	char* used = heapFree - len;
	if (used <= ((char*)stackFree + 2000) || nominalLeft < 0) 
		ReportBug((char*)"FATAL: Out of permanent heap space\r\n")
    if (word) 
	{
		if (purelocal) // never use clear true with this
		{
			*newword++ = LCLVARDATA_PREFIX;
			*newword++ = LCLVARDATA_PREFIX;
			len -= 2;
		}
		memcpy(newword,word,--len);
		newword[len] = 0;
	}
	else if (clear) memset(newword,0,len);
    return newword;
}
/////////////////////////////////////////////////////////
/// FILE SYSTEM
/////////////////////////////////////////////////////////

int FClose(FILE* file)
{
	if (file) fclose(file);
	*currentFilename = 0;
	return 0;
}

void InitUserFiles()
{ 
	// these are dynamically stored, so CS can be a DLL.
	userFileSystem.userCreate = FopenBinaryWrite;
	userFileSystem.userOpen = FopenReadWritten;
	userFileSystem.userClose = FClose;
	userFileSystem.userRead = fread;
	userFileSystem.userWrite = fwrite;
	userFileSystem.userDelete = FileDelete;
	userFileSystem.userDecrypt = NULL;
	userFileSystem.userEncrypt = NULL;
	filesystemOverride = NORMALFILES;
}

static size_t CleanupCryption(char* buffer,bool decrypt,char* filekind)
{
	// clean up answer data: {"datavalues": {"USER": xxxx }} 
	char* answer = strstr(buffer,filekind);
	if (!answer) 
	{
		*buffer = 0;
		return 0;
	}
	answer += strlen(filekind) + 2; // skip USER":
	int realsize = jsonOpenSize - (answer-buffer) - 2; // the closing two }
	memmove(buffer,answer,realsize); // drop head data 
	buffer[realsize] = 0;	// remove trailing data

	// legal json doesnt allow control characters, but we cannot change our file system ones to \r\n because
	// we cant tell which are ours and which are users. So we changed to 7f7f coding.
	if (decrypt)
	{
		char* at = buffer-1;
		while (*++at) 
		{
			if (*at == 0x7f && at[1] == 0x7f)
			{
				*at = '\r';
				at[1] = '\n';
			}
		}
	}
	return realsize;
}

void ProtectNL(char* buffer) // save ascii \r\n in json - only comes from userdata write using them
{
	char* at = buffer;
	while ((at = strchr(at,'\r'))) // legal convert
	{
		if (at[1] == '\n' ) // legal convert
		{
			*at++ = 0x7f;
			*at++ = 0x7f;
		}
	}
}

bool notcrypting = false;
static int JsonOpenCryption(char* buffer, size_t size, char* xserver, bool decrypt,char* filekind)
{
	if (size == 0) return 0;
	if (notcrypting) return size;
	if (decrypt && size > 50) return size; // it was never encrypted, this is original material
	char serverurl[1000];
	char* id = loginID;
	if (*id == 'b' && !id[1]) id = "u-8b02518d-c148-5d45-936b-491d39ced70c"; // cheat override to test outside of normal logins
	if (decrypt) sprintf(serverurl, "%s%s/datavalues/decryptedtokens", xserver, id);
	else sprintf(serverurl, "%s%s/datavalues/encryptedtokens", xserver, id);

//loginID
	// for decryption, the value we are passed is a quoted string. We need to avoid doubling those
	
#ifdef INFORMATION
	// legal json requires these characters be escaped, but we cannot change our file system ones to \r\n because
	// we cant tell which are ours and which are users. So we change to 7f7f coding for /r/n.
	// and we change to 0x31 for double quote.
	// \b  Backspace (ascii code 08) -- never seeable
	// \f  Form feed (ascii code 0C) -- never seable
	// \n  New line -- only from our user topic write
	// \r  Carriage return -- only from our user topic write
	// \t  Tab -- never seeable
	//   Double quote -- seeable in user data
	// \\  Backslash character -- ??? not expected but possible
	UserFile encoding responsible for normal JSON safe data.

	Note MongoDB code also encrypts \r\n the same way. but we dont know HERE that mongo is used
	and we dont know THERE that encryption was used. That is redundant but minor.
#endif
	if (!decrypt) ProtectNL(buffer); // should not alter size
	else // remember what we decrypt so we can overwrite it later when we save
	{
		if (!stricmp(filekind, "USER")) strcpy(encryptUser,buffer);
		else strcpy(encryptLTM, buffer);
	}
	// prepare body for transfer to server to look like this for encryption:
	// {"datavalues":{"user": {"data": {"userdata1":"abc"}}}
	// or  {"datavalues":{"ltm": {"data": {"userdata1":"abc"}}}
	// or   "{datavalues":{ "user": {"data": {"userdata1":"abc", "token" : "Sy4KoML_e"}}}
	// FOR decryption: {"datavalues":{"USER": "HkD_r-KFl"}}
	if (decrypt) sprintf(buffer + size, "}}"); // add suffix to data - no quote
	else // encrypt gives a token if reusing
	{
		char* at = buffer + size;
		char* which = NULL;
		if (!stricmp(filekind, "USER")) which = encryptUser;
		else which = encryptLTM;
		if (which && *which)
		{
			sprintf(at,"\",\"token\": %s",which);
			at += strlen(at); 
		}
		else
		{
			*at++ = '"';
		}
		sprintf(at, "}}}"); // add suffix to data
	}
	size += strlen(buffer+size);
	char url[500];
	if (decrypt) sprintf(url, "{\"datavalues\":{\"%s\": ", filekind);
	else sprintf(url, "{\"datavalues\":{\"%s\": {\"data\": \"", filekind);
	int headerlen = strlen(url);
	memmove(buffer+headerlen,buffer,size+1); // move the data over to put in the header
	strncpy(buffer,url,headerlen);

	// set up call to json open
	char header[500];
	strcpy(header,"Content-Type: application/json");
	int oldArgumentIndex = callArgumentIndex;
	int oldArgumentBase = callArgumentBase;
	callArgumentBase = callArgumentIndex - 1;
	callArgumentList[callArgumentIndex++] = (char*)"direct"; // get the text of it gives us, dont make facts out of it
	callArgumentList[callArgumentIndex++] = (char*)"POST";
	strcpy(url, serverurl);
	callArgumentList[callArgumentIndex++] =    url;
	callArgumentList[callArgumentIndex++] =    (char*) buffer;
	callArgumentList[callArgumentIndex++] =    header;
	callArgumentList[callArgumentIndex++] =    (char*)""; // timer override
	FunctionResult result = FAILRULE_BIT;
#ifndef DISCARDJSONOPEN
	result = JSONOpenCode((char*) buffer); 
#endif
	callArgumentIndex = oldArgumentIndex;	 
	callArgumentBase = oldArgumentBase;
	if (http_response != 200 || result != NOPROBLEM_BIT) 
	{
		ReportBug("Encrpytion/decryption server failed %s doing %s\r\n",buffer,decrypt ? (char*) "decrypt" : (char*) "encrypt");
		return 0;
	}
	return CleanupCryption((char*) buffer,decrypt,filekind);
}

static size_t Decrypt(void* buffer,size_t size, size_t count, FILE* file,char* filekind)
{
	return JsonOpenCryption((char*) buffer, size * count,decryptServer,true,filekind);
}

static size_t Encrypt(const void* buffer, size_t size, size_t count, FILE* file,char* filekind)
{
	return JsonOpenCryption((char*) buffer, size * count,encryptServer,false,filekind);
}

void EncryptInit(char* params) // required
{
	*encryptServer = 0;
	if (*params) strcpy(encryptServer,params);
	if (*encryptServer) userFileSystem.userEncrypt = Encrypt;
}

void ResetEncryptTags()
{
	encryptUser[0] = 0;
	encryptLTM[0] = 0;
}

void DecryptInit(char* params) // required
{
	*decryptServer = 0;
	if (*params)  strcpy(decryptServer,params);
	if (*decryptServer) userFileSystem.userDecrypt = Decrypt;
}

void EncryptRestart() // required
{
	if (*encryptServer) userFileSystem.userEncrypt = Encrypt; // reestablish encrypt/decrypt bindings
	if (*decryptServer) userFileSystem.userDecrypt = Decrypt; // reestablish encrypt/decrypt bindings
}

size_t DecryptableFileRead(void* buffer,size_t size, size_t count, FILE* file,bool decrypt,char* filekind)
{
	size_t len = userFileSystem.userRead(buffer,size,count,file);
	if (userFileSystem.userDecrypt && decrypt) return userFileSystem.userDecrypt(buffer,1,len,file,filekind); // can revise buffer
	return len;
}

size_t EncryptableFileWrite(void* buffer,size_t size, size_t count, FILE* file,bool encrypt,char* filekind)
{
	if (userFileSystem.userEncrypt && encrypt) 
	{
		size_t msgsize = userFileSystem.userEncrypt(buffer,size,count,file,filekind); // can revise buffer
        size = 1;
        count = msgsize;
	}
    size_t wrote = userFileSystem.userWrite(buffer,size,count,file);
    if (wrote != count && dieonwritefail) myexit("UserTopic Write failed");
    return wrote;
}

void CopyFile2File(const char* newname,const char* oldname, bool automaticNumber)
{
	char name[MAX_WORD_SIZE];
	FILE* out;
	if (automaticNumber) // get next number
	{
		const char* at = strchr(newname,'.');	//   get suffix
		int len = at - newname;
		strncpy(name,newname,len);
		strcpy(name,newname); //   base part
		char* endbase = name + len;
		int j = 0;
		while (++j)
		{
			sprintf(endbase,(char*)"%d.%s",j,at+1);
			out = FopenReadWritten(name);
			if (out) fclose(out);
			else break;
		}
	}
	else strcpy(name,newname);

	FILE* in = FopenReadWritten(oldname);
	if (!in) 
	{
		unlink(name); // kill any old one
		return;	
	}
	out = FopenUTF8Write(name);
	if (!out) // cannot create 
	{
		return;
	}
	fseek (in, 0, SEEK_END);
	unsigned long size = ftell(in);
	fseek (in, 0, SEEK_SET);

	char buffer[RECORD_SIZE];
	while (size >= RECORD_SIZE)
	{
		fread(buffer,1,RECORD_SIZE,in);
		fwrite(buffer,1,RECORD_SIZE,out);
		size -= RECORD_SIZE;
	}
	if (size > 0)
	{
		fread(buffer,1,size,in);
		fwrite(buffer,1,size,out);
	}

	fclose(out);
	fclose(in);
}

int MakeDirectory(const char* directory)
{
	int result;

#ifdef WIN32
	char word[MAX_WORD_SIZE];
	char* path = _getcwd(word,MAX_WORD_SIZE);
	strcat(word,(char*)"/");
	strcat(word,directory);
    if( _access( path, 0 ) == 0 ){ // does directory exist, yes
        struct stat status;
        stat( path, &status );
        if ((status.st_mode & S_IFDIR) != 0) return -1;
    }
	result = _mkdir(directory);
#else 
	result = mkdir(directory, 0777); 
#endif
	return result;
}

void C_Directories(char* x)
{
	char word[MAX_WORD_SIZE];
	size_t len = MAX_WORD_SIZE;
	int bytes;
#ifdef WIN32
	bytes = GetModuleFileName(NULL, word, len);
#else
	char szTmp[32];
	sprintf(szTmp, "/proc/%d/exe", getpid());
	bytes = readlink(szTmp, word, len);
#endif
	if (bytes >= 0) 
	{
		word[bytes] = 0;
		Log(USERLOG,"execution path: %s\r\n",word);
	}

	if (GetCurrentDir(word, MAX_WORD_SIZE)) Log(USERLOG,"current directory path: %s\r\n",word);

	Log(USERLOG,"readPath: %s\r\n",readPath);
	Log(USERLOG,"writeablePath: %s\r\n",writePath);
	Log(USERLOG,"untouchedPath: %s\r\n",staticPath);
}

void InitFileSystem(char* untouchedPath,char* readablePath,char* writeablePath)
{
	if (readablePath) strcpy(readPath,readablePath);
	else *readPath = 0;
	if (writeablePath) strcpy(writePath,writeablePath);
	else *writePath = 0;
	if (untouchedPath) strcpy(staticPath,untouchedPath);
	else *staticPath = 0;

	InitUserFiles(); // default init all the io operations to file routines
}

void StartFile(const char* name)
{
	if (strnicmp(name,"TMP",3)) maxFileLine = currentFileLine = 0;
	strcpy(currentFilename,name); // in case name is simple

	char* at = strrchr((char*) name,'/');	// last end of path
	if (at) strcpy(currentFilename,at+1);
	at = strrchr(currentFilename,'\\');		// windows last end of path
	if (at) strcpy(currentFilename,at+1);
}

FILE* FopenStaticReadOnly(const char* name) // static data file read path, never changed (DICT/LIVEDATA/src)
{
	StartFile(name);
	char path[MAX_WORD_SIZE];
	if (name[1] == ':' || *name == '/') strcpy(path, name); // windows absolute path drive c:
	else if (*readPath) sprintf(path,(char*)"%s/%s",staticPath,name);
	else strcpy(path,name);
	return fopen(path,(char*)"rb");
}

FILE* FopenReadOnly(const char* name) // read-only potentially changed data file read path (TOPIC)
{
	StartFile(name);
	char path[MAX_WORD_SIZE];
	if (name[1] == ':' || *name == '/') strcpy(path, name); // windows absolute path drive c:
	else if (*readPath) sprintf(path,(char*)"%s/%s",readPath,name);
	else strcpy(path,name);
	return fopen(path,(char*)"rb");
}

FILE* FopenReadNormal(const char* name) // normal C read unrelated to special paths
{
	StartFile(name);
	return fopen(name,(char*)"rb");
}

void FileDelete(const char* name)
{
}

int FileSize(FILE* in, char* buffer, size_t allowedSize)
{
	fseek(in, 0, SEEK_END);
	int actualSize = (int) ftell(in);
	fseek(in, 0, SEEK_SET);
	return actualSize;
}

FILE* FopenBinaryWrite(const char* name) // writeable file path
{
	char path[MAX_WORD_SIZE];
	if (name[1] == ':' || *name == '/') strcpy(path, name); // windows absolute path drive c:
	else if (*writePath) sprintf(path,(char*)"%s/%s",writePath,name);
	else strcpy(path,name);
	FILE* out = fopen(path,(char*)"wb");
	if (out == NULL && !inLog) 
		ReportBug((char*)"Error opening binary write file %s: %s\r\n",path,strerror(errno)); // NOT FATAL, upper level may create user folder if this fails
	return out;
}

FILE* FopenReadWritten(const char* name) // read from files that have been written by us
{
	StartFile(name);
	char path[MAX_WORD_SIZE];
	if (name[1] == ':' || *name == '/') strcpy(path, name); // windows absolute path drive c:
	else if (*writePath) sprintf(path,(char*)"%s/%s",writePath,name);
	else strcpy(path,name);
	return fopen(path,(char*)"rb");
}

FILE* FopenUTF8Write(const char* filename) // insure file has BOM for UTF8
{
	char path[MAX_WORD_SIZE];
	if (filename[1] == ':' || *filename == '/') strcpy(path, filename); // windows absolute path drive c:
	else if (*writePath) sprintf(path,(char*)"%s/%s",writePath,filename);
	else strcpy(path,filename);

	FILE* out = fopen(path,(char*)"wb");
	if (out) // mark file as UTF8 
	{
		unsigned char bom[3];
		bom[0] = 0xEF;
		bom[1] = 0xBB;
		bom[2] = 0xBF;
		fwrite(bom,1,3,out);
	}
	else ReportBug((char*)"Error opening utf8 write file %s: %s\r\n",path,strerror(errno)); // probably dont make it FATAL
	return out;
}

FILE* FopenUTF8WriteAppend(const char* filename,const char* flags) 
{
	char path[MAX_WORD_SIZE];
	if (filename[1] == ':' || *filename == '/') strcpy(path, filename); // windows absolute path drive c:
	else if (*writePath) sprintf(path,(char*)"%s/%s",writePath,filename);
	else strcpy(path,filename);

	FILE* in = fopen(path,(char*)"rb"); // see if file already exists
	if (in) fclose(in); // dont erase currentfilame, dont call FClose
	FILE* out = fopen(path,flags);
	if (out && !in) // mark file as UTF8 if new
	{
		unsigned char bom[3];
		bom[0] = 0xEF;
		bom[1] = 0xBB;
		bom[2] = 0xBF;
		fwrite(bom,1,3,out);
	}
	else if (!out && !inLog) ReportBug((char*)"Error opening utf8writeappend file %s: %s\r\n",path,strerror(errno)); // probably dont make it FATAL
	return out;
}

#ifndef WIN32

int isDirectory(const char *path) 
{
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) return 0;
    return S_ISDIR(statbuf.st_mode);
}

int getdir (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
 		ReportBug((char*)"No such directory %s\r\n",strerror(errno));
		return errno;
    }
    while ((dirp = readdir(dp)) != NULL) files.push_back(string(dirp->d_name));
    closedir(dp);
    return 0;
}
#endif

void WalkDirectory(char* directory,FILEWALK function, uint64 flags,bool recursive) 
{
	char name[MAX_WORD_SIZE];
	char fulldir[MAX_WORD_SIZE];
    char xname[MAX_WORD_SIZE];
    size_t len = strlen(directory);
	if (directory[len-1] == '/') directory[len-1] = 0;	// remove the / since we add it 
	if (*readPath) sprintf(fulldir,(char*)"%s/%s",staticPath,directory);
	else strcpy(fulldir,directory);
    bool seendirs = false;

#ifdef WIN32 // do all files in src directory
	WIN32_FIND_DATA FindFileData;
	DWORD dwError;
	LPSTR DirSpec;
   
	  // Prepare string for use with FindFile functions.  First, 
	  // copy the string to a buffer, then append '\*' to the 
	  // directory name.
    DirSpec = (LPSTR)malloc(MAX_PATH);
	if (!DirSpec) ReportBug("WalkDirectory malloc failed")
    strcpy(DirSpec,fulldir);
	strcat(DirSpec,(char*)"/*");
	// Find the first file in the directory.
    HANDLE hFind = FindFirstFile(DirSpec, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		ReportBug((char*)"No such directory %s\r\n",DirSpec);
        free(DirSpec); 
        return;
	} 
    if (FindFileData.cFileName[0] != '.' && stricmp(FindFileData.cFileName, (char*)"bugs.txt"))
    {
        if (FindFileData.dwFileAttributes  & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (recursive) seendirs = true;
        }
        else
        {
            sprintf(name, (char*)"%s/%s", directory, FindFileData.cFileName);
            (*function)(name, flags);
        }
    }
    while (FindNextFile(hFind, &FindFileData) != 0)
    {
        if (FindFileData.cFileName[0] == '.' || !stricmp(FindFileData.cFileName, (char*)"bugs.txt")) continue;
        if (FindFileData.dwFileAttributes  & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (recursive) seendirs = true;
        }
        else // simple file
        {
            sprintf(name, (char*)"%s/%s", directory, FindFileData.cFileName);
            (*function)(name, flags);
        }
    }
    free(DirSpec);
    dwError = GetLastError();
	FindClose(hFind);
	if (dwError != ERROR_NO_MORE_FILES) 
	{
		ReportBug((char*)"FindNextFile error. Error is %u.\n", dwError);
        return;
	}
    if (!seendirs) return;

    // now recurse in directories
    // Find the first file in the directory.
    DirSpec = (LPSTR)malloc(MAX_PATH);
	if (!DirSpec) ReportBug("WalkDirectory malloc failed2")
    strcpy(DirSpec, fulldir);
    strcat(DirSpec, (char*)"/*");
    hFind = FindFirstFile(DirSpec, &FindFileData);
    if (FindFileData.cFileName[0] != '.' && stricmp(FindFileData.cFileName, (char*)"bugs.txt"))
    {
        if (FindFileData.dwFileAttributes  & FILE_ATTRIBUTE_DIRECTORY)
        {
            sprintf(xname, "%s/%s", directory, FindFileData.cFileName);
            WalkDirectory(xname, function, flags, true);
        }
    }
    while (FindNextFile(hFind, &FindFileData) != 0)
    {
        if (FindFileData.cFileName[0] == '.' || !stricmp(FindFileData.cFileName, (char*)"bugs.txt")) continue;
        if (FindFileData.dwFileAttributes  & FILE_ATTRIBUTE_DIRECTORY)
        {
            sprintf(xname, "%s/%s", directory, FindFileData.cFileName);
            WalkDirectory(xname, function, flags, true);
        }
    }
    FindClose(hFind);
	free(DirSpec);
#else
    string dir = string(fulldir);
    vector<string> files = vector<string>();
    getdir(dir,files);
	for (unsigned int i = 0;i < files.size();i++) 
	{
		const char* file = files[i].c_str();
        size_t len = strlen(file);
		if (*file != '.') 
		{
			sprintf(name,(char*)"%s/%s",directory,file);
			(*function)(name,flags); // fails if directory
            if (recursive && isDirectory(name)) seendirs = true;
		}
     }

    if (!seendirs) return;
    // recurse
    for (unsigned int i = 0; i < files.size(); i++)
    {
        const char* file = files[i].c_str();
        if (*file == '.' || !stricmp(file, (char*)"bugs.txt")) continue;
        sprintf(xname, "%s/%s", directory, file);
        if (isDirectory(xname))
        {
            WalkDirectory(xname, function, flags, true);
        }
     }
#endif
}

string GetUserPathString(const string &login)
{
    string userPath;
    if (login.length() == 0)  return userPath; // empty string
	size_t len = login.length();
	int counter = 0;
    for (size_t i = 0; i < len; i++) // use each character
	{
		if (login.c_str()[i] == '.' || login.c_str()[i] == '_') continue; // ignore IP . stuff
        userPath.append(1, login.c_str()[i]);
        userPath.append(1, '/');
		if (++counter >= 4) break;
    }
    return userPath;
}

#ifdef USERPATHPREFIX
static int MakePath(const string &rootDir, const string &path)
{
#ifndef WIN32
    struct stat st;
    if (stat((rootDir + "/" + path).c_str(), &st) == 0)  return 1;
#endif
    string pathToCreate(rootDir);
    size_t previous = 0;
    for (size_t next = path.find('/'); next != string::npos; previous = next + 1, next = path.find('/', next + 1)) 
	{
        pathToCreate.append((char*)"/");
        pathToCreate.append(path.substr(previous, next - previous));
#ifdef WIN32
        if (_mkdir(pathToCreate.c_str()) == -1 && errno != EEXIST) return 0;
#elif EVSERVER
  		if (mkdir(pathToCreate.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1 && errno != EEXIST) return 0;
#else
  		if (mkdir(pathToCreate.c_str(), S_IRWXU | S_IRWXG) == -1 && errno != EEXIST) return 0;
#endif
    }
    return 1;
}
#endif

char* GetUserPath(char* login)
{
	static string userPath;
	char* path = "";
#ifdef USERPATHPREFIX
	if (server)
	{
		if (!filesystemOverride)  // do not use this with db storage
		{
			userPath = GetUserPathString(login);
			MakePath(usersfolder, userPath);
			path = (char*) userPath.c_str();
		}
	}
#endif
	return path;
}

/////////////////////////////////////////////////////////
/// TIME FUNCTIONS
/////////////////////////////////////////////////////////

void mylocaltime (const time_t * timer,struct tm* ptm)
{
	SafeLock();
	*ptm = *localtime(timer); // copy data across so not depending on it outside of lock
	SafeUnlock();
}

void myctime(time_t * timer,char* buffer)//	Www Mmm dd hh:mm:ss yyyy
{
	SafeLock();
	strcpy(buffer,ctime(timer));
	SafeUnlock();
}

char* GetTimeInfo(struct tm* ptm, bool nouser,bool utc) //   Www Mmm dd hh:mm:ss yyyy Where Www is the weekday, Mmm the month in letters, dd the day of the month, hh:mm:ss the time, and yyyy the year. Sat May 20 15:21:51 2000
{
    time_t curr = time(0); // local machine time
    if (regression) curr = 44444444; 
	mylocaltime (&curr,ptm);

	char* utcoffset = (nouser) ? (char*)"" : GetUserVariable((char*)"$cs_utcoffset");
	if (utc) utcoffset = (char*)"+0";
	if (*utcoffset) // report UTC relative time - so if time is 1PM and offset is -1:00, time reported to user is 12 PM.  
	{
		SafeLock();
 		*ptm = *gmtime (&curr); // this library call is not thread safe, copy its data
		SafeUnlock();

		// determine leap year status
		int year = ptm->tm_year + 1900;
		bool leap = false;
		if ((year / 400) == 0) leap = true;
		else if ((year / 100) != 0 && (year / 4) == 0) leap = true;

		// sign of offset
		int sign = 1;
		if (*utcoffset == '-') 
		{
			sign = -1;
			++utcoffset;
		}
		else if (*utcoffset == '+') ++utcoffset;

		// adjust hours, minutes, seconds
		int offset = atoi(utcoffset) * sign; // hours offset
		ptm->tm_hour += offset;
		char* colon = strchr(utcoffset,':'); // is there a minutes offset?
		if (colon)
		{
			offset = atoi(colon+1) * sign; // minutes offset same sign
			ptm->tm_min += offset;
			colon = strchr(colon+1,':');
			if (colon) // seconds offset
			{
				offset = atoi(colon+1) * sign; // seconds offset
				ptm->tm_sec += offset;
			}
		}

		// correct for over and underflows
		if (ptm->tm_sec < 0) // sec underflow caused by timezone
		{ 
			ptm->tm_sec += 60;
			ptm->tm_min -= 1;
		}
		else if (ptm->tm_sec >= 60) // sec overflow caused by timezone
		{ 
			ptm->tm_sec -= 60;
			ptm->tm_min += 1;
		}

		if (ptm->tm_min < 0) // min underflow caused by timezone
		{ 
			ptm->tm_min += 60;
			ptm->tm_hour -= 1;
		}
		else if (ptm->tm_min >= 60) // min overflow caused by timezone
		{ 
			ptm->tm_min -= 60;
			ptm->tm_hour += 1;
		}

		if (ptm->tm_hour < 0) // hour underflow caused by timezone
		{ 
			ptm->tm_hour += 24;
			ptm->tm_yday -= 1;
			ptm->tm_mday -= 1;
			ptm->tm_wday -= 1;
		}
		else if (ptm->tm_hour >= 24) // hour overflow caused by timezone
		{ 
			ptm->tm_hour -= 24;
			ptm->tm_yday += 1;
			ptm->tm_mday += 1;
			ptm->tm_wday += 1;
		}
		
		if (ptm->tm_wday < 0) ptm->tm_wday += 7; // day of week underflow  0-6  
		else if (ptm->tm_wday >= 7) ptm->tm_wday -= 7; // day of week overflow  0-6  
		
		if (ptm->tm_yday < 0) ptm->tm_yday += 365; // day of year underflow  0-365  
		else if (ptm->tm_yday >= 365 && !leap ) ptm->tm_yday -= 365; // day of year overflow  0-365  
		else if (ptm->tm_yday >= 366) ptm->tm_yday -= 366; // day of year leap overflow  0-365  

		if (ptm->tm_mday <= 0) // day of month underflow  1-31  
		{
			ptm->tm_mon -= 1;
			if (ptm->tm_mon == 1) // feb
			{
				if (leap) ptm->tm_mday = 29; 
				else ptm->tm_mday = 28; 
			}
			else if (ptm->tm_mon == 3) ptm->tm_mday = 30;  // apr
			else if (ptm->tm_mon == 5) ptm->tm_mday = 30;// june
			else if (ptm->tm_mon == 8) ptm->tm_mday = 30;// sept
			else if (ptm->tm_mon == 10) ptm->tm_mday = 30; // nov
			else ptm->tm_mday = 31;
		}
		else  // day of month overflow by 1?  1-31
		{
			if (ptm->tm_mon == 1 ) // feb
			{
				if (leap && ptm->tm_mday == 30) {ptm->tm_mon++; ptm->tm_mday = 1;}
				else if (ptm->tm_mday == 29) {ptm->tm_mon++; ptm->tm_mday = 1;}
			}
			else if (ptm->tm_mon == 3 && ptm->tm_mday == 31) {ptm->tm_mon++; ptm->tm_mday = 1;}  // apr
			else if (ptm->tm_mon == 5 && ptm->tm_mday == 31) {ptm->tm_mon++; ptm->tm_mday = 1;}// june
			else if (ptm->tm_mon == 8 && ptm->tm_mday == 31) {ptm->tm_mon++; ptm->tm_mday = 1;}// sept
			else if (ptm->tm_mon == 10 && ptm->tm_mday == 31) {ptm->tm_mon++; ptm->tm_mday = 1;} // nov
			else if (ptm->tm_mday == 32) {ptm->tm_mon++; ptm->tm_mday = 1;}
		}

		if (ptm->tm_mon < 0) // month underflow  0-11
		{ 
			ptm->tm_mon += 12; // back to december
			ptm->tm_year -= 1; // prior year
		}
		else if (ptm->tm_mon >= 12 ) // month overflow  0-11
		{ 
			ptm->tm_mon -= 12; //  january
			ptm->tm_year += 1; // on to next year
		}
	}

	SafeLock();
	char *mytime = asctime (ptm); // not thread safe
	SafeUnlock();
	mytime[strlen(mytime)-1] = 0; //   remove newline
	if (mytime[8] == ' ') mytime[8] = '0';
    return mytime;
}

char* GetMyTime(time_t curr)
{
	char mytime[100];
	myctime(&curr,mytime); //	Www Mmm dd hh:mm:ss yyyy
	static char when[40];
	strncpy(when,mytime+4,3); // mmm
	if (mytime[8] == ' ') mytime[8] = '0';
	strncpy(when+3,mytime+8,2); // dd
	when[5] = '\'';
	strncpy(when+6,mytime+22,2); // yy
	when[8] = '-';
	strncpy(when+9,mytime+11,8); // hh:mm:ss
	when[17] = 0;
	return when;
}

void GetTimeMS(uint64 time, char* when)
{
    struct tm ptm;
    time_t curr = time / 1000;
    int ms = time % 1000;
    mylocaltime (&curr,&ptm);

    SafeLock();
    char *mytime = asctime (&ptm); // not thread safe
    SafeUnlock();

    if (mytime[8] == ' ') mytime[8] = '0';
    strncpy(when,mytime+4,15); // mmm dd hh:nn:ss
    when[15] = '.';
    sprintf(when+16, "%03d", ms);
    when[19] = ' ';
    strncpy(when+20,mytime+20,4); // yyyy
    when[24] = 0;
}

#ifdef IOS
#elif __MACH__ 
void clock_get_mactime(struct timespec &ts)
{
	clock_serv_t cclock; 
	mach_timespec_t mts; 
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock); 
	clock_get_time(cclock, &mts); 
	mach_port_deallocate(mach_task_self(), cclock); 
	ts.tv_sec = mts.tv_sec; 
	ts.tv_nsec = mts.tv_nsec; 
}
#endif

uint64 ElapsedMilliseconds()
{
    uint64 count;
#ifdef WIN32
    //count = GetTickCount64();
    FILETIME t; // 100 - nanosecond intervals since midnight Jan 1, 1601.
    GetSystemTimeAsFileTime(&t);
    uint64 x = (LONGLONG)t.dwLowDateTime + ((LONGLONG)(t.dwHighDateTime) << 32LL);
    x -= 116444736000000000LL; //  unix epoch  Jan 1, 1970.
	x /= 10000; // 100-nanosecond intervals in a millisecond
	count = x;
#elif IOS
	struct timeval x_time; 
	gettimeofday(&x_time, NULL); 
	count = x_time.tv_sec * 1000; 
	count += x_time.tv_usec / 1000; 
#elif __MACH__
	struct timespec abs_time; 
	clock_get_mactime( abs_time);
	count = abs_time.tv_sec * 1000; 
	count += abs_time.tv_nsec / 1000000; 
#else // LINUX
	struct timeval x_time;
	gettimeofday(&x_time, NULL);
	count = x_time.tv_sec * 1000; 
	count += x_time.tv_usec / 1000;
#endif
	return count;
}

#ifndef WIN32
unsigned int GetFutureSeconds(unsigned int seconds)
{
	struct timespec abs_time; 
#ifdef __MACH__
	clock_get_mactime(abs_time);
#else
	clock_gettime(CLOCK_REALTIME,&abs_time); 
#endif
	return abs_time.tv_sec + seconds; 
}
#endif

////////////////////////////////////////////////////////////////////////
/// HOOKS
/// Private code should call RegisterHookFunction, typically in PrivateInit, to connect an actual function to a hook name
////////////////////////////////////////////////////////////////////////

HookInfo hookSet[] =
{
    { (char*)"PerformChatArguments",0,(char*)"Adjust the arguments to PerformChat" },
    { (char*)"SignalHandler",0,(char*)"Extend signal handler processing" },
    { (char*)"TokenizeWord",0,(char*)"Extend word tokenization" },
    { (char*)"IsValidTokenWord",0,(char*)"Checks to see if a token is recognised" },
    { (char*)"SpellCheckWord",0,(char*)"Spell check an individual word" },
#ifndef DISCARDMONGO
    { (char*)"MongoQueryParams",0,(char*)"Add query parameters to Mongo query" },
    { (char*)"MongoUpsertKeyValues",0,(char*)"Add key values to Mongo upsert" },
    { (char*)"MongoGotDocument",0,(char*)"Process document results from Mongo query" },
#endif

    { 0,0,(char*)"" }
};


HOOKPTR FindHookFunction(char* hookName)
{
    int i = -1;
    HookInfo *hook = NULL;
    
    while ((hook = &hookSet[++i]) && hook->name)
    {
        size_t len = strlen(hook->name);
        if (!strnicmp(hook->name,hookName,len))
        {
            if (hook->fn) return hook->fn;
            break;
        }
    }
    
    return NULL;
}

void RegisterHookFunction(char* hookName, HOOKPTR fn)
{
    int i = -1;
    HookInfo *hook = NULL;
    
    while ((hook = &hookSet[++i]) && hook->name)
    {
        size_t len = strlen(hook->name);
        if (!strnicmp(hook->name,hookName,len))
        {
            hook->fn = fn;
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////
/// RANDOM NUMBERS
////////////////////////////////////////////////////////////////////////

uint64 Hashit(unsigned char * data, int len,bool & hasUpperCharacters, bool & hasUTF8Characters)
{
	hasUpperCharacters = hasUTF8Characters = false;
	uint64 crc = HASHSEED;
	while (len-- > 0)
	{ 
		unsigned char c = *data++;
		if (c >= 0x41 && c <= 0x5a) // ordinary ascii upper case
		{
			c += 32;
			hasUpperCharacters = true;
		}
		else if (c & 0x80) // some kind of utf8 extended char
		{
			hasUTF8Characters = true;
			if (c == 0xc3 && *data >= 0x80 && *data <= 0x9e && *data != 0x97)
			{
				hasUpperCharacters = true;
                crc = HASHFN(crc, c);
				c = *data++; // get the cap form
				c -= 0x80;
				c += 0xa0; // get lower case form
				--len;
			}
			else if (c >= 0xc4 && c <= 0xc9 && !(c & 1))
			{
				hasUpperCharacters = true;
                crc = HASHFN(crc, c);
				c = *data++; // get the cap form
				c |= 1;  // get lower case form
				--len;
			}
			else // other utf8, do all but last char
			{
				int size = UTFCharSize((char*)data - 1);
				while (--size)
				{
                    crc = HASHFN(crc, c);
					c = *data++;
					--len;
				}
			}
		}
		if (c == ' ') c = '_';	// force common hash on space vs _
        crc = HASHFN(crc, c);
	}
	return crc;
} 

// Better random numbers based on LCG using good values from https://arxiv.org/pdf/2001.05304.pdf
#define RANDA 0xadb4a92d
#define RANDC 1
#define RANDM 0x100000000
unsigned int random(unsigned int range)
{
	if (regression || range <= 1) return 0;
    randIndex = (RANDA * randIndex + RANDC) % RANDM;
	return randIndex % range;
}

/////////////////////////////////////////////////////////
/// LOGGING
/////////////////////////////////////////////////////////
uint64 logCount = 0;

bool TraceFunctionArgs(FILE* out, char* name, int start, int end)
{
	if (!name || name[0] == '~') return false;
	WORDP D = FindWord(name);
	if (!D) return false;		// like fake ^ruleoutput

	unsigned int args = end - start;
	char arg[MAX_WORD_SIZE];
	fprintf(out, "( ");
	for (unsigned int i = 0; i < args; ++i)
	{
		strncpy(arg, callArgumentList[start + i], 50); // may be name and not value?
		arg[50] = 0;
		fprintf(out, "%s  ", arg);
	}
	fprintf(out, ")");
	return true;
}

void BugBacktrace(FILE* out)
{
	unsigned int i = globalDepth;
	char rule[MAX_WORD_SIZE];
    CALLFRAME* frame = GetCallFrame(i);
	if (frame && frame->label && *(frame->label)) 
	{
		rule[0] = 0;
		if (currentRule) {
			strncpy(rule, currentRule, 50);
			rule[50] = 0;
		}
        CALLFRAME* priorframe = GetCallFrame(i - 1);
		fprintf(out,"CurrentDepth %u: heapusedOnEntry: %u heapUsedNow: %u buffers:%u stackused: %u stackusedNow:%u   %s ",
			i,frame->heapDepth,(unsigned int)(heapBase-heapFree),frame->memindex,(unsigned int)(heapFree - (char*)releaseStackDepth[i]), (unsigned int)(stackFree-stackStart),frame->label);
		if (priorframe && !TraceFunctionArgs(out, frame->label, (i > 0) ? priorframe->argumentStartIndex: 0, frame->argumentStartIndex)) fprintf(out, " - %s", rule);
		fprintf(out, "\r\n");
	}
	while ( i && --i > 1) 
	{
        frame = GetCallFrame(i);
        if (frame && frame->rule) strncpy(rule,frame->rule,50);
        else strcpy(rule, "unknown rule");
        rule[50] = 0;
		fprintf(out,"Depth %u: heapusedOnEntry: %u buffers:%u stackused: %u   %s ",
			i,frame->heapDepth,GetCallFrame(i)->memindex,
            (unsigned int)(heapFree - (char*)releaseStackDepth[i]), frame->label);
        CALLFRAME* priorFrame = GetCallFrame(i - 1);
		if (!TraceFunctionArgs(out, frame->label, priorFrame->argumentStartIndex, priorFrame->argumentStartIndex)) fprintf(out, " - %s", rule);
		fprintf(out, "\r\n");
	}
}

CALLFRAME* ChangeDepth(int value,const char* name,bool nostackCutback, char* code)
{
    if (value == 0) // secondary call from userfunction. frame is now valid
    {
        CALLFRAME* frame = releaseStackDepth[globalDepth];
        if (showDepth) Log(USERLOG,"same depth %u %s \r\n", globalDepth, name);
        if (name) frame->label = (char*)name; // override
        if (code) frame->code = code;
#ifndef DISCARDTESTING
        if (debugCall) (*debugCall)(frame->label, true);
#endif
        return frame;
    }
	else if (value < 0) // leaving depth
	{
        bool abort = false;
        CALLFRAME* frame = releaseStackDepth[globalDepth];
		releaseStackDepth[globalDepth] = NULL;

#ifndef DISCARDTESTING
        if (globalDepth && *name && debugCall)
        {
            char* result = (*debugCall)((char*) name, false);
            if (result) abort = true;
        }
#endif
        if (frame->memindex != bufferIndex)
        {
            ReportBug((char*)"INFO: depth %d not closing bufferindex correctly at %s bufferindex now %d was %d\r\n", globalDepth, name, bufferIndex, frame->memindex);
            bufferIndex = frame->memindex; // recover release
        }
        frame->name = NULL;
        frame->rule = NULL;
        frame->label = NULL;
        callArgumentIndex = frame->argumentStartIndex;
        currentRuleID = frame->oldRuleID;
        currentTopicID = frame->oldTopic;
        currentRuleTopic = frame->oldRuleTopic;
        currentRule = frame->oldRule;

		// engine functions that are streams should not destroy potential local adjustments
		if (!nostackCutback) 
			stackFree = (char*) frame; // deallocoate ARGUMENT space
		if (showDepth) Log(USERLOG,"-depth %d %s bufferindex %d heapused:%d\r\n", globalDepth,name, bufferIndex,(int)(heapBase-heapFree));
        globalDepth += value;
        if (globalDepth < 0) { ReportBug((char*)"bad global depth in %s", name); globalDepth = 0; }
        return (abort) ?  (CALLFRAME*)1 : NULL; // abort code
    }
	else // value > 0
	{
        stackFree = (char*)(((uint64)stackFree + 7) & 0xFFFFFFFFFFFFFFF8ULL);
        CALLFRAME* frame = (CALLFRAME*) stackFree;
        stackFree += sizeof(CALLFRAME);
        memset(frame, 0,sizeof(CALLFRAME));
        frame->label = (char*)name;
        frame->code = code;
		frame->heapstart = heapFree;
        frame->rule = (currentRule) ? currentRule : (char*) "";
        frame->outputlevel = outputlevel + 1;
        frame->argumentStartIndex = callArgumentIndex;
        frame->oldRuleID = currentRuleID;
        frame->oldTopic = currentTopicID;
        frame->oldRuleTopic = currentRuleTopic;
        frame->oldRule = currentRule;
        frame->memindex = bufferIndex;
        frame->heapDepth = heapBase - heapFree;
        globalDepth += value;
        if (showDepth) Log(USERLOG,"+depth %d %s bufferindex %d heapused: %d stackused:%d gap:%d\r\n", globalDepth, name, bufferIndex, (int)(heapBase - heapFree), (int)(stackFree - stackStart), (int)(heapFree - stackFree));
		frame->depth = globalDepth;
		releaseStackDepth[globalDepth] = frame; // define argument start space - release back to here on exit
#ifndef DISCARDTESTING
		if (globalDepth && *name != '*' && debugCall) (*debugCall)((char*)name, true); 
#endif
        if (globalDepth >= (MAX_GLOBAL - 1)) ReportBug((char*)"FATAL: globaldepth too deep at %s\r\n", name);
        if (globalDepth > maxGlobalSeen) maxGlobalSeen = globalDepth;
        return frame;
    }
}

bool LogEndedCleanly()
{
	return (logLastCharacter == '\n' || !logLastCharacter); // legal
}

char* myprinter(const char* ptr, char* at, va_list ap)
{
    char* base = at;
    // start writing normal data here
    char* s;
    int i;
    while (*++ptr)
    {
        if (*ptr == '%')
        {
            ++ptr;
            if (*ptr == 'c') sprintf(at, (char*)"%c", (char)va_arg(ap, int)); // char
            else if (*ptr == 'd') sprintf(at, (char*)"%d", va_arg(ap, int)); // int %d
            else if (*ptr == 'I') //   I64
            {
#ifdef WIN32
                sprintf(at, (char*)"%I64u", va_arg(ap, uint64));
#else
                sprintf(at, (char*)"%llu", va_arg(ap, uint64));
#endif
                ptr += 2;
            }
            else if (*ptr == 'l' && ptr[1] == 'd') // ld
            {
                sprintf(at, (char*)"%ld", va_arg(ap, long int));
                ++ptr;
            }
            else if (*ptr == 'l' && ptr[1] == 'l') // lld
            {
#ifdef WIN32
                sprintf(at, (char*)"%I64u", (uint64)va_arg(ap, long long int));
#else
                sprintf(at, (char*)"%llu", (uint64)va_arg(ap, long long int));
#endif
                ptr += 2;
            }
            else if (*ptr == 'p') sprintf(at, (char*)"%p", va_arg(ap, char*)); // ptr
            else if (*ptr == 'f')
            {
                double f = (double)va_arg(ap, double);
                sprintf(at, (char*)"%f", f); // float
            }
            else if (*ptr == 's') // string
            {
                s = va_arg(ap, char*);
                if (s)
                {
                    unsigned int len = strlen(s);
                    if ((at - base + len) > (logsize - SAFE_BUFFER_MARGIN))
                    {
                        sprintf(at, (char*)" ... ");
                        break;
                    }
                    sprintf(at, (char*)"%s", s);
                }
            }
            else if (*ptr == 'x') sprintf(at, (char*)"%x", (unsigned int)va_arg(ap, unsigned int)); // hex 
            else if (IsDigit(*ptr)) // int %2d or %08x
            {
                i = va_arg(ap, int);
                unsigned int precision = atoi(ptr);
                while (*ptr && *ptr != 'd' && *ptr != 'x') ++ptr;
                if (*ptr == 'd')
                {
                    if (precision == 2) sprintf(at, (char*)"%2d", i);
                    else if (precision == 3) sprintf(at, (char*)"%3d", i);
                    else if (precision == 4) sprintf(at, (char*)"%4d", i);
                    else if (precision == 5) sprintf(at, (char*)"%5d", i);
                    else if (precision == 6) sprintf(at, (char*)"%6d", i);
                    else sprintf(at, (char*)" Bad int precision %u ", precision);
                }
                else
                {
                    if (precision == 8) sprintf(at, (char*)"%08x", i);
                    else sprintf(at, (char*)" Bad hex precision %u ", precision);
                }
            }
            else
            {
                sprintf(at, (char*)"%s", (char*)"unknown format ");
                ptr = 0;
            }
        }
        else  sprintf(at, (char*)"%c", *ptr);

        at += strlen(at);
        if (!ptr) break;
        if ((at - base) >= ((int)logsize - SAFE_BUFFER_MARGIN)) break; // prevent log overflow
    }
    *at = 0;
    return at;
}

static FILE* rotateLogOnLimit(const char *fname,const char* directory) {
    FILE* out = FopenUTF8WriteAppend(fname);
    if (!out) // see if we can create the directory (assuming its missing)
    {
        MakeDirectory(directory);
        out = userFileSystem.userCreate(fname);
        if (!out && !inLog) ReportBug((char*)"unable to create logfile %s", fname);
#ifdef WIN32
		int check_write = _access(fname, 02);
#else
		int check_write = access(fname, W_OK);
#endif
        if (!check_write) ReportBug((char*)"FATAL: unable to write to logfile %s", fname);
    }

    int64 size = 0;
    if (out && loglimit) {
        // roll log if it is too big
        int r = fseek(out, 0, SEEK_END);
#ifdef WIN32
        size = _ftelli64(out);
#else
        size = ftello(out);
#endif
        // get MB count of it roughly
        size /= 1000000;  // MB bytes
    }
    if (size > loglimit)
    {
        // roll log if it is too big
        fclose(out);
        time_t curr = time(0);
        char* when = GetMyTime(curr); // now
        char newname[MAX_WORD_SIZE];
#ifdef WIN32
		const char* old = strrchr(fname, '/');
		strcpy(newname, old + 1); // just the name, no directory path
#else
		strcpy(newname, fname);
#endif
		char* at = strrchr(newname, '.');
        sprintf(at, "-%s.txt", when);
        at = strchr(newname, ':');
        *at = '-';
        at = strchr(newname, ':');
        *at = '-';

		int result = 0;
#ifdef WIN32
		SetCurrentDirectory(directory);
		result = rename(old + 1, newname); // some renames cant handle directory spec
#else
		result = rename(fname, newname);
#endif
		if (result != 0)
		{
			char msg[1000];
			sprintf(msg,"Error %d renaming file from %s to %s  ", errno, fname, newname);
			perror(msg);
		}
#ifdef WIN32
		SetCurrentDirectory((char*)"..");
#endif
		out = FopenUTF8WriteAppend(fname);
    }
    return out;
}

void PrepIndent()
{
	char buffer[1000];
	for (int i = 0; i < 100; ++i)
	{
		char* at = buffer;
		for (int n = 0; n <= i; ++n)
		{
			if (n == 0)
			{
				if (i == 0) strcpy(at, "    ");
				else sprintf(at, "%2d  ", i);
				at += 4;
			}
			else *at++ = ' ';
		}
		*at = 0;
		indents[i] = AllocateHeap(buffer, 0, 1);
	}
}

static void IndentLog(int& priordepth,char*& at,int channel,char*& format)
{
	int depth = globalDepth;
	if (csapicall == TEST_PATTERN || csapicall == TEST_OUTPUT) depth = 0;
	if (logLastCharacter == 1 && depth == priordepth) {} // we indented already
	else if (logLastCharacter == 1 && depth > priordepth) // we need to indent a bit more
	{
		for (int i = priordepth; i < depth; i++)  *at++ = ' ';
		priordepth = depth;
	}
	else
	{
		if (!LogEndedCleanly()) // log legal
		{
			*at++ = '\r'; //   close out this prior thing
			*at++ = '\n'; // log legal
		}
		while (format[1] == '\n' || format[1] == '\r') *at++ = *++format; // we point BEFORE the format

		int n = depth + patternDepth + adjustIndent;
		if (n < 0) n = 0; //   just in case
		if (n > 99) n = 99; // limit
		if (csapicall != TEST_PATTERN && csapicall != TEST_OUTPUT && !compiling)
		{
			strcpy(at, indents[n]);
			at += 4;
		}
		else strcpy(at, indents[n]+4);
		at += n;

		priordepth = depth;
	}
}

static void HideFromLog(char*& at)
{
	// implement unlogged data
		char* hidden = hide; // multiple hiddens separated by spaces, presumed covered by quotes in real data
		char* next = hidden; // start of next field name
		char word[MAX_WORD_SIZE]; // thing we search for
		*word = '"';
		while (next)
		{
			char* begin = next;
			next = strchr(begin, ' '); // example is  "authorized" or authorized or "authorized": or whatever
			if (next) *next = 0;
			strcpy(word + 1, begin);
			if (next) *next++ = ' ';
			size_t len = strlen(word);
			word[len++] = '"';
			word[len] = 0; // key now in quotes

			char* found = strstr(logmainbuffer, word); // find instance of key (presumed only one)
			if (found) // is this item in our output to log? assume only 1 occurrence per output
			{
				// this will json field name, not requiring quotes but JSON will probably have them.
				// value will be simple string, not a structure
				char* after = found + len;	// immediately past item is what?
				while (*after == ' ') ++after; // skip past space after
				if (*after == ':') ++after; // skip a separator
				else break;	 // illegal, not a field name
				while (*after == ' ') ++after; // skip past space after
				if (*after == '"') // string value to skip
				{
					char* quote = after + 1;
					while ((quote = strchr(quote, '"'))) // find closing quote - MUST BE FOUND
					{
						if (*(quote - 1) == '\\') ++quote;	// escaped quote ignore
						else // ending quote
						{
							++quote;
							while (*quote == ' ') ++quote;	// skip trailing space
							if (*quote == ',') ++quote;	// skip comma
							at -= (quote - found);	// move the end point by the number of characters skipped
							memmove(found, quote, strlen(found));
							break;
						}
					}
				}
			}
		}
}

static void LogInput(char* input, FILE* out)
{
	size_t len = strlen(input+3);
	fwrite(logmainbuffer, 1, strlen(logmainbuffer), out);
	fwrite(originalUserInput, 1, strlen(originalUserInput), out); // separate write in case bigger than logbuffer
	char ms[250000];
	strcpy(ms, input + 3);
	char* x = strchr(ms, '`');
	if (x) *x = 0; // protect from something strange
	size_t y = strlen(ms);
	fwrite(ms, 1, strlen(ms), out);
}

static void NormalLog(const char* name, const char* folder, FILE* out, int channel,int bufLen,bool dobugLog)
{
	if (!out)
	{
		if (channel == USERLOG)
		{
			if (userlogFile) out = userlogFile;
			else
			{
				out = rotateLogOnLimit(name, folder);
				if ((trace || compiling) && !server)  userlogFile = out;
			}
		}
		else 	out = rotateLogOnLimit(name, folder);
	}
	char* input = strstr(logmainbuffer, "`*`");
	if (input)
	{
		*input = 0;
	}
	if (!out) {}
	else if (input)
	{
		LogInput(input,out);
	}
	else fwrite(logmainbuffer, 1, bufLen, out);
	
	if (!out) {}
	else if (channel != USERLOG)  // various logging- server, timing logs
	{
		struct tm ptm;
		if (!dobugLog); // we are not reporting a bug, otherwise tell where we are in main log
		else if (*currentFilename) fprintf(out, (char*)"   in %s at %u: %s\r\n    ", currentFilename, currentFileLine, readBuffer);
		else if (originalUserInput  && *originalUserInput)
		{
			fprintf(out, (char*)"%u %s in sentence:    ", volleyCount, GetTimeInfo(&ptm, true));
			fwrite(originalUserInput, 1, strlen(originalUserInput), out); // separate write in case bigger than logbuffer
			fwrite("\r\n", 1, 2, out); 
		}
	}
	if (out && out != userlogFile && out != stdout && out != stderr)  fclose(out); // dont use FClose
	if (input) *input = '`';
}

static void BugLog(char* name, char* folder, FILE* bug,char* located)
{
	bool isFile =  bug == NULL ;  // bug is either existing std channel or null and we have to go get it
	if (isFile) bug = rotateLogOnLimit(name, folder);
	if (!compiling && !loading && bug)
	{
		struct tm ptm;
		char data[10000];
		*data = 0;
		if (*currentFilename) sprintf(data, (char*)" in %s at %u: %s ", currentFilename, currentFileLine, readBuffer);
		fprintf(bug, (char*)"\r\nBUG: %s: %s volley:%u ", GetTimeInfo(&ptm, true), data, volleyCount);
		fprintf(bug, "%s", logmainbuffer);
		fprintf(bug, "caller:%s callee:%s at %s in sentence: ", loginID, loginID, located);
		fprintf(bug, "%s\r\n", originalUserInput);
		if (!strstr(logmainbuffer, "No such bot")) BugBacktrace(bug);
	}
	else if (*currentFilename && bug)
	{
		fprintf(bug, (char*)"BUG in %s at %u: %s | %s\r\n", currentFilename, currentFileLine, readBuffer, logmainbuffer);
	}
	if (isFile && bug) fclose(bug); // dont use FClose

	if (serverLog) // show in context
	{
		FILE* server = rotateLogOnLimit(serverLogfileName, logsfolder);
		if (server) fprintf(server, "BUG%s\r\n", logmainbuffer);
		fclose(server);
	}
}

void Prelog(char* user, char* usee, char* incoming)
{
	if (!userLog && !serverLog) return;
	if (serverLog & FILE_LOG) serverLog |= PRE_LOG;
	if (userLog & FILE_LOG) userLog |= PRE_LOG;

	if ((serverLog & PRE_LOG) || (userLog & PRE_LOG) || prelog)
	{
		// obscure user data for logging
		char* userInput = NULL;
		char endInput = 0;
		if (strstr(hide, "usermessage")) {
			// allow tracing of OOB but thats all
			userInput = BalanceParen(incoming, false, false);
			if (userInput) { // hide user message, allowing only oob
				endInput = *userInput;
				*userInput = 0;
			}
		}
		int id = 0;
		size_t len = strlen(incoming);
#ifdef LINUX
		id = getpid();
#endif
		logLastCharacter = 0;
		if (serverLog || userLog || prelog)
		{
            char startdate[40];
            GetTimeMS(ElapsedMilliseconds(), startdate);
            char buffer[MAX_WORD_SIZE];
			sprintf(buffer,"\r\n%s ServerPre: pid: %d %s (%s) size=%u `*`\r\n", startdate, id, user, usee, len);
			if (serverLog & PRE_LOG || prelog) Log(PASSTHRUSERVERLOG, buffer);
			if (userLog & PRE_LOG || prelog) Log(PASSTHRUUSERLOG, buffer);
		}
		if (userInput) *userInput = endInput; // restore user message if hidden
	}
}

void LogChat(uint64 starttime, char* user, char* bot, char* IP, int turn, char* input, char* output, uint64 qtime)
{
	if (!serverLog && !userLog) return;

    char startdate[40], enddate[40];
    GetTimeMS(starttime, startdate);
	size_t len = strlen(output);
	char* why = output + len + HIDDEN_OFFSET; //skip terminator + 2 ctrl z end marker
	size_t len1 = strlen(why);
	char* myactiveTopic = (*why) ? (char*)(why + len1 + 1) : (char*)"";
	uint64 endtime = ElapsedMilliseconds();
    GetTimeMS(endtime, enddate);
	if (qtime) qtime = starttime - qtime; // delay waiting in q

	char* nl = (LogEndedCleanly()) ? (char*)"" : (char*)"\r\n";
	char* tmpOutput = Purify(output);

	// hide bot output message (not oob) from log?
	char* userOutput = NULL;
	char endOutput = 0;
	if (*hide && strstr(hide, "botmessage"))
	{
		// allow tracing of OOB but thats all
		userOutput = BalanceParen(tmpOutput, false, false);
		if (userOutput)
		{
			endOutput = *userOutput;
			*userOutput = 0;
		}
	}

	// hide user input from log (except oob)
	char* hideUserInput = NULL;
	char endInput = 0;
	if (*originalUserInput)
	{
		if (strstr(hide, "usermessage"))
		{
			// allow tracing of OOB but thats all
			hideUserInput = BalanceParen(originalUserInput, false, false);
			if (hideUserInput)
			{
				endInput = *hideUserInput;
				*hideUserInput = 0;
			}
		}

		// completion logging
		size_t len = (serverLog || userLog) ? strlen(input) : 0;
		logLastCharacter = 0;
		if (serverLog || userLog)
		{
			char* buffer = AllocateBuffer();
			sprintf(buffer, "%s%s Respond: user:%s bot:%s len:%u ip:%s (%s) %d `*`  ==> %s  When:%s %dms %dwait %s JOpen:%d/%d\r\n", nl, startdate, user, bot, len, IP, myactiveTopic, turn, tmpOutput, enddate, (int)(endtime - starttime), (int)qtime, why, (int)json_open_time, (int)json_open_counter);
			if (serverLog) Log(PASSTHRUSERVERLOG, buffer);
			if (userLog) Log(PASSTHRUUSERLOG, buffer);
			FreeBuffer();
		}
		if ((unsigned int)(endtime - starttime + qtime) > timeLog)
		{
#ifdef WIN32
			const char* restarted = (restartfromdeath) ? "rebooted" : "";
			ReportBug("INFO: Excess Time nltime:%d qtime:%d %s %s", (int)(endtime - starttime), qtime, restarted, input);
#endif
		}
	}
	else Log(SERVERLOG, "%s%s Start: user:%s bot:%s ip:%s (%s) %d ==> %s  When:%s %dms %d Version:%s Build0:%s Build1:%s %s\r\n", nl, startdate, user, bot, IP, myactiveTopic, turn, tmpOutput, enddate, (int)(endtime - starttime), (int)qtime, version, timeStamp[0], timeStamp[1], why);

	if (endInput) *hideUserInput = endInput;
	if (endOutput) *userOutput = endOutput;
}

/*for logging controls:  1 = file   2 = stdout  4 = stderror    
  types are: user, server, bugreport, timebug, timing
#define SERVERLOG 0 // top-level 
#define USERLOG 1 // top-level 
			#define ECHOUSERLOG 3 // remaps to USERLOG + local echo
			#define ECHOSERVERLOG 4 // remaps to SERVERLOG + local echo
			#define STDTIMELOG 5  // remaps to USERLOG + noecho
#define DBTIMELOG 6 // top-level mongo db operation took longer than expected
#define WARNSCRIPTLOG 8 // only during script compilation
#define BADSCRIPTLOG 9  // only during script compilation
#define BUGLOG 10 // top-level for bugs
		#define FORCESTAYUSERLOG 401 // remaps to USERLOG after setting lastchar to 0 (must end mod 100 to 1)
*/
unsigned int Log(unsigned int channel, const char * fmt, ...)
{
	static unsigned int id = 1000;
	if (!compiling && serverLog == NO_LOG && userLog == NO_LOG && !stdlogging &&
		channel != BUGLOG && channel != DBTIMELOG && channel != STDDEBUGLOG &&
		csapicall == NO_API_CALL) return id;
	if (quitting) return id;

	bool passthru = false;
	bool localecho = false;
	bool noecho = false;
	logged = true;
	bool noindent = (channel == USERLOG || channel == FORCESTAYUSERLOG); 
	if (channel == PASSTHRUSERVERLOG)
	{
		channel = SERVERLOG;
		passthru = true;
	}
	else if (channel == PASSTHRUUSERLOG)
	{
		channel = USERLOG;
		passthru = true;
	}
	else if (channel == ECHOSERVERLOG)
	{
		channel = SERVERLOG;
		localecho = true;
	}
	else if (channel == ECHOUSERLOG)
	{
		localecho = true;
		channel = USERLOG;
	}
	else if (channel == STDTIMELOG)
	{
		noecho = true;
		channel = USERLOG;
	}
	else if (channel == FORCETABUSERLOG)
	{
		channel = USERLOG;
		logLastCharacter = '\n';
	}
	else if (channel == FORCESTAYUSERLOG)
	{
		channel = USERLOG;
		logLastCharacter = ' ';
	}
	else if (channel == ECHOSTAYOUSERLOG)
	{
		channel = USERLOG;
		logLastCharacter = ' ';
		localecho = true;
	}
	if (channel == SERVERLOG && server && (!serverLog && !stdlogging))  return id; // not logging server data

	// allow user logging if trace is on.
	if (!fmt)  return id; // no format or no buffer to use

	if (stdlogging) // will be outputting via stdout so no need to echo
	{
		localecho = false;
		noecho = true;
		serverLog |= STDOUT_LOG; // server log must go out to stdout in addition to anywhere else
	}

	static int priordepth = 0;

	if (!logmainbuffer)
	{
		if (logsize < maxBufferSize) logsize = maxBufferSize; 
		logmainbuffer = (char*)malloc(logsize);
		if (!logmainbuffer) exit(1);
		*logmainbuffer = 0;
	}
	char* at = logmainbuffer;
	*at = 0;

	// start writing normal data here

	//   when this channel matches the ID of the prior output of log,
	//   we dont force new line on it.
	if (channel == id) //   join result code onto intial description  This will only happen with userlogging
	{
		channel = USERLOG;
		strcpy(at, (char*)"    ");
		at += 4;
	}
	//   any channel above 1000 is a user log channel
	else if (channel > 1000) channel = USERLOG; //   force result code to indent new line

	// user logging turned off, abort logging under most conditions
	if (channel != WARNSCRIPTLOG && channel != BADSCRIPTLOG && channel != SERVERLOG && channel != BUGLOG &&  !userLog)
	{
		if (channel == USERLOG && (csapicall == COMPILE_PATTERN || csapicall == COMPILE_OUTPUT) ) return id;
		if (!debugcommand && !csapicall) return id;
		if (server && csapicall != TEST_PATTERN && csapicall != TEST_OUTPUT) return id;
	}

	++logCount;
	channel %= 100;
	
	va_list ap;
	va_start(ap, fmt);
	if (!passthru)
	{
		char* ptr = (char*)(fmt - 1);
		if (channel == USERLOG && (!noindent || logLastCharacter == '\n')) //   indented by call level and not merged
		{
			IndentLog(priordepth, at, channel, ptr);
		}
		at = myprinter(ptr, at, ap);
	}
	else
	{
		strcpy(logmainbuffer, fmt);
		at = logmainbuffer + strlen(logmainbuffer);
	}
	va_end(ap);
	if (traceTestPatternBuffer) // trace output going to api calls (eg ^testpattern)
	{
		UpdateTrace(logmainbuffer);
		*logmainbuffer = 0;
		inLog = false;
		return ++id;
	}

	if (*hide) HideFromLog(at); // implement unlogged data

	if (channel == STDDEBUGLOG) // debugger output
	{
		if (debugOutput) (*debugOutput)(logmainbuffer); // send to CS debugger
		else printf("%s", logmainbuffer); // directly show to user
		inLog = false;
		return ++id;
	}

	logLastCharacter = (at > logmainbuffer) ? *(at - 1) : 0; //   ends on NL?
	if (fmt && !*fmt) logLastCharacter = 1; // special marker 
	if (logLastCharacter == '\\') *--at = 0;	//   dont show it (we intend to merge lines)
	logUpdated = true; // in case someone wants to see if intervening output happened
	size_t bufLen = at - logmainbuffer;
	inLog = true;
	bool dobugLog = false;

	if (channel == WARNSCRIPTLOG)
	{
		AddWarning(logmainbuffer);
		bufLen = strlen(logmainbuffer); // eol might have been added by addwarning
		channel = USERLOG;
	}
	else if (channel == BADSCRIPTLOG)
	{
		AddError(logmainbuffer);
		channel = USERLOG;
	}
	if (csapicall && (channel == SERVERLOG || channel == USERLOG))
	{
		if (!trace) return id; // API calls need not continue as they dont get logged in files
		if (server) return id; // server does not trace
#ifdef DLL
		if (!server) return id;	 // dll never displays trace
#endif
	}
	if (!userLog && (channel == USERLOG || channel > 1000 || channel == id) && !testOutput && !trace) return id;
	
	// trace on for no user log will go to server log

	if (bugLog && channel == BUGLOG && csapicall != COMPILE_PATTERN && csapicall != COMPILE_OUTPUT )
	{
		dobugLog = true; // we are outputting to some bug log 
		char located[MAX_WORD_SIZE];
		*located = 0;
		if (currentTopicID && currentRule) sprintf(located, (char*)" script: %s.%u.%u", GetTopicName(currentTopicID), TOPLEVELID(currentRuleID), REJOINDERID(currentRuleID));
		
		char name[MAX_WORD_SIZE];
		sprintf(name, (char*)"%s/bugs.txt", logsfolder);
		if (stdlogging) bugLog |= 4;
		if (bugLog & FILE_LOG)
		{
			BugLog(name, logsfolder, NULL, located);
			if (*externalBugLog) // if want global backup outside of cs folder
			{
				char directory[100];
				strcpy(directory, externalBugLog);
				char* dir = strrchr(directory, '/');
				if (dir) *dir = 0;
				else dir = "";
				BugLog(externalBugLog, directory, NULL, located); // always log bugs into bug folder
			}
		}
		if (bugLog & STDOUT_LOG) BugLog(name, logsfolder, stdout, located);
		if (bugLog & STDERR_LOG) BugLog(name, logsfolder, stderr, located);

		if ((echo || localecho) && !silent && !server)
		{
			struct tm ptm;
			if (*currentFilename) fprintf(stdout, (char*)"\r\n   in %s at %u: %s\r\n    ", currentFilename, currentFileLine, readBuffer);
			else if (!compiling && !commandLineCompile && currentInput && *currentInput) fprintf(stdout, (char*)"\r\n%u %s in sentence: %s \r\n    ", volleyCount, GetTimeInfo(&ptm, true), currentInput);
		}

		strcpy(at, (char*)"\r\n");	//   end it
		at += 2;
		bufLen += 2;

		if (strstr(logmainbuffer, "FATAL:")) // at start of message, this should force server to reload or die
		{
				myexit(logmainbuffer); // never returns normally, exit or longjump
		}
	}
	if (server && trace && !userLog) channel = SERVERLOG;	// force traced server to go to server log since no user log
	
	if (channel == USERLOG)
	{	
		if (!server && !noecho && (echo || localecho || trace & TRACE_ECHO) && !(userLog & STDOUT_LOG))
		{
			if (!debugOutput) fwrite(logmainbuffer, 1, bufLen, stdout);
			else (*debugOutput)(logmainbuffer);
		}

		char userfname[MAX_WORD_SIZE];
		*userfname = 0;
		if (*logFilename && userLog & FILE_LOG)
		{
			strcpy(userfname, logFilename);
			char defaultlogFilename[1000];
			sprintf(defaultlogFilename, "%s/log%u.txt", logsfolder, port); // DEFAULT LOG
			if (!strcmp(userfname, defaultlogFilename)) // of log is default log i.t log<port>.txt
			{
				struct tm ptm;
				sprintf(defaultlogFilename, "%s - ", GetTimeInfo(&ptm));
				memmove(logmainbuffer + strlen(defaultlogFilename), logmainbuffer, at - logmainbuffer + 1);
			}
		}
		if (userLog & FILE_LOG)	NormalLog(userfname, usersfolder, NULL, channel, at - logmainbuffer, dobugLog);
		if (userLog & STDOUT_LOG) NormalLog("", "", stdout, channel, at - logmainbuffer, dobugLog);
		if (userLog & STDERR_LOG)	NormalLog("", "", stderr, channel, at - logmainbuffer, dobugLog);
	}
	else if (channel == DBTIMELOG) // mongo operation took longer than limit
	{
		if (bugLog & FILE_LOG)	NormalLog(dbTimeLogfileName, logsfolder, NULL, channel, at - logmainbuffer, dobugLog);
		if (bugLog & STDOUT_LOG)	NormalLog("", "", stdout, channel, at - logmainbuffer, dobugLog);
		if (bugLog & STDERR_LOG)	NormalLog("", "", stderr, channel, at - logmainbuffer, dobugLog);
	}
	else if (channel == BUGLOG)
	{
		if (bugLog & FILE_LOG)	NormalLog("bugs.txt", logsfolder, NULL, channel, at - logmainbuffer, dobugLog);
		if (bugLog & STDOUT_LOG)	NormalLog("", "", stdout, channel, at - logmainbuffer, dobugLog);
		if (bugLog & STDERR_LOG)	NormalLog("", "", stderr, channel, at - logmainbuffer, dobugLog);
	}
	else // SERVERLOG
	{
		if (serverLog & FILE_LOG)	NormalLog(serverLogfileName, logsfolder, NULL, channel, at - logmainbuffer, dobugLog);
		if (serverLog & STDOUT_LOG)	NormalLog("", "", stdout, channel, at - logmainbuffer, dobugLog);
		if (serverLog & STDERR_LOG)	NormalLog("", "", stderr, channel, at - logmainbuffer, dobugLog);
	}

#ifndef DISCARDSERVER
	if (testOutput && server) // command outputs
	{
		size_t len = strlen(testOutput);
		if ((len + bufLen) < (maxBufferSize - SAFE_BUFFER_MARGIN)) strcat(testOutput, logmainbuffer);
	}

#endif

	inLog = false;
	return ++id;
}



