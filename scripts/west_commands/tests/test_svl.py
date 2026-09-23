# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import argparse
import io
import logging
from types import SimpleNamespace
from unittest.mock import PropertyMock, mock_open, patch

import pytest

from runners.svl import (
    SVL_CMD_BLMODE,
    SVL_CMD_DONE,
    SVL_CMD_FRAME,
    SVL_CMD_NEXT,
    SVL_CMD_RETRY,
    SVL_CMD_VERSION,
    SVL_FRAME_SIZE,
    SVL_LOAD_ADDRESS,
    SvlBinaryRunner,
    SvlError,
    SvlUploader,
    crc16,
    encode_packet,
    read_packet,
)


def test_crc16_check_value():
    assert crc16(b'123456789') == 0xFEE8


def test_crc16_empty():
    assert crc16(b'') == 0


def test_encode_packet_layout():
    packet = encode_packet(SVL_CMD_VERSION, b'\x05')
    assert packet == b'\x00\x04\x01\x05' + crc16(b'\x01\x05').to_bytes(2, 'big')


def test_encode_packet_without_payload():
    assert encode_packet(SVL_CMD_NEXT) == b'\x00\x03\x03' + crc16(b'\x03').to_bytes(2, 'big')


def test_read_packet_round_trip():
    data = bytes(range(256)) * 8
    assert read_packet(io.BytesIO(encode_packet(SVL_CMD_FRAME, data))) == (SVL_CMD_FRAME, data)


def test_read_packet_bad_crc():
    packet = bytearray(encode_packet(SVL_CMD_VERSION, b'\x05'))
    packet[-1] ^= 0xFF
    with pytest.raises(SvlError, match='CRC'):
        read_packet(io.BytesIO(bytes(packet)))


def test_read_packet_header_timeout():
    with pytest.raises(SvlError, match='header'):
        read_packet(io.BytesIO(b'\x00'))


def test_read_packet_body_timeout():
    with pytest.raises(SvlError, match='2 of 4'):
        read_packet(io.BytesIO(b'\x00\x04\x01\x05'))


def test_read_packet_invalid_length():
    with pytest.raises(SvlError, match='invalid packet length 2'):
        read_packet(io.BytesIO(b'\x00\x02\x01\x05'))


class FakePort:
    def __init__(self, replies: bytes = b''):
        self.rx = io.BytesIO(replies)
        self.tx = bytearray()
        self.dtr_writes = []
        self.flushed = False

    @property
    def dtr(self):
        return self.dtr_writes[-1] if self.dtr_writes else False

    @dtr.setter
    def dtr(self, value):
        self.dtr_writes.append(value)

    def read(self, n):
        return self.rx.read(n)

    def write(self, data):
        self.tx += data

    def reset_input_buffer(self):
        self.flushed = True


def packets(*cmds):
    return b''.join(encode_packet(cmd) for cmd in cmds)


def make_uploader(replies):
    port = FakePort(replies)
    return SvlUploader(port, logging.getLogger('test_svl'), reset_delay=0), port


def test_enter_bootloader():
    uploader, port = make_uploader(encode_packet(SVL_CMD_VERSION, b'\x05'))
    assert uploader.enter_bootloader() == 5
    assert port.dtr_writes == [False, True]
    assert port.flushed
    assert port.tx == b'U' + encode_packet(SVL_CMD_BLMODE)


def test_enter_bootloader_no_response():
    uploader, _ = make_uploader(b'')
    with pytest.raises(SvlError, match='header'):
        uploader.enter_bootloader()


def test_enter_bootloader_unexpected_command():
    uploader, _ = make_uploader(packets(SVL_CMD_NEXT))
    with pytest.raises(SvlError, match='expected VERSION'):
        uploader.enter_bootloader()


def test_enter_bootloader_bad_version_payload():
    uploader, _ = make_uploader(encode_packet(SVL_CMD_VERSION, b'\x05\x06'))
    with pytest.raises(SvlError, match='VERSION packet has 2 payload bytes'):
        uploader.enter_bootloader()


def test_upload_sends_frames_in_order():
    image = bytes(range(256)) * 17
    uploader, port = make_uploader(packets(SVL_CMD_NEXT, SVL_CMD_NEXT, SVL_CMD_NEXT, SVL_CMD_NEXT))
    uploader.upload(image)
    assert port.tx == (
        encode_packet(SVL_CMD_FRAME, image[:SVL_FRAME_SIZE])
        + encode_packet(SVL_CMD_FRAME, image[SVL_FRAME_SIZE : 2 * SVL_FRAME_SIZE])
        + encode_packet(SVL_CMD_FRAME, image[2 * SVL_FRAME_SIZE :])
        + encode_packet(SVL_CMD_DONE)
    )


def test_upload_pads_last_frame():
    uploader, port = make_uploader(packets(SVL_CMD_NEXT, SVL_CMD_NEXT))
    uploader.upload(b'\x01\x02\x03\x04\x05\x06')
    assert port.tx.startswith(encode_packet(SVL_CMD_FRAME, b'\x01\x02\x03\x04\x05\x06\xff\xff'))


def test_upload_resends_frame_on_retry():
    uploader, port = make_uploader(packets(SVL_CMD_NEXT, SVL_CMD_RETRY, SVL_CMD_NEXT))
    uploader.upload(b'\xaa' * 8)
    frame = encode_packet(SVL_CMD_FRAME, b'\xaa' * 8)
    assert port.tx == frame + frame + encode_packet(SVL_CMD_DONE)


def test_upload_gives_up_after_max_retries():
    uploader, _ = make_uploader(packets(SVL_CMD_NEXT, *[SVL_CMD_RETRY] * 5))
    with pytest.raises(SvlError, match='frame 0 rejected 5 times'):
        uploader.upload(b'\xaa' * 8)


def test_upload_rejects_retry_before_first_frame():
    uploader, _ = make_uploader(packets(SVL_CMD_RETRY))
    with pytest.raises(SvlError, match='unexpected command 0x05'):
        uploader.upload(b'\xaa' * 8)


def test_upload_timeout_mid_session():
    uploader, _ = make_uploader(packets(SVL_CMD_NEXT))
    with pytest.raises(SvlError, match='header'):
        uploader.upload(b'\xaa' * 8)


IMAGE = b'\xaa' * 2048


def make_runner(runner_config, *argv):
    parser = argparse.ArgumentParser(allow_abbrev=False)
    SvlBinaryRunner.add_parser(parser)
    args = parser.parse_args(['--port', '/dev/ttyTEST', *argv])
    return SvlBinaryRunner.create(runner_config, args)


@pytest.fixture
def flash_env():
    with (
        patch('runners.svl.SvlBinaryRunner.ensure_output'),
        patch('builtins.open', mock_open(read_data=IMAGE)),
        patch(
            'runners.svl.SvlBinaryRunner.flash_address_from_build_conf',
            return_value=SVL_LOAD_ADDRESS,
        ) as address,
        patch(
            'runners.svl.SvlBinaryRunner.build_conf',
            new_callable=PropertyMock,
            return_value={'CONFIG_FLASH_SIZE': 1024},
        ) as build_conf,
        patch('runners.svl.serial.Serial') as serial_cls,
        patch('runners.svl.SvlUploader') as uploader_cls,
    ):
        uploader = uploader_cls.return_value
        uploader.enter_bootloader.return_value = 5
        yield SimpleNamespace(
            address=address, build_conf=build_conf, serial_cls=serial_cls, uploader=uploader
        )


def test_runner_defaults(runner_config):
    runner = make_runner(runner_config)
    assert (runner.port, runner.baud) == ('/dev/ttyTEST', 115200)


def test_runner_baud(runner_config):
    assert make_runner(runner_config, '--baud', '921600').baud == 921600


def test_runner_rejects_unsupported_baud(runner_config):
    with pytest.raises(SystemExit):
        make_runner(runner_config, '--baud', '9600')


def test_runner_requires_port():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    SvlBinaryRunner.add_parser(parser)
    with pytest.raises(SystemExit):
        parser.parse_args([])


def test_flash(runner_config, flash_env):
    make_runner(runner_config).run('flash')
    flash_env.serial_cls.assert_called_once_with('/dev/ttyTEST', 115200, timeout=0.5)
    flash_env.uploader.upload.assert_called_once_with(IMAGE)


def test_flash_rejects_wrong_link_address(runner_config, flash_env):
    flash_env.address.return_value = 0xC000
    with pytest.raises(RuntimeError, match='linked at 0xc000'):
        make_runner(runner_config).run('flash')
    flash_env.serial_cls.assert_not_called()


def test_flash_rejects_oversized_image(runner_config, flash_env):
    flash_env.build_conf.return_value = {'CONFIG_FLASH_SIZE': 65}
    with pytest.raises(RuntimeError, match='room for 1024'):
        make_runner(runner_config).run('flash')
    flash_env.serial_cls.assert_not_called()


def test_flash_retries_bootloader_entry(runner_config, flash_env):
    flash_env.uploader.enter_bootloader.side_effect = [SvlError('x'), SvlError('x'), 5]
    make_runner(runner_config).run('flash')
    assert flash_env.uploader.enter_bootloader.call_count == 3
    flash_env.uploader.upload.assert_called_once_with(IMAGE)


def test_flash_gives_up_after_attempts(runner_config, flash_env):
    flash_env.uploader.enter_bootloader.side_effect = SvlError('x')
    with pytest.raises(SvlError, match='did not enter SVL'):
        make_runner(runner_config).run('flash')
    flash_env.uploader.upload.assert_not_called()
