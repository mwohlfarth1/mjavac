%{
    #include "y.tab.h"
    #include <stdio.h>
    #include <stdlib.h>
    extern int yylex();
    void    yyerror(char *);

%}

%option yylineno

%%

boolean                                             { return BOOLEAN;               }
class                                               { return CLASS;                 }
extends                                             { return EXTENDS;               }
false                                               { return FALSE;                 }
int                                                 { return INT;                   }
length                                              { return LENGTH;                }
main                                                { return MAIN;                  }
new                                                 { return NEW;                   }
public                                              { return PUBLIC;                }
static                                              { return STATIC;                }
String                                              { return STRING;                }
System\.out\.print                                  { return SYSTEM_OUT_PRINT;      }
System\.out\.println                                { return SYSTEM_OUT_PRINTLN;    }
Integer\.parseInt[^)]*[)]                           { // this is something in the form
                                                      // Integer.parseInt("%d") ,
                                                      // so we just want to get
                                                      // the string which is the number
                                                      // and call atoi() with it,
                                                      // then return a string literal
                                                      // to the parser.
                                                      // NOTE: because the course
                                                      // specified that we can assume
                                                      // the argument to this
                                                      // function is always an integer
                                                      // literal, we don't need
                                                      // to do any typechecking.
                                                      // if the argument was not an
                                                      // integer, it would probably
                                                      // cause unexpected results

                                                      char* number = strdup(yytext);

                                                      // delimit the string
                                                      number[strlen(number) - 2] = '\0';

                                                      // cut the front off the string
                                                      char* actual = number + 18;

                                                      // now convert this to an integer
                                                      int num = atoi(actual);

                                                      yylval.integer = num;
                                                      return INTEGER_LITERAL;       }
if                                                  { return IF;                    }
else                                                { return ELSE;                  }
this                                                { return THIS;                  }
true                                                { return TRUE;                  }
void                                                { return VOID;                  }
while                                               { return WHILE;                 }
return                                              { return RETURN;                }
\&\&                                                { return AND;                   }
\|\|                                                { return OR;                    }
\=\=                                                { return EQUAL_EQUAL;           }
\>\=                                                { return GREATER_EQUAL;         }
\<\=                                                { return LESS_EQUAL;            }
\!\=                                                { return NOT_EQUAL;             }
\+\+                                                { return PLUS_PLUS;             }
\-\-                                                { return MINUS_MINUS;           }
\]                                                  { return *yytext;               }
\}                                                  { return *yytext;               }
\)                                                  { return *yytext;               }
\,                                                  { return *yytext;               }
\/                                                  { return *yytext;               }
\.                                                  { return *yytext;               }
\=                                                  { return *yytext;               }
\!                                                  { return *yytext;               }
\>                                                  { return *yytext;               }
\<                                                  { return *yytext;               }
\-                                                  { return *yytext;               }
\*                                                  { return *yytext;               }
\[                                                  { return *yytext;               }
\{                                                  { return *yytext;               } 
\(                                                  { return *yytext;               }
\+                                                  { return *yytext;               }
\;                                                  { return *yytext;               }

[0-9]+                                              { 
                                                      yylval.integer = atoi(strdup(yytext));
                                                      return INTEGER_LITERAL;
                                                    }

[a-zA-Z][a-zA-Z0-9_]*                               { 
                                                      yylval.string = strdup(yytext);
                                                      return ID;                 
                                                    }

\"([^"\\]|\\.)*\"                                   { 
                                                      yylval.string = strdup(yytext);
                                                      return STRING_LITERAL;                    
                                                    }

"//"[^\n]*                                          { } /* Single line comments */
"/*"([^*]|\*+[^/*])*\*+"/"                          { } /* Multiline comments */

[ \t\n]                                             { } /* skip whitespace */

%%


int yywrap(void) {
    return 1;
}
