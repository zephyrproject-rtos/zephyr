.. _api_overview:

API Overview
############

The table lists Zephyr's APIs and information about them, including their
current :ref:`stability level <api_lifecycle>`.  More details about API changes
between major releases are available in the :ref:`zephyr_release_notes`.

The version column uses `semantic version <https://semver.org/>`_, and has the
following expectations:

 * Major version zero (0.y.z) is for initial development. Anything MAY
   change at any time. The public API SHOULD NOT be considered stable.

   * If minor version is up to one (0.1.z), API is considered
     :ref:`experimental <api_lifecycle_experimental>`.
   * If minor version is larger than one (0.y.z | y > 1), API is considered
     :ref:`unstable <api_lifecycle_unstable>`.

 * Version 1.0.0 defines the public API. The way in which the version number
   is incremented after this release is dependent on this public API and how it
   changes.

   * APIs with major versions equal or larger than one (x.y.z | x >= 1 ) are
     considered :ref:`stable <api_lifecycle_stable>`.
   * All existing stable APIs in Zephyr will be start with version 1.0.0.

 * Patch version Z (x.y.Z | x > 0) MUST be incremented if only backwards
   compatible bug fixes are introduced. A bug fix is defined as an internal
   change that fixes incorrect behavior.

 * Minor version Y (x.Y.z | x > 0) MUST be incremented if new, backwards
   compatible functionality is introduced to the public API. It MUST be
   incremented if any public API functionality is marked as deprecated. It MAY
   be incremented if substantial new functionality or improvements are
   introduced within the private code. It MAY include patch level changes.
   Patch version MUST be reset to 0 when minor version is incremented.

 * Major version X (x.Y.z | x > 0) MUST be incremented if a compatibility
   breaking change was made to the API.

.. note::
   Version for existing APIs are initially set based on the current state of the
   APIs:

    - 0.1.0 denotes an :ref:`experimental <api_lifecycle_experimental>` API
    - 0.8.0 denote an :ref:`unstable <api_lifecycle_unstable>` API,
    - and finally 1.0.0 indicates a :ref:`stable <api_lifecycle_stable>` APIs.

   Changes to APIs in the future will require adapting the version following the
   guidelines above.


.. api-overview-table::
