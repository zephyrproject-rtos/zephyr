#!/usr/bin/env python3

# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

'''
This script takes a twister-out folder that may contain metadata about builds
including edt.pickle, .config, and compile_commands. It builds an sqlite database
containing metadata about the builds of each test/sample enabling relational querying
to be done from boards to files.
'''

import sqlite3
import argparse
import os
import pickle
import sys
import json
from pathlib import PurePath, Path
from concurrent.futures import ProcessPoolExecutor
from collections import defaultdict

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'dts', 'python-devicetree',
                                'src'))

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'kconfig'))

from kconfiglib import Kconfig, standard_kconfig, MenuNode, Symbol, Choice

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--twister-out", required=True,
                        help="twister-out directory to scan")
    parser.add_argument("--sqlite-db", required=True,
                        help="path to sqlite database to add pairs of (board,compatible) strings"
                        "to")
    parser.add_argument("--no-concurrency", required=False, default=False, action='store_true',
                        help="path to sqlite database to add pairs of (board,compatible) strings"
                        "to")

    return parser.parse_args()

def path_board_name(path):
    p = PurePath(path)
    board_name = p.parts[1]
    return board_name

def path_test_name(path):
    p = PurePath(path)
    parts = p.parts
    test_name = p.parts[len(parts) - 1]
    return test_name

def insert_board_test(cur, board_name, test_name):
    cur.execute("INSERT OR IGNORE INTO board_test(board, test) VALUES(?, ?) RETURNING id;", (board_name,
                                                                                   test_name))
    row = cur.fetchone()
    if row:
        (board_test_id, ) = row
        return board_test_id
    else:
        cur.execute("SELECT id FROM board_test WHERE board = ? and test = ?;", (board_name,
                                                                                test_name))
        row = cur.fetchone()
        (board_test_id, ) = row
        return board_test_id

def insert_edt_data(cur, board_test_id, edt):
    rows = []
    for node in edt.nodes:
        for prop in node.props:
            if prop == 'compatible':
                for comp in node.props[prop].val:
                    rows.append((board_test_id, comp))

    print(f"inserting {len(rows)} board compatible rows for board {board_test_id}")
    cur.executemany("INSERT OR IGNORE INTO board_test_compatible VALUES(?, ?)", rows)

def insert_compile_commands_data(cur, board_test_id, compile_commands):
    cwd = os.getcwd()
    file_paths = []
    for command in compile_commands:
        abspath = command['file']
        relpath = os.path.relpath(command['file'], cwd)
        if PurePath(relpath).parts[0] == 'twister-out':
            continue
        file_paths.append(relpath)

    rows = []
    for file_path in file_paths:
        rows.append((board_test_id, file_path))
    print(f"inserting {len(rows)} board test file rows for board {board_test_id}")
    cur.executemany("INSERT OR IGNORE INTO board_test_file VALUES(?, ?)", rows)


def insert_kconfig(cur, kconfig):
    cur.execute("INSERT OR IGNORE INTO kconfig(config) VALUES(?) RETURNING id;", (config))
    row = cur.fetchone()
    if row:
        (config_id, ) = row
        return config_id
    else:
        cur.execute("SELECT id FROM kconfig WHERE config = ?;", (config))
        row = cur.fetchone()
        (config_id, ) = row
        return config_id

def insert_symbol(cur, symbol):
    print(f"symbol {symbol}, type {type(symbol)}")
    cur.execute("INSERT OR IGNORE INTO symbol(symbol) VALUES(?);", [(symbol)])

def insert_symbol_selects(cur, symbol, sels):
    rows = [(symbol, sel) for sel in sels]
    cur.executemany("INSERT OR IGNORE INTO symbol_selects(symbol, selects) VALUES(?, ?);", rows)

def insert_symbol_depends(cur, symbol, deps):
    rows = [(symbol, dep) for dep in deps]
    cur.executemany("INSERT OR IGNORE INTO symbol_depends_on(symbol, depends_on) VALUES(?, ?);", rows)

def process(sqlite_db, path):
    conn = sqlite3.connect(sqlite_db)
    process_path(conn, path)

def process_path(conn, path):
    board_name = path_board_name(path)
    test_name = path_test_name(path)

    cur = conn.cursor()
    board_test_id = insert_board_test(cur, board_name, test_name)
    conn.commit()

    pickle_path = os.path.join(os.path.join(path, "zephyr"), "edt.pickle")
    print(f"checking if pickle path {pickle_path} exists..")
    if os.path.exists(pickle_path):
        print(f"pickle path {pickle_path} exists, loading")
        with open(pickle_path, 'rb') as f:
            edt = pickle.load(f)
            cur = conn.cursor()
            insert_edt_data(cur, board_test_id, edt)
            conn.commit()
    compile_commands_path = os.path.join(path, "compile_commands.json")
    if os.path.exists(compile_commands_path):
        with open(compile_commands_path, 'rb') as f:
            print(f"compile commands path {compile_commands_path} exists, loading")
            compile_commands = json.load(f)
            insert_compile_commands_data(cur, board_test_id, compile_commands)
            conn.commit()

def map_symbol_dep(dep):
    if isinstance(dep, Symbol):
        return dep.name
    elif isinstance(dep, tuple):
        sel = list(sel)
        sel = list(map(map_symbol_dep, dep))
        return sel
    else:
        print(f"got dep as unknown type {type(dep)}")
        return ""

def map_symbol_select(sel):
    if isinstance(sel, Symbol):
        return sel.name
    elif isinstance(sel, tuple):
        sel = list(sel)
        sel = list(map(map_symbol_select, sel))
        return sel
    else:
        print(f"got sel as unknown type {type(sel)}")
        return ""

def flatten(list_of_lists):
    if len(list_of_lists) == 0:
        return list_of_lists
    if isinstance(list_of_lists[0], list):
        return flatten(list_of_lists[0]) + flatten(list_of_lists[1:])
    return list_of_lists[:1] + flatten(list_of_lists[1:])

def visit_symbol(conn, symbol):
    cur = conn.cursor()
    symbol_name = symbol.name
    symbol_deps = []
    if isinstance(symbol.direct_dep, list):
        symbol_deps = list(map(map_symbol_dep, symbol.direct_dep))
        symbol_deps = flatten(symbol_deps)
        symbol_deps = list(filter(lambda elem: elem != 'y' and elem != "",
                                     symbol_deps))
    elif isinstance(symbol.direct_dep, Symbol):
        symbol_deps = [symbol.direct_dep.name]
    else:
        print(f"can't handle {type(symbol.direct_dep)}")

    symbol_selects = []
    if type(symbol.selects) is list:
        symbol_selects = list(map(map_symbol_select, symbol.selects))
        symbol_selects = flatten(symbol_selects)
        symbol_selects = list(filter(lambda elem: elem != 'y' and elem != "",
                                     symbol_selects))
    elif type(symbol.selects) is tuple:
        print(f"can't handle selects tuple {symbol.selects}")
    else:
        print(f"can't handle selects type {type(symbol.selects)}")

    print(f'''symbol name: {symbol_name}, direct dep {symbol_deps}, selects {symbol_selects}''')
    insert_symbol(cur, symbol_name)
    insert_symbol_selects(cur, symbol_name, symbol_selects)
    insert_symbol_depends(cur, symbol_name, symbol_deps)

def visit_choice(conn, choice):
    cur = conn.cursor()
    insert_symbol(cur, choice.name)
    for sym in choice.syms:
        visit_symbol(conn, sym)

def parse_kconfig(conn):
    ZEPHYR_BASE = os.environ.get('ZEPHYR_BASE')
    if not ZEPHYR_BASE:
        ZEPHYR_BASE = str(Path(__file__).resolve().parents[1])
        os.environ['ZEPHYR_BASE'] = ZEPHYR_BASE

    os.environ['srctree'] = ZEPHYR_BASE
    os.environ['SOC_DIR'] = 'soc/'
    os.environ['ARCH_DIR'] = 'arch/'
    os.environ['BOARD_DIR'] = os.path.join("boards", '*', '*')
    os.environ['ARCH'] = '*'
    os.environ['DEVICETREE_CONF'] = 'dummy'

    # read in Kconfig
    kconf = Kconfig()

    for node in kconf.node_iter():
        item = node.item
        if isinstance(item, Symbol):
            visit_symbol(conn, item)
        elif isinstance(item, Choice):
            visit_choice(conn, item)
        else:
            print(f"unknown node item type {type(node.item)}")

    conn.commit()
    return kconf


def test_paths(twister_out):
    for (dirpath, dirnames, filenames) in os.walk(twister_out):
        if "CMakeCache.txt" in filenames:
            yield dirpath

def main():

    args = parse_args()
    conn = sqlite3.connect(args.sqlite_db)

    cur = conn.cursor()
    cur.execute('''CREATE TABLE IF NOT EXISTS symbol(symbol TEXT NOT NULL, UNIQUE (symbol))''')
    cur.execute('''CREATE TABLE IF NOT EXISTS symbol_depends_on(symbol TEXT NOT NULL, depends_on TEXT
                NOT NULL, UNIQUE (symbol, depends_on))''')
    cur.execute('''CREATE TABLE IF NOT EXISTS symbol_selects(symbol TEXT NOT NULL, selects TEXT
                NOT NULL, UNIQUE (symbol, selects))''')
    cur.execute('''CREATE TABLE IF NOT EXISTS board_test(id INTEGER PRIMARY KEY, board TEXT NOT
                NULL, test TEXT NOT NULL, UNIQUE (board, test))''')
    cur.execute('''CREATE TABLE IF NOT EXISTS board_test_compatible(board_test_id integer,
                compatible text, UNIQUE (board_test_id, compatible))''')
    cur.execute('''CREATE TABLE IF NOT EXISTS board_test_compatible_config(board_test_id integer,
                compatible text, config text, UNIQUE (board_test_id, compatible, config))''')
    cur.execute('''CREATE TABLE IF NOT EXISTS board_test_file(board_test_id integer, file text,
                UNIQUE(board_test_id, file))''')
    conn.commit()

    parse_kconfig(conn)

    if args.no_concurrency:
        for path in test_paths(args.twister_out):
            process_path(conn, path)
    else:
        with ProcessPoolExecutor() as pool:
            for path in test_paths(args.twister_out):
                print(f"Submitting path {path}")
                pool.submit(process, args.sqlite_db, path)

if __name__ == "__main__":
    main()

