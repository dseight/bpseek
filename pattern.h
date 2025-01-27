#pragma once

#include <stddef.h>
#include <sys/types.h>
#include "common.h"

struct pattern {
    size_t len;
    off_t off;      // global offset within data
    void *mask;
    void *xor_buf;
};

struct pattern *find_pattern(const void *data, size_t data_size,
                             size_t size_min, size_t size_max, size_t size_step,
                             off_t off_min, off_t off_max, off_t off_step);

void free_pattern(struct pattern *pattern);

DEFINE_PTR_CLEANUP_FUNC(struct pattern *, free_pattern)
