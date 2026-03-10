"""Tests for the Alpha language interpreter."""

import io
import pytest
from alpha import run_source
from alpha.interpreter import RuntimeError as AlphaRuntimeError


def run(source: str) -> str:
    """Run Alpha source and return captured stdout as a string."""
    buf = io.StringIO()
    run_source(source, output=buf)
    return buf.getvalue()


class TestPrint:
    def test_number(self):
        assert run("print 42;") == "42\n"

    def test_string(self):
        assert run('print "hello";') == "hello\n"

    def test_bool_true(self):
        assert run("print true;") == "true\n"

    def test_bool_false(self):
        assert run("print false;") == "false\n"


class TestArithmetic:
    def test_addition(self):
        assert run("print 1 + 2;") == "3\n"

    def test_subtraction(self):
        assert run("print 10 - 3;") == "7\n"

    def test_multiplication(self):
        assert run("print 4 * 5;") == "20\n"

    def test_division(self):
        assert run("print 10 / 4;") == "2.5\n"

    def test_modulo(self):
        assert run("print 10 % 3;") == "1\n"

    def test_division_by_zero(self):
        with pytest.raises(AlphaRuntimeError, match="Division by zero"):
            run("print 1 / 0;")

    def test_modulo_by_zero(self):
        with pytest.raises(AlphaRuntimeError, match="Modulo by zero"):
            run("print 5 % 0;")

    def test_string_concatenation(self):
        assert run('print "foo" + "bar";') == "foobar\n"

    def test_precedence(self):
        assert run("print 2 + 3 * 4;") == "14\n"

    def test_unary_minus(self):
        assert run("print -5;") == "-5\n"


class TestVariables:
    def test_let_and_print(self):
        assert run("let x = 7; print x;") == "7\n"

    def test_assign(self):
        assert run("let x = 1; x = 99; print x;") == "99\n"

    def test_undefined_variable(self):
        with pytest.raises(AlphaRuntimeError, match="Undefined variable"):
            run("print y;")

    def test_reassign_undefined(self):
        with pytest.raises(AlphaRuntimeError, match="Undefined variable"):
            run("y = 5;")


class TestComparisons:
    def test_less_than_true(self):
        assert run("print 1 < 2;") == "true\n"

    def test_less_than_false(self):
        assert run("print 3 < 2;") == "false\n"

    def test_equality(self):
        assert run("print 1 == 1;") == "true\n"

    def test_inequality(self):
        assert run("print 1 != 2;") == "true\n"

    def test_gte(self):
        assert run("print 5 >= 5;") == "true\n"


class TestIfStatement:
    def test_then_branch(self):
        assert run("if (true) { print 1; }") == "1\n"

    def test_else_branch(self):
        assert run("if (false) { print 1; } else { print 2; }") == "2\n"

    def test_nested(self):
        src = """
        let x = 5;
        if (x > 10) {
            print "big";
        } else {
            if (x > 3) {
                print "medium";
            } else {
                print "small";
            }
        }
        """
        assert run(src) == "medium\n"


class TestWhileLoop:
    def test_count_up(self):
        src = "let i = 0; while (i < 3) { print i; i = i + 1; }"
        assert run(src) == "0\n1\n2\n"

    def test_never_executes(self):
        assert run("while (false) { print 1; }") == ""


class TestFunctions:
    def test_simple_call(self):
        src = """
        func greet() {
            print "hello";
        }
        greet();
        """
        assert run(src) == "hello\n"

    def test_return_value(self):
        src = """
        func double(n) {
            return n * 2;
        }
        print double(5);
        """
        assert run(src) == "10\n"

    def test_recursive_fibonacci(self):
        src = """
        func fib(n) {
            if (n <= 1) {
                return n;
            }
            return fib(n - 1) + fib(n - 2);
        }
        print fib(7);
        """
        assert run(src) == "13\n"

    def test_wrong_arg_count(self):
        src = """
        func add(a, b) { return a + b; }
        add(1);
        """
        with pytest.raises(AlphaRuntimeError, match="expects 2 argument"):
            run(src)

    def test_closure_scope(self):
        src = """
        let x = 10;
        func show() {
            print x;
        }
        show();
        """
        assert run(src) == "10\n"


class TestLogical:
    def test_and_true(self):
        assert run("print true && true;") == "true\n"

    def test_and_false(self):
        assert run("print true && false;") == "false\n"

    def test_or_true(self):
        assert run("print false || true;") == "true\n"

    def test_not(self):
        assert run("print !false;") == "true\n"
