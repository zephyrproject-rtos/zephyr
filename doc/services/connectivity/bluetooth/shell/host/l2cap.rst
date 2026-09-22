Bluetooth: L2CAP Shell
######################

The :code:`l2cap` command exposes parts of the L2CAP API. The following example shows how to
register a LE PSM, connect to it from another device and send 3 packets of 14 octets each.

The example assumes that the two devices are already connected.

On device A, register the LE PSM:

.. code-block:: console

        uart:~$ l2cap register 29
        L2CAP psm 41 sec_level 1 registered

On device B, connect to the registered LE PSM and send data:

.. code-block:: console

        uart:~$ l2cap connect 29
        Chan sec: 1
        L2CAP connection pending
        Channel 0x20000210 connected
        Channel 0x20000210 status 1
        uart:~$ l2cap send 3 14
        Rem 2
        Rem 1
        Rem 0
        Outgoing data channel 0x20000210 transmitted
        Outgoing data channel 0x20000210 transmitted
        Outgoing data channel 0x20000210 transmitted

On device A, you should have received the data:

.. code-block:: console

        Incoming conn 0x20002398
        Channel 0x20000210 status 1
        Channel 0x20000210 connected
        Channel 0x20000210 requires buffer
        Incoming data channel 0x20000210 len 14
        00000000: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff       |........ ......  |
        Channel 0x20000210 requires buffer
        Incoming data channel 0x20000210 len 14
        00000000: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff       |........ ......  |
        Channel 0x20000210 requires buffer
        Incoming data channel 0x20000210 len 14
        00000000: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff       |........ ......  |
