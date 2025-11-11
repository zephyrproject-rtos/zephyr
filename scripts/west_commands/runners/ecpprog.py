# Copyright (c) 2024 tinyVision.ai Inc.
#
# SPDX-License-Identifier: Apache-2.0

"""Runner for the ecpprog programming tool for Lattice FPGAs."""
# https://github.com/gregdavill/ecpprog

from runners.core import BuildConfiguration, RunnerCaps, ZephyrBinaryRunner


class EcpprogBinaryRunner(ZephyrBinaryRunner):
    """Runner front-end for programming the FPGA flash at some offset."""

    def __init__(self, cfg, device=None):
        super().__init__(cfg)
        self.device = device

    @classmethod
    def name(cls):
        return "ecpprog"

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={"flash"})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            "--device", dest="device", help="Device identifier such as i:<vid>:<pid>"
        )

    @classmethod
    def do_create(cls, cfg, args):
        return EcpprogBinaryRunner(cfg, device=args.device)

    def do_run(self, command, **kwargs):
        build_conf = BuildConfiguration(self.cfg.build_dir)
        load_offset = build_conf.get("CONFIG_FLASH_LOAD_OFFSET", 0)
        command = ("ecpprog", "-o", hex(load_offset), self.cfg.bin_file)
        self.logger.debug(" ".join(command))
        self.check_call(command)
