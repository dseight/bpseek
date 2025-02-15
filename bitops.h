#pragma once

#include "common.h"
#include <stddef.h>
#include <stdint.h>

_maybe_unused
static void accumulate_xor_bufs8(const uint8_t *buf1, const uint8_t *buf2,
                                 size_t len, uint8_t *acc)
{
    for (int i = 0; i < len; i++)
        acc[i] |= buf1[i] ^ buf2[i];
}

_maybe_unused
static void accumulate_xor_bufs16(const uint16_t *buf1, const uint16_t *buf2,
                                  size_t len, uint16_t *acc)
{
    for (int i = 0; i < len / 2; i++)
        acc[i] |= buf1[i] ^ buf2[i];
}

_maybe_unused
static void accumulate_xor_bufs32(const uint32_t *buf1, const uint32_t *buf2,
                                  size_t len, uint32_t *acc)
{
    for (int i = 0; i < len / 4; i++)
        acc[i] |= buf1[i] ^ buf2[i];
}

_maybe_unused
static void accumulate_xor_bufs64(const uint64_t *buf1, const uint64_t *buf2,
                                  size_t len, uint64_t *acc)
{
    for (int i = 0; i < len / 8; i++)
        acc[i] |= buf1[i] ^ buf2[i];
}

_maybe_unused
static uint64_t popcount_buf8(const uint8_t *buf, size_t len)
{
    uint64_t ret = 0;
    for (int i = 0; i < len; i++)
        ret += __builtin_popcount(buf[i]);
    return ret;
}

_maybe_unused
static uint64_t popcount_buf16(const uint16_t *buf, size_t len)
{
    uint64_t ret = 0;
    for (int i = 0; i < len / 2; i++)
        ret += __builtin_popcount(buf[i]);
    return ret;
}

_maybe_unused
static uint64_t popcount_buf32(const uint32_t *buf, size_t len)
{
    uint64_t ret = 0;
    for (int i = 0; i < len / 4; i++)
        ret += __builtin_popcount(buf[i]);
    return ret;
}

_maybe_unused
static uint64_t popcount_buf64(const uint64_t *buf, size_t len)
{
    uint64_t ret = 0;
    for (int i = 0; i < len / 8; i++)
        ret += __builtin_popcountl(buf[i]);
    return ret;
}
