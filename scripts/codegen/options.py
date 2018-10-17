# Copyright 2004-2016, Ned Batchelder.
#           http://nedbatchelder.com/code/cog
# Copyright (c) 2018 Bobby Noelte.
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: MIT

import os
import argparse

class Options(object):

    @staticmethod
    def is_valid_directory(parser, arg):
        if not os.path.isdir(arg):
            parser.error('The directory {} does not exist!'.format(arg))
        else:
            # File directory so return the directory
            return arg

    @staticmethod
    def is_valid_file(parser, arg):
        if not os.path.isfile(arg):
            parser.error('The file {} does not exist!'.format(arg))
        else:
            # File exists so return the file
            return arg

    def __init__(self, data):
        # Defaults for argument values.
        self.args = []
        self.includePath = []
        self.defines = {}
        self.bNoGenerate = False
        self.sOutputName = None
        self.sEncoding = "utf-8"
        self.verbosity = 2
        
        self.data = data
        self.data['runtime'] = dict()

        self._parser = argparse.ArgumentParser(
                            description='Generate code with inlined Python code.')
        self._parser.add_argument('-d', '--delete-code',
            dest='bDeleteCode', action='store_true',
            help='Delete the generator code from the output file.')
        self._parser.add_argument('-D', '--define', nargs=1, metavar='DEFINE',
            dest='defines', action='append',
            help='Define a global string available to your generator code.')
        self._parser.add_argument('-e', '--warn-empty',
            dest='bWarnEmpty', action='store_true',
            help='Warn if a file has no generator code in it.')
        self._parser.add_argument('-U', '--unix-newlines',
            dest='bNewlines', action='store_true',
            help='Write the output with Unix newlines (only LF line-endings).')
        self._parser.add_argument('-I', '--include', nargs=1, metavar='DIR',
            dest='includePath', action='append',
            type=lambda x: self.is_valid_directory(self._parser, x),
            help='Add DIR to the list of directories for data files and modules.')
        self._parser.add_argument('-n', '--encoding', nargs=1,
            dest='sEncoding', action='store', metavar='ENCODING',
            type=lambda x: self.is_valid_file(self._parser, x),
            help='Use ENCODING when reading and writing files.')
        self._parser.add_argument('-i', '--input', nargs=1, metavar='FILE',
            dest='input_file', action='store',
            type=lambda x: self.is_valid_file(self._parser, x),
            help='Get the input from FILE.')
        self._parser.add_argument('-o', '--output', nargs=1, metavar='FILE',
            dest='output_name', action='store',
            help='Write the output to FILE.')
        self._parser.add_argument('-l', '--log', nargs=1, metavar='FILE',
            dest='log_file', action='store',
            help='Log to FILE.')

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
        self.data['runtime']['delete_code'] = args.bDeleteCode
        self.data['runtime']['warn_empty'] = args.bWarnEmpty
        self.data['runtime']['new_lines'] = args.bNewlines

        if args.includePath is None:
            self.data['runtime']['include_path'] = []
        else:
            include_paths = []
            for inc in args.includePath:
                include_paths.append(inc[0])

            self.data['runtime']['include_path'] = include_paths

        self.data['runtime']['encoding'] = args.sEncoding

        if args.input_file is None:
            self.data['runtime']['input_file'] = None
        else:
            self.data['runtime']['input_file'] = args.input_file[0]

        if args.output_name is None:
            self.data['runtime']['output_name'] = None
        else:
            self.data['runtime']['output_name'] = args.output_name[0]

        if args.log_file is None:
            self.data['runtime']['log_file'] = None
        else:
            self.data['runtime']['log_file'] = args.log_file[0]

        self.data['runtime']['defines'] = dict()
        if args.defines is not None:
            for define in args.defines:
                d = define[0].split('=')
                if len(d) > 1:
                    value = d[1]
                else:
                    value = None

                self.data['runtime']['defines'].update({d[0]:value})


class OptionsMixin(object):
    __slots__ = []

    def option(self, option_name):
	    return getattr(self.options, option_name)
