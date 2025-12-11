:orphan:

.. start_include_here

Preparing the Boot Device
-------------------------

Prepare a USB flash drive to boot the Zephyr application image on
a board.

#. Format the USB flash drive as FAT32.

   On Windows, open ``File Explorer``, and right-click on the USB flash drive.
   Select ``Format...``. Make sure in ``File System``, ``FAT32`` is selected.
   Click on the ``Format`` button and wait for it to finish.

   On Linux, graphical utilities such as ``gparted`` can be used to format
   the USB flash drive as FAT32. Alternatively, under terminal, find out
   the corresponding device node for the USB flash drive (for example,
   ``/dev/sdd``). Execute the following command:

   .. code-block:: console

      $ mkfs.vfat -F 32 <device-node>

   .. important::
      Make sure the device node is the actual device node for
      the USB flash drive. Or else you may erase other storage devices
      on your system, and will render the system unusable afterwards.

#. Copy the Zephyr EFI image file :file:`zephyr/zephyr.efi` to the USB drive.

Booting Zephyr on a board
-------------------------

Boot the board to the EFI shell with USB flash drive connected.

#. Insert the prepared boot device (USB flash drive) into the board.

#. Connect the board to the host system using the serial cable and
   configure your host system to watch for serial data. See board's
   website for more information.

   .. note::
      Use a baud rate of 115200.

#. Power on the board.

#. When the following output appears, press :kbd:`F7`:

   .. code-block:: console

      Press <DEL> or <ESC> to enter setup.

#. From the menu that appears, select the menu entry that describes
   that particular EFI shell.

#. From the EFI shell select Zephyr EFI image to boot.

   .. code-block:: console

      Shell> fs0:zephyr.efi

#. When the boot process completes, you have finished booting the
   Zephyr application image.
