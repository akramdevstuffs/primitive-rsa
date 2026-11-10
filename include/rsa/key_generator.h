#ifndef RSA_KGN_H
#define RSA_KGN_H

#include "rsa/debug.h"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <bigint/bigint.h>
#include "rsa/algorithms.h"
#include <stdbool.h>

#ifdef MODULE
    #undef MODULE
#endif

#define MODULE "KEY_GENERATOR"


void kgn_set_seed(const uint32_t *var);

int kgn_generate_pq(bigint_t **p_dest, bigint_t **q_dest, size_t size);

bigint_t *kgn_compute_inverse(const bigint_t *d, const bigint_t *mod);

int kgn_generate_skey_pair(const bigint_t *p, const bigint_t *q, bigint_t **d_dest, bigint_t **e_dest, size_t size);

int kgn_gen_key_pair(bigint_t **n_dest, bigint_t **d_dest, bigint_t **e_dest, size_t mx_size);

#endif