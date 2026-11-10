#include "rsa/algorithms.h"
#include "rsa/debug.h"
#include <stdlib.h>

uint32_t alg_random_uint32(){
    // uses <<16 to fill all 32 bits
    return ((uint32_t)rand() << 16) ^ (uint32_t)rand();
}

// dest should have enough space to contain any num under bounds
void alg_generate_bigint(const bigint_t *ub, bigint_t *dest){
    dest->limbs[ub->size-1] = alg_random_uint32() % ub->limbs[ub->size-1];
    dest->size = ub->size;

    for(size_t i = 0; i<dest->size-1;i++){
        dest->limbs[i] = alg_random_uint32();
    }
    while(dest->size>0 && dest->limbs[dest->size-1]==0) dest->size--;
    LOG_BIGINT(MODULE, LOG_LEVEL_TRACE, "Generated bigint ", dest);
}

bool check_composite(const bigint_t *n, const bigint_t *a, const bigint_t *d, uint32_t s){
    if(a==0) return true;

    // Initialize montgomery for speedup mod multiplication operatons
    montgomery_ctx ctx;
    if(montgomery_init(n, &ctx)==0) {
        // Initialization failed no cleanup
        // Return error
        return true;
    }

    bool result = false;
    bigint_t *x = NULL;
    bigint_t *n_1 = NULL;
    bigint_t *one = NULL;
    bigint_t *x2 = NULL;

    x = bigint_modpow(a,d,n);
    if(x==NULL) goto cleanup;

    if(montgomery_transform(x, &ctx)!=1) goto cleanup;

    // Transform all comparison variables to perform comparision

    one = bigint_from_uint32(1);
    if(one==NULL || 
        montgomery_transform(one, &ctx)!=1
    ) goto cleanup;

    n_1 = bigint_copy(n);
    if(n_1==NULL || 
        bigint_sub_uint32_inplace(n_1, 1)!=1 || 
        montgomery_transform(n_1, &ctx)!=1
    )  goto cleanup;

    if(bigint_compare(x, one)==0 || bigint_compare(x,n_1)==0) {
        goto cleanup;
    }

    for(uint32_t r = 1; r<s;r++){
        x2 = montgomery_mul(x,x,&ctx);

        if(x2==NULL || bigint_compare(x2, n_1)==0){
            LOG_BIGINT(MODULE, LOG_LEVEL_TRACE, "Composite test failed at x2=n-1",x2);
            goto cleanup;
        }
        bigint_free(x);
        x = x2;
        x2 = NULL;
    }

    result = true;

cleanup:
    if(one) bigint_free(one);
    if(x) bigint_free(x);
    if(x2) bigint_free(x2);
    if(n_1) bigint_free(n_1);
    montgomery_free(&ctx);

    return result;
}

/*
* Provides implementation of miller rabin primality test
* Uses multiple rounds = 40, to lower the probability of the number to be composite
* Uses montgnomery reduction to speedup mod operations
*/
bool alg_miller_rabin(const bigint_t *num){
    if(num->size==0) return false;
    if(num->size==1 && num->limbs[0]<2) return false;

    bool result = false;
    bigint_t *range = NULL;
    bigint_t *d = NULL;
    bigint_t *a = NULL;

    range = bigint_copy(num);
    if(range==NULL || bigint_sub_uint32_inplace(range,3)!=1){
        goto cleanup;
    }

    uint32_t s = 0;
    d = bigint_copy(num);
    if(d==NULL || bigint_sub_uint32_inplace(d, 1)!=1) goto cleanup;

    while(d->size && (d->limbs[0]&1) == 0){
        bigint_shift_right_inplace(d, 1);
        s++;
    }

    for(int i = 0; i<100; i++){
        a = bigint_create(num->size);
        if(a==NULL) goto cleanup;
        alg_generate_bigint(range, a);
        if(bigint_add_uint32_inplace(a, 2)!=1) goto cleanup;

        if(check_composite(num, a, d, s)){
            LOG_BIGINT(MODULE, LOG_LEVEL_TRACE, "Prime failed: num", num);
            LOG_BIGINT(MODULE, LOG_LEVEL_TRACE, "Prime failed: a", a);
            LOG_BIGINT(MODULE, LOG_LEVEL_TRACE, "Prime failed: d", d);
            LOG(MODULE, LOG_LEVEL_TRACE, "Prime failed: s=%u", s);
            goto cleanup;
        }

        bigint_free(a);
        a=NULL;
    }

    result = true;

cleanup:
    if(a) bigint_free(a);
    if(range) bigint_free(range);
    if(d) bigint_free(d);

    LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, (result)?"Miller rabin test Passed: ":"Miller rabin test failed: ", num);

    return result;
}

#define MAX_PRIME_ATTEMPT 150

/*
* Generate prime number p < ub, 
*/
bigint_t *alg_generate_prime(const bigint_t *lb, const bigint_t *ub){
    bigint_t *nub = NULL;
    bigint_t *nlb = NULL;
    bigint_t *result = NULL;

    if(lb==NULL) nlb = bigint_create(0);
    else nlb = bigint_copy(lb);
    if(nlb==NULL) goto cleanup;

    nub = bigint_add(ub, nlb);
    if(nub==NULL) goto cleanup;

    bigint_t *p = bigint_create(nub->size);
    for(int attempt=1; attempt<=MAX_PRIME_ATTEMPT; attempt++){
        alg_generate_bigint(nub, p);
        if(bigint_sub_inplace(p, nlb)!=1) goto cleanup;
        // Check if even then make it odd
        if((p->limbs[0]&1)==0 &&
            bigint_add_uint32_inplace(p, 1)!=1) goto cleanup;
        // if it's ub-1 and even then retry
        if(ub->size>=1 && bigint_compare(ub, p)==0) {
            continue;
        }

        if(p->negative){
            LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "p=",p);
            LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "ub=",ub);
            LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "lb=",lb);
            LOG(MODULE, LOG_LEVEL_FATAL, "Generated a negative probable prime: %d",attempt);
        }

        // Test for prime
        if(alg_miller_rabin(p)){
            result = p;
            p = NULL;
            break;
        }
    }

cleanup:
    if(nlb) bigint_free(nlb);
    if(nub) bigint_free(nub);
    if(p)bigint_free(p);

    if(result)
    LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "Generated prime: ", result);

    return result;
}

bigint_t *alg_gcd(const bigint_t *a, const bigint_t *b){
    bigint_t *x = NULL;
    bigint_t *y = NULL;
    bigint_t *result = NULL;
    x = bigint_copy(a); if(!x) goto cleanup;
    y = bigint_copy(b); if(!y) goto cleanup;

    while(bigint_compare_abs_uint32(y,0)!=0){
        if(bigint_mod_inplace(x, y)!=1) goto cleanup;
        
        bigint_t *temp = x;
        x = y;
        y = temp;
    }

    result = x;
    x=NULL;

cleanup:
    if(x) bigint_free(x);
    if(y) bigint_free(y);

    LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "GCD: ", result);

    return result;
}

bigint_t *alg_ext_euclied_gcd(const bigint_t *a, const bigint_t *b, bigint_t **x_dest, bigint_t **y_dest){
    bigint_t *x = NULL;
    bigint_t *y = NULL;
    bigint_t *x1 = NULL, *y1 = NULL, *a1 = NULL, *b1 = NULL;
    bigint_t *q = NULL;
    bigint_t *q_x1 = NULL, *q_y1 = NULL, *q_b1 = NULL;
    bigint_t *result = NULL;

    x = bigint_from_uint32(1); if(!x) goto cleanup;
    y = bigint_create(0); if(!y) goto cleanup;
    x1 = bigint_create(0); 
    y1 = bigint_from_uint32(1); 
    a1 = bigint_copy(a); 
    b1 = bigint_copy(b); 
    if(!x1 || !y1 || !a1 || !b1) goto cleanup;

    while(bigint_compare_abs_uint32(b1, 0)!=0){
        q = bigint_div(a1,b1); if(!q) goto cleanup;
        q_x1 = bigint_mul_fft(q,x1);
        q_y1 = bigint_mul_fft(q,y1);
        q_b1 = bigint_mul_fft(q,b1);

        bigint_free(q);
        q = NULL;

        if(!q_x1 || !q_y1 || !q_b1) goto cleanup;

        bigint_t *temp_x = bigint_sub(x ,q_x1);
        bigint_t *temp_y = bigint_sub(y, q_y1);
        bigint_t *temp_b = bigint_sub(a1, q_b1);

        bigint_free(q_x1); q_x1 = NULL;
        bigint_free(q_y1); q_y1 = NULL;
        bigint_free(q_b1); q_b1 = NULL;

        bigint_free(x);
        bigint_free(y);
        bigint_free(a1);
        // Transfer ownership
        x =  x1;
        x1 = temp_x;
        
        y = y1;
        y1 = temp_y;

        a1 = b1;
        b1 = temp_b;

        if(!x1 || !y1 || !b1) goto cleanup;
    }

    result = a1;
    a1=NULL;

    // Assign answer x,y to dest
    *x_dest = x; x = NULL;
    *y_dest = y; y = NULL;

cleanup:
    if(x) bigint_free(x);
    if(y) bigint_free(y);
    if(x1) bigint_free(x1);
    if(y1) bigint_free(y1);
    if(a1) bigint_free(a1);
    if(b1) bigint_free(b1);
    if(q) bigint_free(q);
    if(q_x1) bigint_free(q_x1);
    if(q_y1) bigint_free(q_y1);
    if(q_b1) bigint_free(q_b1);

    LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "GCD: ", result);
    LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "GCD coefficient x: ", *x_dest);
    LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "GCD coefficient y: ", *y_dest);

    return result;
}