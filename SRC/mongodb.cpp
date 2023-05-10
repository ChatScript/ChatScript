#include "common.h"

//////////////////////////////////////////////////////////
/// MONGO DATABASE FUNCTIONS
//////////////////////////////////////////////////////////

#ifndef DISCARDMONGO
    #ifdef WIN32
        #include "mongo/MongoDBClient.h"
        #include "mongo/stdafx.h"

        #pragma comment(lib, "../SRC/mongo/mongoc-1.0.lib")
        #pragma comment(lib, "../SRC/mongo/bson-1.0.lib")
    #else
        // #include <mongo/MongoDBClient.h>
		#include "mongo/MongoDBClient.h"
    #endif

#include "mongoc.h"

static bool mongoInited = false;		// have we inited mongo overall
static bool mongoShutdown = false;
char* mongoBuffer = NULL;
char mongodbparams[MONGO_DBPARAM_SIZE];

// for script use
mongoc_client_t*		g_pClient = NULL;
mongoc_database_t*		g_pDatabase = NULL;
mongoc_collection_t*	g_pCollection = NULL;
mongoc_read_prefs_t*    g_pReadPrefs = NULL;
// for file system use
mongoc_client_t*		g_filesysClient = NULL;
mongoc_database_t*		g_filesysDatabase = NULL;
mongoc_collection_t*    g_filesysCollectionTopic = NULL; // user topic
mongoc_read_prefs_t*    g_filesysReadPrefsTopic = NULL;  // read preferences


// Mongo Collection to File map and related functions
typedef struct CollectionInfo {
	char name[MONGO_COLLECTION_NAME_LENGTH];    // collection name
	mongoc_collection_t *collectionHandle;      // collection handle
    mongoc_read_prefs_t *readPrefs;             // read preferences
} CollectionInfo;

CollectionInfo collectionSet[MAX_COLLECTIONS_LIMIT];

mongoc_read_prefs_t* parseMongoCollectionOptions(char* data)
{
    // mimic the format used on the Mongo connection string
    char *at = strchr(data, '?');
    if (!at) return NULL;

    mongoc_read_prefs_t* read_prefs = NULL;

    *at = 0;
    
    // default read preference is primary
    read_prefs = mongoc_read_prefs_new(MONGOC_READ_PRIMARY);

    // tags can appear multiple times
    bson_t* tagset = bson_new();
    int numTags = 0;

    while(*++at)
    {
        char *amp = strchr(at,'&');
        if (amp) *amp = 0;
        else amp = at + strlen(at) - 1;

        if (!strnicmp(at, "readPreference=", 15))
        {
            at += 15;
            if (!stricmp(at, "primary")) mongoc_read_prefs_set_mode(read_prefs, MONGOC_READ_PRIMARY);
            else if (!stricmp(at, "primaryPreferred")) mongoc_read_prefs_set_mode(read_prefs, MONGOC_READ_PRIMARY_PREFERRED);
            else if (!stricmp(at, "secondary")) mongoc_read_prefs_set_mode(read_prefs, MONGOC_READ_SECONDARY);
            else if (!stricmp(at, "secondaryPreferred")) mongoc_read_prefs_set_mode(read_prefs, MONGOC_READ_SECONDARY_PREFERRED);
            else if (!stricmp(at, "nearest")) mongoc_read_prefs_set_mode(read_prefs, MONGOC_READ_NEAREST);
        }
        
        else if (!strnicmp(at, "maxStalenessSeconds=", 20))
        {
            at += 20;
            int64_t secs = atoi(at);
            // anything between 0 and 90 is an error according to Mongo documentation
            if (secs == -1 || secs >= 90) mongoc_read_prefs_set_max_staleness_seconds(read_prefs, secs);
        }
        
        else if (!strnicmp(at, "readPreferenceTags=", 19))
        {
            at += 19;

            bson_t* tag = bson_new();

            // a tag has name:value pairs in a comma separated list
            // accumulate them for now
            char* ptr = at - 1;
            while (*++ptr)
            {
                char* comma = strchr(ptr,',');
                if (comma) *comma = 0;
                else comma = ptr + strlen(ptr) - 1;
                
                char* colon = strchr(ptr,':');
                if (colon)
                {
                    *colon = 0;
                    BSON_APPEND_UTF8(tag, ptr, (colon+1) );
                }

                ptr = comma;
            }

            char key[10];
            sprintf(key, "%d", numTags++);
            BSON_APPEND_DOCUMENT(tagset, key, tag);
            bson_destroy(tag);
        }
        
        else if (!strnicmp(at, "hedge=", 6))
        {
            at += 6;
            bson_t* hedge = bson_new();
            if (!stricmp(at,"true")) BSON_APPEND_BOOL(hedge, "enabled", true);
            else BSON_APPEND_BOOL(hedge, "enabled", false);
            mongoc_read_prefs_set_hedge(read_prefs, hedge);
            bson_destroy(hedge);
        }

        at = amp;
    }

    // might have some tags to add
    if (numTags > 0) mongoc_read_prefs_set_tags (read_prefs, tagset);
    bson_destroy(tagset);

    return(read_prefs);
}

bool initCollectionHandle(char* name, char* dbname)
{
    bool initHandle = false;
    if (!name) return initHandle;
    int i = -1;
    CollectionInfo *collection = NULL;

	while ((collection = &collectionSet[++i]) && strlen(collection->name) > 0) {;}
	if (i < MAX_COLLECTIONS_LIMIT) {
		strcpy(collection->name, name);
		addFileTypeAsKnown(name);
        
        // dbname might incorporate read preferences, so strip them out
        collection->readPrefs = parseMongoCollectionOptions(dbname);
        
		initHandle = true;
	}
	return initHandle;
}

bool hasMongoCollectionHandle(char* filetype)
{
    bool isCollectionHandle = false;
    if (!filetype) return isCollectionHandle;
    int i = -1;
    CollectionInfo *collection = NULL;

    while ((collection = &collectionSet[++i]) && strlen(collection->name) > 0)
        if (!stricmp(collection->name, filetype)) break;

    if (collection->collectionHandle) isCollectionHandle = true;

    return isCollectionHandle;
}

mongoc_collection_t** getMongoCollectionHandleAddress(char* filetype)
{
    if (!filetype) return NULL;
    int i = -1;
    CollectionInfo *collection = NULL;

    while ((collection = &collectionSet[++i]) && strlen(collection->name) > 0)
        if (!stricmp(collection->name, filetype)) break;

    return &collection->collectionHandle;
}

mongoc_collection_t* getMongoCollectionHandle(char* filetype)
{
    if (!filetype) return NULL;
    int i = -1;
    CollectionInfo *collection = NULL;

    while ((collection = &collectionSet[++i]) && strlen(collection->name) > 0)
        if (!stricmp(collection->name, filetype)) break;

    return collection->collectionHandle;
}

mongoc_read_prefs_t* getMongoCollectionReadPrefs(char* filetype)
{
    if (!filetype) return NULL;
    int i = -1;
    CollectionInfo *collection = NULL;

    while ((collection = &collectionSet[++i]) && strlen(collection->name) > 0)
        if (!stricmp(collection->name, filetype)) break;

    return collection->readPrefs;
}


void destroyMongoCollectionData()
{
    int i = -1;
    CollectionInfo *collection = NULL;

	while ((collection = &collectionSet[++i]) && strlen(collection->name) > 0)
	{
		if (collection->collectionHandle != NULL) mongoc_collection_destroy(collection->collectionHandle);
		collection->collectionHandle = NULL;
        if (collection->readPrefs != NULL) mongoc_read_prefs_destroy(collection->readPrefs);
        collection->readPrefs = NULL;
		memset(collection->name, 0, MONGO_COLLECTION_NAME_LENGTH);
	}
}


const char* MongoVersion()
{
    static char version[MAX_WORD_SIZE] = "";
    if (*version) return(version);
    
    const char *data = mongoc_get_version();

    sprintf(version,"Mongo %s", data);
    return(version);
}

char* MongoCleanEscapes(char* to, char* at,int limit) 
{ // any sequence of \\\ means mongo added \\  and any freestanding \ means mongo added that
	// and 0x7f 0x7f is our own crnl tag
	*to = 0;
	char* start = to;
	if (!at || !*at) return to;
	// need to remove escapes we put there
	--at;
	while (*++at)
	{
		if (*at == 0x7f && at[1] == 0x7f) // own special CR NL coding
		{
			*to++ = '\r'; // legal
			*to++ = '\n'; // legal
			++at;
		}
		/*else if (*at == '\\') // remove backslashed "
		{
			*to++ = *++at;
		}*/
		else *to++ = *at;
		if ((to-start) >= limit) // too much, kill it
		{
			*to = 0;
			break;
		}
	}
	*to = 0;
	return start;
}

void mongoLogError(bson_error_t error)
{
    char* msg = AllocateBuffer();
    
    sprintf(msg, "%d: %s", error.code, error.message);
    SetUserVariable((char*)"$$mongo_error",msg);    // pass along the error to script

    sprintf(msg, "Mongo Error: %d - %s", error.code, error.message);
    Log(USERLOG,msg);

    FreeBuffer();
}


// Apply additional key values from a JSON object to a document query
void mongoAppendKeys(bson_t *doc, char* var)
{
    char* externalMongoData = GetUserVariable(var,false);
    if (*externalMongoData && IsValidJSONName(externalMongoData, 'o'))
    {
        WORDP D = FindWord(externalMongoData);
        if (D)
        {
            FACT* F = GetSubjectNondeadHead(D);
            while (F)
            {
                char* additionalKey = Meaning2Word(F->verb)->word;
                if (!bson_has_field(doc, additionalKey))
                {
                    char* additionalValue = Meaning2Word(F->object)->word;
                    BSON_APPEND_UTF8(doc, additionalKey, additionalValue);
                }
                F = GetSubjectNondeadNext(F);
            }
        }
    }
}

// connect to server, db, and collection
eReturnValue EstablishConnection(	const char* pStrSeverUri, // eg "mongodb://localhost:27017"
									const char* pStrDBName,   // eg "testMongo"
									const char* pStrCollName, // eg "testCollection"
									mongoc_collection_t** mycollect)
{
	if( ( pStrSeverUri == NULL ) || ( pStrDBName == NULL ) || ( pStrCollName == NULL ) ) return eReturnValue_WRONG_ARGUEMENTS;

    bool isScriptUser = (mycollect == &g_pCollection ? true : false);

	// Required to initialize libmongoc's internals
	if (!mongoInited) mongoc_init();
	mongoInited = true;

	// Create a new client instance (user/script or interal/filesystem)
    mongoc_client_t* myclient = NULL;
    if ( isScriptUser && g_pClient != NULL ) myclient = g_pClient;
    else if ( g_filesysClient != NULL ) myclient = g_filesysClient;
    else
    {
        myclient  = mongoc_client_new( pStrSeverUri );
        if( myclient == NULL ) return eReturnValue_DATABASE_OPEN_CLIENT_CONNECTION_FAILED;
        if ( isScriptUser ) g_pClient = myclient;  // script user
        else g_filesysClient = myclient; // all 3 filesys collections use this client
    }
	
    mongoc_database_t* mydb = NULL;
    if ( isScriptUser && g_pDatabase != NULL ) mydb = g_pDatabase;
    else if ( g_filesysDatabase != NULL ) mydb = g_filesysDatabase;
    else
    {
        char* enableSSL = GetUserVariable("$mongo_enable_ssl", false);
#ifdef MONGOSSLOPTS
        if(stricmp(enableSSL,(char*)"true") == 0) {
		
            char* validateSSL = GetUserVariable("$mongovalidatessl", false);
            char* sslCAFile = GetUserVariable("$mongosslcafile", false);
            char* sslPemFile = GetUserVariable("$mongosslpemfile", false);
            char* sslPemPwd = GetUserVariable("$mongosslpempwd", false);

            const mongoc_ssl_opt_t *ssl_default = mongoc_ssl_opt_get_default ();
            mongoc_ssl_opt_t ssl_opts = { 0 };
            /* optionally copy in a custom trust directory or file; otherwise the default is used. */
            memcpy (&ssl_opts, ssl_default, sizeof ssl_opts);
            if(stricmp(sslPemFile,(char*)"") != 0) ssl_opts.pem_file = sslPemFile;
            if(stricmp(sslPemPwd,(char*)"") != 0) ssl_opts.pem_pwd = sslPemPwd;
            if(stricmp(sslCAFile,(char*)"") != 0) ssl_opts.ca_file = sslCAFile;
            if ( stricmp(validateSSL,(char*)"true") != 0) ssl_opts.weak_cert_validation = true;
            mongoc_client_set_ssl_opts (myclient, &ssl_opts);
        }
#endif

        // Get a handle on the database "db_name"
        mydb = mongoc_client_get_database(myclient, pStrDBName);
        if( mydb == NULL ) return eReturnValue_DATABASE_GET_FAILED;
        if ( isScriptUser ) g_pDatabase = mydb; // script user
        else g_filesysDatabase = mydb; // all 3 filesys collections use this database
    }

	// Get a handle on the collection "coll_name"
	*mycollect  = mongoc_client_get_collection(myclient, pStrDBName, pStrCollName);
	if( *mycollect == NULL ) 
	{
		if (!isScriptUser) ReportBug("INFO: Failed to open mongo filesys");
		return eReturnValue_DATABASE_GET_COLLECTION_FAILED;
	}
	return eReturnValue_SUCCESS;
}

FunctionResult MongoClose(char* buffer)
{
	if (!buffer && !g_filesysClient) return FAILRULE_BIT; // filesys mongo not being used
	if (buffer && !g_pClient)
	{
		if (mongoShutdown) return FAILRULE_BIT;
		char* msg = "DB is not open\r\n";
		SetUserVariable((char*)"$$mongo_error",msg);	// pass message along the error
		Log(USERLOG,msg);
		return FAILRULE_BIT;
	}

	// Release our handles and clean up libmongoc
	if (buffer)
	{
        if( g_pReadPrefs != NULL ) mongoc_read_prefs_destroy(g_pReadPrefs);
		if( g_pCollection != NULL ) mongoc_collection_destroy(g_pCollection);
		if( g_pDatabase != NULL ) mongoc_database_destroy(g_pDatabase);
		if( g_pClient != NULL ) mongoc_client_destroy(g_pClient);
        g_pReadPrefs = NULL;
		g_pCollection = NULL;
		g_pDatabase = NULL;
		g_pClient = NULL;
	}
	else
    {
		destroyMongoCollectionData();

        if( g_filesysReadPrefsTopic != NULL ) mongoc_read_prefs_destroy(g_filesysReadPrefsTopic);
        if( g_filesysCollectionTopic != NULL ) mongoc_collection_destroy(g_filesysCollectionTopic);
		if( g_filesysDatabase != NULL ) mongoc_database_destroy(g_filesysDatabase);
		if( g_filesysClient != NULL ) mongoc_client_destroy(g_filesysClient);
        g_filesysReadPrefsTopic = NULL;
		g_filesysCollectionTopic = NULL;
		g_filesysDatabase =  NULL;
		g_filesysClient = NULL;
	}
	*currentFilename = 0;
	return (buffer == NULL) ? FAILRULE_BIT : NOPROBLEM_BIT; 
}

FunctionResult MongoInit(char* buffer)
{
	if (buffer && g_pClient) // assume system never gets it wrong so dont check
	{
		char* msg = "DB is already opened\r\n";
		SetUserVariable((char*)"$$mongo_error",msg);	// pass message along the error
		Log(USERLOG,msg);
 		return FAILRULE_BIT;
	}

    /* Make a connection to the database */
	mongoc_collection_t** collectvar = &g_pCollection; // default user 
	if (!stricmp(ARGUMENT(4),"topic"))
    {
        collectvar = &g_filesysCollectionTopic; // filesys
        g_filesysReadPrefsTopic = parseMongoCollectionOptions(ARGUMENT(3));
    }
	else if (initCollectionHandle(ARGUMENT(4), ARGUMENT(3)))
    {
        collectvar = getMongoCollectionHandleAddress(ARGUMENT(4)); // filesys
    }
    else
    {
        g_pReadPrefs = parseMongoCollectionOptions(ARGUMENT(3));
    }
    
    eReturnValue  eRetVal = EstablishConnection(ARGUMENT(1), ARGUMENT(2), ARGUMENT(3), collectvar); // server, dbname, collection
    if (eRetVal != eReturnValue_SUCCESS )
    {	
		char* msg = "DB opening error \r\n";
		SetUserVariable((char*)"$$mongo_error",msg);	// pass message along the error
        Log(USERLOG, "Opening connection failed with error: %d",  eRetVal);
		if ((buffer && g_pClient) || (!buffer && g_filesysClient)) MongoClose(buffer);
		return FAILRULE_BIT;
	}
	return NOPROBLEM_BIT;
}

FunctionResult mongoGetDocument(char* key,char* buffer,int limit,bool user)
{
	if (*key == '"') // remove quotes
	{
		++key;
		size_t len = strlen(key);
		if (key[len-1] == '"') key[len-1] = 0;
	}
    mongoc_database_t* db = NULL;
    mongoc_collection_t* collection = NULL;
    mongoc_read_prefs_t* read_prefs = NULL;
	if (user)
    {
        db = g_pDatabase;
        collection = g_pCollection;
        read_prefs = g_pReadPrefs;
    }
	else if (hasMongoCollectionHandle(ARGUMENT(2)))
    {
        db = g_filesysDatabase;
        collection = getMongoCollectionHandle(ARGUMENT(2));
        read_prefs = getMongoCollectionReadPrefs(ARGUMENT(2));
    }
	else
    {
        db = g_filesysDatabase;
        collection = g_filesysCollectionTopic;
        read_prefs = g_filesysReadPrefsTopic;
    }
    if (!collection)
    {
        char* msg = "DB is not open\r\n";
        SetUserVariable((char*)"$$mongo_error",msg);
        Log(USERLOG,msg);
        return FAILRULE_BIT;
    }
    
    char* mongoKeyValue = NULL;   // keyValue result string
    eReturnValue eRetVal = eReturnValue_SUCCESS;
    bson_t* pDoc = NULL;
    mongoc_cursor_t* psCursor = NULL;
    bson_t* psQuery = NULL;
    char* pStrTemp = NULL;
    do
    {
        if( collection == NULL )
        {
            eRetVal = eReturnValue_DATABASE_COLLECTION_INVALID;
            break;
        }
        
        psQuery = bson_new ();
        if( psQuery == NULL )
        {
            eRetVal = eReturnValue_DATABASE_QUERY_CREATION_FAILED;
            break;
        }
        
        BSON_APPEND_UTF8 ( psQuery, "KeyName", key );
        // updates the mongo read query with key value pairs from $cs_mongoQueryParams
        mongoAppendKeys( psQuery, (char*)"$cs_mongoQueryParams");
#ifdef PRIVATE_CODE
        // Check for private hook function to adjust the Mongo query parameters
        static HOOKPTR fnq = FindHookFunction((char*)"MongoQueryParams");
        if (fnq)
        {
            ((MongoQueryParamsHOOKFN) fnq)(psQuery);
        }
        static HOOKPTR fnos = FindHookFunction((char*)"DatabaseOperationStart");
        if (fnos)
        {
            ((DatabaseOperationStartHOOKFN) fnos)("Mongo", mongoc_database_get_name(db), mongoc_collection_get_name(collection), "get", key);
        }
#endif

        uint64 starttime = ElapsedMilliseconds();
        
        psCursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, psQuery, NULL, read_prefs);
        
        if( psCursor == NULL )
        {
            eRetVal = eReturnValue_DATABASE_QUERY_FAILED;
            break;
        }
        size_t retrievedDataSize = 0;
    	uint64 cursorReadStart = ElapsedMilliseconds();
        while( mongoc_cursor_next (psCursor, (const bson_t **)&pDoc) )
        {
            if (pDoc != NULL)
            {
            	bson_iter_t iter;
            	if (bson_iter_init_find(&iter, pDoc, "KeyValue")) {
					const bson_value_t *value;
    				value = bson_iter_value (&iter);
    				if ( value->value_type == BSON_TYPE_UTF8){
    					mongoKeyValue = value->value.v_utf8.str;
                        retrievedDataSize = strlen(mongoKeyValue);
    				}
            	}
#ifdef PRIVATE_CODE
                // Check for private hook function to process the document results
                static HOOKPTR fnqG = FindHookFunction((char*)"MongoGotDocument");
                if (fnqG)
                {
                    ((MongoGotDocumentHOOKFN) fnqG)(pDoc);
                }
#endif
            }
        }
		uint64 endtime = ElapsedMilliseconds();
		unsigned int diff = (unsigned int)(endtime - starttime);
		unsigned int crdiff = (unsigned int)(endtime - cursorReadStart);
		unsigned int limit = 100;
		char* val = GetUserVariable("$mongo_timeexcess", false);
		if (*val) limit = unsigned(atoi(val));
		if (diff >= limit){
			char dbmsg[512];
			struct tm ptm;
			sprintf(dbmsg,"%s Mongo Find took longer than expected for %s, with total retrieved data size in bytes = %zu, cursor read time = %ums And time taken = %ums\r\n", GetTimeInfo(&ptm),key, retrievedDataSize, crdiff, diff);
			Log(DBTIMELOG, dbmsg);
		}
#ifdef PRIVATE_CODE
        static HOOKPTR fnoe = FindHookFunction((char*)"DatabaseOperationEnd");
        if (fnoe)
        {
            ((DatabaseOperationEndHOOKFN) fnoe)();
        }
#endif
    }while(false);
    
    FunctionResult result = NOPROBLEM_BIT;
    if (eRetVal != eReturnValue_SUCCESS)
    {
        char* msg = "Error while looking for document \r\n";
        SetUserVariable((char*)"$$mongo_error",msg);	// pass along the error
        Log(USERLOG, "Find document failed with error: %d",  eRetVal);
        result = FAILRULE_BIT;
    }
    else if (mongoKeyValue) 
    {
        unsigned int remainingSize = (user) ? (currentOutputLimit - (buffer - currentOutputBase) - 1)
			: (userCacheSize - MAX_USERNAME);
		MongoCleanEscapes(buffer, mongoKeyValue,remainingSize);
    }
    else result = FAILRULE_BIT;
    
	if( pStrTemp != NULL ) bson_free( pStrTemp );
    if( pDoc != NULL ) bson_destroy( pDoc );
    if( psCursor != NULL ) mongoc_cursor_destroy( psCursor );
	if( psQuery != NULL ) bson_destroy( psQuery );
	return result;
}

FunctionResult mongoFindDocument(char* buffer) // from user not system but can name filesys refs as ARGUMENT(2)
{
    unsigned int remainingSize = currentOutputLimit - (buffer - currentOutputBase) - 1;
	char* dot = strchr(ARGUMENT(1),'.');
	if (dot) *dot = 0;	 // terminate any suffix, not legal in mongo key
	FunctionResult result = mongoGetDocument(ARGUMENT(1),buffer,remainingSize,true);
	if (dot) *dot = '.';	 // terminate any suffix, not legal in mongo key
	return result;
}

FunctionResult mongoDeleteDocument(char* buffer) 
{
	char* dot = strchr(ARGUMENT(1),'.');
	if (dot) *dot = 0; // not allowed by mongo
    mongoc_database_t* db = g_filesysDatabase;
	mongoc_collection_t* collection;
	if (buffer)
    {
        db = g_pDatabase;
        collection = g_pCollection; // user script
    }
	else if (hasMongoCollectionHandle(ARGUMENT(2))) collection = getMongoCollectionHandle(ARGUMENT(2));
	else collection = g_filesysCollectionTopic;
    if (!collection)
    {
        char* msg = "DB is not open\r\n";
        SetUserVariable((char*)"$$mongo_error",msg);	// pass along the error
        Log(USERLOG,msg);
        return FAILRULE_BIT;
    }
    
    /* delete document from the database */
    char* keyname = ARGUMENT(1); // name the key
    eReturnValue eRetVal = eReturnValue_SUCCESS;
    bson_t* pDoc = NULL;
    do
    {
        if( keyname == NULL )
        {
            eRetVal = eReturnValue_WRONG_ARGUEMENTS;
            break;
        }
        
        pDoc = bson_new();
        if( pDoc == NULL )
        {
            eRetVal = eReturnValue_DATABASE_DOCUMENT_CREATION_FAILED;
            break;
        }
        
        BSON_APPEND_UTF8 ( pDoc, "KeyName", keyname );
#ifdef PRIVATE_CODE
        static HOOKPTR fnos = FindHookFunction((char*)"DatabaseOperationStart");
        if (fnos)
        {
            ((DatabaseOperationStartHOOKFN) fnos)("Mongo", mongoc_database_get_name(db), mongoc_collection_get_name(collection), "delete", keyname);
        }
#endif
        
        bson_error_t sError;
    	uint64 starttime = ElapsedMilliseconds();
        
        if (!mongoc_collection_remove(	collection, MONGOC_REMOVE_SINGLE_REMOVE, pDoc, NULL, &sError))
        {
            eRetVal = eReturnValue_DATABASE_DOCUMENT_DELETION_FAILED;
        }
        else mongoLogError(sError);
        uint64 endtime = ElapsedMilliseconds();
	    unsigned int diff = (unsigned int)endtime - (unsigned int)starttime;
	    unsigned int limit = 100;
		char* val = GetUserVariable("$mongo_timeexcess", false);
		if (*val) limit = unsigned(atoi(val));
	    if (diff >= limit){
	    	char dbmsg[512];
	    	struct tm ptm;
	    	sprintf(dbmsg,"%s Mongo Delete took longer than expected for %s, time taken = %ums\r\n", GetTimeInfo(&ptm),keyname, diff);
	    	Log(DBTIMELOG, dbmsg);
	    }
#ifdef PRIVATE_CODE
        static HOOKPTR fnoe = FindHookFunction((char*)"DatabaseOperationEnd");
        if (fnoe)
        {
            ((DatabaseOperationEndHOOKFN) fnoe)();
        }
#endif
    }while(false);
    
    if( pDoc != NULL ) bson_destroy( pDoc );
    
    if (eRetVal != eReturnValue_SUCCESS)
    {
        char* msg = "Error while insert document \r\n";
        SetUserVariable((char*)"$$mongo_error",msg);	// pass along the error
        Log(USERLOG, "Delete document failed with error: %d",  eRetVal);
        return FAILRULE_BIT;
    }
    return NOPROBLEM_BIT;
}

static FunctionResult MongoUpsertDoc(mongoc_collection_t* collection,char* keyname, char* value)
{// assumes mongo client and db set up - document is merely a json string ready to ship
	if( !keyname || !*keyname || !value) return FAILRULE_BIT;
	char* dot = strchr(keyname,'.');
	if (dot) *dot = 0; // not allowed by mongo
 
 	//  we output no text result
    if (!collection)
    {
        char* msg = "DB is not open\r\n";
        SetUserVariable((char*)"$$mongo_error",msg);    // pass along the error
        Log(USERLOG,msg);
        return FAILRULE_BIT;
    }
    
    /* insert/update document to the database */
    FunctionResult result = FAILRULE_BIT;
    bson_error_t error;
    bson_oid_t oid;
    bson_t *doc = NULL;
    bson_t *update = NULL;
    bson_t *query = NULL;
	bson_t child;
    bson_oid_init (&oid, NULL);
    
	query = bson_new();
	BSON_APPEND_UTF8 (query, "KeyName", keyname);
    // updates the mongo upsert query with key value pairs from $cs_mongoQueryParams
    mongoAppendKeys(query, (char*)"$cs_mongoQueryParams");
#ifdef PRIVATE_CODE
    // Check for private hook function to adjust the Mongo query parameters
    static HOOKPTR fnq = FindHookFunction((char*)"MongoQueryParams");
    if (fnq)
    {
        ((MongoQueryParamsHOOKFN) fnq)(query);
    }
    static HOOKPTR fnos = FindHookFunction((char*)"DatabaseOperationStart");
    if (fnos)
    {
        ((DatabaseOperationStartHOOKFN) fnos)("Mongo", mongoc_database_get_name(collection == g_pCollection ? g_pDatabase : g_filesysDatabase), mongoc_collection_get_name(collection), "upsert", keyname);
    }
#endif

    uint64 starttime = ElapsedMilliseconds();
	update = bson_new();
	BSON_APPEND_DOCUMENT_BEGIN (update, "$set", &child);
	BSON_APPEND_UTF8 (&child, "KeyName", keyname);
	BSON_APPEND_UTF8 (&child, "KeyValue", value);
	BSON_APPEND_DATE_TIME (&child, "lmodified", starttime);
	// updates the mongo upsert document with key value pairs from $cs_mongoKeyValues
    mongoAppendKeys(&child, (char*)"$cs_mongoKeyValues");
#ifdef PRIVATE_CODE
    // Check for private hook function to adjust the Mongo upsert parameters
    static HOOKPTR fnu = FindHookFunction((char*)"MongoUpsertKeyValues");
    if (fnu)
    {
        ((MongoUpsertKeyValuesHOOKFN) fnu)(&child);
    }
#endif

    bson_append_document_end(update, &child);
    if (mongoc_collection_update (collection, MONGOC_UPDATE_UPSERT, query, update, NULL, &error)) result = NOPROBLEM_BIT;
    else mongoLogError(error);
    
    if (doc) bson_destroy (doc);
    if (query) bson_destroy (query);
    if (update) bson_destroy (update);
    uint64 endtime = ElapsedMilliseconds();
    unsigned int diff = (unsigned int)(endtime - starttime);
    unsigned int limit = 100;
	char* val = GetUserVariable("$mongo_timeexcess", false);
	if (*val) limit = unsigned(atoi(val));
    if (diff >= limit){
    	char dbmsg[512];
    	struct tm ptm;
    	sprintf(dbmsg,"%s Mongo Upsert took longer than expected for %s, time taken = %ums\r\n", GetTimeInfo(&ptm),keyname, diff);
    	Log(DBTIMELOG, dbmsg);
    }
#ifdef PRIVATE_CODE
    static HOOKPTR fnoe = FindHookFunction((char*)"DatabaseOperationEnd");
    if (fnoe)
    {
        ((DatabaseOperationEndHOOKFN) fnoe)();
    }
#endif

    return result;
}

FunctionResult mongoInsertDocument(char* buffer)
{ // always comes from USER script, not the file system
	char* revisedBuffer = GetFileBuffer(); // use a filesystem buffer
	char* dot = strchr(ARGUMENT(1),'.');
	if (dot) *dot = 0;	 // terminate any suffix, not legal in mongo key
	strcpy(revisedBuffer,ARGUMENT(2)); // content to do
	FunctionResult result =  MongoUpsertDoc(g_pCollection,ARGUMENT(1),revisedBuffer);
	if (dot) *dot = '.';	 // terminate any suffix, not legal in mongo key
    FreeFileBuffer();	// release back to file system
	return result;
}

//====  mongo C client calls ===========================================================================================

void MongoUserFilesClose()
{
	FunctionResult result = MongoClose(NULL);
	InitUserFiles(); // default back to normal filesystem
}

FILE* mongouserCreate(const char* name) // pretend user topic filename a file
{
	return (FILE*)name;
}

FILE* mongouserOpen(const char* name) // pretend user topic filename a file
{
	return (FILE*)name;
}

int mongouserClose(FILE*)
{
	return 0;
}

size_t mongouserRead(void* buffer,size_t size, size_t count, FILE* file)
{ // file buffer passed to us. Read everything, ignore size/count
	mongoBuffer = (char*) buffer; // will be a filebuffer
	*mongoBuffer = 0;
	char* filename = (char*) file;
	char* dot = strchr(filename,'.');
	if (dot) *dot = 0;	 // terminate any suffix, not legal in mongo key
    char* filetype = getFileType(filename);
    if (hasMongoCollectionHandle(filetype)) ARGUMENT(2) = filetype;
	else ARGUMENT(2) = (char*)"topic";
	FunctionResult result = mongoGetDocument(filename,mongoBuffer,(userCacheSize - MAX_USERNAME),false);
	if (dot) *dot ='.';	 
	size_t len = strlen(mongoBuffer);
	return  len;
}

size_t mongouserWrite(const void* buffer,size_t size, size_t count, FILE* file)
{// writes topic files, export files
	// data is already mongo safe, except for possible cr/nl 
	char* mongoBuffer = (char*) buffer;
	ProtectNL(mongoBuffer); // replace cr/nl
	char* dot = strchr((char*)file,'.');
	char* keyname = (char*)file;
    char* filetype = getFileType(keyname);
	mongoc_collection_t* collection = g_filesysCollectionTopic;
    if (hasMongoCollectionHandle(filetype)) collection = getMongoCollectionHandle(filetype);
	if (dot) *dot = 0;	 // terminate any suffix, not legal in mongo key
	FunctionResult result = MongoUpsertDoc(collection,keyname, mongoBuffer);
	if (dot) *dot ='.';	 
	if (result != NOPROBLEM_BIT) 
	{
		ReportBug("INFO: Mongo filessys write failed for %s",keyname);
		return 0; // failed
	}
	return size * count; // is len a match
}

void MongoUserFilesInit() // start mongo as fileserver
{
	FunctionResult result = MongoInit(NULL); // files init
	if (result == NOPROBLEM_BIT)
	{
		// these are dynamically stored, so CS can be a DLL.
		userFileSystem.userCreate = mongouserCreate;
		userFileSystem.userOpen = mongouserOpen;
		userFileSystem.userClose = mongouserClose;
		userFileSystem.userRead = mongouserRead;
		userFileSystem.userWrite = mongouserWrite;
		filesystemOverride = MONGOFILES;
	}
	else 
	{
		ReportBug("INFO: Unable to open mongo fileserver");
		Log(SERVERLOG,"Unable to open mongo fileserver");
		(*printer)("Unable to open mongo fileserver");
	}
}

void MongoSystemInit(char* params) // required
{
	if (!params) return;
	char arg1[MAX_WORD_SIZE]; // url
	char arg2[MAX_WORD_SIZE]; // dbname
	char arg3[MAX_WORD_SIZE]; // topic , logsfolder, ltm
	params = ReadCompiledWord(params,arg1);
	params = ReadCompiledWord(params,arg2);
	ARGUMENT(1) = arg1;
	ARGUMENT(2) = arg2;
	while(*params)
	{
		params = ReadCompiledWord(params,arg3);
		if (!*arg3) break;
        char collectionname[MAX_WORD_SIZE];
        char *collectiondbname = NULL;
        strcpy(collectionname, arg3);
        char *split = strchr(collectionname, ':');
        if (split) {
            collectiondbname = split + 1;
            *split = 0;
		}
        if (collectionname && collectiondbname)
		{
            ARGUMENT(3) = collectiondbname;
            ARGUMENT(4) = collectionname;
		}
		else // old style
		{
			ARGUMENT(3) = arg3;
			ARGUMENT(4) = (char*)"topic";
		}		
		MongoUserFilesInit(); // init topic
	}
}

void MongoSystemShutdown() // required
{
	char buffer[10];
	mongoShutdown = true;
	MongoClose(buffer); // close user one not closed
	MongoUserFilesClose();
	if (mongoInited) mongoc_cleanup();
}
#endif     // END OF DISCARDMONGO

