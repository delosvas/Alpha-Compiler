"""AST node definitions for the Alpha language."""

from __future__ import annotations
from dataclasses import dataclass, field
from typing import List, Optional


# ---------------------------------------------------------------------------
# Base
# ---------------------------------------------------------------------------

class Node:
    """Base class for all AST nodes."""


# ---------------------------------------------------------------------------
# Expressions
# ---------------------------------------------------------------------------

@dataclass
class NumberLiteral(Node):
    value: float | int


@dataclass
class StringLiteral(Node):
    value: str


@dataclass
class BoolLiteral(Node):
    value: bool


@dataclass
class Identifier(Node):
    name: str


@dataclass
class BinaryOp(Node):
    op: str
    left: Node
    right: Node


@dataclass
class UnaryOp(Node):
    op: str
    operand: Node


@dataclass
class CallExpr(Node):
    callee: str
    args: List[Node] = field(default_factory=list)


# ---------------------------------------------------------------------------
# Statements
# ---------------------------------------------------------------------------

@dataclass
class LetStatement(Node):
    name: str
    value: Node


@dataclass
class AssignStatement(Node):
    name: str
    value: Node


@dataclass
class PrintStatement(Node):
    value: Node


@dataclass
class IfStatement(Node):
    condition: Node
    then_body: List[Node]
    else_body: Optional[List[Node]] = None


@dataclass
class WhileStatement(Node):
    condition: Node
    body: List[Node]


@dataclass
class FuncDeclaration(Node):
    name: str
    params: List[str]
    body: List[Node]


@dataclass
class ReturnStatement(Node):
    value: Optional[Node]


@dataclass
class ExprStatement(Node):
    expr: Node


@dataclass
class Program(Node):
    body: List[Node] = field(default_factory=list)
