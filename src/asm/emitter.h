#define EMITTER_H

#ifndef PARSER_H
#include "parser.h"
#endif

typedef struct LabelEntry {
    char *lbl_name;
    int offset;
} LabelEntry;

typedef struct LabelVec {
    int len, cap;
    LabelEntry *data;
} LabelVec;

typedef struct EmitErr {
    int is_err;
    char *msg;
    int line;
} EmitErr;

LabelVec find_labels(ParseNode p[], int num_nodes, EmitErr *err);
void emit_all(char *buf, ParseNode p[], int num_nodes, LabelVec labels, EmitErr *err);