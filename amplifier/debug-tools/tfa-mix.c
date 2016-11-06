#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "tfa.h"
#include "tfa9888.h"
#include "tfa9888-map.h"

static tfa_t *t;

static void dump_i(size_t i)
{
    unsigned value = tfa_get_bitfield(t, bitfields[i].bf);
    printf("%4x %-35s %d 0x%x\n", bitfields[i].bf, bitfields[i].name, value, value);
}

void dump_all(void)
{
    size_t i;

    for (i = 0; i < n_bitfields; i++) {
        dump_i(i);
    }
}

void dump_one(const char *name)
{
    size_t i;

    for (i = 0; i < n_bitfields; i++) {
        if (strcmp(bitfields[i].name, name) == 0) dump_i(i);
    }
}

void set_one(const char *name, const char *value)
{
    size_t i;

    for (i = 0; i < n_bitfields; i++) {
        if (strcmp(bitfields[i].name, name) == 0) {
            tfa_set_bitfield(t, bitfields[i].bf, atoi(value));
            dump_i(i);
        }
    }
}

int main(int argc, char **argv)
{
    struct pcm *pcm = NULL;

    t = tfa_new();
    if (! t) exit(1);

    if (argc > 1 && strcmp(argv[1], "--no-clocks") == 0) {
        argc -= 1;
        argv += 1;
    } else {
        pcm = tfa_clocks_on(t);
    }

    if (argc == 1) dump_all();
    else if (argc == 2) dump_one(argv[1]);
    else if (argc == 3) set_one(argv[1], argv[2]);
    else fprintf(stderr, "usage: [ name [ value ]]\n");

    if (pcm) tfa_clocks_off(t, pcm);

    return 0;
}
