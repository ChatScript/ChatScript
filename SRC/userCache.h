#ifndef _USERCACHEH
#define _USERCACHEH
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


#define DEFAULT_USER_CACHE 5000000
#define CPU_TICKS ( clock() / CLOCKS_PER_SEC )

// cache data
#define MAX_VOLLEYCOUNT 0xff
#define MAX_CACHE_DURATION 0x00ffffff
#define CACHE_DURATION_BITS 24

#define SETTIMESTAMP(x) (unsigned int)((CPU_TICKS - startSystem) | (x << CACHE_DURATION_BITS))
#define TIMESTAMP(x) (x->timestamp & MAX_CACHE_DURATION)
#define VOLLEYCOUNT(x) (x->timestamp >> CACHE_DURATION_BITS)

#define CACHEITEM(c, x) (c + x)
#define CACHEINDEX(c, x) (int)(x - c)

typedef struct USERCACHE
{
    int prior;
    int next;
    unsigned int timestamp;
    unsigned int hash;
    unsigned int buffer;
} USERCACHE;

#define DEFAULT_VOLLEY_LIMIT 0 // write always and read always
#define OVERFLOW_SAFETY_MARGIN 5000
#define MAX_USERNAME 400

extern unsigned int userTopicStoreSize,userTableSize; // memory used which we will display
extern unsigned int userCacheCount,userCacheSize;
extern unsigned int fileCacheCount,fileCacheSize;
extern bool cacheUsers;
extern  int volleyLimit;
extern char* cacheBase;

// cache system
void InitUserCache();
void CloseUserCache();
char* GetFileBuffer();
void FreeFileBuffer();
char* FindFileCache(char* filename, bool recordHit = false);
void ClearFileCache(char* filename);
void Cache(char* buffer,size_t len);
void FlushCache();
void FreeAllUserCaches();
void DumpFileCaches();
#endif
