#include "hex.h"
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>

static bool use_color;
static unsigned char empty_byte;
static const size_t bytes_per_line = 16;

#define CL_NULL             "\x1b[90m"
#define CL_OFFSET           "\x1b[90m"
#define CL_ASCII_PRINTABLE  "\x1b[36m"
#define CL_ASCII_WHITESPACE "\x1b[32m"
#define CL_ASCII_OTHER      "\x1b[32m"
#define CL_NONASCII         "\x1b[33m"
#define CL_MASKED           "\x1b[31m"
#define CL_RESET            "\x1b[39m"

/*
 * Categorization and some other code around is inspired by
 * https://github.com/sharkdp/hexyl.git
 */
enum ByteCategory {
    Null,
    AsciiPrintable,
    AsciiWhitespace,
    AsciiOther,
    NonAscii,
    Masked,
};

static const char ascii_octet[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

static enum ByteCategory byte_category(unsigned char byte)
{
    if (byte == empty_byte) {
        return Null;
    } else if (isgraph(byte)) {
        return AsciiPrintable;
    } else if (isspace(byte)) {
        return AsciiWhitespace;
    } else if (isascii(byte)) {
        return AsciiOther;
    } else {
        return NonAscii;
    }
}

static const char *color(enum ByteCategory category)
{
    if (!use_color)
        return "";

    switch (category) {
    case Null:
        return CL_NULL;
    case AsciiPrintable:
        return CL_ASCII_PRINTABLE;
    case AsciiWhitespace:
        return CL_ASCII_WHITESPACE;
    case AsciiOther:
        return CL_ASCII_OTHER;
    case NonAscii:
        return CL_NONASCII;
    case Masked:
        return CL_MASKED;
    default:
        return "";
    }
}

static void print_byte(unsigned char byte)
{
    printf("%s%02x", color(byte_category(byte)), byte);
}

static void print_char_with_mask(unsigned char byte, unsigned char mask)
{
    enum ByteCategory category;

    if (mask)
        category = Masked;
    else
        category = byte_category(byte);

    printf("%s", color(category));

    switch (category) {
    case Null:
        printf("⋄");
        break;
    case AsciiPrintable:
        printf("%c", byte);
        break;
    case AsciiWhitespace:
        printf("%c", byte == 0x20 ? ' ' : '_');
        break;
    case AsciiOther:
        printf("•");
        break;
    case NonAscii:
    case Masked:
        printf("×");
        break;
    }
}

static void print_byte_with_mask(unsigned char byte, unsigned char mask)
{
    if (!mask) {
        print_byte(byte);
        return;
    }

    if (mask & 0xf0) {
        printf("%s", color(Masked));
        putchar('X');
    } else {
        printf("%s", color(NonAscii));
        putchar(ascii_octet[(byte >> 4) & 0xf]);
    }

    if (mask & 0x0f) {
        printf("%s", color(Masked));
        putchar('X');
    } else {
        printf("%s", color(NonAscii));
        putchar(ascii_octet[byte & 0xf]);
    }
}

static void print_hex_line_with_mask(const unsigned char *buf,
                                     const unsigned char *mask,
                                     size_t off, size_t len)
{
    const unsigned char *saved_buf = buf;
    const unsigned char *saved_mask = mask;
    size_t saved_len = len;
    size_t delim_pos = bytes_per_line / 2 - 1;

    /* TODO: wrap `use_color ? X : ""` into something */
    printf("│%s%08zx%s│", use_color ? CL_OFFSET : "", off,
           use_color ? CL_RESET : "");

    while (len--) {
        if (len == delim_pos)
            printf(" %s┊", use_color ? CL_RESET : "");
        printf(" ");
        print_byte_with_mask(*buf++, *mask++);
    }

    printf("%s │", use_color ? CL_RESET : "");

    buf = saved_buf;
    mask = saved_mask;
    len = saved_len;

    /* FIXME: this will not print correctly if len != bytes_per_line */
    while (len--) {
        if (len == delim_pos)
            printf("%s┊", use_color ? CL_RESET : "");
        print_char_with_mask(*buf++, *mask++);
    }

    printf("%s│\n", use_color ? CL_RESET : "");
}

void set_use_color(bool use)
{
    use_color = use;
}

void set_empty_byte(unsigned char byte)
{
    empty_byte = byte;
}

void print_hex(const unsigned char *buf, size_t len, size_t addr)
{
    print_hex_with_mask(buf, NULL, len, addr);
}

void print_hex_with_mask(const unsigned char *buf,
                         const unsigned char *mask,
                         size_t len, size_t addr)
{
    /* TODO: print header and footer */
    for (size_t off = 0; off < len; off += bytes_per_line) {
        size_t bytes = len - off >= bytes_per_line ? bytes_per_line : len - off;
        print_hex_line_with_mask(buf + off, mask + off, addr + off, bytes);
    }
}
