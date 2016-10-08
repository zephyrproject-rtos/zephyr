Title: Disco demo

Description:

A simple 'disco' demo. The demo assumes that 2 LEDs are connected to
GPIO outputs of the MCU/board. The sample code is configured to work
on Nucleo-64 F103RB board, with LEDs connected to PB5 and PB8
pins.

After startup, the program looks up a predefined GPIO device (GPIOB),
and configures pins 5 and 8 in output mode. During each iteration of
the main loop, the state of GPIO lines will be changed so that one of
the lines is in high state, while the other is in low, thus switching
the LEDs on and off in an alternating pattern.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project does not output to the console, but instead
causes two LEDs connected to the GPIO device to blink in an
alternating pattern. It can be built for a nucleo_f103rb board as
follows:

    make

The code may need adaption before running the code on another board.

--------------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

--------------------------------------------------------------------------------
