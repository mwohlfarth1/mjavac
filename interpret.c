#include "interpret.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

struct symbol_table* global;
void *return_value;
int num_temp_vars = 0; // the number of temporary variables that we've needed
                       // so far
int num_labels = 0; // the number of labels that we have had to autogenerate so that
                    // they don't conflict with other labels
char* additional_string_literal_instructions[1000]; // any instructions of the type "label: .asciz ''"
int num_addition_string_literal_instructions = 0; // the number of string literal instructions
char* argv_name;

/* 
 * handles passing the control flow eventually to interpretStatement
 */
void interpret_program(struct node * program, struct symbol_table* g, char* input_name){
    global = g; // the global symbol table

    struct asm_node* main;

    // we've already created the symbol table, so we don't have to
    // we've also already added the argv to the table, so we don't have to
    if (program->numOfChildren == 2) { // classdecllist follows the mainclass
        struct asm_node* class_decl_list = interpret_class_decl_list(program->children[1]);

        // we need to interpret everything that's in the main class
        // NOTE: the mainclass node knows what the name of the class is,
        // so we don't have to worry about telling it
        main = interpret_main_method(program->children[0]);
    
        // we now have main and the list of methods from the various classes.
        // cat them all together
        main = cat_asm_lists(main, class_decl_list);
    }
    else {
        // we need to interpret everything that's in the main class
        // NOTE: the mainclass node knows what the name of the class is,
        // so we don't have to worry about telling it
        main = interpret_main_method(program->children[0]);
    }

    // now main contains the main method and any other methods which it might reference.
    // we need to add things to the top of this file. 
    // the things that we need to add to the top of the file are:
        // any string literals
        // format strings for println and print

    // create a list of nodes for the format strings and other stuff that goes at the top
    struct asm_node* node1;
    struct asm_node* node2;
    struct asm_node* node3;
    struct asm_node* node4;
    char* instr1;
    char* instr2;
    char* instr3;

    node1 = make_asm_node(".section .text");
    node2 = make_asm_node("println_int_fmt: .asciz \"%d\\n\"");
    node3 = make_asm_node("println_true_fmt: .asciz \"true\\n\"");
    node4 = make_asm_node("println_false_fmt: .asciz \"false\\n\"");

    // string these nodes together
    node1 = cat_asm_lists(node1, node2);
    node1 = cat_asm_lists(node1, node3);
    node1 = cat_asm_lists(node1, node4);

    // add more formats
    node2 = make_asm_node("println_str_fmt: .asciz \"%s\\n\"");
    node3 = make_asm_node("print_int_fmt: .asciz \"%d\"");
    node4 = make_asm_node("print_true_fmt: .asciz \"true\"");

    // string these together
    node1 = cat_asm_lists(node1, node2);
    node1 = cat_asm_lists(node1, node3);
    node1 = cat_asm_lists(node1, node4);

    // add more formats
    node2 = make_asm_node("print_false_fmt: .asciz \"false\"");
    node3 = make_asm_node("print_str_fmt: .asciz \"%s\"");

    // string these together
    node1 = cat_asm_lists(node1, node2);
    node1 = cat_asm_lists(node1, node3);

    // add all the formats that were defined in other functions
    for (int i = 0; i < num_addition_string_literal_instructions; i++) {
        node2 = make_asm_node(additional_string_literal_instructions[i]);
        node1 = cat_asm_lists(node1, node2);
    }

    // add on main
    node1 = cat_asm_lists(node1, main);

    // now, we need to add things to the end of this file in the .data section
    // we need to add:
        // the .section .data label
        // the offsets for the various things that we have

    // create the .section .data label
    instr3 = ".section .data";
    node3 = make_asm_node(instr3);

    // add a newline to the top of this so the source file is more
    // readable
    node3 = add_comment(node3, "");

    // get a list of nodes which is the offset data for all the methods
    struct asm_node* offsets = get_all_equ_offsets();

    // cat these lists together
    node1 = cat_asm_lists(node1, node3);
    node1 = cat_asm_lists(node1, offsets);

    // create a file to print the result to
    char* output_name = strdup(input_name);
    output_name[strlen(output_name) - 4] = 's';
    output_name[strlen(output_name) - 3] = '\0';

    FILE* out = fopen(output_name, "w");
    if (out == NULL) {
        printf("ERROR: fopen returned NULL\n");
        return;
    }

    // print the result to that file
    print_asm_list(node1, out);

}

/* 
 * interpret the statements that make up the main method
 */
struct asm_node* interpret_main_method(struct node* main_method) {
    char* instr1;
    char* instr2;
    char* instr3;
    char* instr4;
    char* instr5;
    struct asm_node* node1;
    struct asm_node* node2;
    struct asm_node* node3;
    struct asm_node* node4;
    struct asm_node* node5;

    // main method has an argv, so let's record what the name of the argv is
    argv_name = strdup(main_method->data.argv_name);

    // a mainclass either has a statementlist or doesn't
    if (main_method->numOfChildren == 0) { // don't have a statementlist
        // do nothing because we don't have to
    }
    else if (main_method->numOfChildren == 1) { // have a statementlist
        // get code which will execute the statement list
        struct asm_node* instr_list = interpret_statement_list(main_method->children[0], global->children[0]->table_name);

        // generate code which goes at the beginning of main
        node1 = make_asm_node(".global main");
        node2 = make_asm_node(".balign 4");
        node3 = make_asm_node("main:");
        node4 = make_asm_node("push {LR}");
        node5 = make_asm_node(build_stack_alloc_for_method("main"));
        char* alloc_instr = node5->instruction;

        // cat these nodes together
        node1 = cat_asm_lists(node1, node2);
        node1 = cat_asm_lists(node1, node3);
        node1 = cat_asm_lists(node1, node4);
        node1 = cat_asm_lists(node1, node5);

        // now that we've made the list, we can use node2-4 again.
        // we need to save the base address of the argv and stuff
        node2 = make_asm_node("sub r0, r0, #1");
        node3 = make_asm_node("add r1, r1, #4");
        node4 = make_asm_node("str r0, [SP, #0]");
        node5 = make_asm_node("str r1, [SP, #4]");

        node1 = cat_asm_lists(node1, node2);
        node1 = cat_asm_lists(node1, node3);
        node1 = cat_asm_lists(node1, node4);
        node1 = cat_asm_lists(node1, node5);

        // make the stack fixer node
        node2 = make_asm_node(build_stack_dealloc_for_method(alloc_instr));

        // make the pop node
        node3 = make_asm_node("pop {PC}");

        // add a note to the end so we know we're exiting main
        node2 = add_comment(node2, "\n@ exiting main");

        // build all of main, which is node1 followed by instr_list followed by the pop
        node1 = cat_asm_lists(node1, instr_list);
        node1 = cat_asm_lists(node1, node2);
        node1 = cat_asm_lists(node1, node3);

        // now that we have all of main built, we need to return the list of instructions
        // to the caller.
        // before we do that, add a newline so that we get better separation
        // in our code so it's more readable
        node1 = add_comment(node1, "");

        return node1;
    }
}

/* 
 * call interpret_statement() for each of the statements in the list
 */
struct asm_node* interpret_statement_list(struct node* statementlist, char* table_name) {
    struct asm_node* lhs_node;
    struct asm_node* rhs_node;

    if (statementlist->nodeType == STATEMENTLIST_STATEMENTLIST_STATEMENT) {
        // the lhs child is a statementlist
        lhs_node = interpret_statement_list(statementlist->children[0], table_name);

        // the rhs child is a statement
        rhs_node = interpret_statement(statementlist->children[1], table_name);

        // now we have lists of commands for both children. just concatenate them
        lhs_node = cat_asm_lists(lhs_node, rhs_node);

        return lhs_node;
    }
    else if (statementlist->nodeType == STATEMENTLIST_STATEMENT) {
        // the only child is a statement
        lhs_node = interpret_statement(statementlist->children[0], table_name);
        return lhs_node;
    }
    else if (statementlist->nodeType == STATEMENTLIST_WRAPPED) {
        // the only child is a statementlist
        lhs_node = interpret_statement_list(statementlist->children[0], table_name);
        return lhs_node;
    }
}

/* 
 * call interpret_class_decl() for each of the classes
 */
struct asm_node* interpret_class_decl_list(struct node* classdecl_list) {
    struct asm_node* lhs;
    struct asm_node* rhs;

    // makes sure the argument is actuall a class declaration list
    if ((classdecl_list->nodeType != CLASSDECLLIST_CLASSDECLLIST_CLASSDECL) && 
        (classdecl_list->nodeType != CLASSDECLLIST_CLASSDECL))  {
        printf("ERROR: interpret_class_decl_list(): argument isn't a class decl list\n");
        return NULL;
    }

    // this is either a ClassDeclList ClassDecl or a ClassDecl
    if (classdecl_list->numOfChildren == 2) {
        // the left child is another classdecllist
        lhs = interpret_class_decl_list(classdecl_list->children[0]);

        // the right child is a classdecl
        rhs = interpret_classdecl(classdecl_list->children[1]);

        // cat these together 
        lhs = cat_asm_lists(lhs, rhs);
    }
    else if (classdecl_list->numOfChildren == 1) {
        // the only child is a classdecl
        lhs = interpret_classdecl(classdecl_list->children[0]);
    }

    return lhs;
}

/* 
 * build code for a class declaration (probably just all the methods)
 */
struct asm_node* interpret_classdecl(struct node* classdecl) {
    struct asm_node* node1;
    struct asm_node* node2;
    struct asm_node* node3;
    struct asm_node* node4;
    struct asm_node* node5;
    struct asm_node* node6;
    struct asm_node* node7;
    struct asm_node* node8;
    struct asm_node* node9;
    
    // we have a class declaration here. that can look many different ways:
        // CLASS ID { }
        // CLASS ID EXTENDS ID { }
        // CLASS ID { StatementList }
        // CLASS ID EXTENDS ID { StatementList }
        // CLASS ID { StatementList MethodDeclList }
        // CLASS ID EXTENDS ID { StatementList MethodDeclList }
        // CLASS ID { MethodDeclList }
        // CLASS ID EXTENDS ID { MethodDeclList }
    // and we want to do something different for each of these cases

    // we either have no children, one child, or two children
    switch (classdecl->numOfChildren) {
        case 0: ; // no children means: 
                    // CLASS ID {}
                    // CLASS ID EXTENDS ID {}
            if (classdecl->data.extendedClass == NULL) { // CLASS ID {}
                // we don't really care about a class that looks like CLASS ID {}
                // because it doesn't have any members or methods and it doesn't
                // extend another class so it doesn't inherit any methods or fields.
                // so someone could instantiate it, but that wouldn't do anything.
                // so we don't even need to process it since it's in the symbol table.
                // DO nothing

                // make an empty node and return that
                node1 = make_asm_node("");
                return node1;
            }
            else { // CLASS ID EXTENDS ID {}
                // this class has no members or fields, but it does inherit stuff
                // from another class. 
                // doesn't really matter though because there's no code to 
                // generate for this class. only for the class it inherits from,
                // which will get processed another time
                // DO nothing

                // make an empty node and return that
                node1 = make_asm_node("");
                return node1;
            }
            break;
        case 1: ; // either have a statementlist or a methoddecllist
            if ((classdecl->children[0]->nodeType == STATEMENTLIST_STATEMENTLIST_STATEMENT) || 
                (classdecl->children[0]->nodeType == STATEMENTLIST_STATEMENT)) {
                // we need to build a constructor with the statement list
                node1 = build_constructor(classdecl);

                return node1;
            }
            else if ((classdecl->children[0]->nodeType == METHODDECLLIST_METHODDECLLIST_METHODDECL) ||
                     (classdecl->children[0]->nodeType == METHODDECLLIST_METHODDECL)) {
                // first child is a method declaration list, so we want to generate code
                // for each of the method declarations in that list.
               
                // get the code for all the method declarations
                node1 = interpret_methoddecl_list(classdecl->children[0]);

                // add a comment
                node1 = add_comment(node1, "\n@ method declarations ===========");

                return node1;
            }

            break;
        case 2: ; // have both a statementlist and a methoddecllist
            // we don't care about the statementlist right now. it'll get dealt
            // with later
            node1 = build_constructor(classdecl);

            // get the code for the methods that are declared here though
            node2 = interpret_methoddecl_list(classdecl->children[1]);

            // add a comment
            node2 = add_comment(node2, "\n@ method declarations ==============");

            node1 = cat_asm_lists(node1, node2);
            return node1;

            break;
        default: ;
            printf("ERROR: interpret_classdecl(): invalid switch case\n");
            return NULL;
            break;
    }

    return NULL;
}

/* 
 * build a constructor for the class that we're dealing with
 */
struct asm_node* build_constructor(struct node* classdecl) {
    struct asm_node* node1;
    struct asm_node* node2;
    struct asm_node* node3;
    struct asm_node* node4;
    struct asm_node* node5;
    struct asm_node* node6;
    struct asm_node* node7;
    struct asm_node* node8;
    struct asm_node* node9;
    struct asm_node* statement_list;

    // build a symbol table for the constructor
    // push {LR}
    // copy all the entries from the other table into this one
    // get the stack alloc instruciton
    // for each of these variables, generate code which memcpy the data in

    // we need to get the name for the constructor so that we can
    // build the branch instruction properly
    char* class_name = classdecl->data.value.stringValue; 
    char* label = malloc(sizeof(char) * (strlen(class_name) + strlen("_constructor:") + 1));
    memcpy(label, class_name, strlen(class_name));
    memcpy(label + strlen(class_name), "_constructor:", strlen("_constructor:"));
    label[strlen(class_name) + strlen("_constructor:")] = '\0';

    // build a symbol table for this constructor
    struct symbol_table* parent = find_table(class_name, global);
    struct symbol_table* const_table = malloc(sizeof(struct symbol_table));
    const_table->parent = parent;
    const_table->num_children = 0;
    const_table->table_name = strdup(label);
    const_table->table_name[strlen(const_table->table_name) - 1] = '\0';
    const_table->numEntries = 0;
    parent->children[parent->num_children++] = const_table;

    // now that we have a symbol table, we can copy in the class variables from the other table
    int num_words_to_alloc = 0;
    for (int i = 0; i < parent->numEntries; i++) {
        if (parent->symbol_table[i]->is_class_var) {
            const_table->symbol_table[const_table->numEntries++] = parent->symbol_table[i];
            num_words_to_alloc++;
        }
    }

    // now that we've loaded the class variables in, we can generate code
    // which evaluates the statement list
    statement_list = interpret_statement_list(classdecl->children[0], const_table->table_name);

    // now that we've done this, we can generate the stack alloc instruction and 
    // start building things
    node1 = make_asm_node(label);
    node2 = make_asm_node("push {LR}");
    char* alloc_instr = build_stack_alloc_for_method(const_table->table_name);
    node3 = make_asm_node(alloc_instr);

    node1 = cat_asm_lists(node1, node2);
    node1 = cat_asm_lists(node1, node3);
    node1 = cat_asm_lists(node1, statement_list);

    // build the instructions which malloc the memory
    node2 = make_asm_node(build_mov_instruction(0, num_words_to_alloc * 4));
    node3 = make_asm_node("bl malloc");
    node2 = add_comment(node2, "\n@ allocate memory for this object");
    node1 = cat_asm_lists(node1, node2);
    node1 = cat_asm_lists(node1, node3);

    // for each of the variables, build a set of instructions which
    // moves the right data into that spot
    int first_time = 1;
    for (int i = 0; i < const_table->numEntries; i++) {
        if (const_table->symbol_table[i]->is_class_var) {
            // build all the instructions
            node2 = make_asm_node("mov r1, SP");

            // build the "add r1, r1, #var_offset" instruction
            char* add_instr = malloc(sizeof(char) * (strlen("add r1, r1, #") + strlen(const_table->symbol_table[i]->offset_name) + 1));
            memcpy(add_instr, "add r1, r1, #", strlen("add r1, r1, #"));
            memcpy(add_instr + strlen("add r1, r1, #"), const_table->symbol_table[i]->offset_name, strlen(const_table->symbol_table[i]->offset_name));
            add_instr[strlen("add r1, r1, #") + strlen(const_table->symbol_table[i]->offset_name)] = '\0';
            node3 = make_asm_node(add_instr);
            node4 = make_asm_node(build_mov_instruction(2, 4));
            node5 = make_asm_node("push {r0}");
            node6 = make_asm_node("bl memcpy");
            node7 = make_asm_node("pop {r0}");

            // instruction for moving r0 into r4 at the beginning
            node8 = make_asm_node("mov r4, r0");

            // instruction for moving the pointer over if it's the second time
            node9 = make_asm_node("add r0, r0, #4");


            if (first_time) {
                first_time = 0;

                node2 = cat_asm_lists(node2, node3);
                node2 = cat_asm_lists(node2, node8);
                node2 = cat_asm_lists(node2, node4);
                node2 = cat_asm_lists(node2, node5);
                node2 = cat_asm_lists(node2, node6);
                node2 = cat_asm_lists(node2, node7);
                node2 = add_comment(node2, "\n@ move data into object's heap location");

                node1 = cat_asm_lists(node1, node2);

            }
            else {
                node9 = cat_asm_lists(node9, node2);
                node9 = cat_asm_lists(node9, node3);
                node9 = cat_asm_lists(node9, node4);
                node9 = cat_asm_lists(node9, node5);
                node9 = cat_asm_lists(node9, node6);
                node9 = cat_asm_lists(node9, node7);
                node9 = add_comment(node9, "\n@ move data into object's heap location");

                node1 = cat_asm_lists(node1, node9);
            }
        }
    }

    // now that we have code that puts the right values into the data that we have,
    // we just need to pop the pointer to the data back into r0 and return
    node2 = make_asm_node("mov r0, r4");
    node2 = add_comment(node2, "\n@ place the base address for an object's data back into r0");

    node3 = make_asm_node(build_stack_dealloc_for_method(alloc_instr));
    node4 = make_asm_node("pop {PC}");
    node3 = add_comment(node3, "\n@ return from this constructor");

    node1 = cat_asm_lists(node1, node2);
    node1 = cat_asm_lists(node1, node3);
    node1 = cat_asm_lists(node1, node4);

    return node1;
}

/* 
 * generate code for all the methods in the list
 */
struct asm_node* interpret_methoddecl_list(struct node* methoddecl_list) {
    struct asm_node* node1;
    struct asm_node* node2;
    struct asm_node* node3;

    if (methoddecl_list->nodeType == METHODDECLLIST_METHODDECLLIST_METHODDECL) {
        // first child is another list. get that code
        node1 = interpret_methoddecl_list(methoddecl_list->children[0]);

        // second child is a method declaration. get that code
        node2 = interpret_methoddecl(methoddecl_list->children[1]);

        // cat these together
        node1 = cat_asm_lists(node1, node2);
    }
    else if (methoddecl_list->nodeType == METHODDECLLIST_METHODDECL) {
        // only child is a method declaration. get that code
        node1 = interpret_methoddecl(methoddecl_list->children[0]);
    }

    return node1;
}

/* 
 * generate code for a single method declaration
 */
struct asm_node* interpret_methoddecl(struct node* methoddecl) {
    struct asm_node* node1;
    struct asm_node* node2;
    struct asm_node* node3;
    struct asm_node* node4;
    struct asm_node* node5;
    struct asm_node* statementlist;

    // there are tons of different types of method declarations, but
    // we'll only worry about the ones that don't return arrays (since this is 3.1).
    // that means we only have these types:
        // Type ID () {}
        // Type ID ( FormalList ) {}
        // Type ID ( ) { StatementList }
        // Type ID ( FormalList) { StatementList }
        // PUBLIC Type ID () {}
        // PUBLIC Type ID ( FormalList ) {}
        // PUBLIC Type ID ( ) { StatementList }
        // PUBLIC Type ID ( FormalList) { StatementList }
    // luckily, since we don't care whether a method is public or not, we
    // can forget about trying to differentiate half of these. so we really
    // only have four types:
        // Type ID () {}
        // Type ID ( FormalList ) {}
        // Type ID ( ) { StatementList }
        // Type ID ( FormalList) { StatementList }
    // and we can differentiate between these by number of children

    switch (methoddecl->numOfChildren) {
        case 1: ; // Type ID () {}
            // this function returns something but doesn't have code
            // to return something. so let's just assume there won't be 
            // any test cases for this
            break;
        case 2: ; // Type ID ( FormalList ) {} OR
                  // Type ID ( ) { StatementList } 
            // figure out if we have a formallist or a statementlist
            if ((methoddecl->children[1]->nodeType == FORMALLIST_FORMALLIST_FORMAL) || 
                (methoddecl->children[1]->nodeType == FORMALLIST_FORMAL)) {
                // it's a formallist. this means we don't have a statementlist 
                // to return the value that we're going to return. so let's
                // just assume this won't show up in the test cases
            }
            else if ((methoddecl->children[1]->nodeType == STATEMENTLIST_STATEMENTLIST_STATEMENT) || 
                     (methoddecl->children[1]->nodeType == STATEMENTLIST_STATEMENT)) {
                // it's a statementlist. since this method has no formal arguments, 
                // we just need to generate code for the statements in the method
                // and put a label on top.
                
                // make a label for this method (using the class name and method_name)
                char* method_name = methoddecl->data.value.stringValue;
                struct asm_node* label = make_asm_node(build_method_label(get_class_name(method_name), method_name));

                // get code for the statementlist
                node1 = interpret_statement_list(methoddecl->children[1], method_name);

                // place a "push {LR}" onto the top
                node2 = make_asm_node("push {LR}");
                node3 = make_asm_node(build_stack_alloc_for_method(method_name));
                node2 = cat_asm_lists(node2, node3);
                node2 = cat_asm_lists(node2, node1);

                // slap the label on top
                label = cat_asm_lists(label, node2);

                // build a stack decalloc instruction
                node1 = make_asm_node(build_stack_dealloc_for_method(node3->instruction));

                // now, we need to add code that returns to the caller, which means
                // we just need to pop the LR into the PC (since we pushed it on
                // at the beginning of the method)
                node3 = make_asm_node("pop {PC}");

                // cat that onto the bottom
                label = cat_asm_lists(label, node1);
                label = cat_asm_lists(label, node3);

                // add a comment
                label = add_comment(label, "\n@ code generated for method =========");

                return label;
            }


            break;
        case 3: ; // Type ID (FormalList) { StatementList }
            // we want to set up our method so it looks like:
                // Classname_methodname:
                // push {LR}
                // sub SP, SP, #args * 4
                // save arguments into stack
                // code that evaluates the statementlist (including return value)
                // add SP, SP, #args * 4
                // pop {PC}

            // get a label for this method
            char* method_name = methoddecl->data.value.stringValue;
            struct asm_node* label = make_asm_node(build_method_label(get_class_name(method_name), method_name));

            // get the code for the statementlist before we do anything
            // so that the symbol table gets populated with the number of temp variables
            // that we need
            statementlist = interpret_statement_list(methoddecl->children[2], method_name);

            // make some nodes
            node1 = make_asm_node("push {LR}");
            node2 = make_asm_node(build_stack_alloc_for_method(method_name));
            char* stack_alloc_instr = strdup(node2->instruction);

            // cat these together so we can reuse things
            node1 = cat_asm_lists(node1, node2);

            // now generate the code which loads the arguments into the appropriate
            // stack locations. to do this, we need to know how many arguments there are.
            int num_args = count_arguments(method_name);
            switch (num_args) {
                case 1: ;
                    // just need one line of code
                    node2 = make_asm_node("str r0, [SP, #0]");
                    break;
                case 2: ; 
                    // just need two lines of code
                    node2 = make_asm_node("str r0, [SP, #0]");
                    node3 = make_asm_node("str r1, [SP, #4]");
                    node2 = cat_asm_lists(node2, node3);
                    break;
                case 3: ;
                    // just need 3 lines of code
                    node2 = make_asm_node("str r0, [SP, #0]");
                    node3 = make_asm_node("str r1, [SP, #4]");
                    node4 = make_asm_node("str r2, [SP, #8]");
                    node2 = cat_asm_lists(node2, node3);
                    node2 = cat_asm_lists(node2, node4);
                    break;
                case 4: ;
                    // just need 4 lines of code
                    node2 = make_asm_node("str r0, [SP, #0]");
                    node3 = make_asm_node("str r1, [SP, #4]");
                    node4 = make_asm_node("str r2, [SP, #8]");
                    node5 = make_asm_node("str r3, [SP, #12]");
                    node2 = cat_asm_lists(node2, node3);
                    node2 = cat_asm_lists(node2, node4);
                    node2 = cat_asm_lists(node2, node5);
                    break;
                default: ;
                    printf("ERROR: interpret_methoddecl(): invalid switch case\n");
                    return NULL;
                    break;
            }

            // node2 now holds the code which stores the arguments into the stack
            // cat that onto what we have so far
            node1 = cat_asm_lists(node1, node2);
            
            // now cat on the statements which evaluate things in the function
            node1 = cat_asm_lists(node1, statementlist);

            // pop the stack frame off
            node2 = make_asm_node(build_stack_dealloc_for_method(stack_alloc_instr));

            // pop the LR into PC
            node3 = make_asm_node("pop {PC}");

            // string everything together
            node1 = cat_asm_lists(node1, node2);
            node1 = cat_asm_lists(node1, node3);
            label = cat_asm_lists(label, node1);

            // add a comment
            label = add_comment(label, "\n@ code generated for method ========");

            return label;

            break;
        default: ;
            printf("ERROR: interpret_methoddecl(): invalid switch case\n");
            return NULL;
            break;
    }
}

/* 
 * interpret a statement using the symbol table
 */
struct asm_node* interpret_statement(struct node* statement, char* table_name) {
    // check arguments
    if ((statement == NULL) || (table_name == NULL)) {
        printf("ERROR: interpret_statement: argument is NULL\n");
    }

    char* to_print;
    struct asm_node* ans;
    char* instr1;
    char* instr2;
    char* instr3;
    char* temp_name;
    char* last_temp_var;
    char* then_label;
    char* end_label;
    struct asm_node* then;
    struct asm_node* node1;
    struct asm_node* node2;
    struct asm_node* node3;
    struct asm_node* node4;
    struct asm_node* node5;
    struct asm_node* node6;
    struct asm_node* node7;
    struct asm_node* node8;
    struct asm_node* node9;

    // each type of statement is handled differently
    switch (statement->nodeType) {
        case STATEMENT_PRINTLN: ; 
            // evaluate the expression to generate code which gets us ready to print
            // by storing the variable to be printed into a temp variable
            ans = evaluate_expression(statement->children[0], table_name);

            // now, we want to generate code which makes a printf call.

            // build an instruction which loads the correct format string
            if (statement->data.type == NULL) {
                if (statement->children[0]->nodeType == EXPRESSION_PIECE) {
                    if ((statement->children[0]->numOfChildren > 0) && 
                        (statement->children[0]->children[0]->nodeType == ONED_ARRAY_REFERENCE)) {
                        if (strcmp(statement->children[0]->children[0]->data.value.stringValue, argv_name) == 0) {
                            statement->data.type = "string";
                        }
                        else {
                            if ((statement->children[0]->children[0]->numOfChildren > 0) &&
                                (statement->children[0]->children[0]->children[0]->nodeType == EXPRESSION_PIECE)) {
                                statement->data.type = statement->children[0]->children[0]->children[0]->data.type;
                            }
                        }
                    }
                } 
            }

            if (strcmp(statement->data.type, "undefined") == 0) {
                if (strcmp(statement->children[0]->data.type, "undefined") != 0) {
                    // if this child doesn't have undefined type, carry the type
                    // up
                    statement->data.type = statement->children[0]->data.type;
                }
                else {
                    statement->data.type = "boolean"; 
                }
            }

            if (strcmp(statement->data.type, "int") == 0) {
                // generate code that loads the format string into r0
                node1 = make_asm_node("ldr r0, =println_int_fmt");

                // build an instruction that loads the variable's value into r1.
                node2 = make_asm_node(build_instruction("ldr", 1, get_last_temp_name()));

                // now build an instruction which branches to printf
                instr3 = "bl printf";
                node3 = make_asm_node(instr3);

                // now string these 3 instructions together
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);

                // add a comment so we know what this does
                node1 = add_comment(node1, "\n@ load an int and println it");

                // now, cat these 3 instructions onto the end of the code which
                // generates the value to print
                ans = cat_asm_lists(ans, node1);

                // now we can return the asm code which prints the value
                return ans;

            }
            else if (strcmp(statement->data.type, "boolean") == 0) {
                // generate code that loads the correct string to print ("true"/"false")
                // based on the value of that boolean. the boolean value is stored in the
                // last temp variable

                // build the instruction which loads the boolean value into r1
                node1 = make_asm_node(build_instruction("ldr", 1, get_last_temp_name()));

                // build the instruction which checks the truthiness of the boolean
                node2 = make_asm_node("cmp r1, #0");

                // we'll need a branch target for true
                char* true_branch = get_new_label();

                // build the branch instruction
                node3 = make_asm_node(build_branch_instruction("bne", true_branch));

                // string these together so we can reuse variables
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);

                // build the instruction which loads the "false\n"
                node2 = make_asm_node("ldr r0, =println_false_fmt");

                // build the instruction which branches to the print
                char* to_print = get_new_label();
                node3 = make_asm_node(build_branch_instruction("b", to_print));

                // string these together so we can reuse variables
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);

                // build the instruction which is the label for the true branch
                node2 = make_asm_node(true_branch);

                // build the instruction which loads the "true\n"
                node3 = make_asm_node("ldr r0, =println_true_fmt");

                // string these together so we can reuse variables
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);

                // build the instruction which is the print branch label
                node2 = make_asm_node(to_print);

                // since we have the correct format string in r0, we can
                // just make the node which branches to printf
                node3 = make_asm_node("bl printf");

                // string these together
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);

                // add a label so that we know what this is
                node1 = add_comment(node1, "\n@ load and println a boolean");

                // now that we have this, we can cat it onto the code which 
                // determined what the value of the boolean is
                ans = cat_asm_lists(ans, node1);

                return ans;
            }
            else if (strcmp(statement->data.type, "string") == 0) {
                // we have a string variable to print. this string variable
                // is on the stack and holds a pointer to the string
                // that we want to print.
                // find that variable's offset so that we can load the address into a register
                // generate code that loads the format string into r0


                // generate instr to put the format string in r0
                node1 = make_asm_node("ldr r0, =println_str_fmt");
                
                // generate instr to put the address of the string we want into r1
                node2 = make_asm_node(build_instruction("ldr", 1, get_last_temp_name()));

                // generate code to branch to printf
                node3 = make_asm_node("bl printf");

                // string these together
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);

                // add a comment so we know what we're doing
                node1 = add_comment(node1, "\n@ load and println a string");

                // now we need to cat this on the code that actually finds that string value
                ans = cat_asm_lists(ans, node1);

                return ans;
            }
            
            break;
        case STATEMENT_PRINT: ; 
            // evaluate the expression to generate code which gets us ready to print
            // by storing the variable to be printed into a temp variable
            ans = evaluate_expression(statement->children[0], table_name);

            if (statement->data.type == NULL) {
                if (statement->children[0]->nodeType == EXPRESSION_PIECE) {
                    if ((statement->children[0]->numOfChildren > 0) && 
                        (statement->children[0]->children[0]->nodeType == ONED_ARRAY_REFERENCE)) {
                        if (strcmp(statement->children[0]->children[0]->data.value.stringValue, argv_name) == 0) {
                            statement->data.type = "string";
                        }
                        else {
                            if ((statement->children[0]->children[0]->numOfChildren > 0) &&
                                (statement->children[0]->children[0]->children[0]->nodeType == EXPRESSION_PIECE)) {
                                statement->data.type = statement->children[0]->children[0]->children[0]->data.type;
                            }
                        }
                    }
                } 
            }
            if ((statement->data.type, "undefined") == 0) {
                if (strcmp(statement->children[0]->data.type, "undefined") != 0) {
                    statement->data.type = statement->children[0]->data.type;
                }
                else {
                    statement->data.type = "boolean";
                }
            }

            // now, we want to generate code which makes a printf call.
			// build an instruction which loads the correct format string
            if (strcmp(statement->data.type, "int") == 0) {
                // generate code that loads the format string into r0
                node1 = make_asm_node("ldr r0, =print_int_fmt");

                // build an instruction that loads the variable's value into r1.
                node2 = make_asm_node(build_instruction("ldr", 1, get_last_temp_name()));

                // now build an instruction which branches to printf
                instr3 = "bl printf";
                node3 = make_asm_node(instr3);

                // now string these 3 instructions together
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);

                // add a comment so we know what this does
                node1 = add_comment(node1, "\n@ load an int and print it");

                // now, cat these 3 instructions onto the end of the code which
                // generates the value to print
                ans = cat_asm_lists(ans, node1);

                // now we can return the asm code which prints the value
                return ans;

            }
            else if (strcmp(statement->data.type, "boolean") == 0) {
                // generate code that loads the correct string to print ("true"/"false")
                // based on the value of that boolean. the boolean value is stored in the
                // last temp variable

                // build the instruction which loads the boolean value into r1
                node1 = make_asm_node(build_instruction("ldr", 1, get_last_temp_name()));

                // build the instruction which checks the truthiness of the boolean
                node2 = make_asm_node("cmp r1, #0");

                // we'll need a branch target for true
                char* true_branch = get_new_label();

                // build the branch instruction
                node3 = make_asm_node(build_branch_instruction("bne", true_branch));

                // string these together so we can reuse variables
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);

                // build the instruction which loads the "false\n"
                node2 = make_asm_node("ldr r0, =print_false_fmt");

                // build the instruction which branches to the print
                char* to_print = get_new_label();
                node3 = make_asm_node(build_branch_instruction("b", to_print));

                // string these together so we can reuse variables
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);

                // build the instruction which is the label for the true branch
                node2 = make_asm_node(true_branch);

                // build the instruction which loads the "true\n"
                node3 = make_asm_node("ldr r0, =print_true_fmt");

                // string these together so we can reuse variables
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);

                // build the instruction which is the print branch label
                node2 = make_asm_node(to_print);

                // since we have the correct format string in r0, we can
                // just make the node which branches to printf
                node3 = make_asm_node("bl printf");

                // string these together
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);

                // add a label so that we know what this is
                node1 = add_comment(node1, "\n@ load and print a boolean");

                // now that we have this, we can cat it onto the code which 
                // determined what the value of the boolean is
                ans = cat_asm_lists(ans, node1);

                return ans;
            }
            else if (strcmp(statement->data.type, "string") == 0) {
                // we have a string variable to print. this string variable
                // is on the stack and holds a pointer to the string
                // that we want to print.
                // find that variable's offset so that we can load the address into a register
                // generate code that loads the format string into r0


                // generate instr to put the format string in r0
                node1 = make_asm_node("ldr r0, =print_str_fmt");
                
                // generate instr to put the address of the string we want into r1
                node2 = make_asm_node(build_instruction("ldr", 1, get_last_temp_name()));

                // generate code to branch to printf
                node3 = make_asm_node("bl printf");

                // string these together
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);

                // add a comment so we know what we're doing
                node1 = add_comment(node1, "\n@ load and print a string");

                // now we need to cat this on the code that actually finds that string value
                ans = cat_asm_lists(ans, node1);

                return ans;
            }

            break;
        case STATEMENT_ASSIGNMENT: ; 
            ans = interpret_assignment(statement->children[0], table_name);
            return ans;
            break;
        case STATEMENT_IF_ELSE: ; 
            // first, get code that evaluates the boolean expression and places
            // the value in a temp variable
            ans = evaluate_expression(statement->children[0], table_name);

            // get the last temp var so we know where to find the value
            // of the boolean expression
            last_temp_var = get_last_temp_name();

            // generate code for the then block
            then = interpret_statement_list(statement->children[1], table_name);

            // generate code for the else block
            struct asm_node* _else = interpret_statement_list(statement->children[2], table_name);

            // we need to set everything up like this:
                // ans_code
                // ldr r0, [SP, last_temp_var]
                // mov r1, #1
                // cmp r0, r1
                // beq _then_label
                // _else code
                // b _end_label
                // _then_label: 
                // then_code
                // _end_label: 
            
            // to do this, we need two labels
            then_label = get_new_label();
            end_label = get_new_label();
            
            // now we can actually generate the code
            node1 = make_asm_node(build_instruction("ldr", 0, last_temp_var));
            node2 = make_asm_node(build_mov_instruction(1, 1));
            node3 = make_asm_node("cmp r0, r1");
            node4 = make_asm_node(build_branch_instruction("beq", then_label));
            node5 = make_asm_node(build_branch_instruction("b", end_label));
            node6 = make_asm_node(then_label);
            node7 = make_asm_node(end_label);

            // now arrange everything in the way it's supposed to be
            node1 = cat_asm_lists(node1, node2);
            node1 = cat_asm_lists(node1, node3);
            node1 = cat_asm_lists(node1, node4);
            node1 = cat_asm_lists(node1, _else);
            node1 = cat_asm_lists(node1, node5);
            node1 = cat_asm_lists(node1, node6);
            node1 = cat_asm_lists(node1, then);
            node1 = cat_asm_lists(node1, node7);

            // add a comment
            node1 = add_comment(node1, "\n@ check result of boolean expression in temp var and evaluate if/then/else");

            // cat this on the code which evaluates the boolean
            ans = cat_asm_lists(ans, node1);

            return ans;
            break;
        case STATEMENT_IF_NO_ELSE: ; 
            // first, get code that evaluates the boolean expression and places
            // the value in a temp variable
            ans = evaluate_expression(statement->children[0], table_name);

            // get the last temp var so we know where to find the value
            // of the boolean expression
            last_temp_var = get_last_temp_name();

            // generate code for the then block
            then = interpret_statement_list(statement->children[1], table_name);

            // can't generate code for the else block because there isn't one

            // we need to set everything up like this:
                // ans_code
                // ldr r0, [SP, last_temp_var]
                // mov r1, #0 @ false
                // cmp r0, r1
                // beq _end_label @ skip the then block if expression is false
                // then_code
                // _end_label: 
            
            // to do this, we need an end label
            end_label = get_new_label();
            
            // now we can actually generate the code
            node1 = make_asm_node(build_instruction("ldr", 0, last_temp_var));
            node2 = make_asm_node(build_mov_instruction(1, 0));
            node3 = make_asm_node("cmp r0, r1");
            node4 = make_asm_node(build_branch_instruction("beq", end_label));
            node5 = make_asm_node(end_label);

            // now arrange everything in the way it's supposed to be
            node1 = cat_asm_lists(node1, node2);
            node1 = cat_asm_lists(node1, node3);
            node1 = cat_asm_lists(node1, node4);
            node1 = cat_asm_lists(node1, then);
            node1 = cat_asm_lists(node1, node5);

            // add a comment
            node1 = add_comment(node1, "\n@ check result of boolean expression in temp var and evaluate if/then/no_else");

            // cat this on the code which evaluates the boolean
            ans = cat_asm_lists(ans, node1);

            return ans;

            break;
        case STATEMENT_WHILE: ; 
            // we want code that looks like the following:
                // _loop_start:
                // code_to_evaluate_boolean_and_store_in_temp_var
                // ldr r0, [SP, last_temp_var]
                // mov r1, #0
                // beq _end
                // code for the loop body
                // b _loop_start
                // _end:

            // generate code to evlaluate the boolean and store val in temp var
            ans = evaluate_expression(statement->children[0], table_name);

            // get the last temp name
            last_temp_var = get_last_temp_name();

            // generate code for the loop body
            then = interpret_statement_list(statement->children[1], table_name);

            // we'll need two labels for this
            char* loop_start_label = get_new_label();
            end_label = get_new_label();

            // now we can generate the code
            node1 = make_asm_node(loop_start_label);
            // code to evaluate boolean goes here
            node2 = make_asm_node(build_instruction("ldr", 0, last_temp_var));
            node3 = make_asm_node(build_mov_instruction(1, 0));
            node4 = make_asm_node("cmp r0, r1");
            node5 = make_asm_node(build_branch_instruction("beq", end_label));
            // code for the loop body goes here
            node6 = make_asm_node(build_branch_instruction("b", loop_start_label));
            node7 = make_asm_node(end_label);

            // put everything together
            node1 = cat_asm_lists(node1, ans);
            node1 = cat_asm_lists(node1, node2);
            node1 = cat_asm_lists(node1, node3);
            node1 = cat_asm_lists(node1, node4);
            node1 = cat_asm_lists(node1, node5);
            node1 = cat_asm_lists(node1, then);
            node1 = cat_asm_lists(node1, node6);
            node1 = cat_asm_lists(node1, node7);

            // add a comment
            node1 = add_comment(node1, "\n@ while loop");

            return node1;
            break;
        case STATEMENT_ONED_ARRAY_ASSIGNMENT: ; 
            ans = interpret_oned_array_assignment(statement->children[0], table_name);
            return ans;
            break;
        case STATEMENT_TWOD_ARRAY_ASSIGNMENT: ; 
            interpret_twod_array_assignment(statement->children[0], table_name);
            break;
        case STATEMENT_RETURN: ; 
            // there are two types of return statements:
                // return Expr;
                // return;
            if (statement->numOfChildren == 1) { // return Expr;
                // first, evaluate the expression so we know what to return
                ans = evaluate_expression(statement->children[0], table_name);

                // get the offset name for the thing we want to return
                last_temp_var = get_last_temp_name();

                // since we're returning from a method, just place
                // that return value into r0.
                node1 = make_asm_node(build_instruction("ldr", 0, last_temp_var));

                // add a comment
                node1 = add_comment(node1, "\n@ load a return value into r0");

                // cat these together
                ans = cat_asm_lists(ans, node1);

                return ans;
            }
            else if (statement->numOfChildren == 0) { // return;
                // do nothing because we don't have to return anything
                // return an empty node
                node1 = make_asm_node("");

                return node1;
            }
            break;
        case STATEMENT_METHODCALL: ;
            // we just need to evaluate the method call
            node1 = interpret_methodcall(statement->children[0], table_name);
            return node1;
            break;
        case STATEMENT_VARDECL: ; 
            // get the list of asm commands that represents the vardecl
            ans = interpret_vardecl(statement->children[0], table_name);

            // return that list of asm commands to the parent
            return ans;
            break;
        case STATEMENT_VARDECL_LIST: ; 
            interpret_vardecl_list(statement->children[0], table_name);
            break;
        case STATEMENT_ONED_ARRAYDECL: ; 
            ans = interpret_oned_arraydecl(statement->children[0], table_name);
            return ans;
            break;
        case STATEMENT_TWOD_ARRAYDECL: ; 
            interpret_twod_arraydecl(statement->children[0], table_name);
            break;
        default:
            printf("ERROR: interpret_statement: invalid switch case\n");
            break;
    }
}

/* 
 * interpret an assignment statement
 */
struct asm_node* interpret_assignment(struct node* assignment, char* table_name) {
    struct asm_node* ans;
    struct symbol_table_entry* entry;
    char* name;
    struct asm_node* node1;
    struct asm_node* node2;
    struct asm_node* node3;
    struct asm_node* node4;
    struct asm_node* node5;
    struct asm_node* node6;
    struct asm_node* node7;

    // an assignment can come in many different forms
    if (assignment->numOfChildren == 0) { // this is "ID = new ID();"
        // we need to call the constructor for this object, but we'd need
        // to know the name for the constructor. build it
        char* class_name = assignment->data.className;
        char* constructor_name = malloc(sizeof(char) * (strlen(class_name) + strlen("_constructor:") + 1));
        memcpy(constructor_name, class_name, strlen(class_name));
        memcpy(constructor_name + strlen(class_name), "_constructor:", strlen("_constructor:"));
        constructor_name[strlen(class_name) + strlen("_constructor:") + 1] = '\0';

        // we'll also need to know the offset for the variable that we're supposed
        // to be assigning to
        char* var_name = assignment->data.value.stringValue;
        char* var_offset_name = get_symbol_tab_entry(var_name, table_name)->offset_name;

        // now we can build what we need
        node1 = make_asm_node(build_branch_instruction("bl", constructor_name));
        node2 = make_asm_node(build_instruction("str", 0, var_offset_name));

        node1 = cat_asm_lists(node1, node2);
        node1 = add_comment(node1, "\n@ call constructor and save results in correct location");

        return node1;


    }
    else if (assignment->numOfChildren == 1) { // this is "ID = Expr;" or
                                               // "ID.ID = Expr;" or
                                               // "this.ID = Expr;"

        if (assignment->data.className == NULL) { // "ID = Expr;" 
            name = assignment->data.value.stringValue;

            // generate code which finds and loads the value of the stuff
            // on the rhs into a temp var
            ans = evaluate_expression(assignment->children[0], table_name);

            // find the offset name for the variable we're assigning to
            char* offset_name = get_offset_name(name, table_name);

            // also, find where that value is stored
            char* value_name = get_last_temp_name();

            // generate code to load the value 
            struct asm_node* load_value = make_asm_node(build_instruction("ldr", 0, value_name));
            
            // generate code to store that value into the variable's stack location
            struct asm_node* store_value = make_asm_node(build_instruction("str", 0, offset_name));

            // add comment so we know what this does
            load_value = add_comment(load_value, "\n@ load a value and assign it to a variable's stack location");

            // string all of these together
            ans = cat_asm_lists(ans, load_value);
            ans = cat_asm_lists(ans, store_value);

            return ans;


        }
        else if (strcmp(assignment->data.className, "this") == 0) { // "this.ID = Expr;"
            // get the expression's value
            ans = evaluate_expression(assignment->children[0], table_name);
        }
        else { // "ID.ID = Expr;"
            // get the code which determines the value on the rhs and puts into temp var
            ans = evaluate_expression(assignment->children[0], table_name);
            char* rhs_name = get_last_temp_name();

            // get the offset for the lhs variable
            char* lhs_offset = get_offset_name(assignment->data.className, table_name);

            // get the offset for the field. unfortunately, the offset for the field
            // is in the constructor's symbol table, so we need to find that
            // constructor's name
            char* class_name = get_symbol_tab_entry(assignment->data.className, table_name)->data.type;
            char* constructor_name = malloc(sizeof(char) * (strlen(class_name) + strlen("_constructor") + 1));
            memcpy(constructor_name, class_name, strlen(class_name));
            memcpy(constructor_name + strlen(class_name), "_constructor", strlen("_constructor"));
            constructor_name[strlen(class_name) + strlen("_constructor")] = '\0';

            // figure out the field name
            char* field_name = assignment->data.value.stringValue;

            // get the constructor's symbol table
            struct symbol_table* const_table = find_table(constructor_name, global);
            if (const_table == NULL) {
                print_symbol_tables(global);
                printf("error\n");
            }

            char* field_offset_name;
            for (int i = 0; i < const_table->numEntries; i++) {
                if (strcmp(const_table->symbol_table[i]->id, field_name) == 0) {
                    field_offset_name = strdup(const_table->symbol_table[i]->offset_name);
                    break;
                }
            }

            // we want to generate code like this:
                // ldr r0, [SP, lhs_variable]
                // add r0, r0, #field_offset
                // ldr r1, [SP, #rhs_name]
                // str r1, [r0, #0]

            node1 = make_asm_node(build_instruction("ldr", 0, lhs_offset));
            char* instr2 = malloc(sizeof(char) * (strlen("add r0, r0, #") + strlen(field_offset_name) + 1));
            memcpy(instr2, "add r0, r0, #", strlen("add r0, r0, #"));
            memcpy(instr2 + strlen("add r0, r0, #"), field_offset_name, strlen(field_offset_name));
            instr2[strlen("add r0, r0, #") + strlen(field_offset_name)] = '\0';
            node2 = make_asm_node(instr2);
            node3 = make_asm_node(build_instruction("ldr", 1, rhs_name));
            node4 = make_asm_node("str r1, [r0, #0]");
            node1 = cat_asm_lists(node1, node2);
            node1 = cat_asm_lists(node1, node3);
            node1 = cat_asm_lists(node1, node4);
            node1 = add_comment(node1, "\n@ write value into var's class variable");

            ans = cat_asm_lists(ans, node1);
            return ans;
        }
    }
    else if (assignment->numOfChildren == 2) { // this is "ID = new Type[Expr];" 
        // get code that evaluates the index and places in temp var
        struct asm_node* index = evaluate_expression(assignment->children[1], table_name);
        char* index_temp_var = get_last_temp_name();

        // now that we know what the value of the index is, 
        // we want to allocate enough space for each of the things.
        // we could allocate a byte for each boolean, but we really
        // just want to allocate 4 to make things easier
        int bytes_per_element = 4;

        // we need to generate code like this:
            // ldr r0, [SP, #index_temp_var]
            // mul r0, r0, #4
            // bl malloc
            // str r0, [SP, #lhs_offset]

        // get the offset for the lhs variable
        char* lhs_offset = get_offset_name(assignment->data.value.stringValue, table_name);

        node1 = make_asm_node(build_instruction("ldr", 0, index_temp_var));
        node2 = make_asm_node(build_mov_instruction(1, 4));
        node3 = make_asm_node("mul r0, r0, r1");
        node4 = make_asm_node("bl malloc");
        node5 = make_asm_node(build_instruction("str", 0, lhs_offset));

        node1 = cat_asm_lists(node1, node2);
        node1 = cat_asm_lists(node1, node3);
        node1 = cat_asm_lists(node1, node4);
        node1 = cat_asm_lists(node1, node5);
        node1 = add_comment(node1, "\n@ allocate memory for an array and assign that to a variable");

        index = cat_asm_lists(index, node1);

        return index;

    }
    else if (assignment->numOfChildren == 3) { // this is "ID = new Type[Expr][Expr];" 
        // get the value of the first index
        ans = evaluate_expression(assignment->children[1], table_name);
        /*
        int first_index = ans.value.intValue;

        // get the value of the second index
        ans = evaluate_expression(assignment->children[2], table_name);
        int second_index = ans.value.intValue;

        // allocate a fresh 2d array
        struct twod_arr* twod = malloc(sizeof(struct twod_arr));
        twod->type = get_datatype_name(assignment->children[0]);
        twod->num_arrays = first_index;
        twod->dims = 2;

        // now allocate a new oned array for each array and assign it to the twod array
        for (int i = 0; i < first_index; i++) {
            // allocate a new oned
            struct oned_arr* oned = malloc(sizeof(struct oned_arr));
            oned->type = twod->type;
            oned->length = second_index;
            if (strcmp(oned->type, "int") == 0) {
                oned->array = malloc(sizeof(int) * second_index);
            }
            else if (strcmp(oned->type, "boolean") == 0) {
                oned->array = malloc(sizeof(bool) * second_index);
            }
            else if (strcmp(oned->type, "string") == 0) {
                oned->array = malloc(sizeof(char) * second_index);
            }

            // assign that oned to the twod
            twod->oned_arrays[i] = oned;
        }

        // now that we've created the twod array, we can replace the current one
        // with this one.
        // get the array from the table
        entry = get_symbol_tab_entry(assignment->data.value.stringValue, table_name);

        // replace the twod array
        entry->array = twod;
        */
    }
}

/* 
 * interpret a method call
 */
struct asm_node* interpret_methodcall(struct node* methodcall, char* table_name) { 

    struct asm_node* node1;
    struct asm_node* node2;
    struct asm_node* node3;
    struct asm_node* node4;
    struct asm_node* node5;
    struct asm_node* node6;
    struct asm_node* node7;
    struct asm_node* node8;
    struct asm_node* node9;
    char* temp_name;
    char* method_label;

    // in my grammar, all methodcalls come from expression pieces and have another
    // methodcall under them
    if (methodcall->numOfChildren > 0) {
        if (methodcall->children[0]->nodeType == METHODCALL) {
            // if the child is a methodcall, we have something like this:
                // NEW ID ( ) . MethodCall OR
                // ID . MethodCall

            // figure out the name of the method that we want to call
            char* method_to_call = methodcall->children[0]->data.value.stringValue;

            // for now, just assume that no two classes will have the same method names,
            // so we can tell what method to call just based on the method name that
            // we have. we don't need to know the type of the variable
            switch (methodcall->children[0]->numOfChildren) {
                case 0: ;
                    // if this has no children, then there's no argument list.
                    // that means we don't have to load anything into registers
                    // before we call that function. we just have to generate
                    // code that jumps to the function body and then
                    // reads the return value into a temp variable so that whoever
                    // needs this can get at the return value.
                    // in order to do that, we need to know the label name for
                    // the method
                    method_label = build_method_label(get_class_name(method_to_call), method_to_call);

                    // build something that looks like: 
                        // b method_label
                        // str r0, [SP, temp_var]

                    // get a temp variable name so that we have a place to store this
                    temp_name = get_temp_name(table_name);

                    node1 = make_asm_node(build_branch_instruction("bl", method_label));
                    node2 = make_asm_node(build_instruction("str", 0, temp_name));

                    // string these together
                    node1 = cat_asm_lists(node1, node2);

                    // add a comment 
                    node1 = add_comment(node1, "\n@ call method with no arguments");

                    return node1;
                    break;
                case 1: ; 
                    // if this has a child, there's an argument list.

                    // we need to figure out how many arguments there are,
                    // load them into r0-r4, branch to the method,
                    // then read the return value out of r0 into a temp var

                    // the first child of this methodcall should be an argument list

                    // keep track of the 4 temp var names and the code which is generated 
                    // to find their values
                    char* temps[4];
                    struct asm_node* temp_nodes[4];

                    // just to make things easier
                    struct node* root = methodcall->children[0];

                    // get all the code and the temp names for the (up to) 4 arguments
                    struct node* current = root->children[0];
                    int num_args = 0;
                    while (current->nodeType == ARGLIST_ARGLIST_ARG) {

                        // the right child is an argument
                        temp_nodes[num_args] = evaluate_expression(current->children[1]->children[0], table_name);
                        temps[num_args] = get_last_temp_name();
                        num_args++;

                        // the left child is an arglist, so we should
                        // set that to be the next current
                        current = current->children[0];
                    }
                    // the current node is now not an ARGLIST_ARGLIST_ARG,
                    // so it was either an ARGLIST_ARG to begin with or it is now
                    temp_nodes[num_args] = evaluate_expression(current->children[0]->children[0], table_name);
                    temps[num_args] = get_last_temp_name();
                    num_args++;

                    // now we have code generated for up to 4 arguments which puts them
                    // in temp variables on our stack, and names that we can use to reference
                    // those. lets build code that loads those arguments into registers
                    switch (num_args) {
                        case 1: ;
                            node1 = make_asm_node(build_instruction("ldr", 0, temps[0]));
                            break;
                        case 2: ; 
                            node1 = make_asm_node(build_instruction("ldr", 0, temps[0]));
                            node2 = make_asm_node(build_instruction("ldr", 1, temps[1]));
                            node1 = cat_asm_lists(node1, node2);

                            temp_nodes[0] = cat_asm_lists(temp_nodes[0], temp_nodes[1]);
                            break;
                        case 3: ; 
                            node1 = make_asm_node(build_instruction("ldr", 0, temps[0]));
                            node2 = make_asm_node(build_instruction("ldr", 1, temps[1]));
                            node3 = make_asm_node(build_instruction("ldr", 2, temps[2]));
                            node1 = cat_asm_lists(node1, node2);
                            node1 = cat_asm_lists(node1, node3);

                            temp_nodes[0] = cat_asm_lists(temp_nodes[0], temp_nodes[1]);
                            temp_nodes[0] = cat_asm_lists(temp_nodes[0], temp_nodes[2]);
                            break;
                        case 4: ; 
                            node1 = make_asm_node(build_instruction("ldr", 0, temps[0]));
                            node2 = make_asm_node(build_instruction("ldr", 1, temps[1]));
                            node3 = make_asm_node(build_instruction("ldr", 2, temps[2]));
                            node4 = make_asm_node(build_instruction("ldr", 3, temps[3]));
                            node1 = cat_asm_lists(node1, node2);
                            node1 = cat_asm_lists(node1, node3);
                            node1 = cat_asm_lists(node1, node4);

                            temp_nodes[0] = cat_asm_lists(temp_nodes[0], temp_nodes[1]);
                            temp_nodes[0] = cat_asm_lists(temp_nodes[0], temp_nodes[2]);
                            temp_nodes[0] = cat_asm_lists(temp_nodes[0], temp_nodes[3]);
                            break;
                        default: ;
                            printf("ERROR: interpret_methodcall(): invalid switch case\n");
                            return NULL;
                            break;
                    }

                    // now, node1 holds those 4 instructions that will load things
                    // into r0-r4.
                    // and temp_nodes[0] holds the list of instructions that has 
                    // to be executed to evaluate the argument values.
                    node1 = add_comment(node1, "\n@ load arguments and call method");
                    temp_nodes[0] = cat_asm_lists(temp_nodes[0], node1);

                    // let's put a branch statement 
                    method_label = build_method_label(get_class_name(method_to_call), method_to_call);
                    node1 = make_asm_node(build_branch_instruction("bl", method_label));

                    // get a temp var so that we can store the return value
                    temp_name = get_temp_name(table_name);
                    node2 = make_asm_node(build_instruction("str", 0, temp_name));

                    temp_nodes[0] = cat_asm_lists(temp_nodes[0], node1);
                    temp_nodes[0] = cat_asm_lists(temp_nodes[0], node2);

                    return temp_nodes[0];
                    break;
                default: ;
                    printf("ERROR: interpret_methodcall(): invalid switch case\n");
                    return NULL;
            }
        }
        else {
        }
    }
    struct asm_node* ret = make_asm_node("");
    return ret;

}

/* 
 * interpret a variable declaration
 */
struct asm_node* interpret_vardecl(struct node* vardecl, char* table_name) { 
    struct asm_node* expression_commands;
    char* name;
    struct symbol_table_entry* entry;
    struct asm_node* node1;
    struct asm_node* node2;
    struct asm_node* node3;
    struct asm_node* node4;
    struct asm_node* node5;

    // a vardecl can come in several different forms
    if (vardecl->numOfChildren == 2) { // Type ID = Expr;
        name = vardecl->data.value.stringValue;

        // get the asm commands that will evaluate the expression
        expression_commands = evaluate_expression(vardecl->children[1], table_name);
        if (expression_commands == NULL) {
            printf("ERROR: interpret_vardecl(): asm list for rhs expression is NULL\n");
            return NULL;
        }

        // we want to generate code that looks like this:
            // ldr r0, [SP, #lhs_var_offset] @ fetch the lhs variable
            // ldr r1, [SP, #temp_var_offset] @ fetch the rhs value
            // str r1, [SP, #lhs_var_offset] @ store r1 (the rhs value) in the
            //                               @ lhs_var_offset location
        // we already know what the temp_var_offset is, because it's the
        // last temp var that was created
        char* temp_name = get_last_temp_name();
        
        // figure out the name we'll use for the lhs variable's offset
        char* lhs_name = vardecl->data.value.stringValue;
        char* lhs_var_offset = get_offset_name(lhs_name, table_name);

        // now that we know the name of the temp variable and 
        // the offset for the lhs var, we can actually generate the code.

        // generate code for "ldr r0, [SP, #_tempn_offset]"
        char* instr1 = build_instruction("ldr", 0, temp_name);
        struct asm_node* node1 = make_asm_node(instr1);

        // generate code for "str r0, [SP, #lhs_var_offset]"
        char* instr2 = build_instruction("str", 0, lhs_var_offset);
        struct asm_node* node2 = make_asm_node(instr2);

        // now that we've generated code for these 3 instructions, string them
        // together
        node1 = cat_asm_lists(node1, node2);

        // add a comment so we know what this code does
        node1 = add_comment(node1, "\n@ write a value to a variable's stack location");

        // now, put this list (headed at node1) at the back of the instruction
        // list
        expression_commands = cat_asm_lists(expression_commands, node1);

        // return the list of commands to the caller (someone who is dealing
        // with a statement list or statement
        return expression_commands;
    }
    else if (vardecl->numOfChildren == 1) { // Type ID; or
                                            // Type ID = new ID();
        if (vardecl->data.className != NULL) { // Type ID = new ID(); 
            // we need to call the constructor for this object, but we'd need
            // to know the name for the constructor. build it
            char* class_name = vardecl->data.className;
            char* constructor_name = malloc(sizeof(char) * (strlen(class_name) + strlen("_constructor:") + 1));
            memcpy(constructor_name, class_name, strlen(class_name));
            memcpy(constructor_name + strlen(class_name), "_constructor:", strlen("_constructor:"));
            constructor_name[strlen(class_name) + strlen("_constructor:") + 1] = '\0';

            // we'll also need to know the offset for the variable that we're supposed
            // to be assigning to
            char* var_name = vardecl->data.value.stringValue;
            char* var_offset_name = get_symbol_tab_entry(var_name, table_name)->offset_name;

            // now we can build what we need
            node1 = make_asm_node(build_branch_instruction("bl", constructor_name));
            node2 = make_asm_node(build_instruction("str", 0, var_offset_name));

            node1 = cat_asm_lists(node1, node2);
            node1 = add_comment(node1, "\n@ call constructor and save results in correct location");

            return node1;

        }
        else { // Type ID; 
            // do nothing. when we deal with the symbol table for this method/class,
            // we'll record what the offset in the stack frame for this ID is, and
            // then we can reference that later. no need to do anything now
            
            // we need to return something so that the function cating all
            // the statements together has something to cat with
            expression_commands = make_asm_node("");
            return expression_commands;
        }
    }
}

// call interpret_vardecl_list_piece_list() to interpret the 
// list piece list
struct asm_node* interpret_vardecl_list(struct node* list, char* table_name) { 
    // this list has two children:
        // the type of the variables that are being defined
        // the list of pieces that make up the multidecl statement

    // since we've already typechecked this statement, the type of
    // the variable is already present in the table. therefore,
    // we can just deal with the list
    struct asm_node* ret = interpret_vardecl_list_piece_list(list->children[1], table_name);
    return ret;
}

/* 
 * call interpret_vardecl_list_piece() for all the pieces in the list
 */
struct asm_node* interpret_vardecl_list_piece_list(struct node* list, char* table_name) { 
    struct asm_node* lhs;
    struct asm_node* rhs;

    if (list->nodeType == VARDECLLISTPIECELIST_VARDECLLISTPIECE) {
        // left child is the list
        lhs = interpret_vardecl_list_piece_list(list->children[0], table_name);

        // right child is a single piece
        rhs = interpret_vardecl_list_piece(list->children[1], table_name);

        // cat these together and return them
        lhs = cat_asm_lists(lhs, rhs);
    }
    else if (list->nodeType == VARDECLLISTPIECE) {
        // only child is the list piece
        lhs = interpret_vardecl_list_piece(list->children[0], table_name);
    }

    return lhs;
}

/* 
 * interpret a piece of a multiple declaration list
 */
struct asm_node* interpret_vardecl_list_piece(struct node* piece, char* table_name) { 
    struct asm_node* ans;
    char* name;
    struct symbol_table_entry* entry;
    struct asm_node* node1;
    struct asm_node* node2;
    struct asm_node* node3;
    struct asm_node* node4;
    struct asm_node* node5;

    if (piece->numOfChildren == 1) { // ID = Expr
        // get the name of the variable that we want to assign to
        name = piece->data.value.stringValue;

        // figure out what the answer to the expression on the rhs is
        ans = evaluate_expression(piece->children[0], table_name);

        // get the offset for the variable that we want to change
        char* var_offset = get_offset_name(name, table_name);

        // now, we can generate code which stores the value in the variable's location
        node1 = make_asm_node(build_instruction("ldr", 0, get_last_temp_name()));
        node2 = make_asm_node(build_instruction("str", 0, var_offset));

        // string these together
        node1 = cat_asm_lists(node1, node2);

        // add a comment
        node1 = add_comment(node1, "\n@ fetch expression result and place in variable's stack location");

        // cat this on the other thing
        ans = cat_asm_lists(ans, node1);

        return ans;
    }
    else if (piece->numOfChildren == 0) { // ID or ID = new ID()
        if (piece->data.className == NULL) { // ID
            // don't have to do anything because this ID is already
            // in the table
            // need to return something, though, so return an empty node
            ans = make_asm_node("");
            return ans;
        }
        else { // ID = new ID()
            char* class_name = piece->data.className;
            char* constructor_name = malloc(sizeof(char) * (strlen(class_name) + strlen("_constructor:") + 1));
            memcpy(constructor_name, class_name, strlen(class_name));
            memcpy(constructor_name + strlen(class_name), "_constructor:", strlen("_constructor:"));
            constructor_name[strlen(class_name) + strlen("_constructor:") + 1] = '\0';

            // we'll also need to know the offset for the variable that we're supposed
            // to be assigning to
            char* var_name = piece->data.value.stringValue;
            char* var_offset_name = get_symbol_tab_entry(var_name, table_name)->offset_name;

            // now we can build what we need
            node1 = make_asm_node(build_branch_instruction("bl", constructor_name));
            node2 = make_asm_node(build_instruction("str", 0, var_offset_name));

            node1 = cat_asm_lists(node1, node2);
            node1 = add_comment(node1, "\n@ call constructor and save results in correct location");


            return node1;
        }
    }
}

/* 
 * interpret a one dimensional array declaration
 */
struct asm_node* interpret_oned_arraydecl(struct node* arraydecl, char* table_name) { 
    struct asm_node* ans; 
    struct symbol_table_entry* entry;
    struct asm_node* index;

    struct asm_node* node1;
    struct asm_node* node2;
    struct asm_node* node3;
    struct asm_node* node4;
    struct asm_node* node5;
    struct asm_node* node6;
    struct asm_node* node7;
    struct asm_node* node8;
    struct asm_node* node9;
    
    // there are different types of array declarations
    if (arraydecl->numOfChildren == 3) { // Type[] ID = new Type[Expr];
        // first, evaluate the expression to get code which calculates the index
        // and places the value in a temp variable
        index = evaluate_expression(arraydecl->children[2], table_name);

        // get the name of the temp variable
        char* index_temp_var = get_last_temp_name();

        // now that we know what the value of the index is, 
        // we want to allocate enough space for each of the things.
        // we could allocate a byte for each boolean, but we really
        // just want to allocate 4 to make things easier
        int bytes_per_element = 4;

        // we need to generate code like this:
            // ldr r0, [SP, #index_temp_var]
            // mul r0, r0, #4
            // bl malloc
            // str r0, [SP, #lhs_offset]

        // get the offset for the lhs variable
        char* lhs_offset = get_offset_name(arraydecl->data.value.stringValue, table_name);

        node1 = make_asm_node(build_instruction("ldr", 0, index_temp_var));
        node2 = make_asm_node(build_mov_instruction(1, 4));
        node3 = make_asm_node("mul r0, r0, r1");
        node4 = make_asm_node("bl malloc");
        node5 = make_asm_node(build_instruction("str", 0, lhs_offset));

        node1 = cat_asm_lists(node1, node2);
        node1 = cat_asm_lists(node1, node3);
        node1 = cat_asm_lists(node1, node4);
        node1 = cat_asm_lists(node1, node5);
        node1 = add_comment(node1, "\n@ allocate memory for an array and assign that to a variable");

        index = cat_asm_lists(index, node1);

        return index;
    }
    else if (arraydecl->numOfChildren == 2) { // Type[] ID = Expr;
        // first, evaluate the expression
        ans = evaluate_expression(arraydecl->children[1], table_name);
    }
    else if (arraydecl->numOfChildren == 1) { // Type[] ID;
        // don't do anything because it's already in the table and we don't
        // have a value to update it with
        ans = make_asm_node("");
        return ans;
    }
}

/* 
 * interpret a 2d array declaration
 */
void interpret_twod_arraydecl(struct node* arraydecl, char* table_name) { 
    struct asm_node* ans;
    struct symbol_table_entry* entry;

    // there are many different types of 2d array declarations
    if (arraydecl->numOfChildren == 4) { // Type[][] ID = new Type[Expr][Expr];
        // figure out the value of the first index
        ans = evaluate_expression(arraydecl->children[2], table_name);
        /*
        int first_index = ans.value.intValue;

        // figure out the value of the second index
        ans = evaluate_expression(arraydecl->children[3], table_name);
        int second_index = ans.value.intValue;

        // allocate a 2d array and fill out the fields
        struct twod_arr* twod = malloc(sizeof(struct twod_arr));
        twod->type = get_datatype_name(arraydecl->children[0]);
        twod->dims = 2;
        twod->num_arrays = first_index;

        // allocate memory for each of the oned arrays and assign
        // them to the twod array
        for (int i = 0; i < first_index; i++) {
            struct oned_arr* oned = malloc(sizeof(struct oned_arr));
            oned->type = twod->type;
            oned->length = second_index;
            // allocate memory for the array that's in the oned_arr struct
            if (strcmp(oned->type, "int") == 0) {
                oned->array = malloc(sizeof(int) * second_index);
            }
            else if (strcmp(oned->type, "boolean") == 0) {
                oned->array = malloc(sizeof(bool) * second_index);
            }
            else {
                oned->array = malloc(sizeof(char) * second_index);
            }

            // assign that oned struct to the twod struct
            twod->oned_arrays[i] = oned;
        }

        // now that we've allocated memory for the 2d array and stuff, 
        // we can update the value in the symbol table with that array
        entry = get_symbol_tab_entry(arraydecl->data.value.stringValue, table_name);
        entry->array = twod;
        */
    }
    else if (arraydecl->numOfChildren == 2) { // Type[][] ID = Expr;
        // NOTE: this should never happen because this will throw a type error.
        // if it does, just print an error message so that we notice
        printf("ERROR: interpret_twod_arraydecl: trying to assign a regular value to a 2d array\n");
    }
    else if (arraydecl->numOfChildren == 3) { // Type[][] ID = new Type[Expr];
        // NOTE: this also shouldn't ever happen because this'll throw a type error
        printf("ERROR: interpret_twod_arraydecl: trying to assign a 1d array to a 2d array\n");
    }
    else if (arraydecl->numOfChildren == 1) { // Type[][] ID;
        // do nothing because the variable is already in the table
        // and we don't have a value to update it with
    }
}

/* 
 * interpret a oned array assignment
 */
struct asm_node* interpret_oned_array_assignment(struct node* assignment, char* table_name) { 
    struct asm_node* ans;
    struct symbol_table_entry* entry;
    struct symbol_table_entry* leftentry;
    struct asm_node* node1;
    struct asm_node* node2;
    struct asm_node* node3;
    struct asm_node* node4;
    struct asm_node* node5;
    struct asm_node* node6;
    struct asm_node* node7;

    if (assignment->numOfChildren == 2) { // ID[Expr] = Expr;
        // get code which determines the index and places in temp var
        ans = evaluate_expression(assignment->children[0], table_name);
        char* index_temp_var = get_last_temp_name();

        // get code which evaluates the rhs and places in a temp var
        struct asm_node* rhs = evaluate_expression(assignment->children[1], table_name);
        char* rhs_temp_var = get_last_temp_name();

        // get the offset for the id on the lhs
        char* lhs_offset = get_offset_name(assignment->data.value.stringValue, table_name);

        // now, we want to find the address that's on the lhs so we can put
        // what's on the rhs into it
        // we want to do something like this:
            // ldr r0, [SP, #lhs_offset]
            // ldr r1, [SP, #index_temp_var]
            // mov r2, #4
            // mul r1, r1, r2
            // add r0, r0, r1
            // @ r0 now holds the location that we want to write to
            // ldr r1, [SP, #rhs_temp_var]
            // str r1, [r0, #0]

        node1 = make_asm_node(build_instruction("ldr", 0, lhs_offset));
        node2 = make_asm_node(build_instruction("ldr", 1, index_temp_var));
        node3 = make_asm_node(build_mov_instruction(2, 4));
        node4 = make_asm_node("mul r1, r1, r2");
        node5 = make_asm_node("add r0, r0, r1");
        node6 = make_asm_node(build_instruction("ldr", 1, rhs_temp_var));
        node7 = make_asm_node("str r1, [r0, #0]");

        node1 = cat_asm_lists(node1, node2);
        node1 = cat_asm_lists(node1, node3);
        node1 = cat_asm_lists(node1, node4);
        node1 = cat_asm_lists(node1, node5);
        node1 = cat_asm_lists(node1, node6);
        node1 = cat_asm_lists(node1, node7);
        node1 = add_comment(node1, "\n@ assign rhs to oned array element");

        // cat on the important things at the beginning
        ans = cat_asm_lists(ans, rhs);
        ans = cat_asm_lists(ans, node1);

        return ans;
    }
    else if (assignment->numOfChildren == 3) { // ID[Expr] = new Type[Expr];
        // figure out what the left index is
        ans = evaluate_expression(assignment->children[0], table_name);
    }
}

/* 
 * interpret a 2 dimensional array assignment
 */
void interpret_twod_array_assignment(struct node* assignment, char* table_name) { 
    struct asm_node* ans;
    struct symbol_table_entry* entry;

    if (assignment->numOfChildren == 3) { // ID[Expr][Expr] = Expr;
        // figure out what the first index is
        ans = evaluate_expression(assignment->children[0], table_name);
        /*
        int first_index = ans.value.intValue;

        // figure out what the second index is
        ans = evaluate_expression(assignment->children[1], table_name);
        int second_index = ans.value.intValue;

        // figure out what the answer on the rhs is
        ans = evaluate_expression(assignment->children[2], table_name);

        // get the entry from the table
        entry = get_symbol_tab_entry(assignment->data.value.stringValue, table_name);

        // assign the value to the location that we're looking at
        if (strcmp(ans.type, "int") == 0) {
            ((int*)(entry->array->oned_arrays[first_index]->array))[second_index] = ans.value.intValue;
        }
        else if (strcmp(ans.type, "boolean") == 0) {
            ((bool*)(entry->array->oned_arrays[first_index]->array))[second_index] = ans.value.booleanValue;
            
        }
        else if (strcmp(ans.type, "string") == 0) {
            ((char**)(entry->array->oned_arrays[first_index]->array))[second_index] = ans.value.stringValue;
        }
        */


    }
    else if (assignment->numOfChildren == 5) { // ID[Expr][Expr] = new Type[Expr][Expr];
        // NOTE: this shouldn't ever happen because this is a type error.
        // you can't assign a 2d array to a spot which is an array location
        printf("ERROR: interpret_twod_array_assignment: trying to assign a 2d array to an array index\n");
    }
    else if (assignment->numOfChildren == 4) { // ID[Expr][Expr] = new Type[Expr];
        // NOTE: this shouldn't ever happen because this is a type error.
        // you can't assign a 1d array to a spot which is an array location
        printf("ERROR: interpret_twod_array_assignment: trying to assign a 1d array to an array index\n");
    }
}

/* 
 * evaluates the expression and returns a data structure
 * that contains the answer. in order to retrieve the answer,
 * check the data.type field.
 * EVALUATE_EXPRESSION
 */
struct asm_node* evaluate_expression(struct node* expression, char* table_name) { // probably done fr 3.1. one of the types of Expression_piece.length needs to be tested

    struct asm_node* ret; // the struct we will return
    struct asm_node* lhs; // the lhs of the expression
    struct asm_node* rhs; // the rhs of the expression
    char* lhs_name;
    char* rhs_name;
    struct symbol_table_entry* entry;
    char* name; // the name of the id we're looking for

    char* instr1;
    char* instr2;
    char* instr3;
    struct asm_node* node1;
    struct asm_node* node2;
    struct asm_node* node3;
    struct asm_node* node4;
    struct asm_node* node5;
    struct asm_node* node6;
    struct asm_node* node7;
    struct asm_node* node8;
    struct asm_node* node9;
    struct asm_node* node10;

    struct asm_node* load_left;
    struct asm_node* load_right;
    struct asm_node* compare;
    struct asm_node* branch_instr;
    char* label1;
    char* temp_name;
    char* end;

    if (expression == NULL) {
        printf("ERROR: expression was NULL in evaluate_expression\n");
        return NULL;
    }

    //printf("evaluating expression with type: %s\n", get_type_name(expression->nodeType));

    // now that we don't have any errors that will cause seg faults, let's evaluate the expression
    switch (expression->nodeType) {
        case EXPRESSION_PIECE: ; // needs booleans and strings for 3.1
            // this is an expression piece (an integer literal, string literal, boolean literal, 
            // or ID, or ONED_ARRAY_REFERENCE or TWOD_ARRAY_REFERENCE.
            // the only way for this function to get called is when the expression
            // is just a single expression piece, so we have to deal with that here

            switch (expression->children[0]->nodeType) {
                case ID_: ; 
                    // if this expression piece is an ID, we should generate
                    // code to get that loads that ID's location into a temp
                    // register. we need to know what the offset for that variable
                    // is in the stack frame though

                    // now, get the offset name for the variable we're referencing
                    char* offset_name = get_offset_name(expression->children[0]->data.value.stringValue, table_name);
                    if (offset_name == NULL) {
                        printf("ERROR: evaluate_expression(): offset name was NULL\n");
                        return NULL;
                    }

                    // now that we have the offset name, we can generate the code.
                    // get a new temp name
                    temp_name = get_temp_name(table_name);

                    // generate the code which loads the address of the variable
                    instr1 = build_instruction("ldr", 0, offset_name);
                    node1 = make_asm_node(instr1);

                    // generate the code which dereferences the address to get the value
                    // and puts that in the temp var slot
                    instr2 = build_instruction("str", 0, temp_name);
                    node2 = make_asm_node(instr2);

                    // that's all we need, so just cat these two lists together
                    node1 = cat_asm_lists(node1, node2);

                    // add a label so we know what this code does
                    node1 = add_comment(node1, "\n@ fetch ID and place in temp var");

                    // now we can just return this list, because that's what the caller
                    // is expecting
                    return node1;

                case ONED_ARRAY_REFERENCE: ; // this might be argv
                    // get the offset for the array name
                    char* argv_offset_name = get_offset_name(expression->children[0]->data.value.stringValue, table_name);

                    // generate code to evaluate the array index
                    load_left = evaluate_expression(expression->children[0]->children[0], table_name);
                    temp_name = get_last_temp_name();

                    // generate code to multiply the index by 4 and load it into r1
                    node1 = make_asm_node(build_instruction("ldr", 0, temp_name));
                    node2 = make_asm_node(build_mov_instruction(1, 4));
                    node3 = make_asm_node("mul r1, r0, r1");
                    temp_name = get_temp_name(table_name);
                    node4 = make_asm_node(build_instruction("str", 1, temp_name));

                    // now that we know where to get the argv, load the base of argv into r0
                    node5 = make_asm_node(build_instruction("ldr", 0, argv_offset_name));

                    // build instruction which will add the index to r0
                    node6 = make_asm_node(build_instruction("ldr", 1, temp_name));
                    node7 = make_asm_node("add r0, r0, r1");

                    // build the instruction which will load the address of argv[index] 
                    char* instruction = malloc(sizeof(char) * (strlen("ldr r0, [rn, #0]") + 1));
                    memcpy(instruction, "ldr r0, [r0, #0]", strlen("ldr r0, [r0, #0]"));
                    instruction[strlen("ldr r0, [r0, #0]")] = '\0';
                    node8 = make_asm_node(instruction);

                    // now that we've loaded the correct adress, store that in a temp variable
                    temp_name = get_temp_name(table_name);
                    node9 = make_asm_node(build_instruction("str", 0, temp_name));

                    // string all these nodes together
                    load_left = cat_asm_lists(load_left, node1);
                    load_left = cat_asm_lists(load_left, node2);
                    load_left = cat_asm_lists(load_left, node3);
                    load_left = cat_asm_lists(load_left, node4);
                    load_left = cat_asm_lists(load_left, node5);
                    load_left = cat_asm_lists(load_left, node6);
                    load_left = cat_asm_lists(load_left, node7);
                    load_left = cat_asm_lists(load_left, node8);
                    load_left = cat_asm_lists(load_left, node9);

                    // add a comment
                    load_left = add_comment(load_left, "\n@ load array element into a temp var");
                    return load_left;
                    break;
                case TWOD_ARRAY_REFERENCE: ; /* don't worry about this for 3.1 */
                    /*
                    // name of the array is in the node
                    
                    // need to evaluate both of the children so that we know what
                    // indices to ask for
                    int index12d = evaluate_expression(expression->children[0]->children[0], table_name).value.intValue;

                    int index22d = evaluate_expression(expression->children[0]->children[1], table_name).value.intValue;

                    // get the entry out of the table
                    entry = get_symbol_tab_entry(expression->children[0]->data.value.stringValue, table_name);

                    // now that we have both the indices, we can get the value at that
                    // location
                    if (strcmp(entry->array->type, "int") == 0) { // this is an int array
                        ret.type = "int";
                        ret.value.intValue = ((int *)(entry->array->oned_arrays[index12d]->array))[index22d];
                    }
                    else if (strcmp(entry->array->type, "boolean") == 0) {// this is a bool array
                        ret.type = "boolean";
                        ret.value.booleanValue = ((bool*)(entry->array->oned_arrays[index12d]->array))[index22d];
                    }
                    else { // this is a string array
                        ret.type = "string";
                        ret.value.stringValue = ((char**)(entry->array->oned_arrays[index12d]->array))[index22d];
                    }

                    return ret;
                    */
                    break;

                case INTEGER_LIT: ; 
                    // we just want to generate asm code that places this integer value
                    // into a temp register so that the statement using this can use
                    // that.

                    // get the value of the integer literal
                    int val = expression->children[0]->data.value.intValue;

                    // generate instruction to mov the constant into r0
                    instr1 = build_mov_instruction(0, val);
                    node1 = make_asm_node(instr1);

                    // get a new temporary variable spot
                    temp_name = get_temp_name(table_name);
                    
                    // generate instruction to str r0 into a temp spot
                    instr2 = build_instruction("str", 0, temp_name);
                    node2 = make_asm_node(instr2);

                    // cat these nodes together
                    node1 = cat_asm_lists(node1, node2);

                    // add a comment so we know what this is
                    node1 = add_comment(node1, "\n@ load an immediate value into a temp var");

                    return node1;
                    break;

                case STRING_LIT: ; 
                    // let's create a new instruction which stores this string
                    // literal at the top of the file so it can be referenced later.
                    char* with_quotes = expression->children[0]->data.value.stringValue;
                    char* stripped = malloc(sizeof(char) * (strlen(with_quotes) - 1));
                    memcpy(stripped, with_quotes + 1, strlen(with_quotes) - 2);
                    stripped[strlen(with_quotes) - 2] = '\0';

                    char* lit_label_name = build_string_literal_instruction(stripped);

                    // we now know which label we can use to reference this literal.
                    // we want to generate code which loads the address of that string
                    // literal into a temp variable.
                    // get a temp variable
                    temp_name = get_temp_name(table_name);

                    // generate the instruciton which loads the address of the string lit
                    // into r0
                    node1 = make_asm_node(build_load_string_lit_address_instruction(0, lit_label_name));

                    // generate the instruction which stores that address in a temp variable
                    node2 = make_asm_node(build_instruction("str", 0, temp_name));

                    // string these all together
                    node1 = cat_asm_lists(node1, node2);

                    // add a comment so we know what this does
                    node1 = add_comment(node1, "\n@ load a string literal's address into a temp variable");

                    return node1;
                    break;
                case BOOLEAN_LIT: ; 
                    // if this expression piece is a boolean literal,
                    // we should generate code which places the value in a temp variable

                    // figure out what the equivalent integer for this boolean would be
                    int int_eq = 0; // the equivalent integer value for the boolean
                    if (expression->children[0]->data.value.booleanValue == true) 
                        int_eq = 1;
                    else 
                        int_eq = 0;

                    // now generate code that places that value into a temp variable
                    temp_name = get_temp_name(table_name);
                    node1 = make_asm_node(build_mov_instruction(0, int_eq));
                    node2 = make_asm_node(build_instruction("str", 0, temp_name));

                    // string these together
                    node1 = cat_asm_lists(node1, node2);

                    // throw on a comment
                    node1 = add_comment(node1, "\n@ load immediate boolean value into a temp var");

                    return node1;
                    break;

                case METHODCALL: ;
                    node1 = interpret_methodcall(expression->children[0], table_name);
                    return node1;
                    break;
                    
                case CLASS_VARIABLE:
                    // this is either ID . ID or THIS . ID
                    if (strcmp(expression->children[0]->data.className, "this") == 0) {
                        // THIS . ID
                        // do this later, since it's only applicable
                        // in the case that it's called from within a method
                    }
                    else {
                        // the class name is something other than "this", so
                        // its the name of a variable
                        char* var_name = expression->children[0]->data.className;

                        // we know what table this variable is in, so we can 
                        // just look it up and grab its offset name
                        char* var_offset_name = get_symbol_tab_entry(var_name, table_name)->offset_name;

                        // figure out the name of the class that the object we're dealing with is
                        char* class_name = get_symbol_tab_entry(var_name, table_name)->data.type;

                        // figure out what the offset for the field on the rhs is.
                        // to do this, we need to find the table for the constructor
                        char* constructor_name = malloc(sizeof(char) * (strlen(class_name) + strlen("_constructor") + 1));
                        memcpy(constructor_name, class_name, strlen(class_name));
                        memcpy(constructor_name + strlen(class_name), "_constructor", strlen("_constructor"));
                        constructor_name[strlen(class_name) + strlen("_constructor")] = '\0';

                        // figure out the field name
                        char* field_name = expression->children[0]->data.value.stringValue;

                        // get the constructor's symbol table
                        struct symbol_table* const_table = find_table(constructor_name, global);
                        if (const_table == NULL) {
                            print_symbol_tables(global);
                            printf("error\n");
                        }

                        char* field_offset_name;
                        for (int i = 0; i < const_table->numEntries; i++) {
                            if (strcmp(const_table->symbol_table[i]->id, field_name) == 0) {
                                field_offset_name = strdup(const_table->symbol_table[i]->offset_name);
                                break;
                            }
                        }

                        // we want to build code like this
                            // ldr r0, [SP, #var_offset_name]
                            // add r0, r0, #offset_for_field_name
                            // ldr r0, [r0, #0]
                            // str r0, [SP, #new_temp_var]

                        node1 = make_asm_node(build_instruction("ldr", 0, var_offset_name));
                        char* node2_instr = malloc(sizeof(char) * (strlen("add r0, r0, #") + strlen(field_offset_name) + 1));
                        memcpy(node2_instr, "add r0, r0, #", strlen("add r0, r0, #"));
                        memcpy(node2_instr + strlen("add r0, r0, #"), field_offset_name, strlen(field_offset_name));
                        node2_instr[strlen("add r0, r0, #") + strlen(field_offset_name)] = '\0';
                        node2 = make_asm_node(node2_instr);
                        node3 = make_asm_node("ldr r0, [r0, #0]");
                        char* new_temp_var = get_temp_name(constructor_name);
                        node4 = make_asm_node(build_instruction("str", 0, new_temp_var));

                        node1 = cat_asm_lists(node1, node2);
                        node1 = cat_asm_lists(node1, node3);
                        node1 = cat_asm_lists(node1, node4);

                        node1 = add_comment(node1, "\n@ get value of field an place in temp var");

                        // before we return, let's set the type of this variable correctly
                        char* type = get_symbol_tab_entry(field_name, constructor_name)->data.type;
                        expression->data.type = strdup(type);
                        expression->children[0]->data.type = strdup(type);
                    
                        return node1;


                    }
                    break;
                default:
                    printf("ERROR: evaluate_expression(): invalid switch case\n");
                    return NULL;
            }

            break;

        case NEG_EXPRESSION_PIECE: ; 
            // assume that the expression piece that we're negating is an integer or an id
            if (expression->children[0]->nodeType == INTEGER_LIT) {
                // get the value of the integer literal
                int val = expression->children[0]->data.value.intValue;

                // load that value into r0
                node1 = make_asm_node(build_mov_instruction(0, val));

                // load -1 into r1
                node2 = make_asm_node("mov r1, #-1");

                // multiply r0 and r1 and put it in r0
                node3 = make_asm_node("mul r0, r0, r1");

                // load the value that we found into a new temp variable
                node4 = make_asm_node(build_instruction("str", 0, get_temp_name(table_name)));
                
                // string these all together
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);
                node1 = cat_asm_lists(node1, node4);

                // add a comment
                node1 = add_comment(node1, "\n@ place negative int literal into temp var");

                return node1;
            }
            else if (expression->children[0]->nodeType == ID_) {
                // get the offset name for this id
                lhs_name = get_offset_name(expression->children[0]->data.value.stringValue, table_name);

                // now that we know the offset name, we can load that
                // value, negate it, and store it in a new temp var
                node1 = make_asm_node(build_instruction("ldr", 0, lhs_name));
                node2 = make_asm_node("mov r1, #-1");
                node3 = make_asm_node("mul r0, r0, r1");
                node4 = make_asm_node(build_instruction("str", 0, get_temp_name(table_name)));

                // string all of these together
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);
                node1 = cat_asm_lists(node1, node4);

                // add a comment
                node1 = add_comment(node1, "\n@ fetch value and put negated value in temp var");

                return node1;
            }

            break;
                            

        case EXPRESSION_PIECE_DOT_LENGTH: ; 
            // for 3.1, assume this is a string

            // this expression might be one of two things:
                // ID.length
                // ExprPiece.length

            if (expression->numOfChildren == 0) { // ID.length 
                // figure out the name
                name = expression->data.value.stringValue;

                // get the offset for that variable
                lhs_name = get_offset_name(name, table_name);

                // get a new temp var so that we can store the result
                temp_name = get_temp_name(table_name);

                // now, we need to generate instructions like this:
                    // ldr r0, [SP, lhs_name]
                    // bl strlen
                    // str r0, [SP, temp_name]

                // this assumes that the lhs name is a pointer to a string 
                // that's on the heap

                // generate instructions
                node1 = make_asm_node(build_instruction("ldr", 0, lhs_name));
                node2 = make_asm_node("bl strlen");
                node3 = make_asm_node(build_instruction("str", 0, temp_name));

                // string these all together
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);

                // add a comment
                node1 = add_comment(node1, "\n@ find length of string and store in temp var");

                return node1;


            }
            else if (expression->numOfChildren == 1) { // ExprPiece.length 
                // figure out the name of the variable on the lhs
                name = expression->children[0]->data.value.stringValue;

                // figure out the offset for that variable
                lhs_name = get_offset_name(name, table_name);

                // get a new temp var so we can store the result
                temp_name = get_temp_name(table_name);

                // now, we need to generate instructions like this:
                    // ldr r0, [SP, lhs_name]
                    // bl strlen
                    // str r0, [SP, temp_name]

                // this assumes that the lhs name is a pointer to a string 
                // that's on the heap

                // generate instructions
                node1 = make_asm_node(build_instruction("ldr", 0, lhs_name));
                node2 = make_asm_node("bl strlen");
                node3 = make_asm_node(build_instruction("str", 0, temp_name));

                // string these all together
                node1 = cat_asm_lists(node1, node2);
                node1 = cat_asm_lists(node1, node3);

                // add a comment
                node1 = add_comment(node1, "\n@ find length of string and store in temp var");

                return node1;
            }
                        
            break;

        case EXPRESSION_MULTIPLY: ; 
            // evaluate the expression for the left child to get the code which
            // loads the left child's value into a temp variable
            lhs = evaluate_expression(expression->children[0], table_name);

            // figure out what the temp variable for the lhs is
            lhs_name = get_last_temp_name();

            // evaluate the expression for the rhs child to get the code which loads the rhs
            // child's value into a temp variable
            rhs = evaluate_expression(expression->children[1], table_name);

            // figure out what the name of the temp variable on the rhs is
            rhs_name = get_last_temp_name();

            // now that we have names for the variables that we'll need to access, we 
            // can generate the code which accesses them. 
            load_left = make_asm_node(build_instruction("ldr", 0, lhs_name));
            load_right = make_asm_node(build_instruction("ldr", 1, rhs_name));
            struct asm_node* mul = make_asm_node("mul r0, r0, r1"); // r0 <- r0 * r1

            // after we get the product, we need to store into another temp variable
            struct asm_node* store_product = make_asm_node(build_instruction("str", 0, get_temp_name(table_name)));

            // now we can just string everything together and return the list of instructions
            lhs = cat_asm_lists(lhs, rhs);
            lhs = cat_asm_lists(lhs, load_left);
            lhs = cat_asm_lists(lhs, load_right);
            lhs = cat_asm_lists(lhs, mul);
            lhs = cat_asm_lists(lhs, store_product);

            // add a newline to the top so that it's more easily readable
            lhs = add_comment(lhs, "");

            return lhs;
        case EXPRESSION_DIVIDE: ; 
            // evaluate the expression for the left child to get the code which
            // loads the left child's value into a temp variable
            lhs = evaluate_expression(expression->children[0], table_name);

            // figure out what the temp variable for the lhs is
            lhs_name = get_last_temp_name();

            // evaluate the expression for the rhs child to get the code which loads the rhs
            // child's value into a temp variable
            rhs = evaluate_expression(expression->children[1], table_name);

            // figure out what the name of the temp variable on the rhs is
            rhs_name = get_last_temp_name();

            // now that we have names for the variables that we'll need to access, we 
            // can generate the code which accesses them. 
            load_left = make_asm_node(build_instruction("ldr", 0, lhs_name));
            load_right = make_asm_node(build_instruction("ldr", 1, rhs_name));
            struct asm_node* div = make_asm_node("bl __aeabi_idiv"); // r0 <- r0 / r1

            // after we get the quotient, we need to store into another temp variable
            struct asm_node* store_quotient = make_asm_node(build_instruction("str", 0, get_temp_name(table_name)));

            // now we can just string everything together and return the list of instructions
            lhs = cat_asm_lists(lhs, rhs);
            lhs = cat_asm_lists(lhs, load_left);
            lhs = cat_asm_lists(lhs, load_right);
            lhs = cat_asm_lists(lhs, div);
            lhs = cat_asm_lists(lhs, store_quotient);

            // add a newline to the top so that it's more easily readable
            lhs = add_comment(lhs, "");

            return lhs;
                                                       
        case EXPRESSION_ADD: ; 
            if (strcmp(expression->data.type, "string") == 0) {
                // these are strings that we want to concatenate

                // we can call build_string_literal_instruction with each of the 
                // string literals that are the two children. that would allow
                // us to record what the labels will be for those two strings,
                // so that we can use them to find the lengths of the strings
                // and stuff

                // get code which loads the lhs string's address into a temp var
                lhs = evaluate_expression(expression->children[0], table_name);

                // get the name of that temp var
                lhs_name = get_last_temp_name();

                // get code which loads the rhs string's address into a temp var
                rhs = evaluate_expression(expression->children[1], table_name);

                // get the name of that temp var
                rhs_name = get_last_temp_name();

                // now we have two labels, and we need to figure out their lengths
                // so we can call malloc. we'll make two calls to strlen and record
                // the outputs in new temp variables.
                // ldr r0, [SP, #lhs_name]
                // bl strlen
                // str r0, [SP, #new_temp_name]
                // ldr r0, [SP, #rhs_name]
                // bl strlen
                // str r0, [SP, #second_new_temp_name]

                node1 = make_asm_node(build_instruction("ldr", 0, lhs_name));
                node2 = make_asm_node("bl strlen");
                char* lhs_length_temp_name = get_temp_name(table_name);
                node3 = make_asm_node(build_instruction("str", 0, lhs_length_temp_name));
                node4 = make_asm_node(build_instruction("ldr", 0, rhs_name));
                node5 = make_asm_node("bl strlen");
                char* rhs_length_temp_name = get_temp_name(table_name);
                node6 = make_asm_node(build_instruction("str", 0, rhs_length_temp_name));

                // cat some stuff together so we can reuse variables
                lhs = cat_asm_lists(lhs, rhs);
                node1 = add_comment(node1, "\n@ get length of two string variables and place in temp vars");
                lhs = cat_asm_lists(lhs, node1);
                lhs = cat_asm_lists(lhs, node2);
                lhs = cat_asm_lists(lhs, node3);
                lhs = cat_asm_lists(lhs, node4);
                lhs = cat_asm_lists(lhs, node5);
                lhs = cat_asm_lists(lhs, node6);

                // now we have the lengths of each of the strings in two temp variables.
                // we need to add these together, add one, and call malloc
                node1 = make_asm_node(build_instruction("ldr", 0, lhs_length_temp_name));
                node2 = make_asm_node(build_instruction("ldr", 1, rhs_length_temp_name));
                node3 = make_asm_node("add r0, r0, r1");
                node4 = make_asm_node("add r0, r0, #1");
                node1 = add_comment(node1, "\n@ calculate length of new string to malloc");

                // cat some thigns
                lhs = cat_asm_lists(lhs, node1);
                lhs = cat_asm_lists(lhs, node2);
                lhs = cat_asm_lists(lhs, node3);
                lhs = cat_asm_lists(lhs, node4);

                // now we just need to make a call to malloc and store the result
                // in a temp variable
                node1 = make_asm_node("bl malloc");

                // cat some things since we need a lot of nodes
                node1 = add_comment(node1, "\n@ call malloc");
                lhs = cat_asm_lists(lhs, node1);

                // now we need to copy the memory in with memcpy:
                    // @ copy the first string in
                    // ldr r1, [SP, #lhs_name]
                    // ldr r2, [SP, #lhs_length_temp_name]
                    // mov r4, r0 @ save the pointer to the beginning of string
                    // mov r5, r2 @ save length of the first string
                    // bl memcpy

                    // mov r0, r4 @ reload the beginning of the string pointer
                    // mov r1, r5 @ reload the length of the first string
                    // add r0, r0, r1 @ add the length to the string so we shift it

                    // @ copy the second string in 
                    // ldr r1, [SP, #rhs_name]
                    // ldr r2, [SP, #rhs_length_temp_name]
                    // bl memcpy

                node1 = make_asm_node(build_instruction("ldr", 1, lhs_name));
                node2 = make_asm_node(build_instruction("ldr", 2, lhs_length_temp_name));
                node3 = make_asm_node("mov r4, r0 @ save the pointer to beg of str");
                node4 = make_asm_node("mov r5, r2 @ save length of first string");
                node5 = make_asm_node("bl memcpy");

                // cat some things
                lhs = cat_asm_lists(lhs, node1);
                lhs = cat_asm_lists(lhs, node2);
                lhs = cat_asm_lists(lhs, node3);
                lhs = cat_asm_lists(lhs, node4);
                lhs = cat_asm_lists(lhs, node5);

                // build instructions which set up the pointer again
                node1 = make_asm_node("mov r0, r4 @ reload beginning of new string");
                node2 = make_asm_node("mov r1, r5 @ reload length of first string");
                node3 = make_asm_node("add r0, r0, r1 @ shift pointer over");

                // build instructions to copy the second string in
                node4 = make_asm_node(build_instruction("ldr", 1, rhs_name));
                node5 = make_asm_node(build_instruction("ldr", 2, rhs_length_temp_name));
                node6 = make_asm_node("bl memcpy @ copy the second string in");

                lhs = cat_asm_lists(lhs, node1);
                lhs = cat_asm_lists(lhs, node2);
                lhs = cat_asm_lists(lhs, node3);
                lhs = cat_asm_lists(lhs, node4);
                lhs = cat_asm_lists(lhs, node5);
                lhs = cat_asm_lists(lhs, node6);

                // now that we've built the whole string, we need to store
                // the pointer to that string in a temp variable so that
                // whoever uses that string knows where to get it
                char* new_temp_name = get_temp_name(table_name);
                node1 = make_asm_node(build_instruction("str", 4, new_temp_name));
                lhs = cat_asm_lists(lhs, node1);

                lhs = add_comment(lhs, "\n@\n@ cat two strings and store the resulting pointer in a temp var\n@");

                return lhs;


            }
            else {
                // these are integers that need to be added together

                // this expression has two children. we need to generate assembly
                // code which finds the value of both of the children and stores
                // the values in temp variables. therefore, we need to call
                // this function for each of the children

                // evaluate the expression for the left child to get the code which
                // loads the left child's value into a temp variable
                lhs = evaluate_expression(expression->children[0], table_name);

                // figure out what the temp variable for the lhs is
                lhs_name = get_last_temp_name();

                // evaluate the expression for the rhs child to get the code which loads the rhs
                // child's value into a temp variable
                rhs = evaluate_expression(expression->children[1], table_name);

                // figure out what the name of the temp variable on the rhs is
                rhs_name = get_last_temp_name();

                // now that we have names for the variables that we'll need to access, we 
                // can generate the code which accesses them. 
                load_left = make_asm_node(build_instruction("ldr", 0, lhs_name));
                load_right = make_asm_node(build_instruction("ldr", 1, rhs_name));
                struct asm_node* add = make_asm_node("add r0, r0, r1"); // add r0 and r1 and store in r0

                // after we add these together, we need to store into another temp variable
                struct asm_node* store_sum = make_asm_node(build_instruction("str", 0, get_temp_name(table_name)));

                // now we can just string everything together and return the list of instructions
                lhs = cat_asm_lists(lhs, rhs);
                lhs = cat_asm_lists(lhs, load_left);
                lhs = cat_asm_lists(lhs, load_right);
                lhs = cat_asm_lists(lhs, add);
                lhs = cat_asm_lists(lhs, store_sum);

                // add a newline to the top so that it's more easily readable
                lhs = add_comment(lhs, "");
                                
                return lhs;
            }

        case EXPRESSION_SUBTRACT: ; 
            // evaluate the expression for the left child to get the code which
            // loads the left child's value into a temp variable
            lhs = evaluate_expression(expression->children[0], table_name);

            // figure out what the temp variable for the lhs is
            lhs_name = get_last_temp_name();

            // evaluate the expression for the rhs child to get the code which loads the rhs
            // child's value into a temp variable
            rhs = evaluate_expression(expression->children[1], table_name);

            // figure out what the name of the temp variable on the rhs is
            rhs_name = get_last_temp_name();

            // now that we have names for the variables that we'll need to access, we 
            // can generate the code which accesses them. 
            load_left = make_asm_node(build_instruction("ldr", 0, lhs_name));
            load_right = make_asm_node(build_instruction("ldr", 1, rhs_name));
            struct asm_node* sub = make_asm_node("sub r0, r0, r1"); // r0 <- r0 - r1

            // after we get the difference, we need to store into another temp variable
            struct asm_node* store_diff = make_asm_node(build_instruction("str", 0, get_temp_name(table_name)));

            // now we can just string everything together and return the list of instructions
            lhs = cat_asm_lists(lhs, rhs);
            lhs = cat_asm_lists(lhs, load_left);
            lhs = cat_asm_lists(lhs, load_right);
            lhs = cat_asm_lists(lhs, sub);
            lhs = cat_asm_lists(lhs, store_diff);

            // add a newline to the top so that it's more easily readable
            lhs = add_comment(lhs, "");

            return lhs;
        case EXPRESSION_PARENTHESES: ; 
            // this looks like (Expr), so just generate code for the Expr
            lhs = evaluate_expression(expression->children[0], table_name);

            return lhs;

        case EXPRESSION_EQUAL_EQUAL: ; 
            ret = gen_code_for_comparison(expression, table_name);
            return ret;
            break;
        case EXPRESSION_NOT_EQUAL: ; 
            ret = gen_code_for_comparison(expression, table_name);
            return ret;
            break;
        case EXPRESSION_GREATER_EQUAL: 
            ret = gen_code_for_comparison(expression, table_name);
            return ret;
            break;
        case EXPRESSION_LESS_EQUAL: 
            ret = gen_code_for_comparison(expression, table_name);
            return ret;
            break;
        case EXPRESSION_GREATER: 
            ret = gen_code_for_comparison(expression, table_name);
            return ret;
            break;
        case EXPRESSION_LESS: 
            ret = gen_code_for_comparison(expression, table_name);
            return ret;
            break;
        case EXPRESSION_AND: 
            // we want to figure out whether the right and left sides are both true.
            // first, we need to get both the right and left sides and record
            // the names that we'll use to access those
            lhs = evaluate_expression(expression->children[0], table_name);
            lhs_name = get_last_temp_name();
            rhs = evaluate_expression(expression->children[1], table_name);
            rhs_name = get_last_temp_name();

            // we want to generate code that looks like this:
                // ldr r0, [SP, lhs_name] @ load lhs into r0
                // mov r1, #1 @ move true into r1
                // cmp r0, r1 @ check whether lhs is true
                // ble _false_label @ if lhs is <= 1, branch to false
                // ldr r0, [SP, rhs_name] @ true, so load rhs into r0 to check it
                // cmp r0, r1 @ check whether rhs is true
                // ble _false_label @ if rhs is <= 1, branch to false
                // @ here, we haven't branched to false so they must both be true
                // mov r0, #1 @ move true into r0
                // str r0, [SP, _temp_var] @ move true into temp var
                // b _end @ don't move false into the temp var too
                // _false_label:
                // mov r0, #0 @ move false into r0
                // str r0, [SP, _temp_var] @ move false into temp var
                // _end:
            
            // get labels to use for the false branch and the end
            label1 = get_new_label();
            end = get_new_label();

            // get a temp var so that we can store the result
            temp_name = get_temp_name(table_name);

            // generate code to load the left and check whether it's true
            load_left = make_asm_node(build_instruction("ldr", 0, lhs_name));
            node1 = make_asm_node(build_mov_instruction(1, 1));
            node2 = make_asm_node("cmp r0, r1");
            node3 = make_asm_node(build_branch_instruction("blt", label1));

            // generate code to load the right and check whether it's true
            load_right = make_asm_node(build_instruction("ldr", 0, rhs_name));
            node4 = make_asm_node(build_mov_instruction(1, 1));
            node5 = make_asm_node("cmp r0, r1");
            node6 = make_asm_node(build_branch_instruction("blt", label1));

            // link these all together so we can reuse variables
            load_left = cat_asm_lists(load_left, node1);
            load_left = cat_asm_lists(load_left, node2);
            load_left = cat_asm_lists(load_left, node3);
            load_left = cat_asm_lists(load_left, load_right);
            load_left = cat_asm_lists(load_left, node4);
            load_left = cat_asm_lists(load_left, node5);
            load_left = cat_asm_lists(load_left, node6);

            // generate code to put true value in temp var and branch to end
            node1 = make_asm_node(build_mov_instruction(0, 1));
            node2 = make_asm_node(build_instruction("str", 0, temp_name));
            node3 = make_asm_node(build_branch_instruction("b", end));

            // generate code for the false branch (which puts false value in temp var
            // and falls through to end)
            node4 = make_asm_node(label1); // label1 is the false branch
            node5 = make_asm_node(build_mov_instruction(0, 0));
            node6 = make_asm_node(build_instruction("str", 0, temp_name));

            // generate code for the end label
            node7 = make_asm_node(end);

            // string these all together
            load_left = cat_asm_lists(load_left, node1);
            load_left = cat_asm_lists(load_left, node2);
            load_left = cat_asm_lists(load_left, node3);
            load_left = cat_asm_lists(load_left, node4);
            load_left = cat_asm_lists(load_left, node5);
            load_left = cat_asm_lists(load_left, node6);
            load_left = cat_asm_lists(load_left, node7);

            // throw a comment at the top so we know what we're doing
            load_left = add_comment(load_left, "\n@ compare && and store result in temp var");

            // wrap everything together and return it
            lhs = cat_asm_lists(lhs, rhs);
            lhs = cat_asm_lists(lhs, load_left);

            return lhs;

            break;

        case EXPRESSION_OR: 
            // we want to figure out whether either the rhs or lhs is true.
            // first, we need to get both the right and left sides and record
            // the names that we'll use to access those
            lhs = evaluate_expression(expression->children[0], table_name);
            lhs_name = get_last_temp_name();
            rhs = evaluate_expression(expression->children[1], table_name);
            rhs_name = get_last_temp_name();

            // we want to generate code that looks like this:
                // ldr r0, [SP, lhs_name] @ load lhs into r0
                // mov r1, #1 @ move true into r1
                // cmp r0, r1 @ check whether lhs is true
                // bge _true_label @ if lhs is >= 1, branch to true
                // ldr r0, [SP, rhs_name] @ false, so load rhs into r0 to check it
                // cmp r0, r1 @ check whether rhs is true
                // bge _true_label @ if rhs is >= 1, branch to true
                // @ here, we haven't branched to true so they must both be false
                // mov r0, #0 @ move false into r0
                // str r0, [SP, _temp_var] @ move false into temp var
                // b _end @ don't move true into the temp var too
                // _true_label:
                // mov r0, #1 @ move true into r0
                // str r0, [SP, _temp_var] @ move true into temp var
                // _end:
            
            // get labels to use for the false branch and the end
            label1 = get_new_label();
            end = get_new_label();

            // get a temp var so that we can store the result
            temp_name = get_temp_name(table_name);

            // generate code to load the left and check whether it's true
            load_left = make_asm_node(build_instruction("ldr", 0, lhs_name));
            node1 = make_asm_node(build_mov_instruction(1, 1));
            node2 = make_asm_node("cmp r0, r1");
            node3 = make_asm_node(build_branch_instruction("bge", label1));

            // generate code to load the right and check whether it's true
            load_right = make_asm_node(build_instruction("ldr", 0, rhs_name));
            node4 = make_asm_node(build_mov_instruction(1, 1));
            node5 = make_asm_node("cmp r0, r1");
            node6 = make_asm_node(build_branch_instruction("bge", label1));

            // link these all together so we can reuse variables
            load_left = cat_asm_lists(load_left, node1);
            load_left = cat_asm_lists(load_left, node2);
            load_left = cat_asm_lists(load_left, node3);
            load_left = cat_asm_lists(load_left, load_right);
            load_left = cat_asm_lists(load_left, node4);
            load_left = cat_asm_lists(load_left, node5);
            load_left = cat_asm_lists(load_left, node6);

            // generate code to put false value in temp var and branch to end
            node1 = make_asm_node(build_mov_instruction(0, 0));
            node2 = make_asm_node(build_instruction("str", 0, temp_name));
            node3 = make_asm_node(build_branch_instruction("b", end));

            // generate code for the true branch (which puts true value in temp var
            // and falls through to end)
            node4 = make_asm_node(label1); // label1 is the true branch
            node5 = make_asm_node(build_mov_instruction(0, 1));
            node6 = make_asm_node(build_instruction("str", 0, temp_name));

            // generate code for the end label
            node7 = make_asm_node(end);

            // string these all together
            load_left = cat_asm_lists(load_left, node1);
            load_left = cat_asm_lists(load_left, node2);
            load_left = cat_asm_lists(load_left, node3);
            load_left = cat_asm_lists(load_left, node4);
            load_left = cat_asm_lists(load_left, node5);
            load_left = cat_asm_lists(load_left, node6);
            load_left = cat_asm_lists(load_left, node7);

            // throw a comment at the top so we know what we're doing
            load_left = add_comment(load_left, "\n@ compare || and store result in temp var");

            // wrap everything together and return it
            lhs = cat_asm_lists(lhs, rhs);
            lhs = cat_asm_lists(lhs, load_left);

            return lhs;

            break;

        case EXPRESSION_NEGATED: 
            break;
    }

    return NULL;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//- misc helper functions ------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/* 
 * gets a pointer to the symbol table entry for the ID that we want.
 */
struct symbol_table_entry* get_symbol_tab_entry(char* id, char* table_name) {
    // first, figure out what table they want us to look in
    struct symbol_table* table = find_table(table_name, global);
    if (table == NULL) {
        printf("ERROR: get_symbol_tab_entry(): table returned by find_table() was NULL\n");
        return NULL;
    }
    
    // now, look in that table for the entry we want
    for (int i = 0; i < table->numEntries; i++) {
        if (strcmp(table->symbol_table[i]->id, id) == 0) {
            return table->symbol_table[i];
        }
    }

    // if we haven't found the entry yet, we should look in other tables
    return NULL;

}

/* 
 * get the name of the last temporary variable that was allocated
 */
char* get_last_temp_name() {
    int last_temp_num = num_temp_vars - 1;

    char* name;

    // temp names will use the 01, 02 format so that we don't have to do 
    // things differently for numbers that are less than 10
    if (last_temp_num < 10) {
        // get some memory for the variable name
        name = malloc(sizeof(char) * strlen("_tempn_offset\0"));

        // fill out the first part
        memcpy(name, "_temp", strlen("_temp"));

        // now fill out the number
        snprintf(name + strlen("_temp"), 2, "%d", last_temp_num);
        
        // add the "_offset\0"
        memcpy(name + strlen("_tempn"), "_offset\0", strlen("_offset\0"));

        return name;
    }
    else if (last_temp_num < 100) {
        // get some memory for the variable name
        name = malloc(sizeof(char) * strlen("_tempnn_offset\0"));

        // fill out the first part
        memcpy(name, "_temp", strlen("_temp"));

        // now fill out the number
        snprintf(name + strlen("_temp"), 3, "%d", last_temp_num);

        // add the "_offset\0"
        memcpy(name + strlen("_tempnn"), "_offset\0", strlen("_offset\0"));

        return name;
    }
    else {
        printf("WARNING: get_last_temp_name(): last_temp_num was 100 or more\n");
        return NULL;
    }
}

/* 
 * get a new temp variable name. when we get a new temp name, 
 * add it to the symbol table for the method/class which is
 * represented by the table_name
 */
char* get_temp_name(char* table_name) {
    // check argument
    if ((table_name == NULL) || 
        (strcmp(table_name, "") == 0)) {
        printf("ERROR: get_temp_name(): arg table_name was either NULL or \"\"\n");
        return NULL;
    }

    char* name;
    int temp_num = num_temp_vars;

    // build the variable name
    if (temp_num < 10) {
        // get some memory for the variable name
        name = malloc(sizeof(char) * strlen("_tempn_offset\0"));

        // fill out the first part
        memcpy(name, "_temp", strlen("_temp"));

        // now fill out the number
        snprintf(name + strlen("_temp"), 2, "%d", temp_num);
        
        // add the "_offset\0"
        memcpy(name + strlen("_tempn"), "_offset\0", strlen("_offset\0"));

    }
    else if (temp_num < 100) {
        // get some memory for the variable name
        name = malloc(sizeof(char) * strlen("_tempnn_offset\0"));

        // fill out the first part
        memcpy(name, "_temp", strlen("_temp"));

        // now fill out the number
        snprintf(name + strlen("_temp"), 3, "%d", temp_num);

        // add the "_offset\0"
        memcpy(name + strlen("_tempnn"), "_offset\0", strlen("_offset\0"));

    }
    else {
        printf("WARNING: get_temp_name(): num_temp_vars was 100 or more\n");
        return NULL;
    }

    // increment the number of temp variables that we've allocated
    num_temp_vars++;

    /* find the table we're dealing with and add this to the end */
    /* of that table                                             */
    // find the table
    struct symbol_table* table = find_table(table_name, global);
    if (table == NULL) {
        printf("ERROR: get_temp_name(): table returned by find_table() was null\n");
        return NULL;
    }

    // allocate for a new symbol table entry
    struct symbol_table_entry* entry = malloc(sizeof(struct symbol_table_entry));

    // fill out the fields for the entry
    entry->id = name; // the name that we computed before
    // assign some data to it
    struct data dat;
    dat.type = "temp_var";
    entry->data = dat;
    entry->is_array = 0; // not an array
    entry->is_method = 0; // not a method
    entry->array_dims = 0; // not an array
    entry->is_class_var = 0; // not relevant
    entry->array = NULL; // there's no array to keep track of
    entry->offset_name = name; // they're both the same

    // add the entry to the table
    table->symbol_table[table->numEntries] = entry;
    table->numEntries++;

    // now that we've added this temp var to the symbol table for the
    // method/class that's asking for a temp var name, we can just give
    // them the temp var name
    return name;

}

/* 
 * builds the load instruction which loads a variable
 * into register reg from the place [SP, #offset_name]:
 * ldr rreg, [SP, #offset_name]
 */
char* build_instruction(const char* op, int reg, char* offset_name) {
    if ((op == NULL) || (offset_name == NULL) || (reg < 0) || (reg > 16)) {
        printf("ERROR: build_instruction(): argument is NULL or invalid\n");
        return NULL;
    }

    // allocate memory for the string
    int instruction_length = (strlen("xxx r0, [SP, #") + strlen(offset_name) + strlen("]\0"));
    char* instr = malloc(sizeof(char) * instruction_length);

    // copy the "op r" part in
    if (strcmp(op, "ldr") == 0) {
        memcpy(instr, "ldr r", strlen("xxx r"));
    }
    else if (strcmp(op, "str") == 0) {
        memcpy(instr, "str r", strlen("xxx r"));
    }
    else {
        printf("WARNING: build_instruction(): unknown asm operation\n");
        return NULL;
    }

    // copy the register number in.
    snprintf(instr + strlen("xxx r"), instruction_length - strlen("xxx r"), "%d", reg);

    // copy the ", [SP, #" in 
    memcpy(instr + strlen("xxx rn"), ", [SP, #", strlen(", [SP, #"));

    // copy the offset name in
    memcpy(instr + strlen("xxx rn, [SP, #"), offset_name, strlen(offset_name));

    // copy the "]\0" in
    memcpy(instr + strlen("xxx rn, [SP, #") + strlen(offset_name), "]\0", 2);

    // we're done building the instruction
    return instr;
}

/* 
 * builds a mov instruction where we move the constant into the register:
 * mov rreg, #constant
 */
char* build_mov_instruction(int reg, int constant) {
    // figure out the length of the constant int
    int constant_length;
    if (constant == 0) {
        constant_length = 1;
    }
    else {
        constant_length = (int)(log10(constant) + 1);
    }

    int instr_length = (strlen("mov rn, #") + constant_length + 1);

    // get memory for the instruction
    char* instruction = malloc(sizeof(char) * instr_length);

    // populate the memory with the instruction
    memcpy(instruction, "mov r", strlen("mov r")); // copy in the "mov r"
    snprintf(instruction + strlen("mov r"), 2, "%d", reg); // copy in the register number
    memcpy(instruction + strlen("mov rn"), ", #", strlen(", #"));
    snprintf(instruction + strlen("mov rn, #"), constant_length + 1, "%d", constant);
    instruction[strlen("mov rn, #") + constant_length] = '\0'; // terminate the string

    return instruction;
}

/* 
 * make an asm node which has the instruction instruction
 */
struct asm_node* make_asm_node(char* instruction) {
    // allocate memory for the node
    struct asm_node* ret = malloc(sizeof(struct asm_node));
    
    // copy the instruction in
    ret->instruction = instruction;

    // set the next field to NULL just in case
    ret->next = NULL;

    // set the prev field to NULL just in case
    ret->prev = NULL;

    return ret;
}

/* 
 * cat list2 onto the end of list1 and return the head of the first list
 */
struct asm_node* cat_asm_lists(struct asm_node* list1, struct asm_node* list2) {
    // check arguments
    if ((list1 == NULL) ||
        (list2 == NULL)) {
        printf("WARNING: cat_asm_lists(): one of the arguments was NULL\n");
        return NULL;
    }

    // find the end of list1
    struct asm_node* current = list1;
    while (current->next != NULL) {
        current = current->next;
    }

    // now we have the last node in list1 (which has a null next node)
    current->next = list2;
    list2->prev = current;

    return list1;
}

/* 
 * print an asm list to the FILE out
 */
void print_asm_list(struct asm_node* list, FILE* out) {
    struct asm_node* current = list;
    while (current != NULL) {
       fprintf(out, "%s\n", current->instruction);
       current = current->next;
    }

    return;
}

/* 
 * get the name which we will use to reference the offset
 * of a variable
 */
char* get_offset_name(char* var_name, char* table_name) {
    // check arguments
    if ((var_name == NULL) || (table_name == NULL)) {
        printf("ERROR: get_offset_name(): argument was NULL\n");
        return NULL;
    }

    // first, get the symbol table entry for this var
    struct symbol_table_entry* entry = get_symbol_tab_entry(var_name, table_name);
    if (entry == NULL) {
        // try to get the entry with __formal on the front
        char* with_formal = malloc(sizeof(char) * (strlen("__formal_") + strlen(var_name) + 1));
        memcpy(with_formal, "__formal_", strlen("__formal_"));
        memcpy(with_formal + strlen("__formal_"), var_name, strlen(var_name));
        with_formal[strlen("__formal_") + strlen(var_name)] = '\0';
        entry = get_symbol_tab_entry(with_formal, table_name);
        if (entry == NULL) {
            printf("ERROR: get_offset_name(): entry from get_symbol_tab_entry() was NULL\n");
            return NULL;
        }
    }

    // get the offset name from the table
    if (entry->offset_name == NULL) {
        printf("ERROR: get_offset_name(): offset_name for %s is NULL\n", entry->id);
        return NULL;
    }
    return entry->offset_name;
}

/*
 * builds the instruction which reserves space on the stack
 * for variables
 */
char* build_stack_alloc_for_method(char* method_name) {
    // the amount of space that we want to allocate is equal to
        // (the number of arguments) * sizeof(argument) + 
        // (the number of variables) * sizeof(variable) + 
        // (the number of temp variables) * sizeof(temp variable)

    int num_words = 0; // the number of variables that we need to put on the stack

    // find the table that we need to look at
    struct symbol_table* table = find_table(method_name, global);
    if (table == NULL) {
        printf("ERROR: build_stack_alloc_for_method(): find_table() returned a null table\n");
        printf("method name was %s\n", method_name);
        return NULL;
    }

    // count the number of words in the table that we need
    // to allocate stack space for
    for (int i = 0; i < table->numEntries; i++) {
        // if this entry is a temp variable entry, we need to allocate a word 
        // for it
        if (strcmp(table->symbol_table[i]->data.type, "temp_var") == 0) {
            num_words++;
        }
        // if this variable's name starts with "__formal_", this variable is 
        // a formal argument to this method. therefore, we need to allocate
        // a word on the stack for each of these (so that we can support
        // recursive calls
        else if (strncmp(table->symbol_table[i]->id, "__formal_", strlen("__formal_")) == 0) {
            num_words++;
        }
        else {
            // this is a regular variable name, so allocate space on the stack
            // for it
            num_words++;
        }
    }

    // now that we know how many words we have, we want to allocate 
    // 4 times that many bytes
    int num_bytes = num_words * 4;

    // figure out how many bytes are needed to represent this
    int bytes_length = 0;
    if (num_bytes == 0) 
        bytes_length = 1;
    else
        bytes_length = (int)(log10(num_bytes) + 1);

    // allocate memory for what we'll return
    char* to_return = malloc(sizeof(char) * (strlen("sub SP, SP, #") + bytes_length + 1));
    // fill out what we'll return
    memcpy(to_return, "sub SP, SP, #", strlen("sub SP, SP, #"));
    snprintf(to_return + strlen("sub SP, SP, #"), bytes_length + 1, "%d", num_bytes);

    // now that we know what we'll return, return it
    return to_return;
}

/* 
 * builds the instruction which shifts the stack up
 * to effectively deallocate the stack frame
 */
char* build_stack_dealloc_for_method(char* alloc_instruction) {
    // rather than having to calculate everything again like we did in
    // build_stack_alloc_for_method(), we can just copy the
    // allocation instruction char for char and change the first
    // three chars from "sub" to "add"
    char* dealloc = strdup(alloc_instruction);
    memcpy(dealloc, "add", strlen("add"));
   
    return dealloc;
}

/* 
 * generate the .equ instructions that go at the end of the .s file
 */
struct asm_node* get_equ_instructions(char* method_name) {
    // find the table that we're supposed to look in
    struct symbol_table* table = find_table(method_name, global);
    if (table == NULL) {
        printf("ERROR: get_equ_instructions(): couldn't find table for this method\n");
        return NULL;
    }

    // we need to generate an equ instruction for:
        // each of the parameters to this function
        // each of the local variables for this function
        // each of the compiler temp variables that we needed for this function

    struct asm_node* head = NULL; // the head of the equ instruction list
    char* current;
    struct asm_node* current_node;
    int current_offset = 0;
    char* current_offset_name;

    int start = 0;
    for (int i = start; i < table->numEntries; i++) {
        // figure out what the name of the offset for this variable is
        current_offset_name = get_offset_name(table->symbol_table[i]->id, table->table_name);

        // if the current offset name is a number, then we don't want to 
        // generate an instruction for this because it's a formal argument
        if (current_offset_name[0] >= '0' && current_offset_name[0] <= '9') {
            current_offset += 4;
            continue;
        }

        /* build the node's instruction */
        // find the length of the offset integer
        int current_offset_length;
        if (current_offset == 0) {
            current_offset_length = 1;
        }
        else {
            current_offset_length = (int)(log10(current_offset) + 1);
        }

        // get memory for this line
        current = malloc(sizeof(char) * (strlen(".equ ") + strlen(current_offset_name) + strlen(", ") + current_offset_length + 1));

        // copy in the ".equ "
        memcpy(current, ".equ ", strlen(".equ "));

        // copy in the offset name
        memcpy(current + strlen(".equ "), current_offset_name, strlen(current_offset_name));

        // copy in the ", "
        memcpy(current + strlen(".equ ") + strlen(current_offset_name), ", ", strlen(", "));

        // copy in the offset integer
        snprintf(current + strlen(".equ ") + strlen(current_offset_name) + strlen(", "), current_offset_length + 1, "%d", current_offset);

        // null terminate the string
        current[strlen(".equ ") + strlen(current_offset_name) + strlen(", ") + current_offset_length] = '\0';

        // make the asm node for this
        current_node = make_asm_node(current);

        // either make the head of the list or add to the list
        if (head == NULL) {
            head = current_node;
        }
        else {
            head = cat_asm_lists(head, current_node);
        }

        // increment the current offset for the next variable
        current_offset += 4; // increment by a single word
    }

    return head;
}

/* 
 * add the comment as a new node in front of the list
 */
struct asm_node* add_comment(struct asm_node* list, char* comment) {
    // check arguments
    if ((list == NULL) || (comment == NULL)) {
        printf("ERROR: add_comment(): one of the arguments was NULL\n");
        return list; // so that we don't blow away the pointer if someone
                     // calls this like "list = add_comment(list, "hello");"
    }

    // create a new node for the comment
    struct asm_node* comm_node = make_asm_node(comment);

    // cat the original list onto this node
    comm_node = cat_asm_lists(comm_node, list);

    return comm_node;

}

/* 
 * builds a branch instruction that looks like "condition label".
 * really just so we don't have to deal with malloc and stuff ourselves
 */
char* build_branch_instruction(char* condition, char* label) {
    // check arguments
    if ((condition == NULL) || (label == NULL)) {
        printf("ERROR: build_branch_instruction(): one of the arguments was NULL\n");
        return NULL;
    }

    // now we can allocate memory for the instruction and populate it
    char* instr = malloc(sizeof(char) * (strlen(condition) + strlen(label) + 2));
    memcpy(instr, condition, strlen(condition));
    memcpy(instr + strlen(condition), " ", 1);
    memcpy(instr + strlen(condition) + 1, label, strlen(label));
    instr[strlen(condition) + 1 + strlen(label) - 1] = '\0'; // overwrite the ':' so that
                                                             // it doesn't get printed

    return instr;
}

/* 
 * returns a label in the format of "_labelN", so that we can use
 * it in the assembly code and be sure that it doesn't conflict with other
 * labels
 */
char* get_new_label() {
    // figure out what the label num will be
    int label_num = num_labels;

    // figure out how long that int will be
    int label_num_length;
    if (label_num == 0) {
        label_num_length = 1;
    }
    else {
        label_num_length = (int)(log10(label_num) + 1);
    }

    // get some memory for the name and copy the stuff in
    char* label = malloc(sizeof(char) * (strlen("_label") + label_num_length + 1 + 1));
    memcpy(label, "_label", strlen("_label"));
    snprintf(label + strlen("_label"), label_num_length + 1, "%d", label_num);
    label[strlen("_label") + label_num_length] = ':';
    label[strlen("_label") + label_num_length + 1] = '\0';

    // because we got a new label
    num_labels++;

    return label;
}

/* 
 * generates code for a comparison operation, depending on the operation requested
 * i.e. == vs != vs <= vs >= vs < vs > 
 * doesn't handle && or || or !Expr because those are special
 */
struct asm_node* gen_code_for_comparison(struct node* expression, char* table_name) {
    // check arguments
    if ((expression == NULL) || (table_name == NULL)) {
        printf("ERROR: gen_code_for_comparison(): argument was NULL\n");
        return NULL;
    }

    // this has two children, the left and right sides of the comparison

    // get the instructions that put the lhs value in a temp var
    struct asm_node* lhs = evaluate_expression(expression->children[0], table_name);

    // figure out the temp name for that result
    char* lhs_name = get_last_temp_name();

    // get the instructions that put the rhs value in a temp var
    struct asm_node* rhs = evaluate_expression(expression->children[1], table_name);

    // figure out the temp name for that result
    char* rhs_name = get_last_temp_name();

    // now that we have the values for both sides, we just have to generate
    // a compare and bne and such. we want:
    // ldr r0, [sp, lhs_offset]
    // ldr r1, [sp, rhs_offset]
    // cmp r0, r1
    // bne _label_for_false (or beq or bge, etc)
    // mov r0, #1 @ move a true value into r0
    // str r0, [sp, _tempn_offset] 
    // b _label_for_end
    // _label_for_false:
    // mov r0, #0 @ move a false value into r0
    // str r0, [sp, _tempn_offset]
    // _label_for_end:

    // build instruction for loading left and right
    struct asm_node* load_left = make_asm_node(build_instruction("ldr", 0, lhs_name));
    struct asm_node* load_right = make_asm_node(build_instruction("ldr", 1, rhs_name));

    // add a comment to the load_left so that the comment goes in the right place
    switch (expression->nodeType) {
        case EXPRESSION_EQUAL_EQUAL: ;
            load_left = add_comment(load_left, "\n@ compare == and store result in temp var");
            break;
        case EXPRESSION_NOT_EQUAL: ;
            load_left = add_comment(load_left, "\n@ compare != and store result in temp var");
            break;
        case EXPRESSION_GREATER_EQUAL: ;
            load_left = add_comment(load_left, "\n@ compare >= and store result in temp var");
            break;
        case EXPRESSION_LESS_EQUAL: ;
            load_left = add_comment(load_left, "\n@ compare <= and store result in temp var");
            break;
        case EXPRESSION_GREATER: ;
            load_left = add_comment(load_left, "\n@ compare > and store result in temp var");
            break;
        case EXPRESSION_LESS: ;
            load_left = add_comment(load_left, "\n@ compare < and store result in temp var");
            break;
        default: 
            printf("ERROR: gen_code_for_comparison(): invalid switch case\n");
            return NULL;
            break;
    }

    // build instruction for the compare
    struct asm_node* compare = make_asm_node("cmp r0, r1");

    // get a branch label and use that to build the branch instruction
    char* label1 = get_new_label();

    // build the branch instruction which branches to the false label if the cmp was 0
    struct asm_node* branch_instr;
    switch (expression->nodeType) {
        case EXPRESSION_EQUAL_EQUAL: ;
            branch_instr = make_asm_node(build_branch_instruction("bne", label1));
            break;
        case EXPRESSION_NOT_EQUAL: ;
            branch_instr = make_asm_node(build_branch_instruction("beq", label1));
            break;
        case EXPRESSION_GREATER_EQUAL: ;
            branch_instr = make_asm_node(build_branch_instruction("blt", label1));
            break;
        case EXPRESSION_LESS_EQUAL: ;
            branch_instr = make_asm_node(build_branch_instruction("bgt", label1));
            break;
        case EXPRESSION_GREATER: ;
            branch_instr = make_asm_node(build_branch_instruction("ble", label1));
            break;
        case EXPRESSION_LESS: ;
            branch_instr = make_asm_node(build_branch_instruction("bge", label1));
            break;
        default: 
            printf("ERROR: gen_code_for_comparison(): invalid switch case\n");
            return NULL;
            break;
    }

    // get a temp variable name to store the result in
    char* temp_name = get_temp_name(table_name);

    // build the mov and store instructions
    struct asm_node* node1 = make_asm_node("mov r0, #1"); // put a true value
    struct asm_node* node2 = make_asm_node(build_instruction("str", 0, temp_name));

    // cat some stuff together so we can reuse variables
    lhs = cat_asm_lists(lhs, rhs);
    lhs = cat_asm_lists(lhs, load_left);
    lhs = cat_asm_lists(lhs, load_right);
    lhs = cat_asm_lists(lhs, compare);
    lhs = cat_asm_lists(lhs, branch_instr);
    lhs = cat_asm_lists(lhs, node1);
    lhs = cat_asm_lists(lhs, node2);

    // get a new label for the end
    char* end = get_new_label(); 

    // build the unconditional branch to the end
    node1 = make_asm_node(build_branch_instruction("b", end));

    // build the label for the ne
    node2 = make_asm_node(label1);

    // cat more things so we can reuse variables
    lhs = cat_asm_lists(lhs, node1);
    lhs = cat_asm_lists(lhs, node2);

    // build the move and store instructions
    node1 = make_asm_node("mov r0, #0"); // put a false value
    node2 = make_asm_node(build_instruction("str", 0, temp_name));

    // build the label for the end. we need to add the colon back first
    struct asm_node* node3 = make_asm_node(end);

    // now cat the rest of the things on
    lhs = cat_asm_lists(lhs, node1);
    lhs = cat_asm_lists(lhs, node2);
    lhs = cat_asm_lists(lhs, node3);

    return lhs;


}

/* 
 * build an instruction like "label: .asciz "string_literal"",
 * add that instruction to the list of such instructions which
 * need to be printed at the top of the file,
 * and return the label which can be used to refer to this string literal
 */
char* build_string_literal_instruction(char* string_literal) {
    // check arguments
    if (string_literal == NULL) {
        printf("ERROR: build_string_literal_instruction(): string_literal was NULL\n");
        return NULL;
    }

    // figure out what number string literal this is
    int num_literal = num_addition_string_literal_instructions;

    // get the length of the number that we're going to tack on
    int num_literal_length;
    if (num_literal == 0) {
        num_literal_length = 1;
    }
    else {
        num_literal_length = (int)(log10(num_literal) + 1);
    }

    // now that we know all of these things, we can allocate memory for this string
    char* instr = malloc(sizeof(char) * (strlen("str_literal: .asciz \"\"") + num_literal_length + 1 + strlen(string_literal)));

    // copy in the "str_literal"
    memcpy(instr, "str_literal", strlen("str_literal"));

    // copy in the number
    snprintf(instr + strlen("str_literal"), num_literal_length + 1, "%d", num_literal);

    // copy in the ": .asciz \""
    memcpy(instr + strlen("str_literal") + num_literal_length, ": .asciz \"", strlen(": .asciz \""));

    // copy in the string that we actually want to put there
    memcpy(instr + strlen("str_literal") + num_literal_length + strlen(": .asciz \""), string_literal, strlen(string_literal));

    // copy in the "\""
    instr[strlen("str_literal") + num_literal_length + strlen(": .asciz \"") + strlen(string_literal)] = '"';


    // record this in the table so that we can print it at the top later
    additional_string_literal_instructions[num_addition_string_literal_instructions] = instr;

    // since we allocated a new one
    num_addition_string_literal_instructions++;

    // now, we want to return a string which is just the label that we just created
    char* label = malloc(sizeof(char) * (strlen("str_literal") + num_literal_length + 1));
    memcpy(label, instr, strlen("str_literal") + num_literal_length);
    label[strlen("str_literal") + num_literal_length] = '\0';

    return label;
}

/* 
 * build an instruction like "ldr rD, =string_lit_label"
 */
char* build_load_string_lit_address_instruction(int _register, char* string_lit_label) {

    // check arguments
    if ((_register < 0) || (_register > 5) || (string_lit_label == NULL)) {
        printf("ERROR: build_load_string_lit_address_instruction(): argument was invalid\n");
        return NULL;
    }

    // allocate some space for this command
    char* instr = malloc(sizeof(char) * (strlen("ldr rn, =") + strlen(string_lit_label) + 1));

    // copy in the "ldr r"
    memcpy(instr, "ldr r", strlen("ldr r"));

    // copy in the register number
    snprintf(instr + strlen("ldr r"), 2, "%d", _register);

    // copy in the ", ="
    memcpy(instr + strlen("ldr rn"), ", =", strlen(", ="));

    // copy the name of the label in
    memcpy(instr + strlen("ldr rn, ="), string_lit_label, strlen(string_lit_label));

    // don't forget to null terminate
    instr[strlen("ldr rn, =") + strlen(string_lit_label)] = '\0';

    return instr;
}

/* 
 * return the name of the class which this method lives under
 */
char* get_class_name(char* method_name) {
    // check arguments
    if (method_name == NULL) {
        printf("ERROR: get_class_name(): method_name was NULL\n");
        return NULL;
    }

    // we want to check every table until we find a table that has
    // an entry that matches the method name and has the field is_method==true

    // we want to check every child of the global table except for the first child
    // because the first child is the main method and the rest of the children
    // are classes
    for (int i = 1; i < global->num_children; i++) {
        // get the class table
        struct symbol_table* current_class = global->children[i];

        // now check every entry in the class's table to see if we can
        // find a method with the right name
        for (int j = 0; j < current_class->numEntries; j++) {
            // if the entry is a method, check the name
            if (current_class->symbol_table[j]->is_method) {
                if (strcmp(current_class->symbol_table[j]->id, method_name) == 0) {
                    // name matches, return the name of the table
                    return current_class->table_name;
                }
            }
        }

        // if we got down here, we didn't find the method with the right name. so we
        // should check the next child. that gets done on the next iteration
    }

    // if we made it down here, we didn't find a method in any of the classes that
    // has this name. this shouldn't happen
    printf("ERROR: get_class_name(): couldn't find a class that owns this method\n");
    return NULL;
}

/* 
 * builds the label that'll be used to reference a class's method
 */
char* build_method_label(char* class_name, char* method_name) {
    // check arguments
    if ((class_name == NULL) || (method_name == NULL)) {
        printf("ERROR: build_method_label(): argument was NULL\n");
        return NULL;
    }

    // allocate some memory for the string we'll return
    char* label = malloc(sizeof(char) * (strlen(class_name) + strlen(method_name) + strlen("__:") + 1));

    // copy in an underscore
    label[0] = '_';

    // copy in the class name
    memcpy(label + strlen("_"), class_name, strlen(class_name));

    // copy in a "_"
    memcpy(label + strlen("_") + strlen(class_name), "_", strlen("_"));

    // copy in the method name
    memcpy(label + strlen("_") + strlen(class_name) + strlen("_"), method_name, strlen(method_name));

    // copy in the colon
    label[strlen("_") + strlen(class_name) + strlen("_") + strlen(method_name)] = ':';
    
    // null terminate the string
    label[strlen("_") + strlen(class_name) + strlen("_") + strlen(method_name) + strlen(":")] = '\0';

    return label;
}

/* 
 * returns a count of the number of arguments to the function specified
 */
int count_arguments(char* method_name) {
    // check arguments
    if (method_name == NULL) {
        printf("ERROR: count_arguments(): method_name was NULL\n");
        return -1;
    }

    // find the table for this method
    struct symbol_table* table = find_table(method_name, global);
    if (table == NULL) {
        printf("ERROR: count_arguments(): find_table() returned NULL\n");
        return -1;
    }

    // now that we know what table to look in, count the number of formal arguments
    int num_args = 0;
    for (int i = 0; i < table->numEntries; i++) {
        if (strncmp(table->symbol_table[i]->id, "__formal", strlen("__formal")) == 0) {
            // if the argument name starts with __formal, then it's a formal argument
            num_args++;
        }
    }

    return num_args;
}

/* 
 * get all the ".equ _tempn_offset, n" instructions for all the methods
 */
struct asm_node* get_all_equ_offsets() {
    // basically, we just want to call get_equ_instructions() for each
    // of these methods

    // the first method is the main method
    struct asm_node* main_offsets = get_equ_instructions(global->children[0]->table_name);

    // now get the rest of them
    for (int i = 1; i < global->num_children; i++) {
        // current entry is a class's symbol table
        struct symbol_table* current = global->children[i];

        for (int j = 0; j < current->num_children; j++) {
            struct symbol_table* current_method = current->children[j];
            struct asm_node* method_instructions = get_equ_instructions(current_method->table_name);
            main_offsets = cat_asm_lists(main_offsets, method_instructions);
        }
    }

    return main_offsets;
}
