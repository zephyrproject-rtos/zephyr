.. _dma_api:

Direct Memory Access (DMA)
##########################

Overview
********

Direct Memory Access (Controller) is a commonly provided type of co-processor that can typically
offload transferring data to and from peripherals and memory.

The DMA API is not a portable API and really cannot be as each DMA has unique memory requirements,
peripheral interactions, and features. The API in effect provides a union of all useful DMA
functionality drivers have needed in the tree. It can still be a good abstraction, with care, for
peripheral devices for vendors where the DMA IP might be very similar but have slight variances.

Driver Implementation Expectations
**********************************

Synchronization and Ownership
+++++++++++++++++++++++++++++

From an API point of view, a DMA channel is a single-owner object, meaning the drivers should not
attempt to wrap a channel with kernel synchronization primitives such as mutexes or semaphores. If
DMA channels require mutating shared registers, those register updates should be wrapped in a spin
lock.

This enables the entire API to be low-cost and callable from any call context, including ISRs where
it may be very useful to start/stop/suspend/resume/reload a channel transfer.

Transfer Descriptor Memory Management
+++++++++++++++++++++++++++++++++++++

Drivers should not attempt to use heap allocations of any kind. If object pools are needed for
transfer descriptors then those should be setup in a way that does not break the promise of
ISR-allowable calls. Many drivers choose to create a simple static descriptor array per channel with
the size of the descriptor array adjustable using Kconfig.

Channel State Machine Expectations
++++++++++++++++++++++++++++++++++

DMA channels should be viewed as state machines that the DMA API provides transition events for in
the form of API calls. Every driver is expected to maintain its own channel state tracking. The busy
state of the channel should be inspectable at any time with :c:func:`dma_get_status()`.

A diagram showing those expectated possible state transitions and their API calls is provided here
for reference.

.. graphviz::
   :caption: DMA state finite state machine

   digraph {
       node [style=rounded];
       edge [fontname=Courier];
       init [shape=point];

       CONFIGURED [label=Configured,shape=box];
       RUNNING [label=Running,shape=box];
       SUSPENDED [label=Suspended,shape=box];

       init -> CONFIGURED [label=dma_config];

       CONFIGURED -> RUNNING [label=dma_start];
       CONFIGURED -> CONFIGURED [label=dma_stop];

       RUNNING -> CONFIGURED [label=dma_stop];
       RUNNING -> RUNNING [label=dma_start];
       RUNNING -> RUNNING [label=dma_resume, headport=w];
       RUNNING -> SUSPENDED [label=dma_suspend];

       SUSPENDED -> SUSPENDED [label=dma_suspend];
       SUSPENDED -> RUNNING [label=dma_resume];
       SUSPENDED -> CONFIGURED [label=dma_stop];
   }

API Reference
*************

.. doxygengroup:: dma_interface
