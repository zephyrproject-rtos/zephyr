#!/usr/bin/env python3

"""Zephyr Check kconfigs Definitions

Check all CONFIG_* in the code are defined into a Kconfig file

usage: checkconfig.py [-h] [-k KCONFIG] [-c] [-e EXCLUDE]

"""

import sys
import os
import re
import argparse
from argparse import SUPPRESS

help_text="""
Checkconfig script scans all '*.c', '*.h' and '*.S' looking for all kconfig
items used in the Zephyr Code and validates if they are defined in any
'*.kconfig*' file.

To run script use command:

checkconfig.py [-h|--help] [-c|--complete-log] [-k|--kconfig <kconfig>]
               [-e|--exclude <dir_to_exclude>]

It will send to output:
-config name
-location where config is used
-full line where the config is being used
-location were config is being defined (there could be multiple declarations)

    [-c|--complete-log] is an optional parameter.
    If it is not given it will print only the kconfigs not defined
    If given, it will print all the kconfigs found

    [-s|--subdir <subdir>] is an optional parameter.
    When a directory is given it will start scan from that specific directorywith .
    By default it scans from zephyr base tree

    [-e|--exclude <subdir_to_exclude>] is an optional parameter.
    When a subdirectory is given it will be appended to defaut exclude dirs list.
    Default exclude dirs are: "doc", "sanity-out" and "outdir"
"""

#the genrest dir contains kconfiglib.py
zephyrbase = os.environ.get('ZEPHYR_BASE')

if zephyrbase == None:
    print ("env. variable ZEPHYR_BASE is not set, "
           "ensure you have source zephyr-env.sh")
    exit(1)
elif not os.path.exists(zephyrbase):
    print ("env. variable ZEPHYR_BASE \""+ zephyrbase +
           "\" does not exist as a valid zephyr base directory")
    exit(1)

os.environ['srctree'] = os.path.join(zephyrbase + os.path.sep)
sys.path.append(os.path.join(zephyrbase,'doc','scripts','genrest'))

import kconfiglib

def separate_location_lines(dirs):
    for dir in dirs:
        print("    ", dir)

def search_kconfig_items(items, name, completelog):
    findConfig = False
    for item in items:
        if item.is_symbol():
            if (item.get_name() == name):
                if (completelog == True):
                    separate_location_lines(item.get_def_locations())
                findConfig = True
                break

        elif item.is_menu():
            if search_kconfig_items(item.get_items(), name, completelog):
                return True

        elif item.is_choice():
            if search_kconfig_items(item.get_items(), name, completelog):
                return True

        elif item.is_comment():
            if (item.get_text() == name):
                print(completelog)
                if (completelog == True):
                    separate_location_lines(item.get_location())
                findConfig = True
                break
    return findConfig

def search_config_in_file(tree, items, completelog, exclude):
    configs = 0
    notdefConfig = 0
    for dirName, subdirs, files in os.walk(tree, topdown=True):
        subdirs[:] = [d for d in subdirs if d not in exclude]
        for fname in files:
            if (fname.endswith(".c") or
            fname.endswith(".h") or
            fname.endswith(".S")):
                with open(os.path.join(dirName, fname), "r", encoding="utf-8", errors="ignore") as f:
                    searchConf = f.readlines()
                for i, line in enumerate(searchConf):
                    if re.search('(^|[\s|(])CONFIG_([a-zA-Z0-9_]+)', line) :
                        configName = re.search('(^|[\s|(])'
                                               +'CONFIG_([a-zA-Z0-9_]+)', line)
                        configs = configs + 1
                        if (completelog == True):
                            print('\n' + configName.group(2) + ' at '
                                   + os.path.join(dirName, fname))
                            print('line: ' + line.rstrip())
                        find = search_kconfig_items(items, configName.group(2),
                                                    completelog)
                        if (find == False):
                            print('\n' + configName.group(2) + ' at '
                                   + os.path.join(dirName, fname)
                                   + ' IS NOT DEFINED')
                            print('line: ' + line.rstrip())
                            notdefConfig = notdefConfig + 1
    if (completelog == True):
        print("\n{} Kconfigs evaluated".format(configs))
        print("{} Kconfigs not defined".format(notdefConfig))

parser = argparse.ArgumentParser(description = help_text,
                                 usage = SUPPRESS,
                                 formatter_class = argparse.RawTextHelpFormatter)
parser.add_argument('-s', '--subdir', action='store', dest='subdir',
                    default="",
                    help='sub directory to be scanned')
parser.add_argument('-c', '--complete-log', action='store_true',
                    dest='completelog', default=False,
                    help='Prints all the kconfigs found')
parser.add_argument('-e', '--exclude', action='append', dest='exclude',
                    default=["doc", "sanity-out", "outdir"],
                    help='Dirs to be excluded for verification')

args= parser.parse_args()
if args.completelog:
    print('sub dir      = ', os.path.join(zephyrbase + args.subdir))
    print('complete-log = ', args.completelog)
    print('exclude dirs = ', args.exclude)

conf = kconfiglib.Config(os.path.join(zephyrbase,'Kconfig'))
search_config_in_file(os.path.join(zephyrbase + os.path.sep + args.subdir),
                      conf.get_top_level_items(), args.completelog, args.exclude)
