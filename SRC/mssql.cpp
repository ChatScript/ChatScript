/** Interface routines for ODBC server, nominally MS SQL Server */ 

#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include "common.h"

#ifndef DISCARDMICROSOFTSQL
#include "mssql_imp.h"
#include "ms_sql.h"
#include "zif.h"

#undef TRACE_ON // sqlext.h redefines TRACE_ON as 1

#include <sql.h>
#include <sqlext.h>

#ifdef LINUX
#include <syslog.h>
#endif

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
static const size_t mssql_header_size = 4;
static bool already_entered_close = false;

// useful mssql commands (to use from sqlcmd). Always follow with newline, and GO
// SELECT * FROM INFORMATION_SCHEMA.TABLES;
// -- lists all tables defined in db(looking to see if userState is defined yet)

// create database chatscript; -- create the database
// USE chatscript;      --make this the current database
// CREATE TABLE userState (userid varchar(100) PRIMARY KEY, ustate VARBINARY(max), lastUpdateDate DATETIME DEFAULT CURRENT_TIMESTAMP)
// select * from information_schema.columns where table_name='userState'; -- lists names of fields and types.
// SELECT * from userState  to show entries

// DELETE  FROM userState WHERE DATE(lastUpdateDate) < DATE(NOW() - INTERVAL 1 DAY)
// Above deletes records 2 days or earlier. A user might converse just before midnight and then continue shortly thereafter, so we do not merely want a single day separator.
// DROP TABLE userState;        --destroy the entire collection of saved entries. Then recreate it.

#ifndef DISCARD_MSSQL_INTERFACE_FOR_TEST

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

static char *z_buffer = nullptr; // for compress and uncomress operations
static int z_buffer_size = 0;

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

static size_t mssqlUserRead(void* buf,size_t size, size_t count, FILE* key)
{ // read topic file record of user
    if (!mssqlInited) {
        ReportBug( "FATAL: mssql not initialized yet" );
        return 0;
    }

    size_t z_size = z_buffer_size;
    int rv = mssql_file_read(conn_id, z_buffer, &z_size, (const char*) key);
    if (rv) {
        ReportBug( mssql_error() );
        return 0;
    }

    size_t buf_size = size * count;
    int rv_u = mssql_maybe_uncompress(z_buffer, z_size, (char*)buf, &buf_size);
    if (rv_u) {
        printf( mssql_error() );
        ReportBug( mssql_error() );
        return 0;
    }

    return buf_size;
}

static size_t mssqlUserWrite(const void* buf, size_t size, size_t count, FILE* key)
{
    if (!mssqlInited) {
        ReportBug( "FATAL: mssql not initialized yet" );
        return 0;
    }

    size_t buf_size = size * count;
    size_t z_size = z_buffer_size;
    int rv_c = mssql_maybe_compress(z_buffer, &z_size, (const char*)buf, buf_size);
    if (rv_c) {
        ReportBug( mssql_error() );
        return 0;
    }

    int rv_w = mssql_file_write(conn_id, z_buffer, z_size, (const char*) key);
    if (rv_w) {
        ReportBug( mssql_error() );
        return 0;
    }

    return buf_size;
}

static void save_info_string_to_mssql_syslog(const char* s)
{
#ifdef LINUX
    syslog(LOG_INFO, "%s user: %s bot: %s msg %s",
           syslogstr, loginID, computerID, s);
#endif
}

// initialize the user system
void MsSqlUserFilesCode(char* params)
{
    if (mssqlInited)   return;

    conn_id = mssql_get_new_id();
    if (conn_id < 0)  ReportBug((char*) mssql_error());

    GetMsSqlParams(params);

    int rv = mssql_init(conn_id, mssqlhost, mssqlport,
                        mssqluser, mssqlpasswd, mssqldatabase);
    if (rv) {
        ReportBug((char*)"FATAL: mssql: Failed to connect to MSSQL: %s %s",
                  params, mssql_error());
        return;
    }

    SetUserFileSystemBlock();

    save_info_string_to_mssql_syslog("init mssql");

    z_buffer = get_mssql_staging_buffer(conn_id, &z_buffer_size);
    if (z_buffer == nullptr) {
        ReportBug((char*)"FATAL: mssql: Unable to get staging buffer",
                  log_filename);
        return;
    }

    already_entered_close = false;
    mssqlInited = true;
}

void MsSqlFullCloseCode(bool restart)
{
    if (already_entered_close) return; // prevent infinite loop because of ReportBug
    already_entered_close = true;
        
    if (!mssqlInited) {
        ReportBug((char*) "mssql: Closing uninitialized block %s", mssql_error());
        return; // closing maybe when failed to init and reported bug then
    }

    save_info_string_to_mssql_syslog("close mssql");

    z_buffer = nullptr;
    z_buffer_size = 0;

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
             "\nFATAL: mssql: The driver reported the following error %s\n", fn);
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

static bool init_db(DbInterface_t* dbp)
{
    dbp->verbose = false;

    dbp->file_buf_size =((SQLLEN)userCacheSize + OVERFLOW_SAFETY_MARGIN);
    dbp->file_buf = nullptr;

    dbp->key_buf_size = 100;
    dbp->key_buf = nullptr;

    dbp->henv = SQL_NULL_HENV;   // Environment handle
    dbp->hdbc = SQL_NULL_HDBC;   // Connection handle
    dbp->hstmt_w = SQL_NULL_HSTMT;  // Write statement handle
    dbp->hstmt_r = SQL_NULL_HSTMT;  // Write statement handle
    dbp->write_prepared = false;
    dbp->write_parameters_bound = false;
    dbp->read_prepared = false;
    dbp->read_parameters_bound = false;
    dbp->use_stored_procedures = true;
    dbp->use_tracing = false;
    dbp->used = 1;

    return true;
}

int mssql_get_new_id(void)
{
    size_t i;
    for (i = 0; i < sizeof(db_list) / sizeof(db_list[0]); ++i) {
        struct DbInterface_t* dbp = &db_list[i];
        if (dbp->used == 0) {
            bool ok = init_db(dbp);
            if (ok)  return i;
        }
    }
    return -1;
}

static DbInterface_t* get_db(int id)
{
    int array_size = sizeof(db_list)/sizeof(db_list[0]);
    if ((id < 0) || (array_size <= id))  return nullptr;

    DbInterface_t* dbp = &db_list[id];
    return dbp;
}

int mssql_set_verbose(int id, bool flag)
{
    DbInterface_t* dbp = get_db(id);
    if (dbp != nullptr) {
        dbp->verbose = flag;
        return 0;
    }
    return -1;
}

int mssql_use_stored_procedures(int id, bool flag)
{
    DbInterface_t* dbp = get_db(id);
    if (dbp != nullptr) {
        dbp->use_stored_procedures = flag;
        return 0;
    }
    return -1;
}

static bool free_statement_handle(DbInterface_t* dbp)
{
    SQLRETURN rv;
    if (dbp->hstmt_w != SQL_NULL_HSTMT) {
        rv = SQLFreeHandle(SQL_HANDLE_STMT, dbp->hstmt_w);
        if (is_error(rv, "SQLFreeHandle", dbp->hstmt_w, SQL_HANDLE_STMT)) {
            return false;
        }
        dbp->hstmt_w = nullptr;
    }
    if (dbp->hstmt_r != SQL_NULL_HSTMT) {
        rv = SQLFreeHandle(SQL_HANDLE_STMT, dbp->hstmt_r);
        if (is_error(rv, "SQLFreeHandle", dbp->hstmt_r, SQL_HANDLE_STMT)) {
            return false;
        }
        dbp->hstmt_r = nullptr;
    }
    return true;
}

static bool free_connection_handle(DbInterface_t* dbp)
{
    SQLRETURN rv;
    if (dbp->hdbc != SQL_NULL_HDBC) {
        rv = SQLDisconnect(dbp->hdbc);
        if (is_error(rv, "SQLDisconnect", dbp->hdbc, SQL_HANDLE_DBC)) {
            return false;
        }
        rv = SQLFreeHandle(SQL_HANDLE_DBC, dbp->hdbc);
        if (is_error(rv, "SQLFreeHandle", dbp->hdbc, SQL_HANDLE_DBC)) {
            return false;
        }
        dbp->hdbc = nullptr;
    }
    return true;
}

static bool free_environment_handle(DbInterface_t* dbp)
{
    SQLRETURN rv;
    if (dbp->henv != SQL_NULL_HENV) {
        rv = SQLFreeHandle(SQL_HANDLE_ENV, dbp->henv);
        if (is_error(rv, "SQLFreeHandle", dbp->henv, SQL_HANDLE_ENV)) {
            return false;
        }
        dbp->henv = nullptr;
    }
    return true;

}

static bool deinit_database_interface_struct(DbInterface_t* dbp)
{
    releaseHeap(dbp->file_buf);
    dbp->file_buf = nullptr;
    dbp->file_buf_size = 0;

    releaseHeap(dbp->key_buf);
    dbp->key_buf = nullptr;
    dbp->key_buf_size = 0;

    dbp->write_prepared = false;
    dbp->write_parameters_bound = false;
    dbp->read_prepared = false;

    dbp->used = 0;

    return true;
}

int mssql_close(int id)
{
    DbInterface_t* dbp = get_db(id);
    bool ok = (dbp != nullptr);

    ok = ok & free_statement_handle(dbp);
    ok = ok & free_connection_handle(dbp);
    ok = ok & free_environment_handle(dbp);
    ok = ok & deinit_database_interface_struct(dbp);

    int rv = (ok ? 0 : -1);

    return rv;
}

static bool alloc_buffers(DbInterface_t* dbp)
{
    dbp->file_buf = getHeap(dbp->file_buf_size);
    if (dbp->file_buf == nullptr) {
        snprintf(error_buf, sizeof(error_buf),
                 "FATAL: mssql: Unable to allocate %ld bytes for file buffer",
                 (long) dbp->file_buf_size);
        return false;
    }

    dbp->key_buf = getHeap(dbp->key_buf_size);
    if (dbp->key_buf == nullptr) {
        snprintf(error_buf, sizeof(error_buf),
                 "FATAL: mssql: Unable to allocate %ld bytes for key buffer",
                 (long) dbp->file_buf_size);
        return false;
    }
    return true;
}
static bool alloc_environment_handle(DbInterface_t* dbp)
{
    SQLRETURN retcode;
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &dbp->henv);
    if (is_error(retcode, "SQLAllocHandle(SQL_HANDLE_ENV)",
                 dbp->henv, SQL_HANDLE_ENV)) {
        return false;
    }
    return true;
}

static bool set_ODBC_version(DbInterface_t* dbp)
{
    SQLRETURN retcode;
    retcode = SQLSetEnvAttr(dbp->henv, SQL_ATTR_ODBC_VERSION,
                            (SQLPOINTER*)SQL_OV_ODBC3, 0);
    if (is_error(retcode, "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)",
                 dbp->henv, SQL_HANDLE_ENV)) {
        return false;
    }
    return true;
}

static bool alloc_connection_handle(DbInterface_t* dbp)
{
    SQLRETURN retcode;
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, dbp->henv, &dbp->hdbc);
    if (is_error(retcode, "SQLAllocHandle(SQL_HANDLE_DBC)",
                 dbp->hdbc, SQL_HANDLE_DBC)) {
        return false;
    }

    return true;
}

static bool set_login_timeout(DbInterface_t* dbp)
{
    // I know the below does not make sense because
    // a number is being used as a pointer,
    // but it comes directly from this MS page:
    // https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlconnect-function?view=sql-server-ver15
    SQLRETURN retcode;
    retcode = SQLSetConnectAttr(dbp->hdbc,
                                SQL_LOGIN_TIMEOUT,
                                (SQLPOINTER)5, // login time in s
                                0);
    if (is_error(retcode, "SQLSetConnectAttr(SQL_LOGIN_TIMEOUT)",
                 dbp->hdbc, SQL_HANDLE_DBC)) {
        return false;
    }
    return true;
}

static bool set_tracing(DbInterface_t* dbp) {
    SQLRETURN retcode;
    if (dbp->use_tracing) {
        retcode = SQLSetConnectAttr(dbp->hdbc,
                                    SQL_ATTR_TRACEFILE,
                                    tracing_filename,
                                    SQL_NTS);
        if (is_error(retcode, "SQLSetConnectAttr - SQL_ATTR_TRACEFILE",
                     dbp->hdbc, SQL_HANDLE_DBC)) {
            return false;
        }

        retcode = SQLSetConnectAttr(dbp->hdbc,
                                    SQL_ATTR_TRACE,
                                    (void*)SQL_OPT_TRACE_ON,
                                    SQL_NTS);
        if (is_error(retcode, "SQLSetConnectAttr - SQL_ATTR_TRACE",
                     dbp->hdbc, SQL_HANDLE_DBC)) {
            return false;
        }
    }
    else {
        retcode = SQLSetConnectAttr(dbp->hdbc,
                                    SQL_ATTR_TRACE,
                                    (void*)SQL_OPT_TRACE_OFF,
                                    SQL_NTS);
        if (is_error(retcode, "SQLSetConnectAttr - SQL_ATTR_TRACE",
                     dbp->hdbc, SQL_HANDLE_DBC)) {
            return false;
        }
    }

    return true;
}

static bool connect(DbInterface_t* dbp,
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
    if (dbp->verbose) {
        printf("in_conn_str %s\n", in_conn_str);
    }
    HWND dhandle = nullptr;
    SQLSMALLINT in_len = (SQLSMALLINT) strlen(in_conn_str);
    outstrlen = sizeof(out_conn_str);
    retcode = SQLDriverConnect(dbp->hdbc, // SQLHDBC ConnectionHandle
                               dhandle,  // SQLHWND WindowHandle
                               (SQLCHAR*)in_conn_str, // SQLCHAR * InConnectionString
                               in_len, // SQLSMALLINT StringLength1
                               (SQLCHAR*)out_conn_str, // SQLCHAR * OutConnectionString
                               outstrlen, // SQLSMALLINT BufferLength
                               &outstrlen, // SQLSMALLINT * StringLength2Ptr
                               SQL_DRIVER_COMPLETE); // SQLUSMALLINT DriverCompletion
    if (dbp->verbose) {
        printf("connect outstrlen %d\n", outstrlen);
        if (0 < outstrlen) {
            printf("outstr: %s\n", out_conn_str);
        }
    }
    if (retcode == SQL_SUCCESS) {
        if (dbp->verbose) {
            printf("Connect: SQL_SUCCESS\n");
        }
        return true;
    }
    else if (retcode == SQL_SUCCESS_WITH_INFO) {
        if (dbp->verbose) {
            extract_error("SQLDriverConnect", dbp->hdbc, SQL_HANDLE_DBC);
            printf("Got SQL_SUCCESS_WITH_INFO %s\n", error_buf);
        }
        return true;
    }

    is_error(retcode, "SQLDriverConnect", dbp->hdbc, SQL_HANDLE_DBC);

    return false;
}

static bool alloc_statement_handles(DbInterface_t* dbp)
{
    SQLRETURN retcode;
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, dbp->hdbc, &dbp->hstmt_w);
    if (is_error(retcode, "SQLAllocHandle(SQL_HANDLE_STMT)",
                 dbp->hstmt_w, SQL_HANDLE_STMT)) {
        return false;
    }
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, dbp->hdbc, &dbp->hstmt_r);
    if (is_error(retcode, "SQLAllocHandle(SQL_HANDLE_STMT)",
                 dbp->hstmt_r, SQL_HANDLE_STMT)) {
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
    DbInterface_t* dbp = get_db(id);
    bool ok = (dbp != nullptr);
    ok = ok && alloc_buffers(dbp);
    ok = ok && alloc_environment_handle(dbp);
    ok = ok && set_ODBC_version(dbp);
    ok = ok && alloc_connection_handle(dbp);
    ok = ok && set_login_timeout(dbp);
    ok = ok && set_tracing(dbp);
    ok = ok && connect(dbp, host, host_port, user, pwd, database);
    ok = ok && alloc_statement_handles(dbp);
    return (ok ? 0 : -1);
}

static bool empty_result_set(SQLHSTMT hstmt)
{
    while (SQLMoreResults(hstmt) == SQL_SUCCESS) {
        ;
    }
    return true;
}

static bool file_write_preformat_data(DbInterface_t* dbp, const void* buf, size_t buf_size)
{
    size_t cushion = 20;
    if (dbp->file_buf_size < (SQLLEN)(buf_size + cushion)) {
        sprintf(error_buf, "FATAL: mssql: file too big. file size %zu", buf_size);
        return false;
    }

    if (buf != dbp->file_buf) { // only copy if buffers are not the same
        memcpy(dbp->file_buf, buf, buf_size);
    }
    dbp->file_buf_len = buf_size;
    if (dbp->verbose) {
        printf("write size %ld\n", (long) dbp->file_buf_len);
    }

    return true;
}

static bool file_write_bind_params(DbInterface_t* dbp)
{
    if (!dbp->write_parameters_bound) { // only bind once
        SQLRETURN retcode;

        SQLULEN p1_column_size = 100;
        SQLSMALLINT p1_digits = 0;
        dbp->key_buf_len = dbp->key_buf_size;
        retcode = SQLBindParameter(dbp->hstmt_w,      // StatementHandle
                                   1,                // ParameterNumber
                                   SQL_PARAM_INPUT,  // InputOutputType
                                   SQL_C_CHAR,       // ValueType
                                   SQL_VARCHAR,      // ParameterType
                                   p1_column_size,   // ColumnSize
                                   p1_digits,        // DecimalDigits
                                   (SQLPOINTER) dbp->key_buf, // ParameterValuePtr
                                   dbp->key_buf_size,            // BufferLength
                                   &dbp->key_buf_len);          // StrLen_or_IndPtr
        if (is_error(retcode, "SQLBindParameter", dbp->hstmt_w, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }

        SQLULEN p2_column_size = dbp->file_buf_size;
        SQLSMALLINT p2_digits = 0;
        retcode = SQLBindParameter(dbp->hstmt_w,      // StatementHandle
                                   2,                // ParameterNumber
                                   SQL_PARAM_INPUT,  // InputOutputType
                                   SQL_C_BINARY,     // ValueType
                                   SQL_LONGVARBINARY, // ParameterType
                                   p2_column_size,   // ColumnSize
                                   p2_digits,        // DecimalDigits
                                   dbp->file_buf,     // ParameterValuePtr
                                   dbp->file_buf_len, // BufferLength, ignored for writes
                                   &dbp->file_buf_len); // StrLen_or_IndPtr
        if (is_error(retcode, "SQLBindParameter", dbp->hstmt_w, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }
        dbp->write_parameters_bound = true;
    }

    return true;
}

static bool file_write_copy_key(DbInterface_t* dbp, const char* key)
{
    memcpy(dbp->key_buf, key, dbp->key_buf_len);
    dbp->key_buf_len = strlen(key);
    return true;
}


static bool file_write_prepare(DbInterface_t* dbp)
{
    if (!dbp->write_prepared) {  // only prepare once
        const char* upsert_sql = ""
            "MERGE INTO userState AS t "
            "  USING  "
            "    (SELECT pk=?, f1= ?) AS s "
            "  ON t.userid = s.pk "
            "  WHEN MATCHED THEN "
            "    UPDATE SET userid=s.pk, ustate=s.f1 "
            "  WHEN NOT MATCHED THEN "
            "    INSERT (userid, ustate) "
            "    VALUES (s.pk, s.f1); "
            "";

        const char* upsert_sp= ""
            "{ CALL spChatScript_UpsertByUserId( ?, ? ) }";
            // "{ CALL uspSetUserData( ?, ? ) }";

        const char* upsert = (dbp->use_stored_procedures ? upsert_sp : upsert_sql);
        SQLRETURN retcode;

        retcode = SQLPrepare(dbp->hstmt_w, (SQLCHAR*)upsert, SQL_NTS);
        if (is_error(retcode, "SQLPrepare", dbp->hstmt_w, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }

        SQLSMALLINT num_params;
        SQLNumParams(dbp->hstmt_w, &num_params);

        int expected_num_params = 2;
        if (num_params != expected_num_params) {
            return false;
        }
        dbp->write_prepared = true;
    }

    return true;
}

static bool file_write_execute(DbInterface_t* dbp)
{
    SQLRETURN retcode;

    retcode = SQLExecute(dbp->hstmt_w);
    if (is_error(retcode, "SQLExecute", dbp->hstmt_w, SQL_HANDLE_STMT)) {
        printf("%s\n", error_buf);
        return false;
    }
    return true;
}

int mssql_file_write(int id, const void* buf, size_t buf_size, const char* key)
{
    DbInterface_t* dbp = get_db(id);
    bool ok = (dbp != nullptr);

    ok = ok && file_write_preformat_data(dbp, buf, buf_size);
    ok = ok && file_write_prepare(dbp);
    ok = ok && file_write_bind_params(dbp);
    ok = ok && file_write_copy_key(dbp, key);
    ok = ok && file_write_execute(dbp);
    ok = ok && empty_result_set(dbp->hstmt_w);

    int ret_val = (ok ? 0 : -1);
    return ret_val;
}

static bool file_read_init(DbInterface_t* dbp, const char* key)
{
    dbp->is_empty = false;

    dbp->key_buf_len = strlen(key);
    memcpy(dbp->key_buf, key, dbp->key_buf_len);

    dbp->file_buf_len = dbp->file_buf_size;

    return true;
}

static bool file_read_bind(DbInterface_t* dbp, const char* key)
{
    if (!dbp->read_parameters_bound) {
        SQLRETURN retcode;
        SQLULEN p1_column_size = 100;
        SQLSMALLINT p1_digits = 0;
        retcode = SQLBindParameter(dbp->hstmt_r,      // StatementHandle
                                   1,                // ParameterNumber
                                   SQL_PARAM_INPUT,  // InputOutputType
                                   SQL_C_CHAR,       // ValueType
                                   SQL_VARCHAR,      // ParameterType
                                   p1_column_size,   // ColumnSize
                                   p1_digits,        // DecimalDigits
                                   (SQLPOINTER) dbp->key_buf, // ParameterValuePtr
                                   dbp->key_buf_len,  // BufferLength
                                   &dbp->key_buf_len); // StrLen_or_IndPtr
        if (is_error(retcode, "SQLBindParameter", dbp->hstmt_r, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }
    }

    return true;
}

static bool file_read_prepare(DbInterface_t* dbp)
{
    if (!dbp->read_prepared) {
        const char* query_sql = ""
            "SELECT ustate from userState \n"
            "WHERE userid = ? ";
        const char* query_sp = ""
            "{ CALL spChatScript_GetByUserId( ? ) } ";
            // "{ CALL uspGetUserData( ? ) } ";
        const char *query = (dbp->use_stored_procedures ? query_sp : query_sql);

        SQLRETURN retcode;
        retcode = SQLPrepare(dbp->hstmt_r, (SQLCHAR*)query, SQL_NTS);
        if (is_error(retcode, "SQLPrepare", dbp->hstmt_r, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }

        dbp->read_prepared = true;
    }

    return true;
}

static bool file_read_execute(DbInterface_t* dbp)
{
    SQLRETURN retcode;
    retcode = SQLExecute(dbp->hstmt_r);
    if (is_error(retcode, "SQLExecute", dbp->hstmt_r, SQL_HANDLE_STMT)) {
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
                                dbp->hstmt_r,// SQLHANDLE Handle
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

static bool file_read_fetch_and_get_data(DbInterface_t* dbp, size_t* buf_size_p)
{
    dbp->is_empty = true;

    SQLRETURN retcode;
    int cycle_count = 0;
    const int max_cycle_count = 5;
    while (cycle_count < max_cycle_count) { // handles multiple result sets
        cycle_count++;
        retcode = SQLFetch(dbp->hstmt_r);
        if (retcode == SQL_NO_DATA) {
            cycle_count = max_cycle_count;
            continue;
        }

        if (is_error(retcode, "SQLFetch", dbp->hstmt_r, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }

        retcode = SQLGetData(dbp->hstmt_r,  // Statement handle
                             1,            // Column number
                             SQL_C_BINARY, // TargetType, the C type
                             (SQLPOINTER) dbp->file_buf, // TargetValudPtr
                             (SQLLEN) dbp->file_buf_size, // BufferLength
                             (SQLLEN*) &dbp->file_buf_len  // strlen_or_ind
                             );
        if (is_error(retcode, "SQLGetData", dbp->hstmt_r, SQL_HANDLE_STMT)) {
            printf("%s\n", error_buf);
            return false;
        }
        if (0 < dbp->file_buf_len) {
            dbp->is_empty = false;
        }
    }
    return true;
}

static bool file_read_postformat_data(DbInterface_t* dbp, void* read_buf, size_t* read_len_p)
{
    if (!dbp->is_empty) {
        int cushion = 20;               // extra chars for bad compression
        if ((SQLLEN)(*read_len_p + cushion) < dbp->file_buf_len) {
            sprintf(error_buf, "FATAL: mssql: Buffer too small for read: %zu vs %ld",
                    *read_len_p, (long) dbp->file_buf_len);
            return false;
        }

        if (read_buf != dbp->file_buf) { // only copy if buffers are not the same
            memcpy(read_buf, dbp->file_buf, dbp->file_buf_len);
        }
        *read_len_p = dbp->file_buf_len;
        if (dbp->verbose) {
            printf("read size %ld\n", (long) dbp->file_buf_len);
        }
    }
    else {
        *read_len_p = 0;
    }

    ((char*)read_buf)[*read_len_p] = '\0'; // null terminal string
    return true;
}

static bool file_read_with_prepare_and_execute(DbInterface_t* dbp,
                                               void* read_buf,
                                               size_t* read_len_p,
                                               const char* key)
{
    bool ok = true;
    ok = ok && file_read_init(dbp, key);
    ok = ok && file_read_bind(dbp, key);
    ok = ok && file_read_prepare(dbp);
    ok = ok && file_read_execute(dbp);
    ok = ok && file_read_fetch_and_get_data(dbp, read_len_p);
    ok = ok && file_read_postformat_data(dbp, read_buf, read_len_p);
    ok = ok && empty_result_set(dbp->hstmt_r);

    return ok;
}

int mssql_file_read(int id, void* read_buf, size_t* read_len_p, const char* key)
{
    DbInterface_t* dbp = get_db(id);
    bool ok = (dbp != nullptr);

    ok = ok && file_read_with_prepare_and_execute(dbp, read_buf, read_len_p, key);

    int ret_val = (ok ? 0 : -1);
    return ret_val;
}

int mssql_set_tracing(int id, bool flag, const char* filename)
{
    DbInterface_t* dbp = get_db(id);
    if (dbp == nullptr) {
        sprintf(error_buf, "FATAL: mssql: id not acceptable\n");
        return -1;
    }

    dbp->use_tracing = flag;
    if (flag) {
        if (filename != nullptr) {
            strcpy(tracing_filename, filename);
            if (dbp->hdbc != SQL_NULL_HDBC) {
                bool ok = set_tracing(dbp);
                if (!ok) {
                    return -1;
                }
            }
        }
        else {
            memset(tracing_filename, '\0', sizeof(tracing_filename));
            sprintf(error_buf, "FATAL: mssql: bad filename\n");
            return -1;
        }
    }
    return 0;
}

int mssql_maybe_compress(char* c_buf, size_t* c_size, const char* p_buf, size_t p_size)
{
    if (p_size == 0 || p_buf == nullptr) {
        strcpy(error_buf, "FATAL: empty buffer in maybe_compress");
        return -1;
    }

    int use_zlib = gzip;
#ifdef DISCARD_TEXT_COMPRESSION
    use_zlib = 0;
#endif

    if (use_zlib) {
        strcpy(c_buf, "gzip");
        struct zif_t zc;
        zc.uncompressed_buf = (char*) p_buf;
        zc.uncompressed_buf_len = p_size;
        zc.compressed_buf = c_buf + mssql_header_size;
        zc.compressed_buf_len = *c_size - mssql_header_size;;
        int rv_c = 0;
#ifndef DISCARD_TEXT_COMPRESSION
        zif_compress(&zc);
#endif
        *c_size = zc.compressed_buf_len + mssql_header_size;
        return rv_c;
    }
    else {
        strcpy(c_buf, "norm");
        memcpy(c_buf + mssql_header_size, p_buf, p_size);
        *c_size = p_size + mssql_header_size;
        return 0;
    }
}

int mssql_maybe_uncompress(const char* c_buf, size_t c_size, char* p_buf, size_t* p_size)
{
    if (c_size == 0) {
        *p_size = 0;
        return 0;
    }
    if (c_buf == nullptr) {
        strcpy(error_buf, "FATAL: empty buffer in maybe_uncompress");
        return -1;
    }

    int use_zlib = gzip;
#ifdef DISCARD_TEXT_COMPRESSION
    use_zlib = 0;
#endif

    if (use_zlib) {
        if (strstr(c_buf, "gzip") == c_buf) {
            struct zif_t zc;
            zc.uncompressed_buf = p_buf;
            zc.uncompressed_buf_len = *p_size;
            zc.compressed_buf = (char*)c_buf + mssql_header_size;
            zc.compressed_buf_len = c_size - mssql_header_size;
            int rv_uc = 0;
#ifndef DISCARD_TEXT_COMPRESSION
            rv_uc = zif_uncompress(&zc);
#endif
            *p_size = zc.uncompressed_buf_len;
            return rv_uc;
        }
        else if (strstr(c_buf, "norm") == c_buf) {
            *p_size = 0; // ignore wrong flavor of compression
            return 0;
        }
    }
    else {
        if (strstr(c_buf, "norm") == c_buf) {
            *p_size = c_size - mssql_header_size;
            memcpy(p_buf, c_buf + mssql_header_size, *p_size);
            return 0;
        }
        else if (strstr(c_buf, "gzip") == c_buf) {
            *p_size = 0; // ignore wrong flavor of compression
            return 0;
        }
    }

    strcpy(error_buf, "FATAL: bad header in maybe_uncompress. "
           "Check database for bad data.");
    return -2;
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

char* get_mssql_staging_buffer(int id, int* staging_buffer_size) {
    DbInterface_t* dbp = get_db(id);
    if (dbp != nullptr) {
        *staging_buffer_size = (int)(dbp->file_buf_size);
        return (char*) dbp->file_buf;
    }
    sprintf(error_buf, "FATAL: mssql: file buffer uninitialized");
    return nullptr;
}

#endif // DISCARDMICROSOFTSQL from common.h
