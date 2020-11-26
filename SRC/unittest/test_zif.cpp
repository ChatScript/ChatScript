#include <string.h>
#include "../zif.h"
#include "catch2.h"

TEST_CASE("compress nominal", "[zif]") {
    char cbuf[500];
    const char* ubuf = "This is a test";
    struct zif_t zs;
    zs.uncompressed_buf = (char*) ubuf;
    zs.uncompressed_buf_len = sizeof(ubuf);
    zs.compressed_buf = cbuf;
    zs.compressed_buf_len = sizeof(cbuf);

    int rv = zif_compress(&zs);
    REQUIRE( 0 == rv );
    REQUIRE( zs.compressed_buf_len < sizeof(cbuf) );

    const char* err = zif_error();
    REQUIRE( err[0] == '\0');
}

TEST_CASE("compress buffer too small", "[zif]") {
    char cbuf[2];
    const char* in_buf = "This is a test";
    struct zif_t zs;
    zs.uncompressed_buf = (char*) in_buf;
    zs.uncompressed_buf_len = sizeof(in_buf);
    zs.compressed_buf = cbuf;
    zs.compressed_buf_len = sizeof(cbuf);

    int rv = zif_compress(&zs);
    REQUIRE( 0 != rv );

    const char* err = zif_error();
    REQUIRE( err[0] != '\0');
}

TEST_CASE("compress buffer null", "[zif]") {
    struct zif_t* zs = nullptr;
    int rv = zif_compress(zs);
    REQUIRE( 0 != rv );

    const char* err = zif_error();
    REQUIRE( err[0] != '\0');
}

TEST_CASE("uncompress nominal", "[zif]") {
    char cbuf[500];
    const char* in_buf = "This is a test of a long line";
    struct zif_t zs_c;
    zs_c.uncompressed_buf = (char*) in_buf;
    zs_c.uncompressed_buf_len = (unsigned long) (strlen(in_buf) + 1);
    zs_c.compressed_buf = cbuf;
    zs_c.compressed_buf_len = sizeof(cbuf);

    int rv_c = zif_compress(&zs_c);
    REQUIRE( 0 == rv_c );
    REQUIRE( zs_c.compressed_buf_len < sizeof(cbuf) );

    char ubuf[500];
    struct zif_t zs_u;
    zs_u.uncompressed_buf = (char*) ubuf;
    zs_u.uncompressed_buf_len = sizeof(ubuf);
    zs_u.compressed_buf = cbuf;
    zs_u.compressed_buf_len = zs_c.compressed_buf_len;

    int rv_u = zif_uncompress(&zs_u);
    REQUIRE( 0 == rv_u );
    REQUIRE( zs_u.compressed_buf_len < sizeof(cbuf) );
    REQUIRE( memcmp(zs_u.uncompressed_buf, in_buf, strlen(in_buf) + 1) == 0 );

    const char* err = zif_error();
    REQUIRE( err[0] == '\0');
}

TEST_CASE("uncompress buffer too small", "[zif]") {
    char cbuf[500];
    const char* in_buf = "This is a test of a long line";
    struct zif_t zs_c;
    zs_c.uncompressed_buf = (char*) in_buf;
    zs_c.uncompressed_buf_len = (unsigned long)strlen(in_buf) + 1;
    zs_c.compressed_buf = cbuf;
    zs_c.compressed_buf_len = sizeof(cbuf);

    int rv_c = zif_compress(&zs_c);
    REQUIRE( 0 == rv_c );
    REQUIRE( zs_c.compressed_buf_len < sizeof(cbuf) );

    char ubuf[5];
    struct zif_t zs_u;
    zs_u.uncompressed_buf = (char*) ubuf;
    zs_u.uncompressed_buf_len = sizeof(ubuf);
    zs_u.compressed_buf = cbuf;
    zs_u.compressed_buf_len = zs_c.compressed_buf_len;

    int rv_u = zif_uncompress(&zs_u);
    REQUIRE( 0 != rv_u );

    const char* err = zif_error();
    REQUIRE( err[0] != '\0');
}

TEST_CASE("uncompress buffer null", "[zif]") {
    struct zif_t* zs = nullptr;
    int rv = zif_uncompress(zs);
    REQUIRE( 0 != rv );

    const char* err = zif_error();
    REQUIRE( err[0] != '\0');
}

TEST_CASE("uncompress buffer tight", "[zif]") {
    char cbuf[500];
    const char* in_buf = "This is a test of a long line";
    struct zif_t zs_c;
    zs_c.uncompressed_buf = (char*) in_buf;
    zs_c.uncompressed_buf_len = (unsigned long)strlen(in_buf);
    zs_c.compressed_buf = cbuf;
    zs_c.compressed_buf_len = sizeof(cbuf);

    int rv_c = zif_compress(&zs_c);
    REQUIRE( 0 == rv_c );
    REQUIRE( zs_c.compressed_buf_len < sizeof(cbuf) );

    char ubuf[500];
    struct zif_t zs_u;
    zs_u.uncompressed_buf = (char*) ubuf;
    zs_u.uncompressed_buf_len = (unsigned long)strlen(in_buf);
    zs_u.compressed_buf = cbuf;
    zs_u.compressed_buf_len = zs_c.compressed_buf_len;

    int rv_u = zif_uncompress(&zs_u);
    REQUIRE( 0 == rv_u );

    // Verify not null terminated if no space for it.
    REQUIRE( ubuf[zs_u.uncompressed_buf_len - 1] != '\0' );

    const char* err = zif_error();
    REQUIRE( err[0] == '\0');
}

TEST_CASE("uncompress empty", "[zif]") {
    char cbuf[500];
    char ubuf[500];
    struct zif_t zs_u;
    zs_u.uncompressed_buf = (char*) ubuf;
    zs_u.uncompressed_buf_len = sizeof(ubuf);
    zs_u.compressed_buf = cbuf;
    zs_u.compressed_buf_len = 0;

    int rv_u = zif_uncompress(&zs_u);
    REQUIRE( 0 == rv_u );

    const char* err = zif_error();
    REQUIRE( err[0] == '\0');
}
