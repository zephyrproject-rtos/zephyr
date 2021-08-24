#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
"""
Script to handle trace data with trace backend.
"""
import os
import time
from shutil import copyfile
import pytest

# .
@pytest.fixture(autouse=True)
def trace_data_handle(cmdopt):
    """ handle tracing data from trace backend """
    # capture trace data for 5s.
    time.sleep(5)
    data_path = cmdopt
    os.environ['data_path'] = str(data_path)
    if not os.path.exists(data_path + "/tracing_test"):
        os.mkdir(data_path + "/tracing_test")

    os.chdir(data_path)
    copyfile("channel0_0", "tracing_test/channel0_0")

# check if semaphore and mutex object be traced.
def test_tracing():
    """ Main Entry Point """
    txt = open("channel0_0", "r").read()
    assert txt.count("sys_trace_k_mutex_init") > 0
    assert txt.count("sys_trace_k_mutex_lock_enter") > 0
    assert txt.count("sys_trace_k_mutex_lock_exit") > 0
    assert txt.count("sys_trace_k_mutex_unlock_enter") > 0
    assert txt.count("sys_trace_k_mutex_unlock_exit") > 0
    assert txt.count("sys_trace_k_sem_init") > 0
    assert txt.count("sys_trace_k_sem_take_enter") > 0
    assert txt.count("sys_trace_k_sem_take_blocking") > 0
    assert txt.count("sys_trace_k_sem_give_enter") > 0

if __name__ == "__main__":
    pytest.main()
