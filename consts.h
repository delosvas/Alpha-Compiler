#ifndef CONSTS_H
#define CONSTS_H

#include <stdio.h>
#include "SymTable.h"  // περιλαμβάνει τον ορισμό του SymEntry

#define MAX_NUM_CONSTS 1024
#define MAX_STRING_CONSTS 1024
#define MAX_USERFUNC_CONSTS 1024
#define MAX_LIBFUNC_CONSTS 1024

extern double numConsts[MAX_NUM_CONSTS];
extern const char* stringConsts[MAX_STRING_CONSTS];
extern SymEntry* userFuncs[MAX_USERFUNC_CONSTS];     // ΔΙΟΡΘΩΜΕΝΟ
extern const char* libFuncs[MAX_LIBFUNC_CONSTS];

unsigned consts_newnumber(double num);
unsigned consts_newstring(const char* str);
unsigned consts_newfunc(SymEntry* sym);              // ΔΙΟΡΘΩΜΕΝΟ
unsigned consts_newlibfunc(const char* name);
const char* consts_getlibfunc(unsigned index);

#endif


