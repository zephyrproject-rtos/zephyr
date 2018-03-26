# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner stub for QEMU.'''

from .core import ZephyrBinaryRunner


class QemuBinaryRunner(ZephyrBinaryRunner):
    '''Place-holder for QEMU runner customizations.'''

    def __init__(self, debug=False):
        super(QemuBinaryRunner, self).__init__(debug=debug)

    @classmethod
    def name(cls):
        return 'qemu'

    @classmethod
    def do_add_parser(cls, parser):
        pass                    # Nothing to do.

    @classmethod
    def create_from_args(command, args):
        return QemuBinaryRunner(debug=args.verbose)

    def do_run(self, command, **kwargs):
        if command == 'debugserver':
            print('Detached GDB server')
