# Useful Concepts
## Iterable
An iterable is any object that can return its members one at a time. By definition, iterables support the iterator protocol, which specifies how object members are returned when an object is used in an iterator.
Python has two commonly used types of iterables:

* Sequences
* Generators

## Sequences
An iterable which supports efficient element access using integer indices.

Examples: `String`, `list`, `tuples`, `Byte sequences`, `byte array`, and `range` object

We will say that a sequence has a deterministic ordering, but a collection does not.

## Generators
An expression that returns an iterator. It looks like a normal expression followed by a for clause defining a loop variable, range, and an optional if clause. The combined expression generates values for an enclosing function:
```python
sum(i*i for i in range(10))         # sum of squares 0, 1, 4, ... 81
```

## Collections
Python collection, unlike a sequence, does not have a deterministic ordering. Examples include sets and dictionaries. In a collection, while ordering is arbitrary, physically, they do have an order.
Every time we visit a set, we get its items in the same order. However, if we add or remove an item, it may affect the order.
Example: `Sets` and `Dictionaries`

### Sets
A set, in Python, is like a mathematical set in Python. It does not hold duplicates. 
```python
nums={2,1,3,2}
```

### Dictionaries
Think of a dictionary as a real-life dictionary. It holds key-value pairs.
```python
a={'name':1,'dob':2}
```

# Useful Methods
## Enumerate
When you use enumerate(), the function gives you back two loop variables:

* The count of the current iteration
* The value of the item at the current iteration

## Named Tuples
Named tuples assign meaning to each position in a tuple and allow for more readable, self-documenting code. They can be used wherever regular tuples are used, and they add the ability to access fields by name instead of position index.
```python
from collections import namedtuple

Point = namedtuple('Point', ['x', 'y'])
p = Point(11, y=22)     # instantiate with positional or keyword arguments

x, y = p                # unpack like a regular tuple

p.x + p.y               # fields also accessible by name

Color = namedtuple('Color', 'red green blue')
Pixel = namedtuple('Pixel', Point._fields + Color._fields)
Pixel(11, 22, 128, 255, 0)
```
