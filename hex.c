#include "hex.h"
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>

static bool use_color;
static unsigned char empty_byte;
static const unsigned int columns = 2;
static enum BorderStyle border_style = BorderStyleUnicode;

#define CL_NULL             "\x1b[90m"
#define CL_OFFSET           "\x1b[90m"
#define CL_ASCII_PRINTABLE  "\x1b[36m"
#define CL_ASCII_WHITESPACE "\x1b[32m"
#define CL_ASCII_OTHER      "\x1b[32m"
#define CL_NONASCII         "\x1b[33m"
#define CL_MASKED           "\x1b[31m"
#define CL_RESET            "\x1b[39m"

static const char *cl_offset = "";
static const char *cl_reset = "";

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

enum BorderElement {
    LeftCorner,
    HorizontalLine,
    ColumnSeparator,
    RightCorner,
};

static const char ascii_octet[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

static const char *header_element(enum BorderElement e)
{
    switch (border_style) {
    case BorderStyleUnicode:
        switch (e) {
        case LeftCorner:
            return "┌";
        case HorizontalLine:
            return "─";
        case ColumnSeparator:
            return "┬";
        case RightCorner:
            return "┐";
        default:
            abort();
        }
    case BorderStyleAscii:
        switch (e) {
        case HorizontalLine:
            return "-";
        case LeftCorner:
        case ColumnSeparator:
        case RightCorner:
            return "+";
        default:
            abort();
        }
    case BorderStyleNone:
    default:
        return "";
    }
}

static const char *footer_element(enum BorderElement e)
{
    switch (border_style) {
    case BorderStyleUnicode:
        switch (e) {
        case LeftCorner:
            return "└";
        case HorizontalLine:
            return "─";
        case ColumnSeparator:
            return "┴";
        case RightCorner:
            return "┘";
        default:
            abort();
        }
    case BorderStyleAscii:
        switch (e) {
        case HorizontalLine:
            return "-";
        case LeftCorner:
        case ColumnSeparator:
        case RightCorner:
            return "+";
        default:
            abort();
        }
    case BorderStyleNone:
    default:
        return "";
    }
}

static const char *outer_sep(void)
{
    switch (border_style) {
    case BorderStyleUnicode:
        return "│";
    case BorderStyleAscii:
        return "|";
    case BorderStyleNone:
    default:
        return " ";
    }
}

static const char *inner_sep(void)
{
    switch (border_style) {
    case BorderStyleUnicode:
        return "┊";
    case BorderStyleAscii:
        return "|";
    case BorderStyleNone:
    default:
        return " ";
    }
}

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

    printf("%s%s%08zx%s%s", outer_sep(), cl_offset, off, cl_reset, outer_sep());

    while (len) {
        if (len % 8 == 0 && len != saved_len && len != 0)
            printf(" %s%s", cl_reset, inner_sep());
        printf(" ");
        print_byte_with_mask(*buf++, *mask++);
        len--;
    }

    printf("%s %s", cl_reset, outer_sep());

    buf = saved_buf;
    mask = saved_mask;
    len = saved_len;

    /* FIXME: this will not print correctly if len != bytes_per_line */
    while (len) {
        if (len % 8 == 0 && len != saved_len && len != 0)
            printf("%s%s", cl_reset, inner_sep());
        print_char_with_mask(*buf++, *mask++);
        len--;
    }

    printf("%s%s\n", cl_reset, outer_sep());
}

static void print_hf(const char *(element)(enum BorderElement e))
{
    printf("%s", element(LeftCorner));

    /* Address is fixed to 8 octets */
    for (int i = 0; i < 8; i++)
        printf("%s", element(HorizontalLine));

    printf("%s", element(ColumnSeparator));

    /* Columns with data */
    for (int i = 0; i < columns; i++) {
        for (int j = 0; j < 3 * 8 + 1; j++) {
            printf("%s", element(HorizontalLine));
        }
        printf("%s", element(ColumnSeparator));
    }

    /* Columns with ascii */
    for (int i = 0; i < columns; i++) {
        for (int j = 0; j < 8; j++) {
            printf("%s", element(HorizontalLine));
        }
        /* Don't print last separator -- right corner goes there */
        if (i != columns - 1)
            printf("%s", element(ColumnSeparator));
    }

    printf("%s\n", element(RightCorner));
}

static void print_header(void)
{
    print_hf(header_element);
}

static void print_footer(void)
{
    print_hf(footer_element);
}

void set_use_color(bool use)
{
    use_color = use;
}

void set_border_style(enum BorderStyle style)
{
    border_style = style;
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
    const size_t bytes_per_line = columns * 8;

    cl_offset = use_color ? CL_OFFSET : "";
    cl_reset = use_color ? CL_RESET : "";

    print_header();

    /* TODO: always align to 16 bytes (blanks in the beginning/end) */
    for (size_t off = 0; off < len; off += bytes_per_line) {
        size_t bytes = len - off >= bytes_per_line ? bytes_per_line : len - off;
        print_hex_line_with_mask(buf + off, mask + off, addr + off, bytes);
    }

    print_footer();
}
