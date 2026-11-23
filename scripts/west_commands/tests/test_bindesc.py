# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

'''Tests for reading a UF2 image in the bindesc command.'''

import struct
from unittest import mock

import pytest

from bindesc import convert_from_uf2

UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30

NO_FLASH_FLAG = 0x1
BASE = 0x1000


class Died(Exception):
    '''Stands in for what WestCommand.die() does: stop.'''


@pytest.fixture
def cmd():
    command = mock.Mock()
    command.die = mock.Mock(side_effect=Died)
    return command


def block(
    addr,
    payload,
    blockno=0,
    numblocks=1,
    flags=0,
    datalen=None,
    magic0=UF2_MAGIC_START0,
    magic1=UF2_MAGIC_START1,
):
    '''One 512-byte UF2 block.'''
    head = struct.pack(
        b'<IIIIIIII',
        magic0,
        magic1,
        flags,
        addr,
        len(payload) if datalen is None else datalen,
        blockno,
        numblocks,
        0,
    )
    return head + payload + b'\x00' * (476 - len(payload)) + struct.pack(b'<I', UF2_MAGIC_END)


def test_contiguous_blocks_are_concatenated(cmd):
    buf = block(BASE, b'A' * 16, 0, 2) + block(BASE + 16, b'B' * 16, 1, 2)

    assert convert_from_uf2(cmd, buf) == b'A' * 16 + b'B' * 16


def test_a_gap_between_blocks_is_padded_with_zeros(cmd):
    '''An image with a hole in it is the normal case, not an odd one.'''
    buf = block(BASE, b'A' * 16, 0, 2) + block(BASE + 16 + 16, b'B' * 16, 1, 2)

    assert convert_from_uf2(cmd, buf) == b'A' * 16 + b'\x00' * 16 + b'B' * 16


@pytest.mark.parametrize('which', ['magic0', 'magic1'])
def test_a_block_with_the_wrong_magic_is_skipped(cmd, which):
    '''Both magic numbers have to be checked, not just the first.'''
    bad = {which: 0xDEADBEEF}
    buf = block(BASE, b'A' * 16, 0, 2, **bad) + block(BASE, b'B' * 16, 1, 2)

    assert convert_from_uf2(cmd, buf) == b'B' * 16
    cmd.inf.assert_called_once()


def test_a_block_marked_not_for_flash_is_skipped(cmd):
    buf = block(BASE, b'A' * 16, 0, 2, flags=NO_FLASH_FLAG) + block(BASE, b'B' * 16, 1, 2)

    assert convert_from_uf2(cmd, buf) == b'B' * 16


def test_trailing_bytes_that_are_not_a_whole_block_are_ignored(cmd):
    buf = block(BASE, b'A' * 16) + b'\x00' * 100

    assert convert_from_uf2(cmd, buf) == b'A' * 16
    # Ignored, not read and rejected: a short tail is not a block with bad magic.
    cmd.inf.assert_not_called()


def test_a_payload_larger_than_the_block_is_refused(cmd):
    buf = block(BASE, b'A' * 16, datalen=477)

    with pytest.raises(Died):
        convert_from_uf2(cmd, buf)
    assert 'Invalid UF2 data size' in cmd.die.call_args[0][0]


@pytest.mark.parametrize('size', [256, 476], ids=['what uf2conv writes', 'the largest legal'])
def test_a_block_that_is_full_is_read(cmd, size):
    '''The other side of the size check, which every real image is on.

    scripts/build/uf2conv.py writes 256 bytes of payload per block, and 476
    is as much as one holds. Only refusing 477 and up would leave the check
    free to move anywhere below it.
    '''
    assert convert_from_uf2(cmd, block(BASE, b'A' * size)) == b'A' * size
    cmd.die.assert_not_called()


@pytest.mark.parametrize('back', [64, 1], ids=['well before', 'one byte'])
def test_blocks_that_go_backwards_are_refused(cmd, back):
    '''Any step backwards is out of order, including one that is not a word.'''
    buf = block(BASE + 64, b'A' * 16, 0, 2) + block(BASE + 64 + 16 - back, b'B' * 16, 1, 2)

    with pytest.raises(Died):
        convert_from_uf2(cmd, buf)
    assert 'out of order' in cmd.die.call_args[0][0]


def test_a_gap_of_more_than_ten_megabytes_is_refused(cmd):
    buf = block(BASE, b'A' * 16, 0, 2) + block(BASE + 16 + 11 * 1024 * 1024, b'B' * 16, 1, 2)

    with pytest.raises(Died):
        convert_from_uf2(cmd, buf)
    assert 'More than 10M of padding' in cmd.die.call_args[0][0]


def test_a_gap_that_is_not_a_whole_number_of_words_is_refused(cmd):
    buf = block(BASE, b'A' * 16, 0, 2) + block(BASE + 16 + 2, b'B' * 16, 1, 2)

    with pytest.raises(Died):
        convert_from_uf2(cmd, buf)
    assert 'Non-word padding' in cmd.die.call_args[0][0]
