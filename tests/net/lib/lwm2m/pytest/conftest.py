# Copyright (c) 2020 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

import pytest
import time
from leshan_helper import Leshan
from device_helper import Device

# add option "--comdopt" to pytest, or it will report "unknown option"
# this option is passed from twister.
def pytest_addoption(parser):
    parser.addoption(
        '--cmdopt'
    )
    parser.addoption(
        '--coverage', action='store_true', default=False, help="Capture code coverage"
    )


@pytest.fixture(scope="module")
def leshan():
    return Leshan()

@pytest.fixture(scope="module")
def device(request):
    if request.config.getoption("--coverage"):
        coverage=True
    else:
        coverage = False
    dev = Device(coverage)
    dev.run()
    yield dev
    dev.stop()

@pytest.fixture(autouse=True, scope="module")
def device_is_registered(leshan, device):
    # Wait for device to register
    resp = None
    for i in range(100):
        try:
            resp = leshan.get("/clients/ztest")
        except:
            # Allow query to fail, as device is not yet registered or
            # Leshan is not yet started, so wait
            pass
        if resp is not None:
            break
        time.sleep(0.1)

    if resp is None:
        raise RuntimeError('Device is not connected')
    # Now run all tests
    yield
    # Unregister the client by sending
    # execute on reboot resource
    leshan.post("/clients/ztest/3/0/4")
