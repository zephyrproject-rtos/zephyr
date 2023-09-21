# SPDX-License-Identifier: Apache-2.0

*** Settings ***
Resource                      ${KEYWORDS}

*** Test Cases ***
Should Read Version From Shell
    # `Prepare Machine` keyword comes from $ZEPHYR_BASE/tests/robot/common.robot file, which is imported as a resource
    Prepare Machine
    Start Emulation
    Wait For Prompt On Uart   uart:~$
    Write Line To Uart        version
    Wait For Line On Uart     Zephyr version
