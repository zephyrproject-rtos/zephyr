.. zephyr:code-sample:: bluetooth_handsfree_ag
   :name: Hands-free Audio Gateway (AG)
   :relevant-api: bt_hfp_ag bluetooth

   Use the Hands-Free Profile Audio Gateway (AG) APIs.

Overview
********

Application demonstrating usage of the Hands-free Audio Gateway (AG) APIs.

Requirements
************

* Running on the host with Bluetooth BR/EDR (Classic) support, or
* A board with Bluetooth BR/EDR (Classic) support

Building and Running
********************

See :zephyr:code-sample-category:`bluetooth` samples for details.

Running
*******

The application works as a Hands-Free Audio Gateway. After the Bluetooth Host stack is initialized,
the GAP discovery procedure will be started automatically. The target device is Hands-Free Unit
(The major of COD is 0x04 (:c:macro:`BT_COD_MAJOR_AUDIO_VIDEO`), the minor of the COD is 0x02
(:c:macro:`BT_COD_MAJOR_AUDIO_VIDEO_MINOR_HANDS_FREE`)). When the target device is discovered,
the AG will connect to the device.

After the ACL connection is established, the AG will initiate the Service Discovery Protocol (SDP)
to discover the Hands-Free Unit's supported features. Once the HFP connection is established, the
AG will be ready to handle incoming calls and audio streaming requests from the connected HFP unit.

The application will remain in a standby state, waiting for the outgoing call request from the
Hands-Free Unit. If there are no incoming or outgoing calls within the duration specified by
:kconfig:option:`CONFIG_BT_HFP_AG_START_CALL_DELAY_TIME`, the AG will start a simulation call. The
direction of the call is determined by :kconfig:option:`CONFIG_BT_HFP_AG_CALL_OUTGOING`.

Once a call is initiated, the AG will establish an SCO (Synchronous Connection-Oriented) link for
audio transmission.

When the SCO connect is established, the application will initialize the codec and pcm interface
for voice streaming if the codec and pcm configurations are available.

The HFP application requires the following optional configuration options:
The codec depends on the devicetree alias named ``i2s-codec-rx`` and ``i2s-codec-tx``.
The PCM interface depends on the devicetree alias named ``pcm-rxtx``, or ``pcm-tx`` and ``pcm-rx``.

After the call is active, the AG will start a delay-able worker with the 10 seconds timeout to
disconnect the ACL connection directly.

This sample has been tested on :zephyr:board:`mimxrt1170_evk@B/mimxrt1176/cm7 <mimxrt1170_evk>`.


.. graphviz::
   :caption: Bluetooth Hands-Free Audio Gateway voice streaming topology


   digraph bluetooth_hfp_ag {
       rankdir=LR;
       node [shape=box, style=rounded];
       edge [fontname=Courier, fontsize=9];
       init [shape=point];

       subgraph cluster_hf {
           label="Hands-Free Unit";
           style=filled;
           HF [label="HFP HF"];
       }

       subgraph cluster_ag {
           label="Zephyr Hands-Free AG";
           style=filled;

           BT_HOST [label="Host Stack"];
           BT_CTRL [label="Controller"];
           HFP_AG_APP [label="HFP AG Application"];
           CODEC [label="Audio Subsystem"];
           SPK [label="Speaker"];
           MIC [label="Microphone"];
       }

       HF -> BT_CTRL [label="SCO Link\n(BR/EDR)"];
       BT_CTRL -> HF [label="SCO Link\n(BR/EDR)"];
       BT_HOST -> BT_CTRL [label="HCI"];
       BT_CTRL -> BT_HOST [label="HCI"];
       BT_HOST -> HFP_AG_APP [label="AG Callbacks"];
       HFP_AG_APP -> BT_HOST [label="AG APIs"];
       HFP_AG_APP -> CODEC [label="Peer voice"];
       CODEC -> HFP_AG_APP [label="Local voice"];
       HFP_AG_APP -> BT_CTRL [label="Local voice\nPCM output"];
       BT_CTRL -> HFP_AG_APP [label="Peer voice\nPCM input"];
       CODEC -> SPK [label="Audio output"]
       MIC -> CODEC [label="Audio input"]
   }
