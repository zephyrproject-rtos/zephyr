#!/usr/bin/env python3

import argparse
import re


def arguments_parse():
    parser = argparse.ArgumentParser()
    parser.add_argument('-q', dest="quiet", action='store_true')
    parser.add_argument(
        '-m', dest='runmake', action='store_false',
        help='only merge the fragments, do not execute the make command')
    parser.add_argument(
        '-n', dest='alltarget', action='store_const', const='allnoconfig',
        default='alldefconfig', help='use allnoconfig instead of alldefconfig')
    parser.add_argument('-r', dest='warnredun', action='store_true',
                        help='list redundant entries when merging fragments')
    parser.add_argument('-O', dest='output_dir',
                        help='to put generated output files', default='.')
    parser.add_argument('config', nargs='+')

    return parser.parse_args()


def get_config_list(config_file):
    cfg_list = []
    pattern = re.compile('^(# ){0,1}(CONFIG_[a-zA-Z0-9_]*)[= ].*')
    with open(config_file, 'r') as file:
        for line in file:
            match = pattern.match(line)
            if match:
                if match.group(2):
                    config_name = match.group(2)
                else:
                    config_name = match.group(1)
                cfg_list.append((config_name, match.group(0), ))
    return cfg_list


def remove_config(initfile_content, cfg):
    pattern = re.compile("{}[ =]".format(cfg))
    lines = initfile_content.split("\n")
    new_lines = []

    for line in lines:
        match = pattern.match(line)
        if not match:
            new_lines.append(line)
    initfile_content = "\n".join(new_lines)
    return initfile_content


def append_files(f1, initfile_content):
    fin = open(f1, "r")
    data2 = fin.read()
    fin.close()
    return(initfile_content + data2)


def main():
    args = arguments_parse()
    init_file = open(args.config[0])
    initfile_content = init_file.read()
    init_file.close()
    print("-- Using %s as base" % args.config[0])

    for config_file in args.config[1:]:
        print("-- Merging %s" % config_file)
        cfg_list = get_config_list(config_file)

        for cfg, full in cfg_list:
            prev=list(filter(lambda x: cfg in x, initfile_content.split("\n")))
            if len(prev) > 0 and prev[0] != full and not args.quiet:
                print("-- Value of {} is redefined by fragment {}".format(cfg, config_file))
                print("-- Previous value: {}".format(prev[0]))
                print("-- New value: {}".format(full))
            elif args.warnredun:
                print("Value of %s is redundant by fragment %s" % (cfg, config_file))

            initfile_content = remove_config(initfile_content, cfg)

        initfile_content = append_files(config_file, initfile_content)

    if args.runmake:
        print("Running make not yet supported")
    else:
        with open('%s/.config' % args.output_dir, 'w') as file:
            file.write(initfile_content)

if __name__ == "__main__":
    main()
