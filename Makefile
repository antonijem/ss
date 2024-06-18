g++ assembler.cpp lex.yy.c parser.tab.c -o assembler

flex lexer.l
bison -d parser.y
