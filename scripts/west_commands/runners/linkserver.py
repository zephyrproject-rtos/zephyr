# Copyright 2023-2024 NXP
# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0
#
# Based on jlink.py

'''Runner for debugging with NXP's LinkServer.'''

import logging
import os
import shlex
import subprocess
import sys

from runners.core import ZephyrBinaryRunner, RunnerCaps


DEFAULT_LINKSERVER_EXE = 'Linkserver.exe' if sys.platform == 'win32' else 'LinkServer'
DEFAULT_LINKSERVER_GDB_PORT =  3333
DEFAULT_LINKSERVER_SEMIHOST_PORT = 8888

class LinkServerBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for NXP Linkserver'''
    def __init__(self, cfg, device, core,
                 linkserver=DEFAULT_LINKSERVER_EXE,
                 dt_flash=True, erase=True,
                 probe='#1',
                 gdb_host='',
                 gdb_port=DEFAULT_LINKSERVER_GDB_PORT,
                 semihost_port=DEFAULT_LINKSERVER_SEMIHOST_PORT,
                 override=[],
                 tui=False, tool_opt=[]):
        super().__init__(cfg)
        self.file = cfg.file
        self.file_type = cfg.file_type
        self.hex_name = cfg.hex_file
        self.bin_name = cfg.bin_file
        self.elf_name = cfg.elf_file
        self.gdb_cmd = cfg.gdb if cfg.gdb else None
        self.device = device
        self.core = core
        self.linkserver = linkserver
        self.dt_flash = dt_flash
        self.erase = erase
        self.probe = probe
        self.gdb_host = gdb_host
        self.gdb_port = gdb_port
        self.semihost_port = semihost_port
        self.tui_arg = ['-tui'] if tui else []
        self.override = override
        self.override_cli = self._build_override_cli()

        self.tool_opt = []
        for opts in [shlex.split(opt) for opt in tool_opt]:
            self.tool_opt += opts

    @classmethod
    def name(cls):
        return 'linkserver'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'debugserver', 'attach'},
                          dev_id=True, flash_addr=True, erase=True,
                          tool_opt=True, file=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--device', required=True, help='device name')

        parser.add_argument('--core', required=False, help='core of the device')

        parser.add_argument('--probe', default='#1',
                            help='interface to use (index, or serial number, default is #1')

        parser.add_argument('--tui', default=False, action='store_true',
                            help='if given, GDB uses -tui')

        parser.add_argument('--gdb-port', default=DEFAULT_LINKSERVER_GDB_PORT,
                            help='gdb port to open, defaults to {}'.format(
                               DEFAULT_LINKSERVER_GDB_PORT))

        parser.add_argument('--semihost-port', default=DEFAULT_LINKSERVER_SEMIHOST_PORT,
                            help='semihost port to open, defaults to the empty string '
                            'and runs a gdb server')
        # keep this, we have to assume that the default 'commander' is on PATH
        parser.add_argument('--linkserver', default=DEFAULT_LINKSERVER_EXE,
                            help=f'''LinkServer executable, default is
                            {DEFAULT_LINKSERVER_EXE}''')
        # user may need to override settings.
        parser.add_argument('--override', required=False, action='append',
                            help=f'''configuration overrides as defined bylinkserver. Example: /device/memory/0/location=0xcafecafe''')

    @classmethod
    def do_create(cls, cfg, args):

        print("RUNNER - gdb_port = " + str(args.gdb_port) + ", semih port = " + str(args.semihost_port))
        return LinkServerBinaryRunner(cfg, args.device, args.core,
                                 linkserver=args.linkserver,
                                 dt_flash=args.dt_flash,
                                 erase=args.erase,
                                 probe=args.probe,
                                 semihost_port=args.semihost_port,
                                 gdb_port=args.gdb_port,
                                 override=args.override,
                                 tui=args.tui, tool_opt=args.tool_opt)

    @property
    def linkserver_version_str(self):

        if not hasattr(self, '_linkserver_version'):
            linkserver_version_cmd=[self.linkserver, "-v"]
            ls_output=self.check_output(linkserver_version_cmd)
            self.linkserver_version = str(ls_output.split()[1].decode()).lower()

        return self.linkserver_version

    def do_run(self, command, **kwargs):

        self.linkserver = self.require(self.linkserver)
        self.logger.info(f'LinkServer: {self.linkserver}, version {self.linkserver_version_str}')

        if command == 'flash':
            self.flash(**kwargs)
        else:
            if self.core is not None:
                _cmd_core = [ "-c",  self.core ]
            else:
                _cmd_core = []

            linkserver_cmd = ([self.linkserver] +
                              ["gdbserver"]    +
                              ["--probe", str(self.probe) ] +
                              ["--gdb-port", str(self.gdb_port )] +
                              ["--semihost-port", str(self.semihost_port) ] +
                              _cmd_core +
                              self.override_cli +
                              [self.device])

            self.logger.debug(f'LinkServer cmd:  + {linkserver_cmd}')

            if command in ('debug', 'attach'):
                if self.elf_name is  None or not os.path.isfile(self.elf_name):
                    raise ValueError('Cannot debug; elf file required')

                gdb_cmd = ([self.gdb_cmd] +
                           self.tui_arg +
                           [self.elf_name] +
                           ['-ex', 'target remote {}:{}'.format(self.gdb_host, self.gdb_port)])

                if command == 'debug':
                    gdb_cmd += [ '-ex', 'load', '-ex', 'monitor reset']

                if command == 'attach':
                    linkserver_cmd += ['--attach']

                self.run_server_and_client(linkserver_cmd, gdb_cmd)

            elif command == 'debugserver':
                if self.gdb_host:
                    raise ValueError('Cannot run debugserver with --gdb-host')

                self.check_call(linkserver_cmd)

    def do_erase(self, **kwargs):

        linkserver_cmd = ([self.linkserver, "flash"] + ["--probe", str(self.probe)] +
                          [self.device] + ["erase"])
        self.logger.debug("flash erase command = " + str(linkserver_cmd))
        self.check_call(linkserver_cmd)

    def _build_override_cli(self):

        override_cli = []

        if self.override is not None:
            for ov in self.override:
                override_cli = (override_cli + ["-o", str(ov)])

        return override_cli

    def flash(self, **kwargs):

        linkserver_cmd = ([self.linkserver, "flash"] + ["--probe", str(self.probe)] + self.override_cli + [self.device])
        self.logger.debug(f'LinkServer cmd:  + {linkserver_cmd}')

        if self.erase:
            self.do_erase()

        # Use hex, bin or elf file provided by the buildsystem.
        # Preferring .hex over .bin and .elf
        if self.supports_hex() and self.hex_name is not None and os.path.isfile(self.hex_name):
            flash_cmd = (["load", self.hex_name])
        # Preferring .bin over .elf
        elif self.bin_name is not None and os.path.isfile(self.bin_name):
            if self.dt_flash:
                load_addr = self.flash_address_from_build_conf(self.build_conf)
            else:
                self.logger.critical("no load flash address could be found...")
                raise RuntimeError("no load flash address could be found...")

            flash_cmd = (["load", "--addr", str(load_addr), self.bin_name])
        elif self.elf_name is not None and os.path.isfile(self.elf_name):
            flash_cmd = (["load", self.elf_name])
        else:
            err = 'Cannot flash; no hex ({}), bin ({}) or elf ({}) files found.'
            raise ValueError(err.format(self.hex_name, self.bin_name, self.elf_name))

        # Flash the selected file
        linkserver_cmd = linkserver_cmd + flash_cmd
        self.logger.debug("flash command = " + str(linkserver_cmd))
        kwargs = {}
        if not self.logger.isEnabledFor(logging.DEBUG):
            if self.linkserver_version_str < "v1.3.15":
                kwargs['stderr'] = subprocess.DEVNULL
            else:
                kwargs['stdout'] = subprocess.DEVNULL

        self.check_call(linkserver_cmd, **kwargs)

    def supports_hex(self):
        # v1.5.30 has added flash support for Intel Hex files.
        return self.linkserver_version_str >= "v1.5.30"
