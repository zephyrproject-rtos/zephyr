.. _mpipe:

Multimedia Pipeline (mpipe)
###########################

.. contents::
   :local:
   :depth: 2

Overview
********

The Multimedia Pipeline subsystem (mpipe) builds a media stream out of
self-contained processing components called **elements**. An application
declares the elements it needs, links them into a graph, and drives that graph
through a state machine; mpipe negotiates the data format between neighboring
elements, settles which buffer pool provides the buffers, and moves those
buffers from one element to the next.

.. graphviz::
   :align: center
   :caption: A pipeline is a graph of elements joined at their pads.

   digraph pipeline {
     rankdir=LR;
     node [shape=record, style=filled, fillcolor="#e8e8e8", fontname="sans"];
     edge [fontname="sans", fontsize=10];

     src   [label="{ source | { <o> src pad } }"];
     trans [label="{ { <i> sink pad } | transform | { <o> src pad } }"];
     sink  [label="{ { <i> sink pad } | sink }"];

     src:o   -> trans:i [label="negotiated format"];
     trans:o -> sink:i  [label="negotiated format"];
   }

mpipe provides the pieces and the rules by which they fit together rather than
finished solutions, so a pipeline is assembled much like building with LEGO
bricks: the same graph runs on a different board by binding its elements to
different devices, and a new requirement is usually one more element rather than
a rewrite.

The elements themselves live in **plugins**, grouped by media domain. A plugin
brings its own directory, its own Kconfig and its own headers, and the build
picks it up without any edit to the framework, so a silicon vendor or a
middleware provider can ship elements without altering the core.

mpipe performs no dynamic allocation: buffers come from pools sized while the
pipeline starts, and everything on the negotiation path is fixed-size and held
by value. The cost lands on the stack instead. Negotiation runs inside
:c:func:`mpipe_element_set_state`, on whichever thread calls it, and holds
several capabilities live at once - budget for it on that thread, which for the
example below is the one running ``main``.

mpipe is optional, and it is not always the right tool. An application driving a
single device is better served by that device's API directly. mpipe earns its
place once several devices have to agree on a format and hand buffers to each
other.

Building a pipeline
*******************

An application includes ``<zephyr/mpipe/mpipe.h>``, and only that, for the whole
core API; each element it instantiates adds that element's own header.

Elements are plain objects the application owns; mpipe allocates none of them.
Each element type has its own init function taking that type and an id:

.. code-block:: c

   /* Concrete element types come from plugins; each has its own header. */
   static struct mpipe pipe;
   static struct my_src source;
   static struct my_sink sink;

   ret = mpipe_pipeline_init(&pipe, PIPE_ID);
   ret = my_src_init(&source, SRC_ID);
   ret = my_sink_init(&sink, SINK_ID);

Ids only have to be unique within the pipeline. Elements are configured through
properties, which is how an element is set up without the caller knowing its
concrete type:

.. code-block:: c

   ret = mpipe_object_set_properties((struct mpipe_object *)&source,
                                     MY_SRC_PROP_PATH, "/SD:/in.bin",
                                     MPIPE_PROP_LIST_END);

They are then added to the pipeline, in any order, and linked in stream order:

.. code-block:: c

   ret = mpipe_bin_add((struct mpipe_bin *)&pipe,
                       (struct mpipe_element *)&source,
                       (struct mpipe_element *)&sink, NULL);

   ret = mpipe_element_link((struct mpipe_element *)&source,
                            (struct mpipe_element *)&sink, NULL);

The casts are well defined because every element embeds its base as its first
member. Linking refuses a pair of elements whose capabilities cannot possibly
intersect, so an impossible graph fails while it is being built rather than when
it is started.

Setting the pipeline to ``MPIPE_STATE_PLAYING`` is not a jump to that state. The
pipeline and its children step through the states in order,
``READY`` to ``PAUSED`` to ``PLAYING``, which is why the format is negotiated
and the pools are started along the way.

The application then observes the pipeline's message channel, both to learn how
the run ends and to act on failures. An error message names the element that
raised it and the phase it was in, so an application can report that capability
negotiation failed at element 3 rather than only that the run stopped. Returning
the graph to ``MPIPE_STATE_READY`` tears it down:

.. code-block:: c

   /* Needs CONFIG_ZBUS_MSG_SUBSCRIBER=y */
   ZBUS_MSG_SUBSCRIBER_DEFINE(main_sub);

   struct zbus_channel *bus = mpipe_element_get_bus_chan((struct mpipe_element *)&pipe);

   ret = zbus_chan_add_obs(bus, &main_sub, K_FOREVER);

   if (mpipe_element_set_state((struct mpipe_element *)&pipe, MPIPE_STATE_PLAYING) !=
       MPIPE_STATE_CHANGE_SUCCESS) {
           /* the element that refused is still in its previous state */
   }

   do {
           ret = zbus_sub_wait_msg(&main_sub, &chan, &msg, K_FOREVER);
   } while ((msg.type & (MPIPE_MESSAGE_ERROR | MPIPE_MESSAGE_EOS)) == 0);

   (void)mpipe_element_set_state((struct mpipe_element *)&pipe, MPIPE_STATE_READY);

Elements, pads and links
************************

Every mpipe *element* derives from :c:struct:`mpipe_object` by embedding it as
its first member. The object layer carries what the framework needs of anything
it holds: an id, the container that holds it, list linkage, and the property
callbacks. On top of it, :c:struct:`mpipe_element` adds the state machine and
the pads, and the element bases specialize it. The types that only carry data -
a capability, a message, a dispatch - are plain structs outside that hierarchy:

.. graphviz::
   :align: center
   :caption: Inheritance is struct embedding, so upcasting is a plain C cast.

   digraph inheritance {
     rankdir=BT;
     node [shape=box, style=filled, fillcolor="#e8e8e8", fontname="sans"];

     object    [label="mpipe_object", fillcolor="#d0d8e8"];
     element   [label="mpipe_element"];

     src       [label="mpipe_src"];
     sink      [label="mpipe_sink"];
     transform [label="mpipe_transform"];
     parser    [label="mpipe_parser"];
     bin       [label="mpipe_bin"];
     pipeline  [label="mpipe"];

     element -> object;
     src -> element;
     sink -> element;
     transform -> element;
     parser -> element;
     bin -> element;
     pipeline -> bin;

     subgraph cluster_data {
       label="plain data, outside the hierarchy";
       fontname="sans";
       style=dashed;
       color="#999999";
       node [style="filled,dashed", fillcolor="#f5f5f5"];
       structure [label="mpipe_structure"];
       value     [label="mpipe_value"];
       message   [label="mpipe_message"];
       dispatch  [label="mpipe_dispatch"];
     }
   }

* :c:struct:`mpipe_src` produces buffers and has source pads only. It also
  drives the negotiation, because it sits at the head of the graph.
* :c:struct:`mpipe_sink` consumes them and has sink pads only.
* :c:struct:`mpipe_transform` has one of each and turns input into output.
* :c:struct:`mpipe_parser` cuts a formless byte stream into whole frames, which
  is a different job from transforming one format into another.
* :c:struct:`mpipe_bin` holds other elements and forwards state changes to them
  in the order the transition requires.
* :c:struct:`mpipe` is the top-level bin: it owns the streaming thread and the
  message channel.

A :c:struct:`mpipe_pad` is where two elements meet. It carries a direction
(source or sink), the peer it is linked to, the capability negotiated on it, and the
callbacks the framework dispatches to: ``chain_fn`` receives a buffer,
``query_fn`` answers a query, and ``event_fn`` handles an event. Linking two
elements links a source pad to a sink pad, and every hop of the stream is one
such pair.

The state machine
*****************

An element is in one of three states and moves between neighbors one step at a
time:

.. graphviz::
   :align: center
   :caption: What each transition does.

   digraph states {
     rankdir=LR;
     node [shape=box, style="rounded,filled", fillcolor="#e8e8e8", fontname="sans"];
     edge [fontname="sans", fontsize=10];

     READY -> PAUSED   [label=" negotiate the format,\l settle and start the pools\l"];
     PAUSED -> PLAYING [label=" start the source thread\l"];
     PLAYING -> PAUSED [label=" pause the source thread,\l keep what is queued\l"];
     PAUSED -> READY   [label=" flush, stop the pools,\l drop the negotiated formats\l"];
   }

``READY`` means constructed and linked, holding no format and no buffers. The
``READY`` to ``PAUSED`` transition is where the work happens: the source drives
capability negotiation across the whole graph, the buffer pool query settles who
provides buffers and how many, and the pools start.

Which way a transition goes determines the order a bin works through its
children:

* Going **up**, children transition from the sink towards the source, so a
  downstream element is ready before anything is pushed into it.
* Going **down**, children transition from the source towards the sink, so
  nothing keeps producing into an element that has already been torn down.

Nothing unwinds a failed transition. An element that refuses one stays where it
was while the elements that already moved keep their new state, which is
deliberate: the resulting graph shows exactly which element refused and in which
transition.

Capability negotiation
**********************

A **capability** describes the data crossing a link: a media type plus a set of
fields, each an :c:enum:`mpipe_caps_field` identifier paired with an
:c:struct:`mpipe_value`. It is a :c:struct:`mpipe_structure`, a fixed-size type
held by value:

.. code-block:: c

   struct mpipe_structure s;

   mpipe_structure_init_fields(&s, MPIPE_MEDIA_VIDEO,
       MPIPE_CAPS_PIXEL_FORMAT, MPIPE_TYPE_UINT, VIDEO_PIX_FMT_RGB565,
       MPIPE_CAPS_IMAGE_WIDTH, MPIPE_TYPE_UINT_RANGE, 16, 1280, 2,
       MPIPE_CAPS_IMAGE_HEIGHT, MPIPE_TYPE_UINT_RANGE, 16, 720, 2,
       MPIPE_CAPS_END);

A field holds either a single value or a range written ``[min, max, step]``, so
the capability above covers every width from 16 to 1280 in steps of 2. Two
capabilities intersect when they share a media type, have at least one field
identifier in common, and every shared field has intersecting values. The result
is the union of both: a shared field holds the intersected value, and a field
only one side carries passes through unchanged, which is what lets a constraint
travel down a chain of elements that do not themselves care about it. An *ANY*
capability constrains nothing and intersects with anything - it is what a pad
carries before it has negotiated - while an *empty* one carries no field and
intersects with nothing.

The source drives the negotiation on ``READY`` to ``PAUSED``, in two passes:

.. mermaid::
   :align: center
   :caption: Capability negotiation, driven by the source on READY to PAUSED
   :alt: Sequence diagram showing a caps query travelling from the source pad
       through the transform's two pads to the sink and the answer coming back,
       then the source fixating the format and a caps event travelling the same
       path while each element applies it with set_caps.

   %%{init: {'themeVariables': {'fontSize': '18px'}, 'sequence': {'actorFontSize': 18, 'messageFontSize': 18, 'noteFontSize': 18}}}%%
   sequenceDiagram
     autonumber
     participant so as src pad
     participant ti as sink pad
     participant to as src pad
     participant si as sink pad
     box rgba(79, 143, 214, 0.18) Source
     participant so
     end
     box rgba(148, 108, 196, 0.18) Transform
     participant ti
     participant to
     end
     box rgba(76, 168, 128, 0.18) Sink
     participant si
     end

     Note over so, si: Pass 1 - the caps query
     so ->> ti: can you take this format?
     ti ->> to: transform_caps()
     to ->> si: can you take this format?
     si -->> to: what I accept
     to ->> ti: transform_caps() back
     ti -->> so: what the chain accepts
     so ->> so: fixate to one format

     Note over so, si: Pass 2 - the caps event
     so ->> ti: this is the format
     ti ->> to: transform_caps(), narrowed
     to ->> si: this is the format
     si ->> si: set_caps()
     to ->> ti: set_caps() on both sides
     so ->> so: set_caps()

A caps query travels downstream, each element narrowing it against what it
supports before asking its own downstream, and the answer comes back narrowed to
what the whole chain has in common. If an element refuses, the source simply
offers the next format it supports rather than failing the negotiation.

A transform is the interesting case, because its two sides may speak different
formats: a decoder takes one format in and produces another out. They need not -
an element working in place rewrites the buffer where it lies, so the same format
crosses it - but where they do differ, the ``transform_caps`` hook is what maps a
capability from one side of the element to what the other side could then be. The
query crosses the element through it in both directions: out to ask the
downstream peer, and back to express the answer in terms of the input side again.

The answer may still hold ranges, so the source **fixates** it - each range
reduced to a single value - and announces the result downstream as a caps event.
Every element applies its side through its ``set_caps`` hook, which is where
hardware is actually configured.

Because a pad holds one capability rather than a set, an element that supports
several formats is walked by index: the framework asks it for capability 0, then
1, and so on until it reports there are no more. An element whose formats come
from a device answers each index by asking its driver, so nothing has to be
materialized in advance.

Buffer pool negotiation
***********************

Formats alone do not say how many buffers a graph needs, how large they must be,
how they must be aligned, or whose pool provides them. A second query settles
that, immediately after the format is fixed and in the same transition:

.. mermaid::
   :align: center
   :caption: Buffer pool negotiation, immediately after the format is fixed
   :alt: Sequence diagram showing a buffer pool query travelling downstream to
       the sink, the sink proposing a pool or a config, and each element
       deciding and starting its own pool as the proposals come back upstream.

   %%{init: {'themeVariables': {'fontSize': '18px'}, 'sequence': {'actorFontSize': 18, 'messageFontSize': 18, 'noteFontSize': 18}}}%%
   sequenceDiagram
     autonumber
     participant so as src pad
     participant ti as sink pad
     participant to as src pad
     participant si as sink pad
     box rgba(79, 143, 214, 0.18) Source
     participant so
     end
     box rgba(148, 108, 196, 0.18) Transform
     participant ti
     participant to
     end
     box rgba(76, 168, 128, 0.18) Sink
     participant si
     end

     so ->> ti: buffer pool query
     ti ->> to: forwarded downstream first
     to ->> si: buffer pool query
     si -->> to: propose_buffer_pool()
     to ->> to: decide_buffer_pool(), start out_pool
     ti -->> so: propose_buffer_pool()
     so ->> so: decide_buffer_pool(), start pool

The query travels down to the sink and proposals are written on the way back up,
so an element always has its downstream's proposal in hand before it decides
anything.

A transform's two sides are settled independently: ``decide_buffer_pool`` uses
what its downstream proposed to settle the **output** pool, while
``propose_buffer_pool`` answers the upstream query with what the element needs on
its **input**. The exception is passthrough, where one buffer crosses the element
and there is only one pool to settle.

A proposal is either an entire pool or a bare config. Offering the pool lets the
upstream element adopt it and avoid a copy; offering a config states requirements
without handing anything over. Either way, a demand reaches a pool its proposer
still owns only through :c:func:`mpipe_buffer_pool_set_config`, and the pool's
owner validates, clamps or refuses it.

Whoever starts a pool is who stops it, on ``PAUSED`` to ``READY``. That symmetry
is what makes a stop and replay behave like a first run.

Buffer flow
***********

**Buffer flow is zero-copy.** Each element that produces data owns a
:c:struct:`mpipe_buffer_pool`, and a pool is a vtable - ``configure``,
``set_config``, ``start``, ``stop``, ``acquire_buffer``, ``release_buffer`` -
over whatever backs it. That
indirection is what lets a plugin hand out the buffer its driver already owns
rather than a copy of it, so a frame captured by a camera reaches the display
without ever being moved. What travels between elements is a reference, not the
pixels.

Buffers are Zephyr :c:struct:`net_buf` allocations, with an
:c:struct:`mpipe_buffer_meta` alongside carrying what the framework needs to
know about one: the pool that owns it, how much of it is valid, a timestamp.

:c:func:`mpipe_push_buffer` walks a buffer downstream: for each hop it takes the
source pad's peer, checks that pad's flushing gate, calls its ``chain_fn``, and
follows the buffer the element produced to the next element. A NULL output means
the buffer was consumed and the walk stops. The chain function owns the buffer
it is given and releases it even when it fails, so ownership never depends on
the error path taken.

Pipeline runtime
****************

The pipeline's own thread drives the source: it acquires a buffer and pushes it
downstream until the source reports the end of its data, at which point it sends
an end-of-stream event downstream and pauses itself. An element that needs to
decouple two halves of a graph onto separate threads does so by placing a
queuing element between them.

Tearing a running graph down is ordered carefully, and the order is what keeps
it from losing data or deadlocking:

* ``PLAYING`` to ``PAUSED`` only pauses the source thread. Whatever is queued is
  preserved, so resuming continues without loss. This is a pause, not a
  teardown.
* ``PAUSED`` to ``READY`` raises a flushing gate on every pad *before* the
  children dismantle their pools, so a buffer still in flight is dropped rather
  than pushed into an element that has already been torn down. The streaming
  thread is joined only *after* the children have drained, because a child still
  holding that thread in a full queue would otherwise deadlock the join.

Messages travel to the application on the pipeline's message channel, a zbus
channel reachable with :c:func:`mpipe_element_get_bus_chan`. A message carries
where something happened - the element that emitted it - and what happened: a
type, plus, on a failure, a domain naming the phase that failed and an errno
saying why. It carries no human-facing string; the sentence describing a failure
belongs in the log at the site that detected it, while an application branches
on the domain and the errno.

A pipeline with several sinks produces one end-of-stream message per sink. The
pipeline counts them and passes on only the last, so the application is notified
exactly once and never tears the graph down while a branch is still running.

Writing an element
******************

An element is written outside the framework and requires no change to it. It embeds
one of the bases as its first member, calls that base's init with its own id, and
overrides only what it cares about:

.. code-block:: c

   struct my_sink {
           struct mpipe_sink sink;   /* must be first */
           const struct device *dev;
   };

   int my_sink_init(struct my_sink *self, uint8_t id)
   {
           int ret = mpipe_sink_init(&self->sink, id);

           if (ret != 0) {
                   return ret;
           }

           self->sink.sink_pad.enum_caps_fn = my_sink_enum_caps;
           self->sink.set_caps = my_sink_set_caps;

           return 0;
   }

An element that overrides ``change_state`` must chain to its base, which is what
performs the capability reset and the pool teardown every element is expected to
do.

The other half is saying what the element supports. A capability known at build
time is a ``static const struct mpipe_structure`` the element copies out of, and
``MPIPE_STRUCTURE_DEFINE`` places one in ``.rodata``; several are an array of
them. An element backed by a device instead builds each capability inside its
``enum_caps_fn`` by asking the driver, and one configured by the application
takes it from a property.

Configuration Options
*********************

* :kconfig:option:`CONFIG_MPIPE` enables the framework. Each plugin has its own
  option and contributes its own Kconfig file.
* :kconfig:option:`CONFIG_MPIPE_STRUCTURE_MAX_FIELDS` sizes one capability. Size
  it for the largest union of fields that can meet in one intersection, not for
  the number of fields an element sets, since an intersection carries through
  the fields only one side constrains.
* :kconfig:option:`CONFIG_MPIPE_NET_BUF_POOL_COUNT` sizes the shared ``net_buf``
  pool: the sum of the in-flight buffers of every pipeline.
* :kconfig:option:`CONFIG_MPIPE_BIN_MAX_CHILDREN` bounds the elements in one
  bin, which sizes the arrays used to order them during a state change.
* :kconfig:option:`CONFIG_MPIPE_THREADS_NUM`,
  :kconfig:option:`CONFIG_MPIPE_THREAD_STACK_SIZE` and
  :kconfig:option:`CONFIG_MPIPE_THREAD_DEFAULT_PRIORITY` configure the pooled
  threads a pipeline draws from.
* :kconfig:option:`CONFIG_MPIPE_WORKQUEUE` lets elements offload finite work
  items onto a shared priority work queue.
* :kconfig:option:`CONFIG_MPIPE_RPC` builds the client-side elements that run
  their processing on another core.
* :kconfig:option:`CONFIG_MPIPE_FAKE_SRC` provides a source of synthetic data,
  used to exercise a graph where a real source would need hardware.

API Reference
*************

.. doxygengroup:: mpipe_framework
