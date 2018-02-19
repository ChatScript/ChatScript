#ifndef DISCARDMYSQL
#include "common.h"
extern "C" {
#include "mysql/include/mysql.h"
}
#include "my_sql.h"

MYSQL *usersconn;
MYSQL_STMT *mystmt_userread;
MYSQL_STMT *mystmt_userinsert;
MYSQL_STMT *mystmt_userupdate;

// mysql config options
bool mysqlconf = false;
char mysqlhost[300];
unsigned int mysqlport;
char mysqldb[300];
char mysqluser[300];
char mysqlpasswd[300];
static bool mysqlInited = false;

// user data is stored in this buffer
static char * mysqlfilesbuffer;
// userid is stored in this buffer
char mysqlUserFilename[MAX_WORD_SIZE];

char const * mydefault_usercreate = "CREATE TABLE userfiles (userid varchar(400) PRIMARY KEY, file blob)";
char const * mydefault_userread = "SELECT file FROM userfiles WHERE userid = ?";
char const * mydefault_userinsert = "INSERT INTO userfiles (file, userid) VALUES (?, ?)";
char const * mydefault_userupdate = "UPDATE userfiles SET file = ? WHERE userid = ?";

//
// allow mysql sql to be overridden to allow different db schemas
// override sql must use the same number, kind, and order of bound arguments
// e.g. userread sql must take a userid and return a blob
//      userinsert must take a blob and a userid
//      userupdate must take a blob and a userid
//
char *mysql_userread = 0;
char *mysql_userinsert = 0;
char *mysql_userupdate = 0;
char my_userread_sql[300];
char my_userinsert_sql[300];
char my_userupdate_sql[300];

// not referenced in code
void MySQLShutDown(){}

FILE* mysqlUserCreate(const char* name)
{
	strcpy(mysqlUserFilename,name);
	return (FILE*)mysqlUserFilename;
}

FILE* mysqlUserOpen(const char* name)
{
	strcpy(mysqlUserFilename,name);
	return (FILE*)mysqlUserFilename;
}

int mysqlUserClose(FILE*)
{
	return 0;
}

size_t mysqlUserRead(void* buf,size_t size, size_t count, FILE* file)
{
	char* buffer = (char*)buf;

	// has read sql been overridden?
	const char * readsql = mydefault_userread;
	if (mysql_userread)
	{
		readsql = mysql_userread;
	}

	// prepare statement; will be reused in this process instance
	if (mystmt_userread == NULL)
	{
		mystmt_userread = mysql_stmt_init(usersconn);
		if (mystmt_userread == NULL)
		{
			ReportBug((char*)"FATAL: Failed to allocate MYSQL_STMT: %s", mysql_error(usersconn));
		}
		if (mysql_stmt_prepare(mystmt_userread, readsql, strlen(readsql)))
		{
			ReportBug((char*)"FATAL: Failed to prepare user read STMT: %s", mysql_stmt_error(mystmt_userread));
		}
	}

	unsigned long filenamelength = strlen(mysqlUserFilename);
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));
	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = mysqlUserFilename;
	bind[0].buffer_length = MAX_WORD_SIZE;
	bind[0].is_null = 0;
	bind[0].length = &filenamelength;

	// bind statement parameters
	if (mysql_stmt_bind_param(mystmt_userread, bind))
	{
		ReportBug((char*)"FATAL: Failed to bind user read STMT: %s", mysql_stmt_error(mystmt_userread));
	}

	// execute statement
	if (mysql_stmt_execute(mystmt_userread))
	{
		ReportBug((char*)"FATAL: Failed to execute user read STMT: %s", mysql_stmt_error(mystmt_userread));
	}

	// fetch result set
	if (mysql_stmt_store_result(mystmt_userread))
	{
		ReportBug((char*)"FATAL: Failed to store result of user read STMT: %s", mysql_stmt_error(mystmt_userread));
	}

	my_ulonglong rowcount = mysql_stmt_num_rows(mystmt_userread);
	if (rowcount == 0)
	{
		// no user data found
		size = 0;
		*buffer = 0;
		return size;
	}

	// fetch result row
	my_bool is_null;
	my_bool is_error;
	unsigned long datalength;
	MYSQL_BIND rsbind[1];
	memset(rsbind, 0, sizeof(rsbind));
	rsbind[0].buffer_type = MYSQL_TYPE_BLOB;
	rsbind[0].buffer = buffer;
	rsbind[0].buffer_length = MAX_WORD_SIZE;
	rsbind[0].length = &datalength;
	rsbind[0].is_null = &is_null;
	rsbind[0].error = &is_error;

	if (mysql_stmt_bind_result(mystmt_userread, rsbind))
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

	if (is_null)
	{
		// null result
		size = 0;
		*buffer = 0;
		return size;
	}

	size = datalength;
	// null-terminate buffer
	char *ptr = buffer;
	ptr += size;
	*ptr = 0;
	return size;
}

size_t mysqlUserWrite(const void* buf,size_t size, size_t count, FILE* file)
{
	const char *insertsql = mydefault_userinsert;
	const char *updatesql = mydefault_userupdate;
	if (mysql_userinsert)
	{
		// insert has been overridden
		insertsql = mysql_userinsert;
		// if insert is overridden, don't use the default update sql
		// ie insert sql could be a stored proc call that does an insert/update
		updatesql = 0;
	}
	if (mysql_userupdate)
	{
		updatesql = mysql_userupdate;
	}

	char * buffer = (char *) buf;

	if (mystmt_userinsert == NULL)
	{
		mystmt_userinsert = mysql_stmt_init(usersconn);
		if (mystmt_userinsert == NULL)
		{
			ReportBug((char*)"FATAL: Failed to allocate mysql insert stmt: %s", mysql_error(usersconn));
		}
		if (mysql_stmt_prepare(mystmt_userinsert, insertsql, strlen(insertsql)))
		{
			ReportBug((char*)"FATAL: Failed to prepare user insert STMT: %s", mysql_stmt_error(mystmt_userinsert));
		}
	}

	// parameter bindings:
	// arg1 is file data (blob)
	// arg2 is userid (varchar)
	unsigned long filenamelength = strlen(mysqlUserFilename);
	unsigned long filedatalength = size * count;	// buffer size, function param

	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_BLOB;
	bind[0].buffer = buffer;
	bind[0].buffer_length = MAX_WORD_SIZE;
	bind[0].is_null = 0;
	bind[0].length = &filedatalength;

	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = mysqlUserFilename;
	bind[1].buffer_length = MAX_WORD_SIZE;
	bind[1].is_null = 0;
	bind[1].length = &filenamelength;

	// bind statement parameters
	if (mysql_stmt_bind_param(mystmt_userinsert, bind))
	{
		ReportBug((char*)"FATAL: Failed to bind user insert STMT: %s", mysql_stmt_error(mystmt_userinsert));
	}

	// execute statement
	// this can fail if a record already exists
	my_ulonglong affectedRows = 0;
	if (mysql_stmt_execute(mystmt_userinsert))
	{
		// if we fail, assume it is due to a primary key violation, and try to update
		const char * error_msg = mysql_stmt_error(mystmt_userinsert);
		unsigned int error_num = mysql_stmt_errno(mystmt_userinsert);
		Log(STDTRACELOG, "SQL Error: %d %s", error_num, error_msg);
	}
	else
	{
		affectedRows = mysql_stmt_affected_rows(mystmt_userinsert);
	}

	if (affectedRows == 1)
	{
		// insert succeeded
		return filedatalength;
	}

	//
	// insert failed; try update
	//
	if (mystmt_userupdate == NULL)
	{
		//
		// initialize update prepared statement
		//
		mystmt_userupdate = mysql_stmt_init(usersconn);
		if (mystmt_userupdate == NULL)
		{
			ReportBug((char*)"FATAL: Failed to allocate mysql update stmt: %s", mysql_error(usersconn));
		}
		if (mysql_stmt_prepare(mystmt_userupdate, updatesql, strlen(updatesql)))
		{
			ReportBug((char*)"FATAL: Failed to prepare user update STMT: %s", mysql_stmt_error(mystmt_userupdate));
		}
	}

	//
	// reuse bind data structure
	//
	memset(bind, 0, sizeof(bind));
	filenamelength = strlen(mysqlUserFilename);
	filedatalength = size * count;	// buffer size, function param

	bind[0].buffer_type = MYSQL_TYPE_BLOB;
	bind[0].buffer = buffer;
	bind[0].buffer_length = MAX_WORD_SIZE;
	bind[0].is_null = 0;
	bind[0].length = &filedatalength;

	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = mysqlUserFilename;
	bind[1].buffer_length = MAX_WORD_SIZE;
	bind[1].is_null = 0;
	bind[1].length = &filenamelength;

	// bind statement parameters
	if (mysql_stmt_bind_param(mystmt_userupdate, bind))
	{
		ReportBug((char*)"FATAL: Failed to bind user update STMT: %s", mysql_stmt_error(mystmt_userupdate));
	}

	// execute statement
	if (mysql_stmt_execute(mystmt_userupdate))
	{
		ReportBug((char*)"FATAL: Failed to execute user update STMT: %s", mysql_stmt_error(mystmt_userupdate));
	}

	affectedRows = mysql_stmt_affected_rows(mystmt_userupdate);
	if (affectedRows != 1)
	{
		ReportBug((char*)"FATAL: affected rows mismatch for user update STMT: %s", mysql_stmt_error(mystmt_userupdate));
	}
	// insert succeeded
	return filedatalength;
}

// initialize the user system
void MySQLUserFilesCode()
{
	if (mysqlInited) return;
#ifdef WIN32
	if (InitWinsock() == FAILRULE_BIT) ReportBug((char*)"FATAL: WSAStartup failed\r\n");
#endif

	usersconn = mysql_init(NULL);
	if (usersconn == NULL)
	{
		ReportBug((char*)"FATAL: Failed to allocate MYSQL");
	}

	// @see https://dev.mysql.com/doc/refman/5.7/en/mysql-real-connect.html
	usersconn = mysql_real_connect(usersconn, mysqlhost, mysqluser, mysqlpasswd, mysqldb, mysqlport, NULL, 0);
	if (usersconn == NULL)
	{
		ReportBug((char*)"FATAL: Failed to connect to MYSQL: %s", mysql_error(usersconn));
	}

	// these are dynamically stored, so CS can be a DLL.
	mysqlfilesbuffer = AllocateBuffer();  // stays globally locked down - may not be big enough
	userFileSystem.userCreate = mysqlUserCreate;
	userFileSystem.userOpen = mysqlUserOpen;
	userFileSystem.userClose = mysqlUserClose;
	userFileSystem.userRead = mysqlUserRead;
	userFileSystem.userWrite = mysqlUserWrite;
	userFileSystem.userDelete = NULL;
	filesystemOverride = MYSQLFILES;

	mysqlInited = true;
}


void MySQLserFilesCloseCode(){ /* mysqlInited = false; */}

extern char* mySQLparams;


FunctionResult MySQLInitCode(char* buffer)
{
   MYSQL *conn;
   MYSQL_RES *res;
   MYSQL_ROW row;
   char *myserver = "localhost";
   char *user = "root";
   char *password = "mysql"; 
   char *database = "mysql";
   conn = mysql_init(NULL);

   if (!mysql_real_connect(conn, myserver, user, password, database, 0, NULL, 0)) {
      fprintf(stderr, "%s\r\n", mysql_error(conn));
      exit(1);
   }
   if (mysql_query(conn, "show tables")) {
      fprintf(stderr, "%s\r\n", mysql_error(conn));
      exit(1);
   }
   res = mysql_use_result(conn);
   (*printer)("MySQL Tables in mysql database:\n");
   while ((row = mysql_fetch_row(res)) != NULL)
      (*printer)("%s \n", row[0]);
   mysql_free_result(res);
   mysql_close(conn);

	return FAILRULE_BIT;
} 

FunctionResult MySQLCloseCode(char* buffer){return FAILRULE_BIT;}
FunctionResult MySQLExecuteCode(char* buffer){return FAILRULE_BIT;}


#endif
