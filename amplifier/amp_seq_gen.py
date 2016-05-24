# Script to generate amplifier configuration sequences
# Feed it kernel logs from stock with a special boot image:
# https://github.com/sultanqasim/ramdisk_htc_pme/tree/tfa-test
# Filter the dmesg as such: adb shell dmesg | grep tfa_i2c

# There are three notable command sequences
# An long initialization sequence, a medium length enable sequence,
# and a short disable sequence.

# Command sequence binary format:
# Byte 1: command type (read 0, write 1)
# Byte 2: length (0-255)
# Byte 3... : data

import sys

def main(argv):
    infile = open(argv[1], 'r')
    outfile = open(argv[2], 'wb')
    for line in infile.readlines():
        if "tfa_i2c_write" in line:
            write = 1
        else:
            write = 0
        trailer = line.split(": [")[1][:-2].rstrip()
        trailer_bytes = bytes([int(x, 16) for x in trailer.split(' ')])
        outfile.write(bytes([write, len(trailer_bytes)]))
        outfile.write(trailer_bytes)
    infile.close()
    outfile.close()

if __name__ == "__main__":
    main(sys.argv)
