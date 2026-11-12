#include <stdio.h>
#include <string.h>
#include "rsa/rsa.h"
#include "rsa/crypt.h"

int main(void) {
    printf("====================================================\n");
    printf("            primitive-rsa Demonstration             \n");
    printf("====================================================\n\n");

    const size_t key_size = 256;
    printf("Generating %zu-bit RSA key pairs for Alice and Bob...\n", key_size);

    rsa_private_key_t *alice_prk = NULL;
    rsa_public_key_t  *alice_pbk = NULL;
    rsa_private_key_t *bob_prk   = NULL;
    rsa_public_key_t  *bob_pbk   = NULL;

    if (rsa_keygen(&alice_prk, &alice_pbk, key_size) != 1) {
        fprintf(stderr, "Failed to generate Alice's keys.\n");
        return 1;
    }
    if (rsa_keygen(&bob_prk, &bob_pbk, key_size) != 1) {
        fprintf(stderr, "Failed to generate Bob's keys.\n");
        rsa_free_prk(alice_prk);
        rsa_free_pbk(alice_pbk);
        return 1;
    }

    printf("[+] Alice and Bob key pairs generated successfully.\n\n");

    // Message must be small enough to fit within key modulus and multiple of 4 bytes
    const char text[] = "Hello, RSA!"; // 12 bytes including null terminator
    printf("Plaintext message: \"%s\" (%zu bytes)\n\n", text, sizeof(text));

    bigint_t *msg = bigint_from_bytes((const uint8_t *)text, sizeof(text));
    if (!msg) {
        fprintf(stderr, "Failed to convert message to BigInt.\n");
        goto cleanup;
    }

    // -----------------------------------------------------------
    // Part 1: Standard RSA Public-Key Encryption & Decryption
    // -----------------------------------------------------------
    printf("--- 1. Standard Public-Key Encryption (Confidentiality) ---\n");
    printf("Alice encrypts message using Bob's public key (c = m^e mod n)...\n");
    bigint_t *cipher = crt_encrypt(bob_pbk->e, bob_pbk->n, msg);
    if (!cipher) {
        fprintf(stderr, "Encryption failed.\n");
        goto cleanup_msg;
    }

    char cipher_str[128];
    bigint_to_string(cipher, cipher_str, sizeof(cipher_str));
    printf("[+] Ciphertext (truncated / decimal): %s...\n", cipher_str);

    printf("Bob decrypts ciphertext using his private key (m = c^d mod n)...\n");
    bigint_t *decrypted_msg = crt_decrypt(bob_prk->d, bob_prk->n, cipher);
    if (!decrypted_msg) {
        fprintf(stderr, "Decryption failed.\n");
        bigint_free(cipher);
        goto cleanup_msg;
    }

    char decrypted_text[32] = {0};
    bigint_to_bytes(decrypted_msg, (uint8_t *)decrypted_text, sizeof(decrypted_text));
    printf("[+] Decrypted plaintext: \"%s\"\n", decrypted_text);
    printf("[+] Verification: %s\n\n", (strcmp(text, decrypted_text) == 0) ? "PASSED" : "FAILED");

    bigint_free(decrypted_msg);
    bigint_free(cipher);

    // -----------------------------------------------------------
    // Part 2: Paper-Faithful Dual Sign-and-Encrypt Flow (1978)
    // -----------------------------------------------------------
    printf("--- 2. Dual Sign-and-Encrypt (Privacy + Authenticity) ---\n");
    
    // As noted in Rivest-Shamir-Adleman (1978) Section VII, dual sign-then-encrypt
    // requires n_sender < n_receiver so the intermediate signature does not overflow.
    rsa_private_key_t *sender_prk = alice_prk;
    rsa_public_key_t  *sender_pbk = alice_pbk;
    rsa_private_key_t *recv_prk   = bob_prk;
    rsa_public_key_t  *recv_pbk   = bob_pbk;
    const char *sender_name = "Alice";
    const char *recv_name = "Bob";

    if (bigint_compare(alice_pbk->n, bob_pbk->n) > 0) {
        // Swap so sender modulus is smaller than receiver modulus
        sender_prk = bob_prk;
        sender_pbk = bob_pbk;
        recv_prk   = alice_prk;
        recv_pbk   = alice_pbk;
        sender_name = "Bob";
        recv_name = "Alice";
    }

    printf("%s signs with private key, then encrypts with %s's public key...\n", sender_name, recv_name);
    bigint_t *auth_cipher = rsa_encrypt(sender_prk, recv_pbk, msg);
    if (!auth_cipher) {
        fprintf(stderr, "Dual encryption failed.\n");
        goto cleanup_msg;
    }

    printf("%s decrypts with private key, then verifies signature with %s's public key...\n", recv_name, sender_name);
    bigint_t *auth_decrypted = rsa_decrypt(recv_prk, sender_pbk, auth_cipher);
    if (!auth_decrypted) {
        fprintf(stderr, "Dual decryption failed.\n");
        bigint_free(auth_cipher);
        goto cleanup_msg;
    }

    char auth_text[32] = {0};
    bigint_to_bytes(auth_decrypted, (uint8_t *)auth_text, sizeof(auth_text));
    printf("[+] Decrypted & verified message: \"%s\"\n", auth_text);
    printf("[+] Verification: %s\n\n", (strcmp(text, auth_text) == 0) ? "PASSED" : "FAILED");

    bigint_free(auth_decrypted);
    bigint_free(auth_cipher);

    printf("====================================================\n");
    printf("All demonstrations finished successfully.\n");
    printf("====================================================\n");

cleanup_msg:
    bigint_free(msg);
cleanup:
    rsa_free_pbk(bob_pbk);
    rsa_free_prk(bob_prk);
    rsa_free_pbk(alice_pbk);
    rsa_free_prk(alice_prk);

    return 0;
}
