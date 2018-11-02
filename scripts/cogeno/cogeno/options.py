# Copyright 2004-2016, Ned Batchelder.
#           http://nedbatchelder.com/code/cog
# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: MIT

import os
import argparse
from pathlib import Path

class Options(object):

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

    def __init__(self):
        # Defaults for argument values.
        self.args = []
        self.includePath = []
        self.defines = {}
        self.bNoGenerate = False
        self.sOutputName = None
        self.bWarnEmpty = False
        self.delete_code = False
        self.bNewlines = False
        self.sEncoding = "utf-8"
        self.verbosity = 2

        self.modules_paths = None
        self.templates_paths = None
        self.bindings_paths = None
        self.input_file = None
        self.output_file = None
        self.log_file = None
        self.lock_file = None

        self._parser = argparse.ArgumentParser(
                            description='Generate code with inlined Python code.')
        self._parser.add_argument('-x', '--delete-code',
            dest='delete_code', action='store_true',
            help='Delete the generator code from the output file.')
        self._parser.add_argument('-w', '--warn-empty',
            dest='bWarnEmpty', action='store_true',
            help='Warn if a file has no generator code in it.')
        self._parser.add_argument('-n', '--encoding', nargs=1,
            dest='sEncoding', action='store', metavar='ENCODING',
            type=lambda x: self.is_valid_file(self._parser, x),
            help='Use ENCODING when reading and writing files.')
        self._parser.add_argument('-U', '--unix-newlines',
            dest='bNewlines', action='store_true',
            help='Write the output with Unix newlines (only LF line-endings).')
        self._parser.add_argument('-D', '--define', nargs=1, metavar='DEFINE',
            dest='defines', action='append',
            help='Define a global string available to your generator code.')
        self._parser.add_argument('-m', '--modules', nargs='+', metavar='DIR',
            dest='modules_paths', action='append',
            help='Use modules from modules DIR. We allow multiple')
        self._parser.add_argument('-t', '--templates', nargs='+', metavar='DIR',
            dest='templates_paths', action='append',
            help='Use templates from templates DIR. We allow multiple')
        self._parser.add_argument('-c', '--config', nargs=1, metavar='FILE',
            dest='config_file', action='store',
            help='Use configuration variables from configuration FILE.')
        self._parser.add_argument('-a', '--cmakecache', nargs=1, metavar='FILE',
            dest='cmakecache_file', action='store',
            type=lambda x: self.is_valid_file(self._parser, x),
            help='Use CMake variables from CMake cache FILE.')
        self._parser.add_argument('-d', '--dts', nargs=1, metavar='FILE',
            dest='dts_file', action='store',
            type=lambda x: self.is_valid_file(self._parser, x),
            help='Load the device tree specification from FILE.')
        self._parser.add_argument('-b', '--bindings', nargs='+', metavar='DIR',
            dest='bindings_paths', action='store',
            type=lambda x: self.is_valid_directory(self._parser, x),
            help='Use bindings from bindings DIR for device tree extraction.' +
                 ' We allow multiple')
        self._parser.add_argument('-e', '--edts', nargs=1, metavar='FILE',
            dest='edts_file', action='store',
            help='Write or read EDTS database to/ from FILE.')
        self._parser.add_argument('-i', '--input', nargs=1, metavar='FILE',
            dest='input_file', action='store',
            type=lambda x: self.is_valid_file(self._parser, x),
            help='Get the input from FILE.')
        self._parser.add_argument('-o', '--output', nargs=1, metavar='FILE',
            dest='sOutputName', action='store',
            help='Write the output to FILE.')
        self._parser.add_argument('-l', '--log', nargs=1, metavar='FILE',
            dest='log_file', action='store',
            help='Log to FILE.')
        self._parser.add_argument('-k', '--lock', nargs=1, metavar='FILE',
            dest='lock_file', action='store',
            help='Use lock FILE for concurrent runs of cogeno.')

    def __str__(self):
        sb = []
        for key in self.__dict__:
            sb.append("{key}='{value}'".format(key=key, value=self.__dict__[key]))
        return ', '.join(sb)

    def __repr__(self):
        return self.__str__()


    def __eq__(self, other):
        """ Comparison operator for tests to use.
        """
        return self.__dict__ == other.__dict__

    def clone(self):
        """ Make a clone of these options, for further refinement.
        """
        return copy.deepcopy(self)

    def parse_args(self, argv):
        args = self._parser.parse_args(argv)
        # set options
        self.delete_code = args.delete_code
        self.bWarnEmpty = args.bWarnEmpty
        self.bNewlines = args.bNewlines
        # --modules
        self.modules_paths = []
        if args.modules_paths is not None:
            paths = args.modules_paths
            if not isinstance(paths, list):
                paths = [paths]
            if isinstance(paths[0], list):
                paths = [item for sublist in paths for item in sublist]
            for path in paths:
                try:
                    path = Path(path).resolve()
                except FileNotFoundError:
                    # Python 3.4/3.5 will throw this exception
                    # Python >= 3.6 will not throw this exception
                    path = Path(path)
                if path.is_dir():
                    self.modules_paths.append(path)
                elif path.is_file():
                    self.modules_paths.append(path.parent)
                else:
                    print("options.py: Unknown modules path", path, "- ignored")
        # --templates
        self.templates_paths = []
        if args.templates_paths is not None:
            paths = args.templates_paths
            if not isinstance(paths, list):
                paths = [paths]
            if isinstance(paths[0], list):
                paths = [item for sublist in paths for item in sublist]
            for path in paths:
                try:
                    path = Path(path).resolve()
                except FileNotFoundError:
                    # Python 3.4/3.5 will throw this exception
                    # Python >= 3.6 will not throw this exception
                    path = Path(path)
                if path.is_dir():
                    self.templates_paths.append(path)
                elif path.is_file():
                    self.templates_paths.append(path.parent)
                else:
                    print("options.py: Unknown templates path", path, "- ignored")
        # --config
        if args.config_file is None:
            self.config_file = None
        else:
            self.config_file = args.config_file[0]
        # --cmakecache
        if args.cmakecache_file is None:
            self.cmakecache_file = None
        else:
            self.cmakecache_file = args.config_file[0]
        # --dts
        if args.dts_file is None:
            self.dts_file = None
        else:
            self.dts_file = args.dts_file[0]
        # --bindings
        self.bindings_paths = []
        if args.bindings_paths is not None:
            paths = args.bindings_paths
            if not isinstance(paths, list):
                paths = [paths]
            if isinstance(paths[0], list):
                paths = [item for sublist in paths for item in sublist]
            for path in paths:
                try:
                    path = Path(path).resolve()
                except FileNotFoundError:
                    # Python 3.4/3.5 will throw this exception
                    # Python >= 3.6 will not throw this exception
                    path = Path(path)
                if path.is_dir():
                    self.bindings_paths.append(path)
                elif path.is_file():
                    self.bindings_paths.append(path.parent)
                else:
                    print("options.py: Unknown bindings path", path, "- ignored")
        # --edts
        if args.edts_file is None:
            self.edts_file = None
        else:
            self.edts_file = args.edts_file[0]
        # --input
        if args.input_file is None:
            self.input_file = None
        else:
            self.input_file = args.input_file[0]
        # --output
        if args.sOutputName is None:
            self.sOutputName = None
        else:
            self.sOutputName = args.sOutputName[0]
	# --log_file
        if args.log_file is None:
            self.log_file = None
        else:
            self.log_file = args.log_file[0]
	# --lock_file
        if args.lock_file is None:
            self.lock_file = None
        else:
            self.lock_file = args.lock_file[0]
        # --defines
        self.defines = {}
        if args.defines is not None:
            for define in args.defines:
                d = define[0].split('=')
                if len(d) > 1:
                    value = d[1]
                else:
                    value = None
                self.defines[d[0]] = value


class OptionsMixin(object):
    __slots__ = []

    def option(self, option_name):
        return getattr(self._context._options, option_name)
