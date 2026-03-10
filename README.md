# Alpha Language Interpreter

An interpreter for the **Alpha** programming language — a small, dynamically
typed scripting language designed for learning and experimentation.

---

## Features

| Feature | Example |
|---|---|
| Variables | `let x = 42;` |
| Arithmetic | `print 2 + 3 * 4;` |
| Strings | `print "Hello, " + name;` |
| Booleans | `print true && !false;` |
| Comparisons | `print x >= 10;` |
| If / else | `if (x > 0) { … } else { … }` |
| While loops | `while (i < 10) { i = i + 1; }` |
| Functions | `func add(a, b) { return a + b; }` |
| Recursion | ✔ |
| Comments | `# this is a comment` |

---

## Quick Start

```bash
# Run a source file
python -m alpha.main examples/hello.alpha

# Read from stdin
echo 'print "hi";' | python -m alpha.main
```

After `pip install -e .` the `alpha` command is available:

```bash
alpha examples/fibonacci.alpha
```

---

## Language Reference

### Variables

Declare a variable with `let`. Reassignment uses plain `=`.

```alpha
let x = 10;
x = x + 1;
print x;   # 11
```

### Arithmetic

`+  -  *  /  %`

String concatenation is done with `+`.

```alpha
print "Hello, " + "World!";
```

### Booleans and comparisons

`true`, `false`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `!`

### If / else

```alpha
if (x > 0) {
    print "positive";
} else {
    print "non-positive";
}
```

### While

```alpha
let i = 0;
while (i < 5) {
    print i;
    i = i + 1;
}
```

### Functions

```alpha
func factorial(n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

print factorial(6);   # 720
```

### Comments

Lines beginning with `#` (or anything after `#`) are ignored.

---

## Project Layout

```
alpha/
  __init__.py      – public API (run_source)
  lexer.py         – tokenizer
  ast_nodes.py     – AST node dataclasses
  parser.py        – recursive-descent parser
  interpreter.py   – tree-walking interpreter
  main.py          – CLI entry point
examples/
  hello.alpha
  fibonacci.alpha
  fizzbuzz.alpha
tests/
  test_lexer.py
  test_parser.py
  test_interpreter.py
```

---

## Running Tests

```bash
pip install pytest
python -m pytest tests/ -v
```

