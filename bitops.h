#pragma once

#include "common.h"
#include <stddef.h>
#include <stdint.h>

static void accumulate_xor_bufs8(const void *buf1,
                                 const void *buf2,
                                 size_t len,
                                 void *acc)
{
    const uint8_t *_buf1 = buf1;
    const uint8_t *_buf2 = buf2;
    uint8_t *_acc = acc;

    for (int i = 0; i < len; i++)
        _acc[i] |= _buf1[i] ^ _buf2[i];
}

static void accumulate_xor_bufs64(const void *buf1,
                                  const void *buf2,
                                  size_t len,
                                  void *acc)
{
    const size_t align_bytes = 8;
    const uintptr_t alignment = align_bytes - (uintptr_t)buf1 % align_bytes;
    const size_t len_header = len < alignment ? len : alignment;
    const size_t len_trailer = (len - len_header) % align_bytes;
    const size_t len_mid = len - len_header - len_trailer;
    const uint64_t *_buf1 = buf1 + len_header;
    const uint64_t *_buf2 = buf2 + len_header;
    uint64_t *_acc = acc + len_header;

    accumulate_xor_bufs8(buf1, buf2, len_header, acc);

    for (int i = 0; i < len_mid / align_bytes; i++)
        _acc[i] |= _buf1[i] ^ _buf2[i];

    accumulate_xor_bufs8(buf1 + len_header + len_mid,
                         buf2 + len_header + len_mid,
                         len_trailer,
                         acc + len_header + len_mid);
}

_maybe_unused
static void accumulate_xor_bufs(const void *buf1,
                                const void *buf2,
                                size_t len, void *acc)
{
    uintptr_t buf1_alignment = (uintptr_t)buf1 % 8;
    uintptr_t buf2_alignment = (uintptr_t)buf2 % 8;

    if (buf1_alignment != buf2_alignment)
        accumulate_xor_bufs8(buf1, buf2, len, acc);
    else
        accumulate_xor_bufs64(buf1, buf2, len, acc);
}

static uint64_t popcount_buf8(const uint8_t *buf, size_t len)
{
    uint64_t ret = 0;
    for (int i = 0; i < len; i++)
        ret += __builtin_popcount(buf[i]);
    return ret;
}

_maybe_unused
static uint64_t popcount_buf(const void *buf, size_t len)
{
    const size_t align_bytes = 8;
    const uintptr_t alignment = align_bytes - (uintptr_t)buf % align_bytes;
    const size_t len_header = len < alignment ? len : alignment;
    const size_t len_trailer = (len - len_header) % align_bytes;
    const size_t len_mid = len - len_header - len_trailer;
    const uint64_t *_buf = buf + len_header;
    uint64_t ret = 0;

    ret += popcount_buf8(buf, len_header);

    for (int i = 0; i < len_mid / align_bytes; i++)
        ret += __builtin_popcountl(_buf[i]);

    ret += popcount_buf8(buf + len_header + len_mid, len_trailer);

    return ret;
}
