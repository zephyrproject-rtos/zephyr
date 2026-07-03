.. zephyr:code-sample:: tflm-streaming-anomaly-detector
   :name: Streaming anomaly detector

   Run a TensorFlow Lite Micro streaming anomaly detector sample.

Overview
********

This sample demonstrates a small streaming embedded AI workflow using
TensorFlow Lite Micro.

The sample pipeline is:

* deterministic synthetic input stream
* fixed-size ring buffer
* sliding-window extraction
* time-domain feature extraction
* quantized int8 TensorFlow Lite Micro inference
* optional CMSIS-NN optimized kernels
* prediction history / stable output
* cycle-level inference timing
* console PASS/FAIL validation

The included model classifies three input stream classes:

* normal
* drift
* transient

The sample uses two overlapping windows per class. Each scenario is reset before
its first window, then advanced by one hop to demonstrate streaming window
updates through the ring buffer.

Building and Running
********************

The reference kernel application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/streaming_anomaly_detector
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

The CMSIS-NN kernel application can be built for EK-RA8M1:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/streaming_anomaly_detector
   :host-os: unix
   :board: ek_ra8m1
   :gen-args: -T sample.tensorflow.streaming_anomaly_detector.cmsis_nn
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    stream_window: 0
    window_start: 0
    features: mean=..., rms=..., peak=..., zcr=..., slope=..., energy=...
    invoke_cycles: ..., invoke_time_us: ...
    raw_prediction: normal, score: ...
    stable_prediction: normal
    result: PASS

Model Generation
****************

The model and deterministic input stream are generated using:

.. code-block:: console

    python samples/modules/tflite-micro/streaming_anomaly_detector/train/generate_model.py

The script trains a small dense model, quantizes it to int8, writes the model to
:file:`src/model.cpp`, and writes deterministic stream data to
:file:`src/input_stream.c`.
