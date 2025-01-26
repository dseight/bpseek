#pragma once

#include <stddef.h>
#include <stdint.h>

/*
 * TODO: combine xor & or operations into a single function to improve
 * cache locality.
 */

static void xor_bufs(const uint8_t *buf1, const uint8_t *buf2,
                     size_t len, uint8_t *dst)
{
    for (int i = 0; i < len; i++)
        dst[i] = buf1[i] ^ buf2[i];
}

static void xor_bufs16(const uint16_t *buf1, const uint16_t *buf2,
                       size_t len, uint16_t *dst)
{
    for (int i = 0; i < len / 2; i++)
        dst[i] = buf1[i] ^ buf2[i];
}

static void xor_bufs32(const uint32_t *buf1, const uint32_t *buf2,
                       size_t len, uint32_t *dst)
{
    for (int i = 0; i < len / 4; i++)
        dst[i] = buf1[i] ^ buf2[i];
}

static void xor_bufs64(const uint64_t *buf1, const uint64_t *buf2,
                       size_t len, uint64_t *dst)
{
    for (int i = 0; i < len / 8; i++)
        dst[i] = buf1[i] ^ buf2[i];
}


/* OR data in dts and src buffers and put it to dst. */
static void or_bufs(uint8_t *dst, const uint8_t *src, size_t len)
{
    for (int i = 0; i < len; i++)
        dst[i] |= src[i];
}

static void or_bufs16(uint16_t *dst, const uint16_t *src, size_t len)
{
    for (int i = 0; i < len / 2; i++)
        dst[i] |= src[i];
}

static void or_bufs32(uint32_t *dst, const uint32_t *src, size_t len)
{
    for (int i = 0; i < len / 4; i++)
        dst[i] |= src[i];
}

static void or_bufs64(uint64_t *dst, const uint64_t *src, size_t len)
{
    for (int i = 0; i < len / 8; i++)
        dst[i] |= src[i];
}


static uint64_t popcount_buf(uint8_t *buf, size_t len)
{
    uint64_t ret = 0;
    for (int i = 0; i < len; i++)
        ret += __builtin_popcount(buf[i]);
    return ret;
}

static uint64_t popcount_buf16(uint16_t *buf, size_t len)
{
    uint64_t ret = 0;
    for (int i = 0; i < len / 2; i++)
        ret += __builtin_popcount(buf[i]);
    return ret;
}

static uint64_t popcount_buf32(uint32_t *buf, size_t len)
{
    uint64_t ret = 0;
    for (int i = 0; i < len / 4; i++)
        ret += __builtin_popcount(buf[i]);
    return ret;
}

static uint64_t popcount_buf64(uint64_t *buf, size_t len)
{
    uint64_t ret = 0;
    for (int i = 0; i < len / 8; i++)
        ret += __builtin_popcountl(buf[i]);
    return ret;
}
