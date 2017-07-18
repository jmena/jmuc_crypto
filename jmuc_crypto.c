
#define JMUC_CRYPTO_IMPLEMENTATION
#include "jmuc_crypto.h"

#include <stdio.h>
#include <string.h>

void test(char* str, char* expected) {
    uint8_t sha1[20];
    jmuc_sha1_compute(str, strlen(str), sha1);

    char sha1_str[100];
    sprintf(sha1_str,
            "%02x%02x%02x%02x "
            "%02x%02x%02x%02x "
            "%02x%02x%02x%02x "
            "%02x%02x%02x%02x "
            "%02x%02x%02x%02x",
            sha1[ 0], sha1[ 1], sha1[ 2], sha1[ 3],
            sha1[ 4], sha1[ 5], sha1[ 6], sha1[ 7],
            sha1[ 8], sha1[ 9], sha1[10], sha1[11],
            sha1[12], sha1[13], sha1[14], sha1[15],
            sha1[16], sha1[17], sha1[18], sha1[19]
    );

    // printf("%s\n", sha1_str);

    if (strcmp(expected, sha1_str) != 0) {
        printf("Invalid case: %s expected: %s got:%s\n", str, expected, sha1_str);
    }
}

int main() {
    test("", "da39a3ee 5e6b4b0d 3255bfef 95601890 afd80709");
    test("abc", "a9993e36 4706816a ba3e2571 7850c26c 9cd0d89d");
    test("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "84983e44 1c3bd26e baae4aa1 f95129e5 e54670f1");
    test("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", "a49b2446 a02c645b f419f995 b6709125 3a04a259");

    char long_str[1000001];
    for (int i=0; i < 1000000; i++) {
        long_str[i] = 'a';
    }
    long_str[1000000] = 0;
    test(long_str, "34aa973c d4c4daa4 f61eeb2b dbad2731 6534016f");

    printf("FINISHED\n");
    return 0;
}
