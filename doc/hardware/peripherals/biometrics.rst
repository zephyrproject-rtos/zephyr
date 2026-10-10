.. _biometrics_api:

Biometrics
##########

Overview
********

The biometrics API provides a common interface for enrollment, template management,
and matching across biometric modalities, including fingerprint, face, and palm.
Devices may support multiple modalities and store templates on the device or host.
Use :c:func:`biometric_get_capabilities` to query supported modalities, storage
modes, and asynchronous operations.

Enrollment can use staged sample capture or device-managed asynchronous capture
and storage. Matching supports verification against a specific template or
identification across the database, depending on the driver.

Staged Enrollment
*****************

Call :c:func:`biometric_enroll_start` with the desired template ID, then
:c:func:`biometric_enroll_capture` for each required sample. The device's
``enrollment_samples_required`` capability gives the sample count.
Call :c:func:`biometric_enroll_finalize` to complete enrollment and store the
template, or :c:func:`biometric_enroll_abort` to cancel.

Use :c:func:`biometric_match` for blocking verification or identification against
stored templates.

Asynchronous Operations
***********************

Register a callback with :c:func:`biometric_callback_set` before starting
:c:func:`biometric_match_async` or :c:func:`biometric_enroll_async`. Events report
matching results, enrollment completion, errors, and operation termination.
Matching can run once or continuously until stopped with
:c:func:`biometric_async_stop`.

Callbacks run in driver thread context and must return promptly. They must not
call control or database APIs for the same device.

See :zephyr:code-sample:`biometrics-async` for an example of enrollment and
continuous identification using shell commands.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_BIOMETRICS`
* :kconfig:option:`CONFIG_BIOMETRICS_INIT_PRIORITY`
* :kconfig:option:`CONFIG_BIOMETRICS_SHELL`

API Reference
*************

.. doxygengroup:: biometrics_interface
