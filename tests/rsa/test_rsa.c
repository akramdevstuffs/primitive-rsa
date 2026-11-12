#include "../test_helpers.h"
#include <stdlib.h>
#include <stdio.h>
#include "rsa/algorithms.h"
#include "rsa/rsa.h"

void test_algorithms(void);
void test_key_generator(void);
void test_crypt(void);


static void test_rsa_keygen(void){
    printf("\n ----- test_rsa_keygen ----- \n");
    rsa_private_key_t *prk = NULL;
    rsa_public_key_t *pbk = NULL;
    const size_t key_size = 256;
    int status = rsa_keygen(&prk, &pbk, key_size);
    CHECK(status==1, "Key generation failed");
    CHECK(prk!=NULL && pbk!=NULL, "pointers are NULL after key generation");

    rsa_free_pbk(pbk);
    rsa_free_prk(prk);
}

static void test_rsa_encrypt_decrypt(void){
    printf("\n ----- test_rsa_encrypt_decrypt ----- \n");

    rsa_private_key_t *prk1 = NULL, *prk2 = NULL;
    rsa_public_key_t *pbk1 = NULL, *pbk2 = NULL;

    const size_t key_size = 256;

    int status1 = rsa_keygen(&prk1, &pbk1, key_size);
    int status2 = rsa_keygen(&prk2, &pbk2, key_size);
    CHECK(status1==1 && status2==1, "Key generation failed");

    // The key_size is 256 bits, so sizeof(msg) < 256
    // the safe size is 7 limbs or 7*32 = 224 bits
    bigint_t *ub = bigint_from_uint32(1);
    CHECK(bigint_shift_left_inplace(ub, 7*32)==1, "Shift failed");

    for(int i = 0; i<10;i++){
        bigint_t *msg = bigint_create(8); 
        alg_generate_bigint(ub, msg);

        bigint_t *cipher = rsa_encrypt(prk1, pbk2, msg);

        CHECK(cipher!=NULL, "Encryption failed");

        CHECK(bigint_compare(cipher, msg)!=0, "Cipher is same as message");

        bigint_t *res = rsa_decrypt(prk2, pbk1, cipher);

        CHECK(res!=NULL, "Decryption failed");

        CHECK(bigint_compare(res, msg)==0, "Message change in between encrypt and decrypt");

        if(bigint_compare(res,msg)!=0){
            LOG_BIGINT("TEST_RSA_LOG", LOG_LEVEL_FATAL, "msg: ", msg);
            LOG_BIGINT("TEST_RSA_LOG", LOG_LEVEL_FATAL, "res: ", res);
        }

        bigint_free(msg);
        bigint_free(cipher);
        bigint_free(res);
    }
    bigint_free(ub);

    // Test with bytes
    char text[] = "Akrambigint"; // size needs to multiple of 4 for no padding
    // convert msg to bigint
    bigint_t *msg = bigint_from_bytes((uint8_t*)text, sizeof(text));

    bigint_t *cipher = rsa_encrypt(prk1, pbk2, msg);
    CHECK(cipher!=NULL, "Bytes encryption failed");
    bigint_t *res = rsa_decrypt(prk2, pbk1, cipher);
    CHECK(res!=NULL, "Bytes decryption failed");

    char buffer[12];
    size_t written = bigint_to_bytes(res, (uint8_t*)buffer, sizeof(buffer));
    CHECK(written!=SIZE_MAX, "buffer size should be enough");
    for(size_t i = 0; i<sizeof(buffer);i++){
        CHECK(buffer[i]==text[i], "Message is not the same");
    }

    bigint_free(res);
    bigint_free(cipher);
    bigint_free(msg);
    
    rsa_free_pbk(pbk1);
    rsa_free_prk(prk1);
    rsa_free_pbk(pbk2);
    rsa_free_prk(prk2);
}

static void test_rsa_mitm_vul(void){
    printf("\n ----- test_rsa_mitm_vul -----\n");
    char text[] = "Akrambigint"; // 12 bytes

    bigint_t *msg = bigint_from_bytes((uint8_t*)text, sizeof(text));
    CHECK(msg!=NULL, "bytes to bigint conversion failed");

    // Alice - 1
    // Bob - 2
    // Mallory - 3   // Man in the middle
    rsa_private_key_t *prk1, *prk2, *prk3;
    rsa_public_key_t *pbk1, *pbk2, *pbk3;
    const size_t key_size = 128;  
    int status1 = rsa_keygen(&prk1, &pbk1, key_size);
    int status2 = rsa_keygen(&prk2, &pbk2, key_size);
    int status3 = rsa_keygen(&prk3, &pbk3, key_size);
    CHECK(status1==1 && status2==1 && status3==1, "Key generation failed");

    /* Scenario
    * Alice sending message to bob
    * Mallory intercepts alice and replaces bob keys with her keys for alice
    * Now mallory will send modified message to bob encrypting with her key
    */ 
    bigint_t *cipher = rsa_encrypt(prk3, pbk2, msg);
    CHECK(cipher!=NULL, "Mallory encryption failed");
    
    // Now bob will think this message came from alice
    // Bob will use alice public key that he has
    bigint_t *res = rsa_decrypt(prk2, pbk1, cipher);
    CHECK(res!=NULL, "Bob decryption of intercepted message failed");

    // Now the res should be junk or simply res!=msg
    CHECK(bigint_compare(res,msg)!=0, "MITM attack succeeded");

    bigint_free(res);
    bigint_free(cipher);
    bigint_free(msg);

    rsa_free_pbk(pbk1);
    rsa_free_prk(prk1);

    rsa_free_pbk(pbk2);
    rsa_free_prk(prk2);

    rsa_free_pbk(pbk3);
    rsa_free_prk(prk3);
}

int main(){
    test_algorithms();
    test_key_generator();
    test_crypt();

    test_rsa_keygen();
    test_rsa_encrypt_decrypt();
    test_rsa_mitm_vul();


    printf("\n================================\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);
    printf("================================\n");

    if (tests_failed != 0) {
        printf("RESULT: FAILED\n");
        return EXIT_FAILURE;
    }

    printf("RESULT: ALL TESTS PASSED\n");
    return EXIT_SUCCESS;
}