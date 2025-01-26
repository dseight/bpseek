#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "pattern.h"
#include "hex.h"

enum UseColor {
    UseColorNever,
    UseColorAlways,
    UseColorAuto
};

static enum UseColor use_color = UseColorAuto;

static void usage(void)
{
    printf(
        "Usage: bpseek [OPTIONS] FILE\n"
        "\n"
        "Positional arguments:\n"
        "  FILE                          Path to the file to search patterns in\n"
        "\n"
        "Options:\n"
        "  --min-size MIN, -m MIN        min size of pattern to search (default: 4)\n"
        "  --max-size MAX, -x MAX        max size of pattern to search (default: 4096)\n"
        "  --size-step STEP, -s STEP     step between patterns to search (default: 4)\n"
        "  --color COLOR                 auto, always, never (default: auto)\n"
        "  --ff, -f                      treat 0xff as \"empty\" data instead of 0x00\n"
        "  --help, -h                    show help\n"
    );
}

static struct option longopts[] = {
    { "min-size",   required_argument,  NULL,  'm' },
    { "max-size",   required_argument,  NULL,  'x' },
    { "size-step",  required_argument,  NULL,  's' },
    { "color",      required_argument,  NULL,  'c' },
    { "ff",         no_argument,        NULL,  'f' },
    { "help",       no_argument,        NULL,  'h' },
    { NULL,         0,                  NULL,  0   }
};

int main(int argc, char *argv[])
{
    int c, fd;
    char *p;
    unsigned long val;
    size_t min_size = 4;
    size_t max_size = 4096;
    size_t size_step = 4;
    void *data;
    struct stat st;
    struct pattern *pattern;

    while (1) {
        c = getopt_long(argc, argv, "m:x:s:c:fh", longopts, NULL);

        if (c == -1)
            break;

        switch (c) {
        case 'm':
            val = strtoul(optarg, &p, 0);
            if (*optarg == '\0' || *p != '\0') {
                fprintf(stderr, "Invalid value for --min-size/-m option\n");
                exit(1);
            }
            min_size = val;
            break;
        case 'x':
            val = strtoul(optarg, &p, 0);
            if (*optarg == '\0' || *p != '\0') {
                fprintf(stderr, "Invalid value for --max-size/-x option\n");
                exit(1);
            }
            max_size = val;
            break;
        case 's':
            val = strtoul(optarg, &p, 0);
            if (*optarg == '\0' || *p != '\0') {
                fprintf(stderr, "Invalid value for --size-step/-s option\n");
                exit(1);
            }
            size_step = val;
            break;
        case 'c':
            if (!strcmp(optarg, "never")) {
                use_color = UseColorNever;
            } else if (!strcmp(optarg, "always")) {
                use_color = UseColorAlways;
            } else if (!strcmp(optarg, "auto")) {
                use_color = UseColorAuto;
            } else {
                fprintf(stderr, "Invalid value for --color option: %s\n", optarg);
                exit(1);
            }
            break;
        case 'f':
            set_empty_byte(0xff);
            break;
        case 'h':
            usage();
            exit(0);
        default:
            usage();
            exit(1);
        }
    }

    argc -= optind;
    argv += optind;

    if (!argv[0]) {
        fprintf(stderr, "No file provided\n");
        exit(1);
    }

    fd = open(argv[0], O_RDONLY);
    if (fd == -1) {
        perror("Can't open file");
        exit(1);
    }

    if (fstat(fd, &st) == -1) {
        perror("Can't get file stat");
        exit(1);
    }

    if (max_size > st.st_size / 2) {
        fprintf(stderr, "Max pattern size to search is bigger than half of the file. "
                "Please set it to something meaningful, e.g. %lu.\n",
                (unsigned long)(st.st_size / 2));
        exit(1);
    }

    data = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("Can't mmap file");
        exit(1);
    }

    switch (use_color) {
    case UseColorNever:
        set_use_color(false);
        break;
    case UseColorAlways:
        set_use_color(true);
        break;
    case UseColorAuto:
        set_use_color(isatty(fileno(stdout)));
        break;
    }

    pattern = find_pattern(data, st.st_size, min_size, max_size, size_step);
    if (!pattern) {
        perror("Something horrible happened");
        exit(1);
    }

    printf("pattern length: %zu\n", pattern->len);
    print_hex_with_mask(data, pattern->mask, pattern->len);
}
