#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "SymTable.h"     // Χρειάζεται
#include "consts.h"

double numConsts[MAX_NUM_CONSTS];
unsigned totalNumConsts = 0;

const char* stringConsts[MAX_STRING_CONSTS];
unsigned totalStringConsts = 0;

SymEntry* userFuncs[MAX_USERFUNC_CONSTS];  
unsigned totalUserFuncs = 0;

const char* libFuncs[MAX_LIBFUNC_CONSTS];
unsigned totalLibFuncs = 0;

unsigned consts_newnumber(double num) {
    for (unsigned i = 0; i < totalNumConsts; ++i) {
        if (numConsts[i] == num)
            return i;
    }
    assert(totalNumConsts < MAX_NUM_CONSTS);
    numConsts[totalNumConsts] = num;
    return totalNumConsts++;
}

unsigned consts_newstring(const char* str) {
    for (unsigned i = 0; i < totalStringConsts; ++i) {
        if (strcmp(stringConsts[i], str) == 0)
            return i;
    }
    assert(totalStringConsts < MAX_STRING_CONSTS);
    stringConsts[totalStringConsts] = strdup(str);
    return totalStringConsts++;
}

unsigned consts_newfunc(SymEntry* sym) {   
    for (unsigned i = 0; i < totalUserFuncs; ++i) {
        if (userFuncs[i] == sym)
            return i;
    }
    assert(totalUserFuncs < MAX_USERFUNC_CONSTS);
    userFuncs[totalUserFuncs] = sym;
    return totalUserFuncs++;
}

unsigned consts_newlibfunc(const char* name) {
    for (unsigned i = 0; i < totalLibFuncs; ++i) {
        if (strcmp(libFuncs[i], name) == 0)
            return i;
    }
    assert(totalLibFuncs < MAX_LIBFUNC_CONSTS);
    libFuncs[totalLibFuncs] = strdup(name);
    return totalLibFuncs++;
}

const char* consts_getlibfunc(unsigned index) {
    assert(index < totalLibFuncs);
    return libFuncs[index];
}

