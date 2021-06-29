CIFAR-10 Convolution Neural Network Example 
==============================================
Example is based on standard [Caffe tutorial](http://caffe.berkeleyvision.org/gathered/examples/cifar10.html) for [CIFAR-10](http://www.cs.toronto.edu/~kriz/cifar.html) dataset. It's a simple classifier built on convolution, pooling and dense layers for tiny images.


Quick Start
--------------

Example supports building with [Zephyr Software Development Kit (SDK)](https://docs.zephyrproject.org/latest/getting_started/installation_linux.html#zephyr-sdk) and running with MetaWare Debuger on [nSim simulator](https://www.synopsys.com/dw/ipdir.php?ds=sim_nSIM).

### Add embarc_mli module to Zephyr instruction

1. Open command line and change working directory to './zephyrproject/zephyr'

2. Download embarc_mli version 1.1

        west update

3. Change working directory to './zephyrproject/modules/lib/embarc_mli'

4. Create a new folder named zephyr and change working directory to it

        mkdir zephyr

        cd zephyr

5. Create module.yml file

        touch module.yml

        vi module.yml

6. Copy and paste the content listed below in vim editor

        name: embarc_mli

        build:

            cmake-ext: True

            kconfig-ext: True

### Build with Zephyr SDK toolchain

    Build requirements:
        - Zephyr SDK toolchain version 0.12.3 or higher
        - gmake

1. Open command line and change working directory to './zephyrproject/zephyr/samples/modules/embarc_mli/example_cifar10_caffe'

2. Build example

        west build -b nsim_em samples/modules/embarc_mli/example_cifar10_caffe 

### Run example

1. Run example 

        west flash

    Result Quality shall be "S/N=4383.8     (72.8 db)"

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
