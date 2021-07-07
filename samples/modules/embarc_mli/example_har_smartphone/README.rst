STM Based Human Activity Recognition (HAR) Example
==============================================
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

1. Open command line and change working directory to './zephyrproject/zephyr/samples/modules/embarc_mli/example_har_smartphone'

2. Build example

        west build -b nsim_em samples/modules/embarc_mli/example_har_smartphone

### Run example

1. Run example

        west flash

    Result Quality shall be "S/N=1823.9     (65.2 db)"

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
