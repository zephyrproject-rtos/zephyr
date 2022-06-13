#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import os
import sys
from pathlib import Path
import json
import logging
import subprocess
import shutil
import re

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)

from twister.error import TwisterRuntimeError

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
if not ZEPHYR_BASE:
    sys.exit("$ZEPHYR_BASE environment variable undefined")

# Use this for internal comparisons; that's what canonicalization is
# for. Don't use it when invoking other components of the build system
# to avoid confusing and hard to trace inconsistencies in error messages
# and logs, generated Makefiles, etc. compared to when users invoke these
# components directly.
# Note "normalization" is different from canonicalization, see os.path.
canonical_zephyr_base = os.path.realpath(ZEPHYR_BASE)


class TwisterEnv:

    def __init__(self, options) -> None:
        self.version = None
        self.toolchain = None
        self.options = options
        if self.options.ninja:
            self.generator_cmd = "ninja"
            self.generator = "Ninja"
        else:
            self.generator_cmd = "make"
            self.generator = "Unix Makefiles"
        logger.info(f"Using {self.generator}..")

    def discover(self):
        self.check_zephyr_version()
        self.get_toolchain()

    def check_zephyr_version(self):
        try:
            subproc = subprocess.run(["git", "describe", "--abbrev=12", "--always"],
                                     stdout=subprocess.PIPE,
                                     universal_newlines=True,
                                     cwd=ZEPHYR_BASE)
            if subproc.returncode == 0:
                self.version = subproc.stdout.strip()
                logger.info(f"Zephyr version: {self.version}")
        except OSError:
            logger.info("Cannot read zephyr version.")


    @staticmethod
    def run_cmake_script(args=[]):

        logger.debug("Running cmake script %s" % (args[0]))

        cmake_args = ["-D{}".format(a.replace('"', '')) for a in args[1:]]
        cmake_args.extend(['-P', args[0]])

        logger.debug("Calling cmake with arguments: {}".format(cmake_args))
        cmake = shutil.which('cmake')
        if not cmake:
            msg = "Unable to find `cmake` in path"
            logger.error(msg)
            raise Exception(msg)
        cmd = [cmake] + cmake_args

        kwargs = dict()
        kwargs['stdout'] = subprocess.PIPE
        # CMake sends the output of message() to stderr unless it's STATUS
        kwargs['stderr'] = subprocess.STDOUT

        p = subprocess.Popen(cmd, **kwargs)
        out, _ = p.communicate()

        # It might happen that the environment adds ANSI escape codes like \x1b[0m,
        # for instance if twister is executed from inside a makefile. In such a
        # scenario it is then necessary to remove them, as otherwise the JSON decoding
        # will fail.
        ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
        out = ansi_escape.sub('', out.decode())

        if p.returncode == 0:
            msg = "Finished running  %s" % (args[0])
            logger.debug(msg)
            results = {"returncode": p.returncode, "msg": msg, "stdout": out}

        else:
            logger.error("Cmake script failure: %s" % (args[0]))
            results = {"returncode": p.returncode, "returnmsg": out}

        return results

    def get_toolchain(self):
        toolchain_script = Path(ZEPHYR_BASE) / Path('cmake/modules/verify-toolchain.cmake')
        result = self.run_cmake_script([toolchain_script, "FORMAT=json"])

        try:
            if result['returncode']:
                raise TwisterRuntimeError(f"E: {result['returnmsg']}")
        except Exception as e:
            print(str(e))
            sys.exit(2)
        self.toolchain = json.loads(result['stdout'])['ZEPHYR_TOOLCHAIN_VARIANT']
        logger.info(f"Using '{self.toolchain}' toolchain.")
