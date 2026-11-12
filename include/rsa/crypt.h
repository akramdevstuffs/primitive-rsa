#ifndef RSA_CRYPT_H
#define RSA_CRYPT_H

#include "rsa/key_generator.h"
#include "bigint/bigint.h"

bigint_t *crt_encrypt(const bigint_t *key, const bigint_t *N, const bigint_t *msg);

bigint_t *crt_decrypt(const bigint_t *key, const bigint_t *N, const bigint_t *cipher);

#endif