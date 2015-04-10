/*
 * Copyright (c) 2011, Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* confdata.c */
P(conf_parse,void,(const char *name));
P(conf_read,int,(const char *name));
P(conf_read_simple,int,(const char *name, int));
P(conf_write,int,(const char *name));
P(conf_write_autoconf,int,(void));
P(conf_get_changed,bool,(void));
P(conf_set_changed_callback, void,(void (*fn)(void)));

/* menu.c */
P(rootmenu,struct menu,);

P(menu_is_visible,bool,(struct menu *menu));
P(menu_get_prompt,const char *,(struct menu *menu));
P(menu_get_root_menu,struct menu *,(struct menu *menu));
P(menu_get_parent_menu,struct menu *,(struct menu *menu));
P(menu_has_help,bool,(struct menu *menu));
P(menu_get_help,const char *,(struct menu *menu));

/* symbol.c */
P(symbol_hash,struct symbol *,[SYMBOL_HASHSIZE]);

P(sym_lookup,struct symbol *,(const char *name, int isconst));
P(sym_find,struct symbol *,(const char *name));
P(sym_re_search,struct symbol **,(const char *pattern));
P(sym_type_name,const char *,(enum symbol_type type));
P(sym_calc_value,void,(struct symbol *sym));
P(sym_get_type,enum symbol_type,(struct symbol *sym));
P(sym_tristate_within_range,bool,(struct symbol *sym,tristate tri));
P(sym_set_tristate_value,bool,(struct symbol *sym,tristate tri));
P(sym_toggle_tristate_value,tristate,(struct symbol *sym));
P(sym_string_valid,bool,(struct symbol *sym, const char *newval));
P(sym_string_within_range,bool,(struct symbol *sym, const char *str));
P(sym_set_string_value,bool,(struct symbol *sym, const char *newval));
P(sym_is_changable,bool,(struct symbol *sym));
P(sym_get_choice_prop,struct property *,(struct symbol *sym));
P(sym_get_default_prop,struct property *,(struct symbol *sym));
P(sym_get_string_value,const char *,(struct symbol *sym));

P(prop_get_type_name,const char *,(enum prop_type type));

/* expr.c */
P(expr_compare_type,int,(enum expr_type t1, enum expr_type t2));
P(expr_print,void,(struct expr *e, void (*fn)(void *, struct symbol *, const char *), void *data, int prevtoken));
