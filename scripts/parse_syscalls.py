#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import sys
import re
import argparse
import os
import json

api_regex = re.compile(r'''
__syscall\s+                    # __syscall attribute, must be first
([^(]+)                         # type and name of system call (split later)
[(]                             # Function opening parenthesis
([^)]*)                         # Arg list (split later)
[)]                             # Closing parenthesis
''', re.MULTILINE | re.VERBOSE)

typename_regex = re.compile(r'(.*?)([A-Za-z0-9_]+)$')

class SyscallParseException(Exception):
    pass


def typename_split(item):
    if "[" in item:
        raise SyscallParseException("Please pass arrays to syscalls as pointers, unable to process '%s'"
                % item)

    if "(" in item:
        raise SyscallParseException("Please use typedefs for function pointers")

    mo = typename_regex.match(item)
    if not mo:
        raise SyscallParseException("Malformed system call invocation")

    m = mo.groups()
    return (m[0].strip(), m[1])


def analyze_fn(match_group, fn):
    func, args = match_group

    try:
        if args == "void":
            args = []
        else:
            args = [typename_split(a.strip()) for a in args.split(",")]

        func_type, func_name = typename_split(func)
    except SyscallParseException:
        sys.stderr.write("In declaration of %s\n" % func)
        raise

    sys_id = "K_SYSCALL_" + func_name.upper()

    if func_type == "void":
        suffix = "_VOID"
        is_void = True
    else:
        is_void = False
        if func_type in ["s64_t", "u64_t"]:
            suffix = "_RET64"
        else:
            suffix = ""

    is_void = (func_type == "void")

    # Get the proper system call macro invocation, which depends on the
    # number of arguments, the return type, and whether the implementation
    # is an inline function
    macro = "K_SYSCALL_DECLARE%d%s" % (len(args), suffix)

    # Flatten the argument lists and generate a comma separated list
    # of t0, p0, t1, p1, ... tN, pN as expected by the macros
    flat_args = [i for sublist in args for i in sublist]
    if not is_void:
        flat_args = [func_type] + flat_args
    flat_args = [sys_id, func_name] + flat_args
    argslist = ", ".join(flat_args)

    invocation = "%s(%s);" % (macro, argslist)

    handler = "_handler_" + func_name

    # Entry in _k_syscall_table
    table_entry = "[%s] = %s" % (sys_id, handler)

    return (fn, handler, invocation, sys_id, table_entry)


def analyze_headers(base_path):
    ret = []

    for root, dirs, files in os.walk(base_path):
        for fn in files:

            # toolchain/common.h has the definition of __syscall which we
            # don't want to trip over
            path = os.path.join(root, fn)
            if not fn.endswith(".h") or path.endswith("toolchain/common.h"):
                continue

            with open(path, "r", encoding="utf-8") as fp:
                try:
                    result = [analyze_fn(mo.groups(), fn)
                              for mo in api_regex.finditer(fp.read())]
                except Exception:
                    sys.stderr.write("While parsing %s\n" % fn)
                    raise

            ret.extend(result)

    return ret

def parse_args():
    global args
    parser = argparse.ArgumentParser(description = __doc__,
            formatter_class = argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-i", "--include", required=True,
            help="Base include directory")
    parser.add_argument("-j", "--json-file", required=True,
            help="Write system call prototype information as json to file")
    args = parser.parse_args()

def main():
    parse_args()

    syscalls = analyze_headers(args.include)

    syscalls_in_json = json.dumps(
        syscalls,
        indent=4,
        sort_keys=True
    )

    # Check if the file already exists, and if there are no changes,
    # don't touch it since that will force an incremental rebuild
    path = args.json_file
    new = syscalls_in_json
    if os.path.exists(path):
        with open(path, 'r') as fp:
            old = fp.read()

        if new != old:
            with open(path, 'w') as fp:
                fp.write(new)
    else:
        with open(path, 'w') as fp:
            fp.write(new)

if __name__ == "__main__":
    main()
