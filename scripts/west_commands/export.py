# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse

from west.commands import WestCommand

from pathlib import PurePath
from zcmake import run_cmake

EXPORT_DESCRIPTION = '''\
This command registers the current Zephyr installation as a CMake
config package in the CMake user package registry.

In Windows, the CMake user package registry is found in:
HKEY_CURRENT_USER\\Software\\Kitware\\CMake\\Packages\\

In Linux and MacOS, the CMake user package registry is found in:
~/.cmake/packages/'''


class ZephyrExport(WestCommand):

    def __init__(self):
        super().__init__(
            'zephyr-export',
            # Keep this in sync with the string in west-commands.yml.
            'export Zephyr installation as a CMake config package',
            EXPORT_DESCRIPTION,
            accepts_unknown_args=False)

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description)
        return parser

    def do_run(self, args, unknown_args):
        zephyr_config_package_path = PurePath(__file__).parents[2] \
            / 'share' / 'zephyr-package' / 'cmake'

        cmake_args = ['-S', f'{zephyr_config_package_path}',
                      '-B', f'{zephyr_config_package_path}']
        lines = run_cmake(cmake_args, capture_output=True)

        # Let's clean up, as Zephyr has now been exported, and we no longer
	# need the generated files.
        cmake_args = ['--build', f'{zephyr_config_package_path}',
                      '--target', 'pristine']
        run_cmake(cmake_args, capture_output=True)

        # Let's ignore the normal CMake printing and instead only print
        # the important information.
        msg = [line for line in lines if not line.startswith('-- ')]
        print('\n'.join(msg))
