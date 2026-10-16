# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

'''Tests for soc/beken/bk7258/bkfil.py, the tool the bkflash runner runs.'''

import importlib.util
import logging
from pathlib import Path
from unittest.mock import Mock, call, patch

import pytest

BKFIL = Path(__file__).parents[3] / 'soc' / 'beken' / 'bk7258' / 'bkfil.py'
_spec = importlib.util.spec_from_file_location('bkfil', BKFIL)
bkfil = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(bkfil)

SR1 = 0x1C
SR2 = 0x02


class FakeSerial:
    '''Serial port double that returns canned bytes and records what is written.'''

    def __init__(self, data=b''):
        self.data = bytearray(data)
        self.timeout = None
        self.written = b''

    @property
    def in_waiting(self):
        return len(self.data)

    def read(self, n):
        chunk = bytes(self.data[:n])
        del self.data[:n]
        return chunk

    def write(self, frame):
        self.written += frame


def test_bkfil_recv_frames():
    # Noise, a short response to another command, then a short and a long response.
    short = bytes([0x04, 0x0E, 0x06, 0x01, 0xE0, 0xFC, 0x03, 0xAA, 0xBB])
    long = bytes([0x04, 0x0E, 0xFF, 0x01, 0xE0, 0xFC, 0xF4, 0x03, 0x00, 0x0C, 0x00, 0x1C])
    other = bytes([0x04, 0x0E, 0x05, 0x01, 0xE0, 0xFC, 0x01, 0x00])
    rom = bkfil.BootRom(FakeSerial(b'boot noise\r\n' + other + short + long), logging.getLogger())
    assert rom.recv(0x03, 1.0) == b'\xaa\xbb'
    assert rom.recv_long_ok(0x0C, 1.0) == b'\x1c'
    with pytest.raises(bkfil.FlashError, match='no response'):
        rom.recv(0x03, 0.01)


class FakeRom:
    '''Flash of three sectors, each filled with its own index.'''

    def read_sector(self, addr):
        return bytes([addr // bkfil.SECTOR]) * bkfil.SECTOR


def test_bkfil_pad_to_sectors():
    sector = bkfil.SECTOR
    # 16 bytes at 0x1ff8 straddle sectors 1 and 2; both are kept around the image.
    start, padded = bkfil.pad_to_sectors(FakeRom(), 0x1FF8, b'\xee' * 16)
    assert start == sector
    assert padded == b'\x01' * (sector - 8) + b'\xee' * 16 + b'\x02' * (sector - 8)


def mock_rom():
    '''A BK7258 with 8 MB of flash whose SR1 is SR1 and SR2 is SR2.'''
    rom = Mock()
    rom.read_reg.return_value = 0x72360001
    rom.jedec_id.return_value = 0x1765C8
    rom.read_sr.side_effect = lambda reg: SR1 if reg == 0x05 else SR2
    return rom


@patch.object(bkfil, 'program')
def test_bkfil_write_restores_sr1(program):
    rom = mock_rom()
    bkfil.write_image(rom, 0, bkfil.ROM_BAUD, b'\x00' * bkfil.SECTOR)
    program.assert_called_once()
    assert rom.write_sr.call_args_list == [call(0x00), call(SR1)]
    rom.drain.assert_not_called()


@pytest.mark.parametrize('error', [KeyboardInterrupt, bkfil.FlashError])
def test_bkfil_write_interrupted_restores_sr1(error):
    rom = mock_rom()
    # Ctrl-C or a failure mid-write: SR1 is put back, and the error is still raised.
    with patch.object(bkfil, 'program', side_effect=error), pytest.raises(error):
        bkfil.write_image(rom, 0, bkfil.ROM_BAUD, b'\x00' * bkfil.SECTOR)
    rom.drain.assert_called_once()
    assert rom.write_sr.call_args_list == [call(0x00), call(SR1)]


def test_bkfil_session_reboots():
    rom = bkfil.BootRom(Mock(), logging.getLogger())
    with patch.multiple(rom, reset_into_rom=Mock(), drain=Mock(), reboot=Mock()):
        with rom.session():
            pass
        rom.reboot.assert_called_once()
        rom.drain.assert_not_called()

        rom.reboot.reset_mock()
        with pytest.raises(KeyboardInterrupt), rom.session():
            raise KeyboardInterrupt
        # Interrupted: a command in flight is let finish before the restart.
        rom.drain.assert_called_once()
        rom.reboot.assert_called_once()


@pytest.mark.parametrize(
    'chip, status', [(0x72360001, 0), (0x72560001, 'unexpected chip id 0x72560001')]
)
def test_bkfil_probe(chip, status, capsys):
    with patch.object(bkfil, 'probe', return_value=chip):
        assert bkfil.main(['--port', '/dev/ttyUSB0', 'probe']) == status
    assert f'chip id 0x{chip:08x}' in capsys.readouterr().out


def test_bkfil_offset():
    assert bkfil.parse_offset('0x1000') == 0x1000
    assert bkfil.parse_offset('4096') == 4096
    with pytest.raises(SystemExit):
        bkfil.main(['--port', '/dev/ttyUSB0', 'write', '--offset', '-1', 'zephyr.crc.bin'])
