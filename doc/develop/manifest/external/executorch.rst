.. _external_module_executorch:

ExecuTorch
##########

Introduction
************

`ExecuTorch <https://github.com/pytorch/executorch>`_ is PyTorch's on-device inference runtime.
In Zephyr, it can be integrated as an external module to run models on CPU and Arm Ethos-U NPUs.

If you are new to ExecuTorch, these resources are a great place to start
learning:

- `How ExecuTorch Works <https://docs.pytorch.org/executorch/stable/intro-how-it-works.html>`_
- `Getting Started Architecture overview
  <https://docs.pytorch.org/executorch/stable/getting-started-architecture.html>`_

ExecuTorch is licensed under the `BSD 3-Clause License
<https://github.com/pytorch/executorch/blob/main/LICENSE>`_.

Usage with Zephyr
*****************

This section covers ExecuTorch module registration, model preparation, and
build/run steps for both CPU and Arm Ethos-U NPU targets.

.. note::

   **Prerequisites**

   - **Python 3.12–3.13** — required by ExecuTorch tooling. Use a separate
     virtual environment, or a Zephyr virtual environment created with a
     compatible Python version.
   - **Arm FVPs** — only required if you are
     targeting a Corstone FVP (e.g. ``mps3/corstone300/fvp``) to simulate
     Cortex-M and the Ethos-U NPU without physical hardware. See
     :ref:`Installing Arm FVPs <fvp-install>` for installation steps.
   - **Docker (macOS only)** — required only for the Arm Ethos-U NPU
     Inference flow via
     `FVPs-on-Mac <https://github.com/Arm-Examples/FVPs-on-Mac.git>`_.

Installing ExecuTorch
=====================

**Step 1:** Register ExecuTorch as an external module by adding the following
project entry to your west manifest. You can either create a dedicated
submanifest file at ``zephyrproject/zephyr/submanifests/executorch.yaml``, or
add it directly to your application's existing ``west.yml``:

.. code-block:: yaml

   manifest:
     projects:
       - name: executorch
         url: https://github.com/pytorch/executorch
         revision: v1.2.0
         path: modules/lib/executorch
         submodules: true

**Step 2:** Run west update:

.. code-block:: console

   west update

**Step 3:** Install ExecuTorch and dependencies:

.. note::

   Run these commands inside your Zephyr virtual environment with a compatible
   Python version (3.12–3.13).

.. code-block:: console

   pip install executorch==1.2.0
   pip install tosa-tools==2026.2.1
   pip install ethos-u-vela==5.0.0

Build and run
=============

When running AI models on embedded devices, your target hardware may include a
dedicated AI accelerator — commonly called an NPU (Neural Processing Unit).
Arm provides the
`Ethos-U NPU family <https://www.arm.com/products/silicon-ip-cpu?families=ethos%20npus>`_
for efficient on-device AI inference.
The tabs below cover both paths: targeting an Ethos-U NPU for accelerated
inference, and CPU-only inference for devices without an NPU.

.. _fvp-install:

.. tabs::

   .. group-tab:: Arm Ethos-U NPU Inference

      .. note::

         **Installing Arm FVPs**

         Fixed Virtual Platforms (FVPs) are Arm-provided simulators that let you
         run Zephyr without physical hardware. They are required here to emulate
         Ethos-U55/U65/U85 accelerators via the Corstone-300 reference platform.

         .. tabs::

            .. group-tab:: Ubuntu

               **Step 1:** Download the FVP installer for Corstone-300 from the
               Arm FVP page. FVPs can be installed anywhere on your machine —
               they do not need to be inside the Zephyr project directory.

               `Arm Corstone FVPs
               <https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms/IoT%20FVPs>`_

               .. note::

                  FVP version numbers change with each release. Set the
                  filename and URL to the version you want from the Arm
                  downloads page.

                  .. code-block:: console

                     FVP_TGZ="FVP_Corstone_SSE-300_11.27_42_Linux64_armv8l.tgz"
                     FVP_URL="https://developer.arm.com/-/cdn-downloads/permalink/FVPs-Corstone-IoT/Corstone-300/${FVP_TGZ}"
                     curl -L -o "${FVP_TGZ}" "${FVP_URL}"

               **Step 2:** Unpack the downloaded archive:

               .. code-block:: console

                  tar -xf "${FVP_TGZ}"

               **Step 3:** Run the installation script:

               .. code-block:: console

                  ./FVP_Corstone_SSE-300.sh --i-agree-to-the-contained-eula --no-interactive -q

               **Step 4:** Add the FVPs to your ``PATH``. Both
               ``FVP_Corstone_SSE-300_Ethos-U55`` and
               ``FVP_Corstone_SSE-300_Ethos-U65`` live in the same directory:

               .. code-block:: console

                  export PATH=$HOME/FVP_Corstone_SSE-300/models/Linux64_armv8l_GCC-9.3:$PATH

               **Step 5:** Install the required runtime dependency
               (``libpython3.9.so.1.0``) by sourcing the provided script.
               The method differs slightly between shells:

               *bash:*

               .. code-block:: console

                  source $HOME/FVP_Corstone_SSE-300/scripts/runtime.sh
                  unset PYTHONHOME

               *zsh:* ``BASH_SOURCE`` must be set manually before sourcing,
               as zsh does not populate it automatically:

               .. code-block:: console

                  BASH_SOURCE=$HOME/FVP_Corstone_SSE-300/scripts/runtime.sh
                  source $HOME/FVP_Corstone_SSE-300/scripts/runtime.sh
                  unset PYTHONHOME

               **Step 6:** Verify the installation:

               .. code-block:: console

                  FVP_Corstone_SSE-300_Ethos-U55 --version
                  FVP_Corstone_SSE-300_Ethos-U65 --version

            .. group-tab:: macOS

               On macOS, FVPs run via Docker wrappers provided by the
               `FVPs-on-Mac <https://github.com/Arm-Examples/FVPs-on-Mac.git>`_ project.

               **Step 1:** Clone the repository and checkout the required commit.
               The repository can be cloned anywhere on your machine — it does
               not need to be inside the Zephyr project directory.

               .. code-block:: console

                  git clone https://github.com/Arm-Examples/FVPs-on-Mac.git

               .. code-block:: console

                  cd FVPs-on-Mac
                  git checkout 1458860

               **Step 2:** Build the Docker wrapper:

               .. code-block:: console

                  ./build.sh

               .. note::

                  Docker must be installed and running before executing the
                  wrapper build and FVP commands on macOS.

               **Step 3:** Sanity check the build:

               .. code-block:: console

                  ./bin/FVP_Corstone_SSE-300 --version

               **Step 4:** Expose the FVP binaries to your environment:

               .. code-block:: console

                  export PATH=$PATH:$(pwd)/bin

            .. group-tab:: Windows

               **Step 1:** Download the FVP installer for Corstone-300 from the
               Arm FVP page. FVPs can be installed anywhere on your machine —
               they do not need to be inside the Zephyr project directory.

               `Arm FVPs
               <https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms/IoT%20FVPs>`_

               Select the Windows installer and run it. Follow the installer wizard.
               The FVP binaries are typically installed to a path such as
               ``C:\Program Files\ARM\FVP_Corstone_SSE-300\models\Win64_VC2019``.

               .. note::

                  You may need to install ``python39.dll`` if you don't already have it

               **Step 2:** Open a new PowerShell window and verify the installation:

               .. code-block:: console

                  .\FVP_Corstone_SSE-300.exe --version

            .. group-tab:: Zephyr Docker CI

               The `Zephyr Docker CI image <https://github.com/zephyrproject-rtos/docker-image>`_
               is an alternative to installing FVPs manually. It is Zephyr's
               official CI container and already includes Corstone-300 and
               Corstone-320 FVPs, along with all other Zephyr build
               dependencies — no separate FVP installation required.

               **Step 1:** Pull the image:

               .. code-block:: console

                  docker pull ghcr.io/zephyrproject-rtos/zephyr-build:main

               **Step 2:** Start a container with your Zephyr workspace and
               Zephyr SDK mounted:

               .. code-block:: console

                  docker run -it --rm \
                    -v $HOME/zephyrproject:$HOME/zephyrproject \
                    -v $HOME/zephyr-sdk-0.17.4:$HOME/zephyr-sdk-0.17.4 \
                    -w $HOME/zephyrproject \
                    -e HOME=$HOME \
                    -e ZEPHYR_SDK_INSTALL_DIR=$HOME/zephyr-sdk-0.17.4 \
                    --user root \
                    --entrypoint /bin/bash \
                    ghcr.io/zephyrproject-rtos/zephyr-build:main

               .. note::

                  The SDK version (``0.17.4``) may change. Replace it with
                  your installed version, or use the generic form:

                  .. code-block:: console

                     docker run -it --rm \
                       -v $HOME/zephyrproject:$HOME/zephyrproject \
                       -v $HOME/zephyr-sdk-<version>:$HOME/zephyr-sdk-<version> \
                       -w $HOME/zephyrproject \
                       -e HOME=$HOME \
                       -e ZEPHYR_SDK_INSTALL_DIR=$HOME/zephyr-sdk-<version> \
                       --user root \
                       --entrypoint /bin/bash \
                       ghcr.io/zephyrproject-rtos/zephyr-build:main

               **Step 3:** Inside the container, activate the Zephyr virtual
               environment:

               .. code-block:: console

                  source $HOME/zephyrproject/.venv/bin/activate

               **Step 4:** Verify the FVPs are available:

               .. code-block:: console

                  FVP_Corstone_SSE-300 --version
                  FVP_Corstone_SSE-320 --version

               You can then run any ``west build`` commands from within the
               container exactly as described in the build steps below.

      .. rubric:: Prepare the Ethos-U55 PTE model

      In ExecuTorch, a ``.pte`` file is a serialized program file and the final
      binary format used to deploy PyTorch models to edge and mobile devices.
      For guidance on exporting and lowering your own PyTorch models with the
      Arm Ethos-U backend, see
      `Using ExecuTorch Export
      <https://docs.pytorch.org/executorch/stable/using-executorch-export.html>`_.

      The model used here is a minimal ``add`` model that takes two tensors
      and adds them element-wise. It exists purely to verify that the full
      ExecuTorch workflow is working correctly — from model compilation through
      to on-device inference via the Ethos-U NPU. The expected output is that
      each element equals ``2 + 2 = 4``.

      From the Zephyr root (for example ``~/zephyrproject``), run:

      .. tabs::

         .. group-tab:: Ubuntu/macOS

            .. code-block:: console

               cd ~/zephyrproject
               python -m modules.lib.executorch.examples.arm.aot_arm_compiler \
                 --model_name=modules/lib/executorch/examples/arm/example_modules/add.py \
                 --quantize --delegate -t ethos-u55-128 --output=add_u55_128.pte

         .. group-tab:: Windows

            .. code-block:: console

               cd ~/zephyrproject
               python -m modules.lib.executorch.examples.arm.aot_arm_compiler `
                 --model_name=modules/lib/executorch/examples/arm/example_modules/add.py `
                 --quantize --delegate -t ethos-u55-128 --output=add_u55_128.pte

      ``--delegate`` tells ``aot_arm_compiler`` to use the Ethos-U backend and
      ``-t ethos-u55-128`` selects the Ethos-U variant and MAC count. These
      must match your hardware or FVP configuration.

      .. rubric:: Build and Run

      From the Zephyr root (``~/zephyrproject``), run:

      .. tabs::

         .. group-tab:: Ubuntu/macOS

            .. code-block:: console

               cd ~/zephyrproject
               west build -p auto -b mps3/corstone300/fvp \
                 modules/lib/executorch/zephyr/samples/hello-executorch \
                 -t run -- -DET_PTE_FILE_PATH=add_u55_128.pte

         .. group-tab:: Windows

            .. code-block:: console

               cd $env:USERPROFILE\zephyrproject
               west build -p auto -b mps3/corstone300/fvp `
                 modules/lib/executorch/zephyr/samples/hello-executorch -t run -- `
                 "-DET_PTE_FILE_PATH=$PWD\add_u55_128.pte"

   .. group-tab:: CPU-only Inference

      .. rubric:: Prepare the model

      The model used here is a minimal ``add`` model exported for Cortex-M55.
      This example uses the CPU-only target currently available in the
      ExecuTorch Arm AOT compiler. Supported targets may evolve over time.
      Some additional Cortex-M boards may also run the generated ``.pte`` if
      the required operators are supported by the runtime, but compatibility
      should be validated per board. From the Zephyr root (for example
      ``~/zephyrproject``), run:

      .. code-block:: console

         python -m modules.lib.executorch.examples.arm.aot_arm_compiler \
           --model_name=modules/lib/executorch/examples/arm/example_modules/add.py \
           --quantize --target=cortex-m55+int8 --output=add_m55.pte

      .. rubric:: Build and Run

      Replace ``<board>`` with your target board. For boards other than those
      validated below, compatibility should be verified on the target.

      .. code-block:: console

         west build -b <board> modules/lib/executorch/zephyr/samples/hello-executorch \
           -t run -- -DET_PTE_FILE_PATH=add_m55.pte

      .. note::

         This sample has been tested on the following boards with the same
         ``add_m55.pte`` artifact:

         - **STM Nucleo-N657X0-Q** (``nucleo_n657x0_q``):

           .. code-block:: console

              west build -b nucleo_n657x0_q \
                modules/lib/executorch/zephyr/samples/hello-executorch \
                -- -DET_PTE_FILE_PATH=add_m55.pte

         - **nRF5340 DK** (``nrf5340dk/nrf5340/cpuapp``):

           .. code-block:: console

              west build -b nrf5340dk/nrf5340/cpuapp \
                modules/lib/executorch/zephyr/samples/hello-executorch \
                -- -DET_PTE_FILE_PATH=add_m55.pte

      To flash to hardware after building:

      .. code-block:: console

         west flash

Expected Run Output
===================

.. code-block:: console

   I [executorch:arm_executor_runner.cpp:450 main()] Model executed successfully.
   I [executorch:arm_executor_runner.cpp:457 main()] Model outputs:
   I [executorch:arm_executor_runner.cpp:464 main()]   output[0]: tensor scalar_type=Float numel=5
   I [executorch:arm_executor_runner.cpp:481 main()]     [0] = 4.000000
   I [executorch:arm_executor_runner.cpp:481 main()]     [1] = 4.000000
   I [executorch:arm_executor_runner.cpp:481 main()]     [2] = 4.000000
   I [executorch:arm_executor_runner.cpp:481 main()]     [3] = 4.000000
   I [executorch:arm_executor_runner.cpp:481 main()]     [4] = 4.000000
   I [executorch:arm_executor_runner.cpp:499 main()] SUCCESS: Program complete, exiting.

The ``output`` values of ``4.000000`` confirm that the model ran successfully
on-device, each element is the result of ``2 + 2``, computed by the Ethos-U NPU
or CPU backend via ExecuTorch.

ExecuTorch on Zephyr is actively being developed, and more complex and
interesting sample applications are on the way. You can follow progress and find
new samples here: `ExecuTorch Zephyr samples
<https://github.com/pytorch/executorch/tree/main/zephyr/samples>`_.

Reference
*********

- `ExecuTorch <https://github.com/pytorch/executorch>`_ — PyTorch's on-device inference runtime.
- `How ExecuTorch Works <https://docs.pytorch.org/executorch/stable/intro-how-it-works.html>`_
  — Primer on the ExecuTorch workflow.
- `Getting Started Architecture
  <https://docs.pytorch.org/executorch/stable/getting-started-architecture.html>`_
  — High-level ExecuTorch architecture overview.
- `Arm FVPs <https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms/IoT%20FVPs>`_
  — Fixed Virtual Platforms for Cortex-M and Ethos-U simulation.
- `FVPs-on-Mac <https://github.com/Arm-Examples/FVPs-on-Mac.git>`_
  — Docker wrappers that enable Arm FVPs to run on macOS.
- `Arm Ethos-U NPU family <https://www.arm.com/products/silicon-ip-cpu?families=ethos%20npus>`_
  — Arm's NPU IP for efficient on-device AI inference.
- `Using ExecuTorch Export
  <https://docs.pytorch.org/executorch/stable/using-executorch-export.html>`_
  — Guide to export and backend lowering for ExecuTorch models.
- `Zephyr Docker CI image <https://github.com/zephyrproject-rtos/docker-image>`_
  — Zephyr's official CI container, includes pre-installed FVPs and build dependencies.
- `hello-executorch sample
  <https://github.com/pytorch/executorch/tree/main/zephyr/samples/hello-executorch>`_
  — Minimal ExecuTorch sample for Zephyr used in this guide.
