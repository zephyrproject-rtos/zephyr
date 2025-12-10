virtual report

@r1@
position p_loop, p_finish;
identifier I, I2;
@@

do {
...
(
* I = ring_buf_put_claim(...);@p_loop
|
* ring_buf_put_claim(...);@p_loop
)
...
} while (...);
...
(
* I2 = ring_buf_put_finish(...);@p_finish
|
* ring_buf_put_finish(...);@p_finish
)

@r2@
position p_loop, p_finish;
identifier I, I2;
@@

while(...) {
...
(
* I = ring_buf_put_claim(...);@p_loop
|
* ring_buf_put_claim(...);@p_loop
)
...
}
...
(
* I2 = ring_buf_put_finish(...);@p_finish
|
* ring_buf_put_finish(...);@p_finish
)


@r3@
position p_loop, p_finish;
identifier I, I2;
@@

do {
...
(
* I = ring_buf_get_claim(...);@p_loop
|
* ring_buf_get_claim(...);@p_loop
)
...
} while (...);
...
(
* I2 = ring_buf_get_finish(...);@p_finish
|
* ring_buf_get_finish(...);@p_finish
)

@r4@
position p_loop, p_finish;
identifier I, I2;
@@

while(...) {
...
(
* I = ring_buf_get_claim(...);@p_loop
|
* ring_buf_get_claim(...);@p_loop
)
...
}
...
(
* I2 = ring_buf_get_finish(...);@p_finish
|
* ring_buf_get_finish(...);@p_finish
)

@script:python depends on r1@
p_loop << r1.p_loop;
p_finish << r1.p_finish;
@@
print("--------------------------------------------------------------------------------")
print(f"WARNING: Potential logic error detected (finish outside of do-while loop).")
print(f"Location of claim Call: {p_loop[0].file}:{p_loop[0].line}")
print(f"Location of Finish Call: {p_finish[0].file}:{p_finish[0].line}")
print("--------------------------------------------------------------------------------")

@script:python depends on r3@
p_loop << r3.p_loop;
p_finish << r3.p_finish;
@@
print("--------------------------------------------------------------------------------")
print(f"WARNING: Potential logic error detected (finish outside of do-while loop).")
print(f"Location of claim Call: {p_loop[0].file}:{p_loop[0].line}")
print(f"Location of Finish Call: {p_finish[0].file}:{p_finish[0].line}")
print("--------------------------------------------------------------------------------")

@script:python depends on r2@
p_loop << r2.p_loop;
p_finish << r2.p_finish;
@@
print("--------------------------------------------------------------------------------")
print(f"WARNING: Potential logic error detected (finish outside of while loop).")
print(f"Location of claim Call: {p_loop[0].file}:{p_loop[0].line}")
print(f"Location of Finish Call: {p_finish[0].file}:{p_finish[0].line}")
print("--------------------------------------------------------------------------------")

@script:python depends on r4@
p_loop << r4.p_loop;
p_finish << r4.p_finish;
@@
print("--------------------------------------------------------------------------------")
print(f"WARNING: Potential logic error detected (finish outside of while loop).")
print(f"Location of claim Call: {p_loop[0].file}:{p_loop[0].line}")
print(f"Location of Finish Call: {p_finish[0].file}:{p_finish[0].line}")
print("--------------------------------------------------------------------------------")
