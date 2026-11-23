# SPDX-License-Identifier: Apache-2.0
# Common keywords for Zephyr QEMU robot tests.
# Provides the same interface as tests/robot/common.robot (Renode) so that
# the same .robot test suites can run on QEMU without modification.
# BUILD_DIR and GENERATOR_CMD are provided by twister.

*** Settings ***
Library    QemuSimLibrary

*** Keywords ***
Prepare Machine
    Start QEMU    ${BUILD_DIR}    ${GENERATOR_CMD}

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
