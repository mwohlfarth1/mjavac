#include "node.h"

/* for variadic arguments to debug */
#include <stdarg.h> 

/* function definitions */
void build_symbol_table(struct node* program, struct symbol_table* global);
void build_main_class_symbol_table(struct node* main, struct symbol_table* main_symbol_table);
void build_class_decl_list_tables(struct node* class_decl_list);
void build_class_symbol_table(struct node* class_decl);
void build_method_list_tables(struct node* method_list, struct symbol_table* class_symbol_table);
void build_method_symbol_table(struct node* method_decl, struct symbol_table* class_symbol_table);
void build_formallist_symbol_table(struct node* formal_list, struct symbol_table* method_table);
void build_statement_list_symbol_table(struct node* root, struct symbol_table* table);
void build_statement_symbol_table(struct node* statement, struct symbol_table* table);
void build_expression_symbol_table(struct node* expression, struct symbol_table* table);
void build_expression_piece_symbol_table(struct node* piece, struct symbol_table* table);
void build_assignment_symbol_table(struct node* assignment, struct symbol_table* table);
void build_oned_array_assignment_symbol_table(struct node* oned, struct symbol_table* table);
void build_twod_array_assignment_symbol_table(struct node* twod, struct symbol_table* table);
void build_methodcall_symbol_table(struct node* methodcall, struct symbol_table* table);
void build_arglist_symbol_table(struct node* arglist, struct symbol_table* table);
void build_arg_symbol_table(struct node* arg, struct symbol_table* table);
void build_vardecl_symbol_table(struct node* vardecl, struct symbol_table* table);
void build_vardecl_list_symbol_table(struct node* vardecl_list, struct symbol_table* table);
void build_vardecl_list_piece_list_symbol_table(struct node* vardecl_piece_list, struct symbol_table* table, char* type);
void build_vardecl_list_piece_symbol_table(struct node* vardecl_list_piece, struct symbol_table* table, char* type);
void build_oned_arraydecl_symbol_table(struct node* oned, struct symbol_table* table);
void build_twod_arraydecl_symbol_table(struct node* twod, struct symbol_table* table);
bool id_exists_in_scope(char* id, struct symbol_table* table);
bool id_exists_in_scope_or_higher(char* id, struct symbol_table* table);
void add_to_symbol_table_tc(char* id, struct data data, int is_array, int is_class_var, int is_method, struct symbol_table* table, struct twod_arr* array, int array_dims);
bool class_var_is_in_symbol_table(char* id, struct symbol_table* table);
void print_symbol_tables(struct symbol_table* table);
struct symbol_table* find_table(char* table_name, struct symbol_table* current);
bool is_valid_type(char* type_name);
void printAST(struct node* root, int depth, int child_number);
void checkMain(struct node* mainClass);
void checkStatement(struct node* statement);
void checkStatementList(struct node* statementList);
bool isMatchingType(int lhs_type, int rhs_type);
bool isUniqueVariableName(char* id);
void checkExpr(struct node* expr);
int checkTypes(struct node* root);
void check_types(struct node* root);
void fix_sym_tab_item_type(char * id, int type);
int get_line_num_from_sym_tab(char * id);
char* get_type_name(int type_number);
char* get_datatype_name(struct node* type);
void debug(const char* fmt, ...);
bool already_in_symbol_table(char* id);
struct symbol_table_entry* get_sym_tab_entry(char* id);
void fix_sym_tab_line_num(char* id, int line_num);
void replace_entry_in_symbol_table(char* id, struct data data);
void add_use_line_num(char* id, int line_num);
void clean_up_symbol_table();

/* global variables */
int numErrors;
struct symbol_table* global; // the symbol table for the global scope
