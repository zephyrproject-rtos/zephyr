.. zephyr:code-sample:: mpipe-nn-detect
   :name: Live detection pipeline
   :relevant-api: mpipe_nn mpipe_vid_overlay mpipe_vid mpipe_disp mpipe_base

   Draw detection boxes over the live camera stream.

Overview
********

A tee splits the camera stream: the display branch draws the most recent
detection boxes into every frame through the overlay element, then rotates it
for the panel where the board has a hardware transform; the inference branch
runs the detector on the frame as the sensor delivers it, so boxes and
detector share one orientation whatever the panel's. The app_sink callback
publishes each result set into a latest-results store the overlay's draw
callback reads, one or two frames late:

.. graphviz::

   digraph pipeline {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8"];
     camera    [label="Camera\nSource"];
     caps      [label="Caps\nFilter"];
     tee       [label="Tee"];
     queue_d   [label="Queue"];
     overlay   [label="Video\nOverlay"];
     transform [label="Video\nTransform"];
     display   [label="Display\nSink"];
     queue_nn  [label="Queue\n(leaky)"];
     convert   [label="NN\nConverter"];
     queue_t   [label="Queue\n(leaky)"];
     runner    [label="NN\nRunner"];
     decode    [label="NN\nDecoder"];
     app_sink   [label="Application\nSink"];
     camera -> caps -> tee;
     tee -> queue_d -> overlay -> transform -> display;
     tee -> queue_nn -> convert -> queue_t -> runner -> decode -> app_sink;
     app_sink -> overlay [style=dashed, label="results store"];
   }

With the fake backend (default) a deterministic box orbits the frame: the
whole pipeline, the detection decoding and the overlay run with no model at
all. For a real detector, build with the TFLM overlay and point ``NN_MODEL``
at a quantized detector; no model is shipped. The decoder understands two
output layouts: the four-tensor TFLite_Detection_PostProcess convention
(boxes, classes, scores, count) of SSD detectors, and the scored box list
(per-anchor class scores followed by a normalized box, with non-maximum
suppression in the decoder) of the Ultraface face detectors. Two of them fit
this sample: the 128x128 model runs at several inferences per second on a
Cortex-M7 but only sees faces filling a good part of the frame; the 320x240
model finds faces of a few pixels and suits the Neutron NPU. The TFLM overlay
is sized for either. The converter shifts each pixel by the model's zero
point (``CONFIG_SAMPLE_NN_INPUT_SHIFT``, -128 by default); disable it for a
model that takes raw bytes. ``CONFIG_SAMPLE_NN_SCORE_THRESHOLD`` sets the
score a box must reach.

The inference branch runs the converter one priority notch above the
streaming threads, so the shared frame is released before the overlay
reaches it, and the model at the lowest application priority on a tensor
queue of its own, so a model slower than the frame rate never delays
capture, display or logging.

Requirements
************

The TFLM configurations need the optional tflite-micro module and a quantized
detector, passed with ``-DNN_MODEL=<path>``. The Ultraface models are in the
``models/ultraface_slim_quant_int8`` directory of the NXP MPP middleware
repository, https://github.com/nxp-mcuxpresso/mcux-sdk-middleware-mpp; the
Neutron configuration also needs the hal_nxp driver blobs:

.. code-block:: console

   west config manifest.group-filter -- +optional
   west update tflite-micro
   west blobs fetch hal_nxp

Building and Running
********************

Fake detector on :zephyr:board:`native_sim` (synthetic video, SDL display;
watch the green box sweep across the test pattern):

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mpipe/nn_detect
   :board: native_sim/native/64
   :snippets: video-sw-generator
   :goals: build run
   :compact:

A real detector on :zephyr:board:`mimxrt1170_evk` with the OV5640 camera and
the RK055HDMIPI4MA0 display:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mpipe/nn_detect
   :board: mimxrt1170_evk/mimxrt1176/cm7
   :shield: nxp_btb44_ov5640,rk055hdmipi4ma0
   :gen-args: -DEXTRA_CONF_FILE=overlay-tflm.conf -DNN_MODEL=/path/to/ultraface.tflite
   :goals: build flash
   :compact:

The 320x240 detector on the eIQ Neutron NPU of :zephyr:board:`mimxrt700_evk`
with the OV7670 camera and the RK055HDMIPI4MA0 display. The camera delivers
the model's own 320x240 RGB565 frames, so nothing is scaled or rotated. The
model must have been compiled by the eIQ Neutron converter matching the
hal_nxp driver blobs; the sample copies it into the SRAM the NPU reaches and
places the arena there. Add ``overlay-neutron.conf`` to the TFLM overlay:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mpipe/nn_detect
   :board: mimxrt700_evk/mimxrt798s/cm33_cpu0
   :shield: dvp_20pin_ov7670,rk055hdmipi4ma0
   :gen-args: -DEXTRA_CONF_FILE="overlay-tflm.conf;overlay-neutron.conf" -DNN_MODEL=npu.tflite
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   [00:00:00.380,000] <inf> mpipe_player: Player #0 state: PLAYING
   [00:00:00.412,000] <inf> main: Frame 1: 2 box(es), best score 90% (class 0)
   [00:00:00.445,000] <inf> main: Frame 2: 2 box(es), best score 90% (class 0)
