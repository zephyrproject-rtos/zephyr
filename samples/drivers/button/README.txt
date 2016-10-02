Title: Button demo

Description:

A simple button demo showcasing the use of GPIO input with interrupts.

The demo assumes that a push button is connected to one of GPIO
lines. The sample code is configured to work on boards with user defined
buttons and that  have defined the SW0_* variable in board.h

After startup, the program looks up a predefined GPIO device,
and configures the pin in input mode, enabling interrupt generation on
falling edge. During each iteration of the main loop, the state of
GPIO line is monitored and printed to the serial console. When the
input button gets pressed, the interrupt handler will print an
information about this event along with its timestamp.

--------------------------------------------------------------------------------

Building and Running Project:

It can be built for a nucleo_f103rb board as follows:

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
