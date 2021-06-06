#!/usr/bin/env python3
# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

from anytree.importer import DictImporter
from anytree import PreOrderIter
from anytree.search  import find
importer = DictImporter()
from datetime import datetime
from dateutil.relativedelta import relativedelta
import os
import json
from git import Repo
from git.exc import BadName

from influxdb import InfluxDBClient
import glob
import argparse
from tabulate import tabulate

TODAY = datetime.utcnow()
two_mon_rel = relativedelta(months=4)

influx_dsn = 'influxdb://localhost:8086/footprint_tracking'

def create_event(data, board, feature, commit, current_time, typ, application):
    footprint_data = []
    client = InfluxDBClient.from_dsn(influx_dsn)
    client.create_database('footprint_tracking')
    for d in data.keys():
        footprint_data.append({
            "measurement": d,
            "tags": {
                "board": board,
                "commit": commit,
                "application": application,
                "type": typ,
                "feature": feature
            },
            "time": current_time,
            "fields": {
                "value": data[d]
            }
        })

    client.write_points(footprint_data, time_precision='s', database='footprint_tracking')


def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-d", "--data", help="Data Directory")
    parser.add_argument("-y", "--dryrun", action="store_true", help="Dry run, do not upload to database")
    parser.add_argument("-z", "--zephyr-base", help="Zephyr tree")
    parser.add_argument("-f", "--file", help="JSON file with footprint data")
    args = parser.parse_args()


def parse_file(json_file):

    with open(json_file, "r") as fp:
        contents = json.load(fp)
        root = importer.import_(contents['symbols'])

    zr = find(root, lambda node: node.name == 'ZEPHYR_BASE')
    ws = find(root, lambda node: node.name == 'WORKSPACE')

    data = {}
    if zr and ws:
        trees = [zr, ws]
    else:
        trees = [root]

    for node in PreOrderIter(root, maxlevel=2):
        if node.name not in ['WORKSPACE', 'ZEPHYR_BASE']:
            if node.name in ['Root', 'Symbols']:
                data['all'] = node.size
            else:
                data[node.name] = node.size

    for t in trees:
        root = t.name
        for node in PreOrderIter(t, maxlevel=2):
            if node.name == root:
                continue
            comp = node.name
            if comp in ['Root', 'Symbols']:
                data['all'] = node.size
            else:
                data[comp] = node.size

    return data

def process_files(data_dir, zephyr_base, dry_run):
    repo = Repo(zephyr_base)

    for hash in os.listdir(f'{data_dir}'):
        if not dry_run:
            client = InfluxDBClient.from_dsn(influx_dsn)
            result = client.query(f"select * from kernel where commit = '{hash}';")
            if result:
                print(f"Skipping {hash}...")
                continue
        print(f"Importing {hash}...")
        for file in glob.glob(f"{args.data}/{hash}/**/*json", recursive=True):
            file_data = file.split("/")
            json_file = os.path.basename(file)
            if 'ram' in json_file:
                typ = 'ram'
            else:
                typ = 'rom'
            commit = file_data[1]
            app = file_data[2]
            feature = file_data[3]
            board = file_data[4]

            data = parse_file(file)

            try:
                gitcommit = repo.commit(f'{commit}')
                current_time = gitcommit.committed_datetime
            except BadName:
                cidx = commit.find('-g') + 2
                gitcommit = repo.commit(f'{commit[cidx:]}')
                current_time = gitcommit.committed_datetime

            print(current_time)

            if not dry_run:
                create_event(data, board, feature, commit, current_time, typ, app)

def main():
    parse_args()

    if args.data and args.zephyr_base:
        process_files(args.data, args.zephyr_base, args.dryrun)

    if args.file:
        data = parse_file(args.file)
        items = []
        for component,value in data.items():
            items.append([component,value])

        table = tabulate(items, headers=['Component', 'Size'], tablefmt='orgtbl')
        print(table)


if __name__ == "__main__":
    main()
