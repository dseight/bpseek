#include "hex.h"
#include "pattern.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>

static int has_failures;

DEFINE_PTR_CLEANUP_FUNC(FILE *, fclose);

static void assert_strequal(const char *actual, const char *expected)
{
    if (strcmp(actual, expected) != 0) {
        fprintf(stderr, "Expected:\n%s\n", expected);
        fprintf(stderr, "Got:\n%s\n", actual);
        abort();
    }
}

static void test_32bit_clean_pattern(void)
{
    _cleanup(free_pattern) struct pattern *pattern = NULL;

    static const uint32_t buf[] = {
        0x12345678, 0x12345678, 0x12345678, 0x12345678,
        0x12345678, 0x12345678, 0x12345678, 0x12345678,
    };

    pattern = find_pattern(buf, sizeof(buf), 1, 8, 1, 0, 0, 1);
    assert(pattern->len == 4);
}

static void test_32bit_varying_pattern(void)
{
    _cleanup(free_pattern) struct pattern *pattern = NULL;

    static const uint32_t buf[] = {
        0x12340678, 0x1234A678, 0x1234B678, 0x12342678,
        0x12343678, 0x12341678, 0x12347678, 0x12340678,
    };

    pattern = find_pattern(buf, sizeof(buf), 1, 8, 1, 0, 0, 1);
    assert(pattern->len == 4);
}

static void test_output_aligned(void)
{
    size_t stdout_buf_size;
    _autofree char *stdout_buf = NULL;
    _cleanup(fclose) FILE *f = NULL;

    static const uint8_t buf[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    };

    set_use_color(false);
    set_style(StyleUnicode);

    f = open_memstream(&stdout_buf, &stdout_buf_size);
    fprint_hex(f, buf, sizeof(buf), 0);
    fflush(f);

    assert_strequal(
        stdout_buf,
        "┌────────┬─────────────────────────┬─────────────────────────┬────────┬────────┐\n"
        "│00000000│ 00 01 02 03 04 05 06 07 ┊ 08 09 0a 0b 0c 0d 0e 0f │⋄•••••••┊•_____••│\n"
        "│00000010│ 10 11 12 13 14 15 16 17 ┊ 18 19 1a 1b 1c 1d 1e 1f │••••••••┊••••••••│\n"
        "└────────┴─────────────────────────┴─────────────────────────┴────────┴────────┘\n"
    );
}

static void test_output_unaligned_multiline(void)
{
    size_t stdout_buf_size;
    _autofree char *stdout_buf = NULL;
    _cleanup(fclose) FILE *f = NULL;

    static const uint8_t buf[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    };

    set_use_color(false);
    set_style(StyleUnicode);

    f = open_memstream(&stdout_buf, &stdout_buf_size);
    fprint_hex(f, buf, sizeof(buf), 3);
    fflush(f);

    assert_strequal(
        stdout_buf,
        "┌────────┬─────────────────────────┬─────────────────────────┬────────┬────────┐\n"
        "│00000000│          00 01 02 03 04 ┊ 05 06 07 08 09 0a 0b 0c │   ⋄••••┊••••____│\n"
        "│00000010│ 0d 0e 0f 10 11 12 13 14 ┊ 15 16 17 18 19 1a 1b 1c │_•••••••┊••••••••│\n"
        "│00000020│ 1d 1e 1f                ┊                         │•••     ┊        │\n"
        "└────────┴─────────────────────────┴─────────────────────────┴────────┴────────┘\n"
    );
}

static void test_output_unaligned_singleline(void)
{
    size_t stdout_buf_size;
    _autofree char *stdout_buf = NULL;
    _cleanup(fclose) FILE *f = NULL;

    static const uint8_t buf[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x0a };

    set_use_color(false);
    set_style(StyleUnicode);

    f = open_memstream(&stdout_buf, &stdout_buf_size);
    fprint_hex(f, buf, sizeof(buf), 4);
    fflush(f);

    assert_strequal(
        stdout_buf,
        "┌────────┬─────────────────────────┬─────────────────────────┬────────┬────────┐\n"
        "│00000000│             31 32 33 34 ┊ 35 0a                   │    1234┊5_      │\n"
        "└────────┴─────────────────────────┴─────────────────────────┴────────┴────────┘\n"
    );
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
    RUN(test_output_aligned);
    RUN(test_output_unaligned_multiline);
    RUN(test_output_unaligned_singleline);
    return has_failures;
}
