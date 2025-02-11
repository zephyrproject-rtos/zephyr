:orphan:

.. start_include_here

Prepare Linux host
------------------

#. Install DHCP, TFTP servers. For example ``dnsmasq``

   .. code-block:: console

      $ sudo apt-get install dnsmasq

#. Configure DHCP server. Configuration for ``dnsmasq`` is below:

   .. code-block:: console

      # Only listen to this interface
      interface=eno2
      dhcp-range=10.1.1.20,10.1.1.30,12h

#. Configure TFTP server.

   .. code-block:: console

      # tftp
      enable-tftp
      tftp-root=/srv/tftp
      dhcp-boot=zephyr.efi

   ``zephyr.efi`` is a Zephyr EFI binary created above.

#. Copy the Zephyr EFI image :file:`zephyr/zephyr.efi` to the
   :file:`/srv/tftp` folder.

    .. code-block:: console

       $ cp zephyr/zephyr.efi /srv/tftp


#. TFTP root should be looking like:

   .. code-block:: console

      $ tree /srv/tftp
      /srv/tftp
      └── zephyr.efi

#. Restart ``dnsmasq`` service:

   .. code-block:: console

      $ sudo systemctl restart dnsmasq.service

Prepare the board for network boot
----------------------------------

#. Enable PXE network from BIOS settings.

#. Make network boot as the first boot option.

Booting the board
-----------------

#. Connect the board to the host system using the serial cable and
   configure your host system to watch for serial data. See board's
   website for more information.

   .. note::
      Use a baud rate of 115200.

#. Power on the board.

#. Verify that the board got an IP address. Run from the Linux host:

   .. code-block:: console

      $ journalctl -f -u dnsmasq
      dnsmasq-dhcp[5386]: DHCPDISCOVER(eno2) 00:07:32:52:25:88
      dnsmasq-dhcp[5386]: DHCPOFFER(eno2) 10.1.1.28 00:07:32:52:25:88
      dnsmasq-dhcp[5386]: DHCPREQUEST(eno2) 10.1.1.28 00:07:32:52:25:88
      dnsmasq-dhcp[5386]: DHCPACK(eno2) 10.1.1.28 00:07:32:52:25:88

#. Verify that network booting is started:

   .. code-block:: console

      $ journalctl -f -u dnsmasq
      dnsmasq-tftp[5386]: sent /srv/tftp/zephyr.efi to 10.1.1.28

#. When the boot process completes, you have finished booting the
   Zephyr application image.
