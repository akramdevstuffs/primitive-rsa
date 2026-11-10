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

void bigint_to_string(const bigint_t *a, char *buffer, size_t buffer_size);

bigint_t *bigint_from_uint32(uint32_t value);

bigint_t *bigint_from_uint64(uint64_t value);

void bigint_free(bigint_t *bigint);

int bigint_compare_abs_uint32(const bigint_t *a, uint32_t b);

int bigint_compare(const bigint_t *a, const bigint_t *b);

void bigint_negate(bigint_t *a);

bool bigint_add_abs(const bigint_t *a, const bigint_t *b, bigint_t *target);

bool bigint_sub_abs(const bigint_t *a, const bigint_t *b, bigint_t *target);

int bigint_add_uint32_inplace(bigint_t *a, uint32_t b);

int bigint_sub_uint32_inplace(bigint_t *a, uint32_t b);

bigint_t *bigint_add(const bigint_t *a, const bigint_t *b);

bigint_t *bigint_sub(const bigint_t *a, const bigint_t *b);

int bigint_add_inplace(bigint_t *a, const bigint_t *b);

int bigint_sub_inplace(bigint_t *a, const bigint_t *b);

int bigint_assign(bigint_t *a, const bigint_t *b);

bigint_t *bigint_copy(const bigint_t *a);

bigint_t *bigint_mul_uint32(const bigint_t *a, const uint32_t b);

bigint_t *bigint_mul(const bigint_t *a, const bigint_t *b);

bigint_t *bigint_mul_fft(const bigint_t *a, const bigint_t *b);

bigint_t *bigint_div_uint(const bigint_t *a, const uint32_t b);

void bigint_div_uint_inplace(bigint_t *a, const uint32_t b);

#ifdef __SIZEOF_INT128__

bigint_t *bigint_div_uint64(const bigint_t *a, const uint64_t b);

void bigint_div_uint64_inplace(bigint_t *a, const uint64_t b);

#endif

void bigint_shift_right_inplace(bigint_t *a, size_t shift);

int bigint_shift_left_inplace(bigint_t *a, size_t shift);

bigint_t *bigint_div_inplace(bigint_t *a, const bigint_t *b);

bigint_t *bigint_div(const bigint_t *a, const bigint_t *b);

// bigint_t *bigint_div(const bigint_t *a, const bigint_t *b);

int bigint_mod_inplace(bigint_t *a, const bigint_t *mod);

bigint_t *bigint_mod(const bigint_t *a, const bigint_t *mod);

bigint_t *bigint_modpow(const bigint_t *a, const bigint_t *b, const bigint_t *mod);


// Temporarily expose montgomery through this file
// TODO: Refactor it, separate montgomery implementation from bigint implementation


typedef struct {
    bigint_t *mod;
    bigint_t *inv;
    size_t r;
} montgomery_ctx;


int montgomery_init(const bigint_t *_mod, montgomery_ctx *_mt);
int montgomery_transform(bigint_t *a, const montgomery_ctx *mt);
bigint_t* montgomery_reduce(const bigint_t *a, const montgomery_ctx *mt);
bigint_t* montgomery_mul(const bigint_t *a, const bigint_t *b, const montgomery_ctx *mt);
bigint_t *montgomery_extract(const bigint_t *a, const montgomery_ctx *mt);
void montgomery_free(montgomery_ctx *mt);



#endif // RSA_BIGINT_H
