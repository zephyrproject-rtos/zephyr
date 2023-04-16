.. _adc_emul_api:


ADC Emulator
###################################

Overview
********
The Analog-to-Digital Converter (ADC) emulator provides the ability to
programmatically set the value reported by the ADC driver to assist in
testing without access to the real hardware.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_ADC`
* :kconfig:option:`CONFIG_ADC_EMUL`
* :kconfig:option:`CONFIG_ADC_EMUL_ACQUISITION_THREAD_PRIO`
* :kconfig:option:`CONFIG_ADC_EMUL_ACQUISITION_THREAD_STACK_SIZE`

API Reference
*************

.. doxygengroup:: adc_emul_interface
