#!/usr/bin/env python3
#
# Copyright (c) 2018 Bobby Noelte
# Copyright (c) 2018 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

import sys
import argparse
from pathlib import Path
from pprint import pprint

##
# Make relative import work also with __main__
if __package__ is None or __package__ == '':
    # use current directory visibility
    from edtsdb.database import EDTSDb
    from edtsdb.device import EDTSDevice
else:
    # use current package visibility
    from .edtsdb.database import EDTSDb
    from .edtsdb.device import EDTSDevice


##
# @brief Extended DTS database
#
class EDTSDatabase(EDTSDb):

    @staticmethod
    def is_valid_directory(parser, arg):
        try:
            path = Path(arg).resolve()
        except:
            path = Path(arg)
        if not path.is_dir():
            parser.error('The directory {} does not exist!'.format(path))
        else:
            # File directory exists so return the directory
            return str(path)

    @staticmethod
    def is_valid_file(parser, arg):
        try:
            path = Path(arg).resolve()
        except:
            path = Path(arg)
        if not path.is_file():
            parser.error('The file {} does not exist!'.format(path))
        else:
            # File exists so return the file
            return str(path)

    def __init__(self, *args, **kw):
        super().__init__(*args, **kw)

    def callable_main(self, args):
        self._parser = argparse.ArgumentParser(
                    description='Extended Device Tree Specification Database.')
        self._parser.add_argument('-l', '--load', nargs=1, metavar='FILE',
            dest='load_file', action='store',
            type=lambda x: EDTSDatabase.is_valid_file(self._parser, x),
            help='Load the input from FILE.')
        self._parser.add_argument('-s', '--save', nargs=1, metavar='FILE',
            dest='save_file', action='store',
            type=lambda x: EDTSDatabase.is_valid_file(self._parser, x),
            help='Save the database to Json FILE.')
        self._parser.add_argument('-i', '--export-header', nargs=1, metavar='FILE',
            dest='export_header', action='store',
            type=lambda x: EDTSDatabase.is_valid_file(self._parser, x),
            help='Export the database to header FILE.')
        self._parser.add_argument('-e', '--extract', nargs=1, metavar='FILE',
            dest='extract_file', action='store',
            type=lambda x: EDTSDatabase.is_valid_file(self._parser, x),
            help='Extract the database from dts FILE.')
        self._parser.add_argument('-b', '--bindings', nargs='+', metavar='DIR',
            dest='bindings_dirs', action='store',
            type=lambda x: EDTSDatabase.is_valid_directory(self._parser, x),
            help='Use bindings from bindings DIR for extraction.' +
                 ' We allow multiple')
        self._parser.add_argument('-p', '--print',
            dest='print_it', action='store_true',
            help='Print EDTS database content.')

        args = self._parser.parse_args(args)

        if args.load_file is not None:
            self.load(args.load_file[0])
        if args.extract_file is not None:
            self.extract(args.extract_file[0], args.bindings_dirs)
        if args.save_file is not None:
            self.save(args.save_file[0])
        if args.export_header is not None:
            self.export_header(args.export_header[0])
        if args.print_it:
            pprint(self._edts)

        return 0

def main():
    EDTSDatabase().callable_main(sys.argv[1:])

if __name__ == '__main__':
    main()

