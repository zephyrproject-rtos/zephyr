#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
"""
Script to parse CTF data and print to the screen in a custom and colorful
format.

Generate trace using samples/subsys/tracing for example:

    west build -b qemu_x86 samples/subsys/tracing  -t run \
      -- -DCONF_FILE=prj_uart_ctf.conf

    mkdir ctf
    cp build/channel0_0 ctf/
    cp subsys/tracing/ctf/tsdl/metadata ctf/
    ./scripts/tracing/parse_ctf.py -t ctf
"""

import sys
import datetime
from colorama import Fore
import argparse
try:
    import bt2
except ImportError:
    sys.exit("Missing dependency: You need to install python bindings of babletrace.")

def parse_args():
    parser = argparse.ArgumentParser(
            description=__doc__,
            formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-t", "--trace",
            required=True,
            help="tracing data (directory with metadata and trace file)")
    args = parser.parse_args()
    return args

def main():
    args = parse_args()

    msg_it = bt2.TraceCollectionMessageIterator(args.trace)
    last_event_ns_from_origin = None
    timeline = []

    def get_thread(name):
        for t in timeline:
            if t.get('name', None) == name and t.get('in', 0 ) != 0 and not t.get('out', None):
                return t
        return {}

    for msg in msg_it:

        if not isinstance(msg, bt2._EventMessageConst):
            continue

        ns_from_origin = msg.default_clock_snapshot.ns_from_origin
        event = msg.event
        # Compute the time difference since the last event message.
        diff_s = 0

        if last_event_ns_from_origin is not None:
            diff_s = (ns_from_origin - last_event_ns_from_origin) / 1e9

        dt = datetime.datetime.fromtimestamp(ns_from_origin / 1e9)

        if event.name in [
                'thread_switched_out',
                'thread_switched_in',
                'thread_pending',
                'thread_ready',
                'thread_resume',
                'thread_suspend',
                'thread_create',
                'thread_abort'
                ]:

            cpu = event.payload_field.get("cpu", None)
            thread_id = event.payload_field.get("thread_id", None)
            thread_name = event.payload_field.get("name", None)

            th = {}
            if event.name in ['thread_switched_out', 'thread_switched_in'] and cpu is not None:
                cpu_string = f"(cpu: {cpu})"
            else:
                cpu_string = ""

            if thread_name:
                print(f"{dt} (+{diff_s:.6f} s): {event.name}: {thread_name} {cpu_string}")
            elif thread_id:
                print(f"{dt} (+{diff_s:.6f} s): {event.name}: {thread_id} {cpu_string}")
            else:
                print(f"{dt} (+{diff_s:.6f} s): {event.name}")

            if event.name in ['thread_switched_out', 'thread_switched_in']:
                if thread_name:
                    th = get_thread(thread_name)
                    if not th:
                        th['name'] = thread_name
                else:
                    th = get_thread(thread_id)
                    if not th:
                        th['name'] = thread_id

                if event.name in ['thread_switched_out']:
                    th['out'] = ns_from_origin
                    tin = th.get('in', None)
                    tout = th.get('out', None)
                    if tout is not None and tin is not None:
                        diff = (tout - tin)
                        th['runtime'] = diff
                elif event.name in ['thread_switched_in']:
                    th['in'] = ns_from_origin

                    timeline.append(th)

        elif event.name in ['thread_info']:
            stack_size = event.payload_field['stack_size']
            print(f"{dt} (+{diff_s:.6f} s): {event.name} (Stack size: {stack_size})")
        elif event.name in ['start_call', 'end_call']:
            if event.payload_field['id'] == 39:
                c = Fore.GREEN
            elif event.payload_field['id'] in [37, 38]:
                c = Fore.CYAN
            else:
                c = Fore.YELLOW
            print(c + f"{dt} (+{diff_s:.6f} s): {event.name} {event.payload_field['id']}" + Fore.RESET)
        elif event.name in ['semaphore_init', 'semaphore_take', 'semaphore_give']:
            c = Fore.CYAN
            print(c + f"{dt} (+{diff_s:.6f} s): {event.name} ({event.payload_field['id']})" + Fore.RESET)
        elif event.name in ['mutex_init', 'mutex_take', 'mutex_give']:
            c = Fore.MAGENTA
            print(c + f"{dt} (+{diff_s:.6f} s): {event.name} ({event.payload_field['id']})" + Fore.RESET)

        else:
            print(f"{dt} (+{diff_s:.6f} s): {event.name}")

        last_event_ns_from_origin = ns_from_origin

if __name__=="__main__":
    main()
