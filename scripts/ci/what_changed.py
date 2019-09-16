#!/usr/bin/env python3
#
# Copyright (c) 2018 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# A script to set labels on pull-rquests based on files being changed

import re, os
import sh
import argparse
from github import Github

if "ZEPHYR_BASE" not in os.environ:
    exit(1)

repository_path = os.environ['ZEPHYR_BASE']
sh_special_args = {
    '_tty_out': False,
    '_cwd': repository_path
}

def git(*args):
    # Runs a git command using the 'sh' library (https://amoffat.github.io/sh/)

    # Do not create a TTY for stdout, so that Git doesn't start a pager.
    #
    # Hack: Setting _cwd to the working dir seems pointless as of writing (it
    # should be the default), but keep it around in case it's working around
    # some issue
    return sh.git(*args, _tty_out=False, _cwd=os.getcwd())


def parse_args():
    parser = argparse.ArgumentParser(
                description="Set labels for a pull request based on files that were changed")
    parser.add_argument('-c', '--commits', default=None,
            help="Commit range in the form: a..b")
    parser.add_argument('-r', '--repo', default=None,
                        help="Github repository")
    parser.add_argument('-p', '--pull-request', default=0, type=int,
                        help="Pull request number")

    return parser.parse_args()

def main():
    args = parse_args()
    if not args.commits:
        exit(1)

    commit = git("diff","--name-only", args.commits)
    files = commit.split("\n")

    matches = [
            {
                "area": "Modem",
                "regex" : ["^drivers/modem"],
                "counter": 0,
                "labels": ["area: Modem"]
                },
            {
                "area": "PWM",
                "regex" : ["^drivers/pwm"],
                "counter": 0,
                "labels": ["area: PWM"]
                },
            {
                "area": "C Library",
                "regex" : ["^lib/libc"],
                "counter": 0,
                "labels": ["area: C Library"]
                },
            {
                "area": "DTS",
                "regex" : ["^dts", ".dts"],
                "counter": 0,
                "labels": ["area: Device Tree"]
                },
            {
                "area": "Watchdog",
                "regex" : ["^drivers/watchdog"],
                "counter": 0,
                "labels": ["area: Watchdog"]
                },
            {
                "area": "Sensors",
                "regex" : ["^drivers/sensor"],
                "counter": 0,
                "labels": ["area: Sensors"]
                },
            {
                "area": "ADC",
                "regex" : ["^drivers/adc"],
                "counter": 0,
                "labels": ["area: ADC"]
                },
            {
                "area": "Counter",
                "regex" : ["^drivers/counter"],
                "counter": 0,
                "labels": ["area: Counter"]
                },
            {
                "area": "Timer",
                "regex" : ["^drivers/timer"],
                "counter": 0,
                "labels": ["area: Timer"]
                },
            {
                "area": "I2S",
                "regex" : ["^drivers/i2s"],
                "counter": 0,
                "labels": ["area: I2S"]
                },
            {
                "area": "I2C",
                "regex" : ["^drivers/i2c"],
                "counter": 0,
                "labels": ["area: I2C"]
                },
            {
                "area": "SPI",
                "regex" : ["^drivers/spi"],
                "counter": 0,
                "labels": ["area: SPI"]
                },
            {
                "area": "Boards",
                "regex" : ["^boards/"],
                "counter": 0,
                "labels": ["area: Boards"]
                },
            {
                "area": "POSIX",
                "regex" : ["^lib/posix/"],
                "counter": 0,
                "labels": ["area: POSIX"]
                },
            {
                "area": "Native Port",
                "regex" : ["^arch/posix/", "^soc/posix", ".*native_posix.*"],
                "counter": 0,
                "labels": ["area: native port"]
                },
            {
                "area": "X86",
                "regex" : ["^arch/x86/"],
                "counter": 0,
                "labels": ["area: X86"]
                },
            {
                "area": "ARM",
                "regex" : ["^arch/arm/"],
                "counter": 0,
                "labels": ["area: ARM"]
                },
            {
                "area": "Nios2",
                "regex" : ["^arch/nios2/"],
                "counter": 0,
                "labels": ["area: NIOS2"]
                },
            {
                "area": "Xtensa",
                "regex" : ["^arch/xtensa/"],
                "counter": 0,
                "labels": ["area: Xtensa"]
                },
            {
                "area": "RISCv32",
                "regex" : ["^arch/riscv32/"],
                "counter": 0,
                "labels": ["area: RISCv32"]
                },
            {
                "area": "ARC",
                "regex" : ["^arch/arc"],
                "counter": 0,
                "labels": ["area: ARC"]
                },
            {
                "area": "Netowrking",
                "regex" : ["^subsys/net", "^samples/net/", "^tests/net/"],
                "counter": 0,
                "labels": ["area: Networking"]
                },
            {
                "area": "Logging",
                "regex" : ["^subsys/logging"],
                "counter": 0,
                "labels": ["area: Logging"]
                },
            {
                "area": "Shell",
                "regex" : ["^subsys/shell"],
                "counter": 0,
                "labels": ["area: Shell"]
                },
            {
                "area": "Console",
                "regex" : ["^subsys/console"],
                "counter": 0,
                "labels": ["area: Console"]
                },
            {
                "area": "Testsuite",
                "regex" : ["^subsys/testsuite"],
                "counter": 0,
                "labels": ["area: Testing Suite"]
                },
            {
                "area": "Settings",
                "regex" : ["^subsys/settings"],
                "counter": 0,
                "labels": ["area: Settings"]
                },
            {
                "area": "File System",
                "regex" : ["^subsys/fs"],
                "counter": 0,
                "labels": ["area: File System"]
                },
            {
                "area": "Storage",
                "regex" : ["^subsys/storage"],
                "counter": 0,
                "labels": ["area: Storage"]
                },
            {
                "area": "Bluetooth",
                "regex" : ["^subsys/bluetooth", ".*bluetooth.*"],
                "counter": 0,
                "labels": ["area: Bluetooth"]
                },
            {
                "area": "Bluetooth Mesh",
                "regex" : ["^subsys/bluetooth/mesh"],
                "counter": 0,
                "labels": ["area: Bluetooth Mesh"]
                },
            {
                "area": "API",
                "regex" : ["^include/"],
                "counter": 0,
                "labels": ["area: API"]
                },
            {
                "area": "Samples",
                "regex" : ["^samples/"],
                "counter": 0,
                "labels": ["area: Samples"]
                },
            {
                "area": "Tests",
                "regex" : ["^tests/"],
                "counter": 0,
                "labels": ["area: Tests"]
                },
            {
                "area": "Kernel",
                "regex" : ["^kernel/", "^tests/kernel/"],
                "counter": 0,
                "labels": ["area: Kernel"]
                },
            {
                "area": "External",
                "regex" : ["^ext/"],
                "counter": 0,
                "labels": ["EXT"]
                },
            {
                "area": "Documentation",
                "regex" : ["^doc/", r"\.rst$", r"\.txt$"],
                "counter": 0,
                "labels": ["area: Documentation"]
                },
            {
                "area": "Build System",
                "regex" : ["^cmake/", "^CMakeLists.txt"],
                "counter": 0,
                "labels": ["area: Build System"]
                },
            {
                "area": "Kconfig",
                "regex" : ["^scripts/kconfig", "^Kconfig", "^Kconfig.zephyr"],
                "counter": 0,
                "labels": ["area: Kconfig"]
                },
            {
                "area": "Sanitycheck",
                "regex" : ["^scripts/sanitycheck", "^scripts/sanity_chk"],
                "counter": 0,
                "labels": ["area: Sanitycheck"]
                },
            {
                "area": "Manifest",
                "regex" : ["^west.yml"],
                "counter": 0,
                "labels": ["area: Modules"]
                },
            ]
    labels = []
    for f in files:
        for m in matches:
            for r in m['regex']:
                rm = re.match(r, f)
                if rm:
                    m['counter'] += 1

    for m in matches:
        # print("{}: {} files have changed".format(m['area'], m['counter']))
        if m['counter'] > 0:
            labels = labels + m['labels']

    print("Labels to apply: {}".format(", ".join(labels)))
    if len(labels)>10:
        print("too many labels, aborting...")
        exit(0)

    if args.pull_request and args.repo:
        github_token = os.environ['GH_TOKEN']
        github_conn = Github(github_token)

        repo = github_conn.get_repo(args.repo)

        post_labels = []
        gh_pr = repo.get_pull(args.pull_request)
        for l in labels:
            repo_label = repo.get_label(l)
            if repo_label:
                post_labels.append(repo_label)
                gh_pr.add_to_labels(repo_label)



if __name__ == "__main__":
    main()
