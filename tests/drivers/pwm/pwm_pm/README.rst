.. SPDX-FileCopyrightText: Copyright 2026 NXP
.. SPDX-License-Identifier: Apache-2.0

PWM device power management
###########################

This test measures the output through an independent PWM capture device.
The output is inactive during suspend and resumes its saved waveform afterwards,
including after a TURN_OFF/TURN_ON sequence. Policy and system-managed scenarios
also check the output device's declared power-state constraints.

Only enabled devicetree power-state nodes participate in those checks, matching
the PM core's device-constraint table.

FRDM-MCXN947
************

Use ``frdm_mcxn947/mcxn947/cpu0`` and connect PIO2_2 (SCT0_OUT0) to
PIO2_6 (PWM1_A0). SCTimer generates the waveform; FlexPWM1 submodule 0,
channel A, measures it. Keep this connection installed for all three scenarios.
The serial console uses separate pins, PIO1_8/PIO1_9.

The policy and system-managed scenarios enable LPTMR0 as a wake-up source.
The board already selects it as the system timer companion.

The test initializes the capture counter range through the unrouted FlexPWM B
channel; channel A remains capture-only. Each successful single measurement is
explicitly disarmed before the next one.

The output-device PM and output-policy checks exercise SCTimer. FlexPWM on this
board provides capture observation without device PM callbacks or disabling-state
declarations, so the capture-device suspend and capture-policy checks are skipped.
SCTimer capture PM is outside the scope of this output-loopback setup.

Hardware execution requires the ``pwm_loopback`` fixture with the connection
described above. Build-only validation covers compilation; waveform measurements
require the wired board.
