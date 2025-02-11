# Xtensa MMU Operation

As with other elements of the architecture, paged virtual memory
management on Xtensa is somewhat unique.  And there is similarly a
lack of introductory material available.  This document is an attempt
to introduce the architecture at an overview/tutorial level, and to
describe Zephyr's specific implementation choices.

## General TLB Operation

The Xtensa MMU operates on top of a fairly conventional TLB cache.
The TLB stores virtual to physical translation for individual pages of
memory.  It is partitioned into an automatically managed
4-way-set-associative bank of entries mapping 4k pages, and 3-6
"special" ways storing mappings under OS control.  Some of these are
for mapping pages larger than 4k, which Zephyr does not directly
support.  A few are for bootstrap and initialization, and will be
discussed below.

Like the L1 cache, the TLB is split into separate instruction and data
entries.  Zephyr manages both as needed, but symmetrically.  The
architecture technically supports separately-virtualized instruction
and data spaces, but the hardware page table refill mechanism (see
below) does not, and Zephyr's memory spaces are unified regardless.

The TLB may be loaded with permissions and attributes controlling
cacheability, access control based on ring (i.e. the contents of the
RING field of the PS register) and togglable write and execute access.
Memory access, even with a matching TLB entry, may therefore create
Kernel/User exceptions as desired to enforce permissions choices on
userspace code.

Live TLB entries are tagged with an 8-bit "ASID" value derived from
their ring field of the PTE that loaded them, via a simple translation
specified in the RASID special register.  The intent is that each
non-kernel address space will get a separate ring 3 ASID set in RASID,
such that you can switch between them without a TLB flush.  The ASID
value of ring zero is fixed at 1, it may not be changed.  (An ASID
value of zero is used to tag an invalid/unmapped TLB entry at
initialization, but this mechanism isn't accessible to OS code except
in special circumstances, and in any case there is already an invalid
attribute value that can be used in a PTE).

## Virtually-mapped Page Tables

Xtensa has a unique (and, to someone exposed for the first time,
extremely confusing) "page table" format.  The simplest was to begin
to explain this is just to describe the (quite simple) hardware
behavior:

On a TLB miss, the hardware immediately does a single fetch (at ring 0
privilege) from RAM by adding the "desired address right shifted by
10 bits with the bottom two bits set to zero" (i.e. the page frame
number in units of 4 bytes) to the value in the PTEVADDR special
register.  If this load succeeds, then the word is treated as a PTE
with which to fill the TLB and use for a (restarted) memory access.
This is extremely simple (just one extra hardware state that does just
one thing the hardware can already do), and quite fast (only one
memory fetch vs. e.g. the 2-5 fetches required to walk a page table on
x86).

This special "refill" fetch is otherwise identical to any other memory
access, meaning it too uses the TLB to translate from a virtual to
physical address.  Which means that the page tables occupy a 4M region
of virtual, not physical, address space, in the same memory space
occupied by the running code.  The 1024 pages in that range (not all
of which might be mapped in physical memory) are a linear array of
1048576 4-byte PTE entries, each describing a mapping for 4k of
virtual memory.  Note especially that exactly one of those pages
contains the 1024 PTE entries for the 4M page table itself, pointed to
by PTEVADDR.

Obviously, the page table memory being virtual means that the fetch
can fail: there are 1024 possible pages in a complete page table
covering all of memory, and the ~16 entry TLB clearly won't contain
entries mapping all of them.  If we are missing a TLB entry for the
page translation we want (NOT for the original requested address, we
already know we're missing that TLB entry), the hardware has exactly
one more special trick: it throws a TLB Miss exception (there are two,
one each for instruction/data TLBs, but in Zephyr they operate
identically).

The job of that exception handler is simply to ensure that the TLB has
an entry for the page table page we want.  And the simplest way to do
that is to just load the faulting PTE as an address, which will then
go through the same refill process above.  This second TLB fetch in
the exception handler may result in an invalid/inapplicable mapping
within the 4M page table region.  This is an typical/expected runtime
fault, and simply indicates unmapped memory.  The result is TLB miss
exception from within the TLB miss exception handler (i.e. while the
EXCM bit is set).  This will produce a Double Exception fault, which
is handled by the OS identically to a general Kernel/User data access
prohibited exception.

After the TLB refill exception, the original faulting instruction is
restarted, which retries the refill process, which succeeds in
fetching a new TLB entry, which is then used to service the original
memory access.  (And may then result in yet another exception if it
turns out that the TLB entry doesn't permit the access requested, of
course.)

## Special Cases

The page-tables-specified-in-virtual-memory trick works very well in
practice.  But it does have a chicken/egg problem with the initial
state.  Because everything depends on state in the TLB, something
needs to tell the hardware how to find a physical address using the
TLB to begin the process.  Here we exploit the separate
non-automatically-refilled TLB ways to store bootstrap records.

First, note that the refill process to load a PTE requires that the 4M
space of PTE entries be resolvable by the TLB directly, without
requiring another refill.  This 4M mapping is provided by a single
page of PTE entries (which itself lives in the 4M page table region!).
This page must always be in the TLB.

Thankfully, for the data TLB Xtensa provides 3 special/non-refillable
ways (ways 7-9) with at least one 4k page mapping each.  We can use
one of these to "pin" the top-level page table entry in place,
ensuring that a refill access will be able to find a PTE address.

But now note that the load from that PTE address for the refill is
done in an exception handler.  And running an exception handler
requires doing a fetch via the instruction TLB.  And that obviously
means that the page(s) containing the exception handler must never
require a refill exception of its own.

Ideally we would just pin the vector/handler page in the ITLB in the
same way we do for data, but somewhat inexplicably, Xtensa does not
provide 4k "pinnable" ways in the instruction TLB (frankly this seems
like a design flaw).

Instead, we load ITLB entries for vector handlers via the refill
mechanism using the data TLB, and so need the refill mechanism for the
vector page to succeed always.  The way to do this is to similarly pin
the page table page containing the (single) PTE for the vector page in
the data TLB, such that instruction fetches always find their TLB
mapping via refill, without requiring an exception.

## Initialization

Unlike most other architectures, Xtensa does not have a "disable" mode
for the MMU.  Virtual address translation through the TLB is active at
all times.  There therefore needs to be a mechanism for the CPU to
execute code before the OS is able to initialize a refillable page
table.

The way Xtensa resolves this (on the hardware Zephyr supports, see the
note below) is to have an 8-entry set ("way 6") of 512M pages able to
cover all of memory.  These 8 entries are initialized as valid, with
attributes specifying that they are accessible only to an ASID of 1
(i.e. the fixed ring zero / kernel ASID), writable, executable, and
uncached.  So at boot the CPU relies on these TLB entries to provide a
clean view of hardware memory.

But that means that enabling page-level translation requires some
care, as the CPU will throw an exception ("multi hit") if a memory
access matches more than one live entry in the TLB.  The
initialization algorithm is therefore:

0. Start with a fully-initialized page table layout, including the
   top-level "L1" page containing the mappings for the page table
   itself.

1. Ensure that the initialization routine does not cross a page
   boundary (to prevent stray TLB refill exceptions), that it occupies
   a separate 4k page than the exception vectors (which we must
   temporarily double-map), and that it operates entirely in registers
   (to avoid doing memory access at inopportune moments).

2. Pin the L1 page table PTE into the data TLB.  This creates a double
   mapping condition, but it is safe as nothing will use it until we
   start refilling.

3. Pin the page table page containing the PTE for the TLB miss
   exception handler into the data TLB.  This will likewise not be
   accessed until the double map condition is resolved.

4. Set PTEVADDR appropriately.  The CPU state to handle refill
   exceptions is now complete, but cannot be used until we resolve the
   double mappings.

5. Disable the initial/way6 data TLB entries first, by setting them to
   an ASID of zero.  This is safe as the code being executed is not
   doing data accesses yet (including refills), and will resolve the
   double mapping conditions we created above.

6. Disable the initial/way6 instruction TLBs second.  The very next
   instruction following the invalidation of the currently-executing
   code page will then cause a TLB refill exception, which will work
   normally because we just resolved the final double-map condition.
   (Pedantic note: if the vector page and the currently-executing page
   are in different 512M way6 pages, disable the mapping for the
   exception handlers first so the trap from our current code can be
   handled.  Currently Zephyr doesn't handle this condition as in all
   reasonable hardware these regions will be near each other)

Note: there is a different variant of the Xtensa MMU architecture
where the way 5/6 pages are immutable, and specify a set of
unchangable mappings from the final 384M of memory to the bottom and
top of physical memory.  The intent here would (presumably) be that
these would be used by the kernel for all physical memory and that the
remaining memory space would be used for virtual mappings.  This
doesn't match Zephyr's architecture well, as we tend to assume
page-level control over physical memory (e.g. .text/.rodata is cached
but .data is not on SMP, etc...).  And in any case we don't have any
such hardware to experiment with.  But with a little address
translation we could support this.

## ASID vs. Virtual Mapping

The ASID mechanism in Xtensa works like other architectures, and is
intended to be used similarly.  The intent of the design is that at
context switch time, you can simply change RADID and the page table
data, and leave any existing mappings in place in the TLB using the
old ASID value(s).  So in the common case where you switch back,
nothing needs to be flushed.

Unfortunately this runs afoul of the virtual mapping of the page
refill: data TLB entries storing the 4M page table mapping space are
stored at ASID 1 (ring 0), they can't change when the page tables
change!  So this region naively would have to be flushed, which is
tantamount to flushing the entire TLB regardless (the TLB is much
smaller than the 1024-page PTE array).

The resolution in Zephyr is to give each ASID its own PTEVADDR mapping
in virtual space, such that the page tables don't overlap.  This is
expensive in virtual address space: assigning 4M of space to each of
the 256 ASIDs (actually 254 as 0 and 1 are never used by user access)
would take a full gigabyte of address space.  Zephyr optimizes this a
bit by deriving a unique sequential ASID from the hardware address of
the statically allocated array of L1 page table pages.

Note, obviously, that any change of the mappings within an ASID
(e.g. to re-use it for another memory domain, or just for any runtime
mapping change other than mapping previously-unmapped pages) still
requires a TLB flush, and always will.

## SMP/Cache Interaction

A final important note is that the hardware PTE refill fetch works
like any other CPU memory access, and in particular it is governed by
the cacheability attributes of the TLB entry through which it was
loaded.  This means that if the page table entries are marked
cacheable, then the hardware TLB refill process will be downstream of
the L1 data cache on the CPU.  If the physical memory storing page
tables has been accessed recently by the CPU (for a refill of another
page mapped within the same cache line, or to change the tables) then
the refill will be served from the data cache and not main memory.

This may or may not be desirable depending on access patterns.  It
lets the L1 data cache act as a "L2 TLB" for applications with a lot
of access variability.  But it also means that the TLB entries end up
being stored twice in the same CPU, wasting transistors that could
presumably store other useful data.

But it is also important to note that the L1 data cache on Xtensa is
incoherent!  The cache being used for refill reflects the last access
on the current CPU only, and not of the underlying memory being
mapped.  Page table changes in the data cache of one CPU will be
invisible to the data cache of another.  There is no simple way of
notifying another CPU of changes to page mappings beyond doing
system-wide flushes on all cpus every time a memory domain is
modified.

The result is that, when SMP is enabled, Zephyr must ensure that all
page table mappings in the system are set uncached.  The OS makes no
attempt to bolt on a software coherence layer.
