.. zephyr:code-sample:: rtl8730e_wifi_tricore
   :name: RTL8730E tri-core WiFi

   Run the full WiFi stack across all three RTL8730E cores on Zephyr.

Overview
********

Builds one flashable image set in which every core runs Zephyr:

* **CA32** (``rtl8730e_evb/rtl8730e/ca32``, default image): network stack,
  WiFi shell, WHC host role (links the prebuilt ``lib_wifi_whc_ipc_host``).
* **KM4** (``.../km4``): WHC network processor -- the 802.11 MAC/PHY driver
  and the WHC IPC device (links the prebuilt ``lib_wifi_whc_np`` and, with
  coexistence, ``lib_coex``).
* **KM0** (``.../km0``): the WiFi MAC real-time firmware (links the prebuilt
  ``lib_wifi_fw``).

The vendor IMG1 (``km4_boot_all.bin``) remains the bootloader; the CA32
image-assembly step folds the KM4/KM0 image2 sub-images into a single
``app.bin`` for the vendor IMG2 flash slot.

Requirements
************

The per-core WiFi libraries must be present in the blob checkout
(``<hal_realtek>/zephyr/blobs/ameba/amebasmart/lib``)::

   lib/                      # CA32 host libraries (shipped by nuwa_lib)
   lib/km4/lib_wifi_whc_np.a # KM4 NP driver  }  from the matching ameba-rtos
   lib/km4/lib_coex.a        # KM4 coex       }  release -- the WHC IPC shared
   lib/km0/lib_wifi_fw.a     # KM0 MAC fw     }  structs are not ABI-stable
                             #                   across SDK generations!

The sample carries no twister configuration until those libraries are part of
the blob distribution.

Building and Running
********************

.. code-block:: console

   west build -b rtl8730e_evb/rtl8730e/ca32 --sysbuild \
        zephyr/samples/boards/realtek/rtl8730e/wifi_tricore
   west flash --port <serial>

Then, on the shell::

   uart:~$ wifi connect -s "<ssid>" -k 1 -p "<password>"
   uart:~$ net ping <gateway>
