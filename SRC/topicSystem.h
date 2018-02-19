#ifndef _TOPICSYSTEMH
#define _TOPICSYSTEMH

#ifdef INFORMATION
Copyright (C)2011-2018 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif

#define LAYER_KERNEL -1
#define LAYER_0 0
#define LAYER_1 1
#define LAYER_BOOT 2
#define LAYER_USER 3

#define MAX_LABEL_SIZE 100

#define NO_REJOINDER -1   
#define BLOCKED_REJOINDER -2 

//   kinds of top level rules
#define QUESTION '?'
#define STATEMENT 's'
#define STATEMENT_QUESTION 'u'
#define RANDOM_GAMBIT   'r' 
#define GAMBIT   't'

#define ENDUNIT '`'				// marks end of a rule in compiled script

#define DUPLICATETOPICSEPARATOR '~'

#define USED_CODES 75			// Encode/Decode is base 75
#define ENDUNITTEXT "`000 "		// script compile rule closing sequence
#define JUMP_OFFSET 4			//   immediately after any ` are 3 characters of jump info, then either  "space letter :" or "space null" or "space x" if coming from 000
#define MAX_JUMP_OFFSET (USED_CODES*USED_CODES*USED_CODES) 

#define MAX_REFERENCE_TOPICS 1000	// how many topics a sentence can refer to at once

#define SAVEOLDCONTEXT() 	int oldRuleID = currentRuleID; int oldTopic = currentTopicID; char* oldRule = currentRule; int oldRuleTopic = currentRuleTopic;

#define RESTOREOLDCONTEXT() currentRuleID = oldRuleID; currentTopicID = oldTopic; currentRule = oldRule; currentRuleTopic = oldRuleTopic;

// decompose a currentRuleID into its pieces
#define TOPLEVELID(x) ((unsigned int) (x & 0x0000ffff))
#define REJOINDERID(x) ( ((unsigned int)x) >> 16)
#define MAKE_REJOINDERID(x) (x << 16)

// decompose RULEMAX info into gambits and top level rules
#define MAKE_GAMBIT_MAX(x) (x << 16)
#define GAMBIT_MAX(x) (x >> 16)
#define RULE_MAX(x) (x & 0x0000ffff)

// used by FindNextRule
#define NEXTRULE -1				// any responder,gambit,rejoinder
#define NEXTTOPLEVEL 2			// only responders and gambits

#define NOMORERULES 0x0fffffff		// finished walking a rule index map

#define MAX_TOPIC_STACK 50
extern int currentBeforeLayer;
extern bool stats;
extern unsigned int ruleCount;
extern char timeStamp[NUMBER_OF_LAYERS][20];
extern char compileVersion[NUMBER_OF_LAYERS][20];
extern char buildStamp[NUMBER_OF_LAYERS][150];
extern char* howTopic;
extern bool ruleErased;
extern bool hypotheticalMatch;

extern unsigned int duplicateCount;
extern unsigned int xrefCount;
extern bool norejoinder;
extern int currentTopicID;
extern char* currentRule;	
extern int currentRuleID;
extern int currentReuseID;
extern int currentReuseTopic;
extern int currentRuleTopic;
extern bool shared;
extern bool loading;
extern int outputRejoinderRuleID;
extern int outputRejoinderTopic;
extern int inputRejoinderRuleID;
extern int inputRejoinderTopic;
extern int sampleTopic;
extern int sampleRule;

typedef struct topicBlock
{
	char* topicName;
	char* topicRestriction;
	char* topicScript;
	char* topicSourceFileName;
	unsigned char* topicDebugRule;
	unsigned char* topicTimingRule;
	unsigned char* topicUsed;
	unsigned int* ruleOffset;
	unsigned int* gambitTag;
	unsigned int* responderTag;
	int topicFlags;
	int topicChecksum;
	int topicMaxRule;
	int topicDebug;
	int topicNoDebug;
	int topicTiming;
	int topicLastGambitted;
	int topicLastRejoindered;
	int topicLastRespondered;
	unsigned short int topicBytesRules;
} topicBlock;

topicBlock* TI(int topicid);

#define MAX_RECENT 100
extern unsigned short topicContext[MAX_RECENT + 1];
extern char labelContext[100][MAX_RECENT+ 1];
extern int inputContext[MAX_RECENT+ 1];
extern unsigned int contextIndex;

extern int numberOfTopicsInLayer[NUMBER_OF_LAYERS+1];
extern  topicBlock* topicBlockPtrs[NUMBER_OF_LAYERS+1];
extern int numberOfTopics;
extern int topicIndex,pendingTopicIndex,originalPendingTopicIndex;
extern int topicStack[MAX_TOPIC_STACK+1];
extern int pendingTopicList[MAX_TOPIC_STACK+1];
extern int originalPendingTopicList[MAX_TOPIC_STACK+1];
void SetSampleFile(int topic);
void ResetContext();
FunctionResult ProcessRuleOutput(char* rule, unsigned int id,char* buffer, bool refine = false);
FunctionResult TestRule(int responderID,char* ptr,char* buffer,bool refine=false);
FunctionResult PerformTopic(int active,char* buffer,char* rule = NULL,unsigned int id = 0);
bool Repeatable(char* rule);
char* GetTopicLocals(int topic);
void CleanOutput(char* word);
bool DifferentTopicContext(int depthadjust, int topicid);
FunctionResult LoadLayer(int layer, const char* name,unsigned int build);
void ResetTopicReply();
void SetRejoinder(char* rule);
void SetErase(bool force = false);
void UndoErase(char* ptr,int topic,int id);
void AddKeep(char* ptr);
int TopicInUse(int topic);
int PushTopic(int topic);
void PopTopic();
bool CheckTopicTrace();
bool CheckTopicTime();
FunctionResult DoOutput(char* buffer,char* rule, unsigned int id, bool refine = false);
unsigned int EstablishTopicTrace();
char* GetRuleIDFromText(char* ptr, int & id);
char* GetVerify(char* tag,int & topicid, int &id);//  ~topic.#.#=LABEL<~topic.#.#  is a maximally complete why
void UnwindUserLayerProtect();
void InitKeywords(const char* name,const char* layer,unsigned int build,bool mark=false,bool concept=true);
bool AreDebugMarksSet();
bool AreTimingMarksSet();

// encoding
void DummyEncode(char* &data);
void Encode(unsigned int val,char* &ptr,int size = false);
unsigned int Decode(char* data,int single = 0);
char* FullEncode(uint64 val,char* ptr);
uint64 FullDecode(char* data);

char* WriteUserContext(char* ptr,bool sharefile);
bool ReadUserContext();

// data accessors
int TopicUsedCount(int topic);
void GetActiveTopicName(char* buffer);
unsigned int FindTopicIDByName(char* request,bool exact = false);
char* FindNextRule(signed char level, char* at,int& id);
int GetTopicFlags(int topic);
char* GetRuleTag(int& topic,int& id,char* tag);
char* GetRule(int topic, int id);
char* GetLabelledRule(int& topic, char* word,char* arg2,bool &fulllabel, bool& crosstopic,int& id, int baseTopic);
int HasGambits(int topic);
char* FindNextLabel(int topic,char* label, char* ptr,int &id,bool alwaysAllowed);
char* RuleBefore(int topic,char* rule);
char* GetTopicFile(int topic);
void CreateFakeTopics(char* data);
void ReleaseFakeTopics();
bool TopLevelQuestion(char* word);
bool TopLevelStatement(char* word);
bool TopLevelGambit(char* word);
bool Rejoinder(char* word);
char* GetLabel(char* rule,char* label);
char* GetPattern(char* rule,char* label,char* pattern,int limit = MAX_WORD_SIZE); // returns start of output and modified pattern
char* GetOutputCopy(char* ptr); // returns copy of output only
bool TopLevelRule(char* word);
char* GetTopicName(int topic,bool actual = true);
char* GetTopicData(int topic);
void SetTopicData(int topic,char* data);
bool BlockedBotAccess(int topic);
void TraceSample(int topic, int ruleID, unsigned int how = STDTRACELOG);
bool SetRuleDisableMark(int topic, int id);
void ClearRuleDisableMark(int topic,int id);
bool UsableRule(int topic,int n);

void AddRepeatable(char* ptr);
void AddTopicFlag(int topic, unsigned int flag);
void RemoveTopicFlag(int topic, unsigned int flag);

void SetTopicDebugMark(int topic,unsigned int val);
void SetTopicTimingMark(int topic, unsigned int val);
void SetDebugRuleMark(int topic,unsigned int id);
void SetTimingRuleMark(int topic, unsigned int id);
char* ShowRule(char* rule,bool concise = false);
char* DisplayTopicFlags(int topic);
void FlushDisabled();

void AddContext(int topic, char* label);
unsigned int InContext(int topic, char* label);
void SetContext(bool val);

// bulk topic I/O
char* WriteUserTopics(char* ptr,bool sharefile);
bool ReadUserTopics();

// general topic system control
void LoadTopicSystem();
void ResetTopicSystem(bool safe);
void ResetTopics();
void ResetTopic(int id);

// Interesting topics
unsigned int GetPendingTopicUnchanged();
void AddPendingTopic(int topic);
bool RemovePendingTopic(int topic);
void ClearPendingTopics();
void PendingTopics(int set);
char* ShowPendingTopics();
bool IsCurrentTopic(int topic);

#endif
