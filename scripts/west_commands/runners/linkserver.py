# Copyright 2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#
# Based on jlink.py

'''Runner for debugging with NXP's LinkServer.'''

import logging
import os
from pathlib import Path
import shlex
import subprocess
import sys

from runners.core import ZephyrBinaryRunner, RunnerCaps


DEFAULT_LINKSERVER_EXE = 'Linkserver.exe' if sys.platform == 'win32' else 'LinkServer'
DEFAULT_LINKSERVER_GDB_PORT =  3333
DEFAULT_LINKSERVER_SEMIHOST_PORT = 3334

#class ToggleAction(argparse.Action):
#
#    def __call__(self, parser, args, ignored, option):
#        setattr(args, self.dest, not option.startswith('--no-'))

class LinkServerBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for NXP Linkserver'''
    def __init__(self, cfg, device,
                 linkserver=DEFAULT_LINKSERVER_EXE,
                 dt_flash=True, erase=True,
                 probe=1,
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

    #@classmethod
    #def dev_id_help(cls) -> str:
    #    return '''Device identifier. Use it to select the probe
    #              of the device connected over USB.'''

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--device', required=True, help='device name')

        parser.add_argument('--probe', default=1,
                            help='interface to use (index, no serial number), default is 1')

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
        return LinkServerBinaryRunner(cfg, args.device,
                                 linkserver=args.linkserver,
                                 dt_flash=args.dt_flash,
                                 erase=args.erase,
                                 probe=args.probe,
                                 semihost_port=args.semihost_port,
                                 gdb_port=args.gdb_port,
                                 override=args.override,
                                 tui=args.tui, tool_opt=args.tool_opt)

## UPDATE
    def print_gdbserver_message(self):
        if not self.thread_info_enabled:
            thread_msg = '; no thread info available'
        elif self.supports_thread_info:
            thread_msg = '; thread info enabled'
        else:
            thread_msg = '; update J-Link software for thread info'
        self.logger.info('J-Link GDB server running on port '
                         f'{self.gdb_port}{thread_msg}')

    @property
    def linkserver_version(self):

        if not hasattr(self, '_linkserver_version'):
            linkserver_version_cmd=[self.linkserver, "-v"]
            ls_output=self.check_output(linkserver_version_cmd)
            self._linkserver_version = str(ls_output.split()[1].decode())

        return self._linkserver_version

    @property
    def linkserver_version_str(self):

        return self.linkserver_version


    @property
    def supports_thread_info(self):
        # TODO: check if linkserver needs this??
        return True

    def do_run(self, command, **kwargs):

        # Convert commander to a real absolute path. We need this to
        # be able to find the shared library that tells us what
        # version of the tools we're using.
        self.linkserver = os.fspath(
            Path(self.require(self.linkserver)).resolve())
        self.logger.info(f'LinkServer Version: {self.linkserver_version_str}')

        #rtos = self.thread_info_enabled and self.supports_thread_info

        if command == 'flash':
            self.flash(**kwargs)
        else:
            linkserver_cmd = ([self.linkserver] +
                              ["gdbserver"]    +
                              ["--probe", "#"+str(self.probe) ] +
                              ["--gdb-port", str(self.gdb_port )] +
                              ["--semihost-port", str(self.semihost_port) ] +
			      self.override_cli +
                              [self.device])

            if command in ('debug', 'attach'):
            # self.print_gdbserver_message()
                if self.elf_name is  None or not os.path.isfile(self.elf_name):
                    raise ValueError('Cannot debug; elf file required')

                gdb_cmd = ([self.gdb_cmd] +
                           self.tui_arg +
                          [self.elf_name] +[
                          '-ex', 'target remote {}:{}'.format(self.gdb_host, self.gdb_port)])

                if command == 'debug':
                    gdb_cmd += [ '-ex', 'load', '-ex', 'monitor reset']

                if command == 'attach':
                    linkserver_cmd += ['--attach']

                self.run_server_and_client(linkserver_cmd, gdb_cmd)

            elif command == 'debugserver':
                if self.gdb_host:
                    raise ValueError('Cannot run debugserver with --gdb-host')

                self.check_call(linkserver_cmd)

            return

    def do_erase(self, **kwargs):

        linkserver_cmd = ([self.linkserver, "flash"] + ["--probe", "#"+str(self.probe)] +
                          [self.device] + ["erase"])
        self.logger.debug("flash erase command = " + str(linkserver_cmd))
        self.check_call(linkserver_cmd)

    def _build_override_cli(self):

        override_cli = []

        if self.override is not None:
            for ov in self.override:
                override_cli = (override_cli + [ "-o", str(ov)])

        return override_cli

    def flash(self, **kwargs):

        linkserver_cmd = ([self.linkserver, "flash"] + ["--probe", "#"+str(self.probe)] + self.override_cli + [self.device])

        if self.erase:
            self.do_erase()

        if self.bin_name is not None and os.path.isfile(self.bin_name):
            if self.dt_flash:
                load_addr = self.flash_address_from_build_conf(self.build_conf)
            else:
                self.logger.critical("no load flash address could be found...")
                raise RuntimeError("missing argument")

            flash_cmd = (["load", "--addr", str(load_addr), self.bin_name])
        else:
            err = 'Cannot flash; no bin ({}) file found.'
            raise ValueError(err.format(self.bin_name))

        # Flash the selected elf file
        linkserver_cmd = linkserver_cmd + flash_cmd
        self.logger.debug("flash command = " + str(linkserver_cmd))
        kwargs = {}
        if not self.logger.isEnabledFor(logging.DEBUG):
            kwargs['stderr'] = subprocess.DEVNULL
        self.check_call(linkserver_cmd, **kwargs)
