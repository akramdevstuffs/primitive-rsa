#include "rsa/bigint.h"
#include <stdlib.h>
#include <string.h>


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

bool ensure_capacity(bigint_t* bigint, int capacity){
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
        for(int i = 0;i<bigint->size;i++){
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

bool *bigint_add_abs(const bigint_t *a, const bigint_t *b, bigint_t *target){

    if(a->size < b->size){
        const bigint_t *temp = a;
        a = b;
        b = temp;
    }

    if(!ensure_capacity(target, a->size+1)){
        return false;
    }

    uint64_t carry = 0;

    for(int i = 0;i<a->size;i++){
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

    for(int i = 0;i<a->size; i++){
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

    if(!ensure_capacity(a, b->capacity)){
        return -1;
    }

    memcpy(a->limbs, b->limbs, sizeof(uint32_t)*b->size);

    trim_size(a);

    return 1;
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

    for(size_t i = 0; i<a->size;i++){
        uint64_t carry = 0;
        for(size_t j = 0; j<b->size; j++){
            uint64_t curr = 
                (uint64_t)a->limbs[i] * b->limbs[i]
                + result->limbs[i+j]
                + carry;
            result->limbs[i] = (uint32_t)curr;
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
    for(size_t i = a->size - 1; i>=0;i--){
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
    for(size_t i = a->size - 1; i>=0;i--){
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
    for(size_t i = a->size - 1; i>=0;i--){
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
    for(size_t i = a->size - 1; i>=0;i--){
        __uint128_t curr = (carry << 32) + a->limbs[i];
        a->limbs[i] = curr/b;
        carry = (uint64_t)(curr%b);
    }
    trim_size(a);
}



#endif

int word_shift_right_inplace(bigint_t *a, size_t word_shift){
    if(word_shift==0) return 1;
    if(word_shift>=a->size){
        a->size = 0;
        return 1;
    }
    size_t new_size = a->size - word_shift;
    memmove(a->limbs, a->limbs + word_shift, new_size * sizeof(uint32_t));
    a->size = new_size;
    return 1;
}

void bigint_shift_right_inplace(bigint_t *a, size_t shift){
    if(a==NULL || shift == 0) return;

    size_t word_shift = shift / 32;
    unsigned bit_shift = shift % 32;

    if(word_shift >= a->size){
        a->size = 0;
        // free the ram
        return;
    }

    if(!word_shift_right_inplace(a,word_shift)) return;

    if(bit_shift>0){
        uint32_t carry = 0;
        for(size_t i = a->size - 1; i>=0;i--){
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

    if(!word_shift_left_inplace(a, word_shift)) return;

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

bigint_t *bigint_div_inplace(bigint_t *a, bigint_t *b) {
    if(b->size==0) return NULL;
    size_t shifted_limbs = 0;
    if(a->size < b->size){
        shifted_limbs = b->size - a->size;
    }
    if(shifted_limbs>0 && !ensure_capacity(a, a->size+shifted_limbs+1)){
        return NULL;
    }
    if(!word_shift_left_inplace(a, shifted_limbs)){
        return NULL;
    }
    uint32_t top = b->limbs[b->size-1];
    if(top==0){
        // Division by zero
        return NULL;
    }
    unsigned f_shift = (unsigned)__builtin_clz(top);
    if(!bigint_shift_left_inplace(a, f_shift)) return NULL;
    if(!bigint_shift_left_inplace(b, f_shift)) return NULL;

    // Ensure the first n limbs of a is less than from n limbs of v
    // Append 0 at a
    a->limbs[a->size++] = 0;

    bigint_t *q = bigint_create(a->size - b->size + 1);

    size_t n = b->size;

    for(size_t i = a->size; i-- >= b->size;){
        const uint64_t base = UINT64_C(1)<<32;
        uint64_t unn = (UINT64_C(1)<<32) * a->limbs[i] + a->limbs[i-1];
        uint32_t vnn = b->limbs[n-1];
        uint64_t qhat = unn/vnn;
        uint32_t rhat = unn % vnn;
        if (qhat >= base || qhat*b->limbs[n-2] > base*rhat + a->limbs[i-2]){
            qhat -= 1;
            rhat += vnn;
        }
        if (rhat < base && (qhat >= base || qhat*b->limbs[n-2] > base * rhat + a->limbs[i-2]))
        {
            qhat -= 1; rhat += vnn;
        }
        bigint_t *qv = bigint_mul_uint32(b, qhat);
        bool borrow = sub_by_limbs_inplace(a->limbs+i+1-n, n+1, qv->limbs, qv->size);
        bigint_free(qv);
        if(borrow){
            qhat--;
            bool carry = add_by_limbs_inplace(a->limbs+i+1-n, n+1, b->limbs, n);
        }
        q->limbs[q->size + 1 - (a->size - i - 1)] = (uint32_t)qhat;
    }
    bigint_shift_right_inplace(a, f_shift);
    bigint_shift_right_inplace(b, f_shift);
    word_shift_right_inplace(a, shifted_limbs);
    word_shift_right_inplace(b, shifted_limbs);
    trim_size(a);
    trim_size(b);
    trim_size(q);
    return q;
}