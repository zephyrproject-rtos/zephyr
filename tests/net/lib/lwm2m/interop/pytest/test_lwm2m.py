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
from datetime import datetime

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
    lifetime = int(lines[0])
    lifetime = lifetime + 10
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
    regid = status["registrationId"]
    assert regid
    # Fake unregister message
    helperclient.delete(f'rd/{regid}', timeout=0.1)
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
    logger.debug('Wait for Queue RX OFF')
    shell._device.readlines_until(regex='.*Queue mode RX window closed', timeout=120)
    # Restore previous value
    shell.exec_command('lwm2m write 1/0/1 -u32 86400')
    shell._device.readlines_until(regex='.*Registration update complete', timeout=10)

def verify_LightweightM2M_1_1_int_201(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-201 - Querying basic information in Plain Text format")
    fmt = leshan.format
    leshan.format = 'TEXT'
    assert leshan.read(endpoint, '3/0/0') == 'Zephyr'
    assert leshan.read(endpoint, '3/0/1') == 'client-1'
    assert leshan.read(endpoint, '3/0/2') == 'serial-1'
    leshan.format = fmt

def verify_device_object(resp):
    ''' Verify that Device object match Configuration 3 '''
    assert resp[0][0] == 'Zephyr'
    assert resp[0][1] == 'client-1'
    assert resp[0][2] == 'serial-1'
    assert resp[0][3] == '1.2.3'
    assert resp[0][11][0] == 0
    assert resp[0][16] == 'U'

def verify_server_object(obj):
    ''' Verify that server object match Configuration 3 '''
    assert obj[0][0] == 1
    assert obj[0][1] == 86400
    assert obj[0][2] == 1
    assert obj[0][3] == 10
    assert obj[0][5] == 86400
    assert obj[0][6] is False
    assert obj[0][7] == 'U'

def verify_LightweightM2M_1_1_int_203(shell: Shell, leshan: Leshan, endpoint: str):
    shell.exec_command('lwm2m update')
    logger.info('LightweightM2M-1.1-int-203 - Querying basic information in TLV format')
    fmt = leshan.format
    leshan.format = 'TLV'
    resp = leshan.read(endpoint,'3/0')
    verify_device_object(resp)
    leshan.format = fmt

def verify_LightweightM2M_1_1_int_204(shell: Shell, leshan: Leshan, endpoint: str):
    shell.exec_command('lwm2m update')
    logger.info('LightweightM2M-1.1-int-204 - Querying basic information in JSON format')
    fmt = leshan.format
    leshan.format = 'JSON'
    resp = leshan.read(endpoint, '3/0')
    verify_device_object(resp)
    leshan.format = fmt

def verify_LightweightM2M_1_1_int_205(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info('LightweightM2M-1.1-int-205 - Setting basic information in Plain Text format')
    fmt = leshan.format
    leshan.format = 'TEXT'
    leshan.write(endpoint, '1/0/2', 101)
    leshan.write(endpoint, '1/0/3', 1010)
    leshan.write(endpoint, '1/0/5', 2000)
    assert leshan.read(endpoint, '1/0/2') == 101
    assert leshan.read(endpoint, '1/0/3') == 1010
    assert leshan.read(endpoint, '1/0/5') == 2000
    leshan.write(endpoint, '1/0/2', 1)
    leshan.write(endpoint, '1/0/3', 10)
    leshan.write(endpoint, '1/0/5', 86400)
    assert leshan.read(endpoint, '1/0/2') == 1
    assert leshan.read(endpoint, '1/0/3') == 10
    assert leshan.read(endpoint, '1/0/5') == 86400
    leshan.format = fmt

def verify_LightweightM2M_1_1_int_211(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info('LightweightM2M-1.1-int-211 - Querying basic information in CBOR format')
    fmt = leshan.format
    leshan.format = 'CBOR'
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 1/0/0 -u16'))
    short_id = int(lines[0])
    assert leshan.read(endpoint, '1/0/0') == short_id
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
    assert leshan.read(endpoint, '1/0/2') == 101
    assert leshan.read(endpoint, '1/0/3') == 1010
    assert leshan.read(endpoint, '1/0/6') is True
    leshan.write(endpoint, '1/0/2', 1)
    leshan.write(endpoint, '1/0/3', 10)
    leshan.write(endpoint, '1/0/6', False)
    leshan.format = fmt

def verify_setting_basic_in_format(shell, leshan, endpoint, format):
    fmt = leshan.format
    leshan.format = format
    server_obj = leshan.read(endpoint, '1/0')
    verify_server_object(server_obj)
    # Remove Read-Only resources, so we don't end up writing those
    del server_obj[0][0]
    data = {
        2: 101,
        3: 1010,
        5: 2000,
        6: True,
        7: 'U'
    }
    assert leshan.update_obj_instance(endpoint, '1/0', data)['status'] == 'CHANGED(204)'
    resp = leshan.read(endpoint, '1/0')
    assert resp[0][2] == 101
    assert resp[0][3] == 1010
    assert resp[0][5] == 2000
    assert resp[0][6] is True
    assert resp[0][7] == 'U'
    assert leshan.replace_obj_instance(endpoint, '1/0', server_obj[0])['status'] == 'CHANGED(204)'
    server_obj = leshan.read(endpoint, '1/0')
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

def verify_LightweightM2M_1_1_int_222(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-222 - Read on Object")
    resp = leshan.read(endpoint, '1')
    assert len(resp) == 1
    assert len(resp[1][0]) == 9
    resp = leshan.read(endpoint, '3')
    assert len(resp) == 1
    assert len(resp[3]) == 1
    assert len(resp[3][0]) == 15
    assert resp[3][0][0] == 'Zephyr'

def verify_LightweightM2M_1_1_int_223(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-223 - Read on Object Instance")
    resp = leshan.read(endpoint, '1/0')
    assert len(resp[0]) == 9
    resp = leshan.read(endpoint, '3/0')
    assert len(resp[0]) == 15
    assert resp[0][0] == 'Zephyr'

def verify_LightweightM2M_1_1_int_224(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-224 - Read on Resource")
    assert leshan.read(endpoint, '1/0/0') == 1
    assert leshan.read(endpoint, '1/0/1') == 86400
    assert leshan.read(endpoint, '1/0/6') is False
    assert leshan.read(endpoint, '1/0/7') == 'U'

def verify_LightweightM2M_1_1_int_225(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-225 - Read on Resource Instance")
    assert leshan.read(endpoint, '3/0/11/0') == 0

def verify_LightweightM2M_1_1_int_226(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-226 - Write (Partial Update) on Object Instance")
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 1/0/1 -u32'))
    lifetime = int(lines[0])
    resources = {
        1: 60,
        6: True
    }
    assert leshan.update_obj_instance(endpoint, '1/0', resources)['status'] == 'CHANGED(204)'
    assert leshan.read(endpoint, '1/0/1') == 60
    assert leshan.read(endpoint, '1/0/6') is True
    resources = {
        1: lifetime,
        6: False
    }
    assert leshan.update_obj_instance(endpoint, '1/0', resources)['status'] == 'CHANGED(204)'

def verify_LightweightM2M_1_1_int_227(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-227 - Write (replace) on Resource")
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 1/0/1 -u32'))
    lifetime = int(lines[0])
    assert leshan.write(endpoint, '1/0/1', int(63))['status'] == 'CHANGED(204)'
    shell._device.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)
    latest = leshan.get(f'/clients/{endpoint}')
    assert latest["lifetime"] == 63
    assert leshan.read(endpoint, '1/0/1') == 63
    assert leshan.write(endpoint, '1/0/1', lifetime)['status'] == 'CHANGED(204)'

def verify_LightweightM2M_1_1_int_228(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-228 - Write on Resource Instance")
    resources = {
        0: {0: 'a', 1: 'b'}
    }
    assert leshan.create_obj_instance(endpoint, '16/0', resources)['status'] == 'CREATED(201)'
    shell._device.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)
    assert leshan.write(endpoint, '16/0/0/0', 'test')['status'] == 'CHANGED(204)'
    assert leshan.read(endpoint, '16/0/0/0') == 'test'

def verify_LightweightM2M_1_1_int_229(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-229 - Read-Composite Operation")
    old_fmt = leshan.format
    for fmt in ['SENML_JSON', 'SENML_CBOR']:
        leshan.format = fmt
        resp = leshan.composite_read(endpoint, ['/3', '1/0'])
        assert len(resp.keys()) == 2
        assert resp[3] is not None
        assert resp[1][0] is not None
        assert len(resp[3][0]) == 15
        assert len(resp[1][0]) == 9

        resp = leshan.composite_read(endpoint, ['1/0/1', '/3/0/11/0'])
        logger.debug(resp)
        assert len(resp.keys()) == 2
        assert resp[1][0][1] is not None
        assert resp[3][0][11][0] is not None
    leshan.format = old_fmt

def verify_LightweightM2M_1_1_int_230(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-230 - Write-Composite Operation")
    resources = {
        "/1/0/1": 60,
        "/1/0/6": True,
        "/16/0/0": {
            "0": "aa",
            "1": "bb",
            "2": "cc",
            "3": "dd"
        }
    }
    old_fmt = leshan.format
    for fmt in ['SENML_JSON', 'SENML_CBOR']:
        leshan.format = fmt
        assert leshan.composite_write(endpoint, resources)['status'] == 'CHANGED(204)'
        resp = leshan.read(endpoint, '1/0')
        assert resp[0][1] == 60
        assert resp[0][6] is True
        resp = leshan.read(endpoint, '16/0/0')
        assert resp[0][0] == "aa"
        assert resp[0][1] == "bb"
        assert resp[0][2] == "cc"
        assert resp[0][3] == "dd"
        # Return to default
        shell.exec_command('lwm2m write /1/0/1 -u32 86400')
        shell.exec_command('lwm2m write /1/0/6 -u8 0')
    leshan.format = old_fmt

def query_basic_in_senml(leshan: Leshan, endpoint: str, fmt: str):
    old_fmt = leshan.format
    leshan.format = fmt
    verify_server_object(leshan.read(endpoint, '1')[1])
    verify_device_object(leshan.read(endpoint, '3/0'))
    assert leshan.read(endpoint, '3/0/16') == 'U'
    assert leshan.read(endpoint, '3/0/11/0') == 0
    leshan.format = old_fmt

def verify_LightweightM2M_1_1_int_231(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-231 - Querying basic information in SenML JSON format")
    query_basic_in_senml(leshan, endpoint, 'SENML_JSON')

def verify_LightweightM2M_1_1_int_232(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-232 - Querying basic information in SenML CBOR format")
    query_basic_in_senml(leshan, endpoint, 'SENML_CBOR')

def setting_basic_senml(shell: Shell, leshan: Leshan, endpoint: str, fmt: str):
    old_fmt = leshan.format
    leshan.format = fmt
    resources = {
        1: 61,
        6: True,
    }
    assert leshan.update_obj_instance(endpoint, '1/0', resources)['status'] == 'CHANGED(204)'
    srv_obj = leshan.read(endpoint, '1/0')
    assert srv_obj[0][1] == 61
    assert srv_obj[0][6] is True
    assert leshan.write(endpoint, '16/0/0/0', 'test_value')['status'] == 'CHANGED(204)'
    portfolio = leshan.read(endpoint, '16')
    assert portfolio[16][0][0][0] == 'test_value'
    assert leshan.write(endpoint, '1/0/1', 63)['status'] == 'CHANGED(204)'
    assert leshan.read(endpoint, '1/0/1') == 63
    shell.exec_command('lwm2m write /1/0/1 -u32 86400')
    shell.exec_command('lwm2m write /1/0/6 -u8 0')
    leshan.format = old_fmt

def verify_LightweightM2M_1_1_int_233(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-233 - Setting basic information in SenML CBOR format")
    setting_basic_senml(shell, leshan, endpoint, 'SENML_CBOR')

def verify_LightweightM2M_1_1_int_234(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-234 - Setting basic information in SenML JSON format")
    setting_basic_senml(shell, leshan, endpoint, 'SENML_JSON')

def verify_LightweightM2M_1_1_int_235():
    """LightweightM2M-1.1-int-235 - Read-Composite Operation on root path"""
     # Unsupported. Leshan does not allow this.

def verify_LightweightM2M_1_1_int_236(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-236 - Read-Composite - Partial Presence")
    resp = leshan.composite_read(endpoint, ['1/0', '/3/0/11/0', '/3339/0/5522', '/3353/0/6030'])
    assert resp[1][0][1] is not None
    assert resp[3][0][11][0] is not None
    assert len(resp) == 2

def verify_LightweightM2M_1_1_int_237(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-237 - Read on Object without specifying Content-Type")
    old_fmt = leshan.format
    leshan.format = None
    assert leshan.read(endpoint, '1')[1][0][1] is not None
    assert leshan.read(endpoint, '3')[3][0][0] == 'Zephyr'
    leshan.format = old_fmt

def verify_LightweightM2M_1_1_int_241(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-241 - Executable Resource: Rebooting the device")
    leshan.execute(endpoint, '3/0/4')
    shell._device.readlines_until(regex='.*DEVICE: REBOOT', timeout=5.0)
    shell._device.readlines_until(regex='.*rd_client_event: Disconnected', timeout=5.0)
    shell.exec_command(f'lwm2m start {endpoint} -b 0')
    shell._device.readlines_until(regex='.*Registration Done', timeout=5.0)
    assert leshan.get(f'/clients/{endpoint}')

def verify_LightweightM2M_1_1_int_256(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-256 - Write Operation Failure")
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 1/0/0 -u16'))
    short_id = int(lines[0])
    assert leshan.write(endpoint, '1/0/0', 123)['status'] == 'METHOD_NOT_ALLOWED(405)'
    assert leshan.read(endpoint, '1/0/0') == short_id

def verify_LightweightM2M_1_1_int_257(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-257 - Write-Composite Operation")
    resources = {
        "/1/0/2": 102,
        "/1/0/6": True,
        "/3/0/13": datetime.fromtimestamp(0)
    }
    old_fmt = leshan.format
    for fmt in ['SENML_JSON', 'SENML_CBOR']:
        leshan.format = fmt
        assert leshan.composite_write(endpoint, resources)['status'] == 'CHANGED(204)'
        assert leshan.read(endpoint, '1/0/2') == 102
        assert leshan.read(endpoint, '1/0/6') is True
        # Cannot verify the /3/0/13, it is a timestamp that moves forward.

        # Return to default
        shell.exec_command(f'lwm2m write /3/0/13 -u32 {int(datetime.now().timestamp())}')
        shell.exec_command('lwm2m write /1/0/6 -u8 0')
        shell.exec_command('lwm2m write /1/0/2 -u32 1')
    leshan.format = old_fmt

def verify_LightweightM2M_1_1_int_260(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-260 - Discover Command")
    resp = leshan.discover(endpoint, '3')
    expected_keys = ['/3', '/3/0', '/3/0/1', '/3/0/2', '/3/0/3', '/3/0/4', '/3/0/6', '/3/0/7', '/3/0/8', '/3/0/9', '/3/0/11', '/3/0/16']
    missing_keys = [key for key in expected_keys if key not in resp.keys()]
    assert len(missing_keys) == 0
    assert leshan.put_raw(f'/clients/{endpoint}/3/attributes?pmin=10')['status'] == 'CHANGED(204)'
    assert leshan.put_raw(f'/clients/{endpoint}/3/attributes?pmax=200')['status'] == 'CHANGED(204)'
    resp = leshan.discover(endpoint, '3/0')
    assert int(resp['/3/0/6']['dim']) == 2
    assert int(resp['/3/0/7']['dim']) == 2
    assert int(resp['/3/0/8']['dim']) == 2
    assert leshan.put_raw(f'/clients/{endpoint}/3/0/7/attributes?lt=1')['status'] == 'CHANGED(204)'
    assert leshan.put_raw(f'/clients/{endpoint}/3/0/7/attributes?gt=6')['status'] == 'CHANGED(204)'
    assert leshan.put_raw(f'/clients/{endpoint}/3/0/7/attributes?st=1')['status'] == 'CHANGED(204)'
    resp = leshan.discover(endpoint, '3/0')
    expected_keys = ['/3/0', '/3/0/1', '/3/0/2', '/3/0/3', '/3/0/4', '/3/0/6', '/3/0/7', '/3/0/8', '/3/0/9', '/3/0/11', '/3/0/16']
    missing_keys = [key for key in expected_keys if key not in resp.keys()]
    assert len(missing_keys) == 0
    assert int(resp['/3/0/7']['dim']) == 2
    assert float(resp['/3/0/7']['lt']) == 1.0
    assert float(resp['/3/0/7']['gt']) == 6.0
    assert float(resp['/3/0/7']['st']) == 1.0
    resp = leshan.discover(endpoint, '3/0/7')
    expected_keys = ['/3/0/7', '/3/0/7/0', '/3/0/7/1']
    missing_keys = [key for key in expected_keys if key not in resp.keys()]
    assert len(missing_keys) == 0
    assert len(resp) == len(expected_keys)

def verify_LightweightM2M_1_1_int_261(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-261 - Write-Attribute Operation on a multiple resource")
    resp = leshan.discover(endpoint, '3/0/11')
    logger.debug(resp)
    expected_keys = ['/3/0/11', '/3/0/11/0']
    missing_keys = [key for key in expected_keys if key not in resp.keys()]
    assert len(missing_keys) == 0
    assert len(resp) == len(expected_keys)
    assert int(resp['/3/0/11']['dim']) == 1
    assert leshan.put_raw(f'/clients/{endpoint}/3/attributes?pmin=10')['status'] == 'CHANGED(204)'
    assert leshan.put_raw(f'/clients/{endpoint}/3/attributes?pmax=200')['status'] == 'CHANGED(204)'
    assert leshan.put_raw(f'/clients/{endpoint}/3/0/attributes?pmax=320')['status'] == 'CHANGED(204)'
    assert leshan.put_raw(f'/clients/{endpoint}/3/0/11/0/attributes?pmax=100')['status'] == 'CHANGED(204)'
    assert leshan.put_raw(f'/clients/{endpoint}/3/0/11/0/attributes?epmin=1')['status'] == 'CHANGED(204)'
    assert leshan.put_raw(f'/clients/{endpoint}/3/0/11/0/attributes?epmax=20')['status'] == 'CHANGED(204)'
    resp = leshan.discover(endpoint, '3/0/11')
    logger.debug(resp)
    assert int(resp['/3/0/11']['pmin']) == 10
    assert int(resp['/3/0/11']['pmax']) == 320
    assert int(resp['/3/0/11/0']['pmax']) == 100
    assert int(resp['/3/0/11/0']['epmin']) == 1
    assert int(resp['/3/0/11/0']['epmax']) == 20

def verify_LightweightM2M_1_1_int_280(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-280 - Successful Read-Composite Operation")
    resp = leshan.composite_read(endpoint, ['/3/0/16', '/3/0/11/0', '/1/0'])
    logger.debug(resp)
    assert len(resp) == 2
    assert len(resp[3]) == 1
    assert len(resp[3][0]) == 2 # No extra resources
    assert resp[3][0][11][0] == 0
    assert resp[3][0][16] == 'U'
    assert resp[1][0][0] == 1
    assert resp[1][0][1] == 86400
    assert resp[1][0][6] is False
    assert resp[1][0][7] == 'U'

def verify_LightweightM2M_1_1_int_281(shell: Shell, leshan: Leshan, endpoint: str):
    logger.info("LightweightM2M-1.1-int-281 - Partially Successful Read-Composite Operation")
    resp = leshan.composite_read(endpoint, ['/1/0/1', '/1/0/7', '/1/0/8'])
    assert len(resp) == 1
    assert len(resp[1][0]) == 2 # /1/0/8 should not be there
    assert resp[1][0][1] == 86400
    assert resp[1][0][7] == 'U'

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
        bs_passwd = ''.join(random.choice(string.ascii_lowercase) for i in range(16))
        passwd = ''.join(random.choice(string.ascii_lowercase) for i in range(16))

        logger.debug('Endpoint: %s', endpoint)
        logger.debug('Boostrap PSK: %s', binascii.b2a_hex(bs_passwd.encode()).decode())
        logger.debug('PSK: %s', binascii.b2a_hex(passwd.encode()).decode())

        # Create device entries in Leshan and Bootstrap server
        leshan_bootstrap.create_bs_device(endpoint, f'coaps://{LESHAN_IP}:{COAPS_PORT}', bs_passwd, passwd)
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
        shell.exec_command(f'lwm2m write 0/0/5 -s {bs_passwd}')
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
        verify_LightweightM2M_1_1_int_220(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_221(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_222(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_223(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_224(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_225(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_226(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_227(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_228(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_229(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_230(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_231(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_232(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_233(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_234(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_236(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_237(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_241(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_256(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_257(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_260(shell, leshan, endpoint)
        # skip, not supported in Leshan, verify_LightweightM2M_1_1_int_261(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_280(shell, leshan, endpoint)
        verify_LightweightM2M_1_1_int_281(shell, leshan, endpoint)

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

    # All done
    shell.exec_command('lwm2m stop')
    shell._device.readlines_until(regex=r'.*Deregistration success', timeout=10.0)
