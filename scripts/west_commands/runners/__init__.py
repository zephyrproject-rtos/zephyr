# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

from runners.core import ZephyrBinaryRunner, MissingProgram

# We import these here to ensure the ZephyrBinaryRunner subclasses are
# defined; otherwise, ZephyrBinaryRunner.get_runners() won't work.

# Explicitly silence the unused import warning.
# flake8: noqa: F401
# Keep this list sorted by runner name.
from runners import blackmagicprobe
from runners import bossac
from runners import canopen_program
from runners import dediprog
from runners import dfu
from runners import esp32
from runners import hifive1
from runners import intel_s1000
from runners import jlink
from runners import misc
from runners import nios2
from runners import nrfjprog
from runners import nsim
from runners import openocd
from runners import pyocd
from runners import qemu
from runners import stm32flash
from runners import xtensa
from runners import mdb
from runners import stm32cubeprogrammer

def get_runner_cls(runner):
    '''Get a runner's class object, given its name.'''
    for cls in ZephyrBinaryRunner.get_runners():
        if cls.name() == runner:
            return cls
    raise ValueError('unknown runner "{}"'.format(runner))

__all__ = ['ZephyrBinaryRunner', 'get_runner_cls']
