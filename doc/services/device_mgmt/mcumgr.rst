.. _mcu_mgr:

MCUmgr
#######

Overview
********

The management subsystem allows remote management of Zephyr-enabled devices.
The following management operations are available:

* Image management
* File System management
* Log management (currently disabled)
* OS management
* Shell management

over the following transports:

* BLE (Bluetooth Low Energy)
* Serial (UART)
* UDP over IP

The management subsystem is based on the Simple Management Protocol (SMP)
provided by `MCUmgr`_, an open source project that provides a
management subsystem that is portable across multiple real-time operating
systems.

The management subsystem is split in two different locations in the Zephyr tree:

* `zephyrproject-rtos/mcumgr repo <https://github.com/zephyrproject-rtos/mcumgr>`_
  contains a clean import of the MCUmgr project
* :zephyr_file:`subsys/mgmt/` contains the Zephyr-specific bindings to MCUmgr

Additionally there is a :ref:`sample <smp_svr_sample>` that provides management
functionality over BLE and serial.

.. _mcumgr_cli:

Command-line Tool
*****************

MCUmgr provides a command-line tool, :file:`mcumgr`, for managing remote devices.
The tool is written in the Go programming language.

To install the tool:

.. tabs::

   .. group-tab:: go version < 1.18

      .. code-block:: console

         go get github.com/apache/mynewt-mcumgr-cli/mcumgr

   .. group-tab:: go version >= 1.18

      .. code-block:: console

         go install github.com/apache/mynewt-mcumgr-cli/mcumgr@latest

Configuring the transport
*************************

There are two command-line options that are responsible for setting and configuring
the transport layer to use when communicating with managed device:

* ``--conntype`` is used to choose the transport used, and
* ``--connstring`` is used to pass a comma separated list of options in the
  ``key=value`` format, where each valid ``key`` depends on the particular
  ``conntype``.

Valid transports for ``--conntype`` are ``serial``, ``ble`` and ``udp``. Each
transport expects a different set of key/value options:

.. tabs::

   .. group-tab:: serial

      ``--connstring`` accepts the following ``key`` values:

      .. list-table::
         :width: 100%
         :widths: 10 60

         * - ``dev``
           - the device name for the OS ``mcumgr`` is running on (eg, ``/dev/ttyUSB0``, ``/dev/tty.usbserial``, ``COM1``, etc).
         * - ``baud``
           - the communication speed; must match the baudrate of the server.
         * - ``mtu``
           - aka Maximum Transmission Unit, the maximum protocol packet size.

   .. group-tab:: ble

      ``--connstring`` accepts the following ``key`` values:

      .. list-table::
         :width: 100%
         :widths: 10 60

         * - ``ctlr_name``
           - an OS specific string for the controller name.
         * - ``own_addr_type``
           - can be one of ``public``, ``random``, ``rpa_pub``, ``rpa_rnd``, where ``random`` is the default.
         * - ``peer_name``
           - the name the peer BLE device advertises, this should match the configuration specified with :kconfig:option:`CONFIG_BT_DEVICE_NAME`.
         * - ``peer_id``
           - the peer BLE device address or UUID. Only required when ``peer_name`` was not given. The format depends on the OS where ``mcumgr`` is run, it is a 6 bytes hexadecimal string separated by colons on Linux, or a 128-bit UUID on macOS.
         * - ``conn_timeout``
           - a float number representing the connection timeout in seconds.

   .. group-tab:: udp

      ``--connstring`` takes the form ``[addr]:port`` where:

      .. list-table::
         :width: 100%
         :widths: 10 60

         * - ``addr``
           - can be a DNS name (if it can be resolved to the device IP), IPv4 addr (app must be built with :kconfig:option:`CONFIG_MCUMGR_SMP_UDP_IPV4`), or IPv6 addr (app must be built with :kconfig:option:`CONFIG_MCUMGR_SMP_UDP_IPV6`)
         * - ``port``
           - any valid UDP port.

Saving the connection config
****************************

The transport configuration can be managed with the ``conn`` sub-command and
later used with ``--conn`` (or ``-c``) parameter to skip typing both ``--conntype``
and ``--connstring``. For example a new config for a serial device that would
require typing ``mcumgr --conntype serial --connstring dev=/dev/ttyACM0,baud=115200,mtu=512``
can be saved with::

  mcumgr conn add acm0 type="serial" connstring="dev=/dev/ttyACM0,baud=115200,mtu=512"

Accessing this port can now be done with::

  mcumgr -c acm0

.. _general_options:

General options
***************

Some options work for every ``mcumgr`` command and might be helpful to debug and fix
issues with the communication, among them the following deserve special mention:

.. list-table::
   :width: 100%
   :widths: 10 60

   * - ``-l <log-level>``
     - Configures the log level, which can be one of ``critical``, ``error``,
       ``warn``, ``info`` or ``debug``, from less to most verbose. When there are
       communication issues, ``-lDEBUG`` might be useful to dump the packets for
       later inspection.
   * - ``-t <timeout>``
     - Changes the timeout waiting for a response from the default of 10s to a
       given value. Some commands might take a long time of processing, eg, the
       erase before an image upload, and might need incrementing the timeout to
       a larger value.
   * - ``-r <tries>``
     - Changes the number of retries on timeout from the default of 1 to a given
       value.

List of Commands
****************

Not all commands defined by ``mcumgr`` (and SMP protocol) are currently supported
on Zephyr. The ones that are supported are described in the following table:

.. tip:: Running ``mcumgr`` with no parameters, or ``-h`` will display the list
   of commands.

.. list-table::
   :widths: 10 30
   :header-rows: 1

   * - Command
     - Description
   * - ``echo``
     - Send data to a device and display the echoed back data. This command is
       part of the ``OS`` group, which must be enabled by setting
       :kconfig:option:`CONFIG_MCUMGR_CMD_OS_MGMT`. The ``echo`` command itself can be
       enabled by setting :kconfig:option:`CONFIG_OS_MGMT_ECHO`.
   * - ``fs``
     - Access files on a device. More info in :ref:`fs_mgmt`.
   * - ``image``
     - Manage images on a device. More info in :ref:`image_mgmt`.
   * - ``reset``
     - Perform a soft reset of a device. This command is part of the ``OS``
       group, which must be enabled by setting :kconfig:option:`CONFIG_MCUMGR_CMD_OS_MGMT`.
       The ``reset`` command itself is always enabled and the time taken for a
       reset to happen can be set with :kconfig:option:`CONFIG_OS_MGMT_RESET_MS` (in ms).
   * - ``shell``
     - Execute a command in the remote shell. This option is disabled by default
       and can be enabled with :kconfig:option:`CONFIG_MCUMGR_CMD_SHELL_MGMT` = ``y``.
       To know more about the shell in Zephyr check :ref:`shell_api`.
   * - ``stat``
     - Read statistics from a device. More info in :ref:`stats_mgmt`.
   * - ``taskstat``
     - Read task statistics from a device. This command is part of the ``OS``
       group, which must be enabled by setting :kconfig:option:`CONFIG_MCUMGR_CMD_OS_MGMT`.
       The ``taskstat`` command itself can be enabled by setting
       :kconfig:option:`CONFIG_OS_MGMT_TASKSTAT`. :kconfig:option:`CONFIG_THREAD_MONITOR` also
       needs to be enabled otherwise a ``-8`` (``MGMT_ERR_ENOTSUP``) will be
       returned.

.. tip::

    ``taskstat`` has a few options that might require tweaking. The
    :kconfig:option:`CONFIG_THREAD_NAME` must be set to display the task names, otherwise
    the priority is displayed. Since the ``taskstat`` packets are large, they
    might need increasing the :kconfig:option:`CONFIG_MCUMGR_BUF_SIZE` option.

.. warning::

    To display the correct stack size in the ``taskstat`` command, the
    :kconfig:option:`CONFIG_THREAD_STACK_INFO` option must be set.
    To display the correct stack usage in the ``taskstat`` command, both
    :kconfig:option:`CONFIG_THREAD_STACK_INFO` and :kconfig:option:`CONFIG_INIT_STACKS` options
    must be set.

.. _mcumgr_jlink_ob_virtual_msd:

J-Link Virtual MSD Interaction Note
***********************************

On boards where a J-Link OB is present which has both CDC and MSC (virtual Mass
Storage Device, also known as drag-and-drop) support, the MSD functionality can
prevent mcumgr commands over the CDC UART port from working due to how USB
endpoints are configured in the J-Link firmware (for example on the Nordic
``nrf52840dk``) because of limiting the maximum packet size (most likely to occur
when using image management commands for updating firmware). This issue can be
resolved by disabling MSD functionality on the J-Link device, follow the
instructions on :ref:`nordic_segger_msd` to disable MSD support.

.. _image_mgmt:

Image Management
****************

The image management provided by ``mcumgr`` is  based on the image format defined
by MCUboot. For more details on the internals see `MCUboot design`_ and :ref:`west-sign`.

To list available images in a device::

  mcumgr <connection-options> image list

This should result in an output similar to this::

  $ mcumgr -c acm0 image list
  Images:
    image=0 slot=0
      version: 1.0.0
      bootable: true
      flags: active confirmed
      hash: 86dca73a3439112b310b5e033d811ec2df728d2264265f2046fced5a9ed00cc7
  Split status: N/A (0)

Where ``image`` is the number of the image pair in a multi-image system, and slot
is the number of the slot where the image is stored, ``0`` for primary and ``1`` for
secondary. This image being ``active`` and ``confirmed`` means it will run again on
next reset. Also relevant is the ``hash``, which is used by other commands to
refer to this specific image when performing operations.

An image can be manually erased using::

  mcumgr <connection-options> image erase

The behavior of ``erase`` is defined by the server (``mcumgr`` in the device).
The current implementation is limited to erasing the image in the secondary
partition.

To upload a new image::

  mcumgr <connection-options> image upload [-n] [-e] [-u] [-w] <signed-bin>

* ``-n``: This option allows uploading a new image to a specific set of images
  in a multi-image system, and is currently only supported by MCUboot when the
  CONFIG\ _MCUBOOT_SERIAL option is enabled.

* ``-e``: This option avoids performing a full erase of the partition before
  starting a new upload.

.. tip::

   The ``-e`` option should always be passed in because the ``upload`` command
   already checks if an erase is required, respecting the
   :kconfig:option:`CONFIG_IMG_ERASE_PROGRESSIVELY` setting.

.. tip::

   If the ``upload`` command times out while waiting for a response from the
   device, ``-t`` might be used to increase the wait time to something larger
   than the default of 10s. See general_options_.

.. warning::

   ``mcumgr`` does not understand .hex files, when uploading a new image always
   use the .bin file.

* ``-u``: This option allows upgrading only to newer image version.

* ``-w``: This option allows setting the maximum size for the window of outstanding chunks in transit.
  It is set to 5 by default.

  .. tip::

     If the option is set to a value lower than the default one, for example ``-w 1``, less chunks are transmitted on the window,
     resulting in lower risk of errors. Conversely, setting a value higher than 5 increases risk of errors and may impact performance.

After an image upload is finished, a new ``image list`` would now have an output
like this::

  $ mcumgr -c acm0 image upload -e build/zephyr/zephyr.signed.bin
    35.69 KiB / 92.92 KiB [==========>---------------]  38.41% 2.97 KiB/s 00m19

Now listing the images again::

  $ mcumgr -c acm0 image list
  Images:
   image=0 slot=0
    version: 1.0.0
    bootable: true
    flags: active confirmed
    hash: 86dca73a3439112b310b5e033d811ec2df728d2264265f2046fced5a9ed00cc7
   image=0 slot=1
    version: 1.1.0
    bootable: true
    flags:
    hash: e8cf0dcef3ec8addee07e8c4d5dc89e64ba3fae46a2c5267fc4efbea4ca0e9f4
  Split status: N/A (0)

To test a new upgrade image the ``test`` command is used::

  mcumgr <connection-options> image test <hash>

This command should mark a ``test`` upgrade, which means that after the next
reboot the bootloader will execute the upgrade and jump into the new image. If no
other image operations are executed on the newly running image, it will ``revert``
back to the image that was previously running on the device on the subsequent reset.
When a ``test`` is requested, ``flags`` will be updated with ``pending`` to inform
that a new image will be run after a reset::

  $ mcumgr -c acm0 image test e8cf0dcef3ec8addee07e8c4d5dc89e64ba3fae46a2c5267fc4efbea4ca0e9f4
  Images:
   image=0 slot=0
    version: 1.0.0
    bootable: true
    flags: active confirmed
    hash: 86dca73a3439112b310b5e033d811ec2df728d2264265f2046fced5a9ed00cc7
   image=0 slot=1
    version: 1.1.0
    bootable: true
    flags: pending
    hash: e8cf0dcef3ec8addee07e8c4d5dc89e64ba3fae46a2c5267fc4efbea4ca0e9f4
  Split status: N/A (0)

After a reset the output with change to::

  $ mcumgr -c acm0 image list
  Images:
   image=0 slot=0
    version: 1.1.0
    bootable: true
    flags: active
    hash: e8cf0dcef3ec8addee07e8c4d5dc89e64ba3fae46a2c5267fc4efbea4ca0e9f4
   image=0 slot=1
    version: 1.0.0
    bootable: true
    flags: confirmed
    hash: 86dca73a3439112b310b5e033d811ec2df728d2264265f2046fced5a9ed00cc7
  Split status: N/A (0)

.. tip::

   It's important to mention that an upgrade only ever happens if the image is
   valid. The first thing MCUboot does when an upgrade is requested is to
   validate the image, using the SHA-256 and/or the signature (depending on
   the configuration). So before uploading an image, one way to be sure it is
   valid is to run ``imgtool verify -k <your-signature-key> <your-image>``,
   where ``-k <your-signature-key`` can be skipped if no signature validation
   was enabled.

The ``confirmed`` flag in the secondary slot tells that after the next reset a
revert upgrade will be performed to switch back to the original layout.

The command used to confirm that an image is OK and no revert should happen
(no ``hash`` required) is::

  mcumgr <connection-options> image confirm [hash]

The ``confirm`` command can also be run passing in a ``hash`` so that instead of
doing a ``test``/``revert`` procedure, the image in the secondary partition is
directly upgraded to.

.. tip::

   The whole ``test``/``revert`` cycle does not need to be done using only the
   ``mcumgr`` command-line tool. A better alternative is to perform a ``test``
   and allow the new running image to self-confirm after any checks by calling
   :c:func:`boot_write_img_confirmed`.

.. tip::

    The maximum size of a chunk communicated between the client and server is set
    with :kconfig:option:`CONFIG_IMG_MGMT_UL_CHUNK_SIZE`. The default is 512 but can be
    decreased for systems with low amount of RAM down to 128. When this value is
    changed, the ``mtu`` of the port must be smaller than or equal to this value.

.. tip::

    Building with :kconfig:option:`CONFIG_IMG_MGMT_VERBOSE_ERR` enables better error
    messages when failures happen (but increases the application size).

.. _stats_mgmt:

Statistics Management
*********************

Statistics are used for troubleshooting, maintenance, and usage monitoring; it
consists basically of user-defined counters which are tightly connected to
``mcumgr`` and can be used to track any information for easy retrieval. The
available sub-commands are::

  mcumgr <connection-options> stat list
  mcumgr <connection-options> stat <section-name>

Statistics are organized in sections (also called groups), and each section can
be individually queried. Defining new statistics sections is done by using macros
available under ``<stats/stats.h>``. Each section consists of multiple variables
(or counters), all with the same size (16, 32 or 64 bits).

To create a new section ``my_stats``::

  STATS_SECT_START(my_stats)
    STATS_SECT_ENTRY(my_stat_counter1)
    STATS_SECT_ENTRY(my_stat_counter2)
    STATS_SECT_ENTRY(my_stat_counter3)
  STATS_SECT_END;

  STATS_SECT_DECL(my_stats) my_stats;

Each entry can be declared with ``STATS_SECT_ENTRY`` (or the equivalent
``STATS_SECT_ENTRY32``), ``STATS_SECT_ENTRY16`` or ``STATS_SECT_ENTRY64``.
All statistics in a section must be declared with the same size.

The statistics counters can either have names or not, depending on the setting
of the :kconfig:option:`CONFIG_STATS_NAMES` option. Using names requires an extra
declaration step::

  STATS_NAME_START(my_stats)
    STATS_NAME(my_stats, my_stat_counter1)
    STATS_NAME(my_stats, my_stat_counter2)
    STATS_NAME(my_stats, my_stat_counter3)
  STATS_NAME_END(my_stats);

.. tip::

   Disabling :kconfig:option:`CONFIG_STATS_NAMES` will free resources. When this option
   is disabled the ``STATS_NAME*`` macros output nothing, so adding them in the
   code does not increase the binary size.

.. tip::

   :kconfig:option:`CONFIG_STAT_MGMT_MAX_NAME_LEN` sets the maximum length of a section
   name that can can be accepted as parameter for showing the section data, and
   might require tweaking for long section names.

The final steps to use a statistics section is to initialize and register it::

  rc = STATS_INIT_AND_REG(my_stats, STATS_SIZE_32, "my_stats");
  assert (rc == 0);

In the running code a statistics counter can be incremented by 1 using
``STATS_INC``, by N using ``STATS_INCN`` or reset with ``STATS_CLEAR``.

Let's suppose we want to increment those counters by ``1``, ``2`` and ``3``
every second. To get a list of stats::

  $ mcumgr --conn acm0 stat list
  stat groups:
    my_stats

To get the current value of the counters in ``my_stats``::

  $ mcumgr --conn acm0 stat my_stats
  stat group: my_stats
        13 my_stat_counter1
        26 my_stat_counter2
        39 my_stat_counter3

  $ mcumgr --conn acm0 stat my_stats
  stat group: my_stats
        16 my_stat_counter1
        32 my_stat_counter2
        48 my_stat_counter3

When :kconfig:option:`CONFIG_STATS_NAMES` is disabled the output will look like this::

  $ mcumgr --conn acm0 stat my_stats
  stat group: my_stats
         8 s0
        16 s1
        24 s2

.. _fs_mgmt:

Filesystem Management
*********************

The filesystem module is disabled by default due to security concerns:
because of a lack of access control every file in the FS will be accessible,
including secrets, etc. To enable it :kconfig:option:`CONFIG_MCUMGR_CMD_FS_MGMT` must
be set (``y``). Once enabled the following sub-commands can be used::

  mcumgr <connection-options> fs download <remote-file> <local-file>
  mcumgr <connection-options> fs upload <local-file> <remote-file>

Using the ``fs`` command, requires :kconfig:option:`CONFIG_FILE_SYSTEM` to be enabled,
and that some particular filesystem is enabled and properly mounted by the running
application, eg for littlefs this would mean enabling
:kconfig:option:`CONFIG_FILE_SYSTEM_LITTLEFS`, defining a storage partition :ref:`flash_map_api`
and mounting the filesystem in the startup (:c:func:`fs_mount`).

Uploading a new file to a littlefs storage, mounted under ``/lfs``, can be done with::

  $ mcumgr -c acm0 fs upload foo.txt /lfs/foo.txt
  25
  Done

Where ``25`` is the size of the file.

For downloading a file, let's first use the ``fs`` command
(:kconfig:option:`CONFIG_FILE_SYSTEM_SHELL` must be enabled) in a remote shell to create
a new file::

  uart:~$ fs write /lfs/bar.txt 41 42 43 44 31 32 33 34 0a
  uart:~$ fs read /lfs/bar.txt
  File size: 9
  00000000  41 42 43 44 31 32 33 34 0A                       ABCD1234.

Now it can be downloaded using::

  $ mcumgr -c acm0 fs download /lfs/bar.txt bar.txt
  0
  9
  Done
  $ cat bar.txt
  ABCD1234

Where ``0`` is the return code, and ``9`` is the size of the file.

.. warning::

   The commands might exhaust the system workqueue, if its size is not large
   enough, so increasing :kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` might be
   required for correct behavior.

The size of the stack allocated buffer used to store the blocks, while transferring
a file can be adjusted with :kconfig:option:`CONFIG_FS_MGMT_DL_CHUNK_SIZE`; this allows
saving RAM resources.

.. tip::

   :kconfig:option:`CONFIG_FS_MGMT_PATH_SIZE` sets the maximum PATH accepted for a file
   name. It might require tweaking for longer file names.

Bootloader integration
**********************

The :ref:`dfu` subsystem integrates the management subsystem with the
bootloader, providing the ability to send and upgrade a Zephyr image to a
device.

Currently only the MCUboot bootloader is supported. See :ref:`mcuboot` for more
information.

.. _MCUmgr: https://github.com/apache/mynewt-mcumgr
.. _MCUboot design: https://github.com/mcu-tools/mcuboot/blob/main/docs/design.md
