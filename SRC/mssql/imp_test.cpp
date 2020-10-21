#include <cassert>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../mssql_imp.h"
#include "timer.h"

#define CATCH_CONFIG_MAIN
#include "catch2.h"

#include <stddef.h>

#ifdef __GNUC__ // ignore warnings from common.h
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#pragma GCC diagnostic ignored "-Wconversion-null"
#include "../common.h"
#pragma GCC diagnostic pop

#else  // not __GNUC__ means Visual Studio
#include "../common.h"
#endif // __GNUC__

#ifdef DISCARDMICROSOFTSQL
#error Cannot build mssql unittests with DISCARDMICROSOFTSQL set
#endif 

struct ms_server_info_t {
    const char* host = "abtodb3";
    const char* port = "1433";
    const char* user = "ja_webusr";
    const char* password = "h0s3i1F";
    const char* database = "chatscript";
};

static void randomize_data(char* write_text, size_t write_len, int seed) {
    for (size_t i=0; i < write_len; ++i) {
        write_text[i] = (char)i;
    }
    srand(seed);
    for (size_t i=0; i < write_len; ++i) {
        int j = rand() % write_len;
        char tmp = write_text[i];
        write_text[i] = write_text[j];
        write_text[j] = tmp;
    }
}

TEST_CASE("get_id", "[get_id]")
{
    int id = mssql_get_id();
    REQUIRE(0 == id);
    mssql_close(id);
}


TEST_CASE("init mssql", "[factorial]") 
{

    int id = mssql_get_id();
    REQUIRE(0 == id);

    bool verbose = false;
    int rv1 = mssql_set_verbose(id, verbose);
    REQUIRE(0 == rv1);

    ms_server_info_t ms;
    int rv = mssql_init(id, ms.host, ms.port, ms.user, ms.password, ms.database);
    REQUIRE(0 == rv);

    const char* expected_in_str = "DRIVER=ODBC Driver 17 for SQL Server; "
        "SERVER=abtodb3,1433; DATABASE=chatscript; UID=ja_webusr; PWD=h0s3i1F";
    const char* actual_in_str = get_mssql_in_conn_str();
    REQUIRE(0 == strcmp(expected_in_str, actual_in_str));

    mssql_close(id);
}

TEST_CASE("file_write", "[file_write]")
{
    int id = mssql_get_id();
    REQUIRE(0 == id);

    ms_server_info_t ms;
    int rv0 = mssql_init(id, ms.host, ms.port, ms.user, ms.password, ms.database);
    REQUIRE(0 == rv0);

    const char* write_text_0 = "Bruce will never read this.";
    size_t write_len_0 = strlen(write_text_0);
    const char* key = "brillig";
    rv0 = mssql_file_write(id, write_text_0, write_len_0, key);
    REQUIRE(0 == rv0);

    const char* write_text_1 = "Sue will never read this.";
    size_t write_len_1 = strlen(write_text_1);
    int rv1 = mssql_file_write(id, write_text_1, write_len_1, key);
    REQUIRE(0 == rv1);

    mssql_close(id);
}

TEST_CASE("file_read", "[file_read]")
{
    int id = mssql_get_id();
    REQUIRE(0 == id);

    bool verbose_flag = false;
    int rv1 = mssql_set_verbose(id, verbose_flag);
    REQUIRE(0 == rv1);

    ms_server_info_t ms;
    int rv0 = mssql_init(id, ms.host, ms.port, ms.user, ms.password, ms.database);
    REQUIRE(0 == rv0);

    //   struct timeval start, stop;
    int num_times = 3;
    for (int i = 1; i <= num_times; ++i) {
        char write_buf[5000];
        size_t write_len = sizeof(write_buf) - 1000 + (i * 50) + (rand() % 100);
        randomize_data(write_buf, write_len, i); // test with random binary data
        snprintf(write_buf, sizeof(write_buf), "Bruce will never read this %d times.", i + 12);
        const char* key = "brillig";
        int rv_write = mssql_file_write(id, write_buf, write_len, key);
        REQUIRE(0 == rv_write);

        char read_buf[5000];
        size_t read_len = sizeof(read_buf);
        int rv_read = mssql_file_read(id, read_buf, &read_len, key);
        REQUIRE(0 == rv_read);
        REQUIRE(write_len == read_len);
        REQUIRE(memcmp(write_buf, read_buf, write_len) == 0);
    }

    mssql_close(id);
}

TEST_CASE("file_read_write_performance", "[file_read_write_performance]")
{
    bool skip_test = false;
    if (skip_test) {
        return;
    }

    int id = mssql_get_id();
    REQUIRE(0 == id);

    ms_server_info_t ms;
    int rv0 = mssql_init(id, ms.host, ms.port, ms.user, ms.password, ms.database);
    REQUIRE(0 == rv0);

    bool verbose_flag = false;
    int rv1 = mssql_set_verbose(id, verbose_flag);
    REQUIRE(0 == rv1);

    char write_buf[5000];
    size_t write_len = sizeof(write_buf);
    randomize_data(write_buf, write_len, 42);

    //    struct timeval start, stop;
    long long dt_write_us, dt_read_us;
    long long dt_sum_us = 0;

    FILE* f = NULL;
    const char* filename = "timed_data.csv";
#ifdef __GNUC__
    f = fopen(filename, "w");
#else
    int err = fopen_s(&f, filename, "w");
    REQUIRE(err == 0);
#endif
    REQUIRE(f != NULL);

    int num_times = 10;
    printf("Starting performance test of %d read/write pairs.\n", num_times);
    for (int i = 0; i < num_times; ++i) {
        snprintf(write_buf, sizeof(write_buf), "Bruce will never read this %d times.", i);
        const char* key = "brillig";
        char read_buf[5010];
        size_t read_len = sizeof(read_buf);

        start_timer();
        int read_rv = mssql_file_read(id, read_buf, &read_len, key);
        REQUIRE(0 == read_rv);
        dt_read_us = stop_timer_us();

        start_timer();
        int write_rv = mssql_file_write(id, write_buf, write_len, key);
        REQUIRE(0 == write_rv);
        dt_write_us = stop_timer_us();
        fprintf(f, "%lld, %lld\n", dt_write_us, dt_read_us);
        dt_sum_us += dt_write_us + dt_read_us;
    }
    double dt_ms = (double)dt_sum_us / (num_times * 1000.);
    printf("\n%d write and read ops took %g ms each\n",
        num_times, dt_ms);

    mssql_close(id);
    if (f != 0) {
        printf("wrote file %s\n", filename);
        fclose(f);
    }
}

TEST_CASE("file_read_only_key_exists", "[file_read_only_key_exists]")
{
    int id = mssql_get_id();
    REQUIRE(0 == id);

    ms_server_info_t ms;
    int rv0 = mssql_init(id, ms.host, ms.port, ms.user, ms.password, ms.database);
    REQUIRE(0 == rv0);

    const char* key = "brillig";

    char read_buf[10000];
    size_t read_len = sizeof(read_buf);
    int rv_read = mssql_file_read(id, read_buf, &read_len, key);
    REQUIRE(0 == rv_read);

    REQUIRE(0 < read_len);
    mssql_close(id);
}

TEST_CASE("file_read_only_no_key_exists", "[file_read_only_no_key_exists]")
{
    int id = mssql_get_id();
    REQUIRE(0 == id);

    ms_server_info_t ms;
    int rv0 = mssql_init(id, ms.host, ms.port, ms.user, ms.password, ms.database);
    REQUIRE(0 == rv0);

    const char* key = "fessik";

    char read_buf[10000];
    size_t read_len = sizeof(read_buf);
    int rv_read = mssql_file_read(id, read_buf, &read_len, key);
    REQUIRE(0 == rv_read);

    REQUIRE(0 == read_len);

    mssql_close(id);
}
