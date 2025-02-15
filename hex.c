#include "hex.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>

static bool use_color;
static unsigned char empty_byte;
static const unsigned int columns = 2;
static enum Style style = StyleUnicode;

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
    switch (style) {
    case StyleUnicode:
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
    case StyleAscii:
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
    case StyleNone:
    default:
        return "";
    }
}

static const char *footer_element(enum BorderElement e)
{
    switch (style) {
    case StyleUnicode:
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
    case StyleAscii:
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
    case StyleNone:
    default:
        return "";
    }
}

static const char *outer_sep(void)
{
    switch (style) {
    case StyleUnicode:
        return "│";
    case StyleAscii:
        return "|";
    case StyleNone:
    default:
        return " ";
    }
}

static const char *inner_sep(void)
{
    switch (style) {
    case StyleUnicode:
        return "┊";
    case StyleAscii:
        return "|";
    case StyleNone:
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

static void print_byte(FILE *f, unsigned char byte)
{
    fprintf(f, "%s%02x", color(byte_category(byte)), byte);
}

static void print_char_with_mask(FILE *f, unsigned char byte, unsigned char mask)
{
    enum ByteCategory category;

    if (mask)
        category = Masked;
    else
        category = byte_category(byte);

    fprintf(f, "%s", color(category));

    switch (category) {
    case Null:
        fprintf(f, style == StyleUnicode ? "⋄" : ".");
        break;
    case AsciiPrintable:
        fprintf(f, "%c", byte);
        break;
    case AsciiWhitespace:
        fprintf(f, "%c", byte == 0x20 ? ' ' : '_');
        break;
    case AsciiOther:
        fprintf(f, style == StyleUnicode ? "•" : ".");
        break;
    case NonAscii:
    case Masked:
        fprintf(f, style == StyleUnicode ? "×" : "x");
        break;
    }
}

static void print_byte_with_mask(FILE *f, unsigned char byte, unsigned char mask)
{
    if (!mask) {
        print_byte(f, byte);
        return;
    }

    if (mask & 0xf0) {
        fprintf(f, "%s", color(Masked));
        fputc('X', f);
    } else {
        fprintf(f, "%s", color(NonAscii));
        fputc(ascii_octet[(byte >> 4) & 0xf], f);
    }

    if (mask & 0x0f) {
        fprintf(f, "%s", color(Masked));
        fputc('X', f);
    } else {
        fprintf(f, "%s", color(NonAscii));
        fputc(ascii_octet[byte & 0xf], f);
    }
}

static void print_hex_part(FILE *f,
                           const unsigned char *buf,
                           const unsigned char *mask,
                           size_t off,
                           size_t len,
                           unsigned int blanks_start,
                           unsigned int blanks_end)
{
    size_t byte_idx = 0;

    while (blanks_start--) {
        if (byte_idx % 8 == 0 && byte_idx != len && byte_idx != 0)
            fprintf(f, " %s%s", cl_reset, inner_sep());
        fprintf(f, "   ");
        byte_idx++;
    }

    while (len--) {
        if (byte_idx % 8 == 0 && byte_idx != len && byte_idx != 0)
            fprintf(f, " %s%s", cl_reset, inner_sep());
        fprintf(f, " ");
        print_byte_with_mask(f, *buf++, mask ? *mask++ : 0);
        byte_idx++;
    }

    while (blanks_end--) {
        if (byte_idx % 8 == 0 && byte_idx != len && byte_idx != 0)
            fprintf(f, " %s%s", cl_reset, inner_sep());
        fprintf(f, "   ");
        byte_idx++;
    }
}

static void print_ascii_part(FILE *f,
                             const unsigned char *buf,
                             const unsigned char *mask,
                             size_t off,
                             size_t len,
                             unsigned int blanks_start,
                             unsigned int blanks_end)
{
    size_t byte_idx = 0;

    while (blanks_start--) {
        if (byte_idx % 8 == 0 && byte_idx != len && byte_idx != 0)
            fprintf(f, "%s%s", cl_reset, inner_sep());
        fprintf(f, " ");
        byte_idx++;
    }

    while (len--) {
        if (byte_idx % 8 == 0 && byte_idx != len && byte_idx != 0)
            fprintf(f, "%s%s", cl_reset, inner_sep());
        print_char_with_mask(f, *buf++, mask ? *mask++ : 0);
        byte_idx++;
    }

    while (blanks_end--) {
        if (byte_idx % 8 == 0 && byte_idx != len && byte_idx != 0)
            fprintf(f, "%s%s", cl_reset, inner_sep());
        fprintf(f, " ");
        byte_idx++;
    }
}

static void print_line_with_mask(FILE *f,
                                 const unsigned char *buf,
                                 const unsigned char *mask,
                                 size_t off,
                                 size_t len,
                                 unsigned int blanks_start,
                                 unsigned int blanks_end)
{
    fprintf(f, "%s%s%08zx%s%s", outer_sep(), cl_offset, off, cl_reset, outer_sep());
    print_hex_part(f, buf, mask, off, len, blanks_start, blanks_end);
    fprintf(f, "%s %s", cl_reset, outer_sep());
    print_ascii_part(f, buf, mask, off, len, blanks_start, blanks_end);
    fprintf(f, "%s%s\n", cl_reset, outer_sep());
}

static void print_hf(FILE *f, const char *(element)(enum BorderElement e))
{
    fprintf(f, "%s", element(LeftCorner));

    /* Address is fixed to 8 octets */
    for (int i = 0; i < 8; i++)
        fprintf(f, "%s", element(HorizontalLine));

    fprintf(f, "%s", element(ColumnSeparator));

    /* Columns with data */
    for (int i = 0; i < columns; i++) {
        for (int j = 0; j < 3 * 8 + 1; j++) {
            fprintf(f, "%s", element(HorizontalLine));
        }
        fprintf(f, "%s", element(ColumnSeparator));
    }

    /* Columns with ascii */
    for (int i = 0; i < columns; i++) {
        for (int j = 0; j < 8; j++) {
            fprintf(f, "%s", element(HorizontalLine));
        }
        /* Don't print last separator -- right corner goes there */
        if (i != columns - 1)
            fprintf(f, "%s", element(ColumnSeparator));
    }

    fprintf(f, "%s\n", element(RightCorner));
}

static void print_header(FILE *f)
{
    if (style != StyleNone)
        print_hf(f, header_element);
}

static void print_footer(FILE *f)
{
    if (style != StyleNone)
        print_hf(f, footer_element);
}

void set_use_color(bool use)
{
    use_color = use;
}

void set_style(enum Style _style)
{
    style = _style;
}

void set_empty_byte(unsigned char byte)
{
    empty_byte = byte;
}

void fprint_hex(FILE *out, const unsigned char *buf, size_t len, size_t addr)
{
    fprint_hex_with_mask(out, buf, NULL, len, addr);
}

void print_hex(const unsigned char *buf, size_t len, size_t addr)
{
    print_hex_with_mask(buf, NULL, len, addr);
}

void fprint_hex_with_mask(FILE *out, const unsigned char *buf,
                          const unsigned char *mask,
                          size_t len, size_t addr)
{
    size_t off = 0;
    const size_t bytes_per_line = columns * 8;
    size_t aligned_addr = rounddown(addr, bytes_per_line);
    unsigned int blanks_start = addr - aligned_addr;

    cl_offset = use_color ? CL_OFFSET : "";
    cl_reset = use_color ? CL_RESET : "";

    print_header(out);

    /* Addr will be compensated with some blanks on the first line */
    addr = aligned_addr;

    while (off < len) {
        size_t bytes = len - off >= bytes_per_line - blanks_start
                     ? bytes_per_line - blanks_start : len - off;

        print_line_with_mask(out,
                             buf + off,
                             mask ? mask + off : NULL,
                             addr,
                             bytes,
                             blanks_start,
                             bytes_per_line - bytes - blanks_start);

        off += bytes;
        addr += bytes_per_line;

        /* After the first line there can't be any blank bytes in the start */
        blanks_start = 0;
    }

    print_footer(out);
}

void print_hex_with_mask(const unsigned char *buf,
                         const unsigned char *mask,
                         size_t len, size_t addr)
{
    fprint_hex_with_mask(stdout, buf, mask, len, addr);
}
