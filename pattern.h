#pragma once

#include <stddef.h>
#include <sys/types.h>
#include "common.h"

struct pattern {
    size_t len;
    off_t off;      // global offset within data
    void *buf;      // underlying mask buffer
    void *mask;     // mask with varying bits
};

struct pattern_search_params {
    size_t size_min;
    size_t size_max;
    size_t size_step;
    off_t off_min;
    off_t off_max;
    off_t off_step;
};

struct pattern *pattern_find(const void *data,
                             size_t data_size,
                             struct pattern_search_params *p);

void pattern_free(struct pattern *pattern);

DEFINE_PTR_CLEANUP_FUNC(struct pattern *, pattern_free)
