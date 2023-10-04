# SPDX-License-Identifier: Apache-2.0

*** Keywords ***
Prepare Machine
    Execute Command           $bin = ${ELF}
    Execute Command           include ${RESC}
    Create Terminal Tester    ${UART}
