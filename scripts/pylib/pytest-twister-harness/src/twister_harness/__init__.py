# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# flake8: noqa

from twister_harness.device.device_abstract import DeviceAbstract as Device
from twister_harness.fixtures.mcumgr import MCUmgr

__all__= ['Device', 'MCUmgr']

__version__ = '0.0.1'
