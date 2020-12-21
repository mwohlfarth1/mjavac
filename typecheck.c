#include "typecheck.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int DEBUG; /* whether or not we should print debug things. this is */
           /* set in the main() function contained in parser.y     */
char* current_vardecl_list_type = NULL;
bool handling_class_declarations = false;
struct symbol_table* global;
struct symbol_table* current_table; // the table we should look at if we're looking up a
                                    // variable
int my_argc; // the argc from the main function
char** my_argv; // the argv from the main function

//---------------------------------------------------------------------------------------------------
//- functions for building the symbol table ---------------------------------------------------------
//---------------------------------------------------------------------------------------------------

/* 
 * traverse the tree and add all ids to the appropriate symbol table. check for
 * some use w/o and before declaration. the rest of the use w/o declaration will
 * be checked by clean_up_symbol_table()
 */
void build_symbol_table(struct node * root, struct symbol_table* g) {
    // this root node is a PROGRAM node. we need to build symbol
    // tables for the main class and each of the other classes.
    // when we get to the other classes, we'll need to build a separate
    // symbol table for each of the methods, but we'll deal with that later.
    // we already have a root symbol table, which doesn't do anything. it's just
    // a parent. 

    global = g;
    
    // a program has to have a MainClass, so we have to build a symbol table
    // for that MainClass.
    // allocate memory for the main class's symbol table and remind the
    // program that the root symbol table is it's parent
    struct symbol_table* main_symbol_table = malloc(sizeof(struct symbol_table));
    main_symbol_table->parent = global; // the global table
    main_symbol_table->num_children = 0;
    main_symbol_table->table_name = strdup(root->children[0]->data.value.stringValue);
    global->children[global->num_children] = main_symbol_table;
    global->num_children++;
    build_main_class_symbol_table(root->children[0], main_symbol_table); // child 0 is the main class

    current_table = main_symbol_table;

    // now that we've built the symbol table for the main class, we have to 
    // build symbol tables for the other classes, if necessary
    if (root->numOfChildren == 2) { // ClassDeclList follows the MainClass
        // we don't want to build any symbol tables yet because we don't 
        // know how many classes there are
        build_class_decl_list_tables(root->children[1]);
    }
    else if (root->numOfChildren > 2) {
        printf("ERROR: PROGRAM has more than 2 children\n");
    }
}

/* 
 * build the symbol table for the main class
 */
void build_main_class_symbol_table(struct node* main_class,
                                   struct symbol_table* main_symbol_table) {

    /* check arguments */
    if (main_class == NULL) {
        printf("ERROR: build_main_class_symbol_table: main_class is NULL\n");
    }
    else if (main_symbol_table == NULL) {
        printf("ERROR: build_main_class_symbol_table: main_symbol_table is NULL\n");
    }

    // a main class either has no statements or a statementlist. we can
    // check that based on the number of children
    struct data dat;
    dat.type = "string"; // argv is always a string array
    dat.lineNum = 2; // it might not actually be 2, but this is pretty much the only
                     // time that that doesn't matter

    my_argv = &my_argv[2]; // shift over two
    my_argc -= 2;

    // create a 2d array
    struct twod_arr* twod = malloc(sizeof(struct twod_arr));
    twod->type = "string";
    twod->dims = 2;
    twod->num_arrays = 0; // so that we can use it as an index

    // okay. let's build an array so that we can put it in the symbol table later.
    // we need to allocate memory for each of the oned arrays (each of the character strings)
    for (int i = 0; i < my_argc; i++) {
        // allocate memory for the struct
        struct oned_arr* oned = malloc(sizeof(struct oned_arr));

        // allocate memory for the characters
        oned->array = (void*) malloc(sizeof(char) * (strlen(my_argv[i]) + 1));

        // copy the characters into the array
        for (int j = 0; j < strlen(my_argv[i] + 1); j++) {
            ((char*)(oned->array))[j] = my_argv[i][j];
        }

        // set the type of the oned array to be a string
        oned->type = "string";

        // set the length of this oned array
        oned->length = strlen(my_argv[i]) + 1;

        // okay now we're done allocating memory and building the struct for the 
        // oned array. now we have to assign that to the twod array
        twod->oned_arrays[twod->num_arrays] = oned;
        twod->num_arrays++;
    }

    // if we haven't found any type errors yet, we can allocate memory for an array
    // of char* and put the argv arguments into it
    if (numErrors == 0) {
        // we don't need to allocate any memory for this array, because it will always
        // exist in the parent scope. we just need to add it to the symbol table
        add_to_symbol_table_tc("argc", dat, 1, 0, 0, main_symbol_table, NULL, 0);
        add_to_symbol_table_tc(main_class->data.argv_name, dat, 1, 0, 0, main_symbol_table, twod, 2);
    }

    if (main_class->numOfChildren == 1) { // has a statementlist
        // then, we can handle the statmentlist and put that data in main's symbol table
        build_statement_list_symbol_table(main_class->children[0], main_symbol_table);
    }
    else if (main_class->numOfChildren == 0) { // doesn't have a statementlist
        // since there are no statements to deal with, we literally just
        // have to add argv to the symbol table. we've already done that
    }

    // now that we've handled argv and any statements that the main class might have,
    // we are done
    return;
}

/* 
 * call build_class_symbol_table() for each class declaration
 */
void build_class_decl_list_tables(struct node* class_decl_list) {
    // for each class declaration that we find, we want to build a new
    // table with the parent of "global".
    // we'll have another function handle each of the class declarations 
    // themselves. here, we'll just traverse the tree until we find each of them.

    if (class_decl_list->nodeType == CLASSDECLLIST_CLASSDECLLIST_CLASSDECL) {
        // this class decl list has two children, the list to the left and
        // a classdecl to the right. recurse on the left and then don't 
        // on the right
        build_class_decl_list_tables(class_decl_list->children[0]);
        build_class_symbol_table(class_decl_list->children[1]);
    }
    else if (class_decl_list->nodeType == CLASSDECLLIST_CLASSDECL) {
        // this class decl list is actually just a class declaration,
        // so just call that function to handle it
        build_class_symbol_table(class_decl_list->children[0]);
    }

    return;
}

/*
 * build a symbol table for just this class
 */
void build_class_symbol_table(struct node* class_decl) {
    // okay. now the class_decl is a pointer to a class declaration.
    // lets check to make sure
    if (class_decl->nodeType != CLASSDECL) {
        printf("ERROR: build_class_symbol_table: class_decl isn't a class declaration\n");
    }

    // allocate a new symbol table for this class
    if (find_table(class_decl->data.value.stringValue, global) != NULL) {
        // if we can find a table that has the same name as this class,
        // then this class is a redeclaration. don't create a new class
        // and don't bother checking anything it contains
        //printf("TYPEERROR: redeclaration on %d\n", class_decl->data.lineNum);
        //numErrors++;
    }
    else {
        struct symbol_table* class_symbol_table = malloc(sizeof(struct symbol_table));
        class_symbol_table->parent = global; // the global table
        class_symbol_table->num_children = 0;
        class_symbol_table->table_name = strdup(class_decl->data.value.stringValue);
        global->children[global->num_children] = class_symbol_table;
        global->num_children++;

        current_table = class_symbol_table;

        // this class might have children that are statementlist and methoddecllist,
        // or it might not have one of those or it might have both.
        // go through the children and do what we're supposed to for each of them
        for (int i = 0; i < class_decl->numOfChildren; i++) {
            // if it's a statementlist, it's a bunch of declarations before the methods
            if (class_decl->children[i]->nodeType == STATEMENTLIST_STATEMENTLIST_STATEMENT ||
                class_decl->children[i]->nodeType == STATEMENTLIST_STATEMENT) {
                handling_class_declarations = true; // turn on so that statementlist 
                                                    // knows what to do
                build_statement_list_symbol_table(class_decl->children[i], class_symbol_table);
                handling_class_declarations = false;
            }
            // if it's a methoddeclaration list, we need to handle each of the methods 
            else if (class_decl->children[i]->nodeType == METHODDECLLIST_METHODDECLLIST_METHODDECL ||
                     class_decl->children[i]->nodeType == METHODDECLLIST_METHODDECL) {
                build_method_list_tables(class_decl->children[i], class_symbol_table);
            }
        }
    }
}

/* 
 * call build_method_symbol_table() for each method in the class
 */
void build_method_list_tables(struct node* method_list, struct symbol_table* class_symbol_table) {
    if (method_list->nodeType == METHODDECLLIST_METHODDECLLIST_METHODDECL) {
        // the left child is a methoddecllist, the right child is a methoddecl.
        // recurse on the left, not on the right
        build_method_list_tables(method_list->children[0], class_symbol_table);
        build_method_symbol_table(method_list->children[1], class_symbol_table);
    }
    else if (method_list->nodeType == METHODDECLLIST_METHODDECL) {
        // the left child is a method declaration. there is no right child
        build_method_symbol_table(method_list->children[0], class_symbol_table);
    }

    return;
}

/* 
 * build the symbol table for the method and make sure the parent 
 * is set to the class symbol table
 */
void build_method_symbol_table(struct node* method_decl, struct symbol_table* class_symbol_table) {
    // first, check arguments
    if ((method_decl == NULL) ||
        (class_symbol_table == NULL)) {
        printf("ERROR: build_method_symbol_table: an argument was invalid\n");
    }
    
    // we need to add this method as an entry in the class's symbol table so that
    // on our second pass, we can make sure that things have valid method names
    add_to_symbol_table_tc(method_decl->data.value.stringValue, method_decl->data, 0, 0, 1, class_symbol_table, NULL, 0);

    // now, create a symbol table for this method
    struct symbol_table* method_table = malloc(sizeof(struct symbol_table));
    method_table->num_children = 0;
    method_table->parent = class_symbol_table;
    method_table->table_name = strdup(method_decl->data.value.stringValue);
    class_symbol_table->children[class_symbol_table->num_children] = method_table;
    class_symbol_table->num_children++;

    current_table = method_table;

    // create a data object for the entry.
    struct data dat;
    // decide what the type of the return value is
    switch (method_decl->children[0]->nodeType) {
        case INT_TYPE: ; 
                        dat.type = "int";
                        break;
        case BOOLEAN_TYPE: ; 
                        dat.type = "boolean";
                        break;
        case STRING_TYPE: ; 
                        dat.type = "string";
                        break;
        case ID_TYPE: ;
                        dat.type = method_decl->children[0]->data.value.stringValue; // ID_TYPE
                        break;
        default:
                        printf("ERROR: build_method_symbol_table: invalid switch case\n");
                        break;
    }

    // we're going to add the return value's type to the symbol table
    // as if it were a variable. that way, when we get to the return statement,
    // we can check to make sure we returned something of the correct type
    /* // don't do this for 3.1
    if (method_decl->nodeType == METHODDECL) { // doesn't return an array
        add_to_symbol_table_tc("__RETVAL", dat, 0, 0, 0, method_table, NULL, 0);
    }
    else if (method_decl->nodeType == METHODDECL_ONED) { // returns a oned array
        add_to_symbol_table_tc("__RETVAL", dat, 1, 0, 0, method_table, NULL, 0);
    }
    else if (method_decl->nodeType == METHODDECL_TWOD) { // returns a 2d array
        add_to_symbol_table_tc("__RETVAL", dat, 1, 0, 0, method_table, NULL, 0);
    }
    */

    // now that we've added the return value to the table, we need to add each
    // of the formal arguments to the table, if there are any.
    // then we need to go through each of the statements

    if (method_decl->numOfChildren == 1) {
        // this method declaration has no formal arguments and no statements.
        // we don't have to put anything in the symbol table. if we were
        // expected to check for missing return statements, we would have to do that
        // here
    }
    else if (method_decl->numOfChildren == 2) {
        // we either have a formallist or a statementlist
        if (method_decl->children[1]->nodeType == FORMALLIST_FORMALLIST_FORMAL || 
            method_decl->children[1]->nodeType == FORMALLIST_FORMAL) {
            // we have a formallist but no statements. therefore, we don't care very much.
            // unfortunately, we have to typecheck when someone calls this function,
            // so we still have to add the arguments to the symbol table
            build_formallist_symbol_table(method_decl->children[1], method_table);
        }
        else if (method_decl->children[1]->nodeType == STATEMENTLIST_STATEMENTLIST_STATEMENT ||
                 method_decl->children[1]->nodeType == STATEMENTLIST_STATEMENT) {
            // we have a statementlist
            build_statement_list_symbol_table(method_decl->children[1], method_table);
        }
    }
    else if (method_decl->numOfChildren == 3) {
        // we have both a formallist and a statement list
        build_formallist_symbol_table(method_decl->children[1], method_table);
        build_statement_list_symbol_table(method_decl->children[2], method_table);
    }

    return;
}

/* 
 * add each of the formal arguments to the symbol table for that method
 */
void build_formallist_symbol_table(struct node* formal_list, struct symbol_table* method_table) {
    struct node* to_handle;

    if (formal_list->nodeType == FORMALLIST_FORMALLIST_FORMAL) {
        // the lhs is a list, the rhs is not. recurse on the left
        build_formallist_symbol_table(formal_list->children[0], method_table);
        to_handle = formal_list->children[1];
    }
    else if (formal_list->nodeType == FORMALLIST_FORMAL) {
        to_handle = formal_list->children[0];
    }

    // now that we've recursed on the lists, we just have to handle the to_handle
    struct data dat; // for adding to symbol table
    // decide what type the variable is
    switch (to_handle->children[0]->nodeType) {
        case INT_TYPE: ; 
                        dat.type = "int";
                        break;
        case BOOLEAN_TYPE: ; 
                        dat.type = "boolean";
                        break;
        case STRING_TYPE: ; 
                        dat.type = "string";
                        break;
        case ID_TYPE: ;
                        dat.type = to_handle->children[0]->data.value.stringValue; // ID_TYPE
                        break;
        default:
                        printf("ERROR: build_method_symbol_table: invalid switch case\n");
                        break;
    }

    // since this is a formal argument, we want to change the name
    // of the variable a little so that we know this is a formal argument
    // when we do code generation.
    char* new_name = malloc(sizeof(char) * (strlen(to_handle->data.value.stringValue) + (strlen("__formal_")) + 1));
    memcpy(new_name, "__formal_", strlen("__formal_")); // copy the "__formal_" in 
    memcpy(new_name + strlen("__formal_"), to_handle->data.value.stringValue, strlen(to_handle->data.value.stringValue)); // copy in the original name of the variable
    new_name[strlen("__formal_") + strlen(to_handle->data.value.stringValue)] = '\0'; // null terminate the string

    // finally, add the formal argument to the symbol table.
    if (to_handle->nodeType == FORMAL) {
        add_to_symbol_table_tc(new_name, dat, 0, 0, 0, method_table, NULL, 0);
    }
    else if (to_handle->nodeType == FORMAL_ONED_ARRAY) {
        add_to_symbol_table_tc(new_name, dat, 1, 0, 0, method_table, NULL, 1);
    }
    else if (to_handle->nodeType == FORMAL_TWOD_ARRAY) {
        add_to_symbol_table_tc(new_name, dat, 1, 0, 0, method_table, NULL, 2);
    }

    return;
}

/* 
 * call build_statement_symbol_table() for each statement in the list
 */
void build_statement_list_symbol_table(struct node* root, struct symbol_table* table) {
    if (root->nodeType == STATEMENTLIST_STATEMENTLIST_STATEMENT) {
        // the lhs child is a statement list
        build_statement_list_symbol_table(root->children[0], table);

        // the rhs child is a statement
        build_statement_symbol_table(root->children[1], table);
    }
    else if (root->nodeType == STATEMENTLIST_STATEMENT) {
        build_statement_symbol_table(root->children[0], table);
    }
    else if (root->nodeType == STATEMENTLIST_WRAPPED) {
        // the only child is just a statementlist
        build_statement_list_symbol_table(root->children[0], table);
    }
}

/*
 * add to the symbol table for the statement in question
 */
void build_statement_symbol_table(struct node* statement, struct symbol_table* table) {

    // check arguments
    if ((statement == NULL) || (table == NULL)) {
        printf("ERROR: build_statement_symbol_table: argument is NULL\n");
    }

    // each type of statement is handled a little differently.
    switch (statement->nodeType) {
        case STATEMENT_PRINTLN: ;
                    build_expression_symbol_table(statement->children[0], table);
                    break;
        case STATEMENT_PRINT: ;
                    build_expression_symbol_table(statement->children[0], table);
                    break;
        case STATEMENT_ASSIGNMENT: ;
                    build_assignment_symbol_table(statement->children[0], table);
                    break;
        case STATEMENT_IF_ELSE: ;
                    build_expression_symbol_table(statement->children[0], table);
                    build_statement_list_symbol_table(statement->children[1], table);
                    build_statement_list_symbol_table(statement->children[2], table);
                    break;
        case STATEMENT_IF_NO_ELSE: ;
                    build_expression_symbol_table(statement->children[0], table);
                    build_statement_list_symbol_table(statement->children[1], table);
                    break;
        case STATEMENT_WHILE: ;
                    build_expression_symbol_table(statement->children[0], table);
                    build_statement_list_symbol_table(statement->children[1], table);
                    break;
        case STATEMENT_ONED_ARRAY_ASSIGNMENT: ;
                    build_oned_array_assignment_symbol_table(statement->children[0], table);
                    break;
        case STATEMENT_TWOD_ARRAY_ASSIGNMENT: ;
                    build_twod_array_assignment_symbol_table(statement->children[0], table);
                    break;
        case STATEMENT_RETURN: ;
                    if (statement->numOfChildren == 1) { // return something;
                        build_expression_symbol_table(statement->children[0], table);
                    }
                    else if (statement->numOfChildren == 0) { // return;
                        // do nothing. 
                    }
                    break;
        case STATEMENT_METHODCALL: ;
                    build_methodcall_symbol_table(statement->children[0], table);
                    break;
        case STATEMENT_VARDECL: ;
                    build_vardecl_symbol_table(statement->children[0], table);
                    break;
        case STATEMENT_VARDECL_LIST: ;
                    build_vardecl_list_symbol_table(statement->children[0], table);
                    break;
        case STATEMENT_ONED_ARRAYDECL: ;
                    build_oned_arraydecl_symbol_table(statement->children[0], table);
                    break;
        case STATEMENT_TWOD_ARRAYDECL: ;
                    build_twod_arraydecl_symbol_table(statement->children[0], table);
                    break;
        default: 
                    printf("ERROR: build_statement_symbol_table: invalid switch case\n");
                    break;
    }

}

/* 
 * build the symbol table for an expression
 */
void build_expression_symbol_table(struct node* expression, struct symbol_table* table) {
    if (expression == NULL) {
        printf("ERROR: build_expression_symbol_table: expression was NULL\n");
    }

    switch (expression->nodeType) {
        case EXPRESSION_MULTIPLY: ; // for these 4, both sides are expressions, so evaluate
                                    // both sides
        case EXPRESSION_DIVIDE: ;
        case EXPRESSION_ADD: ;
        case EXPRESSION_SUBTRACT: ;
            build_expression_symbol_table(expression->children[0], table);
            build_expression_symbol_table(expression->children[1], table);
            break;
        case EXPRESSION_PARENTHESES: ;
            // only child is an expression, evaluate that
            build_expression_symbol_table(expression->children[0], table);
            break;
        case EXPRESSION_PIECE: ; // for these 2, only child is an expression piece
        case NEG_EXPRESSION_PIECE: ;
            // only child is an expression piece
            build_expression_piece_symbol_table(expression->children[0], table);
            break;
        case EXPRESSION_NEGATED: ;
            // only child is an expression
            build_expression_symbol_table(expression->children[0], table);
            break;
        case EXPRESSION_EQUAL_EQUAL: ; // these 8 have two expression children
        case EXPRESSION_NOT_EQUAL: ;
        case EXPRESSION_GREATER_EQUAL: ;
        case EXPRESSION_LESS_EQUAL: ;
        case EXPRESSION_GREATER: ;
        case EXPRESSION_LESS: ;
        case EXPRESSION_AND: ;
        case EXPRESSION_OR: ;
            // two children are both expressions
            build_expression_symbol_table(expression->children[0], table);
            build_expression_symbol_table(expression->children[1], table);
            break;
        case EXPRESSION_PIECE_DOT_LENGTH: ;
            // this is either:
                // ID.length
                // ExprPiece.length

            if (expression->numOfChildren == 1) { // ExprPiece.length
                // check the expression piece
                build_expression_piece_symbol_table(expression->children[0], table);
            }
            else { // ID.length
                // if this ID isn't in the symbol table, it's not in a previous
                // statement, it's not a formal argument or a class member,
                // and it's not a global variable.
                // therefore, it's a use without declaration
                if (!id_exists_in_scope_or_higher(expression->data.value.stringValue, table)) {
                    //printf("TYPEERROR: use without declaration on %d\n", expression->data.lineNum);
                    //numErrors++;
                }
                else {
                    // if this ID is in a symbol table (or one higher up), then
                    // this is allowed. do nothing
                }

                // we're not going to bother adding the ID to the symbol table because
                // it's either already there or we don't want it there because
                // we want to throw use without declaration for later uses
            }
            break;
        default:
            printf("ERROR: build_expression_symbol_table: invalid switch case\n");
            break;
    }
}

/* 
 * build the symbol table for a single expression piece
 */
void build_expression_piece_symbol_table(struct node* piece, struct symbol_table* table) {
    // an expression piece can be a lot of different things.
    switch (piece->nodeType) {
        case INTEGER_LIT: ;
            // we don't store these in the symbol table
            break;
        case STRING_LIT: ;
            // we don't store these in the symbol table
            break;
        case BOOLEAN_LIT: ;
            // we don't store these in the symbol table
            break;
        case ID_: ;
            // check whether the ID is in the symbol table.
            // if the ID is in the symbol table (or a higher up one), then
            // we have no qualms with this
            if (id_exists_in_scope_or_higher(piece->data.value.stringValue, table)) {
                // do nothing
            }
            // if the ID isn't in any symbol tables, then this is a use without
            // declaration
            else {
                //printf("TYPEERROR: use without declaration on %d\n", piece->data.lineNum);
                //numErrors++;
            }

            break;
        case ONED_ARRAY_REFERENCE: ; // ID[Expr]
            // if the ID exists in this scope or higher, we don't care
            if (id_exists_in_scope_or_higher(piece->data.value.stringValue, table)) {
                // do nothing
            }
            // otherwise, this is a use without declaration
            else {
                //printf("TYPEERROR: use without declaration on %d\n", piece->data.lineNum);
                //numErrors++;
            }

            // check the expression in the index
            build_expression_symbol_table(piece->children[0], table);

            break;
        case TWOD_ARRAY_REFERENCE: ; // ID[Expr][Expr]
            // if the ID exists in this scope or higher, we don't care
            if (id_exists_in_scope_or_higher(piece->data.value.stringValue, table)) {
                // do nothing
            }
            // otherwise, this is a use without declaration
            else {
                //printf("TYPEERROR: use without declaration on %d\n", piece->data.lineNum);
                //numErrors++;
            }

            // check the first index expression
            build_expression_symbol_table(piece->children[0], table);

            // check the second index expression
            build_expression_symbol_table(piece->children[1], table);

            break;
        case CLASS_VARIABLE: ; // ID.ID or this.ID
            if (strcmp(piece->data.className, "this") == 0) { // this.ID
                // make sure that the current class has the ID we want
                if (class_var_is_in_symbol_table(piece->data.value.stringValue, table)) {
                    // do nothing. this is fine
                }
                else {
                    //printf("TYPEERROR: use without declaration on %d\n", piece->data.lineNum);
                    //numErrors++;
                }
            }
            else { // ID.ID
                // nothing to do.
            }

            break;
        case METHODCALL: ; // MethodCall or new ID().MethodCall
            if (piece->data.value.stringValue == NULL) { // MethodCall
                // check the MethodCall child
                build_methodcall_symbol_table(piece->children[0], table);
            }
            else { // new ID().MethodCall
                // check the MethodCall child
                build_methodcall_symbol_table(piece->children[0], table);
            }
            break;
        default:
            printf("ERROR: build_expression_piece_symbol_table: invalid switch case\n");
            break;
    }
}

/* 
 * build the symbol table for an assignment statement
 */
void build_assignment_symbol_table(struct node* assignment, struct symbol_table* table) {
    // there are a lot of different types of assignment statements. we can differentiate
    // between them by number of children and then go deeper
    if (assignment->numOfChildren == 0) { // this is "ID = new ID();"
        // if ID is already in the table, we're good
        if (id_exists_in_scope_or_higher(assignment->data.value.stringValue, table)) {
            // do nothing
        }
        // otherwise, this is a use without declaration
        else {
            //printf("TYPEERROR: use without declaration on %d\n", assignment->data.lineNum);
            //numErrors++;
        }

    }
    else if (assignment->numOfChildren == 1) { // this is "ID = Expr;" or
                                               // "ID.ID = Expr;" or
                                               // "this.ID = Expr;"

        if (assignment->data.className == NULL) { // "ID = Expr;"
            // if id exists, then we're good
            if (id_exists_in_scope_or_higher(assignment->data.value.stringValue, table)) {
            }
            // otherwise, this is a use without declaration
            else {
                //printf("TYPEERROR: use without declaration on %d\n", assignment->data.lineNum);
                //numErrors++;
            }

            // check the rhs expression
            build_expression_symbol_table(assignment->children[0], table);

        }
        else if (strcmp(assignment->data.className, "this") == 0) { // "this.ID = Expr;"

            // NOTE: since we're dealing with an assignment that contains "this", we've
            // already finished dealing with the class variables. therefore, they're
            // in the table and we can look for them

            // if this is a valid class variable, we're good
            if (class_var_is_in_symbol_table(assignment->data.value.stringValue, table)) {
                // do nothing
            }
            // otherwise, this is a use without declaration
            else {
                //printf("TYPEERROR: use without declaration on %d\n", assignment->data.lineNum);
                //numErrors++;
            }

            // check the rhs expression
            build_expression_symbol_table(assignment->children[0], table);
        }
        else { // "ID.ID = Expr;"
            // check the rhs expression
            build_expression_symbol_table(assignment->children[0], table);
        }

    }
    else if (assignment->numOfChildren == 2) { // this is "ID = new Type[Expr];"
        // if the ID on the lhs exists, we're good
        if (id_exists_in_scope_or_higher(assignment->data.value.stringValue, table)) {
            // do nothing
        }
        // otherwise, this is a use without declaration
        else {
            //printf("TYPEERROR: use without declaration on %d\n", assignment->data.lineNum);
            //numErrors++;
        }

        // check the expression on the rhs
        build_expression_symbol_table(assignment->children[1], table);

    }
    else if (assignment->numOfChildren == 3) { // this is "ID = new Type[Expr][Expr];"
        // if the ID on the lhs exists, we're good
        if (id_exists_in_scope_or_higher(assignment->data.value.stringValue, table)) {
            // do nothing
        }
        // otherwise, this is a use without declaration
        else {
            //printf("TYPEERROR: use without declaration on %d\n", assignment->data.lineNum);
            //numErrors++;
        }

        // check the first expression on the rhs
        build_expression_symbol_table(assignment->children[1], table);

        // check the second expression on the rhs
        build_expression_symbol_table(assignment->children[2], table);
    }
}

/* 
 * build the symbol table for a one dimensional array assignment
 * and for a twod array that the first layer is being assigned to
 */
void build_oned_array_assignment_symbol_table(struct node* oned, struct symbol_table* table) {
    // there are two types of array assignments:
        // "ID[Expr] = Expr;"
        // "ID[Expr] = new Type[Expr];"
    // these have different numbers of children

    if (oned->numOfChildren == 2) { // ID[Expr] = Expr;
        // if the lhs id is already in the table, we're good
        if (id_exists_in_scope_or_higher(oned->data.value.stringValue, table)) {
            // do nothing
        }
        // otherwise, this is a use without declaration
        else {
            //printf("TYPEERROR: use without declaration on %d\n", oned->data.lineNum);
            //numErrors++;
        }

        // check the expression that's the left index
        build_expression_symbol_table(oned->children[0], table);

        // check the expression that's the rhs
        build_expression_symbol_table(oned->children[1], table);
    }
    else if (oned->numOfChildren == 3) { // ID[Expr] = new Type[Expr];

        // if the lhs ID is in the table, we're good
        if (id_exists_in_scope_or_higher(oned->data.value.stringValue, table)) {
            // do nothing
        }
        // otherwise, this is a use without declaration
        else {
            //printf("TYPEERROR: use without declaration on %d\n", oned->data.lineNum);
            //numErrors++;
        }

        // check the expression that's the lhs index
        build_expression_symbol_table(oned->children[0], table);

        // check the expression that's the rhs index
        build_expression_symbol_table(oned->children[2], table);
    }
}

/* 
 * build the symbol table for two dimensional array assignments
 */
void build_twod_array_assignment_symbol_table(struct node* twod, struct symbol_table* table) { 
    // there are three types of 2d array assignments, and they each have different 
    // number of children

    if (twod->numOfChildren == 3) { // ID[Expr][Expr] = Expr;
        // if the lhs is already in the table, we're good
        if (id_exists_in_scope_or_higher(twod->data.value.stringValue, table)) {
            // do nothing
        }
        // otherwise, this is a use without declaration
        else {
            //printf("TYPEERROR: use without declaration on %d\n", twod->data.lineNum);
            //numErrors++;
        }

        // check the first index on the lhs
        build_expression_symbol_table(twod->children[0], table);

        // check the second index on the lhs
        build_expression_symbol_table(twod->children[1], table);

        // check the expression on the rhs
        build_expression_symbol_table(twod->children[2], table);
    }
    else if (twod->numOfChildren == 5) { // ID[Expr][Expr] = new Type[Expr][Expr];
        // if the lhs id is already in the table, we're good
        if (id_exists_in_scope_or_higher(twod->data.value.stringValue, table)) {
            // do nothing
        }
        // otherwise, this is a use without declaration
        else {
            //printf("TYPEERROR: use without declaration on %d\n", twod->data.lineNum);
            //numErrors++;
        }

        // check the first index on the lhs
        build_expression_symbol_table(twod->children[0], table);

        // check the second index on the lhs
        build_expression_symbol_table(twod->children[1], table);

        // check the first index on the rhs
        build_expression_symbol_table(twod->children[2], table);

        // check the second index on the rhs
        build_expression_symbol_table(twod->children[3], table);
    }
    else if (twod->numOfChildren == 4) { // ID[Expr][Expr] = new Type[Expr];
        // if the lhs id is already in the table, we're good
        if (id_exists_in_scope_or_higher(twod->data.value.stringValue, table)) {
            // do nothing
        }
        // otherwise, this is a use without declaration
        else {
            //printf("TYPEERROR: use without declaration on %d\n", twod->data.lineNum);
            //numErrors++;
        }

        // check the first index on the lhs
        build_expression_symbol_table(twod->children[0], table);

        // check the second index on the lhs
        build_expression_symbol_table(twod->children[1], table);

        // check the index on the rhs
        build_expression_symbol_table(twod->children[3], table);
    }
}

/* 
 * build the symbol table for a method call
 */
void build_methodcall_symbol_table(struct node* methodcall, struct symbol_table* table) {
    if (methodcall->numOfChildren == 1) {
        // there is an arglist that we have to deal with.
        // this is in the form:
            // ID(ArgList)
            // this.ID(ArgList)
            // ID.ID(ArgList)

        if (methodcall->data.className == NULL) { // ID(ArgList)

            // check the arglist
            build_arglist_symbol_table(methodcall->children[0], table);
        }
        else if (strcmp(methodcall->data.className, "this") == 0) { // this.ID(ArgList)

            // check the arglist
            build_arglist_symbol_table(methodcall->children[0], table);
        }
        else { // ID.ID(ArgList)

            // check the arglist
            build_arglist_symbol_table(methodcall->children[0], table);
        }
    }
    else {
        // there is no arglist to deal with.
        // this is in the form:
            // ID()
            // this.ID()
            // ID.ID()

        if (methodcall->data.className == NULL) { // ID()
            // nothing to do.
        }
        else if (strcmp(methodcall->data.className, "this") == 0) { // this.ID()
            // nothing to do.
        }
        else { // ID.ID()
            // nothing to do.
        }
    }
}

/* 
 * build the symbol table for an argument list.
 * NOTE: this function doesn't actually add anything to the symbol table.
 * it just makes sure that we report use without declarations for anything
 * that's used as an argument. on a later pass, we'll check to make sure
 * the arguments in the list fit what's expected
 */
void build_arglist_symbol_table(struct node* arglist, struct symbol_table* table) {
    if (arglist->nodeType == ARGLIST_ARGLIST_ARG) {
        // the left child is another arglist
        build_arglist_symbol_table(arglist->children[0], table);

        // the right child is an arg
        build_arg_symbol_table(arglist->children[1], table);
    }
    else if (arglist->nodeType == ARGLIST_ARG) {
        // the only child is an arg
        build_arg_symbol_table(arglist->children[0], table);
    }
}

/*
 * DONE: build the symbol table for an argument.
 * see header for build_arglist_symbol_table() for an important note
 */
void build_arg_symbol_table(struct node* arg, struct symbol_table* table) {
    // there is only one type of argument, and it's an expression.
    // check the expression
    build_expression_symbol_table(arg->children[0], table);
}

/*
 * build the symbol table for a variable declaration
 */
void build_vardecl_symbol_table(struct node* vardecl, struct symbol_table* table) {
    // a vardecl can be of three types:
        // Type ID = Expr;
        // Type ID;
        // Type ID = new ID();

    struct data dat;
    // figure out the type of the lhs
    dat.type = get_datatype_name(vardecl->children[0]);
    dat.lineNum = vardecl->data.lineNum;

    if (vardecl->nodeType == VARDECL) { // Type ID = Expr;
        // check the expression on the rhs before we add anything to the 
        // table so that we can check whether the things on the rhs are
        // in the table yet (for the case like "int z = z + z;"
        build_expression_symbol_table(vardecl->children[1], table);

        // if this variable exists in the current scope, this is a redeclaration
        if (id_exists_in_scope(vardecl->data.value.stringValue, table)) {
            //printf("TYPEERROR: redeclaration on %d\n", vardecl->data.lineNum);
            //numErrors++;
        }
        // otherwise, if this variable is a class variable, this is a 
        // declaration of the non-class version
        else if (class_var_is_in_symbol_table(vardecl->data.value.stringValue, table)) {
            add_to_symbol_table_tc(vardecl->data.value.stringValue, dat, 0, 1, 0, table, NULL, 0);
        }
        // otherwise, this variable isn't in the current scope and it isn't a class
        // variable, so we can just declare it normally
        else {
            add_to_symbol_table_tc(vardecl->data.value.stringValue, dat, 0, 0, 0, table, NULL, 0);
        }


    }
    else if (vardecl->nodeType == VARDECL_NO_RHS) { // Type ID;
        // if this variable exists in the current scope, this is a redeclaration
        if (id_exists_in_scope(vardecl->data.value.stringValue, table)) {
            //printf("TYPEERROR: redeclaration on %d\n", vardecl->data.lineNum);
            //numErrors++;
        }
        // otherwise, if this variable is a class variable, this is a 
        // declaration of the non-class version
        else if (class_var_is_in_symbol_table(vardecl->data.value.stringValue, table)) {
            // add to the symbol table as a non-class variable
            vardecl->data.type = get_datatype_name(vardecl->children[0]);
            add_to_symbol_table_tc(vardecl->data.value.stringValue, vardecl->data, 0, 1, 0, table, NULL, 0);
        }
        // otherwise, this variable isn't in the current scope and it isn't a class
        // variable, so we can just declare it normally
        else {
            // add to the symbol table as a non-class variable
            vardecl->data.type = get_datatype_name(vardecl->children[0]);
            add_to_symbol_table_tc(vardecl->data.value.stringValue, vardecl->data, 0, 0, 0, table, NULL, 0);

        }
    }
    else if (vardecl->nodeType == VARDECL_NEW_RHS) { // Type ID = new ID();
        // if this variable exists in the current scope, this is a redeclaration
        if (id_exists_in_scope(vardecl->data.value.stringValue, table)) {
            //printf("TYPEERROR: redeclaration on %d\n", vardecl->data.lineNum);
            //numErrors++;
        }
        // otherwise, if this variable is a class variable, this is a 
        // declaration of the non-class version
        else if (class_var_is_in_symbol_table(vardecl->data.value.stringValue, table)) {
            // add to the symbol table as a non-class variable
            add_to_symbol_table_tc(vardecl->data.value.stringValue, dat, 0, 1, 0, table, NULL, 0);
        }
        // otherwise, this variable isn't in the current scope and it isn't a class
        // variable, so we can just declare it normally
        else {
            // add to the symbol table as a non-class variable
            add_to_symbol_table_tc(vardecl->data.value.stringValue, dat, 0, 0, 0, table, NULL, 0);
        }
    }
}

/* 
 * build a symbol table for a vardecl list
 */
void build_vardecl_list_symbol_table(struct node* vardecl_list, struct symbol_table* table) {
    // a vardecl list has two children:
        // the type of the list
        // a VarDeclListPieceList

    // get the type of this list
    char* type;
    switch (vardecl_list->children[0]->nodeType) {
        case INT_TYPE: ;
            type = "int";
            break;
        case BOOLEAN_TYPE: ;
            type = "boolean";
            break;
        case STRING_TYPE: ;
            type = "string";
            break;
        case ID_TYPE: ;
            type = vardecl_list->children[0]->data.value.stringValue;
            break;
        default:
            printf("ERROR: build_vardecl_list_symbol_table: invalid switch case\n");
            break;
    }
    current_vardecl_list_type = type;

    // now handle the list pieces
    build_vardecl_list_piece_list_symbol_table(vardecl_list->children[1], table, type);
}

/* 
 * call the appropriate function to handle a vardecl list piece
 */
void build_vardecl_list_piece_list_symbol_table(struct node* vardecl_piece_list, struct symbol_table* table, char* type) {
    // a vardecllistpiecelist has two types:
        // VARDECLLISTPIECELIST_VARDECLLISTPIECE
        // VARDECLLISTPIECE

    if (vardecl_piece_list->nodeType == VARDECLLISTPIECELIST_VARDECLLISTPIECE) {
        // the left child is a vardecllistpiecelist
        build_vardecl_list_piece_list_symbol_table(vardecl_piece_list->children[0], table, type);

        // the right child is a vardecllist piece
        build_vardecl_list_piece_symbol_table(vardecl_piece_list->children[1], table, type);
    }
    else if (vardecl_piece_list->nodeType == VARDECLLISTPIECE) {
        // the only child is a vadecllistpiece
        build_vardecl_list_piece_symbol_table(vardecl_piece_list->children[0], table, type);
    }
}

/* 
 * build the symbol table for a piece of a vardecl list
 */
void build_vardecl_list_piece_symbol_table(struct node* vardecl_list_piece, struct symbol_table* table, char* type) {
    // there are 3 types of vardecl list pieces
        // ID = Expr
        // ID
        // ID = new ID()

    switch (vardecl_list_piece->nodeType) {
        case VARDECLLISTPIECE_ID_EQ_EXPR: ; // ID = Expr
            if (id_exists_in_scope(vardecl_list_piece->data.value.stringValue, table)) {
                // this ID already exists in the current scope, so this is a redeclaration
                //printf("TYPEERROR: redeclaration on %d\n", vardecl_list_piece->data.lineNum);
                //numErrors++;
            }
            else if (class_var_is_in_symbol_table(vardecl_list_piece->data.value.stringValue, table)) {
                // this ID doesn't exist in the current scope but does exist as a class variable,
                // so we can just redeclare it as a non-class variable
                // add to symbol table as a non-class variable
                vardecl_list_piece->data.type = type;
                add_to_symbol_table_tc(vardecl_list_piece->data.value.stringValue, vardecl_list_piece->data, 0, 1, 0, table, NULL, 0);
            }
            else {
                // this ID doesn't exist anywhere, so we can just declare it as normal
                // add to symbol table as a non-class variable
                vardecl_list_piece->data.type = type;
                add_to_symbol_table_tc(vardecl_list_piece->data.value.stringValue, vardecl_list_piece->data, 0, 0, 0, table, NULL, 0);
            }
           
            // check the expression on the rhs
            build_expression_symbol_table(vardecl_list_piece->children[0], table);
            break;

        case VARDECLLISTPIECE_ID: ; // ID
            if (id_exists_in_scope(vardecl_list_piece->data.value.stringValue, table)) {
                // this ID already exists in the current scope, so this is a redeclaration
                //printf("TYPEERROR: redeclaration on %d\n", vardecl_list_piece->data.lineNum);
                //numErrors++;
            }
            else if (class_var_is_in_symbol_table(vardecl_list_piece->data.value.stringValue, table)) {
                // this ID doesn't exist in the current scope but does exist as a class variable,
                // so we can just redeclare it as a non-class variable
                // add to symbol table as a non-class variable
                vardecl_list_piece->data.type = type;
                add_to_symbol_table_tc(vardecl_list_piece->data.value.stringValue, vardecl_list_piece->data, 0, 1, 0, table, NULL, 0);
            }
            else {
                // this ID doesn't exist anywhere, so we can just declare it as normal
                // add to symbol table as a non-class variable
                vardecl_list_piece->data.type = type;
                add_to_symbol_table_tc(vardecl_list_piece->data.value.stringValue, vardecl_list_piece->data, 0, 0, 0, table, NULL, 0);
            }

            break;

        case VARDECLLISTPIECE_ID_EQ_NEW: ; // ID = new ID()
            if (id_exists_in_scope(vardecl_list_piece->data.value.stringValue, table)) {
                // this ID already exists in the current scope, so this is a redeclaration
                //printf("TYPEERROR: redeclaration on %d\n", vardecl_list_piece->data.lineNum);
                //numErrors++;
            }
            else if (class_var_is_in_symbol_table(vardecl_list_piece->data.value.stringValue, table)) {
                // this ID doesn't exist in the current scope but does exist as a class variable,
                // so we can just redeclare it as a non-class variable
                // add to symbol table as a non-class variable
                vardecl_list_piece->data.type = type;
                add_to_symbol_table_tc(vardecl_list_piece->data.value.stringValue, vardecl_list_piece->data, 0, 1, 0, table, NULL, 0);
            }
            else {
                // this ID doesn't exist anywhere, so we can just declare it as normal
                // add to symbol table as a non-class variable
                vardecl_list_piece->data.type = type;
                add_to_symbol_table_tc(vardecl_list_piece->data.value.stringValue, vardecl_list_piece->data, 0, 0, 0, table, NULL, 0);
            }

            break;
        default:
            printf("ERROR: build_vardecl_list_piece_symbol_table: invalid switch case\n");
            break;
    }
}

/* 
 * build the symbol table for a oned array declaration
 */
void build_oned_arraydecl_symbol_table(struct node* oned, struct symbol_table* table) {
    // there are three types of oned array declarations:
        // Type[] ID = new Type[Expr];
        // Type[] ID = Expr;
        // Type[] ID;

    if (oned->numOfChildren == 3) { // Type[] ID = new Type[Expr];
        // make sure the ID isn't already in the symbol table
        if (id_exists_in_scope(oned->data.value.stringValue, table)) {
            // id exists in current scope, so this is a redeclaration
            //printf("TYPEERROR: redeclaration on %d\n", oned->data.lineNum);
            //numErrors++;
        }
        else if (class_var_is_in_symbol_table(oned->data.value.stringValue, table)) {
            // id exists as a class variable, so we can just make a non-class version
            // add to symbol table as a non-class variable
            oned->data.type = get_datatype_name(oned->children[0]);
            add_to_symbol_table_tc(oned->data.value.stringValue, oned->data, 1, 1, 0, table, NULL, 1);
        }
        else {
            // id doesn't exist, so we can just add to the symbol table
            // add to symbol table as a non-class variable
            oned->data.type = get_datatype_name(oned->children[0]);
            add_to_symbol_table_tc(oned->data.value.stringValue, oned->data, 1, 0, 0, table, NULL, 1);
        }

        // check the expression on the rhs
        build_expression_symbol_table(oned->children[2], table);
    }
    else if (oned->numOfChildren == 2) { // Type[] ID = Expr;

        // make sure the ID isn't already in the symbol table
        if (id_exists_in_scope(oned->data.value.stringValue, table)) {
            // id exists in current scope, so this is a redeclaration
            //printf("TYPEERROR: redeclaration on %d\n", oned->data.lineNum);
            //numErrors++;
        }
        else if (class_var_is_in_symbol_table(oned->data.value.stringValue, table)) {
            // id exists as a class variable, so we can just make a non-class version
            // add to symbol table as a non-class variable
            oned->data.type = get_datatype_name(oned->children[0]);
            add_to_symbol_table_tc(oned->data.value.stringValue, oned->data, 1, 0, 0, table, NULL, 1);
        }
        else {
            // id doesn't exist, so we can just add to the symbol table
            // add to symbol table as a non-class variable
            oned->data.type = get_datatype_name(oned->children[0]);
            add_to_symbol_table_tc(oned->data.value.stringValue, oned->data, 1, 0, 0, table, NULL, 1);
        }

        // check the expression on the rhs
        build_expression_symbol_table(oned->children[1], table);

        // you're not allowed to assign an expression to an array
        //printf("TYPEERROR: type mismatch for declaration on %d\n", oned->data.lineNum);
        //numErrors++;

    }
    else if (oned->numOfChildren == 1) { // Type[] ID;

        // make sure the ID isn't already in the symbol table
        if (id_exists_in_scope(oned->data.value.stringValue, table)) {
            // id exists in current scope, so this is a redeclaration
            //printf("TYPEERROR: redeclaration on %d\n", oned->data.lineNum);
            //numErrors++;
        }
        else if (class_var_is_in_symbol_table(oned->data.value.stringValue, table)) {
            // id exists as a class variable, so we can just make a non-class version
            // add to symbol table as a non-class variable
            oned->data.type = get_datatype_name(oned->children[0]);
            add_to_symbol_table_tc(oned->data.value.stringValue, oned->data, 1, 0, 0, table, NULL, 1);
        }
        else {
            // id doesn't exist, so we can just add to the symbol table
            // add to symbol table as a non-class variable
            oned->data.type = get_datatype_name(oned->children[0]);
            add_to_symbol_table_tc(oned->data.value.stringValue, oned->data, 1, 0, 0, table, NULL, 1);
        }
    }
}

/*
 * build the symbol table for twod array declarations
 */
void build_twod_arraydecl_symbol_table(struct node* twod, struct symbol_table* table) {
    // there are 4 types of twod array declarations:
        // Type[][] ID = new Type[Expr][Expr];
        // Type[][] ID = Expr;
        // Type[][] ID = new Type[Expr];
        // Type[][] ID;

    if (twod->numOfChildren == 4) { // Type[][] ID = new Type[Expr][Expr];

        // make sure the ID isn't already in the symbol table
        if (id_exists_in_scope(twod->data.value.stringValue, table)) {
            // id exists in current scope, so this is a redeclaration
            //printf("TYPEERROR: redeclaration on %d\n", twod->data.lineNum);
            //numErrors++;
        }
        else if (class_var_is_in_symbol_table(twod->data.value.stringValue, table)) {
            // id exists as a class variable, so we can just make a non-class version
            // add to symbol table as a non-class variable
            twod->data.type = get_datatype_name(twod->children[0]);
            add_to_symbol_table_tc(twod->data.value.stringValue, twod->data, 1, 0, 0, table, NULL, 2);
        }
        else {
            // id doesn't exist, so we can just add to the symbol table
            // add to symbol table as a non-class variable
            twod->data.type = get_datatype_name(twod->children[0]);
            add_to_symbol_table_tc(twod->data.value.stringValue, twod->data, 1, 0, 0, table, NULL, 2);
        }

        // check the expression which is the first index
        build_expression_symbol_table(twod->children[2], table);

        // check the expression which is the second index
        build_expression_symbol_table(twod->children[3], table);
    }
    else if (twod->numOfChildren == 2) { // Type[][] ID = Expr;

        // make sure the ID isn't already in the symbol table
        if (id_exists_in_scope(twod->data.value.stringValue, table)) {
            // id exists in current scope, so this is a redeclaration
            //printf("TYPEERROR: redeclaration on %d\n", twod->data.lineNum);
            //numErrors++;
        }
        else if (class_var_is_in_symbol_table(twod->data.value.stringValue, table)) {
            // id exists as a class variable, so we can just make a non-class version
            // add to symbol table as a non-class variable
            twod->data.type = get_datatype_name(twod->children[0]);
            add_to_symbol_table_tc(twod->data.value.stringValue, twod->data, 1, 0, 0, table, NULL, 2);
        }
        else {
            // id doesn't exist, so we can just add to the symbol table
            // add to symbol table as a non-class variable
            twod->data.type = get_datatype_name(twod->children[0]);
            add_to_symbol_table_tc(twod->data.value.stringValue, twod->data, 1, 0, 0, table, NULL, 2);
        }

        // check the expression on the rhs
        build_expression_symbol_table(twod->children[1], table);

        // you can't declare an array with an expression on the rhs
        //printf("TYPEERROR: type mismatch for declaration on %d\n", twod->data.lineNum);
        //numErrors++;
    }
    else if (twod->numOfChildren == 3) { // Type[][] ID = new Type[Expr];

        // make sure the ID isn't already in the symbol table
        if (id_exists_in_scope(twod->data.value.stringValue, table)) {
            // id exists in current scope, so this is a redeclaration
            //printf("TYPEERROR: redeclaration on %d\n", twod->data.lineNum);
            //numErrors++;
        }
        else if (class_var_is_in_symbol_table(twod->data.value.stringValue, table)) {
            // id exists as a class variable, so we can just make a non-class version
            // add to symbol table as a non-class variable
            twod->data.type = get_datatype_name(twod->children[0]);
            add_to_symbol_table_tc(twod->data.value.stringValue, twod->data, 1, 0, 0, table, NULL, 2);
        }
        else {
            // id doesn't exist, so we can just add to the symbol table
            // add to symbol table as a non-class variable
            twod->data.type = get_datatype_name(twod->children[0]);
            add_to_symbol_table_tc(twod->data.value.stringValue, twod->data, 1, 0, 0, table, NULL, 2);
        }
        
        // check the expression on the rhs 
        build_expression_symbol_table(twod->children[2], table);

        // you can't declare an array with an expression on the rhs
        //printf("TYPEERROR: type mismatch for declaration on %d\n", twod->data.lineNum);
        //numErrors++;
    }
    else if (twod->numOfChildren == 1) { // Type[][] ID;

        // make sure the ID isn't already in the symbol table
        if (id_exists_in_scope(twod->data.value.stringValue, table)) {
            // id exists in current scope, so this is a redeclaration
            //printf("TYPEERROR: redeclaration on %d\n", twod->data.lineNum);
            //numErrors++;
        }
        else if (class_var_is_in_symbol_table(twod->data.value.stringValue, table)) {
            // id exists as a class variable, so we can just make a non-class version
            // add to symbol table as a non-class variable
            twod->data.type = get_datatype_name(twod->children[0]);
            add_to_symbol_table_tc(twod->data.value.stringValue, twod->data, 1, 0, 0, table, NULL, 2);
        }
        else {
            // id doesn't exist, so we can just add to the symbol table
            // add to symbol table as a non-class variable
            twod->data.type = get_datatype_name(twod->children[0]);
            add_to_symbol_table_tc(twod->data.value.stringValue, twod->data, 1, 0, 0, table, NULL, 2);
        }
    }
}

//---------------------------------------------------------------------------------------------------
//- type checking functions -------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

/* 
 * traverse the tree and check for incorrect types in any/all operations
 */
void check_types(struct node * root) {

    int error; // whether or not we've found an error at some point

    //printf("root->nodeType: %s\n", get_type_name(root->nodeType));

    // base case is when we deal with expression pieces.
    // we can just set the type of this node and return
    if ((root->nodeType == EXPRESSION_PIECE) ||
        (root->nodeType == NEG_EXPRESSION_PIECE)) {
        if (root->children[0]->nodeType == ID_) {
            struct symbol_table_entry* entry = get_sym_tab_entry(root->children[0]->data.value.stringValue);
            if (entry == NULL) {
                // if the entry was NULL, that means that the variable was a use without
                // declaration and was never added to the table. 
                //printf("ERROR: entry was NULL after getting entry from symbol table\n");
            }
            else {
                root->data.type = strdup(entry->data.type);
            }
        }
        else if (root->children[0]->nodeType == ONED_ARRAY_REFERENCE) {
            // this oned array reference actually has an expression piece under it,
            // so just recurse
            check_types(root->children[0]);
            
            // the expression on the rhs has been evaluated, so we know it
            // doesn't have any type errors. the expression on the rhs has
            // the same type as the array that's on the lhs
            root->children[0]->data.type = root->data.value.stringValue;
            root->data.type = root->children[0]->data.type;
        }
        else if (root->children[0]->nodeType == TWOD_ARRAY_REFERENCE) {
            // this twod array refernce is actually just an expression piece, so
            // recurse
            check_types(root->children[0]);
        }
        else {
            root->data.type = strdup(root->children[0]->data.type);
        }
        return;
    }

    // since we weren't dealing with an expression piece, we may have been
    // dealing with a new class or method declaration. in that case,
    // when we recurse and later ask for the type of a variable
    // from the symbol table, we need to let the getter function know
    // which table to look in.
    if ((root->nodeType == MAINCLASS) ||
        (root->nodeType == CLASSDECL) ||
        (root->nodeType == METHODDECL)) {
        current_table = find_table(root->data.value.stringValue, global);
    }

    if (root->nodeType == VARDECL_LIST) {
        current_vardecl_list_type = get_datatype_name(root->children[0]);
    }

    // if this node isn't an expression piece, then we want to make sure we have
    // the right types for the children, so recurse
    for (int i = 0; i < root->numOfChildren; i++) {
        if (root->nodeType == STATEMENT_IF_ELSE) {
            if (root->children[i] != NULL) {
                check_types(root->children[i]);
            }
        }
        else if (strcmp(root->children[i]->data.type, "undefined") == 0) {

            //printf("child %d has undefined type, recursing on it\n", i);
            check_types(root->children[i]);
        }
        else if (root->nodeType == STATEMENT_ONED_ARRAYDECL) {
            check_types(root->children[i]);
        }
    }

    // now, we've recursed and ensured that the children have the right type. check
    // if the types match
    char* lhs_type;
    char* rhs_type;
    switch (root->nodeType) {
        case ASSIGNMENT: ;
            struct symbol_table_entry* entry2 = get_sym_tab_entry(root->data.value.stringValue);
            if (entry2 == NULL) {
                // if the entry is NULL, that's because this variable was a use without
                // declaration so we didn't even bother putting it in the table.
                // we should do nothing if this is the case. we've already
                // checked the expression on the rhs so we don't have to do anything else
            }
            else {
                if (root->numOfChildren == 0) {
                    // if it has no children, then this statement looks like:
                        // ID = new ID();

                    // we've already checked to make sure the lhs variable exists
                    // (in another function)

                    // now, we need to make sure that the rhs is a valid type
                    if (!is_valid_type(root->data.className)) {
                        //printf("TYPEERROR: invalid type for new() on %d\n", root->data.lineNum);
                        //numErrors++;
                    }

                    // now, let's make sure that the types on the right and left are the same
                    struct symbol_table_entry* entry = get_sym_tab_entry(root->data.value.stringValue);
                    if (strcmp(root->data.className, entry->data.type) != 0) {
                        //printf("TYPEERROR: type mismatch for assignment on %d\n", root->data.lineNum);
                        //numErrors++;
                    }
                }
                else {
                    lhs_type = strdup(entry2->data.type);
                    rhs_type = strdup(root->children[0]->data.type);
                    if ((strcmp(lhs_type, "undefined") == 0) || 
                        (strcmp(rhs_type, "undefined") == 0)) {
                        root->data.type = "undefined";
                    }
                    else if (strcmp(lhs_type, rhs_type) != 0) {
                        //printf("TYPEERROR: type mismatch for assignment on %d\n", root->data.lineNum);
                        //numErrors++;
                        root->data.type = "undefined";
                    }
                    else {
                        root->data.type = strdup(lhs_type);
                    }

                    // now, if the lhs is an array and the rhs is an expression, this is invalid
                    if (entry2->is_array) {
                        if ((root->numOfChildren == 0) ||
                            (root->numOfChildren == 1)) {
                            //printf("TYPEERROR: type mismatch for assignment on %d\n", root->data.lineNum);
                            //numErrors++;
                        }
                    }
                }
            }

            break;
        case VARDECLLISTPIECE_ID_EQ_EXPR: ;
            lhs_type = strdup(get_sym_tab_entry(root->data.value.stringValue)->data.type);
            rhs_type = strdup(root->children[0]->data.type);
            if ((strcmp(lhs_type, "undefined") == 0) || 
                (strcmp(rhs_type, "undefined") == 0)) {
                root->data.type = "undefined";
            }
            else if (strcmp(lhs_type, rhs_type) != 0) {
                //printf("TYPEERROR: type mismatch for assignment on %d\n", root->data.lineNum);
                //numErrors++;
                root->data.type = "undefined";
            }
            else {
                root->data.type = strdup(lhs_type);
                return;
            }
            break;
        case VARDECLLISTPIECE_ID: ;
            // this is a vardecllistpiece which is just an ID.
            // this is already in the symbol table because we've already
            // gone through the first pass. we don't need to do anything
            // because we don't have a value to update this with.
            break;
        case EXPRESSION_PARENTHESES: ;
            // we've already recursed on this node's children to determine
            // the type of it's children, so the single child that this
            // node has will be the type that these parentheses have
            root->data.type = strdup(root->children[0]->data.type);
            break;
        case ONED_ARRAYDECL: ;
            // we've already recursed on each of the children so we
            // definitely have the right type for the index
            if (root->numOfChildren == 3) { // Type[] ID = new Type[Expr];
                // we need to make sure that:
                    // the lhs type is a valid one
                    // the rhs type is a valid one
                    // the lhs and rhs types are the same
                    // the exprssion on the rhs is an int expression

                // check to make sure the lhs type is a valid one
                if (!is_valid_type(root->children[0]->data.type)) {
                    //printf("TYPEERROR: invalid type for declaration on %d\n", root->data.lineNum);
                    //numErrors++;
                }

                // check to make sure the rhs is a valid type
                if (!is_valid_type(root->children[1]->data.type)) {
                    //printf("TYPEERROR: invalid type for declaration on %d\n", root->data.lineNum);
                    //numErrors++;
                }

                // check to make sure the two types are the same
                if (strcmp(root->children[0]->data.type, root->children[1]->data.type) != 0) {
                    //printf("TYPEERROR: type mismatch for declaration on %d\n", root->data.lineNum);
                    //numErrors++;
                }

                // check to make sure the expression on the rhs is an int
                if (strcmp(root->children[2]->data.type, "int") != 0) {
                    // uh oh, the index isn't an integer
                    //printf("TYPEERROR: type mismatch for array index on %d\n", root->data.lineNum);
                    //numErrors++;
                }
            }
            else if (root->numOfChildren == 2) { // Type[] ID = Expr;
                // we need to make sure that:
                    // the type on the lhs is a valid one

                // make sure the type on the lhs is valid
                if (!is_valid_type(root->children[0]->data.type)) {
                    //printf("TYPEERROR: invalid type for declaration on %d\n", root->data.lineNum);
                    //numErrors++;
                }
            }
            else if (root->numOfChildren == 1) { // Type[] ID;
                // make sure the type on the lhs is a valid one
                if (!is_valid_type(root->children[0]->data.type)) {
                    //printf("TYPEERROR: invalid type for declaration on %d\n", root->data.lineNum);
                    //numErrors++;
                }
            }
            break;
        case ONED_ARRAY_REFERENCE: ;
            // need to make sure the index is an int
            root->data.type = root->children[0]->data.type;

            return;
            break;

        case EXPRESSION_PIECE_DOT_LENGTH: ;
            struct symbol_table_entry* entry = NULL;

            // this could either be "ID.length" or "ExprPiece.length"
            if (root->numOfChildren == 1) { // ExprPiece.length
                // we've already checked the expression piece, so we know its type.
                // we need to make sure that that type is either an array
                // or a string
                if (strcmp(root->children[0]->data.type, "string") == 0) {
                    // this is allowed, so we're fine
                    // the length of a string will be an int
                    root->data.type = "int";
                }
                else {
                    // because this isn't a string type, we need to get the symbol table
                    // entry and check whether or not this is an array. the entry
                    // is really stored by the child because there is a child
                    entry = get_sym_tab_entry(root->children[0]->data.value.stringValue);

                    if (entry->is_array) {
                        // we don't care what type it is. we're allowed to find the length for it
                        // the length will be an int
                        root->data.type = "int";
                    }
                    else {
                        // this isn't an array and it's not a string, so we can't do a .length on it
                        //printf("TYPEERROR: type mismatch for .length on %d\n", root->data.lineNum);
                        //numErrors++;
                    }
                }
            }
            else if (root->numOfChildren == 0) { // ID.length
                // we need to look up the ID so that we can check what the type is and such
                entry = get_sym_tab_entry(root->data.value.stringValue);

                if (entry->is_array) {
                    // no problem. we can take the length of an array
                    // the length of an array will be an int
                    root->data.type = "int";
                }
                else if (strcmp(entry->data.type, "string") == 0) {
                    // no problem. we can take the length of a string (or an array
                    // of strings, for that matter)
                    // the length of a string will be an int
                    root->data.type = "int";
                }
                else {
                    // this isn't an array or a string, so we can't find the length of it
                    //printf("TYPEERROR: type mismatch for .length on %d\n", root->data.lineNum);
                    //numErrors++;
                }
            }
            break;
        case STATEMENT_WHILE: ;
            // this is a while loop, so we just need to make sure the first child
            // is a boolean expression or a boolean value

            // we've already ensured that the conditional in the parentheses has
            // all the right types and doesn't include any redeclarations or anything.
            // we still need to make sure the expression is a logical one.
            if ((root->children[0]->nodeType == EXPRESSION_EQUAL_EQUAL) || 
                (root->children[0]->nodeType == EXPRESSION_NOT_EQUAL) || 
                (root->children[0]->nodeType == EXPRESSION_GREATER_EQUAL) || 
                (root->children[0]->nodeType == EXPRESSION_LESS_EQUAL) || 
                (root->children[0]->nodeType == EXPRESSION_GREATER) || 
                (root->children[0]->nodeType == EXPRESSION_LESS) ||
                (root->children[0]->nodeType == EXPRESSION_AND) ||
                (root->children[0]->nodeType == EXPRESSION_OR)) {
                // the rhs is a logical expression. allowed
            }
            else if ((root->children[0]->nodeType == EXPRESSION_PIECE) &&
                     (strcmp(root->children[0]->children[0]->data.type, "boolean") == 0)) {
                // the rhs is just a boolean value, which is allowed
            }
            else {
                // the rhs is not a boolean expression or value, so this is not allowed
                //printf("TYPEERROR: non-logical expression for while condition on %d\n", root->children[0]->data.lineNum);
                //numErrors++;
            }

            break;
        case TWOD_ARRAYDECL: ;
            // need to make sure both of the children are integer types
            if ((strcmp(root->children[0]->data.type, "int") != 0) || 
                (strcmp(root->children[1]->data.type, "int") != 0)) {
                //printf("TYPEERROR: type mismatch for array index on %d\n", root->data.lineNum);
                //numErrors++;
            }
            break;
        case TWOD_ARRAY_REFERENCE: ;
            error = 0;

            // need to make sure the two kids are integer types
            if ((strcmp(root->children[0]->data.type, "int") != 0) || 
                (strcmp(root->children[1]->data.type, "int") != 0)) {
                //printf("TYPEERROR: type mismatch for array index on %d\n", root->data.lineNum);
                error = 1;
                //numErrors++;
            }

            if (!error) {
                root->data.type = "int";
            }
            break;
        case STATEMENT_IF_ELSE: ;
            if (root->nodeType == STATEMENT_IF_ELSE) {

                // we've already ensured that the conditional in the parentheses has
                // all the right types and doesn't include any redeclarations or anything.
                // we still need to make sure the expression is a logical one.
                if ((root->children[0]->nodeType == EXPRESSION_EQUAL_EQUAL) || 
                    (root->children[0]->nodeType == EXPRESSION_NOT_EQUAL) || 
                    (root->children[0]->nodeType == EXPRESSION_GREATER_EQUAL) || 
                    (root->children[0]->nodeType == EXPRESSION_LESS_EQUAL) || 
                    (root->children[0]->nodeType == EXPRESSION_GREATER) || 
                    (root->children[0]->nodeType == EXPRESSION_LESS) ||
                    (root->children[0]->nodeType == EXPRESSION_AND) ||
                    (root->children[0]->nodeType == EXPRESSION_OR)) {
                    // the rhs is a logical expression. allowed
                }
                else if ((root->children[0]->nodeType == EXPRESSION_PIECE) &&
                         (strcmp(root->children[0]->children[0]->data.type, "boolean") == 0)) {
                    // the rhs is just a boolean value, which is allowed
                }
                else {
                    // the rhs is not a boolean expression or value, so this is not allowed
                    //printf("TYPEERROR: non-logical expression for if condition on %d\n", root->children[0]->data.lineNum);
                    //numErrors++;
                }
            }
            else if (root->nodeType == TWOD_ARRAY_ASSIGNMENT) {
                error = 0; 

                // first child is the first index
                if (strcmp(root->children[0]->data.type, "int") != 0) {
                    //printf("TYPEERROR: type mismatch for array index on %d\n", root->data.lineNum);
                    error = 1;
                    //numErrors++;
                }
                
                // second child is the second index
                if (strcmp(root->children[1]->data.type, "int") != 0) {
                    //printf("TYPEERROR: type mismatch for array index on %d\n", root->data.lineNum);
                    error = 1;
                    //numErrors++;
                }

                // third child is the expression on the rhs
                if (strcmp(root->children[2]->data.type, "int") != 0) {
                    //printf("TYPEERROR: type mismatch for assignment on %d\n", root->data.lineNum);
                    error = 1;
                    //numErrors++;
                }

                if (!error) {
                    root->data.type = "int";
                }
            }
            break;
        case STATEMENT_VARDECL: ;
            // we've already recursed on each of the children, so we just need
            // to carry the type of the child up the chain
            root->data.type = root->children[0]->data.type;
            break;
        case STATEMENTLIST_STATEMENT: ;
            // we've already recursed on the child (which is the statement), so we
            // have nothing to do
            break;
        case STATEMENT_PRINTLN: ;
            // we've already recursed on the child (which is the statement), so we
            // have nothing to do.
            // set the type of this node to the type of the child
            root->data.type = root->children[0]->data.type;
            break;
        case VARDECL_NO_RHS: ;
            break;
        case VARDECL_NEW_RHS: ;
            // this is in the form:
                // Type ID = new ID();
            
            // we need to make sure that there exists a class called ID
            // and that the type on the left is the same as that
            if (!is_valid_type(root->data.className)) {
                //printf("TYPEERROR: invalid type for new() on %d\n", root->data.lineNum);
                //numErrors++;
            }

            // make sure that the type on the lhs is a valid type
            if (!is_valid_type(get_datatype_name(root->children[0]))) {
                //printf("TYPEERROR: invalid type for declaration on %d\n", root->data.lineNum);
                //numErrors++;
            }

            // make sure that the left and right types are the same
            if (strcmp(root->data.className, get_datatype_name(root->children[0])) != 0) {
                //printf("TYPEERROR: type mismatch for declaration on %d\n", root->data.lineNum);
                //numErrors++;
            }

            break;
        case STATEMENT_ONED_ARRAY_ASSIGNMENT: ;
            // we've already checked the oned array assignment, so we have nothing to do
            break;
        case STATEMENT_ONED_ARRAYDECL: ;
            // we've already checked the oned arraydeclaration, so we have nothing to do
            break;
        case STATEMENT_ASSIGNMENT: ;
            // we've already checked the assignment, so we have nothing to do
            break;
        case STATEMENT_TWOD_ARRAY_ASSIGNMENT: ;
            // we've already checked the assignment, so we have nothing to do
            break;
        case STATEMENT_TWOD_ARRAYDECL: ;
            // we've already dealt with the child because we've already recursed.
            // nothing to do.
            break;
        case STATEMENT_PRINT: ;
            // we've already checked the statement, so we have nothing to do.
            // set the type of this node to the type of the child
            root->data.type = root->children[0]->data.type;
            break;
        case CLASSDECL: ;
            // we've already checked all the children and we've already checked
            // whether or not this is a new class, so we really just need to make
            // sure that we're extending a class that we're allowed to
            break;
        case ONED_ARRAY_ASSIGNMENT: ;
            // this is either "ID[Expr] = Expr;"
            // or "ID[Expr] = new Type[Expr];"
            if (root->numOfChildren == 2) { // ID[Expr] = Expr;
                // need to make sure the expression on the lhs is an int expression
                if (strcmp(root->children[0]->data.type, "int") != 0) {
                    // the index on the left isn't an int, so this is an error
                    //printf("TYPEERROR: type mismatch for array index on %d\n", root->data.lineNum);
                    //numErrors++;
                }

                // the expression on the rhs might be a regular variable, 
                // but it might be an array
                if ((root->children[1]->nodeType == EXPRESSION_PIECE) &&
                    (root->children[1]->children[0]->nodeType == ONED_ARRAY_REFERENCE)) {
                    // get the right thing out of the table
                    entry = get_sym_tab_entry(root->children[1]->children[0]->data.value.stringValue);

                    if (entry->array_dims == 2) {
                        // if the lhs is a 2d array, then we want to swap these rows
                        // in the 2d array
                    }
                    else {
                        // the thing on the right hand side is a oned array or 
                    }
                }
                else {
                    struct symbol_table_entry* entry = get_sym_tab_entry(root->data.value.stringValue);
                    if (entry != NULL) {
                        //printf("root->children[1]->data.type: %s\n", root->children[1]->data.type);
                        if (strcmp(entry->data.type, root->children[1]->data.type) != 0) {
                            // we're trying to assign a string to an int array, for example
                            //printf("TYPEERROR: type mismatch for assignment on %d\n", root->data.lineNum);
                            //numErrors++;
                        }
                    }
                }
            }
            else if (root->numOfChildren == 3) { // ID[Expr] = new Type[Expr];
                // make sure that the lhs index is an int
                if (strcmp(root->children[0]->data.type, "int") != 0) {
                    //printf("TYPEERROR: type mismatch for array index on %d\n", root->data.lineNum);
                    //numErrors++;
                }

                // make sure the type on the rhs is a valid one
                if (!is_valid_type(root->children[1]->data.type)) {
                    //printf("TYPEERROR: invalid type for assignment on %d\n", root->data.lineNum);
                    //numErrors++;
                }

                // make sure the types are the same
                // get the type of the variable out of the table
                entry = get_sym_tab_entry(root->data.value.stringValue);

                if (strcmp(entry->data.type, root->children[1]->data.type) != 0) {
                    // these are different types, so there is an error
                    //printf("TYPEERROR: type mismatch for assignment on %d\n", root->data.lineNum);
                    //numErrors++;
                }

                // make sure the expression on the rhs is an int expression
                if (strcmp(root->children[2]->data.type, "int") != 0) {
                    //printf("TYPEERROR: type mismatch for array index on %d\n", root->data.lineNum);
                    //numErrors++;
                }
            }

            break;
        case INT_TYPE: ;
            root->data.type = "int";
            break;
        case EXPRESSION_NEGATED: ;
            // this has only one child, the expression.
            // we need to check the expression, but we've already done that above.
            // now we just need to make sure the expression is a boolean or string
            // expression so that we are allowed to negate it
            if ((strcmp(root->children[0]->data.type, "int") == 0) || 
                (strcmp(root->children[0]->data.type, "boolean") == 0)) {
                // we're good
            }
            else {
                //printf("TYPEERROR: type mismatch for arguments to expression on %d\n", root->data.lineNum);
                //numErrors++;
            }
            
            break;
        case VARDECLLISTPIECE: ;
            // we've already called this function on each of the children,
            // so the children have the right types. what we need to do is,
            // if this is "ID = Expr", make sure that the expression has the right type
            if (root->numOfChildren == 1) { // ID = Expr
                if (root->children[0]->nodeType == VARDECLLISTPIECE_ID_EQ_EXPR) {
                }
                else if (strcmp(current_vardecl_list_type, root->children[0]->data.type) != 0) {
                    //printf("TYPEERROR: type mismatch for assignment on %d\n", root->data.lineNum);
                    //numErrors++;
                }
                
            }
            break;
        case TWOD_ARRAY_ASSIGNMENT: ;
            // there are a couple different types
            if (root->numOfChildren == 3) {
                // make sure the type on the rhs is the same
                // as the type on the lhs
                entry = get_sym_tab_entry(root->data.value.stringValue);

                // make sure the entry has the same type as child 3
                if (strcmp(entry->data.type, root->children[2]->data.type) != 0) {
                    //printf("TYPEERROR: type mismatch for assignment on %d\n", root->data.lineNum);
                    //numErrors++;
                }
            }
            else if (root->numOfChildren == 5) {
                // this is allowed in the grammar but not when we actually do things
                //printf("TYPEERROR: type mismatch for assignment on %d\n", root->data.lineNum);
                //numErrors++;
            }
            else if (root->numOfChildren == 4) {
                // this is allowed in the grammar but not when we actually do things
                //printf("TYPEERROR: type mismatch for assignment on %d\n", root->data.lineNum);
                //numErrors++;
            }
            
            break;
        case FORMAL: ;
            // this is in the form: Type ID
            // we don't have to typecheck this because we just trust that 
            // the person who wrote the method knows what they're doing

            break;
        case FORMAL_ONED_ARRAY: ;
            // we don't have to typecheck this for the same reason
            break;
        case FORMAL_TWOD_ARRAY: ;
            // we don't have to typecheck this for the same reason
            break;
        case STATEMENT_RETURN: ;
            // there are a couple types of return statements
            if (root->numOfChildren == 1) {
                // this statement returns something
                // nothing to do
            }
            else if (root->numOfChildren == 0) {
                // this is just a "return;" statement
                // nothing to do.

            }

            break;
        case VARDECLLISTPIECE_ID_EQ_NEW:
            break;
        default:
            //printf("case: %s\n", get_type_name(root->nodeType));

            if (root->nodeType == VARDECL) {
                if ((root->numOfChildren == 1)) {
                    // we don't have anything to do for this
                    return;
                }
                else {
                    if (root->children[1]->data.type == NULL) {
                        root->children[1]->data.type = "undefined";
                    }
                }
            }

            // if either of the types is undefined, set the type of the parent to undefined
            // and don't report any errors
            if ((strcmp(root->children[0]->data.type, "undefined") == 0) || 
                (strcmp(root->children[1]->data.type, "undefined") == 0)) {
                root->data.type = "undefined";
                return;
            }
            
            // if this is a variable declaration and the left side is a boolean, we need
            // to allow the right side (an expression) to be a boolean expression, which
            // might contain ints, so we have to check this manually
            if ((root->nodeType == VARDECL) && 
                (strcmp(root->children[0]->data.type, "boolean") == 0)) {
                // we are assigning something to a boolean. only allow the rhs
                // to be an expression if it's a logical expression 

                if ((root->children[1]->nodeType == EXPRESSION_EQUAL_EQUAL) || 
                    (root->children[1]->nodeType == EXPRESSION_NOT_EQUAL) || 
                    (root->children[1]->nodeType == EXPRESSION_GREATER_EQUAL) || 
                    (root->children[1]->nodeType == EXPRESSION_LESS_EQUAL) || 
                    (root->children[1]->nodeType == EXPRESSION_GREATER) || 
                    (root->children[1]->nodeType == EXPRESSION_LESS) || 
                    (root->children[1]->nodeType == EXPRESSION_OR) || 
                    (root->children[1]->nodeType == EXPRESSION_AND)) {
                    // logical expression on rhs. allowed

                }
                else if ((root->children[1]->nodeType == EXPRESSION_PIECE)) {
                    if ((strcmp(root->children[1]->children[0]->data.type, "boolean") == 0)) {
                        // the rhs is just a boolean value, which is allowed
                    }
                    else if (root->children[1]->children[0]->nodeType == ID_) {
                        // get the ID from the table and figure out it's type
                        entry = get_sym_tab_entry(root->children[1]->children[0]->data.value.stringValue);

                        // make sure that the type on the left is the same as the type 
                        // on the right
                        if (strcmp(entry->data.type, "boolean") != 0) {
                            //printf("TYPEERROR: type mismatch for declaration on %d\n", root->data.lineNum);
                            //numErrors++;
                        }
                    }
                }
                else {
                    // not logical expression. not allowed
                    //printf("TYPEERROR: type mismatch for declaration on %d\n", root->data.lineNum);
                    //numErrors++;
                }

            }
            else if ((root->nodeType == VARDECL)) {
                // if we're assigning to something that's not a boolean,
                // we need to make sure that the type on the rhs is the same
                // as the type on the lhs
                if (strcmp(root->children[0]->data.type, root->children[1]->data.type) != 0) {
                    //printf("TYPEERROR: type mismatch for declaration on %d\n", root->data.lineNum);
                    //numErrors++;
                }
            }
            // otherwise, if the types don't match, throw a type mismatch error
            // and set the type of the parent to undefined
            else if (strcmp(root->children[0]->data.type, root->children[1]->data.type) != 0) {

                // depending on the type of error, we want to print a personalized
                // message so that I (me) can better ensure i'm passing test cases
                // correctly
                switch (root->nodeType) {
                    case EXPRESSION_MULTIPLY: // fall through
                    case EXPRESSION_DIVIDE: // fall through 
                    case EXPRESSION_SUBTRACT: // fall through
                    case EXPRESSION_ADD: 
                                    //printf("TYPEERROR: type mismatch for operands to expression on %d\n", root->data.lineNum);
                                    //numErrors++;
                                    break;
                    case VARDECL:
                                    //printf("TYPEERROR: type mismatch for declaration on %d\n", root->data.lineNum);
                                    //numErrors++;
                                    break;
                    case EXPRESSION_EQUAL_EQUAL:
                                    //printf("TYPEERROR: type mismatch for operands to conditional on %d\n", root->data.lineNum);
                                    //numErrors++;
                                    break;
                    case EXPRESSION_NOT_EQUAL: 
                                    //printf("TYPEERROR: type mismatch for operands to conditional on %d\n", root->data.lineNum);
                                    //numErrors++;
                                    break;
                    case EXPRESSION_GREATER_EQUAL:
                                    //printf("TYPEERROR: type mismatch for operands to conditional on %d\n", root->data.lineNum);
                                    //numErrors++;
                                    break;
                    case EXPRESSION_LESS_EQUAL:
                                    //printf("TYPEERROR: type mismatch for operands to conditional on %d\n", root->data.lineNum);
                                    //numErrors++;
                                    break;
                    case EXPRESSION_GREATER:
                                    //printf("TYPEERROR: type mismatch for operands to conditional on %d\n", root->data.lineNum);
                                    //numErrors++;
                                    break;
                    case EXPRESSION_LESS:
                                    //printf("TYPEERROR: type mismatch for operands to conditional on %d\n", root->data.lineNum);
                                    //numErrors++;
                                    break;
                    case EXPRESSION_AND:
                                    //printf("TYPEERROR: type mismatch for operands to conditional on %d\n", root->data.lineNum);
                                    //numErrors++;
                                    break;
                    case EXPRESSION_OR:
                                    //printf("TYPEERROR: type mismatch for operands to conditional on %d\n", root->data.lineNum);
                                    //numErrors++;
                                    break;

                }
            }
            // if the children's types DO match, then we need to make sure they are
            // the right type for the operation that we're requesting. if they
            // are, then just assign the type of the first child to the parent
            // to the type of the children
            else {
                switch (root->nodeType) {
                    case EXPRESSION_EQUAL_EQUAL: ;
                    case EXPRESSION_NOT_EQUAL: ;
                    case EXPRESSION_GREATER_EQUAL: ;
                    case EXPRESSION_LESS_EQUAL: ;
                    case EXPRESSION_GREATER: ;
                    case EXPRESSION_LESS: ;
                    case EXPRESSION_AND: ;
                    case EXPRESSION_OR: ;
                        root->data.type = "boolean";
                        break;
                    case EXPRESSION_ADD: ;
                                            if (strcmp(root->children[0]->data.type, "boolean") == 0) { // boolean
                                                // can't do + on booleans, so error
                                                //printf("TYPEERROR: invalid types for expression on %d\n", root->data.lineNum);
                                                //numErrors++;
                                            }
                                            else {
                                                root->data.type = strdup(root->children[0]->data.type);
                                            }
                   
                                            break;
                    case EXPRESSION_SUBTRACT: ;
                                            if ((strcmp(root->children[0]->data.type, "string") == 0) || // string
                                                (strcmp(root->children[0]->data.type, "boolean") == 0)) { // boolean
                                                // can't do - on strings or booleans, so error
                                                //printf("TYPEERROR: invalid types for expression on %d\n", root->data.lineNum);
                                                //numErrors++;
                                            }
                                            else {
                                                root->data.type = strdup(root->children[0]->data.type);
                                            }

                                            break;
                    case EXPRESSION_DIVIDE: ;
                                            if ((strcmp(root->children[0]->data.type, "string") == 0) || // string
                                                (strcmp(root->children[0]->data.type, "boolean") == 0)) { // boolean
                                                // can't do / on strings or booleans, so error
                                                //printf("TYPEERROR: invalid types for expression on %d\n", root->data.lineNum);
                                                //numErrors++;
                                            }
                                            else {
                                                root->data.type = strdup(root->children[0]->data.type);
                                            }
     
                                            break;
                    case EXPRESSION_MULTIPLY: ;
                                            if ((strcmp(root->children[0]->data.type, "string") == 0) || // string
                                                (strcmp(root->children[0]->data.type, "boolean") == 0)) { // boolean
                                                // can't do * on strings or booleans, so error
                                                //printf("TYPEERROR: invalid types for expression on %d\n", root->data.lineNum);
                                                //numErrors++;
                                            }
                                            else {
                                                root->data.type = strdup(root->children[0]->data.type);
                                            }
     
                                            break;
                    default:
                                            root->data.type = strdup(root->children[0]->data.type);
                                            break;
                }
            }
            break;
    }
}

//---------------------------------------------------------------------------------------------------
//- misc functions ----------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

/* 
 * returns true if the type that's given as type_name is "int", "boolean", "string",
 * or any of the types defined by new class definitions
 */
bool is_valid_type(char* type_name) {
    if ((strcmp(type_name, "int") == 0) || 
        (strcmp(type_name, "boolean") == 0) ||
        (strcmp(type_name, "string") == 0)) {
        return true;
    }
    else {
        // it's either an invalid type name or it's the name of a class.
        // we can test if it's a class by trying to find that class
        struct symbol_table* table = NULL;
        table = find_table(type_name, global);
        if (table == NULL) {
            // we didn't find the type 
            return false;
        }
        else {
            // we did find the type name, so that's a valid type
            return true;
        }
    }
}

/*
 * returns the entry in the current symbol table denoted by ID.
 * does not check whether that entry is a class variable or not
 */
struct symbol_table_entry* get_sym_tab_entry(char* id) {
    struct symbol_table* table = current_table;

    for (int i = 0; i < table->numEntries; i++) {
        if (strcmp(id, table->symbol_table[i]->id) == 0) {
            return table->symbol_table[i];
        }
    }

    // since we didn't find it, let's go up a level and see if we 
    // can find it there
    table = current_table->parent;
    for (int i = 0; i < table->numEntries; i++) {
        if (strcmp(id, table->symbol_table[i]->id) == 0) {
            return table->symbol_table[i];
        }
    }

    return NULL; // we didn't find it
}

/*
 * returns the symbol table denoted by the table_name
 */
struct symbol_table* find_table(char* table_name, struct symbol_table* current) {
    //print_symbol_tables(global);
    // if the table we're asking for is "main", then we really just want
    // the first child of the global table.
    if (strcmp(table_name, "main") == 0) {
        return global->children[0];
    }

    // otherwise, try to find the table
    while (true) {
        // check the current table
        //printf("trying to find: %s\n", table_name);
        //printf("found table name: %s\n", current->table_name);
        if (strcmp(current->table_name, table_name) == 0) {
            return current;
        }
        
        // if we haven't returned, then we need to check each
        // of the children
        //printf("this table has %d children. checking them\n", current->num_children);
        if (current->num_children == 0) {
            break;
        }

        struct symbol_table* ret = NULL;
        for (int i = 0; i < current->num_children; i++) {
            ret = find_table(table_name, current->children[i]);
            if (ret != NULL) {
                return ret;
            }
        }

        if (ret == NULL) {
            return NULL;
        }
    }

    //printf("returning NULL\n");
    return NULL;
}

/*
 * returns true if the ID exists in this scope (but doesn't check higher scopes)
 */
bool id_exists_in_scope(char* id, struct symbol_table* table) {
    for (int i = 0; i < table->numEntries; i++) {
        if (strcmp(id, table->symbol_table[i]->id) == 0) {
            return true;
        }
    }
    return false;
}

/*
 * returns true if the ID is present in the current class as a class variable.
 * returns false otherwise
 */
bool class_var_is_in_symbol_table(char* id, struct symbol_table* table) {
    struct symbol_table* current_table = table;

    while (true) {
        // check the current table
        for (int i = 0; i < current_table->numEntries; i++) {
            if (current_table->symbol_table[i]->is_class_var) {
                if (strcmp(id, current_table->symbol_table[i]->id) == 0) {
                    return true;
                }
            }
        }

        // now, if this table's parent is the global table, stop here. 
        // otherwise, move up
        if ((current_table->parent == NULL) || (current_table->parent->parent == NULL)) {
            break;
        }
        else {
            current_table = current_table->parent;
        }
    }

    return false;
}

/* 
 * returns true if the id is present in the symbol table provided or any of it's parents
 */
bool id_exists_in_scope_or_higher(char * id, struct symbol_table* table) {
    struct symbol_table* current_table = table;
    while (true) {
        // check the current table
        for (int i = 0; i < current_table->numEntries; i++) {
            if (strcmp(id, current_table->symbol_table[i]->id) == 0) {
                return true;
            }
        }

        // since we made it here, we know that we didn't find that entry.
        // so we should move on to the parent and check that
        if (current_table->parent != NULL) {
            current_table = current_table->parent;
        }
        else { 
            // the parent is NULL, so we know that this table is the global table.
            // since we've already checked everything, we have no other tables to check
            break;
        }
    }

    return false;
}

/* 
 * add to the symbol table 
 */
void add_to_symbol_table_tc(char* id, struct data data, int is_array, int is_class_var, int is_method, struct symbol_table* table, struct twod_arr* array, int array_dims) {
    // get some memory for the entry
    struct symbol_table_entry* entry = malloc(sizeof(struct symbol_table_entry));

    // fill out the fields for the entry
    entry->id = id;
    entry->data = data;
    entry->is_array = is_array;
    entry->is_method = is_method;
    entry->array_dims = array_dims;
    if (handling_class_declarations) {
        entry->is_class_var = 1;
    }
    else {
        entry->is_class_var = 0;
    }

    // deal with array fields if needed
    entry->is_array = is_array;
    entry->array = array;

    /* build the offset name for this variable */
    // if this is a formal argument, the offset will be a number
    // instead of an offset .equ thing
    if (strncmp(id, "__formal_", strlen("__formal_")) == 0) {
        // we have to know how many formal arguments are already in this
        // table though
        int args = 0;
        for (int i = 0; i < table->numEntries; i++) {
            if (strncmp(table->symbol_table[i]->id, "__formal_", strlen("__formal_")) == 0) {
                args++;
            }
        }

        // now that we know how many args have already been put in the table,
        // we can figure out what number we should give as the offset name
        int offset_num = args * 4;

        // allocate a string that's the right length
        int offset_length;
        if (offset_num <= 8) {
            offset_length = 1;
        }
        else {
            // can only be 12
            offset_length = 2;
        }

        // build the offset name
        entry->offset_name = malloc(sizeof(char) * (offset_length + 1));
        snprintf(entry->offset_name, offset_length + 1, "%d", offset_num);
        entry->offset_name[offset_length] = '\0';
        
    }
    else {
        if (entry->is_class_var) {
            // if this is a class variable, then we need the offset to 
            // include the class name
            // get the class name from the table that we're adding to
            char* class_name = table->table_name;
            entry->offset_name = malloc(sizeof(char) * (strlen(class_name) + strlen(id) + strlen("_") + strlen("_offset") + 1));
            memcpy(entry->offset_name, class_name, strlen(class_name));
            entry->offset_name[strlen(class_name)] = '_';
            memcpy(entry->offset_name + strlen(class_name) + 1, id, strlen(id));
            memcpy(entry->offset_name + strlen(class_name) + strlen("_") + strlen(id), "_offset", strlen("_offset"));
            entry->offset_name[strlen(class_name) + strlen(id) + 1 + strlen("_offset")] = '\0';

        }
        else {
            // get the memory that we need
            entry->offset_name = malloc(sizeof(char) * (strlen(id) + strlen("_offset") + 1));
            memcpy(entry->offset_name, id, strlen(id));
            memcpy(entry->offset_name + strlen(id), "_offset\0", strlen("_offset\0"));
        }
    }

    // add the entry to the table
    table->symbol_table[table->numEntries] = entry;
    table->numEntries++;
}

/*
 * convert a type number to a type name for printing
 */
char* get_type_name(int type_number) {
    switch (type_number) {
        case 0: return "PROGRAM";
        case 1: return "MAINCLASS";
        case 2: return "STATEMENTLIST_STATEMENTLIST_STATEMENT";
        case 3: return "STATEMENTLIST_STATEMENT";
        case 4: return "VARDECL";
        case 5: return "INT_TYPE";
        case 6: return "BOOLEAN_TYPE";
        case 7: return "STRING_TYPE";
        case 8: return "STATEMENT_VARDECL";
        case 9: return "STATEMENT_PRINTLN";
        case 10: return "STATEMENT_PRINT";
        case 11: return "STATEMENT_ASSIGNMENT";
        case 12: return "ASSIGNMENT";
        case 13: return "EXPRESSION_MULTIPLY";
        case 14: return "EXPRESSION_DIVIDE";
        case 15: return "EXPRESSION_ADD";
        case 16: return "EXPRESSION_SUBTRACT";
        case 17: return "EXPRESSION_PIECE";
        case 18: return "INTEGER_LIT";
        case 19: return "STRING_LIT";
        case 20: return "BOOLEAN_LIT";
        case 21: return "ID_";
        case 22: return "STATEMENT";
        case 23: return "VARDECL_NO_RHS";
        case 24: return "EXPRESSION_PARENTHESES";
        case 25: return "NEG_EXPRESSION_PIECE";
        case 26: return "VARDECL_LIST";
        case 27: return "VARDECLLISTPIECELIST_VARDECLLISTPIECE";
        case 28: return "VARDECLLISTPIECE";
        case 29: return "VARDECLLISTPIECE_ID_EQ_EXPR";
        case 30: return "VARDECLLISTPIECE_ID";
        case 31: return "STATEMENT_IF_ELSE";
        case 32: return "EXPRESSION_EQUAL_EQUAL";
        case 33: return "EXPRESSION_NOT_EQUAL";
        case 34: return "EXPRESSION_GREATER_EQUAL";
        case 35: return "EXPRESSION_LESS_EQUAL";
        case 36: return "EXPRESSION_GREATER";
        case 37: return "EXPRESSION_LESS";
        case 38: return "EXPRESSION_AND";
        case 39: return "EXPRESSION_OR";
        case 40: return "STATEMENT_WHILE";
        case 41: return "STATEMENT_ONED_ARRAYDECL";
        case 42: return "STATEMENT_TWOD_ARRAYDECL";
        case 43: return "ONED_ARRAYDECL";
        case 44: return "TWOD_ARRAYDECL";
        case 45: return "STATEMENT_ONED_ARRAY_ASSIGNMENT";
        case 46: return "STATEMENT_TWOD_ARRAY_ASSIGNMENT";
        case 47: return "ONED_ARRAY_ASSIGNMENT";
        case 48: return "TWOD_ARRAY_ASSIGNMENT";
        case 49: return "ONED_ARRAY_REFERENCE"; 
        case 50: return "TWOD_ARRAY_REFERENCE"; 
        case 51: return "EXPRESSION_PIECE_DOT_LENGTH";
        case 52: return "EXPRESSION_NEGATED";
        case 53: return "CLASSDECLLIST_CLASSDECLLIST_CLASSDECL";
        case 54: return "CLASSDECLLIST_CLASSDECL";
        case 55: return "STATEMENT_VARDECL_LIST";
        case 56: return "STATEMENTLIST_WRAPPED";
        case 57: return "METHODDECLLIST_METHODDECLLIST_METHODDECL"; 
        case 58: return "METHODDECLLIST_METHODDECL"; 
        case 59: return "METHODDECL"; 
        case 60: return "FORMALLIST_FORMALLIST_FORMAL"; 
        case 61: return "FORMALLIST_FORMAL"; 
        case 62: return "FORMAL"; 
        case 63: return "CLASSDECL"; 
        case 64: return "CLASS_VARIABLE"; 
        case 65: return "STATEMENT_RETURN"; 
        case 66: return "ID_TYPE"; 
        case 67: return "METHODCALL"; 
        case 68: return "STATEMENT_METHODCALL"; 
        case 69: return "VARDECL_NEW_RHS"; 
        case 70: return "FORMAL_ONED_ARRAY"; 
        case 71: return "FORMAL_TWOD_ARRAY"; 
        case 72: return "STATEMENT_IF_NO_ELSE"; 
        case 73: return "VARDECLLISTPIECE_ID_EQ_NEW"; 
        case 74: return "ARGLIST_ARGLIST_ARG"; 
        case 75: return "ARGLIST_ARG"; 
        case 76: return "ARG"; 
        case 77: return "METHODDECL_ONED"; 
        case 78: return "METHODDECL_TWOD"; 
        case 79: return "STATEMENTLIST_STATEMENTLIST_SUB"; 
        default:
            return "INVALID";
    }
}

/*
 * get the name of the datatype from the integer value
 */
char* get_datatype_name(struct node* type) {
    switch (type->nodeType) {
        case INT_TYPE: return "int";
        case BOOLEAN_TYPE: return "boolean";
        case STRING_TYPE: return "string";
        case ID_TYPE: return type->data.value.stringValue;
        default:
            return "ERROR: get_datatype_name: invalid switch case";
    }
}

/* 
 * print the thing (according to printf format) if debug is on
 */
void debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (DEBUG) {
        printf(fmt, args);
    }
    return;
}

/* 
 * print the AST so that we can debug our AST creation
 */
void printAST(struct node * root, int depth, int child_number) {
	for (int i = 0; i < depth; i++) {
		printf("| ");
	}
	
	if (root == NULL) {
		printf("NULL\n");
		return;
	}

	printf("%d: %s, %s\n", child_number, get_type_name(root->nodeType), root->data.type);
	for (int i = 0; i < root->numOfChildren; i++) {
		printAST(root->children[i], depth + 1, i);
	}	

	return;
}

/*
 * print the symbol tables
 */
void print_symbol_tables(struct symbol_table* table) {
    // first, print the contents of the current table

    printf("Table: %s\n", table->table_name);
	printf("numEntries: %d\n", table->numEntries);
	for (int i = 0; i < table->numEntries; i++) {
        if (table->symbol_table[i]->is_method) {
            printf("METHOD: ");
        }
        else if (table->symbol_table[i]->is_class_var) {
            printf("CLASS_VAR: ");
        }

		printf("%s, type: %s", table->symbol_table[i]->id, table->symbol_table[i]->data.type);
        if (strcmp(table->symbol_table[i]->data.type, "int") == 0) {
            printf(", value: %d", table->symbol_table[i]->data.value.intValue);
        }
        else if (strcmp(table->symbol_table[i]->data.type, "boolean") == 0) {
            printf(", value: ");
            if (table->symbol_table[i]->data.value.booleanValue == true) {
                printf("true");
            }
            else {
                printf("false");
            }
        }
        else if(strcmp(table->symbol_table[i]->data.type, "undefined") == 0) {
            printf(", value: N/A");
        }
        else {
            printf(", value: N/A");
        }
		printf(", lineNum: %d", table->symbol_table[i]->data.lineNum);
        printf(", offset_name: %s\n", table->symbol_table[i]->offset_name);
	}

    // now, recurse on each of the children
    for (int i = 0; i < table->num_children; i++) {
        print_symbol_tables(table->children[i]);
    }
}
