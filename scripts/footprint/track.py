#!/usr/bin/env python3
# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import csv
import subprocess
from git import Git
import pathlib
import shutil
import argparse

def parse_args():
    parser = argparse.ArgumentParser(
                description="Generate footprint data based on a predefined plan.")
    parser.add_argument("-p", "--plan", help="Path of test plan", required=True)

    return parser.parse_args()

def main():
    args = parse_args()
    g = Git(".")
    version = g.describe("--abbrev=12")
    pathlib.Path(f'footprint_data/{version}').mkdir(exist_ok=True, parents=True)

    with open(args.plan) as csvfile:
        csvreader = csv.reader(csvfile)
        for row in csvreader:
            name=row[0]
            feature=row[1]
            board=row[2]
            app=row[3]
            options=row[4]

            cmd = ['west',
                'build',
                '-d',
                f'out/{name}/{feature}/{board}',
                '-b',
                board,
                f'{app}',
                '-t',
                'footprint']

            if options != '':
                cmd += ['--', f'{options}']

            print(" ".join(cmd))


            try:
                subprocess.check_output(cmd, stderr=subprocess.STDOUT, timeout=120, universal_newlines=True)
                print("Copying files...")
                pathlib.Path(f'footprint_data/{version}/{name}/{feature}/{board}').mkdir(parents=True, exist_ok=True)

                shutil.copy(f'out/{name}/{feature}/{board}/ram.json', f'footprint_data/{version}/{name}/{feature}/{board}')
                shutil.copy(f'out/{name}/{feature}/{board}/rom.json', f'footprint_data/{version}/{name}/{feature}/{board}')
            except subprocess.CalledProcessError as exc:
                print("Status : FAIL", exc.returncode, exc.output)


if __name__ == "__main__":
    main()
