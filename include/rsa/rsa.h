#ifndef RSA_H
#define RSA_H
#include "bigint/bigint.h"

typedef struct {
    bigint_t *n;    // RSA modulus
    bigint_t *e;    // Public key
    bigint_t *d;    // Private key

} rsa_private_key_t;

typedef struct {
    bigint_t *n;    // RSA modulus
    bigint_t *e;    // Public key
} rsa_public_key_t;

int rsa_keygen(rsa_private_key_t **prk, rsa_public_key_t **pbk, size_t key_size);

void rsa_free_prk(rsa_private_key_t *prk);
void rsa_free_pbk(rsa_public_key_t *pbk);

bigint_t *rsa_encrypt(
    const rsa_private_key_t *sender_prk, 
    const rsa_public_key_t *receiver_pbk, 
    const bigint_t *msg
);

bigint_t *rsa_decrypt(
    const rsa_private_key_t *receiver_prk, 
    const rsa_public_key_t *sender_pbk, 
    const bigint_t *cipher
);

#endif