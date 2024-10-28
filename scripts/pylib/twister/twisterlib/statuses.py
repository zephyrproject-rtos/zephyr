#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation
# Copyright (c) 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
"""
Status classes to be used instead of str statuses.
set environment variable TWISTER_STATUS_CHECK
to enable check
"""
from __future__ import annotations

import logging
import os
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

class TwisterStateMachine(StateMachine):
    '''
        TwisterStatemachine basic
    '''
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

class TwisterCaseStatusMachineInner(TwisterStateMachine):
    '''
        twister status state machine
    '''
    none_state = State(str(TwisterStatus.NONE), initial=True)
    pass_state = State(str(TwisterStatus.PASS))
    notrun_state = State(str(TwisterStatus.NOTRUN))
    skip_state = State(str(TwisterStatus.SKIP))
    filter_state = State(str(TwisterStatus.FILTER))
    block_state = State(str(TwisterStatus.BLOCK))
    fail_state = State(str(TwisterStatus.FAIL))
    error_state = State(str(TwisterStatus.ERROR))
    started_state = State(str(TwisterStatus.STARTED))

    # Define transitions
    # for TestCase
    cycles_case = (
        none_state.to(started_state,    cond="is_start")
        | none_state.to(fail_state,     cond="is_abnormal")
        | started_state.to(fail_state,  cond="is_abnormal")
        | started_state.to(pass_state,  cond="is_pass")
        | started_state.to(skip_state,  cond="is_skip")
        #| started_state.to(filter_state,cond="is_filtered")
        #| started_state.to(block_state, cond="is_blocked")
        | started_state.to(error_state, cond="is_error")
        | started_state.to(notrun_state,cond="is_build_only")
        | none_state.to(pass_state,  cond="is_load_pass")
        | none_state.to(skip_state,  cond="is_load_skip")
        | none_state.to(filter_state,cond="is_load_filtered")
        | none_state.to(block_state, cond="is_load_blocked")
        | none_state.to(error_state, cond="is_load_error")
        | none_state.to(notrun_state,cond="is_load_build_only")
        | fail_state.to(none_state,    cond="is_force")
        | pass_state.to(none_state,    cond="is_force")
        | skip_state.to(none_state,    cond="is_force")
        | block_state.to(none_state,   cond="is_force")
        | error_state.to(none_state,   cond="is_force")
        | filter_state.to(none_state,  cond="is_force")
        | none_state.to.itself(internal=True,    cond="is_force")
        | started_state.to.itself(internal=True, cond="is_start")
        | fail_state.to.itself(internal=True,   cond="is_abnormal")
        | pass_state.to.itself(internal=True,   cond="is_pass")
        | skip_state.to.itself(internal=True,   cond="is_skip")
        | block_state.to.itself(internal=True,  cond="is_blocked")
        | error_state.to.itself(internal=True,  cond="is_error")
        | notrun_state.to.itself(internal=True, cond="is_force")
        | filter_state.to.itself(internal=True, cond="is_filtered")
    )

    def trigger_case(self, event : TwisterStatus) -> None:
        '''
            trigger state
        '''
        self.send("cycles_case", event)

    def force_state_case(self, event : TwisterStatus) -> None:
        '''
            trigger state
        '''
        if event == TwisterStatus.NONE:
            self.send("cycles_case", event)
            return

        if self.current_state.id != "none_state":
            self.send("cycles_case", TwisterStatus.NONE)
        self.send("cycles_case", TwisterStatus.STARTED)
        self.send("cycles_case", event)

class TwisterHarnessStatusMachineInner(TwisterStateMachine):
    '''
        twister status state machine
    '''
    none_state = State(str(TwisterStatus.NONE), initial=True)
    pass_state = State(str(TwisterStatus.PASS))
    skip_state = State(str(TwisterStatus.SKIP))
    fail_state = State(str(TwisterStatus.FAIL))
    error_state = State(str(TwisterStatus.ERROR))

    # Define transitions
    # for harness
    cycles_harness = (
        none_state.to(fail_state,     cond="is_abnormal")
        | none_state.to(pass_state,  cond="is_load_pass")
        | none_state.to(skip_state,  cond="is_load_skip")
        | none_state.to(error_state, cond="is_load_error")
        | fail_state.to(none_state,    cond="is_force")
        | pass_state.to(none_state,    cond="is_force")
        | skip_state.to(none_state,    cond="is_force")
        | error_state.to(none_state,   cond="is_force")
        | none_state.to.itself(internal=True,    cond="is_force")
        | fail_state.to.itself(internal=True,   cond="is_abnormal")
        | pass_state.to.itself(internal=True,   cond="is_pass")
        | skip_state.to.itself(internal=True,   cond="is_skip")
        | error_state.to.itself(internal=True,  cond="is_error")
    )

    def trigger_harness(self, event : TwisterStatus) -> None:
        '''
            trigger state
        '''
        self.send("cycles_harness", event)

    def force_state_harness(self, event : TwisterStatus) -> None:
        '''
            trigger state
        '''
        if event == TwisterStatus.NONE:
            self.send("cycles_harness", event)
            return

        if self.current_state.id != "none_state":
            self.send("cycles_harness", TwisterStatus.NONE)
        self.send("cycles_harness", event)

class TwisterInstanceStatusMachineInner(TwisterStateMachine):
    '''
        twister status state machine
    '''
    none_state = State(str(TwisterStatus.NONE), initial=True)
    pass_state = State(str(TwisterStatus.PASS))
    notrun_state = State(str(TwisterStatus.NOTRUN))
    skip_state = State(str(TwisterStatus.SKIP))
    filter_state = State(str(TwisterStatus.FILTER))
    fail_state = State(str(TwisterStatus.FAIL))
    error_state = State(str(TwisterStatus.ERROR))

    # Define transitions
    # for TestInstance
    cycles_instance = (
        none_state.to(fail_state,     cond="is_abnormal")
        | none_state.to(pass_state,  cond="is_load_pass")
        | none_state.to(skip_state,  cond="is_load_skip")
        | none_state.to(filter_state,cond="is_load_filtered")
        | none_state.to(error_state, cond="is_load_error")
        | none_state.to(notrun_state,cond="is_load_build_only")
        | fail_state.to(none_state,    cond="is_force")
        | pass_state.to(none_state,    cond="is_force")
        | skip_state.to(none_state,    cond="is_force")
        | error_state.to(none_state,   cond="is_force")
        | filter_state.to(none_state,  cond="is_force")
        | none_state.to.itself(internal=True,    cond="is_force")
        | fail_state.to.itself(internal=True,   cond="is_abnormal")
        | pass_state.to.itself(internal=True,   cond="is_pass")
        | skip_state.to.itself(internal=True,   cond="is_skip")
        | error_state.to.itself(internal=True,  cond="is_error")
        | notrun_state.to.itself(internal=True, cond="is_force")
        | filter_state.to.itself(internal=True, cond="is_filtered")
    )

    def trigger_instance(self, event : TwisterStatus) -> None:
        '''
            trigger state
        '''
        self.send("cycles_instance", event)

    def force_state_instance(self, event : TwisterStatus) -> None:
        '''
            trigger state
        '''
        if event == TwisterStatus.NONE:
            self.send("cycles_instance", event)
            return

        if self.current_state.id != "none_state":
            self.send("cycles_instance", TwisterStatus.NONE)
        self.send("cycles_instance", event)

class TwisterSuiteStatusMachineInner(TwisterStateMachine):
    '''
        twister status state machine
    '''
    none_state = State(str(TwisterStatus.NONE), initial=True)
    pass_state = State(str(TwisterStatus.PASS))
    notrun_state = State(str(TwisterStatus.NOTRUN))
    skip_state = State(str(TwisterStatus.SKIP))
    filter_state = State(str(TwisterStatus.FILTER))
    fail_state = State(str(TwisterStatus.FAIL))
    error_state = State(str(TwisterStatus.ERROR))

    # Define transitions
    # for TestSuite
    cycles_suite = (
        none_state.to(fail_state,     cond="is_abnormal")
        | none_state.to(pass_state,  cond="is_load_pass")
        | none_state.to(skip_state,  cond="is_load_skip")
        | none_state.to(filter_state,cond="is_load_filtered")
        | none_state.to(error_state, cond="is_load_error")
        | none_state.to(notrun_state,cond="is_load_build_only")
        | fail_state.to(none_state,    cond="is_force")
        | pass_state.to(none_state,    cond="is_force")
        | skip_state.to(none_state,    cond="is_force")
        | error_state.to(none_state,   cond="is_force")
        | filter_state.to(none_state,  cond="is_force")
        | none_state.to.itself(internal=True,    cond="is_force")
        | fail_state.to.itself(internal=True,   cond="is_abnormal")
        | pass_state.to.itself(internal=True,   cond="is_pass")
        | skip_state.to.itself(internal=True,   cond="is_skip")
        | error_state.to.itself(internal=True,  cond="is_error")
        | notrun_state.to.itself(internal=True, cond="is_force")
        | filter_state.to.itself(internal=True, cond="is_filtered")
    )

    def trigger_suite(self, event : TwisterStatus) -> None:
        '''
            trigger state
        '''
        self.send("cycles_suite", event)

    def force_state_suite(self, event : TwisterStatus) -> None:
        '''
            trigger state
        '''
        if event == TwisterStatus.NONE:
            self.send("cycles_suite", event)
            return

        if self.current_state.id != "none_state":
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
        check_enable = os.environ.get("TWISTER_STATUS_CHECK")
        if check_enable:
            tsm = TwisterCaseStatusMachineInner()
            tsm.force_state_case(self.current_state)
            try:
                tsm.trigger_case(event)
            except exceptions.TransitionNotAllowed:
                # here we violate the pre-defined transition rules
                logger.warning(f"Transition from {self.current_state} to {event} is not allowed.")
                g()
            finally:
                self.force_state(event)
        else:
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
        check_enable = os.environ.get("TWISTER_STATUS_CHECK")
        if check_enable:
            tsm = TwisterHarnessStatusMachineInner()
            tsm.force_state_harness(self.current_state)
            try:
                tsm.trigger_harness(event)
            except exceptions.TransitionNotAllowed:
                # here we violate the pre-defined transition rules
                logger.warning(f"Transition from {self.current_state} to {event} is not allowed.")
                g()
            finally:
                self.force_state(event)
        else:
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
        check_enable = os.environ.get("TWISTER_STATUS_CHECK")
        if check_enable:
            tsm = TwisterInstanceStatusMachineInner()
            tsm.force_state_instance(self.current_state)
            try:
                tsm.trigger_instance(event)
            except exceptions.TransitionNotAllowed:
                # here we violate the pre-defined transition rules
                logger.warning(f"Transition from {self.current_state} to {event} is not allowed.")
                g()
            finally:
                self.force_state(event)
        else:
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
        check_enable = os.environ.get("TWISTER_STATUS_CHECK")
        if check_enable:
            tsm = TwisterSuiteStatusMachineInner()
            tsm.force_state_suite(self.current_state)
            try:
                tsm.trigger_suite(event)
            except exceptions.TransitionNotAllowed:
                # here we violate the pre-defined transition rules
                logger.warning(f"Transition from {self.current_state} to {event} is not allowed.")
                g()
            finally:
                self.force_state(event)
        else:
            self.force_state(event)

    def force_state(self, event : TwisterStatus) -> None:
        self.current_state = event

def get_graph(tm: TwisterStateMachine, file_name: str)-> None:
    '''
        draw graphic
    '''
    graph = DotGraphMachine(tm)
    dot = graph()
    dot.write_png(file_name)

def main():
    # Initialize the colorama library
    init()
    os.environ["TWISTER_STATUS_CHECK"] = "y"
    tm = TwisterStatusMachineCase()
    serialized = pickle.dumps(tm)
    deserialized = pickle.loads(serialized)
    assert deserialized
    tsm = TwisterCaseStatusMachineInner()
    get_graph(tsm, "case.png")
    tsm = TwisterHarnessStatusMachineInner()
    get_graph(tsm, "harness.png")
    tsm = TwisterInstanceStatusMachineInner()
    get_graph(tsm, "instance.png")
    tsm = TwisterSuiteStatusMachineInner()
    get_graph(tsm, "suite.png")
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
