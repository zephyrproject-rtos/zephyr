<!--
SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
SPDX-License-Identifier: Apache-2.0
-->

# Two-board UART communication test results

These results are from a user-supplied capture of the sender and receiver
consoles. The firmware banner was `v4.4.0-17319-gc19cbdf6fbb5`, and both
boards printed `MC UART communication test PASS`. The assigned boards are
sender `1057753351` and receiver `1057744552`.

The sample runs the `micro_core` communication cases over the physical UART
link. Protocol UART runs at 1 Mbaud; the two VCOM0 consoles run at 115200 baud.
Elapsed time and frame counters are measurements from this run, not fixed
acceptance thresholds.

## Case results

| Case | Sender | Receiver | What the case checks |
|---|---:|---:|---|
| `invalid_creation` | PASS, 1,726 us | PASS, 1,713 us | Invalid communication configurations are rejected. |
| `invalid_argument` | PASS, 1,730 us | PASS, 1,720 us | Invalid API arguments are rejected. |
| `valid_creation` | PASS, 1,553 us | PASS, 1,543 us | A valid communication object can be created. |
| `singly_direction` | PASS, 7,986,607 us | PASS, 9,314,392 us | Normal message transfer. |
| `singly_repetitive` | PASS, 175,428,392 us | PASS, 173,797,876 us | Duplicate transport frames. |
| `singly_low_lossy` | PASS, 662,325,209 us | PASS, 666,045,532 us | 20% configured fault rate and 100 ms periodic link changes. |
| `singly_high_lossy` | PASS, 133,911,575 us | PASS, 146,314,536 us | 96% configured fault rate and five-second periodic link changes. |
| `singly_timed_out` | PASS, 2,496 us | PASS, 2,498 us | Local send, receive, and flush timeout checks. |

All four transfer cases printed progress from 0% to 100% in 5% steps on
both consoles. The role banners, case names, counters, and final results were
readable in the supplied capture.

## Transport observations

The console's `Completed` fields count transport callback activity, not
application messages delivered. The `Failed` fields count injected fault
decisions. They do not separately report drops versus bit flips.

| Case and role | Completed recv/send | Failed recv/send | Throughput |
|---|---:|---:|---:|
| Normal, sender | 373 / 373 | 0 / 0 | 103.01 KBps |
| Normal, receiver | 373 / 373 | 0 / 0 | 86.09 KBps |
| Repetitive, sender | 17,317 / 8,744 | 0 / 0 | 4.57 KBps |
| Repetitive, receiver | 8,659 / 17,318 | 0 / 0 | 4.61 KBps |
| Low-loss, sender | 1,125 / 18,763 | 206 (18.31%) / 1,930 (10.29%) | 1.21 KBps |
| Low-loss, receiver | 8,920 / 12,484 | 1,719 (19.27%) / 1,312 (10.51%) | 1.21 KBps |
| High-loss, sender | 1,460 / 5,121 | 1,393 (95.41%) / 3,234 (63.15%) | 2.99 KBps |
| High-loss, receiver | 2,081 / 2,618 | 1,990 (95.63%) / 1,652 (63.10%) | 3.04 KBps |

The receive-side fault percentages are close to the configured 20% and 96%
rates. Send-side percentages are lower because duplicate physical sends
increase the `Send` denominator without adding another fault decision. The
repetitive case also shows much more transport activity and lower throughput
than the normal case, even though no faults were injected.

`singly_high_lossy` permits communication timeouts. Its `PASSED` line means
the case accepted the observed outcome; it should not be read as proof that
every attempted frame was delivered. The local `singly_timed_out` case uses
dummy callbacks rather than injecting a timeout into the physical UART link.

The capture reports no `FAILED` case and ends with the same final line on
both boards:

```text
MC UART communication test PASS
```
