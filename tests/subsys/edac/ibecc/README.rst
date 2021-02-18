.. _edac_ibecc_tests:

Testing Error Detection and Correction
######################################
Tests verify API and use error injection method to inject
errors.

Prerequisites
*************
IBECC should be enabled in BIOS. This is usually enabled in the default
BIOS configuration. Verify following:
Intel Advanced Menu -> Memory Configuration -> In-Band ECC ->  <Enabled>.
Verify also operational mode with:
Intel Advanced Menu -> Memory Configuration -> In-Band ECC Operation Mode -> 2

For injection test Error Injection should be enabled.

Error Injection
===============
IBECC includes a feature to ease the verification effort such as Error
Injection capability. This helps to test the error checking, logging and
reporting mechanism within IBECC.

In order to use Error Injection user need to use BIOS Boot Guard 0 profile.

Additionally Error Injection need to be enabled in the following BIOS menu:
Intel Advanced Menu -> Memory Configuration -> In-Band ECC Error -> <Enabled>.

Configurations
**************
Due to high security risk Error Injection capability should not be
enabled for production. Due to this reason test has production configuration
and debug configuration. The main difference is that debug configuration
includes Error Injection.
