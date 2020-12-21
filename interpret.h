#include "node.h"
#include "typecheck.h"

/* so that we can use FILE* in function declarations here */
#include <stdio.h>

/* struct definitions */
struct asm_node {
    char* instruction; // the instruction that we will print when we print them all
    struct asm_node* next; // the instruction that comes after this one
    struct asm_node* prev; // probably not going to use
};

/* function definitions */
void interpret_program(struct node* program, struct symbol_table* g, char* input_name);
struct asm_node* interpret_main_method(struct node* mainclass);
struct asm_node* interpret_statement_list(struct node* statementlist, char* table_name);
struct asm_node* interpret_class_decl_list(struct node* classdecl_list);
struct asm_node* interpret_statement(struct node* statement, char* table_name);
struct asm_node* interpret_assignment(struct node* assignment, char* table_name);
struct asm_node* interpret_methodcall(struct node* methodcall, char* table_name);
struct asm_node* interpret_vardecl(struct node* vardecl, char* table_name);
struct asm_node* interpret_vardecl_list(struct node* list, char* table_name);
struct asm_node* interpret_vardecl_list_piece_list(struct node* list, char* table_name);
struct asm_node* interpret_vardecl_list_piece(struct node* piece, char* table_name);
struct asm_node* interpret_oned_arraydecl(struct node* arraydecl, char* table_name);
void interpret_twod_arraydecl(struct node* arraydecl, char* table_name);
struct asm_node* interpret_oned_array_assignment(struct node* assignment, char* table_name);
void interpret_twod_array_assignment(struct node* assignment, char* table_name);
struct symbol_table_entry* get_symbol_tab_entry(char* id, char* table_name);
struct asm_node* evaluate_expression(struct node* expression, char* table_name);
struct asm_node* interpret_classdecl(struct node* classdecl);
struct asm_node* interpret_methoddecl_list(struct node* methoddecl_list);
struct asm_node* interpret_methoddecl(struct node* methoddecl);
char* get_last_temp_name();
char* get_temp_name(char* table_name);
char* build_instruction(const char* op, int _register, char* offset_name);
char* build_mov_instruction(int _register, int constant);
char* build_branch_instruction(char* condition, char* label);
char* build_stack_alloc_for_method(char* method_name);
char* build_stack_dealloc_for_method(char* method_name);
struct asm_node* make_asm_node(char* instruction);
struct asm_node* cat_asm_lists(struct asm_node* list1, struct asm_node* list2);
void print_asm_list(struct asm_node* list, FILE* out);
char* get_offset_name(char* id, char* table_name);
struct asm_node* get_equ_instructions(char* method_name);
struct asm_node* add_comment(struct asm_node* list, char* comment);
char* get_new_label();
struct asm_node* gen_code_for_comparison(struct node* expression, char* table_name);
char* build_string_literal_instruction(char* string_literal);
char* build_load_string_lit_address_instruction(int _register, char* string_lit_label);
char* get_class_name(char* method_name);
char* build_method_label(char* class_name, char* method_name);
int count_arguments(char* method_name);
struct asm_node* get_all_equ_offsets();
struct asm_node* build_constructor(struct node* classdecl);
