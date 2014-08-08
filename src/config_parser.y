%code requires {
#include <stdio.h>

#include <memory.h>
#include <list.h>
#include <primp_config.h>
}

%{
#include <stdio.h>

#include <list.h>
#include <primp_config.h>

void yyerror(void *yyscanner, list_t **configs_ptr, char *errmsg) {
  fprintf(stderr, "error: %s\n", errmsg);
}

int yylex(void *yylval, void *yyscanner);

void print_hostnames(list_t *);
%}

%define api.pure
%param { void *yyscanner }
%parse-param { list_t **configs_ptr }

%union {
  int number;
  char *string;
  filter_t *filter;
  resolver_t *resolver;
  list_t *list;
  config_t *config;
  choice_t *choice;
}

%type  <list>     start

%type  <config>   config
%type  <config>   config_items

%type  <filter>   filter
%type  <resolver> resolver

%type  <list>     hostnames
%type  <list>     ports

%type  <list>     choice_groups
%type  <list>     choice_group

%type  <list>     choices
%type  <choice>   choice

%token <filter>   TOKEN_DHOSTNAMES
%token <filter>   TOKEN_DPORTS
%token <resolver> TOKEN_DSTATICPATH
%token <resolver> TOKEN_DSERVER

%token <number>   TOKEN_NUMBER
%token <string>   TOKEN_STRING

%%

start: config start { list_t *configs = cons($1, $2); *configs_ptr = configs; $$ = configs; }
     | %empty       { $$ = EMPTY_LIST; }
     ;

config: '{' config_items choice_groups '}' { $2->choice_groups = $3; $$ = $2; };

config_items: filter   config_items { cons_filter($1, $2); $$ = $2; }
            | resolver config_items { cons_resolver($1, $2); $$ = $2; }
            | %empty                { $$ = make_empty_config(); }
            ;

filter: TOKEN_DHOSTNAMES  hostnames ';' { $$ = make_hostnames_filter($2); }
      | TOKEN_DPORTS      ports     ';' { $$ = make_ports_filter($2); }
      ;

resolver: TOKEN_DSTATICPATH TOKEN_STRING              ';' { $$ = make_static_path_resolver($2); }
        | TOKEN_DSERVER     TOKEN_STRING TOKEN_NUMBER ';' { $$ = make_server_resolver($2, $3); }
        ;

hostnames: TOKEN_STRING hostnames { $$ = cons($1, $2); }
         | %empty                 { $$ = NULL; }
         ;

ports: TOKEN_NUMBER ports { int *port = (int*)smalloc(sizeof(int)); *port = $1; $$ = cons(port, $2); }
     | %empty             { $$ = EMPTY_LIST; }
     ;

choice_groups: choice_group choice_groups { $$ = cons($1, $2); }
             | %empty                     { $$ = EMPTY_LIST; }
             ;

choice_group: '[' choices ']' { $$ = $2; };

choices: choice choices { $$ = cons($1, $2); }
       | %empty         { $$ = EMPTY_LIST; }
       ;

choice: TOKEN_NUMBER config { $$ = make_choice($1, $2); };

%%
