#include "rsa/bigint.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


bigint_t *bigint_create(size_t capacity) {
    bigint_t *bigint = (bigint_t *)malloc(sizeof(bigint_t));

    if (bigint == NULL) 
        return NULL;

    bigint->capacity = capacity;
    bigint->negative = false;
    bigint->size = 0;

    bigint->limbs = (uint32_t*)malloc(sizeof(uint32_t)*capacity);

    if(bigint->limbs == NULL && capacity>0){
        free(bigint);
        return NULL;
    }

    return bigint;
}

void trim_size(bigint_t* a){
    while (a->size > 0 && a->limbs[a->size - 1] == 0) {
            a->size--;
    }

    if (a->size == 0) {
        a->negative = false;
    }

    if(a->capacity > 2*(a->size+1)){
        uint32_t *limbs = (uint32_t*)realloc(a->limbs, sizeof(uint32_t)*(a->size+1)*2);
        if(limbs==NULL){
            return;
        }
        a->limbs = limbs;
        a->capacity = 2*(a->size+1);
    }
}

bool ensure_capacity(bigint_t* bigint, size_t capacity){
    if(bigint->capacity >= capacity) return true;

    // increment the size by doubling the current
    size_t new_capacity = 2*bigint->capacity;
    if(new_capacity < capacity)
        new_capacity = capacity;

    if (new_capacity > SIZE_MAX / sizeof(uint32_t)){
        return false;
    }
    
    uint32_t *limbs = (uint32_t*)realloc(bigint->limbs,sizeof(uint32_t)*new_capacity);

    if(limbs == NULL)
        return false;

    bigint->capacity = new_capacity;
    bigint->limbs = limbs;

    return true;
}

bigint_t *bigint_from_string(const char *s){
    if(s==NULL) return NULL;
    // Find sign char if exists
    bool negative = false;
    if(*s=='+' || *s=='-'){
        negative = (*s=='-');
        s++;
    }
    if(*s=='\0'){
        return NULL;
    }
    bigint_t *bigint = bigint_create(1);
    bigint->negative = negative;
    bigint->size = 1;
    bigint->limbs[0] = 0;

    for(const char *p = s; *p!='\0'; p++){
        if(*p < '0' || *p > '9'){
            bigint_free(bigint);
            return NULL;
        }
        uint32_t digit = (*p - '0');
        uint64_t carry = digit;
        for(size_t i = 0;i<bigint->size;i++){
            carry = (uint64_t)bigint->limbs[i] * 10 + carry;

            bigint->limbs[i] = (uint32_t)carry;
            carry >>= 32;
        }

        if(carry!=0){
            if(ensure_capacity(bigint, bigint->size+1)==false){
                bigint_free(bigint);
                return NULL;
            }
            bigint->limbs[bigint->size] = carry;
            bigint->size++;
        }
    }
    
    trim_size(bigint);

    return bigint;
}

void bigint_free(bigint_t* bigint){
    free(bigint->limbs);
    free(bigint);
}



int is_greater_base(const bigint_t* a, const bigint_t* b){
    if(a->size!=b->size) return 1 - 2*(a->size < b->size);

    for(size_t i = a->size; i>0;i--){
        size_t j = i - 1;
        if(a->limbs[j]==b->limbs[j]) continue;
        return 1 - 2*(a->limbs[j] < b->limbs[j]);
    }
    return 0;
}

int bigint_compare(const bigint_t* a, const bigint_t* b){
    if(a->negative!=b->negative) return 1 - 2*(a->negative);
    int cmp = is_greater_base(a,b);
    
    return a->negative ? -cmp : cmp;
}

bool bigint_add_abs(const bigint_t *a, const bigint_t *b, bigint_t *target){

    if(a->size < b->size){
        const bigint_t *temp = a;
        a = b;
        b = temp;
    }

    if(!ensure_capacity(target, a->size+1)){
        return false;
    }

    uint64_t carry = 0;

    for(size_t i = 0;i<a->size;i++){
        uint32_t u = a->limbs[i];
        uint32_t v = i<b->size? b->limbs[i]: 0;
        target->limbs[i] = carry+u+v;
        carry = (carry+u+v)>>32;
    }

    target->limbs[a->size] = carry;

    target->size = a->size+1;

    trim_size(target);

    return true;
}

bool bigint_sub_abs(const bigint_t* a, const bigint_t* b, bigint_t* target){

    if(!ensure_capacity(target, a->size))
        return false;

    uint64_t borrow = 0;

    for(size_t i = 0;i<a->size; i++){
        uint32_t u = a->limbs[i];
        uint32_t v = i<b->size ? b->limbs[i] : 0;

        uint64_t subtrahend = v + borrow;

        target->limbs[i] = (uint32_t)(u - subtrahend);
        borrow = (u<subtrahend);
    }

    target->size = a->size;
    trim_size(target);

    return true;
}

bigint_t *bigint_add(const bigint_t *a, const bigint_t *b){
    size_t max_size = a->size+1;
    if(max_size <= b->size){
        max_size = b->size + 1;
    }

    bigint_t *result = bigint_create(max_size);
    if(result==NULL) return NULL;

    if(a->negative!=b->negative){
        int res = is_greater_base(a,b);
        if(res==-1){
            const bigint_t *temp = a;
            a = b;
            b = temp;
        }

        if(bigint_sub_abs(a,b,result)==false){
            bigint_free(result);
            return NULL;
        }
    }
    else if(!bigint_add_abs(a,b,result)){
        bigint_free(result);
        return NULL;
    }

    result->negative = a->negative;

    return result;
}

int bigint_assign(bigint_t *a, const bigint_t *b){
    if(a==NULL){
        return -1;
    }

    if(!ensure_capacity(a, b->size)){
        return -1;
    }

    memcpy(a->limbs, b->limbs, sizeof(uint32_t)*b->size);

    a->size = b->size;

    trim_size(a);

    return 1;
}

bigint_t *bigint_copy(const bigint_t *a){
    if(a==NULL) return NULL;
    bigint_t *b = bigint_create(a->size);
    if(b==NULL){
        return NULL;
    }
    if(bigint_assign(b, a)!=1){
        bigint_free(b);
        return NULL;
    }
    return b;
}

/*
* Helper function for modulo of to wo 64 bit multiplication
* Provides a compiler safe function to multiply two 64 bit uint
*/
uint64_t modmul_uint64(uint64_t a, uint64_t b, uint64_t mod){
    #ifdef __SIZEOF_INT128__

    // If compliler supports 128 bit

    return ((__uint128_t)a*b)%mod;

    #else

    // Use binary multiplication to prevent overflow
    uint64_t result = 0;
    a %= mod;
    while(b){
        if(b&1){
            result = (result >= mod - a)
                    ? result - (mod - a)
                    : result + a;
        }

        b >>= 1;

        if(b){
            a = (a >= mod -a)
                ? a - (mod - a)
                : a + a;
        }
    }

    return result;

    #endif

}

/*
* Helper function to calcualte modulo exponent using binary exponentiation
*/
uint32_t modpow(uint32_t a, uint32_t b, uint32_t mod){

    uint32_t res = 1;
    a%=mod;

    while(b>0){
        if(b&1) res = (uint32_t)(((uint64_t)res*a)%mod);
        a = (uint32_t)(((uint64_t)a * a) % mod);
        b >>= 1;
    }
    return res;
}

void ntt(uint32_t *a, size_t N, uint32_t p, uint32_t g, bool inverse){
    for (size_t i = 1, j = 0; i < N; i++) {
        size_t bit = N >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;

        if (i < j){
            uint32_t temp = a[i];
            a[i] = a[j];
            a[j] = temp;
        }
    }
    uint32_t root = modpow(g, (p-1)/N, p);
    uint32_t root_inv = modpow(root, N-1, p);

    for(size_t len = 2; len<=N; len<<=1){

        uint32_t wlen = inverse?root_inv:root;

        for(size_t i = len; i<N; i<<=1)
            wlen = (uint32_t)(((uint64_t)wlen*wlen)%p);
        
        for(size_t i = 0; i<N; i+=len){
            uint32_t w = 1;
            
            for(size_t j = 0; j<len/2; j++){
                uint32_t u = a[i+j], v = (uint32_t)(((uint64_t)a[i+j+len/2]*w)%p);
                a[i+j] = u + v < p ? u + v : u+v-p;
                a[i+j+len/2] = u>=v ? u-v : p+u-v;
                w = (uint32_t)((uint64_t)w*wlen % p);
            }
        }
    }

    if(inverse){
        uint32_t N_inv = modpow(N, p-2, p);
        for(size_t i = 0; i<N;i++)
            a[i] = (uint32_t)(((uint64_t)a[i]*N_inv) % p);
    }
}

bigint_t *bigint_mul_fft(const bigint_t *a, const bigint_t *b){
    if(a==NULL || b==NULL) return NULL;
    if(a->size == 0 || b->size == 0){
        return bigint_create(0);
    }
    size_t N = 1;
    while(N < 2*(a->size + b->size)) N <<= 1;

    // Check if required N is supported by the ntt roots
    if(N > (1<<21)){
        return NULL;
    }

    uint32_t *u = (uint32_t*)malloc(sizeof(uint32_t)*N);
    if(u==NULL) return NULL;
    uint32_t *v = (uint32_t*)malloc(sizeof(uint32_t)*N);
    if(v==NULL) {
        free(u);
        return NULL;
    }
    // Split the 32 bit limbs in 16 bit limbs to prevent overflow
    // the result from fft will fit in 64 bits considering 32 bit size of N
    // for 64 bit size of N use 128 bit to fit result fft
    for(size_t i = 0; i<a->size;i++) {
        u[2*i] = (uint16_t)a->limbs[i];
        u[2*i | 1] = (a->limbs[i]>>16);
    }
    for(size_t i = 0; i<b->size;i++) {
        v[2*i] = (uint16_t)b->limbs[i];
        v[2*i | 1] = (b->limbs[i]>>16);
    }
    memset(u + 2*a->size, 0, sizeof(uint32_t)*(N - 2*a->size));
    memset(v + 2*b->size, 0, sizeof(uint32_t)*(N - 2*b->size));

    struct primitive_root {
        uint32_t p,g,pw;
    };

    static const struct primitive_root roots[2] = {
        {998244353, 3, (1<<23)},
        {1004535809, 3, (1<<21)},
        // {469762049, 3, (1<<26)}
    };

    static const unsigned no_of_roots = (sizeof(roots)/sizeof(*roots));

    uint32_t* results[no_of_roots];

    for(unsigned i = 0; i<no_of_roots; i++){
        uint32_t *u1 = u;
        uint32_t *v1 = v;
        if(i!=no_of_roots-1){
            u1 = (uint32_t*)malloc(sizeof(uint32_t)*N);
            v1 = (uint32_t*)malloc(sizeof(uint32_t)*N);
            if(u1==NULL || v1 == NULL){
                if(u1!=NULL) free(u1);
                if(v1!=NULL) free(v1);
                free(u);
                free(v);
                for(unsigned j=0; j<i;j++) free(results[j]);
                return NULL;
            }
            memcpy(u1, u, sizeof(*u)*N);
            memcpy(v1, v, sizeof(*v)*N);
        }
        ntt(u1, N, roots[i].p, roots[i].g, false);
        ntt(v1, N, roots[i].p, roots[i].g, false);

        // Multiply to compute convolution
        for(size_t j = 0; j<N;j++) u1[j] = (uint32_t)((UINT64_C(1)*u1[j]*v1[j]) % roots[i].p);
        ntt(u1, N, roots[i].p, roots[i].g, true);
        results[i] = u1;
        free(v1);
    }


    // Reconstruct the actual value of convolution using CRT
    uint64_t *res = (uint64_t*)malloc(sizeof(*res)*N);
    if(res==NULL) {
        for(unsigned i = 0;i<no_of_roots;i++) free(results[i]);
        return NULL;
    }
    memset(res, 0, sizeof(*res)*N);
    uint64_t M = 1;
    for(unsigned i = 0;i<no_of_roots;i++) M *= roots[i].p;

    // Calculate N_i to avoid recomputations
    uint32_t N_s[no_of_roots];

    for(unsigned i = 0; i<no_of_roots;i++)
        N_s[i] = modpow(M/roots[i].p, roots[i].p-2, roots[i].p);

    for(unsigned i = 0;i<N;i++){
        for(size_t j=0; j<no_of_roots;j++){
            uint32_t a_i = results[j][i];
            uint32_t M_i = M / roots[j].p;
            // Mod inverse of M_i using p
            uint32_t N_i = N_s[j];
            res[i] = (res[i] + modmul_uint64((uint64_t)a_i*M_i, N_i, M)) % M;
        }
    }

    for(unsigned i = 0; i<no_of_roots;i++) free(results[i]);

    // Normalise the res each limb should have 2^16 in range value
    uint64_t carry = 0;
    for(size_t i = 0; i<N;i++){
        uint64_t curr = res[i] + carry;
        res[i] = (uint16_t)curr;
        carry = curr>>16;
    }

    // combine the 2^16 words to 2^32 words
    bigint_t *result = bigint_create(N/2);
    for(size_t i = 0; i<N/2;i++){
        result->limbs[i] = (uint32_t)((res[2*i|1]<<16) | res[2*i]);
    }

    result->size = N>>1;
    result->negative = a->negative!=b->negative;
    trim_size(result);

    free(res);

    return result;
}

bigint_t *bigint_mul_uint32(const bigint_t *a, const uint32_t b){
    if(a==NULL) return NULL;
    size_t result_size = a->size + 1;
    bigint_t *result = bigint_create(result_size);
    if(result==NULL) return NULL;
    memset(result->limbs, 0, result_size*sizeof(*result->limbs));
    uint32_t carry = 0;
    for(size_t i = 0; i<a->size;i++){
        uint64_t curr = (uint64_t)a->limbs[i] * b + result->limbs[i] + carry;
        result->limbs[i] = (uint32_t)curr;
        carry = (curr>>32);
    }
    result->size = a->size;
    if(carry)
    result->limbs[result->size++] = (uint32_t)carry;
    trim_size(result);
    return result;
}

bigint_t *bigint_mul(const bigint_t *a, const bigint_t *b){
    if(a==NULL || b==NULL) return NULL;
    if(a->size==0 || b->size==0){
        bigint_t *zero = bigint_create(0);
        if(zero==NULL){
            return NULL;
        }
        zero->size = 0;
        zero->negative = false;
        return zero;
    }

    // Check for overflow
    if(a->size > SIZE_MAX - b->size){
        return NULL;
    }

    size_t capacity = a->size+b->size;
    bigint_t *result = bigint_create(capacity);
    if(result==NULL){
        return NULL;
    }
    memset(result->limbs, 0,sizeof(*result->limbs)*capacity);
    result->negative = (a->negative!=b->negative);
    result->size = capacity;

    for(size_t i = 0; i<a->size;i++){
        uint64_t carry = 0;
        for(size_t j = 0; j<b->size; j++){
            uint64_t curr = 
                (uint64_t)a->limbs[i] * b->limbs[j]
                + result->limbs[i+j]
                + carry;
            result->limbs[i+j] = (uint32_t)curr;
            carry = curr>>32;
        }

        result->limbs[i+b->size] = (uint32_t)carry;
    }

    trim_size(result);

    return result;
}

bigint_t *bigint_div_uint(const bigint_t *a, const uint32_t b){
    if(a==NULL || b==0) return NULL;
    if(a->size==0)
        return bigint_create(0);
    bigint_t *result = bigint_create(a->size);
    uint64_t carry = 0;
    for(size_t i = a->size - 1; i-- >0;){
        uint64_t curr = a->limbs[i] + (carry << 32);
        result->limbs[i] = (uint32_t)(curr/b);
        carry = (uint32_t)(curr%b);
    }
    trim_size(result);
    return result;
}

void bigint_div_uint_inplace(bigint_t *a,  const uint32_t b){
    if(a==NULL || b==0 || a->size == 0) return;
    uint64_t carry = 0;
    for(size_t i = a->size - 1; i-- >0;){
        uint64_t curr = a->limbs[i] + (carry << 32);
        a->limbs[i] = curr/b;
        carry = (uint32_t)(curr%b);
    }
    trim_size(a);
}

#ifdef __SIZEOF_INT128__

bigint_t *bigint_div_uint64(const bigint_t *a, const uint64_t b){
    if(a==NULL || b==0) return NULL;
    if(a->size==0)
        return bigint_create(0);
    bigint_t *result = bigint_create(a->size);
    __uint128_t carry = 0;
    for(size_t i = a->size - 1; i-- >0;){
        __uint128_t curr = (carry << 32) + a->limbs[i];
        result->limbs[i] = curr/b;
        carry = (uint64_t)(curr%b);
    }
    trim_size(result);
    return result;
}

void bigint_div_uint64_inplace(bigint_t *a,  const uint64_t b){
    if(a==NULL || b==0 || a->size == 0) return;
    __uint128_t carry = 0;
    for(size_t i = a->size - 1; i-- > 0;){
        __uint128_t curr = (carry << 32) + a->limbs[i];
        a->limbs[i] = curr/b;
        carry = (uint64_t)(curr%b);
    }
    trim_size(a);
}



#endif

void word_shift_right_inplace(bigint_t *a, size_t word_shift){
    if(word_shift==0) return;
    if(word_shift>=a->size){
        a->size = 0;
        return;
    }
    size_t new_size = a->size - word_shift;
    memmove(a->limbs, a->limbs + word_shift, new_size * sizeof(uint32_t));
    a->size = new_size;
    return;
}

void bigint_shift_right_inplace(bigint_t *a, size_t shift){
    if(a==NULL || shift == 0) return;

    size_t word_shift = shift / 32;
    unsigned bit_shift = shift % 32;

    if(word_shift >= a->size){
        a->size = 0;
        trim_size(a);
        return;
    }

    word_shift_right_inplace(a,word_shift);

    if(bit_shift>0){
        uint32_t carry = 0;
        for(size_t i = a->size; i-- > 0;){
            uint32_t curr = a->limbs[i];
            a->limbs[i] = (curr >> bit_shift) | carry;
            carry = curr << (32 - bit_shift);
        }
    }
    trim_size(a);
}

int word_shift_left_inplace(bigint_t *a, size_t word_shift){
    if(word_shift==0) return 1;
    if(!ensure_capacity(a, a->size + word_shift)) return 0;
    size_t old_size = a->size;
    
    memmove(a->limbs+word_shift, a->limbs, old_size * sizeof(uint32_t));

    memset(a->limbs, 0, sizeof(uint32_t)*word_shift);
    a->size += word_shift;
    return 1;
}

int bigint_shift_left_inplace(bigint_t *a, size_t shift){
    if(a==NULL) return 0;

    size_t word_shift = shift / 32;
    unsigned bit_shift = shift % 32;

    size_t new_size = a->size + word_shift + (bit_shift!=0);

    if(!ensure_capacity(a, new_size)){
        return 0;
    }

    if(!word_shift_left_inplace(a, word_shift)) return 0;

    if(bit_shift>0){
        uint32_t carry = 0;
        for(size_t i = word_shift; i<a->size;i++){
            uint64_t curr = ((uint64_t)a->limbs[i] << bit_shift) | carry;
            a->limbs[i] = (uint32_t)curr;
            carry = (uint32_t)(curr>>32);
        }
        if(carry != 0)
            a->limbs[a->size++] = carry;
    }
    trim_size(a);
    return 1;
}

bigint_t *bigint_div_inplace(bigint_t *_a, const bigint_t *_b) {
    if(_b->size==0) return NULL;

    size_t shifted_limbs = 0;
    if(_a->size < _b->size){
        shifted_limbs = _b->size - _a->size;
    }
    if(shifted_limbs>0 && !ensure_capacity(_a, _a->size+shifted_limbs+1)){
        return NULL;
    }
    if(!word_shift_left_inplace(_a, shifted_limbs)){
        return NULL;
    }

    bigint_t *a = _a;

    // Create a copy of b
    bigint_t *b = bigint_copy(_b);
    if(b==NULL){
        return NULL;
    }

    uint32_t top = b->limbs[b->size-1];
    if(top==0){
        // Division by zero
        return NULL;
    }
    unsigned f_shift = (unsigned)__builtin_clz(top);
    if(!bigint_shift_left_inplace(a, f_shift) || !bigint_shift_left_inplace(b, f_shift)) {
        bigint_free(b);
        return NULL;
    }

    // Ensure the first n limbs of a is less than from n limbs of v
    // Append 0 at a
    a->limbs[a->size++] = 0;

    bigint_t *q = bigint_create(a->size - b->size);
    q->size = a->size - b->size;
    q->negative = (a->negative!=b->negative);

    size_t n = b->size;

    for(size_t i = a->size; i-- > b->size;){
        const uint64_t base = UINT64_C(1)<<32;
        uint64_t unn = (UINT64_C(1)<<32) * a->limbs[i] + a->limbs[i-1];
        uint32_t vnn = b->limbs[n-1];
        uint64_t qhat = unn/vnn;
        if(n>1){
            uint64_t rhat = unn % vnn;
            if (qhat >= base || qhat*b->limbs[n-2] > base*rhat + a->limbs[i-2]){
                qhat -= 1;
                rhat += vnn;
            }
            if (rhat < base && (qhat >= base || qhat*b->limbs[n-2] > base * rhat + a->limbs[i-2]))
            {
                qhat -= 1; rhat += vnn;
            }
        }
        bigint_t *qv = bigint_mul_uint32(b, (uint32_t)qhat);
        unsigned borrow = 0;
        for(size_t j = 0, k=i-n; j<=n && k<=i; j++,k++){
            uint32_t u = a->limbs[k];
            uint32_t v = (j < qv->size) ? qv->limbs[j] : 0;
            uint64_t subrehand = (uint64_t)v + borrow;
            a->limbs[k] = (uint32_t)(u - subrehand);
            borrow = (u<subrehand);
        }
        bigint_free(qv);
        if(borrow){
            qhat--;
            unsigned carry = 0;
            for(size_t j = 0, k=i-n; j<=n && k<=i; j++,k++){
                uint32_t u = a->limbs[k];
                uint32_t v = (j<n)?b->limbs[j]:0;
                uint64_t s = (uint64_t)u + v + carry;
                a->limbs[k] = (uint32_t)s;
                carry = (s>>32);
            }
        }
        q->limbs[i - n] = (uint32_t)qhat;
    }

    // Free the copy of _b
    bigint_free(b);

    bigint_shift_right_inplace(a, f_shift);
    word_shift_right_inplace(a, shifted_limbs);
    trim_size(a);
    trim_size(q);
    
    /*
    * Use this to restore b if operating on the given divisor to save space
    // bigint_shift_right_inplace(b, f_shift);
    // word_shift_right_inplace(b, shifted_limbs);
    // trim_size(b);
    */

    return q;
}

bigint_t *bigint_mod(const bigint_t *a, const bigint_t *b){
    bigint_t *rem = bigint_copy(a);
    if(rem==NULL) return NULL;
    bigint_t *q = bigint_div_inplace(rem, b);
    bigint_free(q);
    return rem;
}