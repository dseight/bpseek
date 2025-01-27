#include "pattern.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>

static int has_failures = 0;

static void test_32bit_clean_pattern(void)
{
    static const uint32_t buf[] = {
        0x12345678, 0x12345678, 0x12345678, 0x12345678,
        0x12345678, 0x12345678, 0x12345678, 0x12345678,
    };

    struct pattern *pattern = find_pattern(buf, sizeof(buf), 1, 8, 1, 0, 0, 1);
    assert(pattern->len == 4);
}

static void test_32bit_varying_pattern(void)
{
    static const uint32_t buf[] = {
        0x12340678, 0x1234A678, 0x1234B678, 0x12342678,
        0x12343678, 0x12341678, 0x12347678, 0x12340678,
    };

    struct pattern *pattern = find_pattern(buf, sizeof(buf), 1, 8, 1, 0, 0, 1);
    assert(pattern->len == 4);
}

static int run_test(void (*fn)(void))
{
    int w, wstatus;
    pid_t pid = fork();

    if (pid == -1) {
        perror("Failed to fork");
        exit(1);
    } else if (pid == 0) {
        fn();
        exit(0);
    } else {
        do {
            w = waitpid(pid, &wstatus, WUNTRACED);
            if (w == -1) {
                perror("waitpid");
                exit(1);
            }
        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
        return wstatus;
    }
}

#define RUN(func) ({                                    \
    int _ret = run_test(func);                          \
    if (_ret)                                           \
        has_failures = 1;                               \
    printf("%s: %s\n", #func, _ret ? "FAIL" : "OK");    \
})

int main(void)
{
    RUN(test_32bit_clean_pattern);
    RUN(test_32bit_varying_pattern);
    return has_failures;
}
