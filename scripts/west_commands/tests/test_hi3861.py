# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import argparse
import binascii
import hashlib
import struct
from types import SimpleNamespace
from unittest.mock import Mock

import pytest

from runners.hi3861 import Hi3861BinaryRunner


def test_parser(runner_config):
    parser = argparse.ArgumentParser(allow_abbrev=False)
    Hi3861BinaryRunner.add_parser(parser)
    args = parser.parse_args(['--port', 'COM12', '--no-at-reset'])
    runner = Hi3861BinaryRunner.create(runner_config, args)
    assert runner.port == 'COM12'
    assert runner.no_at_reset
    assert runner.capabilities().commands == {'flash'}


@pytest.mark.parametrize('size', [1, 16, 1024])
def test_signature(size):
    data = Hi3861BinaryRunner.signed_image(b'A' * size)
    header = struct.unpack('<16I', data[:64])
    assert header[0] == 0x55AA55AA
    assert header[8] == 64
    assert header[9] == (size + 15) // 16 * 16
    assert data[-48:-16] == hashlib.sha256(data[:-48]).digest()
    assert struct.unpack('<I', data[-8:-4])[0] == len(data)
    assert data[-4:] == bytes.fromhex('aa55aa55')


@pytest.mark.parametrize('data', [b'', b'A' * 65536], ids=['empty', 'oversized'])
def test_reject_invalid_image(data):
    with pytest.raises(ValueError):
        Hi3861BinaryRunner.signed_image(data)


def test_rom_frame():
    frame = Hi3861BinaryRunner.frame(struct.pack('<HII', 0x0FF0, 115200, 0x0108))
    assert frame[:4] == bytes.fromhex('efbeadde')
    assert struct.unpack_from('<H', frame, 4)[0] == len(frame)
    assert struct.unpack_from('<H', frame, 6)[0] == 0x0FF0
    assert struct.unpack('<H', frame[-2:])[0] == binascii.crc_hqx(frame[:-2], 0)


@pytest.mark.parametrize('sequence', [1, 255, 256])
def test_ymodem(sequence):
    packet = Hi3861BinaryRunner.ymodem_packet(sequence, b'abc', 1024)
    assert packet[:3] == bytes([2, sequence & 255, ~sequence & 255])
    assert packet[3:6] == b'abc'
    assert len(packet) == 1029
    assert struct.unpack('>H', packet[-2:])[0] == binascii.crc_hqx(packet[3:-2], 0)


@pytest.mark.parametrize(
    'entry,end,valid',
    [
        (0x10A040, 0x110000, True),
        (0x20000000, 0x20001000, False),
        (0x10A040, 0x11A001, False),
    ],
)
def test_elf_window(runner_config, tmp_path, monkeypatch, entry, end, valid):
    path = tmp_path / 'zephyr.elf'
    path.write_bytes(b'ELF')
    elf = SimpleNamespace(
        header=SimpleNamespace(e_machine='EM_RISCV', e_entry=entry),
        iter_segments=lambda: iter(
            [
                {
                    'p_type': 'PT_LOAD',
                    'p_vaddr': entry,
                    'p_memsz': end - entry,
                }
            ]
        ),
    )
    monkeypatch.setattr('runners.hi3861.ELFFile', lambda file: elf)
    runner = Hi3861BinaryRunner(runner_config._replace(elf_file=str(path)), 'COM12')
    if valid:
        runner.check_elf()
    else:
        with pytest.raises(ValueError):
            runner.check_elf()


def test_retry_is_reported(runner_config, caplog):
    runner = Hi3861BinaryRunner(runner_config, 'COM12')
    runner.wait_for = Mock(side_effect=[TimeoutError('missing ACK'), None])
    link = Mock()
    runner.send_packet(link, b'packet')
    assert link.write.call_count == 2
    assert 'retrying packet' in caplog.text


def test_ram_transfer_never_erases_flash(runner_config):
    runner = Hi3861BinaryRunner(runner_config, 'COM12')
    runner.wait_for = Mock()
    link = Mock()
    runner.transfer(link, runner.signed_image(b'image'))
    packets = [call.args[0] for call in link.write.call_args_list]
    assert [packet[0] for packet in packets] == [1, 2, 4, 1]
    assert all(not packet.startswith(runner.MAGIC) for packet in packets)


def test_connect_accepts_only_crc_checked_rom_ack(runner_config, monkeypatch):
    runner = Hi3861BinaryRunner(runner_config, 'COM12', no_at_reset=True)
    link = Mock()
    ack = runner.frame(runner.ROM_ACK)
    link.read.side_effect = [b'log' + ack[:5], ack[5:], b'C']
    monkeypatch.setattr('runners.hi3861.time.sleep', lambda seconds: None)
    runner.connect_rom(link)
    writes = [call.args[0] for call in link.write.call_args_list]
    assert writes[-2:] == [b'\x03', b'\0']
    assert all(packet[6] == 0xF0 for packet in writes if packet.startswith(runner.MAGIC))
