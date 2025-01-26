#pragma once

#include <stddef.h>
#include "common.h"

struct pattern {
    size_t len;
    void *mask;
    void *xor_buf;
};

struct pattern *find_pattern(const void *data, size_t data_size,
                             size_t min_size, size_t max_size, size_t size_step);

void free_pattern(struct pattern *pattern);

DEFINE_PTR_CLEANUP_FUNC(struct pattern *, free_pattern)
