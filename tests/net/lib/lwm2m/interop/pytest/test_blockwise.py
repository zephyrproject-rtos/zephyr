"""
Tests for Block-Wise transfers in LwM2M
#######################################

Copyright (c) 2024 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0


"""

import binascii
import logging
import random
import re
import string
import time
import zlib

from leshan import Leshan
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)

def test_blockwise_1(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """Blockwise test 1: Block-Wise PUT using OPAQUE content format"""

    fw = b'1234567890' * 500
    fmt = leshan.format
    to = leshan.timeout
    leshan.format = 'OPAQUE'
    leshan.timeout = 600
    leshan.write(endpoint, '5/0/0', fw)
    # Our Firmware object prints out the CRC of the received firmware
    # when Update is executed
    leshan.execute(endpoint, '5/0/2')
    lines = dut.readlines_until(regex='app_fw_update: UPDATE', timeout=5.0)
    assert len(lines) > 0
    line = lines[-1]
    crc = int(re.search('CRC ([0-9]+)', line).group(1))
    # Verify that CRC matches
    assert crc == zlib.crc32(fw)
    leshan.format =  fmt
    leshan.timeout = to

def test_blockwise_2(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """Blockwise test 2: Block-Wise PUT with retry"""

    fw = b'1234567890' * 500
    fmt = leshan.format
    to = leshan.timeout
    leshan.format = 'OPAQUE'
    # Set timeout to 1 second to force Leshan to stop sending
    leshan.timeout = 1
    try:
        leshan.write(endpoint, '5/0/0', fw)
    except Exception as e:
        logger.debug(f'Caught exception: {e}')
        shell.exec_command('lwm2m update')
        time.sleep(1)
    # Now send the firmware again using longer timeout
    leshan.timeout = 600
    leshan.write(endpoint, '5/0/0', fw)
    # Our Firmware object prints out the CRC of the received firmware
    # when Update is executed
    leshan.execute(endpoint, '5/0/2')
    lines = dut.readlines_until(regex='app_fw_update: UPDATE', timeout=5.0)
    assert len(lines) > 0
    line = lines[-1]
    crc = int(re.search('CRC ([0-9]+)', line).group(1))
    # Verify that CRC matches
    assert crc == zlib.crc32(fw)
    leshan.format =  fmt
    leshan.timeout = to


def test_blockwise_3(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """Blockwise test 3: Block-Wise Get using TLV and SenML-CBOR content formats"""

    shell.exec_command('lwm2m create /19/0')
    # Wait for update to finish so server is aware of the /19/0 object
    dut.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)

    # Generate 4 kB of binary app-data
    # and write it into BinaryAppData object
    data = ''.join(random.choice(string.ascii_letters) for i in range(4096)).encode()
    fmt = leshan.format
    to = leshan.timeout
    leshan.format = 'OPAQUE'
    leshan.timeout = 600
    leshan.write(endpoint, '19/0/0/0', data)

    # Verify data is correctly transferred
    lines = shell.get_filtered_output(shell.exec_command('lwm2m read /19/0/0/0 -crc32'))
    crc = int(lines[0])
    assert crc == zlib.crc32(data)

    # Try reading the data using different content formats
    for fmt in ['TLV', 'SENML_CBOR']:
        leshan.format = fmt
        data = leshan.read(endpoint, '19/0/0')
        data = binascii.a2b_hex(data[0][0])
        assert crc == zlib.crc32(data)

    leshan.format =  fmt
    leshan.timeout = to
    shell.exec_command('lwm2m delete /19/0')

def test_blockwise_4(shell: Shell, dut: DeviceAdapter, leshan: Leshan, endpoint: str):
    """Blockwise test 4: Block-Wise SEND using SenML-CBOR content format"""

    shell.exec_command('lwm2m create /19/0')
    # Wait for update to finish so server is aware of the /19/0 object
    dut.readlines_until(regex='.*net_lwm2m_rd_client: Update Done', timeout=5.0)

    # Generate 4 kB of binary app-data
    data = ''.join(random.choice(string.ascii_letters) for i in range(4096)).encode()
    fmt = leshan.format
    to = leshan.timeout
    leshan.format = 'OPAQUE'
    leshan.timeout = 600
    leshan.write(endpoint, '19/0/0/0', data)
    leshan.format = 'SENML_CBOR'

    # Execute SEND and verify that correct data is received
    with leshan.get_event_stream(endpoint) as events:
        shell.exec_command('lwm2m send /19/0')
        dut.readlines_until(regex=r'.*SEND status: 0', timeout=5.0)
        send = events.next_event('SEND')
        assert send is not None
        content = binascii.a2b_hex(send[19][0][0][0])
        assert zlib.crc32(content) == zlib.crc32(data)

    leshan.format =  fmt
    leshan.timeout = to

    shell.exec_command('lwm2m delete /19/0')
