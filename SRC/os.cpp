#include "common.h"

#ifdef SAFETIME // some time routines are not thread safe (not relevant with EVSERVER)
#include <mutex>
static std::mutex mtx;
#endif 
int loglimit = 0;
int ide = 0;
bool idestop = false;
bool idekey = false;
bool inputAvailable = false;
static char encryptUser[200];
static char encryptLTM[200];
static char logLastCharacter = 0;
#define MAX_STRING_SPACE 100000000  // transient+heap space 100MB
size_t maxHeapBytes = MAX_STRING_SPACE;
char* heapBase;					// start of heap space (runs backward)
char* heapFree;					// current free string ptr
char* stackFree;
char* infiniteCaller = "";
char* stackStart;
char* heapEnd;
static bool infiniteStack = false;
bool userEncrypt = false;
bool ltmEncrypt = false;
unsigned long minHeapAvailable;
bool showDepth = false;
char serverLogfileName[200];				// file to log server to
char dbTimeLogfileName[200];				// file to log db time to
char logFilename[MAX_WORD_SIZE];			// file to user log to
bool logUpdated = false;					// has logging happened
int logsize = MAX_BUFFER_SIZE;
int outputsize = MAX_BUFFER_SIZE;
char* logmainbuffer = NULL;					// where we build a log line
static bool pendingWarning = false;			// log entry we are building is a warning message
static bool pendingError = false;			// log entry we are building is an error message
int userLog = LOGGING_NOT_SET;				// do we log user
int serverLog = LOGGING_NOT_SET;			// do we log server
char hide[4000];							// dont log this json field 
bool serverPreLog = true;					// show what server got BEFORE it works on it
bool serverctrlz = false;					// close communication with \0 and ctrlz
bool echo = false;							// show log output onto console as well
bool oob = false;							// show oob data
bool silent = false;						// dont display outputs of chat
bool logged = false;
bool showmem = false;
int filesystemOverride = NORMALFILES;
bool inLog = false;
char* testOutput = NULL;					// testing commands output reroute
static char encryptServer[1000];
static char decryptServer[1000];

// buffer information
#define MAX_BUFFER_COUNT 80
unsigned int maxReleaseStack = 0;
unsigned int maxReleaseStackGap = 0xffffffff;

unsigned int maxBufferLimit = MAX_BUFFER_COUNT;		// default number of system buffers for AllocateBuffer
unsigned int maxBufferSize = MAX_BUFFER_SIZE;		// how big std system buffers from AllocateBuffer should be
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
unsigned int maxFileLine = 0;				// line number in file being read
unsigned int peekLine = 0;
char currentFilename[MAX_WORD_SIZE];	// name of file being read

// error recover 
jmp_buf scriptJump[5];
int jumpIndex = -1;

unsigned int randIndex = 0;
unsigned int oldRandIndex = 0;

#ifdef WIN32
#include <conio.h>
#include <direct.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <Winbase.h>
#endif

void Bug()
{
	int xx = 0; // a hook to debug bug reports
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

void myexit(char* msg, int code)
{	

#ifndef DISCARDTESTING
	// CheckAbort(msg);
#endif

#ifndef DISCARDPOSTGRES
	if (*postgresparams)  
	{
		PostgresShutDown(); // any script connection
		PGUserFilesCloseCode();	// filesystem
	}
#endif
	char name[MAX_WORD_SIZE];
	sprintf(name,(char*)"%s/exitlog.txt",logs);
	FILE* in = FopenUTF8WriteAppend(name);
	if (in) 
	{
		struct tm ptm;
		fprintf(in,(char*)"%s %d - called myexit at %s\r\n",msg,code,GetTimeInfo(&ptm,true));
		FClose(in);
	}
	if (code == 0) exit(0);
	else exit(EXIT_FAILURE);
}

void mystart(char* msg)
{
	char name[MAX_WORD_SIZE];
    MakeDirectory(logs);
	sprintf(name, (char*)"%s/startlog.txt", logs);
	FILE* in = FopenUTF8WriteAppend(name);
	char word[MAX_WORD_SIZE];
	struct tm ptm;
	sprintf(word, (char*)"System startup %s %s\r\n", msg, GetTimeInfo(&ptm, true));
	if (in)
	{
		fprintf(in, (char*)"%s", word);
		FClose(in);
	}
	if (server) Log(SERVERLOG, word);
}

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

char* AllocateBuffer(char* name)
{// CANNOT USE LOG INSIDE HERE, AS LOG ALLOCATES A BUFFER
	char* buffer = buffers + (maxBufferSize * bufferIndex); 
	if (++bufferIndex >= maxBufferLimit ) // want more than nominally allowed
	{
		if (bufferIndex > (maxBufferLimit+2) || overflowIndex > 20) 
		{
			char word[MAX_WORD_SIZE];
			sprintf(word,(char*)"Corrupt bufferIndex %d or overflowIndex %d\r\n",bufferIndex,overflowIndex);
			Log(STDTRACELOG,(char*)"%s\r\n",word);
			ReportBug(word);
			myexit(word);
		}
		--bufferIndex;

		// try to acquire more space, permanently
		if (overflowIndex >= overflowLimit) 
		{
			overflowBuffers[overflowLimit] = (char*) malloc(maxBufferSize);
			if (!overflowBuffers[overflowLimit]) 
			{
				ReportBug((char*)"FATAL: out of buffers\r\n");
			}
			overflowLimit++;
			if (overflowLimit >= MAX_OVERFLOW_BUFFERS) ReportBug((char*)"FATAL: Out of overflow buffers\r\n");
			Log(STDTRACELOG,(char*)"Allocated extra buffer %d\r\n",overflowLimit);
		}
		buffer = overflowBuffers[overflowIndex++];
	}
	else if (bufferIndex > maxBufferUsed) maxBufferUsed = bufferIndex;
	if (showmem) Log(STDTRACELOG,(char*)"Buffer alloc %d %s\r\n",bufferIndex,name);
	*buffer++ = 0;	//   prior value
	*buffer = 0;	//   empty string
	return buffer;
}

void FreeBuffer(char* name)
{
	if (showmem) Log(STDTRACELOG,(char*)"Buffer free %d %s\r\n",bufferIndex,name);
	if (overflowIndex) --overflowIndex; // keep the dynamically allocated memory for now.
	else if (bufferIndex)  --bufferIndex; 
	else ReportBug((char*)"Buffer allocation underflow")
}

void InitStackHeap()
{
	size_t size = maxHeapBytes / 64;
	size = (size * 64) + 64; // 64 bit align both ends
	heapEnd = ((char*) malloc(size));	// point to end
	if (!heapEnd)
	{
		(*printer)((char*)"Out of  memory space for text space %d\r\n",(int)size);
		ReportBug((char*)"FATAL: Cannot allocate memory space for text %d\r\n",(int)size)
	}
	heapFree = heapBase = heapEnd + size; // allocate backwards
	stackFree = heapEnd;
	minHeapAvailable = maxHeapBytes;
	stackStart = stackFree;
	ClearNumbers();
}

void FreeStackHeap()
{
	if (heapEnd)
	{
		free(heapEnd);
		heapEnd = NULL;
	}
}

char* AllocateStack(char* word, size_t len,bool localvar,int align) // call with (0,len) to get a buffer
{
	if (infiniteStack) 
		ReportBug("Allocating stack while InfiniteStack in progress from %s\r\n",infiniteCaller);
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

	if ((stackFree + len + 1) >= heapFree - 30000000) // dont get close
	{
		int xx = 0;
	}
	if ((stackFree + len + 1) >= heapFree - 5000) // dont get close
    {
		ReportBug((char*)"Out of stack space stringSpace:%d ReleaseStackspace:%d \r\n",heapBase-heapFree,stackFree - stackStart);
		return NULL;
    }
	char* answer = stackFree;
	if (localvar) // give hidden data
	{
		*answer = '`';
		answer[1] = '`';
		answer += 2;
	}
	if (word) strncpy(answer,word,len);
	else *answer = 0;
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
    if (!stricmp(variable, "$_specialty"))
    {
        int xx = 0;
    }

#ifndef DISCARDTESTING
    if (debugVar) (*debugVar)(variable, D->w.userValue);
#endif
	return ++slot;
}

char* InfiniteStack(char*& limit,char* caller)
{
	if (infiniteStack) ReportBug("Allocating InfiniteStack from %s while one already in progress from %s\r\n",caller, infiniteCaller);
	infiniteCaller = caller;
	infiniteStack = true;
	limit = heapFree - 5000; // leave safe margin of error
	*stackFree = 0;
	return stackFree;
}

char* InfiniteStack64(char*& limit,char* caller)
{
	if (infiniteStack) ReportBug("Allocating InfiniteStack from %s while one already in progress from %s\r\n",caller, infiniteCaller);
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

void CompleteBindStack64(int n,char* base)
{
	stackFree =  base + ((n+1) * sizeof(FACT*)); // convert infinite allocation to fixed one given element count
	size_t len = heapFree - stackFree;
	if (len < maxReleaseStackGap) maxReleaseStackGap = len;
	infiniteStack = false;
}

void CompleteBindStack()
{
	stackFree += strlen(stackFree) + 1; // convert infinite allocation to fixed one
	size_t len = heapFree - stackFree;
	if (len < maxReleaseStackGap) maxReleaseStackGap = len;
	infiniteStack = false;
}

char* Index2Heap(unsigned int offset) 
{ 
	if (!offset) return NULL;
	char* ptr = heapBase - offset;
	if (ptr < heapFree)  
	{
		ReportBug((char*)"String offset into free space\r\n")
		return NULL;
	}
	if (ptr > heapBase)  
	{
		ReportBug((char*)"String offset before heap space\r\n")
		return NULL;
	}
	return ptr;
}

bool PreallocateHeap(size_t len) // do we have the space
{
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

char* AllocateHeap(char* word,size_t len,int bytes,bool clear, bool purelocal) // BYTES means size of unit
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
	len *= bytes; // this many units of this size
	if (len == 0)
	{
		if (!word ) return NULL;
		len = strlen(word);
	}
	if (word) ++len;	// null terminate string
	if (purelocal) len += 2;	// for `` prefix
	size_t allocationSize = len;
	if (bytes == 1 && dictionaryLocked && !compiling && !loading) 
	{
		allocationSize += ALLOCATESTRING_SIZE_PREFIX + ALLOCATESTRING_SIZE_SAFEMARKER; 
		// reserve space at front for allocation sizing not in dict items though (variables not in plannning mode can reuse space if prior is big enough)
		// initial 2 test area
	}

	//   always allocate in word units
	unsigned int allocate = ((allocationSize + 3) / 4) * 4;
	heapFree -= allocate; // heap grows lower, stack grows higher til they collide
 	if (bytes > 4) // force 64bit alignment alignment
	{
		uint64 base = (uint64) heapFree;
		base &= 0xFFFFFFFFFFFFFFF8ULL;  // 8 byte align
		heapFree = (char*) base;
	}
 	else if (bytes == 4) // force 32bit alignment alignment
	{
		uint64 base = (uint64) heapFree;
		base &= 0xFFFFFFFFFFFFFFFCULL;  // 4 byte align
		heapFree = (char*) base;
	}
 	else if (bytes == 2) // force 16bit alignment alignment
	{
		uint64 base = (uint64) heapFree;
		base &= 0xFFFFFFFFFFFFFFFEULL; // 2 byte align
		heapFree = (char*) base;
	}
	else if (bytes != 1) 
		ReportBug((char*)"Allocation of bytes is not std unit %d", bytes);

	// create marker
	if (bytes == 1  && dictionaryLocked && !compiling && !loading) 
	{
		heapFree[0] = ALLOCATESTRING_MARKER; // we put ff ff  just before the sizing data
		heapFree[1] = ALLOCATESTRING_MARKER;
	}

	char* newword =  heapFree;
	if (bytes == 1 && dictionaryLocked && !compiling && !loading) // store size of allocation to enable potential reuse by $var assign and by wordStarts tokenize
	{
		newword += ALLOCATESTRING_SIZE_SAFEMARKER;
		allocationSize -= ALLOCATESTRING_SIZE_PREFIX + ALLOCATESTRING_SIZE_SAFEMARKER; // includes the end of string marker
		if (ALLOCATESTRING_SIZE_PREFIX == 3)		*newword++ = (unsigned char)(allocationSize >> 16); 
		*newword++ = (unsigned char)(allocationSize >> 8) & 0x000000ff;  
		*newword++ = (unsigned char) (allocationSize & 0x000000ff);
	}
	
	int nominalLeft = maxHeapBytes - (heapBase - heapFree);
	if ((unsigned long) nominalLeft < minHeapAvailable) minHeapAvailable = nominalLeft;
	if ((heapBase-heapFree) > 50000000) // when heap has used up 50Mb
	{
		int xx = 0;
	}
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
	char server[1000];
	char* id = loginID;
	if (*id == 'b' && !id[1]) id = "u-8b02518d-c148-5d45-936b-491d39ced70c"; // cheat override to test outside of kore logins
	if (decrypt) sprintf(server, "%s%s/datavalues/decryptedtokens", xserver, id);
	else sprintf(server, "%s%s/datavalues/encryptedtokens", xserver, id);

//loginID
	// for decryption, the value we are passed is a quoted string. We need to avoid doubling those
	
#ifdef INFORMATION
	// legal json requires these characters be escaped, but we cannot change our file system ones to \r\n because
	// we cant tell which are ours and which are users. So we change to 7f7f coding for /r/n.
	// and we change to 0x31 for double quote.
	\b  Backspace (ascii code 08) -- never seeable
	\f  Form feed (ascii code 0C) -- never seable
	\n  New line -- only from our user topic write
	\r  Carriage return -- only from our user topic write
	\t  Tab -- never seeable
	\"  Double quote -- seeable in user data
	\\  Backslash character -- ??? not expected but possible
	UserFile encoding responsible for normal JSON safe data.

	Note MongoDB code also encrypts \r\n the same way. but we dont know HERE that mongo is used
	and we don't know THERE that encryption was used. That is redundant but minor.
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
	callArgumentList[callArgumentIndex++] =   "direct"; // get the text of it gives us, dont make facts out of it
	callArgumentList[callArgumentIndex++] =    "POST";
	strcpy(url,server);
	callArgumentList[callArgumentIndex++] =    url;
	callArgumentList[callArgumentIndex++] =    (char*) buffer;
	callArgumentList[callArgumentIndex++] =    header;
	callArgumentList[callArgumentIndex++] =    ""; // timer override
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
		return userFileSystem.userWrite(buffer,1,msgsize,file);
	}
	else return userFileSystem.userWrite(buffer,size,count,file);
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

int MakeDirectory(char* directory)
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
		Log(STDTRACELOG,(char*)"execution path: %s\r\n",word);
	}

	if (GetCurrentDir(word, MAX_WORD_SIZE)) Log(STDTRACELOG,(char*)"current directory path: %s\r\n",word);

	Log(STDTRACELOG,(char*)"readPath: %s\r\n",readPath);
	Log(STDTRACELOG,(char*)"writeablePath: %s\r\n",writePath);
	Log(STDTRACELOG,(char*)"untouchedPath: %s\r\n",staticPath);
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
	if (*readPath) sprintf(path,(char*)"%s/%s",staticPath,name);
	else strcpy(path,name);
	return fopen(path,(char*)"rb");
}

FILE* FopenReadOnly(const char* name) // read-only potentially changed data file read path (TOPIC)
{
	StartFile(name);
	char path[MAX_WORD_SIZE];
	if (*readPath) sprintf(path,(char*)"%s/%s",readPath,name);
	else strcpy(path,name);
	return fopen(path,(char*)"rb");
}

FILE* FopenReadNormal(char* name) // normal C read unrelated to special paths
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
	if (*writePath) sprintf(path,(char*)"%s/%s",writePath,name);
	else strcpy(path,name);
	FILE* out = fopen(path,(char*)"wb");
	if (out == NULL && !inLog) 
		ReportBug((char*)"Error opening binary write file %s: %s\r\n",path,strerror(errno));
	return out;
}

FILE* FopenReadWritten(const char* name) // read from files that have been written by us
{
	StartFile(name);
	char path[MAX_WORD_SIZE];
	if (*writePath) sprintf(path,(char*)"%s/%s",writePath,name);
	else strcpy(path,name);
	return fopen(path,(char*)"rb");
}

FILE* FopenUTF8Write(const char* filename) // insure file has BOM for UTF8
{
	char path[MAX_WORD_SIZE];
	if (*writePath) sprintf(path,(char*)"%s/%s",writePath,filename);
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
	else ReportBug((char*)"Error opening utf8 write file %s: %s\r\n",path,strerror(errno));
	return out;
}

FILE* FopenUTF8WriteAppend(const char* filename,const char* flags) 
{
	char path[MAX_WORD_SIZE];
	if (*writePath) sprintf(path,(char*)"%s/%s",writePath,filename);
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
	else if (!out && !inLog)
		ReportBug((char*)"Error opening utf8writeappend file %s: %s\r\n",path,strerror(errno));
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
            size_t len = strlen(FindFileData.cFileName);
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
            if (recursive && isDirectory(xname)) seendirs = true;
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

string GetUserPathString(const string &loginID)
{
    string userPath;
    if (loginID.length() == 0)  return userPath; // empty string
	size_t len = loginID.length();
	int counter = 0;
    for (size_t i = 0; i < len; i++) // use each character
	{
		if (loginID.c_str()[i] == '.' || loginID.c_str()[i] == '_') continue; // ignore IP . stuff
        userPath.append(1, loginID.c_str()[i]);
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
			MakePath(users, userPath);
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
    x /= 10000; // 100-nanosecond intervals in a millisecond
    x -= 116444736000000000LL; //  unix epoch  Jan 1, 1970.
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
/// RANDOM NUMBERS
////////////////////////////////////////////////////////////////////////



#define X64(n) (n##ULL)
uint64 X64_Table[256] = //   hash table randomizer
{
       X64(0x0000000000000000), X64(0x42f0e1eba9ea3693),       X64(0x85e1c3d753d46d26), X64(0xc711223cfa3e5bb5),
       X64(0x493366450e42ecdf), X64(0x0bc387aea7a8da4c),       X64(0xccd2a5925d9681f9), X64(0x8e224479f47cb76a),
       X64(0x9266cc8a1c85d9be), X64(0xd0962d61b56fef2d),       X64(0x17870f5d4f51b498), X64(0x5577eeb6e6bb820b),
       X64(0xdb55aacf12c73561), X64(0x99a54b24bb2d03f2),       X64(0x5eb4691841135847), X64(0x1c4488f3e8f96ed4),
       X64(0x663d78ff90e185ef), X64(0x24cd9914390bb37c),       X64(0xe3dcbb28c335e8c9), X64(0xa12c5ac36adfde5a),
       X64(0x2f0e1eba9ea36930), X64(0x6dfeff5137495fa3),       X64(0xaaefdd6dcd770416), X64(0xe81f3c86649d3285),
       X64(0xf45bb4758c645c51), X64(0xb6ab559e258e6ac2),       X64(0x71ba77a2dfb03177), X64(0x334a9649765a07e4),
       X64(0xbd68d2308226b08e), X64(0xff9833db2bcc861d),       X64(0x388911e7d1f2dda8), X64(0x7a79f00c7818eb3b),
       X64(0xcc7af1ff21c30bde), X64(0x8e8a101488293d4d),       X64(0x499b3228721766f8), X64(0x0b6bd3c3dbfd506b),
       X64(0x854997ba2f81e701), X64(0xc7b97651866bd192),       X64(0x00a8546d7c558a27), X64(0x4258b586d5bfbcb4),
       X64(0x5e1c3d753d46d260), X64(0x1cecdc9e94ace4f3),       X64(0xdbfdfea26e92bf46), X64(0x990d1f49c77889d5),
       X64(0x172f5b3033043ebf), X64(0x55dfbadb9aee082c),       X64(0x92ce98e760d05399), X64(0xd03e790cc93a650a),
       X64(0xaa478900b1228e31), X64(0xe8b768eb18c8b8a2),       X64(0x2fa64ad7e2f6e317), X64(0x6d56ab3c4b1cd584),
       X64(0xe374ef45bf6062ee), X64(0xa1840eae168a547d),       X64(0x66952c92ecb40fc8), X64(0x2465cd79455e395b),
       X64(0x3821458aada7578f), X64(0x7ad1a461044d611c),       X64(0xbdc0865dfe733aa9), X64(0xff3067b657990c3a),
       X64(0x711223cfa3e5bb50), X64(0x33e2c2240a0f8dc3),       X64(0xf4f3e018f031d676), X64(0xb60301f359dbe0e5),
       X64(0xda050215ea6c212f), X64(0x98f5e3fe438617bc),       X64(0x5fe4c1c2b9b84c09), X64(0x1d14202910527a9a),
       X64(0x93366450e42ecdf0), X64(0xd1c685bb4dc4fb63),       X64(0x16d7a787b7faa0d6), X64(0x5427466c1e109645),
       X64(0x4863ce9ff6e9f891), X64(0x0a932f745f03ce02),       X64(0xcd820d48a53d95b7), X64(0x8f72eca30cd7a324),
       X64(0x0150a8daf8ab144e), X64(0x43a04931514122dd),       X64(0x84b16b0dab7f7968), X64(0xc6418ae602954ffb),
       X64(0xbc387aea7a8da4c0), X64(0xfec89b01d3679253),       X64(0x39d9b93d2959c9e6), X64(0x7b2958d680b3ff75),
       X64(0xf50b1caf74cf481f), X64(0xb7fbfd44dd257e8c),       X64(0x70eadf78271b2539), X64(0x321a3e938ef113aa),
       X64(0x2e5eb66066087d7e), X64(0x6cae578bcfe24bed),       X64(0xabbf75b735dc1058), X64(0xe94f945c9c3626cb),
       X64(0x676dd025684a91a1), X64(0x259d31cec1a0a732),       X64(0xe28c13f23b9efc87), X64(0xa07cf2199274ca14),
       X64(0x167ff3eacbaf2af1), X64(0x548f120162451c62),       X64(0x939e303d987b47d7), X64(0xd16ed1d631917144),
       X64(0x5f4c95afc5edc62e), X64(0x1dbc74446c07f0bd),       X64(0xdaad56789639ab08), X64(0x985db7933fd39d9b),
       X64(0x84193f60d72af34f), X64(0xc6e9de8b7ec0c5dc),       X64(0x01f8fcb784fe9e69), X64(0x43081d5c2d14a8fa),
       X64(0xcd2a5925d9681f90), X64(0x8fdab8ce70822903),       X64(0x48cb9af28abc72b6), X64(0x0a3b7b1923564425),
       X64(0x70428b155b4eaf1e), X64(0x32b26afef2a4998d),       X64(0xf5a348c2089ac238), X64(0xb753a929a170f4ab),
       X64(0x3971ed50550c43c1), X64(0x7b810cbbfce67552),       X64(0xbc902e8706d82ee7), X64(0xfe60cf6caf321874),
       X64(0xe224479f47cb76a0), X64(0xa0d4a674ee214033),       X64(0x67c58448141f1b86), X64(0x253565a3bdf52d15),
       X64(0xab1721da49899a7f), X64(0xe9e7c031e063acec),       X64(0x2ef6e20d1a5df759), X64(0x6c0603e6b3b7c1ca),
       X64(0xf6fae5c07d3274cd), X64(0xb40a042bd4d8425e),       X64(0x731b26172ee619eb), X64(0x31ebc7fc870c2f78),
       X64(0xbfc9838573709812), X64(0xfd39626eda9aae81),       X64(0x3a28405220a4f534), X64(0x78d8a1b9894ec3a7),
       X64(0x649c294a61b7ad73), X64(0x266cc8a1c85d9be0),       X64(0xe17dea9d3263c055), X64(0xa38d0b769b89f6c6),
       X64(0x2daf4f0f6ff541ac), X64(0x6f5faee4c61f773f),       X64(0xa84e8cd83c212c8a), X64(0xeabe6d3395cb1a19),
       X64(0x90c79d3fedd3f122), X64(0xd2377cd44439c7b1),       X64(0x15265ee8be079c04), X64(0x57d6bf0317edaa97),
	   X64(0xd9f4fb7ae3911dfd), X64(0x9b041a914a7b2b6e),       X64(0x5c1538adb04570db), X64(0x1ee5d94619af4648),
       X64(0x02a151b5f156289c), X64(0x4051b05e58bc1e0f),       X64(0x87409262a28245ba), X64(0xc5b073890b687329),
       X64(0x4b9237f0ff14c443), X64(0x0962d61b56fef2d0),       X64(0xce73f427acc0a965), X64(0x8c8315cc052a9ff6),
       X64(0x3a80143f5cf17f13), X64(0x7870f5d4f51b4980),       X64(0xbf61d7e80f251235), X64(0xfd913603a6cf24a6),
       X64(0x73b3727a52b393cc), X64(0x31439391fb59a55f),       X64(0xf652b1ad0167feea), X64(0xb4a25046a88dc879),
       X64(0xa8e6d8b54074a6ad), X64(0xea16395ee99e903e),       X64(0x2d071b6213a0cb8b), X64(0x6ff7fa89ba4afd18),
       X64(0xe1d5bef04e364a72), X64(0xa3255f1be7dc7ce1),       X64(0x64347d271de22754), X64(0x26c49cccb40811c7),
       X64(0x5cbd6cc0cc10fafc), X64(0x1e4d8d2b65facc6f),       X64(0xd95caf179fc497da), X64(0x9bac4efc362ea149),
       X64(0x158e0a85c2521623), X64(0x577eeb6e6bb820b0),       X64(0x906fc95291867b05), X64(0xd29f28b9386c4d96),
       X64(0xcedba04ad0952342), X64(0x8c2b41a1797f15d1),       X64(0x4b3a639d83414e64), X64(0x09ca82762aab78f7),
       X64(0x87e8c60fded7cf9d), X64(0xc51827e4773df90e),       X64(0x020905d88d03a2bb), X64(0x40f9e43324e99428),
       X64(0x2cffe7d5975e55e2), X64(0x6e0f063e3eb46371),       X64(0xa91e2402c48a38c4), X64(0xebeec5e96d600e57),
       X64(0x65cc8190991cb93d), X64(0x273c607b30f68fae),       X64(0xe02d4247cac8d41b), X64(0xa2dda3ac6322e288),
       X64(0xbe992b5f8bdb8c5c), X64(0xfc69cab42231bacf),       X64(0x3b78e888d80fe17a), X64(0x7988096371e5d7e9),
       X64(0xf7aa4d1a85996083), X64(0xb55aacf12c735610),       X64(0x724b8ecdd64d0da5), X64(0x30bb6f267fa73b36),
       X64(0x4ac29f2a07bfd00d), X64(0x08327ec1ae55e69e),       X64(0xcf235cfd546bbd2b), X64(0x8dd3bd16fd818bb8),
       X64(0x03f1f96f09fd3cd2), X64(0x41011884a0170a41),       X64(0x86103ab85a2951f4), X64(0xc4e0db53f3c36767),
       X64(0xd8a453a01b3a09b3), X64(0x9a54b24bb2d03f20),       X64(0x5d45907748ee6495), X64(0x1fb5719ce1045206),
       X64(0x919735e51578e56c), X64(0xd367d40ebc92d3ff),       X64(0x1476f63246ac884a), X64(0x568617d9ef46bed9),
       X64(0xe085162ab69d5e3c), X64(0xa275f7c11f7768af),       X64(0x6564d5fde549331a), X64(0x279434164ca30589),
       X64(0xa9b6706fb8dfb2e3), X64(0xeb46918411358470),       X64(0x2c57b3b8eb0bdfc5), X64(0x6ea7525342e1e956),
       X64(0x72e3daa0aa188782), X64(0x30133b4b03f2b111),       X64(0xf7021977f9cceaa4), X64(0xb5f2f89c5026dc37),
       X64(0x3bd0bce5a45a6b5d), X64(0x79205d0e0db05dce),       X64(0xbe317f32f78e067b), X64(0xfcc19ed95e6430e8),
       X64(0x86b86ed5267cdbd3), X64(0xc4488f3e8f96ed40),       X64(0x0359ad0275a8b6f5), X64(0x41a94ce9dc428066),
       X64(0xcf8b0890283e370c), X64(0x8d7be97b81d4019f),       X64(0x4a6acb477bea5a2a), X64(0x089a2aacd2006cb9),
       X64(0x14dea25f3af9026d), X64(0x562e43b4931334fe),       X64(0x913f6188692d6f4b), X64(0xd3cf8063c0c759d8),
       X64(0x5dedc41a34bbeeb2), X64(0x1f1d25f19d51d821),       X64(0xd80c07cd676f8394), X64(0x9afce626ce85b507)
};

uint64 Hashit(unsigned char * data, int len,bool & hasUpperCharacters, bool & hasUTF8Characters)
{
	hasUpperCharacters = hasUTF8Characters = false;
	uint64 crc = 5381;
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
				crc = ((crc << 5) + crc) + c;
				//crc = X64_Table[(crc >> 56) ^ c] ^ (crc << 8);
				c = *data++; // get the cap form
				c -= 0x80;
				c += 0xa0; // get lower case form
				--len;
			}
			else if (c >= 0xc4 && c <= 0xc9 && !(c & 1))
			{
				hasUpperCharacters = true;
				crc = ((crc << 5) + crc) + c;
				//crc = X64_Table[(crc >> 56) ^ c] ^ (crc << 8);
				c = *data++; // get the cap form
				c |= 1;  // get lower case form
				--len;
			}
			else // other utf8, do all but last char
			{
				int size = UTFCharSize((char*)data - 1);
				while (--size)
				{
					crc = ((crc << 5) + crc) + c;
					//crc = X64_Table[(crc >> 56) ^ c] ^ (crc << 8);
					c = *data++;
					--len;
				}
			}
		}
		if (c == ' ') c = '_';	// force common hash on space vs _
	//better but slower	crc = X64_Table[(crc >> 56) ^ c ] ^ (crc << 8 );
		crc = ((crc << 5) + crc) + c;
	}
	return crc;
} 

unsigned int random(unsigned int range)
{
	if (regression || range <= 1) return 0;
	unsigned int x = (unsigned int)X64_Table[randIndex++ % MAXRAND];
	return x % range;
}

/////////////////////////////////////////////////////////
/// LOGGING
/////////////////////////////////////////////////////////
uint64 logCount = 0;

bool TraceFunctionArgs(FILE* out, char* name, int start, int end)
{
	if (name[0] == '~') return false;
	WORDP D = FindWord(name);
	if (!D) return false;		// like fake ^ruleoutput

	unsigned int args = end - start;
	char arg[MAX_WORD_SIZE];
	fprintf(out, "( ");
	for (int i = 0; i < args; ++i)
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
	int i = globalDepth;
	char rule[MAX_WORD_SIZE];
    CALLFRAME* frame = GetCallFrame(i);
	if (frame && *(frame->label)) 
	{
		rule[0] = 0;
		if (currentRule) {
			strncpy(rule, currentRule, 50);
			rule[50] = 0;
		}
        CALLFRAME* priorframe = GetCallFrame(i - 1);
		fprintf(out,"Finished %d: heapusedOnEntry: %d heapUsedNow: %d buffers:%d stackused: %d stackusedNow:%d %s ",
			i,frame->heapDepth,(int)(heapBase-heapFree),frame->memindex,(int)(heapFree - (char*)releaseStackDepth[i]), (int)(stackFree-stackStart),frame->label);
		if (priorframe && !TraceFunctionArgs(out, frame->label, (i > 0) ? priorframe->argumentStartIndex: 0, frame->argumentStartIndex)) fprintf(out, " - %s", rule);
		fprintf(out, "\r\n");
	}
	while (--i > 1) 
	{
        frame = GetCallFrame(i);
		strncpy(rule,frame->rule,50);
		rule[50] = 0;
		fprintf(out,"BugDepth %d: heapusedOnEntry: %d buffers:%d stackused: %d %s ",
			i,frame->heapDepth,GetCallFrame(i)->memindex,
            (int)(heapFree - (char*)releaseStackDepth[i]), frame->label);
		if (!TraceFunctionArgs(out, frame->label, GetCallFrame(i-1)->argumentStartIndex, GetCallFrame(i - 1)->argumentStartIndex)) fprintf(out, " - %s", rule);
		fprintf(out, "\r\n");
	}
}

CALLFRAME* ChangeDepth(int value,char* name,bool nostackCutback, char* code)
{
    if (value == 0) // secondary call from userfunction. frame is now valid
    {
        CALLFRAME* frame = releaseStackDepth[globalDepth];
        if (showDepth) Log(STDUSERLOG,(char*)"same depth %d %s \r\n", globalDepth, name);
        if (name) frame->label = name; // override
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

#ifndef DISCARDTESTING
        if (globalDepth && *name && debugCall)
        {
            char* result = (*debugCall)(name, false);
            if (result) abort = true;
        }
#endif
		if (frame->memindex  != bufferIndex)
		{
			ReportBug((char*)"depth %d not closing bufferindex correctly at %s bufferindex now %d was %d\r\n",globalDepth,name,bufferIndex,frame->memindex);
		}
        callArgumentIndex = frame->argumentStartIndex;
        currentRuleID = frame->oldRuleID;
        currentTopicID = frame->oldTopic;
        currentRuleTopic = frame->oldRuleTopic;
        currentRule = frame->oldRule;

		// engine functions that are streams should not destroy potential local adjustments
		if (!nostackCutback) 
			stackFree = (char*) frame; // deallocoate ARGUMENT space
		if (showDepth) Log(STDUSERLOG, (char*)"-depth %d %s bufferindex %d heapused:%d\r\n", globalDepth,name, bufferIndex,(int)(heapBase-heapFree));
        globalDepth += value;
        if (globalDepth < 0) { ReportBug((char*)"bad global depth in %s", name); globalDepth = 0; }
        return (abort) ?  (CALLFRAME*)1 : NULL; // abort code
    }
	else // value > 0
	{
        stackFree = (char*)(((uint64)stackFree + 7) & 0xFFFFFFFFFFFFFFF8ULL);
        CALLFRAME* frame = (CALLFRAME*) stackFree;
        stackFree += sizeof(CALLFRAME);
        memset(frame, 0, sizeof(CALLFRAME));
        frame->label = name;
        frame->code = code;
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
        if (showDepth) Log(STDUSERLOG, (char*)"+depth %d %s bufferindex %d heapused: %d stackused:%d gap:%d\r\n", globalDepth, name, bufferIndex, (int)(heapBase - heapFree), (int)(stackFree - stackStart), (int)(heapFree - stackFree));
        releaseStackDepth[globalDepth] = frame; // define argument start space - release back to here on exit

#ifndef DISCARDTESTING
		if (globalDepth && *name != '*' && debugCall) (*debugCall)(name, true); 
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
                sprintf(at, (char*)"%I64d", va_arg(ap, uint64));
#else
                sprintf(at, (char*)"%lld", va_arg(ap, uint64));
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
                sprintf(at, (char*)"%I64d", va_arg(ap, long long int));
#else
                sprintf(at, (char*)"%lld", va_arg(ap, long long int));
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
                if (s) sprintf(at, (char*)"%s", s);
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
                    else sprintf(at, (char*)" Bad int precision %d ", precision);
                }
                else
                {
                    if (precision == 8) sprintf(at, (char*)"%08x", i);
                    else sprintf(at, (char*)" Bad hex precision %d ", precision);
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
        if ((at - base) >= (logsize - SAFE_BUFFER_MARGIN)) break; // prevent log overflow
    }
    *at = 0;
    return at;
}

int myprintf(const char * fmt, ...)
{
    if (!logmainbuffer) logmainbuffer = (char*)malloc(logsize);
    char* at = logmainbuffer;
    *at = 0;
    va_list ap;
    va_start(ap, fmt);
    const char *ptr = fmt - 1;
    at = myprinter(ptr, at, ap);
    va_end(ap);
    if (debugOutput) (*debugOutput)(logmainbuffer);
    return 1;
}

unsigned int Log(unsigned int channel,const char * fmt, ...)
{
	if (channel == STDTRACELOG) channel = STDUSERLOG;
	static unsigned int id = 1000;	
	if (quitting) return id;
	logged = true;
	bool localecho = false;
	bool noecho = false;
	if (channel == ECHOSTDTRACELOG)
	{
		localecho = true;
		channel = STDUSERLOG;
	}
	if (channel == STDTIMELOG) {
		noecho = true;
		channel = STDUSERLOG;
	}
	if (channel == STDTIMETABLOG) {
		noecho = true;
		channel = STDTRACETABLOG;
	}
	// allow user logging if trace is on.
	if (!userLog && (channel == STDUSERLOG || channel > 1000 || channel == id) && !testOutput && !trace) return id;
    if (!fmt)  return id; // no format or no buffer to use
	if ((channel == SERVERLOG) && server && !serverLog)  return id; // not logging server data

	static int priordepth = 0;

	// start writing normal data here
    if (!logmainbuffer) logmainbuffer = (char*)malloc(logsize);
    char* at = logmainbuffer;
    *at = 0;

    va_list ap; 
    va_start(ap,fmt);
	++logCount;
    const char *ptr = fmt - 1;

	//   when this channel matches the ID of the prior output of log,
	//   we dont force new line on it.
	if (channel == id) //   join result code onto intial description
	{
		channel = 1;
		strcpy(at,(char*)"    ");
		at += 4;
	}
	//   any channel above 1000 is same as 101
	else if (channel > 1000) channel = STDTRACELOG; //   force result code to indent new line

	//   channels above 100 will indent when prior line not ended
	if (channel != BUGLOG && channel >= STDTRACELOG && logLastCharacter != '\\') //   indented by call level and not merged
	{ //   STDTRACELOG 101 is std indending characters  201 = attention getting
		if (logLastCharacter == 1 && globalDepth == priordepth) {} // we indented already
		else if (logLastCharacter == 1 && globalDepth > priordepth) // we need to indent a bit more
		{
			for (int i = priordepth; i < globalDepth; i++)
			{
				*at++ = (i == 4 || i == 9) ? ',' : '.';
			}
			priordepth = globalDepth;
		}
		else 
		{
			if (logLastCharacter != '\n') // log legal
			{
				*at++ = '\r'; //   close out this prior thing
				*at++ = '\n'; // log legal
			}
			while (ptr[1] == '\n' || ptr[1] == '\r') // we point BEFORE the format
			{
				*at++ = *++ptr;
			}

			int n = globalDepth;
			if (n < 0) n = 0; //   just in case
			for (int i = 0; i < n; i++)
			{
				if (channel == STDTRACEATTNLOG) *at++ = (i == 1) ? '*' : ' ';
				else *at++ = (i == 4 || i == 9) ? ',' : '.';
			}
			priordepth = globalDepth;
		}
 	}
	channel %= 100;
    at = myprinter(ptr,at, ap);
    va_end(ap); 

	// implement unlogged data
	if (hide)
	{
		char* hidden = hide; // multiple hiddens separated by spaces, presumed covered by quotes in real data
		char* next = hidden; // start of next field name
		char word[MAX_WORD_SIZE]; // thing we search for
		*word = '"';
		while (next)
		{
			char* begin = next; 
			next = strchr(begin, ' '); // example is  "authorized" or authorized or "authorized": or whatever
			if (next) *next = 0;
			strcpy(word+1, begin);
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

	if (channel == STDDEBUGLOG) // debugger output
	{
		if (debugOutput) (*debugOutput)(logmainbuffer); // send to CS debugger
		else printf("%s",logmainbuffer); // directly show to user
		inLog = false;
		return ++id;
	}

	logLastCharacter = (at > logmainbuffer) ? *(at-1) : 0; //   ends on NL?
	if (fmt && !*fmt) logLastCharacter = 1; // special marker 
	if (logLastCharacter == '\\') *--at = 0;	//   dont show it (we intend to merge lines)
	logUpdated = true; // in case someone wants to see if intervening output happened
	size_t bufLen = at - logmainbuffer;
	inLog = true;
	bool bugLog = false;
	
	if (pendingWarning)
	{
		AddWarning(logmainbuffer);
		pendingWarning = false;
	}
	else if (!strnicmp(logmainbuffer,(char*)"*** Warning",11))
		pendingWarning = true;// replicate for easy dump later
	
	if (pendingError)
	{
		AddError(logmainbuffer);
		pendingError= false;
	}
	else if (!strnicmp(logmainbuffer,(char*)"*** Error",9))
		pendingError = true;// replicate for easy dump later

#ifndef DISCARDSERVER
#ifndef EVSERVER
    if (server) GetLogLock();
#endif
#endif	

	if (channel == BADSCRIPTLOG || channel == BUGLOG) 
	{
		if (!strnicmp(logmainbuffer,"FATAL",5)){;} // log fatalities anyway
		else if (channel == BUGLOG && server && !serverLog)  return id; // not logging server data
	
		bugLog = true;
		char name[MAX_WORD_SIZE];
		sprintf(name,(char*)"%s/bugs.txt",logs);
		FILE* bug = FopenUTF8WriteAppend(name);
		char located[MAX_WORD_SIZE];
		*located = 0;
		if (currentTopicID && currentRule) sprintf(located,(char*)" script: %s.%d.%d",GetTopicName(currentTopicID),TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID));
		if (bug) //   write to a bugs file
		{
			if (*currentFilename) fprintf(bug,(char*)"BUG in %s at %d: %s ",currentFilename,currentFileLine,readBuffer);
			if (!compiling && !loading && channel == BUGLOG && *currentInput)  
			{
				char* buffer = AllocateBuffer(); // transient - cannot insure not called from context of InfiniteStack
				struct tm ptm;
				if (buffer) fprintf(bug,(char*)"\r\nBUG: %s: input:%d %s caller:%s callee:%s at %s in sentence: %s\r\n",GetTimeInfo(&ptm,true),volleyCount, logmainbuffer,loginID,computerID,located,currentInput);
				else fprintf(bug,(char*)"\r\nBUG: %s: input:%d %s caller:%s callee:%s at %s\r\n",GetTimeInfo(&ptm,true),volleyCount, logmainbuffer,loginID,computerID,located);
				FreeBuffer();
			}
			fwrite(logmainbuffer,1,bufLen,bug);
			fprintf(bug,(char*)"\r\n");
			if (!compiling && !loading && !strstr(logmainbuffer, "No such bot"))
			{
				fprintf(bug,(char*)"MinReleaseStackGap %dMB MinHeapAvailable %dMB\r\n",maxReleaseStackGap/1000000,(int)(minHeapAvailable/1000000));
				fprintf(bug,(char*)"MaxBuffers used %d of %d\r\n\r\n",maxBufferUsed,maxBufferLimit);
				BugBacktrace(bug);
			}
			fclose(bug); // dont use FClose

		}
		if ((echo||localecho) && !silent && !server)
		{
			struct tm ptm;
			if (*currentFilename) fprintf(stdout,(char*)"\r\n   in %s at %d: %s\r\n    ",currentFilename,currentFileLine,readBuffer);
			else if (*currentInput) fprintf(stdout,(char*)"\r\n%d %s in sentence: %s \r\n    ",volleyCount,GetTimeInfo(&ptm,true),currentInput);
		}
		strcat(logmainbuffer,(char*)"\r\n");	//   end it

		if (!strnicmp(logmainbuffer,"FATAL",5))
		{
			if (!compiling && !loading) 
			{
				char fname[100];
				sprintf(fname,(char*)"%s/exitlog.txt",logs);
				FILE* out = FopenUTF8WriteAppend(fname);
				if (out) 
				{
					struct tm ptm;
					fprintf(out,(char*)"\r\n%s: input:%s caller:%s callee:%s\r\n",GetTimeInfo(&ptm,true),currentInput,loginID,computerID);
					BugBacktrace(bug);
					fclose(out);
				}
			}
			myexit(logmainbuffer); // log fatalities anyway
		}
		channel = STDUSERLOG;	//   use normal logging as well
	}

	if (server){} // dont echo  onto server console 
    else if ((!noecho && (echo || localecho || trace & TRACE_ECHO) && channel == STDUSERLOG))
    {
       if (!debugOutput) fwrite(logmainbuffer, 1, bufLen, stdout);
       else (*debugOutput)(logmainbuffer);
    }
	bool doserver = true;

    FILE* out = NULL;
	if (server && trace && !userLog) channel = SERVERLOG;	// force traced server to go to server log since no user log
	char fname[MAX_WORD_SIZE];
    if (logFilename[0] != 0 && channel != SERVERLOG && channel != DBTIMELOG)  
	{
		strcpy(fname, logFilename);
		char defaultlogFilename[MAX_BUFFER_SIZE];
		sprintf(defaultlogFilename,"%s/log%u.txt",logs,port); // DEFAULT LOG
		if (strcmp(fname, defaultlogFilename) == 0){ // of log is default log i.t log<port>.txt
			char holdBuffer[MAX_BUFFER_SIZE];
			struct tm ptm;
			sprintf(holdBuffer,"%s - %s", GetTimeInfo(&ptm),logmainbuffer);
			strcpy(logmainbuffer, holdBuffer);
		}
		out =  FopenUTF8WriteAppend(fname);
 		if (!out) // see if we can create the directory (assuming its missing)
		{
            MakeDirectory(users);
			out = userFileSystem.userCreate(fname);
			if (!out && !inLog) ReportBug((char*)"unable to create user logfile %s", fname);
		}
	}
	else if(channel == DBTIMELOG) // do db log 
	{
		strcpy(fname, dbTimeLogfileName); // one might speed up forked servers by having mutex per pid instead of one common one
		out = FopenUTF8WriteAppend(fname);
 		if (!out) // see if we can create the directory (assuming its missing)
		{
            MakeDirectory(logs);
			out = userFileSystem.userCreate(fname);
		}

		if (loglimit)
		{ 
			// roll log if it is too big
			int64 size;
			int r = fseek(out, 0, SEEK_END);
	#ifdef WIN32
			size = _ftelli64(out);
	#else
			size = ftello(out);
	#endif
			// get MB count of it roughly
			size /= 1000000;  // MB bytes
			if (size >= loglimit)
			{
				fclose(out);
				time_t curr = time(0);
				char* when = GetMyTime(curr); // now
				char newname[MAX_WORD_SIZE];
				char* old = strrchr(fname, '/');
				strcpy(newname, old+1); // just the name, no directory path
				char* at = strrchr(newname, '.');
				sprintf(at, "-%s.txt",when);
				at = strchr(newname, ':');
				*at = '-';
				at = strchr(newname, ':');
				*at = '-';

#ifdef WIN32
				SetCurrentDirectory((char*)"LOGS"); 
#else
				chdir((char*)"LOGS");
#endif
				int result = rename(old+1, newname); // some renames cant handle directory spec
				if (result != 0) 
					perror("Error renaming file");

#ifdef WIN32
				SetCurrentDirectory((char*)"..");
#else
				chdir((char*)"..");
#endif

				out = FopenUTF8WriteAppend(fname);
			}
		}
	}
    else // do server log 
	{
		strcpy(fname, serverLogfileName); // one might speed up forked servers by having mutex per pid instead of one common one
		out = FopenUTF8WriteAppend(fname);
 		if (!out) // see if we can create the directory (assuming its missing)
		{
            MakeDirectory(logs);
			out = userFileSystem.userCreate(fname);
		}

		if (loglimit)
		{ 
			// roll log if it is too big
			int64 size;
			int r = fseek(out, 0, SEEK_END);
	#ifdef WIN32
			size = _ftelli64(out);
	#else
			size = ftello(out);
	#endif
			// get MB count of it roughly
			size /= 1000000;  // MB bytes
			if (size >= loglimit)
			{
				fclose(out);
				time_t curr = time(0);
				char* when = GetMyTime(curr); // now
				char newname[MAX_WORD_SIZE];
				char* old = strrchr(fname, '/');
				strcpy(newname, old+1); // just the name, no directory path
				char* at = strrchr(newname, '.');
				sprintf(at, "-%s.txt",when);
				at = strchr(newname, ':');
				*at = '-';
				at = strchr(newname, ':');
				*at = '-';

#ifdef WIN32
				SetCurrentDirectory((char*)"LOGS"); 
#else
				chdir((char*)"LOGS");
#endif
				int result = rename(old+1, newname); // some renames cant handle directory spec
				if (result != 0) 
					perror("Error renaming file");

#ifdef WIN32
				SetCurrentDirectory((char*)"..");
#else
				chdir((char*)"..");
#endif

				out = FopenUTF8WriteAppend(fname);
			}
		}
	}

    if (out) 
    {
		if (doserver)
		{
			fwrite(logmainbuffer,1,bufLen,out);
			struct tm ptm;
			if (!bugLog);
 			else if (*currentFilename) fprintf(out,(char*)"   in %s at %d: %s\r\n    ",currentFilename,currentFileLine,readBuffer);
			else if (*currentInput) fprintf(out,(char*)"%d %s in sentence: %s \r\n    ",volleyCount,GetTimeInfo(&ptm,true),currentInput);
		}
		fclose(out); // dont use FClose
		if (channel == SERVERLOG && echoServer)  (*printer)((char*)"%s", logmainbuffer);
    }
	
#ifndef DISCARDSERVER
	if (testOutput && server) // command outputs
	{
		size_t len = strlen(testOutput);
		if ((len + bufLen) < (maxBufferSize - SAFE_BUFFER_MARGIN)) strcat(testOutput, logmainbuffer);
	}

#ifndef EVSERVER
    if (server) ReleaseLogLock();
#endif
#endif

	inLog = false;
	return ++id;
}

