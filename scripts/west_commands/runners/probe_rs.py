# Copyright (c) 2024 Chen Xingyu <hi@xingrz.me>
# SPDX-License-Identifier: Apache-2.0

'''Runner for probe-rs.'''

from runners.core import ZephyrBinaryRunner, RunnerCaps


class ProbeRsBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for probe-rs.'''

    def __init__(self, cfg, chip,
                 probe_rs='probe-rs',
                 dev_id=None,
                 erase=False,
                 tool_opt=None):
        super().__init__(cfg)

        self.probe_rs = probe_rs
        self.erase = erase

        self.args = ['--chip', chip]

        if dev_id is not None:
            self.args += ['--probe', dev_id]

        if tool_opt is not None:
            self.args += tool_opt

        self.elf_name = cfg.elf_file

    @classmethod
    def name(cls):
        return 'probe-rs'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'},
                          dev_id=True,
                          erase=True,
                          tool_opt=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--chip', required=True,
                            help='chip name')
        parser.add_argument('--probe-rs', default='probe-rs',
                            help='path to probe-rs tool, default is probe-rs')

    @classmethod
    def dev_id_help(cls) -> str:
        return '''select a specific probe, in the form `VID:PID:<Serial>`'''

    @classmethod
    def tool_opt_help(cls) -> str:
        return '''additional options for probe-rs,
                  e.g. --chip-description-path=/path/to/chip.yml'''

    @classmethod
    def do_create(cls, cfg, args):
        return ProbeRsBinaryRunner(cfg, args.chip,
                                   probe_rs=args.probe_rs,
                                   dev_id=args.dev_id,
                                   erase=args.erase,
                                   tool_opt=args.tool_opt)

    def do_run(self, command, **kwargs):
        self.require(self.probe_rs)
        if command == 'flash':
            self.do_flash(**kwargs)

    def do_flash(self, **kwargs):
        download_args = []
        if self.erase:
            download_args += ['--chip-erase']
        download_args += [self.elf_name]

        self.check_call([self.probe_rs, 'download']
                        + self.args + download_args)

        self.check_call([self.probe_rs, 'reset']
                        + self.args)
