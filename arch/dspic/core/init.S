.section .init,code
.global __custom_data_init
.global __custom_data_init_extended

.equ __custom_data_init, __custom_data_init_extended
__custom_data_init_extended:

	.equiv   FMT_CLEAR,0
	.equiv   FMT_COPY2,1
	.equiv   FMT_COPY3,2
	.equiv   FMT_CALL, 3
	.equiv   FMT_DUO_COPY3,0x1F

#define DINIT     w0
#define TLSOFFSET w1
#define TBLOFFSET w9
#define DSTOFFSET w10
#define LEN       w11
#define FORMAT    w12

	mov.l    w0, TBLOFFSET
	bra      4f

1:
	add.l    DSTOFFSET,TLSOFFSET,DSTOFFSET
	mov.l    [TBLOFFSET++], LEN
	mov.l    [TBLOFFSET++], FORMAT

	cp.b     FORMAT, #FMT_CALL
	bra nz,  2f
	call     DSTOFFSET
	bra      4f

2:
    cp.b     FORMAT, #FMT_CLEAR
    bra nz,  2f
9:
	sub.l    LEN, #1, LEN
	repeat   LEN
	clr.b    [DSTOFFSET++]
	bra      4f

2:
	cp.b     FORMAT, #FMT_COPY2
	bra z,   3f

3:
	sub.l   LEN, #1, LEN
	repeat  LEN
	mov.b   [TBLOFFSET++], [DSTOFFSET++]
	add.l   TBLOFFSET, #3, TBLOFFSET
	and1.l  TBLOFFSET, #0x7C, TBLOFFSET

4:
	sub.l   [TBLOFFSET++], #0, DSTOFFSET
	bra nz, 1b
	return
