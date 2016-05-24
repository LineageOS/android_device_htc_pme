/*
 * Copyright (C) 2016, The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define AMP_PATH "/dev/tfa9888"

static int amp_load_sequence(FILE *seq, int amp_fd) {
    char buff[255];
    char buff2[255];
    int ret = 0;
    unsigned int do_write, length;
    size_t len_read, len_written;

    while (true) {
        do_write = fgetc(seq);
        if ((int)do_write == EOF) {
            // this is the normal place to EOF
            break;
        }

        length = fgetc(seq);
        if ((int)length == EOF) {
            ret = -4;
            break;
        }
        length &= 0xFF;

        len_read = fread(buff, 1, length, seq);
        if (len_read < length) {
            ret = -4;
            break;
        }

        if (do_write) {
            len_written = write(amp_fd, buff, length);
            if (len_written < length) {
                ret = -5;
                break;
            }
        } else {
            len_read = read(amp_fd, buff2, length);
            if (len_read < length) {
                ret = -5;
                break;
            }
            for (unsigned x = 0; x < length; x++) printf("%02X ", buff2[x]);
            if (memcmp(buff, buff2, length) != 0) {
                // don't give up
                ret = -6;
                printf("bad\n");
            } else {
                printf("good\n");
            }
        }
    }

    return ret;
}

int main(int argc, char **argv) {
    FILE *seq;
    int amp;
    int ret;

    if (argc != 2) {
        fprintf(stderr, "Usage: amp_loader sequence.asq\n");
        return -1;
    }

    amp = open(AMP_PATH, O_RDWR);
    if (amp < 0) {
        fprintf(stderr, "Failed to open amplifier %s\n", AMP_PATH);
        return -errno;
    }

    seq = fopen(argv[1], "rb");
    if (!seq) {
        fprintf(stderr, "Failed to open loading sequence %s\n", argv[1]);
        close(amp);
        return -3;
    }

    ret = amp_load_sequence(seq, amp);
    if (ret == -4) {
        fprintf(stderr, "Unexpected EOF\n");
    } else if (ret == -5) {
        fprintf(stderr, "IO error\n");
    } else if (ret == -6) {
        fprintf(stderr, "An unexpected response was received\n");
    }

    fclose(seq);
    close(amp);

    return ret;
}
