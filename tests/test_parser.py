"""Tests for the Alpha language parser."""

import pytest
from alpha.lexer import Lexer
from alpha.parser import Parser, ParseError
from alpha.ast_nodes import (
    Program, LetStatement, AssignStatement, PrintStatement,
    IfStatement, WhileStatement, FuncDeclaration, ReturnStatement,
    ExprStatement, BinaryOp, UnaryOp, NumberLiteral, StringLiteral,
    BoolLiteral, Identifier, CallExpr,
)


def parse(source: str) -> Program:
    tokens = Lexer(source).tokenize()
    return Parser(tokens).parse()


def first_stmt(source: str):
    return parse(source).body[0]


class TestLiterals:
    def test_number(self):
        stmt = first_stmt("42;")
        assert isinstance(stmt, ExprStatement)
        assert isinstance(stmt.expr, NumberLiteral)
        assert stmt.expr.value == 42

    def test_string(self):
        stmt = first_stmt('"hi";')
        assert isinstance(stmt.expr, StringLiteral)
        assert stmt.expr.value == "hi"

    def test_bool_true(self):
        stmt = first_stmt("true;")
        assert isinstance(stmt.expr, BoolLiteral)
        assert stmt.expr.value is True

    def test_bool_false(self):
        stmt = first_stmt("false;")
        assert isinstance(stmt.expr, BoolLiteral)
        assert stmt.expr.value is False


class TestLetStatement:
    def test_basic(self):
        stmt = first_stmt("let x = 10;")
        assert isinstance(stmt, LetStatement)
        assert stmt.name == "x"
        assert isinstance(stmt.value, NumberLiteral)
        assert stmt.value.value == 10

    def test_missing_semicolon(self):
        with pytest.raises(ParseError):
            parse("let x = 10")


class TestAssignStatement:
    def test_basic(self):
        stmt = first_stmt("let x = 1; x = 2;")
        # second statement
        stmt = parse("let x = 1; x = 2;").body[1]
        assert isinstance(stmt, AssignStatement)
        assert stmt.name == "x"
        assert isinstance(stmt.value, NumberLiteral)


class TestPrintStatement:
    def test_basic(self):
        stmt = first_stmt('print "hello";')
        assert isinstance(stmt, PrintStatement)
        assert isinstance(stmt.value, StringLiteral)


class TestBinaryOps:
    def test_addition(self):
        stmt = first_stmt("1 + 2;")
        expr = stmt.expr
        assert isinstance(expr, BinaryOp)
        assert expr.op == "+"

    def test_precedence_multiply_before_add(self):
        stmt = first_stmt("1 + 2 * 3;")
        expr = stmt.expr
        assert isinstance(expr, BinaryOp)
        assert expr.op == "+"
        assert isinstance(expr.right, BinaryOp)
        assert expr.right.op == "*"

    def test_parentheses_override_precedence(self):
        stmt = first_stmt("(1 + 2) * 3;")
        expr = stmt.expr
        assert isinstance(expr, BinaryOp)
        assert expr.op == "*"
        assert isinstance(expr.left, BinaryOp)
        assert expr.left.op == "+"


class TestUnaryOps:
    def test_negate(self):
        stmt = first_stmt("-5;")
        expr = stmt.expr
        assert isinstance(expr, UnaryOp)
        assert expr.op == "-"

    def test_not(self):
        stmt = first_stmt("!true;")
        expr = stmt.expr
        assert isinstance(expr, UnaryOp)
        assert expr.op == "!"


class TestIfStatement:
    def test_basic(self):
        stmt = first_stmt("if (x > 0) { print x; }")
        assert isinstance(stmt, IfStatement)
        assert stmt.else_body is None

    def test_with_else(self):
        stmt = first_stmt("if (x > 0) { print x; } else { print 0; }")
        assert isinstance(stmt, IfStatement)
        assert stmt.else_body is not None


class TestWhileStatement:
    def test_basic(self):
        stmt = first_stmt("while (i < 10) { i = i + 1; }")
        assert isinstance(stmt, WhileStatement)


class TestFuncDeclaration:
    def test_no_params(self):
        stmt = first_stmt("func greet() { print \"hi\"; }")
        assert isinstance(stmt, FuncDeclaration)
        assert stmt.name == "greet"
        assert stmt.params == []

    def test_with_params(self):
        stmt = first_stmt("func add(a, b) { return a + b; }")
        assert isinstance(stmt, FuncDeclaration)
        assert stmt.params == ["a", "b"]


class TestReturnStatement:
    def test_with_value(self):
        prog = parse("func f() { return 42; }")
        func = prog.body[0]
        ret = func.body[0]
        assert isinstance(ret, ReturnStatement)
        assert isinstance(ret.value, NumberLiteral)

    def test_without_value(self):
        prog = parse("func f() { return; }")
        func = prog.body[0]
        ret = func.body[0]
        assert isinstance(ret, ReturnStatement)
        assert ret.value is None


class TestCallExpr:
    def test_no_args(self):
        stmt = first_stmt("greet();")
        assert isinstance(stmt.expr, CallExpr)
        assert stmt.expr.callee == "greet"
        assert stmt.expr.args == []

    def test_with_args(self):
        stmt = first_stmt("add(1, 2);")
        assert isinstance(stmt.expr, CallExpr)
        assert len(stmt.expr.args) == 2
