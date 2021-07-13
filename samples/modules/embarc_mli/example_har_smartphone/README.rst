.. _embarc_mli_example_har_smartphone:

STM Based Human Activity Recognition (HAR) Example
##################################################

Overview
********
Example shows how to work with recurrent primitives (LSTM and basic RNN) implemented in embARC MLI Library. It is based on open source [GitHub project](https://github.com/guillaume-chevalier/LSTM-Human-Activity-Recognition) by Guillaume Chevalie. Chosen approach, complexity of the model and [dataset](https://archive.ics.uci.edu/ml/datasets/Human+Activity+Recognition+Using+Smartphones) are relevant to IoT domain. The model is intended to differentiate human activity between 6 classes based on inputs from embedded inertial sensors from waist-mounted smartphone. Classes:
 * 0: WALKING
 * 1: WALKING_UPSTAIRS
 * 2: WALKING_DOWNSTAIRS
 * 3: SITTING
 * 4: STANDING
 * 5: LAYING

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

1. Open command line and change working directory to './zephyrproject/zephyr/samples/modules/embarc_mli/example_har_smartphone'

2. Build example

        west build -b nsim_em samples/modules/embarc_mli/example_har_smartphone

Run example
--------------

1. Run example

        west flash

    Result Quality shall be "S/N=1823.9     (65.2 db)"

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

* If hostlink is not available, please reads vectors from input Array file, passes it to the model, and accumulates number of successive classifications according to labels array file:

       _C_ARRAY_

Example Structure
--------------------
Structure of example application may be divided logically on three parts:

* **Application.** Implements Input/output data flow and it's processing by the other modules. Application includes:
   * ml_api_har_smartphone_main.c
   * ../auxiliary/examples_aux.h(.c)
* **Inference Module.** Uses embARC MLI Library to process input according to pre-defined graph. All model related constants are pre-defined and model coefficients is declared in the separate compile unit
   * har_smartphone_model.h
   * har_smartphone_model.c
   * har_smartphone_constants.h
   * har_smartphone_coefficients.c
* **Auxiliary code.** Various helper functions for measurements, IDX file IO, etc.
   * ../auxiliary/tensor_transform.h(.c)
   * ../auxiliary/tests_aux.h(.c)
   * ../auxiliary/idx_file.h(.c)

References
----------------------------
GitHub project served as starting point for this example:
> Guillaume Chevalier, *LSTMs for Human Activity Recognition*, 2016,[https://github.com/guillaume-chevalier/LSTM-Human-Activity-Recognition](https://github.com/guillaume-chevalier/LSTM-Human-Activity-Recognition)

Human Activity Recognition Using Smartphones [Dataset](https://archive.ics.uci.edu/ml/datasets/Human+Activity+Recognition+Using+Smartphones):
> Davide Anguita, Alessandro Ghio, Luca Oneto, Xavier Parra and Jorge L. Reyes-Ortiz. *"A Public Domain Dataset for Human Activity Recognition Using Smartphones."* 21th European Symposium on Artificial Neural Networks, Computational Intelligence and Machine Learning, ESANN 2013. Bruges, Belgium 24-26 April 2013:

IDX file format originally was used for [MNIST database](http://yann.lecun.com/exdb/mnist/). There is a python [package](https://pypi.org/project/idx2numpy/) for working with it through transformation to/from numpy array. *auxiliary/idx_file.c(.h)* is used by the test app for working with IDX files:
> Y. LeCun, L. Bottou, Y. Bengio, and P. Haffner. *"Gradient-based learning applied to document recognition."* Proceedings of the IEEE, 86(11):2278-2324, November 1998. [on-line version]
