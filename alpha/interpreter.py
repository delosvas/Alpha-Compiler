"""Tree-walking interpreter for the Alpha language."""

from __future__ import annotations

from typing import Any, Dict, List, Optional

from .ast_nodes import (
    Node, Program, NumberLiteral, StringLiteral, BoolLiteral,
    Identifier, BinaryOp, UnaryOp, CallExpr,
    LetStatement, AssignStatement, PrintStatement,
    IfStatement, WhileStatement, FuncDeclaration, ReturnStatement,
    ExprStatement,
)


class RuntimeError(Exception):
    pass


class ReturnSignal(Exception):
    """Used to unwind the call stack when a return statement is executed."""
    def __init__(self, value: Any):
        self.value = value


class Environment:
    """A lexically scoped variable store."""

    def __init__(self, parent: Optional[Environment] = None):
        self._vars: Dict[str, Any] = {}
        self._parent = parent

    def define(self, name: str, value: Any) -> None:
        self._vars[name] = value

    def assign(self, name: str, value: Any) -> None:
        if name in self._vars:
            self._vars[name] = value
        elif self._parent is not None:
            self._parent.assign(name, value)
        else:
            raise RuntimeError(f"Undefined variable '{name}'")

    def get(self, name: str) -> Any:
        if name in self._vars:
            return self._vars[name]
        if self._parent is not None:
            return self._parent.get(name)
        raise RuntimeError(f"Undefined variable '{name}'")


class AlphaFunction:
    """A user-defined function."""

    def __init__(self, name: str, params: List[str], body: List[Node], closure: Environment):
        self.name = name
        self.params = params
        self.body = body
        self.closure = closure

    def __repr__(self) -> str:
        return f"<func {self.name}>"


class Interpreter:
    """Evaluates an Alpha AST."""

    def __init__(self, output=None):
        """
        Parameters
        ----------
        output : file-like, optional
            Where ``print`` statements write to. Defaults to ``sys.stdout``.
        """
        import sys
        self._output = output or sys.stdout
        self._globals = Environment()

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def run(self, program: Program) -> None:
        for stmt in program.body:
            self._exec(stmt, self._globals)

    # ------------------------------------------------------------------
    # Statement execution
    # ------------------------------------------------------------------

    def _exec(self, node: Node, env: Environment) -> None:
        if isinstance(node, LetStatement):
            value = self._eval(node.value, env)
            env.define(node.name, value)

        elif isinstance(node, AssignStatement):
            value = self._eval(node.value, env)
            env.assign(node.name, value)

        elif isinstance(node, PrintStatement):
            value = self._eval(node.value, env)
            print(self._to_str(value), file=self._output)

        elif isinstance(node, IfStatement):
            condition = self._eval(node.condition, env)
            if self._is_truthy(condition):
                self._exec_block(node.then_body, Environment(env))
            elif node.else_body is not None:
                self._exec_block(node.else_body, Environment(env))

        elif isinstance(node, WhileStatement):
            while self._is_truthy(self._eval(node.condition, env)):
                try:
                    self._exec_block(node.body, Environment(env))
                except ReturnSignal:
                    raise

        elif isinstance(node, FuncDeclaration):
            func = AlphaFunction(node.name, node.params, node.body, env)
            env.define(node.name, func)

        elif isinstance(node, ReturnStatement):
            value = self._eval(node.value, env) if node.value is not None else None
            raise ReturnSignal(value)

        elif isinstance(node, ExprStatement):
            self._eval(node.expr, env)

        else:
            raise RuntimeError(f"Unknown statement type: {type(node).__name__}")

    def _exec_block(self, stmts: List[Node], env: Environment) -> None:
        for stmt in stmts:
            self._exec(stmt, env)

    # ------------------------------------------------------------------
    # Expression evaluation
    # ------------------------------------------------------------------

    def _eval(self, node: Node, env: Environment) -> Any:
        if isinstance(node, NumberLiteral):
            return node.value

        if isinstance(node, StringLiteral):
            return node.value

        if isinstance(node, BoolLiteral):
            return node.value

        if isinstance(node, Identifier):
            return env.get(node.name)

        if isinstance(node, UnaryOp):
            operand = self._eval(node.operand, env)
            if node.op == "-":
                if not isinstance(operand, (int, float)):
                    raise RuntimeError(f"Unary '-' requires a number, got {type(operand).__name__}")
                return -operand
            if node.op == "!":
                return not self._is_truthy(operand)
            raise RuntimeError(f"Unknown unary operator: {node.op}")

        if isinstance(node, BinaryOp):
            return self._eval_binary(node, env)

        if isinstance(node, CallExpr):
            return self._eval_call(node, env)

        raise RuntimeError(f"Unknown expression type: {type(node).__name__}")

    def _eval_binary(self, node: BinaryOp, env: Environment) -> Any:
        left = self._eval(node.left, env)
        right = self._eval(node.right, env)
        op = node.op

        if op == "+":
            if isinstance(left, str) or isinstance(right, str):
                return self._to_str(left) + self._to_str(right)
            return self._num(left, op) + self._num(right, op)
        if op == "-":
            return self._num(left, op) - self._num(right, op)
        if op == "*":
            return self._num(left, op) * self._num(right, op)
        if op == "/":
            r = self._num(right, op)
            if r == 0:
                raise RuntimeError("Division by zero")
            return self._num(left, op) / r
        if op == "%":
            r = self._num(right, op)
            if r == 0:
                raise RuntimeError("Modulo by zero")
            return self._num(left, op) % r
        if op == "==":
            return left == right
        if op == "!=":
            return left != right
        if op == "<":
            return self._num(left, op) < self._num(right, op)
        if op == "<=":
            return self._num(left, op) <= self._num(right, op)
        if op == ">":
            return self._num(left, op) > self._num(right, op)
        if op == ">=":
            return self._num(left, op) >= self._num(right, op)
        if op == "&&":
            return self._is_truthy(left) and self._is_truthy(right)
        if op == "||":
            return self._is_truthy(left) or self._is_truthy(right)

        raise RuntimeError(f"Unknown binary operator: {op}")

    def _eval_call(self, node: CallExpr, env: Environment) -> Any:
        callee = env.get(node.callee)
        if not isinstance(callee, AlphaFunction):
            raise RuntimeError(f"'{node.callee}' is not a function")
        if len(node.args) != len(callee.params):
            raise RuntimeError(
                f"Function '{node.callee}' expects {len(callee.params)} argument(s), "
                f"got {len(node.args)}"
            )
        call_env = Environment(callee.closure)
        for param, arg_node in zip(callee.params, node.args):
            call_env.define(param, self._eval(arg_node, env))
        try:
            self._exec_block(callee.body, call_env)
        except ReturnSignal as ret:
            return ret.value
        return None

    # ------------------------------------------------------------------
    # Utilities
    # ------------------------------------------------------------------

    @staticmethod
    def _is_truthy(value: Any) -> bool:
        if value is None:
            return False
        if isinstance(value, bool):
            return value
        if isinstance(value, (int, float)):
            return value != 0
        if isinstance(value, str):
            return len(value) > 0
        return True

    @staticmethod
    def _num(value: Any, op: str) -> float | int:
        if not isinstance(value, (int, float)):
            raise RuntimeError(
                f"Operator '{op}' requires numbers, got {type(value).__name__}"
            )
        return value

    @staticmethod
    def _to_str(value: Any) -> str:
        if isinstance(value, bool):
            return "true" if value else "false"
        if value is None:
            return "null"
        return str(value)
