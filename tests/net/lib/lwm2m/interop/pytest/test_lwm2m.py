# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import time
import logging
import pytest
from leshan import Leshan
import os
import binascii
import random
import string

from twister_harness import Shell

LESHAN_IP: str = '192.0.2.2'
COAP_PORT: int = 5683
COAPS_PORT: int = 5684
BOOTSTRAP_COAPS_PORT: int = 5784

logger = logging.getLogger(__name__)

@pytest.fixture(scope='module')
def helperclient() -> object:
    try:
        from coapthon.client.helperclient import HelperClient
    except ModuleNotFoundError:
        pytest.skip('CoAPthon3 package not installed')
    return HelperClient(server=('127.0.0.1', COAP_PORT))

@pytest.fixture(scope='session')
def leshan() -> Leshan:
    try:
        return Leshan("http://localhost:8080/api")
    except RuntimeError:
        pytest.skip('Leshan server not available')

@pytest.fixture(scope='session')
def leshan_bootstrap() -> Leshan:
    try:
        return Leshan("http://localhost:8081/api")
    except RuntimeError:
        pytest.skip('Leshan Bootstrap server not available')

#
# Test specification:
# https://www.openmobilealliance.org/release/LightweightM2M/ETS/OMA-ETS-LightweightM2M-V1_1-20190912-D.pdf
#

def verify_LightweightM2M_1_1_int_0(shell: Shell):
    logger.info("LightweightM2M-1.1-int-0 - Client Initiated Bootstrap")
    shell._device.readlines_until(regex='.*Bootstrap started with endpoint', timeout=5.0)
    shell._device.readlines_until(regex='.*Bootstrap registration done', timeout=5.0)
    shell._device.readlines_until(regex='.*Bootstrap data transfer done', timeout=5.0)

def verify_LightweightM2M_1_1_int_1(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-1 - Client Initiated Bootstrap Full (PSK)")
    verify_LightweightM2M_1_1_int_0(shell)
    verify_LightweightM2M_1_1_int_101(shell, leshan, endpoint)
    verify_LightweightM2M_1_1_int_401(shell, leshan, endpoint)

def verify_LightweightM2M_1_1_int_101(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-101 - Initial Registration")
    shell._device.readlines_until(regex='.*Registration Done', timeout=5.0)
    assert leshan.get(f'/clients/{endpoint}')

def verify_LightweightM2M_1_1_int_102(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-102 - Registration Update")
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 1/0/1 -u32'))
    litetime = int(lines[0])
    lifetime = litetime + 10
    start_time = time.time() * 1000
    leshan.write(endpoint, '1/0/1', lifetime)
    shell._device.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)
    latest = leshan.get(f'/clients/{endpoint}')
    assert latest["lastUpdate"] > start_time
    assert latest["lastUpdate"] <= time.time()*1000
    assert latest["lifetime"] == lifetime
    shell.exec_command('lwm2m write 1/0/1 -u32 86400')

def verify_LightweightM2M_1_1_int_103():
    """LightweightM2M-1.1-int-103 - Deregistration"""
    # Unsupported. We don't have "disabled" functionality in server object

def verify_LightweightM2M_1_1_int_104(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-104 - Registration Update Trigger")
    shell.exec_command('lwm2m update')
    shell._device.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)
    leshan.execute(endpoint, '1/0/8')
    shell._device.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)

def verify_LightweightM2M_1_1_int_105(shell: Shell, leshan: Leshan, endpoint: str, helperclient: object):
    logger.info("LightweightM2M-1.1-int-105 - Discarded Register Update")
    status = leshan.get(f'/clients/{endpoint}')
    if status["secure"]:
        logger.debug("Skip, requires non-secure connection")
        return
    id = status["registrationId"]
    assert id
    # Fake unregister message
    helperclient.delete(f'rd/{id}', timeout=0.1)
    helperclient.stop()
    time.sleep(1)
    shell.exec_command('lwm2m update')
    shell._device.readlines_until(regex=r'.*Failed with code 4\.4', timeout=5.0)
    shell._device.readlines_until(regex='.*Registration Done', timeout=10.0)

def verify_LightweightM2M_1_1_int_107(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-107 - Extending the lifetime of a registration")
    leshan.write(endpoint, '1/0/1', 120)
    shell._device.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 1/0/1 -u32'))
    lifetime = int(lines[0])
    assert lifetime == 120
    logger.debug(f'Wait for update, max {lifetime} s')
    shell._device.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=lifetime)
    assert leshan.get(f'/clients/{endpoint}')

def verify_LightweightM2M_1_1_int_108(leshan, endpoint):
    logger.info("LightweightM2M-1.1-int-108 - Turn on Queue Mode")
    assert leshan.get(f'/clients/{endpoint}')["queuemode"]

def verify_LightweightM2M_1_1_int_109(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-109 - Behavior in Queue Mode")
    verify_LightweightM2M_1_1_int_107(shell, leshan, endpoint)
    logger.debug('Wait for Queue RX OFF')
    shell._device.readlines_until(regex='.*Queue mode RX window closed', timeout=120)
    # Restore previous value
    shell.exec_command('lwm2m write 1/0/1 -u32 86400')
    shell._device.readlines_until(regex='.*Registration update complete', timeout=10)

def verify_LightweightM2M_1_1_int_201(shell: Shell, leshan: Leshan, endpoint: str):

    logger.info("LightweightM2M-1.1-int-201 - Querying basic information in Plain Text format")
    fmt = leshan.format
    leshan.format = 'TEXT'
    assert leshan.get(f'/clients/{endpoint}/3/0/0')['content']['value'] == 'Zephyr'
    assert leshan.get(f'/clients/{endpoint}/3/0/1')['content']['value'] == 'client-1'
    assert leshan.get(f'/clients/{endpoint}/3/0/2')['content']['value'] == 'serial-1'
    leshan.format = fmt

def verify_device_object(resp):
    ''' Verify that Device object match Configuration 3 '''
    assert resp['valid'] is True
    found = 0
    for res in resp['content']['resources']:
        if res['id'] == 0:
            assert res['value'] == 'Zephyr'
            found += 1
        elif res['id'] == 1:
            assert res['value'] == 'client-1'
            found += 1
        elif res['id'] == 2:
            assert res['value'] == 'serial-1'
            found += 1
        elif res['id'] == 3:
            assert res['value'] == '1.2.3'
            found += 1
        elif res['id'] == 11:
            assert res['kind'] == 'multiResource'
            assert res['values']['0'] == '0'
            found += 1
        elif res['id'] == 16:
            assert res['value'] == 'U'
            found += 1
    assert found == 6

def verify_server_object(obj):
    ''' Verify that server object match Configuration 3 '''
    found = 0
    for res in obj['resources']:
        if res['id'] == 0:
            assert res['value'] == '1'
            found += 1
        elif res['id'] == 1:
            assert res['value'] == '86400'
            found += 1
        elif res['id'] == 2:
            assert res['value'] == '1'
            found += 1
        elif res['id'] == 3:
            assert res['value'] == '10'
            found += 1
        elif res['id'] == 5:
            assert res['value'] == '86400'
            found += 1
        elif res['id'] == 6:
            assert res['value'] is False
            found += 1
        elif res['id'] == 7:
            assert res['value'] == 'U'
            found += 1
    assert found == 7

def verify_LightweightM2M_1_1_int_203(shell: Shell, leshan: Leshan, endpoint: str):
    shell.exec_command('lwm2m update')
    logger.info('LightweightM2M-1.1-int-203 - Querying basic information in TLV format')
    fmt = leshan.format
    leshan.format = 'TLV'
    resp = leshan.get(f'/clients/{endpoint}/3/0')
    verify_device_object(resp)
    leshan.format = fmt

def verify_LightweightM2M_1_1_int_204(shell: Shell, leshan: Leshan, endpoint: str):
    shell.exec_command('lwm2m update')
    logger.info('LightweightM2M-1.1-int-204 - Querying basic information in JSON format')
    fmt = leshan.format
    leshan.format = 'JSON'
    resp = leshan.get(f'/clients/{endpoint}/3/0')
    verify_device_object(resp)
    leshan.format = fmt

def verify_LightweightM2M_1_1_int_205(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info('LightweightM2M-1.1-int-205 - Setting basic information in Plain Text format')
    fmt = leshan.format
    leshan.format = 'TEXT'
    leshan.write(endpoint, '1/0/2', 101)
    leshan.write(endpoint, '1/0/3', 1010)
    leshan.write(endpoint, '1/0/5', 2000)
    assert leshan.read(endpoint, '1/0/2') == '101'
    assert leshan.read(endpoint, '1/0/3') == '1010'
    assert leshan.read(endpoint, '1/0/5') == '2000'
    leshan.write(endpoint, '1/0/2', 1)
    leshan.write(endpoint, '1/0/3', 10)
    leshan.write(endpoint, '1/0/5', 86400)
    assert leshan.read(endpoint, '1/0/2') == '1'
    assert leshan.read(endpoint, '1/0/3') == '10'
    assert leshan.read(endpoint, '1/0/5') == '86400'
    leshan.format = fmt

def verify_LightweightM2M_1_1_int_211(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info('LightweightM2M-1.1-int-211 - Querying basic information in CBOR format')
    fmt = leshan.format
    leshan.format = 'CBOR'
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 1/0/0 -u16'))
    id = lines[0]
    assert leshan.read(endpoint, '1/0/0') == id
    assert leshan.read(endpoint, '1/0/6') is False
    assert leshan.read(endpoint, '1/0/7') == 'U'
    leshan.format = fmt

def verify_LightweightM2M_1_1_int_212(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info('LightweightM2M-1.1-int-212 - Setting basic information in CBOR format')
    fmt = leshan.format
    leshan.format = 'CBOR'
    leshan.write(endpoint, '1/0/2', 101)
    leshan.write(endpoint, '1/0/3', 1010)
    leshan.write(endpoint, '1/0/6', True)
    assert leshan.read(endpoint, '1/0/2') == '101'
    assert leshan.read(endpoint, '1/0/3') == '1010'
    assert leshan.read(endpoint, '1/0/6') is True
    leshan.write(endpoint, '1/0/2', 1)
    leshan.write(endpoint, '1/0/3', 10)
    leshan.write(endpoint, '1/0/6', False)
    leshan.format = fmt

def verify_setting_basic_in_format(shell, leshan, endpoint, format):
    fmt = leshan.format
    leshan.format = format
    server_obj = leshan.get(f'/clients/{endpoint}/1/0')['content']
    verify_server_object(server_obj)
    # Remove Read-Only resources, so we don't end up writing those
    for res in server_obj['resources']:
        if res['id'] in (0, 11, 12):
            server_obj['resources'].remove(res)
    data = '''{
        "kind": "instance",
        "id": 0,
        "resources": [
                {
                        "id": 2,
                        "kind": "singleResource",
                        "value": "101",
                        "type": "integer"
                },
                {
                        "id": 3,
                        "kind": "singleResource",
                        "value": "1010",
                        "type": "integer"
                },
                {
                        "id": 5,
                        "kind": "singleResource",
                        "value": "2000",
                        "type": "integer"
                },
                {
                        "id": 6,
                        "kind": "singleResource",
                        "value": true,
                        "type": "boolean"
                },
                {
                        "id": 7,
                        "kind": "singleResource",
                        "value": "U",
                        "type": "string"
                }
        ]
    }'''
    assert leshan.put(f'/clients/{endpoint}/1/0', data, uri_options = '&replace=false')['status'] == 'CHANGED(204)'
    resp = leshan.get(f'/clients/{endpoint}/1/0')
    assert resp['valid'] is True
    found = 0
    for res in resp['content']['resources']:
        if res['id'] == 2:
            assert res['value'] == '101'
            found += 1
        elif res['id'] == 3:
            assert res['value'] == '1010'
            found += 1
        elif res['id'] == 5:
            assert res['value'] == '2000'
            found += 1
        elif res['id'] == 6:
            assert res['value'] is True
            found += 1
        elif res['id'] == 7:
            assert res['value'] == 'U'
            found += 1
    assert found == 5
    assert leshan.put(f'/clients/{endpoint}/1/0', data = server_obj, uri_options = '&replace=true')['status'] == 'CHANGED(204)'
    server_obj = leshan.get(f'/clients/{endpoint}/1/0')['content']
    verify_server_object(server_obj)
    leshan.format = fmt

def verify_LightweightM2M_1_1_int_215(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info('LightweightM2M-1.1-int-215 - Setting basic information in TLV format')
    verify_setting_basic_in_format(shell, leshan, endpoint, 'TLV')

def verify_LightweightM2M_1_1_int_220(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info('LightweightM2M-1.1-int-220 - Setting basic information in JSON format')
    verify_setting_basic_in_format(shell, leshan, endpoint, 'JSON')

def verify_LightweightM2M_1_1_int_221(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info('LightweightM2M-1.1-int-221 - Attempt to perform operations on Security')
    assert leshan.read(endpoint, '0/0')['status'] == 'UNAUTHORIZED(401)'
    assert leshan.write(endpoint, '0/0/0', 'coap://localhost')['status'] == 'UNAUTHORIZED(401)'
    assert leshan.put_raw(f'/clients/{endpoint}/0/attributes?pmin=10')['status'] == 'UNAUTHORIZED(401)'

def verify_LightweightM2M_1_1_int_401(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-401 - UDP Channel Security - Pre-shared Key Mode")
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 0/0/0 -s'))
    host = lines[0]
    assert 'coaps://' in host
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 0/0/2 -u8'))
    mode = int(lines[0])
    assert mode == 0
    resp = leshan.get(f'/clients/{endpoint}')
    assert resp["secure"]

def test_lwm2m_bootstrap_psk(shell: Shell, leshan, leshan_bootstrap):
    try:
        # Generate randon device id and password (PSK key)
        endpoint = 'client_' + binascii.b2a_hex(os.urandom(1)).decode()
        passwd = ''.join(random.choice(string.ascii_lowercase) for i in range(16))


        # Create device entries in Leshan and Bootstrap server
        leshan_bootstrap.create_bs_device(endpoint, f'coaps://{LESHAN_IP}:{COAPS_PORT}', passwd)
        leshan.create_psk_device(endpoint, passwd)

        # Allow engine to start & stop once.
        time.sleep(2)

        #
        # Verify PSK security using Bootstrap
        #

        # Write bootsrap server information and PSK keys
        shell.exec_command(f'lwm2m write 0/0/0 -s coaps://{LESHAN_IP}:{BOOTSTRAP_COAPS_PORT}')
        shell.exec_command('lwm2m write 0/0/1 -b 1')
        shell.exec_command('lwm2m write 0/0/2 -u8 0')
        shell.exec_command(f'lwm2m write 0/0/3 -s {endpoint}')
        shell.exec_command(f'lwm2m write 0/0/5 -s {passwd}')
        shell.exec_command(f'lwm2m start {endpoint} -b 1')


        #
        # Bootstrap Interface test cases
        # LightweightM2M-1.1-int-0 (included)
        # LightweightM2M-1.1-int-401 (included)
        verify_LightweightM2M_1_1_int_1(shell, leshan, endpoint)

        #
        # Registration Interface test cases (using PSK security)
        #
        verify_LightweightM2M_1_1_int_102(shell, leshan, endpoint)
        # skip, not implemented verify_LightweightM2M_1_1_int_103()
        verify_LightweightM2M_1_1_int_104(shell, leshan, endpoint)
        # skip, included in 109: verify_LightweightM2M_1_1_int_107(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_108(leshan, endpoint)
        verify_LightweightM2M_1_1_int_109(shell, leshan, endpoint)

        #
        # Device management & Service Enablement Interface test cases
        #
        verify_LightweightM2M_1_1_int_201(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_203(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_204(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_205(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_211(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_212(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_215(shell, leshan, endpoint)

        shell.exec_command('lwm2m stop')
        shell._device.readlines_until(regex=r'.*Deregistration success', timeout=10.0)

    finally:
        # Remove device and bootstrap information
        # Leshan does not accept non-secure connection if device information is provided with PSK
        leshan.delete_device(endpoint)
        leshan_bootstrap.delete_bs_device(endpoint)


def test_lwm2m_nosecure(shell: Shell, leshan, helperclient):

    # Allow engine to start & stop once.
    time.sleep(2)

    # Generate randon device id and password (PSK key)
    endpoint = 'client_' + binascii.b2a_hex(os.urandom(1)).decode()

    #
    # Registration Interface test cases (using Non-secure mode)
    #
    shell.exec_command(f'lwm2m write 0/0/0 -s coap://{LESHAN_IP}:{COAP_PORT}')
    shell.exec_command('lwm2m write 0/0/1 -b 0')
    shell.exec_command('lwm2m write 0/0/2 -u8 3')
    shell.exec_command(f'lwm2m write 0/0/3 -s {endpoint}')
    shell.exec_command('lwm2m create 1/0')
    shell.exec_command('lwm2m write 0/0/10 -u16 1')
    shell.exec_command('lwm2m write 1/0/0 -u16 1')
    shell.exec_command('lwm2m write 1/0/1 -u32 86400')
    shell.exec_command(f'lwm2m start {endpoint} -b 0')
    shell._device.readlines_until(regex=f"RD Client started with endpoint '{endpoint}'", timeout=10.0)

    verify_LightweightM2M_1_1_int_101(shell, leshan, endpoint)
    verify_LightweightM2M_1_1_int_105(shell, leshan, endpoint, helperclient) # needs no-security
    verify_LightweightM2M_1_1_int_215(shell, leshan, endpoint)
    verify_LightweightM2M_1_1_int_220(shell, leshan, endpoint)
    verify_LightweightM2M_1_1_int_221(shell, leshan, endpoint)

    # All done
    shell.exec_command('lwm2m stop')
    shell._device.readlines_until(regex=r'.*Deregistration success', timeout=10.0)
