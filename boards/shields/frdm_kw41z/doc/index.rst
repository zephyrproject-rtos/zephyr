.. _frdm_kw41z_shield:

NXP FRDM-KW41Z Shield
#####################

Overview
********

The FRDM-KW41Z is a development kit enabled by the Kinetis |reg| W series
KW41Z/31Z/21Z (KW41Z) family built on ARM |reg| Cortex |reg|-M0+ processor with
integrated 2.4 GHz transceiver supporting Bluetooth |reg| Smart/Bluetooth
|reg| Low Energy
(BLE) v4.2, Generic FSK, IEEE |reg| 802.15.4 and Thread.

The FRDM-KW41Z can be used as a standalone board or as an Arduino shield. This
document covers usage as a shield; see :zephyr:board:`frdm_kw41z` for usage as a
standalone board.

Bluetooth Controller
********************

To use the FRDM-KW41Z as a Bluetooth low energy controller shield with a serial
host controller interface (HCI):

#. Download the MCUXpresso SDK for FRDM-KW41Z from the `MCUXpresso SDK Builder
   Website`_.

#. Open the MCUXpresso IDE or IAR project in
   :file:`boards/frdmkw41z/wireless_examples/bluetooth/hci_black_box/bm`

#. Open :file:`source/common/app_preinclude.h` and add the following line:

   .. code-block:: console

      #define gSerialMgrRxBufSize_c 64

#. Build the project to generate a binary :file:`hci_black_box_frdmkw41z.bin`.

#. Connect the FRDM-KW41Z board to your computer with a USB cable. A USB mass
   storage device should enumerate.

#. Program the binary to flash by copying it to the USB mass storage device.

#. Remove the USB cable to power down the board.

#. Configure the jumpers J30 and J31 such that:

   - J30 pin 1 is attached to J31 pin 2
   - J30 pin 2 is attached to J31 pin 1

   The jumpers should be parallel to the Arduino headers. This configuration
   routes the UART RX and TX signals to the Arduino header, rather than to the
   OpenSDA circuit.

#. Attach the FRDM-KW41Z to the Arduino header on your selected main board,
   such as :zephyr:board:`mimxrt1050_evk` or :zephyr:board:`frdm_k64f`.

#. Set ``--shield frdm_kw41z`` when you invoke ``west build`` in
   your Zephyr bluetooth application. For example,

   .. zephyr-app-commands::
      :zephyr-app: samples/bluetooth/peripheral_hr
      :board: frdm_k64f
      :shield: frdm_kw41z
      :goals: build

References
**********

.. target-notes::

.. _MCUXpresso SDK Builder Website:
   https://mcuxpresso.nxp.com
