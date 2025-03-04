.. _sip_porting_guide:

SiP Porting Guide
###################

This page describes how to add support for a new :term:`SiP` in Zephyr, be it in
the upstream Zephyr project or locally in your own repository.

SiP Definitions
***************

It is expected that you are familiar with the board concept in Zephyr.
A high level overview of the hardware support hierarchy and terms used in the
Zephyr documentation can be seen in :ref:`hw_support_hierarchy`.

For SiP porting, the most important terms are:

- SoC: A system on chip containing cpu clusters.
- SiP: The system in package the SoCs and integrated components are part of.
- Board: The board the SiPs and external components are part of.

Create your SiP directory
*************************

Each SiP must have a unique name. Use the official name given by the SiP vendor
and check that it's not already in use. In some cases someone else may have
contributed a SiP with identical name. If the SoC name is already in use, then
you should probably improve the existing SiP instead of creating a new one.
The script ``list_hardware`` can be used to retrieve a list of all SiPs known
in Zephyr, for example ``./scripts/list_hardware.py --sip-root=. --sips`` from
the Zephyr base directory for a list of names that are already in use.

Start by creating the directory ``zephyr/sip/<VENDOR>/<SIP>``, where
``<VENDOR>`` is your vendor subdirectory. and ``<SIP>`` is the name of the SiP.

Your SiP directory should look like this:

.. code-block:: none

   sip/<VENDOR>/<SIP>
   ├── sip.yml
   ├── Kconfig.sip

The files are:

#. :file:`sip.yml`: A YAML file describing the high-level meta data of the
   SiP such as:

   - SiP name: The name of the SiP
   - SoCs: The SoCs contained in the SiP

#. :file:`Kconfig.sip`: The base SiP configuration which defines a Kconfig SiP
   symbol in the form of ``config SIP_<SIP>_<SOC>`` and provides the SiP name to
   the Kconfig ``SIP`` setting.

Write your SiP YAML
*******************

The SiP YAML file describes the SiP and its SoCs at a high level.

Detailed configurations, such as hardware description and configuration are done
in devicetree and Kconfig.

The skeleton of a SiP YAML file is:

.. code-block:: yaml

   sips:
     - name: <SIP>
       socs:
         - <SOC>

The ``<SIP>`` is the unique SiP name, The ``<SOC>`` in the ``socs`` list are
existing SoCs defined a ``soc.yml``, see :ref:`_soc_porting_guide`.

Write your SiP devicetree
*************************

SiP devicetree include files are located in the :file:`<ZEPHYR_BASE>/dts` folder
under a corresponding :file:`sip/<VENDOR>`.

The SiP file name contains the ``<SIP>``, followed by the ``<SOC>``, and
optionally, the ``<CPUCLUSTER>`` separated by ``_``.

The file :file:`dts/sip/<VENDOR>/<SIP>_<SOC>_<CPUCLUSTER>.dtsi` describes your SiP
hardware in the Devicetree Source (DTS) format and must be included by any boards
which use the SiP.

If a highlevel :file:`<SOC>_<CPUCLUSTER>.dtsi` file exists, then a good starting
point is to include this file in your :file:`<VENDOR>/<SIP>_<SOC>_<CPUCLUSTER>.dtsi`.

In general, :file:`<VENDOR>/<SIP>_<SOC>_<CPUCLUSTER>.dtsi` should look like this:

.. code-block:: devicetree

   /* The SoC CPU cluster */
   #include <<ARCH>/<VENDOR>/<SOC>/<CPUCLUSTER>.dtsi>

   /* Integrated hardware connected to the SoC CPU cluster */
   &spi0 {
           sensor@0 {

           };

           flash@1 {

           };

           pmic@2 {

           };
   };

Write Kconfig files
*******************

Zephyr uses the Kconfig language to configure software features. Your SiP
needs to provide some Kconfig settings before you can compile a Zephyr
application for it.

Setting Kconfig configuration values is documented in detail in
:ref:`setting_configuration_values`.

The :file:`Kconfig.sip` file defines Kconfig settings for the SiP which
will be selected by the board, The SiP Kconfig settings select the SoC:

.. code-block:: none

   board -> selects -> sip -> selects -> soc -> selects -> arch

The following is defined for each SiP, SoC, CPU cluster and package
combination:

.. code-block:: kconfig

   config SIP_<SIP>_<SOC>_<CPUCLUSTER>_<SIP PACKAGE>
           bool
           select SOC_<SOC>_<CPUCLUSTER>_<SOC PACKAGE>

The ``<CPUCLUSTER>``, ``<SIP PACKAGE>`` and ``<SOC PACKAGE>`` are optional.
