# Zephyr API Reference

This is the reference documentation for the [Zephyr RTOS](https://zephyrproject.org) C API: all the
functions, macros, and data structures available to Zephyr applications and subsystems, generated
directly from the source code.

## Browse the API by topic

The API is organized as a hierarchy of [**Topics**](topics.html) covering the main areas of the OS.
Start with one of the main categories below, or browse the [full topic list](topics.html).

<div class="topic-cards">
  <a class="topic-card" href="group__kernel__apis.html">
    <span class="topic-card-title">Kernel</span>
    <span class="topic-card-desc">
      Threads, scheduling, synchronization, timers, and other core RTOS primitives.
    </span>
  </a>
  <a class="topic-card" href="group__io__interfaces.html">
    <span class="topic-card-title">Device Drivers</span>
    <span class="topic-card-desc">
      Hardware-agnostic interfaces for peripherals: GPIO, I2C, SPI, sensors, and more.
    </span>
  </a>
  <a class="topic-card" href="group__os__services.html">
    <span class="topic-card-title">Operating System Services</span>
    <span class="topic-card-desc">
      Networking, Bluetooth, storage, logging, and other services built on the kernel.
    </span>
  </a>
  <a class="topic-card" href="group__devicetree.html">
    <span class="topic-card-title">Devicetree</span>
    <span class="topic-card-desc">
      Macros for reading the hardware description of the system at build time.
    </span>
  </a>
  <a class="topic-card" href="group__device__model.html">
    <span class="topic-card-title">Device Model</span>
    <span class="topic-card-desc">
      The abstraction that binds drivers to the hardware instances they manage.
    </span>
  </a>
  <a class="topic-card" href="group__mem__mgmt.html">
    <span class="topic-card-title">Memory Management</span>
    <span class="topic-card-desc">Heaps, memory slabs, and memory-mapping interfaces.</span>
  </a>
  <a class="topic-card" href="group__utilities.html">
    <span class="topic-card-title">Utilities</span>
    <span class="topic-card-desc">General-purpose helpers, including the data structure APIs.</span>
  </a>
  <a class="topic-card" href="group__testing.html">
    <span class="topic-card-title">Testing</span>
    <span class="topic-card-desc">The Ztest framework and related testing helpers.</span>
  </a>
</div>

## Other ways to explore

- [Data Structures](annotated.html): an index of all documented `struct`s and `union`s.
- [Files](files.html): browse the API header by header.
- [Deprecated List](deprecated.html): APIs scheduled for removal, and what to use instead.
- Use the search box to find any documented symbol by name.

## Looking for something else?

This site only covers the C API. For guides, samples, supported boards, and everything else, head
over to the main [Zephyr Project documentation](https://docs.zephyrproject.org/latest/).
