#ifndef RSA_ALG_H
#define RSA_ALG_H

#include "bigint/bigint.h"
#include "debug.h"
#include <stdbool.h>

#ifdef MODULE
    #undef MODULE
#endif

#define MODULE "ALGORITHMS"


uint32_t alg_random_uint32();

void alg_generate_bigint(const bigint_t *ub, bigint_t *dest);

bool alg_miller_rabin(const bigint_t *num);

bigint_t *alg_generate_prime(const bigint_t *lb, const bigint_t *ub);

bigint_t *alg_gcd(const bigint_t *a, const bigint_t *b);

bigint_t *alg_ext_euclied_gcd(const bigint_t *a, const bigint_t *b, bigint_t **x_dest, bigint_t **y_dest);

#endif