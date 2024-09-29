# Copyright (c) 2018 Foundries.io
#
# SPDX-License-Identifier: Apache-2.0

import abc
import argparse
import os
import pathlib
import pickle
import platform
import shutil
import subprocess
import sys

from west import manifest
from west.commands import Verbosity
from west.util import quote_sh_list

from build_helpers import find_build_dir, is_zephyr_build, \
    FIND_BUILD_DIR_DESCRIPTION
from runners.core import BuildConfiguration
from zcmake import CMakeCache
from zephyr_ext_common import Forceable, ZEPHYR_SCRIPTS

# This is needed to load edt.pickle files.
sys.path.insert(0, str(ZEPHYR_SCRIPTS / 'dts' / 'python-devicetree' / 'src'))

SIGN_DESCRIPTION = '''\
This command automates some of the drudgery of creating signed Zephyr
binaries for chain-loading by a bootloader.

In the simplest usage, run this from your build directory:

   west sign -t your_tool -- ARGS_FOR_YOUR_TOOL

The "ARGS_FOR_YOUR_TOOL" value can be any additional arguments you want to
pass to the tool, such as the location of a signing key etc. Depending on
which sort of ARGS_FOR_YOUR_TOOLS you use, the `--` separator/sentinel may
not always be required. To avoid ambiguity and having to find and
understand POSIX 12.2 Guideline 10, always use `--`.

See tool-specific help below for details.'''

SIGN_EPILOG = '''\
imgtool
-------

To build a signed binary you can load with MCUboot using imgtool,
run this from your build directory:

   west sign -t imgtool -- --key YOUR_SIGNING_KEY.pem

For this to work, either imgtool must be installed (e.g. using pip3),
or you must pass the path to imgtool.py using the -p option.

Assuming your binary was properly built for processing and handling by
imgtool, this creates zephyr.signed.bin and zephyr.signed.hex
files which are ready for use by your bootloader.

The version number, image header size, alignment, and slot sizes are
determined from the build directory using .config and the device tree.
As shown above, extra arguments after a '--' are passed to imgtool
directly.

rimage
------

To create a signed binary with the rimage tool, run this from your build
directory:

   west sign -t rimage -- -k YOUR_SIGNING_KEY.pem

For this to work, either rimage must be installed or you must pass
the path to rimage using the -p option.

You can also pass additional arguments to rimage thanks to [sign] and
[rimage] sections in your west config file(s); this is especially useful
when invoking west sign _indirectly_ through CMake/ninja. See how at
https://docs.zephyrproject.org/latest/develop/west/sign.html
'''

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
        parser.add_argument('-q', '--quiet', action='store_true',
                            help='suppress non-error output')
        self.add_force_arg(parser)

        # general options
        group = parser.add_argument_group('tool control options')
        group.add_argument('-t', '--tool', choices=['imgtool', 'rimage'],
                           help='''image signing tool name; imgtool and rimage
                           are currently supported (imgtool is deprecated)''')
        group.add_argument('-p', '--tool-path', default=None,
                           help='''path to the tool itself, if needed''')
        group.add_argument('-D', '--tool-data', default=None,
                           help='''path to a tool-specific data/configuration directory, if needed''')
        group.add_argument('--if-tool-available', action='store_true',
                           help='''Do not fail if the rimage tool is not found or the rimage signing
schema (rimage "target") is not defined in board.cmake.''')
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
        build_conf = BuildConfiguration(build_dir)

        if not args.tool:
            args.tool = self.config_get('sign.tool')

        # Decide on output formats.
        formats = []
        bin_exists = build_conf.getboolean('CONFIG_BUILD_OUTPUT_BIN')
        if args.gen_bin:
            self.check_force(bin_exists,
                             '--bin given but CONFIG_BUILD_OUTPUT_BIN not set '
                             "in build directory's ({}) .config".
                             format(build_dir))
            formats.append('bin')
        elif args.gen_bin is None and bin_exists:
            formats.append('bin')

        hex_exists = build_conf.getboolean('CONFIG_BUILD_OUTPUT_HEX')
        if args.gen_hex:
            self.check_force(hex_exists,
                             '--hex given but CONFIG_BUILD_OUTPUT_HEX not set '
                             "in build directory's ({}) .config".
                             format(build_dir))
            formats.append('hex')
        elif args.gen_hex is None and hex_exists:
            formats.append('hex')

        # Delegate to the signer.
        if args.tool == 'imgtool':
            if args.if_tool_available:
                self.die('imgtool does not support --if-tool-available')
            signer = ImgtoolSigner()
        elif args.tool == 'rimage':
            signer = RimageSigner()
        # (Add support for other signers here in elif blocks)
        else:
            if args.tool is None:
                self.die('one --tool is required')
            else:
                self.die(f'invalid tool: {args.tool}')

        signer.sign(self, build_dir, build_conf, formats)


class Signer(abc.ABC):
    '''Common abstract superclass for signers.

    To add support for a new tool, subclass this and add support for
    it in the Sign.do_run() method.'''

    @abc.abstractmethod
    def sign(self, command, build_dir, build_conf, formats):
        '''Abstract method to perform a signature; subclasses must implement.

        :param command: the Sign instance
        :param build_dir: the build directory
        :param build_conf: BuildConfiguration for build directory
        :param formats: list of formats to generate ('bin', 'hex')
        '''


class ImgtoolSigner(Signer):

    def sign(self, command, build_dir, build_conf, formats):
        if not formats:
            return

        args = command.args
        b = pathlib.Path(build_dir)

        command.wrn("west sign using imgtool is deprecated and will be removed in a future release")

        imgtool = self.find_imgtool(command, args)
        # The vector table offset and application version are set in Kconfig:
        appver = self.get_cfg(command, build_conf, 'CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION')
        vtoff = self.get_cfg(command, build_conf, 'CONFIG_ROM_START_OFFSET')
        # Flash device write alignment and the partition's slot size
        # come from devicetree:
        flash = self.edt_flash_node(command, b, args.quiet)
        align, addr, size = self.edt_flash_params(command, flash)

        if not build_conf.getboolean('CONFIG_BOOTLOADER_MCUBOOT'):
            command.wrn("CONFIG_BOOTLOADER_MCUBOOT is not set to y in "
                        f"{build_conf.path}; this probably won't work")

        kernel = build_conf.get('CONFIG_KERNEL_BIN_NAME', 'zephyr')

        if 'bin' in formats:
            in_bin = b / 'zephyr' / f'{kernel}.bin'
            if not in_bin.is_file():
                command.die(f"no unsigned .bin found at {in_bin}")
            in_bin = os.fspath(in_bin)
        else:
            in_bin = None
        if 'hex' in formats:
            in_hex = b / 'zephyr' / f'{kernel}.hex'
            if not in_hex.is_file():
                command.die(f"no unsigned .hex found at {in_hex}")
            in_hex = os.fspath(in_hex)
        else:
            in_hex = None

        if not args.quiet:
            command.banner('image configuration:')
            command.inf('partition offset: {0} (0x{0:x})'.format(addr))
            command.inf('partition size: {0} (0x{0:x})'.format(size))
            command.inf('rom start offset: {0} (0x{0:x})'.format(vtoff))

        # Base sign command.
        sign_base = imgtool + ['sign',
                               '--version', str(appver),
                               '--align', str(align),
                               '--header-size', str(vtoff),
                               '--slot-size', str(size)]
        sign_base.extend(args.tool_args)

        if not args.quiet:
            command.banner('signing binaries')
        if in_bin:
            out_bin = args.sbin or str(b / 'zephyr' / 'zephyr.signed.bin')
            sign_bin = sign_base + [in_bin, out_bin]
            if not args.quiet:
                command.inf(f'unsigned bin: {in_bin}')
                command.inf(f'signed bin:   {out_bin}')
                command.dbg(quote_sh_list(sign_bin))
            subprocess.check_call(sign_bin, stdout=subprocess.PIPE if args.quiet else None)
        if in_hex:
            out_hex = args.shex or str(b / 'zephyr' / 'zephyr.signed.hex')
            sign_hex = sign_base + [in_hex, out_hex]
            if not args.quiet:
                command.inf(f'unsigned hex: {in_hex}')
                command.inf(f'signed hex:   {out_hex}')
                command.dbg(quote_sh_list(sign_hex))
            subprocess.check_call(sign_hex, stdout=subprocess.PIPE if args.quiet else None)

    @staticmethod
    def find_imgtool(cmd, args):
        if args.tool_path:
            imgtool = args.tool_path
            if not os.path.isfile(imgtool):
                cmd.die(f'--tool-path {imgtool}: no such file')
        else:
            imgtool = shutil.which('imgtool') or shutil.which('imgtool.py')
            if not imgtool:
                cmd.die('imgtool not found; either install it',
                        '(e.g. "pip3 install imgtool") or provide --tool-path')

        if platform.system() == 'Windows' and imgtool.endswith('.py'):
            # Windows users may not be able to run .py files
            # as executables in subprocesses, regardless of
            # what the mode says. Always run imgtool as
            # 'python path/to/imgtool.py' instead of
            # 'path/to/imgtool.py' in these cases.
            # https://github.com/zephyrproject-rtos/zephyr/issues/31876
            return [sys.executable, imgtool]

        return [imgtool]

    @staticmethod
    def get_cfg(command, build_conf, item):
        try:
            return build_conf[item]
        except KeyError:
            command.check_force(
                False, "build .config is missing a {} value".format(item))
            return None

    @staticmethod
    def edt_flash_node(cmd, b, quiet=False):
        # Get the EDT Node corresponding to the zephyr,flash chosen DT
        # node; 'b' is the build directory as a pathlib object.

        # Ensure the build directory has a compiled DTS file
        # where we expect it to be.
        dts = b / 'zephyr' / 'zephyr.dts'
        if not quiet:
            cmd.dbg('DTS file:', dts, level=Verbosity.DBG_MORE)
        edt_pickle = b / 'zephyr' / 'edt.pickle'
        if not edt_pickle.is_file():
            cmd.die("can't load devicetree; expected to find:", edt_pickle)

        # Load the devicetree.
        with open(edt_pickle, 'rb') as f:
            edt = pickle.load(f)

        # By convention, the zephyr,flash chosen node contains the
        # partition information about the zephyr image to sign.
        flash = edt.chosen_node('zephyr,flash')
        if not flash:
            cmd.die('devicetree has no chosen zephyr,flash node;',
                    "can't infer flash write block or slot0_partition slot sizes")

        return flash

    @staticmethod
    def edt_flash_params(cmd, flash):
        # Get the flash device's write alignment and offset from the
        # slot0_partition and the size from slot1_partition , out of the
        # build directory's devicetree. slot1_partition size is used,
        # when available, because in swap-move mode it can be one sector
        # smaller. When not available, fallback to slot0_partition (single slot dfu).

        # The node must have a "partitions" child node, which in turn
        # must have child nodes with label slot0_partition and may have a child node
        # with label slot1_partition. By convention, the slots for consumption by
        # imgtool are linked into these partitions.
        if 'partitions' not in flash.children:
            cmd.die("DT zephyr,flash chosen node has no partitions,",
                    "can't find partitions for MCUboot slots")

        partitions = flash.children['partitions']
        slots = {
            label: node for node in partitions.children.values()
                        for label in node.labels
                        if label in set(['slot0_partition', 'slot1_partition'])
        }

        if 'slot0_partition' not in slots:
            cmd.die("DT zephyr,flash chosen node has no slot0_partition partition,",
                    "can't determine its address")

        # Die on missing or zero alignment or slot_size.
        if "write-block-size" not in flash.props:
            cmd.die('DT zephyr,flash node has no write-block-size;',
                    "can't determine imgtool write alignment")
        align = flash.props['write-block-size'].val
        if align == 0:
            cmd.die('expected nonzero flash alignment, but got '
                    'DT flash device write-block-size {}'.format(align))

        # The partitions node, and its subnode, must provide
        # the size of slot1_partition or slot0_partition partition via the regs property.
        slot_key = 'slot1_partition' if 'slot1_partition' in slots else 'slot0_partition'
        if not slots[slot_key].regs:
            cmd.die(f'{slot_key} flash partition has no regs property;',
                    "can't determine size of slot")

        # always use addr of slot0_partition, which is where slots are run
        addr = slots['slot0_partition'].regs[0].addr

        size = slots[slot_key].regs[0].size
        if size == 0:
            cmd.die('expected nonzero slot size for {}'.format(slot_key))

        return (align, addr, size)

class RimageSigner(Signer):

    def rimage_config_dir(self):
        'Returns the rimage/config/ directory with the highest precedence'
        args = self.command.args
        if args.tool_data:
            conf_dir = pathlib.Path(args.tool_data)
        elif self.cmake_cache.get('RIMAGE_CONFIG_PATH'):
            conf_dir = pathlib.Path(self.cmake_cache['RIMAGE_CONFIG_PATH'])
        else:
            conf_dir = self.sof_src_dir / 'tools' / 'rimage' / 'config'
        self.command.dbg(f'rimage config directory={conf_dir}')
        return conf_dir

    def preprocess_toml(self, config_dir, toml_basename, subdir):
        'Runs the C pre-processor on config_dir/toml_basename.h'

        compiler_path = self.cmake_cache.get("CMAKE_C_COMPILER")
        preproc_cmd = [compiler_path, '-E', str(config_dir / (toml_basename + '.h'))]
        # -P removes line markers to keep the .toml output reproducible.  To
        # trace #includes, temporarily comment out '-P' (-f*-prefix-map
        # unfortunately don't seem to make any difference here and they're
        # gcc-specific)
        preproc_cmd += ['-P']

        # "REM" escapes _leading_ '#' characters from cpp and allows
        # such comments to be preserved in generated/*.toml files:
        #
        #      REM # my comment...
        #
        # Note _trailing_ '#' characters and comments are ignored by cpp
        # and don't need any REM trick.
        preproc_cmd += ['-DREM=']

        preproc_cmd += ['-I', str(self.sof_src_dir / 'src')]
        preproc_cmd += ['-imacros',
                        str(pathlib.Path('zephyr') / 'include' / 'generated' / 'zephyr' / 'autoconf.h')]
        preproc_cmd += ['-o', str(subdir / 'rimage_config.toml')]
        self.command.inf(quote_sh_list(preproc_cmd))
        subprocess.run(preproc_cmd, check=True, cwd=self.build_dir)

    def sign(self, command, build_dir, build_conf, formats):
        self.command = command
        args = command.args

        b = pathlib.Path(build_dir)
        self.build_dir = b
        cache = CMakeCache.from_build_dir(build_dir)
        self.cmake_cache = cache

        # Warning: RIMAGE_TARGET in Zephyr is a duplicate of
        # CONFIG_RIMAGE_SIGNING_SCHEMA in SOF.
        target = cache.get('RIMAGE_TARGET')

        if not target:
            msg = 'rimage target not defined in board.cmake'
            if args.if_tool_available:
                command.inf(msg)
                sys.exit(0)
            else:
                command.die(msg)

        kernel_name = build_conf.get('CONFIG_KERNEL_BIN_NAME', 'zephyr')

        # TODO: make this a new sign.py --bootloader option.
        if target in ('imx8', 'imx8m', 'imx8ulp', 'imx95'):
            bootloader = None
            kernel = str(b / 'zephyr' / f'{kernel_name}.elf')
            out_bin = str(b / 'zephyr' / f'{kernel_name}.ri')
            out_xman = str(b / 'zephyr' / f'{kernel_name}.ri.xman')
            out_tmp = str(b / 'zephyr' / f'{kernel_name}.rix')
        else:
            bootloader = str(b / 'zephyr' / 'boot.mod')
            kernel = str(b / 'zephyr' / 'main.mod')
            out_bin = str(b / 'zephyr' / f'{kernel_name}.ri')
            out_xman = str(b / 'zephyr' / f'{kernel_name}.ri.xman')
            out_tmp = str(b / 'zephyr' / f'{kernel_name}.rix')

        # Clean any stale output. This is especially important when using --if-tool-available
        # (but not just)
        for o in [ out_bin, out_xman, out_tmp ]:
            pathlib.Path(o).unlink(missing_ok=True)

        tool_path = (
            args.tool_path if args.tool_path else
            command.config_get('rimage.path', None)
        )
        err_prefix = '--tool-path' if args.tool_path else 'west config'

        if tool_path:
            command.check_force(shutil.which(tool_path),
                                f'{err_prefix} {tool_path}: not an executable')
        else:
            tool_path = shutil.which('rimage')
            if not tool_path:
                err_msg = 'rimage not found; either install it or provide --tool-path'
                if args.if_tool_available:
                    command.wrn(err_msg)
                    command.wrn('zephyr binary _not_ signed!')
                    return
                else:
                    command.die(err_msg)

        #### -c sof/rimage/config/signing_schema.toml  ####

        if not args.quiet:
            command.inf('Signing with tool {}'.format(tool_path))

        try:
            sof_proj = command.manifest.get_projects(['sof'], allow_paths=False)
            sof_src_dir = pathlib.Path(sof_proj[0].abspath)
        except ValueError: # sof is the manifest
            sof_src_dir = pathlib.Path(manifest.manifest_path()).parent

        self.sof_src_dir = sof_src_dir


        command.inf('Signing for SOC target ' + target)

        # FIXME: deprecate --no-manifest and replace it with a much
        # simpler and more direct `-- -e` which the user can _already_
        # pass today! With unclear consequences right now...
        if '--no-manifest' in args.tool_args:
            no_manifest = True
            args.tool_args.remove('--no-manifest')
        else:
            no_manifest = False

        # Non-SOF build does not have extended manifest data for
        # rimage to process, which might result in rimage error.
        # So skip it when not doing SOF builds.
        is_sof_build = build_conf.getboolean('CONFIG_SOF')
        if not is_sof_build:
            no_manifest = True

        if no_manifest:
            extra_ri_args = [ ]
        else:
            extra_ri_args = ['-e']

        sign_base = [tool_path]

        # Align rimage verbosity.
        # Sub-command arg 'west sign -q' takes precedence over west '-v'
        if not args.quiet and args.verbose:
            sign_base += ['-v'] * args.verbose

        components = [ ] if bootloader is None else [ bootloader ]
        components += [ kernel ]

        sign_config_extra_args = command.config_get_words('rimage.extra-args', [])

        if '-k' not in sign_config_extra_args + args.tool_args:
            # rimage requires a key argument even when it does not sign
            cmake_default_key = cache.get('RIMAGE_SIGN_KEY', 'key placeholder from sign.py')
            extra_ri_args += [ '-k', str(sof_src_dir / 'keys' / cmake_default_key) ]

        if args.tool_data and '-c' in args.tool_args:
            command.wrn('--tool-data ' + args.tool_data + ' ignored! Overridden by: -- -c ... ')

        if '-c' not in sign_config_extra_args + args.tool_args:
            conf_dir = self.rimage_config_dir()
            toml_basename = target + '.toml'
            if ((conf_dir / toml_basename).exists() and
               (conf_dir / (toml_basename + '.h')).exists()):
                command.die(f"Cannot have both {toml_basename + '.h'} and {toml_basename} in {conf_dir}")

            if (conf_dir / (toml_basename + '.h')).exists():
                generated_subdir = pathlib.Path('zephyr') / 'misc' / 'generated'
                self.preprocess_toml(conf_dir, toml_basename, generated_subdir)
                extra_ri_args += ['-c', str(b / generated_subdir / 'rimage_config.toml')]
            else:
                toml_dir = conf_dir
                extra_ri_args += ['-c', str(toml_dir / toml_basename)]

        # Warning: while not officially supported (yet?), the rimage --option that is last
        # on the command line currently wins in case of duplicate options. So pay
        # attention to the _args order below.
        sign_base += (['-o', out_bin] + sign_config_extra_args +
                      extra_ri_args + args.tool_args + components)

        command.inf(quote_sh_list(sign_base))
        subprocess.check_call(sign_base)

        if no_manifest:
            filenames = [out_bin]
        else:
            filenames = [out_xman, out_bin]
            if not args.quiet:
                command.inf('Prefixing ' + out_bin + ' with manifest ' + out_xman)
        with open(out_tmp, 'wb') as outfile:
            for fname in filenames:
                with open(fname, 'rb') as infile:
                    outfile.write(infile.read())

        os.remove(out_bin)
        os.rename(out_tmp, out_bin)
