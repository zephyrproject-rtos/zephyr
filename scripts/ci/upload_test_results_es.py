#!/usr/bin/env python3

# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0


# This script upload test ci results to the zephyr ES instance for reporting and analysis.
# see https://kibana.zephyrproject.io/

from elasticsearch import Elasticsearch
from elasticsearch.helpers import bulk
import sys
import os
import json
import argparse

def gendata(f, index, run_date=None):
    with open(f, "r") as j:
        data = json.load(j)
        for t in data['testsuites']:
            name = t['name']
            _grouping = name.split("/")[-1]
            main_group = _grouping.split(".")[0]
            sub_group = _grouping.split(".")[1]
            env = data['environment']
            if run_date:
                env['run_date'] =  run_date
            t['environment'] = env
            t['component'] = main_group
            t['sub_component'] = sub_group
            yield {
                    "_index": index,
                    "_source": t
                    }

def main():
    args = parse_args()

    if args.index:
        index_name = args.index
    else:
        index_name = 'tests-zephyr-1'

    settings = {
            "index": {
                "number_of_shards": 4
                }
            }
    mappings = {
            "properties": {
                "execution_time": {"type": "float"},
                "retries": {"type": "integer"},
                "testcases.execution_time": {"type": "float"},
                }
            }

    if args.dry_run:
        xx = None
        for f in args.files:
            xx = gendata(f, index_name)
        for x in xx:
            print(x)
        sys.exit(0)

    es = Elasticsearch(
        [os.environ['ELASTICSEARCH_SERVER']],
        api_key=os.environ['ELASTICSEARCH_KEY'],
        verify_certs=False
        )

    if args.create_index:
        es.indices.create(index=index_name, mappings=mappings, settings=settings)
    else:
        if args.run_date:
            print(f"Setting run date from command line: {args.run_date}")
        for f in args.files:
            bulk(es, gendata(f, index_name, args.run_date))


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument('-y','--dry-run', action="store_true", help='Dry run.')
    parser.add_argument('-c','--create-index', action="store_true", help='Create index.')
    parser.add_argument('-i', '--index', help='index to push to.', required=True)
    parser.add_argument('-r', '--run-date', help='Run date in ISO format', required=False)
    parser.add_argument('files', metavar='FILE', nargs='+', help='file with test data.')

    args = parser.parse_args()

    return args


if __name__ == '__main__':
    main()
