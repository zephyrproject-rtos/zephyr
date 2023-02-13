#!/usr/bin/env python3

# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import os
import json
import argparse
from opensearchpy import OpenSearch
from opensearchpy.helpers import bulk

host = "dashboards.staging.zephyrproject.io"
port = 443

def main():
    args = parse_args()
    if args.user and args.password:
        auth = (args.user, args.password)
    else:
        auth = (os.environ['OPENSEARCH_USER'], os.environ['OPENSEARCH_PASS'])

    client = OpenSearch(
            hosts = [{'host': host, 'port': port}],
            http_auth=auth,
            use_ssl=True,
            verify_certs = False,
            ssl_assert_hostname = False,
            ssl_show_warn = False,
    )
    index_name = args.index

    for f in args.files:
        with open(f, "r") as j:
            data = json.load(j)
            bulk_data = []
            for t in data['testsuites']:
                t['environment'] = data['environment']
                bulk_data.append({
                            "_index": index_name,
                            "_id": t['run_id'],
                            "_source": t
                            }
                        )

        bulk(client, bulk_data)


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument('-u', '--user', help='username')
    parser.add_argument('-p', '--password', help='password')
    parser.add_argument('-i', '--index', help='index to push to.', required=True)
    parser.add_argument('files', metavar='FILE', nargs='+', help='file with test data.')

    args = parser.parse_args()

    return args

if __name__ == '__main__':
    main()
