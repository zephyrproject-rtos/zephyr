.. _wasm-micro-runtime-wasm_app_manager:

WebAssembly Micro Runtime Application Manager
##################################################

Overview
********
This sample project illustrates how to start WebAssembly application manager in device side which runs on Zephyr OS, and how to install/uninstall WebAssembly apps from host side. It builds out host tools running on the host side, and application manager running on the device side. The device application consists of wasm runtime, application library, application manager and timer support. The device runs on Zephyr OS and interacts with host tools.

It demonstrates an end to end scenario, the wasm applications life cycle management and communication programming models.

Directory structure
*******************

.. code-block:: console

    wasm_app_manager
    ├── build.sh
    ├── CMakeLists.txt
    ├── prj.conf
    ├── README.md
    ├── src
    │   └── main.c
    └── wasm-apps
        ├── event_publisher.c
        ├── event_subscriber.c
        ├── request_handler.c
        ├── request_sender.c
        └── timer.c


- build.sh
  The script to build all binaries.
- CMakeLists.txt
  CMake file used to build the wasm_app_manager zephyr project.
- prj.conf
  Zephy project configuration file.
- README.md
  The file you are reading currently.
- src/main.c
  This file is the implementation by platform integrator. It implements the interfaces that enable the application manager communicating with the host side. See "${WAMR_DIR}/core/app-mgr/app-mgr-shared/app_manager_export.h" for the definition of the host interface.

.. code-block:: console

    /* Interfaces of host communication */
    typedef struct host_interface {
        host_init_func init;
        host_send_fun send;
        host_destroy_fun destroy;
    } host_interface;


    host_interface interface = {
        .init = host_init,
        .send = host_send,
        .destroy = host_destroy
    };

This interface is passed to application manager by calling

.. code-block:: console

    app_manager_startup(&interface);


The "host_init_func" is called when the application manager starts up. And "host_send_fun" is called by the application manager to send data to the host.

Note: Currently application manager keeps running and never exit, "host_destroy_fun" has no chance to get executed. So you can leave this API implementation empty.

- wasm-apps
  Source files of sample wasm applications.

Install required SDK and libraries
**********************************
Install WASI SDK: Download the [wasi-sdk](https://github.com/CraneStation/wasi-sdk/releases) and extract the archive to default path "/opt/wasi-sdk"

Prepare STM32 board
*******************
Since you may install multiple wasm applications, it is recommended that RAM SIZE not less than 320KB. By default we use nucleo_f767zi.

Build all binaries
******************
Execute the build.sh script then all binaries including wasm application files would be generated in 'out' directory.

.. code-block:: console

    ./build.sh
    By default the board nucleo_f767zi is used. If you want to build Zephyr for other boards, please run:
    ./build.sh <your_board_name>

Out directory structure
***********************

.. code-block:: console

    out/
    ├── host_tool
    ├── zephyr-build/zephyr
    │   └── zephyr.elf
    └── wasm-apps
        ├── event_publisher.wasm
        ├── event_subscriber.wasm
        ├── request_handler.wasm
        ├── request_sender.wasm
        └── timer.wasm


- host_tool:
  A small testing tool to interact with WAMR. See the usage of this tool by executing "./host_tool -h".

.. code-block:: console

    ./host_tool -h

- zephry-build/zephyr/zephyr.elf:
  The zephyr image file containing WAMR to be flashed to board. A simple testing tool running on the host side that interact with WAMR. It is used to install, uninstall and query WASM applications in WAMR, and send request or subscribe event, etc.

- wasm-apps:
  Sample wasm applications that demonstrate all APIs of the WAMR programming model. The source codes are in the wasm-apps directory under the root of this sample.

.. code-block:: console

    event_publisher.wasm
    This application shows the sub/pub programming model. The pub application publishes the event "alert/overheat" by calling api_publish_event() API. The subscriber could be host_tool or other wasm application.

    event_subscriber.wasm
    This application shows the sub/pub programming model. The sub application subscribes the "alert/overheat" event by calling api_subscribe_event() API so that it is able to receive the event once generated and published by the pub application. To make the process clear to interpret, the sub application dumps the event when receiving it.

    request_handler.wasm
    This application shows the request/response programming model. The request handler application registers 2 resources(/url1 and /url2) by calling api_register_resource_handler() API. The request sender could be host_tool or other wasm application.

    request_sender.wasm
    This application shows the request/response programming model. The sender application sends 2 requests, one is "/app/request_handler/url1" and the other is "url1". The former is an accurate request which explicitly specifies the name of request handler application in the middle of the URL and the later is a general request.

    timer.wasm
    This application shows the timer programming model. It creates a periodic timer that prints the current expiry number in every second.

Run the scenario
****************
- Enter the zephyr-build directory

.. code-block:: console

    $ cd ./out/zephyr-build/


- Startup the board and falsh zephyr image and you would see "App Manager started." on board's terminal.

.. code-block:: console

    $ ninja flash


- Query all installed applications

.. code-block:: console

    $ cd ..
    $ sudo ./host_tool -D /dev/ttyUSB0 -q

    response status 69
    {
        "num":    0
    }


The "69" stands for response status to this query request which means query success and a payload is attached with the response. See "{WAMR_ROOT}/core/iwasm/lib/app-libs/base/wasm_app.h" for the definitions of response codes. The payload is printed with JSON format where the "num" stands for application installations number and value "0" means currently no application is installed yet.

- Install the request handler wasm application

.. code-block:: console

    $ sudo ./host_tool -D /dev/ttyUSB0 -i request_handler -f ./wasm-apps/request_handler.wasm

    response status 65

The "65" stands for response status to this installation request which means success.

Output of board

.. code-block:: console

    Install WASM app success!
    sent 16 bytes to host
    WASM app 'request_handler' started


Now the request handler application is running and waiting for host or other wasm application to send a request.

- Query again

.. code-block:: console

    $ sudo ./host_tool -D /dev/ttyUSB0 -q

    response status 69
    {
        "num":    1,
        "applet1":    "request_handler",
        "heap1":    49152
    }


In the payload, we can see "num" is 1 which means 1 application is installed. "applet1" stands for the name of the 1st application. "heap1" stands for the heap size of the 1st application.

- Send request from host to specific wasm application

.. code-block:: console

    $ sudo ./host_tool -D /dev/ttyUSB0 -r /app/request_handler/url1 -A GET

    response status 69
    {
        "key1":    "value1",
        "key2":    "value2"
    }


We can see a response with status "69" and a payload is received.

Output of board

.. code-block:: console

    connection established!
    Send request to applet: request_handler
    Send request to app request_handler success.
    App request_handler got request, url url1, action 1
    [resp] ### user resource 1 handler called
    sent 150 bytes to host
    Wasm app process request success.


- Send a general request from host (not specify target application name)

.. code-block:: console

    $ sudo ./host_tool -D /dev/ttyUSB0 -r /url1 -A GET

    response status 69
    {
        "key1":    "value1",
        "key2":    "value2"
    }


Output of board

.. code-block:: console

    connection established!
    Send request to app request_handler success.
    App request_handler got request, url /url1, action 1
    [resp] ### user resource 1 handler called
    sent 150 bytes to host
    Wasm app process request success.


- Install the event publisher wasm application

.. code-block:: console

    $ sudo ./host_tool -D /dev/ttyUSB0 -i pub -f ./wasm-apps/event_publisher.wasm

    response status 65


- Subscribe event by host_tool

.. code-block:: console

    $ sudo ./host_tool -D /dev/ttyUSB0 -s /alert/overheat -a 3000

    response status 69

    received an event alert/overheat
    {
        "warning":    "temperature is over high"
    }
    received an event alert/overheat
    {
        "warning":    "temperature is over high"
    }
    received an event alert/overheat
    {
        "warning":    "temperature is over high"
    }
    received an event alert/overheat
    {
        "warning":    "temperature is over high"
    }


We can see 4 "alert/overheat" events are received in 3 seconds which is published by the "pub" application.

Output of board

.. code-block:: console

    connection established!
    am_register_event adding url:(alert/overheat)
    client: -3 registered event (alert/overheat)
    sent 16 bytes to host
    sent 142 bytes to host
    sent 142 bytes to host
    sent 142 bytes to host
    sent 142 bytes to host

- Install the event subscriber wasm application

.. code-block:: console


    $ sudo ./host_tool -D /dev/ttyUSB0 -i sub -f ./wasm-apps/event_subscriber.wasm

    response status 65

The "sub" application is installed.

Output of board

.. code-block:: console

    connection established!
    Install WASM app success!
    WASM app 'sub' started
    am_register_event adding url:(alert/overheat)
    client: 3 registered event (alert/overheat)
    sent 16 bytes to host
    Send request to app sub success.
    App sub got request, url alert/overheat, action 6
    ### user over heat event handler called
    Attribute container dump:
    Tag:
    Attribute list:
      key: warning, type: string, value: temperature is over high

    Wasm app process request success.


We can see the "sub" application receives the "alert/overheat" event and dumps it out.
At device side, the event is represented by an attribute container which contains key-value pairs like below:

.. code-block:: console

    Attribute container dump:
    Tag:
    Attribute list:
      key: warning, type: string, value: temperature is over high

    "warning" is the key's name. "string" means this is a string value and "temperature is over high" is the value.

- Uninstall the wasm application

.. code-block:: console


    $ sudo ./host_tool -D /dev/ttyUSB0 -u request_handler

    response status 66

    $ sudo ./host_tool -D /dev/ttyUSB0 -u pub

    response status 66

    $ sudo ./host_tool -D /dev/ttyUSB0 -u sub

    response status 66


- Query again

.. code-block:: console


    $ sudo ./host_tool -D /dev/ttyUSB0 -q

    response status 69
    {
        "num":    0
    }


Note:
*****
Here we only install part of the sample WASM applications, you can also try others by yourself.
And we only run the WASM apps with interpreter mode, you can also run them with AOT (Ahead of Time) mode to improve the performance:

(1) Build the wamrc tool (WAMR AOT compiler), ref to: https://github.com/bytecodealliance/wasm-micro-runtime#build-wamrc-aot-compiler

(2) Compile the WASM file into AOT file with wamrc tool, e.g.:

.. code-block:: console

    $ wamrc --target=thumbv7 --target-abi=eabi --cpu=cortex-m7 -o wasm-apps/timer.aot wasm-apps/timer.wasm
    $ wamrc --target=thumbv7 --target-abi=eabi --cpu=cortex-m7 -o wasm-apps/event_publisher.aot wasm-apps/event_publisher.wasm
    $ wamrc --target=thumbv7 --target-abi=eabi --cpu=cortex-m7 -o wasm-apps/event_subscriber.aot wasm-apps/event_subscriber.wasm

(3) Install AOT file, e.g.:

.. code-block:: console

    $ sudo ./host_tool -D /dev/ttyUSB0 -i timer -f ./wasm-apps/timer.aot
    $ sudo ./host_tool -D /dev/ttyUSB0 -i pub -f ./wasm-apps/event_publisher.aot
    $ sudo ./host_tool -D /dev/ttyUSB0 -i sub -f ./wasm-apps/event_subscriber.aot

References
**********

  - WAMR littlevgl sample: https://github.com/bytecodealliance/wasm-micro-runtime/tree/main/samples/littlevgl
  - WAMR gui sample: https://github.com/bytecodealliance/wasm-micro-runtime/tree/main/samples/gui
