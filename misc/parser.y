
%{
    #include <stdio.h>

    int yylex();
    int yyerror(char *s);

%}

%union {
    int number;
    char* ident;
}

%token <ident> TOKEN_LBRA
%token <ident> TOKEN_RBRA
%token <ident> TOKEN_LPAR
%token <ident> TOKEN_RPAR
%token <ident> TOKEN_PLUS
%token <ident> TOKEN_MINUS
%token <ident> TOKEN_MUL
%token <ident> TOKEN_DIV
%token <ident> TOKEN_SEMI
%token <ident> TOKEN_COMMA
%token <ident> TOKEN_QUOTE

%token <num> TOKEN_NUMBER
%token <num> TOKEN_NUMBERD

%token <ident> TOKEN_REG
%token <ident> TOKEN_CREG

%token <ident> TOKEN_SYMBOL
%token <ident> TOKEN_SYMBOLD
%token <ident> TOKEN_ANNA
%token <ident> TOKEN_ANDA
%token <ident> TOKEN_ANJAR
%token <ident> TOKEN_ANJAO
%token <ident> TOKEN_LD
%token <ident> TOKEN_ST
%token <ident> TOKEN_CSRRD
%token <ident> TOKEN_CSRWR
%token <ident> TOKEN_ANTA

%token <ident> TOKEN_ADVA
%token <ident> TOKEN_WORD
%token <ident> TOKEN_SECTION
%token <ident> TOKEN_SKIP
%token <ident> TOKEN_ASCII
%token <ident> TOKEN_ADDA
%token <ident> TOKEN_ADNA
%token <ident> TOKEN_CLMN


%%

prog
  :
  | instr prog
  ;

instr
  : TOKEN_ANNA
  | TOKEN_ANJAR TOKEN_REG
  | TOKEN_ANJAO operandSkok
  | TOKEN_ANDA TOKEN_REG TOKEN_COMMA TOKEN_REG
  | TOKEN_LD operand TOKEN_COMMA TOKEN_REG
  | TOKEN_ST TOKEN_REG TOKEN_COMMA operand
  | TOKEN_CSRRD TOKEN_CREG TOKEN_COMMA TOKEN_REG
  | TOKEN_CSRWR TOKEN_REG TOKEN_COMMA TOKEN_CREG
  | TOKEN_ANTA TOKEN_REG TOKEN_COMMA TOKEN_REG TOKEN_COMMA operand
  | TOKEN_ADVA multOperands
  | TOKEN_WORD symsOrOperands
  | TOKEN_SECTION TOKEN_SYMBOLD
  | TOKEN_SKIP TOKEN_NUMBERD
  | TOKEN_ASCII  TOKEN_QUOTE strings TOKEN_QUOTE
  | TOKEN_ADDA TOKEN_SYMBOLD TOKEN_COMMA equation
  | TOKEN_ADNA
  | TOKEN_SYMBOLD TOKEN_CLMN
  ;

operand
  : TOKEN_NUMBER
  | TOKEN_SYMBOL
  | TOKEN_NUMBERD
  | TOKEN_SYMBOLD
  | TOKEN_REG
  | TOKEN_LBRA TOKEN_REG TOKEN_RBRA
  | TOKEN_LBRA TOKEN_REG TOKEN_PLUS TOKEN_NUMBERD TOKEN_RBRA
  | TOKEN_LBRA TOKEN_REG TOKEN_PLUS TOKEN_SYMBOLD TOKEN_RBRA
  ;

bracketEquation
  : TOKEN_REG
  | TOKEN_SYMBOLD
  | TOKEN_NUMBERD
  | TOKEN_REG TOKEN_PLUS bracketEquationReg
  | TOKEN_NUMBERD TOKEN_PLUS bracketEquationNumSym
  | TOKEN_SYMBOLD TOKEN_PLUS bracketEquationNumSym
  ;

bracketEquationReg
  : TOKEN_REG
  | TOKEN_NUMBERD
  | TOKEN_SYMBOLD
  | TOKEN_REG TOKEN_PLUS TOKEN_NUMBERD
  | TOKEN_REG TOKEN_PLUS TOKEN_SYMBOLD
  | TOKEN_SYMBOLD TOKEN_PLUS TOKEN_REG
  | TOKEN_NUMBERD TOKEN_PLUS TOKEN_REG
  ;  


bracketEquationNumSym
  : TOKEN_REG
  | TOKEN_REG TOKEN_PLUS TOKEN_REG
  ;  

operandSkok
  : TOKEN_NUMBERD
  | TOKEN_SYMBOLD
  ;

multOperands
  : TOKEN_NUMBERD TOKEN_COMMA multOperands
  | TOKEN_NUMBERD
  | TOKEN_SYMBOLD TOKEN_COMMA multOperands
  | TOKEN_SYMBOLD 
  ;

symsOrOperands
  : TOKEN_NUMBERD TOKEN_COMMA symsOrOperands
  | TOKEN_SYMBOLD TOKEN_COMMA symsOrOperands
  | TOKEN_NUMBERD
  | TOKEN_SYMBOLD
  ;

equation
  : TOKEN_NUMBERD operation equation
  | TOKEN_NUMBERD
  | TOKEN_SYMBOLD operation equation
  | TOKEN_SYMBOLD
  | TOKEN_LPAR equation TOKEN_RPAR
  | TOKEN_LPAR equation TOKEN_RPAR operation equation
  ;

operation
  : TOKEN_PLUS
  | TOKEN_MINUS
  | TOKEN_MUL 
  | TOKEN_DIV
  ;

strings
  : TOKEN_SYMBOLD strings
  | TOKEN_SYMBOLD
  ;
%%

extern int yylineno;

int yyerror(char *s)
{
	printf("Syntax Error on line %d\n", yylineno);
	return -1;
}