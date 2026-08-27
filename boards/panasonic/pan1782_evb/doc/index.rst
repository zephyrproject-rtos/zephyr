.. zephyr:board:: pan1782_evb

Overview
********

The PAN1782 Evaluation Board is a development tool for the PAN1782 module
which is based on the nRF52833 chipset from Nordic Semiconductor.

You can find more information about the PAN1782 module and the PAN1782
evaluation board on the `product website`_.

Please also refer to :zephyr:board:`nrf52833dk` for general information about
development kits for the nRF52833 from Nordic Semiconductor.

The PAN1782 evaluation board is closely linked to these other evaluation
boards:

* :zephyr:board:`pan1780_evb`
* :zephyr:board:`pan1781_evb`

Usage
*****

You can find the `user guide`_ for the PAN1782 Evaluation Board in the
`Panasonic Wireless Connectivity Development Hub`_.

The user guide contains (amongst other things) detailed information about

* pin mapping
* powering options
* breakout pin header interface
* current consumption measurement
* software development

and other things.

The schematics for the PAN1782 Evaluation Board are available in the
`download section`_ of the `Panasonic Wireless Connectivity Development Hub`_.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Please use the ``pan1782_evb`` board configuration when
:ref:`build_an_application` and :ref:`application_run`.

.. target-notes::

.. _product website: https://industry.panasonic.eu/products/devices/wireless-connectivity/bluetooth-low-energy-modules/pan1782-nrf52833
.. _Panasonic Wireless Connectivity Development Hub: https://pideu.panasonic.de/development-hub/
.. _user guide: https://pideu.panasonic.de/development-hub/pan1782/evaluation_board/user_guide/
.. _download section: https://pideu.panasonic.de/development-hub/pan1782/downloads/
