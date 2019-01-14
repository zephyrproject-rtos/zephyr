# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

from west.runners.core import ZephyrBinaryRunner

# We import these here to ensure the ZephyrBinaryRunner subclasses are
# defined; otherwise, ZephyrBinaryRunner.create_for_shell_script()
# won't work.

# Explicitly silence the unused import warning.
# flake8: noqa: F401
from west.runners import arc
from west.runners import bossac
from west.runners import dfu
from west.runners import esp32
from west.runners import jlink
from west.runners import nios2
from west.runners import nrfjprog
from west.runners import nsim
from west.runners import openocd
from west.runners import pyocd
from west.runners import qemu
from west.runners import xtensa
from west.runners import intel_s1000
from west.runners import blackmagicprobe

def get_runner_cls(runner):
    '''Get a runner's class object, given its name.'''
    for cls in ZephyrBinaryRunner.get_runners():
        if cls.name() == runner:
            return cls
    raise ValueError('unknown runner "{}"'.format(runner))

__all__ = ['ZephyrBinaryRunner', 'get_runner_cls']
