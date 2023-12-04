"""
Various LwM2M interoperability tests
####################################

Copyright (c) 2023 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

Test specification:
===================
https://www.openmobilealliance.org/release/LightweightM2M/ETS/OMA-ETS-LightweightM2M-V1_1-20190912-D.pdf


This module contains testcases for
 * Registration Interface [100-199]
 * Device management & Service Enablement Interface [200-299]
 * Information Reporting Interface [300-399]

"""

import time
import logging
from datetime import datetime
import pytest
from leshan import Leshan

from twister_harness import Shell
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


def test_LightweightM2M_1_1_int_102(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-102 - Registration Update"""
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 1/0/1 -u32'))
    lifetime = int(lines[0])
    lifetime = lifetime + 10
    start_time = time.time() * 1000
    leshan.write(endpoint, '1/0/1', lifetime)
    dut.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)
    latest = leshan.get(f'/clients/{endpoint}')
    assert latest["lastUpdate"] > start_time
    assert latest["lastUpdate"] <= time.time()*1000
    assert latest["lifetime"] == lifetime
    shell.exec_command('lwm2m write 1/0/1 -u32 86400')

def test_LightweightM2M_1_1_int_103(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-103 - Deregistration"""
    leshan.execute(endpoint, '1/0/4')
    dut.readlines_until(regex='LwM2M server disabled', timeout=5.0)
    dut.readlines_until(regex='Deregistration success', timeout=5.0)
    # Reset timers by restarting the client
    shell.exec_command('lwm2m stop')
    time.sleep(1)
    shell.exec_command(f'lwm2m start {endpoint}')
    dut.readlines_until(regex='.*Registration Done', timeout=5.0)

def test_LightweightM2M_1_1_int_104(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-104 - Registration Update Trigger"""
    shell.exec_command('lwm2m update')
    dut.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)
    leshan.execute(endpoint, '1/0/8')
    dut.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)

@pytest.mark.slow
def test_LightweightM2M_1_1_int_107(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-107 - Extending the lifetime of a registration"""
    leshan.write(endpoint, '1/0/1', 120)
    dut.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 1/0/1 -u32'))
    lifetime = int(lines[0])
    assert lifetime == 120
    logger.debug(f'Wait for update, max {lifetime} s')
    dut.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=lifetime)
    assert leshan.get(f'/clients/{endpoint}')

def test_LightweightM2M_1_1_int_108(leshan, endpoint):
    """LightweightM2M-1.1-int-108 - Turn on Queue Mode"""
    assert leshan.get(f'/clients/{endpoint}')["queuemode"]

@pytest.mark.slow
def test_LightweightM2M_1_1_int_109(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-109 - Behavior in Queue Mode"""
    logger.debug('Wait for Queue RX OFF')
    dut.readlines_until(regex='.*Queue mode RX window closed', timeout=120)
    # Restore previous value
    shell.exec_command('lwm2m write 1/0/1 -u32 86400')
    dut.readlines_until(regex='.*Registration update complete', timeout=10)

def test_LightweightM2M_1_1_int_201(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-201 - Querying basic information in Plain Text format"""
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

def test_LightweightM2M_1_1_int_203(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-203 - Querying basic information in TLV format"""
    fmt = leshan.format
    leshan.format = 'TLV'
    resp = leshan.read(endpoint,'3/0')
    verify_device_object(resp)
    leshan.format = fmt

def test_LightweightM2M_1_1_int_204(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-204 - Querying basic information in JSON format"""
    fmt = leshan.format
    leshan.format = 'JSON'
    resp = leshan.read(endpoint, '3/0')
    verify_device_object(resp)
    leshan.format = fmt

def test_LightweightM2M_1_1_int_205(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-205 - Setting basic information in Plain Text format"""
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

def test_LightweightM2M_1_1_int_211(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-211 - Querying basic information in CBOR format"""
    fmt = leshan.format
    leshan.format = 'CBOR'
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 1/0/0 -u16'))
    short_id = int(lines[0])
    assert leshan.read(endpoint, '1/0/0') == short_id
    assert leshan.read(endpoint, '1/0/6') is False
    assert leshan.read(endpoint, '1/0/7') == 'U'
    leshan.format = fmt

def test_LightweightM2M_1_1_int_212(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-212 - Setting basic information in CBOR format"""
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
    del server_obj[0][13]
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

def test_LightweightM2M_1_1_int_215(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-215 - Setting basic information in TLV format"""
    verify_setting_basic_in_format(shell, leshan, endpoint, 'TLV')

def test_LightweightM2M_1_1_int_220(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-220 - Setting basic information in JSON format"""
    verify_setting_basic_in_format(shell, leshan, endpoint, 'JSON')

def test_LightweightM2M_1_1_int_221(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-221 - Attempt to perform operations on Security"""
    assert leshan.read(endpoint, '0/0')['status'] == 'UNAUTHORIZED(401)'
    assert leshan.write(endpoint, '0/0/0', 'coap://localhost')['status'] == 'UNAUTHORIZED(401)'
    assert leshan.write_attributes(endpoint, '0', {'pmin':10})['status'] == 'UNAUTHORIZED(401)'

def test_LightweightM2M_1_1_int_222(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-222 - Read on Object"""
    resp = leshan.read(endpoint, '1')
    assert len(resp) == 1
    assert len(resp[1][0]) == 11
    resp = leshan.read(endpoint, '3')
    assert len(resp) == 1
    assert len(resp[3]) == 1
    assert len(resp[3][0]) == 15
    assert resp[3][0][0] == 'Zephyr'

def test_LightweightM2M_1_1_int_223(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-223 - Read on Object Instance"""
    resp = leshan.read(endpoint, '1/0')
    assert len(resp[0]) == 11
    resp = leshan.read(endpoint, '3/0')
    assert len(resp[0]) == 15
    assert resp[0][0] == 'Zephyr'

def test_LightweightM2M_1_1_int_224(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-224 - Read on Resource"""
    assert leshan.read(endpoint, '1/0/0') == 1
    assert leshan.read(endpoint, '1/0/1') == 86400
    assert leshan.read(endpoint, '1/0/6') is False
    assert leshan.read(endpoint, '1/0/7') == 'U'

def test_LightweightM2M_1_1_int_225(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-225 - Read on Resource Instance"""
    assert leshan.read(endpoint, '3/0/11/0') == 0

def test_LightweightM2M_1_1_int_226(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-226 - Write (Partial Update) on Object Instance"""
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

def test_LightweightM2M_1_1_int_227(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-227 - Write (replace) on Resource"""
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 1/0/1 -u32'))
    lifetime = int(lines[0])
    assert leshan.write(endpoint, '1/0/1', int(63))['status'] == 'CHANGED(204)'
    dut.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)
    latest = leshan.get(f'/clients/{endpoint}')
    assert latest["lifetime"] == 63
    assert leshan.read(endpoint, '1/0/1') == 63
    assert leshan.write(endpoint, '1/0/1', lifetime)['status'] == 'CHANGED(204)'

def test_LightweightM2M_1_1_int_228(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-228 - Write on Resource Instance"""
    resources = {
        0: {0: 'a', 1: 'b'}
    }
    assert leshan.create_obj_instance(endpoint, '16/0', resources)['status'] == 'CREATED(201)'
    dut.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)
    assert leshan.write(endpoint, '16/0/0/0', 'test')['status'] == 'CHANGED(204)'
    assert leshan.read(endpoint, '16/0/0/0') == 'test'

def test_LightweightM2M_1_1_int_229(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-229 - Read-Composite Operation"""
    old_fmt = leshan.format
    for fmt in ['SENML_JSON', 'SENML_CBOR']:
        leshan.format = fmt
        resp = leshan.composite_read(endpoint, ['/3', '1/0'])
        assert len(resp.keys()) == 2
        assert resp[3] is not None
        assert resp[1][0] is not None
        assert len(resp[3][0]) == 15
        assert len(resp[1][0]) == 11

        resp = leshan.composite_read(endpoint, ['1/0/1', '/3/0/11/0'])
        logger.debug(resp)
        assert len(resp.keys()) == 2
        assert resp[1][0][1] is not None
        assert resp[3][0][11][0] is not None
    leshan.format = old_fmt

def test_LightweightM2M_1_1_int_230(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-230 - Write-Composite Operation"""
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
    """Querying basic information in one of the SenML formats"""
    old_fmt = leshan.format
    leshan.format = fmt
    verify_server_object(leshan.read(endpoint, '1')[1])
    verify_device_object(leshan.read(endpoint, '3/0'))
    assert leshan.read(endpoint, '3/0/16') == 'U'
    assert leshan.read(endpoint, '3/0/11/0') == 0
    leshan.format = old_fmt

def test_LightweightM2M_1_1_int_231(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-231 - Querying basic information in SenML JSON format"""
    query_basic_in_senml(leshan, endpoint, 'SENML_JSON')

def test_LightweightM2M_1_1_int_232(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-232 - Querying basic information in SenML CBOR format"""
    query_basic_in_senml(leshan, endpoint, 'SENML_CBOR')

def setting_basic_senml(shell: Shell, leshan: Leshan, endpoint: str, fmt: str):
    """Setting basic information in one of the SenML formats"""
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

def test_LightweightM2M_1_1_int_233(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-233 - Setting basic information in SenML CBOR format"""
    setting_basic_senml(shell, leshan, endpoint, 'SENML_CBOR')

def test_LightweightM2M_1_1_int_234(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-234 - Setting basic information in SenML JSON format"""
    setting_basic_senml(shell, leshan, endpoint, 'SENML_JSON')

def test_LightweightM2M_1_1_int_235(leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-235 - Read-Composite Operation on root path"""
    resp = leshan.composite_read(endpoint, ['/'])
    expected_keys = [1, 3, 5]
    missing_keys = [key for key in expected_keys if key not in resp.keys()]
    assert len(missing_keys) == 0

def test_LightweightM2M_1_1_int_236(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-236 - Read-Composite - Partial Presence"""
    resp = leshan.composite_read(endpoint, ['1/0', '/3/0/11/0', '/3339/0/5522', '/3353/0/6030'])
    assert resp[1][0][1] is not None
    assert resp[3][0][11][0] is not None
    assert len(resp) == 2

def test_LightweightM2M_1_1_int_237(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-237 - Read on Object without specifying Content-Type"""
    old_fmt = leshan.format
    leshan.format = None
    assert leshan.read(endpoint, '1')[1][0][1] is not None
    assert leshan.read(endpoint, '3')[3][0][0] == 'Zephyr'
    leshan.format = old_fmt

def test_LightweightM2M_1_1_int_241(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-241 - Executable Resource: Rebooting the device"""
    leshan.execute(endpoint, '3/0/4')
    dut.readlines_until(regex='.*DEVICE: REBOOT', timeout=5.0)
    dut.readlines_until(regex='.*rd_client_event: Disconnected', timeout=5.0)
    shell.exec_command(f'lwm2m start {endpoint} -b 0')
    dut.readlines_until(regex='.*Registration Done', timeout=5.0)
    assert leshan.get(f'/clients/{endpoint}')

def test_LightweightM2M_1_1_int_256(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-256 - Write Operation Failure"""
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read 1/0/0 -u16'))
    short_id = int(lines[0])
    assert leshan.write(endpoint, '1/0/0', 123)['status'] == 'METHOD_NOT_ALLOWED(405)'
    assert leshan.read(endpoint, '1/0/0') == short_id

def test_LightweightM2M_1_1_int_257(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-257 - Write-Composite Operation"""
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

def test_LightweightM2M_1_1_int_260(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-260 - Discover Command"""
    resp = leshan.discover(endpoint, '3')
    expected_keys = ['/3', '/3/0', '/3/0/1', '/3/0/2', '/3/0/3', '/3/0/4', '/3/0/6', '/3/0/7', '/3/0/8', '/3/0/9', '/3/0/11', '/3/0/16']
    missing_keys = [key for key in expected_keys if key not in resp.keys()]
    assert len(missing_keys) == 0
    assert leshan.write_attributes(endpoint, '3', {'pmin': 10, 'pmax': 200})['status'] == 'CHANGED(204)'
    resp = leshan.discover(endpoint, '3/0')
    assert int(resp['/3/0/6']['dim']) == 2
    assert int(resp['/3/0/7']['dim']) == 2
    assert int(resp['/3/0/8']['dim']) == 2
    assert leshan.write_attributes(endpoint, '3/0/7', {'lt': 1, 'gt': 6, 'st': 1})['status'] == 'CHANGED(204)'
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
    # restore
    leshan.remove_attributes(endpoint, '3', ['pmin', 'pmax'])

def test_LightweightM2M_1_1_int_261(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-261 - Write-Attribute Operation on a multiple resource"""
    resp = leshan.discover(endpoint, '3/0/11')
    logger.debug(resp)
    expected_keys = ['/3/0/11', '/3/0/11/0']
    missing_keys = [key for key in expected_keys if key not in resp.keys()]
    assert len(missing_keys) == 0
    assert len(resp) == len(expected_keys)
    assert int(resp['/3/0/11']['dim']) == 1
    assert leshan.write_attributes(endpoint, '3', {'pmin':10, 'pmax':200})['status'] == 'CHANGED(204)'
    assert leshan.write_attributes(endpoint, '3/0', {'pmax':320})['status'] == 'CHANGED(204)'
    assert leshan.write_attributes(endpoint, '3/0/11/0', {'pmax':100, 'epmin':1, 'epmax':20})['status'] == 'CHANGED(204)'
    resp = leshan.discover(endpoint, '3/0/11')
    logger.debug(resp)
    assert int(resp['/3/0/11']['pmin']) == 10
    assert int(resp['/3/0/11']['pmax']) == 320
    assert int(resp['/3/0/11/0']['pmax']) == 100
    # Note: Zephyr does not support epmin&epmax.
    # Restore
    leshan.remove_attributes(endpoint, '3', ['pmin', 'pmax'])
    leshan.remove_attributes(endpoint, '3/0', ['pmax'])
    leshan.remove_attributes(endpoint, '3/0/11/0', ['pmax'])


def test_LightweightM2M_1_1_int_280(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-280 - Successful Read-Composite Operation"""
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

def test_LightweightM2M_1_1_int_281(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-281 - Partially Successful Read-Composite Operation"""
    resp = leshan.composite_read(endpoint, ['/1/0/1', '/1/0/7', '/1/0/8'])
    assert len(resp) == 1
    assert len(resp[1][0]) == 2 # /1/0/8 should not be there
    assert resp[1][0][1] == 86400
    assert resp[1][0][7] == 'U'

#
# Information Reporting Interface [300-399]
#

@pytest.mark.slow
def test_LightweightM2M_1_1_int_301(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-301 - Observation and Notification of parameter values"""
    pwr_src = leshan.read(endpoint, '3/0/6')
    logger.debug(pwr_src)
    assert pwr_src[6][0] == 1
    assert pwr_src[6][1] == 5
    assert leshan.write_attributes(endpoint, '3/0/7', {'pmin': 5, 'pmax': 10})['status'] == 'CHANGED(204)'
    leshan.observe(endpoint, '3/0/7')
    with leshan.get_event_stream(endpoint, timeout=30) as events:
        shell.exec_command('lwm2m write /3/0/7/0 -u32 3000')
        data =  events.next_event('NOTIFICATION')
        assert data is not None
        assert data[3][0][7][0] == 3000
        # Ensure that we don't get new data before pMin
        start = time.time()
        shell.exec_command('lwm2m write /3/0/7/0 -u32 3500')
        data =  events.next_event('NOTIFICATION')
        assert data[3][0][7][0] == 3500
        assert (start + 5) < time.time() + 0.5 # Allow 0.5 second diff
        assert (start + 5) > time.time() - 0.5
        # Ensure that we get update when pMax expires
        data =  events.next_event('NOTIFICATION')
        assert data[3][0][7][0] == 3500
        assert (start + 15) <= time.time() + 1 # Allow 1 second slack. (pMinx + pMax=15)
    leshan.cancel_observe(endpoint, '3/0/7')
    leshan.remove_attributes(endpoint, '3/0/7', ['pmin', 'pmax'])

def test_LightweightM2M_1_1_int_302(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-302 - Cancel Observations using Reset Operation"""
    leshan.observe(endpoint, '3/0/7')
    leshan.observe(endpoint, '3/0/8')
    with leshan.get_event_stream(endpoint) as events:
        shell.exec_command('lwm2m write /3/0/7/0 -u32 4000')
        data =  events.next_event('NOTIFICATION')
        assert data[3][0][7][0] == 4000
    leshan.passive_cancel_observe(endpoint, '3/0/7')
    shell.exec_command('lwm2m write /3/0/7/0 -u32 3000')
    dut.readlines_until(regex=r'.*Observer removed for 3/0/7')
    with leshan.get_event_stream(endpoint) as events:
        shell.exec_command('lwm2m write /3/0/8/0 -u32 100')
        data =  events.next_event('NOTIFICATION')
        assert data[3][0][8][0] == 100
    leshan.passive_cancel_observe(endpoint, '3/0/8')
    shell.exec_command('lwm2m write /3/0/8/0 -u32 50')
    dut.readlines_until(regex=r'.*Observer removed for 3/0/8')

def test_LightweightM2M_1_1_int_303(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-303 - Cancel observations using Observe with Cancel parameter"""
    leshan.observe(endpoint, '3/0/7')
    leshan.observe(endpoint, '3/0/8')
    with leshan.get_event_stream(endpoint) as events:
        shell.exec_command('lwm2m write /3/0/7/0 -u32 4000')
        data =  events.next_event('NOTIFICATION')
        assert data[3][0][7][0] == 4000
    leshan.cancel_observe(endpoint, '3/0/7')
    dut.readlines_until(regex=r'.*Observer removed for 3/0/7')
    with leshan.get_event_stream(endpoint) as events:
        shell.exec_command('lwm2m write /3/0/8/0 -u32 100')
        data =  events.next_event('NOTIFICATION')
        assert data[3][0][8][0] == 100
    leshan.cancel_observe(endpoint, '3/0/8')
    dut.readlines_until(regex=r'.*Observer removed for 3/0/8')

@pytest.mark.slow
def test_LightweightM2M_1_1_int_304(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-304 - Observe-Composite Operation"""
    # Need to use Configuration C.1
    shell.exec_command('lwm2m write 1/0/2 -u32 0')
    shell.exec_command('lwm2m write 1/0/3 -u32 0')
    assert leshan.write_attributes(endpoint, '1/0/1', {'pmin': 30, 'pmax': 45})['status'] == 'CHANGED(204)'
    data = leshan.composite_observe(endpoint, ['/1/0/1', '/3/0/11/0', '/3/0/16'])
    assert data[1][0][1] is not None
    assert data[3][0][11][0] is not None
    assert data[3][0][16] == 'U'
    assert len(data) == 2
    assert len(data[1]) == 1
    assert len(data[3][0]) == 2
    start = time.time()
    with leshan.get_event_stream(endpoint, timeout=50) as events:
        data =  events.next_event('NOTIFICATION')
        logger.debug(data)
        assert data[1][0][1] is not None
        assert data[3][0][11][0] is not None
        assert data[3][0][16] == 'U'
        assert len(data) == 2
        assert len(data[1]) == 1
        assert len(data[3][0]) == 2
        assert (start + 30) < time.time()
        assert (start + 45) > time.time() - 1
    leshan.cancel_composite_observe(endpoint, ['/1/0/1', '/3/0/11/0', '/3/0/16'])
    # Restore configuration C.3
    shell.exec_command('lwm2m write 1/0/2 -u32 1')
    shell.exec_command('lwm2m write 1/0/3 -u32 10')
    leshan.remove_attributes(endpoint, '1/0/1', ['pmin', 'pmax'])

def test_LightweightM2M_1_1_int_305(dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-305 - Cancel Observation-Composite Operation"""
    leshan.composite_observe(endpoint, ['/1/0/1', '/3/0/11/0', '/3/0/16'])
    leshan.cancel_composite_observe(endpoint, ['/1/0/1', '/3/0/11/0', '/3/0/16'])
    dut.readlines_until(regex=r'.*Observer removed for 1/0/1')
    dut.readlines_until(regex=r'.*Observer removed for 3/0/11/0')
    dut.readlines_until(regex=r'.*Observer removed for 3/0/16')

def test_LightweightM2M_1_1_int_306(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-306 - Send Operation"""
    with leshan.get_event_stream(endpoint) as events:
        shell.exec_command('lwm2m send /1 /3')
        dut.readlines_until(regex=r'.*SEND status: 0', timeout=5.0)
        data = events.next_event('SEND')
        assert data is not None
        verify_server_object(data[1])
        verify_device_object(data[3])

def test_LightweightM2M_1_1_int_307(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-307 - Muting Send"""
    leshan.write(endpoint, '1/0/23', True)
    lines = shell.get_filtered_output(shell.exec_command('lwm2m send /3/0'))
    assert any("can't do send operation" in line for line in lines)
    leshan.write(endpoint, '1/0/23', False)
    shell.exec_command('lwm2m send /3/0')
    dut.readlines_until(regex=r'.*SEND status: 0', timeout=5.0)


@pytest.mark.slow
def test_LightweightM2M_1_1_int_308(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-308 - Observe-Composite and Creating Object Instance"""
    shell.exec_command('lwm2m delete /16/0')
    shell.exec_command('lwm2m delete /16/1')
    # Need to use Configuration C.1
    shell.exec_command('lwm2m write 1/0/2 -u32 0')
    shell.exec_command('lwm2m write 1/0/3 -u32 0')
    resources_a = {
        0: {0: 'aa',
            1: 'bb',
            2: 'cc',
            3: 'dd'}
    }
    content_one = {16: {0: resources_a}}
    resources_b = {
        0: {0: '11',
            1: '22',
            2: '33',
            3: '44'}
    }
    content_both = {16: {0: resources_a, 1: resources_b}}
    assert leshan.create_obj_instance(endpoint, '16/0', resources_a)['status'] == 'CREATED(201)'
    dut.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)
    assert leshan.write_attributes(endpoint, '16/0', {'pmin': 30, 'pmax': 45})['status'] == 'CHANGED(204)'
    data = leshan.composite_observe(endpoint, ['/16/0', '/16/1'])
    assert data == content_one
    with leshan.get_event_stream(endpoint, timeout=50) as events:
        data =  events.next_event('NOTIFICATION')
        start = time.time()
        assert data == content_one
        assert leshan.create_obj_instance(endpoint, '16/1', resources_b)['status'] == 'CREATED(201)'
        data =  events.next_event('NOTIFICATION')
        assert (start + 30) < time.time() + 2
        assert (start + 45) > time.time() - 2
        assert data == content_both
    leshan.cancel_composite_observe(endpoint, ['/16/0', '/16/1'])
    # Restore configuration C.3
    shell.exec_command('lwm2m write 1/0/2 -u32 1')
    shell.exec_command('lwm2m write 1/0/3 -u32 10')
    leshan.remove_attributes(endpoint, '16/0', ['pmin','pmax'])

@pytest.mark.slow
def test_LightweightM2M_1_1_int_309(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-309 - Observe-Composite and Deleting Object Instance"""
    shell.exec_command('lwm2m delete /16/0')
    shell.exec_command('lwm2m delete /16/1')
    # Need to use Configuration C.1
    shell.exec_command('lwm2m write 1/0/2 -u32 0')
    shell.exec_command('lwm2m write 1/0/3 -u32 0')
    resources_a = {
        0: {0: 'aa',
            1: 'bb',
            2: 'cc',
            3: 'dd'}
    }
    content_one = {16: {0: resources_a}}
    resources_b = {
        0: {0: '11',
            1: '22',
            2: '33',
            3: '44'}
    }
    content_both = {16: {0: resources_a, 1: resources_b}}
    assert leshan.create_obj_instance(endpoint, '16/0', resources_a)['status'] == 'CREATED(201)'
    assert leshan.create_obj_instance(endpoint, '16/1', resources_b)['status'] == 'CREATED(201)'
    dut.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)
    assert leshan.write_attributes(endpoint, '16/0', {'pmin': 30, 'pmax': 45})['status'] == 'CHANGED(204)'
    data = leshan.composite_observe(endpoint, ['/16/0', '/16/1'])
    assert data == content_both
    with leshan.get_event_stream(endpoint, timeout=50) as events:
        data =  events.next_event('NOTIFICATION')
        start = time.time()
        assert data == content_both
        assert leshan.delete(endpoint, '16/1')['status'] == 'DELETED(202)'
        data =  events.next_event('NOTIFICATION')
        assert (start + 30) < time.time() + 2
        assert (start + 45) > time.time() - 2
        assert data == content_one
    leshan.cancel_composite_observe(endpoint, ['/16/0', '/16/1'])
    # Restore configuration C.3
    shell.exec_command('lwm2m write 1/0/2 -u32 1')
    shell.exec_command('lwm2m write 1/0/3 -u32 10')
    leshan.remove_attributes(endpoint, '16/0', ['pmin', 'pmax'])

@pytest.mark.slow
def test_LightweightM2M_1_1_int_310(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-310 - Observe-Composite and modification of parameter values"""
    # Need to use Configuration C.1
    shell.exec_command('lwm2m write 1/0/2 -u32 0')
    shell.exec_command('lwm2m write 1/0/3 -u32 0')
    leshan.composite_observe(endpoint, ['/1/0/1', '/3/0'])
    with leshan.get_event_stream(endpoint, timeout=50) as events:
        assert leshan.write_attributes(endpoint, '3', {'pmax': 5})['status'] == 'CHANGED(204)'
        start = time.time()
        data =  events.next_event('NOTIFICATION')
        assert data[3][0][0] == 'Zephyr'
        assert data[1] == {0: {1: 86400}}
        assert (start + 5) > time.time() - 1
        start = time.time()
        data =  events.next_event('NOTIFICATION')
        assert (start + 5) > time.time() - 1
    leshan.cancel_composite_observe(endpoint, ['/1/0/1', '/3/0'])
    # Restore configuration C.3
    shell.exec_command('lwm2m write 1/0/2 -u32 1')
    shell.exec_command('lwm2m write 1/0/3 -u32 10')
    leshan.remove_attributes(endpoint, '3', ['pmax'])

def test_LightweightM2M_1_1_int_311(shell: Shell, leshan: Leshan, endpoint: str):
    """LightweightM2M-1.1-int-311 - Send command"""
    with leshan.get_event_stream(endpoint, timeout=50) as events:
        shell.exec_command('lwm2m send /1/0/1 /3/0/11')
        data =  events.next_event('SEND')
        assert data == {3: {0: {11: {0: 0}}}, 1: {0: {1: 86400}}}
