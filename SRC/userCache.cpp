#include "common.h"

#define NO_CACHEID -1

static int userTopicCacheHead = NO_CACHEID;		// our marker for last allocated cache, used to find next free one
static USERCACHE* userTopicCacheIndex = NULL;	// data ring of the user topic caches + timestamp/volley info
char* cacheBase = NULL;					// start of contiguous cache block of caches
char* cacheDataEnd = NULL;              // end of cached data
char* cacheFree = NULL;                 // next free in cache
char* fileBuffer = NULL;                // stack pointer
static int currentCache = NO_CACHEID;	// the current user file buffer
unsigned int userCacheCount = 1;		// holds 1 user by default
unsigned int userCacheSize = DEFAULT_USER_CACHE; // size of user file buffer (our largest buffers)

unsigned int fileCacheCount = 0;        // number of read only files cached
unsigned int fileCacheSize = 0;         // amount of cache given over to files
static USERCACHE* fileCacheIndex = NULL;    // index of recent cached files
USERCACHE* fileCacheIndexFree = NULL;    // index of recent cached files
unsigned int fileCacheTargetSize = 0;   // target size for recency cache
static int fileCacheRecencyHead = NO_CACHEID;
static int fileCacheFrequencyHead = NO_CACHEID;
static int fileCacheRecencyHistoryHead = NO_CACHEID;
static int fileCacheFrequencyHistoryHead = NO_CACHEID;
static unsigned int fileCacheRecencySize = 0;
static unsigned int fileCacheFrequencySize = 0;
static unsigned int fileCacheRecencyHistorySize = 0;
static unsigned int fileCacheFrequencyHistorySize = 0;

int volleyLimit =  -1;					// default save user records to file every n volley (use default 0 if this value is unchanged by user)
unsigned int userTopicStoreSize = 0;
unsigned int userTableSize = 0; // memory used which we will display

void InitUserCache()
{
    if (cacheBase) return;    // no need to reallocate on reload

    // Cache divided as:
    // | cached topic + file data | user topic index | imported file indexes |
    // all files read into stack, and if needed to be cached then copied into free slot in cache
    
    userTopicStoreSize = (userCacheCount * (userCacheSize + 2)) + fileCacheSize; //  minimum file cache spot
    userTopicStoreSize /= 64;
    userTopicStoreSize = (userTopicStoreSize * 64) + 64;

    userTableSize = (userCacheCount * sizeof(USERCACHE)) + (fileCacheCount * 2 * sizeof(USERCACHE));
    userTableSize /= 64;
    userTableSize = (userTableSize * 64) + 64; // 64 bit align both ends

    cacheBase = (char*) malloc( userTopicStoreSize + userTableSize );
    if (!cacheBase)
    {
        (*printer)((char*)"Out of memory space for user cache %d %d %d\r\n",userTopicStoreSize,userTableSize,MAX_DICTIONARY);
        ReportBug((char*)"FATAL: Cannot allocate memory space for user cache %ld\r\n",(long int) userTopicStoreSize);
    }
    
    // start of actual data cache
    cacheDataEnd = cacheBase + userTopicStoreSize;
    *cacheBase = 0;
    cacheFree = cacheBase + 1;

    // initialize the user topic index - simple LRU circular list
    userTopicCacheIndex = (USERCACHE *)cacheDataEnd;
    if (userCacheCount > 0)
    {
        USERCACHE* C = userTopicCacheIndex;
        for (unsigned int i = 0; i < userCacheCount; ++i)
        {
            C->prior = (i == 0 ? userCacheCount : i) - 1; // before ptr
            C->next = ( i == (userCacheCount-1) ? 0 : i + 1); // after ptr
            C->timestamp = 0;
            C->hash = 0;
            C->buffer = 0;
            ++C;
        }
        userTopicCacheHead = 0;
    }
    
    // initialize data file index - CAR: Clock with Adaptive Replacement
    // https://www.usenix.org/legacy/publications/library/proceedings/fast04/tech/full_papers/bansal/bansal.pdf
    // need to split the cache into 2 halfs, recent and frequent
    if (fileCacheCount > 1)
    {
        fileCacheIndex = userTopicCacheIndex + userCacheCount;
        fileCacheIndexFree = fileCacheIndex;
    }
}

void CloseUserCache()
{
    if (cacheBase) free(cacheBase);
    cacheBase = NULL;
    cacheFree = NULL;
}

static bool IsUserTopicFile(char* name)
{
    char word[MAX_WORD_SIZE];
    sprintf(word,(char*)"%s/%stopic_", usersfolder, GetUserPath(loginID));
    return (!strnicmp(name, word, strlen(word)));
}

static unsigned int FilenameHash(char* filename)
{
    size_t len = strlen(filename);
    if (len == 0) return 0;

    return Hashit((unsigned char*)filename, len, hasUpperCharacters, hasUTF8Characters, hasSeparatorCharacters);
}

static void BufferFilename(char* buffer, char* filename)
{
    *filename = 0;
    char* at = strchr(buffer,'\r'); // separate off the name from the rest of the data
    if (at)
    {
        *at = 0;
        strcpy(filename, buffer);
        *at = '\r';
    }
}

static char* GetCacheBuffer(USERCACHE* C)
{
    return (cacheBase + C->buffer);
}

static char* GetUserTopicCacheBuffer(int which)
{
    return (which < 0) ? NULL : (GetCacheBuffer(CACHEITEM(userTopicCacheIndex, which)));
}

static char* GetFileCacheBuffer(int which)
{
    return (which < 0) ? NULL : (GetCacheBuffer(CACHEITEM(fileCacheIndex, which)));
}

static void WriteBuffer(char* filename, char* ptr, size_t size)
{
    if (!*ptr) return;    // nothing to write out

    if (size == 0) size = strlen(ptr);// request to compute size
    uint64 start_time = (timing & TIME_USERCACHE) ? ElapsedMilliseconds() : 0;

    FILE* out = userFileSystem.userCreate(filename); // wb binary file (if external as db write should not fail)
    if (!out) // see if we can create the directory (assuming its missing)
    {
        char call[MAX_WORD_SIZE];
        sprintf(call,(char*)"mkdir %s",usersfolder);
        MakeDirectory(usersfolder);
        out = userFileSystem.userCreate(filename);

        if (!out)
        {
            ReportBug((char*)"cannot open user state file %s to write\r\n", ptr);
            if (dieonwritefail) myexit("Failed to open user state file to write");
            return;
        }
    }

#ifdef LOCKUSERFILE
#ifdef LINUX
    if (server && filesystemOverride == NORMALFILES)
    {
        int fd = fileno(out);
        if (fd < 0)
        {
            FClose(out);
            return;
        }

        struct flock fl;
        fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
        fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
        fl.l_start  = 0;    /* Offset from l_whence         */
        fl.l_len    = 0;    /* length, 0 = to EOF           */
        fl.l_pid    = getpid(); /* our PID                      */
        if (fcntl(fd, F_SETLKW, &fl) < 0)
        {
             userFileSystem.userClose(out);
             return;
        }
    }
#endif
#endif

    EncryptableFileWrite(ptr,1,size,out,userEncrypt,"USER"); // user topic file write
    userFileSystem.userClose(out);
    if (timing & TIME_USERCACHE) 
    {
        int diff = (int)(ElapsedMilliseconds() - start_time);
        if (timing & TIME_ALWAYS || diff > 0) Log((server) ? SERVERLOG : STDTIMELOG, (char*)"Write user cache file %s time: %d ms\r\n", filename, diff);
    }
}

static void WriteCache(unsigned int which, size_t size)
{
 	char* ptr = GetUserTopicCacheBuffer(which);
	if (!*ptr) return;	// nothing to write out

    USERCACHE* C = CACHEITEM(userTopicCacheIndex, which);
    unsigned int volleys = VOLLEYCOUNT(C);
	if (!volleys) return; // unchanged from prior write out

	if (size == 0) size = strlen(ptr);// request to compute size
	char filename[SMALL_WORD_SIZE];
    BufferFilename(ptr, filename);
    WriteBuffer(filename, ptr, size);

    C->timestamp &= MAX_CACHE_DURATION;    // clear volley count since last written but keep time info

	if (trace & TRACE_USERCACHE) Log((server) ? SERVERLOG : USERLOG,(char*)"write cache (%d)  %s \r\n",which, filename);
}

void FlushCache() // writes out the cache but does not invalidate it
{
    if (!cacheBase) return; // never inited
    int start = userTopicCacheHead;
    while (userCacheCount) // kill all active user topic caches
    {
        WriteCache(start, 0);
        start = CACHEITEM(userTopicCacheIndex, start)->next;
        if (start == userTopicCacheHead) break;    // been round in a circle
    }
}

static void MoveBuffers(USERCACHE* cache, int head, unsigned int after, unsigned int diff)
{
    if (head < 0) return;
    int start = head;
    while (ALWAYS)
    {
        USERCACHE* C = CACHEITEM(cache, start);
        if (C->hash > 0 && C->buffer > after) C->buffer -= diff;
        start = C->next;
        if (start == head) break;
    }
}

static void ChangeBlockSize(USERCACHE* cache, int which, unsigned int newSize)
{
    USERCACHE* C = CACHEITEM(cache, which);
    if (C->hash == 0) return;

    char* ptr = GetCacheBuffer(C);
    if (!ptr) return;

    unsigned int curBlock = C->buffer;
    unsigned int curSize = strlen(ptr);
    unsigned int diff = (newSize == 0 ? curSize : curSize - newSize + 1);

    // need to move all the other buffer references from all the other indexes
    MoveBuffers(userTopicCacheIndex, userTopicCacheHead, curBlock, diff);
    MoveBuffers(fileCacheIndex, fileCacheRecencyHead, curBlock, diff);
    MoveBuffers(fileCacheIndex, fileCacheFrequencyHead, curBlock, diff);
    
    char* dst = ptr + newSize;
    char* src = ptr + curSize;
    size_t len = cacheFree - src;
    cacheFree -= diff;
    if (newSize > 0)
    {
        // need string length terminator
        ++dst;
        cacheFree += 2;
    }
    else
    {
        // clear cache so it can be reused
        *ptr = 0;
        C->buffer = 0;
    }

    memmove(dst, src, len);
}

static void EvictFromUserTopicCache(int which, bool write) // remove an item from the cache
{
    char* ptr = GetUserTopicCacheBuffer(which);
    if (ptr && CACHEITEM(userTopicCacheIndex, which)->hash > 0)
    {
        if (write) WriteCache(which, 0);
        ChangeBlockSize(userTopicCacheIndex, which, 0);
        CACHEITEM(userTopicCacheIndex, which)->hash = 0;
        if (trace & TRACE_USERCACHE) Log((server) ? SERVERLOG : USERLOG,(char*)"evicted user topic cache (%d)\r\n",which);
    }
}

static USERCACHE* AddFileCacheEntry(USERCACHE* C, int &head)
{
    if (!C) C = fileCacheIndexFree++;
    unsigned int index = CACHEINDEX(fileCacheIndex, C);

    if (head >= 0)
    {
        USERCACHE* CH = CACHEITEM(fileCacheIndex, head);
        unsigned int last = CH->prior;
        USERCACHE* CL = CACHEITEM(fileCacheIndex, last);
        
        CH->prior = index;
        CL->next = index;
        
        C->prior = last;
        C->next = head;
    }
    else
    {
        head = index;
        C->prior = head;
        C->next = head;
    }

    return C;
}

// shuffles an existing index to the head of an existing list
static void MoveIndexToHead(USERCACHE* cache, USERCACHE* C, unsigned int head)
{
    unsigned int index = CACHEINDEX(cache, C);
    if (index == head) return;
    unsigned int prior = C->prior;
    unsigned int next = C->next;
    
    // decouple from where it is
    CACHEITEM(cache, prior)->next = next;
    CACHEITEM(cache, next)->prior = prior;

    // now insert in front
    USERCACHE* H = CACHEITEM(cache, head);
    prior = H->prior;
    H->prior = index;

    CACHEITEM(cache, prior)->next = index;
    C->prior = prior;
    C->next = head;
}

// add an index before a target point
// the index might not be linked in, or target might not exist
static void MoveFileBeforeIndex(USERCACHE* C, int target)
{
    int sourceIndex = CACHEINDEX(fileCacheIndex, C);
    if (sourceIndex == target || C->next == target) return;  // don't move

    // detach head from the source cache
    USERCACHE* SN = CACHEITEM(fileCacheIndex, C->next);
    USERCACHE* SP = CACHEITEM(fileCacheIndex, C->prior);
    if (C->next != sourceIndex)
    {
        int newSourceHead = C->next;
        SN->prior = C->prior;
        SP->next = newSourceHead;
    }

    // and add between tail and head of target
    if (target < 0)
    {
        // target doesn't exist
        C->prior = sourceIndex;
        C->next = sourceIndex;
    }
    else
    {
        USERCACHE* TH = CACHEITEM(fileCacheIndex, target);
        USERCACHE* TP = CACHEITEM(fileCacheIndex, TH->prior);
        TP->next = sourceIndex;
        C->prior = TH->prior;
        C->next = target;
        TH->prior = sourceIndex;
    }
}

static void MoveFileCache(int &sourceHead, int &targetHead, bool makeHead, bool clearBuffer)
{
    USERCACHE* C = CACHEITEM(fileCacheIndex, sourceHead);
    int newSourceHead = (C->next == sourceHead ? NO_CACHEID : C->next);

    // might need to remove the actual cached document
    if (clearBuffer) ChangeBlockSize(fileCacheIndex, sourceHead, 0);

    MoveFileBeforeIndex(C, targetHead);

    if (targetHead < 0 || makeHead) targetHead = sourceHead;
    sourceHead = newSourceHead;
}

static void ReplaceFileCache()
{
    while(ALWAYS)
    {
        if (fileCacheRecencySize > fmax(1, fileCacheTargetSize)) {
            // size of the recent cache is greater than what we desire
            // so need to remove one of its pages
            USERCACHE* C = CACHEITEM(fileCacheIndex, fileCacheRecencyHead);
            if (VOLLEYCOUNT(C) == 0)
            {
                // demote an "inactive" recent page to the MRU in the recent history
                MoveFileCache(fileCacheRecencyHead, fileCacheRecencyHistoryHead, true, true);
                --fileCacheRecencySize;
                ++fileCacheRecencyHistorySize;
                break;
            }
            else
            {
                // move an "active" recent page to the end of the frequent cache
                C->timestamp = SETTIMESTAMP(0);
                MoveFileCache(fileCacheRecencyHead, fileCacheFrequencyHead, false, false);
                --fileCacheRecencySize;
                ++fileCacheFrequencySize;
            }
        }
        else
        {
            // recent cache is of a desirable size
            // so need to remove one of the frequent cached pages instead
            USERCACHE* C = CACHEITEM(fileCacheIndex, fileCacheFrequencyHead);
            if (VOLLEYCOUNT(C) == 0)
            {
                // demote an "inactive" frequent page to the MRU of the frequent history
                MoveFileCache(fileCacheFrequencyHead, fileCacheFrequencyHistoryHead, true, true);
                --fileCacheFrequencySize;
                ++fileCacheFrequencyHistorySize;
                break;
            }
            else
            {
                // cycle an "active" page to the end of the frequent queue
                // and make it "inactive"
                C->timestamp = SETTIMESTAMP(0);
                if (fileCacheFrequencySize > 2) MoveFileBeforeIndex(C, fileCacheFrequencyHead);
                fileCacheFrequencyHead = C->next;
            }
        }
    }
}

static void RestoreHistoryFileCache(USERCACHE* C, int &sourceHead, int &targetHead)
{
    // need to tweak the head of the source if this item is at the head
    if (CACHEINDEX(fileCacheIndex, C) == sourceHead) sourceHead = (C->next == sourceHead ? NO_CACHEID : C->next);

    MoveFileBeforeIndex(C, targetHead);
    
    // set the page reference to zero
    C->timestamp = SETTIMESTAMP(0);
}

static USERCACHE* RemoveTail(USERCACHE* cache, unsigned int head)
{
    if (head < 0) return NULL;
    USERCACHE* CH = CACHEITEM(cache, head);
    USERCACHE* CT = CACHEITEM(cache, CH->prior);
    
    CH->prior = CT->prior;
    CACHEITEM(cache, CT->prior)->next = head;
    
    CT->next = NO_CACHEID;
    CT->prior = NO_CACHEID;
    CT->hash = 0;
    
    return CT;
}

static void UpdateCacheEntry(USERCACHE* C, char* filename, char* buffer, size_t size, unsigned int volley)
{
    C->hash = FilenameHash(filename);
    C->buffer = (unsigned int)(cacheFree - cacheBase);
    C->timestamp = SETTIMESTAMP(volley); // how many cpu seconds since start of program launch
    cacheFree += size + 1;
    
    char* ptr = GetCacheBuffer(C);
    *ptr = 0;
    
    memcpy(ptr, buffer, size);
    ptr[size] = 0;
}

static void AddToUserTopicCache(char* filename, char* buffer, size_t size, unsigned int volley) // allocate backwards from current, so in use is always NEXT from current
{
    if (!userCacheCount || size == 0) return;

    // remove the current version from cache
    if (currentCache != NO_CACHEID) EvictFromUserTopicCache(currentCache, false);

    // need to find a cache that's free else flush oldest one
    unsigned int last = userTopicCacheHead;
    char* ptr;
    
    unsigned int blockSize = size + 1;
    
    USERCACHE* C = CACHEITEM(userTopicCacheIndex, last);
    do
    {
        last = C->prior;
        C = CACHEITEM(userTopicCacheIndex, last);
        ptr = GetUserTopicCacheBuffer(last);
        if (C->hash > 0) // this is in use
        {
            if (IsUserTopicFile(ptr)) EvictFromUserTopicCache(last, true);
        }
    }
    while ((unsigned int)(cacheDataEnd - cacheFree) <= blockSize);
    
    UpdateCacheEntry(C, filename, buffer, size, volley);
    currentCache = userTopicCacheHead = last; // just rotate the ring - has more than one block

    if (trace & TRACE_USERCACHE) Log((server) ? SERVERLOG : USERLOG,(char*)"added free cache (%d)\r\n",last);
}

void FreeAllUserCaches()
{
	if (!cacheBase || userTopicCacheHead == NO_CACHEID) return; // never inited
	unsigned int start = userTopicCacheHead;
	while (userCacheCount) // kill all active caches
	{
        EvictFromUserTopicCache(start, true);
		start = CACHEITEM(userTopicCacheIndex, start)->next;
		if (start == (unsigned int)userTopicCacheHead) break;	// been round in a circle
	}
	currentCache = NO_CACHEID;
}

static USERCACHE* FindCacheEntry(USERCACHE* cache, int head, char* filename, bool move)
{
    if (head < 0) return NULL;
    
    unsigned int fullhash = FilenameHash(filename);

    int start = head;
    while (ALWAYS)
    {
        USERCACHE* C = CACHEITEM(cache, start);
        if (C->hash == fullhash)
        {
            // make him FIRST on the list
            if (move) MoveIndexToHead(cache, C, head);
            return C;
        }
        start = C->next;
        if (start == head) break;    // reached start again
    }
    return NULL;
}

static char* FindCachedUserTopic(char* filename, bool recordHit)
{
    // already in cache?
    size_t len = strlen(filename);
    if (len == 0 || !userCacheCount) return NULL;
    
    USERCACHE* C = FindCacheEntry(userTopicCacheIndex, userTopicCacheHead, filename, true);
    if (C)
    {
        if (recordHit)
        {
            unsigned int volleys = VOLLEYCOUNT(C) + 1;
            if (volleys > MAX_VOLLEYCOUNT) volleys = MAX_VOLLEYCOUNT;

            unsigned int duration = CPU_TICKS - startSystem; // how many seconds since start of program launch

            // dont want to overflow 24bit second store... so reset system start if needed
            if (duration > MAX_CACHE_DURATION)
            {
                startSystem = CPU_TICKS;
                duration = 0; // how many seconds since start of program launch
                // allow all in cache to stay longer
                for (unsigned int i = 0; i < userCacheCount; ++i) CACHEITEM(userTopicCacheIndex, i)->timestamp = duration;
            }

            C->timestamp = SETTIMESTAMP(volleys);

            if (trace & TRACE_USERCACHE) Log((server) ? SERVERLOG : USERLOG,(char*)"user cache (%d) hit %d for %s\r\n", CACHEINDEX(userTopicCacheIndex,C), VOLLEYCOUNT(C), filename);
        }
        currentCache = userTopicCacheHead = CACHEINDEX(userTopicCacheIndex, C);
        return GetCacheBuffer(C);
    }

    return NULL;
}

static USERCACHE* FindCachedFile(char* filename, bool recordHit)
{
    // already in cache?
    size_t len = strlen(filename);
    if (len == 0 || !fileCacheIndex) return NULL;
    
    USERCACHE* C = NULL;
    
    if (fileCacheRecencySize > 0) C = FindCacheEntry(fileCacheIndex, fileCacheRecencyHead, filename, false);
    if (!C && fileCacheFrequencySize > 0) C = FindCacheEntry(fileCacheIndex, fileCacheFrequencyHead, filename, false);
    
    // use volley as page reference bit
    if (C && recordHit) C->timestamp = SETTIMESTAMP(1);
 
    return C;
}

char* FindFileCache(char* filename, bool recordHit)
{
    if (IsUserTopicFile(filename))
    {
        return FindCachedUserTopic(filename, recordHit);
    }
    else
    {
        USERCACHE* C = FindCachedFile(filename, recordHit);
        if (C)
        {
            if (recordHit && trace & TRACE_USERCACHE) Log((server) ? SERVERLOG : USERLOG,(char*)"user cache (%d) hit %d for %s\r\n", CACHEINDEX(fileCacheIndex,C), VOLLEYCOUNT(C), filename);
            return GetCacheBuffer(C);
        }
    }
	return NULL;
}

void ClearFileCache(char* filename)
{
    if (IsUserTopicFile(filename))
    {
        char* ptr = FindCachedUserTopic(filename, false);
        if (ptr && currentCache != NO_CACHEID) EvictFromUserTopicCache(currentCache, false);
    }
}

// Return reference to temporary cache buffer to read files into
char* GetFileBuffer()
{
    char* limit;
    if (fileBuffer) ReportBug("FATAL: Currently reading one file while trying to read another \r\n");

    fileBuffer = InfiniteStack(limit, "user file buffer");
	return fileBuffer;
}

void FreeFileBuffer()
{
    if (infiniteStack) ReleaseInfiniteStack();
    else if (fileBuffer) ReleaseStack(fileBuffer);
    fileBuffer = NULL;
}

static void CacheFile(char* filename, char* buffer, size_t size)
{
    if (!fileCacheIndex) return;  // file cache not enabled
    
    if (size >= userTopicStoreSize)
    {
        // No chance of caching a file of this size, but don't need to stop the process
        Log(BUGLOG, "File cache too small to store %d bytes for %s", size, filename);
        Log((server) ? SERVERLOG : USERLOG, "File cache too small to store %d bytes for %s", size, filename);
        return;
    }
        
    USERCACHE* C = FindCachedFile(filename, true);
    if (C)
    {
        // already in the cache
        // if want a new version, then use a different file name
        // the CLOCK aspect of the cache just sets page reference bit
        // don't need to move pages around in the list
    }
    else
    {
        // not in main cache
        // check history
        USERCACHE* HR = NULL;
        USERCACHE* HF = NULL;
        if (fileCacheRecencyHistorySize > 0) HR = FindCacheEntry(fileCacheIndex, fileCacheRecencyHistoryHead, filename, false);
        if (!HR && fileCacheFrequencyHistorySize > 0) HF = FindCacheEntry(fileCacheIndex, fileCacheFrequencyHistoryHead, filename, false);
        
        if ((fileCacheRecencySize + fileCacheFrequencySize) == fileCacheCount)
        {
            // cache is full, need to evict a file
            // first move files from the main cache to the history
            ReplaceFileCache();

            // if the file wasn't in the history either then need to evict a
            // history index, but only if we know there's space
            // otherwise we'll leave a gap in the index sequence
            if (!HR && !HF && size < (size_t)(cacheDataEnd - cacheFree))
            {
                if ((fileCacheRecencySize + fileCacheRecencyHistorySize) == fileCacheCount)
                {
                    // discard tail of recent history
                    C = RemoveTail(fileCacheIndex, fileCacheRecencyHistoryHead);
                    --fileCacheRecencyHistorySize;
                    if (fileCacheRecencyHistorySize == 0) fileCacheRecencyHistoryHead = NO_CACHEID;
                }
                else if ((fileCacheRecencySize + fileCacheFrequencySize + fileCacheRecencyHistorySize + fileCacheFrequencyHistorySize) == (2 * fileCacheCount))
                {
                    // discard tail of frequent history
                    C = RemoveTail(fileCacheIndex, fileCacheFrequencyHistoryHead);
                    --fileCacheFrequencyHistorySize;
                    if (fileCacheFrequencyHistorySize == 0) fileCacheFrequencyHistoryHead = NO_CACHEID;
                }
            }
        }
        
        if (size >= (size_t)(cacheDataEnd - cacheFree))
        {
            // not enough space left in file data cache to copy this buffer
            // not being able to cache a file is not a grevious failure
            // just record it in case the configuration needs to be changed
            Log(BUGLOG, "No space left in file cache to store %d bytes for %s", size, filename);
            Log((server) ? SERVERLOG : USERLOG, "No space left in file cache to store %d bytes for %s", size, filename);
            return;
        }
        
        if (!HR && !HF)
        {
            // not in either history, add to tail of recent cache
            // could be reusing the index from history eviction
            C = AddFileCacheEntry(C, fileCacheRecencyHead);
            ++fileCacheRecencySize;
        }
        else if (HR)
        {
            // adapt desired target size
            fileCacheTargetSize = (unsigned int)fmin(fileCacheTargetSize + fmax(1, fileCacheFrequencyHistorySize / fileCacheRecencyHistorySize), fileCacheCount);
            // in recent history, move to tail of frequent cache
            RestoreHistoryFileCache(HR, fileCacheRecencyHistoryHead, fileCacheFrequencyHead);
            --fileCacheRecencyHistorySize;
            ++fileCacheFrequencySize;
            C = HR;
        }
        else
        {
            // adapt desired target size
            fileCacheTargetSize = (unsigned int)fmax(fileCacheTargetSize - fmax(1, fileCacheRecencyHistorySize / fileCacheFrequencyHistorySize), fileCacheCount);
            // in frequent history, move to tail of frequent cache
            RestoreHistoryFileCache(HF, fileCacheFrequencyHistoryHead, fileCacheFrequencyHead);
            --fileCacheFrequencyHistorySize;
            ++fileCacheFrequencySize;
            C = HF;
        }
        
        // add the details about this file, reset page reference to 0
        UpdateCacheEntry(C, filename, buffer, size, 0);

        if (trace & TRACE_USERCACHE) Log((server) ? SERVERLOG : USERLOG,(char*)"added file %s cache (%d)\r\n",filename, CACHEINDEX(fileCacheIndex, C));
    }
}

void Cache(char* buffer, size_t size) // save a file in a buffer into cache
{
	if (!buffer) // ran out of room
	{
		ReportBug("User write failed, too big for %s", loginID);
		return;
	}

    // is this file already cached?
    char filename[SMALL_WORD_SIZE];
    BufferFilename(buffer, filename);
    if (!*filename) return;

    if (IsUserTopicFile(filename))
    {
        char* ptr = FindCachedUserTopic(filename, true);
        int volley = (ptr ? VOLLEYCOUNT(CACHEITEM(userTopicCacheIndex, currentCache)) : 1);
        if (volley >= volleyLimit || size >= (size_t)(cacheDataEnd - cacheFree))
        {
            // large topic files can just be written out instead of cache
            WriteBuffer(filename, buffer, size);
            volley = 0;
        }
        if (volleyLimit && userCacheCount && size < (size_t)(cacheDataEnd - cacheFree))
        {
            // save to cache
            AddToUserTopicCache(filename, buffer, size, volley);
        }
        currentCache = NO_CACHEID;
    }
    else
    {
        CacheFile(filename, buffer, size);
    }
}

static void DumpFileCache(USERCACHE* cache, unsigned int head, bool showTime)
{
    if (head < 0) return;
    unsigned int start = head;
    while (ALWAYS)
    {
        USERCACHE* C = CACHEITEM(cache, start);
        if (C->hash > 0)
        {
            char* ptr = GetCacheBuffer(C);
            unsigned int volleys = VOLLEYCOUNT(C);
            unsigned int timestamp = TIMESTAMP(C);

            char filename[SMALL_WORD_SIZE];
            BufferFilename(ptr, filename);
            if (!*filename) sprintf(filename, "%llu", (long long)C->hash);

            if (showTime) Log((server) ? SERVERLOG : USERLOG, "    %5d: %5d %6d %s \r\n", start, volleys, timestamp, filename );
            else Log((server) ? SERVERLOG : USERLOG, "    %5d: %5d %s \r\n", start, volleys, filename );
        }
        start = C->next;
        if (start == head) break;    // end of loop when all are in use
    }
}

void DumpFileCaches()
{
    if (userCacheCount)
    {
        if (userTopicCacheHead > NO_CACHEID && CACHEITEM(userTopicCacheIndex, userTopicCacheHead)->hash > 0)
        {
            Log((server) ? SERVERLOG : USERLOG, "User Topic  hits   time file \r\n");
            DumpFileCache(userTopicCacheIndex, userTopicCacheHead, true);
        }
        else if (volleyLimit) Log((server) ? SERVERLOG : USERLOG, "User topic cache empty \r\n");
    }
    
    if (fileCacheCount)
    {
        bool entries = false;
        if (fileCacheRecencyHead > NO_CACHEID)
        {
            Log((server) ? SERVERLOG : USERLOG, "    Recent  pref file \r\n");
            DumpFileCache(fileCacheIndex, fileCacheRecencyHead, false);
            entries = true;
        }
        if (fileCacheFrequencyHead > NO_CACHEID)
        {
            Log((server) ? SERVERLOG : USERLOG, "  Frequent  pref file \r\n");
            DumpFileCache(fileCacheIndex, fileCacheFrequencyHead, false);
            entries = true;
        }
        
        if (fileCacheRecencyHistorySize > 0) Log((server) ? SERVERLOG : USERLOG, "Recent File History: %d \r\n", fileCacheRecencyHistorySize);

        if (fileCacheFrequencyHistorySize > 0) Log((server) ? SERVERLOG : USERLOG, "Frequent File History: %d \r\n", fileCacheFrequencyHistorySize);

        // cache will not be just history
        if (!entries) Log((server) ? SERVERLOG : USERLOG, "File cache empty \r\n");
    }
    
    Log((server) ? SERVERLOG : USERLOG, "Free: %dkb \r\n", (unsigned int)((cacheDataEnd - cacheFree)/1000) );
}
