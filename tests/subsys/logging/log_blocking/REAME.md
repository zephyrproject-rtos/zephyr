# Blocking the Log Processing Thread

## Overview

When the core log buffer becomes full, the logging subsystem can be configured to

* Drop older log messages with `CONFIG_LOG_MODE_OVERFLOW=y` (**default**)
* Drop newer log messages with `CONFIG_LOG_MODE_OVERFLOW=n`, or
* Drop no log messages at all with `CONFIG_LOG_BLOCK_IN_THREAD=y`, `CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS=-1`.

In the last configuration, the log processing thread will block until space
becomes available again in the core log buffer.

> Warning ⚠️: Blocking the log processing thread is generally not recommended
> and should only be attempted in advanced use cases.

## Logging and Flow Rates

There are roughly 4 scenarios we care about testing with
`CONFIG_LOG_BLOCK_IN_THREAD`, and they can all be characterized by comparing
log message flow rates. Typically, one would describe log message flow rates
with units such as `[msg/s]`. However, in the table below, we are mainly
concerned with the ratio of the Output Rate to the Input Rate, and in that
case, the units themselves cancel-out. In the table we assume there exists an
`N` such that `N > 1`.

| Name           | Input Rate | Output Rate | Rate |
|----------------|------------|-------------|------|
| Input-Limited  | 1          | N           | 1    |
| Matched        | 1          | 1           | 1    |
| Output-Limited | 1          | 1/N         | 1/N  |
| Stalled        | 0          | 0           | 0    |

The resultant _Rate_ is always `Rate = MIN(Input Rate, Output Rate)`.

Rate-limiting of any kind can be described approximately as _back pressure_.
Back pressure is fine in short bursts but it can cause delays in application
and driver code if the pressure is not relieved promptly.

## Physical Sources of Backpressure

Many log backends, such as UARTs, have a built-in hardware FIFO that
inherently provides back-pressure; output log processing is rate-limited
based on the baud rate of the UART. Other backends, such as UDP sockets or
DMA, can provide significantly higher throughput but are still inherently
rate-limited by the physical layer over which they operate, be it Gigabit
Ethernet or PCI express.

Even a trivial _message source_ or _message sink_ is still rate-limited by
memory or the CPU. From that perspective, we can infer that there is a finite
limit in the log processing rate for practical systems. That may be
comforting to know, even if it is something astronomical like 1G `[msg/s]`.

## Input-Limited Log Rate

The ideal scenario is when the output "bandwidth" exceeds the input rate. If
so configured, we minimize the liklihood that the log processing thread will
stall. We can also be sure that the output will be able to relieve
backpressure (i.e. the core log buffer usage will tend to zero over time).

## Rate-Matched Input and Output

When the input rate and output rates are equal, one might think this is the
ideal scenario. In reality, it is not. The rates could be matched, but a
sustained increase (or several small increases) in the input log rate, could
cause the core log buffer to approach 100% capacity. Since the output log rate
is still only matched with the input log rate, the core log buffer capacity
would not decrease from 100%, and it would remain saturated.

Logging has a tendency to be bursty, so it is definitely preferable to
operate in the _Input-limited Log Rate_ regime.

## Output-Limited Log Rate

If the rate of output processing is less than the rate of input processing,
the core log buffer will approach 100% capacity and, eventually, stall the
log processing thread.

## Stalling the Log Processing Thread

When any log backend is unable to process logs for whatever reason,
the output rate approaches 0 `[msg/s]`. If application or
driver code continue to submit logs, the core log buffer approaches 100%
capacity. Once the core log buffer is full, the log processing thread is
unable to allocate new log messages and it will be stalled.

Stalling a real-time application produces unexpected behaviour, so it is
advised to avoid this for any non-negligible amount of time.

It is absolutely critical that the log backend is capable of operating
correctly _even when the log processing thread is blocking_ in order to
automatically recover from a stall.

On a live system, it may be necessary to manually perform remediation of log
backends that are unable to recover from stalling the log processing thread.
Remediation could involve disabling the log backend and freeing any in-use
buffers.
