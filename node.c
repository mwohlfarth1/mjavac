#include "node.h"
#include <stdlib.h>

struct node* new_node(int nodeType){
    struct node* ast_node = malloc(sizeof(struct node));
    ast_node->nodeType = nodeType;
    ast_node->numOfChildren = 0;
    ast_node->data.lineNum = yylineno;
    ast_node->data.type = "undefined"; // default to undefined type
    ast_node->data.num_uses = 0;
    return ast_node;
}

void add_child(struct node* parent, struct node* child){
    parent -> children[parent->numOfChildren] = child;
    parent->numOfChildren++;
}

void setStringValue(struct node* node, char* s){
    node->data.type = "string";
    node->data.value.stringValue = s;
}

void setIntValue(struct node* node, int i){
    node->data.type = "int";
    node->data.value.intValue = i;
}

void setBooleanValue(struct node* node, bool b){
    node->data.type = "boolean";
    node->data.value.booleanValue = b;
}



