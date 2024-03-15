# SPDX-License-Identifier: Apache-2.0

*** Settings ***
Resource                      ${KEYWORDS}

*** Test Cases ***
Should Split Lines
    Prepare Machine
    Start Emulation
    Wait For Next Line On Uart
    Write Line To Uart          \rabc\nd\n               waitForEcho=false
    Wait For Line On Uart       getline: abc;
    Write Line To Uart          \rabc\nd\n               waitForEcho=false
    Wait For Line On Uart       ^abc$                    treatAsRegex=true
