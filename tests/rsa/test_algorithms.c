#include "../test_helpers.h"
#include <stdio.h>
#include "rsa/algorithms.h"

void test_rnd_u32_gen(void){
    printf("\n ------ test_rnd_u32_gen -------\n");
    uint32_t num1 = alg_random_uint32();
    uint32_t num2 = alg_random_uint32();
    uint32_t num3 = alg_random_uint32();
    

    CHECK(!(num1==num2 && num2==num3), "All number are same");
    CHECK(!((num1==0 && num2==0) || (num1==0 && num3==0) || (num2==0 && num3==0)), "More than 1 number is 0");
}

void test_gen_bigint(void){
    printf("\n ----- test_gen_bigint ------\n");

    bigint_t *ub = bigint_from_uint32(1);
    CHECK(bigint_shift_left_inplace(ub, 64)==1, "Shift failed");
    bigint_t *num1 = bigint_create(2);
    bigint_t *num2 = bigint_create(2);
    alg_generate_bigint(ub, num1);
    alg_generate_bigint(ub, num2);

    CHECK(num1->size<ub->size, "The generated num is large");
    CHECK(num2->size<ub->size, "The generated num is large");

    CHECK(bigint_compare(num1, num2)!=0, "a==b");

    bigint_free(ub);
    bigint_free(num1);
    bigint_free(num2);
}

void test_miller_rabin(void){
    bigint_t *one = bigint_from_string("1");
    bigint_t *two = bigint_from_string("2");
    bigint_t *p1 = bigint_from_string("572120418077488944040195383577");
    bigint_t *p2 = bigint_from_string("38198562107930254723859342749");
    bigint_t *p3 = bigint_from_string("895289810434358894562681681509");
    bigint_t *np1 = bigint_from_string("798434119848962720592820037525");
    bigint_t *np2 = bigint_from_string("633305191317760886953552783794");
    bigint_t *np3 = bigint_from_string("929893409404367781530848726073");

    CHECK(alg_miller_rabin(one)==false, "One is not a prime");
    CHECK(alg_miller_rabin(two)==true, "Two is a prime");
    CHECK(alg_miller_rabin(p1)==true, "Prime1 number failed test");
    CHECK(alg_miller_rabin(p2)==true, "Prime2 number failed test");
    CHECK(alg_miller_rabin(p3)==true, "Prime3 number failed test");
    CHECK(alg_miller_rabin(np1)==false, "Non-prime-1 passed test");
    CHECK(alg_miller_rabin(np2)==false, "Non-prime-2 passed test");
    CHECK(alg_miller_rabin(np3)==false, "Non-prime-3 passed test");

    bigint_free(one);
    bigint_free(two);
    bigint_free(p1);
    bigint_free(p2);
    bigint_free(p3);
    bigint_free(np1);
    bigint_free(np2);
    bigint_free(np3);
}


uint64_t ref_gcd(uint64_t x, uint64_t y){
    return y==0?x: ref_gcd(y, x%y);
}

void test_gcd(void){
    printf("\n ----- test_gcd ------\n");

    bigint_t *zero = bigint_create(0);
    bigint_t *one = bigint_from_uint32(1);
    bigint_t *two = bigint_from_uint32(2);

    bigint_t *gcd_2 = alg_gcd(two, zero);
    bigint_t *gcd_1 = alg_gcd(one, zero);
    bigint_t *gcd_0 = alg_gcd(zero, zero);
    CHECK(bigint_compare(gcd_2, two)==0, "gcd(0,2)!=2");
    CHECK(bigint_compare(gcd_1, one)==0, "gcd(1,0)!=1");
    CHECK(bigint_compare(gcd_0, zero)==0, "gcd(0,0)!=0");

    bigint_free(gcd_1); gcd_1 = NULL;
    bigint_free(gcd_2); gcd_2 = NULL;
    bigint_free(gcd_0); gcd_0 = NULL;

    gcd_1 = alg_gcd(one, two);
    CHECK(bigint_compare(one, gcd_1)==0, "gcd(1,2)!=1");
    bigint_free(gcd_1);

    for(int i = 0; i<100;i++){
        // Test against __builtin_gcd
        uint64_t x = alg_random_uint32();
        x<<=32;
        x |= alg_random_uint32();
        uint64_t y = alg_random_uint32(); y<<=32;
        y |= alg_random_uint32();

        bigint_t *xb = bigint_from_uint64(x);
        bigint_t *yb = bigint_from_uint64(y);
        uint64_t g = ref_gcd(x,y);
        bigint_t *g_res = bigint_from_uint64(g);
        bigint_t *gb = alg_gcd(xb,yb);
        CHECK(bigint_compare(gb, g_res)==0, "gcd against reference test failed");

        bigint_free(xb);
        bigint_free(yb);
        bigint_free(gb);
        bigint_free(g_res);
    }

    bigint_t *x = NULL, *y = NULL, *g = NULL, *res = NULL;

    x = bigint_from_string("12345678901234567890");
    y = bigint_from_string("9876543210");
    g = bigint_from_string("90");
    res = alg_gcd(x,y);
    CHECK(bigint_compare(g,res)==0, "GCD test failed");
    bigint_free(x); bigint_free(y); bigint_free(g); bigint_free(res);


    x = bigint_from_string("98765432109876543210");
    y = bigint_from_string("12345678901234567890");
    g = bigint_from_string("900000000090");
    res = alg_gcd(x,y);
    CHECK(bigint_compare(g,res)==0, "GCD test failed");
    bigint_free(x); bigint_free(y); bigint_free(g); bigint_free(res);


    bigint_free(zero);
    bigint_free(one);
    bigint_free(two);
}

void test_gcd_coeff(void){
    printf("\n----- test_gcd_coeff -----\n");

    // Diferential test against alg_gcd
    bigint_t *ub = bigint_from_uint32(1);
    bigint_shift_left_inplace(ub, 128);
    for(int i = 0;i<100;i++){
        bigint_t *a = bigint_create(ub->size-1);
        bigint_t *b = bigint_create(ub->size-1);

        alg_generate_bigint(ub, a);
        alg_generate_bigint(ub, b);

        bigint_t *g = alg_gcd(a,b);

        bigint_t *x, *y;
        
        bigint_t *res = alg_ext_euclied_gcd(a,b,&x,&y);

        CHECK(bigint_compare(g,res)==0, "Generated gcd and res is not equal");

        bigint_t *a1 = bigint_mul_fft(x,a);
        bigint_t *b1 = bigint_mul_fft(y,b);

        bigint_t *r = bigint_add(a1, b1);

        // By equation a*x + b*y = gcd(a,b)
        if(bigint_compare(r,res)!=0){
            LOG(MODULE, LOG_LEVEL_DEBUG, "gcd equation failed at test-#%d",i);
            LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "a=", a);
            LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "b=", b);
            LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "g=", g);
            LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "res=", res);
            LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "x=", x);
            LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "y=", y);
            LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "a1=", a1);
            LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "b1=", b1);
            LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "r=", r);
        }
        CHECK(bigint_compare(r, res)==0, "Equation of gcd and coefficient doesn't match");

        bigint_free(a);
        bigint_free(b);
        bigint_free(g);
        bigint_free(res);
        bigint_free(a1);
        bigint_free(b1);
        bigint_free(r);
        bigint_free(x);
        bigint_free(y);
    }
    bigint_free(ub);
}

void test_algorithms(void){
    printf("\n ============ TEST RSA/ALGORITHMS ========\n");
    test_rnd_u32_gen();
    test_gen_bigint();
    test_miller_rabin();
    test_gcd();
    test_gcd_coeff();
    printf("\n ============= END RSA/ALGORITHMS ========\n");
}