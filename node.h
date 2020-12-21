#include <stdbool.h>
#ifndef NODE_H
#define NODE_H

/* types of nodes so that we can stop doing strcmp so often */
#define PROGRAM 0
#define MAINCLASS 1
#define STATEMENTLIST_STATEMENTLIST_STATEMENT 2
#define STATEMENTLIST_STATEMENT 3
#define VARDECL 4
#define INT_TYPE 5
#define BOOLEAN_TYPE 6
#define STRING_TYPE 7
#define STATEMENT_VARDECL 8
#define STATEMENT_PRINTLN 9
#define STATEMENT_PRINT 10
#define STATEMENT_ASSIGNMENT 11
#define ASSIGNMENT 12
#define EXPRESSION_MULTIPLY 13
#define EXPRESSION_DIVIDE 14
#define EXPRESSION_ADD 15
#define EXPRESSION_SUBTRACT 16
#define EXPRESSION_PIECE 17
#define INTEGER_LIT 18
#define STRING_LIT 19
#define BOOLEAN_LIT 20
#define ID_ 21
#define STATEMENT 22
#define VARDECL_NO_RHS 23
#define EXPRESSION_PARENTHESES 24
#define NEG_EXPRESSION_PIECE 25
#define VARDECL_LIST 26
#define VARDECLLISTPIECELIST_VARDECLLISTPIECE 27
#define VARDECLLISTPIECE 28
#define VARDECLLISTPIECE_ID_EQ_EXPR 29
#define VARDECLLISTPIECE_ID 30
#define STATEMENT_IF_ELSE 31
#define EXPRESSION_EQUAL_EQUAL 32
#define EXPRESSION_NOT_EQUAL 33
#define EXPRESSION_GREATER_EQUAL 34
#define EXPRESSION_LESS_EQUAL 35
#define EXPRESSION_GREATER 36
#define EXPRESSION_LESS 37
#define EXPRESSION_AND 38
#define EXPRESSION_OR 39
#define STATEMENT_WHILE 40
#define STATEMENT_ONED_ARRAYDECL 41
#define STATEMENT_TWOD_ARRAYDECL 42 
#define ONED_ARRAYDECL 43
#define TWOD_ARRAYDECL 44
#define STATEMENT_ONED_ARRAY_ASSIGNMENT 45
#define STATEMENT_TWOD_ARRAY_ASSIGNMENT 46
#define ONED_ARRAY_ASSIGNMENT 47
#define TWOD_ARRAY_ASSIGNMENT 48
#define ONED_ARRAY_REFERENCE 49
#define TWOD_ARRAY_REFERENCE 50
#define EXPRESSION_PIECE_DOT_LENGTH 51
#define EXPRESSION_NEGATED 52
#define CLASSDECLLIST_CLASSDECLLIST_CLASSDECL 53
#define CLASSDECLLIST_CLASSDECL 54
#define STATEMENT_VARDECL_LIST 55
#define STATEMENTLIST_WRAPPED 56
#define METHODDECLLIST_METHODDECLLIST_METHODDECL 57
#define METHODDECLLIST_METHODDECL 58
#define METHODDECL 59
#define FORMALLIST_FORMALLIST_FORMAL 60
#define FORMALLIST_FORMAL 61
#define FORMAL 62
#define CLASSDECL 63
#define CLASS_VARIABLE 64
#define STATEMENT_RETURN 65
#define ID_TYPE 66
#define METHODCALL 67
#define STATEMENT_METHODCALL 68
#define VARDECL_NEW_RHS 69
#define FORMAL_ONED_ARRAY 70
#define FORMAL_TWOD_ARRAY 71
#define STATEMENT_IF_NO_ELSE 72
#define VARDECLLISTPIECE_ID_EQ_NEW 73
#define ARGLIST_ARGLIST_ARG 74
#define ARGLIST_ARG 75
#define ARG 76
#define METHODDECL_ONED 77
#define METHODDECL_TWOD 78
#define STATEMENTLIST_STATEMENTLIST_SUB 79

struct data {
    union value_t{
        char* stringValue;
        int intValue;
        bool booleanValue;
    } value;
    int lineNum; // the line number that this node was created at
    int use_line_nums[50]; // line numbers that this was used at
    int num_uses;
    char* extendedClass; // the class that this class extends
    char* className; // the class that this variable belongs to
    char* argv_name; // the name of the argv array
    char* type; // the type of the variable
};

struct node{
    struct data data;
    int nodeType;
    struct node* children[5];
    int numOfChildren;
};

// a oned array which remembers it's length
struct oned_arr {
    char* type; // the type of each of the elements
    int length; // the length of this oned array
    void* array; // the actual array. this is a void pointer
                 // because it could be of many different types
};

// a representation of an array
struct twod_arr {
    char* type; // the type of each of the elements
    int dims; // the number of dimensions (1 or 2)
    int num_arrays; // the number of 1d arrays
    struct oned_arr* oned_arrays[100]; // allow 100 arrays
};

struct symbol_table_entry {
    char* id;
    struct data data;

    int is_array;
    int is_class_var;
    int is_method;
    int array_dims;

    struct twod_arr* array;
    char* offset_name;
};

struct symbol_table {
    struct symbol_table_entry* symbol_table[500];
    int numEntries;
    struct symbol_table* parent;
    struct symbol_table* children[10];
    int num_children;
    char* table_name;
};

struct node* new_node(int nodeType);
void add_child(struct node* parent, struct node* child);
void setStringValue(struct node* node, char* s);
void setIntValue(struct node* node, int i);
void setBooleanValue(struct node* node, bool b);

extern int yylineno;
#endif
