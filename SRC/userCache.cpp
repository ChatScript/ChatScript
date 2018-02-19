#include "common.h"

#define NO_CACHEID -1

static unsigned int cacheHead = 0;		// our marker for last allocated cache, used to find next free one
static unsigned int* cacheIndex = 0;	// data ring of the caches + timestamp/volley info
char* cacheBase = 0;					// start of contiguous cache block of caches
static int currentCache = NO_CACHEID;	// the current user file buffer
unsigned int userCacheCount = 1;		// holds 1 user by default
unsigned int userCacheSize = DEFAULT_USER_CACHE; // size of user file buffer (our largest buffers)
int volleyLimit =  -1;					// default save user records to file every n volley (use default 0 if this value is unchanged by user)
char* userDataBase = NULL;				// current write user record base
unsigned int userTopicStoreSize,userTableSize; // memory used which we will display

void InitCache()
{
	if (cacheBase != 0) return;	// no need to reallocate on reload
	
	userTableSize = userCacheCount * 3 * sizeof(unsigned int);
	userTableSize /= 64;
	userTableSize = (userTableSize * 64) + 64; // 64 bit align both ends

	cacheBase = (char*) malloc( userTopicStoreSize + userTableSize );
	if (!cacheBase)
	{
		(*printer)((char*)"Out of  memory space for user cache %d %d %d\r\n",userTopicStoreSize,userTableSize,MAX_DICTIONARY);
		ReportBug((char*)"FATAL: Cannot allocate memory space for user cache %ld\r\n",(long int) userTopicStoreSize)
	}
	cacheIndex = (unsigned int*) (cacheBase + userTopicStoreSize); // linked list for caches - each entry is [3] wide 0=prior 1=next 2=TIMESTAMP
	char* ptr = cacheBase;
	for (unsigned int i = 0; i < userCacheCount; ++i) 
	{
		*ptr = 0; // item is empty
		cacheIndex[PRIOR(i)] = i - 1; // before ptr
		cacheIndex[NEXT(i)] = i + 1; // after ptr
		ptr += userCacheSize;
	}
	cacheIndex[PRIOR(0)] = userCacheCount-1; // last one as prior
	cacheIndex[NEXT(userCacheCount-1)] = 0;	// start as next one (circular list)
	ClearNumbers();
}

void CloseCache()
{
	free(cacheBase);
	cacheBase = NULL;
}

static void WriteCache(unsigned int which,size_t size)
{
	char* ptr =  GetCacheBuffer(which);
	if (!*ptr) return;	// nothing to write out

	unsigned int volleys = VOLLEYCOUNT(which);
	if (!volleys) return; // unchanged from prior write out

	if (size == 0) size = strlen(ptr);// request to compute size
	char* at = strchr(ptr,'\r'); // separate off the name from the rest of the data
	*at = 0;
	char filename[SMALL_WORD_SIZE];
	strcpy(filename,ptr); // safe separation
	*at = '\r'; // legal
	uint64 start_time = ElapsedMilliseconds();

	FILE* out = userFileSystem.userCreate(filename); // wb binary file (if external as db write should not fail)
	if (!out) // see if we can create the directory (assuming its missing)
	{
		char call[MAX_WORD_SIZE];
		sprintf(call,(char*)"mkdir %s",users);
		system(call);
		out = userFileSystem.userCreate(filename);

		if (!out) 
		{
			ReportBug((char*)"cannot open user state file %s to write\r\n",ptr)
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
        fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK	*/
        fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
        fl.l_start  = 0;	/* Offset from l_whence         */
        fl.l_len    = 0;	/* length, 0 = to EOF           */
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
	if (trace & TRACE_USERCACHE) Log((server) ? SERVERLOG : STDTRACELOG,(char*)"write out cache (%d)\r\n",which);
	if (timing & TIME_USERCACHE) {
		int diff = (int)(ElapsedMilliseconds() - start_time);
		if (timing & TIME_ALWAYS || diff > 0) Log((server) ? SERVERLOG : STDTIMELOG, (char*)"Write user cache %d in file %s time: %d ms\r\n", which, filename, diff);
	}

	cacheIndex[TIMESTAMP(which)] &= 0x00ffffff;	// clear volley count since last written but keep time info
}

void FlushCache() // writes out the cache but does not invalidate it
{
	if (!userCacheCount) return;
	unsigned int start = cacheHead;
	while (ALWAYS) 
	{
		WriteCache(start,0);
		start = cacheIndex[NEXT(start)];
		if (start == cacheHead) break;	// end of loop when all are in use
	}
}

char* GetFreeCache() // allocate backwards from current, so in use is always NEXT from current
{
	if (!userCacheCount) return NULL;
	unsigned int duration = (clock() / 	CLOCKS_PER_SEC) - startSystem; // how many seconds since start of program launch
	// need to find a cache that's free else flush oldest one
	unsigned int last = cacheIndex[PRIOR(cacheHead)];	// PRIOR ptr of list start
	char* ptr = GetCacheBuffer(last);
	if (*ptr) // this is in use
	{
		WriteCache(last,0);  
		*ptr = 0; // clear cache so it can be reused
	}
	cacheIndex[TIMESTAMP(last)] = duration; // includes 0 volley count and when this was allocated
	currentCache = cacheHead = last; // just rotate the ring - has more than one block
	return ptr;
}

void FreeUserCache()
{
	if (currentCache != NO_CACHEID ) // this is the current cache, disuse it
	{
		*GetCacheBuffer(currentCache) = 0;
		cacheHead = cacheIndex[NEXT(currentCache)];
		currentCache = NO_CACHEID;
	}
}

void FreeAllUserCaches()
{
	FlushCache();
	unsigned int start = cacheHead;
	while (userCacheCount) // kill all active caches
	{
		*GetCacheBuffer(start) = 0;
		start = cacheIndex[NEXT(start)];
		if (start == cacheHead) break;	// end of loop
	}
	currentCache = NO_CACHEID;
}

char* FindUserCache(char* word)
{
	// already in cache?
	unsigned int start = cacheHead;
	while (userCacheCount)
	{
		char* ptr = GetCacheBuffer(start);
		size_t len = strlen(word);
		if (!strnicmp(ptr,word,len) && ptr[len+1] == '\r') // this is the user
		{
			if (start != cacheHead) // make him FIRST on the list
			{
				unsigned int prior = cacheIndex[PRIOR(start)];
				unsigned int next = cacheIndex[NEXT(start)];
				// decouple from where it is
				cacheIndex[NEXT(prior)] = next;
				cacheIndex[PRIOR(next)] = prior;

				// now insert in front
				prior = cacheIndex[PRIOR(cacheHead)];
				cacheIndex[PRIOR(cacheHead)] = start;

				cacheIndex[NEXT(prior)] = start;
				cacheIndex[PRIOR(start)] = prior;
				cacheIndex[NEXT(start)] = cacheHead;
			}
			currentCache = cacheHead = start;
			return ptr;
		}
		start = cacheIndex[NEXT(start)];
		if (start == cacheHead) break;	// end of loop
	}
	return NULL;
}

void CopyUserTopicFile(char* newname)
{
	char file[SMALL_WORD_SIZE];
	sprintf(file,(char*)"%s/topic_%s_%s.txt",users,loginID,computerID);
	if (stricmp(language, "english")) sprintf(file, (char*)"%s/topic_%s_%s_%s.txt", users, loginID, computerID,language);
	char newfile[MAX_WORD_SIZE];
	sprintf(newfile,(char*)"LOGS/%s-topic_%s_%s.txt",newname,loginID,computerID);
	if (stricmp(language, "english")) sprintf(newfile, (char*)"LOGS/%s-topic_%s_%s_%s.txt", newname, loginID, computerID, language);
	CopyFile2File(file,newfile,false);
}

char* GetFileRead(char* user,char* computer)
{
	char word[MAX_WORD_SIZE];
	sprintf(word,(char*)"%s/%stopic_%s_%s.txt",users,GetUserPath(loginID),user,computer);
	if (stricmp(language,"english")) sprintf(word, (char*)"%s/%stopic_%s_%s_%s.txt", users, GetUserPath(loginID), user, computer,language);
	char* buffer;
	if ( filesystemOverride == NORMALFILES) // local files
	{
		char name[MAX_WORD_SIZE];
		strcpy(name,word);
		strcat(name,"\r\n");
		buffer = FindUserCache(name); // sets currentCache and makes it first if non-zero return -  will either find but not assign if not found
		if (buffer) return buffer;
	}
	uint64 start_time = ElapsedMilliseconds();

	// have to go read it
	buffer = GetFreeCache(); // get cache buffer 
    FILE* in = (!buffer) ? NULL : userFileSystem.userOpen(word); // user topic file

#ifdef LOCKUSERFILE
#ifdef LINUX
	if (server && in &&  filesystemOverride == NORMALFILES)
	{
		int fd = fileno(in);
		if (fd < 0) 
		{
		    userFileSystem.userClose(in);
			in = 0;
		}
		else
		{
			struct flock fl;
			fl.l_type   = F_RDLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK	*/
			fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
			fl.l_start  = 0;	/* Offset from l_whence		*/
			fl.l_len    = 0;	/* length, 0 = to EOF		*/
			fl.l_pid    = getpid(); /* our PID			*/
			if (fcntl(fd, F_SETLKW, &fl) < 0) 
			{
				userFileSystem.userClose(in);
				in = 0;
			}
		}
	}
#endif
#endif
	if (in) // read in data if file exists
	{
		size_t readit;
		readit = DecryptableFileRead(buffer,1,userCacheSize,in,userEncrypt,"USER"); // reading topic file of user
		buffer[readit] = 0;
		buffer[readit+1] = 0; // insure nothing can overrun
		userFileSystem.userClose(in);
		if (trace & TRACE_USERCACHE) Log((server) ? SERVERLOG : STDTRACELOG,(char*)"read in %s cache (%d)\r\n",word,currentCache);
		if (timing & TIME_USERCACHE) {
			int diff = (int)(ElapsedMilliseconds() - start_time);
			if (timing & TIME_ALWAYS || diff > 0) Log((server) ? SERVERLOG : STDTIMELOG, (char*)"Read user cache %d in file %s time: %d ms\r\n", currentCache, word, diff);
		}
	}
	return buffer;
}

char* GetCacheBuffer(int which)
{
	return (which < 0) ? GetFreeCache() : (cacheBase+(which * userCacheSize)); // NOT from cache system, get a cache buffer
}

char* GetUserFileBuffer() // when we need a really big buffer (several megabytes)
{
	return GetFreeCache();
}

void Cache(char* buffer, size_t size) // save into cache
{
	if (!buffer) // ran out of room
	{
		ReportBug("User write failed, too big for %s", loginID);
		return;
	}
	unsigned int duration = (clock() / 	CLOCKS_PER_SEC) - startSystem; // how many seconds since start of program launch

	// dont want to overflow 24bit second store... so reset system start if needed
	if ( duration > 0x000fffff) 
	{
		startSystem = clock() / CLOCKS_PER_SEC; 
		duration = 0; // how many seconds since start of program launch
		// allow all in cache to stay longer
		for (unsigned int i = 0; i < userCacheCount; ++i) cacheIndex[TIMESTAMP(i)] = duration; 
	}

	// write out users that haven't been saved recently
	unsigned int volleys = VOLLEYCOUNT(currentCache) + 1; 
	cacheIndex[TIMESTAMP(currentCache)] = duration | (volleys << 24); 
	if ( (int)volleys >= volleyLimit) WriteCache(currentCache,size); // writecache clears the volley count - default 1 ALWAYS writes
	if (!volleyLimit) FreeUserCache(); // force reload each time, it will have already been written out immediately before

	currentCache = NO_CACHEID;
}
