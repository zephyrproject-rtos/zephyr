.. zephyr:code-sample:: tflite-fft-event-classifier
   :name: FFT event classifier

   Run a TensorFlow Lite Micro FFT event classification sample.

Overview
********

This sample demonstrates a small embedded AI workflow using a DSP frontend and
TensorFlow Lite Micro inference.

The sample pipeline is:

* fixed synthetic sensor windows
* DC removal
* Hann windowing
* CMSIS-DSP real FFT
* spectral feature extraction
* quantized int8 TensorFlow Lite Micro inference
* optional CMSIS-NN optimized kernels
* cycle-level timing
* console PASS/FAIL validation

The included model classifies three fixed input classes:

* normal
* high_frequency
* transient

Building and Running
********************

The reference kernel application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/fft_event_classifier
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

The CMSIS-NN kernel application can be built for EK-RA8M1:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/fft_event_classifier
   :host-os: unix
   :board: ek_ra8m1
   :gen-args: -T sample.tensorflow.fft_event_classifier.cmsis_nn
   :goals: build
   :compact:

Sample Output
=============

EK-RA8M1 output using the CMSIS-NN configuration:

.. code-block:: console

    input_window: normal

    features: rms=707.141724, peak=1000.000000, zcr=0.047619, low=0.999997, mid=0.000003, high=0.000000, centroid=0.063203

    preprocess_cycles: 43200, preprocess_time_us: 90

    invoke_cycles: 14524, invoke_time_us: 30

    total_cycles: 57724, total_time_us: 120

    prediction: normal, score: 6.597734

    result: PASS

    input_window: high_frequency

    features: rms=707.141663, peak=1000.000000, zcr=0.301587, low=0.000000, mid=0.999995, high=0.000004, centroid=0.312849

    preprocess_cycles: 42576, preprocess_time_us: 88

    invoke_cycles: 14352, invoke_time_us: 29

    total_cycles: 56928, total_time_us: 117

    prediction: high_frequency, score: 9.389083

    result: PASS

    input_window: transient

    features: rms=764.939636, peak=2500.000000, zcr=0.047619, low=0.803148, mid=0.056402, high=0.140449, centroid=0.343116

    preprocess_cycles: 42194, preprocess_time_us: 87

    invoke_cycles: 14142, invoke_time_us: 29

    total_cycles: 56336, total_time_us: 116

    prediction: transient, score: 6.597734

    result: PASS

Model Generation
****************

The model is generated from synthetic FFT event windows using:

.. code-block:: console

    python samples/modules/tflite-micro/fft_event_classifier/train/generate_model.py

The script trains a small dense model, quantizes it to int8, and writes the
generated model into :file:`src/model.cpp`.
