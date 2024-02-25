#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

import sys
import argparse
import json


HEADER = """
/**
@page Requirements
@tableofcontents

"""

FOOTER = """
*/
"""

def debug(msg):
    print("DEBUG: {}".format(msg))

def parse_nodes(nodes, grouped):
    for n in nodes:
        if n['TYPE'] in ['TEXT'] :
            continue
        elif n['TYPE'] in ['SECTION'] :
            grouped = parse_nodes(n['NODES'], grouped)
        else:
            rid = n.get('UID')
            name = n['TITLE']
            text= n['STATEMENT']
            group = n.get('COMPONENT', None)
            if not group:
                debug("No group for {}".format(rid))
                continue
            if not grouped.get(group, None):
                grouped[group] = []
            grouped[group].append({'rid': rid, 'req': text, 'name': name })

    return grouped

def parse_strictdoc_json(filename):
    grouped = dict()
    with open(filename) as fp:
        data = json.load(fp)
        docs = data.get('DOCUMENTS')
        for d in docs:
            parse_nodes(d['NODES'], grouped)
    return grouped

def write_dox(grouped, output="requirements.dox"):
    with open(output, "w") as req:
        req.write(HEADER)
        counter = 0
        for r in grouped.keys():
            comp = grouped[r]
            counter += 1
            req.write(f"\n@section REQSEC{counter} {r}\n\n")
            for c in comp:
                req.write("@subsection {} {}: {}\n{}\n\n\n".format(c['rid'], c['rid'], c['name'], c['req']))

        req.write(FOOTER)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Create doxygen requirements page', allow_abbrev=False)
    parser.add_argument('--json', help='JSON file to parse')
    parser.add_argument('--output', help='Output file to write to')

    args = parser.parse_args()

    if args.json:
        filename = args.json
        write_dox(parse_strictdoc_json(filename), args.output)
    else:
        sys.exit("no input given")
