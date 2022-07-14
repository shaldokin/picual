
# cython: language_level=3
# distutils: language = c++

# import dependencies
import datetime, importlib, pickle
import socket, sys

# extern
cdef extern from "picual_c.cpp":
    _dumps(obj)
    _loads(data)
    void _init(get_class_name, pickle_dump_func, pickle_load_func, datetime_class, pack_datetime_func, unpack_timedelta, timedelta_class, pack_timedelta_func, unpack_timedelta_func, unpack_object_func)
    void _store_refr(obj)

    cdef cppclass BuffReaderGen:
        int is_wrong_type
        int g_complete
        gen_next()
        void reset()

    BuffReaderGen* _loadgs(data)


# util
cpdef get_obj_from_refr(str name):

    cdef str m_name
    m_name = name[:name.find('.')]

    cdef str c_name
    c_name = name[name.find('.'):]

    cdef str e_str
    e_str = f'sys.modules["{m_name}"]{c_name}'

    try:
        return eval(e_str)
    except KeyError:
        importlib.import_module(m_name)
        return eval(e_str)


# object
cpdef unpack_object(str c_name):
    cls = get_obj_from_refr(c_name)
    n_obj = object.__new__(cls)
    return n_obj


# generator
cdef class PicualLoadGenerator:
    cdef BuffReaderGen* reader

    def __next__(self):
        if self.reader.g_complete:
            self.reader.reset()
            raise StopIteration
        return self.reader.gen_next()

    def __iter__(self):
        while self.reader.g_complete == 0:
            yield self.reader.gen_next()
        self.reader.reset()

# connection
cdef class PicualDumpCon:
    cdef object _sock


# functionality
cpdef bytes dumps(obj):
    return _dumps(obj)

cpdef loads(bytes data):
    return _loads(data)

cpdef loadgs(bytes data):
    cdef PicualLoadGenerator gen = PicualLoadGenerator()
    gen.reader = _loadgs(data)
    if gen.reader.is_wrong_type:
        raise TypeError('You can only load a generator from a list, tuple, or dictionary')
    return gen

# datetime
cpdef pack_datetime(obj):
    cdef double tsv = obj.timestamp()
    if obj.microsecond:
        return tsv
    else:
        return int(tsv)

# timedelta
cpdef pack_timedelta(obj):
    cdef double tsv = obj.total_seconds()
    if obj.microseconds:
        return tsv
    else:
        return int(tsv)

cpdef unpack_timedelta(obj):
    return datetime.timedelta(seconds=obj)

# utility
cpdef store_refr(obj):
  _store_refr(obj)
  return obj

# initiate
datetime_class = datetime.datetime
timedelta_class = datetime.timedelta
pickle_dump_func = pickle.dumps
pickle_load_func = pickle.loads
unpack_datetime = datetime.datetime.fromtimestamp

cpdef get_class_name(obj):
    return obj.__class__.__module__ + '.' + obj.__class__.__name__

_init(get_class_name, pickle_dump_func, pickle_load_func, datetime_class, pack_datetime, unpack_datetime, timedelta_class, pack_timedelta, unpack_timedelta, unpack_object)
