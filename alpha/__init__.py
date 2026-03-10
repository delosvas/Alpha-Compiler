"""Alpha language package."""

from .lexer import Lexer, LexerError, Token, TokenType
from .parser import Parser, ParseError
from .interpreter import Interpreter

__all__ = [
    "Lexer", "LexerError", "Token", "TokenType",
    "Parser", "ParseError",
    "Interpreter",
    "run_source",
]


def run_source(source: str, output=None) -> None:
    """Lex, parse, and interpret an Alpha source string.

    Parameters
    ----------
    source:
        Alpha source code as a string.
    output:
        Optional file-like object for ``print`` output; defaults to stdout.
    """
    lexer = Lexer(source)
    tokens = lexer.tokenize()

    parser = Parser(tokens)
    program = parser.parse()

    interpreter = Interpreter(output=output)
    interpreter.run(program)
