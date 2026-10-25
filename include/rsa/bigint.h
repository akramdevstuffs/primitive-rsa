#ifndef RSA_BIGINT_H
#define RSA_BIGINT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


typedef struct {
    uint32_t *limbs;
    size_t size;
    size_t capacity;
    bool negative;
} bigint_t;

bigint_t *bigint_create(size_t capacity);

bigint_t *bigint_from_string(const char *s);

void bigint_free(bigint_t *bigint);

int bigint_compare(const bigint_t *a, const bigint_t *b);

bool *bigint_add_abs(const bigint_t *a, const bigint_t *b, bigint_t *target);

bool bigint_sub_abs(const bigint_t *a, const bigint_t *b, bigint_t *target);

bigint_t *bigint_add(const bigint_t *a, const bigint_t *b);

int bigint_assign(bigint_t *a, const bigint_t *b);

bigint_t *bigint_mul(const bigint_t *a, const bigint_t *b);

bigint_t *bigint_div_uint(const bigint_t *a, const uint32_t b);

void bigint_div_uint_inplace(bigint_t *a, const uint32_t b);

#ifdef __SIZEOF_INT128__

bigint_t *bigint_div_uint64(const bigint_t *a, const uint64_t b);

void bigint_div_uint64_inplace(bigint_t *a, const uint64_t b);

#endif

void bigint_shift_right_inplace(bigint_t *a, size_t shift);

int bigint_shift_left_inplace(bigint_t *a, size_t shift);


bigint_t *bigint_div(const bigint_t *a, const bigint_t *b);

bigint_t *bigint_mod(const bigint_t *a, const bigint_t *b);


#endif // RSA_BIGINT_H
