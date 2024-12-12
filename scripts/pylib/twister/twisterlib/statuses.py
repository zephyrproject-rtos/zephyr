#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
# Copyright (c) 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
"""
Status classes to be used instead of str statuses.
"""
from __future__ import annotations

import logging
import pickle
import traceback
from abc import ABC, abstractmethod
from enum import Enum

from colorama import Fore, init
from statemachine import State, StateMachine, exceptions
from statemachine.contrib.diagram import DotGraphMachine

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)

def g():
    logger.warning("======call stack dump start==========")
    for line in traceback.format_stack():
        logger.info(line.strip())
    logger.warning("======call stack dump end============")


class TwisterStatus(str, Enum):
    def __str__(self):
        return str(self.value)

    @classmethod
    def _missing_(cls, value):
        super()._missing_(value)
        if value is None:
            return TwisterStatus.NONE

    @staticmethod
    def get_color(status: TwisterStatus) -> str:
        status2color = {
            TwisterStatus.PASS: Fore.GREEN,
            TwisterStatus.NOTRUN: Fore.CYAN,
            TwisterStatus.SKIP: Fore.YELLOW,
            TwisterStatus.FILTER: Fore.YELLOW,
            TwisterStatus.BLOCK: Fore.YELLOW,
            TwisterStatus.FAIL: Fore.RED,
            TwisterStatus.ERROR: Fore.RED,
            TwisterStatus.STARTED: Fore.MAGENTA,
            TwisterStatus.NONE: Fore.MAGENTA
        }
        return status2color.get(status, Fore.RESET)

    # All statuses below this comment can be used for TestCase
    BLOCK = 'blocked'
    STARTED = 'started'

    # All statuses below this comment can be used for TestSuite
    # All statuses below this comment can be used for TestInstance
    FILTER = 'filtered'
    NOTRUN = 'not run'

    # All statuses below this comment can be used for Harness
    NONE = None
    ERROR = 'error'
    FAIL = 'failed'
    PASS = 'passed'
    SKIP = 'skipped'


class TwisterStatusMachineInner(StateMachine):
    '''
        twister status statue machine
    '''
    none_statue = State(str(TwisterStatus.NONE), initial=True)
    pass_statue = State(str(TwisterStatus.PASS))
    notrun_statue = State(str(TwisterStatus.NOTRUN))
    skip_statue = State(str(TwisterStatus.SKIP))
    filter_statue = State(str(TwisterStatus.FILTER))
    block_statue = State(str(TwisterStatus.BLOCK))
    fail_statue = State(str(TwisterStatus.FAIL))
    error_statue = State(str(TwisterStatus.ERROR))
    started_statue = State(str(TwisterStatus.STARTED))

    # Define transitions
    # for TestCase
    cycles_case = (
        none_statue.to(started_statue,    cond="is_start")
        | none_statue.to(fail_statue,     cond="is_abnormal")
        | started_statue.to(fail_statue,  cond="is_abnormal")
        | started_statue.to(pass_statue,  cond="is_pass")
        | started_statue.to(skip_statue,  cond="is_skip")
        | started_statue.to(filter_statue,cond="is_filtered")
        | started_statue.to(block_statue, cond="is_blocked")
        | started_statue.to(error_statue, cond="is_error")
        | started_statue.to(notrun_statue,cond="is_build_only")
        | none_statue.to(pass_statue,  cond="is_load_pass")
        | none_statue.to(skip_statue,  cond="is_load_skip")
        | none_statue.to(filter_statue,cond="is_load_filtered")
        | none_statue.to(block_statue, cond="is_load_blocked")
        | none_statue.to(error_statue, cond="is_load_error")
        | none_statue.to(notrun_statue,cond="is_load_build_only")
        | fail_statue.to(none_statue,    cond="is_force")
        | pass_statue.to(none_statue,    cond="is_force")
        | skip_statue.to(none_statue,    cond="is_force")
        | block_statue.to(none_statue,   cond="is_force")
        | error_statue.to(none_statue,   cond="is_force")
        | filter_statue.to(none_statue,  cond="is_force")
        | none_statue.to.itself(internal=True,    cond="is_force")
        | started_statue.to.itself(internal=True, cond="is_start")
        | fail_statue.to.itself(internal=True,   cond="is_abnormal")
        | pass_statue.to.itself(internal=True,   cond="is_pass")
        | skip_statue.to.itself(internal=True,   cond="is_skip")
        | block_statue.to.itself(internal=True,  cond="is_blocked")
        | error_statue.to.itself(internal=True,  cond="is_error")
        | notrun_statue.to.itself(internal=True, cond="is_force")
        | filter_statue.to.itself(internal=True, cond="is_filtered")
    )

    # for TestInstance
    cycles_instance = (
        none_statue.to(fail_statue,     cond="is_abnormal")
        | none_statue.to(pass_statue,  cond="is_load_pass")
        | none_statue.to(skip_statue,  cond="is_load_skip")
        | none_statue.to(filter_statue,cond="is_load_filtered")
        | none_statue.to(error_statue, cond="is_load_error")
        | none_statue.to(notrun_statue,cond="is_load_build_only")
        | fail_statue.to(none_statue,    cond="is_force")
        | pass_statue.to(none_statue,    cond="is_force")
        | skip_statue.to(none_statue,    cond="is_force")
        | error_statue.to(none_statue,   cond="is_force")
        | filter_statue.to(none_statue,  cond="is_force")
        | none_statue.to.itself(internal=True,    cond="is_force")
        | fail_statue.to.itself(internal=True,   cond="is_abnormal")
        | pass_statue.to.itself(internal=True,   cond="is_pass")
        | skip_statue.to.itself(internal=True,   cond="is_skip")
        | error_statue.to.itself(internal=True,  cond="is_error")
        | notrun_statue.to.itself(internal=True, cond="is_force")
        | filter_statue.to.itself(internal=True, cond="is_filtered")
    )

    # for TestSuite
    cycles_suite = (
        none_statue.to(fail_statue,     cond="is_abnormal")
        | none_statue.to(pass_statue,  cond="is_load_pass")
        | none_statue.to(skip_statue,  cond="is_load_skip")
        | none_statue.to(filter_statue,cond="is_load_filtered")
        | none_statue.to(error_statue, cond="is_load_error")
        | none_statue.to(notrun_statue,cond="is_load_build_only")
        | fail_statue.to(none_statue,    cond="is_force")
        | pass_statue.to(none_statue,    cond="is_force")
        | skip_statue.to(none_statue,    cond="is_force")
        | error_statue.to(none_statue,   cond="is_force")
        | filter_statue.to(none_statue,  cond="is_force")
        | none_statue.to.itself(internal=True,    cond="is_force")
        | fail_statue.to.itself(internal=True,   cond="is_abnormal")
        | pass_statue.to.itself(internal=True,   cond="is_pass")
        | skip_statue.to.itself(internal=True,   cond="is_skip")
        | error_statue.to.itself(internal=True,  cond="is_error")
        | notrun_statue.to.itself(internal=True, cond="is_force")
        | filter_statue.to.itself(internal=True, cond="is_filtered")
    )

    # for harness
    cycles_harness = (
        none_statue.to(fail_statue,     cond="is_abnormal")
        | none_statue.to(pass_statue,  cond="is_load_pass")
        | none_statue.to(skip_statue,  cond="is_load_skip")
        | none_statue.to(filter_statue,cond="is_load_filtered")
        | none_statue.to(error_statue, cond="is_load_error")
        | fail_statue.to(none_statue,    cond="is_force")
        | pass_statue.to(none_statue,    cond="is_force")
        | skip_statue.to(none_statue,    cond="is_force")
        | error_statue.to(none_statue,   cond="is_force")
        | none_statue.to.itself(internal=True,    cond="is_force")
        | fail_statue.to.itself(internal=True,   cond="is_abnormal")
        | pass_statue.to.itself(internal=True,   cond="is_pass")
        | skip_statue.to.itself(internal=True,   cond="is_skip")
        | error_statue.to.itself(internal=True,  cond="is_error")
    )

    def is_start(self, cond: TwisterStatus):
        return cond == TwisterStatus.STARTED

    def is_abnormal(self, cond: TwisterStatus):
        return cond == TwisterStatus.FAIL

    def is_pass(self, cond: TwisterStatus):
        return cond == TwisterStatus.PASS

    def is_skip(self, cond: TwisterStatus):
        return cond == TwisterStatus.SKIP

    def is_filtered(self, cond: TwisterStatus):
        return cond == TwisterStatus.FILTER

    def is_blocked(self, cond: TwisterStatus):
        return cond == TwisterStatus.BLOCK

    def is_error(self, cond: TwisterStatus):
        return cond == TwisterStatus.ERROR

    def is_build_only(self, cond: TwisterStatus):
        return cond == TwisterStatus.NOTRUN

    def is_force(self, cond: TwisterStatus):
        return cond == TwisterStatus.NONE

    def is_load_pass(self, cond: TwisterStatus):
        return cond == TwisterStatus.PASS

    def is_load_skip(self, cond: TwisterStatus):
        return cond == TwisterStatus.SKIP

    def is_load_filtered(self, cond: TwisterStatus):
        return cond == TwisterStatus.FILTER

    def is_load_blocked(self, cond: TwisterStatus):
        return cond == TwisterStatus.BLOCK

    def is_load_error(self, cond: TwisterStatus):
        return cond == TwisterStatus.ERROR

    def is_load_build_only(self, cond: TwisterStatus):
        return cond == TwisterStatus.NOTRUN

    def after_transition(self, event: str, source: State, target: State, event_data):
        # logger.info(f"Running {event} from {source!s} to {target!s} on {event_data!s}")
        pass

    def trigger_case(self, event : TwisterStatus) -> None:
        '''
            trigger statue
        '''
        self.send("cycles_case", event)

    def force_state_case(self, event : TwisterStatus) -> None:
        '''
            trigger statue
        '''
        if event == TwisterStatus.NONE:
            self.send("cycles_case", event)
            return

        if self.current_state.id != "none_statue":
            self.send("cycles_case", TwisterStatus.NONE)
        self.send("cycles_case", TwisterStatus.STARTED)
        self.send("cycles_case", event)

    def trigger_harness(self, event : TwisterStatus) -> None:
        '''
            trigger statue
        '''
        self.send("cycles_harness", event)

    def force_state_harness(self, event : TwisterStatus) -> None:
        '''
            trigger statue
        '''
        if event == TwisterStatus.NONE:
            self.send("cycles_harness", event)
            return

        if self.current_state.id != "none_statue":
            self.send("cycles_harness", TwisterStatus.NONE)
        self.send("cycles_harness", event)

    def trigger_instance(self, event : TwisterStatus) -> None:
        '''
            trigger statue
        '''
        self.send("cycles_instance", event)

    def force_state_instance(self, event : TwisterStatus) -> None:
        '''
            trigger statue
        '''
        if event == TwisterStatus.NONE:
            self.send("cycles_instance", event)
            return

        if self.current_state.id != "none_statue":
            self.send("cycles_instance", TwisterStatus.NONE)
        self.send("cycles_instance", event)

    def trigger_suite(self, event : TwisterStatus) -> None:
        '''
            trigger statue
        '''
        self.send("cycles_suite", event)

    def force_state_suite(self, event : TwisterStatus) -> None:
        '''
            trigger statue
        '''
        if event == TwisterStatus.NONE:
            self.send("cycles_suite", event)
            return

        if self.current_state.id != "none_statue":
            self.send("cycles_suite", TwisterStatus.NONE)
        self.send("cycles_suite", event)


class TwisterStatusMachine(ABC):
    '''
        TwisterStatusMachine abstract class
    '''

    @abstractmethod
    def trigger(self, event : TwisterStatus):
        pass

    @abstractmethod
    def force_state(self, event : TwisterStatus):
        pass


class TwisterStatusMachineCase(TwisterStatusMachine):
    '''
        TwisterStatusMachine_Case TestCase Statemachine
    '''
    def __init__(self, state=TwisterStatus.NONE):
        self.current_state = state

    def trigger(self, event : TwisterStatus) -> None:
        tsm = TwisterStatusMachineInner()
        tsm.force_state_case(self.current_state)
        try:
            tsm.trigger_case(event)
        except exceptions.TransitionNotAllowed:
            # here we violate the pre-defined transition rules
            logger.warning(f"Transition from {self.current_state} to {event} is not allowed.")
            g()
        finally:
            self.force_state(event)

    def force_state(self, event : TwisterStatus) -> None:
        self.current_state = event

class TwisterStatusMachineHarness(TwisterStatusMachine):
    '''
        TwisterStatusMachineHarness Harness Statemachine
    '''
    def __init__(self, state=TwisterStatus.NONE):
        self.current_state = state

    def trigger(self, event : TwisterStatus) -> None:
        tsm = TwisterStatusMachineInner()
        tsm.force_state_harness(self.current_state)
        try:
            tsm.trigger_harness(event)
        except exceptions.TransitionNotAllowed:
            # here we violate the pre-defined transition rules
            logger.warning(f"Transition from {self.current_state} to {event} is not allowed.")
            g()
        finally:
            self.force_state(event)

    def force_state(self, event : TwisterStatus) -> None:
        self.current_state = event

class TwisterStatusMachineInstance(TwisterStatusMachine):
    '''
        TwisterStatusMachineInstance TestInstance Statemachine
    '''
    def __init__(self, state=TwisterStatus.NONE):
        self.current_state = state

    def trigger(self, event : TwisterStatus) -> None:
        tsm = TwisterStatusMachineInner()
        tsm.force_state_instance(self.current_state)
        try:
            tsm.trigger_instance(event)
        except exceptions.TransitionNotAllowed:
            # here we violate the pre-defined transition rules
            logger.warning(f"Transition from {self.current_state} to {event} is not allowed.")
            g()
        finally:
            self.force_state(event)

    def force_state(self, event : TwisterStatus) -> None:
        self.current_state = event

class TwisterStatusMachineSuite(TwisterStatusMachine):
    '''
        TwisterStatusMachineSuite TestSuite Statemachine
    '''
    def __init__(self, state=TwisterStatus.NONE):
        self.current_state = state

    def trigger(self, event : TwisterStatus) -> None:
        tsm = TwisterStatusMachineInner()
        tsm.force_state_suite(self.current_state)
        try:
            tsm.trigger_suite(event)
        except exceptions.TransitionNotAllowed:
            # here we violate the pre-defined transition rules
            logger.warning(f"Transition from {self.current_state} to {event} is not allowed.")
            g()
        finally:
            self.force_state(event)

    def force_state(self, event : TwisterStatus) -> None:
        self.current_state = event

def get_graph(tm: TwisterStatusMachineInner)-> None:
    '''
        draw graphic
    '''
    graph = DotGraphMachine(tm)
    dot = graph()
    dot.write_png("state.png")

def main():
    # Initialize the colorama library
    init()
    tm = TwisterStatusMachineCase()
    serialized = pickle.dumps(tm)
    deserialized = pickle.loads(serialized)
    assert deserialized
    tsm = TwisterStatusMachineInner()
    get_graph(tsm)
    tm.trigger(TwisterStatus.NONE)
    assert str(tm.current_state) == TwisterStatus.NONE
    tm.trigger(TwisterStatus.STARTED)
    assert str(tm.current_state) == TwisterStatus.STARTED
    tm.trigger(TwisterStatus.PASS)
    assert str(tm.current_state) == TwisterStatus.PASS
    tm.trigger(TwisterStatus.NONE)
    assert str(tm.current_state) == TwisterStatus.NONE
    tm.force_state(TwisterStatus.FILTER)
    assert str(tm.current_state) == TwisterStatus.FILTER
    tm.force_state(TwisterStatus.PASS)
    assert str(tm.current_state) == TwisterStatus.PASS
    tm.force_state(TwisterStatus.FAIL)
    assert str(tm.current_state) == TwisterStatus.FAIL
    tm.trigger(TwisterStatus.NONE)
    assert str(tm.current_state) == TwisterStatus.NONE
    tm.force_state(TwisterStatus.PASS)
    assert str(tm.current_state) == TwisterStatus.PASS
    tm.force_state(TwisterStatus.NONE)
    assert str(tm.current_state) == TwisterStatus.NONE
    tm.trigger(TwisterStatus.FILTER)
    assert str(tm.current_state) == TwisterStatus.FILTER
    print("ALL Tests PASS")

if __name__ == "__main__":
    main()
