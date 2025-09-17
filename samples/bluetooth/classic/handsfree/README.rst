.. zephyr:code-sample:: bluetooth_handsfree
   :name: Hands-free
   :relevant-api: bt_hfp bluetooth

   Use the Hands-Free Profile (HFP) APIs.

Overview
********

Application demonstrating usage of the Hands-free Profile (HFP) APIs.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth BR/EDR (Classic) support

Building
********

See :zephyr:code-sample-category:`bluetooth` samples for details.

Running
*******

The application works a Hands-Free uint function. After the Bluetooth Host stack initialized, the
connectable and discoverable will be automatically enabled. The peer device AG (Audio Gateway) can
discover and connect to the device.

When the SCO connect established, the application will initialize the codec and pcm interface for
voice streaming if the codec and pcm configurations are available.

The HFP application requires the following optional configuration options:
The codec depends on the devicetree alias named ``i2s-codec-rx`` and ``i2s-codec-tx``.
The PCM interface depends on the devicetree alias named ``pcm-rxtx``, or ``pcm-tx`` and ``pcm-rx``.

This sample has been tested on :zephyr:board:`mimxrt1170_evk@B/mimxrt1176/cm7 <mimxrt1170_evk>`.


.. graphviz::
   :caption: Bluetooth Hands-Free voice streaming topology


   digraph bluetooth_hfp {
       rankdir=LR;
       node [shape=box, style=rounded];
       edge [fontname=Courier, fontsize=9];
       init [shape=point];

       subgraph cluster_ag {
           label="Audio Gateway (Phone/Car)";
           style=filled;
           AG [label="HFP AG"];
       }

       subgraph cluster_hf {
           label="Zephyr HF Device";
           style=filled;

           BT_HOST [label="Host Stack"];
           BT_CTRL [label="Controller"];
           HFP_APP [label="HFP HF Application"];
           CODEC [label="Audio Subsystem"];
           SPK [label="Speaker"];
           MIC [label="Microphone"];
       }

       AG -> BT_CTRL [label="SCO Link\n(BR/EDR)"];
       BT_CTRL -> AG [label="SCO Link\n(BR/EDR)"];
       BT_HOST -> BT_CTRL [label="HCI"];
       BT_CTRL -> BT_HOST [label="HCI"];
       BT_HOST -> HFP_APP [label="HF Callbacks"];
       HFP_APP -> BT_HOST [label="HF APIs"];
       HFP_APP -> CODEC [label="Peer voice"];
       CODEC -> HFP_APP [label="Local voice"];
       HFP_APP -> BT_CTRL [label="Local voice\nPCM output"];
       BT_CTRL -> HFP_APP [label="Peer voice\nPCM input"];
       CODEC -> SPK [label="Audio output"]
       MIC -> CODEC [label="Audio input"]
   }
