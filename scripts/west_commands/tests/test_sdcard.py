# Copyright (c) 2026 Analog Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from unittest.mock import patch

import pytest

from runners.sdcard import SdcardBinaryRunner


def make_runner(runner_config, tmp_path, args, file=None):
    image = tmp_path / 'zephyr.bin'
    image.write_bytes(b'zephyr')
    cfg = runner_config._replace(build_dir=str(tmp_path), bin_file=str(image), file=file)
    parser = argparse.ArgumentParser(allow_abbrev=False)
    SdcardBinaryRunner.add_parser(parser)
    return SdcardBinaryRunner.create(cfg, parser.parse_args(args))


@pytest.fixture
def mountpoint(tmp_path):
    mnt = tmp_path / 'mnt'
    mnt.mkdir()
    with patch('os.path.ismount', side_effect=lambda p: str(p) == str(mnt)):
        yield mnt


def test_sdcard_copies_bin(runner_config, tmp_path, mountpoint):
    runner = make_runner(runner_config, tmp_path, ['--mountpoint', str(mountpoint)])
    runner.run('flash')
    assert (mountpoint / 'zephyr.bin').read_bytes() == b'zephyr'


def test_sdcard_dest_name(runner_config, tmp_path, mountpoint):
    runner = make_runner(
        runner_config, tmp_path, ['--mountpoint', str(mountpoint), '--dest-name', 'app.bin']
    )
    runner.run('flash')
    assert (mountpoint / 'app.bin').read_bytes() == b'zephyr'


def test_sdcard_file(runner_config, tmp_path, mountpoint):
    other = tmp_path / 'other.elf'
    other.write_bytes(b'elf')
    runner = make_runner(
        runner_config, tmp_path, ['--mountpoint', str(mountpoint)], file=str(other)
    )
    runner.run('flash')
    assert (mountpoint / 'other.elf').read_bytes() == b'elf'


def test_sdcard_dry_run(runner_config, tmp_path, mountpoint):
    runner = make_runner(runner_config, tmp_path, ['--mountpoint', str(mountpoint), '--dry-run'])
    runner.run('flash')
    assert not (mountpoint / 'zephyr.bin').exists()


def test_sdcard_no_mountpoint(runner_config, tmp_path):
    with pytest.raises(SystemExit):
        make_runner(runner_config, tmp_path, [])


def test_sdcard_not_mounted(runner_config, tmp_path, mountpoint):
    unmounted = tmp_path / 'empty'
    unmounted.mkdir()
    runner = make_runner(runner_config, tmp_path, ['--mountpoint', str(unmounted)])
    with pytest.raises(RuntimeError, match='not a mount point'):
        runner.run('flash')
    assert not (unmounted / 'zephyr.bin').exists()
