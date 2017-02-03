#!/usr/bin/env python3
#
# Copyright (c) 2017, Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# Very quick script to move docs from different places into the doc directory
# to fix the website and external links

import os
import shutil
import re
import sys
import fnmatch


# direcories to search for .rst files
CONTENT_DIRS = ["samples", "boards"]

# directives to parse for included files
DIRECTIVES = ["figure","include","image","literalinclude"]

if "ZEPHYR_BASE" not in os.environ:
    sys.stderr.write("$ZEPHYR_BASE environment variable undefined.\n")
    exit(1)
ZEPHYR_BASE = os.environ["ZEPHYR_BASE"]

def get_rst_files(dir):
    matches = []
    for root, dirnames, filenames in os.walk('%s/%s' %(ZEPHYR_BASE, dir)):
        for filename in fnmatch.filter(filenames, '*.rst'):
            matches.append(os.path.join(root, filename))
    for file in matches:
        frel = file.replace(ZEPHYR_BASE,"").strip("/")
        dir=os.path.dirname(frel)
        if not os.path.exists(os.path.join(ZEPHYR_BASE, "doc", dir)):
            os.makedirs(os.path.join(ZEPHYR_BASE, "doc", dir))

        shutil.copyfile(file, os.path.join(ZEPHYR_BASE, "doc", frel))

        with open(file, encoding="utf-8") as f:
            content = f.readlines()
        content = [x.strip() for x in content]
        directives = "|".join(DIRECTIVES)
        pattern = re.compile("\s*\.\.\s+(%s)::\s+(.*)" %directives)
        for l in content:
            m = pattern.match(l)
            if m:
                inf = m.group(2)
                ind = os.path.dirname(inf)
                if not os.path.exists(os.path.join(ZEPHYR_BASE, "doc", dir, ind)):
                    os.makedirs(os.path.join(ZEPHYR_BASE, "doc", dir, ind))

                shutil.copyfile(os.path.join(ZEPHYR_BASE, dir, inf),
                        os.path.join(ZEPHYR_BASE, "doc", dir, inf))

        f.close()

def main():
    for d in CONTENT_DIRS:
        get_rst_files(d)

if __name__ == "__main__":
    main()
