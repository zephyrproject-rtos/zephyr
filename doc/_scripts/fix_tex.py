#!/usr/bin/env python3
#
# Copyright (c) 2018, Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# Fix the .tex file produced by doxygen
# before feeding to pdflatex or latexmk.

import argparse
import re

def regex_replace(tex_file):
    patterns = [
            # runaway argument
            ("}}\r?\n=\r?\n\r?\n\t{6}", "}} = ")
            ]

    f = open(tex_file, mode='r', encoding="utf-8")
    content = f.read()
    f.close()

    for p in patterns:
        content = re.sub(p[0], p[1], content)

    f = open(tex_file, mode='w', encoding="utf-8")
    f.write(content)
    f.close()

def main():

    parser = argparse.ArgumentParser(description='Fix the .tex file produced '
                                     'by doxygen before feeding it to '
                                     'latexmk (or pdflatex).')

    parser.add_argument('tex_file', nargs=1)
    args = parser.parse_args()

    regex_replace(args.tex_file[0])

if __name__ == "__main__":
    main()
