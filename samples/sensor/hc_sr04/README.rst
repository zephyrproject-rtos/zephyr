.. zephyr:code-sample:: hc_sr04
   :name: HC-SR04

   Read the measured distance from the HC-SR04 ultrasonic module and print it to the console.

Overview
********

This is a sample application that reads the measured distance from an external HC-SR04 module
and prints it (in cm) to the output.

Building and Running
********************

This application can be built and executed on the Blackpill_F411CE as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/hc_sr04
   :host-os: unix
   :board: blackpill_f411ce
   :goals: build flash
   :compact:

To build for another board, add an overlay file and include the module node as follows:

.. code-block:: devicetree

   / {
      hc_sr04: hc_sr04 {
         compatible = "hc-sr04";
         trigger-gpios = <&gpioa 4 GPIO_ACTIVE_HIGH>;
         echo-gpios = <&gpioa 5 GPIO_ACTIVE_HIGH>;
         status = "okay";
      };
   };




Sample Output
=============

.. code-block:: console

    [03:23:40.677,000] <inf> hc_sr04_sample: Distance: 166 cm
    [03:23:40.699,000] <inf> hc_sr04_sample: Distance: 166 cm
    [03:23:40.721,000] <inf> hc_sr04_sample: Distance: 166 cm
    [03:23:40.743,000] <inf> hc_sr04_sample: Distance: 166 cm
