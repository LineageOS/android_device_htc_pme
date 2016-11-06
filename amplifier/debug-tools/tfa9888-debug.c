#include <stdio.h>
#include <assert.h>
#include "tfa9888.h"
#include "tfa9888-map.h"

void tfa9888_print_bitfield(FILE *f, unsigned reg, unsigned v)
{
    size_t i;

    fprintf(f, " %02x", v);
    for (i = 0; i < n_bitfields; i++) {
        if (reg == bitfields[i].bf >> 8) {
            int shift = (bitfields[i].bf & 0xf0) >> 4;
            int nbits = (bitfields[i].bf & 0xf) + 1;
            int mask = (1<<nbits)-1;
            int bits = (v >> shift) & mask;

            if (bits) fprintf(f, " %s:%d", bitfields[i].name, bits);
        }
    }
    fprintf(f, "\n");
}
