#!/usr/bin/env python3

import argparse
import re


def arguments_parse():
    parser = argparse.ArgumentParser()
    parser.add_argument('-q', dest="quiet", action='store_true')
    parser.add_argument('-r', dest='warnredun', action='store_true',
                        help='list redundant entries when merging fragments')
    parser.add_argument('-O', dest='output_dir',
                        help='to put generated output files', default='.')
    parser.add_argument('config', nargs='+')

    return parser.parse_args()


def get_config_name(line):
    # '# CONFIG_FOO is not set' should be treated by merge_config as a
    # state like any other

    is_not_set_pattern = re.compile('^# (CONFIG_[a-zA-Z0-9_]*) is not set.*')
    pattern            = re.compile('^(CONFIG_[a-zA-Z0-9_]*)[= ].*')

    match            =            pattern.match(line)
    match_is_not_set = is_not_set_pattern.match(line)

    if match_is_not_set:
        return match_is_not_set.group(1)
    elif match:
        return match.group(1)

    return "" # No match

def main():
    args = arguments_parse()

    # Create a datastructure in 'conf' that looks like this:
    # [
    #   ("CONFIG_UART", "CONFIG_UART=y"),
    #   ("CONFIG_ARM", "# CONFIG_ARM is not set")
    # ]
    # In other words [(config_name, config_line), ... ]
    #
    # Note that "# CONFIG_ARM is not set" is not the same as a
    # comment, it has meaning (for now).
    # https://github.com/zephyrproject-rtos/zephyr/issues/5443
    conf = []
    for i, fragment_path in enumerate(args.config):
        with open(fragment_path, "r") as f:
            fragment = f.read()
        fragment_list = fragment.split("\n")

        fragment_conf = []
        for line in fragment_list:
            config_name = get_config_name(line)
            if config_name:
                fragment_conf.append( (config_name, line) )

        if i == 0:
            print("-- Using {} as base".format(fragment_path))
        else:
            print("-- Merging {}".format(fragment_path))

            for config_name, line in fragment_conf:
                for (i, (prev_config_name, prev_line)) in enumerate(list(conf)):
                    if config_name == prev_config_name:
                        # The fragment is defining a config that has
                        # already been defined, the fragment has
                        # priority, so we remove the existing entry
                        # and then possibly issue a warning.
                        conf.pop(i)

                        is_redundant = line == prev_line
                        is_redefine  = line != prev_line
                        if not args.quiet:
                            if is_redefine:
                                print("-- Value of {} is redefined by fragment {}".format(config_name, fragment_path))
                                print("-- Previous value: {}".format(prev_line))
                                print("-- New value: {}".format(line))

                            if is_redundant and args.warnredun:
                                print("-- Value of {} is redundant by fragment {}".format(config_name, fragment_path))

        conf.extend(fragment_conf)

    with open('{}/.config'.format(args.output_dir), 'w') as f:
        for (config_name, line) in conf:
            f.write("{}\n".format(line))

if __name__ == "__main__":
    main()
