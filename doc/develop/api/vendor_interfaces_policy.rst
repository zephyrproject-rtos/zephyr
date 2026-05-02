.. _vendor_specific_api_policy:

Vendor-Specific API Placement Policy
####################################

Overview
*********

This policy defines how vendor-specific hardware functionality must be contributed,
structured, and exposed within the Zephyr Project. It ensures that Zephyr’s public API
surface remains stable, that vendor extensions do not bypass the standard API lifecycle,
and that potential cross-vendor features have a clear path to evolve into
generic driver classes.

Vendor-specific APIs must not be exposed as public Zephyr interfaces unless they meet
the criteria required for inclusion in Zephyr’s formal API lifecycle
(experimental → stable → deprecated). Features that are inherently tied to a single
SoC or vendor must remain encapsulated within the vendor’s SoC directories.

This policy provides a consistent decision model for contributors and maintainers.

Guiding Principles
******************

- **Public Zephyr APIs must not contain vendor-specific interfaces.**

  Interfaces placed under ``include/zephyr/`` are expected to evolve through the standard
  API lifecycle. Vendor-specific interfaces that cannot satisfy this evolution
  must not be exposed as public Zephyr APIs.

- **Prefer generic solutions over vendor-specific ones.**

  When a hardware feature applies to multiple vendors or might in the future
  it should be implemented as part of an existing driver class or as a new cross-vendor
  driver class.

- **Vendor-specific features must be encapsulated.**

  Vendor-only mechanisms should live within the vendor’s platform namespace (SoCs, boards, etc.).

- **Evolution must remain possible.**

  Placement must not prevent later promotion of a vendor-specific implementation
  into a portable Zephyr driver class.

Decision Process
****************

When adding new hardware functionality or vendor-specific behavior,
contributors must follow the decision tree below, in the given order:

- **Does the feature logically extend an existing driver class?**

  If yes, extend the existing class; do not create a vendor-specific API.

- **Could the feature become a cross-vendor driver class?**

  If the hardware concept is not clearly unique to a single vendor, or
  resembles patterns likely to appear across multiple architectures,
  create a new driver class.

- **Is the functionality clearly vendor-specific and unlikely to generalize?**

  In this case, place both headers and implementations under:

  .. code-block:: none

      soc/<vendor>/..


  This ensures true encapsulation, long-term maintainability, and clean removal
  when an SoC is dropped.

  Vendor-specific code should be implemented in headers located under
  ``soc/<vendor>/include`` and code in ``soc/<vendor>/common`` or ``soc/<vendor>/<series>``.

  Such interfaces would be private and/or internal to the vendor to support their
  specific hardware and will be scrutinized in order to avoid creating feature islands specific
  to vendors.

  Applications using these interfaces, given that they are internal and/or private to the vendor,
  shall expect no guarantees as to the lifecycle or backwards compatibility.

Allowed Locations Summary
*************************

1. When extending existing driver classes

  - Where:

    .. code-block:: none

      include/zephyr/drivers/<class>.h
      drivers/<class>/

  - Use when:

    The functionality fits or extends an existing class.


2. Vendor-specific extensions to existing driver classes

  - Where:

    .. code-block:: none

      include/zephyr/drivers/<class>/
      drivers/<class>/

  - Use when:

    The feature is a vendor-specific extension to an existing class, but the core
    concept is not unique to a single vendor and could be standardized in the future.

3. Create New Driver Class (Cross-Vendor)

  - Where:

    .. code-block:: none

      include/zephyr/drivers/<class>.h
      drivers/<class>/

  - Use when:

    The feature could reasonably apply to multiple vendors or evolve into a generic abstraction.

4. Vendor-Specific, SoC-Local Code (Default for Vendor-Unique Features)

  - Where:

    Vendor-specific code should be implemented in headers located under ``soc/<vendor>/include``
    and code in ``soc/<vendor>/common`` or ``soc/<vendor>/<series>``.

  - Use when:

    The feature is unique to a vendor or SoC and is not intended as a public API.
