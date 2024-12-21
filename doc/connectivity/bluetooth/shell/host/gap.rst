Bluetooth: GAP Shell
####################

The GAP shell is the "main" shell of Bluetooth and handles connection management, scanning,
advertising, and more.


Identities
**********

Identities are a Zephyr host concept, allowing a single physical device to behave like multiple
logical Bluetooth devices.

The shell allows the creation of multiple identities, to a maximum that is set by the Kconfig symbol
:kconfig:option:`CONFIG_BT_ID_MAX`. To create a new identity, use :code:`bt id-create` command. You
can then use it by selecting it with its ID :code:`bt id-select <id>`. Finally, you can list all the
available identities with :code:`id-show`.

Scan for devices
****************

Start scanning by using the :code:`bt scan on` command. Depending on the environment you're in, you
may see a lot of lines printed on the shell. To stop the scan, run :code:`bt scan off`, the
scrolling should stop.

Here is an example of what you can expect:

.. code-block:: console

        uart:~$ bt scan on
        Bluetooth active scan enabled
        [DEVICE]: CB:01:1A:2D:6E:AE (random), AD evt type 0, RSSI -78  C:1 S:1 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
        [DEVICE]: 20:C2:EE:59:85:5B (random), AD evt type 3, RSSI -62  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
        [DEVICE]: E3:72:76:87:2F:E8 (random), AD evt type 3, RSSI -74  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
        [DEVICE]: 1E:19:25:8A:CB:84 (random), AD evt type 3, RSSI -67  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
        [DEVICE]: 26:42:F3:D5:A0:86 (random), AD evt type 3, RSSI -73  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
        [DEVICE]: 0C:61:D1:B9:5D:9E (random), AD evt type 3, RSSI -87  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
        [DEVICE]: 20:C2:EE:59:85:5B (random), AD evt type 3, RSSI -66  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
        [DEVICE]: 25:3F:7A:EE:0F:55 (random), AD evt type 3, RSSI -83  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
        uart:~$ bt scan off
        Scan successfully stopped

As you can see, this can lead to a high number of results. To reduce that number and easily find a
specific device, you can enable scan filters. There are four types of filters: by name, by RSSI, by
address and by periodic advertising interval. To apply a filter, use the :code:`bt scan-set-filter`
command followed by the type of filters. You can add multiple filters by using the commands again.

For example, if you want to look only for devices with the name *test shell*:

.. code-block:: console

        uart:~$ bt scan-filter-set name "test shell"

Or if you want to look for devices at a very close range:

.. code-block:: console

        uart:~$ bt scan-filter-set rssi -40
        RSSI cutoff set at -40 dB

Finally, if you want to remove all filters:

.. code-block:: console

        uart:~$ bt scan-filter-clear all

You can use the command :code:`bt scan on` to create an *active* scanner, meaning that the scanner
will ask the advertisers for more information by sending a *scan request* packet. Alternatively, you
can create a *passive scanner* by using the :code:`bt scan passive` command, so the scanner will not
ask the advertiser for more information.

Connecting to a device
**********************

To connect to a device, you need to know its address and type of address and use the
:code:`bt connect` command with the address and the type as arguments.

Here is an example:

.. code-block:: console

        uart:~$ bt connect 52:84:F6:BD:CE:48 random
        Connection pending
        Connected: 52:84:F6:BD:CE:48 (random)
        Remote LMP version 5.3 (0x0c) subversion 0xffff manufacturer 0x05f1
        LE Features: 0x000000000001412f
        LE PHY updated: TX PHY LE 2M, RX PHY LE 2M
        LE conn  param req: int (0x0018, 0x0028) lat 0 to 42
        LE conn param updated: int 0x0028 lat 0 to 42

You can list the active connections of the shell using the :code:`bt connections` command. The shell
maximum number of connections is defined by :kconfig:option:`CONFIG_BT_MAX_CONN`. You can disconnect
from a connection with the
:code:`bt disconnect <address: XX:XX:XX:XX:XX:XX> <type: (public|random)>` command.

.. note::

        If you were scanning just before, you can connect to the last scanned device by
        simply running the :code:`bt connect` command.

        Alternatively, you can use the :code:`bt connect-name <name>` command to automatically
        enable scanning with a name filter and connect to the first match.

Advertising
***********

Begin advertising by using the :code:`bt advertise on` command. This will use the default parameters
and advertise a resolvable private address with the name of the device. You can choose to use the
identity address instead by running the :code:`bt advertise on identity` command. To stop
advertising use the :code:`bt advertise off` command.

To enable more advanced features of advertising, you should create an advertiser using the
:code:`bt adv-create` command. Parameters for the advertiser can be passed either at the creation of
it or by using the :code:`bt adv-param` command. To begin advertising with this newly created
advertiser, use the :code:`bt adv-start` command, and then the :code:`bt adv-stop` command to stop
advertising.

When using the custom advertisers, you can choose if it will be connectable or scannable. This leads
to four options: :code:`conn-scan`, :code:`conn-nscan`, :code:`nconn-scan` and :code:`nconn-nscan`.
Those parameters are mandatory when creating an advertiser or updating its parameters.

For example, if you want to create a connectable and scannable advertiser and start it:

.. code-block:: console

        uart:~$ bt adv-create conn-scan
        Created adv id: 0, adv: 0x200022f0
        uart:~$ bt adv-start
        Advertiser[0] 0x200022f0 set started

You may notice that with this, the custom advertiser does not advertise the device name; you need to
add it. Continuing from the previous example:

.. code-block:: console

        uart:~$ bt adv-stop
        Advertiser set stopped
        uart:~$ bt adv-data dev-name
        uart:~$ bt adv-start
        Advertiser[0] 0x200022f0 set started

You should now see the name of the device in the advertising data. You can also set a custom name by
using :code:`name <custom name>` instead of :code:`dev-name`. It is also possible to set the
advertising data manually with the :code:`bt adv-data` command. The following example shows how
to set the advertiser name with it using raw advertising data:

.. code-block:: console

        uart:~$ bt adv-create conn-scan
        Created adv id: 0, adv: 0x20002348
        uart:~$ bt adv-data 1009426C7565746F6F74682D5368656C6C
        uart:~$ bt adv-start
        Advertiser[0] 0x20002348 set started

The data must be formatted according to the Bluetooth Core Specification (see version 5.3, vol. 3,
part C, 11). In this example, the first octet is the size of the data (the data and one octet for
the data type), the second one is the type of data, ``0x09`` is the Complete Local Name and the
remaining data are the name in ASCII. So, on the other device you should see the name
*Bluetooth-Shell*.

When advertising, if others devices use an *active* scanner, you may receive *scan request* packets.
To visualize those packets, you can add :code:`scan-reports` to the parameters of your advertiser.

Directed Advertising
====================

It is possible to use directed advertising on the shell if you want to reconnect to a device. The
following example demonstrates how to create a directed advertiser with the address specified right
after the parameter :code:`directed`. The :code:`low` parameter indicates that we want to use the
low duty cycle mode, and the :code:`dir-rpa` parameter is required if the remote device is
privacy-enabled and supports address resolution of the target address in directed advertisement.

.. code-block:: console

        uart:~$ bt adv-create conn-scan directed D7:54:03:CE:F3:B4 random low dir-rpa
        Created adv id: 0, adv: 0x20002348

After that, you can start the advertiser and then the target device will be able to reconnect.

Extended Advertising
====================

Let's now have a look at some extended advertising features. To enable extended advertising, use the
``ext-adv`` parameter.

.. code-block:: console

        uart:~$ bt adv-create conn-nscan ext-adv name-ad
        Created adv id: 0, adv: 0x200022f0
        uart:~$ bt adv-start
        Advertiser[0] 0x200022f0 set started

This will create an extended advertiser, that is connectable and non-scannable.

Encrypted Advertising Data
==========================

Zephyr has support for the Encrypted Advertising Data feature. The :code:`bt encrypted-ad`
sub-commands allow managing the advertising data of a given advertiser.

To encrypt the advertising data, key materials need to be provided, that can be done with :code:`bt
encrypted-ad set-keys <session key> <init vector>`. The session key is 16 bytes long and the
initialisation vector is 8 bytes long.

You can add advertising data by using :code:`bt encrypted-ad add-ad` and :code:`bt encrypted-ad
add-ead`. The former will take add one advertising data structure (as defined in the Core
Specification), when the later will read the given data, encrypt them and then add the generated
encrypted advertising data structure. It's possible to mix encrypted and non-encrypted data, when
done adding advertising data, :code:`bt encrypted-ad commit-ad` can be used to apply the change to
the data to the selected advertiser. After that the advertiser can be started as described
previously. It's possible to clear the advertising data by using :code:`bt encrypted-ad clear-ad`.

On the Central side, it's possible to decrypt the received encrypted advertising data by setting the
correct keys material as described earlier and then enabling the decrypting of the data with
:code:`bt encrypted-ad decrypt-scan on`.

.. note::

        To see the advertising data in the scan report :code:`bt scan-verbose-output` need to be
        enabled.

.. note::

        It's possible to increase the length of the advertising data by increasing the value of
        :kconfig:option:`CONFIG_BT_CTLR_ADV_DATA_LEN_MAX` and
        :kconfig:option:`CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX`.

Here is a simple example demonstrating the usage of EAD:

.. tabs::

        .. group-tab:: Peripheral

                .. code-block:: console

                        uart:~$ bt init
                        ...
                        uart:~$ bt adv-create conn-nscan ext-adv
                        Created adv id: 0, adv: 0x81769a0
                        uart:~$ bt encrypted-ad set-keys 9ba22d3824efc70feb800c80294cba38 2e83f3d4d47695b6
                        session key set to:
                        00000000: 9b a2 2d 38 24 ef c7 0f  eb 80 0c 80 29 4c ba 38 |..-8$... ....)L.8|
                        initialisation vector set to:
                        00000000: 2e 83 f3 d4 d4 76 95 b6                          |.....v..         |
                        uart:~$ bt encrypted-ad add-ad 06097368656C6C
                        uart:~$ bt encrypted-ad add-ead 03ffdead03ffbeef
                        uart:~$ bt encrypted-ad commit-ad
                        Advertising data for Advertiser[0] 0x81769a0 updated.
                        uart:~$ bt adv-start
                        Advertiser[0] 0x81769a0 set started

        .. group-tab:: Central

                .. code-block:: console

                        uart:~$ bt init
                        ...
                        uart:~$ bt scan-verbose-output on
                        uart:~$ bt encrypted-ad set-keys 9ba22d3824efc70feb800c80294cba38 2e83f3d4d47695b6
                        session key set to:
                        00000000: 9b a2 2d 38 24 ef c7 0f  eb 80 0c 80 29 4c ba 38 |..-8$... ....)L.8|
                        initialisation vector set to:
                        00000000: 2e 83 f3 d4 d4 76 95 b6                          |.....v..         |
                        uart:~$ bt encrypted-ad decrypt-scan on
                        Received encrypted advertising data will now be decrypted using provided key materials.
                        uart:~$ bt scan on
                        Bluetooth active scan enabled
                        [DEVICE]: 68:49:30:68:49:30 (random), AD evt type 5, RSSI -59   shell C:1 S:0 D:0 SR:0 E:1 Prim: LE 1M, Secn: LE 2M, Interval: 0x0000 (0 us), SID: 0x0
                                [SCAN DATA START - EXT_ADV]
                                Type 0x09:    shell
                                Type 0x31: Encrypted Advertising Data: 0xe2, 0x17, 0xed, 0x04, 0xe7, 0x02, 0x1d, 0xc9, 0x40, 0x07, uart:~0x18, 0x90, 0x6c, 0x4b, 0xfe, 0x34, 0xad
                                [START DECRYPTED DATA]
                                Type 0xff: 0xde, 0xad
                                Type 0xff: 0xbe, 0xef
                                [END DECRYPTED DATA]
                                [SCAN DATA END]
                        ...

Filter Accept List
******************

It's possible to create a list of allowed addresses that can be used to
connect to those addresses automatically. Here is how to do it:

.. code-block:: console

        uart:~$ bt fal-add 47:38:76:EA:29:36 random
        uart:~$ bt fal-add 66:C8:80:2A:05:73 random
        uart:~$ bt fal-connect on

The shell will then connect to the first available device. In the example, if both devices are
advertising at the same time, we will connect to the first address added to the list.

The Filter Accept List can also be used for scanning or advertising by using the option :code:`fal`.
For example, if we want to scan for a bunch of selected addresses, we can set up a Filter Accept
List:

.. code-block:: console

        uart:~$ bt fal-add 65:4B:9E:83:AF:73 random
        uart:~$ bt fal-add 73:72:82:B4:8F:B9 random
        uart:~$ bt fal-add 5D:85:50:1C:72:64 random
        uart:~$ bt scan on fal

You should see only those three addresses reported by the scanner.

Enabling security
*****************

When connected to a device, you can enable multiple levels of security, here is the list for
Bluetooth LE:

* **1** No encryption and no authentication;
* **2** Encryption and no authentication;
* **3** Encryption and authentication;
* **4** Bluetooth LE Secure Connection.

To enable security, use the :code:`bt security <level>` command. For levels requiring authentication
(level 3 and above), you must first set the authentication method. To do it, you can use the
:code:`bt auth all` command. After that, when you will set the security level, you will be asked to
confirm the passkey on both devices. On the shell side, do it with the command
:code:`bt auth-passkey-confirm`.

Pairing
=======

Enabling authentication requires the devices to be bondable. By default the shell is bondable. You
can make the shell not bondable using :code:`bt bondable off`. You can list all the devices you are
paired with using the command :code:`bt bonds`.

The maximum number of paired devices is set using :kconfig:option:`CONFIG_BT_MAX_PAIRED`. You can
remove a paired device using :code:`bt clear <address: XX:XX:XX:XX:XX:XX> <type: (public|random)>`
or remove all paired devices with the command :code:`bt clear all`.
