# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

from .core import ZephyrBinaryRunner

# We import these here to ensure the ZephyrBinaryRunner subclasses are
# defined; otherwise, ZephyrBinaryRunner.create_for_shell_script()
# won't work.

# Explicitly silence the unused import warning.
# flake8: noqa: F401
from . import arc
from . import bossac
from . import dfu
from . import esp32
from . import jlink
from . import nios2
from . import nrfjprog
from . import nsim
from . import openocd
from . import pyocd
from . import qemu
from . import xtensa
from . import intel_s1000

def get_runner_cls(runner):
    '''Get a runner's class object, given its name.'''
    for cls in ZephyrBinaryRunner.get_runners():
        if cls.name() == runner:
            return cls
    raise ValueError('unknown runner "{}"'.format(runner))

__all__ = ['ZephyrBinaryRunner', 'get_runner_cls']
