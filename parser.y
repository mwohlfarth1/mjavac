%{

    #include <stdio.h>
    #include <string.h>
    #include "interpret.h"
    #include "typecheck.h"
    void yyerror(char *);
    extern int yylex();
    extern int yylineno;
    extern char* yytext;
    extern FILE *yyin;

    struct node* root;
    extern int DEBUG;
    extern int my_argc;
    extern char** my_argv;

%}
%union{
    struct node* node;
    int integer;
    char * string;
}

// Left hand non-terminals
%type <node> Program MainClass VarDecl Expr Type StatementList Assignment ExprPiece VarDeclList VarDeclListPieceList VarDeclListPiece OneDArrayDecl TwoDArrayDecl OneDArrayAssignment TwoDArrayAssignment RegStatement DeclStatement ClassDecl ClassDeclList MethodDeclList MethodDecl FormalList Formal MethodCall ArgList Arg

// Terminal symbols
%token <node> AND BOOLEAN CLASS ']' '}' ')' ','
%token <node> '/' '.' '=' EQUAL_EQUAL '!' EXTENDS FALSE '>'
%token <node> GREATER_EQUAL INT  LENGTH '<' LESS_EQUAL MAIN
%token <node> '-' MINUS_MINUS '*' NEW NOT_EQUAL '[' '{'
%token <node> '(' OR '+' PLUS_PLUS PUBLIC ';'
%token <node> STATIC STRING  SYSTEM_OUT_PRINT SYSTEM_OUT_PRINTLN
%token <node> THIS TRUE VOID IF ELSE WHILE RETURN

%token <integer> INTEGER_LITERAL
%token <string> STRING_LITERAL ID

%left '+' '-'
%left '*' '/'

%start Program

%%

Program:                MainClass 
                        {
                            $$ = new_node(PROGRAM);
                            root = $$;
                            add_child($$, $1);
                        }
                        | MainClass ClassDeclList
                        {
                            $$ = new_node(PROGRAM);
                            root = $$;
                            add_child($$, $1);
                            add_child($$, $2);
                        }

                    ;

MainClass:              CLASS ID '{' PUBLIC STATIC VOID MAIN '(' STRING '[' ']' ID ')' '{' StatementList '}' '}'
                        {
                            $$ = new_node(MAINCLASS);
                            $$->data.value.stringValue = $2; // remember the class name
                            add_child($$, $15); // the statementlist
                            $$->data.argv_name = $12; // remember the argv name
                        }
                        
	| 		CLASS ID '{' PUBLIC STATIC VOID MAIN '(' STRING '[' ']' ID ')' '{' '}' '}'
			{
                            $$ = new_node(MAINCLASS);
                            $$->data.value.stringValue = $2; // remember the class name
                            $$->data.argv_name = $12; // remember the argv name
			}
            
	;

ClassDeclList: ClassDeclList ClassDecl
                    {
                        $$ = new_node(CLASSDECLLIST_CLASSDECLLIST_CLASSDECL);
                        add_child($$, $1);
                        add_child($$, $2);
                    }
            | ClassDecl
                    {
                        $$ = new_node(CLASSDECLLIST_CLASSDECL);
                        add_child($$, $1);
                    }
    ;
                
ClassDecl: CLASS ID '{' '}'
                {
                    $$ = new_node(CLASSDECL);
                    $$->data.value.stringValue = $2; // remember the class name
                    $$->data.extendedClass = NULL;
                }
            | CLASS ID EXTENDS ID '{' '}'
                {
                    $$ = new_node(CLASSDECL);
                    $$->data.value.stringValue = $2; // remember the class name
                    $$->data.extendedClass = $4; // the class it extends
                }
            | CLASS ID '{' StatementList '}'
                {
                    $$ = new_node(CLASSDECL);
                    $$->data.value.stringValue = $2; // remember the class name
                    add_child($$, $4);
                    $$->data.extendedClass = NULL;
                }
            | CLASS ID EXTENDS ID '{' StatementList '}'
                {
                    $$ = new_node(CLASSDECL);
                    $$->data.value.stringValue = $2; // remember the class name
                    $$->data.extendedClass = $4; // the class it extends
                    add_child($$, $6);
                }
            | CLASS ID '{' StatementList MethodDeclList '}'
                {
                    $$ = new_node(CLASSDECL);
                    $$->data.value.stringValue = $2; // remember the class name
                    add_child($$, $4);
                    add_child($$, $5);
                    $$->data.extendedClass = NULL;
                }
            | CLASS ID EXTENDS ID '{' StatementList MethodDeclList '}'
                {
                    $$ = new_node(CLASSDECL);
                    $$->data.value.stringValue = $2; // remember the class name
                    $$->data.extendedClass = $4; // the class it extends
                    add_child($$, $6);
                    add_child($$, $7);
                }
            | CLASS ID '{' MethodDeclList '}'
                {
                    $$ = new_node(CLASSDECL);
                    $$->data.value.stringValue = $2; // remember the class name
                    add_child($$, $4);
                    $$->data.extendedClass = NULL;
                }
            | CLASS ID EXTENDS ID '{' MethodDeclList '}'
                {
                    $$ = new_node(CLASSDECL);
                    $$->data.value.stringValue = $2; // remember the class name
                    $$->data.extendedClass = $4; // the class it extends
                    add_child($$, $6); // the method declaration list
                }
    ;

MethodDeclList: MethodDeclList MethodDecl
                    {
                        $$ = new_node(METHODDECLLIST_METHODDECLLIST_METHODDECL);
                        add_child($$, $1);
                        add_child($$, $2);
                    }
                | MethodDecl
                    {
                        $$ = new_node(METHODDECLLIST_METHODDECL);
                        add_child($$, $1);
                    }
            ;

MethodDecl: Type ID '(' ')' '{' '}'
                {
                    $$ = new_node(METHODDECL);
                    $$->data.value.stringValue = $2; // remember the method name
                    add_child($$, $1); // add the type as a child
                }
            | Type ID '(' FormalList ')' '{' '}'
                {
                    $$ = new_node(METHODDECL);
                    $$->data.value.stringValue = $2; // remember the method name
                    add_child($$, $1); // add the type as a child
                    add_child($$, $4); // add the formallist as a child
                }
            | Type ID '(' ')' '{' StatementList '}'
                {
                    $$ = new_node(METHODDECL);
                    $$->data.value.stringValue = $2; // remember the method name
                    add_child($$, $1); // add the type as a child
                    add_child($$, $6); // add the statementlist as a child
                }
            | Type ID '(' FormalList ')' '{' StatementList '}'
                {
                    $$ = new_node(METHODDECL);
                    $$->data.value.stringValue = $2; // remember the method name
                    add_child($$, $1); // add the type as a child
                    add_child($$, $4); // formalist
                    add_child($$, $7); // statementlist
                }
            | PUBLIC Type ID '(' ')' '{' '}'
                {
                    $$ = new_node(METHODDECL);
                    $$->data.value.stringValue = $3; // remember the method name
                    add_child($$, $2); // add the type as a child
                }
            | PUBLIC Type ID '(' FormalList ')' '{' '}'
                {
                    $$ = new_node(METHODDECL);
                    $$->data.value.stringValue = $3; // remember the method name
                    add_child($$, $2); // add the type as a child
                    add_child($$, $5); // formallist
                }
            | PUBLIC Type ID '(' ')' '{' StatementList '}'
                {
                    $$ = new_node(METHODDECL);
                    $$->data.value.stringValue = $3; // remember the method name
                    add_child($$, $2); // add the type as a child
                    add_child($$, $7); // statementlist
                }
            | PUBLIC Type ID '(' FormalList ')' '{' StatementList '}'
                {
                    $$ = new_node(METHODDECL);
                    $$->data.value.stringValue = $3; // remember the method name
                    add_child($$, $2); // add the type as a child
                    add_child($$, $5); // formallist
                    add_child($$, $8); // statementlist
                }
 
            | Type '[' ']' ID '(' ')' '{' '}'
                {
                    $$ = new_node(METHODDECL_ONED);
                    $$->data.value.stringValue = $4; // remember the method name
                    add_child($$, $1); // add the type as a child
                }
            | Type '[' ']' ID '(' FormalList ')' '{' '}'
                {
                    $$ = new_node(METHODDECL_ONED);
                    $$->data.value.stringValue = $4; // remember the method name
                    add_child($$, $1); // add the type as a child
                    add_child($$, $6); // add the formallist as a child
                }
            | Type '[' ']' ID '(' ')' '{' StatementList '}'
                {
                    $$ = new_node(METHODDECL_ONED);
                    $$->data.value.stringValue = $4; // remember the method name
                    add_child($$, $1); // add the type as a child
                    add_child($$, $8); // add the statementlist as a child
                }
            | Type '[' ']' ID '(' FormalList ')' '{' StatementList '}'
                {
                    $$ = new_node(METHODDECL_ONED);
                    $$->data.value.stringValue = $4; // remember the method name
                    add_child($$, $1); // add the type as a child
                    add_child($$, $6); // formalist
                    add_child($$, $9); // statementlist
                }
            | PUBLIC Type '[' ']' ID '(' ')' '{' '}'
                {
                    $$ = new_node(METHODDECL_ONED);
                    $$->data.value.stringValue = $5; // remember the method name
                    add_child($$, $2); // add the type as a child
                }
            | PUBLIC Type '[' ']' ID '(' FormalList ')' '{' '}'
                {
                    $$ = new_node(METHODDECL_ONED);
                    $$->data.value.stringValue = $5; // remember the method name
                    add_child($$, $2); // add the type as a child
                    add_child($$, $7); // formallist
                }
            | PUBLIC Type '[' ']' ID '(' ')' '{' StatementList '}'
                {
                    $$ = new_node(METHODDECL_ONED);
                    $$->data.value.stringValue = $5; // remember the method name
                    add_child($$, $2); // add the type as a child
                    add_child($$, $9); // statementlist
                }
            | PUBLIC Type '[' ']' ID '(' FormalList ')' '{' StatementList '}'
                {
                    $$ = new_node(METHODDECL_ONED);
                    $$->data.value.stringValue = $5; // remember the method name
                    add_child($$, $2); // add the type as a child
                    add_child($$, $7); // formallist
                    add_child($$, $10); // statementlist
                }           
            | Type '[' ']' '[' ']' ID '(' ')' '{' '}'
                {
                    $$ = new_node(METHODDECL_TWOD);
                    $$->data.value.stringValue = $6; // remember the method name
                    add_child($$, $1); // add the type as a child
                }
            | Type '[' ']' '[' ']' ID '(' FormalList ')' '{' '}'
                {
                    $$ = new_node(METHODDECL_TWOD);
                    $$->data.value.stringValue = $6; // remember the method name
                    add_child($$, $1); // add the type as a child
                    add_child($$, $8); // add the formallist as a child
                }
            | Type '[' ']' '[' ']' ID '(' ')' '{' StatementList '}'
                {
                    $$ = new_node(METHODDECL_TWOD);
                    $$->data.value.stringValue = $6; // remember the method name
                    add_child($$, $1); // add the type as a child
                    add_child($$, $10); // add the statementlist as a child
                }
            | Type '[' ']' '[' ']' ID '(' FormalList ')' '{' StatementList '}'
                {
                    $$ = new_node(METHODDECL_TWOD);
                    $$->data.value.stringValue = $6; // remember the method name
                    add_child($$, $1); // add the type as a child
                    add_child($$, $8); // formalist
                    add_child($$, $11); // statementlist
                }
            | PUBLIC Type '[' ']' '[' ']' ID '(' ')' '{' '}'
                {
                    $$ = new_node(METHODDECL_TWOD);
                    $$->data.value.stringValue = $7; // remember the method name
                    add_child($$, $2); // add the type as a child
                }
            | PUBLIC Type '[' ']' '[' ']' ID '(' FormalList ')' '{' '}'
                {
                    $$ = new_node(METHODDECL_TWOD);
                    $$->data.value.stringValue = $7; // remember the method name
                    add_child($$, $2); // add the type as a child
                    add_child($$, $9); // formallist
                }
            | PUBLIC Type '[' ']' '[' ']' ID '(' ')' '{' StatementList '}'
                {
                    $$ = new_node(METHODDECL_TWOD);
                    $$->data.value.stringValue = $7; // remember the method name
                    add_child($$, $2); // add the type as a child
                    add_child($$, $11); // statementlist
                }
            | PUBLIC Type '[' ']' '[' ']' ID '(' FormalList ')' '{' StatementList '}'
                {
                    $$ = new_node(METHODDECL_TWOD);
                    $$->data.value.stringValue = $7; // remember the method name
                    add_child($$, $2); // add the type as a child
                    add_child($$, $9); // formallist
                    add_child($$, $12); // statementlist
                }           
    ;

MethodCall: ID '(' ')'
                {
                    $$ = new_node(METHODCALL);
                    $$->data.value.stringValue = $1;
                }
            | ID '(' ArgList ')'
                {
                    $$ = new_node(METHODCALL);
                    $$->data.value.stringValue = $1;
                    add_child($$, $3);
                }
            | THIS '.' ID '(' ')'
                {
                    $$ = new_node(METHODCALL);
                    $$->data.value.stringValue = $3;
                    $$->data.className = "this";

                }
            | THIS '.' ID '(' ArgList ')'
                {
                    $$ = new_node(METHODCALL);
                    $$->data.value.stringValue = $3;
                    $$->data.className = "this";
                    add_child($$, $5);
                }
            | ID '.' ID '(' ')'
                {
                    $$ = new_node(METHODCALL);
                    $$->data.value.stringValue = $3;
                    $$->data.className = $1;
                }
            | ID '.' ID '(' ArgList ')'
                {
                    $$ = new_node(METHODCALL);
                    $$->data.value.stringValue = $3;
                    $$->data.className = $1;
                    add_child($$, $5);
                }

           ;

FormalList: FormalList ',' Formal
                {
                    $$ = new_node(FORMALLIST_FORMALLIST_FORMAL);
                    add_child($$, $1);
                    add_child($$, $3);
                }
            | Formal
                {
                    $$ = new_node(FORMALLIST_FORMAL);
                    add_child($$, $1);
                }
    ;

Formal: Type ID
            {
                $$ = new_node(FORMAL);
                $$->data.value.stringValue = $2; // the ID
                add_child($$, $1);
            }
        | Type '[' ']' ID
            {
                $$ = new_node(FORMAL_ONED_ARRAY);
                $$->data.value.stringValue = $4;
                add_child($$, $1);
            }
        | Type '[' ']' '[' ']' ID
            {
                $$ = new_node(FORMAL_TWOD_ARRAY);
                $$->data.value.stringValue = $6;
                add_child($$, $1);
            }
    ;

ArgList: ArgList ',' Arg
                {
                    $$ = new_node(ARGLIST_ARGLIST_ARG);
                    add_child($$, $1);
                    add_child($$, $3);
                }
            | Arg
                {
                    $$ = new_node(ARGLIST_ARG);
                    add_child($$, $1);
                }
        ;

Arg:    Expr
        {
            $$ = new_node(ARG);
            add_child($$, $1);
        }
    ;

StatementList: StatementList RegStatement
                {
                    $$ = new_node(STATEMENTLIST_STATEMENTLIST_STATEMENT);
                    add_child($$, $1);
                    add_child($$, $2);

                    // NOTE: we differentiate between statementlists 
                    // that consist of regular statements and statementlist
                    // that consist of variable declaration statements
                    // because we want to be able to do typechecking
                    // later to make sure that class declarations only
                    // contain variable declaration statements and nothing else
                }
            | StatementList DeclStatement
                {
                    $$ = new_node(STATEMENTLIST_STATEMENTLIST_STATEMENT);
                    add_child($$, $1);
                    add_child($$, $2);
                }
            | RegStatement
                {
                    $$ = new_node(STATEMENTLIST_STATEMENT);
                    add_child($$, $1);
                }
            | DeclStatement
                {
                    $$ = new_node(STATEMENTLIST_STATEMENT);
                    add_child($$, $1);
                }
	;

RegStatement:        
            SYSTEM_OUT_PRINTLN '(' Expr ')' ';'
                        {
                            $$ = new_node(STATEMENT_PRINTLN);
                            add_child($$, $3);
                        }
                        | SYSTEM_OUT_PRINT '(' Expr ')' ';'
                        {
                            $$ = new_node(STATEMENT_PRINT);
                            add_child($$, $3);
                        }
			| Assignment
			{
				$$ = new_node(STATEMENT_ASSIGNMENT);	
				add_child($$, $1);
			}
            | IF '(' Expr ')' '{' StatementList '}' ELSE '{' StatementList '}'
            {
                $$ = new_node(STATEMENT_IF_ELSE);
                add_child($$, $3);
                add_child($$, $6);
                add_child($$, $10);
            }
            | IF '(' Expr ')' '{' StatementList '}' ELSE '{' '}'
                {
                    $$ = new_node(STATEMENT_IF_NO_ELSE);
                    add_child($$, $3);
                    add_child($$, $6);
                }
            | IF '(' Expr ')' StatementList ELSE '{' StatementList '}'
                {
                    $$ = new_node(STATEMENT_IF_ELSE);
                    add_child($$, $3);
                    add_child($$, $5);
                    add_child($$, $8);
                }
            | IF '(' Expr ')' StatementList ELSE '{' '}'
                {
                    $$ = new_node(STATEMENT_IF_NO_ELSE);
                    add_child($$, $3);
                    add_child($$, $5);
                }
            | WHILE '(' Expr ')' '{' StatementList '}'
                {
                    $$ = new_node(STATEMENT_WHILE);
                    add_child($$, $3);
                    add_child($$, $6);
                }
            | OneDArrayAssignment
                {
                    $$ = new_node(STATEMENT_ONED_ARRAY_ASSIGNMENT);
                    add_child($$, $1);
                }
            | TwoDArrayAssignment
                {
                    $$ = new_node(STATEMENT_TWOD_ARRAY_ASSIGNMENT);
                    add_child($$, $1);
                }
            | RETURN Expr ';'
                {
                    $$ = new_node(STATEMENT_RETURN);
                    add_child($$, $2); // the expression we want to evaluate and return
                }
            | RETURN ';'
                {
                    $$ = new_node(STATEMENT_RETURN);
                }
            | MethodCall ';'
                {
                    $$ = new_node(STATEMENT_METHODCALL);
                    add_child($$, $1);
                }
	;

DeclStatement: VarDecl
                {
                    $$ = new_node(STATEMENT_VARDECL);
                    add_child($$, $1);
                }
            | VarDeclList 
                {
                    $$ = new_node(STATEMENT_VARDECL_LIST);
                    add_child($$, $1);
                }
            | OneDArrayDecl
                {
                    $$ = new_node(STATEMENT_ONED_ARRAYDECL);
                    add_child($$, $1);
                }
            | TwoDArrayDecl
                {
                    $$ = new_node(STATEMENT_TWOD_ARRAYDECL);
                    add_child($$, $1);
                }
    ;

VarDecl:                
    Type ID '=' Expr ';'
        {
            $$ = new_node(VARDECL);
            $$->data.value.stringValue = $2;

            add_child($$, $1);
            add_child($$, $4);
        }
    | Type ID ';'
        {
            $$ = new_node(VARDECL_NO_RHS);
            $$->data.value.stringValue = $2;

            add_child($$, $1);
        }
    | Type ID '=' NEW ID '(' ')' ';'
        {
            $$ = new_node(VARDECL_NEW_RHS);
            $$->data.value.stringValue = $2;

            add_child($$, $1); // lhs type
            $$->data.className = $5;
        }
    ;

VarDeclList: 
    Type VarDeclListPieceList ';'
    {
        $$ = new_node(VARDECL_LIST);
        add_child($$, $1);
        add_child($$, $2);
    }
    ;

VarDeclListPieceList: VarDeclListPieceList ',' VarDeclListPiece
                        {
                            $$ = new_node(VARDECLLISTPIECELIST_VARDECLLISTPIECE);
                            add_child($$, $1);
                            add_child($$, $3);
                        }
    | VarDeclListPiece
        {
            $$ = new_node(VARDECLLISTPIECE);
            add_child($$, $1);
        }
    ;

VarDeclListPiece:
    ID '=' Expr
        {
            $$ = new_node(VARDECLLISTPIECE_ID_EQ_EXPR);
            $$->data.value.stringValue = $1;
            add_child($$, $3);
        }
    | ID
        {
            $$ = new_node(VARDECLLISTPIECE_ID);
            $$->data.value.stringValue = $1;
        }
    | ID '=' NEW ID '(' ')'
        {
            $$ = new_node(VARDECLLISTPIECE_ID_EQ_NEW);
            $$->data.value.stringValue = $1;
            $$->data.className = $4; 
        }
    ;
    
OneDArrayDecl:
            Type '[' ']' ID '=' NEW Type '[' Expr ']' ';'
                {
                    $$ = new_node(ONED_ARRAYDECL);
                    // put the name of the array in the parent
                    $$->data.value.stringValue = $4;
                    add_child($$, $1); // add the lhs type
                    add_child($$, $7); // add the rhs type
                    add_child($$, $9); // add the expression
                }
            | Type '[' ']' ID '=' Expr ';'
                {
                    // this is a line of the form:
                    // int[] a = 4;
                    // this isn't allowed. however, it's allowed in the grammer
                    // so we allow it and check the number of children when we typecheck
                    $$ = new_node(ONED_ARRAYDECL);
                    $$->data.value.stringValue = $4;
                    add_child($$, $1); // add the lhs type
                    add_child($$, $6); // add the expression on the rhs even though we don't care
                }
            | Type '[' ']' ID ';'
                {
                    // here, we're just making a new array and not initializing it.
                    // we should add it to the symbol table in the even that we
                    // end up assigning to it later
                    $$ = new_node(ONED_ARRAYDECL);
                    $$->data.value.stringValue = $4; // remember the array name
                    add_child($$, $1); // remember the type
                }
            ;

TwoDArrayDecl: Type '[' ']' '[' ']' ID '=' NEW Type '[' Expr ']' '[' Expr ']' ';'
                {
                    $$ = new_node(TWOD_ARRAYDECL);
                    // put the name of the array in the parent
                    $$->data.value.stringValue = $6;

                    add_child($$, $1); // add the lhs type
                    add_child($$, $9); // add the rhs type
                    add_child($$, $11); // add the first expression
                    add_child($$, $14); // add the second expression
                }
            | Type '[' ']' '[' ']' ID '=' Expr ';'
                {
                    // this is a line in the form:
                    // int[][] a = 5;
                    // this isn't allowed, but the grammar allows it, so let's
                    // allow it and just check for the number of children in the typechecking
                    $$ = new_node(TWOD_ARRAYDECL);

                    // put the name of the array in the parent
                    $$->data.value.stringValue = $6;

                    add_child($$, $1); // the lhs type
                    add_child($$, $8); // the expression
                }
            | Type '[' ']' '[' ']' ID '=' NEW Type '[' Expr ']' ';'
                {
                    // here, we're trying to assign a oned array to a 2d array.
                    // you can't do that. the grammar allows it though so we'll
                    // just typecheck it later
                    $$ = new_node(TWOD_ARRAYDECL);
                    $$->data.value.stringValue = $6; // remember the array name
                    add_child($$, $1); // lhs type
                    add_child($$, $9); // the rhs type
                    add_child($$, $11); // the expression for the index
                }
            | Type '[' ']' '[' ']' ID ';'
                {
                    // here, we're creating a new array without assigning to it.
                    // add it to the symbol table so that we can type check it later
                    $$ = new_node(TWOD_ARRAYDECL);
                    $$->data.value.stringValue = $6; // remember the array name
                    add_child($$, $1); // add the type as a child
                }
            ;

Type:                   INT
                        {
                            $$ = new_node(INT_TYPE);
                            $$ -> data.type = "int";
                        }
                        | BOOLEAN
                        {
                            $$ = new_node(BOOLEAN_TYPE);
                            $$ -> data.type = "boolean";
                        }
                        | STRING
                        {
                            $$ = new_node(STRING_TYPE);
                            $$ -> data.type = "string";
                        }
                        | ID
                        {
                            $$ = new_node(ID_TYPE);
                            $$->data.type = $1;
                            $$->data.value.stringValue = $1; // in case someone wants to access 
                                                             // it this way instead
                        }
	;
                    
Assignment: ID '=' Expr ';'
            {
                $$ = new_node(ASSIGNMENT);
                $$->data.value.stringValue = $1;
                add_child($$, $3);
            }
        | ID '=' NEW Type '[' Expr ']' ';'
            {
                // this is when we assign a new array to an
                // array that we've already declared. we just need to make sure
                // that the rhs has the same type that the previously declared lhs
                // does and that the expression on the rhs is valid and results in a
                // number. we also need to make sure the array on the lhs has the same
                // dimension as the array on the rhs
                $$ = new_node(ASSIGNMENT);
                $$->data.value.stringValue = $1; // remember the variable name
                add_child($$, $4); // remember the rhs type
                add_child($$, $6); // the expression
            }
        | ID '=' NEW Type '[' Expr ']' '[' Expr ']' ';'
            {
                // this is same as above, but we're assigning to a 2d array.
                $$ = new_node(ASSIGNMENT);
                $$->data.value.stringValue = $1; // remember the name
                add_child($$, $4); // the rhs type
                add_child($$, $6); // the first expression
                add_child($$, $9); // the second expression
            }
        | ID '.' ID '=' Expr ';'
            {
                // assigning a value to a class variable
                $$ = new_node(ASSIGNMENT);
                $$->data.value.stringValue = $3; // the name of the class variable
                $$->data.className = $1; // the name of the class to get the variable from
                add_child($$, $5); // the expression on the rhs
            }
        | THIS '.' ID '=' Expr ';'
            {
                // assigning to this class variable
                $$ = new_node(ASSIGNMENT);
                $$->data.value.stringValue = $3; // the variable name
                $$->data.className = "this"; // THIS
                add_child($$, $5); // the expression on the rhs
            }
        | ID '=' NEW ID '(' ')' ';'
            {
                $$ = new_node(ASSIGNMENT);
                $$->data.value.stringValue = $1;
                $$->data.className = $4; // the type on the rhs
            }

    ;

OneDArrayAssignment: ID '[' Expr ']' '=' Expr ';'
                    {
                        $$ = new_node(ONED_ARRAY_ASSIGNMENT);
                        $$->data.value.stringValue = $1;
                        add_child($$, $3);
                        add_child($$, $6);
                    }
                | ID '[' Expr ']' '=' NEW Type '[' Expr ']' ';'
                    {
                        // this is when you take the first layer of a 2d array
                        // and reassign a new 1d array to that layer.
                        // it's allowed. we just need to make sure the types are good
                        $$ = new_node(ONED_ARRAY_ASSIGNMENT);
                        $$->data.value.stringValue = $1; // remember the array name
                        add_child($$, $3); // the lhs index expression
                        add_child($$, $7); // the rhs type
                        add_child($$, $9); // the expression that's the size on the rhs
                    }
                ;

TwoDArrayAssignment: ID '[' Expr ']' '[' Expr ']' '=' Expr ';'
                    {
                        $$ = new_node(TWOD_ARRAY_ASSIGNMENT);
                        $$->data.value.stringValue = $1;
                        add_child($$, $3);
                        add_child($$, $6);
                        add_child($$, $9);
                    }
                | ID '[' Expr ']' '[' Expr ']' '=' NEW Type '[' Expr ']' '[' Expr ']' ';'
                    {
                        // this is when we redeclare an array with a different type.
                        // this is allowed unfortunately
                        $$ = new_node(TWOD_ARRAY_ASSIGNMENT);
                        $$->data.value.stringValue = $1; // remember the name of the array
                        add_child($$, $3); // the first index on the lhs
                        add_child($$, $6); // the second index on the lhs
                        add_child($$, $10); // the rhs type
                        add_child($$, $12); // the rhs first index
                        add_child($$, $15); // the rhs second index
                    }
                | ID '[' Expr ']' '[' Expr ']' '=' NEW Type '[' Expr ']' ';'
                    {
                        // this is when we try to assign an array to a spot that
                        // isn't an array
                        $$ = new_node(TWOD_ARRAY_ASSIGNMENT);
                        $$->data.value.stringValue = $1; // remember the array name
                        add_child($$, $3); // the first lhs index
                        add_child($$, $6); // the second lhs index
                        add_child($$, $10); // the rhs type
                        add_child($$, $12); // the rhs index
                    }
                ;

Expr: Expr '*' Expr
		{
			$$ = new_node(EXPRESSION_MULTIPLY);
			add_child($$, $1);
			add_child($$, $3);
		}
	| Expr '/' Expr
		{
			$$ = new_node(EXPRESSION_DIVIDE);
			add_child($$, $1);
			add_child($$, $3);
		}
	| Expr '+' Expr
		{
			$$ = new_node(EXPRESSION_ADD);
			add_child($$, $1);
			add_child($$, $3);
		}
	| Expr '-' Expr
		{
			$$ = new_node(EXPRESSION_SUBTRACT);
			add_child($$, $1);
			add_child($$, $3);
		}
    | '(' Expr ')'
        {
            $$ = new_node(EXPRESSION_PARENTHESES);
            add_child($$, $2);
        }
	| ExprPiece
		{
			$$ = new_node(EXPRESSION_PIECE);
			add_child($$, $1);
		}
    | '+'ExprPiece
        {
            // pretend that this is just an expression piece and forget about the '+'
            $$ = new_node(EXPRESSION_PIECE);
            add_child($$, $2);
            
        }
    | '-'ExprPiece
        {
            $$ = new_node(NEG_EXPRESSION_PIECE);
            add_child($$, $2);
        }
    | '!'Expr
        {
            $$ = new_node(EXPRESSION_NEGATED);
            add_child($$, $2);
        }
    | Expr EQUAL_EQUAL Expr
        {
            $$ = new_node(EXPRESSION_EQUAL_EQUAL);
            add_child($$, $1);
            add_child($$, $3);
        }
    | Expr NOT_EQUAL Expr
        {
            $$ = new_node(EXPRESSION_NOT_EQUAL);
            add_child($$, $1);
            add_child($$, $3);
        }
    | Expr GREATER_EQUAL Expr
        {
            $$ = new_node(EXPRESSION_GREATER_EQUAL);
            add_child($$, $1);
            add_child($$, $3);
        }
    | Expr LESS_EQUAL Expr
        {
            $$ = new_node(EXPRESSION_LESS_EQUAL);
            add_child($$, $1);
            add_child($$, $3);
        }
    | Expr '>' Expr
        {
            $$ = new_node(EXPRESSION_GREATER);
            add_child($$, $1);
            add_child($$, $3);
        }
    | Expr '<' Expr
        {
            $$ = new_node(EXPRESSION_LESS);
            add_child($$, $1);
            add_child($$, $3);
        }
    | Expr AND Expr
        {
            $$ = new_node(EXPRESSION_AND);
            add_child($$, $1);
            add_child($$, $3);
        }
    | Expr OR Expr
        {
            $$ = new_node(EXPRESSION_OR);
            add_child($$, $1);
            add_child($$, $3);
        }
    | ID '.' LENGTH
        {
            $$ = new_node(EXPRESSION_PIECE_DOT_LENGTH);
            $$->data.value.stringValue = $1;
        }
    | ExprPiece '.' LENGTH
        {
            $$ = new_node(EXPRESSION_PIECE_DOT_LENGTH);
            add_child($$, $1);
        }
	;

ExprPiece:                    INTEGER_LITERAL 
                        {
                            $$ = new_node(INTEGER_LIT);
                            setIntValue($$, $1);
                        }
                        | STRING_LITERAL
                        {
                            $$ = new_node(STRING_LIT);
                            setStringValue($$, $1);
                        }
                        | TRUE
                        {
                           $$ = new_node(BOOLEAN_LIT);
                           setBooleanValue($$, true);
                        }
                        | FALSE
                        {
                            $$ = new_node(BOOLEAN_LIT);
                            setBooleanValue($$, false);
                        }
			| ID
			{
				$$ = new_node(ID_);
                $$->data.value.stringValue = $1;
			}
            | ID '[' Expr ']'
                {
                    $$ = new_node(ONED_ARRAY_REFERENCE);
                    $$->data.value.stringValue = $1;
                    add_child($$, $3);
                }
            | ID '[' Expr ']' '[' Expr ']'
                {
                    $$ = new_node(TWOD_ARRAY_REFERENCE);
                    $$->data.value.stringValue = $1;
                    add_child($$, $3);
                    add_child($$, $6);
                }
            | ID '.' ID
                {
                    // this is retrieving a class variable
                    $$ = new_node(CLASS_VARIABLE);
                    $$->data.className = $1;
                    $$->data.value.stringValue = $3; // the name of the class variable
                }
            | THIS '.' ID
                {
                    // this is retrieving a class variable
                    $$ = new_node(CLASS_VARIABLE);
                    $$->data.className = "this";
                    $$->data.value.stringValue = $3; // the variable name
                }
            | MethodCall
                {
                    // this is a method call that's part of an expression
                    $$ = new_node(METHODCALL);
                    $$->data.value.stringValue = NULL;
                    add_child($$, $1);
                }
            | NEW ID '(' ')' '.' MethodCall
                {
                    $$ = new_node(METHODCALL);
                    $$->data.value.stringValue = $2;
                    add_child($$, $6);
                }
	;

%%

void yyerror(char* s) {
    fprintf(stderr, "%s on %d\n", s, yylineno);
}

int main(int argc, char* argv[] )
{

    yyin = fopen( argv[1], "r" );

    my_argc = argc;
    my_argv = argv;

    // if there was a "-d" flag after the filename then set the DEBUG flag.
    // this is used elsewhere in the program to only print debug things when
    // we're in debug mode (and not when the program is run by the testing script)
    if ((argc == 3) && 
        (strcmp(argv[2], "-d") == 0)) {
        DEBUG = 1;
    }
    else {
        DEBUG = 0;
    }

    // Checks for syntax errors and constructs AST
    if (yyparse() != 0)
        return 1;

    // allocate memory for a global symbol table
    struct symbol_table* global = malloc(sizeof(struct symbol_table));
    global->parent = NULL;
    global->num_children = 0;
    global->table_name = "Global";

    // Traverse the AST to build the symbol table. this function
    // will report most use w/o declaration and use before declaration
    build_symbol_table(root, global);

    // now check for type errors like incorrect assignments or different
    // types for an operation
    check_types(root); 

    // if DEBUG flag has been set, print the AST so that we can work out
    // why we're failing the test case
    if (DEBUG) printAST(root, 0, 0);
    if (DEBUG) print_symbol_tables(global);

    // Traverse the AST again to interpret the program if no semantic errors
    if(numErrors == 0){
        interpret_program(root, global, argv[1]);
    }

    if (DEBUG) printAST(root, 0, 0);
    if (DEBUG) print_symbol_tables(global);

    return 0;
}
