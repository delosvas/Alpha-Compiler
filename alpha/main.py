"""Command-line entry point for the Alpha interpreter."""

import sys
import argparse

from alpha import run_source
from alpha.lexer import LexerError
from alpha.parser import ParseError
from alpha.interpreter import RuntimeError as AlphaRuntimeError


def main(argv=None):
    ap = argparse.ArgumentParser(
        prog="alpha",
        description="Alpha language interpreter",
    )
    ap.add_argument(
        "file",
        nargs="?",
        help="Alpha source file to run (omit to read from stdin)",
    )
    ap.add_argument(
        "--version",
        action="version",
        version="%(prog)s 0.1.0",
    )
    args = ap.parse_args(argv)

    if args.file:
        try:
            with open(args.file, "r", encoding="utf-8") as fh:
                source = fh.read()
        except OSError as exc:
            print(f"alpha: cannot open '{args.file}': {exc.strerror}", file=sys.stderr)
            sys.exit(1)
    else:
        source = sys.stdin.read()

    try:
        run_source(source)
    except LexerError as exc:
        print(f"Lexer error: {exc}", file=sys.stderr)
        sys.exit(1)
    except ParseError as exc:
        print(f"Parse error: {exc}", file=sys.stderr)
        sys.exit(1)
    except AlphaRuntimeError as exc:
        print(f"Runtime error: {exc}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
