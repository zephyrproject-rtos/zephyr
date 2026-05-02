.. _control_flow_integrity:

Control Flow Integrity
######################

Control Flow Integrity (CFI) is a security feature that ensures that the control flow of a program follows a predefined path, preventing attackers from diverting execution to malicious code. CFI is particularly useful in defending against control flow hijacking attacks, such as return-oriented programming (ROP) and jump-oriented programming (JOP).

CFI works by validating the control flow of a program at runtime, ensuring that function calls and returns are made to legitimate targets. This is typically achieved through instrumentation of the code, which adds checks to verify that the control flow adheres to the expected paths defined by the program's control flow graph.

Forward and return edges
------------------------

In the context of CFI, control flow edges can be categorized into two types:

1. **Forward edges**: These represent the flow of control from one function to another, such as a function call. Forward edges are typically validated to ensure that the target of a call is a legitimate function within the program.
2. **Return edges**: These represent the return from a function back to the caller. Return edges are validated to ensure that the return address is legitimate and corresponds to a valid call site.

Forward edges can be supported by compiler instrumentation, which adds checks to verify that the target of a function call is valid. Return edges can be supported by maintaining a shadow stack or using other mechanisms to ensure that return addresses are legitimate.

Zephyr does support maintaining a shadow stack which can be enabled via :kconfig:option:`CONFIG_HW_SHADOW_STACK`. Then, the kernel will use macros like :c:macro:`K_THREAD_HW_SHADOW_STACK_DEFINE`, in tandem with other thread stack related macros, to provide the area for shadow stacks used by threads. Usually, applications only need to enable :kconfig:option:`CONFIG_HW_SHADOW_STACK` (and related ones, such as :kconfig:option:`CONFIG_HW_SHADOW_STACK_PERCENTAGE_SIZE` and :kconfig:option:`CONFIG_HW_SHADOW_STACK_MIN_SIZE`) to enable shadow stack support. The kernel will then automatically manage the shadow stack for each thread.

Implementation details
**********************

The ``K_THREAD_HW_SHADOW_STACK*`` family of macros does a minimal setup of the shadow stack parameters. Then, they invoke arch-specific ``ARCH_THREAD_HW_SHADOW_STACK*`` macros to perform the actual setup.

Hardware support
----------------

While CFI can be implemented in software, hardware support can significantly enhance its effectiveness and performance. Currently, Zephyr supports Intel Control-flow Enforcement Technology (CET), which provides hardware-based support for CFI.

Intel CET
*********

Intel Control-flow Enforcement Technology (CET) is a set of hardware features that enhance the security of applications by providing support for CFI. CET includes two main components:

1. **Shadow Stack**: This feature maintains a separate stack for return addresses, ensuring that return addresses cannot be tampered with. When a function returns, the return address is popped from the shadow stack and compared to that of the regular stack, providing an additional layer of protection against control flow hijacking.
2. **Indirect Branch Tracking (IBT)**: This feature tracks indirect branches (such as function pointers) and ensures that they only target valid locations in the code. It prevents attackers from redirecting execution to arbitrary code through techniques like ROP.

These two features proved the return and forward edge validation, respectively. To enable shadow stack support in Zephyr, on supported hardware, you can use the :kconfig:option:`CONFIG_HW_SHADOW_STACK` Kconfig option. To enable IBT, use :kconfig:option:`CONFIG_X86_CET_IBT`.

As IBT is effectively implemented by the compiler, toolchain support is necessary. Currently, Zephyr SDK x86 toolchain can be used to build applications with IBT support. However, their precompiled artifacts, such as ``libc`` and ``libgcc``, do not have IBT enabled. Therefore, for ``libc``, one needs to build it as a module, for example, by using :kconfig:option:`CONFIG_PICOLIBC_USE_MODULE`. For other bits, one would need a custom toolchain.

Intel CET Limitations
^^^^^^^^^^^^^^^^^^^^^

Currently, the shadow stacks that are created behind the scenes live on the global namespace. Thus, one *cannot* reuse thread stack names, even across different compilation units.

ARM Pointer Authentication and Branch Target Identification
*************************************************************

ARM platforms provide hardware-based CFI features through Pointer Authentication (PAC) and Branch Target Identification (BTI), available on both Cortex-M and ARM64 (Cortex-A/R) architectures:

1. **Pointer Authentication (PAC)**: Signs return addresses and other pointers using cryptographic signatures. When a return address is pushed to the stack, it is signed with a key unique to each thread. Before returning, the signature is verified, ensuring the return address has not been tampered with. This protects against Return-Oriented Programming (ROP) attacks. Available in ARMv8.1-M Mainline (Cortex-M) and ARMv8.3-A and later (ARM64).

2. **Branch Target Identification (BTI)**: Marks legitimate indirect branch targets with special BTI landing pad instructions. The processor verifies that indirect branches land only on valid targets marked with BTI instructions. This protects against Jump-Oriented Programming (JOP) attacks. Available in ARMv8.1-M Mainline (Cortex-M) and ARMv8.5-A and later (ARM64).

These features provide return and forward edge validation, respectively. PAC and BTI can be enabled via the :kconfig:option:`ARM_PACBTI` menu. Available options include :kconfig:option:`CONFIG_ARM_PACBTI_STANDARD` (both PAC and BTI), :kconfig:option:`CONFIG_ARM_PACBTI_PACRET` (PAC only), :kconfig:option:`CONFIG_ARM_PACBTI_BTI` (BTI only), and other variants. Both features can be used independently or combined for comprehensive control flow integrity.

Like x86 CET IBT, these features require compiler support and proper C library instrumentation. PAC requires that functions use the appropriate signing and verification instructions, while BTI requires that all legitimate branch targets include BTI landing pad instructions. Precompiled C libraries from toolchains typically lack this instrumentation.

For BTI support, the C library must be compiled with ``-mbranch-protection`` flags to include BTI landing pads in all functions. Therefore, when using any option that enables BTI, only :kconfig:option:`CONFIG_MINIMAL_LIBC` or picolibc built as a module via :kconfig:option:`CONFIG_PICOLIBC_USE_MODULE` can be used. Newlib from toolchains is not supported with BTI.

PAC has less strict requirements as it primarily affects function prologues and epilogues, but for optimal security, building the C library with PAC support is recommended.
