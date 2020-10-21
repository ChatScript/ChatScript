/* Routines to implement interface to MS SQL database */

#ifndef MSSQLIMPH_
#define MSSQLIMPH_

int mssql_get_id(void);

// All return 0 on success.

int mssql_init(int id, const char* host, const char* port,
               const char* user, const char* pwd, const char* database);
int mssql_close(int id);

// File operations
int mssql_file_read(int id, void* buf, size_t* buf_size, const char* key);
int mssql_file_write(int id, const void* buf, size_t buf_size, const char* key);

// Script operations
int mssql_exec(int id, const char* statement);
int mssql_store_result(int id);
int mssql_num_fields(int id);
int mssql_free_result(int id);
int mssql_num_rows(int id);
int mssql_fetch_row(int id);
int mssql_fetch_lengths(int id);

// Utility functions
const char* mssql_error(void);
int mssql_set_verbose(int id, bool flag);
int mssql_use_stored_procedures(int id, bool flag);
int mssql_set_tracing(int id, bool flag, const char* filename);
const char* get_mssql_in_conn_str(void);
const char* get_mssql_out_conn_str(void);

#endif /* MSSQLIMPH_ */
