#ifndef _MONGO_DB_CLIENT_H
#define _MONGO_DB_CLIENT_H

enum eReturnValue
{
	eReturnValue_FAILURE = 0,
	eReturnValue_SUCCESS = 1,
	eReturnValue_WRONG_ARGUEMENTS = 2,

	eReturnValue_DATABASE_OPEN_CLIENT_CONNECTION_FAILED = 3,
	eReturnValue_DATABASE_GET_FAILED = 4,
	eReturnValue_DATABASE_GET_COLLECTION_FAILED = 5,

	eReturnValue_DATABASE_CLIENT_CONNECTION_INVALID = 6,
	eReturnValue_DATABASE_INVALID = 7,
	eReturnValue_DATABASE_COLLECTION_INVALID = 8,

	eReturnValue_DATABASE_COMMAND_CREATION_FAILED = 9,
	eReturnValue_DATABASE_PING_COMMAND_FAILED = 10,

	eReturnValue_DATABASE_DOCUMENT_CREATION_FAILED = 11,
	eReturnValue_DATABASE_DOCUMENT_INSERT_FAILED = 12,

	eReturnValue_DATABASE_QUERY_CREATION_FAILED = 13,
	eReturnValue_DATABASE_QUERY_FAILED = 14,

	eReturnValue_DATABASE_DOCUMENT_DELETION_FAILED = 15
};

eReturnValue EstablishConnection(	const char* pStrSeverUri,
									const char* pStrDBName,
									const char* pStrCollName);
eReturnValue DestroyConnection();

eReturnValue InsertDocument(const char* pStrKeyName,
							const char* pStrKeyValue);

eReturnValue FindDocument(	const char* pStrKeyName,
							char** ppStrKeyValue);

eReturnValue DeleteDocument(const char* pStrKeyName);

#endif // _MONGO_DB_CLIENT_H

