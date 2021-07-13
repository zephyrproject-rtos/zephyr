.. _embarc_mli_example_cifar10_caffe:

CIFAR-10 Convolution Neural Network Example
###########################################

Overview
********
Example is based on standard [Caffe tutorial](http://caffe.berkeleyvision.org/gathered/examples/cifar10.html) for [CIFAR-10](http://www.cs.toronto.edu/~kriz/cifar.html) dataset. It's a simple classifier built on convolution, pooling and dense layers for tiny images.


Quick Start
--------------

Example supports building with [Zephyr Software Development Kit (SDK)](https://docs.zephyrproject.org/latest/getting_started/installation_linux.html#zephyr-sdk) and running with MetaWare Debuger on [nSim simulator](https://www.synopsys.com/dw/ipdir.php?ds=sim_nSIM).

Add embarc_mli module to Zephyr instruction
-------------------------------------------

1. Open command line and change working directory to './zephyrproject/zephyr'

2. Download embarc_mli version 1.1

        west update

Build with Zephyr SDK toolchain
-------------------------------

    Build requirements:
        - Zephyr SDK toolchain version 0.12.3 or higher
        - gmake

1. Open command line and change working directory to './zephyrproject/zephyr/samples/modules/embarc_mli/example_cifar10_caffe'

2. Build example

        west build -b nsim_em samples/modules/embarc_mli/example_cifar10_caffe

Run example
--------------

1. Run example

        west flash

    Result Quality shall be "S/N=4383.8     (72.8 db)"

More options
--------------

You can change mode in ml_api_har_smartphone_main.c to 1,2,3:

* mode=1:

       Built-in input processing. Uses only hard-coded vector for the single input model inference.

* mode=2:

       Unavailable right now due to hostlink error. External test-set processing. Reads vectors from input IDX file, passes it to the model, and writes it's output to the other IDX file (if input is *tests.idx* then output will be *tests.idx_out*).

* mode=3:

       Accuracy measurement for testset. Reads vectors from input IDX file, passes it to the model, and accumulates number of successive classifications according to labels IDX file. If hostlink is unavailble, please add _C_ARRAY_ definition.

You can add different definitions to zephyr_compile_definitions() in 'zephyr/samples/modules/embarc_mli/example_har_smartphone/CMakeLists.txt' to implement numerous model:

* 16 bit depth of coefficients and data (default):

       MODEL_BIT_DEPTH=16

* 8 bit depth of coefficients and data:

       MODEL_BIT_DEPTH=8

* 8x16: 8 bit depth of coefficients and 16 bit depth of data:

       MODEL_BIT_DEPTH=816

* Big neural network model (default is small model):

       MODEL_BIG

* If hostlink is not available, please reads vectors from input Array file, passes it to the model, and accumulates number of successive classifications according to labels array file:

       _C_ARRAY_

Example Structure
--------------------
Structure of example application may be logically divided on three parts:

* **Application.** Implements Input/output data flow and data processing by the other modules. Application includes
   * src/ml_api_cifar10_caffe_main.c
   * ../auxiliary/examples_aux.h(.c)
* **Inference Module.** Uses embARC MLI Library to process input according to pre-defined graph. All model related constants are pre-defined and model coefficients is declared in the separate compile unit
   * src/cifar10_model.h
   * src/cifar10_model_chw.c (cifar10_model_hwc.c)
   * src/cifar10_constants.h
   * src/cifar10_coefficients_chw.c (cifar10_coefficients_hwc.c)
* **Auxiliary code.** Various helper functions for measurements, IDX file IO, etc.
   * ../auxiliary/tensor_transform.h(.c)
   * ../auxiliary/tests_aux.h(.c)
   * ../auxiliary/idx_file.h(.c)

References
----------------------------
CIFAR-10 Dataset:
> Alex Krizhevsky. *"Learning Multiple Layers of Features from Tiny Images."* 2009.

Caffe framework:
> Jia, Yangqing and Shelhamer, Evan and Donahue, Jeff and Karayev, Sergey and Long, Jonathan and Girshick, Ross and Guadarrama, Sergio and Darrell, Trevor. *"Caffe: Convolu-tional Architecture for Fast Feature Embedding."* arXiv preprint arXiv:1408.5093. 2014: http://caffe.berkeleyvision.org/

IDX file format originally was used for [MNIST database](http://yann.lecun.com/exdb/mnist/). There is a python [package](https://pypi.org/project/idx2numpy/) for working with it through transformation to/from numpy array. *auxiliary/idx_file.c(.h)* is used by the test app for working with IDX files:
> Y. LeCun, L. Bottou, Y. Bengio, and P. Haffner. *"Gradient-based learning applied to document recognition."* Proceedings of the IEEE, 86(11):2278-2324, November 1998. [on-line version]
