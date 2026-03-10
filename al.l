%{
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include "parser.h"

	/*#define YY_DECL int alpha_yylex (void* ylval)  parametrized yylex*/

	typedef struct alpha_token_t {
		long int line_number;
		long int token_id;
		char* token_text;
		char* token_type;
		char* data_type;
		char* pattern_name;
		struct alpha_token_t* next_token;
		struct {
			unsigned int hash_code;
			unsigned int reference_count;
			unsigned char is_analyzed;
		} token_metadata;
	} al_token;

	struct {
		al_token* token_list;
		long int token_counter;
		struct {
			unsigned int total_tokens;
			unsigned int unique_tokens;
			unsigned int max_line_number;
		} token_stats;
		struct {
			int line;
			int col;
		} location;
	} global_state[1] = {
		{
			.token_list = NULL,
			.token_counter = 0,
			.token_stats = {0, 0, 0},
			.location = {0, 0}
		}
	};

	unsigned int calculate_token_hash(const char* str) {
		unsigned int hash = 5381;
		int c;
		while ((c = *str++))
			hash = ((hash << 5) + hash) + c;
		return hash;
	}

	void update_token_stats(al_token* token) {
		if (!token) return;
		global_state[0].token_stats.total_tokens++;
		if (token->line_number > global_state[0].token_stats.max_line_number)
			global_state[0].token_stats.max_line_number = token->line_number;
	}

	al_token* create_token_node(void) {
		al_token* node = (al_token*)malloc(sizeof(al_token));
		if (!node) {
			fprintf(stderr, "Memory allocation failed\n");
			exit(1);
		}
		memset(node, 0, sizeof(al_token));
		return node;
	}

	void initialize_token(al_token* token_node, char* pattern_name, char* data_type, 
						 long int line_number, long int token_id, 
						 char* token_text, char* token_type) {
		if (!token_node) return;
		
		token_node->pattern_name = strdup(pattern_name);
		token_node->token_id = token_id;
		token_node->data_type = data_type;
		token_node->token_type = token_type;
		token_node->line_number = line_number;
		token_node->token_text = strdup(token_text);
		token_node->next_token = NULL;
		
		token_node->token_metadata.hash_code = calculate_token_hash(token_text);
		token_node->token_metadata.is_analyzed = 0;
		token_node->token_metadata.reference_count = 0;

		update_token_stats(token_node);
	}

	al_token* append_token(al_token* token_list, al_token* new_token) {
		if (!token_list) return new_token;
		
		al_token* current_token = token_list;
		do {
			if (!current_token->next_token) {
				current_token->next_token = new_token;
				break;
			}
			current_token = current_token->next_token;
		} while (1);
		
		return token_list;
	}

	al_token* Init_Token(char* pattern_name, char* data_type, long int line_number, 
					   long int token_id, char* token_text, 
					   char* token_type, al_token* token_list) {
		al_token* new_token = create_token_node();
		initialize_token(new_token, pattern_name, data_type, line_number, 
						token_id, token_text, token_type);
		return append_token(token_list, new_token);
	}

	void print_token_header(void) {
		printf("====> Token List (Total: %u, Unique: %u, Max Line: %u)\n",
			   global_state[0].token_stats.total_tokens,
			   global_state[0].token_stats.unique_tokens,
			   global_state[0].token_stats.max_line_number);
	}

	void print_token_content(al_token* current_token) {
		if (!current_token) {
			printf("Empty List, NO text included to your file!\n");
			return;
		}

		for (; current_token; current_token = current_token->next_token) {
			switch (current_token->token_type[0]) {
				case 'S':
					if (strcmp(current_token->token_type, "STRINGCONST") == 0) {
						printf("%ld: #%ld\t%s\t%s %s <- %s\n", 
							current_token->line_number, current_token->token_id, current_token->token_text, 
							current_token->token_type, current_token->pattern_name, current_token->data_type);
						break;
					}
				case 'I':
					if (strcmp(current_token->token_type, "IDENT") == 0) {
						printf("%ld: #%ld\t\"%s\"\t%s \"%s\" <- %s\n", 
							current_token->line_number, current_token->token_id, current_token->token_text, 
							current_token->token_type, current_token->pattern_name, current_token->data_type);
						break;
					}
				case 'M':
					if (strcmp(current_token->token_type, "MULTILINE_COMMENT") == 0) {
						printf("%ld: #%ld\t\"/* %s */\"\t%s  %s <- %s\n", 
							current_token->line_number, current_token->token_id, current_token->token_text, 
							current_token->token_type, current_token->pattern_name, current_token->data_type);
						break;
					}
				default:
					printf("%ld: #%ld\t\"%s\"\t%s  %s <- %s\n", 
						current_token->line_number, current_token->token_id, current_token->token_text, 
						current_token->token_type, current_token->pattern_name, current_token->data_type);
			}
		}
	}

	void print_list(al_token* L) {
		print_token_header();
		print_token_content(L);
	}

	void analyze_token_usage(al_token* token_node) {
		if (!token_node) return;
		token_node->token_metadata.reference_count++;
		token_node->token_metadata.is_analyzed = 1;
	}
%}

%option yylineno
%option	noyywrap
IF                   "if"
ELSE                 "else"
WHILE                "while"
FOR                  "for"
FUNCTION             "function"
RETURN               "return"
BREAK                "break"
CONTINUE             "continue"
LOCAL                "local"
TRUE                 "true"
FALSE                "false"
NIL                  "nil"

AND                  "and"
OR                   "or"
NOT                  "not"

ASSIGN               "="
ADD                  "+"
MINUS                "-"
MULTIPLICATION       "*"
DIVISION             "/"
MODULO               "%"
EQUAL                "=="
INEQUAL              "!="
INCREMENT            "++"
DECREMENT            "--"
GREATER              ">"
GREATER_EQUAL        ">="
LESS                 "<"
LESS_EQUAL           "<="

LEFT_CURLY_BRACKET       "{"
RIGHT_CURLY_BRACKET      "}"
LEFT_SQUARE_BRACKET      "["
RIGHT_SQUARE_BRACKET     "]"
LEFT_PARENTHESIS         "("
RIGHT_PARENTHESIS        ")"
SEMICOLON                ";"
COMMA                    ","
COLON                    ":"
DOUBLE_COLON             "::"
DOT                      "."
DOUBLE_DOT               ".."
SINGLE_LINE_COMMENT      "//"[^\n]*

IDENT       [a-zA-Z][a-zA-Z_0-9]*
INTEGER     (0[xX][0-9A-Fa-f]+)|([0-9]+)
REAL        [0-9]*\.[0-9]+([eE][+-]?[0-9]+)?
OTHERCHAR   .


%%

"&&" { Init_Token("AND","operator", yylineno, ++global_state[0].token_counter, "&&", "OPERATOR", global_state[0].token_list); return AND; }
"||" { Init_Token("OR","operator", yylineno, ++global_state[0].token_counter, "||", "OPERATOR", global_state[0].token_list); return OR; }
{IF}         { Init_Token("IF","enumerated", yylineno, ++global_state[0].token_counter, "if", "KEYWORD", global_state[0].token_list); return IF; }
{ELSE}       { Init_Token("ELSE","enumerated", yylineno, ++global_state[0].token_counter, "else", "KEYWORD", global_state[0].token_list); return ELSE; }
{WHILE}      { Init_Token("WHILE","enumerated", yylineno, ++global_state[0].token_counter, "while", "KEYWORD", global_state[0].token_list); return WHILE; }
{FOR}        { Init_Token("FOR","enumerated", yylineno, ++global_state[0].token_counter, "for", "KEYWORD", global_state[0].token_list); return FOR; }
{FUNCTION}   { Init_Token("FUNCTION","enumerated", yylineno, ++global_state[0].token_counter, "function", "KEYWORD", global_state[0].token_list); return FUNCTION; }
{RETURN}     { Init_Token("RETURN","enumerated", yylineno, ++global_state[0].token_counter, "return", "KEYWORD", global_state[0].token_list); return RETURN; }
{BREAK}      { Init_Token("BREAK","enumerated", yylineno, ++global_state[0].token_counter, "break", "KEYWORD", global_state[0].token_list); return BREAK; }
{CONTINUE}   { Init_Token("CONTINUE","enumerated", yylineno, ++global_state[0].token_counter, "continue", "KEYWORD", global_state[0].token_list); return CONTINUE; }
{AND}        { Init_Token("AND","enumerated", yylineno, ++global_state[0].token_counter, "and", "KEYWORD", global_state[0].token_list); return AND; }
{NOT}        { Init_Token("NOT","enumerated", yylineno, ++global_state[0].token_counter, "not", "KEYWORD", global_state[0].token_list); return NOT; }
{OR}         { Init_Token("OR","enumerated", yylineno, ++global_state[0].token_counter, "or", "KEYWORD", global_state[0].token_list); return OR; }
{LOCAL}      { Init_Token("LOCAL","enumerated", yylineno, ++global_state[0].token_counter, "local", "KEYWORD", global_state[0].token_list); return LOCAL; }
{TRUE}       { Init_Token("TRUE","enumerated", yylineno, ++global_state[0].token_counter, "true", "KEYWORD", global_state[0].token_list); return TRUE; }
{FALSE}      { Init_Token("FALSE","enumerated", yylineno, ++global_state[0].token_counter, "false", "KEYWORD", global_state[0].token_list); return FALSE; }
{NIL}        { Init_Token("NIL","enumerated", yylineno, ++global_state[0].token_counter, "nil", "KEYWORD", global_state[0].token_list); return NIL; }

{ASSIGN}         { Init_Token("ASSIGN","enumerated", yylineno, ++global_state[0].token_counter, "=", "OPERATOR", global_state[0].token_list); return ASSIGN; }
{ADD}            { Init_Token("ADD","enumerated", yylineno, ++global_state[0].token_counter, "+", "OPERATOR", global_state[0].token_list); return ADD; }
{MINUS}          { Init_Token("MINUS","enumerated", yylineno, ++global_state[0].token_counter, "-", "OPERATOR", global_state[0].token_list); return MINUS; }
{MULTIPLICATION} { Init_Token("MULTIPLICATION","enumerated", yylineno, ++global_state[0].token_counter, "*", "OPERATOR", global_state[0].token_list); return MULTIPLICATION; }
{DIVISION}       { Init_Token("DIVISION","enumerated", yylineno, ++global_state[0].token_counter, "/", "OPERATOR", global_state[0].token_list); return DIVISION; }
{MODULO}         { Init_Token("MODULO","enumerated", yylineno, ++global_state[0].token_counter, "%", "OPERATOR", global_state[0].token_list); return MODULO; }
{EQUAL}          { Init_Token("EQUAL","enumerated", yylineno, ++global_state[0].token_counter, "==", "OPERATOR", global_state[0].token_list); return EQUAL; }
{INEQUAL}        { Init_Token("INEQUAL","enumerated", yylineno, ++global_state[0].token_counter, "!=", "OPERATOR", global_state[0].token_list); return INEQUAL; }
{INCREMENT}      { Init_Token("INCREMENT","enumerated", yylineno, ++global_state[0].token_counter, "++", "OPERATOR", global_state[0].token_list); return INCREMENT; }
{DECREMENT}      { Init_Token("DECREMENT","enumerated", yylineno, ++global_state[0].token_counter, "--", "OPERATOR", global_state[0].token_list); return DECREMENT; }
{GREATER}        { Init_Token("GREATER","enumerated", yylineno, ++global_state[0].token_counter, ">", "OPERATOR", global_state[0].token_list); return GREATER; }
{GREATER_EQUAL}  { Init_Token("GREATER_EQUAL","enumerated", yylineno, ++global_state[0].token_counter, ">=", "OPERATOR", global_state[0].token_list); return GREATER_EQUAL; }
{LESS}           { Init_Token("LESS","enumerated", yylineno, ++global_state[0].token_counter, "<", "OPERATOR", global_state[0].token_list); return LESS; }
{LESS_EQUAL}     { Init_Token("LESS_EQUAL","enumerated", yylineno, ++global_state[0].token_counter, "<=", "OPERATOR", global_state[0].token_list); return LESS_EQUAL; }

{IDENT} {
    Init_Token(yytext, "char*", yylineno, ++global_state[0].token_counter, yytext, "IDENT", global_state[0].token_list);
    yylval.strVal = strdup(yytext);
    return IDENT;
}

{INTEGER} {
    Init_Token(yytext, "integer", yylineno, ++global_state[0].token_counter, yytext, "INTCONST", global_state[0].token_list);
    yylval.intval = atoi(yytext);
    return INTEGER;
}

{REAL} {
    Init_Token(yytext, "real", yylineno, ++global_state[0].token_counter, yytext, "FLOATCONST", global_state[0].token_list);
    yylval.doubleVal = atof(yytext);
    return REAL;
}

\" {
    const int start_line = yylineno;
    char* result = NULL;
    size_t size = 0;
    size_t pos = 0;
    int current;

    // Allocate initial buffer
    result = (char*)malloc(1);
    if (!result) {
        fprintf(stderr, "Memory allocation failed at line %d\n", start_line);
        return STRINGCONST;
    }

   // Process string contents
while ((current = input()) != EOF) {
    // Error: unescaped newline = unterminated string
    if (current == '\n') {
        fprintf(stderr, "ERROR: Unterminated string (newline) at line %d\n", start_line);
        free(result);
        exit(1);
    }

    // Handle escape sequences
    if (current == '\\') {
        current = input();
        char escaped;

        switch (current) {
            case '\"': escaped = '\"'; break;
            case 't':  escaped = '\t'; break;
            case 'n':  escaped = '\n'; break;
            case '\\': escaped = '\\'; break;
            default:   escaped = current; break;
        }

        if (pos >= size) {
            size = size ? size * 2 : 1;
            char* new_buf = (char*)realloc(result, size + 1);
            if (!new_buf) {
                fprintf(stderr, "Memory reallocation failed at line %d\n", start_line);
                free(result);
                return STRINGCONST;
            }
            result = new_buf;
        }
        result[pos++] = escaped;
        continue;
    }

    // Check for string end
    if (current == '\"') {
        result[pos] = '\0';
        Init_Token(result, "char*", start_line, ++global_state[0].token_counter,
                  result, "STRINGCONST", global_state[0].token_list);
        yylval.strVal = strdup(result);
        free(result);
        return STRINGCONST;
    }

    // Regular character
    if (pos >= size) {
        size = size ? size * 2 : 1;
        char* new_buf = (char*)realloc(result, size + 1);
        if (!new_buf) {
            fprintf(stderr, "Memory reallocation failed at line %d\n", start_line);
            free(result);
            return STRINGCONST;
        }
        result = new_buf;
    }
    result[pos++] = current;
}


    // Handle unterminated string
    fprintf(stderr, "Error: String started at line %d was not properly terminated\n", start_line);
    free(result);
    return STRINGCONST;
}

{LEFT_CURLY_BRACKET}    { Init_Token("LEFT_CURLY_BRACKET","enumerated",yylineno, ++global_state[0].token_counter, "{","PUNCTUATION", global_state[0].token_list); return LEFT_CURLY_BRACKET; }
{RIGHT_CURLY_BRACKET}   { Init_Token("RIGHT_CURLY_BRACKET","enumerated",yylineno, ++global_state[0].token_counter, "}","PUNCTUATION", global_state[0].token_list); return RIGHT_CURLY_BRACKET; }
{LEFT_SQUARE_BRACKET}   { Init_Token("LEFT_SQUARE_BRACKET","enumerated",yylineno, ++global_state[0].token_counter, "[","PUNCTUATION", global_state[0].token_list); return LEFT_SQUARE_BRACKET; }
{RIGHT_SQUARE_BRACKET}  { Init_Token("RIGHT_SQUARE_BRACKET","enumerated",yylineno, ++global_state[0].token_counter, "]","PUNCTUATION", global_state[0].token_list); return RIGHT_SQUARE_BRACKET; }
{LEFT_PARENTHESIS}      { Init_Token("LEFT_PARENTHESIS","enumerated",yylineno, ++global_state[0].token_counter, "(","PUNCTUATION", global_state[0].token_list); return LEFT_PARENTHESIS; }
{RIGHT_PARENTHESIS}     { Init_Token("RIGHT_PARENTHESIS","enumerated",yylineno, ++global_state[0].token_counter, ")","PUNCTUATION", global_state[0].token_list); return RIGHT_PARENTHESIS; }
{SEMICOLON}             { Init_Token("SEMICOLON","enumerated",yylineno, ++global_state[0].token_counter, ";","PUNCTUATION", global_state[0].token_list); return SEMICOLON; }
{COMMA}                 { Init_Token("COMMA","enumerated",yylineno, ++global_state[0].token_counter, ",","PUNCTUATION", global_state[0].token_list); return COMMA; }
{COLON}                 { Init_Token("COLON","enumerated",yylineno, ++global_state[0].token_counter, ":","PUNCTUATION", global_state[0].token_list); return COLON; }
{DOUBLE_COLON}          { Init_Token("DOUBLE_COLON","enumerated",yylineno, ++global_state[0].token_counter, "::","PUNCTUATION", global_state[0].token_list); return DOUBLE_COLON; }
{DOT}                   { Init_Token("DOT","enumerated",yylineno, ++global_state[0].token_counter, ".","PUNCTUATION", global_state[0].token_list); return DOT; }
{DOUBLE_DOT}            { Init_Token("DOUBLE_DOT","enumerated",yylineno, ++global_state[0].token_counter, "..","PUNCTUATION", global_state[0].token_list); return DOUBLE_DOT; }
{SINGLE_LINE_COMMENT}   { Init_Token("COMMENT","enumerated",yylineno, ++global_state[0].token_counter, "//","SINGLE_LINE_COMMENT", global_state[0].token_list); }

"\n"  { }
"\r"  { }
" "   { }
"\t"  { }

"/*" {
    int c;
    while ((c = input()) != EOF) {
        if (c == '*') {
            if ((c = input()) == '/') break;
            else unput(c);
        }
    }
    if (c == EOF) {
        fprintf(stderr, "ERROR, multiline comment doesn't close, line: %d\n", yylineno);
        exit(0);
    }
    Init_Token("COMMENT","enumerated", yylineno, ++global_state[0].token_counter, "", "MULTILINE_COMMENT", global_state[0].token_list);
}

. {
    fprintf(stderr,"ERROR, UNDEFINED CHARACTER '%s' FOUND! line: %d CHARACTER '%c'\n", yytext, yylineno, yytext[0]);
    return OTHERCHAR;
}

%%






