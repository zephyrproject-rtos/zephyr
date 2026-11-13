# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

import pytest
from zspdx.walker import Walker, WalkerConfig


@pytest.fixture
def walker():
    return Walker(WalkerConfig())


@pytest.mark.parametrize(
    "url",
    [
        "https://github.com/zephyrproject-rtos/zephyr",
        "https://github.com/zephyrproject-rtos/zephyr.git",
        "https://github.com/zephyrproject-rtos/zephyr.git/",
        "http://github.com/zephyrproject-rtos/zephyr/",
        "https://user@github.com/zephyrproject-rtos/zephyr.git",
        "https://user:password@github.com/zephyrproject-rtos/zephyr.git",
        "https://user:p%40ss%3Aword@github.com:8443/zephyrproject-rtos/zephyr.git",
        "git@github.com:zephyrproject-rtos/zephyr.git",
        "git@github.com:zephyrproject-rtos/zephyr.git/",
        "ssh://git@github.com:2222/zephyrproject-rtos/zephyr.git",
    ],
)
@pytest.mark.parametrize(
    ("version", "expected_purl"),
    [
        (None, "pkg:github/zephyrproject-rtos/zephyr"),
        ("v4.4.0", "pkg:github/zephyrproject-rtos/zephyr@v4.4.0"),
    ],
)
def test_zephyr_repository_identity(walker, url, version, expected_purl):
    assert walker._build_purl(url, version) == expected_purl
    assert walker._supplier_from_url(url) == "The Zephyr Project"


@pytest.mark.parametrize(
    "url",
    [
        "https://gitlab-ci-token:token@gitlab.com/group/subgroup/my.project.git",
        "git@gitlab.com:group/subgroup/my.project.git",
    ],
)
def test_gitlab_repository_identity(walker, url):
    assert walker._build_purl(url, "abc123") == "pkg:gitlab/group/subgroup/my.project@abc123"
    assert walker._supplier_from_url(url) == "group/subgroup"


@pytest.mark.parametrize(
    "url",
    [
        None,
        "",
        "/local/org/repo",
        "org/repo",
        "file:///local/org/repo",
        "ftp://github.com/org/repo.git",
        "https:///org/repo.git",
        "https://github.com",
        "https://github.com/org",
        "https://github.com/org/",
        "https://github.com/org/.git",
        "git@github.com/org/repo.git",
        "https://[github.com/org/repo.git",
        "https://github.com:invalid/org/repo.git",
        "https://github.com:65536/org/repo.git",
        "https://github.com/org/repo.git?query=value",
        "https://github.com/org/repo.git#fragment",
        "ssh://git@code.example/team/lib@debug.git",
        "https://github.com/team@debug/repo.git",
        "git@github.com:team/lib@debug.git",
        "https://github.com/zephyrproject-rtos/zephyr%2Egit",
        "https://github.com/te%61m/repo.git",
        "git@github.com:team/repo%2Egit",
    ],
)
def test_unsupported_repository_identity(walker, url):
    assert walker._build_purl(url) is None
    assert walker._supplier_from_url(url) == ""
