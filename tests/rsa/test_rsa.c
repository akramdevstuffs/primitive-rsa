#include "../test_helpers.h"
#include <stdlib.h>
#include <stdio.h>

void test_algorithms(void);
void test_key_generator(void);

int main(){
    test_algorithms();
    test_key_generator();


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