.. zephyr:code-sample:: amp_talk
   :name: OpenAMP DSP AMP Sample
   :relevant-api: ipm_interface

   Send messages between multiple cores using OpenAMP with concurrent communication channels.

Overview
********

This application demonstrates how to use OpenAMP for multi-core communication on NXP RT700
series processors. It showcases concurrent OpenAMP communication from a primary core (CM33_0)
to multiple remote cores:

- **CM33_0 ↔ CM33_1**: Inter-processor communication between Cortex-M33 cores
- **CM33_0 ↔ HiFi4 DSP**: Communication between ARM Cortex-M33 and Cadence HiFi4 DSP

The application demonstrates:

- Multiple concurrent OpenAMP/RPMSG instances
- Separate shared memory regions for each communication channel
- Independent message passing with different remote cores
- Proper resource isolation and synchronization

Architecture
************

The sample consists of three separate applications:

1. **Primary Core (CM33_0)**:

   - Runs two concurrent threads
   - Manages communication with both CM33_1 and HiFi4 DSP
   - Acts as RPMSG host for both channels

2. **Remote CM33 Core (CM33_1)**:

   - Runs as RPMSG remote
   - Communicates with CM33_0 via dedicated shared memory

3. **Remote DSP Core (HiFi4)**:

   - Runs as RPMSG remote
   - Communicates with CM33_0 via separate shared memory region

Building the application for mimxrt700_evk
******************************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/adsp/rtxxx/amp_talk/
   :board: mimxrt700_evk/mimxrt798s/cm33_cpu0
   :goals: debug
   :west-args: --sysbuild

Open a serial terminal (minicom, putty, etc.) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and the following message will appear on the corresponding
serial port, one is main another is remote:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-4126-g68238d6377d2 ***
    [ARM] Starting DSP...
    *** Booting Zephyr OS build v4.2.0-4126-g68238d6377d2 ***
    [DSP] Starting application thread!
    OpenAMP[DSP] demo started
    Starting application threads!

    OpenAMP[main->CM33] demo started
    Main core received a message: 1
    Main core received a message: 3
    Main core received a message: 5
    Main core received a message: 7
    Main core received a message: 9
    Main core received a message: 11
    ...
    Main core received a message: 97
    Main core received a message: 99
    OpenAMP demo ended.

    OpenAMP[main->DSP] demo started
    [DSP] Remote core received a message: 0
    Main core received a message: 1
    [DSP] Remote core received a message: 2
    Main core received a message: 3
    [DSP] Remote core received a message: 4
    Main core received a message: 5
    [DSP] Remote core received a message: 6
    Main core received a message: 7
    [DSP] Remote core received a message: 8
    Main core received a message: 9
    [DSP] Remote core received a message: 10
    Main core received a message: 11
    ...
    [DSP] Remote core received a message: 98
    Main core received a message: 99
    OpenAMP demo ended.
    [DSP] OpenAMP demo ended.

.. code-block:: console

    *** Booting Zephyr OS build v4.2.0-4126-g68238d6377d2 ***
    Starting application thread!

    OpenAMP[remote] demo started
    Remote core received a message: 0
    Remote core received a message: 2
    Remote core received a message: 4
    Remote core received a message: 6
    Remote core received a message: 8
    Remote core received a message: 10
    Remote core received a message: 12
    ...
    Remote core received a message: 96
    Remote core received a message: 98
    OpenAMP demo ended.

Key Features
************

- **Concurrent Communication**: Primary core manages two independent OpenAMP channels simultaneously
- **Resource Isolation**: Each communication channel uses separate shared memory regions and IPM instances
- **Multi-Architecture Support**: Demonstrates ARM Cortex-M33 to Cortex-M33 and ARM to DSP communication
- **Scalable Design**: Architecture can be extended to support additional remote cores

Technical Details
*****************

The implementation uses:

- Separate RPMSG virtio devices for each communication channel
- Independent shared memory pools and regions
- Dedicated IPM (Inter-Processor Messaging) instances
- Proper OpenAMP resource management and cleanup

This sample serves as a reference for implementing complex multi-core communication
scenarios in embedded systems using the OpenAMP framework.
