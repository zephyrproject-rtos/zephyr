# Copyright (c) 2020 Synopsys.
#
# SPDX-License-Identifier: Apache-2.0

'''Runners for Synopsys Metaware Debugger(mdb).'''


import shutil
import os
from os import path

from runners.core import ZephyrBinaryRunner, RunnerCaps


# normally we should create class with common functionality inherited from
# ZephyrBinaryRunner and inherit MdbNsimBinaryRunner and MdbHwBinaryRunner
# from it. However as we do lookup for runners with
# ZephyrBinaryRunner.__subclasses__() such sub-sub-classes won't be found.
# So, we move all common functionality to helper functions instead.
def is_simulation_run(mdb_runner):
    return mdb_runner.nsim_args != ''

def is_hostlink_used(mdb_runner):
    return mdb_runner.build_conf.getboolean('CONFIG_UART_HOSTLINK')

def is_flash_cmd_need_exit_immediately(mdb_runner):
    if is_simulation_run(mdb_runner):
        # for nsim, we can't run and quit immediately
        return False

    # if hostlink is used we can't run and quit immediately, as we still need MDB process
    # attached to process hostlink IO
    return not is_hostlink_used(mdb_runner)

def smp_core_order(mdb_runner, id):
    if is_simulation_run(mdb_runner):
        # for simulation targets we start cores in direct order (core 0 first, core 1 second, etc...)
        # otherwise we face mismatch arcnum (code ID) with ARConnect ID and core ID in instruction traces
        return id
    else:
        # for HW targets we want to start the primary core last, to avoid ARConnect initialization interfere
        # with secondary cores startup - so we reverse start order
        return mdb_runner.cores - 1 - id

def mdb_do_run(mdb_runner, command):
    commander = "mdb64"

    mdb_runner.require(commander)

    mdb_basic_options = ['-nooptions', '-nogoifmain', '-toggle=include_local_symbols=1']

    # remove previous .sc.project folder which has temporary settings
    # for MDB. This is useful for troubleshooting situations with
    # unexpected behavior of the debugger
    mdb_cfg_dir = path.join(mdb_runner.build_dir, '.sc.project')
    if path.exists(mdb_cfg_dir):
        shutil.rmtree(mdb_cfg_dir)

    # nsim
    if is_simulation_run(mdb_runner):
        mdb_target = ['-nsim', '@' + mdb_runner.nsim_args]
    # hardware target
    else:
        if mdb_runner.jtag == 'digilent':
            mdb_target = ['-digilent']
            if mdb_runner.dig_device: mdb_target += [mdb_runner.dig_device]
        else:
            # \todo: add support of other debuggers
            raise ValueError('unsupported jtag adapter {}'.format(mdb_runner.jtag))

    if command == 'flash':
        if is_flash_cmd_need_exit_immediately(mdb_runner):
            mdb_run = ['-run', '-cmd=-nowaitq run', '-cmd=quit', '-cl']
        else:
            mdb_run = ['-run', '-cl']
    elif command == 'debug':
        # use mdb gui to debug
        mdb_run = ['-OKN']

    if mdb_runner.cores == 1:
        # single core's mdb command is different with multicores
        mdb_cmd = [commander] + mdb_basic_options + mdb_target + mdb_run + [mdb_runner.elf_name]
    elif 1 < mdb_runner.cores <= 12:
        mdb_multifiles = '-multifiles='
        for i in range(mdb_runner.cores):
            mdb_sub_cmd = [commander] + ['-pset={}'.format(i + 1), '-psetname=core{}'.format(i)]
            # -prop=download=2 is used for SMP application debug, only the 1st core
            # will download the shared image.
            if i > 0: mdb_sub_cmd += ['-prop=download=2']
            mdb_sub_cmd += mdb_basic_options + mdb_target + [mdb_runner.elf_name]
            mdb_runner.check_call(mdb_sub_cmd, cwd=mdb_runner.build_dir)
            mdb_multifiles += ('core{}' if i == 0 else ',core{}').format(smp_core_order(mdb_runner, i))

        # to enable multi-core aware mode for use with the MetaWare debugger,
        # need to set the NSIM_MULTICORE environment variable to a non-zero value
        if is_simulation_run(mdb_runner):
            os.environ["NSIM_MULTICORE"] = '1'

        mdb_cmd = [commander] + [mdb_multifiles] + mdb_run
    else:
        raise ValueError('unsupported cores {}'.format(mdb_runner.cores))

    mdb_runner.call(mdb_cmd, cwd=mdb_runner.build_dir)


class MdbNsimBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for nSIM via mdb.'''

    def __init__(self, cfg, cores=1, nsim_args=''):
        super().__init__(cfg)
        self.jtag = ''
        self.cores = int(cores)
        if nsim_args != '':
            self.nsim_args = path.join(cfg.board_dir, 'support', nsim_args)
        else:
            self.nsim_args = ''
        self.elf_name = cfg.elf_file
        self.build_dir = cfg.build_dir
        self.dig_device = ''

    @classmethod
    def name(cls):
        return 'mdb-nsim'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--cores', default=1,
                            help='''choose the cores that target has, e.g.
                                    --cores=1''')
        parser.add_argument('--nsim_args', default='',
                            help='''if given, arguments for nsim simulator
                                 through mdb which should be in
                                 <board_dir>/support, e.g. --nsim-args=
                                 mdb_em.args''')

    @classmethod
    def do_create(cls, cfg, args):
        return MdbNsimBinaryRunner(
            cfg,
            cores=args.cores,
            nsim_args=args.nsim_args)

    def do_run(self, command, **kwargs):
        mdb_do_run(self, command)


class MdbHwBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for mdb.'''

    def __init__(self, cfg, cores=1, jtag='digilent', dig_device=''):
        super().__init__(cfg)
        self.jtag = jtag
        self.cores = int(cores)
        self.nsim_args = ''
        self.elf_name = cfg.elf_file
        if dig_device != '':
            self.dig_device = '-prop=dig_device=' + dig_device
        else:
            self.dig_device = ''
        self.build_dir = cfg.build_dir

    @classmethod
    def name(cls):
        return 'mdb-hw'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--jtag', default='digilent',
                            help='''choose the jtag interface for hardware
                                    targets, e.g. --jtag=digilent for digilent
                                    jtag adapter''')
        parser.add_argument('--cores', default=1,
                            help='''choose the number of cores that target has,
                                    e.g. --cores=1''')
        parser.add_argument('--dig-device', default='',
                            help='''choose the specific digilent device to
                             connect, this is useful when multiple
                             targets are connected''')

    @classmethod
    def do_create(cls, cfg, args):
        return MdbHwBinaryRunner(
            cfg,
            cores=args.cores,
            jtag=args.jtag,
            dig_device=args.dig_device)

    def do_run(self, command, **kwargs):
        mdb_do_run(self, command)
