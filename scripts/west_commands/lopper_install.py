# Copyright (c) 2024 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

import subprocess
import sys
import os
from west.commands import WestCommand

def runcmd(cmd, cwd=None, logfile=None) -> bool:
    """
    Run the shell commands.

    Args:
        | cmd: shell command that needs to be called
        | logfile: file to save the command output if required
    """
    ret = True
    if logfile is None:
        try:
            subprocess.check_call(cmd, cwd=cwd, shell=True)
        except subprocess.CalledProcessError as exc:
            ret = False
            sys.exit(1)
    else:
        try:
            subprocess.check_call(cmd, cwd=cwd, shell=True, stdout=logfile, stderr=logfile)
        except subprocess.CalledProcessError:
            ret = False
    return ret

class LopperInstall(WestCommand):
    def __init__(self):
        super().__init__(
            'lopper-install',  # The name of the command
            'Install lopper after west update',  # Help text for the command
            'This command installs lopper after west update.'
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(self.name, help=self.help)
        return parser

    def do_run(self, args, unknown_args):
        # Install lopper and its dependencies
        self.install_lopper()

    def install_lopper(self):
        env_path = os.environ.get("VIRTUAL_ENV")
        lopper_dir = os.path.join(env_path, "../", "lopper")
        runcmd(f"pip install ./[dt,server,yaml,pcpp]",
                cwd = lopper_dir)
