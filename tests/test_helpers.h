#ifndef TEST_HELPER
#define TEST_HELPER

extern int tests_failed;
extern int tests_run;

#define CHECK(condition, message)                              \
    do {                                                       \
        tests_run++;                                           \
        if (!(condition)) {                                    \
            fprintf(stderr, "FAIL: %s (line %d)\n",            \
                    message, __LINE__);                         \
            tests_failed++;                                    \
        }                                                      \
    } while (0)

#endif