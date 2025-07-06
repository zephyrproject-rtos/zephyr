# SPDX-License-Identifier: Apache-2.0

*** Keywords ***
Prepare Machine
    Execute Command           $elf = ${ELF}
    Execute Command           include ${RESC}
    Create Terminal Tester    ${UART}  defaultPauseEmulation=True
    Write Char Delay          0.01
