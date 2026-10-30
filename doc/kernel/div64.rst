.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. _div64_api:

64-bit Division by Reciprocal
#############################

Most 32-bit processors have no instruction to divide a 64-bit value. When code
divides a ``uint64_t``, the compiler calls a library routine instead, such as
``__aeabi_uldivmod`` on Arm or ``__udivdi3`` on RISC-V. That routine divides in
software, using 32-bit divide instructions when the processor has them. It is
large and slow, and it is called even when the divisor is a constant like 1000.

Zephyr divides 64-bit values often: uptime in nanoseconds, cycle and tick
counts, and every conversion between them. The helpers in
:zephyr_file:`include/zephyr/sys/div64.h` replace the division with a
multiplication by a reciprocal of the divisor. The result is exact, not an
approximation.

On 64-bit targets, the helpers use plain division, which the hardware does
well.

How a reciprocal works
**********************

Dividing by 4 is the same as multiplying by 0.25. With integers only, multiply
by 25 and drop the last two decimal digits: 36 * 25 = 900, so 36 / 4 = 9.
Dropping digits is cheap, and multiplying is fast on nearly every processor,
while dividing is slow or done in software.

Most divisors have no exact reciprocal. Dividing by 3 means multiplying by
0.333..., which must be cut somewhere. Cutting it down to 0.33 does not work:
the result is then always a little too small, and falls short at every multiple
of 3. For n = 3, 3 * 33 = 99 gives 0 instead of 1.

So the last digit is rounded up instead, to 0.34. The result is now a little too
large, and the dropped digits absorb the excess. The excess grows with n, and
eventually reaches the next integer:

* Multiplying by 34 and dropping two digits gives the right quotient up to
  n = 49. For n = 50, it gives 1700 / 100 = 17, where 50 / 3 is 16.
* Multiplying by 334 and dropping three digits is right up to n = 499.
* Multiplying by 3334 and dropping four digits is right up to n = 4999.

Each extra digit of precision makes the result exact for ten times more
dividends. So for a given range of dividends, there is a precision that is
exact for all of them.

Computers do the same in binary: they multiply by a reciprocal scaled by a power
of two, then shift right, which drops binary digits. For 8-bit values, dividing
by 10 is multiplying by 205 and shifting right by 11, which divides by 2048.
2048 is the largest power of two for which the reciprocal still fits in 8 bits,
and 205 is 2048 / 10 = 204.8 rounded up, as 0.333... was rounded up to 0.34:

.. code-block:: none

   99 / 10 = (99 * 205) >> 11 = 20295 >> 11 = 9

That is exact for every value from 0 to 255. Compilers already do this for 32-bit
division by a constant. They do not do it for 64-bit dividends on 32-bit
targets, which is where these helpers come in.

For a 64-bit dividend and a 32-bit divisor, the reciprocal has 64 bits, and the
quotient is the top half of a 128-bit product, shifted right a few more bits. A
32-bit processor computes that with four 32-by-32-bit multiplications, plus
additions.

Rounding up is not exact for every 64-bit dividend with every divisor. When it
is not, the reciprocal is rounded down instead. That falls short, as with 0.33,
so the product is computed as if 1 were added to the dividend, which makes up
for it. The choice is made when the reciprocal is computed. The derivation is in
the comments of :zephyr_file:`include/zephyr/sys/div64.h`.

Dividing by a power of two needs no reciprocal: it is a shift.

Where the reciprocal comes from
*******************************

When the divisor is a constant and the build is optimized, the compiler
computes the reciprocal while building. No trace of that computation remains in
the code.

When the divisor is known only at run time, such as a clock rate read from the
hardware, :c:func:`sys_div64_recip_init` computes the reciprocal once, with a
few 32-bit divisions. Each later division by that divisor is then a
multiplication.

Benefits
********

* Size: the library division routine is about 700 bytes on Cortex-M3 and about
  1 KiB on 32-bit RISC-V. Once nothing else calls it, it is no longer linked.
  The multiplication is a shared helper of about 100 to 200 bytes, so each
  call site stays a single call.

* Speed: the library routine takes longer for larger dividends, and relies on
  slow divide instructions. On Cortex-M0, which has no divide instruction, it
  divides in software, which is slower still. The reciprocal takes nearly the
  same time for every dividend.

The table lists the instructions executed by one division, from QEMU. Both
columns divide a 64-bit value by 1000. The reciprocal column uses the shared
helper. Instruction counts are not cycle counts: a divide instruction takes up
to 12 cycles on Cortex-M3, and up to about 34 on many small RISC-V cores, while
a multiplication takes 1 to 7.

.. list-table::
   :header-rows: 1

   * - Target
     - Library division
     - Reciprocal
   * - Cortex-M0 (no divide instruction)
     - 511 to 985
     - 232 to 235
   * - Cortex-M3
     - 62 to 94, with 2 to 4 divides
     - 44, with no divide
   * - RISC-V 32-bit, with the M extension
     - 59 to 86, with 4 to 8 divides
     - 48, with no divide

With :kconfig:option:`CONFIG_SPEED_OPTIMIZATIONS`, the multiplication is inline
instead, for about 182, 24 and 35 instructions respectively.

.. note::

   Cortex-M0 and M0+ cores built with the optional small multiplier take 32
   cycles per multiplication. That reduces the gain, and can cancel it for small
   dividends.

Usage
*****

Time unit conversions such as :c:func:`k_cyc_to_ms_floor64` and
:c:func:`k_ns_to_ticks_floor64` already use these helpers when the rates are
constants, so most code gets the benefit without changes. Use them directly
when dividing a 64-bit value in your own code.

Constant divisors
=================

:c:macro:`sys_div64_u32_const` divides by a constant from 1 to ``UINT32_MAX``,
and :c:macro:`sys_mod64_u32_const` gives the remainder. Used together on the same
operands, the quotient is computed once:

.. code-block:: c

   #include <zephyr/sys/div64.h>
   #include <zephyr/sys/printk.h>

   void print_uptime(uint64_t ns)
   {
           uint64_t secs = sys_div64_u32_const(ns, 1000000000);
           uint32_t frac = sys_mod64_u32_const(ns, 1000000000);

           printk("%u.%09u\n", (uint32_t)secs, frac);
   }

:c:macro:`sys_sdiv64_u32_const` divides a signed value by a positive constant,
rounding toward zero like the C division operator.
:c:macro:`sys_div64_u32_const_inline` always inlines the multiplication, for hot
paths worth a few dozen bytes per call site.

For example, with a tick rate of 10000 Hz, this converts ticks to seconds:

.. code-block:: c

   uint64_t ticks_to_secs(uint64_t ticks)
   {
           return sys_div64_u32_const_inline(ticks, 10000);
   }

On Cortex-M3, GCC may compile it to:

.. code-block:: none

   ticks_to_secs:
           mov     r2, r1
           push    {r4, r5, lr}
           ldr     r1, .L2
           ldr     r4, .L2+4
           umull   r3, r5, r1, r0
           umull   r0, r3, r0, r4
           umlal   r0, r3, r2, r1
           movs    r1, #0
           adds    r0, r0, r5
           adc     r0, r3, #0
           umlal   r0, r1, r2, r4
           lsrs    r0, r0, #11
           orr     r0, r0, r1, lsl #21
           lsrs    r1, r1, #11
           pop     {r4, r5, pc}
   .L2:
           .word   0x3886594b
           .word   0x346dc5d6

There is no division: a plain ``ticks / 10000`` would call ``__aeabi_uldivmod``
instead.

A constant dividend gives a constant result, in a static initializer too:

.. code-block:: c

   static const uint64_t five_s_us = sys_div64_u32_const(5000000000ULL, 1000);

If the divisor is not a constant, or the build has no optimizer
(:kconfig:option:`CONFIG_NO_OPTIMIZATIONS`), these helpers fall back to plain
division. The result is still correct, only without the speedup.

Run-time divisors
=================

Set up a :c:struct:`sys_div64_recip` with :c:func:`sys_div64_recip_init` when
the divisor changes, then divide with :c:func:`sys_div64_recip_div` and
:c:func:`sys_div64_recip_mod`:

.. code-block:: c

   #include <zephyr/sys/div64.h>

   static struct sys_div64_recip counter_hz;

   void counter_rate_set(uint32_t hz)
   {
           sys_div64_recip_init(&counter_hz, hz);
   }

   uint64_t counter_to_secs(uint64_t count)
   {
           return sys_div64_recip_div(count, &counter_hz);
   }

The structure is a few words of plain data. If one thread updates it while
another divides by it, protect it like any other shared data, with a lock or
by updating it before the readers start.

:c:macro:`SYS_DIV64_RECIP_INIT` computes a reciprocal at build time, for a
constant divisor used through a pointer:

.. code-block:: c

   static const struct sys_div64_recip rates[] = {
           SYS_DIV64_RECIP_INIT(32768),
           SYS_DIV64_RECIP_INIT(19200000),
   };

Limits
======

The divisor is 32 bits. Dividing by a 64-bit value still uses the library
routine.

API reference
*************

.. doxygengroup:: sys_div64
