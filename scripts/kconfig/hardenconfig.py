#!/usr/bin/env python3

# Copyright (c) 2019-2024 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import csv
import os

from kconfiglib import standard_kconfig


def hardenconfig(kconf):
    kconf.load_config()

    hardened_kconf_filename = os.path.join(os.environ['ZEPHYR_BASE'],
                                           'scripts', 'kconfig', 'hardened.csv')

    options = compare_with_hardened_conf(kconf, hardened_kconf_filename)

    display_results(options)


class Option:

    def __init__(self, name, recommended, current=None, symbol=None):
        self.name = name
        self.recommended = recommended
        self.current = current
        self.symbol = symbol

        if current is None:
            self.result = 'NA'
        elif recommended == current:
            self.result = 'PASS'
        else:
            self.result = 'FAIL'


def compare_with_hardened_conf(kconf, hardened_kconf_filename):
    options = []

    with open(hardened_kconf_filename) as csvfile:
        csvreader = csv.reader(csvfile)
        for row in csvreader:
            if len(row) > 1:
                name = row[0]
                recommended = row[1]
                try:
                    symbol = kconf.syms[name]
                    current = symbol.str_value
                except KeyError:
                    symbol = None
                    current = None
                options.append(Option(name=name, current=current,
                                  recommended=recommended, symbol=symbol))
    for node in kconf.node_iter():
        for select in node.selects:
            if kconf.syms["EXPERIMENTAL"] in select or kconf.syms["DEPRECATED"] in select:
                options.append(Option(name=node.item.name, current=node.item.str_value, recommended='n', symbol=node.item))

    return options


def display_results(options):
    # header
    print('{:^50}|{:^13}|{:^20}'.format('name', 'current', 'recommended'), end='')
    print('||{:^28}\n'.format('check result'), end='')
    print('=' * 116)

    # results, only printing options that have failed for now. It simplify the readability.
    # TODO: add command line option to show all results
    for opt in options:
        if opt.result == 'FAIL' and opt.symbol.visibility != 0:
            print('CONFIG_{:<43}|{:^13}|{:^20}'.format(
                opt.name, opt.current, opt.recommended), end='')
            print('||{:^28}\n'.format(opt.result), end='')
    print()


def main():
    hardenconfig(standard_kconfig())


if __name__ == '__main__':
    main()
