#pragma once

#include <stddef.h>
#include <stdbool.h>

void set_use_color(bool use);

/*
 * Override what is considered "empty". By default it is 0x00.
 * But this is not the case for NOR and NAND memory (empty is 0xFF).
 */
void set_empty_byte(unsigned char byte);

void print_hex(const unsigned char *buf, size_t len, size_t addr);

/* As print_hex(), but octets with mask on them shown as "X" */
void print_hex_with_mask(const unsigned char *buf,
                         const unsigned char *mask,
                         size_t len, size_t addr);
