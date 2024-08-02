TF-M Requirements
#################

The following are some of the boards that can be used with TF-M:

.. list-table::
   :header-rows: 1

   * - Board
     - NSPE board name
   * - :ref:`mps2_an521_board`
     - ``mps2_an521_ns`` (qemu supported)
   * - :ref:`mps3_an547_board`
     - ``mps3_an547_ns`` (qemu supported)
   * - :ref:`bl5340_dvk`
     - ``bl5340_dvk/nrf5340/cpuapp/ns``
   * - :ref:`lpcxpresso55s69`
     - ``lpcxpresso55s69_ns``
   * - :ref:`nrf9160dk_nrf9160`
     - ``nrf9160dk/nrf9160/ns``
   * - :ref:`nrf5340dk_nrf5340`
     - ``nrf5340dk/nrf5340/cpuapp/ns``
   * - :ref:`b_u585i_iot02a_board`
     - ``b_u585i_iot02a/stm32u585xx/ns``
   * - :ref:`nucleo_l552ze_q_board`
     - ``nucleo_l552ze_q/stm32l552xx/ns``
   * - :ref:`stm32l562e_dk_board`
     - ``stm32l562e_dk/stm32l562xx/ns``
   * - :ref:`v2m_musca_b1_board`
     - ``v2m_musca_b1_ns``
   * - :ref:`v2m_musca_s1_board`
     - ``v2m_musca_s1_ns``

You can run ``west boards -n _ns$`` to search for non-secure variants
of different board targets. To make sure TF-M is supported for a board
in its output, check that :kconfig:option:`CONFIG_TRUSTED_EXECUTION_NONSECURE`
is set to ``y`` in that board's default configuration.

Software Requirements
*********************

The following Python modules are required when building TF-M binaries:

* cryptography
* pyasn1
* pyyaml
* cbor>=1.0.0
* imgtool>=1.9.0
* jinja2
* click

You can install them via:

   .. code-block:: bash

      $ pip3 install --user cryptography pyasn1 pyyaml cbor>=1.0.0 imgtool>=1.9.0 jinja2 click

They are used by TF-M's signing utility to prepare firmware images for
validation by the bootloader.

Part of the process of generating binaries for QEMU and merging signed
secure and non-secure binaries on certain platforms also requires the use of
the ``srec_cat`` utility.

This can be installed on Linux via:

   .. code-block:: bash

      $ sudo apt-get install srecord

And on OS X via:

   .. code-block:: bash

      $ brew install srecord

For Windows-based systems, please make sure you have a copy of the utility
available on your system path. See, for example:
`SRecord for Windows <https://sourceforge.net/projects/srecord/files/srecord-win32>`_
