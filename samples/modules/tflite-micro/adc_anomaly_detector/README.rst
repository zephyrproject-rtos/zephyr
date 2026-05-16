.. zephyr:code-sample:: tflm-adc-anomaly-detector
   :name: ADC anomaly detector

   Run a TensorFlow Lite Micro anomaly detector sample with synthetic and ADC
   sample-source support.

Overview
********

This sample demonstrates an embedded AI signal chain using TensorFlow Lite
Micro.

The default path uses a deterministic synthetic source for QEMU and CI. The
sample also includes an optional ADC source backend that can be enabled for
hardware demonstrations.

The sample pipeline is:

* sample source, synthetic by default or ADC when enabled
* fixed-size ring buffer
* sliding-window extraction
* time-domain feature extraction
* quantized int8 TensorFlow Lite Micro inference
* optional CMSIS-NN optimized kernels
* prediction history / stable output
* cycle-level inference timing
* console PASS/FAIL validation for the synthetic path

The generated model classifies three signal classes:

* normal
* drift
* transient

Building and Running
********************

Build and run the deterministic synthetic source on QEMU:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/adc_anomaly_detector
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Build the CMSIS-NN configuration for EK-RA8M1:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/adc_anomaly_detector
   :host-os: unix
   :board: ek_ra8m1
   :gen-args: -T sample.tensorflow.adc_anomaly_detector.cmsis_nn
   :goals: build
   :compact:

ADC Source
**********

The ADC source can be enabled with:

.. code-block:: console

   -DCONFIG_ADC=y -DCONFIG_TFLM_ADC_ANOMALY_DETECTOR_ADC_SOURCE=y

The ADC source expects an ``io-channels`` entry under ``zephyr,user``. For
example:

.. code-block:: devicetree

   / {
       zephyr,user {
           io-channels = <&adc0 0>;
       };
   };

The default CI path does not require ADC hardware.

Model Generation
****************

The model and deterministic synthetic stream data are generated using:

.. code-block:: console

   python samples/modules/tflite-micro/adc_anomaly_detector/train/generate_model.py

The script trains a small dense model, quantizes it to int8, writes the model to
:file:`src/model.cpp`, and writes the deterministic synthetic source data to
:file:`src/synthetic_source.c`.
