.. _os_services:

Services
########

Zephyr provides a comprehensive set of subsystems and associated services that applications can
leverage. These services provide standardized APIs that abstract hardware implementation details,
enabling application portability across different platforms.

.. grid:: 1 1 2 2
   :gutter: 3

   .. grid-item-card:: :material-twotone:`memory;36px` System
      :link: system_services
      :link-type: ref

      Memory management, low-level infrastructure, and other utility APIs.

   .. grid-item-card:: :material-twotone:`device_hub;36px` Connectivity
      :link: connectivity_services
      :link-type: ref

      Networking, Bluetooth, USB, and other connectivity services.

   .. grid-item-card:: :material-twotone:`bug_report;36px` Observability & Debugging
      :link: observability_services
      :link-type: ref

      Logging, tracing, shell, and other debugging and observability tools.

   .. grid-item-card:: :material-twotone:`import_export;36px` Input / Output
      :link: io_services
      :link-type: ref

      Console, input devices abstraction, RTIO, and other I/O utilities.

   .. grid-item-card:: :material-twotone:`save;36px` Storage & Configuration
      :link: storage_services
      :link-type: ref

      Flash access, file systems, key-value stores, and persistent data.

   .. grid-item-card:: :material-twotone:`battery_charging_80;36px` Power Management
      :link: power_management_services
      :link-type: ref

      Energy consumption, performance scaling, and resource lifecycle control.

   .. grid-item-card:: :material-twotone:`chat_bubble;36px` IPC & Messaging
      :link: messaging_services
      :link-type: ref

      Inter-thread, inter-processor, and external communication mechanisms.

   .. grid-item-card:: :material-twotone:`security;36px` Security & Identity
      :link: security_services
      :link-type: ref

      Cryptographic primitives, trusted execution environments, and identity services.

   .. grid-item-card:: :material-twotone:`data_object;36px` Algorithms
      :link: algorithms_services
      :link-type: ref

      Algorithms for data transformation, encoding, and validation.

   .. grid-item-card:: :material-twotone:`layers;36px` Portability
      :link: osal
      :link-type: ref

      OS abstraction layers such as POSIX or CMSIS RTOS.

   .. grid-item-card:: :material-twotone:`widgets;36px` Application Services
      :link: application_framework_services
      :link-type: ref

      High-level frameworks and libraries for application development.

.. toctree::
   :maxdepth: 2
   :hidden:

   system
   connectivity/index
   observability
   io
   storage/index
   power_management
   messaging
   security
   algorithms
   portability/index
   application_services
