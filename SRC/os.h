#ifndef _OSH_
#define _OSH_

#ifdef INFORMATION
Copyright (C)2011-2021 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif

#define SAFE_BUFFER_MARGIN 2000

struct FACT;

typedef unsigned int FACTOID; //   a fact index
typedef unsigned int FACTOID_OR_MEANING;	// a fact or a meaning (same representation)

typedef char* STACKREF; 
typedef char* HEAPREF;
typedef unsigned int HEAPINDEX; 

struct WORDENTRY;
typedef WORDENTRY* WORDP;

typedef struct WORDENTRY //   a dictionary entry  - starred items are written to the dictionary
{
    uint64  properties;				//   main language description of this node OR numeric value of DEFINE
    uint64	hash;
    uint64  systemFlags;			//   additional dictionary and non-dictionary properties 
    char*     word;					//   entry name
    unsigned int internalBits;
    unsigned int parseBits;			// only for words, not for function names or concept names
                                    // functions/topics use this for offset into the map file where it is defined (debugger)
									// OR offset of primary entry for which this is number value

                                    // if you edit this, you may need to change ReadBinaryEntry and WriteBinaryEntry
    union {
        char* topicBots;			//  for topic name (start with ~) or planname (start with ^) - bot topic applies to  - only used by script compiler
        unsigned int planArgCount;	//  number of arguments in a plan
        unsigned char* fndefinition;//  for nonplan macro name (start with ^) - if FUNCTION_NAME is on and not system function, is user script - 1st byte is argument count
        char* userValue;			//  if a $uservar (start with $) OR if a query label is query string
        WORDP substitutes;			//  words (with systemFlags HAS_SUBSTITUTE) that should be adjusted to during tokenization
        MEANING*  glosses;			//  for ordinary words: list of glosses for synset head meanings - is offset to allocstring and id index of meaning involved.
        char* conditionalIdiom;		//  test code headed by ` for accepting word as an idiom instead of its individual word components
    }w;

    FACTOID subjectHead;		//  start threads for facts run thru here 
    FACTOID verbHead;			//  start threads for facts run thru here 
    FACTOID objectHead;			//  start threads for facts run thru here 

    MEANING  meanings;			//  list of meanings (synsets) of this word - Will be wordnet synset id OR self ptr -- 1-based since 0th meaning means all meanings
    unsigned int length;		//  length of the word
    unsigned int inferMark;		// (functions use as trace control bits) no need to erase been here marker during marking facts, inferencing (for propogation) and during scriptcompile 
    MEANING spellNode;			// next word of same length as this - not used for function names (time tracing bits go here) and concept names
    unsigned int nextNode;		// bucket-link for dictionary hash + top bye GETMULTIWORDHEADER // can this word lead a phrase to be joined - can vary based on :build state -- really only needs 4 bits

    union {
        unsigned int topicIndex;	//   for a ~topic or %systemVariable or plan, this is its id
        unsigned int codeIndex;		//   for a system function, its the table index for it
        unsigned int debugIndex;	//   for a :test function, its the table index for it
    }x;
#ifndef DISCARDCOUNTER
    unsigned int counter;			// general storage slot
//    unsigned int counter1;			// general storage slot
#endif
} WORDENTRY;

typedef struct CALLFRAME
{
    char* label;
    char* rule;
    char** display; // where display archive is saved
    char* definition; // actual definition chosen to execute: (locals) + code
    WORDP name; // basic name
    char* code; // code after locals
    char* oldRule;
    char* heapstart;
    int varBaseIndex; // where fnvar base is
    unsigned int arguments; // how many arguments function has
    int oldbase;
    unsigned int outputlevel;
    int argumentStartIndex;
    unsigned int oldRuleID;
    unsigned int oldTopic;
    unsigned int oldRuleTopic;
	unsigned int depth;
    unsigned int memindex;
    unsigned int heapDepth;
    union  {
        FunctionResult result;
        int ownvalue;
    }x;

}CALLFRAME;

enum APICall {
	NO_API_CALL = 0,
	COMPILE_PATTERN = 1,
	COMPILE_OUTPUT = 2,
	TEST_PATTERN = 3,
	TEST_OUTPUT = 4
};

#define SAVESYSTEMSTATE()     int oldDepth = globalDepth; int oldScriptBufferIndex = bufferIndex; char* oldStack = stackFree;
#define RESTORESYSTEMSTATE()  infiniteStack = false; globalDepth = oldDepth; bufferIndex = oldScriptBufferIndex; stackFree = oldStack;

// EXCEPTION/ERROR
// error recovery
extern jmp_buf scriptJump[20];
extern char* lastheapfree;
extern bool prelog;
extern jmp_buf crashJump;
extern int jumpIndex;
void ShowMemory(char* label);
void JumpBack();
void myexit(const char* msg, int code = 4);
void mystart(char* msg);
extern char hide[4000];
#define NORMALFILES 0
#define MONGOFILES 1
#define POSTGRESFILES 2
#define MYSQLFILES 3
#define MICROSOFTSQLFILES 4
extern int adjustIndent;
extern bool logged;
extern int filesystemOverride;
#define MAX_GLOBAL 600
extern int ide;
extern bool idestop;
extern bool idekey;
#define RECORD_SIZE 4000
extern char externalBugLog[1000];
extern FILE* userlogFile;
extern bool pseudoServer;
extern bool authorize;

// MEMORY SYSTEM
extern char logLastCharacter;
extern int inputSize;
extern bool inputLimitHit;
extern bool convertTabs;
extern bool infiniteStack;
extern bool infiniteHeap;
extern CALLFRAME* releaseStackDepth[MAX_GLOBAL];
extern unsigned int maxBufferLimit;
extern unsigned int maxReleaseStackGap;
extern unsigned int maxBufferSize;
extern unsigned int maxBufferUsed;	
extern unsigned int bufferIndex;
extern unsigned int baseBufferIndex;
extern unsigned int overflowIndex;
extern char* buffers;
extern uint64 discard;
extern bool showmem;
extern size_t maxHeapBytes;
extern char* heapBase;
extern char* heapFree;
extern char* stackFree;
extern char* stackStart;
extern char* heapEnd;
extern unsigned long minHeapAvailable;
extern int loglimit;
HEAPREF Index2Heap(HEAPINDEX offset);
inline HEAPINDEX Heap2Index(char* str) {return (!str) ? 0 : (unsigned int)(heapBase - str);}

// MEMORY SYSTEM
void ResetBuffers();
char* AllocateBuffer(char*name = (char*) "");
void FreeBuffer(char*name = (char*) "");
void CloseBuffers();
char* AllocateStack(const char* word, size_t len = 0, bool localvar = false, int align = 0);
void ReleaseInfiniteStack();
void CompleteInfiniteHeap(char* base);
void ReleaseStack(char* word);
char* InfiniteHeap(const char* caller);
char* InfiniteStack(char*& limit, const char* caller);
void CompleteBindStack64(int n,char* base);
void CompleteBindStack(int used = 0);
bool AllocateStackSlot(char* variable);
char** RestoreStackSlot(char* variable,char** slot);
char* AllocateHeap(const char* word,size_t len = 0,int bytes= 1,bool clear = false,bool purelocal = false);
char* AllocateConstHeap(char* word, size_t len = 0, int bytes = 1, bool clear = false, bool purelocal = false);
bool PreallocateHeap(size_t len);
void LoggingCheats(char* incoming);
void ResetHeapFree(char* buffer);
void ProtectNL(char* buffer);
void PrepIndent();
HEAPREF AllocateHeapval(HEAPREF linkval, uint64 val1, uint64 val2, uint64 val3 = 0);
HEAPREF UnpackHeapval(HEAPREF linkval, uint64 & val1, uint64 & val2, uint64& val3 = discard);
STACKREF AllocateStackval(STACKREF linkval, uint64 val1, uint64 val2 = 0, uint64 val3 = 0);
STACKREF UnpackStackval(STACKREF linkval, uint64& val1, uint64& val2 = discard, uint64& val3 = discard);
void InitStackHeap();
void FreeStackHeap();
bool KeyReady();
bool InHeap(char* ptr);
bool InStack(char* ptr);
void CloseDatabases(bool restart = false);
FunctionResult AuthorizedCode(char* buffer);
// FILE SYSTEM
int MakeDirectory(const char* directory);
void EncryptInit(char* params);
void DecryptInit(char* params);
void EncryptRestart();
extern unsigned int currentFileLine;
extern unsigned int currentLineColumn;
extern unsigned int maxFileLine;
extern char currentFilename[MAX_WORD_SIZE];
int FClose(FILE* file);
void InitFileSystem(char* untouchedPath,char* readablePath,char* writeablePath);
void C_Directories(char* x);
void ResetEncryptTags();
void StartFile(const char* name);
int FileSize(FILE* in,char* buffer,size_t allowedSize);
void FileDelete(const char* filename);
FILE* FopenStaticReadOnly(const char* name);
FILE* FopenReadOnly(const char* name);
FILE* FopenReadNormal(const char* name);
FILE* FopenReadWritten(const char* name);
FILE* FopenBinaryWrite(const char* name); // only for binary data files
FILE* FopenUTF8Write(const char* filename);
FILE* FopenUTF8WriteAppend(const char* filename, const char* flags = "ab");
typedef void (*FILEWALK)(char* name, uint64 flag);
void CopyFile2File(const char* newname,const char* oldname,bool autoNumber);
bool LogEndedCleanly();
typedef int (*PRINTER)(const char * format, ...);
typedef void(*DEBUGINTPUT)(const char * data);
typedef void(*DEBUGOUTPUT)(const char * data);
extern PRINTER  printer;

#ifdef INFORMATION
Normally user topic files are saved on the local filesystem using fopen, fclose, fread, fwrite, fseek, ftell calls.
If you want to store them elsewhere, like in a database, you can pass into InitSystem a nonnull USERFILESYSTEM block.
The pattern of calls is that the system will do a create or open call. A nonzero value will be passed around, but it doesnt really matter
because you can assume only one "FILE" is in use at a time so you can manage that with globals outside of CS. 
For writing, the system will  create the file (it should be "binary utf8"), perform a single write,  and then close it.
For reading, the system will first open it, ask for the size of its contents, then read once and close it.
#endif

typedef FILE* (*UserFileCreate)(const char* name);
typedef FILE* (*UserFileOpen)(const char* name);
typedef int (*UserFileClose)(FILE*);
typedef size_t (*UserFileRead)(void* buffer,size_t size, size_t count, FILE* file);
typedef size_t (*UserFileWrite)(const void* buffer,size_t size, size_t count, FILE* file);
typedef int (*UserFileSize)(FILE* file, char* buffer, size_t allowedSize);
typedef void (*UserFileDelete)(const char* name);
typedef size_t (*UserFileDecrypt)(void* buffer,size_t size, size_t count, FILE* file,char* filekind);
typedef size_t (*UserFileEncrypt)(const void* buffer,size_t size, size_t count, FILE* file,char* filekind);

typedef char*(*AllocatePtr)(char* word, size_t len, int bytes, bool clear, bool purelocal);

typedef struct USERFILESYSTEM //  how to access user topic data
{
	UserFileCreate userCreate;  // "wb" implied
	UserFileOpen userOpen; 
	UserFileClose userClose;
	UserFileRead userRead;
	UserFileWrite userWrite;
	UserFileDelete userDelete;
	UserFileEncrypt userEncrypt;
	UserFileDecrypt userDecrypt;

} USERFILESYSTEM;

extern USERFILESYSTEM userFileSystem;
void InitUserFiles();
void WalkDirectory(char* directory,FILEWALK function, uint64 flags,bool recursive);
size_t DecryptableFileRead(void* buffer,size_t size, size_t count, FILE* file,bool decrypt,char* filekind);
size_t EncryptableFileWrite(void* buffer,size_t size, size_t count, FILE* file,bool encrypt,char* filekind);
char* GetUserPath(char* name);

// TIME

#define SKIPWEEKDAY 4 // from gettimeinfo

char* GetTimeInfo(struct tm* ptm, bool nouser=false,bool utc=false);
char* GetMyTime(time_t curr);
void mylocaltime (const time_t * timer, struct tm* ptm);
void myctime(time_t * timer,char* buffer);

#ifdef __MACH__
void clock_get_mactime(struct timespec &ts);
#endif
uint64 ElapsedMilliseconds();
#ifndef WIN32
unsigned int GetFutureSeconds(unsigned int seconds);
#endif

#include <map>
using namespace std;
extern std::map <WORDP, uint64> timeSummary;  // per volley time data about functions etc

// LOGGING WHERE
#define NO_LOG 0
#define FILE_LOG 1
#define STDOUT_LOG 2
#define STDERR_LOG 4
#define PRE_LOG 8

// LOGGING WHICH
#define SERVERLOG 0 // top-level 
#define USERLOG 1 // top-level 
#define ECHOUSERLOG 3 // remaps to USERLOG + local echo
#define ECHOSERVERLOG 4 // remaps to SERVERLOG + local echo
#define STDTIMELOG 5  // remaps to USERLOG + noecho
#define DBTIMELOG 6 // top-level mongo db operation took longer than expected
#define PASSTHRUSERVERLOG 7 // remaps to SERVERLOG +  pass thru fmt as value
#define PASSTHRUUSERLOG 8 // remaps to USERLOG +  pass thru fmt as value

#define WARNSCRIPTLOG 9 // only during script compilation
#define BADSCRIPTLOG 10  // only during script compilation
#define BUGLOG 11 // top-level for bugs
#define FORCESTAYUSERLOG 401 // remaps to USERLOG after insuring no indent
#define FORCETABUSERLOG 501 // remaps to USERLOG after setting lastchar to 0 
#define STDDEBUGLOG 500 // top level IDE data
#define ECHOSTAYOUSERLOG 601 // remaps to USERLOG + local echo + after setting lastchar to \n (must end mod 100 to 1)

extern bool userEncrypt;
extern bool ltmEncrypt;
extern bool echo;
extern bool showDepth;
extern bool oob;
extern bool detailpattern;
extern bool silent;
extern uint64 logCount;
extern char* testOutput;
#define DebugPrint(...) Log(STDDEBUGLOG, __VA_ARGS__)
#define ReportBug(...) { Log(BUGLOG, __VA_ARGS__); if (server && serverLog) Log(SERVERLOG, __VA_ARGS__); if (!server && userLog) Log(USERLOG, __VA_ARGS__); Bug();  }
extern char logFilename[MAX_WORD_SIZE];
extern bool logUpdated;
extern unsigned int logsize;
extern unsigned int outputsize;
extern char serverLogfileName[1000];
extern char dbTimeLogfileName[1000];
extern int userLog;
extern int serverLog;
extern int holdServerLog;
extern int holdUserLog;
extern int bugLog;
extern bool serverPreLog;
extern bool serverctrlz;
extern char syslogstr[];

unsigned int Log(unsigned int spot,const char * fmt, ...);
CALLFRAME* ChangeDepth(int value,const char* where,bool nostackCutboack = false,char* code = NULL);
void BugBacktrace(FILE* out);
void Bug();

#define TIMESUMMARY_COUNT_OFFSET 32
#define TIMESUMMARY_TIME 0xFFFF
void TrackTime(char* name, int elapsed);
void TrackTime(WORDP D, int elapsed);
void Prelog(char* user, char* usee, char* incoming);
void LogChat(uint64 starttime, char* user, char* bot, char* IP, int turn, char* input, char* output, uint64 qtime);

// HOOKS
typedef void (*HOOKPTR)(void);

typedef void (*PerformChatArgumentsHOOKFN)(char*& user, char*& usee, char*& incoming);
typedef void (*SignalHandlerHOOKFN)(int signalcode, char*& msg);
typedef char* (*TokenizeWordHOOKFN)(char* ptr, char** words, int count);
typedef bool (*IsValidTokenWordHOOKFN)(char* token);
typedef char* (*SpellCheckWordHOOKFN)(char* word, int i);
#ifndef DISCARDMONGO
typedef void (*MongoQueryParamsHOOKFN)(bson_t *query);
typedef void (*MongoUpsertKeyValuesHOOKFN)(bson_t *doc);
typedef void (*MongoGotDocumentHOOKFN)(bson_t *doc);
#endif

typedef struct HookInfo
{
    const char* name;           // hook name
    HOOKPTR fn;                 // optional function to use to run it
    const char* comment;        // what to say about it
} HookInfo;

extern HookInfo hookSet[];
char* Myfgets(char* readBuffer,  int size, FILE* in);
HOOKPTR FindHookFunction(char* hookName);
void RegisterHookFunction(char* hookName, HOOKPTR fn);

// RANDOM NUMBERS

#define MAXRAND 256

extern unsigned int randIndex,oldRandIndex;

unsigned int random(unsigned int range);
uint64 Hashit(unsigned char * data, int len,bool & hasUpperCharacters, bool & hasUTF8Characters);

#endif

#ifdef FNVHASH
    // FNV1a hash - seems to have fewer collisions with UTF-8 emoji characters
    #define FNV_PRIME 0x00000100000001b3
    #define HASHSEED  0xcbf29ce484222325
    #define HASHFN(crc, c) ((crc ^ c) * FNV_PRIME)
#else
    // DJB2 hash
    #define HASHSEED 5381
    #define HASHFN(crc, c) (((crc << 5) + crc) + c)
#endif


#ifdef LINUX
	void setSignalHandlers ();
	void signalHandler( int signalcode );
#endif
