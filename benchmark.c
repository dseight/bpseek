#define _GNU_SOURCE
#include "generator.h"
#include "pattern.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE        0x80000
#define PATTERN_LEN     1972

static void print_ts_diff(const char *title,
                          struct timespec *ts_start,
                          struct timespec *ts_stop)
{
    struct timeval start_val;
    struct timeval stop_val;
    struct timeval result;
    double elapsed;

    TIMESPEC_TO_TIMEVAL(&start_val, ts_start);
    TIMESPEC_TO_TIMEVAL(&stop_val, ts_stop);
    timersub(&stop_val, &start_val, &result);

    elapsed = result.tv_sec + (double) result.tv_usec / 1000000.0;

    printf("%s took: %.6fs\n", title, elapsed);
}

int main(void)
{
    struct timespec ts_start, ts_stop;
    _autofree uint8_t *buf = NULL;
    _cleanup(free_pattern) struct pattern *pattern = NULL;

    buf = malloc(BUF_SIZE);
    srand48(13);

    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    generate_pattern(buf, BUF_SIZE, 0x00, 1024, PATTERN_LEN, 64);
    clock_gettime(CLOCK_MONOTONIC, &ts_stop);
    print_ts_diff("Pattern generation", &ts_start, &ts_stop);

    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    pattern = find_pattern(buf, BUF_SIZE, 64, 2048, 1, 0, 0x1000, 8);
    clock_gettime(CLOCK_MONOTONIC, &ts_stop);
    print_ts_diff("Pattern search", &ts_start, &ts_stop);

    if (pattern->len != PATTERN_LEN) {
        fprintf(stderr, "Found len doesn't match (expected %u, found %zu)\n",
                PATTERN_LEN, pattern->len);
        exit(1);
    }

    exit(0);
}
