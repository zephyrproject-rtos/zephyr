/// Unsigned expressions cannot be lesser than zero. Presence of
/// comparisons 'unsigned (<|<=) 0' often indicates a bug,
/// usually wrong type of variable.
///
// Confidence: High
// Copyright: (C) 2015 Andrzej Hajda, Samsung Electronics Co., Ltd. GPLv2.
// URL: http://coccinelle.lip6.fr/

virtual org
virtual report

@r_cmp depends on !(file in "ext")@
position p;
typedef uint8_t, uint16_t, uint32_t, uint64_t;
{unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long,
	size_t, uint8_t, uint16_t, uint32_t, uint64_t} v;
@@

	(\( v@p < 0 \| v@p <= 0 \))

@script:python depends on org@
p << r_cmp.p;
@@

msg = "WARNING: Unsigned expression compared with zero."
coccilib.org.print_todo(p[0], msg)

@script:python depends on report@
p << r_cmp.p;
@@

msg = "WARNING: Unsigned expression compared with zero."
coccilib.report.print_report(p[0], msg)
