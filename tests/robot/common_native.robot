# SPDX-License-Identifier: Apache-2.0
# Common keywords for Zephyr native_sim robot tests.
# Provides the same interface as tests/robot/common.robot (Renode) so that
# the same .robot test suites can run on native_sim without modification.
# Requires CONFIG_UART_NATIVE_PTY_0_ON_STDINOUT=y.

*** Settings ***
Library    NativeSimLibrary

*** Keywords ***
Prepare Machine
    Start Native Binary    ${BINARY}

Wait For Prompt On Uart
    [Arguments]    ${prompt}    ${timeout}=15
    # Prompt contains literal characters (e.g. the $ in uart:~$) so use
    # a plain substring search rather than a regex.
    Wait For Literal Output    ${prompt}    ${timeout}

Write Line To Uart
    [Arguments]    ${text}
    Write To Uart    ${text}

Wait For Line On Uart
    [Arguments]    ${pattern}    ${timeout}=15
    Wait For Output    ${pattern}    ${timeout}
