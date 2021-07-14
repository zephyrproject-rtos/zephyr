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
import subprocess
from shutil import copyfile
import pytest

# Dumping the trace data in text format using babeltrace.
@pytest.fixture(autouse=True)
def trace_ctf_handle(cmdopt):
    """ handle ctf data from trace backend """
    #capture trace date for 5s.
    time.sleep(5)
    data_path = cmdopt
    os.environ['data_path'] = str(data_path)
    if not os.path.exists(data_path + "/ctf"):
        os.mkdir(data_path + "/ctf")

    if os.path.exists("subsys/tracing/ctf/tsdl/metadata"):
        copyfile("subsys/tracing/ctf/tsdl/metadata", data_path + "/ctf/metadata")

    os.chdir(data_path)
    copyfile("channel0_0", "ctf/channel0_0")

    if subprocess.call("babeltrace ctf/ > ctf/tracing.txt 2>&1", shell=True) == 127:
        print("skip: please install babeltrace with：sudo apt install babeltrace.")

# check if thread/semaphore object be traced.
def test_ctf():
    """ Main Entry Point """
    txt = open("ctf/tracing.txt", "r").read()
    assert txt.count("thread_switched_out") > 0
    assert txt.count("thread_switched_in") > 0
    assert txt.count("thread_priority") > 0
    assert txt.count("thread_create") > 0
    assert txt.count("thread_suspend") > 0
    assert txt.count("thread_resume") > 0
    assert txt.count("thread_ready") > 0
    assert txt.count("thread_pending") > 0
    assert txt.count("thread_info") > 0
    assert txt.count("thread_name_set") > 0
    assert txt.count("isr_enter") > 0
    assert txt.count("isr_exit") > 0
    assert txt.count("idle") > 0
    assert txt.count("semaphore_init") > 0
    assert txt.count("semaphore_take") > 0
    assert txt.count("semaphore_give") > 0
    assert txt.count("end_call") > 0
    assert txt.count("mutex_init") > 0
    assert txt.count("mutex_lock") > 0
    assert txt.count("mutex_unlock") > 0

if __name__ == "__main__":
    pytest.main()
