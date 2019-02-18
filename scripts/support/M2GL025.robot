*** Settings ***
Suite Setup                   Setup
Suite Teardown                Teardown
Test Setup                    Reset Emulation
# The following line loads Renode keywords and starts/stops Renode itself.
# Each testing solution should have a similar library providing required keywords.
# The test runner could set up the ${RUNNER_KEYWORDS} variable to provide it.
Resource                      ${PATH_TO_RENODE}/RobotFrameworkEngine/renode-keywords.robot

*** Variables ***
# This could be also set up by the test runner to indicate how to access the UART. For Renode it's a peripheral name,
# but could be a pseudoterminal.
${UART}                       sysbus.uart0

*** Keywords ***
Setup Simulation
    [Arguments]               ${testcase}  ${script}
    [Documentation]           This keyword is Renode-specific. It sets up a variable and runs a script.
    ...                       Other test solutions could have other implementations, meeting their specific requirements.

    Execute Command           $bin = ${elf}
    Execute Script            ${SCRIPT}

*** Test Cases ***
Should Run Shell
    [Documentation]           Runs Zephyr's 'shell' sample on a custom platform
    [Tags]                    zephyr  uart  interrupts

    # If we set these to point to zephyr.elf with Shell and m2gl025.resc script, we will have a working testcase.
    ${elf} =  Get Environment Variable  ZEPHYR_TEST_ELF
    ${script} = Get Environment Variable  RENODE_SCRIPT

    Setup Simulation          ${elf}  ${script}
    # In the next keyword we provide a prompt we will wait for in Wait For Prompt
    # This is currently required because Wait For Line expects \n
    Create Terminal Tester    ${UART}  uart:~$
    # We need to setup the tester before we start the emulation/test, not to miss any data
    Start Emulation

    Write Line To Uart        help
    Wait For Line On Uart     Please refer to shell documentation for more details.
    Wait For Prompt On Uart

    Write Line To Uart        kernel cycles
    Wait For Line On Uart     hw cycles

# More complex tests based on Zephyr can be found here:
# https://github.com/renode/renode/blob/master/tests/platforms/QuarkC1000/QuarkC1000.robot
# These tests show interaction with sensors, buttons and host network
