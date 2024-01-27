"""
Copyright (c) 2024 Zephyr Project members and individual contributors
SPDX-License-Identifier: Apache-2.0

This document contains functions to substitute variables in code-block
directives using python.

Currently it substitutes SDK version related literals in the Developing 
with Zephyr section.
"""
import sys
import os
from pathlib import Path
import argparse

ZEPHYR_BASE = Path(__file__).resolve().parents[2]
ZEPHYR_DOC_BASE = ZEPHYR_BASE / "doc" / "develop"

# parse SDK version from 'SDK_VERSION' file
with open(ZEPHYR_BASE / "SDK_VERSION") as f:
    sdk_version = f.read().strip()

sdk_url_base = "https://github.com/zephyrproject-rtos/sdk-ng/releases/"
sdk_url_prefix = f"{sdk_url_base}/download/v{sdk_version}/zephyr-sdk-{sdk_version}"
sdk_url_shasum = f"{sdk_url_base}/download/v{sdk_version}/sha256.sum"

subs_list = [("|sdk-url-linux|", f"`{sdk_url_prefix}_linux-x86_64.tar.xz`"),
             ("|sdk-url-macos|", f"`{sdk_url_prefix}_macos-x86_64.tar.xz`"),
             ("|sdk-url-windows|", f"`{sdk_url_prefix}_windows-x86_64.7z`"),
             ("|sdk-url-shasum|", f"`{sdk_url_shasum}`"),
             ("|sdk-version-literal|", f"``{sdk_version}``"),
             ("|sdk-version|", f"{sdk_version}"),
]

def find_and_replace_all(root, post):
    orig = 0 if post else 1
    for dname, _, files in os.walk(root, topdown=True, followlinks=False):
        for file in files:
            if file.endswith('.rst'):
                fpath = os.path.join(dname, file)
                with open(fpath, 'r+') as f:
                    contents = f.read()
                    for sub in subs_list:
                        if sub[1-orig] in contents:
                            contents = contents.replace(sub[1-orig], sub[orig])
                            f.seek(0)
                            f.write(contents)
                            f.truncate()
                    

def main():
    # Parse sys.argv to determine if pre or post
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument('--post', required=False, action='store_true')
    post_substitute = parser.parse_args().post
    find_and_replace_all(ZEPHYR_DOC_BASE, post_substitute)


if __name__ == '__main__':
    main()
    sys.exit(0)
