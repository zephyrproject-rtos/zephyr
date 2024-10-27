.. _mcu_mgr:

MCUmgr
#######

Overview
********

The management subsystem allows remote management of Zephyr-enabled devices.
The following management operations are available:

* Image management
* File System management
* OS management
* Settings (config) management
* Shell management
* Statistic management
* Zephyr management

over the following transports:

* BLE (Bluetooth Low Energy)
* Serial (UART)
* UDP over IP

The management subsystem is based on the Simple Management Protocol (SMP)
provided by `MCUmgr`_, an open source project that provides a
management subsystem that is portable across multiple real-time operating
systems.

The management subsystem is located in :zephyr_file:`subsys/mgmt/` inside of
the Zephyr tree.

Additionally, there is a :zephyr:code-sample:`sample <smp-svr>` server that provides
management functionality over BLE and serial.

.. _mcumgr_tools_libraries:

Tools/libraries
***************

There are various tools and libraries available which enable usage of MCUmgr functionality on a
device which are listed below. Note that these tools are not part of or related to the Zephyr
project.

.. only:: html

    .. table:: Tools and Libraries for MCUmgr
        :align: center

        +--------------------------------------------------------------------------------+-------------------------------------------+--------------------------+---------------------------------------------------------+---------------+------------+------------+
        | Name                                                                           | OS support                                | Transports               | Groups                                                  | Type          | Language   | License    |
        |                                                                                +---------+-------+-----+--------+----------+--------+-----------+-----+----+-----+------+----------+----+-------+------+--------+               |            |            |
        |                                                                                | Windows | Linux | mac | Mobile | Embedded | Serial | Bluetooth | UDP | OS | IMG | Stat | Settings | FS | Shell | Enum | Zephyr |               |            |            |
        +================================================================================+=========+=======+=====+========+==========+========+===========+=====+====+=====+======+==========+====+=======+======+========+===============+============+============+
        | `AuTerm <https://github.com/thedjnK/AuTerm/>`_                                 | ✓       | ✓     | ✓   | ✕      | ✕        | ✓      | ✓         | ✓   | ✓  | ✓   | ✓    | ✓        | ✓  | ✓     | ✓    | ✓      | Application   | C++ (Qt)   | GPL-3.0    |
        +--------------------------------------------------------------------------------+---------+-------+-----+--------+----------+--------+-----------+-----+----+-----+------+----------+----+-------+------+--------+---------------+------------+------------+
        | `mcumgr-client <https://github.com/vouch-opensource/mcumgr-client/>`_          | ✓       | ✓     | ✓   | ✕      | ✕        | ✓      | ✕         | ✕   | ✕  | ✓   | ✕    | ✕        | ✕  | ✕     | ✕    | ✕      | Application   | Rust       | Apache-2.0 |
        +--------------------------------------------------------------------------------+---------+-------+-----+--------+----------+--------+-----------+-----+----+-----+------+----------+----+-------+------+--------+---------------+------------+------------+
        | `mcumgr-web <https://github.com/boogie/mcumgr-web/>`_                          | ✓       | ✓     | ✓   | ✕      | ✕        | ✕      | ✓         | ✕   | ✕  | ✓   | ✕    | ✕        | ✕  | ✕     | ✕    | ✕      | Web page      | Javascript | MIT        |
        |                                                                                |         |       |     |        |          |        |           |     |    |     |      |          |    |       |      |        | (chrome only) |            |            |
        +--------------------------------------------------------------------------------+---------+-------+-----+--------+----------+--------+-----------+-----+----+-----+------+----------+----+-------+------+--------+---------------+------------+------------+
        | nRF Connect Device Manager: |br|                                               |         |       |     |        |          |        |           |     |    |     |      |          |    |       |      |        |               |            |            |
        | `Android                                                                       | ✕       | ✕     | ✕   | ✓      | ✕        | ✕      | ✓         | ✕   | ✓  | ✓   | ✓    | ✓        | ✓  | ✓     | ✕    | ✓      | Library and   | Java,      | Apache-2.0 |
        | <https://github.com/NordicSemiconductor/Android-nRF-Connect-Device-Manager/>`_ |         |       |     |        |          |        |           |     |    |     |      |          |    |       |      |        | application   | Kotlin,    |            |
        | and `iOS                                                                       |         |       |     |        |          |        |           |     |    |     |      |          |    |       |      |        |               | Swift      |            |
        | <https://github.com/NordicSemiconductor/IOS-nRF-Connect-Device-Manager>`_      |         |       |     |        |          |        |           |     |    |     |      |          |    |       |      |        |               |            |            |
        +--------------------------------------------------------------------------------+---------+-------+-----+--------+----------+--------+-----------+-----+----+-----+------+----------+----+-------+------+--------+---------------+------------+------------+
        | `smp <https://pypi.org/project/smp/>`_                                         | ✓       | ✓     | ✓   | ✓      | ✕        | N/A    | N/A       | N/A | ✓  | ✓   | ✓    | ✓        | ✓  | ✓     | ✕    | ✓      | Library       | Python     | Apache-2.0 |
        +--------------------------------------------------------------------------------+---------+-------+-----+--------+----------+--------+-----------+-----+----+-----+------+----------+----+-------+------+--------+---------------+------------+------------+
        | `smpclient <https://pypi.org/project/smpclient/>`_                             | ✓       | ✓     | ✓   | ✕      | ✕        | ✓      | ✓         | ✓   | ✓  | ✓   | ✓    | ✓        | ✓  | ✓     | ✕    | ✓      | Library       | Python     | Apache-2.0 |
        +--------------------------------------------------------------------------------+---------+-------+-----+--------+----------+--------+-----------+-----+----+-----+------+----------+----+-------+------+--------+---------------+------------+------------+
        | Zephyr MCUmgr client (in-tree)                                                 | ✕       | ✓     | ✕   | ✕      | ✓        | ✓      | ✓         | ✓   | ✓  | ✓   | ✕    | ✕        | ✕  | ✕     | ✕    | ✕      | Library       | C          | Apache-2.0 |
        +--------------------------------------------------------------------------------+---------+-------+-----+--------+----------+--------+-----------+-----+----+-----+------+----------+----+-------+------+--------+---------------+------------+------------+

.. only:: latex

    .. raw:: latex

       \begin{landscape}

    .. table:: Tools and Libraries for MCUmgr
        :align: center

        +--------------------------------------------------------------------------------+---------------+-----------------+---------------------------------------------------------+---------------+------------+
        | Name                                                                           | OS support    | Transports      | Groups                                                  | Type          | Language   |
        |                                                                                |               |                 +----+-----+------+----------+----+-------+------+--------+               |            |
        |                                                                                |               |                 | OS | IMG | Stat | Settings | FS | Shell | Enum | Zephyr |               |            |
        +================================================================================+===============+=================+====+=====+======+==========+====+=======+======+========+===============+============+
        | `AuTerm <https://github.com/thedjnK/AuTerm/>`_                                 | Windows, |br| | Serial, |br|    | ✓  | ✓   | ✓    | ✓        | ✓  | ✓     | ✓    | ✓      | App           | C++ (Qt)   |
        |                                                                                | Linux, |br|   | Bluetooth, |br| |    |     |      |          |    |       |      |        |               |            |
        |                                                                                | macOS         | UDP             |    |     |      |          |    |       |      |        |               |            |
        +--------------------------------------------------------------------------------+---------------+-----------------+----+-----+------+----------+----+-------+------+--------+---------------+------------+
        | `mcumgr-client <https://github.com/vouch-opensource/mcumgr-client/>`_          | Windows, |br| | Serial          | ✕  | ✓   | ✕    | ✕        | ✕  | ✕     | ✕    | ✕      | App           | Rust       |
        |                                                                                | Linux, |br|   |                 |    |     |      |          |    |       |      |        |               |            |
        |                                                                                | macOS         |                 |    |     |      |          |    |       |      |        |               |            |
        +--------------------------------------------------------------------------------+---------------+-----------------+----+-----+------+----------+----+-------+------+--------+---------------+------------+
        | `mcumgr-web <https://github.com/boogie/mcumgr-web/>`_                          | Windows, |br| | Bluetooth       | ✕  | ✓   | ✕    | ✕        | ✕  | ✕     | ✕    | ✕      | Web (chrome   | Javascript |
        |                                                                                | Linux, |br|   |                 |    |     |      |          |    |       |      |        | only)         |            |
        |                                                                                | macOS         |                 |    |     |      |          |    |       |      |        |               |            |
        +--------------------------------------------------------------------------------+---------------+-----------------+----+-----+------+----------+----+-------+------+--------+---------------+------------+
        | nRF Connect Device Manager: |br|                                               | iOS, |br|     | Bluetooth       | ✓  | ✓   | ✓    | ✓        | ✓  | ✓     | ✕    | ✓      | Library, App  | Java,      |
        | `Android                                                                       | Android       |                 |    |     |      |          |    |       |      |        |               | Kotlin,    |
        | <https://github.com/NordicSemiconductor/Android-nRF-Connect-Device-Manager/>`_ |               |                 |    |     |      |          |    |       |      |        |               | Swift      |
        | and `iOS                                                                       |               |                 |    |     |      |          |    |       |      |        |               |            |
        | <https://github.com/NordicSemiconductor/IOS-nRF-Connect-Device-Manager>`_      |               |                 |    |     |      |          |    |       |      |        |               |            |
        +--------------------------------------------------------------------------------+---------------+-----------------+----+-----+------+----------+----+-------+------+--------+---------------+------------+
        | `smp <https://pypi.org/project/smp/>`_                                         | Windows, |br| | N/A             | ✓  | ✓   | ✓    | ✓        | ✓  | ✓     | ✕    | ✓      | Library       | Python     |
        |                                                                                | Linux, |br|   |                 |    |     |      |          |    |       |      |        |               |            |
        |                                                                                | macOS, |br|   |                 |    |     |      |          |    |       |      |        |               |            |
        |                                                                                | iOS, |br|     |                 |    |     |      |          |    |       |      |        |               |            |
        |                                                                                | Android       |                 |    |     |      |          |    |       |      |        |               |            |
        +--------------------------------------------------------------------------------+---------------+-----------------+----+-----+------+----------+----+-------+------+--------+---------------+------------+
        | `smpclient <https://pypi.org/project/smpclient/>`_                             | Windows, |br| | Serial, |br|    | ✓  | ✓   | ✓    | ✓        | ✓  | ✓     | ✕    | ✓      | Library       | Python     |
        |                                                                                | Linux, |br|   | Bluetooth, |br| |    |     |      |          |    |       |      |        |               |            |
        |                                                                                | macOS         | UDP             |    |     |      |          |    |       |      |        |               |            |
        +--------------------------------------------------------------------------------+---------------+-----------------+----+-----+------+----------+----+-------+------+--------+---------------+------------+
        | Zephyr MCUmgr client (in-tree)                                                 | Linux, |br|   | Serial, |br|    | ✓  | ✓   | ✕    | ✕        | ✕  | ✕     | ✕    | ✕      | Library       | C          |
        |                                                                                | Zephyr        | Bluetooth, |br| |    |     |      |          |    |       |      |        |               |            |
        |                                                                                |               | UDP             |    |     |      |          |    |       |      |        |               |            |
        +--------------------------------------------------------------------------------+---------------+-----------------+----+-----+------+----------+----+-------+------+--------+---------------+------------+

    .. raw:: latex

        \end{landscape}

Note that a tick for a particular group indicates basic support for that group in the code, it is
possible that not all commands/features of a group are supported by the implementation.

.. _mcumgr_jlink_ob_virtual_msd:

J-Link Virtual MSD Interaction Note
***********************************

On boards where a J-Link OB is present which has both CDC and MSC (virtual Mass
Storage Device, also known as drag-and-drop) support, the MSD functionality can
prevent MCUmgr commands over the CDC UART port from working due to how USB
endpoints are configured in the J-Link firmware (for example on the
:ref:`Nordic nrf52840dk/nrf52840 board <nrf52840dk_nrf52840>`) because of
limiting the maximum packet size (most likely to occur when using image
management commands for updating firmware). This issue can be
resolved by disabling MSD functionality on the J-Link device, follow the
instructions on :ref:`nordic_segger_msd` to disable MSD support.

Bootloader Integration
**********************

The :ref:`dfu` subsystem integrates the management subsystem with the
bootloader, providing the ability to send and upgrade a Zephyr image to a
device.

Currently only the MCUboot bootloader is supported. See :ref:`mcuboot` for more
information.

.. _MCUmgr: https://github.com/apache/mynewt-mcumgr
.. _MCUboot design: https://github.com/mcu-tools/mcuboot/blob/main/docs/design.md

Discord channel
***************

Developers welcome!

* Discord mcumgr channel: https://discord.com/invite/Ck7jw53nU2

API Reference
*************

.. doxygengroup:: mcumgr_mgmt_api
