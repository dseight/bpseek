#include "hex.h"
#include "pattern.h"
#include "generator.h"
#include "bitops.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>

static int has_failures;

DEFINE_PTR_CLEANUP_FUNC(FILE *, fclose);

#define fprint_num(f, val) _Generic((val),              \
    int: fprintf_int,                                   \
    long long: fprintf_long_long,                       \
    unsigned long: fprintf_unsigned_long,               \
    unsigned long long: fprintf_unsigned_long_long      \
)(f, val)

static void fprintf_int(FILE *f, int val)
{
    fprintf(f, "%d", val);
}

static void fprintf_long_long(FILE *f, long long val)
{
    fprintf(f, "%lld", val);
}

static void fprintf_unsigned_long(FILE *f, unsigned long val)
{
    fprintf(f, "%lu", val);
}

static void fprintf_unsigned_long_long(FILE *f, unsigned long long val)
{
    fprintf(f, "%llu", val);
}

#define assert_eq(a, b)                                             \
    ({                                                              \
        typeof(a) _a = (a);                                         \
        typeof(b) _b = (b);                                         \
        if (_a != _b) {                                             \
            fprintf(stderr, "Assert `" #a " == " #b "` failed:");   \
            fprintf(stderr, "\n  " #a " = ");                       \
            fprint_num(stderr, _a);                                 \
            fprintf(stderr, "\n  " #b " = ");                       \
            fprint_num(stderr, _b);                                 \
            fprintf(stderr, "\n");                                  \
            abort();                                                \
        }                                                           \
    })

static void assert_strequal(const char *actual, const char *expected)
{
    if (strcmp(actual, expected) != 0) {
        fprintf(stderr, "Expected:\n%s\n", expected);
        fprintf(stderr, "Got:\n%s\n", actual);
        abort();
    }
}

static void assert_memequal(const uint8_t *actual, size_t actual_len,
                            const uint8_t *expected, size_t expected_len)
{
    bool fail = false;

    if (actual_len != expected_len) {
        fprintf(stderr, "Expected len = %zu, got %zu\n",
                expected_len, actual_len);
        fail = true;
    }
    if (memcmp(actual, expected, actual_len) != 0)
        fail = true;

    if (fail) {
        set_use_color(false);
        fprintf(stderr, "Expected:\n");
        print_hex(expected, expected_len, 0);
        fprintf(stderr, "Got:\n");
        print_hex(actual, actual_len, 0);
        abort();
    }
}

static void test_32bit_clean_pattern(void)
{
    _cleanup(pattern_free) struct pattern *pattern = NULL;

    static const uint32_t buf[] = {
        0x12345678, 0x12345678, 0x12345678, 0x12345678,
        0x12345678, 0x12345678, 0x12345678, 0x12345678,
    };

    pattern = pattern_find(buf, sizeof(buf), &(struct pattern_search_params){
        .size_min = 1,
        .size_max = 8,
        .size_step = 1,
        .off_min = 0,
        .off_max = 0,
        .off_step = 1,
    });
    assert_eq(pattern->len, 4);
}

static void test_32bit_varying_pattern(void)
{
    _cleanup(pattern_free) struct pattern *pattern = NULL;

    static const uint32_t buf[] = {
        0x12340678, 0x1234A678, 0x1234B678, 0x12342678,
        0x12343678, 0x12341678, 0x12347678, 0x12340678,
    };

    pattern = pattern_find(buf, sizeof(buf), &(struct pattern_search_params){
        .size_min = 1,
        .size_max = 8,
        .size_step = 1,
        .off_min = 0,
        .off_max = 0,
        .off_step = 1,
    });
    assert_eq(pattern->len, 4);
}

static void test_popcount(void)
{
    static const uint8_t buf1[] = {0x00, 0x00, 0x00, 0x00};
    assert_eq(popcount_buf8(buf1, sizeof(buf1)), 0);

    static const uint8_t buf2[] = {0x80, 0x00, 0x00, 0x00};
    assert_eq(popcount_buf8(buf2, sizeof(buf2)), 1);

    static const uint8_t buf3[] = {0x88, 0xff};
    assert_eq(popcount_buf8(buf3, sizeof(buf3)), 10);

    static const uint8_t buf4[] = {
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
    };
    assert_eq(popcount_buf(buf4, sizeof(buf4)), 2);
}

static void test_xor_bufs(void)
{
    static const uint8_t buf1[] = {
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    static const uint8_t buf2[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10
    };
    static const uint8_t acc_init[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    static const uint8_t expected_acc[] = {
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10
    };
    _autofree uint8_t *acc = NULL;
    size_t len = sizeof(expected_acc);

    acc = malloc(len);

    memcpy(acc, acc_init, len);
    accumulate_xor_bufs8(buf1, buf2, len, acc);
    assert_memequal(acc, len, expected_acc, len);

    memcpy(acc, acc_init, len);
    accumulate_xor_bufs(buf1, buf2, len, acc);
    assert_memequal(acc, len, expected_acc, len);
}

static void test_xor_bufs_unaligned(void)
{
    static const uint8_t buf1_init[] = {
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x0f
    };
    static const uint8_t buf2_init[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0a, 0x00, 0xf0
    };
    static const uint8_t acc_init[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    static const uint8_t expected_acc[] = {
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xaa, 0x00, 0xff
    };
    static const off_t off = 3;

    _autofree void *_buf1 = NULL;
    _autofree void *_buf2 = NULL;
    _autofree void *_acc = NULL;
    size_t len = sizeof(expected_acc);

    /* Allocate aligned buffers, so they can be easily misaligned */
    assert(posix_memalign(&_buf1, 8, 2 * len) == 0);
    assert(posix_memalign(&_buf2, 8, 2 * len) == 0);
    assert(posix_memalign(&_acc, 8, 2 * len) == 0);

    /* Make buffers misaligned */
    uint8_t *buf1 = _buf1 + off;
    uint8_t *buf2 = _buf2 + off;
    uint8_t *acc = _acc + off;

    memcpy(buf1, buf1_init, len);
    memcpy(buf2, buf2_init, len);
    memcpy(acc, acc_init, len);

    accumulate_xor_bufs(buf1, buf2, len, acc);
    assert_memequal(acc, len, expected_acc, len);
}

static void test_xor_bufs_unaligned_long(void)
{
    static const uint8_t buf1_init[] = {
                          0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xaa, 0x00, 0x00, 0x03, 0x07, 0x00,
        0x00, 0x00, 0xa0, 0x00, 0x0f
    };
    static const uint8_t buf2_init[] = {
                          0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x55, 0x00, 0x00, 0x03, 0x00, 0x00,
        0x00, 0x10, 0x0a, 0x00, 0xf0
    };
    static const uint8_t acc_init[] = {
                          0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00
    };
    static const uint8_t expected_acc[] = {
                          0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x07, 0x00,
        0x00, 0x10, 0xaa, 0x00, 0xff
    };
    static const off_t off = 3;

    _autofree void *_buf1 = NULL;
    _autofree void *_buf2 = NULL;
    _autofree void *_acc = NULL;
    size_t len = sizeof(expected_acc);

    /* Allocate aligned buffers, so they can be easily misaligned */
    assert(posix_memalign(&_buf1, 8, 2 * len) == 0);
    assert(posix_memalign(&_buf2, 8, 2 * len) == 0);
    assert(posix_memalign(&_acc, 8, 2 * len) == 0);

    /* Make buffers misaligned */
    uint8_t *buf1 = _buf1 + off;
    uint8_t *buf2 = _buf2 + off;
    uint8_t *acc = _acc + off;

    memcpy(buf1, buf1_init, len);
    memcpy(buf2, buf2_init, len);
    memcpy(acc, acc_init, len);

    accumulate_xor_bufs(buf1, buf2, len, acc);
    assert_memequal(acc, len, expected_acc, len);
}

static void test_short_generated_pattern(void)
{
    size_t buf_len = 4096;
    uint8_t fill_byte = 0x00;
    size_t header_len = 0;
    size_t pattern_len = 64;
    unsigned int blips_count = 8;
    _autofree void *buf = NULL;
    _cleanup(pattern_free) struct pattern *pattern = NULL;

    assert((buf = malloc(buf_len)));
    srand48(0);
    generate_pattern(buf, buf_len, fill_byte, header_len, pattern_len, blips_count);

    static const uint8_t expected_mask[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2e, 0x00, 0xfe,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbf, 0x00, 0x00, 0xf9,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x3d, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x37, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
    };
    static const uint8_t expected_pattern[] = {
        0x62, 0x46, 0x20, 0x24, 0x86, 0x00, 0xea, 0x2e, 0x0d, 0x8f, 0x44, 0x8d,
        0x91, 0x00, 0x19, 0x00, 0x30, 0x00, 0x00, 0x00, 0x8b, 0x75, 0xd7, 0x3a,
        0x00, 0xdc, 0x00, 0x00, 0x59, 0x00, 0x00, 0x3b, 0xde, 0xc5, 0x7a, 0x34,
        0x00, 0xb9, 0x69, 0x00, 0x00, 0x22, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xee, 0x00, 0x00, 0x4f, 0x1e, 0x00, 0x00, 0x00, 0x00,
        0xb8, 0x00, 0x00, 0x66,
    };
    const off_t expected_off = 0x44;

    /* Check that generated pattern is okay first */
    assert_memequal(buf + expected_off, pattern_len,
                    expected_pattern, pattern_len);

    pattern = pattern_find(buf, buf_len, &(struct pattern_search_params){
        .size_min = 4,
        .size_max = 128,
        .size_step = 4,
        .off_min = 0,
        .off_max = 128,
        .off_step = 4,
    });

    assert_memequal(pattern->mask + pattern->off, pattern->len,
                    expected_mask, pattern_len);
    assert_eq(popcount_buf8(pattern->mask, pattern->len), 26);

    assert_memequal(buf + pattern->off, pattern->len,
                    expected_pattern, pattern_len);

    assert_eq(pattern->off, expected_off);
}

static void test_long_generated_pattern(void)
{
    size_t buf_len = 4096;
    uint8_t fill_byte = 0x00;
    size_t header_len = 14;
    size_t pattern_len = 129;
    unsigned int blips_count = 12;
    _autofree void *buf = NULL;
    _cleanup(pattern_free) struct pattern *pattern = NULL;

    assert((buf = malloc(buf_len)));
    srand48(0);
    generate_pattern(buf, buf_len, fill_byte, header_len, pattern_len, blips_count);

    pattern = pattern_find(buf, buf_len, &(struct pattern_search_params){
        .size_min = 16,
        .size_max = 200,
        .size_step = 1,
        .off_min = 0,
        .off_max = 100,
        .off_step = 1,
    });
    assert(pattern->len == pattern_len);
    assert_eq(pattern->off, 0x51);
    assert_eq(popcount_buf8(pattern->mask, pattern->len), 48);
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
    RUN(test_output_aligned);
    RUN(test_output_unaligned_multiline);
    RUN(test_output_unaligned_singleline);
    RUN(test_popcount);
    RUN(test_xor_bufs);
    RUN(test_xor_bufs_unaligned);
    RUN(test_xor_bufs_unaligned_long);
    RUN(test_32bit_clean_pattern);
    RUN(test_32bit_varying_pattern);
    RUN(test_short_generated_pattern);
    RUN(test_long_generated_pattern);
    return has_failures;
}
