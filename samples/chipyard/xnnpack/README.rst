.. zephyr:code-sample:: simple_eigen
   :name: Hello world XNNPack library

   Demonstrate basic XNNPack usage with one fully connected layer.

   To switch on/off the RISC-V Vector micro-kernels in XNNPack, set the CMake option ``XNNPACK_ENABLE_RISCV_VECTOR`` to ``ON`` or ``OFF``.

   Some toolchain may need a patch to C header files to support XNNPack. The  gcc14.patch is targeted for the RISC-V GCC14.

