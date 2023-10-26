#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for jobserver.py classes' methods
"""

import functools
import mock
import os
import pytest
import sys

from contextlib import nullcontext
from errno import ENOENT
from fcntl import F_GETFL
from selectors import EVENT_READ

from twisterlib.jobserver import (
    JobHandle,
    JobClient,
    GNUMakeJobClient,
    GNUMakeJobServer
)


def test_jobhandle(capfd):
    def f(a, b, c=None, d=None):
        print(f'{a}, {b}, {c}, {d}')

    def exiter():
        with JobHandle(f, 1, 2, c='three', d=4):
            return

    exiter()

    out, err = capfd.readouterr()
    sys.stdout.write(out)
    sys.stderr.write(err)

    assert '1, 2, three, 4' in out


def test_jobclient_get_job():
    jc = JobClient()

    job = jc.get_job()

    assert isinstance(job, JobHandle)
    assert job.release_func is None


def test_jobclient_env():
    env = JobClient.env()

    assert env == {}


def test_jobclient_pass_fds():
    fds = JobClient.pass_fds()

    assert fds == []


TESTDATA_1 = [
    ({}, {'env': {'k': 'v'}, 'pass_fds': []}),
    ({'env': {}, 'pass_fds': ['fd']}, {'env': {}, 'pass_fds': ['fd']}),
]

@pytest.mark.parametrize(
    'kwargs, expected_kwargs',
    TESTDATA_1,
    ids=['no values', 'preexisting values']
)
def test_jobclient_popen(kwargs, expected_kwargs):
    jc = JobClient()

    argv = ['cmd', 'and', 'some', 'args']
    proc_mock = mock.Mock()
    popen_mock = mock.Mock(return_value=proc_mock)
    env_mock = {'k': 'v'}

    with mock.patch('subprocess.Popen', popen_mock), \
         mock.patch('os.environ', env_mock):
        proc = jc.popen(argv, **kwargs)

    popen_mock.assert_called_once_with(argv, **expected_kwargs)
    assert proc == proc_mock


TESTDATA_2 = [
    (False, 0),
    (True, 0),
    (False, 4),
    (True, 16),
]


@pytest.mark.parametrize(
    'inheritable, internal_jobs',
    TESTDATA_2,
    ids=['no inheritable, no internal', 'inheritable, no internal',
         'no inheritable, internal', 'inheritable, internal']
)
def test_gnumakejobclient_dunders(inheritable, internal_jobs):
    inherit_read_fd = mock.Mock()
    inherit_write_fd = mock.Mock()
    inheritable_pipe = (inherit_read_fd, inherit_write_fd) if inheritable else \
                      None

    internal_read_fd = mock.Mock()
    internal_write_fd = mock.Mock()

    def mock_pipe():
        return (internal_read_fd, internal_write_fd)

    close_mock = mock.Mock()
    write_mock = mock.Mock()
    set_blocking_mock = mock.Mock()
    selector_mock = mock.Mock()

    def deleter():
        jobs = mock.Mock()
        makeflags = mock.Mock()

        gmjc = GNUMakeJobClient(
            inheritable_pipe,
            jobs,
            internal_jobs=internal_jobs,
            makeflags=makeflags
        )

        assert gmjc.jobs == jobs
        if internal_jobs:
            write_mock.assert_called_once_with(internal_write_fd,
                                               b'+' * internal_jobs)
            set_blocking_mock.assert_any_call(internal_read_fd, False)
            selector_mock().register.assert_any_call(internal_read_fd,
                                                     EVENT_READ,
                                                     internal_write_fd)
        if inheritable:
            set_blocking_mock.assert_any_call(inherit_read_fd, False)
            selector_mock().register.assert_any_call(inherit_read_fd,
                                                     EVENT_READ,
                                                     inherit_write_fd)

    with mock.patch('os.close', close_mock), \
         mock.patch('os.write', write_mock), \
         mock.patch('os.set_blocking', set_blocking_mock), \
         mock.patch('os.pipe', mock_pipe), \
         mock.patch('selectors.DefaultSelector', selector_mock):
        deleter()

    if internal_jobs:
        close_mock.assert_any_call(internal_read_fd)
        close_mock.assert_any_call(internal_write_fd)
    if inheritable:
        close_mock.assert_any_call(inherit_read_fd)
        close_mock.assert_any_call(inherit_write_fd)


TESTDATA_3 = [
    (
        {'MAKEFLAGS': '-j1'},
        0,
        (False, False),
        ['Running in sequential mode (-j1)'],
        None,
        [None, 1],
        {'internal_jobs': 1, 'makeflags': '-j1'}
    ),
    (
        {'MAKEFLAGS': 'n--jobserver-auth=0,1'},
        1,
        (True, True),
        [
            '-jN forced on command line; ignoring GNU make jobserver',
            'MAKEFLAGS contained dry-run flag'
        ],
        0,
        None,
        None
    ),
    (
        {'MAKEFLAGS': '--jobserver-auth=0,1'},
        0,
        (True, True),
        ['using GNU make jobserver'],
        None,
        [[0, 1], 0],
        {'internal_jobs': 1, 'makeflags': '--jobserver-auth=0,1'}
    ),
    (
        {'MAKEFLAGS': '--jobserver-auth=123,321'},
        0,
        (False, False),
        ['No file descriptors; ignoring GNU make jobserver'],
        None,
        [None, 0],
        {'internal_jobs': 1, 'makeflags': '--jobserver-auth=123,321'}
    ),
    (
        {'MAKEFLAGS': '--jobserver-auth=0,1'},
        0,
        (False, True),
        [f'FD 0 is not readable (flags=2); ignoring GNU make jobserver'],
        None,
        [None, 0],
        {'internal_jobs': 1, 'makeflags': '--jobserver-auth=0,1'}
    ),
    (
        {'MAKEFLAGS': '--jobserver-auth=0,1'},
        0,
        (True, False),
        [f'FD 1 is not writable (flags=2); ignoring GNU make jobserver'],
        None,
        [None, 0],
        {'internal_jobs': 1, 'makeflags': '--jobserver-auth=0,1'}
    ),
    (None, 0, (False, False), [], None, None, None),
]

@pytest.mark.parametrize(
    'env, jobs, fcntl_ok_per_pipe, expected_logs,' \
    ' exit_code, expected_args, expected_kwargs',
    TESTDATA_3,
    ids=['env, no jobserver-auth', 'env, jobs, dry run', 'env, no jobs',
         'env, no jobs, oserror', 'env, no jobs, wrong read pipe',
         'env, no jobs, wrong write pipe', 'environ, no makeflags']
)
def test_gnumakejobclient_from_environ(
    caplog,
    env,
    jobs,
    fcntl_ok_per_pipe,
    expected_logs,
    exit_code,
    expected_args,
    expected_kwargs
):
    def mock_fcntl(fd, flag):
        if flag == F_GETFL:
            if fd == 0:
                if fcntl_ok_per_pipe[0]:
                    return os.O_RDONLY
                else:
                    return 2
            elif fd == 1:
                if fcntl_ok_per_pipe[1]:
                    return os.O_WRONLY
                else:
                    return 2
        raise OSError(ENOENT, 'dummy error')

    gmjc_init_mock = mock.Mock(return_value=None)

    with mock.patch('fcntl.fcntl', mock_fcntl), \
         mock.patch('os.close', mock.Mock()), \
         mock.patch('twisterlib.jobserver.GNUMakeJobClient.__init__',
                    gmjc_init_mock), \
         pytest.raises(SystemExit) if exit_code is not None else \
         nullcontext() as se:
        gmjc = GNUMakeJobClient.from_environ(env=env, jobs=jobs)

        # As patching __del__ is hard to do, we'll instead
        # cover possible exceptions and mock os calls
        if gmjc:
            gmjc._inheritable_pipe = getattr(gmjc, '_inheritable_pipe', None)
        if gmjc:
            gmjc._internal_pipe = getattr(gmjc, '_internal_pipe', None)

    assert all([log in caplog.text for log in expected_logs])

    if se:
        assert str(se.value) == str(exit_code)
        return

    if expected_args is None and expected_kwargs is None:
        assert gmjc is None
    else:
        gmjc_init_mock.assert_called_once_with(*expected_args,
                                               **expected_kwargs)


def test_gnumakejobclient_get_job():
    inherit_read_fd = mock.Mock()
    inherit_write_fd = mock.Mock()
    inheritable_pipe = (inherit_read_fd, inherit_write_fd)

    internal_read_fd = mock.Mock()
    internal_write_fd = mock.Mock()

    def mock_pipe():
        return (internal_read_fd, internal_write_fd)

    selected = [[mock.Mock(fd=0, data=1)], [mock.Mock(fd=1, data=0)]]

    def mock_select():
        nonlocal selected
        return selected

    def mock_read(fd, length):
        nonlocal selected
        if fd == 0:
            selected = selected[1:]
            raise BlockingIOError
        return b'?' * length

    close_mock = mock.Mock()
    write_mock = mock.Mock()
    set_blocking_mock = mock.Mock()
    selector_mock = mock.Mock()
    selector_mock().select = mock.Mock(side_effect=mock_select)

    def deleter():
        jobs = mock.Mock()

        gmjc = GNUMakeJobClient(
            inheritable_pipe,
            jobs
        )

        with mock.patch('os.read', side_effect=mock_read):
            job = gmjc.get_job()
            with job:
                expected_func = functools.partial(os.write, 0, b'?')

                assert job.release_func.func == expected_func.func
                assert job.release_func.args == expected_func.args
                assert job.release_func.keywords == expected_func.keywords

    with mock.patch('os.close', close_mock), \
         mock.patch('os.write', write_mock), \
         mock.patch('os.set_blocking', set_blocking_mock), \
         mock.patch('os.pipe', mock_pipe), \
         mock.patch('selectors.DefaultSelector', selector_mock):
        deleter()

    write_mock.assert_any_call(0, b'?')


TESTDATA_4 = [
    ('dummy makeflags', mock.ANY, mock.ANY, {'MAKEFLAGS': 'dummy makeflags'}),
    (None, 0, False, {'MAKEFLAGS': ''}),
    (None, 1, True, {'MAKEFLAGS': ' -j1'}),
    (None, 2, True, {'MAKEFLAGS': ' -j2 --jobserver-auth=0,1'}),
    (None, 0, True, {'MAKEFLAGS': ' --jobserver-auth=0,1'}),
]

@pytest.mark.parametrize(
    'makeflags, jobs, use_inheritable_pipe, expected_makeflags',
    TESTDATA_4,
    ids=['preexisting makeflags', 'no jobs, no pipe', 'one job',
         ' multiple jobs', 'no jobs']
)
def test_gnumakejobclient_env(
    makeflags,
    jobs,
    use_inheritable_pipe,
    expected_makeflags
):
    inheritable_pipe = (0, 1) if use_inheritable_pipe else None

    selector_mock = mock.Mock()

    env = None

    def deleter():
        gmjc = GNUMakeJobClient(None, None)
        gmjc.jobs = jobs
        gmjc._makeflags = makeflags
        gmjc._inheritable_pipe = inheritable_pipe

        nonlocal env
        env = gmjc.env()

    with mock.patch.object(GNUMakeJobClient, '__del__', mock.Mock()), \
         mock.patch('selectors.DefaultSelector', selector_mock):
        deleter()

    assert env == expected_makeflags


TESTDATA_5 = [
    (2, False, []),
    (1, True, []),
    (2, True, (0, 1)),
    (0, True, (0, 1)),
]

@pytest.mark.parametrize(
    'jobs, use_inheritable_pipe, expected_fds',
    TESTDATA_5,
    ids=['no pipe', 'one job', ' multiple jobs', 'no jobs']
)
def test_gnumakejobclient_pass_fds(jobs, use_inheritable_pipe, expected_fds):
    inheritable_pipe = (0, 1) if use_inheritable_pipe else None

    selector_mock = mock.Mock()

    fds = None

    def deleter():
        gmjc = GNUMakeJobClient(None, None)
        gmjc.jobs = jobs
        gmjc._inheritable_pipe = inheritable_pipe

        nonlocal fds
        fds = gmjc.pass_fds()

    with mock.patch('twisterlib.jobserver.GNUMakeJobClient.__del__',
                    mock.Mock()), \
         mock.patch('selectors.DefaultSelector', selector_mock):
        deleter()

    assert fds == expected_fds


TESTDATA_6 = [
    (0, 8),
    (32, 16),
    (4, 4),
]

@pytest.mark.parametrize(
    'jobs, expected_jobs',
    TESTDATA_6,
    ids=['no jobs', 'too many jobs', 'valid jobs']
)
def test_gnumakejobserver(jobs, expected_jobs):
    def mock_init(self, p, j):
        self._inheritable_pipe = p
        self._internal_pipe = None
        self.jobs = j

    pipe = (0, 1)
    cpu_count = 8
    pipe_buf = 16

    selector_mock = mock.Mock()
    write_mock = mock.Mock()
    del_mock = mock.Mock()

    def deleter():
        GNUMakeJobServer(jobs=jobs)

    with mock.patch.object(GNUMakeJobClient, '__del__', del_mock), \
         mock.patch.object(GNUMakeJobClient, '__init__', mock_init), \
         mock.patch('os.pipe', return_value=pipe), \
         mock.patch('os.write', write_mock), \
         mock.patch('multiprocessing.cpu_count', return_value=cpu_count), \
         mock.patch('select.PIPE_BUF', pipe_buf), \
         mock.patch('selectors.DefaultSelector', selector_mock):
        deleter()

    write_mock.assert_called_once_with(pipe[1], b'+' * expected_jobs)
