.. zephyr:code-sample:: tflm-cnn-fault-classifier
   :name: 1D CNN fault classifier

   Run a TensorFlow Lite Micro 1D CNN fault-classification sample.

Overview
********

This sample demonstrates raw-window TinyML fault classification using
TensorFlow Lite Micro.

Unlike the feature-based samples, this sample feeds a raw 64-sample signal
window directly into a small quantized convolutional model. The 1D convolution
is represented as a narrow 2D convolution with an input shape of
``64 x 1 x 1`` so it can use TensorFlow Lite Micro's ``CONV_2D`` operator.

The sample pipeline is:

* deterministic synthetic signal windows
* raw-sample normalization and quantization
* quantized convolutional TensorFlow Lite Micro inference
* optional CMSIS-NN optimized kernels
* cycle-level timing
* console PASS/FAIL validation

The generated model classifies three signal classes:

* normal
* high_frequency_fault
* impulse_fault

Building and Running
********************

The reference kernel application can be built and executed on QEMU:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/cnn_fault_classifier
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

The CMSIS-NN kernel application can be built for EK-RA8M1:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/cnn_fault_classifier
   :host-os: unix
   :board: ek_ra8m1
   :gen-args: -T sample.tensorflow.cnn_fault_classifier.cmsis_nn
   :goals: build
   :compact:

Model Generation
****************

The model and deterministic input windows are generated using:

.. code-block:: console

   python samples/modules/tflite-micro/cnn_fault_classifier/train/generate_model.py

The script trains a small quantized convolutional model, writes the model to
:file:`src/model.cpp`, and writes deterministic input windows to
:file:`src/input_data.c`.
