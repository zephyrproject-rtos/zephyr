# Copyright 2018 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''West's commands subpackage.

All commands should be implemented within modules in this package.
'''

from abc import ABC, abstractmethod

__all__ = ['CommandContextError', 'WestCommand']


class CommandContextError(RuntimeError):
    '''Indicates that a context-dependent command could not be run.'''


class WestCommand(ABC):
    '''Abstract superclass for a west command.

    All top-level commands supported by west implement this interface.'''

    def __init__(self, name, description, accepts_unknown_args=False):
        '''Create a command instance.

        `name`: the command's name, as entered by the user.
        `description`: one-line command description to show to the user.

        `accepts_unknown_args`: if true, the command can handle
        arbitrary unknown command line arguments in its run()
        method. Otherwise, passing unknown arguments will cause
        UnknownArgumentsError to be raised.
        '''
        self.name = name
        self.description = description
        self._accept_unknown = accepts_unknown_args

    def run(self, args, unknown):
        '''Run the command.

        `args`: known arguments parsed via `register_arguments()`
        `unknown`: unknown arguments present on the command line
        '''
        if unknown and not self._accept_unknown:
            self.parser.error('unexpected arguments: {}'.format(unknown))
        self.do_run(args, unknown)

    def add_parser(self, parser_adder):
        '''Registers a parser for this command, and returns it.
        '''
        self.parser = self.do_add_parser(parser_adder)
        return self.parser

    #
    # Mandatory subclass hooks
    #

    @abstractmethod
    def do_add_parser(self, parser_adder):
        '''Subclass method for registering command line arguments.

        `parser_adder` is an argparse argument subparsers adder.'''

    @abstractmethod
    def do_run(self, args, unknown):
        '''Subclasses must implement; called when the command is run.

        `args` is the namespace of parsed known arguments.

        If `accepts_unknown_args` was False when constructing this
        object, `unknown` will be empty. Otherwise, it is an iterable
        containing all unknown arguments present on the command line.
        '''
