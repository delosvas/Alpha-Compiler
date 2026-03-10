"""Tests for the Alpha language lexer."""

import pytest
from alpha.lexer import Lexer, LexerError, TokenType


def tokenize(source: str):
    return Lexer(source).tokenize()


def token_types(source: str):
    return [t.type for t in tokenize(source) if t.type != TokenType.EOF]


class TestNumbers:
    def test_integer(self):
        toks = tokenize("42")
        assert toks[0].type == TokenType.NUMBER
        assert toks[0].value == 42

    def test_float(self):
        toks = tokenize("3.14")
        assert toks[0].type == TokenType.NUMBER
        assert toks[0].value == pytest.approx(3.14)

    def test_zero(self):
        toks = tokenize("0")
        assert toks[0].value == 0


class TestStrings:
    def test_double_quote(self):
        toks = tokenize('"hello"')
        assert toks[0].type == TokenType.STRING
        assert toks[0].value == "hello"

    def test_single_quote(self):
        toks = tokenize("'world'")
        assert toks[0].type == TokenType.STRING
        assert toks[0].value == "world"

    def test_escape_sequences(self):
        toks = tokenize(r'"a\nb\tc"')
        assert toks[0].value == "a\nb\tc"

    def test_unterminated_string_raises(self):
        with pytest.raises(LexerError, match="Unterminated string"):
            tokenize('"oops')


class TestKeywords:
    def test_keywords_recognized(self):
        for kw, expected in [
            ("let", TokenType.LET),
            ("if", TokenType.IF),
            ("else", TokenType.ELSE),
            ("while", TokenType.WHILE),
            ("print", TokenType.PRINT),
            ("func", TokenType.FUNC),
            ("return", TokenType.RETURN),
            ("true", TokenType.TRUE),
            ("false", TokenType.FALSE),
        ]:
            types = token_types(kw)
            assert types == [expected], f"Failed for keyword: {kw}"

    def test_true_value(self):
        toks = tokenize("true")
        assert toks[0].value is True

    def test_false_value(self):
        toks = tokenize("false")
        assert toks[0].value is False


class TestOperators:
    def test_single_char_ops(self):
        for ch, expected in [
            ("+", TokenType.PLUS),
            ("-", TokenType.MINUS),
            ("*", TokenType.STAR),
            ("/", TokenType.SLASH),
            ("%", TokenType.PERCENT),
            ("=", TokenType.EQ),
            ("<", TokenType.LT),
            (">", TokenType.GT),
            ("!", TokenType.NOT),
        ]:
            types = token_types(ch)
            assert types == [expected], f"Failed for: {ch}"

    def test_two_char_ops(self):
        for src, expected in [
            ("==", TokenType.EQEQ),
            ("!=", TokenType.NEQ),
            ("<=", TokenType.LTE),
            (">=", TokenType.GTE),
            ("&&", TokenType.AND),
            ("||", TokenType.OR),
        ]:
            types = token_types(src)
            assert types == [expected], f"Failed for: {src}"


class TestComments:
    def test_comment_skipped(self):
        types = token_types("# this is a comment\n42")
        assert types == [TokenType.NUMBER]

    def test_inline_comment(self):
        types = token_types("42 # comment")
        assert types == [TokenType.NUMBER]


class TestUnexpectedCharacter:
    def test_raises_on_unknown(self):
        with pytest.raises(LexerError, match="Unexpected character"):
            tokenize("@")
