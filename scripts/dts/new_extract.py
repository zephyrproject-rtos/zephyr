#!/usr/bin/env python3

import argparse
import os
import sys

import edtlib

def str_to_label(s):
    # Change ,-@/ to _ and uppercase
    return s.replace('-', '_') \
            .replace(',', '_') \
            .replace('@', '_') \
            .replace('/', '_') \
            .upper()

def main():
    # Copied from extract_dts_includes.py
    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--dts", required=True, help="DTS file")
    parser.add_argument("-y", "--yaml", nargs='+', required=True,
                        help="YAML file directories, we allow multiple")
    parser.add_argument("-i", "--include",
                        help="path to write header to")
    parser.add_argument("-k", "--keyvalue",
                        help="path to write configuration file to")
    parser.add_argument("--old-alias-names", action='store_true',
                        help="Generate aliases also in the old way, without "
                             "compatibility information in their labels")
    args = parser.parse_args()

    edt = edtlib.EDT(args.dts, args.yaml[0])

    import pprint
    dt_output = {}
    dt_alias = {}
    for dev in edt.devices.values():
        postfix = False
        if len(dev.regs) > 1:
            postfix = True
        for r in dev.regs:
            pre = "DT_{}".format(str_to_label(dev.compat))
            body = "{:X}".format(r.addr)
            post = "BASE_ADDRESS"
            if postfix:
                post = "{}_{}".format(post, dev.regs.index(r))
            tag = (pre, body, post)
            dt_output[tag] = r.addr
            dt_alias[(pre, dev.instance, post)] = tag

    with open(args.keyvalue + "-new", "w") as f:
        for (pre, body, post) in dt_output:
            print("#define %s_%s_%s\t0x%x" % (pre, body, post, dt_output[(pre, body, post)]))
        for (pre, body, post) in dt_alias:
            (alias_pre, alias_body, alias_post) = dt_alias[(pre, body, post)]
            print("#define %s_%s_%s\t%s_%s_%s" % (pre, body, post, alias_pre, alias_body, alias_post))

    with open(args.include + "-new", "w") as f:
        for dev in edt.devices.values():
            print("#define {} 1".format(dev.name), file=f)


if __name__ == "__main__":
    main()
