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
import shlex
import subprocess
import sys
import intelhex

from west import log
from west import manifest
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

The "ARGS_FOR_YOUR_TOOL" value can be any additional
arguments you want to pass to the tool, such as the location of a
signing key, a version identifier, etc.

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

The image header size, alignment, and slot sizes are determined from
the build directory using .config and the device tree. A default
version number of 0.0.0+0 is used (which can be overridden by passing
"--version x.y.z+w" after "--key"). As shown above, extra arguments
after a '--' are passed to imgtool directly.

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


def config_get_words(west_config, section_key, fallback=None):
    unparsed = west_config.get(section_key)
    log.dbg(f'west config {section_key}={unparsed}')
    return fallback if unparsed is None else shlex.split(unparsed)


def config_get(west_config, section_key, fallback=None):
    words = config_get_words(west_config, section_key)
    if words is None:
        return fallback
    if len(words) != 1:
        log.die(f'Single word expected for: {section_key}={words}. Use quotes?')
    return words[0]


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
                           are currently supported''')
        group.add_argument('-p', '--tool-path', default=None,
                           help='''path to the tool itself, if needed''')
        group.add_argument('-D', '--tool-data', default=None,
                           help='''path to a tool-specific data/configuration directory, if needed''')
        group.add_argument('--if-tool-available', action='store_true',
                           help='''Do not fail if rimage is missing, just warn.''')
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
            args.tool = config_get(self.config, 'sign.tool')

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
                log.die('imgtool does not support --if-tool-available')
            signer = ImgtoolSigner()
        elif args.tool == 'rimage':
            signer = RimageSigner()
        # (Add support for other signers here in elif blocks)
        else:
            if args.tool is None:
                log.die('one --tool is required')
            else:
                log.die(f'invalid tool: {args.tool}')

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

    class DTPartition:
        '''DT partition parameters'''

        @staticmethod
        def _is_partition(node):
            '''Check if node is partition

            :param node: partition node to check
            :type node: Node
            :return: True if node is 'fixed-partition' child node
            :rtype: Bool
            '''

            if node.parent.matching_compat == 'fixed-partitions':
                return True
            return False

        def __init__(self, node):
            if not __class__._is_partition(node):
                log.die(f"{node} is not 'fixed-partition child node")

            self.node = node
            self.labels = node.labels

        @staticmethod
        def _get_flash(partition):
            '''Get flash node where the partition is placed

            :param partition: partition node to get flash node for
            :type partition: Node
            :returns: DT node for flash device
            :rtype: Node
            '''

            return partition.parent.parent

        @staticmethod
        def _is_flash_internal(node):
            '''Check if flash device is internal SoC flash

            :param node: flash node to check
            :type node: Node
            :returns: True if flash device is internal SoC flash
            :type: Bool
            '''

            if 'soc-nv-flash' in node.parent.compats:
                return True
            return False

        @staticmethod
        def _get_flash_param(flash, key, default_val=0):
            '''Get flash parameter or default

            :param flash: flash node to get flash parameter for
            :type partition: Node
            :param key: flash parameter name
            :type key: str
            :param default_val: default value when not found
            :type default_val: int
            '''

            try:
                return flash.props[key].val
            except KeyError:
                if not __class__._is_flash_internal(flash):
                    log.wrn(f'Could not get {key} value for {flash}'
                            f' defaulting to {default_val}')
            return default_val

        def get_parameters(self):
            '''Get partition parameters: offset from flash device beginning and size

            :returns: (address, size)
            :type: (int, int)
            '''

            if not self.node.regs:
                log.die(f"${self.node} flash partition has no regs property;",
                        "can't determine size of slot")

            addr = self.node.regs[0].addr
            size = self.node.regs[0].size
            if size == 0:
                log.die(f'expected non-zero slot size for ${self.node}')

            return (addr, size)


        def is_internal(self):
            '''Check if partition is on SoC flash

            :returns: True if partition is on internal (SoC) flash
            :rtype: Bool
            '''

            return __class__._is_flash_internal(self.node.parent)

        def flash_parameters(self):
            '''Get write alignment and erase block size for flash under
               partition.

            :returns: (write alignment, erase block size)
            :rtype: (int, int)
            '''

            flash = __class__._get_flash(self.node)
            wap = __class__._get_flash_param(flash, 'write-block-size')
            ebs = __class__._get_flash_param(flash, 'erase-block-size')

            return (wap, ebs)

    class MCUbootDTConfig:

        def __get_dt(self, build_dir_path):
            '''Read device tree pickle

            :param build_dir_path: path to a pickle to read
            :type build_dir_path: Path or string representing Path
            :returns: DT tree object
            :rtype: EDT
            '''

            dts = build_dir_path / 'zephyr' / 'zephyr.dts'
            if not self.quiet:
                log.dbg('DTS file:', dts, level=log.VERBOSE_VERY)
            edt_pickle = build_dir_path / 'zephyr' / 'edt.pickle'
            if not edt_pickle.is_file():
                log.die("can't load devicetree; expected to find:", edt_pickle)

            # Load the devicetree.
            with open(edt_pickle, 'rb') as f:
                edt = pickle.load(f)

            return edt

        def __init__(self, build_dir_path, quiet=False):
            self.quiet = quiet
            self.edt = self.__get_dt(build_dir_path)


        def get_partition(self, dt_label):
            '''Get code partition Node by DT label.

            :param dt_label: DT node label
            :type dt_label: str
            :returns: DT node with given label
            :rtype: Node
            '''

            try:
                node = self.edt.label2node[dt_label]
            except KeyError:
                node = None

            if node:
                return ImgtoolSigner.DTPartition(node)

            return None

        def get_code_partition(self):
            '''Get app boot partition which is chosen zephyr,code-partition Node

            :returns: partition object
            :rtype: DTPartition
            '''

            node = self.edt.chosen_node('zephyr,code-partition')

            if not node:
                log.die("devicetree has no chosen zephyr,code-partition node;",
                        " can not validate application partition")

            partition = ImgtoolSigner.DTPartition(node)

            if not partition.is_internal():
                log.die("west sign currently does not support signing image"
                        " built for code-partition that has not been placed"
                        " on internal flash")

            return partition

    def __init__(self, quiet=False):
        self.quiet = quiet

    @staticmethod
    def mcuboot_magic_size(flash_alignment=8):
        '''Get MCUboot magic size

        Magic size is is 16 octets aligned to flash write alignment (min. 8) + 3 flags,
        each 1 octet and each separately aligned to flash write alignment.

        :parameter flash_alignment: flash device write alignment
        :type flash_alignment: int
        :returns: size of MCUboot magic
        :rtype: int
        '''

        return (int((16 + flash_alignment - 1) / flash_alignment) + 3) * flash_alignment

    @staticmethod
    def mcuboot_check_alignment(alignment):
        '''Minimal MCUboot write alignment is 8 bytes, maximum is 32

        :parameter alignment: flash alignment
        :type alignment: int
        :returns: alignment, corrected if required
        :rtype: int
        '''

        if alignment < 8:
            return 8

        if alignment > 32:
            log.die("MCUboot does not work with flash with write alignment > 32")

        return alignment

    @staticmethod
    def mcuboot_swap_validate(mcbdtconf, bin_size, quiet=False):
        '''Check whether DT configuration of partitions is valid for MCUboot swap-move algorithm.

        If configuration is valid, returns parameters needed by imgtool sign command.

        :param mcbdtconf: MCUboot configuration reader
        :type: MCUbootDTConfig
        :param bin_size: size of application binary; this is size application will take
                         on flash.
        :type bin_size: int
        :param quiet: suppress extra diagnostic information
        :type quiet: Bool
        :returns: (flash alignment, application image offset on flash, image size)
        :rtype: (int, int, int)
        '''

        code = mcbdtconf.get_code_partition()

        if 'slot0_partition' not in code.labels:
            log.die("west sign, for MCUboot in swap mode, expects DT chosen"
                    " zephyr,code-partition to point to node labeled"
                    " slot0_partition. Currently selected node is labeled"
                    " " + str(code.labels))

        if 'slot1_partition' in code.labels:
            log.die("chosen zephyr,code-partition may not have slot0_partition"
                    " and slot1_partition at the same time")

        (write_alignment, erase_size) = code.flash_parameters()

        if write_alignment == 0:
            log.die('expected non-zero flash alignment, but got'
                    f' DT flash device write-block-size {write_alignment} for {code}')

        write_alignment = __class__.mcuboot_check_alignment(write_alignment)

        cp_addr, cp_size = code.get_parameters()

        other = mcbdtconf.get_partition('slot1_partition')

        if other:
            _, op_size = other.get_parameters()
            # Always go with smaller partition size, as swap will not be possible
            if op_size != cp_size:
                log.wrn("slot0_partition and slot1_partition differ in size:"
                        f" ${cp_size} vs ${op_size}. Smaller size will be used.")
                if op_size < cp_size:
                    cp_size = op_size

            bin_size_wm = bin_size + __class__.mcuboot_magic_size(write_alignment)

            # Binary size can not be greater than slot size without last page,
            # as the last page is used for move algorithm.
            if bin_size_wm > (cp_size - erase_size):
                log.die("Application image with added MCUboot magic will not fit"
                        " in application slot, for swap-move configured MCUboot.\n"
                        f"Binary size is {bin_size}, with magic it is {bin_size_wm}.\n"
                        f"Max allowed slot size is {cp_size} - {erase_size}")
        else:
            log.wrn('slot1_partition not found, assuming single slot configuration')

        return (write_alignment, cp_addr, cp_size)

    @staticmethod
    def _get_app_size(path_to_bin, path_to_hex):
        '''Get file size from binary or hex, or both for comparison

           :parameter path_to_bin: path to .bin file
           :type: '''

        bin_size = 0
        hex_size = 0

        if path_to_bin:
            bin_size = os.path.getsize(path_to_bin)

        if path_to_hex:
            ih = intelhex.IntelHex(source=path_to_hex)
            hex_size = ih.maxaddr() - ih.minaddr() + 1

        if path_to_hex and path_to_hex and bin_size != hex_size:
            log.die("Application size calculated of bin and hex differ:"
                    f" {bin_size} vs {hex_size}")

        return bin_size if path_to_bin else hex_size

    def sign(self, command, build_dir, build_conf, formats):
        if not formats:
            return

        args = command.args
        b = pathlib.Path(build_dir)

        imgtool = self.find_imgtool(command, args)

        if not build_conf.getboolean('CONFIG_BOOTLOADER_MCUBOOT'):
            log.wrn("CONFIG_BOOTLOADER_MCUBOOT is not set to y in "
                    f"{build_conf.path}; this probably won't work")

        kernel = build_conf.get('CONFIG_KERNEL_BIN_NAME', 'zephyr')

        if 'bin' in formats:
            in_bin = b / 'zephyr' / f'{kernel}.bin'
            if not in_bin.is_file():
                log.die(f"no unsigned .bin found at {in_bin}")
            in_bin = os.fspath(in_bin)
        else:
            in_bin = None
        if 'hex' in formats:
            in_hex = b / 'zephyr' / f'{kernel}.hex'
            if not in_hex.is_file():
                log.die(f"no unsigned .hex found at {in_hex}")
            in_hex = os.fspath(in_hex)
        else:
            in_hex = None

        bin_size = __class__._get_app_size(in_bin, in_hex)

        # The vector table offset is set in Kconfig:
        vtoff = self.get_cfg(command, build_conf, 'CONFIG_ROM_START_OFFSET')
        # Flash device write alignment and the partition's slot size
        # come from devicetree:
        mcuboot_dtconf = self.MCUbootDTConfig(b, self.quiet)
        align, addr, size = self.mcuboot_swap_validate(mcuboot_dtconf, bin_size, self.quiet)

        if not args.quiet:
            log.banner('image configuration:')
            log.inf('partition offset: {0} (0x{0:x})'.format(addr))
            log.inf('partition size: {0} (0x{0:x})'.format(size))
            log.inf('rom start offset: {0} (0x{0:x})'.format(vtoff))

        # Base sign command.
        #
        # We provide a default --version in case the user is just
        # messing around and doesn't want to set one. It will be
        # overridden if there is a --version in args.tool_args.
        sign_base = imgtool + ['sign',
                               '--version', '0.0.0+0',
                               '--align', str(align),
                               '--header-size', str(vtoff),
                               '--slot-size', str(size)]
        sign_base.extend(args.tool_args)

        if not args.quiet:
            log.banner('signing binaries')
        if in_bin:
            out_bin = args.sbin or str(b / 'zephyr' / 'zephyr.signed.bin')
            sign_bin = sign_base + [in_bin, out_bin]
            if not args.quiet:
                log.inf(f'unsigned bin: {in_bin}')
                log.inf(f'signed bin:   {out_bin}')
                log.dbg(quote_sh_list(sign_bin))
            subprocess.check_call(sign_bin)
        if in_hex:
            out_hex = args.shex or str(b / 'zephyr' / 'zephyr.signed.hex')
            sign_hex = sign_base + [in_hex, out_hex]
            if not args.quiet:
                log.inf(f'unsigned hex: {in_hex}')
                log.inf(f'signed hex:   {out_hex}')
                log.dbg(quote_sh_list(sign_hex))
            subprocess.check_call(sign_hex)

    @staticmethod
    def find_imgtool(command, args):
        if args.tool_path:
            imgtool = args.tool_path
            if not os.path.isfile(imgtool):
                log.die(f'--tool-path {imgtool}: no such file')
        else:
            imgtool = shutil.which('imgtool') or shutil.which('imgtool.py')
            if not imgtool:
                log.die('imgtool not found; either install it',
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


class RimageSigner(Signer):

    def sign(self, command, build_dir, build_conf, formats):
        args = command.args

        b = pathlib.Path(build_dir)
        cache = CMakeCache.from_build_dir(build_dir)

        # warning: RIMAGE_TARGET is a duplicate of CONFIG_RIMAGE_SIGNING_SCHEMA
        target = cache.get('RIMAGE_TARGET')
        if not target:
            log.die('rimage target not defined')

        if target in ('imx8', 'imx8m'):
            kernel = str(b / 'zephyr' / 'zephyr.elf')
            out_bin = str(b / 'zephyr' / 'zephyr.ri')
            out_xman = str(b / 'zephyr' / 'zephyr.ri.xman')
            out_tmp = str(b / 'zephyr' / 'zephyr.rix')
        else:
            bootloader = str(b / 'zephyr' / 'boot.mod')
            kernel = str(b / 'zephyr' / 'main.mod')
            out_bin = str(b / 'zephyr' / 'zephyr.ri')
            out_xman = str(b / 'zephyr' / 'zephyr.ri.xman')
            out_tmp = str(b / 'zephyr' / 'zephyr.rix')

        # Clean any stale output. This is especially important when using --if-tool-available
        # (but not just)
        for o in [ out_bin, out_xman, out_tmp ]:
            pathlib.Path(o).unlink(missing_ok=True)

        tool_path = (
            args.tool_path if args.tool_path else
            config_get(command.config, 'rimage.path', None)
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
                    log.wrn(err_msg)
                    log.wrn('zephyr binary _not_ signed!')
                    return
                else:
                    log.die(err_msg)

        #### -c sof/rimage/config/signing_schema.toml  ####

        cmake_toml = target + '.toml'

        if not args.quiet:
            log.inf('Signing with tool {}'.format(tool_path))

        try:
            sof_proj = command.manifest.get_projects(['sof'], allow_paths=False)
            sof_src_dir = pathlib.Path(sof_proj[0].abspath)
        except ValueError: # sof is the manifest
            sof_src_dir = pathlib.Path(manifest.manifest_path()).parent

        if '-c' in args.tool_args:
            # Precedence to the arguments passed after '--': west sign ...  -- -c ...
            if args.tool_data:
                log.wrn('--tool-data ' + args.tool_data + ' ignored, overridden by: -- -c ... ')
            conf_dir = None
        elif args.tool_data:
            conf_dir = pathlib.Path(args.tool_data)
        elif cache.get('RIMAGE_CONFIG_PATH'):
            conf_dir = pathlib.Path(cache['RIMAGE_CONFIG_PATH'])
        else:
            conf_dir = sof_src_dir / 'rimage' / 'config'

        conf_path_cmd = ['-c', str(conf_dir / cmake_toml)] if conf_dir else []

        log.inf('Signing for SOC target ' + target)

        # FIXME: deprecate --no-manifest and replace it with a much
        # simpler and more direct `-- -e` which the user can _already_
        # pass today! With unclear consequences right now...
        if '--no-manifest' in args.tool_args:
            no_manifest = True
            args.tool_args.remove('--no-manifest')
        else:
            no_manifest = False

        if no_manifest:
            extra_ri_args = [ ]
        else:
            extra_ri_args = ['-e']

        sign_base = [tool_path]

        # Sub-command arg '-q' takes precedence over west '-v'
        if not args.quiet and args.verbose:
            sign_base += ['-v'] * args.verbose

        components = [ ] if (target in ('imx8', 'imx8m')) else [ bootloader ]
        components += [ kernel ]

        sign_config_extra_args = config_get_words(command.config, 'rimage.extra-args', [])

        if '-k' not in sign_config_extra_args + args.tool_args:
            cmake_default_key = cache.get('RIMAGE_SIGN_KEY')
            extra_ri_args += [ '-k', str(sof_src_dir / 'keys' / cmake_default_key) ]

        # Warning: while not officially supported (yet?), the rimage --option that is last
        # on the command line currently wins in case of duplicate options. So pay
        # attention to the _args order below.
        sign_base += (['-o', out_bin] + sign_config_extra_args + conf_path_cmd +
                      extra_ri_args + args.tool_args + components)

        if not args.quiet:
            log.inf(quote_sh_list(sign_base))
        subprocess.check_call(sign_base)

        if no_manifest:
            filenames = [out_bin]
        else:
            filenames = [out_xman, out_bin]
            if not args.quiet:
                log.inf('Prefixing ' + out_bin + ' with manifest ' + out_xman)
        with open(out_tmp, 'wb') as outfile:
            for fname in filenames:
                with open(fname, 'rb') as infile:
                    outfile.write(infile.read())

        os.remove(out_bin)
        os.rename(out_tmp, out_bin)
