#ifndef _MONGOH
#define _MONGOH
#ifdef INFORMATION
Copyright (C)2011-2020 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif

#ifndef DISCARDMONGO

#define MAX_COLLECTIONS_LIMIT 10
#define MONGO_COLLECTION_NAME_LENGTH 256

#define MONGO_DBPARAM_SIZE MAX_WORD_SIZE * (4 + MAX_COLLECTIONS_LIMIT)  // 4+ parts: url database topic ltm {additionalCollections} - can also include read preference data for each collection
extern char mongodbparams[MONGO_DBPARAM_SIZE];

const char* MongoVersion();
void MongoSystemInit(char* params);
void MongoShutdown();
void MongoUserFilesClose();

FunctionResult MongoInit(char* buffer);
FunctionResult MongoClose(char* buffer);
void MongoSystemShutdown();
FunctionResult mongoFindDocument(char* buffer);
FILE* mongouserCreate(const char* name);
FunctionResult mongoInsertDocument(char* buffer);
FunctionResult mongoDeleteDocument(char* buffer);
FunctionResult mongoGetDocument(char* key,char* buffer,int limit,bool user);
#endif

#endif
