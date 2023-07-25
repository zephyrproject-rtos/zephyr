.. _bluetooth_encrypted_advertising_sample:

Bluetooth: Encrypted Advertising
################################

Overview
********

This sample demonstrates the use of the encrypted advertising feature, such as:

 - the exchange of the session key and the initialization vector using the Key
   Material characteristic,
 - the encryption of advertising payloads,
 - the decryption of those advertising payloads,
 - and the update of the Randomizer field whenever the RPA is changed.

To use the `bt_ead_encrypt` and `bt_ead_decrypt` functions, you must enable
the Kconfig symbol :kconfig:option:`CONFIG_BT_EAD`.

While this sample uses extended advertising, it is **not** mandatory when using
the encrypted advertising. The feature can be used with legacy advertising as
well.

Requirements
************

* Two boards with Bluetooth Low Energy support
* Two boards with a push button connected via a GPIO pin, see :ref:`Button
  sample <button-sample>` for more details

Building and Running
********************

This sample can be found under
:zephyr_file:`samples/bluetooth/encrypted_advertising` in the Zephyr tree.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.

This sample uses two applications, so two devices need to be setup.
Flash one device with the central application, and another device with the
peripheral application.

The two devices should automatically connect if they are close enough.

After the boards are connected, you will be asked to press the configured push
button on both boards to confirm the pairing request.

Here are the outputs you should get by default:

Peripheral:

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v3.3.0-1872-g6fac3c7581dc ***
        <inf> ead_peripheral_sample: Advertising data size: 64
        Passkey for 46:04:2E:6F:80:12 (random): 059306
        Confirm passkey by pressing button at gpio@50000000 pin 11...
        Passkey confirmed.
        <inf> ead_peripheral_sample: Advertising data size: 64


Central:

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v3.3.0-1872-g6fac3c7581dc ***
        Passkey for 6C:7F:67:C2:8B:29 (random): 059306
        Confirm passkey by pressing button at gpio@50000000 pin 11...
        Passkey confirmed.
        <inf> ead_central_sample: Received data size: 64
        <inf> ead_central_sample: len : 10
        <inf> ead_central_sample: type: 0x09
        <inf> ead_central_sample: data:
                                  45 41 44 20 53 61 6d 70  6c 65                   |EAD Samp le
        <inf> ead_central_sample: len : 8
        <inf> ead_central_sample: type: 0xff
        <inf> ead_central_sample: data:
                                  05 f1 5a 65 70 68 79 72                          |..Zephyr
        <inf> ead_central_sample: len : 7
        <inf> ead_central_sample: type: 0xff
        <inf> ead_central_sample: data:
                                  05 f1 49 d2 f4 55 76                             |..I..Uv
        <inf> ead_central_sample: len : 4
        <inf> ead_central_sample: type: 0xff
        <inf> ead_central_sample: data:
                                  05 f1 c1 25                                      |...%
        <inf> ead_central_sample: len : 3
        <inf> ead_central_sample: type: 0xff
        <inf> ead_central_sample: data:
                                  05 f1 17                                         |...
