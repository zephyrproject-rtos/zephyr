# Copyright (c) 2020 Synopsys.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for Synopsys Metaware Debugger(mdb).'''


import shutil
import os
from os import path
from runners.core import ZephyrBinaryRunner, RunnerCaps

class MdbBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for mdb.'''

    def __init__(self, cfg, cores=1, jtag='digilent', nsim_args='',
                dig_device=''):
        super().__init__(cfg)
        self.jtag = jtag
        self.cores = int(cores)
        if nsim_args != '':
            self.nsim_args = path.join(cfg.board_dir, 'support', nsim_args)
        else:
            self.nsim_args = ''
        self.elf_name = cfg.elf_file
        if dig_device != '':
            self.dig_device = '-prop=dig_device=' + dig_device
        else:
            self.dig_device = ''
        self.commander = 'mdb'
        self.mdb_cfg_dir = path.join(cfg.build_dir, '.sc.project')

    @classmethod
    def name(cls):
        return 'mdb'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--jtag', default='digilent',
                            help='''choose the jtag interface for hardware
                                    targets, e.g. --jtat=digilent for digilent
                                    jtag debugger''')
        parser.add_argument('--cores', default=1,
                            help='''choose the cores that target has,e.g.
                                    --cores=1''')
        parser.add_argument('--nsim_args', default='',
                            help='''if given, arguments for nsim simulator
                                 through mdb which should be in
                                 <board_dir>/support, e.g. --nsim-args=
                                 mdb_em.args''')
        parser.add_argument('--dig-device', default='',
                            help='''choose the the specific digilent device to
                             connect, this is useful when multi
                             targets are connected''')

    @classmethod
    def do_create(cls, cfg, args):
        return MdbBinaryRunner(
            cfg,
            cores=args.cores,
            jtag=args.jtag,
            nsim_args=args.nsim_args,
            dig_device=args.dig_device
            )

    def do_run(self, command, **kwargs):
        self.require(self.commander)

        mdb_basic_options = ['-nooptions', '-nogoifmain',
                            '-toggle=include_local_symbols=1']

        # remove previous .sc.project folder which has temporary settings
        # for MDB. This is useful for troubleshooting situations with
        # unexpected behavior of the debugger
        if path.exists(self.mdb_cfg_dir):
            shutil.rmtree(self.mdb_cfg_dir)
        # nsim
        if self.nsim_args != '':
            mdb_target = ['-nsim', '@' + self.nsim_args]
        # hardware target
        else:
            if self.jtag == 'digilent':
                mdb_target = ['-digilent', self.dig_device]
            else:
                # \todo: add support of other debuggers
                mdb_target = ['']

        if command == 'flash':
            if self.nsim_args != '':
                # for nsim , can't run and quit immediately
                mdb_run = ['-run', '-cl']
            else:
                mdb_run = ['-run', '-cmd=-nowaitq run', '-cmd=quit', '-cl']
        elif command == 'debug':
            # use mdb gui to debug
            mdb_run = ['-OKN']

        if self.cores == 1:
            # single core's mdb command is different with multicores
            mdb_cmd = ([self.commander] + mdb_basic_options + mdb_target +
                       mdb_run + [self.elf_name])
        elif self.cores <= 4:
            mdb_multifiles='-multifiles='
            for i in range(self.cores):
                # note that: mdb requires -pset starting from 1, not 0 !!!
                mdb_sub_cmd = ([self.commander] +
                            ['-pset={}'.format(i+1),
                             '-psetname=core{}'.format(i),
            # -prop=download=2 is used for SMP application debug, only the 1st
            # core will download the shared image.
                             ('-prop=download=2' if i > 0 else '')] +
                             mdb_basic_options + mdb_target + [self.elf_name])
                self.check_call(mdb_sub_cmd)
                mdb_multifiles += (',core{}'.format(i) if i > 0 else 'core{}'.format(i))

            # to enable multi-core aware mode for use with the MetaWare debugger,
            # need to set the NSIM_MULTICORE environment variable to a non-zero value
            if self.nsim_args != '':
                os.environ["NSIM_MULTICORE"] = '1'

            mdb_cmd = ([self.commander] + [mdb_multifiles] + mdb_run)
        else:
            raise ValueError('unsupported cores {}'.format(self.cores))
        self.check_call(mdb_cmd)
