#include "rsa/crypt.h"
#include <stdlib.h>

bigint_t *crt_decrypt(const bigint_t *key, const bigint_t *N, const bigint_t *cipher)
{
    return bigint_modpow(cipher, key, N);
}

bigint_t *crt_encrypt(const bigint_t *key, const bigint_t *N, const bigint_t *msg)
{
    return bigint_modpow(msg, key, N);
}