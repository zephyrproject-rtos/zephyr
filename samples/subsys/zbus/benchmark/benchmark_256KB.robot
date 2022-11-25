*** Settings ***
Library             Process
Library             String
Library             SerialLibrary
Library             CSVLibrary

Suite Teardown      Terminate All Processes    kill=True


*** Variables ***
${csv_file}     zbus_dyn_benchmark_256kb.csv
${board}        hifive1_revb
${serial_port}     /dev/ttyACM0

*** Tasks ***
Clear Old CSV File
    Empty Csv File    ${csv_file}

Zbus Benchmark
    FOR    ${async}    IN    "n"    "y"
        FOR    ${consumers}    IN    1    2    4    8
            FOR    ${msg_size}    IN    1    2    4    8    16    32    64    128    256
                Benchmark Report For    message_size=${msg_size}    one_to=${consumers}    asynchronous=${async}
            END
        END
    END


*** Keywords ***
Run Memory Report
    [Arguments]    ${type}
    ${result}    Run Process    west build -t ${type}_report    shell=True
    Should Be Equal As Integers    ${result.rc}    0
    ${mem}    Get Substring    ${result.stdout}    -20
    ${mem}    Strip String    ${mem}
    ${mem}    Convert To Integer    ${mem}
    RETURN    ${mem}

Measure Results
    ${total}    Set Variable    0
    Add Port    ${serial_port}    timeout=120    baudrate=115200
    Set Encoding    ascii
    FOR    ${count}    IN RANGE    3
        ${result}    Run Process    west flash    shell=True
        Should Be Equal As Integers    ${result.rc}    0
        ${val}    Read Until    expected=@    encoding=ascii
        ${val}    Read Until    encoding=ascii
        ${val}    Strip String    ${val}
        ${val}    Convert To Integer    ${val}
        ${total}    Evaluate    ${total}+${val}
    END
    ${duration}    Evaluate    ${total}/3.0
    RETURN    ${duration}
    [Teardown]    Delete All Ports

Benchmark
    [Arguments]    ${message_size}=256    ${one_to}=1    ${asynchronous}=n
    ${result}    Run Process
    ...    west build -b ${board} -p always -- -DCONFIG_BM_MESSAGE_SIZE\=${message_size} -DCONFIG_BM_ONE_TO\=${one_to} -DCONFIG_BM_ASYNC\=${asynchronous}
    ...    shell=True
    Should Be Equal As Integers    ${result.rc}    0
    ${duration}    Measure Results
    RETURN    ${duration}

Benchmark Report For
    [Arguments]    ${message_size}=256    ${one_to}=1    ${asynchronous}=n
    ${duration}    Benchmark    message_size=${message_size}    one_to=${one_to}    asynchronous=${asynchronous}
    ${ram_amount}    Run Memory Report    ram
    ${rom_amount}    Run Memory Report    rom
    IF    ${asynchronous} == "y"
        ${async_str}    Set Variable    ASYNC
    ELSE
        ${async_str}    Set Variable    SYNC
    END
    @{results}    Create List
    ...    ${async_str}
    ...    ${one_to}
    ...    ${message_size}
    ...    ${duration}
    ...    ${ram_amount}
    ...    ${rom_amount}
    Log To Console    \n${results}
    Append To Csv File    ${csv_file}    ${results}
