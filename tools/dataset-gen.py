#!/usr/bin/python3

import random
import argparse
import struct

def parse_args():
    parser = argparse.ArgumentParser(
        description = 'Create a file with N random floating point numbers.')

    parser.add_argument("output_file_x", type = str)
    parser.add_argument("output_file_y", type = str)
    parser.add_argument("output_file_z", type = str)
    parser.add_argument("--size", type = int, default = 1000)
    parser.add_argument("--min", type = float, default = -3.0)
    parser.add_argument("--max", type = float, default = 3.0)
    parser.add_argument("--error", type = float, default = 0.05)

    return parser.parse_args()

def main():
    args = parse_args()

    with open(args.output_file_x, "wb") as fout_x:
        with open(args.output_file_y, "wb") as fout_y:
            with open(args.output_file_z, "wb") as fout_z:
                for i in range(args.size):
                    x = random.uniform(args.min, args.max)
                    z = random.uniform(args.min, args.max)
                    y = x * 2.0 + z * 3.0
                    y += y * random.uniform(-args.error, args.error)
                    fout_x.write(struct.pack("@d", x))
                    fout_y.write(struct.pack("@d", y))
                    fout_z.write(struct.pack("@d", z))

if __name__ == "__main__":
    main()
