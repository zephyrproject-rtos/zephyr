#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""tests for subfolder_list.py"""

import os
import sys
import time

sys.path.insert(0, os.path.join(os.environ["ZEPHYR_BASE"], "scripts"))
import subfolder_list as iut  # Implementation Under Test

OUT_FILE = "out_file.txt"
DIR_NAME_PREFIX = "dir-"


def mkdirs(name_sfxs_range):
    """Create directories"""
    dir_names = [os.getcwd()]

    for sfx in name_sfxs_range:
        name = f"{DIR_NAME_PREFIX}{sfx}"
        os.mkdir(name)
        dir_names.append(os.path.join(os.getcwd(), name))

    return dir_names


def assert_out_file_has(dir_names):
    """Assert that out file has names of directories"""
    for dir_name in open(OUT_FILE).readlines():
        dir_name = dir_name.strip()
        assert dir_name in dir_names


def test_subfolder_list(tmpdir):
    """Test subfolder list is correct"""
    tmpdir.chdir()
    dir_names = mkdirs(range(5))
    iut_dir_names = iut.get_subfolder_list((str(tmpdir)))
    assert dir_names == iut_dir_names


def test_links(tmpdir):
    """Test directory links creation"""
    tmpdir.chdir()
    links_dir = str(tmpdir)
    dirs_dir = "dirs"
    subdirs_parent_sfx = 1

    dirs_range = range(5)
    subdirs_range = range(5, 9)

    expect_links = []
    for i in dirs_range:
        expect_links.append("%s_%s%d" % (dirs_dir, DIR_NAME_PREFIX, i))
    for i in subdirs_range:
        expect_links.append("%s_%s%d_%s%d" % (
            dirs_dir, DIR_NAME_PREFIX, subdirs_parent_sfx, DIR_NAME_PREFIX, i))

    tmpdir.mkdir(dirs_dir)
    os.chdir(dirs_dir)
    mkdirs(dirs_range)

    os.chdir(f"{DIR_NAME_PREFIX}{subdirs_parent_sfx}")
    mkdirs(subdirs_range)
    tmpdir.chdir()

    iut.get_subfolder_list(dirs_dir, links_dir)

    links = [f for f in os.listdir(links_dir) if os.path.islink(f)]
    assert sorted(expect_links) == sorted(links)


def test_gen_out_file(tmpdir):
    """Test generating the list output file"""
    tmpdir.chdir()
    dirs = []
    for sfx in range(10):
        dirs.append(f"{DIR_NAME_PREFIX}{sfx}")

    iut.gen_out_file(OUT_FILE, dirs)
    assert_out_file_has(dirs)
    st_info = os.stat(OUT_FILE)

    # should not be updated if it already exists and has the same content
    iut.gen_out_file(OUT_FILE, dirs)
    st_info2 = os.stat(OUT_FILE)
    assert st_info == st_info2

    # should be updated if exists with different content
    with open(OUT_FILE, "a") as out_fo:
        out_fo.write("A" * 79)
    st_info = os.stat(OUT_FILE)
    iut.gen_out_file(OUT_FILE, dirs)
    st_info2 = os.stat(OUT_FILE)
    assert st_info != st_info2
    assert_out_file_has(dirs)


def test_trigger_file(tmpdir):
    """Test trigger file feature"""
    trigger_file = "trigger_file"
    tmpdir.chdir()
    mkdirs(range(5))

    # should be created if it does not exist
    iut.touch(trigger_file)
    assert os.path.exists(trigger_file)

    # should be touched if it exists
    with open(trigger_file, 'w'):
        fake_time = 12345
        os.utime(trigger_file, (fake_time, fake_time))

    iut.touch(trigger_file)
    st_info = os.stat(trigger_file)

    time_now = time.time()
    time_since_touch = 5.0  # seconds, rough estimate

    assert (time_now - st_info.st_atime) <= time_since_touch
    assert (time_now - st_info.st_mtime) <= time_since_touch
