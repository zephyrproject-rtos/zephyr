.. zephyr:code-sample:: simple_eigen
   :name: Hello world Executorch library

   Demonstrate basic Executorch usage with XNNPack backend with one MobileNetV2 model.

   Python Dependencies:
   - Executorch
   - flatc

   To use, first run `./generate_pte.sh` to generate the model_pte.c file.

   To modify the example model, please modify `gen_pte.py`.

   To switch on/off the RISC-V Vector micro-kernels in XNNPack, set the CMake option ``XNNPACK_ENABLE_RISCV_VECTOR`` to ``ON`` or ``OFF``.

   Some toolchain may need a patch to C header files to support XNNPack. The gcc14.patch is targeted for the RISC-V GCC14.

