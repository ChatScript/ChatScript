#ifndef MAINSYSTEMH
#define MAINSYSTEMH
#ifdef INFORMATION
Copyright (C)2011-2018 by Bruce Wilcox

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
    unsigned int responseInputIndex;                        // which input sentence does this stem from?
	int topic;										// topic of rule
	char id[24];											// .100.30
	char* response;						// answer sentences, 1 or more per input line
} RESPONSE;

#define SENTENCE_LIMIT 50 // how many sentence from user do we accept
#define MAX_RESPONSE_SENTENCES 20
#define MAX_SENTENCE_LENGTH 256 // room for +4 after content (keep a power of 4 for savesentence code)
#define REAL_SENTENCE_LIMIT 252 // stay under char size boundary and leave excess room
#define TIMEOUT_INSTANCE 1000000

#define PENDING_RESTART -1	// perform chat returns this flag on turn

typedef char* (*DEBUGAPI)(char* buffer);
typedef char* (*DEBUGLOOPAPI)(char* buffer,bool in);
typedef char* (*DEBUGVARAPI)(char* var, char* value);
extern DEBUGAPI debugMessage;
extern bool loadingUser;
extern DEBUGAPI debugInput; // user input from windows to CS
extern DEBUGAPI debugOutput; // CS output to windows
extern DEBUGAPI debugEndTurn; // about to save user file marker
extern DEBUGLOOPAPI debugCall;
extern unsigned int idetrace;
extern DEBUGVARAPI debugVar;
extern DEBUGVARAPI debugMark;
extern int outputlevel;
extern DEBUGAPI debugAction;
extern int forkcount;
extern char* outputCode[MAX_GLOBAL];
#define START_BIT 0x8000000000000000ULL	// used looping thru bit masks
#define INPUTMARKER '`'	// used to start and end ^input data

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
extern char treetaggerParams[200];
extern unsigned short int derivationIndex[256];
extern int derivationLength;
extern char* derivationSentence[MAX_SENTENCE_LENGTH];
extern bool docstats;
extern unsigned int docSentenceCount;
extern uint64 startTimeInfo;
extern unsigned int outputLength;
extern bool readingDocument;
extern int inputRetryRejoinderTopic;
extern int inputRetryRejoinderRuleID;
extern bool build0Requested;
extern bool build1Requested;
extern char traceuser[500];
extern bool callback;
extern char inputCopy[INPUT_BUFFER_SIZE]; 
extern unsigned char responseOrder[MAX_RESPONSE_SENTENCES+1];
extern RESPONSE responseData[MAX_RESPONSE_SENTENCES+1];
extern char language[40];
extern char livedata[500];
extern char languageFolder[500];
extern char systemFolder[500];
extern bool rebooting;
extern int responseIndex;
extern bool documentMode;
extern bool assignedLogin;
extern bool servertrace;
extern int outputchoice;
extern int traceUniversal;
extern char apikey[100];
extern char defaultbot[100];
extern unsigned int volleyCount;
extern FILE* sourceFile;
extern bool multiuser;
extern bool oobExists;
extern char hostname[100];
extern int argc;
extern char** argv;
extern  bool pendingRestart;
extern  bool pendingUserReset;
extern uint64 sourceStart;
extern unsigned int sourceTokens;
extern unsigned int sourceLines;
extern char* version;
extern unsigned int tokenCount;
extern unsigned int choiceCount;
extern int externalTagger;
extern bool redo;
extern bool commandLineCompile;
extern int inputCounter,totalCounter;
extern int inputSentenceCount;  
extern char* extraTopicData;
extern char postProcessing;
extern char rawSentenceCopy[INPUT_BUFFER_SIZE];
extern char authorizations[200];
extern FILE* userInitFile;
extern uint64 tokenControl;
extern unsigned int responseControl;
extern bool moreToCome,moreToComeQuestion;
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
extern int timerLimit;
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
extern char users[100];
extern char logs[100];
extern char topic[100];
extern char tmp[100];
extern char buildfiles[100];

// pending control
extern int systemReset;
extern bool quitting;
extern bool unusedRejoinder;
extern bool overrideAuthorization;
extern bool noReact;

// server
extern unsigned int port;
extern bool server;
#ifndef DISCARDSERVER
extern std::string interfaceKind;
#endif

// buffers
extern char ourMainInputBuffer[INPUT_BUFFER_SIZE];
extern char* mainInputBuffer;
extern char* ourMainOutputBuffer;
extern char* mainOutputBuffer;
extern char currentInput[INPUT_BUFFER_SIZE];
extern char revertBuffer[INPUT_BUFFER_SIZE];
extern char* readBuffer;
extern char* nextInput;

extern char activeTopic[200];

extern int always; 
extern uint64 callBackTime;
extern uint64 callBackDelay;
extern uint64 loopBackTime;
extern uint64 loopBackDelay;
extern uint64 alarmTime;
extern uint64 alarmDelay;
extern  char userPrefix[MAX_WORD_SIZE];			// label prefix for user input
extern char botPrefix[MAX_WORD_SIZE];			// label prefix for bot output

void Restart();
void ProcessInputFile();
bool ProcessInputDelays(char* buffer,bool hitkey);
char* SkipOOB(char* buffer);
void EmitOutput();

// startup
#ifdef DLL
extern "C" __declspec(dllexport) unsigned int InitSystem(int argc, char * argv[],char* unchangedPath = NULL,char* readonlyPath = NULL, char* writablePath = NULL, USERFILESYSTEM* userfiles = NULL,DEBUGAPI in = NULL, DEBUGAPI out = NULL);
#else
unsigned int InitSystem(int argc, char * argv[],char* unchangedPath = NULL,char* readonlyPath = NULL, char* writablePath = NULL, USERFILESYSTEM* userfiles = NULL,DEBUGAPI in = NULL, DEBUGAPI out = NULL);
#endif
int FindOOBEnd(int start);
void InitStandalone();
void CreateSystem();
void ReloadSystem();
#ifdef DLL
extern "C" __declspec(dllexport) void CloseSystem();
#else
void CloseSystem();
#endif
void NLPipeline(int trace);
void PartiallyCloseSystem();
int main(int argc, char * argv[]);
char* ReviseOutput(char* out, char* prefix);
void ProcessOOB(char* buffer);
void ComputeWhy(char* buffer, int n);
void FlipResponses();
void MoreToCome();
int CountWordsInBuckets(int& unused, unsigned int* depthcount, int limit);
// Input processing
void MainLoop();
void FinishVolley(char* input,char* output,char* summary,int limit = outputsize);
int ProcessInput(char* input);
FunctionResult DoSentence(char* prepassTopic,bool atlimit);

#ifdef DLL
extern "C" __declspec(dllexport) int PerformChat(char* user, char* usee, char* incoming,char* ip,char* output);
extern "C" __declspec(dllexport) int PerformChatGivenTopic(char* user, char* usee, char* incoming,char* ip,char* output,char* topic);
#else
int PerformChat(char* user, char* usee, char* incoming,char* ip,char* output);
int PerformChatGivenTopic(char* user, char* usee, char* incoming,char* ip,char* output,char* topic);
#endif

void ResetSentence();
void ResetToPreUser();
void PrepareSentence(char* input,bool mark = true,bool user=true, bool analyze = false, bool oobstart = false,bool atlimit = false);
bool PrepassSentence(char* presspassTopic);
FunctionResult Reply();
void OnceCode(const char* var,char* topic = NULL);
void AddBotUsed(const char* reply,unsigned int len);
void AddHumanUsed(const char* reply);
bool HasAlreadySaid(char* msg);
bool AddResponse(char* msg, unsigned int controls);

#endif
