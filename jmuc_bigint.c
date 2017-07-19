

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t  *data;
    uint32_t size;
    uint32_t reserved;
} jmuc_bigint;


void print_bigint(jmuc_bigint num) {
    // uint32_t idx = num.size;
    // while (idx--) {
    //     printf("%02x ", num.data[idx]);
    // }
    for (int i=0; i < num.size; i++) {
        printf("%02x ", num.data[i]);
    }
    printf("\n");
}


void jmuc_bigint_set_zero(jmuc_bigint* n) {
    n->size = 0;
}

void jmuc_bigint_set_zeros(jmuc_bigint* n) {
    n->size = 0;
    memset(n->data, 0, n->reserved);
}

void jmuc_bigint_free(jmuc_bigint* n) {
    free(n->data);
    n->size = 0;
    n->data = 0;
    n->reserved = 0;
}


static char to_hex(uint8_t v) {
    // printf("%d", v);
    if (v > 0xF) {
        return '*';
    }
    return "0123456789ABCDEF"[v];
}

static char from_hex(char ch) {
    if ('0' <= ch && ch <= '9') {
        return ch - '0';
    } else if ('a' <= ch && ch <= 'f') {
        return ch - 'a' + 10;
    } else if ('A' <= ch && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return 0;
}

static void reduce_size(jmuc_bigint *n) {
    while (n->size != 0 && n->data[n->size-1] == 0) {
        n->size--;
    }
}

uint32_t jmuc_bigint_to_hex(jmuc_bigint* n, char *buffer, uint32_t buffer_size) {
    reduce_size(n);
    memset(buffer, 0, buffer_size);
    // printf("to-hex: %d\n", n->size);
    if (buffer == 0 || buffer_size <= n->size) {
        // printf("here: %d\n", n->size);
        return n->size + 1;
    }
    for (uint32_t i=0; i < n->size; i++) {
        //printf("%02x", n->data[i]);
        buffer[2*i]     = to_hex(n->data[n->size - i - 1] >> 4);
        buffer[2*i + 1] = to_hex(n->data[n->size - i - 1] & 0x0F);
    }

    // buffer[n->size * 2] = 0;
    return 0;
}


jmuc_bigint jmuc_bigint_new2() {
    jmuc_bigint num;
    num.data = 0;
    num.size = 0;
    num.reserved = 0;
    return num;
}


void jmuc_bigint_reserve_size(jmuc_bigint* num, uint32_t reserve) {

    if (num->reserved >= reserve) {
        return;
    }

    num->data = realloc(num->data, reserve);
    if (num->reserved < reserve) {
        for (uint32_t i=num->reserved; i < reserve; i++) {
            num->data[i] = 0;
        }
    }
    num->reserved = reserve;
}

void jmuc_bigint_push_byte(jmuc_bigint* num, uint8_t v) {
    // printf("pushing: %02x\n", v);
    jmuc_bigint_reserve_size(num, num->size+1);
    num->data[num->size] = v;
    num->size++;
}

void jmuc_bigint_from_hex(jmuc_bigint* n, char *buffer, uint32_t buffer_size) {
    jmuc_bigint_set_zero(n);
    for (uint32_t i=0; i < buffer_size; i += 2) {
        uint32_t hi = from_hex(buffer[buffer_size - i - 2]);
        uint32_t lo = from_hex(buffer[buffer_size - i - 1]);
        jmuc_bigint_push_byte(n, (hi << 4) | lo);
    }
}


void set_uint64_t(jmuc_bigint* n, uint64_t v) {
    jmuc_bigint_reserve_size(n, 8);
    jmuc_bigint_set_zero(n);

    // printf("-2-\n");
    for (int i=0; i < 8; i++) {
        jmuc_bigint_push_byte(n, v & 0xFF);
        v = v >> 8;
    }
}

uint64_t get_uint64_t(jmuc_bigint n) {
    uint64_t v = 0;
    // printf("-1-\n");
    for (int i=0; i < n.size; i++) {
        v = v << 8;
        // printf("getting %02x 0x%llx\n", n.data[7 - i], v);
        v = v | n.data[n.size - i - 1];
    }
    // printf("returning: %lld\n", v);
    return v;
}





// jmuc_bigint jmuc_bigint_new(uint32_t reserved) {
//     jmuc_bigint num;
//     num.data = (uint8_t*) calloc(reserved, 1);
//     num.size = 0;
// //    num.reserved = reserved;
//     return num;
// }

void jmuc_bigint_add(jmuc_bigint n1, jmuc_bigint n2, jmuc_bigint* num) {
    // TODO: something faster is possible

    uint32_t max_size = (n1.size > n2.size) ? n1.size : n2.size;
    uint32_t reserve = max_size + 2;
    jmuc_bigint_reserve_size(num, reserve);

    // copy n1
    for (uint32_t i=0; i < n1.size; i++) {
        num->data[i] = n1.size;
        num->size++;
    }

    // copy n2
    for (uint32_t i=0; i < n2.size; i++) {
        uint32_t sum = num->data[i] + n2.data[i];
        num->data[i] = sum & 0xFF;
        num->data[i+1] = sum >> 1;
    }

    // determine size
    for (uint32_t i=0; i < reserve; i++) {
        if (num->data[i] != 0) {
            num->size = i + 1;
        }
    }
}


// void print_bigint2(jmuc_bigint num, uint32_t idx) {
//     while (idx--) {
//         printf("%02x ", num.data[idx]);
//     }
//     printf("\n");
// }

void jmuc_bigint_mult(jmuc_bigint n1, jmuc_bigint n2, jmuc_bigint* num) {
    // TODO: something faster is possible

    uint32_t reserved = n1.size + n2.size + 2;
    jmuc_bigint_reserve_size(num, reserved);
    jmuc_bigint_set_zeros(num);

    // printf("multiply: 0x%llx 0x%llx\n", get_uint64_t(n1), get_uint64_t(n2));

    for (uint32_t i=0; i < n1.size; i++) {

        uint32_t d1 = n1.data[i];
        if (d1 == 0) {
            continue;
        }

        // printf("multiply: %2x\n", d1);

        for (uint32_t j=0; j < n2.size; j++) {
            uint32_t d2 = n2.data[j];
            uint32_t mult = d1 * d2;

            if (d2 == 0) {
                continue;
            }

            // printf("      by: %2x => mult:%x\n", d2, mult);

            uint32_t sum1 = num->data[i+j]   + ((mult) & 0xFF);
            uint32_t sum2 = num->data[i+j+1] + ((mult >> 8) & 0xFF) + ((sum1 >> 8) & 0xFF);
            num->data[i+j]   = sum1;
            num->data[i+j+1] = sum2;

            uint32_t carry = sum2 >> 8;
            for (int k=i+j+2; carry != 0; k++) {
                uint32_t new_sum = num->data[k] + carry;
                num->data[k] = new_sum & 0xFF;
                carry = new_sum >> 8;
            }
            // uint32_t sum3 = num->data[i+j+2] + ((sum2) >> 8);
            // uint32_t sum4 = num->data[i+j+3] + ((sum3) >> 8);
            // uint32_t sum5 = num->data[i+j+4] + ((sum4) >> 8);

            // if (sum5 > 0xFF) {
                // printf("sum4: %x\n", sum3);
                // num->data[0] = 0;
                // num->data[1] = 0;
            // }


            // num->data[i+j+2] = sum3;
            // num->data[i+j+3] = sum4;
            // num->data[i+j+4] = sum5;

            // printf("current: 0x%llx\n", get_uint64_t(*num));
        }
    }
    // determine size
    for (uint32_t i=0; i < reserved; i++) {
        if (num->data[i] != 0) {
            num->size = i+1;
        }
    }
}

int compare(jmuc_bigint n1, jmuc_bigint n2) {
    uint32_t n1_size = n1.size;
    uint32_t n2_size = n2.size;
    while (n1_size > 0 && n1.data[n1_size-1] == 0) {
        n1_size--;
    }
    while (n2_size > 0 && n2.data[n2_size-1] == 0) {
        n2_size--;
    }

    if (n1_size < n2_size) {
        return -1;
    } else if (n1_size > n2_size) {
        return 1;
    }
    // same size
    for (int i=n2_size-1; i >= 0; i--) {
        int diff = (int) n1.data[i] - (int)n2.data[i];
        if (diff < 0) {
            return -1;
        } else if (diff > 0) {
            return 1;
        }
    }
    return 0;
}

jmuc_bigint jmuc_bigint_top_number(jmuc_bigint src, uint32_t digits) {
    jmuc_bigint num;
    num.data = src.data + src.size - digits;
    num.size = digits;
    return num;
}

uint32_t top_digit(jmuc_bigint src, uint32_t offset) {
    return src.data[src.size - offset - 1];
}

inline static void jmuc_bigint_decrease(uint8_t *data) {
    while (*data == 0) {
        *data = 0xFF;
        ++data;
    }
    --(*data);
}

jmuc_bigint sub_inplace(jmuc_bigint n1, jmuc_bigint n2) {
    // jmuc_bigint num = jmuc_bigint_new(n1.size);
    // for (int i = 0; i < n1.size; i++) {
    //     num.data[i] = n1.data[i];
    //     num.size = n1.size;
    // }

    for (int i = 0; i < n2.size; i++) {

        uint32_t base = n1.data[i] - n2.data[i];
        // printf("  %02x - %02x = %02x\n", n1.data[i], n2.data[i], base);
        if (base > 0xFF * 0xFF) {
            base += 0x100;
            // printf("  ===> %02x\n", base);
            jmuc_bigint_decrease(n1.data + i + 1);
        }
        n1.data[i] = base & 0xFF;
    }

    return n1;
}


int is_zero(jmuc_bigint n) {
    for (int i=n.size; i--; ) {
        if (n.data[i] != 0) {
            return 0;
        }
    }
    return 1;
}


void jmuc_bigint_copy(jmuc_bigint* dst, jmuc_bigint* src) {
    jmuc_bigint_reserve_size(dst, src->size);
    dst->size = src->size;
    memcpy(dst->data, src->data, src->size);
}

void jmuc_bigint_div(jmuc_bigint* n, jmuc_bigint* d, jmuc_bigint* q, jmuc_bigint* r) {

    reduce_size(n);
    reduce_size(d);
    jmuc_bigint_set_zero(q);
    jmuc_bigint_set_zero(r);

    jmuc_bigint_copy(r, n);

    // printf("--div-- 0x%llx\n"
    //         "      / 0x%llx   (res:0x%llx)\n",
    //         get_uint64_t(*r),
    //         get_uint64_t(*d),
    //         get_uint64_t(*q)
    // );

    uint8_t raw_num;

    jmuc_bigint a_num;
    a_num.size = 1;
    a_num.data = &raw_num;

    jmuc_bigint mult_res = jmuc_bigint_new2();

    jmuc_bigint a_num_window;
    a_num_window.size = d->size;
    a_num_window.data = r->data + r->size - d->size;
    a_num_window.reserved = r->reserved;
    if (a_num_window.data < r->data) {
        return;
    }


    for (;;) {
        // printf("start main cycle: w:0x%llx / 0x%llx  n1:%llx c:%d\n",
        //     get_uint64_t(a_num_window),
        //     get_uint64_t(*d),
        //     get_uint64_t(*r),
        //     compare(a_num_window, *d)
        // );
        uint32_t candidate = 0;
        if (compare(a_num_window, *d) >= 0) {
            if (d->size == a_num_window.size) {
                // printf("candidate option1\n");
                candidate = top_digit(a_num_window, 0) / top_digit(*d, 0);
            } else if (a_num_window.size == d->size+1) {
                // printf("candidate option2\n");
                candidate = ((top_digit(a_num_window, 0) << 8) + top_digit(a_num_window, 1)) / top_digit(*d, 0);
            } else {
                // printf("candidate option3\n");
                candidate = 0xFF;
            }
            if (candidate > 0xFF) {
                candidate = 0xFF;
            }

            for (;;) {
                raw_num = candidate;
                jmuc_bigint_mult(a_num, *d, &mult_res);
                // printf("here3. raw candidate: 0x%x mul:0x%llx w:0x%llx c:%d\n",
                //     raw_num,
                //     get_uint64_t(mult_res),
                //     get_uint64_t(a_num_window),
                //     compare(mult_res, a_num_window)
                // );
                //reduce_size(&mult_res);
                if (compare(mult_res, a_num_window) <= 0) {

                    // printf("sub: 0x%llx\n", get_uint64_t(a_num_window));
                    // printf("  -: 0x%llx   (0x%x x 0x%llx)\n", get_uint64_t(mult_res), candidate, get_uint64_t(*n2));

                    jmuc_bigint sub_res = sub_inplace(a_num_window, mult_res);

                    // printf("  =: 0x%llx\n", get_uint64_t(sub_res));
                    break;
                }

                candidate--;
            }
        }

        jmuc_bigint_push_byte(q, candidate);

        if (a_num_window.data == r->data) {
            // printf("Stopping. #3\n");
            break;
        }
        // move the window
        a_num_window.data--;
        a_num_window.reserved++;
        a_num_window.size++;

        reduce_size(&a_num_window);

    }

    // we need to invert the data in the result
    // printf("size of res:%d - value:0x%llx\n", res->size, get_uint64_t(*res));
    for (uint32_t i = q->size / 2; i--; ) {
        uint8_t t = q->data[i];
        q->data[i] = q->data[q->size - i - 1];
        q->data[q->size - i - 1] = t;
    }
    // printf("size of res:%d - value:0x%llx\n", res->size, get_uint64_t(*res));

    jmuc_bigint_free(&mult_res);
}


int jmuc_bigint_is_odd(jmuc_bigint* n) {
    if (n->size == 0) {
        return 0;
    }
    return n->data[0] % 2 == 1;
}

void jmuc_bigint_pow_mod(jmuc_bigint* base_, jmuc_bigint* exp_, jmuc_bigint* mod, jmuc_bigint* r) {

    // printf("here!\n");

    // calculate c = m^e (mod n)
    jmuc_bigint_set_zero(r);
    jmuc_bigint_push_byte(r, 1);

    // printf("here!\n");

    jmuc_bigint base = jmuc_bigint_new2();
    jmuc_bigint_copy(&base, base_);

    jmuc_bigint expo = jmuc_bigint_new2();
    jmuc_bigint_copy(&expo, exp_);

    jmuc_bigint tmp = jmuc_bigint_new2();
    jmuc_bigint tmp2 = jmuc_bigint_new2();

    jmuc_bigint two;
    uint8_t raw_data = 2;
    two.data = &raw_data;
    two.size = 1;

    // printf("base := base %% modulus. b:%lld mod:%lld\n", get_uint64_t(base), get_uint64_t(*mod));

    // base := base % modulus
    jmuc_bigint_div(&base, mod, &tmp, &tmp2);
    // printf("here! x3.1\n");
    jmuc_bigint_copy(&base, &tmp2);

    // printf("here! x3.2\n");
    while (!is_zero(expo)) {
        if (jmuc_bigint_is_odd(&expo)) {
            // result := (result * base) % modulus
            jmuc_bigint_mult(*r, base, &tmp); // tmp = result * base
            jmuc_bigint_div(&tmp, mod, &tmp2, r); // result = (tmp % mod)
        }

        // exponent := exponent >> 1
        jmuc_bigint_div(&expo, &two, &tmp, &tmp2);
        jmuc_bigint_copy(&expo, &tmp);

        // base := (base * base) mod modulus
        jmuc_bigint_mult(base, base, &tmp); // tmp = base * base
        jmuc_bigint_div(&tmp, mod, &tmp2, &base); // base = (tmp % mod)
    }
}


/*

1636471287       /     9823

0536471287       /     9823  = 17
 9823
--------------
 65217


0952171287       /     9823

*/



void test_mult(jmuc_bigint* n1, jmuc_bigint* n2, jmuc_bigint* n3, uint64_t v1, uint64_t v2) {
    set_uint64_t(n1, v1);
    set_uint64_t(n2, v2);
    jmuc_bigint_mult(*n1, *n2, n3);
    uint64_t res = get_uint64_t(*n3);
    if (res != v1 * v2) {
        printf("error. 0x%llx x 0x%llx = 0x%llx but received 0x%llx\n", v1, v2, v1*v2, res);
        exit(1);
    }
}

void test_div(jmuc_bigint* n1, jmuc_bigint* n2, jmuc_bigint* n3, jmuc_bigint* n4, uint64_t v1, uint64_t v2) {
    // uint64_t q = v1 / v2;
    // uint64_t r = v1 % v2;
    set_uint64_t(n1, v1);
    set_uint64_t(n2, v2);
    jmuc_bigint_div(n1, n2, n3, n4);
    uint64_t q = get_uint64_t(*n3);
    uint64_t r = get_uint64_t(*n4);
    if (q != v1 / v2) {
        printf("error. 0x%llx / 0x%llx = 0x%llx but received 0x%llx\n", v1, v2, v1/v2, q);
        exit(1);
    }
    if (r != v1 % v2) {
        printf("error. 0x%llx / 0x%llx = 0x%llx but received 0x%llx\n", v1, v2, v1%v2, r);
        exit(1);
    }
}


int main() {
    uint32_t buffer_size = 4000000;
    char *buffer = malloc(buffer_size);
    jmuc_bigint n1 = jmuc_bigint_new2();
    jmuc_bigint n2 = jmuc_bigint_new2();
    jmuc_bigint n3 = jmuc_bigint_new2();
    jmuc_bigint n4 = jmuc_bigint_new2();

    test_div(&n1, &n2, &n3, &n4, 0x3b9aca00, 0x2720);
    test_div(&n1, &n2, &n3, &n4, 4, 497);

    // set_uint64_t(&n1, 1001);
    // set_uint64_t(&n2, 345);
    //
    // if (get_uint64_t(n1) != 1001) {
    //     printf("Error. expecting %lld but received: %lld\n", 1001LL, get_uint64_t(n1));
    // }
    // if (get_uint64_t(n2) != 345) {
    //     printf("Error. expecting %lld but received: %lld\n", 345LL, get_uint64_t(n2));
    // }
    //
    // jmuc_bigint n3 = jmuc_bigint_new2();
    //
    // test_mult(&n1, &n2, &n3, 0x3b9ace26, 0xfd);
    // test_mult(&n1, &n2, &n3, 0x3b9ace26, 0xcafd);
    // test_mult(&n1, &n2, &n3, 0x3b9ace26, 0x9acafd);
    //
    // test_mult(&n1, &n2, &n3, 0x26, 0x3b9acafd);
    // test_mult(&n1, &n2, &n3, 0xce26, 0x3b9acafd);
    // test_mult(&n1, &n2, &n3, 0x9ace26, 0x3b9acafd);
    //
    // test_mult(&n1, &n2, &n3, 0x3b, 0x3b9acafd);
    // test_mult(&n1, &n2, &n3, 0x9a, 0x3b9acafd);
    // test_mult(&n1, &n2, &n3, 0xce, 0x3b9acafd);
    // test_mult(&n1, &n2, &n3, 0x26, 0x3b9acafd);
    //
    // test_mult(&n1, &n2, &n3, 0x9ace26, 0x3b9acafd);
    //
    // test_mult(&n1, &n2, &n3, 0x3b9ace26, 0x3b9acafd);

    // jmuc_bigint_set_zero(&n1);
    // jmuc_bigint_set_zero(&n2);
    // for (int i=0; i < 10000; i++) {
    //     jmuc_bigint_push_byte(&n1, 0xFF);
    //     jmuc_bigint_push_byte(&n2, 0xFF);
    // }
    // printf("done 1\n");

    // uint64_t start_i = 1000000000;
    // uint64_t how_many_i = 10000;
    // uint64_t start_j = 10000;
    // uint64_t how_many_j = 10000;
    //
    // for (uint64_t i=start_i; i < start_i + how_many_i; i++) {
    //     printf("Current i: %lld\n", i);
    //     // for (uint64_t j=1000000000; j < 1000010000; j++) {
    //     for (uint64_t j=start_j; j < start_j + how_many_j; j++) {
    //         set_uint64_t(&n1, i);
    //         set_uint64_t(&n2, j);
    //         test_div(&n1, &n2, &n3, &n4, i, j);
    //     }
    // }


//0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF

    // n1.data[0] = 0xFF;
    // n1.data[1] = 0xFE;
    // n1.data[2] = 0xFD;
    // n1.data[3] = 0xFB;
    // n1.size = 4;
    // print_bigint(n1);
    // n2.data[0] = 0xF9;
    // n2.data[1] = 0xF8;
    // n2.data[2] = 0xF7;
    // n2.data[3] = 0xF6;
    // n2.data[4] = 0xF5;
    // n2.size = 5;
    // print_bigint(n2);
    //
    // jmuc_bigint_mult(n1, n2, &n3);

    // printf("here xxx\n");
    set_uint64_t(&n1, 5);
    // printf("here xxx\n");
    set_uint64_t(&n2, 117);
    // printf("here xxx\n");
    set_uint64_t(&n3, 19);
    // printf("here xxx\n");
    jmuc_bigint_pow_mod(&n1, &n2, &n3, &n4);
    // printf("%lld^%lld (mod %lld) = %lld\n", get_uint64_t(n1), get_uint64_t(n2),get_uint64_t(n3), get_uint64_t(n4));
    //
    // printf("done 2\n");


    char *str1 = "d94d889e88853dd89769a18015a0a2e6bf82bf356fe14f251fb4f5e2df0d9f9a94a68a30c428b39e3362fb3779a497eceaea37100f264d7fb9fb1a97fbf621133de55fdcb9b1ad0d7a31b379216d79252f5c527b9bc63d83d4ecf4d1d45cbf843e8474babc655e9bb6799cba77a47eafa838296474afc24beb9c825b73ebf549";
    char *str2 = "047b9cfde843176b88741d68cf096952e950813151058ce46f2b048791a26e507a1095793c12bae1e09d82213ad9326928cf7c2350acb19c98f19d32d577d666cd7bb8b2b5ba629d25ccf72a5ceb8a8da038906c84dcdb1fe677dffb2c029fd8926318eede1b58272af22bda5c5232be066839398e42f5352df58848adad11a1";
    if (strlen(str1) % 2 == 1) {
        printf("error in str1\n");
        return 0;
    }
    if (strlen(str2) % 2 == 1) {
        printf("error in str2\n");
        return 0;
    }

    jmuc_bigint public_mod = jmuc_bigint_new2();
    jmuc_bigint_from_hex(&public_mod, str1, strlen(str1));

    char *str_e = "010001";
    jmuc_bigint public_e = jmuc_bigint_new2();
    jmuc_bigint_from_hex(&public_e, str_e, strlen(str_e));

    // jmuc_bigint_from_hex(&private_mod, str2, strlen(str2));

    jmuc_bigint message = jmuc_bigint_new2();
    jmuc_bigint encrypted = jmuc_bigint_new2();
    set_uint64_t(&message, 0xabcdef1234LL);

    jmuc_bigint_to_hex(&n3, buffer, buffer_size);


    jmuc_bigint_to_hex(&message, buffer, buffer_size);
    printf("message:%s\n", buffer);
    jmuc_bigint_to_hex(&public_e, buffer, buffer_size);
    printf("public_e:%s\n", buffer);
    jmuc_bigint_to_hex(&public_mod, buffer, buffer_size);
    printf("public_mod:%s\n", buffer);

    jmuc_bigint_pow_mod(&message, &public_e, &public_mod, &encrypted);

    jmuc_bigint_to_hex(&encrypted, buffer, buffer_size);
    printf("encrypted:%s\n", buffer);



    // printf("out:%d\n", jmuc_bigint_to_hex(&n3, buffer, buffer_size));
    // printf("%s\n", buffer);

    // for (uint64_t i=1000000000; i < 1000100000; i++) {
    //     printf("Current i: %lld\n", i);
    //     for (uint64_t j=1000000000; j < 1000100000; j++) {
    //         set_uint64_t(&n1, i);
    //         set_uint64_t(&n2, j);
    //         jmuc_bigint_mult(n1, n2, &n3);
    //         uint64_t res = get_uint64_t(n3);
    //         if (res != i * j) {
    //             printf("error. %lld x %lld = %lld but received %lld\n", i, j, i*j, res);
    //             return 1;
    //         }
    //     }
    // }


    //
    // printf("---------\n");
    // print_bigint(jmuc_bigint_top_number(n3, 1));
    // print_bigint(jmuc_bigint_top_number(n3, 2));
    // print_bigint(jmuc_bigint_top_number(n3, 3));
    // print_bigint(jmuc_bigint_top_number(n3, 4));
    // print_bigint(jmuc_bigint_top_number(n3, 5));
    // print_bigint(jmuc_bigint_top_number(n3, 6));
    // print_bigint(jmuc_bigint_top_number(n3, 7));
    //
    // n1.data[0] = 1;
    // jmuc_bigint_div(n3, n1, &x, &y);
    // printf("    ");
    // print_bigint(n3);
    //
    // printf("  - ");
    // print_bigint(n1);
    //
    // printf("  = ");
    // print_bigint(sub(n3, n1));

}
