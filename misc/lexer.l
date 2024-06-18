%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"
%}

%option noyywrap

LBRA  "["
RBRA  "]"
LPAR  "("
RPAR  ")"
PLUS  "+"
MINUS "-"
MUL   "*"
DIV   "/"
SEMI  ";"
CLMN ":" 
COMMA ","
QUOTE "\""

HEX   	0[Xx][0-9A-Fa-f]+

NUMBER  "%"([0-9])+
NUMBERD [0-9]+
SYMBOL	"$"([a-zA-Z0-9_])+
SYMBOLD	([a-zA-Z][a-zA-Z0-9_]*)

ANNA    ("halt"|"int"|"iret"|"ret")
ANJAR	  ("push "|"pop "|"not ")
ANJAO   ("call "|"jmp ")
ANDA   	("xchg "|"add "|"sub "|"mul "|"div "|"and "|"or "|"xor "|"shl "|"shr ")
LD      "ld "
ST      "st "
CSRRD 	"csrrd "
CSRWR	  "csrwr "
ANTA	  ("beq"|"bne"|"bgt") 

ADVA    (".global"|".extern")
WORD 	  ".word "
SKIP 	  ".skip "
ASCII 	".ascii "
ADDA	  ".equ "
ADNA	  ".end"


REG		"%"("r"[0-9]|"r"1[0-5]|"sp"|"pc")
CREG	"%"("status"|"handler"|"cause")

SECTION	".section "

COM   "#"[^\n\r]*
WS    [ \r\n\t]*

%%

\n          {yylineno++;}
{COM}       { /* also skip comments */ }
{WS}        { /* skip whitespace */ }
{HEX}		    { 
              //fprintf(stderr, " HEX %d\n", yylineno);
	            sscanf(yytext, "%x", &yylval.number);
			        return TOKEN_NUMBERD;
            }

{NUMBER}    {/*fprintf(stderr, " NUMBER %d\n", yylineno);*/ yylval.number = atoi(yytext + 1); return TOKEN_NUMBER;}
{NUMBERD}   {/*fprintf(stderr, " NUMBERD %d\n", yylineno);*/ yylval.number = atoi(yytext); return TOKEN_NUMBERD;}
{REG}       {/*fprintf(stderr, " REG %d\n", yylineno);*/ yylval.ident = strdup(yytext); return TOKEN_REG;}
{CREG}      {/*fprintf(stderr, " CREG %d\n", yylineno);*/ yylval.ident = strdup(yytext); return TOKEN_CREG;}
{SYMBOL}    {
             yylval.ident = strdup(yytext + 1); 
             //fprintf(stderr, " SYMBOL %d\n", yylineno);
             return TOKEN_SYMBOL;
            }
            
{ANNA}     {  
              //fprintf(stderr, " ANNA %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_ANNA;
            }
{ANJAR}     {
              //fprintf(stderr, " ANJAR %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_ANJAR;
            }
{ANJAO}     {
              //fprintf(stderr, " ANJAO %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_ANJAO;
            }
{ANDA}     {
	            //fprintf(stderr, " ANDA %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_ANDA;
            }
{LD}       {
              //fprintf(stderr, " LD %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_LD;
            }  
{ST}        {
              //fprintf(stderr, " ST %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_ST;
            }
{CSRRD}     { 
              //fprintf(stderr, " CSRRD %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_CSRRD;
            }
{CSRWR}     {
              //fprintf(stderr, " CSRWR %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_CSRWR;
            }
{ANTA}     {  
              //fprintf(stderr, " ANTA %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_ANTA;
            }
{ADVA}     {
              //fprintf(stderr, " ADVA %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_ADVA;
            }
{WORD}     {
              //fprintf(stderr, " WORD %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_WORD;
            }
{SKIP}     {
              //fprintf(stderr, " SKIP %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_SKIP;
            }
{ASCII}     {
              //fprintf(stderr, " ASCII %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_ASCII;
            }
{ADDA}     {
              //fprintf(stderr, " ADDA %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_ADDA;
            }
{ADNA}     {
              //fprintf(stderr, " ADNA %d\n", yylineno);
	            yylval.ident = strdup(yytext);
			        return TOKEN_ADNA;
            }          
{SYMBOLD}   {
              if (yytext[0] != '.' && yytext[0] != '$') {
                //fprintf(stderr, " SYMBOLD %d\n", yylineno);
                yylval.ident = strdup(yytext);
                return TOKEN_SYMBOLD;
              } else {
                fprintf(stderr, "Lexer error: Invalid token starting with '.'\n");
                return 0; // Return an error code
              }
            }
{SECTION}   {
              if (strlen(yytext) == 9) {
                //fprintf(stderr, " SECTION %d\n", yylineno);
                yylval.ident = strdup(yytext);
                return TOKEN_SECTION;
              } else if (strlen(yytext) > 8) {
                fprintf(stderr, "Lexer error: Invalid token '%s'\n", yytext);
                return 0; // Return an error code
              }
              // Ignore other tokens starting with "."
            }

{LBRA}      { return TOKEN_LBRA; }
{RBRA}      { return TOKEN_RBRA; }
{LPAR}      { return TOKEN_LPAR; }
{RPAR}      { return TOKEN_RPAR; }
{PLUS}      { return TOKEN_PLUS; }
{MINUS}     { return TOKEN_MINUS;}
{MUL}       { return TOKEN_MUL;}
{DIV}       { return TOKEN_DIV;}
{SEMI}      { return TOKEN_SEMI; }
{CLMN}      { /*fprintf(stderr, " CLMN %d\n", yylineno);*/return TOKEN_CLMN; }
{COMMA}     { /*fprintf(stderr, " COMMA %d\n", yylineno);*/ return TOKEN_COMMA; }
{QUOTE}     { /*fprintf(stderr, " QUOTE %d\n", yylineno);*/ return TOKEN_QUOTE; }
.           {
              fprintf(stderr, "Lexer error: Undefined token '%s' at line %d\n", yytext, yylineno);
              return 0; // Return an error code
            }

%%

extern int yylineno;

int yyerror(const char* msg)
{
    fprintf(stderr, "Parser error at line %d: %s\n", yylineno, msg);
    return -1;
}