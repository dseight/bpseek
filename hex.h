#pragma once

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

enum Style {
    /* Use Unicode characters in character panel and for drawing a border. */
    StyleUnicode,
    /* Use only ASCII characters in character panel and for drawing a border. */
    StyleAscii,
    /* Do not draw a border at all. ASCII characters in character panel. */
    StyleNone,
};

void set_use_color(bool use);

void set_style(enum Style style);

/*
 * Override what is considered "empty". By default it is 0x00.
 * But this is not the case for NOR and NAND memory (empty is 0xFF).
 */
void set_empty_byte(unsigned char byte);

void fprint_hex(FILE *out, const unsigned char *buf, size_t len, size_t addr);

void print_hex(const unsigned char *buf, size_t len, size_t addr);

void fprint_hex_with_mask(FILE *out, const unsigned char *buf,
                          const unsigned char *mask,
                          size_t len, size_t addr);

/* As print_hex(), but octets with mask on them shown as "X" */
void print_hex_with_mask(const unsigned char *buf,
                         const unsigned char *mask,
                         size_t len, size_t addr);
