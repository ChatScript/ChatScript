#ifndef MAINSYSTEMH
#define MAINSYSTEMH
#ifdef INFORMATION
Copyright (C)2011-2024 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif

#define ID_SIZE 500
#define OUTPUT_BUFFER_SIZE 40000

typedef struct RESPONSE
{
    char* response;						// answer sentences, 1 or more per input line
    char* patternchoice;
    unsigned int responseInputIndex;                        // which input sentence does this stem from?
	int topic;										// topic of rule
	char id[24];											// .100.30
} RESPONSE;

#define SENTENCES_LIMIT 50 // how many sentence from user do we accept
#define MAX_RESPONSE_SENTENCES 20
#define MAX_SENTENCE_LENGTH 256 // room for +4 after content (keep a power of 4 for savesentence code)
#define REAL_SENTENCE_WORD_LIMIT 252 // stay under char size boundary and leave excess room
#define TIMEOUT_INSTANCE 1000000

#define PENDING_RESTART -1	// perform chat returns this flag on turn
extern bool loadingUser;

typedef char* (*DEBUGAPI)(char* buffer);
typedef char* (*DEBUGLOOPAPI)(char* buffer,bool in);
typedef char* (*DEBUGVARAPI)(char* var, char* value);
extern DEBUGAPI debugMessage;
extern DEBUGAPI debugInput; // user input from windows to CS
extern DEBUGAPI debugOutput; // CS output to windows
extern DEBUGAPI debugEndTurn; // about to save user file marker
extern DEBUGLOOPAPI debugCall;
extern DEBUGVARAPI debugVar;
extern unsigned int parseLimit;
extern DEBUGVARAPI debugMark;
extern char myip[100];
extern const char* buildString;
extern uint64 timedeployed;
extern char repairinput[20];
extern char* repairat;
extern bool nophrases;
extern bool outputapicall;
extern unsigned long startLogDelay;
extern int cs_qsize;
extern char baseLanguage[50];
extern int pendingUserLimit;
extern  std::atomic<int> userQueue;
extern char  overflowMessage[400];
extern int overflowLen;
extern DEBUGAPI debugAction;
extern unsigned int idetrace;
extern bool dieonwritefail;
extern bool blockapitrace;
extern unsigned int timeLog;
extern int outputlevel;
extern char* releaseBotVar;
extern int hadAuthCode;
extern bool crashset;
extern bool crashBack;
extern bool restartBack;
extern int forkcount;
extern char jmeter[100];
extern int sentenceloopcount;
extern bool debugcommand;
extern bool restartfromdeath;
extern bool sentenceOverflow;
#define START_BIT 0x8000000000000000ULL	// used looping thru bit masks

extern char* originalUserInput; // entire input including oob
extern char* originalUserMessage; // user input component
// values of prepareMode
 enum PrepareMode { // how to treat input
	NO_MODE = 0,				// std processing of user inputs
	POS_MODE = 1,				// just pos tag the inputs
	PREPARE_MODE = 2,			// just prepare the inputs
	POSVERIFY_MODE = 4,
	POSTIME_MODE = 8,
	PENN_MODE = 16,
	REGRESS_MODE = 32,			// inputs are from a regression test
	TOKENIZE_MODE = 64,			// just prepare the inputs
 };

 enum ResetMode {
	 NO_RESET = 0,
	 MILD_RESET = 1,
	 FULL_RESET = 2
 };

 enum RegressionMode { 
	NO_REGRESSION = 0,
	NORMAL_REGRESSION = 1,
	REGRESS_REGRESSION = 2,
};

// values of echoSource
 enum EchoSource {
	NO_SOURCE_ECHO = 0,
	SOURCE_ECHO_USER = 1,
	SOURCE_ECHO_LOG = 2,
};
#define MAX_TRACED_FUNCTIONS 50
#define TESTPATTERN_TRACE_SIZE 200000
 extern char* tracebuffer;
 extern int  configLinesLength;
 extern char* configFile;	// can set config params
 extern  char* configFile2;	// can set config params
 extern char* configLines[MAX_WORD_SIZE];
 extern char treetaggerParams[200];
extern unsigned short int derivationIndex[MAX_SENTENCE_LENGTH];
extern unsigned int derivationLength;
extern bool client;
extern char* derivationSentence[MAX_SENTENCE_LENGTH];
extern char derivationSeparator[MAX_SENTENCE_LENGTH];
extern bool docstats;
extern unsigned int docSentenceCount;
extern unsigned int outputLength;
extern bool readingDocument;
extern int inputRetryRejoinderTopic;
extern bool newuser;
extern int inputRetryRejoinderRuleID;
extern bool build0Requested;
extern bool jabugpatch;
extern bool build1Requested;
extern char traceuser[500];
extern bool callback;
extern int sentenceLimit;
extern char rootdir[MAX_WORD_SIZE];
extern char incomingDir[MAX_WORD_SIZE];
extern int volleyFile;
extern  int forcedRandom;
extern unsigned char responseOrder[MAX_RESPONSE_SENTENCES+1];
extern RESPONSE responseData[MAX_RESPONSE_SENTENCES+1];
extern bool logline;
extern bool integrationTest;
extern char livedataFolder[500];
extern char languageFolder[500];
extern char systemFolder[500];
extern bool rebooting;
extern bool fastload;
extern int responseIndex;
extern bool trustpos;
extern bool documentMode;
extern bool servertrace;
extern unsigned int inputLimit;
extern unsigned int fullInputLimit;
extern char serverlogauthcode[30];
extern int outputchoice;
extern int traceUniversal;
extern char apikey[100];
extern char defaultbot[100];
extern char buildflags[100];
extern unsigned int volleyCount;
extern FILE* sourceFile;
extern bool multiuser;
extern bool oobExists;
extern char hostname[100];
extern unsigned int currentBuild;
extern unsigned int argc;
extern uint64 startNLTime;
extern char** argv;
extern  bool pendingRestart;
extern  bool pendingUserReset;
extern uint64 sourceStart;
extern unsigned int sourceTokens;
extern unsigned int sourceLines;
extern char* version;
extern FILE* tsvFile;
extern unsigned int tsvIndex;
extern unsigned int tsvMessageField;
extern unsigned int tsvLoginField;
extern unsigned int tsvSpeakerField;
extern unsigned int tsvCount;
extern unsigned int tsvSkip;
extern char tsvpriorlogin[MAX_WORD_SIZE]; 
extern char speaker[100];
extern unsigned int tokenCount;
extern unsigned int choiceCount;
extern int externalTagger;
extern bool redo;
extern bool commandLineCompile;
extern int inputCounter,totalCounter;
extern int inputSentenceCount;  
extern char* extraTopicData;
extern uint64 preparationtime;
extern char newusername[100];
extern char  erasename[100];
extern uint64 replytime;
extern bool postProcessing;
extern char* rawSentenceCopy;
extern char authorizations[200];
extern FILE* userInitFile;
extern uint64 tokenControl;
extern unsigned int responseControl;
extern unsigned int trace;
extern unsigned int timing;
extern int regression;
extern char debugEntry[1000];
extern EchoSource echoSource;
extern bool all;
extern PrepareMode prepareMode;
extern PrepareMode tmpPrepareMode;
extern clock_t startSystem;
extern char oktest[MAX_WORD_SIZE];
extern uint64 timerLimit;
extern int timerCheckRate;
extern int timerCheckInstance;
extern uint64 volleyStartTime;
extern bool autonumber;
extern bool showWhy;
extern bool noboot;
extern bool showTopic;
extern bool showInput;
extern bool showReject;
extern bool showTopics;
extern bool shortPos;
extern char usersfolder[100];
extern char logsfolder[100];
extern char topicfolder[100];
extern char tmpfolder[100];
extern char buildfiles[100];

// pending control
extern ResetMode buildReset;
extern bool quitting;
extern bool unusedRejoinder;
extern bool scriptOverrideAuthorization;
extern bool noReact;

// server
extern unsigned int port;
extern bool server;
#ifndef DISCARDSERVER
extern std::string interfaceKind;
#endif

// buffers
extern char* ourMainInputBuffer;
extern char* ourMainOutputBuffer;
extern char* revertBuffer;
extern char* readBuffer;

extern char activeTopic[200];
extern int db;
extern int gzip;
extern int always; 
extern uint64 callBackTime;
extern uint64 callBackDelay;
extern uint64 loopBackTime;
extern uint64 loopBackDelay;
extern uint64 alarmTime;
extern uint64 alarmDelay;
extern  char userPrefix[MAX_WORD_SIZE];			// label prefix for user input
extern char botPrefix[MAX_WORD_SIZE];			// label prefix for bot output
extern bool errorOnMissingElse;


void Restart();
void ProcessInputFile();
bool ProcessInputDelays(char* buffer,bool hitkey);
void EmitOutput();

// startup
#ifdef DLL
#ifdef __linux__
extern "C" unsigned int InitSystem(int argc, char* argv[], char* unchangedPath = NULL, char* readonlyPath = NULL, char* writablePath = NULL, USERFILESYSTEM* userfiles = NULL, DEBUGAPI in = NULL, DEBUGAPI out = NULL);
#else
extern "C" __declspec(dllexport) unsigned int InitSystem(int argc, char* argv[], char* unchangedPath = NULL, char* readonlyPath = NULL, char* writablePath = NULL, USERFILESYSTEM* userfiles = NULL, DEBUGAPI in = NULL, DEBUGAPI out = NULL);
#endif
#else
unsigned int InitSystem(int argc, char* argv[], char* unchangedPath = NULL, char* readonlyPath = NULL, char* writablePath = NULL, USERFILESYSTEM* userfiles = NULL, DEBUGAPI in = NULL, DEBUGAPI out = NULL);
#endif

int FindOOBEnd(unsigned int start);
void InitStandalone();
void CreateSystem();
void LoadSystem(unsigned int limit,unsigned int argc, char** argv);
void Rebegin(unsigned int buildId,unsigned int argc, char** argv);

#ifdef DLL
#ifdef __linux__
extern "C" void CloseSystem();
#else
extern "C" __declspec(dllexport) void CloseSystem();
#endif
#else
void CloseSystem();
#endif

void NLPipeline(int trace);
void EmergencyResetUser();
char* DoOutputAdjustments(char* msg, unsigned int control,char* &buffer,char* limit);
void PartiallyCloseSystem(bool alien = false);
int main(int argc, char * argv[]);
char* ReviseOutput(char* out, char* prefix);
void ProcessOOB(char* buffer);
void ComputeWhy(char* buffer, int n);
void FlipResponses();
unsigned int CountWordsInBuckets(unsigned int& unused, unsigned int* depthcount, int limit);
// Input processing
void MainLoop();
void FinishVolley(char* output,char* summary,int limit = outputsize);
int ProcessInput();
FunctionResult DoOneSentence(char* incoming,char* prepassTopic,bool atlimit);
void ExecuteVolleyFile(FILE* sourceFile);

#ifdef DLL
#ifdef __linux__
extern "C" int PerformChat(char* user, char* usee, char* incoming, char* ip, char* output);
extern "C" int PerformChatGivenTopic(char* user, char* usee, char* incoming, char* ip, char* output, char* topic);
#else
extern "C" __declspec(dllexport) int PerformChat(char* user, char* usee, char* incoming, char* ip, char* output);
extern "C" __declspec(dllexport) int PerformChatGivenTopic(char* user, char* usee, char* incoming, char* ip, char* output, char* topic);
#endif
#else
int PerformChat(char* user, char* usee, char* incoming, char* ip, char* output);
int PerformChatGivenTopic(char* user, char* usee, char* incoming, char* ip, char* output, char* topic);
#endif

void ResetSentence();
void ResetToPreUser(bool saveBuffers = false);
void PrepareSentence(char* input,bool mark = true,bool user=true, bool analyze = false, bool oobstart = false,bool atlimit = false);
bool PrepassSentence(char* presspassTopic);
FunctionResult Reply();
FunctionResult OnceCode(const char* var,char* topic = NULL);
void AddBotUsed(const char* reply,unsigned int len);
void AddHumanUsed(const char* reply);
bool HasAlreadySaid(char* msg);
bool AddResponse(char* msg, unsigned int controls);

void HandlePermanentBuffers(bool init); // for unit testing
#endif
