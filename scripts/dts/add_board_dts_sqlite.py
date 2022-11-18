#!/usr/bin/env python3

# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

'''
This script uses takes a given dts file path and sqlite file
path and adds the pair of board name to compatible name
string pairs to a sqlite table.

This table can then be queried in forward or reverse to get
which boards support which compatibles, and which compatibles
a board supports.

Additional tables could be added to then map the compatibles
using Kconfig to drivers and APIs.
'''

import sqlite3
import argparse
import os
import pickle
import sys
from collections import defaultdict

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'python-devicetree',
                                'src'))

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--board-name", required=True,
                        help="name of the board")
    parser.add_argument("--edt-pickle", required=True,
                        help="path to read the pickled edtlib.EDT object from")
    parser.add_argument("--sqlite-db", required=True,
                        help="path to sqlite database to add pairs of (board,compatible) strings"
                        "to")
    return parser.parse_args()

def main():
    args = parse_args()
    conn = sqlite3.connect(args.sqlite_db)

    with open(args.edt_pickle, 'rb') as f:
        edt = pickle.load(f)

    cur = conn.cursor()

    cur.execute("CREATE TABLE IF NOT EXISTS board_compatible(board, compatible)")

    rows = []

    for node in edt.nodes:
        for prop in node.props:
            if prop == 'compatible':
                # compatibles is always an array
                for comp in node.props[prop].val:
                    rows.append((args.board_name, comp))

    cur.executemany("INSERT INTO board_compatible VALUES(?, ?)", rows)
    conn.commit()

if __name__ == "__main__":
    main()

