#include "rsa/key_generator.h"


/*
* Update the seed based on the given input by combining it with time and other variables
* Takes as pointer to an uint32_t, 
* in case of NULL pointer it will relay only on other variable to generate seed.
*/
void kgn_set_seed(const uint32_t *var){
    uint32_t user_input = (var)?*var:0;

    int variable = 10;

    uint32_t seed = 
        (uint32_t)time(NULL) ^
        (uint32_t)clock() ^
        (uint32_t)getpid() ^
        (uint32_t)(uintptr_t)&variable ^
        user_input;
    
    srand(2829603626);

    LOG(MODULE, LOG_LEVEL_DEBUG, "The generated seed %u",seed);
}

/*
* Generates a random bigint of number of limbs given in input
*/
void generate_bigint(size_t limbs, bigint_t *num){
    for(size_t i = 0; i<limbs;i++){
        uint32_t val = rand();
        // Multiplys the number so num <= 2^32 - 1
        val *= rand();
        num->limbs[i] = val;
    }
    num->size = limbs;
    num->negative = false;
}


/*
* Generate two primes in range best suited for key
* Key under the size
*/
int kgn_generate_pq(bigint_t **p_dest, bigint_t **q_dest, size_t size){
    int result = 0;
    bigint_t *ub = NULL;
    bigint_t *p = NULL;
    bigint_t *q = NULL;


    ub = bigint_create(1);
    if(ub==NULL) goto cleanup;

    if(bigint_add_uint32_inplace(ub,1)!=1) goto cleanup;

    if(bigint_shift_left_inplace(ub, size)!=1) goto cleanup;

// TODO: Make infinite looping protection, or logging on infinite looping
retry:
    p = alg_generate_prime(NULL, ub);
    q = alg_generate_prime(NULL, ub);
    if(!p || !q) goto cleanup;

    // check if both are not same or suitable
    if(bigint_compare(p, q)==0){
        bigint_free(p);bigint_free(q);
        p = q = NULL;
        goto retry;
    }

    result = 1;
    *p_dest = p; p = NULL;
    *q_dest = q; q = NULL;

cleanup:
    if(ub) bigint_free(ub);
    if(p) bigint_free(p);
    if(q) bigint_free(q);

    return result;
}

/*
* Check if inverse exists, if exists compute it's inverse
* Uses algorithm similar to euclid's algorithm for gcd
* This is the implementation of the algorithm presented in RSA paper
* It's simultaneously compute gcd and also inverse.
*/
bigint_t *kgn_compute_inverse(const bigint_t *d, const bigint_t *mod){
    bigint_t *x, *y;
    bigint_t *g = alg_ext_euclied_gcd(d, mod, &x, &y);
    if(g==NULL) return NULL;
    
    // GCD is not 1, inverse not exits
    if(bigint_compare_abs_uint32(g, 1)!=0){
        LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "inverse doesn't exits: ", g);

        bigint_free(g);
        bigint_free(x);
        bigint_free(y);
        return NULL;
    }
    bigint_free(g);
    bigint_free(y);

    while(x->negative || bigint_compare(x,mod)>=0){
        if(x->negative){
            if(bigint_add_inplace(x,mod)!=1){

            bigint_free(x); return NULL;
            }
        }
        else if(bigint_sub_inplace(x, mod)!=1){
            bigint_free(x); return NULL;
        }

    }

    return x;
}

int kgn_generate_skey_pair(const bigint_t *p, const bigint_t *q, bigint_t **d_dest, bigint_t **e_dest, size_t size){
    const bigint_t *lb = p; // Max of p and q
    if(bigint_compare(p,q)==-1) lb = q;

    bigint_t *ub = NULL;
    bigint_t *d = NULL;
    bigint_t *e = NULL;
    bigint_t *p_1 = NULL, *q_1 = NULL;
    bigint_t *totient_pq = NULL;
    
    int result = 0;
    

    ub = bigint_create(1); if(ub==NULL) goto cleanup;
    if(bigint_add_uint32_inplace(ub, 1)!=1) goto cleanup;

    if(bigint_shift_left_inplace(ub, size)!=1) goto cleanup;

    // calculate totem(pq)
    p_1 = bigint_copy(p);
    q_1 = bigint_copy(q);
    if(!p_1 || !q_1) goto cleanup;

    if(bigint_sub_uint32_inplace(p_1, 1)!=1 
        || bigint_sub_uint32_inplace(q_1, 1)!=1)
        goto cleanup;

    totient_pq = bigint_mul_fft(p_1, q_1);
    if(totient_pq==NULL) goto cleanup;


    for(int i=0; i<40;i++){
        // Generate d, first key
        d = alg_generate_prime(lb, ub); if(d==NULL) goto cleanup;

        LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "Generated d=",d);

        // Now generate e against d
        e = kgn_compute_inverse(d, totient_pq); 

        if(e) {
            LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "Inverse e=",e);
            break;

        }

        LOG_BIGINT(MODULE, LOG_LEVEL_DEBUG, "Inverse generation failed for d=",d);
        // if e fails or inverse doesn't exists try with another d
        bigint_free(d);
        d=NULL;
    }

    *d_dest = d; d=NULL;
    *e_dest = e; e=NULL;

    result = 1;

cleanup:
    if(ub) bigint_free(ub);
    if(d) bigint_free(d);
    if(e) bigint_free(e);
    if(p_1) bigint_free(p_1);
    if(q_1) bigint_free(q_1);
    if(totient_pq) bigint_free(totient_pq);

    return result;
}

int kgn_gen_key_pair(
    bigint_t **n_dest,
    bigint_t **d_dest,
    bigint_t **e_dest,
    size_t mx_size
){
    bigint_t *p = NULL, *q = NULL;
    bigint_t *n = NULL;
    bigint_t *d = NULL, *e = NULL;
    int result = 0;
    if(kgn_generate_pq(&p, &q, mx_size>>1)!=1) goto cleanup;

    n = bigint_mul_fft(p,q); if(n==NULL) goto cleanup;

    if(kgn_generate_skey_pair(p,q, &d, &e, mx_size)!=1) goto cleanup;

    *n_dest = n; n = NULL;
    *d_dest = d; d=NULL;
    *e_dest = e; e=NULL;

    result = 1;

cleanup:
    if(p) bigint_free(p);
    if(q) bigint_free(q);
    if(n) bigint_free(n);
    if(d) bigint_free(d);
    if(e) bigint_free(e);

    return result;
}