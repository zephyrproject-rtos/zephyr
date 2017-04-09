Title: crypto

Description:

An example to illustrate the usage of crypto APIs.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on QEMU as follows:

    make run

--------------------------------------------------------------------------------

Troubleshooting:

Problems caused by outdated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

--------------------------------------------------------------------------------

Sample Output:

[general] [INF] main: Encryption Sample

[general] [INF] cbc_mode: CBC Mode

[general] [INF] cbc_mode: cbc mode ENCRYPT - Match

[general] [INF] cbc_mode: cbc mode DECRYPT - Match

[general] [INF] ctr_mode: CTR Mode

[general] [INF] ctr_mode: ctr mode ENCRYPT - Match

[general] [INF] ctr_mode: ctr mode DECRYPT - Match

[general] [INF] ccm_mode: CCM Mode

[general] [INF] ccm_mode: CCM mode ENCRYPT - Match

[general] [INF] ccm_mode: CCM mode DECRYPT - Match

