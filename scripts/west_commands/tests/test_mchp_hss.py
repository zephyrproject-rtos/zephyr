# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import binascii
import logging
import zlib

import pytest

from runners.mchp_hss import (
    ACK,
    EOT,
    STX,
    HssConsole,
    HssError,
    MchpHssBinaryRunner,
    ymodem_block,
    ymodem_header,
)


class FakeBuildConf(dict):
    def getboolean(self, option):
        return self.get(option, False)


def make_runner(runner_config, conf, **kwargs):
    runner = MchpHssBinaryRunner(runner_config, **kwargs)
    runner._build_conf = FakeBuildConf(conf)
    return runner


class FakeHss:
    '''Serial port stand-in that answers like the HSS console.'''

    def __init__(self, write_ok=True, boot_name='Zephyr'):
        self.out = b''
        self.write_ok = write_ok
        self.boot_name = boot_name
        self.in_ymodem = False
        self.pending = b''
        self.data = b''
        self.size = None
        self.commands = []

    # pyserial API used by HssConsole
    @property
    def in_waiting(self):
        return len(self.out)

    def read(self, n):
        data, self.out = self.out[:n], self.out[n:]
        return data

    def reset_input_buffer(self):
        self.out = b''

    def flush(self):
        pass

    def write(self, data):
        if self.in_ymodem:
            self.pending += data
            self.ymodem()
            return
        self.commands.append(data)
        replies = {
            b'\x01': b'\r>> ',
            b'RESET\r': b'RESET\r\nPress a key to enter CLI, ESC to skip\n',
            b' ': b'Type HELP for list of commands\n>> ',
            b'YMODEM\r': b'MMC Utility\n Select a number:\n',
            b'6': b'\n>> ',
            b'BOOT\r': f'Boot image set name: "{self.boot_name}"\n'.encode(),
        }
        if data == b'3':
            self.out += b'\nAttempting to receive .bin file using YMODEM (CTRL-C to cancel)\nC'
            self.in_ymodem = True
        elif data == b'5':
            result = b'Success' if self.write_ok else b'FAILED'
            self.out += (
                b'\nInitializing MMC ...  Success\n\nProgramming MMC ... '
                + result
                + b'\n Select a number:\n'
            )
        else:
            self.out += replies[data]

    def ymodem(self):
        if self.pending[:1] == bytes([EOT]):
            self.pending = b''
            self.in_ymodem = False
            crc = zlib.crc32(self.data[: self.size])
            self.out += (
                bytes([ACK, ACK])
                + (
                    f'\n\nReceived {self.size} bytes from hss-payload.bin '
                    f'(CRC32 is 0x{crc:08X})\n Select a number:\n'
                ).encode()
            )
            return
        length = 1024 if self.pending[:1] == bytes([STX]) else 128
        if len(self.pending) < length + 5:
            return
        packet, self.pending = self.pending[: length + 5], self.pending[length + 5 :]
        assert binascii.crc_hqx(packet[3:], 0) == 0
        if packet[1] == 0:
            name, size = packet[3:].split(b'\x00')[:2]
            assert name == b'hss-payload.bin'
            self.size = int(size)
        else:
            self.data += packet[3:-2]
        self.out += bytes([ACK])


def test_ymodem_block_layout():
    block = ymodem_block(3, b'abc', 1024)
    assert len(block) == 1024 + 5
    assert block[:3] == bytes([STX, 3, 0xFC])
    assert block[3:6] == b'abc'
    assert block[6:1027] == b'\x1a' * 1021
    assert binascii.crc_hqx(block[3:], 0) == 0


def test_ymodem_header_layout():
    block = ymodem_header('file.bin', 1234)
    assert len(block) == 128 + 5
    assert block[:3] == bytes([0x01, 0, 0xFF])
    assert block[3:].startswith(b'file.bin\x001234\x00\x00')
    assert binascii.crc_hqx(block[3:], 0) == 0


def test_payload_config_single(runner_config):
    runner = make_runner(runner_config, {'CONFIG_RV_BOOT_HART': 1})
    assert runner.payload_config('/b/zephyr.elf', 0x80000000) == (
        "set-name: 'Zephyr'\n"
        "hart-entry-points: {u54_1: '0x80000000'}\n"
        'payloads:\n'
        '  "/b/zephyr.elf": {exec-addr: \'0x80000000\', owner-hart: u54_1, '
        "priv-mode: prv_m, skip-opensbi: true, payload-name: 'zephyr'}\n"
    )


def test_payload_config_smp(runner_config):
    runner = make_runner(
        runner_config, {'CONFIG_RV_BOOT_HART': 1, 'CONFIG_SMP': True, 'CONFIG_MP_MAX_NUM_CPUS': 4}
    )
    config = runner.payload_config('/b/zephyr.elf', 0x80000000)
    assert (
        "{u54_1: '0x80000000', u54_2: '0x80000000', u54_3: '0x80000000', u54_4: '0x80000000'}"
    ) in config
    assert (
        'owner-hart: u54_1, secondary-hart: u54_2, secondary-hart: u54_3, secondary-hart: u54_4,'
    ) in config


def test_e51_rejected(runner_config):
    runner = make_runner(runner_config, {})
    with pytest.raises(ValueError, match='E51'):
        runner.hss_harts()


@pytest.mark.parametrize('receive_only', [False, True])
def test_flash_dialogue(runner_config, receive_only):
    payload = bytes(range(256)) * 9 + b'tail'
    runner = make_runner(runner_config, {}, receive_only=receive_only)
    hss = FakeHss()
    runner.flash_over_console(HssConsole(hss, logging.getLogger('test')), payload)
    assert hss.data[: len(payload)] == payload
    assert hss.size == len(payload)
    expected = [b'\x01', b'RESET\r', b' ', b'YMODEM\r', b'3']
    expected += [] if receive_only else [b'5']
    expected += [b'6', b'BOOT\r']
    assert hss.commands == expected


def test_flash_write_failure(runner_config):
    runner = make_runner(runner_config, {})
    hss = FakeHss(write_ok=False)
    with pytest.raises(HssError, match='failed to write'):
        runner.flash_over_console(HssConsole(hss, logging.getLogger('test')), b'x' * 10)
