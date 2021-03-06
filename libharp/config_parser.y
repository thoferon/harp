%code requires {
#include <stdio.h>

#include <smem.h>
#include <harp.h>
}

%{
#include <stdio.h>

#include <harp.h>

void yyerror(void *yyscanner, harp_list_t **configs_ptr, char *errmsg);
int yylex(void *yylval, void *yyscanner);
%}

%define api.pure
%param { void *yyscanner }
%parse-param { harp_list_t **configs_ptr }

%union {
  int number;
  char *string;
  harp_filter_t *filter;
  harp_resolver_t *resolver;
  harp_list_t *list;
  harp_config_t *config;
  harp_choice_t *choice;
}

%type  <list>     start

%type  <config>   config
%type  <config>   config_items

%type  <filter>   filter
%type  <string>   tag
%type  <resolver> resolver

%type  <list>     hostnames
%type  <list>     ports

%type  <list>     choice_group

%type  <list>     choices
%type  <choice>   choice

%token <filter>   TOKEN_DHOSTNAMES
%token <filter>   TOKEN_DPORTS
%token <string>   TOKEN_DTAG
%token <resolver> TOKEN_DSTATICPATH
%token <resolver> TOKEN_DSERVER

%token <number>   TOKEN_NUMBER
%token <string>   TOKEN_STRING

%token <string>   TOKEN_BADINCLUDE
%token <string>   TOKEN_BADCHARACTER

%%

start: config start  { harp_list_t *configs = harp_cons($1, $2); *configs_ptr = configs; $$ = configs; }
     | %empty        { $$ = HARP_EMPTY_LIST; }
     ;

config: '{' config_items '}' { $$ = $2; };

config_items: filter       config_items { harp_cons_filter($1, $2); $$ = $2; }
            | tag          config_items { harp_cons_tag($1, $2); $$ = $2; }
            | resolver     config_items { harp_cons_resolver($1, $2); $$ = $2; }
            | choice_group config_items { harp_cons_choice_group($1, $2); $$ = $2; }
            | config       config_items { harp_cons_subconfig($1, $2); $$ = $2; }
            | %empty                    { $$ = harp_make_empty_config(); }
            ;

filter: TOKEN_DHOSTNAMES  hostnames ';' { $$ = harp_make_hostnames_filter($2); }
      | TOKEN_DPORTS      ports     ';' { $$ = harp_make_ports_filter($2); }
      ;

tag: TOKEN_DTAG TOKEN_STRING ';' { $$ = $2; };

resolver: TOKEN_DSTATICPATH TOKEN_STRING              ';' { $$ = harp_make_static_path_resolver($2); }
        | TOKEN_DSERVER     TOKEN_STRING TOKEN_NUMBER ';' { $$ = harp_make_server_resolver($2, $3); }
        ;

hostnames: TOKEN_STRING hostnames { $$ = harp_cons($1, $2); }
         | %empty                 { $$ = HARP_EMPTY_LIST; }
         ;

ports: TOKEN_NUMBER ports { int *port = (int*)smalloc(sizeof(int)); *port = $1; $$ = harp_cons(port, $2); }
     | %empty             { $$ = HARP_EMPTY_LIST; }
     ;

choice_group: '[' choices ']' { $$ = $2; };

choices: choice choices { $$ = harp_cons($1, $2); }
       | %empty         { $$ = HARP_EMPTY_LIST; }
       ;

choice: TOKEN_NUMBER config { $$ = harp_make_choice($1, $2); };

%%

// FIXME: a bit hacky
union YYSTYPE* yyget_lval(void *);

void yyerror(void *yyscanner, harp_list_t **configs_ptr, char *errmsg) {
  harp_errno = HARP_ERROR_PARSE_ERROR;
  snprintf(harp_errstr, 1024, "Parse error: %s (%s)",
           errmsg, yyget_lval(yyscanner)->string);
}
