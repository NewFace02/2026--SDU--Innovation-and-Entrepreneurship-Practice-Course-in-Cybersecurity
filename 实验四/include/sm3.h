#ifndef SM3_H
#define SM3_H

#include <stddef.h>
#include <stdint.h>

#define SM3_DIGEST_SIZE 32

typedef enum {
    SM3_IMPL_SCALAR = 0,
    SM3_IMPL_AVX2,
    SM3_IMPL_AVX512,
    SM3_IMPL_NEON
} sm3_impl;

void sm3_hash(const uint8_t *data, size_t len, uint8_t out[SM3_DIGEST_SIZE]);

/*
 * Hash equally-sized independent messages in parallel.  The optimized
 * implementations use 8 AVX2, 16 AVX-512, or 4 NEON lanes per batch and
 * transparently fall back to the scalar implementation for a tail batch.
 * Returns 0 on success and -1 if the requested ISA is unavailable.
 */
int sm3_hash_many(sm3_impl impl, const uint8_t *const data[], size_t count,
                  size_t len, uint8_t out[][SM3_DIGEST_SIZE]);

int sm3_impl_available(sm3_impl impl);
const char *sm3_impl_name(sm3_impl impl);

#endif
