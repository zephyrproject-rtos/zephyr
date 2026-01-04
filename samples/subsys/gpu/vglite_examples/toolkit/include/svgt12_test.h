#include "vglite_layer.h"
static uint16_t svg1_SVGFreeSansASCII_GA_unicode[] = {
	0x0020, 0x0022, 0x0024, 0x002e, 0x0031, 0x0036, 0x003a, 0x003d,
	0x0042, 0x0052, 0x0061, 0x0063, 0x0064, 0x0065, 0x0069, 0x006b,
	0x006e, 0x006f, 0x0070, 0x0072, 0x0073, 0x0074, 0x0076,
};

static int8_t svg1_SVGFreeSansASCII_0022_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE,
	VLC_OP_LINE, VLC_OP_LINE, VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE,
	VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0022_args[] = {
	52.00,  709.00, 145.00, 709.00, 145.00, 598.00, 118.00, 464.00, 79.00,  464.00,
	52.00,  598.00, 52.00,  709.00, 212.00, 709.00, 305.00, 709.00, 305.00, 598.00,
	278.00, 464.00, 239.00, 464.00, 212.00, 598.00, 212.00, 709.00,

};

static int8_t svg1_SVGFreeSansASCII_0024_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_LINE,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_MOVE,
	VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0024_args[] = {
	243.00,  770.00, 302.00, 770.00, 302.00, 716.00, 427.00, 707.00, 474.00, 615.00,  496.00,
	574.00,  496.00, 520.00, 496.00, 519.00, 417.00, 519.00, 416.00, 596.00, 359.00,  630.00,
	339.00,  641.00, 315.00, 645.00, 302.00, 646.00, 302.00, 397.00, 402.00, 366.00,  431.00,
	350.00,  433.00, 349.00, 517.00, 301.00, 518.00, 196.00, 518.00, 195.00, 518.00,  91.00,
	450.00,  30.00,  426.00, 12.00,  377.00, -17.00, 302.00, -23.00, 302.00, -126.00, 243.00,
	-126.00, 243.00, -23.00, 89.00,  -13.00, 46.00,  104.00, 30.00,  150.00, 33.00,   208.00,
	112.00,  208.00, 119.00, 134.00, 133.00, 110.00, 137.00, 103.00, 172.00, 55.00,   243.00,
	46.00,   243.00, 318.00, 150.00, 346.00, 117.00, 370.00, 46.00,  421.00, 46.00,   515.00,
	46.00,   516.00, 46.00,  662.00, 186.00, 704.00, 212.00, 712.00, 243.00, 716.00,  243.00,
	770.00,  243.00, 405.00, 243.00, 645.00, 147.00, 632.00, 130.00, 556.00, 127.00,  526.00,
	127.00,  436.00, 243.00, 405.00, 302.00, 309.00, 302.00, 46.00,  363.00, 53.00,   394.00,
	83.00,   436.00, 123.00, 436.00, 182.00, 436.00, 183.00, 436.00, 244.00, 389.00,  274.00,
	360.00,  292.00, 302.00, 309.00,

};

static int8_t svg1_SVGFreeSansASCII_002e_cmds[] = {VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE,
						   VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_002e_args[] = {
	191.00, 104.00, 191.00, 0.00, 87.00, 0.00, 87.00, 104.00, 191.00, 104.00,

};

static int8_t svg1_SVGFreeSansASCII_0031_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0031_args[] = {
	259.00, 505.00, 102.00, 505.00, 102.00, 568.00, 204.00, 581.00, 234.00, 604.00,
	235.00, 604.00, 245.00, 611.00, 252.00, 621.00, 271.00, 645.00, 289.00, 709.00,
	347.00, 709.00, 347.00, 0.00,   259.00, 0.00,   259.00, 505.00,

};

static int8_t svg1_SVGFreeSansASCII_0036_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_LINE, VLC_OP_MOVE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0036_args[] = {
	498.00, 524.00, 410.00, 524.00, 392.00, 611.00, 321.00, 628.00, 307.00, 631.00, 291.00,
	631.00, 180.00, 631.00, 146.00, 487.00, 133.00, 433.00, 133.00, 362.00, 191.00, 441.00,
	296.00, 441.00, 407.00, 441.00, 469.00, 358.00, 513.00, 299.00, 513.00, 217.00, 513.00,
	216.00, 513.00, 97.00,  431.00, 29.00,  369.00, -23.00, 281.00, -23.00, 162.00, -23.00,
	103.00, 65.00,  45.00,  153.00, 43.00,  312.00, 43.00,  317.00, 43.00,  322.00, 43.00,
	323.00, 43.00,  508.00, 107.00, 608.00, 163.00, 692.00, 263.00, 707.00, 280.00, 709.00,
	297.00, 709.00, 412.00, 709.00, 467.00, 616.00, 490.00, 577.00, 498.00, 525.00, 498.00,
	524.00, 285.00, 363.00, 200.00, 363.00, 160.00, 297.00, 138.00, 262.00, 138.00, 215.00,
	138.00, 214.00, 138.00, 128.00, 199.00, 82.00,  235.00, 55.00,  282.00, 55.00,  358.00,
	55.00,  398.00, 118.00, 423.00, 157.00, 423.00, 208.00, 423.00, 209.00, 423.00, 311.00,
	352.00, 348.00, 323.00, 363.00, 285.00, 363.00,

};

static int8_t svg1_SVGFreeSansASCII_003a_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_MOVE,
	VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_003a_args[] = {
	214.00, 104.00, 214.00, 0.00,   110.00, 0.00,   110.00, 104.00, 214.00, 104.00,
	214.00, 524.00, 214.00, 420.00, 110.00, 420.00, 110.00, 524.00, 214.00, 524.00,

};

static int8_t svg1_SVGFreeSansASCII_003d_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_MOVE,
	VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_003d_args[] = {
	534.00, 353.00, 534.00, 283.00, 50.00, 283.00, 50.00, 353.00, 534.00, 353.00,
	534.00, 181.00, 534.00, 111.00, 50.00, 111.00, 50.00, 181.00, 534.00, 181.00,

};

static int8_t svg1_SVGFreeSansASCII_0042_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE,
	VLC_OP_LINE, VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_LINE,
	VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0042_args[] = {
	79.00,  0.00,   79.00,  729.00, 375.00, 729.00, 478.00, 729.00, 533.00, 678.00,
	591.00, 627.00, 591.00, 545.00, 591.00, 544.00, 591.00, 432.00, 490.00, 385.00,
	595.00, 344.00, 616.00, 264.00, 622.00, 244.00, 623.00, 221.00, 623.00, 215.00,
	623.00, 209.00, 623.00, 208.00, 623.00, 120.00, 567.00, 61.00,  511.00, 0.00,
	409.00, 0.00,   408.00, 0.00,   79.00,  0.00,   172.00, 415.00, 352.00, 415.00,
	424.00, 415.00, 458.00, 441.00, 498.00, 471.00, 498.00, 530.00, 498.00, 531.00,
	498.00, 590.00, 458.00, 621.00, 424.00, 647.00, 352.00, 647.00, 172.00, 647.00,
	172.00, 415.00, 172.00, 82.00,  399.00, 82.00,  463.00, 82.00,  495.00, 116.00,
	496.00, 117.00, 530.00, 152.00, 530.00, 206.00, 530.00, 207.00, 530.00, 262.00,
	496.00, 298.00, 464.00, 333.00, 399.00, 333.00, 172.00, 333.00, 172.00, 82.00,

};

static int8_t svg1_SVGFreeSansASCII_0052_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_LINE, VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0052_args[] = {
	186.00, 314.00, 186.00, 0.00,   93.00,  0.00,   93.00,  729.00, 429.00, 729.00, 599.00,
	729.00, 640.00, 609.00, 651.00, 576.00, 651.00, 535.00, 651.00, 534.00, 651.00, 436.00,
	579.00, 385.00, 560.00, 372.00, 536.00, 360.00, 598.00, 333.00, 617.00, 293.00, 634.00,
	256.00, 635.00, 170.00, 637.00, 74.00,  654.00, 47.00,  663.00, 34.00,  679.00, 23.00,
	679.00, 0.00,   566.00, 0.00,   545.00, 48.00,  545.00, 118.00, 545.00, 119.00, 546.00,
	184.00, 546.00, 293.00, 466.00, 310.00, 448.00, 314.00, 426.00, 314.00, 186.00, 314.00,
	186.00, 396.00, 411.00, 396.00, 532.00, 396.00, 550.00, 482.00, 554.00, 499.00, 554.00,
	520.00, 554.00, 521.00, 554.00, 587.00, 516.00, 619.00, 484.00, 647.00, 411.00, 647.00,
	186.00, 647.00, 186.00, 396.00,

};

static int8_t svg1_SVGFreeSansASCII_0061_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE,
	VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0061_args[] = {
	535.00, 49.00,  535.00, -14.00, 500.00, -23.00, 478.00, -23.00, 407.00, -23.00, 394.00,
	40.00,  393.00, 47.00,  392.00, 54.00,  309.00, -22.00, 218.00, -23.00, 214.00, -23.00,
	104.00, -23.00, 61.00,  52.00,  42.00,  86.00,  42.00,  131.00, 42.00,  132.00, 42.00,
	234.00, 134.00, 272.00, 170.00, 287.00, 264.00, 299.00, 302.00, 304.00, 375.00, 313.00,
	386.00, 342.00, 386.00, 343.00, 386.00, 344.00, 389.00, 362.00, 389.00, 384.00, 389.00,
	448.00, 308.00, 460.00, 291.00, 462.00, 272.00, 462.00, 169.00, 462.00, 152.00, 387.00,
	150.00, 378.00, 149.00, 369.00, 65.00,  369.00, 68.00,  441.00, 101.00, 478.00, 156.00,
	539.00, 275.00, 539.00, 451.00, 539.00, 470.00, 423.00, 472.00, 411.00, 472.00, 397.00,
	472.00, 396.00, 472.00, 88.00,  472.00, 47.00,  517.00, 47.00,  535.00, 49.00,  389.00,
	165.00, 389.00, 259.00, 357.00, 244.00, 275.00, 233.00, 266.00, 232.00, 255.00, 230.00,
	149.00, 215.00, 133.00, 161.00, 129.00, 135.00, 129.00, 134.00, 129.00, 69.00,  196.00,
	54.00,  212.00, 50.00,  232.00, 50.00,  304.00, 50.00,  356.00, 97.00,  388.00, 126.00,
	389.00, 161.00, 389.00, 162.00, 389.00, 162.00, 389.00, 162.00, 389.00, 163.00, 389.00,
	163.00, 389.00, 165.00,

};

static int8_t svg1_SVGFreeSansASCII_0063_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0063_args[] = {
	471.00, 348.00, 387.00, 348.00, 374.00, 445.00, 290.00, 460.00, 282.00, 461.00, 272.00,
	462.00, 263.00, 462.00, 169.00, 462.00, 134.00, 362.00, 118.00, 316.00, 118.00, 254.00,
	118.00, 253.00, 118.00, 116.00, 197.00, 71.00,  226.00, 54.00,  265.00, 54.00,  372.00,
	54.00,  393.00, 180.00, 477.00, 180.00, 466.00, 43.00,  362.00, -3.00,  319.00, -23.00,
	263.00, -23.00, 134.00, -23.00, 72.00,  82.00,  31.00,  151.00, 31.00,  252.00, 31.00,
	253.00, 31.00,  413.00, 123.00, 490.00, 182.00, 539.00, 264.00, 539.00, 370.00, 539.00,
	428.00, 471.00, 436.00, 461.00, 465.00, 418.00, 471.00, 348.00,

};

static int8_t svg1_SVGFreeSansASCII_0064_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE,
	VLC_OP_LINE, VLC_OP_MOVE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0064_args[] = {
	495.00, 729.00, 495.00, 0.00,   421.00, 0.00,   421.00, 69.00,  363.00, -14.00, 277.00,
	-22.00, 266.00, -23.00, 254.00, -23.00, 124.00, -23.00, 64.00,  89.00,  26.00,  159.00,
	26.00,  261.00, 26.00,  261.00, 26.00,  262.00, 26.00,  263.00, 26.00,  417.00, 115.00,
	492.00, 172.00, 539.00, 251.00, 539.00, 359.00, 539.00, 412.00, 458.00, 412.00, 729.00,
	495.00, 729.00, 265.00, 461.00, 180.00, 461.00, 139.00, 379.00, 119.00, 341.00, 114.00,
	290.00, 113.00, 275.00, 113.00, 259.00, 113.00, 258.00, 113.00, 136.00, 183.00, 82.00,
	219.00, 55.00,  266.00, 55.00,  348.00, 55.00,  387.00, 136.00, 412.00, 186.00, 412.00,
	255.00, 412.00, 256.00, 412.00, 387.00, 340.00, 438.00, 307.00, 461.00, 265.00, 461.00,

};

static int8_t svg1_SVGFreeSansASCII_0065_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0065_args[] = {
	513.00, 234.00, 127.00, 234.00, 128.00, 162.00, 155.00, 122.00, 198.00, 54.00,
	281.00, 54.00,  383.00, 54.00,  418.00, 159.00, 502.00, 159.00, 480.00, 38.00,
	376.00, -5.00,  332.00, -23.00, 278.00, -23.00, 142.00, -23.00, 79.00,  87.00,
	40.00,  155.00, 40.00,  253.00, 40.00,  255.00, 40.00,  413.00, 134.00, 490.00,
	194.00, 539.00, 280.00, 539.00, 395.00, 539.00, 460.00, 457.00, 480.00, 432.00,
	492.00, 401.00, 513.00, 347.00, 513.00, 235.00, 513.00, 234.00, 129.00, 302.00,
	423.00, 302.00, 424.00, 308.00, 424.00, 388.00, 365.00, 433.00, 327.00, 462.00,
	279.00, 462.00, 194.00, 462.00, 153.00, 387.00, 133.00, 351.00, 129.00, 302.00,

};

static int8_t svg1_SVGFreeSansASCII_0069_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_MOVE,
	VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0069_args[] = {
	150.00, 524.00, 150.00, 0.00,   67.00, 0.00,   67.00, 524.00, 150.00, 524.00,
	150.00, 729.00, 150.00, 624.00, 66.00, 624.00, 66.00, 729.00, 150.00, 729.00,

};

static int8_t svg1_SVGFreeSansASCII_006b_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE,
	VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_006b_args[] = {
	141.00, 729.00, 141.00, 302.00, 363.00, 524.00, 470.00, 524.00, 288.00,
	343.00, 502.00, 0.00,   399.00, 0.00,   222.00, 284.00, 141.00, 204.00,
	141.00, 0.00,   58.00,  0.00,   58.00,  729.00, 141.00, 729.00,

};

static int8_t svg1_SVGFreeSansASCII_006e_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_006e_args[] = {
	70.00,  524.00, 147.00, 524.00, 147.00, 436.00, 201.00, 521.00, 277.00, 535.00, 297.00,
	539.00, 321.00, 539.00, 425.00, 539.00, 467.00, 470.00, 486.00, 439.00, 487.00, 398.00,
	487.00, 396.00, 487.00, 0.00,   404.00, 0.00,   404.00, 363.00, 404.00, 432.00, 346.00,
	457.00, 324.00, 466.00, 296.00, 466.00, 212.00, 466.00, 175.00, 389.00, 154.00, 347.00,
	154.00, 290.00, 154.00, 289.00, 154.00, 0.00,   70.00,  0.00,   70.00,  524.00,

};

static int8_t svg1_SVGFreeSansASCII_006f_cmds[] = {
	VLC_OP_MOVE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_MOVE,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_006f_args[] = {
	272.00, 539.00, 424.00, 539.00, 481.00, 410.00, 510.00, 345.00, 510.00, 255.00, 510.00,
	254.00, 510.00, 87.00,  408.00, 16.00,  352.00, -23.00, 273.00, -23.00, 129.00, -23.00,
	69.00,  96.00,  36.00,  162.00, 36.00,  257.00, 36.00,  258.00, 36.00,  432.00, 141.00,
	502.00, 196.00, 539.00, 272.00, 539.00, 273.00, 462.00, 180.00, 462.00, 142.00, 370.00,
	123.00, 323.00, 123.00, 259.00, 123.00, 258.00, 123.00, 121.00, 201.00, 73.00,  232.00,
	54.00,  273.00, 54.00,  363.00, 54.00,  402.00, 142.00, 423.00, 189.00, 423.00, 254.00,
	423.00, 255.00, 423.00, 400.00, 341.00, 446.00, 311.00, 462.00, 273.00, 462.00,

};

static int8_t svg1_SVGFreeSansASCII_0070_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE,
	VLC_OP_LINE, VLC_OP_MOVE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0070_args[] = {
	54.00,  -218.00, 54.00,  524.00, 131.00, 524.00, 131.00, 445.00, 190.00,  539.00, 298.00,
	539.00, 425.00,  539.00, 485.00, 428.00, 523.00, 357.00, 523.00, 254.00,  523.00, 253.00,
	523.00, 99.00,   434.00, 24.00,  395.00, -9.00,  343.00, -19.00, 322.00,  -23.00, 299.00,
	-23.00, 202.00,  -23.00, 139.00, 54.00,  138.00, 55.00,  138.00, -218.00, 54.00,  -218.00,
	284.00, 461.00,  201.00, 461.00, 162.00, 377.00, 138.00, 328.00, 138.00,  259.00, 138.00,
	258.00, 138.00,  133.00, 207.00, 81.00,  240.00, 55.00,  284.00, 55.00,   368.00, 55.00,
	409.00, 134.00,  436.00, 185.00, 436.00, 254.00, 436.00, 255.00, 436.00,  382.00, 365.00,
	435.00, 330.00,  461.00, 284.00, 461.00,

};

static int8_t svg1_SVGFreeSansASCII_0072_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0072_args[] = {
	69.00,  524.00, 146.00, 524.00, 146.00, 429.00, 204.00, 522.00, 262.00, 536.00,
	275.00, 539.00, 289.00, 539.00, 321.00, 536.00, 321.00, 451.00, 249.00, 450.00,
	218.00, 429.00, 215.00, 427.00, 212.00, 425.00, 153.00, 382.00, 153.00, 273.00,
	153.00, 272.00, 153.00, 0.00,   69.00,  0.00,   69.00,  524.00,

};

static int8_t svg1_SVGFreeSansASCII_0073_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD,
	VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0073_args[] = {
	438.00, 378.00, 350.00, 378.00, 347.00, 462.00, 245.00, 462.00, 163.00, 462.00, 140.00,
	413.00, 134.00, 400.00, 134.00, 384.00, 134.00, 383.00, 134.00, 338.00, 200.00, 316.00,
	231.00, 308.00, 311.00, 289.00, 429.00, 261.00, 452.00, 191.00, 459.00, 170.00, 459.00,
	144.00, 459.00, 143.00, 459.00, 47.00,  369.00, 3.00,   317.00, -23.00, 243.00, -23.00,
	49.00,  -23.00, 35.00,  139.00, 34.00,  156.00, 122.00, 156.00, 128.00, 109.00, 146.00,
	89.00,  179.00, 54.00,  250.00, 54.00,  334.00, 54.00,  362.00, 101.00, 372.00, 116.00,
	372.00, 135.00, 372.00, 136.00, 372.00, 182.00, 318.00, 201.00, 309.00, 204.00, 299.00,
	207.00, 298.00, 207.00, 291.00, 209.00, 213.00, 228.00, 94.00,  257.00, 63.00,  308.00,
	54.00,  324.00, 50.00,  345.00, 47.00,  379.00, 47.00,  472.00, 131.00, 514.00, 180.00,
	539.00, 248.00, 539.00, 393.00, 539.00, 428.00, 438.00, 438.00, 411.00, 438.00, 378.00,

};

static int8_t svg1_SVGFreeSansASCII_0074_cmds[] = {
	VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_QUAD,
	VLC_OP_LINE, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_QUAD, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE,
	VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0074_args[] = {
	254.00, 524.00, 254.00, 456.00, 168.00, 456.00, 168.00, 97.00,  168.00, 59.00,
	190.00, 53.00,  214.00, 50.00,  239.00, 50.00,  254.00, 54.00,  254.00, -16.00,
	215.00, -23.00, 186.00, -23.00, 97.00,  -23.00, 86.00,  44.00,  85.00,  60.00,
	85.00,  60.00,  85.00,  456.00, 14.00,  456.00, 14.00,  524.00, 85.00,  524.00,
	85.00,  668.00, 168.00, 668.00, 168.00, 524.00, 254.00, 524.00,

};

static int8_t svg1_SVGFreeSansASCII_0076_cmds[] = {VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE,
						   VLC_OP_LINE, VLC_OP_LINE, VLC_OP_LINE,
						   VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svg1_SVGFreeSansASCII_0076_args[] = {
	285.00, 0.00,  194.00, 0.00,   10.00,  524.00, 104.00, 524.00,
	244.00, 99.00, 392.00, 524.00, 486.00, 524.00, 285.00, 0.00,

};

static font_info_t svg1_SVGFreeSansASCII = {
	.font_name = "SVGFreeSansASCII",
	.max_glyphs = 23,
	.units_per_em = 1000,
	.unicode = svg1_SVGFreeSansASCII_GA_unicode,
	.gi = {{.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0022_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0022_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0022_cmds),
		.end_path_flag = 0,
		.bbox = {52.0, 464.0, 305.0, 709.0},
		.hx = 355},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0024_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0024_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0024_cmds),
		.end_path_flag = 0,
		.bbox = {30.0, 12.0, 518.0, 770.0},
		.hx = 556},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_002e_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_002e_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_002e_cmds),
		.end_path_flag = 0,
		.bbox = {87.0, 0.0, 191.0, 104.0},
		.hx = 278},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0031_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0031_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0031_cmds),
		.end_path_flag = 0,
		.bbox = {102.0, 0.0, 347.0, 709.0},
		.hx = 556},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0036_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0036_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0036_cmds),
		.end_path_flag = 0,
		.bbox = {43.0, 23.0, 513.0, 709.0},
		.hx = 556},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_003a_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_003a_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_003a_cmds),
		.end_path_flag = 0,
		.bbox = {110.0, 0.0, 214.0, 524.0},
		.hx = 278},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_003d_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_003d_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_003d_cmds),
		.end_path_flag = 0,
		.bbox = {50.0, 111.0, 534.0, 353.0},
		.hx = 584},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0042_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0042_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0042_cmds),
		.end_path_flag = 0,
		.bbox = {79.0, 0.0, 623.0, 729.0},
		.hx = 667},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0052_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0052_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0052_cmds),
		.end_path_flag = 0,
		.bbox = {93.0, 0.0, 679.0, 729.0},
		.hx = 722},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0061_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0061_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0061_cmds),
		.end_path_flag = 0,
		.bbox = {42.0, 14.0, 535.0, 539.0},
		.hx = 556},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0063_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0063_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0063_cmds),
		.end_path_flag = 0,
		.bbox = {31.0, 23.0, 477.0, 539.0},
		.hx = 500},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0064_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0064_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0064_cmds),
		.end_path_flag = 0,
		.bbox = {26.0, 0.0, 495.0, 729.0},
		.hx = 556},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0065_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0065_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0065_cmds),
		.end_path_flag = 0,
		.bbox = {40.0, 23.0, 513.0, 539.0},
		.hx = 556},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0069_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0069_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0069_cmds),
		.end_path_flag = 0,
		.bbox = {66.0, 0.0, 150.0, 729.0},
		.hx = 222},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_006b_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_006b_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_006b_cmds),
		.end_path_flag = 0,
		.bbox = {58.0, 0.0, 502.0, 729.0},
		.hx = 500},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_006e_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_006e_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_006e_cmds),
		.end_path_flag = 0,
		.bbox = {70.0, 0.0, 487.0, 539.0},
		.hx = 556},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_006f_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_006f_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_006f_cmds),
		.end_path_flag = 0,
		.bbox = {36.0, 23.0, 510.0, 539.0},
		.hx = 556},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0070_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0070_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0070_cmds),
		.end_path_flag = 0,
		.bbox = {54.0, 9.0, 523.0, 539.0},
		.hx = 556},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0072_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0072_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0072_cmds),
		.end_path_flag = 0,
		.bbox = {69.0, 0.0, 321.0, 539.0},
		.hx = 333},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0073_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0073_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0073_cmds),
		.end_path_flag = 0,
		.bbox = {34.0, 23.0, 459.0, 539.0},
		.hx = 500},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0074_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0074_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0074_cmds),
		.end_path_flag = 0,
		.bbox = {14.0, 16.0, 254.0, 668.0},
		.hx = 278},
	       {.path_cmds = (uint8_t *)svg1_SVGFreeSansASCII_0076_cmds,
		.path_args = (int32_t *)svg1_SVGFreeSansASCII_0076_args,
		.path_length = sizeof(svg1_SVGFreeSansASCII_0076_cmds),
		.end_path_flag = 0,
		.bbox = {10.0, 0.0, 486.0, 524.0},
		.hx = 500}},
};

#ifndef STATIC_PATH_DEFINES_H
#define STATIC_PATH_DEFINES_H

#include "vg_lite.h"

typedef union data_mnemonic {
	int32_t cmd;
	int32_t data;
} data_mnemonic_t;

typedef struct path_info {
	uint32_t path_length;
	float bounding_box[4];
	int32_t *path_args;
	uint8_t *path_cmds;
	uint8_t end_path_flag;
} path_info_t;

typedef struct stroke_info {
	uint32_t dashPatternCnt;
	float dashPhase;
	float *dashPattern;
	float strokeWidth;
	float miterlimit;
	uint32_t strokeColor;
	vg_lite_cap_style_t linecap;
	vg_lite_join_style_t linejoin;
} stroke_info_t;

typedef struct image_info {
	char *image_name;
	int image_size[2];
	vg_lite_format_t data_format;
	float *transform;
	svg_node_t *render_sequence;
	int drawable_count;
	int path_count;
	stroke_info_t *stroke_info;
	uint8_t image_count;
	image_buf_data_t **raw_images;
	path_info_t paths_info[];
} image_info_t;

typedef struct stopValue {
	float offset;
	uint32_t stop_color;
} stopValue_t;

typedef struct linearGradient {
	uint32_t num_stop_points;
	vg_lite_linear_gradient_parameter_t linear_gradient;
	stopValue_t *stops;
} linearGradient_t;

typedef struct radialGradient {
	uint32_t num_stop_points;
	vg_lite_radial_gradient_parameter_t radial_gradient;
	stopValue_t *stops;
} radialGradient_t;

typedef struct hybridPath {
	fill_mode_t fillType;
	vg_lite_draw_path_type_t pathType;
} hybridPath_t;

typedef struct gradient_mode {
	linearGradient_t **linearGrads;
	radialGradient_t **radialGrads;
	hybridPath_t *hybridPath;
	vg_lite_fill_t *fillRule;
} gradient_mode_t;

typedef struct msg_glyph_item {
	uint16_t g_u16; /* Unicode number of glyph */
	uint16_t k;     /* Horizontal advancement for next glyph */
} msg_glyph_item_t;

typedef struct svg_text_string_data {
	int x, y;               /* Top,left co-ordinates of text region */
	int font_size;          /* font-size for text rendering */
	float *tmatrix;         /* Transform matrix for text area */
	uint32_t text_color;    /* Color value in ARGB format */
	font_info_t *font_face; /* Pointer to pre-defined font-face */
	int msg_len;            /* Length of message in number of unicode bytes */
	int direction;          /* Glyph layout direction */
				/*
				 * Supported: left->right
				 * Unsupported: right->left, top->bottom, bottom->top
				 */
	msg_glyph_item_t *msg_glyphs;
} svg_text_string_data_t;

typedef struct svg_text_info {
	int num_text_strings;
	svg_text_string_data_t *text_strings;
} svg_text_info_t;

#endif

/*path id=stroke-01*/
static uint8_t svgt12_test_rect_1_cmds[] = {VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE,
					    VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svgt12_test_rect_1_args[] = {
	90.00, 70.00, 390.00, 70.00, 390.00, 120.00, 90.00, 120.00, 90.00, 70.00,
};

/*path id=stroke-02*/
static uint8_t svgt12_test_rect_2_cmds[] = {VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE,
					    VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svgt12_test_rect_2_args[] = {
	90.00, 190.00, 390.00, 190.00, 390.00, 240.00, 90.00, 240.00, 90.00, 190.00,
};

/*path id=test-frame*/
static uint8_t svgt12_test_rect_3_cmds[] = {VLC_OP_MOVE, VLC_OP_LINE, VLC_OP_LINE,
					    VLC_OP_LINE, VLC_OP_LINE, VLC_OP_END};

static int32_t svgt12_test_rect_3_args[] = {
	1.00, 1.00, 479.00, 1.00, 479.00, 359.00, 1.00, 359.00, 1.00, 1.00,
};

static stroke_info_t svgt12_test_stroke_info_data[] = {
	{
		/*rect id=stroke-01*/
	},
	{/*rect id=stroke-02*/
	 .dashPatternCnt = 0,
	 .dashPattern = NULL,
	 .dashPhase = 0,
	 .strokeWidth = 20,
	 .miterlimit = 4,
	 .strokeColor = 0xffff0000U,
	 .linecap = VG_LITE_CAP_BUTT,
	 .linejoin = VG_LITE_JOIN_MITER},
	{/*rect id=test-frame*/
	 .dashPatternCnt = 0,
	 .dashPattern = NULL,
	 .dashPhase = 0,
	 .strokeWidth = 1,
	 .miterlimit = 4,
	 .strokeColor = 0xff000000U,
	 .linecap = VG_LITE_CAP_BUTT,
	 .linejoin = VG_LITE_JOIN_MITER},

};

hybridPath_t svgt12_test_hybrid_path[] = {
	{.fillType = FILL_CONSTANT, .pathType = VG_LITE_DRAW_FILL_PATH},
	{.fillType = NO_FILL_MODE, .pathType = VG_LITE_DRAW_ZERO},
	{.fillType = FILL_CONSTANT, .pathType = VG_LITE_DRAW_FILL_PATH},
	{.fillType = STROKE, .pathType = VG_LITE_DRAW_STROKE_PATH},
	{.fillType = STROKE, .pathType = VG_LITE_DRAW_STROKE_PATH},
	{.fillType = STROKE, .pathType = VG_LITE_DRAW_STROKE_PATH},

};

static vg_lite_fill_t svgt12_test_fill_rule[] = {VG_LITE_FILL_EVEN_ODD, VG_LITE_FILL_EVEN_ODD,
						 VG_LITE_FILL_EVEN_ODD};

static gradient_mode_t svgt12_test_gradient_info = {.linearGrads = NULL,
						    .radialGrads = NULL,
						    .hybridPath = svgt12_test_hybrid_path,
						    .fillRule = svgt12_test_fill_rule};

static float svgt12_test_transform_matrix[] = {
	1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};

static float svgt12_test_transform_matrix_for_text_info[] = {
	1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

static msg_glyph_item_t svgt12_test_text_strings_0[] = {
	{.g_u16 = 0x0042, .k = 0},   /* B */
	{.g_u16 = 0x0061, .k = 0},   /* a */
	{.g_u16 = 0x0073, .k = 0},   /* s */
	{.g_u16 = 0x0069, .k = 0},   /* i */
	{.g_u16 = 0x0063, .k = 0},   /* c */
	{.g_u16 = 0x0020, .k = 0},   /*   */
	{.g_u16 = 0x0070, .k = 0},   /* p */
	{.g_u16 = 0x0061, .k = 0},   /* a */
	{.g_u16 = 0x0069, .k = 0},   /* i */
	{.g_u16 = 0x006e, .k = 0},   /* n */
	{.g_u16 = 0x0074, .k = 0},   /* t */
	{.g_u16 = 0x003a, .k = 28},  /* : */
	{.g_u16 = 0x0020, .k = 0},   /*   */
	{.g_u16 = 0x0073, .k = 0},   /* s */
	{.g_u16 = 0x0074, .k = 3},   /* t */
	{.g_u16 = 0x0072, .k = 0},   /* r */
	{.g_u16 = 0x006f, .k = 6},   /* o */
	{.g_u16 = 0x006b, .k = 0},   /* k */
	{.g_u16 = 0x0065, .k = 21},  /* e */
	{.g_u16 = 0x0020, .k = 0},   /*   */
	{.g_u16 = 0x0070, .k = 0},   /* p */
	{.g_u16 = 0x0072, .k = 0},   /* r */
	{.g_u16 = 0x006f, .k = 6},   /* o */
	{.g_u16 = 0x0070, .k = 0},   /* p */
	{.g_u16 = 0x0065, .k = 0},   /* e */
	{.g_u16 = 0x0072, .k = 0},   /* r */
	{.g_u16 = 0x0074, .k = -28}, /* t */
	{.g_u16 = 0x0069, .k = 0},   /* i */
	{.g_u16 = 0x0065, .k = 0},   /* e */
	{.g_u16 = 0x0073, .k = 0},   /* s */
	{.g_u16 = 0x002e, .k = 0},   /* . */
};
static msg_glyph_item_t svgt12_test_text_strings_1[] = {
	{.g_u16 = 0x0073, .k = 0},  /* s */
	{.g_u16 = 0x0074, .k = 3},  /* t */
	{.g_u16 = 0x0072, .k = 0},  /* r */
	{.g_u16 = 0x006f, .k = 6},  /* o */
	{.g_u16 = 0x006b, .k = 0},  /* k */
	{.g_u16 = 0x0065, .k = 21}, /* e */
	{.g_u16 = 0x003d, .k = 0},  /* = */
	{.g_u16 = 0x0022, .k = 0},  /* " */
	{.g_u16 = 0x006e, .k = 0},  /* n */
	{.g_u16 = 0x006f, .k = 0},  /* o */
	{.g_u16 = 0x006e, .k = 0},  /* n */
	{.g_u16 = 0x0065, .k = 0},  /* e */
	{.g_u16 = 0x0022, .k = 0},  /* " */
};
static msg_glyph_item_t svgt12_test_text_strings_2[] = {
	{.g_u16 = 0x0073, .k = 0},  /* s */
	{.g_u16 = 0x0074, .k = 3},  /* t */
	{.g_u16 = 0x0072, .k = 0},  /* r */
	{.g_u16 = 0x006f, .k = 6},  /* o */
	{.g_u16 = 0x006b, .k = 0},  /* k */
	{.g_u16 = 0x0065, .k = 21}, /* e */
	{.g_u16 = 0x003d, .k = 0},  /* = */
	{.g_u16 = 0x0022, .k = 0},  /* " */
	{.g_u16 = 0x0072, .k = 0},  /* r */
	{.g_u16 = 0x0065, .k = 11}, /* e */
	{.g_u16 = 0x0064, .k = 0},  /* d */
	{.g_u16 = 0x0022, .k = 0},  /* " */
};
static msg_glyph_item_t svgt12_test_text_strings_3[] = {
	{.g_u16 = 0x0024, .k = 0},  /* $ */
	{.g_u16 = 0x0052, .k = 0},  /* R */
	{.g_u16 = 0x0065, .k = 12}, /* e */
	{.g_u16 = 0x0076, .k = 15}, /* v */
	{.g_u16 = 0x0069, .k = 0},  /* i */
	{.g_u16 = 0x0073, .k = 0},  /* s */
	{.g_u16 = 0x0069, .k = 0},  /* i */
	{.g_u16 = 0x006f, .k = 0},  /* o */
	{.g_u16 = 0x006e, .k = 0},  /* n */
	{.g_u16 = 0x003a, .k = 0},  /* : */
	{.g_u16 = 0x0020, .k = 0},  /*   */
	{.g_u16 = 0x0031, .k = 0},  /* 1 */
	{.g_u16 = 0x002e, .k = 74}, /* . */
	{.g_u16 = 0x0036, .k = 0},  /* 6 */
	{.g_u16 = 0x0020, .k = 0},  /*   */
	{.g_u16 = 0x0024, .k = 0},  /* $ */
};

static svg_text_string_data_t svgt12_test_text_strings[] = {
	{
		/* [Basic paint: stroke properties.] */
		.x = 10,
		.y = 40,
		.font_size = 36,
		.tmatrix = svgt12_test_transform_matrix_for_text_info,
		.text_color = 0xff000000U,
		.direction = 0,
		.font_face = &svg1_SVGFreeSansASCII,
		.msg_len = 31,
		.msg_glyphs = svgt12_test_text_strings_0,
	},
	{
		/* [stroke="none"] */
		.x = 140,
		.y = 150,
		.font_size = 40,
		.tmatrix = svgt12_test_transform_matrix_for_text_info,
		.text_color = 0xff000000U,
		.direction = 0,
		.font_face = &svg1_SVGFreeSansASCII,
		.msg_len = 13,
		.msg_glyphs = svgt12_test_text_strings_1,
	},
	{
		/* [stroke="red"] */
		.x = 148,
		.y = 280,
		.font_size = 40,
		.tmatrix = svgt12_test_transform_matrix_for_text_info,
		.text_color = 0xff000000U,
		.direction = 0,
		.font_face = &svg1_SVGFreeSansASCII,
		.msg_len = 12,
		.msg_glyphs = svgt12_test_text_strings_2,
	},
	{
		/* [Revision: 1.6 ] */
		.x = 10,
		.y = 340,
		.font_size = 32,
		.tmatrix = svgt12_test_transform_matrix_for_text_info,
		.text_color = 0xff000000U,
		.direction = 0,
		.font_face = &svg1_SVGFreeSansASCII,
		.msg_len = 16,
		.msg_glyphs = svgt12_test_text_strings_3,
	}};

static svg_text_info_t svgt12_test_text_info_data = {.num_text_strings = 4,
						     .text_strings = svgt12_test_text_strings};

static svg_node_t svgt12_test_render_sequence[] = {
	eTextNode, eRectNode, eRectNode, eTextNode, eTextNode, eTextNode, eRectNode,
};

static image_info_t svgt12_test = {
	.image_name = "paint-stroke-01-t",
	.image_size = {480, 360},
	.data_format = VG_LITE_S32,
	.transform = svgt12_test_transform_matrix,
	.render_sequence = svgt12_test_render_sequence,
	.drawable_count = 7,
	.path_count = 3,
	.stroke_info = svgt12_test_stroke_info_data,
	.image_count = 0,
	.raw_images = (image_buf_data_t **)NULL,
	.text_info = &svgt12_test_text_info_data,
	.paths_info = {{.path_cmds = (uint8_t *)svgt12_test_rect_1_cmds,
			.path_args = (int32_t *)svgt12_test_rect_1_args,
			.path_length = sizeof(svgt12_test_rect_1_cmds),
			.end_path_flag = 0,
			.bounding_box = {90.00, 70.00, 390.00, 120.00}},
		       {.path_cmds = (uint8_t *)svgt12_test_rect_2_cmds,
			.path_args = (int32_t *)svgt12_test_rect_2_args,
			.path_length = sizeof(svgt12_test_rect_2_cmds),
			.end_path_flag = 0,
			.bounding_box = {90.00, 190.00, 390.00, 240.00}},
		       {.path_cmds = (uint8_t *)svgt12_test_rect_3_cmds,
			.path_args = (int32_t *)svgt12_test_rect_3_args,
			.path_length = sizeof(svgt12_test_rect_3_cmds),
			.end_path_flag = 0,
			.bounding_box = {1.00, 1.00, 479.00, 359.00}}},
};

uint32_t svgt12_test_color_data[] = {0xff0000ffU, 0xff0000ffU, 0xff000000U};
