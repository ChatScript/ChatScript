/* Routines to implement interface to MS SQL database */

#ifndef MSSQLIMPH_
#define MSSQLIMPH_

enum ConnectionId { None = -1, User = 0, Script = 1 };
int mssql_init_db_struct(ConnectionId c_id);

// All return 0 on success.

int mssql_init(int id, const char* host, const char* port,
               const char* user, const char* pwd, const char* database);
int mssql_close(int id);

// File operations
int mssql_file_read(int id, void* buf, size_t* buf_size, const char* key);
int mssql_file_write(int id, const void* buf, size_t buf_size, const char* key);

// Utility functions
const char* mssql_error(void);
int mssql_set_verbose(int id, bool flag);
int mssql_use_stored_procedures(int id, bool flag);
int mssql_set_tracing(int id, bool flag, const char* filename);
const char* get_mssql_in_conn_str(void);
const char* get_mssql_out_conn_str(void);

int mssql_maybe_compress(char* c_buf, size_t* c_size, const char* p_buf, size_t p_size);
int mssql_maybe_uncompress(const char* c_buf, size_t c_size, char* p_buf, size_t* p_size);

// For testing
bool is_connected(ConnectionId id);

#endif /* MSSQLIMPH_ */
