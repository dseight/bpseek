#include "pattern.h"
#include "bitops.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

#include <stdio.h>
#include <inttypes.h>

static size_t cache_line_size(void)
{
    static size_t line_size;

    /* Return saved value */
    if (line_size)
        return line_size;

#if defined(__APPLE__)
    size_t sizeof_line_size = sizeof(line_size);
    sysctlbyname("hw.cachelinesize", &line_size, &sizeof_line_size, 0, 0);
#else
    line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
#endif

    return line_size;
}

static struct pattern *pattern_new(size_t max_size)
{
    _cleanup(pattern_free) struct pattern *pattern = NULL;
    /*
     * Allocate 2 extra cache lines around the required buffer,
     * so the mask alignment can match the data alignment.
     */
    size_t size = max_size + 2 * cache_line_size();

    pattern = calloc(1, sizeof(*pattern));
    if (!pattern)
        return NULL;

    pattern->buf = malloc(size);
    if (!pattern->buf)
        return NULL;

    return _steal_ptr(pattern);
}

void pattern_free(struct pattern *pattern)
{
    free(pattern->buf);
    free(pattern);
}

/**
 * Calculate ratio of varying bits to the size of the potential pattern.
 *
 * @param data: data to calculate ratio on
 * @param data_size: size of the data
 * @param pattern_size: size of the potential pattern
 * @param mask: where to store the "varying bits" mask, must have the
 *      same alignment as the data pointer.
 */
static double calc_ratio(const void *data,
                         size_t data_size,
                         size_t pattern_size,
                         void *mask)
{
    size_t chunks = data_size / pattern_size;
    uint64_t pattern_bits = pattern_size * 8;
    uint64_t varying_bits;

    memset(mask, 0, pattern_size);

    for (int i = 0; i < chunks - 1; i++)
        accumulate_xor_bufs(data, data + pattern_size, pattern_size, mask);

    varying_bits = popcount_buf(mask, pattern_size);

    return (double)varying_bits / (double)pattern_bits;
}

struct pattern *pattern_find(const void *data,
                             size_t data_size,
                             struct pattern_search_params *p)
{
    assert(p != NULL);
    assert(p->size_min <= data_size);
    assert(p->size_max <= data_size);
    /* step can only be 0 if min and max are the same */
    assert(p->size_step > 0 || p->size_min == p->size_max);
    assert(p->off_min >= 0);
    assert(p->off_max >= 0);
    assert(p->off_min < data_size);
    assert(p->off_max < data_size);
    assert(p->off_step >= 0);
    assert(p->off_step > 0 || p->off_min == p->off_max);

    uintptr_t data_addr;
    off_t data_align;
    double min_ratio = 1.0;
    struct pattern *pattern = pattern_new(p->size_max);

    for (off_t off = p->off_min; off <= p->off_max; off += p->off_step) {
        data_addr = (uintptr_t)(data + off);
        data_align = data_addr & (cache_line_size() - 1);
        /* Make mask ptr misaligned the same way as the data */
        pattern->mask = pattern->buf + data_align;

        for (size_t size = p->size_min; size <= p->size_max; size += p->size_step) {
            double ratio;

            ratio = calc_ratio(data + off, data_size - off,
                               size, pattern->mask);

            // printf("ratio(%zu, %zu): %f\n", (size_t)off, size, ratio);
            if (ratio < min_ratio) {
                min_ratio = ratio;
                pattern->off = off;
                pattern->len = size;
            }
        }
    }

    data_addr = (uintptr_t)(data + pattern->off);
    data_align = data_addr & (cache_line_size() - 1);
    /* Make mask ptr misaligned the same way as the data */
    pattern->mask = pattern->buf + data_align;

    /* recalculate mask for the best match */
    calc_ratio(data + pattern->off, data_size - pattern->off,
               pattern->len, pattern->mask);

    return pattern;
}
