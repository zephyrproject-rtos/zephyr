# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import struct
from types import SimpleNamespace
from unittest.mock import patch

import pytest

try:
    import termios
except ImportError:
    termios = None

from runners.ap_bootloader import APBootloaderBinaryRunner, bootloader_crc

TEST_BOARD_ID = 1186
TEST_FW_SIZE = 4096
TEST_PORT = '/dev/ttyACM9'
TEST_USB_ID = (0x26AC, 0x1186)
TEST_APP_PORT = '/dev/ttyACM8'
TEST_APP_USB_ID = (0x1209, 0x5740)
FLASH_BASE = 0x08000000
APP_START = 0x08020000
LEAD_WORDS = 8


def crc32_small(data, crc=0):
    '''Bitwise CRC-32 of AP_Math/crc.cpp, kept independent of the runner.'''
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1))
    return crc


class FakeBootloader:
    '''Serial port double that answers like Tools/AP_Bootloader/bl_protocol.cpp.

    The first words of the image are held back until BOOT, and erase or program
    commands are refused before sync and the device information requests. With
    app_running, the port stays silent until it receives the MAVLink reboot.
    '''

    def __init__(self, board_id=TEST_BOARD_ID, app_running=False):
        self.board_id = board_id
        self.app_running = app_running
        self.flash = bytearray(b'\x5a' * TEST_FW_SIZE)
        self.first_words = [0xFFFFFFFF] * LEAD_WORDS
        self.address = TEST_FW_SIZE
        self.synced = False
        self.info_flags = 0
        self.erased = False
        self.booted = False
        self.tx = bytearray()
        self.opened = []

    def __call__(self, port, *args, **kwargs):
        self.opened.append(port)
        return self

    def __enter__(self):
        return self

    def __exit__(self, *args):
        pass

    def close(self):
        pass

    def flush(self):
        pass

    def reset_input_buffer(self):
        self.tx.clear()

    def read(self, count):
        data = bytes(self.tx[:count])
        del self.tx[:count]
        return data

    def write(self, data):
        data = bytes(data)
        if self.app_running:
            if data.startswith(b'\xfe\x21') and len(data) == 41:
                self.app_running = False
            return
        handler = {
            0x21: self._sync,
            0x22: self._get_device,
            0x23: self._erase,
            0x27: self._program,
            0x29: self._get_crc,
            0x30: self._boot,
        }.get(data[0])
        if handler is None or data[-1] != 0x20:
            self._reply(0x13)
        else:
            handler(data[1:-1])

    def _reply(self, status, word=None):
        if word is not None:
            self.tx += struct.pack('<I', word)
        self.tx += bytes([0x12, status])

    def _ready(self):
        return self.synced and (self.info_flags & 0xB) == 0xB

    def _sync(self, args):
        self.synced = True
        self._reply(0x10)

    def _get_device(self, args):
        value = {1: 5, 2: self.board_id, 4: TEST_FW_SIZE}.get(args[0])
        if value is None:
            self._reply(0x13)
            return
        self.info_flags |= 1 << (args[0] - 1)
        self._reply(0x10, value)

    def _erase(self, args):
        if not self._ready():
            self._reply(0x13)
            return
        self.flash[:] = b'\xff' * TEST_FW_SIZE
        self.address = 0
        self.erased = True
        self._reply(0x10)

    def _program(self, args):
        count, chunk = args[0], bytearray(args[1:])
        if not self._ready() or count != len(chunk) or self.address + count > TEST_FW_SIZE:
            self._reply(0x13)
            return
        if self.address < LEAD_WORDS * 4:
            held = min(LEAD_WORDS * 4 - self.address, count)
            for i in range(0, held, 4):
                self.first_words[(self.address + i) // 4] = struct.unpack_from('<I', chunk, i)[0]
            chunk[:held] = b'\xff' * held
        self.flash[self.address : self.address + count] = chunk
        self.address += count
        self._reply(0x10)

    def _image(self):
        image = bytearray(self.flash)
        if self.first_words[0] != 0xFFFFFFFF:
            image[: LEAD_WORDS * 4] = struct.pack(f'<{LEAD_WORDS}I', *self.first_words)
        return bytes(image)

    def _get_crc(self, args):
        self._reply(0x10, crc32_small(self._image()))

    def _boot(self, args):
        self.flash[:] = self._image()
        self.booted = True
        self._reply(0x10)


@pytest.fixture
def image(tmpdir):
    data = struct.pack('<2I', 0x24002000, APP_START | 0x101) + bytes(range(256)) * 3 + b'\x01'
    path = tmpdir / 'zephyr.bin'
    path.write_binary(data)
    return data


def make_runner(runner_config, tmpdir, args):
    zephyr = tmpdir / 'zephyr'
    zephyr.mkdir()
    (zephyr / '.config').write_text(f'CONFIG_FLASH_BASE_ADDRESS=0x{FLASH_BASE:x}\n', 'utf-8')
    cfg = runner_config._replace(
        build_dir=os.fspath(tmpdir), bin_file=os.fspath(tmpdir / 'zephyr.bin')
    )
    parser = argparse.ArgumentParser(allow_abbrev=False)
    APBootloaderBinaryRunner.add_parser(parser)
    return APBootloaderBinaryRunner.create(cfg, parser.parse_args(args))


def usb_port(usb_id, device):
    return SimpleNamespace(vid=usb_id[0], pid=usb_id[1], device=device)


def run_flash(runner, fake, load_address=APP_START, comports=None):
    if comports is None:
        ports = [usb_port(TEST_USB_ID, TEST_PORT)]

        def comports():
            return ports

    with (
        patch('runners.ap_bootloader.serial.Serial', fake),
        patch('runners.ap_bootloader.serial.tools.list_ports.comports', comports),
        patch(
            'runners.core.ZephyrBinaryRunner.flash_address_from_build_conf',
            return_value=load_address,
        ),
    ):
        runner.run('flash')


def test_ap_bootloader_crc():
    '''The CRC matches the bootloader's, unlike the usual CRC-32 with inversions.'''
    assert bootloader_crc(b'123456789') == 0x2DFD2D88
    assert bootloader_crc(b'56789', bootloader_crc(b'1234')) == 0x2DFD2D88


def test_ap_bootloader_flash(runner_config, tmpdir, image):
    '''The image is erased, programmed, verified and booted.'''
    fake = FakeBootloader()
    runner = make_runner(
        runner_config, tmpdir, ['--board-id', str(TEST_BOARD_ID), '--usb-id', '26ac:1186']
    )
    run_flash(runner, fake)
    padded = image + b'\xff' * (-len(image) % 4)
    assert fake.booted
    assert fake.flash == padded + b'\xff' * (TEST_FW_SIZE - len(padded))


def test_ap_bootloader_reboots_running_app(runner_config, tmpdir, image):
    '''A running ArduPilot is rebooted into the bootloader over MAVLink.'''
    fake = FakeBootloader(app_running=True)
    runner = make_runner(
        runner_config,
        tmpdir,
        ['--board-id', str(TEST_BOARD_ID), '--port', TEST_PORT, '--timeout', '10'],
    )
    run_flash(runner, fake)
    assert fake.booted


def test_ap_bootloader_reboots_app_with_other_usb_id(runner_config, tmpdir, image):
    '''An application enumerating with another USB ID is rebooted, then the bootloader found.'''
    fake = FakeBootloader(app_running=True)
    runner = make_runner(
        runner_config,
        tmpdir,
        [
            '--board-id',
            str(TEST_BOARD_ID),
            '--usb-id',
            '26ac:1186',
            '--app-usb-id',
            '1209:5740',
            '--timeout',
            '10',
        ],
    )

    def comports():
        if fake.app_running:
            return [usb_port(TEST_APP_USB_ID, TEST_APP_PORT)]
        return [usb_port(TEST_USB_ID, TEST_PORT)]

    run_flash(runner, fake, comports=comports)
    assert fake.opened[0] == TEST_APP_PORT
    assert fake.opened[-1] == TEST_PORT
    assert fake.booted


class VanishingPort(FakeBootloader):
    '''The port of a rebooting board: still listed, but gone once opened.'''

    def __init__(self, vanishing):
        super().__init__(app_running=True)
        self.rebooted = False
        self.vanishing = vanishing

    def write(self, data):
        rebooting = self.app_running
        super().write(data)
        if rebooting and not self.app_running:
            self.rebooted = True

    def reset_input_buffer(self):
        if self.rebooted and self.vanishing > 0:
            self.vanishing -= 1
            raise termios.error(6, 'Device not configured')
        super().reset_input_buffer()


@pytest.mark.skipif(termios is None, reason='termios is POSIX only')
def test_ap_bootloader_port_vanishes_on_reboot(runner_config, tmpdir, image):
    '''The port of the rebooted board vanishes for a while before the bootloader answers.'''
    fake = VanishingPort(vanishing=5)
    runner = make_runner(
        runner_config,
        tmpdir,
        ['--board-id', str(TEST_BOARD_ID), '--port', TEST_PORT, '--timeout', '10'],
    )
    run_flash(runner, fake)
    assert fake.vanishing == 0
    assert fake.booted


def test_ap_bootloader_board_id_mismatch(runner_config, tmpdir, image):
    '''A bootloader with another board ID is left untouched.'''
    fake = FakeBootloader(board_id=9)
    runner = make_runner(
        runner_config, tmpdir, ['--board-id', str(TEST_BOARD_ID), '--port', TEST_PORT]
    )
    with pytest.raises(RuntimeError, match='board ID 9'):
        run_flash(runner, fake)
    assert not fake.erased


def test_ap_bootloader_image_too_large(runner_config, tmpdir):
    '''An image larger than the application area is refused before erasing.'''
    (tmpdir / 'zephyr.bin').write_binary(b'\x00' * (TEST_FW_SIZE + 4))
    fake = FakeBootloader()
    runner = make_runner(
        runner_config, tmpdir, ['--board-id', str(TEST_BOARD_ID), '--port', TEST_PORT]
    )
    with pytest.raises(RuntimeError, match='does not fit'):
        run_flash(runner, fake)
    assert not fake.erased


def test_ap_bootloader_rejects_image_at_flash_base(runner_config, tmpdir, image):
    '''An image linked over the bootloader is refused before connecting.'''
    fake = FakeBootloader()
    runner = make_runner(
        runner_config, tmpdir, ['--board-id', str(TEST_BOARD_ID), '--port', TEST_PORT]
    )
    with pytest.raises(RuntimeError, match='start of flash'):
        run_flash(runner, fake, load_address=FLASH_BASE)
    assert not fake.synced


def test_ap_bootloader_needs_port_or_usb_id(runner_config, tmpdir):
    '''Either a port or a USB ID is needed to find the bootloader.'''
    with pytest.raises(ValueError):
        make_runner(runner_config, tmpdir, ['--board-id', str(TEST_BOARD_ID)])
