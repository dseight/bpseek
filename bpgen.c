#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "generator.h"

static void usage(void)
{
    printf(
        "Usage: bpgen [OPTIONS] FILE\n"
        "\n"
        "Positional arguments:\n"
        "  FILE                          Path to the file to generate patterns in\n"
        "\n"
        "Options:\n"
        "  --size SIZE, -s              size of the file to generate (default: 4096)\n"
        "  --len LEN, -l LEN            length of the pattern (default: 64)\n"
        "  --header-len HLEN, -H HLEN   length of the generated header (default: 0)\n"
        "  --blips BCNT, -b BCNT        amount of blips in pattern (default: 8)\n"
        "  --seed SEED, -S SEED         seed value\n"
        "  --fill FILL, -f FILL         byte to fill empty data with (default: 0x00)\n"
        "  --help, -h                   show help\n"
    );
}

static struct option longopts[] = {
    { "size",       required_argument,  NULL,  's' },
    { "len",        required_argument,  NULL,  'l' },
    { "header-len", required_argument,  NULL,  'H' },
    { "blips",      required_argument,  NULL,  'b' },
    { "seed",       required_argument,  NULL,  'S' },
    { "fill",       required_argument,  NULL,  'f' },
    { "help",       no_argument,        NULL,  'h' },
    { NULL,         0,                  NULL,  0   }
};


int main(int argc, char *argv[])
{
    int c, fd;
    char *p;
    unsigned long val;
    size_t data_len = 4096;
    size_t pattern_len = 64;
    size_t header_len = 0;
    size_t blips_count = 8;
    uint8_t fill_byte = 0;
    long seed = 0;
    void *data;

    while (1) {
        c = getopt_long(argc, argv, "s:l:H:b:S:f:h", longopts, NULL);

        if (c == -1)
            break;

        switch (c) {
        case 's':
            val = strtoul(optarg, &p, 0);
            if (*optarg == '\0' || *p != '\0') {
                fprintf(stderr, "Invalid value for --size/-s option\n");
                exit(1);
            }
            data_len = val;
            break;
        case 'l':
            val = strtoul(optarg, &p, 0);
            if (*optarg == '\0' || *p != '\0') {
                fprintf(stderr, "Invalid value for --len/-l option\n");
                exit(1);
            }
            pattern_len = val;
            break;
        case 'H':
            val = strtoul(optarg, &p, 0);
            if (*optarg == '\0' || *p != '\0') {
                fprintf(stderr, "Invalid value for --header-len/-H option\n");
                exit(1);
            }
            header_len = val;
            break;
        case 'b':
            val = strtoul(optarg, &p, 0);
            if (*optarg == '\0' || *p != '\0') {
                fprintf(stderr, "Invalid value for --blips/-b option\n");
                exit(1);
            }
            blips_count = val;
            break;
        case 'S':
            val = strtoul(optarg, &p, 0);
            if (*optarg == '\0' || *p != '\0') {
                fprintf(stderr, "Invalid value for --seed/-S option\n");
                exit(1);
            }
            seed = val;
            break;
        case 'f':
            val = strtoul(optarg, &p, 0);
            if (*optarg == '\0' || *p != '\0') {
                fprintf(stderr, "Invalid value for --fill/-f option\n");
                exit(1);
            }
            fill_byte = val;
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

    fd = open(argv[0], O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("Can't open file");
        exit(1);
    }

    if (ftruncate(fd, data_len)) {
        perror("Can't ftruncate file");
        exit(1);
    }

    data = mmap(NULL, data_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("Can't mmap file");
        exit(1);
    }

    srand48(seed);
    generate_pattern(data, data_len, fill_byte, header_len, pattern_len, blips_count);

    return 0;
}
