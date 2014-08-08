%{
#include <stdlib.h>
#include <stdio.h>

#include <memory.h>
#include <config_parser.h>

void put_number(yyscan_t);
void put_string(yyscan_t);
%}

%option 8bit reentrant bison-bridge
%option warn nodefault nounput noyywrap
%option header-file="config_lexer.h"

%pointer

LBRACE      "{"
RBRACE      "}"
LSQUARE     "["
RSQUARE     "]"
WHITESPACES [ \t\r\n]+
SEMICOLON   ";"

DHOSTNAMES  "hostnames"
DPORTS      "ports"
DSTATICPATH "static-path"
DSERVER     "server"

NUMBER      [0-9]+
STRING      \"(\\\"|[^\"])*\"

%%
{LBRACE}      { return '{'; }
{RBRACE}      { return '}'; }
{LSQUARE}     { return '['; }
{RSQUARE}     { return ']'; }
{WHITESPACES} /* skip */
{SEMICOLON}   { return ';'; }

{DHOSTNAMES}  { return TOKEN_DHOSTNAMES; }
{DPORTS}      { return TOKEN_DPORTS; }
{DSTATICPATH} { return TOKEN_DSTATICPATH; }
{DSERVER}     { return TOKEN_DSERVER; }

{NUMBER}      { put_number(yyscanner); return TOKEN_NUMBER; }
{STRING}      { put_string(yyscanner); return TOKEN_STRING; }

.             { printf("Bad character '%s' at line %d\n", yyget_text(yyscanner), yyget_lineno(yyscanner)); return *yyget_text(yyscanner); }
%%

void put_number(yyscan_t yyscanner) {
  YYSTYPE *lval = yyget_lval(yyscanner);
  sscanf(yyget_text(yyscanner), "%d", &lval->number);
}

void put_string(yyscan_t yyscanner) {
  YYSTYPE *lval = yyget_lval(yyscanner);
  char *source  = yyget_text(yyscanner);
  char *string  = (char*)smalloc(yyget_leng(yyscanner) - 2 + 1); // - 2 quotes + 1 null
  int i;

  // Skip the first double quote
  source++;

  for(i = 0; *source != '\0'; i++, source++) {
    if(*source == '\\') {
      source++;
      if(*source == '\0') {
        break;
      }
    }
    string[i] = *source;
  }

  // --i to remove the last double quote
  string[--i] = '\0';

  string = srealloc(string, i + 1);

  lval->string = string;
}
