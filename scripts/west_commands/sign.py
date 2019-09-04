# Copyright (c) 2018 Foundries.io
#
# SPDX-License-Identifier: Apache-2.0

import abc
import argparse
import os
import pathlib
import shutil
import subprocess

from west import cmake
from west import log
from west.build import is_zephyr_build
from west.util import quote_sh_list

from runners.core import BuildConfiguration

from build_helpers import find_build_dir, \
    FIND_BUILD_DIR_DESCRIPTION

from zephyr_ext_common import Forceable, \
    cached_runner_config

SIGN_DESCRIPTION = '''\
This command automates some of the drudgery of creating signed Zephyr
binaries for chain-loading by a bootloader.

In the simplest usage, run this from your build directory:

   west sign -t your_tool -- ARGS_FOR_YOUR_TOOL

Assuming your binary was properly built for processing and handling by
tool "your_tool", this creates zephyr.signed.bin and zephyr.signed.hex
files (if supported by "your_tool") which are ready for use by your
bootloader. The "ARGS_FOR_YOUR_TOOL" value can be any additional
arguments you want to pass to the tool, such as the location of a
signing key, a version identifier, etc.

See tool-specific help below for details.'''

SIGN_EPILOG = '''\
imgtool
-------

Currently, MCUboot's 'imgtool' tool is supported. To build a signed
binary you can load with MCUboot using imgtool, run this from your
build directory:

   west sign -t imgtool -- --key YOUR_SIGNING_KEY.pem

For this to work, either imgtool must be installed (e.g. using pip3),
or you must pass the path to imgtool.py using the -p option.

The image header size, alignment, and slot sizes are determined from
the build directory using .config and the device tree. A default
version number of 0.0.0+0 is used (which can be overridden by passing
"--version x.y.z+w" after "--key"). As shown above, extra arguments
after a '--' are passed to imgtool directly.'''


class ToggleAction(argparse.Action):

    def __call__(self, parser, args, ignored, option):
        setattr(args, self.dest, not option.startswith('--no-'))


class Sign(Forceable):
    def __init__(self):
        super(Sign, self).__init__(
            'sign',
            # Keep this in sync with the string in west-commands.yml.
            'sign a Zephyr binary for bootloader chain-loading',
            SIGN_DESCRIPTION,
            accepts_unknown_args=False)

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            epilog=SIGN_EPILOG,
            help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description)

        parser.add_argument('-d', '--build-dir',
                            help=FIND_BUILD_DIR_DESCRIPTION)
        self.add_force_arg(parser)

        # general options
        group = parser.add_argument_group('tool control options')
        group.add_argument('-t', '--tool', choices=['imgtool'], required=True,
                           help='''image signing tool name; only imgtool is
                           currently supported''')
        group.add_argument('-p', '--tool-path', default=None,
                           help='''path to the tool itself, if needed''')
        group.add_argument('tool_args', nargs='*', metavar='tool_opt',
                           help='extra option(s) to pass to the signing tool')

        # bin file options
        group = parser.add_argument_group('binary (.bin) file options')
        group.add_argument('--bin', '--no-bin', dest='gen_bin', nargs=0,
                           action=ToggleAction,
                           help='''produce a signed .bin file?
                           (default: yes, if supported and unsigned bin
                           exists)''')
        group.add_argument('-B', '--sbin', metavar='BIN',
                           help='''signed .bin file name
                           (default: zephyr.signed.bin in the build
                           directory, next to zephyr.bin)''')

        # hex file options
        group = parser.add_argument_group('Intel HEX (.hex) file options')
        group.add_argument('--hex', '--no-hex', dest='gen_hex', nargs=0,
                           action=ToggleAction,
                           help='''produce a signed .hex file?
                           (default: yes, if supported and unsigned hex
                           exists)''')
        group.add_argument('-H', '--shex', metavar='HEX',
                           help='''signed .hex file name
                           (default: zephyr.signed.hex in the build
                           directory, next to zephyr.hex)''')

        return parser

    def do_run(self, args, ignored):
        self.args = args        # for check_force

        # Find the build directory and parse .config and DT.
        build_dir = find_build_dir(args.build_dir)
        self.check_force(os.path.isdir(build_dir),
                         'no such build directory {}'.format(build_dir))
        self.check_force(is_zephyr_build(build_dir),
                         "build directory {} doesn't look like a Zephyr build "
                         'directory'.format(build_dir))
        bcfg = BuildConfiguration(build_dir)

        # Decide on output formats.
        formats = []
        bin_exists = 'CONFIG_BUILD_OUTPUT_BIN' in bcfg
        if args.gen_bin:
            self.check_force(bin_exists,
                             '--bin given but CONFIG_BUILD_OUTPUT_BIN not set '
                             "in build directory's ({}) .config".
                             format(build_dir))
            formats.append('bin')
        elif args.gen_bin is None and bin_exists:
            formats.append('bin')

        hex_exists = 'CONFIG_BUILD_OUTPUT_HEX' in bcfg
        if args.gen_hex:
            self.check_force(hex_exists,

                             '--hex given but CONFIG_BUILD_OUTPUT_HEX not set '
                             "in build directory's ({}) .config".
                             format(build_dir))
            formats.append('hex')
        elif args.gen_hex is None and hex_exists:
            formats.append('hex')

        if not formats:
            log.dbg('nothing to do: no output files')
            return

        # Delegate to the signer.
        if args.tool == 'imgtool':
            signer = ImgtoolSigner()
        # (Add support for other signers here in elif blocks)
        else:
            raise RuntimeError("can't happen")

        signer.sign(self, build_dir, bcfg, formats)


class Signer(abc.ABC):
    '''Common abstract superclass for signers.

    To add support for a new tool, subclass this and add support for
    it in the Sign.do_run() method.'''

    @abc.abstractmethod
    def sign(self, command, build_dir, bcfg, formats):
        '''Abstract method to perform a signature; subclasses must implement.

        :param command: the Sign instance
        :param build_dir: the build directory
        :param bcfg: BuildConfiguration for build directory
        :param formats: list of formats to generate ('bin', 'hex')
        '''


class ImgtoolSigner(Signer):

    def sign(self, command, build_dir, bcfg, formats):
        args = command.args

        if args.tool_path:
            command.check_force(shutil.which(args.tool_path),
                                '--tool-path {}: not an executable'.
                                format(args.tool_path))
            tool_path = args.tool_path
        else:
            tool_path = shutil.which('imgtool')
            if not tool_path:
                log.die('imgtool not found; either install it',
                        '(e.g. "pip3 install imgtool") or provide --tool-path')

        align, vtoff, slot_size = [self.get_cfg(command, bcfg, x) for x in
                                   ('DT_FLASH_WRITE_BLOCK_SIZE',
                                    'CONFIG_TEXT_SECTION_OFFSET',
                                    'DT_FLASH_AREA_IMAGE_0_SIZE')]

        log.dbg('build config: --align={}, --header-size={}, --slot-size={}'.
                format(align, vtoff, slot_size))

        # Base sign command.
        #
        # We provide a default --version in case the user is just
        # messing around and doesn't want to set one. It will be
        # overridden if there is a --version in args.tool_args.
        sign_base = [tool_path, 'sign', '--version', '0.0.0+0']
        if align:
            sign_base.extend(['--align', str(align)])
        else:
            log.wrn('expected nonzero flash alignment, but '
                    'DT_FLASH_WRITE_BLOCK_SIZE={} '
                    "in build directory's ({}) device tree".
                    format(align, build_dir))

        if vtoff:
            sign_base.extend(['--header-size', str(vtoff)])
        else:
            log.wrn('expected nonzero header size, but '
                    'CONFIG_TEXT_SECTION_OFFSET={} '
                    "in build directory's ({}) .config".
                    format(vtoff, build_dir))

        if slot_size:
            sign_base.extend(['--slot-size', str(slot_size)])
        else:
            log.wrn('expected nonzero slot size, but '
                    'DT_FLASH_AREA_IMAGE_0_SIZE={} '
                    "in build directory's ({}) device tree".
                    format(slot_size, build_dir))

        b = pathlib.Path(build_dir)
        cache = cmake.CMakeCache.from_build_dir(build_dir)
        runner_config = cached_runner_config(build_dir, cache)

        # Build a signed .bin
        if 'bin' in formats and runner_config.bin_file:
            out_bin = args.sbin or str(b / 'zephyr' / 'zephyr.signed.bin')
            log.inf('Generating:', out_bin)
            sign_bin = (sign_base + args.tool_args +
                        [runner_config.bin_file, out_bin])
            log.dbg(quote_sh_list(sign_bin))
            subprocess.check_call(sign_bin)

        # Build a signed .hex
        if 'hex' in formats and runner_config.hex_file:
            out_hex = args.shex or str(b / 'zephyr' / 'zephyr.signed.hex')
            log.inf('Generating:', out_hex)
            sign_hex = (sign_base + args.tool_args +
                        [runner_config.hex_file, out_hex])
            log.dbg(quote_sh_list(sign_hex))
            subprocess.check_call(sign_hex)

    @staticmethod
    def get_cfg(command, bcfg, item):
        try:
            return bcfg[item]
        except KeyError:
            command.check_force(
                False,
                "imgtool parameter unknown, build directory has no {} {}".
                format('device tree define' if item.startswith('DT_') else
                       'Kconfig option',
                       item))
            return None
