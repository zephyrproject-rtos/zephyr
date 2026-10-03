# Problem Description

It is the nature of heap allocated memory to cross execution-context boundaries and outlive the
function that created them.
A simple malloc/free model cannot express shared ownership of memory blocks. This limitation makes
it cumbersome to write code that interacts with shared memory between different execution contexts
and modules.

By enabling shared ownership of heap allocations, memory can be passed between subsystems without copying.
As asynchronous programming patterns, such as message passing, often require multiple consumers to
hold references to the same memory area safely.
Shared owernship becomes essential, as it reduces code complexity and memory usage significantly.
Multiple consumers can safely access the same memory as long as each signals its ownership claim, and the
memory is reclaimed automatically once all consumers have released their claim.

# Proposed Change (Summary)

The `memref` API introduces a thin abstraction layer between the user and the memory allocator.
It provides a standard interface for incrementing (`ref`) and decrementing (`unref`) reference
counts, while allowing the underlying memory to be managed by a pluggable backend.
This allows for shared ownership of memory blocks without embedding refcounting logic into the
allocator itself.

# Proposed Change (Detailed)

This proposal introduces a small, explicit shared-memory abstraction (`memref`) that provides
reference-counted lifetime management on top of existing memory backends.
The intent is to enable shared ownership of dynamically allocated memory across asynchronous
execution contexts, without imposing the refcounting mechanism on a memory allocator explicitly.

The idea is that this abstraction can be hidden behind higher-level APIs in the application
developer's code, to provide a clean and easy-to-use interface for shared memory management.

## Introduction of a control block for lifetime management

Each allocated memory area is prepended by a private control block that tracks the memory's metadata.
The control block contains an atomic reference counter, an optional user-provided `free` callback,
and a pointer to the backend the allocation was made from.
This metadata is internal to the `memref` implementation and is not visible to consumers. The
payload offset is rounded up so the user-visible pointer keeps the backend's alignment guarantee.

```c
typedef void (*memref_free_cb_t)(void *ptr);
struct control_block {
	atomic_t ref_count;
	memref_free_cb_t free_cb;
	const struct memref_backend *backend;
};
```

## Backend-agnostic allocation and deallocation

Memory allocation and deallocation are delegated to a backend via the `memref_backend`
interface. This allows memref to operate on top of different allocation strategies (including
heap-based allocators, memory pools, and/or slab allocators) without embedding allocator-specific
assumptions into the `memref` API.
The backend context pointer is passed through unchanged, preserving flexibility and enabling
integration with existing memory management systems.
The backend *must* remain valid for the entire lifetime of all memref allocations made through it.
However, this also means that the `memref` instance inherits any limitations of the underlying
backend, such as ISR safety.

The allocation routine (`memref_alloc`) requests a contiguous block large enough to store both the
control block (including padding for alignment) and the user-visible memory.
Size addition overflow is rejected with NULL. On success, the reference count is initialized to one
and the backend pointer is stored, establishing clear initial ownership of the memory block.
The returned pointer refers only to the user-visible memory region, ensuring that callers cannot
accidentally access or modify the control metadata.

The deallocation routine (`memref_unref`) decrements the reference count and checks if it has
reached zero. If so, it invokes the optional user-provided free callback before releasing the memory
back to the stored backend.
This ensures that any additional resources associated with the memory are properly cleaned up before
the memory is reclaimed.

The user *must* ensure that the backend used is compatible with the intended execution contexts.
For example, `memref_unref` must not be called from ISR unless the backend and the optional
destructor are explicitly documented as ISR-safe.

## Explicit reference acquisition and release semantics

The API introduces two explicit lifetime-management operations: `memref_ref` and `memref_unref`.
Calling `memref_ref` increments the atomic reference counter, signaling that a new execution context
has acquired ownership of the memory.
Calling `memref_unref` decrements the reference counter, releasing ownership from the caller context.

When the reference count reaches zero, a final destruction is triggered if it was supplied at
allocation time. It is invoked prior to releasing the underlying memory.
This callback allows users to perform struct-specific teardown such as releasing additional dynamic
resources connected to the memory before it is reclaimed.

```c
void broadcast_to_consumers(struct message *msg) {
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        consumer[i].cb(msg); // consumer can choose to take shared ownership before return
    }
}

... {
    struct message *msg = memref_alloc(backend, sizeof(*msg), NULL);
    broadcast_to_consumers(msg);
    memref_unref(msg); // Release the sender's reference
}
```

## Footprint and opt-in complexity

The `memref` abstraction is deliberately minimal. It provides a single, explicit mechanism for
shared ownership that can be adopted incrementally where needed by an application developer.
Systems that prefer strict single-ownership models are not forced to use it, while software modules
that require shared lifetimes gain a clear and reusable API for managing them safely.

# Concerns and Unresolved Questions

# Alternatives Considered

The backend pointer is stored in the control block at allocation time. This costs one
pointer per allocation, but `memref_ref` and `memref_unref` then operate on the pointer
alone without requiring the caller to track which backend it came from. This enables
passing memref pointers between modules using different backends seamlessly and removes
the `ref`/`unref` asymmetry where `ref` ignored the backend argument.

The alterative is to require the caller to pass the backend pointer to `memref_ref` and
`memref_unref` which I considered but eventually rejected (still unsure about the decision). This
would have been the "optimal" solution for a system with only one backend (the likely but not
guaranteed usecase), but as soon as multiple backends are in use, it would fail to provide a clean
interface for passing memref pointers between modules.
