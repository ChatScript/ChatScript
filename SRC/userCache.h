#ifndef _USERCACHEH
#define _USERCACHEH
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


#define DEFAULT_USER_CACHE 5000000

// cache data
#define PRIOR(x) ( (3 * (x) ))
#define NEXT(x) ( (3 * (x) ) + 1)
#define TIMESTAMP(x) ( ( 3 * (x)) + 2)
#define VOLLEYCOUNT(x) (cacheIndex[TIMESTAMP(x)] >> 24)
#define DEFAULT_VOLLEY_LIMIT 0 // write always and read always
#define OVERFLOW_SAFETY_MARGIN 5000
#define MAX_USERNAME 400
extern unsigned int userTopicStoreSize,userTableSize; // memory used which we will display
extern unsigned int userCacheCount,userCacheSize;
extern bool cacheUsers;
extern  int volleyLimit;
extern char* cacheBase;
extern char* userDataBase;

// cache system
char* GetFileRead(char* user,char* computer);
void Cache(char* buffer,size_t len);
void FreeUserCache();
char* GetFreeCache();
void FlushCache();
void FreeAllUserCaches();
char* FindUserCache(char* word);
char* GetCacheBuffer(int cacheID);
void InitCache();
void CloseCache();
char* GetUserFileBuffer();
#endif
