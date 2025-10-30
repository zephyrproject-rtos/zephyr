# Stub file syntax

Variables with annotations do not need to be assigned a value.
So by convention, we omit them in the stub file.

```py
x: int
```


## For function

```py
def func_1(code: str) -> int: ...
```

## For default arguments

```py
def func_2(a: int, b: int = ...) -> int: ...
```

## For class

```py
class Foo: ...
class Bar:
    a: int
    b: list[str]
    c: str

    def get(self) -> str: ...
```

For more to see https://mypy.readthedocs.io/en/stable/stubs.html
