#!/usr/bin/env python3

import argparse
import re

def arguments_parse():
    parser = argparse.ArgumentParser()
    parser.add_argument('-q', dest="quiet", action='store_true')
    parser.add_argument('-m', dest='runmake', action='store_false',
        help='only merge the fragments, do not execute the make command')
    parser.add_argument('-n', dest='alltarget', action='store_const', const='allnoconfig', default='alldefconfig',
        help='use allnoconfig instead of alldefconfig')
    parser.add_argument('-r', dest='warnredun', action='store_true',
        help='list redundant entries when merging fragments')
    parser.add_argument('-O', dest='output_dir',
        help='to put generated output files', default='.')
    parser.add_argument('config', nargs='+')

    return parser.parse_args()


def main():
    args = arguments_parse()

    pattern = re.compile('^(CONFIG_[a-zA-Z0-9_]*)[= ].*')
    config_values = {}

    for config_file in args.config[:]:
        print("Merging %s" % config_file)
        with open(config_file, 'r') as file:
            for line in file:
                match = pattern.match(line)
                if match:
                    config_name = match.group(1)
                    config_full = match.group(0)
                    if config_name in config_values:
                        if config_values[config_name] != config_full and not args.quiet:
                            print("Value of %s is redefined by fragment %s" % (config_name, config_file))
                            print("Previous value %s" % config_values[config_name])
                            print("new value %s" % config_full)
                        elif args.warnredun:
                            print("Value of %s is redundant by fragment %s" % (config_name, config_file))
                    config_values[config_name] = config_full

    if args.runmake:
        print("Running make not yet supported")

    with open('%s/.config' % args.output_dir, 'w') as out_config:
        out_config.write('\n'.join(map(lambda x: x[1], config_values.items())))

if __name__ == "__main__":
    main()
