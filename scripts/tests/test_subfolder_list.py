#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""pytests for subfolder_list.py"""

import os
import time
import shlex
import subprocess


OUT_FILE = "out_file.txt"
TRIGGER_FILE = "trigger_file"
DIR_NAME_PREFIX = "dir-"


def mkdirs(name_sfxs):
    """Create directories

    name_sfxs -- list of directory name suffices
    return -- list of names of created directories

    """
    dir_names = [os.getcwd()]  # cwd is in the SUT generated list

    for sfx in name_sfxs:
        name = f"{DIR_NAME_PREFIX}{sfx}"
        os.mkdir(name)
        dir_names.append(os.path.join(os.getcwd(), name))

    return dir_names


def get_sut_cmd(directory, create_links=None, out_file=OUT_FILE,
                trigger_file=None):
    """Return command to run script under test

    If an argument is set it is passed to SUT as a same name command line
    option

    """
    zephyr_base_var = "ZEPHYR_BASE"
    assert os.environ[zephyr_base_var]
    sut_path = os.path.join(os.environ[zephyr_base_var],
                            "scripts/subfolder_list.py")
    sut_exe = f"python3 {sut_path}"
    cmd = "%s --directory %s --out-file %s" % (sut_exe, directory, out_file)

    if create_links:
        cmd += " --create-links %s" % create_links

    if trigger_file:
        cmd += " --trigger-file %s" % trigger_file

    return shlex.split(cmd)


def run_sut(*args, **kwds):
    """Run script under test

    All arguments are passed to get_sut_cmd

    """
    subprocess.check_call(get_sut_cmd(*args, **kwds))


def assert_out_file_has(dir_names):
    """Assert that out file has names of directories"""

    for dir_name in open(OUT_FILE).readlines():
        dir_name = dir_name.strip()
        assert dir_name in dir_names


def test_out_file_create(tmpdir):
    """Test generating the list output file

    Also verify its that contents of the output file are correct

    Test steps:
    1. create directories
    2. run SUT - verify it creates out file with correct directories

    """
    os.chdir(tmpdir)

    # 1
    dir_names = mkdirs(range(5))

    # 2
    run_sut(tmpdir)
    assert_out_file_has(dir_names)


def test_out_file_update(tmpdir):
    """Test update the list output file

    SUT should update the output file in case sub-directories have been added
    or removed since previous invocation.

    Test steps:
    1. create out file with contents
    2. create directories somehow different from the out file
    3. run SUT - verify it correctly updates out file

    """
    os.chdir(tmpdir)
    max_dir_sfxs = 11

    # 1
    with open(OUT_FILE, 'w') as out_file_fo:
        out_file_fo.write(f"{DIR_NAME_PREFIX}55\n")  # add in the begining

        for sfx in range(max_dir_sfxs):
            if sfx in [1, 5, 10]:  # remove some dirs
                continue
            out_file_fo.write(f"{DIR_NAME_PREFIX}{sfx}\n")

        out_file_fo.write(f"{DIR_NAME_PREFIX}33\n")  # add at the end

    # 2
    dir_names = mkdirs(range(max_dir_sfxs))

    # 3
    run_sut(tmpdir)
    assert_out_file_has(dir_names)


def test_trigger_file_create(tmpdir):
    """Test trigger file creation

    SUT should create trigger file if it does not exist

    Test steps:
    1. create directories
    2. run SUT - test create empty trigger file

    """
    os.chdir(tmpdir)

    # 1
    mkdirs(range(5))

    # 2
    run_sut(tmpdir, trigger_file=TRIGGER_FILE)
    assert os.path.exists(TRIGGER_FILE)


def test_trigger_file_update(tmpdir):
    """Test trigger file is touched

    SUT should update access and modified times of trigger file

    Test steps:
    1. create directories
    2. create trigger file with fake access and modified times
    3. run SUT - verify it correctly updates access and modified times of the
       trigger file

    """
    os.chdir(tmpdir)

    # 1
    mkdirs(range(5))

    # 2
    with open(TRIGGER_FILE, 'w'):
        fake_time = 12345
        os.utime(TRIGGER_FILE, (fake_time, fake_time))

    # 3
    run_sut(tmpdir, trigger_file=TRIGGER_FILE)
    st_info = os.stat(TRIGGER_FILE)

    time_now = time.time()
    time_since_touch = 5.0  # seconds, rough estimate

    assert (time_now - st_info.st_atime) <= time_since_touch
    assert (time_now - st_info.st_mtime) <= time_since_touch


def test_links(tmpdir):
    """Test directory links creation

    SUT should create links to directories

    Test steps:
    1. create directories, one with sub-directories
    2. run SUT to create links
    3. verify links are correctly created

    """
    os.chdir(tmpdir)

    links_dir = tmpdir
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

    # 1
    os.mkdir(dirs_dir)
    os.chdir(dirs_dir)
    mkdirs(dirs_range)

    os.chdir(f"{DIR_NAME_PREFIX}{subdirs_parent_sfx}")
    mkdirs(subdirs_range)
    os.chdir(tmpdir)

    # 2
    run_sut(dirs_dir, create_links=links_dir)

    # 3
    links = [f for f in os.listdir(links_dir) if os.path.islink(f)]
    assert sorted(expect_links) == sorted(links)
