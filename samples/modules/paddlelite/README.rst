.. _paddlelite:

Paddle Lite CNN sample
######################

Overview
********

This example demonstrates the Paddle Lite application's inference on a CNN model.
The Paddle Lite version is Paddle Lite2.6. The models are trained on Baidu’s official website and cropped using the opt tool of Baidu Paddle Lite2.6.
Due to the lack of the opencv library, a tensor whose values are all one is used to simulate an image, and the output is the output shape and corresponding probability.


Building and Running
********************

This application can also be built and executed on roc_rk3568_pc as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/paddlelite
   :host-os: unix
   :board: roc_rk3568_pc
   :goals: run
   :compact:


After completing the above process, you need to execute the following command to run:

setenv serverip 192.168.0.102
setenv ipaddr 192.168.0.103
tftp 0x40000000 zephyr.bin;
tftp 0x70000000 resnet18.nb;
dcache flush; icache flush; dcache off; icache off; go 0x40000000;

The above means selecting resnet18. If you want to select mobilenet_v1, execute tftp 0x70000000 mobilenet_v1.nb.

The trained model can be downloaded directly from the Paddle Lite official website.Currently supported models include: SqueezeNet, ShuffleNetV2, MnasNet, MobileNetV2, MobileNetV1, InceptionV4, ResNet18, ResNet50.
(https://www.bookstack.cn/read/paddle-lite-2.6-zh/cd5a7bd0383910a4.md)

The .nb file is a trimmed and quantized binary file of Paddle Lite's model file.After downloading the corresponding model file, you need to use the opt tool to trim and quantify the model.
(https://www.bookstack.cn/read/paddle-lite-2.6-zh/74ac56cc6daa2455.md)

Sample Output
=============

Take the output of mobilenet_v1 as an example. It is trained for imagenet, so the output shape is 1000, and 10 of the probabilities are selected as output.

.. code-block:: console

    Output shape 1000
    Output[0]: 0.000191311
    Output[100]: 0.000159713
    Output[200]: 0.000264313
    Output[300]: 0.000210793
    Output[400]: 0.00103236
    Output[500]: 0.000110071
    Output[600]: 0.00482922
    Output[700]: 0.00184533
    Output[800]: 0.000202115
    Output[900]: 0.000585589


