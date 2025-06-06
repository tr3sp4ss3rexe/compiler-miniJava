compiler: lex.yy.c parser.tab.o main.cc
		g++ -g -w -ocompiler parser.tab.o lex.yy.c main.cc -std=c++14
parser.tab.o: parser.tab.cc
		g++ -g -w -c parser.tab.cc -std=c++14
parser.tab.cc: parser.yy
		bison parser.yy
lex.yy.c: lexer.flex parser.tab.cc
		flex lexer.flex
tree: 
		dot -Tpdf tree.dot -otree.pdf
ir:
		dot -Tpdf ir.dot -o ir.pdf
interpreter:
		g++ -g -w -o interpreter interpreter.cc -std=c++14
clean:
		rm -f parser.tab.* lex.yy.c* compiler stack.hh position.hh location.hh *.dot *.pdf output.class
		rm -R compiler.dSYM
cleanInterpreter:
		rm -rf interpreter