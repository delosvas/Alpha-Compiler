"""Parser for the Alpha language.

Grammar (simplified):

    program        := statement* EOF
    statement      := let_stmt
                    | assign_stmt
                    | if_stmt
                    | while_stmt
                    | func_decl
                    | return_stmt
                    | print_stmt
                    | expr_stmt
    let_stmt       := 'let' IDENT '=' expression ';'
    assign_stmt    := IDENT '=' expression ';'
    if_stmt        := 'if' '(' expression ')' block ( 'else' block )?
    while_stmt     := 'while' '(' expression ')' block
    func_decl      := 'func' IDENT '(' param_list? ')' block
    return_stmt    := 'return' expression? ';'
    print_stmt     := 'print' expression ';'
    expr_stmt      := expression ';'
    block          := '{' statement* '}'
    expression     := or_expr
    or_expr        := and_expr ( '||' and_expr )*
    and_expr       := equality ( '&&' equality )*
    equality       := comparison ( ( '==' | '!=' ) comparison )*
    comparison     := additive ( ( '<' | '<=' | '>' | '>=' ) additive )*
    additive       := multiplicative ( ( '+' | '-' ) multiplicative )*
    multiplicative := unary ( ( '*' | '/' | '%' ) unary )*
    unary          := ( '-' | '!' ) unary | primary
    primary        := NUMBER | STRING | 'true' | 'false'
                    | IDENT ( '(' arg_list? ')' )?
                    | '(' expression ')'
"""

from typing import List, Optional

from .lexer import Token, TokenType
from .ast_nodes import (
    Node, Program, NumberLiteral, StringLiteral, BoolLiteral,
    Identifier, BinaryOp, UnaryOp, CallExpr,
    LetStatement, AssignStatement, PrintStatement,
    IfStatement, WhileStatement, FuncDeclaration, ReturnStatement,
    ExprStatement,
)


class ParseError(Exception):
    def __init__(self, message: str, token: Token):
        super().__init__(f"{message} (line {token.line}, col {token.col})")
        self.token = token


class Parser:
    """Recursive-descent parser that produces an AST from a token stream."""

    def __init__(self, tokens: List[Token]):
        self._tokens = tokens
        self._pos = 0

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def parse(self) -> Program:
        stmts: List[Node] = []
        while not self._check(TokenType.EOF):
            stmts.append(self._statement())
        return Program(body=stmts)

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _peek(self) -> Token:
        return self._tokens[self._pos]

    def _check(self, *types: TokenType) -> bool:
        return self._peek().type in types

    def _advance(self) -> Token:
        tok = self._tokens[self._pos]
        if tok.type != TokenType.EOF:
            self._pos += 1
        return tok

    def _match(self, *types: TokenType) -> Optional[Token]:
        if self._check(*types):
            return self._advance()
        return None

    def _expect(self, typ: TokenType, msg: str = "") -> Token:
        tok = self._peek()
        if tok.type != typ:
            raise ParseError(msg or f"Expected {typ.name}, got {tok.type.name}", tok)
        return self._advance()

    # ------------------------------------------------------------------
    # Statements
    # ------------------------------------------------------------------

    def _statement(self) -> Node:
        tok = self._peek()

        if tok.type == TokenType.LET:
            return self._let_stmt()
        if tok.type == TokenType.IF:
            return self._if_stmt()
        if tok.type == TokenType.WHILE:
            return self._while_stmt()
        if tok.type == TokenType.FUNC:
            return self._func_decl()
        if tok.type == TokenType.RETURN:
            return self._return_stmt()
        if tok.type == TokenType.PRINT:
            return self._print_stmt()

        # Assignment: IDENT '=' expr ';'
        if tok.type == TokenType.IDENTIFIER and self._lookahead_is_assign():
            return self._assign_stmt()

        return self._expr_stmt()

    def _lookahead_is_assign(self) -> bool:
        """Return True if next token after current IDENT is '=' (not '==')."""
        if self._pos + 1 < len(self._tokens):
            next_tok = self._tokens[self._pos + 1]
            return next_tok.type == TokenType.EQ
        return False

    def _let_stmt(self) -> LetStatement:
        self._advance()  # consume 'let'
        name_tok = self._expect(TokenType.IDENTIFIER, "Expected variable name after 'let'")
        self._expect(TokenType.EQ, "Expected '=' after variable name")
        value = self._expression()
        self._expect(TokenType.SEMICOLON, "Expected ';' after expression")
        return LetStatement(name=name_tok.value, value=value)

    def _assign_stmt(self) -> AssignStatement:
        name_tok = self._advance()  # IDENT
        self._advance()              # '='
        value = self._expression()
        self._expect(TokenType.SEMICOLON, "Expected ';' after expression")
        return AssignStatement(name=name_tok.value, value=value)

    def _print_stmt(self) -> PrintStatement:
        self._advance()  # consume 'print'
        value = self._expression()
        self._expect(TokenType.SEMICOLON, "Expected ';' after print expression")
        return PrintStatement(value=value)

    def _if_stmt(self) -> IfStatement:
        self._advance()  # consume 'if'
        self._expect(TokenType.LPAREN, "Expected '(' after 'if'")
        condition = self._expression()
        self._expect(TokenType.RPAREN, "Expected ')' after if condition")
        then_body = self._block()
        else_body = None
        if self._match(TokenType.ELSE):
            else_body = self._block()
        return IfStatement(condition=condition, then_body=then_body, else_body=else_body)

    def _while_stmt(self) -> WhileStatement:
        self._advance()  # consume 'while'
        self._expect(TokenType.LPAREN, "Expected '(' after 'while'")
        condition = self._expression()
        self._expect(TokenType.RPAREN, "Expected ')' after while condition")
        body = self._block()
        return WhileStatement(condition=condition, body=body)

    def _func_decl(self) -> FuncDeclaration:
        self._advance()  # consume 'func'
        name_tok = self._expect(TokenType.IDENTIFIER, "Expected function name after 'func'")
        self._expect(TokenType.LPAREN, "Expected '(' after function name")
        params: List[str] = []
        if not self._check(TokenType.RPAREN):
            params.append(self._expect(TokenType.IDENTIFIER, "Expected parameter name").value)
            while self._match(TokenType.COMMA):
                params.append(self._expect(TokenType.IDENTIFIER, "Expected parameter name").value)
        self._expect(TokenType.RPAREN, "Expected ')' after parameters")
        body = self._block()
        return FuncDeclaration(name=name_tok.value, params=params, body=body)

    def _return_stmt(self) -> ReturnStatement:
        self._advance()  # consume 'return'
        value = None
        if not self._check(TokenType.SEMICOLON):
            value = self._expression()
        self._expect(TokenType.SEMICOLON, "Expected ';' after return")
        return ReturnStatement(value=value)

    def _expr_stmt(self) -> ExprStatement:
        expr = self._expression()
        self._expect(TokenType.SEMICOLON, "Expected ';' after expression")
        return ExprStatement(expr=expr)

    def _block(self) -> List[Node]:
        self._expect(TokenType.LBRACE, "Expected '{'")
        stmts: List[Node] = []
        while not self._check(TokenType.RBRACE, TokenType.EOF):
            stmts.append(self._statement())
        self._expect(TokenType.RBRACE, "Expected '}'")
        return stmts

    # ------------------------------------------------------------------
    # Expressions (precedence climbing)
    # ------------------------------------------------------------------

    def _expression(self) -> Node:
        return self._or_expr()

    def _or_expr(self) -> Node:
        left = self._and_expr()
        while self._check(TokenType.OR):
            op = self._advance().value
            right = self._and_expr()
            left = BinaryOp(op=op, left=left, right=right)
        return left

    def _and_expr(self) -> Node:
        left = self._equality()
        while self._check(TokenType.AND):
            op = self._advance().value
            right = self._equality()
            left = BinaryOp(op=op, left=left, right=right)
        return left

    def _equality(self) -> Node:
        left = self._comparison()
        while self._check(TokenType.EQEQ, TokenType.NEQ):
            op = self._advance().value
            right = self._comparison()
            left = BinaryOp(op=op, left=left, right=right)
        return left

    def _comparison(self) -> Node:
        left = self._additive()
        while self._check(TokenType.LT, TokenType.LTE, TokenType.GT, TokenType.GTE):
            op = self._advance().value
            right = self._additive()
            left = BinaryOp(op=op, left=left, right=right)
        return left

    def _additive(self) -> Node:
        left = self._multiplicative()
        while self._check(TokenType.PLUS, TokenType.MINUS):
            op = self._advance().value
            right = self._multiplicative()
            left = BinaryOp(op=op, left=left, right=right)
        return left

    def _multiplicative(self) -> Node:
        left = self._unary()
        while self._check(TokenType.STAR, TokenType.SLASH, TokenType.PERCENT):
            op = self._advance().value
            right = self._unary()
            left = BinaryOp(op=op, left=left, right=right)
        return left

    def _unary(self) -> Node:
        if self._check(TokenType.MINUS):
            op = self._advance().value
            return UnaryOp(op=op, operand=self._unary())
        if self._check(TokenType.NOT):
            op = self._advance().value
            return UnaryOp(op=op, operand=self._unary())
        return self._primary()

    def _primary(self) -> Node:
        tok = self._peek()

        if tok.type == TokenType.NUMBER:
            self._advance()
            return NumberLiteral(value=tok.value)

        if tok.type == TokenType.STRING:
            self._advance()
            return StringLiteral(value=tok.value)

        if tok.type in (TokenType.TRUE, TokenType.FALSE):
            self._advance()
            return BoolLiteral(value=tok.value)

        if tok.type == TokenType.IDENTIFIER:
            self._advance()
            # Function call?
            if self._match(TokenType.LPAREN):
                args: List[Node] = []
                if not self._check(TokenType.RPAREN):
                    args.append(self._expression())
                    while self._match(TokenType.COMMA):
                        args.append(self._expression())
                self._expect(TokenType.RPAREN, "Expected ')' after arguments")
                return CallExpr(callee=tok.value, args=args)
            return Identifier(name=tok.value)

        if tok.type == TokenType.LPAREN:
            self._advance()
            expr = self._expression()
            self._expect(TokenType.RPAREN, "Expected ')'")
            return expr

        raise ParseError(f"Unexpected token {tok.type.name}", tok)
