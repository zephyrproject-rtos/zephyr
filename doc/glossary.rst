:orphan:

.. _glossary:

Glossary of Terms
#################

From ISO/IEC 26580:2021 -

   ISO and IEC maintain terminological databases for use in
   standardization at the following addresses:

   - ISO Online browsing platform: available at https://www.iso.org/obp
   - IEC Electropedia: available at http://www.electropedia.org

   NOTE 1  For additional terms and definitions in the field of systems
   and software engineering, see ISO/IEC/IEEE 24765, which is published
   periodically as a "snapshot" of the SEVOCAB (Systems and software
   engineering - Vocabulary) database and is publicly accessible at
   https://www.computer.org/sevocab.


.. glossary::
   :sorted:

   API
      (Application Program Interface) A defined set of routines and protocols for
      building application software.

   application
      The set of user-supplied files that the Zephyr build system uses
      to build an application image for a specified board configuration.
      It can contain application-specific code, kernel configuration settings,
      and at least one CMakeLists.txt file.
      The application's kernel configuration settings direct the build system
      to create a custom kernel that makes efficient use of the board's
      resources.
      An application can sometimes be built for more than one type of board
      configuration (including boards with different CPU architectures),
      if it does not require any board-specific capabilities.

   application image
      A binary file that is loaded and executed by the board for which
      it was built.
      Each application image contains both the application's code and the
      Zephyr kernel code needed to support it. They are compiled as a single,
      fully-linked binary.
      Once an application image is loaded onto a board, the image takes control
      of the system, initializes it, and runs as the system's sole application.
      Both application code and kernel code execute as privileged code
      within a single shared address space.

   board
      A target system with a defined set of devices and capabilities,
      which can load and execute an application image. It may be an actual
      hardware system or a simulated system running under QEMU.
      The Zephyr kernel supports a :ref:`variety of boards <boards>`.

   board configuration
      A set of kernel configuration options that specify how the devices
      present on a board are used by the kernel.
      The Zephyr build system defines one or more board configurations
      for each board it supports. The kernel configuration settings that are
      specified by the build system can be over-ridden by the application,
      if desired.

   device runtime power management
      Device Runtime Power Management (PM) refers the capability of devices to
      save energy independently of the the system power state. Devices will keep
      reference of their usage and will automatically be suspended or resumed.
      This feature is enabled via the :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME`
      Kconfig option.

   idle thread
      A system thread that runs when there are no other threads ready to run.

   IDT
      (Interrupt Descriptor Table) a data structure used by the x86
      architecture to implement an interrupt vector table. The IDT is used
      to determine the correct response to interrupts and exceptions.

   ISR
      (Interrupt Service Routine) Also known as an interrupt handler, an ISR
      is a callback function whose execution is triggered by a hardware
      interrupt (or software interrupt instructions) and is used to handle
      high-priority conditions that require interrupting the current code
      executing on the processor.

   kernel
      The set of Zephyr-supplied files that implement the Zephyr kernel,
      including its core services, device drivers, network stack, and so on.

   power domain
      A power domain is a collection of devices for which power is
      applied and removed collectively in a single action. Power
      domains are represented by :c:struct:`device`.

   power gating
      Power gating reduces power consumption by shutting off areas of an
      integrated circuit that are not in use.

   SoC
      `System on a chip`_

   system power state
      System power states describe the power consumption of the system as a
      whole. System power states are are represented by :c:enum:`pm_state`.

   west
      A multi-repo meta-tool developed for the Zephyr project. See :ref:`west`.

   west installation
      An obsolete term for a :term:`west workspace` used prior to west 0.7.

   west manifest
      A YAML file, usually named :file:`west.yml`, which describes projects, or
      the Git repositories which make up a :term:`west workspace`, along with
      additional metadata. See :ref:`west-basics` for general information
      and :ref:`west-manifests` for details.

   west manifest repository
      The Git repository in a :term:`west workspace` which contains the
      :term:`west manifest`. Its location is given by the :ref:`manifest.path
      configuration option <west-config-index>`. See :ref:`west-basics`.

   west workspace
      A directory on your system with a :file:`.west` subdirectory and
      a :term:`west manifest repository`. You clone the Zephyr source
      code onto your system by creating a west workspace using the
      ``west init`` command. See :ref:`west-basics`.

   XIP
      (eXecute In Place) a method of executing programs directly from long
      term storage rather than copying it into RAM, saving writable memory for
      dynamic data and not the static program code.



   application architecture
      architecture including the architectural structure and rules
      (e.g. common rules and constraints) that constrains a specific
      :term:`member product` within a :term:`product line`
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

      Note 1 to entry: The application architecture captures the
      high-level design of a specific :term:`member product` of as
      :term:`product line`.  An application architecture of the member
      products included in the product line reuses (possibly with
      modification) the common parts and binds variable parts of the
      :term:`domain architecture`.  In most cases, an application
      architecture of the member products needs to develop
      application-specific variability.

   application asset
      output of a specific :term:`application engineering` process
      (e.g. application realization) that may be exploited in other
      lifecycle processes of application engineering and may be adapted
      as a :term:`domain asset` based on a product management decision
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

      Note 1 to entry: Application asset encompasses requirements,
      an architectural design, components, and tests.  In contrast to
      domain assets that need to support the mass-customization of
      multiple applications within the product line, most application
      assets do not contain variability.  However, applications may
      possess variability (e.g. end-users may be enabled to
      mass-customize the applications they are using by binding
      application variability during run time).  Application Assets
      may thus possess variability as well, but the variability of
      an application asset only serves the purposeses of the particular
      application, for which the application asset has been created.
      As a result, the scope of application asset variability is typically
      much narrower than the scope of domain asset variability.

      Note 2 to entry: Application assets are not physical products
      available off-the-shelf and ready for commissioning.  Physical
      products (e.g. mechanical parts, electronic components, harnesses,
      optic lenses) are stored and managed according to the best practices
      of their respective disciplines.  Application assets have their own
      life cycles; ISO/IEC/IEEE 15288 may be used to manage a life cycle.

   application design
      process of application engineering where a single
      :term:`application architecture` conforming to the
      :term:`domain architecture` is derived
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

   application engineering
      life cycle consisting of a set of processes in which the
      :term:`application assets <application asset>` and
      :term:`member products <member product>` of the
      :term:`product line` are implemented and
      managed by reusing :term:`domain assets <domain asset>` in
      conformance to the :term:`domain architecture` and by binding the
      :term:`variability` of the platform
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

      Note 1 to entry: Application engineering the the traditional
      sense means the development of single products without the
      strategic reuse of domain assets and without explicit variability
      modeling and binding.

   application realization
      process of :term:`application engineering` that develops
      :term:`application assets <application asset>`, some of which
      may be derived from :term:`domain assets <domain asset>`, and
      :term:`member products <member product>` based on the
      :term:`application architecture` and the sets of application
      assets and domain assets
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

   application variability model
      variability model for a particular application including
      variability binding results, application specifically modified
      variability, and application specifically added variability
      *(from ISO/IEC 26558:2017, Software and systems engineering -
      Methods and tools for variability modelling in software and
      systems product line)*

   asset base
      reusable assets produced from both domain and application
      engineering
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

   asset scoping
      process of identifying the potential
      :term:`domain assets <domain asset>` and
      estimating the returns of investments in the assets
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

      Note 1 to entry: Information produced during asset scoping,
      together with the information produced by :term:`product scoping`
      and :term:`domain scoping`, can be used to determine whether to
      introduce a :term:`product line` into an organization.  Asset
      scoping takes place after domain scoping.

   bill-of-features
      specification for a :term:`member product` in the
      :term:`product line`, rendered in terms of the specific
      :term:`features <feature>` from the :term:`feature catalogue`
      that are chosen for that member product
      *(from ISO/IEC 26580:2021, Software and systems engineering -
      Methods and tools for the feature-based approach to software
      and systems product line engineering)*

   bill-of-features portfolio
      collection comprising the :term:`bill-of-features` for each
      :term:`member product` in a :term:`product line`
      *(from ISO/IEC 26580:2021, Software and systems engineering -
      Methods and tools for the feature-based approach to software
      and systems product line engineering)*

   binding
      task to make a decision on relevant :term:`variants <variant>`,
      which will be :term:`application assets <application asset>`,
      from :term:`domain assets <domain asset>` using the
      :term:`domain variability model` and from application assets
      using the :term:`application variability model`
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

      Note 1 to entry: Performing the binding is a task to apply the
      binding definition to generate new application from domain and
      application assets using the domain and application variability
      models.

   commonality
      set of functional and non-functional characteristics that is
      shared by all applications belonging to the :term:`product line`
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

   domain architecture
   reference architecture
   product line architecture
      core architecture that captures the high-level design of a
      software and systems :term:`product line` including the
      architectural structure and texture (e.g. common rules and
      constraints) that constrains all
      :term:`member products <member product>` within a software and
      systems product line
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

      Note 1 to entry:
      :term:`Application architectures <application architecture>` of the
      member products included in the product line reuse
      (possibly with modifications) the common parts and bind
      variable parts of the :term:`domain architecture`.
      Application architectures of the member products may
      (but do not need to) provide :term:`variability`.

   domain asset
   core asset
      output of :term:`domain engineering` life cycle processes and
      can be reused in producing products during
      :term:`application engineering`
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

      Note 1 to entry: Domain assets may include domain features,
      domain models, domain requirements specifications, domain
      architectures, domain components, domain test cases, domain
      process descriptions, and other assets.

      Note 2 to entry: In systems engineering, domain assets may be
      subsystems or components to be reused in further system designs.
      Domain assets are considered through their original requirements
      and technical characteristics.  Domain assets include but are not
      limited to use cases, logical principles, environmental behavioral
      data, and risks or opportunities learnt from previous projects.
      Domain assets are not physical products available off-the-shelf
      and ready for commissioining.  Physical products (e.g. mechanical
      parts, electronic components, harnesses, optic lenses) are stored
      and managed according to the best practices of their respective
      disciplines.  Domain assets have their own life cycles.
      ISO/IEC/IEEE 15288 may be used to manage a life cycle.

   domain engineering
      life cycle consisting of a set of processes for specifying and
      managing the :term:`commonality` and :term:`variability` of a
      :term:`product line`
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

      Note 1 to entry: :term:`Domain assets <domain asset>` are developed
      and managed in domain engineering processes and are reused in
      :term:`application engineering` processes.

      Note 2 to entry: Depending on the type of the domain asset,
      that is, a system domain asset or a software domain asset,
      the engineering processes to be used may be determined by the
      relevant discipline.

      Note 3 to entry: IEE 1517-2010, Clause 3 defines domain engineering
      as a reuse-based approach to defining the scope
      (i.e., domain definition), specifying the structure
      (i.e., domain architecture), and building the assets
      (e.g. requirements, designs, software code, documentation)
      for a class of systems, subsystems, or member products.

   domain scoping
      subprocess for identifying and bounding the functional domains
      that are important to an envisioned :term:`product line` and
      provide sufficient reuse potential to justify the product line
      creation
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

   domain supersets
      collection comprising the :term:`feature catalogue` and
      :term:`shared asset supersets <shared asset superset>`
      *(from ISO/IEC 26580:2021, Software and systems engineering -
      Methods and tools for the feature-based approach to software
      and systems product line engineering)*

   domain variability model
      explicit definition of product line varability
      *(from ISO/IEC 26558:2017, Software and systems engineering -
      Methods and tools for variability modelling in software and
      systems product line)*

   feature
      abstract functional characteristic of a system of interest
      that end-uses and other stakeholders can understand

      Note 1 to entry: In systems engineering, features are syntheses
      of the needs of the stakeholders.  These features will be used,
      amongst others, to build the technical requirement baselines.
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

   feature catalogue
      model of the collection of all the :term:`feature` options and
      :term:`feature constraints <feature constraint>` available across
      the entire :term:`product line`
      *(from ISO/IEC 26580:2021, Software and systems engineering -
      Methods and tools for the feature-based approach to software
      and systems product line engineering)*

   feature constraint
      formal relationship between two or more :term:`features <feature>`
      that is necessarily satisfied for all
      :term:`member products <member product>`
      *(from ISO/IEC 26580:2021, Software and systems engineering -
      Methods and tools for the feature-based approach to software
      and systems product line engineering)*

   feature language
      syntax and semantics for the formal representation, structural
      taxonomy, and relationships among the concepts and constructs
      in the :term:`feature catalogue`, :term:`bill-of-features portfolio`,
      and :term:`shared asset superset`
      :term:`variation points <variation point>`
      *(from ISO/IEC 26580:2021, Software and systems engineering -
      Methods and tools for the feature-based approach to software
      and systems product line engineering)*

   member product
   application  (ISO/IEC 26550:2015)
      product belonging to the product line
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

   product line
   product family
      set of products and/or services sharing explicitly defined and
      managed common and variable features and relying on the same
      :term:`domain architecture` to meet the :term:`common <commonality>`
      and :term:`variable <variability>` needs of specific markets
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

   product line platform
      :term:`product line architecture`, a configuration management
      plan, and :term:`domain assets <domain asset>` enabling
      :term:`application engineering` to effectively reuse and produce
      a set of derivative products
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

      Note 1 to entry: Platforms have their own life cycles.
      ISO/IEC/IEEE 15288 may be used to manage a life cycle.

   product line reference model
      abstract representation of the domain and
      :term:`application engineering` life cycle processes, the roles and
      relationships of the processes, and the assets produced, managed,
      and used during :term:`product line` engineering and management
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

   product line scoping
      process for defining the :term:`member products <member product>`
      that will be produced within a :term:`product line` and the major
      common and variable :term:`features <feature>` among the products,
      analyzes the products from an economic point of view, and controls
      and schedules the development, production, and marketing of the
      product line and its products
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

      Note 1 to entry: Product management is primarily responsible for
      :term:`product line scoping`.

   product scoping
      subprocess of :term:`product line scoping` that determines the
      product roadmap, that is (1) the targeted markets; (2) the
      product categories that the product line organization should be
      developing, producing, marketing, and selling; (3) the common
      and variable :term:`features <feature>` that the products should
      provide in order to reach the long and short term business
      objectives of the product line organization, and (4) the schedule
      for introducing products to markets
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

   shared asset
      software and systems engineering lifecycle digital artefacts that
      compose a part of a delivered :term:`member product` or support
      the engineering process to create and maintain a member product
      *(from ISO/IEC 26580:2021, Software and systems engineering -
      Methods and tools for the feature-based approach to software
      and systems product line engineering)*

      Note 1 to entry: A shared asset is analogous to a domain asset
      (ISO/IEC 26550)

      Note 2 to entry: Typical shared assets are requirements, design
      specifications or models for mechanical, electrical and software,
      source code, build files or scripts, test plans and test cases,
      user documentation, repair manuals and installation guides, project
      budgets, schedules, and work plans, product calibration and
      configuration files, mechanical bills-of-materials, electrical
      circuit board and wiring harness designs, engineering management
      plans, engineering drawings, training plans and training materials,
      skill set requirements, manufacturing plans and instructions, and
      shipping manifests.

   shared asset superset
      representation of a :term:`shared asset` that includes all content
      needed by any of the :term:`member products <member product>`
      *(from ISO/IEC 26580:2021, Software and systems engineering -
      Methods and tools for the feature-based approach to software
      and systems product line engineering)*

   variability
      characteristics that can differ among members of the
      :term:`product line`
      *(from ISO/IEC 26550:2015, Software and systems engineering -
      Reference model for product line engineering and management)*

   variant
      alternative that can be used to realize a particular
      :term:`variation point`
      *(from ISO/IEC 26580:2021, Software and systems engineering -
      Methods and tools for the feature-based approach to software
      and systems product line engineering)*

      [SOURCE: ISO/IEC 26550:2015, 3.28, modified - the word "one" at
      the beginning of the definition has been removed; "may" has been
      changed to "can"; "particular varation points" has been changed to
      "a particular variation point"; note 1 to entry has been removed.]

   variation point
      identification of a specific piece of :term:`shared asset superset`
      content and a mapping from :term:`feature` selection(s) to the form
      of the content that appears in a product asset instance
      *(from ISO/IEC 26580:2021, Software and systems engineering -
      Methods and tools for the feature-based approach to software
      and systems product line engineering)*

      Note 1 to entry: In [ISO/IEC 26580], all features express
      characteristics that differ among
      :term:`member products <member product>` which according to
      ISO/IEC 26550 would also make every feature a variation
      point.  To avoid this redundancy, [ISO/IEC 26580] does not call out
      features as variation points.

      Note 2 to entry: See [ISO/IEC 26580] Annex A for the definition
      of this term in ISO/IEC 26550. The definition in [ISO/IEC 26580]
      is more specific to feature-based PLE and the
      PLE factory configurator approach of producing product
      asset instances from shared asset supersets.


.. _System on a chip: https://en.wikipedia.org/wiki/System_on_a_chip
