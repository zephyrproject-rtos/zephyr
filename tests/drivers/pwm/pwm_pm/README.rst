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
