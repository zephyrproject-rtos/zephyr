Title: Power management hooks template

Description:

A template app that defines the power management hooks and
enables the power management related CONFIG flags.  This
app will enable build testing of power management code inside
the CONFIG flags.

This project is intended only for build testing.  For running
real PM tests use other applications that are full implementations
specific to SOCs

--------------------------------------------------------------------------------

Building Project:

    make pristine && make
