/** Interface for gzip-like compression and uncompression */

#ifndef ZIFH_
#define ZIFH_

struct zif_t {
    char* uncompressed_buf;
    size_t uncompressed_buf_len;
    char* compressed_buf;
    size_t compressed_buf_len;
};

/** These functions return 0 on success */
int zif_compress(struct zif_t* zs);
int zif_uncompress(struct zif_t* zs);

const char* zif_error(void);

#endif /* define ZIFH_ */
