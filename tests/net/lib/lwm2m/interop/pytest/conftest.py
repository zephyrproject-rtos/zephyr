"""
Common test fixtures
####################

Copyright (c) 2023 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

"""

import time
import logging
import os
import binascii
import random
import string
import pytest
from leshan import Leshan

from twister_harness import Shell
from twister_harness import DeviceAdapter

LESHAN_IP: str = '192.0.2.2'
COAP_PORT: int = 5683
COAPS_PORT: int = 5684
BOOTSTRAP_COAPS_PORT: int = 5784

logger = logging.getLogger(__name__)

class Endpoint:
    def __init__(self, name: str, shell: Shell, registered: bool = False, bootstrap: bool = False):
        self.name = name
        self.registered = registered
        self.bootstrap = bootstrap
        self.shell = shell
        self.last_update = 0.0

    def check_update(self):
        if not self.registered:
            return
        if self.last_update < time.time() - 5:
            self.shell.exec_command('lwm2m update')
            self.last_update = time.time()

    def __str__(self):
        return self.name


@pytest.fixture(scope='session')
def leshan() -> Leshan:
    """
    Fixture that returns a Leshan object for interacting with the Leshan server.

    :return: The Leshan object.
    :rtype: Leshan
    """
    try:
        return Leshan("http://localhost:8080/api")
    except RuntimeError:
        pytest.skip('Leshan server not available')

@pytest.fixture(scope='session')
def leshan_bootstrap() -> Leshan:
    """
    Fixture that returns a Leshan object for interacting with the Bootstrap Leshan server.

    :return: The Leshan object.
    :rtype: Leshan
    """
    try:
        return Leshan("http://localhost:8081/api")
    except RuntimeError:
        pytest.skip('Leshan Bootstrap server not available')

@pytest.fixture(scope='module')
def helperclient() -> object:
    """
    Fixture that returns a helper client object for testing.

    :return: The helper client object.
    :rtype: object
    """
    try:
        from coapthon.client.helperclient import HelperClient
    except ModuleNotFoundError:
        pytest.skip('CoAPthon3 package not installed')
    return HelperClient(server=('127.0.0.1', COAP_PORT))


@pytest.fixture(scope='module')
def endpoint_nosec(shell: Shell, dut: DeviceAdapter, leshan: Leshan) -> str:
    """Fixture that returns an endpoint that starts on no-secure mode"""
    # Allow engine to start & stop once.
    time.sleep(2)

    # Generate randon device id and password (PSK key)
    ep = 'client_' + binascii.b2a_hex(os.urandom(1)).decode()

    #
    # Registration Interface test cases (using Non-secure mode)
    #
    shell.exec_command(f'lwm2m write 0/0/0 -s coap://{LESHAN_IP}:{COAP_PORT}')
    shell.exec_command('lwm2m write 0/0/1 -b 0')
    shell.exec_command('lwm2m write 0/0/2 -u8 3')
    shell.exec_command(f'lwm2m write 0/0/3 -s {ep}')
    shell.exec_command('lwm2m create 1/0')
    shell.exec_command('lwm2m write 0/0/10 -u16 1')
    shell.exec_command('lwm2m write 1/0/0 -u16 1')
    shell.exec_command('lwm2m write 1/0/1 -u32 86400')
    shell.exec_command(f'lwm2m start {ep} -b 0')

    yield Endpoint(ep, shell)

    # All done
    shell.exec_command('lwm2m stop')
    dut.readlines_until(regex=r'.*Deregistration success', timeout=10.0)

@pytest.fixture(scope='module')
def endpoint_bootstrap(shell: Shell, dut: DeviceAdapter, leshan: Leshan, leshan_bootstrap: Leshan) -> str:
    """Fixture that returns an endpoint that starts the bootstrap."""
    try:
        # Generate randon device id and password (PSK key)
        ep = 'client_' + binascii.b2a_hex(os.urandom(1)).decode()
        bs_passwd = ''.join(random.choice(string.ascii_lowercase) for i in range(16))
        passwd = ''.join(random.choice(string.ascii_lowercase) for i in range(16))

        logger.debug('Endpoint: %s', ep)
        logger.debug('Boostrap PSK: %s', binascii.b2a_hex(bs_passwd.encode()).decode())
        logger.debug('PSK: %s', binascii.b2a_hex(passwd.encode()).decode())

        # Create device entries in Leshan and Bootstrap server
        leshan_bootstrap.create_bs_device(ep, f'coaps://{LESHAN_IP}:{COAPS_PORT}', bs_passwd, passwd)
        leshan.create_psk_device(ep, passwd)

        # Allow engine to start & stop once.
        time.sleep(2)

        # Write bootsrap server information and PSK keys
        shell.exec_command(f'lwm2m write 0/0/0 -s coaps://{LESHAN_IP}:{BOOTSTRAP_COAPS_PORT}')
        shell.exec_command('lwm2m write 0/0/1 -b 1')
        shell.exec_command('lwm2m write 0/0/2 -u8 0')
        shell.exec_command(f'lwm2m write 0/0/3 -s {ep}')
        shell.exec_command(f'lwm2m write 0/0/5 -s {bs_passwd}')
        shell.exec_command(f'lwm2m start {ep} -b 1')

        yield Endpoint(ep, shell)

        shell.exec_command('lwm2m stop')
        dut.readlines_until(regex=r'.*Deregistration success', timeout=10.0)

    finally:
        # Remove device and bootstrap information
        # Leshan does not accept non-secure connection if device information is provided with PSK
        leshan.delete_device(ep)
        leshan_bootstrap.delete_bs_device(ep)

@pytest.fixture(scope='module')
def endpoint_registered(endpoint_bootstrap, dut: DeviceAdapter) -> str:
    """Fixture that returns an endpoint that is registered."""
    if not endpoint_bootstrap.registered:
        dut.readlines_until(regex='.*Registration Done', timeout=5.0)
        endpoint_bootstrap.bootstrap = True
        endpoint_bootstrap.registered = True
    return endpoint_bootstrap

@pytest.fixture(scope='function')
def endpoint(endpoint_registered) -> str:
    """Fixture that returns an endpoint that is registered."""
    endpoint_registered.check_update()
    return endpoint_registered
