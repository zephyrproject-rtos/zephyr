#!/usr/bin/env python3

import argparse
import os
import sys

import edtlib


def str2ident(s):
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

    with open(args.keyvalue + "-new", "w") as f:
        # Write register addresses
        for dev in edt.devices.values():
            for reg_i, reg in enumerate(dev.regs):
                # Identifier
                ident = "DT_{}_{:X}_BASE_ADDRESS".format(
                    str2ident(dev.compat), reg.addr)
                if len(dev.regs) > 1:
                    ident += "_" + str(reg_i)

                print("#define {}\t0x{:x}".format(ident, reg.addr), file=f)

        # Write aliases
        for dev in edt.devices.values():
            for reg_i, reg in enumerate(dev.regs):
                post = "BASE_ADDRESS"
                if len(dev.regs) > 1:
                    post += "_" + str(reg_i)

                print("#define DT_{}_{}_{}\tDT_{}_{:X}_{}".format(
                          str2ident(dev.compat), dev.instance, post,
                          str2ident(dev.compat), reg.addr, post),
                      file=f)

    with open(args.include + "-new", "w") as f:
        for dev in edt.devices.values():
            print("#define {} 1".format(dev.name), file=f)


if __name__ == "__main__":
    main()
