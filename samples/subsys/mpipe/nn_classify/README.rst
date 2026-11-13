.. zephyr:code-sample:: mpipe-nn-classify
   :name: Live classification pipeline
   :relevant-api: mpipe_nn mpipe_vid mpipe_disp mpipe_base

   Show the camera stream while a neural network classifies it.

Overview
********

A tee splits the camera stream into two branches: one shows the live video
on the display, the other feeds a neural network classifier. The inference branch
sits behind a single-slot queue that drops the oldest frame, so inference
always works on the freshest frame and the display never waits for the model:

.. graphviz::

   digraph pipeline {
     rankdir=LR;
     node [shape=box, style=filled, fillcolor="#e8e8e8"];
     camera    [label="Camera\nSource"];
     caps      [label="Caps\nFilter"];
     tee       [label="Tee"];
     queue_d   [label="Queue"];
     transform [label="Video\nTransform"];
     display   [label="Display\nSink"];
     queue_nn  [label="Queue\n(leaky)"];
     convert   [label="NN\nConverter"];
     runner    [label="NN\nRunner"];
     decode    [label="NN\nDecoder"];
     app_sink   [label="Application\nSink"];
     camera -> caps -> tee;
     tee -> queue_d -> transform -> display;
     tee -> queue_nn -> convert -> runner -> decode -> app_sink;
   }

The tee negotiates one video capability for both branches because the
converter absorbs the geometry and format change on the inference side: on the
i.MX RT1170 the camera streams 1280x720 frames, the display branch rotates
them through the PXP, and the inference branch converts them straight to the 96x96
grayscale tensor in software. The converter shifts each pixel by the model's
zero point (``CONFIG_SAMPLE_NN_INPUT_SHIFT``, -128 for the person detection
model, which was trained on pixels scaled to -1..1); disable it for a model
that takes raw bytes.

With the TFLM backend (``overlay-tflm.conf``) the model is the stock person
detection network from the tflite-micro module; the classification of each
analyzed frame is logged on the console while the video runs on the display.
The fake backend (default) exercises the identical pipeline without any
module.

Requirements
************

The TFLM configuration needs the optional tflite-micro module:

.. code-block:: console

   west config manifest.group-filter -- +optional
   west update tflite-micro

Building and Running
********************

Fake backend on :zephyr:board:`native_sim` (synthetic video, SDL display):

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mpipe/nn_classify
   :board: native_sim/native/64
   :snippets: video-sw-generator
   :goals: build run
   :compact:

Person detection on :zephyr:board:`mimxrt1170_evk` with the OV5640 camera and
the RK055HDMIPI4MA0 display:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mpipe/nn_classify
   :board: mimxrt1170_evk/mimxrt1176/cm7
   :shield: nxp_btb44_ov5640,rk055hdmipi4ma0
   :gen-args: -DEXTRA_CONF_FILE=overlay-tflm.conf
   :goals: build flash
   :compact:

Sample Output
*************

Point the camera at a person and watch the score flip:

.. code-block:: console

   [00:00:00.380,000] <inf> mpipe_player: Player #0 state: PLAYING
   [00:00:01.102,000] <inf> main: Frame 12: no person (score 71%)
   [00:00:01.170,000] <inf> main: Frame 13: person (score 83%)
