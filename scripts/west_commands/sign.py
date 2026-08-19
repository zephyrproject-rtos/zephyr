# Copyright (c) 2018 Foundries.io
#
# SPDX-License-Identifier: Apache-2.0

import abc
import argparse
import os
import pathlib
import shutil
import subprocess
import sys

from elftools.elf.elffile import ELFFile

from west import manifest
from west.util import quote_sh_list

from build_helpers import BUILD_HELPERS_LOGGER, find_build_dir, is_zephyr_build, \
    forward_logging_to_west, FIND_BUILD_DIR_DESCRIPTION
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

silabs_commander
----------------

To create a signed binary with the silabs_commander tool, run this from your
build directory:

   west sign -t silabs_commander -- [--sign PRIVATE.pem] [--encrypt KEY] [--mic KEY]

For this to work, either "commander" must be installed or you must pass
the path to "commander" using the -p option.

If an argument is not specified, the value provided by Kconfig
(CONFIG_SIWX91X_SIGN_KEY, CONFIG_SIWX91X_MIC_KEY and CONFIG_SIWX91X_ENCRYPT)
is used.

The exact behavior of these option are described in Silabs UG574[1] or in the
output of "commander rps converter --help"

[1]: https://www.silabs.com/documents/public/user-guides/ug574-siwx917-soc-manufacturing-utility-user-guide.pdf
'''

class Sign(Forceable):
    def __init__(self):
        super(Sign, self).__init__(
            'sign',
            '',
            description=SIGN_DESCRIPTION,
            accepts_unknown_args=False)

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            epilog=SIGN_EPILOG,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description)

        parser.add_argument('-d', '--build-dir',
                            help=FIND_BUILD_DIR_DESCRIPTION)
        parser.add_argument('-q', '--quiet', action='store_true',
                            help='suppress non-error output')
        self.add_force_arg(parser)

        # general options
        group = parser.add_argument_group('tool control options')
        group.add_argument('-t', '--tool', choices=['picotool', 'rimage', 'silabs_commander'],
                           help='''image signing tool name; picotool, rimage and silabs_commander
                           are currently supported''')
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
        group.add_argument('--bin', dest='gen_bin', action=argparse.BooleanOptionalAction,
                           help='''produce a signed .bin file?
                           (default: yes, if supported and unsigned bin
                           exists)''')
        group.add_argument('-B', '--sbin', metavar='BIN',
                           help='''signed .bin file name
                           (default: zephyr.signed.bin in the build
                           directory, next to zephyr.bin)''')

        # hex file options
        group = parser.add_argument_group('Intel HEX (.hex) file options')
        group.add_argument('--hex', dest='gen_hex', action=argparse.BooleanOptionalAction,
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
        # Forward debug output from the build_helpers/zcmake module
        # loggers so it is visible under "west -v" / "west -vv".
        forward_logging_to_west(self, [BUILD_HELPERS_LOGGER, 'zcmake'])

        # Find the build directory and parse .config and DT.
        build_dir = find_build_dir(args.build_dir, config=self.config)
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
        if args.tool == 'picotool':
            signer = PicotoolSigner()
        elif args.tool == 'rimage':
            signer = RimageSigner()
        elif args.tool == 'silabs_commander':
            signer = CommanderSigner()
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


# Resolve a path to a tool binary using either --tool-path or the `which` utility
def get_tool_path(command, tool_name):
    if command.args.tool_path:
        tool = command.args.tool_path
        if not os.path.isfile(tool):
            command.die(f'--tool-path {tool}: no such file')
    else:
        tool = shutil.which(tool_name)
        if not tool:
            command.die(f'"{tool_name}" not found; either make it available on PATH or provide --tool-path')
    return tool

# This function returns the path to build result, without the file extension
def get_kernel_bin_name(build_conf):
    return build_conf.get('CONFIG_KERNEL_BIN_NAME', "zephyr")


class RimageSigner(Signer):

    def rimage_config_dir(self):
        'Returns the rimage/config/ directory with the highest precedence'
        args = self.command.args
        if args.tool_data:
            conf_dir = pathlib.Path(args.tool_data)
        elif self.cmake_cache.get('RIMAGE_CONFIG_PATH'):
            conf_dir = pathlib.Path(self.cmake_cache['RIMAGE_CONFIG_PATH'])
        elif self.sof_src_dir:
            conf_dir = self.sof_src_dir / 'tools' / 'rimage' / 'config'
        else:
            conf_dir = pathlib.Path(self.cmake_cache['BOARD_DIR']) / 'support'
        self.command.dbg(f'rimage config directory={conf_dir}')
        return conf_dir

    def generate_uuid_registry(self):
        'Runs the uuid-registry.h generator script'

        generate_cmd = [sys.executable, str(self.sof_src_dir / 'scripts' / 'gen-uuid-reg.py'),
                        str(self.sof_src_dir / 'uuid-registry.txt'),
                        str(pathlib.Path('zephyr') / 'include' / 'generated' / 'uuid-registry.h')
                       ]

        self.command.inf(quote_sh_list(generate_cmd))
        subprocess.run(generate_cmd, check=True, cwd=self.build_dir)

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
        preproc_cmd += ['-imacros',
                        str(pathlib.Path('zephyr') / 'include' / 'generated' / 'uuid-registry.h')]

        # Need to preprocess the TOML file twice: once with
        # LLEXT_FORCE_ALL_MODULAR defined and once without it
        full_preproc_cmd = preproc_cmd + ['-o', str(subdir / 'rimage_config_full.toml'), '-DLLEXT_FORCE_ALL_MODULAR']
        preproc_cmd += ['-o', str(subdir / 'rimage_config.toml')]
        self.command.inf(quote_sh_list(preproc_cmd))
        subprocess.run(preproc_cmd, check=True, cwd=self.build_dir)
        subprocess.run(full_preproc_cmd, check=True, cwd=self.build_dir)

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

        kernel_name = pathlib.Path('zephyr') / get_kernel_bin_name(build_conf)

        bootloader = None
        cold = None
        kernel = str(b / f'{kernel_name}.elf')
        out_bin = str(b / f'{kernel_name}.ri')
        out_xman = str(b / f'{kernel_name}.ri.xman')
        out_tmp = str(b / f'{kernel_name}.rix')

        # Intel platforms generate a "boot.mod" and "main.mod" as
        # separate intermediates to use.  Other platforms just use
        # zephyr.elf directly.
        if os.path.exists(str(b / 'zephyr' / 'boot.mod')):
            bootloader = str(b / 'zephyr' / 'boot.mod')
        if os.path.exists(str(b / 'zephyr' / 'cold.mod')):
            cold = str(b / 'zephyr' / 'cold.mod')
            with open(cold, 'rb') as f_cold:
                elf = ELFFile(f_cold)
                if elf.get_section_by_name('.cold') is None:
                    cold = None
        if os.path.exists(str(b / 'zephyr' / 'main.mod')):
            kernel = str(b / 'zephyr' / 'main.mod')

        # Clean any stale output. This is especially important when using --if-tool-available
        # (but not just)
        for o in [ out_bin, out_xman, out_tmp ]:
            pathlib.Path(o).unlink(missing_ok=True)

        tool_path = (
            args.tool_path if args.tool_path else
            command.config_get('rimage.path', None)
        )
        err_prefix = '--tool-path' if args.tool_path else 'west config'

        # TODO: use get_tool_path
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

        # CONFIG_RIMAGE_SIGNING_SCHEMA is only defined in SOF tree.
        # If this does not exist, we assume that we are not building SOF.
        rimage_schema = build_conf.get('CONFIG_RIMAGE_SIGNING_SCHEMA', None)
        if rimage_schema:
            self.sof_src_dir = pathlib.Path(manifest.manifest_path()).parent
            self.generate_uuid_registry()

            no_manifest = False
        else:
            self.sof_src_dir = None

            # Non-SOF build does not have extended manifest data for
            # rimage to process, which might result in rimage error.
            # So skip it when not doing SOF builds.
            no_manifest = True

        command.inf('Signing for SOC target ' + target)

        if no_manifest:
            extra_ri_args = [ ]
        else:
            extra_ri_args = ['-e']

        sign_base = [tool_path]

        # Align rimage verbosity.
        # Sub-command arg 'west sign -q' takes precedence over west '-v'
        if not args.quiet and args.verbose:
            sign_base += ['-v'] * args.verbose

        # Order is important
        components = [ ] if bootloader is None else [ bootloader ]
        if cold is not None:
            components += [ cold ]
        components += [ kernel ]

        sign_config_extra_args = command.config_get_words('rimage.extra-args', [])

        if '-k' not in sign_config_extra_args + args.tool_args:
            # rimage requires a key argument even when it does not sign
            cmake_default_key = cache.get('RIMAGE_SIGN_KEY', 'key placeholder from sign.py')
            if os.path.exists(cmake_default_key):
                extra_ri_args += [ '-k', str(cmake_default_key) ]
            else:
                if self.sof_src_dir:
                    key_path = self.sof_src_dir / 'keys'
                else:
                    key_path = self.rimage_config_dir()
                extra_ri_args += [ '-k', str(key_path / cmake_default_key) ]

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

class CommanderSigner(Signer):
    # TODO: replace with get_tool_path
    @staticmethod
    def get_tool(command):
        if command.args.tool_path:
            tool = command.args.tool_path
            if not os.path.isfile(tool):
                command.die(f'--tool-path {tool}: no such file')
        else:
            tool = shutil.which('commander')
            if not tool:
                command.die('"commander" not found; either install it or provide --tool-path')
        return tool

    @staticmethod
    def get_keys(command, build_conf):
        sign_key = getattr(command.args, 'sign',
                           build_conf.get('CONFIG_SIWX91X_SIGN_KEY', None))
        mic_key = getattr(command.args, 'mic',
                          build_conf.get('CONFIG_SIWX91X_MIC_KEY', None))
        encrypt_key = None
        if build_conf.get('CONFIG_SIWX91X_ENCRYPT', False):
            encrypt_key = mic_key
        encrypt_key = getattr(command.args, 'encrypt', encrypt_key)
        return (sign_key, mic_key, encrypt_key)

    @staticmethod
    def get_input_output(command, build_dir, build_conf):
        kernel_prefix = pathlib.Path(build_dir) / 'zephyr' / get_kernel_bin_name(build_conf)
        in_file = f'{kernel_prefix}.rps'
        out_file = command.args.sbin or f'{kernel_prefix}.signed.rps'
        return (in_file, out_file)

    def sign(self, command, build_dir, build_conf, formats):
        tool = self.get_tool(command)
        in_file, out_file = self.get_input_output(command, build_dir, build_conf)
        sign_key, mic_key, encrypt_key = self.get_keys(command, build_conf)

        commandline = [ tool, "rps", "convert", out_file, "--app", in_file ]
        if mic_key:
            commandline.extend(["--mic", mic_key])
        if encrypt_key:
            commandline.extend(["--encrypt", encrypt_key])
        if sign_key:
            commandline.extend(["--sign", sign_key])
        commandline.extend(command.args.tool_args)

        if not command.args.quiet:
            command.inf("Signing with:", ' '.join(commandline))
        subprocess.run(commandline, check=True)

class PicotoolSigner(Signer):
    @staticmethod
    def get_input_output(command, build_dir, build_conf):
        kernel_prefix = pathlib.Path(build_dir) / 'zephyr' / get_kernel_bin_name(build_conf)
        in_file = f'{kernel_prefix}.elf'
        out_file = command.args.sbin or f'{kernel_prefix}.signed.elf'
        return (in_file, out_file)

    def sign(self, command, build_dir, build_conf, formats):
        tool = get_tool_path(command, 'picotool')
        in_file, out_file = self.get_input_output(command, build_dir, build_conf)
        key_file = getattr(command.args, 'key',
                           build_conf.get('CONFIG_RPI_PICO_SIGNING_KEY', None))

        if not key_file:
            command.die('Please provide a key file using RPI_PICO_SIGNING_KEY Kconfig option')

        commandline = [ tool, "seal", "--sign", in_file, out_file, key_file ]
        commandline.extend(command.args.tool_args)

        if not command.args.quiet:
            command.inf("Signing with:", ' '.join(commandline))
        subprocess.run(commandline, check=True)
