#include <cassert>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../mssql_imp.h"
#include "timer.h"

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
    const char* host = "devdb5";
    // const char* host = "abtodb3";
    const char* port = "1433";
    const char* user = "chatscript_user";
    // const char* user = "ja_webusr";
    const char* password = "h0s3i1F";
    const char* database = "chatscript";
};

#include "../userCache.h"

// #define SKIP_TEST = 1

// *** Must match declaration in userCache.h ***
unsigned int userCacheSize = 10000;

// *** Must match declaration in mainSystem.h ***
int gzip;

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

#ifndef SKIP_TEST
TEST_CASE("get_id", "[mssql]")
{
    int id = mssql_init_db_struct(User);
    REQUIRE(0 == id);
    mssql_close(id);
}
#endif // SKIP_TEST


#ifndef SKIP_TEST
TEST_CASE("init mssql", "[mssql]") 
{

    int id = mssql_init_db_struct(User);
    REQUIRE(0 == id);

    bool verbose = false;
    int rv1 = mssql_set_verbose(id, verbose);
    REQUIRE(0 == rv1);

    ms_server_info_t ms;
    int rv = mssql_init(id, ms.host, ms.port, ms.user, ms.password, ms.database);
    REQUIRE(0 == rv);

    char expected_in_str[200];
    sprintf(expected_in_str, "DRIVER=ODBC Driver 17 for SQL Server; "
            "SERVER=%s,1433; DATABASE=chatscript; UID=chatscript_user; PWD=h0s3i1F",
            ms.host);
            
    const char* actual_in_str = get_mssql_in_conn_str();
    REQUIRE(0 == strcmp(expected_in_str, actual_in_str));

    mssql_close(id);
}
#endif // SKIP_TEST

#ifndef SKIP_TEST
TEST_CASE("file_write", "[mssql]")
{
    int id = mssql_init_db_struct(User);
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

    // Clean up
    const char* empty_text = "";
    size_t empty_len = strlen(empty_text) + 1;
    int rv2 = mssql_file_write(id, empty_text, empty_len, key);
    REQUIRE(0 == rv2);
    
    mssql_close(id);
}
#endif // SKIP_TEST

#ifndef SKIP_TEST
TEST_CASE("file_read", "[mssql]")
{
    int id = mssql_init_db_struct(Script);
    REQUIRE(1 == id);

    bool verbose_flag = false;
    int rv1 = mssql_set_verbose(id, verbose_flag);
    REQUIRE(0 == rv1);

    ms_server_info_t ms;
    int rv0 = mssql_init(id, ms.host, ms.port, ms.user, ms.password, ms.database);
    REQUIRE(0 == rv0);

    int num_times = 3;
    const char* key = "brillig";
    for (int i = 1; i <= num_times; ++i) {
        char write_buf[5000];
        size_t write_len = sizeof(write_buf) - 1000 + (i * 50) + (rand() % 100);
        randomize_data(write_buf, write_len, i); // test with random binary data
        snprintf(write_buf, sizeof(write_buf), "Bruce will never read this %d times.", i + 12);
        int rv_write = mssql_file_write(id, write_buf, write_len, key);
        REQUIRE(0 == rv_write);

        char read_buf[5000];
        size_t read_len = sizeof(read_buf);
        int rv_read = mssql_file_read(id, read_buf, &read_len, key);
        REQUIRE(0 == rv_read);
        REQUIRE(write_len == read_len);
        REQUIRE(memcmp(write_buf, read_buf, write_len) == 0);
    }

    // Clean up
    const char* empty_text = "";
    size_t empty_len = strlen(empty_text) + 1;
    int rv_write = mssql_file_write(id, empty_text, empty_len, key);
    REQUIRE(0 == rv_write);

    mssql_close(id);
}
#endif // SKIP_TEST

#ifndef SKIP_TEST
TEST_CASE("file_read_write_performance", "[mssql]")
{
    bool skip_test = true;
    if (skip_test) {
        return;
    }

    int id = mssql_init_db_struct(User);
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
    const char* key = "brillig";
    for (int i = 0; i < num_times; ++i) {
        snprintf(write_buf, sizeof(write_buf), "Bruce will never read this %d times.", i);
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

    // Clean up
    const char* empty_text = "";
    size_t empty_len = strlen(empty_text) + 1;
    int rv_write = mssql_file_write(id, empty_text, empty_len, key);
    REQUIRE(0 == rv_write);

    mssql_close(id);
    if (f != 0) {
        printf("wrote file %s\n", filename);
        fclose(f);
    }
    mssql_close(id);
}
#endif // SKIP_TEST

#ifndef SKIP_TEST
TEST_CASE("file_read_only_key_exists", "[mssql]")
{
    int id = mssql_init_db_struct(User);
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
#endif // SKIP_TEST

#ifndef SKIP_TEST
TEST_CASE("file_read_only_no_key_exists", "[mssql]")
{
    int id = mssql_init_db_struct(User);
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
#endif // SKIP_TEST

#ifndef SKIP_TEST
TEST_CASE("mssql_maybe_compress_nominal_gzip_1", "[mssql]")
{
    char c_buf[200];
    size_t c_size = sizeof(c_buf);
    char p_buf[] = "This is a test";
    size_t p_size = sizeof(p_buf);

    memset(c_buf, 0, c_size);
    
    gzip = 1;
    int rv_c = mssql_maybe_compress(c_buf, &c_size, p_buf, p_size);
    REQUIRE(0 == rv_c);
    REQUIRE(strstr(c_buf, "gzip") == c_buf);
}
#endif // SKIP_TEST

#ifndef SKIP_TEST
TEST_CASE("mssql_maybe_compress_nominal_gzip_0", "[mssql]")
{
    char c_buf[200];
    size_t c_size = sizeof(c_buf);
    char p_buf[] = "This is a test";
    size_t p_size = sizeof(p_buf);

    memset(c_buf, 0, c_size);
    
    gzip = 0;
    int rv_c = mssql_maybe_compress(c_buf, &c_size, p_buf, p_size);
    REQUIRE(0 == rv_c);
    REQUIRE(strstr(c_buf, "norm") == c_buf);
}
#endif // SKIP_TEST

#ifndef SKIP_TEST
TEST_CASE("mssql_maybe_uncompress_nominal", "[mssql]")
{
    for (int g=0; g <= 1; ++g) {
        gzip = g;

        char c_buf[200];
        size_t c_size = sizeof(c_buf);
        char p_buf[] = "This is a test";
        size_t p_size = sizeof(p_buf);

        memset(c_buf, 0, c_size);

        int rv_c = mssql_maybe_compress(c_buf, &c_size, p_buf, p_size);

        char rcv_buf[200];
        size_t rcv_size = sizeof(rcv_buf);
        memset(rcv_buf, 0, rcv_size);

        int rv_r = mssql_maybe_uncompress(c_buf, c_size, rcv_buf, &rcv_size);
        REQUIRE(0 == rv_r);
        REQUIRE(rcv_size == p_size);
        REQUIRE(strcmp(p_buf, rcv_buf) == 0);
    }
}
#endif // SKIP_TEST

#ifndef SKIP_TEST
TEST_CASE("mssql_maybe_uncompress_0_length", "[mssql]")
{
    for (int g=0; g <= 1; ++g) {
        gzip = g;

        char c_buf[200];
        size_t c_size = 0;
        char rcv_buf[200];
        size_t rcv_size = sizeof(rcv_buf);

        int rv_r = mssql_maybe_uncompress(c_buf, c_size, rcv_buf, &rcv_size);
        REQUIRE(0 == rv_r);
        REQUIRE(0 == rcv_size);
    }
}
#endif // SKIP_TEST

#ifndef SKIP_TEST
TEST_CASE("mssql_maybe_uncompress_comp_nullptr", "[mssql]")
{
    for (int g=0; g <= 1; ++g) {
        gzip = g;

        char c_buf[200];
        size_t c_size = sizeof(c_buf);
        char rcv_buf[200];
        size_t rcv_size = sizeof(rcv_buf);

        int rv_r = mssql_maybe_uncompress(nullptr, c_size, rcv_buf, &rcv_size);
        REQUIRE(rv_r < 0);
    }
}
#endif // SKIP_TEST

#ifndef SKIP_TEST
TEST_CASE("mssql_maybe_uncompress_mismatch", "[mssql]")
{
    for (int g=0; g <= 1; ++g) {
        gzip = g;

        char c_buf[200];
        size_t c_size = sizeof(c_buf);
        char p_buf[] = "This is a test";
        size_t p_size = sizeof(p_buf);
        memset(c_buf, 0, c_size);

        int rv_c = mssql_maybe_compress(c_buf, &c_size, p_buf, p_size);

        char rcv_buf[200];
        size_t rcv_size = sizeof(rcv_buf);
        memset(rcv_buf, 0, rcv_size);

        gzip = !gzip;           // create mismatch
        
        int rv_r = mssql_maybe_uncompress(c_buf, c_size, rcv_buf, &rcv_size);
        REQUIRE(rcv_size == 0);
        REQUIRE(rv_r == 0);
    }
}
#endif // SKIP_TEST

#ifndef SKIP_TEST
TEST_CASE("mssql_maybe_uncompress_header_garbage", "[mssql]")
{
    for (int g=0; g <= 1; ++g) {
        gzip = g;

        char c_buf[200];
        size_t c_size = sizeof(c_buf);
        char p_buf[] = "This is a test";
        size_t p_size = sizeof(p_buf);
        memset(c_buf, 0, c_size);

        int rv_c = mssql_maybe_compress(c_buf, &c_size, p_buf, p_size);

        char rcv_buf[200];
        size_t rcv_size = sizeof(rcv_buf);
        memset(rcv_buf, 0, rcv_size);

        c_buf[0] = 'x';         // add garbage to header
        int rv_r = mssql_maybe_uncompress(c_buf, c_size, rcv_buf, &rcv_size);
        REQUIRE(rv_r < 0);
    }
}
#endif // SKIP_TEST

#ifndef SKIP_TEST
TEST_CASE("mssql_script_interface", "[mssql]")
{
    char* key1 = "bbb";
    struct ms_server_info_t ms;
    mssql_init(Script, ms.host, ms.port, ms.user, ms.password, ms.database);

    char* dummy_data = "12345";
    size_t dummy_size = strlen(dummy_data) + 1;
    int rv_w, rv_r;
    rv_w = mssql_file_write(Script, dummy_data, dummy_size, (const char*) key1);
    REQUIRE(rv_w == 0);

    char read_buf[100];
    size_t read_len = sizeof(read_buf);
    rv_r = mssql_file_read(Script, read_buf, &read_len, (const char*) key1);
    REQUIRE(rv_r == 0);
    REQUIRE(read_len == dummy_size);
    REQUIRE(memcmp(dummy_data, read_buf, read_len) == 0);

    // Clean up
    char* empty_data = "";
    size_t empty_size = strlen(empty_data) + 1;
    rv_w = mssql_file_write(Script, empty_data, empty_size, (const char*) key1);
    REQUIRE(rv_w == 0);
}
#endif // SKIP_TEST

