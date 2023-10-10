#
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

from __future__ import annotations
from asyncio.log import logger
import platform
import os
import sys
import subprocess
import logging
import threading

from twisterlib.cmakecache import CMakeCache

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)

_WINDOWS = platform.system() == 'Windows'

class Agent:
    '''
    Agent class serves to Harness class as a delegate separating data
    flow control functions from data interpretation functions.

    Agent object facilitates general process control when Harness needs
    to run some external programm: test data interpreter or a filter.

    Agent is also able to be an additional lightweight data source
    of test data either from the same DUT managed by Handler,
    or from another physical harness.

    Currently Twister has two kinds of Harnesses: 'simple' harnesses
    (Test, Ztest, Gtest, Console) and 'heavy' harnesses (Robot and Pytest)
    and the Agent class it meant to be helpful to refactor the latter
    for code unification and reuse.
    '''

    def __init__(self):
        self.harness: Harness | None = None
        self.proc: Popen | None = None
        self.reader_t: Thread | None = None
        self.timeout = None

    def configure(self, harness: Harness):
        self.harness = harness

    def output_reader(self):
        if self.harness.agent_feed:
            while self.proc.stdout.readable() and self.proc.poll() is None:
                line = self.proc.stdout.readline().decode(encoding='utf-8', errors="ignore").rstrip()
                if not line:
                    continue
                self.harness.handle(line)
            #
        #
        self.proc.communicate(timeout = self.timeout)

    def start(self, command = [], timeout = 60):
        self.timeout = timeout

        self.proc = subprocess.Popen(command,
                                     stdout = subprocess.PIPE,
                                     stderr = subprocess.PIPE,
                                     env = os.environ.copy())
        self.reader_t = threading.Thread(target = self.output_reader)
        self.reader_t.start()
        logger.info(f"AGENT:{self.__class__.__name__}: pid:{self.proc.pid} with:{command}")
    #

    def end(self):
        if self.reader_t is not None:
            logger.info(f"AGENT:{self.__class__.__name__}: wait for the agent process "
                        f"pid:{self.proc.pid} to close its stdout in max {self.timeout} sec.")
            self.reader_t.join(self.timeout)
            if self.reader_t.is_alive():
                logger.warning(f"AGENT:{self.__class__.__name__}: terminate the agent process "
                               f"pid:{self.proc.pid} after {self.timeout} seconds."
                               f"Set Harness:{self.harness.__class__.__name__} as FAILED")
                terminate_process(self.proc)
                self.harness.set_state('failed')
            elif self.harness.agent_check_exit_status and self.proc.returncode != 0:
                logger.warning(f"AGENT:{self.__class__.__name__}: agent process "
                               f"pid:{self.proc.pid} returns {self.proc.returncode}."
                               f"Set Harness:{self.harness.__class__.__name__} as FAILED")
                self.harness.set_state('failed')
            else:
                logger.info(f"AGENT:{self.__class__.__name__}: agent process "
                            f"pid:{self.proc.pid} returns {self.proc.returncode}.")
    #

# class Agent


class AgentImporter:
    '''
    Factory for Agent class instances
    '''

    @staticmethod
    def get_agent_class(agent_class_name):
        thismodule = sys.modules[__name__]
        if agent_class_name and isinstance(agent_class_name, str):
            agent_class = getattr(thismodule, agent_class_name)
        else:
            agent_class = getattr(thismodule, 'Agent')
        return agent_class()
#
