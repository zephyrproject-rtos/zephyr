/* raise_int.c - utility functions for generating SW interrupt */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * DESCRIPTION
 * This file implements raiseInt(), which generates the specified SW interrupt
 * (in the range 0 to 255).
 * This method is being used because the MM_POMS model prevents the
 * execution of a dynamically generated stub.
 */

#include <stdint.h>

#if defined(CONFIG_X86_32)

static void genInt0 (void)
	{
	__asm__ volatile("int $0");
	}

static void genInt1 (void)
	{
	__asm__ volatile("int $1");
	}

static void genInt2 (void)
	{
	__asm__ volatile("int $2");
	}

static void genInt3 (void)
	{
	__asm__ volatile("int $3");
	}

static void genInt4 (void)
	{
	__asm__ volatile("int $4");
	}

static void genInt5 (void)
	{
	__asm__ volatile("int $5");
	}

static void genInt6 (void)
	{
	__asm__ volatile("int $6");
	}

static void genInt7 (void)
	{
	__asm__ volatile("int $7");
	}

static void genInt8 (void)
	{
	__asm__ volatile("int $8");
	}

static void genInt9 (void)
	{
	__asm__ volatile("int $9");
	}

static void genInt10 (void)
	{
	__asm__ volatile("int $10");
	}

static void genInt11 (void)
	{
	__asm__ volatile("int $11");
	}

static void genInt12 (void)
	{
	__asm__ volatile("int $12");
	}

static void genInt13 (void)
	{
	__asm__ volatile("int $13");
	}

static void genInt14 (void)
	{
	__asm__ volatile("int $14");
	}

static void genInt15 (void)
	{
	__asm__ volatile("int $15");
	}

static void genInt16 (void)
	{
	__asm__ volatile("int $16");
	}

static void genInt17 (void)
	{
	__asm__ volatile("int $17");
	}

static void genInt18 (void)
	{
	__asm__ volatile("int $18");
	}

static void genInt19 (void)
	{
	__asm__ volatile("int $19");
	}

static void genInt20 (void)
	{
	__asm__ volatile("int $20");
	}

static void genInt21 (void)
	{
	__asm__ volatile("int $21");
	}

static void genInt22 (void)
	{
	__asm__ volatile("int $22");
	}

static void genInt23 (void)
	{
	__asm__ volatile("int $23");
	}

static void genInt24 (void)
	{
	__asm__ volatile("int $24");
	}

static void genInt25 (void)
	{
	__asm__ volatile("int $25");
	}

static void genInt26 (void)
	{
	__asm__ volatile("int $26");
	}

static void genInt27 (void)
	{
	__asm__ volatile("int $27");
	}

static void genInt28 (void)
	{
	__asm__ volatile("int $28");
	}

static void genInt29 (void)
	{
	__asm__ volatile("int $29");
	}

static void genInt30 (void)
	{
	__asm__ volatile("int $30");
	}

static void genInt31 (void)
	{
	__asm__ volatile("int $31");
	}

static void genInt32 (void)
	{
	__asm__ volatile("int $32");
	}

static void genInt33 (void)
	{
	__asm__ volatile("int $33");
	}

static void genInt34 (void)
	{
	__asm__ volatile("int $34");
	}

static void genInt35 (void)
	{
	__asm__ volatile("int $35");
	}

static void genInt36 (void)
	{
	__asm__ volatile("int $36");
	}

static void genInt37 (void)
	{
	__asm__ volatile("int $37");
	}

static void genInt38 (void)
	{
	__asm__ volatile("int $38");
	}

static void genInt39 (void)
	{
	__asm__ volatile("int $39");
	}

static void genInt40 (void)
	{
	__asm__ volatile("int $40");
	}

static void genInt41 (void)
	{
	__asm__ volatile("int $41");
	}

static void genInt42 (void)
	{
	__asm__ volatile("int $42");
	}

static void genInt43 (void)
	{
	__asm__ volatile("int $43");
	}

static void genInt44 (void)
	{
	__asm__ volatile("int $44");
	}

static void genInt45 (void)
	{
	__asm__ volatile("int $45");
	}

static void genInt46 (void)
	{
	__asm__ volatile("int $46");
	}

static void genInt47 (void)
	{
	__asm__ volatile("int $47");
	}

static void genInt48 (void)
	{
	__asm__ volatile("int $48");
	}

static void genInt49 (void)
	{
	__asm__ volatile("int $49");
	}

static void genInt50 (void)
	{
	__asm__ volatile("int $50");
	}

static void genInt51 (void)
	{
	__asm__ volatile("int $51");
	}

static void genInt52 (void)
	{
	__asm__ volatile("int $52");
	}

static void genInt53 (void)
	{
	__asm__ volatile("int $53");
	}

static void genInt54 (void)
	{
	__asm__ volatile("int $54");
	}

static void genInt55 (void)
	{
	__asm__ volatile("int $55");
	}

static void genInt56 (void)
	{
	__asm__ volatile("int $56");
	}

static void genInt57 (void)
	{
	__asm__ volatile("int $57");
	}

static void genInt58 (void)
	{
	__asm__ volatile("int $58");
	}

static void genInt59 (void)
	{
	__asm__ volatile("int $59");
	}

static void genInt60 (void)
	{
	__asm__ volatile("int $60");
	}

static void genInt61 (void)
	{
	__asm__ volatile("int $61");
	}

static void genInt62 (void)
	{
	__asm__ volatile("int $62");
	}

static void genInt63 (void)
	{
	__asm__ volatile("int $63");
	}

static void genInt64 (void)
	{
	__asm__ volatile("int $64");
	}

static void genInt65 (void)
	{
	__asm__ volatile("int $65");
	}

static void genInt66 (void)
	{
	__asm__ volatile("int $66");
	}

static void genInt67 (void)
	{
	__asm__ volatile("int $67");
	}

static void genInt68 (void)
	{
	__asm__ volatile("int $68");
	}

static void genInt69 (void)
	{
	__asm__ volatile("int $69");
	}

static void genInt70 (void)
	{
	__asm__ volatile("int $70");
	}

static void genInt71 (void)
	{
	__asm__ volatile("int $71");
	}

static void genInt72 (void)
	{
	__asm__ volatile("int $72");
	}

static void genInt73 (void)
	{
	__asm__ volatile("int $73");
	}

static void genInt74 (void)
	{
	__asm__ volatile("int $74");
	}

static void genInt75 (void)
	{
	__asm__ volatile("int $75");
	}

static void genInt76 (void)
	{
	__asm__ volatile("int $76");
	}

static void genInt77 (void)
	{
	__asm__ volatile("int $77");
	}

static void genInt78 (void)
	{
	__asm__ volatile("int $78");
	}

static void genInt79 (void)
	{
	__asm__ volatile("int $79");
	}

static void genInt80 (void)
	{
	__asm__ volatile("int $80");
	}

static void genInt81 (void)
	{
	__asm__ volatile("int $81");
	}

static void genInt82 (void)
	{
	__asm__ volatile("int $82");
	}

static void genInt83 (void)
	{
	__asm__ volatile("int $83");
	}

static void genInt84 (void)
	{
	__asm__ volatile("int $84");
	}

static void genInt85 (void)
	{
	__asm__ volatile("int $85");
	}

static void genInt86 (void)
	{
	__asm__ volatile("int $86");
	}

static void genInt87 (void)
	{
	__asm__ volatile("int $87");
	}

static void genInt88 (void)
	{
	__asm__ volatile("int $88");
	}

static void genInt89 (void)
	{
	__asm__ volatile("int $89");
	}

static void genInt90 (void)
	{
	__asm__ volatile("int $90");
	}

static void genInt91 (void)
	{
	__asm__ volatile("int $91");
	}

static void genInt92 (void)
	{
	__asm__ volatile("int $92");
	}

static void genInt93 (void)
	{
	__asm__ volatile("int $93");
	}

static void genInt94 (void)
	{
	__asm__ volatile("int $94");
	}

static void genInt95 (void)
	{
	__asm__ volatile("int $95");
	}

static void genInt96 (void)
	{
	__asm__ volatile("int $96");
	}

static void genInt97 (void)
	{
	__asm__ volatile("int $97");
	}

static void genInt98 (void)
	{
	__asm__ volatile("int $98");
	}

static void genInt99 (void)
	{
	__asm__ volatile("int $99");
	}

static void genInt100 (void)
	{
	__asm__ volatile("int $100");
	}

static void genInt101 (void)
	{
	__asm__ volatile("int $101");
	}

static void genInt102 (void)
	{
	__asm__ volatile("int $102");
	}

static void genInt103 (void)
	{
	__asm__ volatile("int $103");
	}

static void genInt104 (void)
	{
	__asm__ volatile("int $104");
	}

static void genInt105 (void)
	{
	__asm__ volatile("int $105");
	}

static void genInt106 (void)
	{
	__asm__ volatile("int $106");
	}

static void genInt107 (void)
	{
	__asm__ volatile("int $107");
	}

static void genInt108 (void)
	{
	__asm__ volatile("int $108");
	}

static void genInt109 (void)
	{
	__asm__ volatile("int $109");
	}

static void genInt110 (void)
	{
	__asm__ volatile("int $110");
	}

static void genInt111 (void)
	{
	__asm__ volatile("int $111");
	}

static void genInt112 (void)
	{
	__asm__ volatile("int $112");
	}

static void genInt113 (void)
	{
	__asm__ volatile("int $113");
	}

static void genInt114 (void)
	{
	__asm__ volatile("int $114");
	}

static void genInt115 (void)
	{
	__asm__ volatile("int $115");
	}

static void genInt116 (void)
	{
	__asm__ volatile("int $116");
	}

static void genInt117 (void)
	{
	__asm__ volatile("int $117");
	}

static void genInt118 (void)
	{
	__asm__ volatile("int $118");
	}

static void genInt119 (void)
	{
	__asm__ volatile("int $119");
	}

static void genInt120 (void)
	{
	__asm__ volatile("int $120");
	}

static void genInt121 (void)
	{
	__asm__ volatile("int $121");
	}

static void genInt122 (void)
	{
	__asm__ volatile("int $122");
	}

static void genInt123 (void)
	{
	__asm__ volatile("int $123");
	}

static void genInt124 (void)
	{
	__asm__ volatile("int $124");
	}

static void genInt125 (void)
	{
	__asm__ volatile("int $125");
	}

static void genInt126 (void)
	{
	__asm__ volatile("int $126");
	}

static void genInt127 (void)
	{
	__asm__ volatile("int $127");
	}

static void genInt128 (void)
	{
	__asm__ volatile("int $128");
	}

static void genInt129 (void)
	{
	__asm__ volatile("int $129");
	}

static void genInt130 (void)
	{
	__asm__ volatile("int $130");
	}

static void genInt131 (void)
	{
	__asm__ volatile("int $131");
	}

static void genInt132 (void)
	{
	__asm__ volatile("int $132");
	}

static void genInt133 (void)
	{
	__asm__ volatile("int $133");
	}

static void genInt134 (void)
	{
	__asm__ volatile("int $134");
	}

static void genInt135 (void)
	{
	__asm__ volatile("int $135");
	}

static void genInt136 (void)
	{
	__asm__ volatile("int $136");
	}

static void genInt137 (void)
	{
	__asm__ volatile("int $137");
	}

static void genInt138 (void)
	{
	__asm__ volatile("int $138");
	}

static void genInt139 (void)
	{
	__asm__ volatile("int $139");
	}

static void genInt140 (void)
	{
	__asm__ volatile("int $140");
	}

static void genInt141 (void)
	{
	__asm__ volatile("int $141");
	}

static void genInt142 (void)
	{
	__asm__ volatile("int $142");
	}

static void genInt143 (void)
	{
	__asm__ volatile("int $143");
	}

static void genInt144 (void)
	{
	__asm__ volatile("int $144");
	}

static void genInt145 (void)
	{
	__asm__ volatile("int $145");
	}

static void genInt146 (void)
	{
	__asm__ volatile("int $146");
	}

static void genInt147 (void)
	{
	__asm__ volatile("int $147");
	}

static void genInt148 (void)
	{
	__asm__ volatile("int $148");
	}

static void genInt149 (void)
	{
	__asm__ volatile("int $149");
	}

static void genInt150 (void)
	{
	__asm__ volatile("int $150");
	}

static void genInt151 (void)
	{
	__asm__ volatile("int $151");
	}

static void genInt152 (void)
	{
	__asm__ volatile("int $152");
	}

static void genInt153 (void)
	{
	__asm__ volatile("int $153");
	}

static void genInt154 (void)
	{
	__asm__ volatile("int $154");
	}

static void genInt155 (void)
	{
	__asm__ volatile("int $155");
	}

static void genInt156 (void)
	{
	__asm__ volatile("int $156");
	}

static void genInt157 (void)
	{
	__asm__ volatile("int $157");
	}

static void genInt158 (void)
	{
	__asm__ volatile("int $158");
	}

static void genInt159 (void)
	{
	__asm__ volatile("int $159");
	}

static void genInt160 (void)
	{
	__asm__ volatile("int $160");
	}

static void genInt161 (void)
	{
	__asm__ volatile("int $161");
	}

static void genInt162 (void)
	{
	__asm__ volatile("int $162");
	}

static void genInt163 (void)
	{
	__asm__ volatile("int $163");
	}

static void genInt164 (void)
	{
	__asm__ volatile("int $164");
	}

static void genInt165 (void)
	{
	__asm__ volatile("int $165");
	}

static void genInt166 (void)
	{
	__asm__ volatile("int $166");
	}

static void genInt167 (void)
	{
	__asm__ volatile("int $167");
	}

static void genInt168 (void)
	{
	__asm__ volatile("int $168");
	}

static void genInt169 (void)
	{
	__asm__ volatile("int $169");
	}

static void genInt170 (void)
	{
	__asm__ volatile("int $170");
	}

static void genInt171 (void)
	{
	__asm__ volatile("int $171");
	}

static void genInt172 (void)
	{
	__asm__ volatile("int $172");
	}

static void genInt173 (void)
	{
	__asm__ volatile("int $173");
	}

static void genInt174 (void)
	{
	__asm__ volatile("int $174");
	}

static void genInt175 (void)
	{
	__asm__ volatile("int $175");
	}

static void genInt176 (void)
	{
	__asm__ volatile("int $176");
	}

static void genInt177 (void)
	{
	__asm__ volatile("int $177");
	}

static void genInt178 (void)
	{
	__asm__ volatile("int $178");
	}

static void genInt179 (void)
	{
	__asm__ volatile("int $179");
	}

static void genInt180 (void)
	{
	__asm__ volatile("int $180");
	}

static void genInt181 (void)
	{
	__asm__ volatile("int $181");
	}

static void genInt182 (void)
	{
	__asm__ volatile("int $182");
	}

static void genInt183 (void)
	{
	__asm__ volatile("int $183");
	}

static void genInt184 (void)
	{
	__asm__ volatile("int $184");
	}

static void genInt185 (void)
	{
	__asm__ volatile("int $185");
	}

static void genInt186 (void)
	{
	__asm__ volatile("int $186");
	}

static void genInt187 (void)
	{
	__asm__ volatile("int $187");
	}

static void genInt188 (void)
	{
	__asm__ volatile("int $188");
	}

static void genInt189 (void)
	{
	__asm__ volatile("int $189");
	}

static void genInt190 (void)
	{
	__asm__ volatile("int $190");
	}

static void genInt191 (void)
	{
	__asm__ volatile("int $191");
	}

static void genInt192 (void)
	{
	__asm__ volatile("int $192");
	}

static void genInt193 (void)
	{
	__asm__ volatile("int $193");
	}

static void genInt194 (void)
	{
	__asm__ volatile("int $194");
	}

static void genInt195 (void)
	{
	__asm__ volatile("int $195");
	}

static void genInt196 (void)
	{
	__asm__ volatile("int $196");
	}

static void genInt197 (void)
	{
	__asm__ volatile("int $197");
	}

static void genInt198 (void)
	{
	__asm__ volatile("int $198");
	}

static void genInt199 (void)
	{
	__asm__ volatile("int $199");
	}

static void genInt200 (void)
	{
	__asm__ volatile("int $200");
	}

static void genInt201 (void)
	{
	__asm__ volatile("int $201");
	}

static void genInt202 (void)
	{
	__asm__ volatile("int $202");
	}

static void genInt203 (void)
	{
	__asm__ volatile("int $203");
	}

static void genInt204 (void)
	{
	__asm__ volatile("int $204");
	}

static void genInt205 (void)
	{
	__asm__ volatile("int $205");
	}

static void genInt206 (void)
	{
	__asm__ volatile("int $206");
	}

static void genInt207 (void)
	{
	__asm__ volatile("int $207");
	}

static void genInt208 (void)
	{
	__asm__ volatile("int $208");
	}

static void genInt209 (void)
	{
	__asm__ volatile("int $209");
	}

static void genInt210 (void)
	{
	__asm__ volatile("int $210");
	}

static void genInt211 (void)
	{
	__asm__ volatile("int $211");
	}

static void genInt212 (void)
	{
	__asm__ volatile("int $212");
	}

static void genInt213 (void)
	{
	__asm__ volatile("int $213");
	}

static void genInt214 (void)
	{
	__asm__ volatile("int $214");
	}

static void genInt215 (void)
	{
	__asm__ volatile("int $215");
	}

static void genInt216 (void)
	{
	__asm__ volatile("int $216");
	}

static void genInt217 (void)
	{
	__asm__ volatile("int $217");
	}

static void genInt218 (void)
	{
	__asm__ volatile("int $218");
	}

static void genInt219 (void)
	{
	__asm__ volatile("int $219");
	}

static void genInt220 (void)
	{
	__asm__ volatile("int $220");
	}

static void genInt221 (void)
	{
	__asm__ volatile("int $221");
	}

static void genInt222 (void)
	{
	__asm__ volatile("int $222");
	}

static void genInt223 (void)
	{
	__asm__ volatile("int $223");
	}

static void genInt224 (void)
	{
	__asm__ volatile("int $224");
	}

static void genInt225 (void)
	{
	__asm__ volatile("int $225");
	}

static void genInt226 (void)
	{
	__asm__ volatile("int $226");
	}

static void genInt227 (void)
	{
	__asm__ volatile("int $227");
	}

static void genInt228 (void)
	{
	__asm__ volatile("int $228");
	}

static void genInt229 (void)
	{
	__asm__ volatile("int $229");
	}

static void genInt230 (void)
	{
	__asm__ volatile("int $230");
	}

static void genInt231 (void)
	{
	__asm__ volatile("int $231");
	}

static void genInt232 (void)
	{
	__asm__ volatile("int $232");
	}

static void genInt233 (void)
	{
	__asm__ volatile("int $233");
	}

static void genInt234 (void)
	{
	__asm__ volatile("int $234");
	}

static void genInt235 (void)
	{
	__asm__ volatile("int $235");
	}

static void genInt236 (void)
	{
	__asm__ volatile("int $236");
	}

static void genInt237 (void)
	{
	__asm__ volatile("int $237");
	}

static void genInt238 (void)
	{
	__asm__ volatile("int $238");
	}

static void genInt239 (void)
	{
	__asm__ volatile("int $239");
	}

static void genInt240 (void)
	{
	__asm__ volatile("int $240");
	}

static void genInt241 (void)
	{
	__asm__ volatile("int $241");
	}

static void genInt242 (void)
	{
	__asm__ volatile("int $242");
	}

static void genInt243 (void)
	{
	__asm__ volatile("int $243");
	}

static void genInt244 (void)
	{
	__asm__ volatile("int $244");
	}

static void genInt245 (void)
	{
	__asm__ volatile("int $245");
	}

static void genInt246 (void)
	{
	__asm__ volatile("int $246");
	}

static void genInt247 (void)
	{
	__asm__ volatile("int $247");
	}

static void genInt248 (void)
	{
	__asm__ volatile("int $248");
	}

static void genInt249 (void)
	{
	__asm__ volatile("int $249");
	}

static void genInt250 (void)
	{
	__asm__ volatile("int $250");
	}

static void genInt251 (void)
	{
	__asm__ volatile("int $251");
	}

static void genInt252 (void)
	{
	__asm__ volatile("int $252");
	}

static void genInt253 (void)
	{
	__asm__ volatile("int $253");
	}

static void genInt254 (void)
	{
	__asm__ volatile("int $254");
	}

static void genInt255 (void)
	{
	__asm__ volatile("int $255");
	}

static void (*intFPtr[256])(void) = {
		                    genInt0, genInt1, genInt2,genInt3,
		                    genInt4, genInt5, genInt6,genInt7,
		                    genInt8, genInt9, genInt10,genInt11,
		                    genInt12, genInt13, genInt14,genInt15,
		                    genInt16, genInt17, genInt18,genInt19,
		                    genInt20, genInt21, genInt22,genInt23,
		                    genInt24, genInt25, genInt26,genInt27,
		                    genInt28, genInt29, genInt30,genInt31,
		                    genInt32, genInt33, genInt34,genInt35,
		                    genInt36, genInt37, genInt38,genInt39,
		                    genInt40, genInt41, genInt42,genInt43,
		                    genInt44, genInt45, genInt46,genInt47,
		                    genInt48, genInt49, genInt50,genInt51,
		                    genInt52, genInt53, genInt54,genInt55,
		                    genInt56, genInt57, genInt58,genInt59,
		                    genInt60, genInt61, genInt62,genInt63,
		                    genInt64, genInt65, genInt66,genInt67,
		                    genInt68, genInt69, genInt70,genInt71,
		                    genInt72, genInt73, genInt74,genInt75,
		                    genInt76, genInt77, genInt78,genInt79,
		                    genInt80, genInt81, genInt82,genInt83,
		                    genInt84, genInt85, genInt86,genInt87,
		                    genInt88, genInt89, genInt90,genInt91,
		                    genInt92, genInt93, genInt94,genInt95,
		                    genInt96, genInt97, genInt98,genInt99,
		                    genInt100, genInt101, genInt102,genInt103,
		                    genInt104, genInt105, genInt106,genInt107,
		                    genInt108, genInt109, genInt110,genInt111,
		                    genInt112, genInt113, genInt114,genInt115,
		                    genInt116, genInt117, genInt118,genInt119,
		                    genInt120, genInt121, genInt122,genInt123,
		                    genInt124, genInt125, genInt126,genInt127,
		                    genInt128, genInt129, genInt130,genInt131,
		                    genInt132, genInt133, genInt134,genInt135,
		                    genInt136, genInt137, genInt138,genInt139,
		                    genInt140, genInt141, genInt142,genInt143,
		                    genInt144, genInt145, genInt146,genInt147,
		                    genInt148, genInt149, genInt150,genInt151,
		                    genInt152, genInt153, genInt154,genInt155,
		                    genInt156, genInt157, genInt158,genInt159,
		                    genInt160, genInt161, genInt162,genInt163,
		                    genInt164, genInt165, genInt166,genInt167,
		                    genInt168, genInt169, genInt170,genInt171,
		                    genInt172, genInt173, genInt174,genInt175,
		                    genInt176, genInt177, genInt178,genInt179,
		                    genInt180, genInt181, genInt182,genInt183,
		                    genInt184, genInt185, genInt186,genInt187,
		                    genInt188, genInt189, genInt190,genInt191,
		                    genInt192, genInt193, genInt194,genInt195,
		                    genInt196, genInt197, genInt198,genInt199,
		                    genInt200, genInt201, genInt202,genInt203,
		                    genInt204, genInt205, genInt206,genInt207,
		                    genInt208, genInt209, genInt210,genInt211,
		                    genInt212, genInt213, genInt214,genInt215,
		                    genInt216, genInt217, genInt218,genInt219,
		                    genInt220, genInt221, genInt222,genInt223,
		                    genInt224, genInt225, genInt226,genInt227,
		                    genInt228, genInt229, genInt230,genInt231,
		                    genInt232, genInt233, genInt234,genInt235,
		                    genInt236, genInt237, genInt238,genInt239,
		                    genInt240, genInt241, genInt242,genInt243,
		                    genInt244, genInt245, genInt246,genInt247,
		                    genInt248, genInt249, genInt250,genInt251,
		                    genInt252, genInt253, genInt254,genInt255};

/**
 *
 * @brief Generate a software interrupt
 *
 * This routine will call one of the genInt functions based upon the
 * value passed to it (which is essentially the interrupt vector number).
 *
 * @return N/A
 */
void raiseInt(uint8_t id)
	{
	(*intFPtr[id])();
	}
#endif /* Intel */

#if defined(CONFIG_CPU_CORTEX_M3_M4)
#include <arch/cpu.h>
/**
 *
 * @brief Generate a software interrupt
 *
 * Trigger via NVIC. <id> is the IRQ number.
 *
 * @return N/A
 */
void raiseInt(uint8_t id)
	{
	_NvicSwInterruptTrigger((unsigned int)id);
	}
#endif
