/** Implementation for gzip-like compression and uncompression interface */

#ifndef DISCARD_TEXT_COMPRESSION

#include <stdio.h>
#include "zlib/zlib.h"

#include "zif.h"

static char zif_error_buf[132];

const char* zif_error(void)
{
    return zif_error_buf;
}


int zif_compress(struct zif_t* zs)
{
    if (zs == nullptr) {
        snprintf(zif_error_buf, sizeof(zif_error_buf),
                "FATAL: zif: null zif_t in zif_compress");
        return -1;
    }
    unsigned long cbl = (unsigned long) zs->compressed_buf_len;
    int rv = compress((Bytef*) zs->compressed_buf,
                      &cbl,
                      (const Bytef*) zs->uncompressed_buf,
                      (unsigned long) zs->uncompressed_buf_len);
    zs->compressed_buf_len = cbl;
    if (rv == Z_OK) {
        zif_error_buf[0] = '\0';
        return 0;
    }
    else if (rv == Z_MEM_ERROR) {
        snprintf(zif_error_buf, sizeof(zif_error_buf),
                "FATAL: zif: not enough memory");
        return -2;
    }
    else if (rv == Z_BUF_ERROR) {
        snprintf(zif_error_buf, sizeof(zif_error_buf),
                "FATAL: zif: output buf too small");
        return -3;
    }
    else {
        snprintf(zif_error_buf, sizeof(zif_error_buf),
                "FATAL: zif: unexpected error in zif_compress %d", rv);
        return -4;
    }
}

int zif_uncompress(struct zif_t* zs)
{
    if (zs == nullptr) {
        snprintf(zif_error_buf, sizeof(zif_error_buf),
                "FATAL: zif: null zif_t in zif_uncompress");
        return -1;
    }
    if (zs->compressed_buf_len == 0) {
        zs->uncompressed_buf_len = 0;
        zs->uncompressed_buf[0] = '\0';
        return 0;
    }
    size_t original_ulen = zs->uncompressed_buf_len;
    unsigned long ubl = (unsigned long) zs->uncompressed_buf_len;
    int rv = uncompress((Bytef*) zs->uncompressed_buf,
                        &ubl,
                        (const Bytef*) zs->compressed_buf,
                        (unsigned long) zs->compressed_buf_len);
    zs->uncompressed_buf_len = ubl;
    if (rv == Z_OK) {
        zif_error_buf[0] = '\0'; // clear error message

        // null terminate if there is space
        if (zs->uncompressed_buf_len < original_ulen) {
            ((char*) zs->uncompressed_buf)[zs->uncompressed_buf_len] = '\0';
        }
        return 0;
    }
    else if (rv == Z_MEM_ERROR) {
        snprintf(zif_error_buf, sizeof(zif_error_buf),
                "FATAL: zif: not enough memory");
        return -2;
    }
    else if (rv == Z_BUF_ERROR) {
        snprintf(zif_error_buf, sizeof(zif_error_buf),
                "FATAL: zif: output buf too small");
        return -3;
    }
    else if (rv == Z_DATA_ERROR) {
        snprintf(zif_error_buf, sizeof(zif_error_buf),
                "FATAL: zif: corrupted data");
        return -4;
    }
    else {
        snprintf(zif_error_buf, sizeof(zif_error_buf),
                "FATAL: zif: unexpected error in zif_uncompress %d", rv);
        return -5;
    }
}

#endif // DISCARD_TEXT_COMPRESSION
