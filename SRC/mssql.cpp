/** Interface routines for ODBC server, nominally MS SQL Server
 */ 

#include <stddef.h>
#include <stdio.h>
#include <time.h>

#ifdef __GNUC__ // ignore warnings from common.h
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#pragma GCC diagnostic ignored "-Wconversion-null"
#include "common.h"
#pragma GCC diagnostic pop

#else  // not __GNUC__ means Visual Studio
#include "common.h"
#endif // __GNUC__

#ifndef DISCARDMICROSOFTSQL      
#include "mssql_imp.h"
#include "ms_sql.h"

#undef TRACE_ON // sqlext.h redefines TRACE_ON as 1

#include <sql.h>
#include <sqlext.h>

#ifndef DISCARD_MSSQL_INTERFACE_FOR_TEST

static int conn_id = -1;               // identify connection
char mssqlUserFilename[MAX_WORD_SIZE]; // current topic file name for user
char mssqlparams[300];          // global variable storing params
static char mssqlhost[100];
static char mssqlport[100];
static char mssqluser[100];
static char mssqlpasswd[100];
static char mssqldatabase[100];
static const char* log_filename = "mssql_init_log.txt";

// library init
static bool mssqlInited = false;

// useful mssql commands (to use from sqlcmd). Always follow with newline, and GO
// SELECT * FROM INFORMATION_SCHEMA.TABLES;
// -- lists all tables defined in db(looking to see if userfiles is defined yet)

// create database chatscript; -- create the database
// USE chatscript;      --make this the current database
// CREATE TABLE userfiles (userid varchar(100) PRIMARY KEY, [file] VARBINARY(max), stored DATETIME DEFAULT CURRENT_TIMESTAMP)
// select * from information_schema.columns where table_name='userfiles'; -- lists names of fields and types.
// SELECT * from userfiles  to show entries

// DELETE  FROM userfiles WHERE DATE(stored) < DATE(NOW() - INTERVAL 1 DAY)
// Above deletes records 2 days or earlier. A user might converse just before midnight and then continue shortly thereafter, so we do not merely want a single day separator.
// DROP TABLE userfiles;        --destroy the entire collection of saved entries. Then recreate it.

static void GetMsSqlParams(char* params)
{
    size_t len = strlen(params);
    if (params[len - 1] == '"') params[len - 1] = 0;
    char* p = strstr(params, "host=");
    if (p) ReadCompiledWord(p + 5, mssqlhost);
    p = strstr(params, "port=");
    if (p) ReadCompiledWord(p + 5, mssqlport);
    p = strstr(params, "user=");
    if (p) ReadCompiledWord(p + 5, mssqluser);
    p = strstr(params, "password=");
    if (p) ReadCompiledWord(p + 9, mssqlpasswd);
    p = strstr(params, "database=");
    if (p) ReadCompiledWord(p + 9, mssqldatabase);
}

static FILE* GetUserFilenameFromName(const char* name)
{
        const char* at = strrchr(name, '/');
        if (at) strcpy(mssqlUserFilename, at + 1);
        else strcpy(mssqlUserFilename,name);

        return (FILE*)mssqlUserFilename;
}

// Declaration to prevent forward reference
static void SetUserFileSystemBlock(void);

////////////////////////////////////
// FILE SYSTEM REPLACEMENT FUNCTIONS
////////////////////////////////////

FILE* mssqlUserCreate(const char* name)
{ // unlike file system create, we dont create anything here, just note it
        return (FILE*)GetUserFilenameFromName(name);
}

FILE* mssqlUserOpen(const char* name)
{ // unlike file system open, we dont open anything here, just note it
        return (FILE*)GetUserFilenameFromName(name);
}

int mssqlUserClose(FILE*)
{ // since we never create or open anything, we dont have to close it
        return 0;
}

size_t mssqlUserRead(void* buf,size_t size, size_t count, FILE* key)
{ // read topic file record of user

    if (!mssqlInited) {
        ReportBug( "mssql not initialized yet" );
        return 0;
    }

    size_t buf_size = size * count;
    int rv = mssql_file_read(conn_id, buf, &buf_size, (const char*) key);
    if (rv) {
        ReportBug( mssql_error() );
        return 0;
    }
    
	return buf_size;
}

static size_t mssqlUserWrite(const void* buf, size_t size, size_t count, FILE* key)
{
    if (!mssqlInited) {
        ReportBug( "mssql not initialized yet" );
        return 0;
    }
    size_t buf_size = size * count;
    int rv = mssql_file_write(conn_id, buf, buf_size, (const char*) key);
    if (rv) {
        ReportBug( mssql_error() );
        return 0;
    }

    return buf_size; 
}

static bool save_string_to_mssql_log_file(const char* s)
{
    const bool store_to_file = true;
    if (store_to_file) {
        FILE* f = fopen(log_filename, "a");
        if (f != NULL) {
            time_t rawtime;
            struct tm* timeinfo;

            time(&rawtime);
            timeinfo = localtime(&rawtime);

            fprintf(f, "%s at %s\n", s, asctime(timeinfo));
            fclose(f);
        }
        else {
            return false;
        }
    }
    return true;
}

// initialize the user system
void MsSqlUserFilesCode(char* params)
{
    if (mssqlInited)   return;

    conn_id = mssql_get_id();
    if (conn_id < 0)  ReportBug((char*) mssql_error());

    GetMsSqlParams(params); 

    int rv = mssql_init(conn_id, mssqlhost, mssqlport,
                        mssqluser, mssqlpasswd, mssqldatabase);
    if (rv) {
        ReportBug((char*)"FATAL: Failed to connect to MSSQL: %s %s",
                  params, mssql_error());
        return;
    }

    SetUserFileSystemBlock();

    rv = save_string_to_mssql_log_file("init mssql");
    if (!rv) {
        ReportBug((char*)"FATAL: Unable to save mssql log file: %s",
                  log_filename);
        return;
    }

    mssqlInited = true;
}

void MsSqlFullCloseCode(bool restart)
{
    if (!mssqlInited) {
        ReportBug((char*) "Closing uninitialized block %s", mssql_error());
        return; // closing maybe when failed to init and reported bug then
    }

    int rv = save_string_to_mssql_log_file("close mssql");
    if (rv) {
        ReportBug((char*)"FATAL: Unable to save mssql log file: %s",
                  log_filename);
        return;
    }

    if (conn_id != -1) mssql_close(conn_id);
    conn_id = -1;
    mssqlInited = false;
}

static void SetUserFileSystemBlock(void)
{
    // these are dynamically stored, so CS can be a DLL.
    userFileSystem.userCreate = mssqlUserCreate;
    userFileSystem.userOpen = mssqlUserOpen;
    userFileSystem.userClose = mssqlUserClose;
    userFileSystem.userRead = mssqlUserRead;
    userFileSystem.userWrite = mssqlUserWrite;
    userFileSystem.userDelete = NULL;
    filesystemOverride = MICROSOFTSQLFILES;
}

////////////////////////////////////
// SCRIPT ACCESS FUNCTIONS
////////////////////////////////////

FunctionResult MsSqlInitCode(char* buffer)
{
    return FAILRULE_BIT;
}

FunctionResult MsSqlCloseCode(char* buffer)
{
    return FAILRULE_BIT;
}

FunctionResult MsSqlQueryCode(char* buffer)
{
    return FAILRULE_BIT;
}

#endif // ifndef DISCARD_MSSQL_INTERFACE_FOR_TEST

/** Low level functions for interfacing to ODBC */

struct DbInterface_t {
    int used; 
    SQLHENV henv;      // Environment handle
    SQLHDBC hdbc;      // Connection handle
    SQLHSTMT hstmt_w;  // Write statement handle
    SQLHSTMT hstmt_r;  // Read statement handle
    bool write_prepared;
    bool write_parameters_bound;
    bool read_prepared;
    bool read_parameters_bound;
    bool use_stored_procedures;
    bool use_tracing;
    bool verbose;
    void* file_buf;             // storage for file data
    SQLLEN file_buf_size;       // allocated size
    SQLLEN file_buf_len;        // how much is used
    void* key_buf;              // storage for key
    SQLLEN key_buf_size;        // allocated size
    SQLLEN key_buf_len;         // how much is used
    bool is_empty;              // no data in result set
};

static struct DbInterface_t db_list[2]; // one for file, one for scripting.
static char error_buf[1024];            // stores error statements
static char in_conn_str[300];      // connection string built by us
static char out_conn_str[300];     // connection string returned by MS
static char tracing_filename[100];

const char* get_mssql_in_conn_str(void)
{
    return in_conn_str;
}

const char* get_mssql_out_conn_str(void)
{
    return out_conn_str;
}

const char* mssql_error(void)
{
    return error_buf;
}

static void replace_char(char* str, char find, char replace)
{
    char *current_pos = strchr(str,find);
    while (current_pos){
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
}

static void extract_error(const char *fn, SQLHANDLE handle, SQLSMALLINT type)
{
    SQLSMALLINT i = 0;
    SQLINTEGER NativeError;
    SQLCHAR SQLState[7];
    SQLCHAR MessageText[256];
    SQLSMALLINT TextLength;
    SQLRETURN ret;

    snprintf(error_buf, sizeof(error_buf),
             "\nThe driver reported the following error %s\n", fn);
    do
    {
        ret = SQLGetDiagRec(type,         // SQLSMALLINT HandleType,
                            handle,       // SQLHANDLE Handle
                            ++i,          // SQLSMALLINT RecNumber
                            SQLState,     // SQLCHAR * SQLState
                            &NativeError, // SQLINTEGER * NativeErrorPtr
                            MessageText,  // SQLCHAR * MessageText
                            sizeof(MessageText), // SQLSMALLINT BufferLength
                            &TextLength); // SQLSMALLINT * TextLengthPtr
        if (SQL_SUCCEEDED(ret)) {
            snprintf(error_buf + strlen(error_buf), sizeof(error_buf),
                     "%s:%ld:%ld:%s\n", 
                     SQLState, (long)i, (long)NativeError, MessageText);
        }
    } while (ret == SQL_SUCCESS);

    replace_char(error_buf, '\n', '*'); // some scripts fail on new lines
    replace_char(error_buf, '\r', '*');
}

static bool is_error(SQLRETURN e, const char* s, SQLHANDLE h, SQLSMALLINT t)
{
    if ((e == SQL_SUCCESS) || (e == SQL_SUCCESS_WITH_INFO)) {
        error_buf[0] = '\0';
        return false;
    }

    extract_error(s, h, t);
    return true;
}

static void* getHeap(size_t size) {
    return malloc(size);
}

static void releaseHeap(void* ptr) {
    free(ptr);
}

static bool init_db(DbInterface_t* db)
{
    db->verbose = false;

    db->file_buf_size = (userCacheSize + OVERFLOW_SAFETY_MARGIN);
    db->file_buf = nullptr;

    db->key_buf_size = 100;
    db->key_buf = nullptr;

    db->henv = SQL_NULL_HENV;   // Environment handle
    db->hdbc = SQL_NULL_HDBC;   // Connection handle
    db->hstmt_w = SQL_NULL_HSTMT;  // Write statement handle
    db->hstmt_r = SQL_NULL_HSTMT;  // Write statement handle
    db->write_prepared = false;
    db->write_parameters_bound = false;
    db->read_prepared = false;
    db->read_parameters_bound = false;
    db->use_stored_procedures = true;
    db->use_tracing = false;
    db->used = 1;

    return true;
}

int mssql_get_id(void)
{
    size_t i;
    for (i = 0; i < sizeof(db_list) / sizeof(db_list[0]); ++i) {
        struct DbInterface_t* db = &db_list[i];
        if (db->used == 0) {
            bool ok = init_db(db);
            if (ok)  return i;
        }
    }
    return -1;
}

static DbInterface_t* get_db(int id)
{
    int array_size = sizeof(db_list)/sizeof(db_list[0]);
    if ((id < 0) || (array_size <= id))  return nullptr;

    DbInterface_t* db = &db_list[id];
    return db;
}

int mssql_set_verbose(int id, bool flag)
{
    DbInterface_t* db = get_db(id);
    if (db != nullptr) {
        db->verbose = flag;
        return 0;
    }
    return -1;
}

int mssql_use_stored_procedures(int id, bool flag)
{
    DbInterface_t* db = get_db(id);
    if (db != nullptr) {
        db->use_stored_procedures = flag;
        return 0;
    }
    return -1;
}

static bool free_statement_handle(DbInterface_t* db)
{
    SQLRETURN rv;
    if (db->hstmt_w != SQL_NULL_HSTMT) {
        rv = SQLFreeHandle(SQL_HANDLE_STMT, db->hstmt_w);
        if (is_error(rv, "SQLFreeHandle", db->hstmt_w, SQL_HANDLE_STMT)) {
            return false;
        }
        db->hstmt_w = nullptr;
    }
    if (db->hstmt_r != SQL_NULL_HSTMT) {
        rv = SQLFreeHandle(SQL_HANDLE_STMT, db->hstmt_r);
        if (is_error(rv, "SQLFreeHandle", db->hstmt_r, SQL_HANDLE_STMT)) {
            return false;
        }
        db->hstmt_r = nullptr;
    }
    return true;
}

static bool free_connection_handle(DbInterface_t* db)
{
    SQLRETURN rv;
    if (db->hdbc != SQL_NULL_HDBC) {
        rv = SQLDisconnect(db->hdbc);
        if (is_error(rv, "SQLDisconnect", db->hdbc, SQL_HANDLE_DBC)) {
            return false;
        }
        rv = SQLFreeHandle(SQL_HANDLE_DBC, db->hdbc);
        if (is_error(rv, "SQLFreeHandle", db->hdbc, SQL_HANDLE_DBC)) {
            return false;
        }
        db->hdbc = nullptr;
    }
    return true;
}

static bool free_environment_handle(DbInterface_t* db)
{
    SQLRETURN rv;
    if (db->henv != SQL_NULL_HENV) {
        rv = SQLFreeHandle(SQL_HANDLE_ENV, db->henv);
        if (is_error(rv, "SQLFreeHandle", db->henv, SQL_HANDLE_ENV)) {
            return false;
        }
        db->henv = nullptr;
    }
    return true;

}

static bool deinit_database_interface_struct(DbInterface_t* db)
{
    releaseHeap(db->file_buf);
    db->file_buf = nullptr;
    db->file_buf_size = 0;

    releaseHeap(db->key_buf);
    db->key_buf = nullptr;
    db->key_buf_size = 0;

    db->write_prepared = false;
    db->write_parameters_bound = false;
    db->read_prepared = false;

    db->used = 0;

    return true;
}

int mssql_close(int id)
{
    DbInterface_t* db = get_db(id);
    bool ok = (db != nullptr);

    ok = ok & free_statement_handle(db);
    ok = ok & free_connection_handle(db);
    ok = ok & free_environment_handle(db);
    ok = ok & deinit_database_interface_struct(db);

    int rv = (ok ? 0 : -1);

    return rv;
}

static bool alloc_buffers(DbInterface_t* db)
{
    db->file_buf = getHeap(db->file_buf_size);
    if (db->file_buf == nullptr) {
        snprintf(error_buf, sizeof(error_buf),
                 "Unable to allocate %ld bytes for file buffer",
                 db->file_buf_size);
        return false;
    }

    db->key_buf = getHeap(db->key_buf_size);
    if (db->key_buf == nullptr) {
        snprintf(error_buf, sizeof(error_buf),
                 "Unable to allocate %ld bytes for key buffer",
                 db->file_buf_size);
        return false;
    }
    return true;
}
static bool alloc_environment_handle(DbInterface_t* db)
{
    SQLRETURN retcode;
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &db->henv);
    if (is_error(retcode, "SQLAllocHandle(SQL_HANDLE_ENV)",
                 db->henv, SQL_HANDLE_ENV)) {
        return false;
    }
    return true;
}

static bool set_ODBC_version(DbInterface_t* db)
{
    SQLRETURN retcode;
    retcode = SQLSetEnvAttr(db->henv, SQL_ATTR_ODBC_VERSION,
                            (SQLPOINTER*)SQL_OV_ODBC3, 0);
    if (is_error(retcode, "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)",
                 db->henv, SQL_HANDLE_ENV)) {
        return false;
    }
    return true;
}

static bool alloc_connection_handle(DbInterface_t* db)
{
    SQLRETURN retcode;
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, db->henv, &db->hdbc);
    if (is_error(retcode, "SQLAllocHandle(SQL_HANDLE_DBC)",
                 db->hdbc, SQL_HANDLE_DBC)) {
        return false;
    }

    return true;
}

static bool set_login_timeout(DbInterface_t* db)
{
    // I know the below does not make sense because
    // a number is being used as a pointer,
    // but it comes directly from this MS page:
    // https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlconnect-function?view=sql-server-ver15
    SQLRETURN retcode;
    retcode = SQLSetConnectAttr(db->hdbc,
                                SQL_LOGIN_TIMEOUT,
                                (SQLPOINTER)5, // login time in s
                                0);
    if (is_error(retcode, "SQLSetConnectAttr(SQL_LOGIN_TIMEOUT)",
                 db->hdbc, SQL_HANDLE_DBC)) {
        return false;
    }
    return true;
}

static bool set_tracing(DbInterface_t* db) {
    SQLRETURN retcode;
    if (db->use_tracing) {
        retcode = SQLSetConnectAttr(db->hdbc,
                                    SQL_ATTR_TRACEFILE,
                                    tracing_filename,
                                    SQL_NTS);
        if (is_error(retcode, "SQLSetConnectAttr - SQL_ATTR_TRACEFILE",
                     db->hdbc, SQL_HANDLE_DBC)) {
            return false;
        }

        retcode = SQLSetConnectAttr(db->hdbc,
                                    SQL_ATTR_TRACE,
                                    (void*)SQL_OPT_TRACE_ON,
                                    SQL_NTS);    
        if (is_error(retcode, "SQLSetConnectAttr - SQL_ATTR_TRACE",
                     db->hdbc, SQL_HANDLE_DBC)) {
            return false;
        }
    }
    else {
        retcode = SQLSetConnectAttr(db->hdbc,
                                    SQL_ATTR_TRACE,
                                    (void*)SQL_OPT_TRACE_OFF,
                                    SQL_NTS);
        if (is_error(retcode, "SQLSetConnectAttr - SQL_ATTR_TRACE",
                     db->hdbc, SQL_HANDLE_DBC)) {
            return false;
        }
    }

    return true;
}

static bool connect(DbInterface_t* db,
                    const char* host,
                    const char* host_port,
                    const char* user,
                    const char* pwd,
                    const char* database)
{
    SQLRETURN retcode;
    SQLSMALLINT outstrlen;
    const char *driver = "ODBC Driver 17 for SQL Server";
    snprintf(in_conn_str, sizeof(in_conn_str),
             "DRIVER=%s; SERVER=%s,%s; DATABASE=%s; UID=%s; PWD=%s",
             driver, host, host_port, database, user, pwd);
    if (db->verbose) {
        printf("in_conn_str %s\n", in_conn_str);
    }
    HWND dhandle = nullptr;
    SQLSMALLINT in_len = (SQLSMALLINT) strlen(in_conn_str);
    outstrlen = sizeof(out_conn_str);
    retcode = SQLDriverConnect(db->hdbc, // SQLHDBC ConnectionHandle
                               dhandle,  // SQLHWND WindowHandle
                               (SQLCHAR*)in_conn_str, // SQLCHAR * InConnectionString
                               in_len, // SQLSMALLINT StringLength1
                               (SQLCHAR*)out_conn_str, // SQLCHAR * OutConnectionString
                               outstrlen, // SQLSMALLINT BufferLength
                               &outstrlen, // SQLSMALLINT * StringLength2Ptr
                               SQL_DRIVER_COMPLETE); // SQLUSMALLINT DriverCompletion
    if (db->verbose) {
        printf("connect outstrlen %d\n", outstrlen);
        if (0 < outstrlen) {
            printf("outstr: %s\n", out_conn_str);
        }
    }
    if (retcode == SQL_SUCCESS) {
        if (db->verbose) {
            printf("Connect: SQL_SUCCESS\n");
        }
        return true;
    }
    else if (retcode == SQL_SUCCESS_WITH_INFO) {
        if (db->verbose) {
            extract_error("SQLDriverConnect", db->hdbc, SQL_HANDLE_DBC);
            printf("Got SQL_SUCCESS_WITH_INFO %s\n", error_buf);
        }
        return true;
    }

    is_error(retcode, "SQLDriverConnect", db->hdbc, SQL_HANDLE_DBC);

    return false;
}

static bool alloc_statement_handles(DbInterface_t* db)
{
    SQLRETURN retcode;
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, db->hdbc, &db->hstmt_w);
    if (is_error(retcode, "SQLAllocHandle(SQL_HANDLE_STMT)",
                 db->hstmt_w, SQL_HANDLE_STMT)) {
        return false;
    }
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, db->hdbc, &db->hstmt_r);
    if (is_error(retcode, "SQLAllocHandle(SQL_HANDLE_STMT)",
                 db->hstmt_r, SQL_HANDLE_STMT)) {
        return false;
    }
    return true;
}

int mssql_init(int id,
               const char* host,
               const char* host_port,
               const char* user,
               const char* pwd,
               const char* database)
{
    DbInterface_t* db = get_db(id);
    bool ok = (db != nullptr);
    ok = ok && alloc_buffers(db);
    ok = ok && alloc_environment_handle(db);
    ok = ok && set_ODBC_version(db);
    ok = ok && alloc_connection_handle(db);
    ok = ok && set_login_timeout(db);
    ok = ok && set_tracing(db);
    ok = ok && connect(db, host, host_port, user, pwd, database);
    ok = ok && alloc_statement_handles(db);
    return (ok ? 0 : -1);
}

static bool empty_result_set(SQLHSTMT hstmt)
{
    while (SQLMoreResults(hstmt) == SQL_SUCCESS) {
        ;
    }
    return true;
}

static bool file_direct_write(DbInterface_t* db,
                              const void* buf,
                              size_t buf_size,
                              const char* key)
{
    SQLRETURN retcode;

    SQLULEN p1_column_size = 100;
    SQLSMALLINT p1_digits = 0;
    static SQLLEN lenP1 = 0;
    lenP1 = strlen(key);
    retcode = SQLBindParameter(db->hstmt_w,      // StatementHandle
                               1,                // ParameterNumber
                               SQL_PARAM_INPUT,  // InputOutputType
                               SQL_C_CHAR,       // ValueType
                               SQL_VARCHAR,      // ParameterType
                               p1_column_size,   // ColumnSize
                               p1_digits,        // DecimalDigits
                               (SQLPOINTER) key, // ParameterValuePtr
                               lenP1,            // BufferLength
                               &lenP1);          // StrLen_or_IndPtr
    if (is_error(retcode, "SQLBindParameter", db->hstmt_w, SQL_HANDLE_STMT)) {
        printf("%s\n", error_buf);
        return false;
    }

    memcpy(db->file_buf, buf, buf_size);
    db->file_buf_len = buf_size;

    SQLULEN p2_column_size = db->file_buf_len;
    SQLSMALLINT p2_digits = 0;
    static SQLLEN lenP2 = 0;
    lenP2 = db->file_buf_len;
    SQLPOINTER src_buf = (SQLPOINTER) db->file_buf;
    retcode = SQLBindParameter(db->hstmt_w,      // StatementHandle
                               2,                // ParameterNumber
                               SQL_PARAM_INPUT,  // InputOutputType
                               SQL_C_BINARY,     // ValueType
                               SQL_VARBINARY,    // ParameterType
                               p2_column_size,   // ColumnSize
                               p2_digits,        // DecimalDigits
                               src_buf,          // ParameterValuePtr
                               lenP2,            // BufferLength
                               &lenP2);          // StrLen_or_IndPtr
    if (is_error(retcode, "SQLBindParameter", db->hstmt_w, SQL_HANDLE_STMT)) {
        printf("%s\n", error_buf);
        return false;
    }

    const char* query2 = ""
        "{ CALL uspSetUserData( ?, ? ) }";

    retcode = SQLExecDirect(db->hstmt_w, (SQLCHAR*)query2, SQL_NTS);
    if (is_error(retcode, "SQLExecDirect", db->hstmt_w, SQL_HANDLE_STMT)) {
        printf("%s\n", error_buf);
        return false;
    }

    SQLLEN row_count = 0;
    retcode = SQLRowCount(db->hstmt_w, &row_count);
    if (is_error(retcode, "SQLRowCount", db->hstmt_w, SQL_HANDLE_STMT)) {
        printf("%s\n", error_buf);
        return false;
    }

    return true;
}

static bool file_write_preformat_data(DbInterface_t* db, const void* buf, size_t buf_size)
{
    size_t cushion = 20;
    if (db->file_buf_size < (SQLLEN)(buf_size + cushion)) {
        sprintf(error_buf, "file too big. file size %zu", buf_size);
        return false;
    }

    memcpy(db->file_buf, buf, buf_size);
    db->file_buf_len = buf_size;
    if (db->verbose) {
        printf("write size %ld\n", db->file_buf_len);
    }
    
    return true;
}

static bool file_write_bind_params(DbInterface_t* db)
{
    if (!db->write_parameters_bound) { // only bind once
        SQLRETURN retcode;

        SQLULEN p1_column_size = 100;
        SQLSMALLINT p1_digits = 0;
        db->key_buf_len = db->key_buf_size;
        retcode = SQLBindParameter(db->hstmt_w,      // StatementHandle
                                   1,                // ParameterNumber
                                   SQL_PARAM_INPUT,  // InputOutputType
                                   SQL_C_CHAR,       // ValueType
                                   SQL_VARCHAR,      // ParameterType
                                   p1_column_size,   // ColumnSize
                                   p1_digits,        // DecimalDigits
                                   (SQLPOINTER) db->key_buf, // ParameterValuePtr
                                   db->key_buf_size,            // BufferLength
                                   &db->key_buf_len);          // StrLen_or_IndPtr
        if (is_error(retcode, "SQLBindParameter", db->hstmt_w, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }

        SQLULEN p2_column_size = db->file_buf_size;
        SQLSMALLINT p2_digits = 0;
        retcode = SQLBindParameter(db->hstmt_w,      // StatementHandle
                                   2,                // ParameterNumber
                                   SQL_PARAM_INPUT,  // InputOutputType
                                   SQL_C_BINARY,     // ValueType
                                   SQL_LONGVARBINARY, // ParameterType
                                   p2_column_size,   // ColumnSize
                                   p2_digits,        // DecimalDigits
                                   db->file_buf,     // ParameterValuePtr
                                   db->file_buf_len, // BufferLength, ignored for writes
                                   &db->file_buf_len); // StrLen_or_IndPtr
        if (is_error(retcode, "SQLBindParameter", db->hstmt_w, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }
        db->write_parameters_bound = true;
    }

    return true;
}

static bool file_write_copy_key(DbInterface_t* db, const char* key)
{
    memcpy(db->key_buf, key, db->key_buf_len);
    db->key_buf_len = strlen(key);
    return true;
}


static bool file_write_prepare(DbInterface_t* db)
{
    if (!db->write_prepared) {  // only prepare once
        const char* upsert_sp = ""
            "MERGE INTO userfiles AS t "
            "  USING  "
            "    (SELECT pk=?, f1= ?) AS s "
            "  ON t.userid = s.pk "
            "  WHEN MATCHED THEN "
            "    UPDATE SET userid=s.pk, [file]=s.f1 "
            "  WHEN NOT MATCHED THEN "
            "    INSERT (userid, [file]) "
            "    VALUES (s.pk, s.f1); "
            "";

        const char* upsert_sql = ""
            "{ CALL uspSetUserData( ?, ? ) }";

        const char* upsert = (db->use_stored_procedures ? upsert_sp : upsert_sql);
        SQLRETURN retcode;
    
        retcode = SQLPrepare(db->hstmt_w, (SQLCHAR*)upsert, SQL_NTS);
        if (is_error(retcode, "SQLPrepare", db->hstmt_w, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }

        SQLSMALLINT num_params;
        SQLNumParams(db->hstmt_w, &num_params);

        int expected_num_params = 2;
        if (num_params != expected_num_params) {
            return false;
        }
        db->write_prepared = true;
    }
    
    return true;
}

static bool file_write_execute(DbInterface_t* db)
{
    SQLRETURN retcode;

    retcode = SQLExecute(db->hstmt_w);
    if (is_error(retcode, "SQLExecute", db->hstmt_w, SQL_HANDLE_STMT)) {
        printf("%s\n", error_buf);
        return false;
    }
    return true;
}

int mssql_file_write(int id, const void* buf, size_t buf_size, const char* key)
{
    DbInterface_t* db = get_db(id);
    bool ok = (db != nullptr);

    bool use_direct = false;
    if (use_direct) {
        ok = ok && file_direct_write(db, buf, buf_size, key);
        ok = ok && empty_result_set(db->hstmt_w);
    }
    else {
        ok = ok && file_write_preformat_data(db, buf, buf_size);
        ok = ok && file_write_prepare(db);
        ok = ok && file_write_bind_params(db);
        ok = ok && file_write_copy_key(db, key);
        ok = ok && file_write_execute(db);
        ok = ok && empty_result_set(db->hstmt_w);
    }

    int ret_val = (ok ? 0 : -1);
    return ret_val;
}

static bool file_direct_read(DbInterface_t* db, void* buf, size_t* buf_size_p, const char* key)
{
    SQLRETURN retcode;

    db->key_buf_len = strlen(key);
    memcpy(db->key_buf, key, db->key_buf_len);
    db->file_buf_len = db->file_buf_size;

    SQLULEN p1_column_size = 100;
    SQLSMALLINT p1_digits = 0;
    retcode = SQLBindParameter(db->hstmt_r,      // StatementHandle
                               1,                // ParameterNumber
                               SQL_PARAM_INPUT,  // InputOutputType
                               SQL_C_CHAR,       // ValueType
                               SQL_VARCHAR,      // ParameterType
                               p1_column_size,   // ColumnSize
                               p1_digits,        // DecimalDigits
                               (SQLPOINTER) db->key_buf, // ParameterValuePtr
                               db->key_buf_len,  // BufferLength
                               &db->key_buf_len); // StrLen_or_IndPtr
    if (is_error(retcode, "SQLBindParameter", db->hstmt_r, SQL_HANDLE_STMT)) {
        printf("%s\n", error_buf);
        return false;
    }

    const char* query_sql = "SELECT [file] from userfiles WHERE userid = ? ";
    const char* query_sp = "{ CALL uspGetUserData( ? ) }";
    const char* query = (db->use_stored_procedures ? query_sp : query_sql);

    retcode = SQLExecDirect(db->hstmt_r, (SQLCHAR*)query, SQL_NTS);
    if (is_error(retcode, "SQLExecDirect", db->hstmt_r, SQL_HANDLE_STMT)) {
        printf("%s\n", error_buf);
        return false;
    }

    if (retcode == SQL_SUCCESS_WITH_INFO) {
        printf("sql success with info %s\n", error_buf);

        SQLSMALLINT i = 0;
        SQLINTEGER NativeError;
        SQLCHAR SQLState[7];
        SQLCHAR MessageText[256];
        SQLSMALLINT TextLength;
        SQLRETURN ret;
        do {
            ret = SQLGetDiagRec(SQL_HANDLE_STMT, // SQLSMALLINT HandleType,
                                db->hstmt_r,     // SQLHANDLE Handle
                                ++i,      // SQLSMALLINT RecNumber
                                SQLState, // SQLCHAR * SQLState
                                &NativeError, // SQLINTEGER * NativeErrorPtr
                                MessageText,  // SQLCHAR * MessageText
                                sizeof(MessageText), // SQLSMALLINT BufferLength
                                &TextLength); // SQLSMALLINT * TextLengthPtr
            if (SQL_SUCCEEDED(ret)) {
                printf("%s:%ld:%ld:%s\n", 
                       SQLState, (long)i, (long)NativeError, MessageText);
            }
            else {
                printf("huh?\n");
            }
        } while (ret == SQL_SUCCESS);
    }

    bool in_result_set = true;
    db->is_empty = true;
    while (in_result_set) {
        retcode = SQLFetch(db->hstmt_r);
        if (retcode == SQL_NO_DATA) {
            in_result_set = false;
            continue;
        }
        else if (is_error(retcode, "SQLFetch", db->hstmt_r, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }
    
        retcode = SQLGetData(db->hstmt_r,  // Statement handle
                             1,            // Column number
                             SQL_C_BINARY, // TargetType, the C type
                             (SQLPOINTER) db->file_buf, // TargetValudPtr
                             (SQLLEN) db->file_buf_size, // BufferLength
                             (SQLLEN*) &db->file_buf_len  // strlen_or_ind
                             );
        if (is_error(retcode, "SQLGetData", db->hstmt_r, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }

        db->is_empty = false;
    }

    if (db->is_empty) {
        *buf_size_p = 0;
        ((char*)buf)[0] = '\0';
    }
    else {
        *buf_size_p = db->file_buf_len;
        memcpy(buf, db->file_buf, db->file_buf_len);
    }

    return true;
}

static bool file_read_init(DbInterface_t* db, const char* key)
{
    db->is_empty = false;

    db->key_buf_len = strlen(key);
    memcpy(db->key_buf, key, db->key_buf_len);

    db->file_buf_len = db->file_buf_size;

    return true;
}

static bool file_read_bind(DbInterface_t* db, const char* key)
{
    if (!db->read_parameters_bound) {
        SQLRETURN retcode;
        SQLULEN p1_column_size = 100;
        SQLSMALLINT p1_digits = 0;
        retcode = SQLBindParameter(db->hstmt_r,      // StatementHandle
                                   1,                // ParameterNumber
                                   SQL_PARAM_INPUT,  // InputOutputType
                                   SQL_C_CHAR,       // ValueType
                                   SQL_VARCHAR,      // ParameterType
                                   p1_column_size,   // ColumnSize
                                   p1_digits,        // DecimalDigits
                                   (SQLPOINTER) db->key_buf, // ParameterValuePtr
                                   db->key_buf_len,  // BufferLength
                                   &db->key_buf_len); // StrLen_or_IndPtr
        if (is_error(retcode, "SQLBindParameter", db->hstmt_r, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }
    }
    
    return true;
}

static bool file_read_prepare(DbInterface_t* db)
{
    if (!db->read_prepared) {
        const char* query_sql = "" 
            "SELECT [file] from userfiles \n"
            "WHERE userid = ? ";
        const char* query_sp = ""
            "{ CALL uspGetUserData( ? ) } ";
        const char *query = (db->use_stored_procedures ? query_sp : query_sql);

        SQLRETURN retcode;
        retcode = SQLPrepare(db->hstmt_r, (SQLCHAR*)query, SQL_NTS);
        if (is_error(retcode, "SQLPrepare", db->hstmt_r, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }

        db->read_prepared = true;
    }
    
    return true;
}

static bool file_read_execute(DbInterface_t* db)
{
    SQLRETURN retcode;
    retcode = SQLExecute(db->hstmt_r);
    if (is_error(retcode, "SQLExecute", db->hstmt_r, SQL_HANDLE_STMT)) {
        printf("%s\n", error_buf);
        return false;
    }

    if (retcode == SQL_SUCCESS_WITH_INFO) {
        printf("sql success with info %s\n", error_buf);

        SQLSMALLINT i = 0;
        SQLINTEGER NativeError;
        SQLCHAR SQLState[7];
        SQLCHAR MessageText[256];
        SQLSMALLINT TextLength;
        SQLRETURN ret;

        do {
            ret = SQLGetDiagRec(SQL_HANDLE_STMT, // SQLSMALLINT HandleType,
                                db->hstmt_r,// SQLHANDLE Handle
                                ++i,        // SQLSMALLINT RecNumber
                                SQLState,   // SQLCHAR * SQLState
                                &NativeError, // SQLINTEGER * NativeErrorPtr
                                MessageText,  // SQLCHAR * MessageText
                                sizeof(MessageText), // SQLSMALLINT BufferLength
                                &TextLength); // SQLSMALLINT * TextLengthPtr
            if (SQL_SUCCEEDED(ret)) {
                printf("%s:%ld:%ld:%s\n", 
                       SQLState, (long)i, (long)NativeError, MessageText);
            }
            else {
                printf("Bizarre response from SQLGetDiagRec %d\n", ret);
                return false;
            }
        } while (ret == SQL_SUCCESS);
    }

    return true;
}

static bool file_read_fetch_and_get_data(DbInterface_t* db, size_t* buf_size_p)
{
    db->is_empty = true;
    
    SQLRETURN retcode;
    int cycle_count = 0;
    const int max_cycle_count = 5;
    while (cycle_count < max_cycle_count) { // handles multiple result sets
        cycle_count++;
        retcode = SQLFetch(db->hstmt_r);
        if (retcode == SQL_NO_DATA) {
            cycle_count = max_cycle_count;
            continue;
        }
    
        if (is_error(retcode, "SQLFetch", db->hstmt_r, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }

        retcode = SQLGetData(db->hstmt_r,  // Statement handle
                             1,            // Column number
                             SQL_C_BINARY, // TargetType, the C type
                             (SQLPOINTER) db->file_buf, // TargetValudPtr
                             (SQLLEN) db->file_buf_size, // BufferLength
                             (SQLLEN*) &db->file_buf_len  // strlen_or_ind
                             );
        if (is_error(retcode, "SQLGetData", db->hstmt_r, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }
        if (0 < db->file_buf_len) {
            db->is_empty = false;
        }
    }
    return true;
}

static bool file_read_postformat_data(DbInterface_t* db, void* read_buf, size_t* read_len_p)
{
    if (!db->is_empty) {
        int cushion = 20;               // extra chars for bad compression
        if ((SQLLEN)(*read_len_p + cushion) < db->file_buf_len) {
            sprintf(error_buf, "Buffer too small for read: %zu vs %ld",
                    *read_len_p, db->file_buf_len);
            return false;
        }

        memcpy(read_buf, db->file_buf, db->file_buf_len);
        *read_len_p = db->file_buf_len;
        if (db->verbose) {
            printf("read size %ld\n", db->file_buf_len);
        }
    }
    else {
        *read_len_p = 0;
    }
        
    ((char*)read_buf)[*read_len_p] = '\0'; // null terminal string
    return true;
}

static bool file_read_with_prepare_and_execute(DbInterface_t* db,
                                               void* read_buf,
                                               size_t* read_len_p,
                                               const char* key)
{
    bool ok = true;
    ok = ok && file_read_init(db, key);
    ok = ok && file_read_bind(db, key);
    ok = ok && file_read_prepare(db);
    ok = ok && file_read_execute(db);
    ok = ok && file_read_fetch_and_get_data(db, read_len_p);
    ok = ok && file_read_postformat_data(db, read_buf, read_len_p);
    ok = ok && empty_result_set(db->hstmt_r);

    return ok;
}

int mssql_file_read(int id, void* read_buf, size_t* read_len_p, const char* key)
{
    DbInterface_t* db = get_db(id);
    bool ok = (db != nullptr);

    bool use_direct = false;
    if (use_direct) {
        ok = ok && file_direct_read(db, read_buf, read_len_p, key);
        empty_result_set(db->hstmt_r);
    }
    else   ok = ok && file_read_with_prepare_and_execute(db, read_buf, read_len_p, key);
   

    int ret_val = (ok ? 0 : -1);
    return ret_val;
}

int mssql_set_tracing(int id, bool flag, const char* filename)
{
    DbInterface_t* db = get_db(id);
    if (db == nullptr) {
        sprintf(error_buf, "id not acceptable\n");
        return -1;
    }
        
    db->use_tracing = flag;
    if (flag) {
        if (filename != nullptr) {
            strcpy(tracing_filename, filename);
            if (db->hdbc != SQL_NULL_HDBC) {
                bool ok = set_tracing(db);
                if (!ok) {
                    return -1;
                }
            }
        }
        else {
            memset(tracing_filename, '\0', sizeof(tracing_filename));
            sprintf(error_buf, "bad filename\n");
            return -1;
        }
    }
    return 0;
}

int mssql_exec(int id, const char* statement)
{
    return -1;
}

int mssql_store_result(int id)
{
    return -1;
}

int mssql_num_fields(int id)
{
    return -1;
}

int mssql_free_result(int id)
{
    return -1;
}

int mssql_num_rows(int id)
{
    return -1;
}

int mssql_fetch_row(int id)
{
    return -1;
}

int mssql_fetch_lengths(int id)
{
    return -1;
}

#endif // DISCARDMICROSOFTSQL from common.h

