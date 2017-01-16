.. _networking_with_qemu:

Networking with Qemu
####################

Follow the instructions described in `QEMU Network Setup`_ to prepare your
environment for testing networking using Qemu.

Follow the prerequisites described in

.. code-block:: console

	git clone https://gerrit.zephyrproject.org/r/net-tools
	cd net-tools
	make

From a terminal window, type:

.. code-block:: console

	./loop-socat.sh

Open another terminal window, change directory to the one where net-tools
are

.. code-block:: console

	sudo ./loop-slip-tap.sh


.. _`QEMU Network Setup`: https://wiki.zephyrproject.org/view/Networking-with-Qemu
