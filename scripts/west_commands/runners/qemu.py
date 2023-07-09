# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner stub for QEMU.'''

import os
from pathlib import Path
import shlex
import sys

import yaml

from runners.core import ZephyrBinaryRunner, RunnerCaps, FileType

DEFAULT_QEMU_EXE = 'qemu-system-arm.exe' if sys.platform == 'win32' else 'qemu-system-arm'
DEFAULT_QEMU_GDB_PORT = 1234

class QemuBinaryRunner(ZephyrBinaryRunner):
    '''Place-holder for QEMU runner customizations.'''

    def __init__(self, cfg, machine, cpu, dtb, qemu_opts,
                 commander=DEFAULT_QEMU_EXE,
                 pipe=None,
                 gdbserver='',
                 gdb_host='127.0.0.1',
                 gdb_port=DEFAULT_QEMU_GDB_PORT,
                 tui=False, tool_opt=[]):
        super().__init__(cfg)
        self.file = cfg.file
        self.file_type = cfg.file_type
        self.elf_name = cfg.elf_file
        self.gdb_cmd = [cfg.gdb] if cfg.gdb else None
        self.machine = machine
        self.cpu = cpu
        self.dtb = dtb
        self.pipe = pipe
        self.use_loader = False
        self.qemu_opts_content = []
        self.qemu_opts = qemu_opts
        self.commander = commander
        self.gdbserver = gdbserver
        self.gdb_host = gdb_host
        self.gdb_port = gdb_port
        self.tui_arg = ['-tui'] if tui else []
        self.tool_opt = []
        for opts in [shlex.split(opt) for opt in tool_opt]:
            self.tool_opt += opts


    @classmethod
    def name(cls):
        return 'qemu'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug'},
                          tool_opt=True, file=True)

    @classmethod
    def do_add_parser(cls, parser):
        # Required:
        parser.add_argument('--machine', required=True, help='machine name')

        # Optional:
        parser.add_argument('--cpu', required=False, dest='cpu',
                            help='specifies a cpu type')
        parser.add_argument('--tui', default=False, action='store_true',
                            help='if given, GDB uses -tui')
        parser.add_argument('--gdbserver', default='JLinkGDBServer',
                            help='GDB server, default is JLinkGDBServer')
        parser.add_argument('--gdb-host', default='127.0.0.1',
                            help='custom gdb host, defaults to the empty string '
                            'and runs a gdb server')
        parser.add_argument('--gdb-port', default=DEFAULT_QEMU_GDB_PORT,
                            help='gdb port, defaults to {}'.format(
                                DEFAULT_QEMU_GDB_PORT))
        parser.add_argument('--dtb', required=False, dest='dtb',
                            help='specifies a dtb file')
        parser.add_argument('--qemu_option', required=False, dest='qemu_opts',
                            help='specifies qemu opts')
        parser.add_argument('--commander', default=DEFAULT_QEMU_EXE,
                            help=f'''QEMU executable, default is
                            {DEFAULT_QEMU_EXE}''')
        parser.add_argument('--pipe', default=None,
                            help='''if given, qemu use pipe''')

    @classmethod
    def do_create(cls, cfg, args):
        return QemuBinaryRunner(cfg, machine=args.machine,
                                 cpu=args.cpu,
                                 dtb=args.dtb,
                                 qemu_opts=args.qemu_opts,
                                 commander=args.commander,
                                 pipe=args.pipe,
                                 gdbserver=args.gdbserver,
                                 gdb_host=args.gdb_host,
                                 gdb_port=args.gdb_port,
                                 tui=args.tui, tool_opt=args.tool_opt)

    def do_run(self, command, **kwargs):
        self.commander = os.fspath(
            Path(self.require(self.commander)).resolve())

        if command == 'flash':
            self.flash(**kwargs)
        elif command == 'debug':
            self.debug(**kwargs)
        else:
            pass


    def get_qemu_opts(self):

        def fstr(template):
            return eval(f"f'{template}'", {'self' : self})
        if self.qemu_opts:
            with open(self.qemu_opts, 'r') as stream:
                try:
                    _d = yaml.safe_load(stream)
                    if "qemu" in _d and "qemu_option" in _d["qemu"]:
                        for _li in _d["qemu"]["qemu_option"]:
                            li_parsed = fstr(_li).format()
                            if "loader,file" in _li:
                                self.use_loader = True
                            self.qemu_opts_content += li_parsed.split(" ")
                    elif "qemu" in _d and "variable_options" in _d["qemu"]:
                        _f = os.path.join(Path(self.cfg.elf_file).parent,
                                       "qemu_runner_variables.conf")
                        with open(_f) as __f:
                            for _li in __f:
                                if "loader,file" in _li:
                                    self.use_loader = True
                                self.qemu_opts_content += fstr(_li).format().split(" ")

                except yaml.YAMLError as _e:
                    self.logger.error("%s raised", format(_e))

    def flash(self, **kwargs):
        flash_file = self.cfg.elf_file
        self.get_qemu_opts()

        if self.file is not None:
            # use file provided by the user
            if not os.path.isfile(self.file):
                err = 'Cannot flash; file ({}) not found'
                raise ValueError(err.format(self.file))
            flash_file = self.file

        cmd = ([self.commander] +
               (['-nographic']) +
               ['-machine', self.machine] +
               (['-cpu', f'{self.cpu}'] if self.cpu else []) +
               (['-dtb', f'{self.dtb}'] if self.dtb else []) +
               (['-serial', f'pipe:{self.pipe}'] if self.pipe else []) +
               (['-kernel', flash_file] if not self.use_loader else []) +
               (self.qemu_opts_content if self.qemu_opts_content else []) +
                self.tool_opt)
        kwargs = {}
        #if not self.logger.isEnabledFor(logging.DEBUG):
        #    kwargs['stdout'] = subprocess.DEVNULL
        self.logger.info("flashing with {}".format(" ".join(cmd)))
        self.logger.info("press ctrl+a x to quit qemu")
        self.check_call(cmd, **kwargs)


    def debug(self, **kwargs):
        flash_file = self.cfg.elf_file
        self.get_qemu_opts()

        if self.file is not None:
            # use file provided by the user
            if not os.path.isfile(self.file):
                err = 'Cannot flash; file ({}) not found'
                raise ValueError(err.format(self.file))

            flash_file = self.cfg.elf_file

            if self.cfg.file_type != FileType.ELF:
                err = 'Cannot flash; qmue runner only supports elf files'
                raise ValueError(err)

            server_cmd = ([self.commander] +
                   (['-nographic']) +
                   ['-machine', self.machine] +
                   (['-cpu', f'{self.cpu}'] if self.cpu else []) +
                   (['-dtb', f'{self.dtb}'] if self.dtb else []) +
                   (['-kernel', flash_file] if not self.use_loader else []) +
                   (self.qemu_opts_content if self.qemu_opts_content else []) +
                   (['-s']) + (['-S']) +
                   self.tool_opt)
            client_cmd = (self.gdb_cmd +
                          self.tui_arg +
                          [flash_file] +
                          ['-ex', '\"target remote {}:{}\"'.format(self.gdb_host, self.gdb_port)] +
                          ['-ex', '\"maintenance packet Qqemu.PhyMemMode:1\"',
                          ])
            kwargs = {}
            self.logger.info("press ctrl+a x to quit qemu")
            self.logger.info("run below command to debug \r\n %s", format(" ".join(client_cmd)))
            self.check_call(server_cmd, **kwargs)
        else:
            err = 'Cannot flash; qmue runner only supports elf files'
            raise ValueError(err)
