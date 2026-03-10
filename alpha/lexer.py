"""Lexer (tokenizer) for the Alpha language."""

import re
from enum import Enum, auto
from dataclasses import dataclass
from typing import List, Optional


class TokenType(Enum):
    # Literals
    NUMBER = auto()
    STRING = auto()

    # Identifiers and keywords
    IDENTIFIER = auto()
    LET = auto()
    IF = auto()
    ELSE = auto()
    WHILE = auto()
    PRINT = auto()
    FUNC = auto()
    RETURN = auto()
    TRUE = auto()
    FALSE = auto()

    # Operators
    PLUS = auto()
    MINUS = auto()
    STAR = auto()
    SLASH = auto()
    PERCENT = auto()
    EQ = auto()       # =
    EQEQ = auto()     # ==
    NEQ = auto()      # !=
    LT = auto()       # <
    LTE = auto()      # <=
    GT = auto()       # >
    GTE = auto()      # >=
    AND = auto()      # &&
    OR = auto()       # ||
    NOT = auto()      # !

    # Delimiters
    LPAREN = auto()
    RPAREN = auto()
    LBRACE = auto()
    RBRACE = auto()
    COMMA = auto()
    SEMICOLON = auto()

    # Special
    EOF = auto()


KEYWORDS = {
    "let":    TokenType.LET,
    "if":     TokenType.IF,
    "else":   TokenType.ELSE,
    "while":  TokenType.WHILE,
    "print":  TokenType.PRINT,
    "func":   TokenType.FUNC,
    "return": TokenType.RETURN,
    "true":   TokenType.TRUE,
    "false":  TokenType.FALSE,
}


@dataclass
class Token:
    type: TokenType
    value: object
    line: int
    col: int

    def __repr__(self) -> str:
        return f"Token({self.type.name}, {self.value!r}, line={self.line}, col={self.col})"


class LexerError(Exception):
    def __init__(self, message: str, line: int, col: int):
        super().__init__(f"{message} (line {line}, col {col})")
        self.line = line
        self.col = col


class Lexer:
    """Tokenizes Alpha source code into a flat list of tokens."""

    def __init__(self, source: str):
        self._src = source
        self._pos = 0
        self._line = 1
        self._col = 1

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def tokenize(self) -> List[Token]:
        tokens: List[Token] = []
        while True:
            tok = self._next_token()
            tokens.append(tok)
            if tok.type == TokenType.EOF:
                break
        return tokens

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _peek(self, offset: int = 0) -> Optional[str]:
        idx = self._pos + offset
        if idx < len(self._src):
            return self._src[idx]
        return None

    def _advance(self) -> str:
        ch = self._src[self._pos]
        self._pos += 1
        if ch == "\n":
            self._line += 1
            self._col = 1
        else:
            self._col += 1
        return ch

    def _skip_whitespace_and_comments(self):
        while self._pos < len(self._src):
            ch = self._peek()
            if ch in (" ", "\t", "\r", "\n"):
                self._advance()
            elif ch == "#":  # single-line comment
                while self._pos < len(self._src) and self._peek() != "\n":
                    self._advance()
            else:
                break

    def _next_token(self) -> Token:
        self._skip_whitespace_and_comments()

        if self._pos >= len(self._src):
            return Token(TokenType.EOF, None, self._line, self._col)

        line, col = self._line, self._col
        ch = self._peek()

        # Numbers
        if ch.isdigit() or (ch == "." and self._peek(1) and self._peek(1).isdigit()):
            return self._read_number(line, col)

        # Strings
        if ch in ('"', "'"):
            return self._read_string(line, col)

        # Identifiers and keywords
        if ch.isalpha() or ch == "_":
            return self._read_identifier(line, col)

        # Two-character operators
        two = (ch or "") + (self._peek(1) or "")
        two_char_ops = {
            "==": TokenType.EQEQ,
            "!=": TokenType.NEQ,
            "<=": TokenType.LTE,
            ">=": TokenType.GTE,
            "&&": TokenType.AND,
            "||": TokenType.OR,
        }
        if two in two_char_ops:
            self._advance()
            self._advance()
            return Token(two_char_ops[two], two, line, col)

        # Single-character tokens
        single_char_ops = {
            "+": TokenType.PLUS,
            "-": TokenType.MINUS,
            "*": TokenType.STAR,
            "/": TokenType.SLASH,
            "%": TokenType.PERCENT,
            "=": TokenType.EQ,
            "<": TokenType.LT,
            ">": TokenType.GT,
            "!": TokenType.NOT,
            "(": TokenType.LPAREN,
            ")": TokenType.RPAREN,
            "{": TokenType.LBRACE,
            "}": TokenType.RBRACE,
            ",": TokenType.COMMA,
            ";": TokenType.SEMICOLON,
        }
        if ch in single_char_ops:
            self._advance()
            return Token(single_char_ops[ch], ch, line, col)

        raise LexerError(f"Unexpected character {ch!r}", line, col)

    def _read_number(self, line: int, col: int) -> Token:
        start = self._pos
        has_dot = False
        while self._pos < len(self._src):
            c = self._peek()
            if c and c.isdigit():
                self._advance()
            elif c == "." and not has_dot and self._peek(1) and self._peek(1).isdigit():
                has_dot = True
                self._advance()
            else:
                break
        raw = self._src[start:self._pos]
        value = float(raw) if has_dot else int(raw)
        return Token(TokenType.NUMBER, value, line, col)

    def _read_string(self, line: int, col: int) -> Token:
        quote = self._advance()  # consume opening quote
        chars = []
        while self._pos < len(self._src):
            c = self._peek()
            if c == quote:
                self._advance()
                break
            if c == "\\":
                self._advance()
                esc = self._advance()
                escape_map = {"n": "\n", "t": "\t", "r": "\r", "\\": "\\",
                              '"': '"', "'": "'"}
                chars.append(escape_map.get(esc, esc))
            else:
                chars.append(self._advance())
        else:
            raise LexerError("Unterminated string literal", line, col)
        return Token(TokenType.STRING, "".join(chars), line, col)

    def _read_identifier(self, line: int, col: int) -> Token:
        start = self._pos
        while self._pos < len(self._src):
            c = self._peek()
            if c and (c.isalnum() or c == "_"):
                self._advance()
            else:
                break
        name = self._src[start:self._pos]
        tok_type = KEYWORDS.get(name, TokenType.IDENTIFIER)
        value: object = name
        if tok_type == TokenType.TRUE:
            value = True
        elif tok_type == TokenType.FALSE:
            value = False
        return Token(tok_type, value, line, col)
