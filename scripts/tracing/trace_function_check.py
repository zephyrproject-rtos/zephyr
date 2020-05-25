#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
"""
Script to capture tracing data with native posix backend.
"""

import argparse
import sys
import os
import time
import _thread

def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-b", "--backend", default='posix_port',
                        required=False, help=
                        "supported backend:uart, usb, posix_port.")
    args = parser.parse_args()

def stop_capturing_with_delay(process_name, delay):
    os.environ['process_name'] = str(process_name)

    time.sleep(delay)
    os.system("ps -ef|grep $process_name|awk '{print $2}'| \
        xargs sudo kill -2 2>/dev/null")

# check if tracing calls produce code when the tracing system is disabled.
def tracing_disabled_handle():
    """compile out non-tracing bin."""
    if os.path.exists("build"):
        os.system('rm -rf build')
    os.system('west build -b native_posix \
        samples/subsys/tracing -- \
        -DCONF_FILE=prj.conf')
    os.system('objdump -t build/zephyr/zephyr.elf > data/objdump.txt')

# Check the result for ctf with posix port.
# Dumping the trace data in text format using babeltrace.
def posix_port_handle():
    """ for posix backend handle. """
    os.system('cp subsys/tracing/ctf/tsdl/metadata data/')

    # compile out the posix bin.
    os.system('west build -b native_posix \
        samples/subsys/tracing -- \
        -DCONF_FILE=prj_native_posix_ctf.conf')

    # capture the tracing data from posix port.
    os.system('python3 scripts/tracing/trace_capture_posix.py \
        -o data/channel0_0')
    os.system('babeltrace data/ > data/tracing.txt')
    os.system('objdump -t build/zephyr/zephyr.elf > data/objdump.txt')

def uart_backend_handle():
    """ for uart backend handle. """
    os.system('west build -b reel_board samples/subsys/tracing/ \
        -DCONF_FILE=prj_uart_ctf.conf')

    read_val = input("Do you have flashed the related build [ y/n ]: ")

    # begin to capture tracing data if yes or input anything.
    if read_val in ('y', ''):
        print("capturing for tracing data ...")
    else:
        print("exit with read_val: " + read_val)
        sys.exit()

    _thread.start_new_thread(stop_capturing_with_delay, \
        ("trace_capture_uart", 5,))
    os.system("python3 scripts/tracing/trace_capture_uart.py \
        -d /dev/ttyACM0 -b 115200 -o data/tracing.txt")

def usb_backend_handle():
    """ for usb backend handle. """
    os.system('west build -b reel_board samples/subsys/tracing/ \
        -DCONF_FILE=prj_usb.conf')

    read_val = input("Do you have flashed the related build [ y/n ]: ")

    # begin to capture tracing data if yes or input anything.
    if read_val in ('y', ''):
        print("capturing for tracing data ...")
    else:
        sys.exit(1)

    _thread.start_new_thread(stop_capturing_with_delay, \
        ("trace_capture_usb", 5,))
    os.system("sudo python3 scripts/tracing/trace_capture_usb.py \
        -v 0x2FE9 -p 0x100 -o data/tracing.txt")

def check_results():
    """ Check the current phase of operation (thread, ISR, idle thread, etc.)
    based on the trace data. """
    txt = open(r"data/tracing.txt", "r").read()
    print("\ntest finished, results as below:")

    # check if thread object be traced.
    if txt.count("thread_switched") > 0:
        print("\033[32m\tpass\033[0m: Thread object be traced.")
    else:
        print("\033[31m\tfail\033[0m: Thread object not support to trace.")

    # check if ISR object be traced.
    if txt.count("isr_enter") > 0:
        print("\033[32m\tpass\033[0m: ISR object be traced.")
    else:
        print("\033[31m\tfail\033[0m: ISR object not support to trace.")

    # check if idle thread object be traced.
    if txt.count("idle") > 0:
        print("\033[32m\tpass\033[0m: Idle thread be traced.")
    else:
        print("\033[31m\tfail\033[0m: Idle thread not support to trace.")

    # Check if the tracing functionality interfere with normal operations
    # of the operating system.
    txt = open(r"data/objdump.txt", "r").read()
    if txt.count("thread_switched") == 0 and txt.count("isr_enter") == 0:
        print("\033[32m\tpass\033[0m: Not produce any code when the tracing system is disabled.")
    else:
        print("\033[31m\tfail\033[0m: shouldn't produce any tracing code.")

    print("\n")

def main():
    """ Main Entry Point """
    parse_args()

    output_file = args.backend
    ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")

    # change to work directory and clean build directory.
    os.chdir(ZEPHYR_BASE)
    if os.path.exists("build"):
        os.system('rm -rf build')

    # creat $ZEPHYR_BASE/data directory to save tracing&meta data.
    if os.path.exists("data"):
        os.system('rm -rf data')
    os.mkdir("data")

    # The tracing system supported multiple backends.
    if output_file == "posix_port":
        posix_port_handle()
    elif output_file == "uart":
        uart_backend_handle()
    elif output_file == "usb":
        usb_backend_handle()
    else:
        print(output_file + " not supported.")
        sys.exit(1)

    #dump elf file when the tracing system is disabled.
    tracing_disabled_handle()

    check_results()

if __name__ == "__main__":
    main()
