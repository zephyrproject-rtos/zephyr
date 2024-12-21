.. zephyr:board:: pan1780_evb

Overview
********

The PAN1780 Evaluation Board is a development tool for the PAN1780 module which
is based on the nRF52840 chipset from Nordic Semiconductor.

It is basically a clone of the official nRF52840 development kit (PCA10056)
from Nordic Semiconductor. Please refer to :ref:`nrf52840dk_nrf52840` for
further information.

You can find more information about the PAN1780 module and the PAN1780
evaluation board on the `product website`_.

The PAN1780 evaluation board is closely linked to these other evaluation
boards:

* :zephyr:board:`pan1781_evb`
* :zephyr:board:`pan1782_evb`

Usage
*****

You can find the `user guide`_ for the PAN1780 Evaluation Board in the
`Panasonic Wireless Connectivity Development Hub`_.

The user guide contains (amongst other things) detailed information about

* pin mapping
* powering options
* breakout pin header interface
* current consumption measurement
* software development

and other things.

The schematics for the PAN1780 Evaluation Board are available in the
`download section`_ of the `Panasonic Wireless Connectivity Development Hub`_.

Programming and Debugging
*************************

Please use the ``pan1780_evb`` board configuration when
:ref:`build_an_application` and :ref:`application_run`.

.. target-notes::
.. _product website: https://industry.panasonic.eu/products/devices/wireless-connectivity/bluetooth-low-energy-modules/pan1780-nrf52840
.. _Panasonic Wireless Connectivity Development Hub: https://pideu.panasonic.de/development-hub/
.. _user guide: https://pideu.panasonic.de/development-hub/pan1780/evaluation_board/user_guide/
.. _download section: https://pideu.panasonic.de/development-hub/pan1780/downloads/
