%option nostdinit noyywrap never-interactive full ecs
%option 8bit nodefault perf-report perf-report
%option noinput
%x COMMAND HELP STRING PARAM
%{
/*
 * Copyright (C) 2002 Roman Zippel <zippel@linux-m68k.org>
 * Released under the terms of the GNU GPL v2.0.
 */

#if defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#else
#include <glob.h>
#endif
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lkc.h"

#define START_STRSIZE	16

static struct {
	struct file *file;
	int lineno;
} current_pos;

static char *text;
static int text_size, text_asize;

struct buffer {
	struct buffer *parent;
	YY_BUFFER_STATE state;
};

struct buffer *current_buf;

static int last_ts, first_ts;

static void zconf_endhelp(void);
static void zconf_endfile(void);

static void new_string(void)
{
	text = xmalloc(START_STRSIZE);
	text_asize = START_STRSIZE;
	text_size = 0;
	*text = 0;
}

static void append_string(const char *str, int size)
{
	int new_size = text_size + size + 1;
	if (new_size > text_asize) {
		new_size += START_STRSIZE - 1;
		new_size &= -START_STRSIZE;
		text = realloc(text, new_size);
		text_asize = new_size;
	}
	memcpy(text + text_size, str, size);
	text_size += size;
	text[text_size] = 0;
}

static void alloc_string(const char *str, int size)
{
	text = xmalloc(size + 1);
	memcpy(text, str, size);
	text[size] = 0;
}
%}

n	[A-Za-z0-9_]

%%
	int str = 0;
	int ts, i;

[ \t]*#.*\n	|
[ \t]*\n	{
	current_file->lineno++;
	return T_EOL;
}
[ \t]*#.*


[ \t]+	{
	BEGIN(COMMAND);
}

.	{
	unput(yytext[0]);
	BEGIN(COMMAND);
}


<COMMAND>{
	{n}+	{
		const struct kconf_id *id = kconf_id_lookup(yytext, yyleng);
		BEGIN(PARAM);
		current_pos.file = current_file;
		current_pos.lineno = current_file->lineno;
		if (id && id->flags & TF_COMMAND) {
			zconflval.id = id;
			return id->token;
		}
		alloc_string(yytext, yyleng);
		zconflval.string = text;
		return T_WORD;
	}
	.
	\n	{
		BEGIN(INITIAL);
		current_file->lineno++;
		return T_EOL;
	}
}

<PARAM>{
	"&&"	return T_AND;
	"||"	return T_OR;
	"("	return T_OPEN_PAREN;
	")"	return T_CLOSE_PAREN;
	"!"	return T_NOT;
	"="	return T_EQUAL;
	"!="	return T_UNEQUAL;
	\"|\'	{
		str = yytext[0];
		new_string();
		BEGIN(STRING);
	}
	\n	BEGIN(INITIAL); current_file->lineno++; return T_EOL;
	---	/* ignore */
	({n}|[-/.])+	{
		const struct kconf_id *id = kconf_id_lookup(yytext, yyleng);
		if (id && id->flags & TF_PARAM) {
			zconflval.id = id;
			return id->token;
		}
		alloc_string(yytext, yyleng);
		zconflval.string = text;
		return T_WORD;
	}
	#.*	/* comment */
	\\\n	current_file->lineno++;
	.
	<<EOF>> {
		BEGIN(INITIAL);
	}
}

<STRING>{
	[^'"\\\n]+/\n	{
		append_string(yytext, yyleng);
		zconflval.string = text;
		return T_WORD_QUOTE;
	}
	[^'"\\\n]+	{
		append_string(yytext, yyleng);
	}
	\\.?/\n	{
		append_string(yytext + 1, yyleng - 1);
		zconflval.string = text;
		return T_WORD_QUOTE;
	}
	\\.?	{
		append_string(yytext + 1, yyleng - 1);
	}
	\'|\"	{
		if (str == yytext[0]) {
			BEGIN(PARAM);
			zconflval.string = text;
			return T_WORD_QUOTE;
		} else
			append_string(yytext, 1);
	}
	\n	{
		printf("%s:%d:warning: multi-line strings not supported\n", zconf_curname(), zconf_lineno());
		current_file->lineno++;
		BEGIN(INITIAL);
		return T_EOL;
	}
	<<EOF>>	{
		BEGIN(INITIAL);
	}
}

<HELP>{
	[ \t]+	{
		ts = 0;
		for (i = 0; i < yyleng; i++) {
			if (yytext[i] == '\t')
				ts = (ts & ~7) + 8;
			else
				ts++;
		}
		last_ts = ts;
		if (first_ts) {
			if (ts < first_ts) {
				zconf_endhelp();
				return T_HELPTEXT;
			}
			ts -= first_ts;
			while (ts > 8) {
				append_string("        ", 8);
				ts -= 8;
			}
			append_string("        ", ts);
		}
	}
	[ \t]*\n/[^ \t\n] {
		current_file->lineno++;
		zconf_endhelp();
		return T_HELPTEXT;
	}
	[ \t]*\n	{
		current_file->lineno++;
		append_string("\n", 1);
	}
	[^ \t\n].* {
		while (yyleng) {
			if ((yytext[yyleng-1] != ' ') && (yytext[yyleng-1] != '\t'))
				break;
			yyleng--;
		}
		append_string(yytext, yyleng);
		if (!first_ts)
			first_ts = last_ts;
	}
	<<EOF>>	{
		zconf_endhelp();
		return T_HELPTEXT;
	}
}

<<EOF>>	{
	if (current_file) {
		zconf_endfile();
		return T_EOL;
	}
	fclose(yyin);
	yyterminate();
}

%%
void zconf_starthelp(void)
{
	new_string();
	last_ts = first_ts = 0;
	BEGIN(HELP);
}

static void zconf_endhelp(void)
{
	zconflval.string = text;
	BEGIN(INITIAL);
}


/*
 * Try to open specified file with following names:
 * ./name
 * $(srctree)/name
 * The latter is used when srctree is separate from objtree
 * when compiling the kernel.
 * Return NULL if file is not found.
 */
FILE *zconf_fopen(const char *name)
{
	char *env, fullname[PATH_MAX+1];
	FILE *f;

	f = fopen(name, "r");
	if (!f && name != NULL && name[0] != '/') {
		env = getenv(SRCTREE);
		if (env) {
			sprintf(fullname, "%s/%s", env, name);
			f = fopen(fullname, "r");
		}
	}
	return f;
}

void zconf_initscan(const char *name)
{
	yyin = zconf_fopen(name);
	if (!yyin) {
		printf("can't find file %s\n", name);
		exit(1);
	}

	current_buf = xmalloc(sizeof(*current_buf));
	memset(current_buf, 0, sizeof(*current_buf));

	current_file = file_lookup(name);
	current_file->lineno = 1;
}

void zconf_nextfile(const char *name)
{
	struct file *iter;
	struct file *file = file_lookup(name);
	struct buffer *buf = xmalloc(sizeof(*buf));
	memset(buf, 0, sizeof(*buf));

	current_buf->state = YY_CURRENT_BUFFER;
	yyin = zconf_fopen(file->name);
	if (!yyin) {
		printf("%s:%d: can't open file \"%s\"\n",
		    zconf_curname(), zconf_lineno(), file->name);
		exit(1);
	}
	yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
	buf->parent = current_buf;
	current_buf = buf;

	for (iter = current_file->parent; iter; iter = iter->parent ) {
		if (!strcmp(current_file->name,iter->name) ) {
			printf("%s:%d: recursive inclusion detected. "
			       "Inclusion path:\n  current file : '%s'\n",
			       zconf_curname(), zconf_lineno(),
			       zconf_curname());
			iter = current_file->parent;
			while (iter && \
			       strcmp(iter->name,current_file->name)) {
				printf("  included from: '%s:%d'\n",
				       iter->name, iter->lineno-1);
				iter = iter->parent;
			}
			if (iter)
				printf("  included from: '%s:%d'\n",
				       iter->name, iter->lineno+1);
			exit(1);
		}
	}
	file->lineno = 1;
	file->parent = current_file;
	current_file = file;
}


#if defined(_WIN32) || defined(__WIN32__)
void zconf_nextfiles(const char *wildcard);
int search_directory_wildcard(const char *path);
void remove_last_part(char *path);
void win_process_files(const char *files_path);
void win_process_directories(char *directories_path, char *remain_path);


int search_directory_wildcard(const char *path)
{
	int wildcard_found=0;
	int len=strlen(path);
	int i;

	for (i=0; i<len; i++) {
		if(wildcard_found) {
			if (path[i] == '\\' || path[i] == '/') {
				return i+1;
			}
		} else {
			if ((path[i] == '?') || (path[i] == '*')) {
				wildcard_found = 1;
			}
		}
	}

	return 0;
}

void remove_last_part(char *path)
{
	int i;

	for(i=strlen(path); i>0; i--) {
		if (path[i] == '\\' || path[i] == '/') {
			path[i] = '\0';
			return;
		}
	}
}

void win_process_files(const char *files_path)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError;
	char *env, fullname[PATH_MAX+1], path[PATH_MAX+1];
	const char *expanded=sym_expand_string_value(files_path);
	TCHAR** lppPart=NULL;

	strcpy(fullname, expanded);

	// Find the first file in the directory.
	hFind = FindFirstFile(fullname, &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		env = getenv(SRCTREE);
		if (env) {
			sprintf(fullname, "%s/%s", env, expanded);
		}
		hFind = FindFirstFile(fullname, &FindFileData);
	}

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			char *strfile;
			GetFullPathName(fullname, PATH_MAX, path, lppPart);
			strcpy(fullname, path);
			strfile = strrchr(fullname, '\\');
			if(strfile){
				sprintf(strfile, "\\%s", FindFileData.cFileName);
			}
			zconf_nextfile(fullname);
		} while (FindNextFile(hFind, &FindFileData) != 0);

		dwError = GetLastError();
		FindClose(hFind);
		if (dwError != ERROR_NO_MORE_FILES) {
			printf ("Error processing '%s' #%lu.\n", fullname, dwError);
		}
	}
}

void win_process_directories(char *directories_path, char *remain_path)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError;
	char *env, fullname[PATH_MAX+1], path[PATH_MAX+1];
	TCHAR** lppPart=NULL;

	strcpy(fullname, directories_path);
	// Find the first file/directory in the path.
	hFind = FindFirstFile(fullname, &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		env = getenv(SRCTREE);
		if (env) {
			sprintf(path, "%s/%s", env, fullname);
			strcpy(fullname, path);
		}
		hFind = FindFirstFile(fullname, &FindFileData);
	}

	if (hFind != INVALID_HANDLE_VALUE) {
		remove_last_part(fullname);
		do {
			// Only the directories are processed
			if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
				(strcmp(FindFileData.cFileName, ".") &&
				strcmp(FindFileData.cFileName, ".."))) {
				GetFullPathName(fullname, PATH_MAX, path, lppPart);
				strcpy(fullname, path);
				sprintf(path, "%s\\%s\\%s", fullname, FindFileData.cFileName, remain_path);
				zconf_nextfiles(path);
			}
		} while (FindNextFile(hFind, &FindFileData) != 0);

		dwError = GetLastError();
		FindClose(hFind);
		if (dwError != ERROR_NO_MORE_FILES) {
			printf ("Error processing '%s' #%lu.\n", fullname, dwError);
		}
	} else {
		printf ("Error processing '%s' #%lu.\n", fullname, GetLastError());
	}
}

void zconf_nextfiles(const char *wildcard)
{
	int index_remain=0;

	index_remain = search_directory_wildcard(wildcard);
	if (index_remain) {
		char *new_section_path=malloc(index_remain + 1);
		char *new_remain_path=malloc(strlen(wildcard) - index_remain + 1);

		strncpy(new_section_path, wildcard, index_remain);
		new_section_path[index_remain-1] = '\0';

		strcpy(new_remain_path, &(wildcard[index_remain]));

		win_process_directories(new_section_path, new_remain_path);

		free(new_remain_path);
		free(new_section_path);
	} else {
		win_process_files(wildcard);
	}
}

#else /* Linux host */

void zconf_nextfiles(const char *wildcard)
{
	glob_t g;
	char **w;
	int i;
	char *env, fullname[PATH_MAX+1];
	const char *expanded = sym_expand_string_value(wildcard);

	if (glob(expanded, 0, NULL, &g) != 0) {
		env = getenv(SRCTREE);
		if (env) {
			sprintf(fullname, "%s/%s", env, expanded);
			if (glob(fullname, 0, NULL, &g) != 0) {
				return;
			}
		} else {
			return;
		}
	}
	if (g.gl_pathv == NULL) {
		globfree(&g);
		return;
	}

	/* working through files backwards, since
	 * we're first pushing them on a stack
	 * before actually handling them.
	 */
	for (i = g.gl_pathc; i > 0; i--) {
		w = &g.gl_pathv[i - 1];
		zconf_nextfile(*w);
	}

	globfree(&g);
}
#endif

static void zconf_endfile(void)
{
	struct buffer *parent;

	current_file = current_file->parent;

	parent = current_buf->parent;
	if (parent) {
		fclose(yyin);
		yy_delete_buffer(YY_CURRENT_BUFFER);
		yy_switch_to_buffer(parent->state);
	}
	free(current_buf);
	current_buf = parent;
}

int zconf_lineno(void)
{
	return current_pos.lineno;
}

const char *zconf_curname(void)
{
	return current_pos.file ? current_pos.file->name : "<none>";
}
