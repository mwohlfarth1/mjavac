mjavac:		y.tab.c lex.yy.o node.c typecheck.c interpret.c
			gcc y.tab.c lex.yy.o  node.c typecheck.c interpret.c -g -o mjavac -lfl -lm

y.tab.c:	parser.y
			bison -v -y -d -g -t --verbose parser.y

lex.yy.c:	parser.l
			lex -l parser.l

lex.yy.o:   lex.yy.c
			gcc -c lex.yy.c

lexParser: 	lex.yy.o
			gcc lex.yy.o -o lexParser

clean:
	rm -f lex.yy.c y.tab.c y.tab.h y.dot y.output mjavac lex.yy.o
