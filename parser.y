%{
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include "SymTable.h"
  #include "quad.h"

  // Maximum nesting level for loops (for break/continue handling)
  #define MAX_NESTING 32
  #define YY_DECL int alpha_yylex (void* ylval)
  extern int alpha_yylex(void*);

  // Function declarations for expression handling and code generation
  expr* reverse_expr_list(expr* head);  // Reverses a linked list of expressions
  void emit_param_list(expr* el);       // Emits parameter list for function calls
  expr* convertToBoolIfNeeded(expr* e); // Converts expression to boolean if needed
  expr* newexpr_conststring(char* s);   // Creates new string constant expression
  expr* lvalue_expr(struct expr* e);    // Handles left-value expressions
  expr* newexpr(expr_t t);             // Creates new expression of given type
  expr* convertBoolToTemp(expr* e);    // Convert boolean to temporary with true/false assignments
  int yyerror (char* yaccProvidedMessage); // Error handling function
  int yylex(void);                     // Lexical analyzer function

  // Global variables for break/continue handling in nested loops
  unsigned int* breakList = NULL;
  unsigned int* continueList = NULL;
  unsigned int* breakStack[MAX_NESTING];
  unsigned int* continueStack[MAX_NESTING];
  int loopDepth = 0;  // Current nesting depth

  // Global variables for symbol table and scope management
  int dollar_counter;    // Counter for anonymous functions
  char buffer[32];        // Buffer for temporary strings
  int scope;            // Current scope level
  int notInsert;        // Flag to control symbol insertion
  int global_LIB_use;   // Flag for library function usage
  int not_accessible;   // Flag for accessibility check
  int infunc;          // Flag indicating if we're inside a function
  int call_depth;      // Current function call depth
  int islibfunc;       // Flag for library function
  int prev;            // Previous state tracking
  int token_is_func;   // Flag for function tokens
  int glo_bal;         // Global scope flag
 int dollar_counter=0; 
  SymEntry* tmp_rec;   // Temporary symbol record

  // External variables from lexer
  extern int yylineno;
  extern char* yytext;
  extern FILE* yyin;
  extern FILE* yyout;
%}

%union {
    int                         intval;
    char*                       strVal;
    double                      doubleVal;
    struct expr*                exprVal;
    struct call_t*              callVal;
    struct stmt_t*              stmtVal;
    struct forprefix*           forprefixVal;
    struct SymbolTableEntry*    symVal;
}

%start program

%token IF ELSE WHILE FOR FUNCTION RETURN BREAK CONTINUE
%token AND NOT OR LOCAL TRUE FALSE NIL
%token EQUAL INEQUAL INCREMENT DECREMENT
%token GREATER GREATER_EQUAL LESS LESS_EQUAL
%token ADD MINUS MULTIPLICATION DIVISION MODULO
%token ASSIGN
%token LEFT_CURLY_BRACKET RIGHT_CURLY_BRACKET
%token LEFT_SQUARE_BRACKET RIGHT_SQUARE_BRACKET
%token LEFT_PARENTHESIS RIGHT_PARENTHESIS
%token SEMICOLON COMMA COLON DOUBLE_COLON DOT DOUBLE_DOT
%token SINGLE_LINE_COMMENT OTHERCHAR


%type <symVal> opt_ident
%token <strVal>     IDENT STRINGCONST 
%token <intval>     INTEGER
%token <doubleVal>  REAL

%type <exprVal>      lvalue
%type <exprVal>      assignexpr
%type <exprVal>      expr
%type <exprVal>      term
%type <exprVal>      stmt
%type <exprVal>      stmt_list
%type <exprVal>      ifstmt
%type <exprVal>      whilestmt
%type <exprVal>      forstmt
%type <exprVal>      returnstmt
%type <exprVal>      block
%type <exprVal>      funcdef
%type <exprVal>      primary
%type <exprVal>      member
%type <exprVal>      call
%type <exprVal>      elist
%type <exprVal>      elist_expressions
%type <exprVal>      objectdef
%type <exprVal>      indexed
%type <exprVal>      indexedelem_list
%type <exprVal>      indexedelem
%type <exprVal>      idlist
%type <exprVal>      idlist_list
%type <exprVal>      const
%type <forprefixVal> forprefix
%type <intval>       ifprefix
%type <intval> elseprefix
%type <intval>       M
%type <intval>       N
%right ASSIGN
%left  OR
%left  AND
%right NOT
%nonassoc EQUAL INEQUAL
%nonassoc GREATER GREATER_EQUAL LESS LESS_EQUAL
%left  ADD MINUS
%left  MULTIPLICATION DIVISION MODULO
%right UMINUS
%right DECREMENT INCREMENT
%left  DOT
%left  LEFT_SQUARE_BRACKET RIGHT_SQUARE_BRACKET
%left  LEFT_PARENTHESIS RIGHT_PARENTHESIS
%left  RIGHT_CURLY_BRACKET LEFT_CURLY_BRACKET
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

// Grammar rules for program structure
program: stmt_list
;
ifprefix:
    IF LEFT_PARENTHESIS expr RIGHT_PARENTHESIS {
        expr* cond = convertToBoolIfNeeded($3);
        if (!cond) {
            yyerror("null condition in if");
            YYABORT;
        }

        // patch true → body
        backpatch(cond->truelist, nextquadlabel());

        // αποθήκευσε για patch μετά το then ή else
        $$ = currQuad;
        emit(jump_, NULL, NULL, NULL, 0, yylineno);
    }
;

elseprefix:
    ELSE {
        $$ = currQuad; // position of jump after then-block
        emit(jump_, NULL, NULL, NULL, 0, yylineno);
    }
;

ifstmt:
    ifprefix stmt {
        patchlabel($1, nextquadlabel());  // patch μετά το body
        $$ = newexpr(boolexpr_e);
    }
  | ifprefix stmt elseprefix stmt {
        patchlabel($1, $3 + 1);           // false jump → else
        patchlabel($3, nextquadlabel()); // μετά το else
        $$ = newexpr(boolexpr_e);
    }
;




;

// While statement handling
whilestmt: WHILE M LEFT_PARENTHESIS expr RIGHT_PARENTHESIS N stmt {
    // Enter new loop scope
    ++loopDepth;
    breakStack[loopDepth] = NULL;
    continueStack[loopDepth] = NULL;
    
    // Convert condition to boolean if needed
    expr* cond = convertToBoolIfNeeded($4);
    
    // $2 (M) is the loop start (condition evaluation position)
    // $6 (N) is the body start marker  
    unsigned loopStart = $2;
    unsigned bodyStart = $6;
    
    // Backpatch condition: true goes to body, false goes to exit
    backpatch(cond->truelist, bodyStart);
    
    // Current position is after the loop body - this is where we exit
    unsigned exitLabel = nextquadlabel();
    backpatch(cond->falselist, exitLabel);
    
    // Emit jump back to condition (creates the loop)
    emit(jump_, NULL, NULL, NULL, loopStart, yylineno);
    
    // Handle break and continue statements accumulated during body parsing
    if (breakStack[loopDepth]) {
        backpatch(breakStack[loopDepth], exitLabel);  // break jumps to exit
        breakStack[loopDepth] = NULL;
    }
    if (continueStack[loopDepth]) {
        backpatch(continueStack[loopDepth], loopStart);  // continue jumps to condition
        continueStack[loopDepth] = NULL;
    }
    
    // Exit loop scope
    --loopDepth;
    
    $$ = newexpr(boolexpr_e);
}
;

// For prefix handling
forprefix:
    elist SEMICOLON M expr SEMICOLON M elist {
        

        // First M is the condition location
        $$->test = $3;

        // Convert condition to boolean
        expr* cond = $4;

        // Manual boolean conversion
        expr* temp_bool = newexpr(var_e);
        temp_bool->sym = newtemp();

        // Create proper expr* for the comparison
        expr* temp_var = newexpr(var_e);
        temp_var->sym = $4->sym;
        emit(if_less, temp_var, newexpr_constvar("N"), NULL, 0, yylineno);         // 2
        emit(assign, newexpr_constbool(0), NULL, temp_bool, 0, yylineno);        // 3
        emit(jump_, NULL, NULL, NULL, 0, yylineno);                              // 4
        emit(assign, newexpr_constbool(1), NULL, temp_bool, 0, yylineno);        // 5

        $$->enter = nextquadlabel();  // Store if_eq label (line 6)
        emit(if_eq, newexpr_constbool(1), temp_bool, NULL, nextquadlabel() + 3, yylineno);         // 6
        emit(jump_, NULL, NULL, NULL, 0, yylineno);                              // 7
    }
;

// For statement handling
forstmt:
    FOR LEFT_PARENTHESIS forprefix RIGHT_PARENTHESIS stmt {
        // Handle for loop
        ++loopDepth;
        breakStack[loopDepth] = NULL;
        continueStack[loopDepth] = NULL;

        // Patch IF that jumps into the loop body
        patchlabel($3->enter + 6, nextquadlabel());

        // Loop body
        // $5 is the stmt (body)

        // Continue jumps go to increment
        unsigned inc_label = nextquadlabel();

        // Handle loop counter
        SymbolTableEntry* sym_i = symtable_lookup_formal("i", scope);
        if (!sym_i) {
            symtable_insert_checked(scope, yylineno, USERFUNC, "i");
            sym_i = symtable_lookup_formal("i", scope);
        }

        // Generate increment code
        expr* e1 = newexpr(var_e); e1->sym = sym_i;
        expr* e2 = newexpr(var_e); e2->sym = sym_i;

        // i = i + 1
        emit(add, e1, newexpr_constnum(1), e2, 0, yylineno);

        // Jump back to test
        emit(jump_, NULL, NULL, NULL, $3->test, yylineno);

        // Patch exit
        patchlabel($3->enter + 7, nextquadlabel());

        // Handle break and continue
        backpatch(breakStack[loopDepth], nextquadlabel());
        backpatch(continueStack[loopDepth], inc_label);

        breakStack[loopDepth] = NULL;
        continueStack[loopDepth] = NULL;
        --loopDepth;

        $$ = newexpr(boolexpr_e);
    }
;

// Return statement handling
returnstmt:
    RETURN { notInsert = 0; } expr SEMICOLON {
        // Return with value
        emit(return_, $3, NULL, NULL, 0, yylineno);
        notInsert = 1;
        $$ = newexpr(boolexpr_e);
    }
  | RETURN SEMICOLON {
        // Return without value
        if (infunc) {  // Only emit return_ if we're in a function
            emit(return_, NULL, NULL, NULL, 0, yylineno);
        }
        $$ = newexpr(boolexpr_e);
    }
;

// Statement handling rules
stmt:
      assignexpr SEMICOLON         { $$ = $1; }  // Assignment statement
    | call SEMICOLON               { $$ = $1; }  // Function call
| expr SEMICOLON {
    // Handle expression statements
    if ($1 && $1->type == boolexpr_e && $1->truelist && $1->falselist) {
        // Boolean expression as statement - need to convert to value
        expr* temp = newexpr(var_e);
        temp->sym = newtemp();
        
        // Backpatch true to assign 1
        backpatch($1->truelist, nextquadlabel());
        emit(assign, newexpr_constbool(1), NULL, temp, 0, yylineno);
        emit(jump_, NULL, NULL, NULL, nextquadlabel() + 2, yylineno);
        
        // Backpatch false to assign 0  
        backpatch($1->falselist, nextquadlabel());
        emit(assign, newexpr_constbool(0), NULL, temp, 0, yylineno);
        
        $$ = temp;
    } else {
        $$ = $1;
    }
}
   | ifstmt                       { $$ = $1; }  // If statement
    | whilestmt                    { $$ = $1; }  // While loop
    | forstmt                      { $$ = $1; }  // For loop
    | returnstmt                   { $$ = $1; }  // Return statement
    | BREAK SEMICOLON {
        // Handle break statement - emit jump_ and add to break list
        emit(jump_, NULL, NULL, NULL, 0, yylineno);
        breakStack[loopDepth] = mergelist(breakStack[loopDepth], newlist(currQuad - 1));
        $$ = newexpr(boolexpr_e);
    }
    | CONTINUE SEMICOLON {
        // Handle continue statement - emit jump_ and add to continue list
        emit(jump_, NULL, NULL, NULL, 0, yylineno);
        continueStack[loopDepth] = mergelist(continueStack[loopDepth], newlist(currQuad - 1));
        $$ = newexpr(boolexpr_e);
    }
    | block                        { $$ = $1; }  // Block of statements
    | funcdef                      { $$ = $1; }  // Function definition
    | SEMICOLON                    { $$ = newexpr(boolexpr_e); }  // Empty statement
;

// Statement list handling
stmt_list:
      stmt stmt_list { $$ = $2; }  // Process statements sequentially
    | /* empty */    { $$ = NULL; } // Empty statement list
;

// Expression handling rules
expr:
        assignexpr { $$ = $1; }  // Assignment expression
      | expr ADD expr {
          // Addition operation
          expr* result = newexpr(arithexpr_e);
          result->sym = newtemp();
          emit(add, $1, $3, result, 0, yylineno);
          $$ = result;
        }
      | expr MINUS expr {
          // Subtraction operation
          expr* result = newexpr(arithexpr_e);
          result->sym = newtemp();
          emit(sub, $1, $3, result, 0, yylineno);
          $$ = result;
        }
      | expr MULTIPLICATION expr {
          // Multiplication operation
          expr* result = newexpr(arithexpr_e);
          result->sym = newtemp();
          emit(mul, $1, $3, result, 0, yylineno);
          $$ = result;
        }
      | expr DIVISION expr {
          // Division operation
          expr* result = newexpr(arithexpr_e);
          result->sym = newtemp();
          emit(div_, $1, $3, result, 0, yylineno);
          $$ = result;
        }
      | expr MODULO expr {
          // Modulo operation
          expr* result = newexpr(arithexpr_e);
          result->sym = newtemp();
          emit(mod, $1, $3, result, 0, yylineno);
          $$ = result;
        }
      | expr GREATER expr {
          // Greater than comparison - using generate_BOOL_EXPR path
          $$ = newexpr(boolexpr_e);
          $$->sym = newtemp();
          emit(if_greater, $1, $3, $$, 0, yylineno);
        }
      | expr GREATER_EQUAL expr {
          // Greater than or equal comparison - using generate_BOOL_EXPR path
          $$ = newexpr(boolexpr_e);
          $$->sym = newtemp();
          emit(if_greatereq, $1, $3, $$, 0, yylineno);
        }
      | expr LESS expr {
          // Less than comparison - using generate_BOOL_EXPR path
          $$ = newexpr(boolexpr_e);
          $$->sym = newtemp();
          emit(if_less, $1, $3, $$, 0, yylineno);
        }
      | expr LESS_EQUAL expr {
          // Less than or equal comparison - using generate_BOOL_EXPR path
          $$ = newexpr(boolexpr_e);
          $$->sym = newtemp();
          emit(if_lesseq, $1, $3, $$, 0, yylineno);
        }
      | expr EQUAL expr {
          // Equality comparison - stolen code approach
          $$ = newexpr(boolexpr_e);
          $$->sym = newtemp();
          unsigned true_label = nextquadlabel() + 3;
          unsigned end_label = nextquadlabel() + 4;
          emit(if_eq, $1, $3, NULL, true_label, yylineno);
          emit(assign, newexpr_constbool(0), NULL, $$, 0, yylineno);
          emit(jump_, NULL, NULL, NULL, end_label, yylineno);
          emit(assign, newexpr_constbool(1), NULL, $$, 0, yylineno);
        }

      | expr INEQUAL expr {
          // Inequality comparison - using generate_BOOL_EXPR path
          $$ = newexpr(boolexpr_e);
          $$->sym = newtemp();
          emit(if_noteq, $1, $3, $$, 0, yylineno);
        }
| expr AND expr {
    // Handle logical AND - simple approach
    $$ = newexpr(boolexpr_e);
    $$->sym = newtemp();
                   emit(and, $1, $3, $$, 0, yylineno);
}


     | expr OR expr {
    // Handle logical OR - simple approach
    $$ = newexpr(boolexpr_e);
    $$->sym = newtemp();
         emit(or_, $1, $3, $$, 0, yylineno);
}





| NOT expr %prec NOT {
    if ($2->type == boolexpr_e && $2->truelist && $2->falselist) {
        $$ = newexpr(boolexpr_e);
        $$->truelist = $2->falselist;
        $$->falselist = $2->truelist;
    } else {
        $$ = newexpr(boolexpr_e);
        $$->sym = newtemp();
        emit(not_, $2, NULL, $$, 0, yylineno);
    }
}



      | term {
          $$ = $1;  // Basic term
      }
;

// Term handling for basic expressions
term:
      IDENT {
          // Handle identifier - look up in symbol table or create new entry
          $$ = newexpr(var_e);
          $$->sym = symtable_lookup($1, scope);
          if (!$$->sym) {
              symtable_insert_checked(scope, yylineno, GLOBAL, $1);
              $$->sym = symtable_lookup($1, scope);
          }
      }
    | LEFT_PARENTHESIS expr RIGHT_PARENTHESIS {
          $$ = $2;  // Parenthesized expression
      }
    | MINUS expr %prec UMINUS {
          // Unary minus operation
          expr* result = newexpr(arithexpr_e);
          result->sym = newtemp();
          emit(uminus, $2, NULL, result, 0, yylineno);
          $$ = result;
      }
    
    | INCREMENT lvalue {
          // Pre-increment operation
          if (token_is_func) {
              yyerror("cannot increment function value");
              token_is_func = 0;
          }
          expr* result = newexpr(arithexpr_e);
          result->sym = newtemp();
          emit(add, $2, newexpr_constnum(1), result, 0, yylineno);
          emit(assign, result, NULL, $2, 0, yylineno);
          $$ = result;
      }
    | lvalue INCREMENT {
          // Post-increment operation
          if (token_is_func) {
              yyerror("cannot increment function value");
              token_is_func = 0;
          }
          expr* result = newexpr(arithexpr_e);
          result->sym = newtemp();
          emit(add, lvalue_expr($1), newexpr_constnum(1), result, 0, yylineno);
          emit(assign, result, NULL, lvalue_expr($1), 0, yylineno);
          $$ = result;
      }
    | DECREMENT lvalue {
          // Pre-decrement operation
          if (token_is_func) {
              yyerror("cannot decrement function value");
              token_is_func = 0;
          }
          expr* result = newexpr(arithexpr_e);
          result->sym = newtemp();
          emit(sub, lvalue_expr($2), newexpr_constnum(1), result, 0, yylineno);
          emit(assign, result, NULL, lvalue_expr($2), 0, yylineno);
          $$ = result;
      }
    | lvalue DECREMENT {
          // Post-decrement operation
          if (token_is_func) {
              yyerror("cannot decrement function value");
              token_is_func = 0;
          }
          expr* result = newexpr(arithexpr_e);
          result->sym = newtemp();
          emit(sub, lvalue_expr($1), newexpr_constnum(1), result, 0, yylineno);
          emit(assign, result, NULL, lvalue_expr($1), 0, yylineno);
          $$ = result;
      }
    | primary {
          $$ = $1;  // Primary expression
      }
;
assignexpr:
      member ASSIGN expr {
          // Table member assignment
          if (!$1 || $1->type != tableitem_e) {
              yyerror("invalid assignment to non-table item");
              $$ = newexpr(assignexpr_e);
              $$->sym = NULL;
          } else {
              expr* rhs = $3;
              
              // Handle boolean expressions in assignments
              if (rhs->type == boolexpr_e && rhs->sym) {
                  // Direct boolean result - no conversion needed
                  // The boolean expression already has a result in rhs->sym
              } else {
                  emit(tablesetelem, $1->index, rhs, $1, 0, yylineno);
              }
              
              $$ = newexpr(assignexpr_e);
              $$->sym = $1->sym;
          }
      }
    | lvalue ASSIGN expr {
          // Regular assignment
          if ($1 != NULL && $1->type == USERFUNC) {
              yyerror("variable already defined as function");
              $$ = newexpr(assignexpr_e);
              $$->sym = NULL;
          } else {

          expr* lv = $1;
          expr* e  = $3;

          if (lv && e) {

          // Handle boolean expressions in assignments
if (e->type == boolexpr_e && e->sym) {
    // Direct boolean result - no conversion needed
    // The boolean expression already has a result in e->sym
}


          if (lv->type == tableitem_e) {
              emit(tablesetelem, lv->index, e, lv, 0, yylineno);
          } else {
              emit(assign, e, NULL, lv, 0, yylineno);
          }

          // Create result temporary for assignment expression
          expr* result = newexpr(var_e);
          result->sym = newtemp();
          emit(assign, lv, NULL, result, 0, yylineno);

          $$ = newexpr(assignexpr_e);
          $$->sym = result->sym;
          } else {
              yyerror("invalid assignment");
              $$ = newexpr(assignexpr_e);
              $$->sym = NULL;
          }
      }
      };

// Primary expression handling
primary:
        lvalue {
          $$ = $1;  // Variable or table item
        }
      | member {
          $$ = $1;  // Must exist for nested expressions
        }
      | call {
          $$ = $1;  // Function call
        }
      | objectdef {
          $$ = $1;  // Object definition
        }
      | LEFT_PARENTHESIS funcdef RIGHT_PARENTHESIS {
          $$ = $2;  // Anonymous function definition
        }
      | const {
          $$ = $1;  // Constant value
        }
;

// Left value handling
lvalue:
    IDENT {
        // Handle identifier - look up in symbol table or create new entry
        $$ = newexpr(var_e);
        $$->sym = symtable_lookup($1, scope);

        if ($$->sym) {
            if ($$->sym->type == LIBFUNC) {
                $$->type = libraryfunc_e;
            }
        }

        if (!$$->sym) {
            // If not found, check if it's a LIBFUNC at global scope (scope 0)
            $$->sym = symtable_lookup($1, 0);

            if ($$->sym && $$->sym->type == LIBFUNC) {
                // It's a library function - set the correct expression type
                $$->type = libraryfunc_e;
            } else if (!$$->sym) {
                // If not even a LIBFUNC, insert as GLOBAL
                symtable_insert_checked(scope, yylineno, GLOBAL, $1);
                $$->sym = symtable_lookup($1, scope);
            }
        }
    }
    | lvalue LEFT_SQUARE_BRACKET expr RIGHT_SQUARE_BRACKET {
          // Array indexing
          $$ = newexpr(tableitem_e);
          $$->sym = newtemp();
          $$->index = $3;
          emit(tablegetelem, $1, $3, $$, 0, yylineno);
        }
    | lvalue DOT IDENT {
          // Object member access
          $$ = newexpr(tableitem_e);
          $$->sym = newtemp();
          $$->index = newexpr_conststring($3);
          emit(tablegetelem, $1, $$->index, $$, 0, yylineno);
        }
    | lvalue LEFT_SQUARE_BRACKET STRINGCONST RIGHT_SQUARE_BRACKET {
          // String-indexed array access
          $$ = newexpr(tableitem_e);
          $$->sym = newtemp();
          $$->index = newexpr_conststring($3);
          emit(tablegetelem, $1, $$->index, $$, 0, yylineno);
        }
;

// Member access handling
member:
        primary DOT IDENT {
            // Object member access through primary expression
            expr* tmp = emit_iftableitem($1);
            $$ = newexpr(tableitem_e);
            $$->sym = tmp->sym;
            $$->index = newexpr_conststring($3);
        }
      | member DOT IDENT {
            // Nested object member access
            expr* tmp = emit_iftableitem($1);
            $$ = newexpr(tableitem_e);
            $$->sym = tmp->sym;
            $$->index = newexpr_conststring($3);
        }
      | primary LEFT_SQUARE_BRACKET expr RIGHT_SQUARE_BRACKET {
            // Array indexing through primary expression
            expr* tmp = emit_iftableitem($1);
            $$ = newexpr(tableitem_e);
            $$->sym = tmp->sym;
            $$->index = $3;
        }
      | member LEFT_SQUARE_BRACKET expr RIGHT_SQUARE_BRACKET {
            // Nested array indexing
            expr* tmp = emit_iftableitem($1);
            $$ = newexpr(tableitem_e);
            $$->sym = tmp->sym;
            $$->index = $3;
        }
;

// Function call handling
call:
    lvalue LEFT_PARENTHESIS elist RIGHT_PARENTHESIS {
        // Regular function call
        $$ = newexpr(call_e);
        $$->sym = newtemp();

        emit_param_list($3);                     // Emit parameters
        emit(call, $1, NULL, NULL, 0, yylineno); // Make the call
        emit(getretval, NULL, NULL, $$, 0, yylineno); // Get return value
    }
  | member LEFT_PARENTHESIS elist RIGHT_PARENTHESIS {
        // Method call
        $$ = newexpr(call_e);
        $$->sym = newtemp();

        emit_param_list($3);
        emit(call, $1, NULL, NULL, 0, yylineno);
        emit(getretval, NULL, NULL, $$, 0, yylineno);
    }
  | LEFT_PARENTHESIS funcdef RIGHT_PARENTHESIS LEFT_PARENTHESIS elist RIGHT_PARENTHESIS {
        // Anonymous function call
        $$ = newexpr(call_e);
        $$->sym = newtemp();

        emit_param_list($5);
        emit(call, $2, NULL, NULL, 0, yylineno);
        emit(getretval, NULL, NULL, $$, 0, yylineno);
    }
;

// Expression list handling
elist:
    expr elist_expressions {
        $$ = $1;
        if ($2) {
            $$->next = $2;  // Link expr with remaining expressions
        }
    }
  | { $$ = NULL; }  // Empty expression list
;

// Expression list continuation handling
elist_expressions:
    COMMA expr elist_expressions {
        $$ = $2;
        if ($3) {
            $$->next = $3;  // Link remaining expressions
        }
    }
  | COMMA expr {
        $$ = $2;
        $$->next = NULL;
    }
  | { $$ = NULL; }  // End of expression list
;

// Object definition handling
objectdef:
        LEFT_SQUARE_BRACKET elist RIGHT_SQUARE_BRACKET {
          // Object with initial values
          $$ = newexpr(newtable_e);
          $$->sym = newtemp();
          emit(tablecreate, NULL, NULL, $$, 0, yylineno);
        }
      | LEFT_SQUARE_BRACKET indexed RIGHT_SQUARE_BRACKET {
          // Object with indexed elements
          $$ = newexpr(newtable_e);
          $$->sym = newtemp();
          emit(tablecreate, NULL, NULL, $$, 0, yylineno);
        }
      | LEFT_SQUARE_BRACKET RIGHT_SQUARE_BRACKET {
          // Empty object
          $$ = newexpr(newtable_e);
          $$->sym = newtemp();
          emit(tablecreate, NULL, NULL, $$, 0, yylineno);
        }
;

// Indexed element handling
indexed:
        indexedelem indexedelem_list {
          $$ = $1;  // First indexed element
        }
      | indexedelem {
          $$ = $1;  // Single indexed element
        }
;

// Indexed element list handling
indexedelem_list:
        COMMA indexedelem indexedelem_list {
          $$ = $2;  // Link indexed elements
        }
      | COMMA indexedelem {
          $$ = $2;  // Last indexed element
        }
      | {
          $$ = NULL;  // End of indexed elements
        }
;

// Individual indexed element handling
indexedelem:
        LEFT_CURLY_BRACKET expr COLON expr RIGHT_CURLY_BRACKET {
          $$ = newexpr(boolexpr_e);
          emit(tablesetelem, $2, $4, $$, 0, yylineno);  // Set table element
        }
;

// Block handling
block:
        LEFT_CURLY_BRACKET { 
            if (scope > 0) {
                scope++;  // Enter new scope
            }
            int old_scope = scope;  // Store current scope
        }
        stmt_list
        RIGHT_CURLY_BRACKET {
          if (scope > 0) {
              hide_scope(scope);  // Hide current scope
              scope--;  // Exit scope
          }
          $$ = newexpr(boolexpr_e);
        }
      | LEFT_CURLY_BRACKET RIGHT_CURLY_BRACKET {
          $$ = newexpr(boolexpr_e);  // Empty block
        }
;
funcdef:
    FUNCTION {
        infunc++;
    }
    opt_ident {
        tmp_rec = $<symVal>2;
    }
    LEFT_PARENTHESIS {
        ++scope;          //  Scope για τα ορίσματα!
        notInsert = 1;
    }
    idlist
    RIGHT_PARENTHESIS {
        hide_scope(scope);   // Κρύψε τα formals
        --scope;             // Επιστροφή στο scope της function
        notInsert = 0;
    }
    block {
        infunc--;
        $$ = newexpr(programfunc_e);
        $$->sym = tmp_rec ? tmp_rec : symtable_lookup(buffer, scope);
    }
;


opt_ident:
    IDENT {
        if (symtable_lookup_lib($1)) {
            yyerror("Invalid: shadows LIBFUNC");
            $$ = NULL;
        } else {
            symtable_insert_checked(scope, yylineno, USERFUNC, $1);
            $$ = symtable_lookup($1, scope);
        }
    }
  | /* empty */ {
        sprintf(buffer, "$%d", dollar_counter++);  // ΜΟΝΟ εδώ
        symtable_insert_checked(scope, yylineno, USERFUNC, buffer);
        $$ = symtable_lookup(buffer, scope);
    }
;
idlist:
    IDENT idlist_list {
        // Έλεγχος για αν το όνομα υπάρχει ήδη στο scope ως formal
        if (symtable_lookup_formal($1, scope)) {
            yyerror("duplicate formal names");
        } else {
            symtable_insert_checked(scope, yylineno, FORMAL, $1);
        }
        $$ = NULL;
    }
  | /* empty */ {
        $$ = NULL;
    }
;

idlist_list:
    COMMA IDENT idlist_list {
        // Έλεγχος για αν το όνομα υπάρχει ήδη στο scope ως formal
        if (symtable_lookup_formal($2, scope)) {
            yyerror("duplicate formal names");
        } else {
            symtable_insert_checked(scope, yylineno, FORMAL, $2);
        }
        $$ = NULL;
    }
  | /* empty */ {
        $$ = NULL;
    }
;

const:
    INTEGER {
        $$ = newexpr_constnum($1);
    }
  | REAL {
        $$ = newexpr_constnum($1);
    }
  | STRINGCONST {
        $$ = newexpr(conststring_e);
        $$->strConst = strdup($1);
    }
  | NIL {
        notInsert = 0;
        $$ = newexpr(nil_e);
    }
  | TRUE {
        $$ = newexpr_constbool(1);
    }
  | FALSE {
        $$ = newexpr_constbool(0);
    }
;



// Semantic markers
M: { $$ = nextquadlabel(); };
N: { $$ = nextquadlabel(); };

%%

expr* reverse_expr_list(expr* head) {
    // Reverses a linked list of expressions
    expr* prev = NULL;
    expr* current = head;
    expr* next;

    while (current) {
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }

    return prev;
}

int yyerror(char* yaccProvidedMessage) {
    fprintf(stderr, "ERROR, %s token: %s, at line %d\n", yaccProvidedMessage, yytext, yylineno);
    return 1;
}

expr* lvalue_expr(struct expr* e) {
    if (e->type == var_e) {
        return e;
    } else if (e->type == tableitem_e) {
        return e;
    }
    return NULL;
}

void emit_param_list(expr* el) {
    // Emits parameter list for function calls in reverse order
    if (!el) return;

    expr* reversed = reverse_expr_list(el);
    for (expr* e = reversed; e != NULL; e = e->next) {
        emit(param, e, NULL, NULL, 0, yylineno);
    }
}
expr* convertToBoolIfNeeded(expr* e) {
    if (!e)
        return NULL;

    // Ήδη boolean με λίστες → μην το ξαναφτιάξεις
    if (e->type == boolexpr_e && e->truelist && e->falselist)
        return e;

    expr* bool_expr = newexpr(boolexpr_e);

    expr* cond_expr = e;

    // Αν δεν έχει sym, αντιγραφή σε temp
    if (!e->sym) {
        cond_expr = newexpr(var_e);
        cond_expr->sym = newtemp();
        emit(assign, e, NULL, cond_expr, 0, yylineno);
    }

    unsigned quad_if = nextquadlabel();
    unsigned label_true = quad_if + 2;
    unsigned label_end = label_true + 1;

    emit(if_eq, cond_expr, newexpr_constbool(1), NULL, label_true, yylineno);
    emit(jump_, NULL, NULL, NULL, label_end, yylineno);

    bool_expr->truelist = newlist(quad_if);
    bool_expr->falselist = newlist(quad_if + 1);

    return bool_expr;
}



