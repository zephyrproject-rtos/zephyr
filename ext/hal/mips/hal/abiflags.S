/*
 * abiflags.S - MIPS ABI flags.
 */

/* Values for the xxx_size bytes of an ABI flags structure.  */
#define AFL_REG_NONE         0x00       /* No registers.  */
#define AFL_REG_32           0x01       /* 32-bit registers.  */
#define AFL_REG_64           0x02       /* 64-bit registers.  */
#define AFL_REG_128          0x03       /* 128-bit registers.  */

/* Masks for the ases word of an ABI flags structure.  */
#define AFL_ASE_DSP          0x00000001  /* DSP ASE.  */
#define AFL_ASE_DSPR2        0x00000002  /* DSP R2 ASE.  */
#define AFL_ASE_EVA          0x00000004  /* Enhanced VA Scheme.  */
#define AFL_ASE_MCU          0x00000008  /* MCU (MicroController) ASE.  */
#define AFL_ASE_MDMX         0x00000010  /* MDMX ASE.  */
#define AFL_ASE_MIPS3D       0x00000020  /* MIPS-3D ASE.  */
#define AFL_ASE_MT           0x00000040  /* MT ASE.  */
#define AFL_ASE_SMARTMIPS    0x00000080  /* SmartMIPS ASE.  */
#define AFL_ASE_VIRT         0x00000100  /* VZ ASE.  */
#define AFL_ASE_MSA          0x00000200  /* MSA ASE.  */
#define AFL_ASE_MIPS16       0x00000400  /* MIPS16 ASE.  */
#define AFL_ASE_MICROMIPS    0x00000800  /* MICROMIPS ASE.  */
#define AFL_ASE_XPA          0x00001000  /* XPA ASE.  */

/* Values for the isa_ext word of an ABI flags structure.  */
#define AFL_EXT_XLR           1  /* RMI Xlr instruction.  */
#define AFL_EXT_OCTEON2       2  /* Cavium Networks Octeon2.  */
#define AFL_EXT_OCTEONP       3  /* Cavium Networks OcteonP.  */
#define AFL_EXT_LOONGSON_3A   4  /* Loongson 3A.  */
#define AFL_EXT_OCTEON        5  /* Cavium Networks Octeon.  */
#define AFL_EXT_5900          6  /* MIPS R5900 instruction.  */
#define AFL_EXT_4650          7  /* MIPS R4650 instruction.  */
#define AFL_EXT_4010          8  /* LSI R4010 instruction.  */
#define AFL_EXT_4100          9  /* NEC VR4100 instruction.  */
#define AFL_EXT_3900         10  /* Toshiba R3900 instruction.  */
#define AFL_EXT_10000        11  /* MIPS R10000 instruction.  */
#define AFL_EXT_SB1          12  /* Broadcom SB-1 instruction.  */
#define AFL_EXT_4111         13  /* NEC VR4111/VR4181 instruction.  */
#define AFL_EXT_4120         14  /* NEC VR4120 instruction.  */
#define AFL_EXT_5400         15  /* NEC VR5400 instruction.  */
#define AFL_EXT_5500         16  /* NEC VR5500 instruction.  */
#define AFL_EXT_LOONGSON_2E  17  /* ST Microelectronics Loongson 2E.  */
#define AFL_EXT_LOONGSON_2F  18  /* ST Microelectronics Loongson 2F.  */

/* Values defined for Tag_GNU_MIPS_ABI_FP.  */
#define Val_GNU_MIPS_ABI_FP_ANY    0  /* Not tagged or not using any ABIs affected by the differences.  */
#define Val_GNU_MIPS_ABI_FP_DOUBLE 1  /* Using hard-float -mdouble-float.  */
#define Val_GNU_MIPS_ABI_FP_SINGLE 2  /* Using hard-float -msingle-float.  */
#define Val_GNU_MIPS_ABI_FP_SOFT   3  /* Using soft-float.  */
#define Val_GNU_MIPS_ABI_FP_OLD_64 4  /* Using -mips32r2 -mfp64.  */
#define Val_GNU_MIPS_ABI_FP_XX     5  /* Using -mfpxx */
#define Val_GNU_MIPS_ABI_FP_64     6  /* Using -mips32r2 -mfp64.  */
#define Val_GNU_MIPS_ABI_MSA_ANY   0  /* Not tagged or not using any ABIs affected by the differences.  */
#define Val_GNU_MIPS_ABI_MSA_128   1  /* Using 128-bit MSA.  */

/* MIPS ABI flags structure */
  .struct 0
ABIFlags_version:
  .struct ABIFlags_version + 2
ABIFlags_isa_level:
  .struct ABIFlags_isa_level + 1
ABIFlags_isa_rev:
  .struct ABIFlags_isa_rev + 1
ABIFlags_gpr_size:
  .struct ABIFlags_gpr_size + 1
ABIFlags_cpr1_size:
  .struct ABIFlags_cpr1_size + 1
ABIFlags_cpr2_size:
  .struct ABIFlags_cpr2_size + 1
ABIFlags_fp_abi:
  .struct ABIFlags_fp_abi + 1
ABIFlags_isa_ext:
  .struct ABIFlags_isa_ext + 4
ABIFlags_ases:
  .struct ABIFlags_ases + 4
ABIFlags_flags1:
  .struct ABIFlags_flags1 + 4
ABIFlags_flags2:
  .struct ABIFlags_flags2 + 4

/*> EOF abiflags.S <*/
