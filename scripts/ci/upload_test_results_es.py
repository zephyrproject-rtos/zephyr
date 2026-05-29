#!/usr/bin/env python3

# Copyright (c) 2022-2024 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

"""
This script uploads ``twister.json`` file to Elasticsearch index for reporting and analysis.
see  https://kibana.zephyrproject.io/

The script expects two evironment variables with the Elasticsearch server connection parameters:
    `ELASTICSEARCH_SERVER`
    `ELASTICSEARCH_KEY`
"""

from elasticsearch import Elasticsearch
from elasticsearch.helpers import bulk, BulkIndexError
import sys
import os
import json
import argparse
import re


def flatten(name, value, name_sep="_", names_dict=None, parent_name=None, escape_sep=""):
    """
    Flatten ``value`` into a plain dictionary.

    :param name: the flattened name of the ``value`` to be used as a name prefix for all its items.
    :param name_sep: string to separate flattened names; if the same string is already present
                     in the names it will be repeated twise.
    :param names_dict: An optional dictionary with 'foo':'bar' items to flatten 'foo' list properties
                       where each item should be a dictionary with the 'bar' item storing an unique
                       name, so it will be taken as a part of the flattened item's name instead of
                       the item's index in its parent list.
    :param parent_name: the short, single-level, name of the ``value``.
    :param value: object to flatten, for example, a dictionary:
                  {
                    "ROM":{
                        "symbols":{
                            "name":"Root",
                            "size":4320,
                            "identifier":"root",
                            "address":0,
                            "children":[
                                {
                                    "name":"(no paths)",
                                    "size":2222,
                                    "identifier":":",
                                    "address":0,
                                    "children":[
                                        {
                                            "name":"var1",
                                            "size":20,
                                            "identifier":":/var1",
                                            "address":1234
                                        }, ...
                                    ]
                                } ...
                           ]
                        }
                   } ...
                 }

     :return: the ``value`` flattened to a plain dictionary where each key is concatenated from
              names of its initially nested items being separated by the ``name_sep``,
              for the above example:
              {
                  "ROM/symbols/name": "Root",
                  "ROM/symbols/size": 4320,
                  "ROM/symbols/identifier": "root",
                  "ROM/symbols/address": 0,
                  "ROM/symbols/(no paths)/size": 2222,
                  "ROM/symbols/(no paths)/identifier": ":",
                  "ROM/symbols/(no paths)/address": 0,
                  "ROM/symbols/(no paths)/var1/size": 20,
                  "ROM/symbols/(no paths)/var1/identifier": ":/var1",
                  "ROM/symbols/(no paths)/var1/address": 1234,
              }
    """
    res_dict = {}
    name_prefix = name + name_sep if name and len(name) else ''
    if isinstance(value, list) and len(value):
        for idx,val in enumerate(value):
            if isinstance(val, dict) and names_dict and parent_name and isinstance(names_dict, dict) and parent_name in names_dict:
                flat_name = name_prefix + str(val[names_dict[parent_name]]).replace(name_sep, escape_sep + name_sep)
                val_ = val.copy()
                val_.pop(names_dict[parent_name])
                flat_item = flatten(flat_name, val_, name_sep, names_dict, parent_name, escape_sep)
            else:
                flat_name = name_prefix + str(idx)
                flat_item = flatten(flat_name, val, name_sep, names_dict, parent_name, escape_sep)
            res_dict = { **res_dict, **flat_item }
    elif isinstance(value, dict) and len(value):
        for key,val in value.items():
            if names_dict and key in names_dict:
                name_k = name
            else:
                name_k = name_prefix + str(key).replace(name_sep, escape_sep + name_sep)
            flat_item = flatten(name_k, val, name_sep, names_dict, key, escape_sep)
            res_dict = { **res_dict, **flat_item }
    elif len(name):
        res_dict[name] = value
    return res_dict

def unflatten(src_dict, name_sep):
    """
    Unflat ``src_dict`` at its deepest level splitting keys with ``name_sep``
    and using the rightmost chunk to name properties.

    :param src_dict: a dictionary to unflat for example:
                     {
                      "ROM/symbols/name": "Root",
                      "ROM/symbols/size": 4320,
                      "ROM/symbols/identifier": "root",
                      "ROM/symbols/address": 0,
                      "ROM/symbols/(no paths)/size": 2222,
                      "ROM/symbols/(no paths)/identifier": ":",
                      "ROM/symbols/(no paths)/address": 0,
                      "ROM/symbols/(no paths)/var1/size": 20,
                      "ROM/symbols/(no paths)/var1/identifier": ":/var1",
                      "ROM/symbols/(no paths)/var1/address": 1234,
                     }

    :param name_sep: string to split the dictionary keys.
    :return: the unflatten dictionary, for the above example:
             {
              "ROM/symbols": {
                  "name": "Root",
                  "size": 4320,
                  "identifier": "root",
                  "address": 0
              },
              "ROM/symbols/(no paths)": {
                  "size": 2222,
                  "identifier": ":",
                  "address": 0
              },
              "ROM/symbols/(no paths)/var1": {
                  "size": 20,
                  "identifier": ":/var1",
                  "address": 1234
              }
            }
    """
    res_dict = {}
    for k,v in src_dict.items():
        k_pref, _, k_suff = k.rpartition(name_sep)
        if not k_pref in res_dict:
            res_dict[k_pref] = {k_suff: v}
        else:
            if k_suff in res_dict[k_pref]:
                if not isinstance(res_dict[k_pref][k_suff], list):
                    res_dict[k_pref][k_suff] = [res_dict[k_pref][k_suff]]
                res_dict[k_pref][k_suff].append(v)
            else:
                res_dict[k_pref][k_suff] = v
    return res_dict


def transform(t, args):
    if args.transform:
        rules = json.loads(str(args.transform).replace("'", "\"").replace("\\", "\\\\"))
        for property_name, rule in rules.items():
            if property_name in t:
                match = re.match(rule, t[property_name])
                if match:
                    t.update(match.groupdict(default=""))
            #
        #
    for excl_item in args.exclude:
        if excl_item in t:
            t.pop(excl_item)

    return t

def gendata(f, args):
    with open(f, "r") as j:
        data = json.load(j)
        for t in data['testsuites']:
            name = t['name']
            _grouping = name.split("/")[-1]
            main_group = _grouping.split(".")[0]
            sub_group = _grouping.split(".")[1]
            env = data['environment']
            if args.run_date:
                env['run_date'] = args.run_date
            if args.run_id:
                env['run_id'] = args.run_id
            if args.run_attempt:
                env['run_attempt'] = args.run_attempt
            if args.run_branch:
                env['run_branch'] = args.run_branch
            if args.run_workflow:
                env['run_workflow'] = args.run_workflow
            t['environment'] = env
            t['component'] = main_group
            t['sub_component'] = sub_group

            yield_records = 0
            # If the flattered property is a dictionary, convert it to a plain list
            # where each item is a flat dictionaly.
            if args.flatten and args.flatten in t and isinstance(t[args.flatten], dict):
                flat = t.pop(args.flatten)
                flat_list_dict = {}
                if args.flatten_list_names:
                    flat_list_dict = json.loads(str(args.flatten_list_names).replace("'", "\"").replace("\\", "\\\\"))
                #
                # Normalize flattening to a plain dictionary.
                flat = flatten('', flat, args.transpose_separator, flat_list_dict, str(args.escape_separator))
                # Unflat one, the deepest level, expecting similar set of property names there.
                flat = unflatten(flat, args.transpose_separator)
                # Keep dictionary names as their properties and flatten the dictionary to a list of dictionaries.
                as_name = args.flatten_dict_name
                if len(as_name):
                    flat_list = []
                    for k,v in flat.items():
                        v[as_name] = k + args.transpose_separator + v[as_name] if as_name in v else k
                        v[as_name + '_depth'] = v[as_name].count(args.transpose_separator)
                        flat_list.append(v)
                    t[args.flatten] = flat_list
                else:
                    t[args.flatten] = flat

            # Flatten lists or dictionaries cloning the records with the rest of their items and
            # rename them composing the flattened property name with the item's name or index respectively.
            if args.flatten and args.flatten in t and isinstance(t[args.flatten], list):
                flat = t.pop(args.flatten)
                for flat_item in flat:
                    t_clone = t.copy()
                    if isinstance(flat_item, dict):
                        t_clone.update({ args.flatten + args.flatten_separator + k : v for k,v in flat_item.items() })
                    elif isinstance(flat_item, list):
                        t_clone.update({ args.flatten + args.flatten_separator + str(idx) : v for idx,v in enumerate(flat_item) })
                    yield {
                        "_index": args.index,
                        "_source": transform(t_clone, args)
                    }
                    yield_records += 1

            if not yield_records:  # also yields a record without an empty flat object.
                yield {
                        "_index": args.index,
                        "_source": transform(t, args)
                }


def main():
    args = parse_args()

    settings = {
            "index": {
                "number_of_shards": 4
                }
            }

    mappings = {}

    if args.map_file:
        with open(args.map_file, "rt") as json_map:
            mappings = json.load(json_map)
    else:
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
            xx = gendata(f, args)
            for x in xx:
                print(json.dumps(x, indent=4))
        sys.exit(0)

    es = Elasticsearch(
        [os.environ['ELASTICSEARCH_SERVER']],
        api_key=os.environ['ELASTICSEARCH_KEY'],
        verify_certs=False
        )

    if args.create_index:
        es.indices.create(index=args.index, mappings=mappings, settings=settings)
    else:
        if args.run_date:
            print(f"Setting run date from command line: {args.run_date}")

        for f in args.files:
            print(f"Process: '{f}'")
            try:
                bulk(es, gendata(f, args), request_timeout=args.bulk_timeout)
            except BulkIndexError as e:
                print(f"ERROR adding '{f}' exception: {e}")
                error_0 = e.errors[0].get("index", {}).get("error", {})
                reason_0 = error_0.get('reason')
                print(f"ERROR reason: {reason_0}")
                raise e
            #
        #
#

def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False,
                                     formatter_class=argparse.RawTextHelpFormatter,
                                     description=__doc__)
    parser.add_argument('-y','--dry-run', action="store_true", help='Dry run.')
    parser.add_argument('-c','--create-index', action="store_true", help='Create index.')
    parser.add_argument('-m', '--map-file', required=False,
                        help='JSON map file with Elasticsearch index structure and data types.')
    parser.add_argument('-i', '--index', required=True, default='tests-zephyr-1',
                        help='Elasticsearch index to push to.')
    parser.add_argument('-r', '--run-date', help='Run date in ISO format', required=False)
    parser.add_argument('--flatten', required=False, default=None,
                        metavar='TESTSUITE_PROPERTY',
                        help="Flatten one of the test suite's properties:\n"
                        "it will be converted to a list where each list item becomes a separate index record\n"
                        "with all other properties of the test suite object duplicated and the flattened\n"
                        "property name used as a prefix for all its items, e.g.\n"
                        "'recording.cycles' becomes 'recording_cycles'.")
    parser.add_argument('--flatten-dict-name', required=False, default="name",
                        metavar='PROPERTY_NAME',
                        help="For dictionaries flattened into a list, use this name for additional property\n"
                        "to store the item's flat concatenated name. One more property with that name\n"
                        "and'_depth' suffix will be added for number of `--transpose_separator`s in the name.\n"
                        "Default: '%(default)s'. Set empty string to disable.")
    parser.add_argument('--flatten-list-names', required=False, default=None,
                        metavar='DICT',
                        help="An optional string with json dictionary like {'children':'name', ...}\n"
                        "to use it for flattening lists of dictionaries named 'children' which should\n"
                        "contain keys 'name' with unique string value as an actual name for the item.\n"
                        "This name value will be composed instead of the container's name 'children' and\n"
                        "the item's numeric index.")
    parser.add_argument('--flatten-separator', required=False, default="_",
                        help="Separator to use it for the flattened property names. Default: '%(default)s'")
    parser.add_argument('--transpose-separator', required=False, default="/",
                        help="Separator to use it for the transposed dictionary names stored in\n"
                        "`flatten-dict-name` properties. Default: '%(default)s'")
    parser.add_argument('--escape-separator', required=False, default='',
                        help="Prepend name separators with the escape string if already present in names. "
                             "Default: '%(default)s'.")
    parser.add_argument('--transform', required=False,
                        metavar='RULE',
                        help="Apply regexp group parsing to selected string properties after flattening.\n"
                        "The string is a json dictionary with property names and regexp strings to apply\n"
                        "on them to extract values, for example:\n"
                        r"\"{ 'recording_metric': '(?P<object>[^\.]+)\.(?P<action>[^\.]+)\.' }\"")
    parser.add_argument('--exclude', required=False, nargs='*', default=[],
                        metavar='TESTSUITE_PROPERTY',
                        help="Don't store these properties in the Elasticsearch index.")
    parser.add_argument('--run-workflow', required=False,
                        help="Source workflow identificator, e.g. the workflow short name "
                        "and its triggering event name.")
    parser.add_argument('--run-branch', required=False,
                        help="Source branch identificator.")
    parser.add_argument('--run-id', required=False,
                        help="unique run-id (e.g. from github.run_id context)")
    parser.add_argument('--run-attempt', required=False,
                        help="unique run attempt number (e.g. from github.run_attempt context)")
    parser.add_argument('--bulk-timeout', required=False, type=int, default=60,
                        help="Elasticsearch bulk request timeout, seconds. Default %(default)s.")
    parser.add_argument('files', metavar='FILE', nargs='+', help='file with test data.')

    args = parser.parse_args()

    return args


if __name__ == '__main__':
    main()
