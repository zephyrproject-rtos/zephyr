# Copyright (c) 2020, Friedt Professional Engineering Services, Inc
#
# SPDX-License-Identifier: Apache-2.0
import struct
import sys

def main():
    print(struct.calcsize("P") * 8)
    sys.exit(0)

if __name__ == "__main__":
    main()
