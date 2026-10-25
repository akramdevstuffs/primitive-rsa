#include "rsa/bigint.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(condition, message)                              \
    do {                                                       \
        tests_run++;                                           \
        if (!(condition)) {                                    \
            fprintf(stderr, "FAIL: %s (line %d)\n",            \
                    message, __LINE__);                         \
            tests_failed++;                                    \
        }                                                      \
    } while (0)

/*
 * Check that a bigint matches an expected representation.
 *
 * Limbs use little-endian base 2^32:
 * value = limbs[0] + limbs[1]*2^32 + ...
 */
static void check_bigint(
    const char *test_name,
    const bigint_t *actual,
    const uint32_t *expected_limbs,
    size_t expected_size,
    bool expected_negative
) {
    CHECK(actual != NULL, test_name);

    if (actual == NULL) {
        return;
    }

    CHECK(actual->size == expected_size, test_name);
    CHECK(actual->negative == expected_negative, test_name);

    size_t count = actual->size < expected_size
                       ? actual->size
                       : expected_size;

    for (size_t i = 0; i < count; i++) {
        if (actual->limbs[i] != expected_limbs[i]) {
            fprintf(stderr,
                    "FAIL: %s: limb[%zu], expected %u, got %u\n",
                    test_name,
                    i,
                    (unsigned int)expected_limbs[i],
                    (unsigned int)actual->limbs[i]);

            tests_failed++;
            tests_run++;
        } else {
            tests_run++;
        }
    }
}

/* ---------------------------------------------------------
 * 1. bigint_from_string()
 * --------------------------------------------------------- */

static void test_parse(void) {
    printf("\n--- bigint_from_string tests ---\n");

    const uint32_t zero[] = {0};
    const uint32_t small[] = {123};

    const uint32_t max_u32[] = {
        UINT32_MAX
    };

    const uint32_t two_pow_32[] = {
        0, 1
    };

    const uint32_t max_u64[] = {
        UINT32_MAX,
        UINT32_MAX
    };

    const uint32_t large[] = {
        UINT32_C(0xEB1F0AD2),
        UINT32_C(0xAB54A98C)
    };

    bigint_t *n;

    n = bigint_from_string("0");
    check_bigint("parse zero", n, zero, 0, false);
    bigint_free(n);

    n = bigint_from_string("123");
    check_bigint("parse positive", n, small, 1, false);
    bigint_free(n);

    n = bigint_from_string("-123");
    check_bigint("parse negative", n, small, 1, true);
    bigint_free(n);

    n = bigint_from_string("+123");
    check_bigint("parse explicit plus", n, small, 1, false);
    bigint_free(n);

    n = bigint_from_string("000123");
    check_bigint("parse leading zeros", n, small, 1, false);
    bigint_free(n);

    n = bigint_from_string("-0");
    check_bigint("parse negative zero", n, zero, 0, false);
    bigint_free(n);

    n = bigint_from_string("4294967295");
    check_bigint("parse UINT32_MAX", n, max_u32, 1, false);
    bigint_free(n);

    n = bigint_from_string("4294967296");
    check_bigint("parse 2^32", n, two_pow_32, 2, false);
    bigint_free(n);

    n = bigint_from_string("18446744073709551615");
    check_bigint("parse UINT64_MAX", n, max_u64, 2, false);
    bigint_free(n);

    n = bigint_from_string("12345678901234567890");
    check_bigint("parse large integer", n, large, 2, false);
    bigint_free(n);

    n = bigint_from_string("-12345678901234567890");
    check_bigint("parse negative large integer", n, large, 2, true);
    bigint_free(n);
}

/* ---------------------------------------------------------
 * 2. Invalid input
 * --------------------------------------------------------- */

static void test_invalid_input(void) {
    printf("\n--- Invalid input tests ---\n");

    const char *invalid[] = {
        "",
        "+",
        "-",
        "abc",
        "12a34",
        "1.23",
        "--123",
        "++123",
        "12 34"
    };

    size_t count = sizeof(invalid) / sizeof(invalid[0]);

    for (size_t i = 0; i < count; i++) {
        bigint_t *n = bigint_from_string(invalid[i]);

        if (n != NULL) {
            bigint_free(n);
        }

        CHECK(n == NULL, "reject invalid input");
    }

    bigint_t *n = bigint_from_string(NULL);
    CHECK(n == NULL, "reject NULL input");
}

/* ---------------------------------------------------------
 * 3. bigint_compare()
 * --------------------------------------------------------- */

static void test_compare(void) {
    printf("\n--- bigint_compare tests ---\n");

    bigint_t *a = bigint_from_string("123");
    bigint_t *b = bigint_from_string("123");
    bigint_t *c = bigint_from_string("124");
    bigint_t *d = bigint_from_string("122");
    bigint_t *e = bigint_from_string("-123");
    bigint_t *f = bigint_from_string("-124");
    bigint_t *g = bigint_from_string("4294967296");

    CHECK(a && b && c && d && e && f && g,
          "create comparison operands");

    if (a && b && c && d && e && f && g) {
        CHECK(bigint_compare(a, b) == 0, "equal numbers");
        CHECK(bigint_compare(a, c) < 0, "123 < 124");
        CHECK(bigint_compare(a, d) > 0, "123 > 122");

        CHECK(bigint_compare(a, e) > 0, "positive > negative");
        CHECK(bigint_compare(e, a) < 0, "negative < positive");

        CHECK(bigint_compare(e, f) > 0, "-123 > -124");
        CHECK(bigint_compare(f, e) < 0, "-124 < -123");

        CHECK(bigint_compare(g, a) > 0,
              "compare different limb counts");
    }

    bigint_free(a);
    bigint_free(b);
    bigint_free(c);
    bigint_free(d);
    bigint_free(e);
    bigint_free(f);
    bigint_free(g);
}

/* ---------------------------------------------------------
 * 4. bigint_add()
 * --------------------------------------------------------- */

static void test_add(void) {
    printf("\n--- bigint_add tests ---\n");

    bigint_t *a;
    bigint_t *b;
    bigint_t *result;

    const uint32_t r1[] = {30};
    a = bigint_from_string("10");
    b = bigint_from_string("20");
    result = bigint_add(a, b);

    check_bigint("10 + 20", result, r1, 1, false);

    bigint_free(a);
    bigint_free(b);
    bigint_free(result);

    // Carry across a 32-bit limb.
    const uint32_t r2[] = {0, 1};
    a = bigint_from_string("4294967295");
    b = bigint_from_string("1");
    result = bigint_add(a, b);

    check_bigint("UINT32_MAX + 1", result, r2, 2, false);

    bigint_free(a);
    bigint_free(b);
    bigint_free(result);

    // Addition of two negative numbers.
    const uint32_t r3[] = {30};
    a = bigint_from_string("-10");
    b = bigint_from_string("-20");
    result = bigint_add(a, b);

    check_bigint("-10 + -20", result, r3, 1, true);

    bigint_free(a);
    bigint_free(b);
    bigint_free(result);

    // Opposite signs: larger positive magnitude.
    const uint32_t r4[] = {5};
    a = bigint_from_string("15");
    b = bigint_from_string("-10");
    result = bigint_add(a, b);

    check_bigint("15 + (-10)", result, r4, 1, false);

    bigint_free(a);
    bigint_free(b);
    bigint_free(result);

    // Opposite signs: larger negative magnitude.
    const uint32_t r5[] = {5};
    a = bigint_from_string("10");
    b = bigint_from_string("-15");
    result = bigint_add(a, b);

    check_bigint("10 + (-15)", result, r5, 1, true);

    bigint_free(a);
    bigint_free(b);
    bigint_free(result);

    // Equal magnitudes with opposite signs.
    const uint32_t r6[] = {0};
    a = bigint_from_string("123");
    b = bigint_from_string("-123");
    result = bigint_add(a, b);

    check_bigint("123 + (-123)", result, r6, 0, false);

    bigint_free(a);
    bigint_free(b);
    bigint_free(result);
}

/*-----------------------
 * Bigint Assign test
 *----------------------- */
static void test_assign(void){

    printf("\n ---- bigint test_assign ------");
    bigint_t *a, *b, *c;
    a = bigint_from_string("10000");
    b = bigint_from_string("10001");
    c = bigint_from_string("10002");

    CHECK(bigint_assign(a, b)==1, "Assignment failed");

    check_bigint("TEST: Assign, a = b", a, b->limbs, b->size, false);

    CHECK(bigint_assign(a,c)==1, "Assignment failed");

    uint32_t limb[] = {10001};
    check_bigint("TEST: Change after assign, b after a=c", b, limb,1, false);
}

/* ---------------------------------------------------------
 * 5. Main
 * --------------------------------------------------------- */

int main(void) {
    test_parse();
    test_invalid_input();
    test_compare();
    test_add();
    test_assign();

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