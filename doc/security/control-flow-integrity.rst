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

Limitations
-----------

Currently, the shadow stacks that are created behind the scenes live on the global namespace. Thus, one *cannot* reuse thread stack names, even across different compilation units.
