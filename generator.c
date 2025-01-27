#include "generator.h"
#include <stdlib.h>
#include <string.h>

void generate_pattern(uint8_t *buf,
                      size_t buf_len,
                      uint8_t fill_byte,
                      size_t header_len,
                      size_t pattern_len,
                      unsigned int blips_count)
{
    unsigned char *p;
    unsigned char *pattern;
    unsigned int *blips_positions;
    size_t pattern_bytes = lrand48() % pattern_len;

    /* TODO: check all input values for validity */

    /* 1. Fill pattern */
    memset(buf, fill_byte, buf_len);

    /* 2. Write header */
    p = buf;
    for (int i = 0; i < header_len; i++)
        *p++ = lrand48() & 0xff;

    /* 3. Generate pattern */
    pattern = p;
    for (int i = 0; i < pattern_bytes; i++)
        pattern[lrand48() % pattern_len] = lrand48() & 0xff;

    /* 4. Spread the pattern over the whole buffer */
    p = pattern + pattern_len;
    while (p + pattern_len < buf + buf_len) {
        memcpy(p, pattern, pattern_len);
        p += pattern_len;
    }

    /* 5. Insert blips */
    blips_positions = calloc(blips_count, sizeof(*blips_positions));
    /* Deliberately ignore allocation failures. Just crash. */

    for (int i = 0; i < blips_count; i++)
        blips_positions[i] = lrand48() % pattern_len;

    p = pattern;
    while (p + pattern_len < buf + buf_len) {
        for (int i = 0; i < blips_count; i++) {
            p[blips_positions[i]] = lrand48();
        }
        p += pattern_len;
    }

    /* FIXME: the last part of the file is not filled in */

    free(blips_positions);
}
