#include "rsa/crypt.h"
#include "../test_helpers.h"
#include "rsa/key_generator.h"
#include "rsa/algorithms.h"

void test_crt_encrypt_decrypt(void){
    printf("\n --- test_crt_encrypt_decrypt ---\n");

    // Generate key
    bigint_t *d,*e,*N;
    const int key_size = 256;  // 256 bits or 32 bytes
    int status = kgn_gen_key_pair(&N, &d, &e, key_size);
    CHECK(status==1, "Key generation failed");

    for(int i = 0; i<10; i++){
        // Generate a random number
        bigint_t *msg = bigint_create((key_size+32)/8); // allocates 288 bits
        alg_generate_bigint(N, msg);

        CHECK(bigint_compare(msg, N)==-1, "Message is larger than ub");

        bigint_t *cipher = crt_encrypt(e, N, msg);
        CHECK(cipher!=NULL, "Encryption failed");

        bigint_t *d_msg = crt_decrypt(d, N, cipher);
        CHECK(d_msg!=NULL, "Decription failed");

        CHECK(bigint_compare(d_msg, msg)==0, "Message is not preserved after encryption and decryption");

        bigint_free(msg);
        bigint_free(cipher);
        bigint_free(d_msg);
    }
    bigint_free(d);
    bigint_free(e);
    bigint_free(N);
}


void test_crypt(void){
    printf("\n ============ TEST RSA/CRYPT ========\n");
    test_crt_encrypt_decrypt();

    printf("\n ============= END RSA/CRYPT ========\n");
}