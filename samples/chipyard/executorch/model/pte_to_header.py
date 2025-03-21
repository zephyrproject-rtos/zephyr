# Copyright (c) Meta Platforms, Inc. and affiliates.
# All rights reserved.
# Copyright 2023-2024 Arm Limited and/or its affiliates.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import binascii
import os
from argparse import ArgumentParser, ArgumentTypeError

# Also see: https://git.mlplatform.org/ml/ethos-u/ml-embedded-evaluation-kit.git/tree/scripts/py/gen_model_cpp.py

bytes_per_line = 32
hex_digits_per_line = bytes_per_line * 2


def input_file_path(path):
    if os.path.exists(path):
        return path
    else:
        raise ArgumentTypeError(f"input filepath:{path} does not exist")


parser = ArgumentParser()
parser.add_argument(
    "-p",
    "--pte",
    help="ExecuTorch .pte model file",
    type=input_file_path,
    required=True,
)

parser.add_argument(
    "-o",
    "--outfile",
    help="Output filename for model header",
    type=str,
    required=False,
    default="model_pte.h",
)

if __name__ == "__main__":
    args = parser.parse_args()
    outfile = args.outfile
    # attr = """#include "model_pte.h"\n"""
    attr = f'__attribute__((section("network_model_sec"), aligned(16))) unsigned char '

    with open(args.pte, "rb") as fr, open(outfile, "w") as fw:
        data = fr.read()
        hexstream = binascii.hexlify(data).decode("utf-8")
        fw.write(attr + "model_pte[] = {")

        for i in range(0, len(hexstream), 2):
            if 0 == (i % hex_digits_per_line):
                fw.write("\n")
            fw.write("0x" + hexstream[i : i + 2] + ", ")

        fw.write("};\n")
        fw.write(f"unsigned int model_pte_size = {len(data)};\n")

        print(
            f"Input: {args.pte} with {len(data)} bytes. Output: {outfile} with {fw.tell()} bytes. Section: network_model_sec."
        )
