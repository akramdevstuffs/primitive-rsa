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

    printf("\n ---- bigint test_assign ------\n");
    bigint_t *a, *b, *c;
    a = bigint_from_string("10000");
    b = bigint_from_string("10001");
    c = bigint_from_string("10002");

    CHECK(bigint_assign(a, b)==1, "Assignment failed");

    check_bigint("TEST: Assign, a = b", a, b->limbs, b->size, false);

    CHECK(bigint_assign(a,c)==1, "Assignment failed");

    uint32_t limb[] = {10001};
    check_bigint("TEST: Change after assign, b after a=c", b, limb,1, false);

    bigint_free(a);
    bigint_free(b);
    bigint_free(c);
}

static void test_copy(void){

    printf("\n ---- bigint test_assign ------\n");
    bigint_t *a = bigint_from_string("1000000010");
    bigint_t *b = bigint_copy(a);
    CHECK(bigint_compare(a,b)==0, "Failed");
    // Destroy a and check if b is still readable
    bigint_free(a);
    CHECK(bigint_compare(b, bigint_from_string("1000000010"))==0, "Compare failed copy is different");
    bigint_free(b);
}

static void test_mul_uint32(void){
    
    printf("\n ---- bigint test_mul_uint32 ----\n");

    bigint_t *a = bigint_from_string("2435405568406840543");

    check_bigint("Multiply by zero", bigint_mul_uint32(a, 0), NULL, 0, false);

    check_bigint("Multiply by 1", bigint_mul_uint32(a,1), a->limbs, a->size, a->negative);

    bigint_t *b = bigint_from_string("1382928004180845554458749");

    check_bigint("Multiply with a*567843==b", bigint_mul_uint32(a, 567843), b->limbs, b->size, b->negative);

    bigint_free(a);
    bigint_free(b);
}

typedef bigint_t *(*bigint_mul_fn)(
    const bigint_t *a,
    const bigint_t *b
);

static void test_mul(
    const char *name,
    bigint_mul_fn mul
)
{
    printf("\n---- %s tests ----\n", name);

    bigint_t *a;
    bigint_t *b;
    bigint_t *result;


    // 1. Basic multiplication: 12 * 34 = 408
    const uint32_t r1[] = {408};

    a = bigint_from_string("12");
    b = bigint_from_string("34");
    result = mul(a, b);

    check_bigint("12 * 34", result, r1, 1, false);

    bigint_free(a);
    bigint_free(b);
    bigint_free(result);

    // 2. Multiplication by zero
    a = bigint_from_string("12345");
    b = bigint_from_string("0");
    result = mul(a, b);

    check_bigint("12345 * 0", result, NULL, 0, false);

    bigint_free(a);
    bigint_free(b);
    bigint_free(result);

    // 3. Multiplication by one
    const uint32_t r3[] = {12345};

    a = bigint_from_string("12345");
    b = bigint_from_string("1");
    result = mul(a, b);

    check_bigint("12345 * 1", result, r3, 1, false);

    bigint_free(a);
    bigint_free(b);
    bigint_free(result);


    // 4. Carry across a 32-bit limb:
    // UINT32_MAX * 2 = 8589934590
    const uint32_t r4[] = {UINT32_MAX - 1, 1};

    a = bigint_from_string("4294967295");
    b = bigint_from_string("2");
    result = mul(a, b);

    check_bigint("UINT32_MAX * 2", result, r4, 2, false);

    bigint_free(a);
    bigint_free(b);
    bigint_free(result);

    // 5. Positive * negative = negative
    const uint32_t r5[] = {408};

    a = bigint_from_string("12");
    b = bigint_from_string("-34");
    result = mul(a, b);

    check_bigint("12 * (-34)", result, r5, 1, true);

    bigint_free(a);
    bigint_free(b);
    bigint_free(result);

    // 6. Negative * negative = positive
    const uint32_t r6[] = {408};

    a = bigint_from_string("-12");
    b = bigint_from_string("-34");
    result = mul(a, b);

    check_bigint("(-12) * (-34)", result, r6, 1, false);

    bigint_free(a);
    bigint_free(b);
    bigint_free(result);


    // 7. Multi-limb multiplication:
    // 4294967296 * 4294967296 = 18446744073709551616
    const uint32_t r7[] = {0, 0, 1};

    a = bigint_from_string("4294967296");
    b = bigint_from_string("4294967296");
    result = mul(a, b);

    check_bigint("2^32 * 2^32", result, r7, 3, false);

    bigint_free(a);
    bigint_free(b);
    bigint_free(result);

    // 8. NULL input
    result = mul(NULL, NULL);

    CHECK(result == NULL, "NULL operands");
}

// Deterministic random number generator
static uint64_t rng_state = UINT64_C(0x123456789ABCDEF);

static uint32_t random_u32(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;

    return (uint32_t)rng_state;
}

enum test_pattern {
    PATTERN_RANDOM,
    PATTERN_MAX,
    PATTERN_ALTERNATING,
    PATTERN_SPARSE
};

static bigint_t *generate_bigint(size_t limbs,
                                 enum test_pattern pattern,
                                 bool negative)
{
    bigint_t *n = bigint_create(limbs);

    if (n == NULL)
        return NULL;

    n->size = limbs;
    n->negative = negative;

    for (size_t i = 0; i < limbs; i++) {
        switch (pattern) {

            case PATTERN_RANDOM:
                n->limbs[i] = random_u32();
                break;

            case PATTERN_MAX:
                n->limbs[i] = UINT32_MAX;
                break;

            case PATTERN_ALTERNATING:
                n->limbs[i] = (i & 1)
                            ? UINT32_MAX
                            : 0;
                break;

            case PATTERN_SPARSE:
                n->limbs[i] = (i % 17 == 0)
                            ? UINT32_MAX
                            : 0;
                break;
        }
    }

    // Ensure the bigint is normalized.
    n->limbs[limbs - 1] |= 1;

    return n;
}


static void test_mul_case(size_t a_size,
                          size_t b_size,
                          enum test_pattern pattern,
                          bool negative_a,
                          bool negative_b)
{
    bigint_t *a = generate_bigint(
        a_size, pattern, negative_a
    );

    bigint_t *b = generate_bigint(
        b_size, pattern, negative_b
    );

    if (a == NULL || b == NULL) {
        fprintf(stderr, "Allocation failure\n");
        tests_failed++;
        tests_run++;
        bigint_free(a);
        bigint_free(b);
        return;
    }

    // Reference implementation: O(n^2)
    bigint_t *expected = bigint_mul(a, b);

    // Optimized implementation: NTT + CRT
    bigint_t *actual = bigint_mul_fft(a, b);

    char message[128];

    snprintf(message, sizeof(message),
             "FFT multiplication: %zu x %zu limbs, pattern %d",
             a_size, b_size, pattern);

    CHECK(expected != NULL, "Schoolbook multiplication");

    CHECK(actual != NULL, "FFT multiplication");

    if (expected != NULL && actual != NULL) {

        CHECK(
            bigint_compare(expected, actual) == 0,
            message
        );

        if (bigint_compare(expected, actual) != 0) {
            fprintf(stderr,
                    "\nMismatch: %s\n"
                    "Expected size: %zu\n"
                    "Actual size:   %zu\n",
                    message,
                    expected->size,
                    actual->size);
        }
    }

    if(a) bigint_free(a);
    if(b) bigint_free(b);
    bigint_free(expected);
    bigint_free(actual);
}


void test_large_mul_fft(void)
{
    printf("\n--- Large FFT vs Schoolbook multiplication ---\n");

    // Small sizes: test edge cases.
    test_mul_case(1, 1, PATTERN_MAX, false, false);
    test_mul_case(2, 3, PATTERN_MAX, false, false);
    test_mul_case(3, 2, PATTERN_RANDOM, false, false);

    // Medium sizes.
    test_mul_case(31, 29, PATTERN_MAX, false, false);
    test_mul_case(128, 128, PATTERN_RANDOM, false, false);
    test_mul_case(256, 257, PATTERN_ALTERNATING, false, false);

    // Large sizes.
    test_mul_case(512, 512, PATTERN_MAX, false, false);
    test_mul_case(1024, 1024, PATTERN_MAX, false, false);
    test_mul_case(2048, 2048, PATTERN_RANDOM, false, false);
    // naive multiplication will hang on here so we can't do differential testing here
    // test_mul_case(131072, 131072, PATTERN_MAX, false, false);

    // Stress carry propagation.
    test_mul_case(2048, 2048, PATTERN_MAX, false, false);
    test_mul_case(4096, 4096, PATTERN_MAX, false, false);

    // Sparse coefficients.
    test_mul_case(2048, 2048, PATTERN_SPARSE, false, false);

    // Unequal operand lengths.
    test_mul_case(4096, 1024, PATTERN_RANDOM, false, false);

    // Sign handling.
    test_mul_case(512, 512, PATTERN_RANDOM, true, false);
    test_mul_case(512, 512, PATTERN_RANDOM, false, true);
    test_mul_case(512, 512, PATTERN_RANDOM, true, true);

    printf("--- Large multiplication tests completed ---\n");
}

void test_right_shift(void) {

    printf("\n --- test_right_shift --- \n");

    bigint_t *a;
    bigint_t *b;

    // Test shifting zero
    a = bigint_from_string("0");
    bigint_shift_right_inplace(a, 1);
    CHECK(a->size == 0, "Zero >> 1 should be zero");
    bigint_free(a);

    // Test basic shifts
    a = bigint_from_string("2");
    b = bigint_from_string("1");
    bigint_shift_right_inplace(a, 1);
    check_bigint("2 >> 1 == 1", a, b->limbs, b->size, b->negative);
    bigint_free(a);
    bigint_free(b);

    // Test odd number
    a = bigint_from_string("7");
    b = bigint_from_string("3");
    bigint_shift_right_inplace(a, 1);
    check_bigint("7 >> 1 == 3", a, b->limbs, b->size, b->negative);
    bigint_free(a);
    bigint_free(b);

    // Test shift greater than bit length
    a = bigint_from_string("15");
    bigint_shift_right_inplace(a, 4);
    CHECK(a->size == 0, "15 >> 4 should be zero");
    bigint_free(a);

    // Test shift equal to bit length
    a = bigint_from_string("16");
    bigint_shift_right_inplace(a, 5);
    CHECK(a->size == 0, "16 >> 5 should be zero");
    bigint_free(a);

    // Test shift by zero
    a = bigint_from_string("123456789");
    b = bigint_from_string("123456789");
    bigint_shift_right_inplace(a, 0);
    check_bigint("123456789 >> 0 == 123456789",
                 a, b->limbs, b->size, b->negative);
    bigint_free(a);
    bigint_free(b);

    // Test shifting across a 32-bit boundary
    a = bigint_from_string("4294967296"); // 2^32
    b = bigint_from_string("1");
    bigint_shift_right_inplace(a, 32);
    check_bigint("2^32 >> 32 == 1",
                 a, b->limbs, b->size, b->negative);
    bigint_free(a);
    bigint_free(b);

    // Test shifting across multiple limbs
    a = bigint_from_string("18446744073709551616"); // 2^64
    b = bigint_from_string("4294967296"); // 2^32
    bigint_shift_right_inplace(a, 32);
    check_bigint("2^64 >> 32 == 2^32",
                 a, b->limbs, b->size, b->negative);
    bigint_free(a);
    bigint_free(b);

    // Test shifting by more than one limb
    a = bigint_from_string("18446744073709551616"); // 2^64
    b = bigint_from_string("1");
    bigint_shift_right_inplace(a, 64);
    check_bigint("2^64 >> 64 == 1",
                 a, b->limbs, b->size, b->negative);
    bigint_free(a);
    bigint_free(b);

    // Test negative numbers (assuming sign-magnitude semantics)
    a = bigint_from_string("-8");
    b = bigint_from_string("-4");
    bigint_shift_right_inplace(a, 1);
    check_bigint("-8 >> 1 == -4",
                 a, b->limbs, b->size, b->negative);
    bigint_free(a);
    bigint_free(b);

    // Test negative odd number (sign-magnitude semantics)
    a = bigint_from_string("-7");
    b = bigint_from_string("-3");
    bigint_shift_right_inplace(a, 1);
    check_bigint("-7 >> 1 == -3",
                 a, b->limbs, b->size, b->negative);
    bigint_free(a);
    bigint_free(b);

    // Test a large shift on a negative number
    a = bigint_from_string("-12345");
    bigint_shift_right_inplace(a, 100);
    CHECK(a->size == 0, "Large right shift of negative number should be zero");
    CHECK(a->negative == 0, "Zero should not be negative");
    bigint_free(a);
}

void test_left_shift(void) {

    printf("\n --- test_left_shift --- \n");

    bigint_t *a;
    bigint_t *b;

    // Zero
    a = bigint_from_string("0");
    bigint_shift_left_inplace(a, 10);
    CHECK(a->size == 0, "0 << 10 should be zero");
    bigint_free(a);

    // Basic shift
    a = bigint_from_string("2");
    b = bigint_from_string("4");
    bigint_shift_left_inplace(a, 1);
    check_bigint("2 << 1 == 4", a, b->limbs, b->size, b->negative);
    bigint_free(a);
    bigint_free(b);

    // Shift by zero
    a = bigint_from_string("123456789");
    b = bigint_from_string("123456789");
    bigint_shift_left_inplace(a, 0);
    check_bigint("x << 0 == x", a, b->limbs, b->size, b->negative);
    bigint_free(a);
    bigint_free(b);

    // Carry across 32-bit limb boundary
    a = bigint_from_string("4294967295");  // 2^32 - 1
    b = bigint_from_string("8589934590");  // 2^33 - 2
    bigint_shift_left_inplace(a, 1);
    check_bigint("(2^32 - 1) << 1", a, b->limbs, b->size, b->negative);
    bigint_free(a);
    bigint_free(b);

    // Shift by exactly 32 bits
    a = bigint_from_string("1");
    b = bigint_from_string("4294967296");  // 2^32
    bigint_shift_left_inplace(a, 32);
    check_bigint("1 << 32 == 2^32", a, b->limbs, b->size, b->negative);
    bigint_free(a);
    bigint_free(b);

    // Shift across multiple limbs
    a = bigint_from_string("4294967296");  // 2^32
    b = bigint_from_string("18446744073709551616"); // 2^64
    bigint_shift_left_inplace(a, 32);
    check_bigint("2^32 << 32 == 2^64", a, b->limbs, b->size, b->negative);
    bigint_free(a);
    bigint_free(b);

    // Negative number
    a = bigint_from_string("-123");
    b = bigint_from_string("-492");
    bigint_shift_left_inplace(a, 2);
    check_bigint("-123 << 2 == -492", a, b->limbs, b->size, b->negative);
    bigint_free(a);
    bigint_free(b);
}

void test_div_inplace(void) {

    printf("\n --- test_div --- \n");

    bigint_t *a, *b, *q, *expected_q, *expected_r;

    // Basic division with remainder
    a = bigint_from_string("10");
    b = bigint_from_string("3");
    expected_q = bigint_from_string("3");
    expected_r = bigint_from_string("1");

    q = bigint_div_inplace(a, b);
    check_bigint("10 / 3 = 3", q, expected_q->limbs, expected_q->size, 0);
    check_bigint("10 % 3 = 1", a, expected_r->limbs, expected_r->size, 0);

    bigint_free(a); bigint_free(b); bigint_free(q);
    bigint_free(expected_q); bigint_free(expected_r);


    // Exact division
    a = bigint_from_string("100");
    b = bigint_from_string("10");
    expected_q = bigint_from_string("10");
    expected_r = bigint_from_string("0");

    q = bigint_div_inplace(a, b);
    check_bigint("100 / 10 = 10", q, expected_q->limbs, expected_q->size, 0);
    CHECK(a->size == 0, "100 % 10 should be zero");

    bigint_free(a); bigint_free(b); bigint_free(q);
    bigint_free(expected_q); bigint_free(expected_r);


    // Dividend smaller than divisor
    a = bigint_from_string("3");
    b = bigint_from_string("10");
    expected_q = bigint_from_string("0");
    expected_r = bigint_from_string("3");

    q = bigint_div_inplace(a, b);
    CHECK(q->size == 0, "3 / 10 should be zero");
    check_bigint("3 % 10 = 3", a, expected_r->limbs, expected_r->size, 0);

    bigint_free(a); bigint_free(b); bigint_free(q);
    bigint_free(expected_q); bigint_free(expected_r);


    // Dividend equals divisor
    a = bigint_from_string("123456789");
    b = bigint_from_string("123456789");
    expected_q = bigint_from_string("1");

    q = bigint_div_inplace(a, b);
    check_bigint("a / a = 1", q, expected_q->limbs, expected_q->size, 0);
    CHECK(a->size == 0, "a % a should be zero");

    bigint_free(a); bigint_free(b); bigint_free(q);
    bigint_free(expected_q);


    // Multi-limb division
    a = bigint_from_string("18446744073709551615"); // 2^64 - 1
    b = bigint_from_string("4294967295");           // 2^32 - 1
    expected_q = bigint_from_string("4294967297");  // 2^32 + 1
    expected_r = bigint_from_string("0");

    q = bigint_div_inplace(a, b);
    check_bigint("Multi-limb exact division", q,
                 expected_q->limbs, expected_q->size, 0);
    CHECK(a->size == 0, "Multi-limb remainder should be zero");

    bigint_free(a); bigint_free(b); bigint_free(q);
    bigint_free(expected_q); bigint_free(expected_r);


    // Large quotient and non-zero remainder
    a = bigint_from_string("12345678901234567890");
    b = bigint_from_string("123456789");
    expected_q = bigint_from_string("100000000010");
    expected_r = bigint_from_string("0");

    q = bigint_div_inplace(a, b);
    check_bigint("Large quotient", q,
                 expected_q->limbs, expected_q->size, 0);
    check_bigint("Large remainder", a,
                 expected_r->limbs, expected_r->size, 0);

    bigint_free(a); bigint_free(b); bigint_free(q);
    bigint_free(expected_q); bigint_free(expected_r);


    // Division by zero
    a = bigint_from_string("12345");
    b = bigint_from_string("0");

    q = bigint_div_inplace(a, b);
    CHECK(q == NULL, "Division by zero should return NULL");
    CHECK(a->size == 1 && a->limbs[0] == 12345,
          "Dividend should remain unchanged");

    bigint_free(a);
    bigint_free(b);
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
    test_copy();
    test_mul_uint32();
    // test_mul("bigint_mul", bigint_mul);
    test_mul("bigint_mul_fft", bigint_mul_fft);

    test_large_mul_fft();

    test_right_shift();

    test_left_shift();

    test_div_inplace();

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