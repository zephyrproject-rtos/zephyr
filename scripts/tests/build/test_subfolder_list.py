#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""tests for subfolder_list.py"""

import hashlib
import os
import sys
import time

sys.path.insert(0, os.path.join(os.environ["ZEPHYR_BASE"], "scripts", "build"))
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
        expect_links.append(
            "%s_%s%d_%s%d" % (dirs_dir, DIR_NAME_PREFIX, subdirs_parent_sfx, DIR_NAME_PREFIX, i)
        )

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


def run_main(monkeypatch, roots, links=None, excluded=()):
    """Run the script with the given roots and return the listed directories"""
    argv = ["subfolder_list.py", "--out-file", OUT_FILE]
    for root in roots:
        argv += ["--directory", str(root)]
    if links is not None:
        argv += ["--create-links", str(links)]
    for directory in excluded:
        argv += ["--exclude-directory", str(directory)]
    monkeypatch.setattr(sys, "argv", argv)
    iut.main()
    with open(OUT_FILE) as out_fo:
        return [line.strip() for line in out_fo]


def test_multiple_roots(tmpdir, monkeypatch):
    """Test directories of several roots are listed"""
    tmpdir.chdir()
    roots = [tmpdir.join(name, "include") for name in ("first", "second")]
    for root in roots:
        root.join("nested").ensure(dir=True)

    expect = [str(path) for root in roots for path in (root, root.join("nested"))]
    assert run_main(monkeypatch, roots) == expect
    assert run_main(monkeypatch, roots[1:]) == expect[2:]


def test_multiple_roots_links(tmpdir, monkeypatch):
    """Test the first root keeps flat links and other roots do not collide"""
    tmpdir.chdir()
    links_dir = tmpdir.join("links")
    roots = [tmpdir.join(name, "include") for name in ("first", "second")]
    for root in roots:
        root.join("nested").ensure(dir=True)
    second_links = links_dir.join(hashlib.sha256(str(roots[1]).encode()).hexdigest())

    expect = [
        str(links_dir.join("include")),
        str(links_dir.join("include_nested")),
        str(second_links.join("include")),
        str(second_links.join("include_nested")),
    ]
    for _ in range(2):
        assert run_main(monkeypatch, roots, links_dir) == expect
    assert run_main(monkeypatch, roots[:1], links_dir) == expect[:2]
    assert run_main(monkeypatch, roots, links_dir) == expect

    targets = [str(root.join(sub)) for root in roots for sub in ("", "nested")]
    assert [os.path.realpath(link) for link in expect] == [os.path.realpath(t) for t in targets]


def test_missing_directory(tmpdir, monkeypatch):
    """Test a missing root is skipped until it is created"""
    tmpdir.chdir()
    present = tmpdir.mkdir("present")
    missing = tmpdir.join("missing")

    assert run_main(monkeypatch, [missing, present]) == [str(present)]
    missing.mkdir()
    assert run_main(monkeypatch, [missing, present]) == [str(missing), str(present)]


def test_excluded_directories(tmpdir, monkeypatch):
    """Test excluded directories and .git are not walked"""
    tmpdir.chdir()
    root = tmpdir.mkdir("app")
    for path in ("api", "build/generated", ".git/refs", "build-other"):
        root.join(path).ensure(dir=True)

    expect = [str(root.join(path)) for path in ("", "api", "build-other")]
    assert run_main(monkeypatch, [root], excluded=[root.join("build")]) == expect
