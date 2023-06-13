#include "common.h"
#ifndef DISCARDMYSQL
extern "C" {
#include "mysql/include/mysql.h"
}
#include "my_sql.h"

// binding data for reads and writes
static MYSQL_BIND readbind[1];
static MYSQL_BIND resultbind[1];
static 	MYSQL_BIND writebind[2];
static 	unsigned long datalength = 0;
static 	my_bool is_null;
static 	my_bool is_error;

// library init
static bool mysqlInited = false;

// filesystem data
static MYSQL *usersconn = NULL;
char mysqlUserFilename[MAX_WORD_SIZE]; // current topic file name for user

// user script data
static MYSQL *scriptconn = NULL;
MYSQL_STMT *mystmt_script = NULL;

// mysql config options
char mysqlparams[300]; // init string for file system using mysql
// data from the mysqlparams to connect with - usersconn uses then scriptconn can overwrite
static char mysqlhost[300];
static unsigned int mysqlport;
static char mysqldb[300];
static char mysqluser[300];
static char mysqlpasswd[300];

// useful mysql commands (to use from MySQL workbench)
// SHOW TABLES	-- lists all tables defined in db(looking to see if userfiles is defined yet)
// CREATE TABLE userfiles (userid varchar(100) PRIMARY KEY, file MEDIUMBLOB, stored DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP)
// SHOW COLUMNS FROM userfiles – lists names of fields and types.
// SELECT * from userfiles  to show entries
// USE chatscript;	--make this the current database
// SET SQL_SAFE_UPDATES=0   -- allow deletes without naming primary key
// DELETE  FROM userfiles WHERE DATE(stored) < DATE(NOW() - INTERVAL 1 DAY)
// Above deletes records 2 days or earlier. A user might converse just before midnight and then continue shortly thereafter, so we don’t merely want a single day separator.
// SET SQL_SAFE_UPDATES=1   -- restore safe update
// DROP TABLE userfiles;	--destroy the entire collection of saved entries. Then recreate it.

const char* MySQLVersion()
{
    static char version[MAX_WORD_SIZE] = "";
    if (*version) return(version);
    
    const char *data = mysql_get_client_info();

    sprintf(version,"MySql %s", data);
    return(version);
}

static void GetMySQLParams(char* params)
{
	// mysql="host=localhost port=3306 user=root password=admin db=chatscript" 
	size_t len = strlen(params);
	if (params[len - 1] == '"') params[len - 1] = 0;
	char* p = strstr(params, "host=");
	if (p) ReadCompiledWord(p + 5, mysqlhost);
	p = strstr(params, "port=");
	if (p) mysqlport = atoi(p + 5);
	p = strstr(params, "user=");
	if (p) ReadCompiledWord(p + 5, mysqluser);
	p = strstr(params, "password=");
	if (p) ReadCompiledWord(p + 9, mysqlpasswd);
	p = strstr(params, "db=");
	if (p) ReadCompiledWord(p + 3, mysqldb);
	if (strstr(params, "appendport")) // optional extension of db name
	{
		char name[MAX_WORD_SIZE];
		sprintf(name, "%s%d", mysqldb, port);
		strcpy(mysqldb,name);
	}
}

////////////////////////////////////
// FILE SYSTEM REPLACEMENT FUNCTIONS
////////////////////////////////////

FILE* mysqlUserCreate(const char* name)
{ // unlike file system create, we dont create anything here, just note it
	const char* at = strrchr(name, '/');
	if (at) strcpy(mysqlUserFilename, at + 1);
	else strcpy(mysqlUserFilename,name);
	return (FILE*)mysqlUserFilename;
}

FILE* mysqlUserOpen(const char* name)
{ // unlike file system open, we dont open anything here, just note it
	const char* at = strrchr(name, '/');
	if (at) strcpy(mysqlUserFilename, at + 1);
	else strcpy(mysqlUserFilename, name);
	return (FILE*)mysqlUserFilename;
}

int mysqlUserClose(FILE*)
{ // since we never create or open anything, we dont have to close it
	return 0;
}

size_t mysqlUserRead(void* buf,size_t size, size_t count, FILE* file)
{ // read topic file record of user
	unsigned long filenamelength = strlen(mysqlUserFilename);
	readbind[0].buffer = mysqlUserFilename;
	readbind[0].length = &filenamelength;

	static const char* mydefault_userread = "SELECT file FROM userfiles WHERE userid = ?";
	MYSQL_STMT *mystmt_userread = mysql_stmt_init(usersconn);
	if (mystmt_userread == NULL)
	{
		ReportBug((char*)"FATAL: Failed to allocate MYSQL_STMT: %s", mysql_error(usersconn));
	}
	if (mysql_stmt_prepare(mystmt_userread, mydefault_userread, strlen(mydefault_userread)))
	{
		ReportBug((char*)"FATAL: Failed to prepare user read STMT: %s", mysql_stmt_error(mystmt_userread));
	}

	// bind statement parameters
	if (mysql_stmt_bind_param(mystmt_userread, readbind))
	{
		ReportBug((char*)"FATAL: Failed to bind user read STMT: %s", mysql_stmt_error(mystmt_userread));
	}

	// execute statement
	if (mysql_stmt_execute(mystmt_userread))
	{
		ReportBug((char*)"FATAL: Failed to execute user read STMT: %s", mysql_stmt_error(mystmt_userread));
	}

	// fetch entire result set from server
	if (mysql_stmt_store_result(mystmt_userread))
	{
		ReportBug((char*)"FATAL: Failed to store result of user read STMT: %s", mysql_stmt_error(mystmt_userread));
	}

	my_ulonglong rowcount = mysql_stmt_num_rows(mystmt_userread);
	if (rowcount == 0) // no user data found (new user)
	{
		mysql_stmt_close(mystmt_userread);
		*(char*)buf = 0;
		return 0;
	}

	// fetch result row
	resultbind[0].buffer = (char*)buf;
	if (mysql_stmt_bind_result(mystmt_userread, resultbind))
	{
		ReportBug((char*)"FATAL: Failed to bind result of user read STMT: %s", mysql_stmt_error(mystmt_userread));
	}
	int res = mysql_stmt_fetch(mystmt_userread);
	if (res == MYSQL_NO_DATA)
	{
		ReportBug((char*)"FATAL: Failed to fetch result of user read STMT");
	}
	else if (res == 1)
	{
		ReportBug((char*)"FATAL: Failed to fetch result of user read STMT %s", mysql_stmt_error(mystmt_userread));
	}

	// user data found
	if (is_error)
	{
		ReportBug((char*)"FATAL: Error during fetch result of user read STMT %s", mysql_stmt_error(mystmt_userread));
	}
	
	mysql_stmt_close(mystmt_userread);

	if (is_null) datalength = 0;
	((char*)buf)[datalength] = 0;
	return datalength;
}

static size_t mysqlUserWrite(const void* buf, size_t size, size_t count, FILE* file)
{
	// bind0 is userid (varchar)
	writebind[0].buffer = mysqlUserFilename;
	writebind[0].buffer_length = strlen(mysqlUserFilename);
	// bind1 is file data (blob)
	unsigned long filedatalength = size * count;	// buffer size, function param
	writebind[1].buffer = (char*) buf;
	writebind[1].length = &filedatalength;

	static const char* mydefault_userreplace = "REPLACE INTO userfiles (userid,file) VALUES (?, ?)";
	MYSQL_STMT *mystmt_userreplace = mysql_stmt_init(usersconn);
	if (mystmt_userreplace == NULL)
	{
		ReportBug((char*)"FATAL: Failed to allocate mysql insert stmt: %s", mysql_error(usersconn));
	}
	if (mysql_stmt_prepare(mystmt_userreplace, mydefault_userreplace, strlen(mydefault_userreplace)))
	{
		ReportBug((char*)"FATAL: Failed to prepare user insert STMT: %s", mysql_stmt_error(mystmt_userreplace));
	}

	// bind statement parameters
	if (mysql_stmt_bind_param(mystmt_userreplace, writebind))
	{
		ReportBug((char*)"FATAL: Failed to bind user replace STMT: %s", mysql_stmt_error(mystmt_userreplace));
	}

	// execute statement
	if (mysql_stmt_execute(mystmt_userreplace)) 
	{
		ReportBug((char*)"FATAL: Failed to write data: %s", mysql_stmt_error(mystmt_userreplace));
	} 
	
	mysql_stmt_close(mystmt_userreplace);

	return filedatalength; 
}

// initialize the user system
void MySQLUserFilesCode(char* params)
{
	if (mysqlInited) return;

	if (usersconn)
	{
		ReportBug((char*)"Duplicate Connection MySql\r\n");
		return;
	}
#ifdef WIN32
	if (InitWinsock() == FAILRULE_BIT) ReportBug((char*)"FATAL: WSAStartup failed\r\n");
#endif
	mysql_library_init(0, NULL, NULL);
	mysqlInited = true;

	usersconn = mysql_init(NULL);
	if (usersconn == NULL)
	{
		ReportBug((char*)"FATAL: Failed to allocate MYSQL");
	}

	// @see https://dev.mysql.com/doc/refman/5.7/en/mysql-real-connect.html
	GetMySQLParams(params); // mysql="host=localhost port=3306 user=root password=admin db=chatscript" 
	usersconn = mysql_real_connect(usersconn, mysqlhost, mysqluser, mysqlpasswd, mysqldb, mysqlport, NULL, 0);
	if (usersconn == NULL)
	{
		ReportBug((char*)"FATAL: Failed to connect to MYSQL: %s", mysql_error(usersconn));
	}

	// read data
	
	// read bind data
	memset(readbind, 0, sizeof(readbind));
	readbind[0].buffer_type = MYSQL_TYPE_STRING;
	readbind[0].buffer_length = MAX_WORD_SIZE;
	readbind[0].is_null = 0;
	// readbind[0].buffer = mysqlUserFilename; // set in read code
	// readbind[0].length = &filenamelength;

	// fetch result row for read
	memset(resultbind, 0, sizeof(resultbind));
	resultbind[0].buffer_type = MYSQL_TYPE_LONG_BLOB;
	resultbind[0].buffer_length = userCacheSize; // how much we can accept
	resultbind[0].is_null = &is_null;
	resultbind[0].error = &is_error;
	resultbind[0].length = &datalength;
	//resultbind[0].buffer = (char*)buf;  done in read code

	// write data

	// write bind data
	memset(writebind, 0, sizeof(writebind));
	writebind[0].buffer_type = MYSQL_TYPE_STRING;
	writebind[0].is_null = 0;
	// writebind[0].buffer = mysqlUserFilename;
	// writebind[0].buffer_length = strlen(mysqlUserFilename);

	writebind[1].buffer_type = MYSQL_TYPE_LONG_BLOB;
	writebind[1].buffer_length = userCacheSize;
	writebind[1].is_null = 0;
	// writebind[1].buffer = (char*)buf; in the write call
	// writebind[1].length = &filedatalength;

	// these are dynamically stored, so CS can be a DLL.
	userFileSystem.userCreate = mysqlUserCreate;
	userFileSystem.userOpen = mysqlUserOpen;
	userFileSystem.userClose = mysqlUserClose;
	userFileSystem.userRead = mysqlUserRead;
	userFileSystem.userWrite = mysqlUserWrite;
	userFileSystem.userDelete = NULL;
	filesystemOverride = MYSQLFILES;
}

void MySQLFullCloseCode(bool restart)
{
	if (scriptconn) // script access to mysql
	{
		mysql_close(scriptconn);
		scriptconn = NULL;
	}
	if (usersconn && !restart)
	{
		mysql_close(usersconn);
		usersconn = NULL;
		InitUserFiles(); // default back to normal filesystem
	}
	if (mysqlInited && !restart)
	{
		mysql_library_end();
		mysqlInited = false;
	}
	// unlike mongo and postgres, we dont have to massage the output buffer so we dont allocate or deallocate a transfer buffer
}

////////////////////////////////////
// SCRIPT ACCESS FUNCTIONS
////////////////////////////////////

FunctionResult MySQLInitCode(char* buffer)
{
	if (scriptconn) return FAILRULE_BIT;

	if (!mysqlInited)
	{
		mysql_library_init(0, NULL, NULL);
		mysqlInited = true;
#ifdef WIN32
		if (InitWinsock() == FAILRULE_BIT)
		{
			SetUserVariable((char*)"$$db_error", "Mysql cannot init Winsock");	// pass along the error
			Log(USERLOG, "Mysql cannot init Winsock");
			return FAILRULE_BIT;
		}
#endif
	}
	*mysqldb = 0;
	GetMySQLParams(ARGUMENT(1));
	scriptconn = mysql_init(NULL);
	scriptconn = mysql_real_connect(scriptconn, mysqlhost, mysqluser, mysqlpasswd, (*mysqldb) ? mysqldb : NULL, mysqlport, NULL, 0);
	if (scriptconn == NULL)
	{
		char error[MAX_WORD_SIZE];
		sprintf(error, "unable to connect to mysql");
		SetUserVariable((char*)"$$db_error", error);	// pass along the error
		Log(USERLOG, error);
		return FAILRULE_BIT;
	}
	return NOPROBLEM_BIT;
}

FunctionResult MySQLCloseCode(char* buffer)
{
	if (!scriptconn) return FAILRULE_BIT;
	mysql_close(scriptconn);
	scriptconn = NULL;
	return NOPROBLEM_BIT;
}

FunctionResult MySQLQueryCode(char* buffer)
{
	if (!scriptconn) return FAILRULE_BIT;
	char error[MAX_WORD_SIZE];

	// do we need to quote the data
	char* func = ARGUMENT(2);
	if (*func == '\'') ++func;
	if (*func && *func != '^')
	{
		sprintf(error, "%s is not a function", func);
		SetUserVariable((char*)"$$db_error", error);	// pass along the error
		Log(USERLOG, error);
		return FAILRULE_BIT;
	}

	char* arg1 = ARGUMENT(1); // the query (eg select)
	if (*arg1 == '"')
	{
		++arg1;
		size_t len = strlen(arg1) - 1;
		if (arg1[len] == '"') arg1[len] = 0;
	}

	// execute statement
	if (mysql_query(scriptconn, arg1))
	{
		sprintf(error, "Failed  %s - %s", arg1, mysql_error(scriptconn));
		SetUserVariable((char*)"$$db_error", error);	// pass along the error
		Log(USERLOG, error);
		return FAILRULE_BIT;
	}
	FunctionResult csresult = NOPROBLEM_BIT;

	MYSQL_RES *result = mysql_store_result(scriptconn); // get all data local
	if (result)  // there are rows
	{
		int flags;
		unsigned int args = GetFnArgCount(func, flags);
		unsigned int num_fields = mysql_num_fields(result);
		if (num_fields > args && *func)
		{
			sprintf(error, "%d data columns but only %d arguments allowed to function %s", num_fields,args,func);
			SetUserVariable((char*)"$$db_error", error);	// pass along the error
			Log(USERLOG, error);
			mysql_free_result(result);
			return FAILRULE_BIT;
		}

		char* psBuffer = AllocateBuffer();
		psBuffer[0] = '(';
		psBuffer[1] = ' ';
		if (args == 1) psBuffer[2] = '"'; // stripable string marker
		else psBuffer[2] = ' ';

		MYSQL_ROW row;
		int rowcount = (int) mysql_num_rows(result);
		while ((row = mysql_fetch_row(result)))
		{
			char* at = psBuffer + 3;
			*at = 0;
			unsigned long *lengths;
			lengths = mysql_fetch_lengths(result);

			/* handle current row here */
			if (*func)
			{
				strcpy(at, "\""); // start of arg
				at += 1;
			}
			for (int i = 0; i < num_fields; i++)
			{
				sprintf(at,"%.*s", (int)lengths[i],row[i] ? row[i] : "NULL");
				at += strlen(at);
				if ((args == 1)  ||  (rowcount > 1 && *func)) // close quotes around the burst args
				{
					strcpy(at, "\"");
					at += 1;
				}
			}
			if (args == 1 && *func) // close quotes around the unburst args
			{
				strcpy(at, "\"");
				at += 1;
			}

			// now invoke function
			if (*func)
			{
				strcat(at, (char*)" )"); // close function call
				DoFunction(func, psBuffer, buffer, csresult);
			}
			else strcpy(buffer, psBuffer + 3); // just pass along output
			buffer += strlen(buffer);
			if (csresult != NOPROBLEM_BIT) break;
		}
		FreeBuffer();
		mysql_free_result(result);
	}
	else  // mysql_store_result() returned nothing; should it have?
	{
		if (mysql_errno(scriptconn))
		{
			sprintf(error, "%s", mysql_error(scriptconn));
			SetUserVariable((char*)"$$db_error", error);	// pass along the error
			Log(USERLOG, error);
			csresult = FAILRULE_BIT;
		}
		// else query does not return data (it was not a SELECT)
	}
	return csresult;
}

#endif
