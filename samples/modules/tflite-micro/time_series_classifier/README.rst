.. zephyr:code-sample:: tflite-time-series-classifier
   :name: Time-series classifier

   Run a TensorFlow Lite Micro time-series classification sample.

Overview
********

This sample demonstrates a TensorFlow Lite Micro inference pipeline for
time-series classification.

The sample is intended to show a small embedded AI workflow:

* fixed time-series input data
* feature extraction / preprocessing
* TensorFlow Lite Micro inference
* optional CMSIS-NN optimized kernels on Arm Cortex-M targets
* cycle-level inference timing
* console output validation

The included model classifies three fixed input windows:

* normal
* low_frequency
* transient

Building and Running
********************

Add the tflite-micro module to your West manifest and pull it:

.. code-block:: console

    west config manifest.project-filter -- +tflite-micro
    west update

The reference kernel application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/time_series_classifier
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

The CMSIS-NN kernel application can be built for EK-RA8M1:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/time_series_classifier
   :host-os: unix
   :board: ek_ra8m1
   :gen-args: -T sample.tensorflow.time_series_classifier.cmsis_nn
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    input_window: normal

    features: mean=0.000000, rms=707.141724, peak=1000.000000, zcr=0.047619

    invoke_cycles: 13104, invoke_time_us: 27

    prediction: normal, score: 5.258944

    result: PASS

    input_window: low_frequency

    features: mean=0.000000, rms=707.072327, peak=1000.000000, zcr=0.015873

    invoke_cycles: 12774, invoke_time_us: 26

    prediction: low_frequency, score: 10.517887

    result: PASS

    input_window: transient

    features: mean=6.250000, rms=866.718567, peak=3200.000000, zcr=0.015873

    invoke_cycles: 12630, invoke_time_us: 26

    prediction: transient, score: 10.682229

    result: PASS

Model Generation
****************

The model is generated from synthetic time-series windows using the script in
:file:`train/generate_model.py`.

The script trains a small dense model, quantizes it to int8, and writes the
generated model into :file:`src/model.cpp`.

.. code-block:: console

    python samples/modules/tflite-micro/time_series_classifier/train/generate_model.py
