virtual patch
virtual report

// ring_buf -> ring_buffer
@@
@@
- struct ring_buf
+ struct ring_buffer

// ring_buf_init(rb, size, buf) -> ring_buffer(rb, buf, size)
@@
expression E1, E2, E3;
@@
- ring_buf_init(E1, E2, E3)
+ ring_buffer_init(E1, E3, E2)

// ring_buf_put_claim -> ring_buffer_write_ptr
// ring_buf_get_claim -> ring_buffer_read_ptr
@@
expression E1, E2, E3;
@@
(
- ring_buf_put_claim(E1, E2, E3)
+ ring_buffer_write_ptr(E1, E2)
|
- ring_buf_get_claim(E1, E2, E3)
+ ring_buffer_read_ptr(E1, E2)
)

//Now a void function
@@
identifier I;
expression E1, E2;
@@
(
- I = ring_buf_get_finish(E1, E2);
+ ring_buf_get_finish(E1, E2);
|
- I = ring_buf_put_finish(E1, E2);
+ ring_buf_put_finish(E1, E2);
)

//remove now void operations
@@
expression E;
@@
(
- ring_buf_put_finish(E, 0);
|
- ring_buf_get_finish(E, 0);
)

//Rename functions
@@
expression E1, E2, E3;
@@
(
- ring_buf_put(E1, E2, E3)
+ ring_buffer_write(E1, E2, E3)
|
- ring_buf_get(E1, E2, E3)
+ ring_buffer_read(E1, E2, E3)
|
- ring_buf_get_finish(E1, E2)
+ ring_buffer_consume(E1, E2)
|
- ring_buf_put_finish(E1, E2)
+ ring_buffer_commit(E1, E2)
|
- ring_buf_space_get(E1)
+ ring_buffer_space(E1)
|
- ring_buf_size_get(E1)
+ ring_buffer_size(E1)
|
- ring_buf_is_empty(E1)
+ ring_buffer_empty(E1)
)
