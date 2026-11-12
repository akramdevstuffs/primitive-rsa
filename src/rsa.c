#include "rsa/rsa.h"
#include "rsa/key_generator.h"
#include "rsa/crypt.h"


int rsa_keygen(rsa_private_key_t **prk, rsa_public_key_t **pbk, size_t key_size)
{

    rsa_private_key_t *private_key = malloc(sizeof(rsa_private_key_t));
    if(private_key==NULL) return -1;
    rsa_public_key_t *public_key = malloc(sizeof(rsa_public_key_t));
    if(public_key==NULL) return -1;


    bigint_t *d, *e, *N;
    int status = kgn_gen_key_pair(&N, &d, &e, key_size);
    if(status!=1) {
        free(private_key);
        free(public_key);
        return 0;
    }

    // Copy 
    bigint_t *e_copy = bigint_copy(e); // Check if copy failed or succeed
    bigint_t *n_copy = bigint_copy(N);

    if(e_copy==NULL || n_copy==NULL){
        if(e_copy) bigint_free(e_copy);
        if(n_copy) bigint_free(n_copy);
        bigint_free(d);
        bigint_free(e);
        bigint_free(N);
        free(private_key);
        free(public_key);
        return -1;
    }

    // Ownership transfer
    private_key->d = d; d = NULL;
    private_key->e = e; e = NULL;
    private_key->n = N; N = NULL;

    public_key->n = n_copy; n_copy = NULL;
    public_key->e = e_copy; e_copy = NULL;

    *prk = private_key; private_key = NULL;
    *pbk = public_key; public_key = NULL;


    return 1;
}

void rsa_free_prk(rsa_private_key_t *prk)
{
    bigint_free(prk->d);
    bigint_free(prk->e);
    bigint_free(prk->n);
    free(prk);
}

void rsa_free_pbk(rsa_public_key_t *pbk)
{
    bigint_free(pbk->e);
    bigint_free(pbk->n);
    free(pbk);
}

/*
* It takes private and public key of sender and receiver and encrypts it
* It perform encryption two times. 
* First it generates sign by using it's private key then encrypt the sign by using public key
* It helps verifying the message originated from the sender who owns the public key (sender side)
*/
bigint_t *rsa_encrypt(const rsa_private_key_t *sender_prk, const rsa_public_key_t *receiver_pbk, const bigint_t *msg)
{
    bigint_t *fingerprint = crt_encrypt(sender_prk->d, sender_prk->n, msg);
    if(fingerprint==NULL) return NULL;

    bigint_t *cipher = crt_encrypt(receiver_pbk->e, receiver_pbk->n, fingerprint);
    bigint_free(fingerprint);
    return cipher;
}

/*
* It takes private and public key of receiver and sender and decrypts it
* It perform decrytion two times. 
* First it generates fingerprint by using it's private key then decrypt the fingerprint by using public key
* It helps verifying the message originated from the sender who owns the public key 
*/
bigint_t *rsa_decrypt(const rsa_private_key_t *receiver_prk, const rsa_public_key_t *sender_pbk, const bigint_t *cipher)
{
    bigint_t *fingerprint = crt_decrypt(receiver_prk->d, receiver_prk->n, cipher);
    if(fingerprint==NULL) return NULL;

    bigint_t *msg = crt_decrypt(sender_pbk->e, sender_pbk->n, fingerprint);
    bigint_free(fingerprint);
    return msg;
}
