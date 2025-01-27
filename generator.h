#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Generate a pattern over provided buffer.
 *
 * To vary the pattern, call srand48().
 *
 * @param buf: buffer to write pattern into
 * @param buf_len: size of the buffer
 * @param fill_byte: what to fill the "empty" data with (e.g., 0x00 or 0xff).
 * @param header_len: if not 0, then a random header of provided size
 *      will be added before the beginning of the pattern.
 * @param pattern_len: length of the pattern that will be scattered across
 *      the buffer.
 * @param blips_count: amount of random blips in a pattern.
 *      These blips will be located in the same place for each
 *      pattern occurence, but will contain random data.
 */
void generate_pattern(uint8_t *buf,
                      size_t buf_len,
                      uint8_t fill_byte,
                      size_t header_len,
                      size_t pattern_len,
                      unsigned int blips_count);
