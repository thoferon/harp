%{
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <libgen.h>

#include <smem.h>
#include <utils.h>
#include <config_parser.h>
#include <harp.h>
#include <lexer_extra.h>

void put_number(yyscan_t);
void put_string(yyscan_t);
int include(yyscan_t);
int shift_extra_file(yyscan_t);
%}

%option 8bit reentrant bison-bridge
%option warn nodefault nounput noyywrap yylineno
%option header-file="config_lexer.h"
%option extra-type="extra_t *"

%pointer

%x incl

LBRACE      "{"
RBRACE      "}"
LSQUARE     "["
RSQUARE     "]"
WHITESPACES [ \t\r\n]+
SEMICOLON   ";"

DHOSTNAMES  "hostnames"
DPORTS      "ports"
DTAG        "tag"
DSTATICPATH "static-path"
DSERVER     "server"
DINCLUDE    "include"

NUMBER      [0-9]+
STRING      \"(\\\\|\\\"|[^\\\"])*\"

%%
{LBRACE}      { return '{'; }
{RBRACE}      { return '}'; }
{LSQUARE}     { return '['; }
{RSQUARE}     { return ']'; }
{WHITESPACES} /* skip */
{SEMICOLON}   { return ';'; }

{DHOSTNAMES}  { return TOKEN_DHOSTNAMES; }
{DPORTS}      { return TOKEN_DPORTS; }
{DTAG}        { return TOKEN_DTAG; }
{DSTATICPATH} { return TOKEN_DSTATICPATH; }
{DSERVER}     { return TOKEN_DSERVER; }

{NUMBER}      { put_number(yyscanner); return TOKEN_NUMBER; }
{STRING}      { put_string(yyscanner); return TOKEN_STRING; }

{DINCLUDE}                              { BEGIN(incl); }
<incl>{WHITESPACES}                     /* skip */
<incl>{STRING}{WHITESPACES}?{SEMICOLON} { int rc = include(yyscanner); if(rc != 0) { return rc; } else { YY_BREAK; } }
<<EOF>>                                 { yypop_buffer_state(yyscanner); int rc = shift_extra_file(yyscanner); if(rc != 0) { return rc; } if(!YY_CURRENT_BUFFER) { yyterminate(); } }

<INITIAL,incl>. { char *buf; asprintf(&buf, "Bad character '%s' at line %d", yyget_text(yyscanner), yyget_lineno(yyscanner)); yyget_lval(yyscanner)->string = buf; return TOKEN_BADCHARACTER; }
%%

void put_number(yyscan_t yyscanner) {
  YYSTYPE *lval = yyget_lval(yyscanner);
  sscanf(yyget_text(yyscanner), "%d", &lval->number);
}

void put_string(yyscan_t yyscanner) {
  YYSTYPE *lval = yyget_lval(yyscanner);
  char *source  = yyget_text(yyscanner);
  /* - 2 quotes + 1 null */
  char *string = (char*)smalloc(yyget_leng(yyscanner) - 2 + 1);
  int i = 0;

  /* It starts with ++source to skip the first double quote */
  while(*(++source) != '\"') {
    if(*source == '\\') {
      source++;
    }
    string[i++] = *source;
  }
  string[i] = '\0';

  string = srealloc(string, i + 1);

  lval->string = string;
}

int include(yyscan_t yyscanner) {
  put_string(yyscanner);
  char *path = yyget_lval(yyscanner)->string;

  extra_t *extra = yyget_extra(yyscanner);

  /* There will always be at least one element in the list */
  extra_file_t *prevfile = (extra_file_t*)extra->files->element;
  char *full_path;
  if(path[0] == '/') {
    if(extra->root_directory == NULL) {
      full_path = strdup(path);
    } else {
      full_path = make_config_path(extra->root_directory, path);
    }
  } else {
    full_path = make_config_path(dirname(prevfile->path), path);
  }

  FILE *f = fopen(full_path, "r");
  if(f == NULL) {
    free(full_path);
    return TOKEN_BADINCLUDE;
  }

  extra_file_t *new_file = make_extra_file(full_path, f);
  cons_extra_file(new_file, extra);

  yypush_buffer_state(yy_create_buffer(f, YY_BUF_SIZE, yyscanner), yyscanner);

  struct yyguts_t *yyg = (struct yyguts_t*)yyscanner;
  BEGIN(INITIAL);
  return 0;
}

int shift_extra_file(yyscan_t yyscanner) {
  extra_t *extra = yyget_extra(yyscanner);

  if(extra->files == HARP_EMPTY_LIST) {
    return TOKEN_BADINCLUDE;
  }

  /* There will always be at least one element in the list */
  extra_file_t *first_extra_file = (extra_file_t*)extra->files->element;
  extra->files = extra->files->next;

  free(first_extra_file->path);

  int rc = fclose(first_extra_file->f);
  if(rc == -1) {
    return TOKEN_BADINCLUDE;
  }

  return 0;
}
