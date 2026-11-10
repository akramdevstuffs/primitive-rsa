#include "rsa/key_generator.h"
#include "../test_helpers.h"

void test_compute_inverse(void) {
    printf("\n----- test_compute_inverse -----\n");

    struct {
        uint32_t a;
        uint32_t mod;
        uint32_t expected;
    } cases[] = {
        { 3, 11, 4 },
        { 17, 3120, 2753 },
        { 7, 26, 15 },
        { 11, 29, 8 },
        { 3, 7, 5 }
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        bigint_t *a = bigint_from_uint32(cases[i].a);
        bigint_t *mod = bigint_from_uint32(cases[i].mod);

        bigint_t *inverse = kgn_compute_inverse(a, mod);
        bigint_t *expected = bigint_from_uint32(cases[i].expected);

        CHECK(inverse != NULL, "Inverse should exist");

        if (inverse) {
            CHECK(
                bigint_compare(inverse, expected) == 0,
                "Incorrect modular inverse"
            );

            // Verify a * inverse mod mod == 1.
            bigint_t *product = bigint_mul_fft(a, inverse);
            bigint_t *remainder = bigint_mod(product, mod);
            bigint_t *one = bigint_from_uint32(1);

            CHECK(
                remainder != NULL &&
                bigint_compare(remainder, one) == 0,
                "Modular inverse equation failed"
            );

            bigint_free(product);
            bigint_free(remainder);
            bigint_free(one);
            bigint_free(inverse);
        }

        bigint_free(a);
        bigint_free(mod);
        bigint_free(expected);
    }

    // No inverse exists: gcd(6, 9) = 3.
    bigint_t *a = bigint_from_uint32(6);
    bigint_t *mod = bigint_from_uint32(9);

    bigint_t *inverse = kgn_compute_inverse(a, mod);

    CHECK(inverse == NULL, "Inverse should not exist when GCD is not 1");

    if (inverse) bigint_free(inverse);

    bigint_free(a);
    bigint_free(mod);
}

void test_kgn_generate_pq_multiple(void) {
    printf("\n----- test_kgn_generate_pq_multiple -----\n");

    const size_t bits = 32;

    for (int i = 0; i < 10; i++) {
        bigint_t *p = NULL;
        bigint_t *q = NULL;

        int status = kgn_generate_pq(&p, &q, bits);

        CHECK(status == 1, "Prime generation failed");

        if (status == 1 && p && q) {
            CHECK(
                bigint_compare(p, q) != 0,
                "Generated primes should be different"
            );

            CHECK(alg_miller_rabin(p), "p is not prime");
            CHECK(alg_miller_rabin(q), "q is not prime");
        }

        if (p) bigint_free(p);
        if (q) bigint_free(q);
    }
}

void test_kgn_generate_skey_pair_multiple(void) {
    printf("\n----- test_kgn_generate_skey_pair_multiple -----\n");

    const uint32_t prime_pairs[][2] = {
        { 11, 13 },
        { 17, 19 },
        { 23, 29 },
        { 31, 37 },
        { 41, 43 }
    };

    for (size_t i = 0; i < sizeof(prime_pairs) / sizeof(prime_pairs[0]); i++) {
        bigint_t *p = bigint_from_uint32(prime_pairs[i][0]);
        bigint_t *q = bigint_from_uint32(prime_pairs[i][1]);

        bigint_t *d = NULL;
        bigint_t *e = NULL;

        int status = kgn_generate_skey_pair(p, q, &d, &e, 16);

        LOG(MODULE, LOG_LEVEL_DEBUG, "SKEY: %lu",i);
        CHECK(status == 1, "Secret key generation failed");

        if (status == 1 && d && e) {
            bigint_t *p1 = bigint_copy(p);
            bigint_t *q1 = bigint_copy(q);

            bigint_sub_uint32_inplace(p1, 1);
            bigint_sub_uint32_inplace(q1, 1);

            bigint_t *phi = bigint_mul_fft(p1, q1);
            bigint_t *product = bigint_mul_fft(d, e);
            bigint_t *remainder = bigint_mod(product, phi);

            bigint_t *one = bigint_from_uint32(1);

            if(bigint_compare(remainder,one)!=0){
                LOG(MODULE, LOG_LEVEL_DEBUG, "exponents are not inverses tests-#%lu",i);
                LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "d=", d);
                LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "e=", e);
                LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "phi=", phi);
            }

            CHECK(
                remainder && bigint_compare(remainder, one) == 0,
                "Generated exponents are not modular inverses"
            );

            bigint_free(p1);
            bigint_free(q1);
            bigint_free(phi);
            bigint_free(product);
            bigint_free(remainder);
            bigint_free(one);
        }

        if (p) bigint_free(p);
        if (q) bigint_free(q);
        if (d) bigint_free(d);
        if (e) bigint_free(e);
    }
}

void test_kgn_gen_key_pair(void) {
    printf("\n----- test_kgn_gen_key_pair -----\n");

    bigint_t *n = NULL;
    bigint_t *d = NULL;
    bigint_t *e = NULL;

    kgn_set_seed(NULL);

    int status = kgn_gen_key_pair(
        &n,
        &d,
        &e,
        32
    );

    CHECK(status == 1, "RSA key generation should succeed");

    if (status == 1) {
        CHECK(n != NULL, "Generated modulus should not be NULL");
        CHECK(d != NULL, "Generated d should not be NULL");
        CHECK(e != NULL, "Generated e should not be NULL");
    }

    if (n) bigint_free(n);
    if (d) bigint_free(d);
    if (e) bigint_free(e);
}

void test_key_generator(void){
    printf("\n ============ TEST RSA/KEY_GENERATOR ========\n");
    test_compute_inverse();
    test_kgn_generate_pq_multiple();
    test_kgn_generate_skey_pair_multiple();
    test_kgn_gen_key_pair();

    printf("\n ============= END RSA/KEY_GENERATOR ========\n");
}