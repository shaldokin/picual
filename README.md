
# Picual
Picual is an alternative pickling library that allows for some extra functionality such as
* load a pickled tuple/list/dict as a generator
* map singletons or other non-picklable objects to a reference, allowing simpler multiprocessing
* dumping over a network
* emphasis on compression
* fully compatible with standard pickling

## Installation
You can currently install picual like so
```
pip install git+https://www.github.com/shaldokin/picual.git
```

## Dumping and Loading
Picual supports the standard pickling functions
* dump(object, stream)
* dumps(object)
* load(stream)
* loads(data)

Picual also uses the same `__getstate__` and `__setstate__` as pickle, and is 100% compatible with pickle.
Objects that do not have this, will just use the standard pickling protocol.

# Load as a Generator
If you dumped a list/tuple/dict, you can load it as a generator.
This is useful for a large dumped sequences.
You can do so like this:
* loadg(stream)
* loadgs(data)


## Dump to a Network
You can open a network for dumping to like so:
```python
import picual
netdump = picual.dumpn('localhost', 1234)
```
You can either use `dumpn` or `dumpns`.

You dump to the connection by opening a connect with `dumpc` and then dumping to it.
```python
import picual

conn = picual.dumpc('localhost', 1234)

conn.dump(123)
conn.dump('picualing is fun')
```

If dumping to a buffer with `dumpns`, you can access the data with the `data` property.

## Using References
Say you wanted to use singletons, or lambda functions, or other things that are generated and cannot be pickled, you can simply have the reference to the object stored.
You can define a reference like so:
```python
import picual

singleton = object()
picual.store_refr('SINGLETON', singleton)
```
Us want to give the singleton an identifier, allowing it to keep the same mapping regardless of changes to the system.

You can also assign a reference using a decorator like so
```python
import picual

@picual.store_refr('foo')
def foo():
    ...
```

This is especially useful for when local functions are creating within functions.

