
# cython: language_level=3
# distutils: language = c++

# import dependencies
import datetime, importlib, pickle
import socket, struct, sys

# extern
cdef extern from "picual_c.cpp":

    cdef int TYPE_MED_TUPLE

    cdef cppclass Reader:
        pass

    cdef cppclass BuffReaderGen(Reader):
        int is_wrong_type
        int g_complete
        gen_next()
        void reset()

    cdef cppclass Writer:
        to_bytes()
        void write_bytes(obj)

    cdef cppclass BuffWriter(Writer):
        pass

    void _dump(obj, stream)
    _dumps(obj)
    _load(stream)
    _loads(data)
    BuffReaderGen* _loadgs(data)

    void _init(get_class_name, pickle_dump_func, pickle_load_func, datetime_class, pack_datetime_func, unpack_timedelta, timedelta_class, pack_timedelta_func, unpack_timedelta_func, unpack_object_func)
    void _store_refr(obj)

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
cdef class PicualDumpNet:

    cdef int count;
    cdef Writer* _writer
    cdef object _sock

    def __init__(self, str addr, int port):
        self.count = 0;
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.bind((addr, port))
        self._sock.setblocking(False)
        self._sock.listen()

    @property
    def data(self):
        return struct.pack('<BI', TYPE_MED_TUPLE, self.count) + self._writer.to_bytes()

    cpdef update(self):
        try:
            conn, addr = self._sock.accept()
            data = b''
            while True:
                get = conn.recv(4096)
                if get:
                    data += get
                else:
                    break
            self.count += 1
            self._writer.write_bytes(data)
        except BlockingIOError:
            pass

    cpdef close(self):
        self._sock.close()

cdef class PicualDumpNetConn:

    cdef str _addr;
    cdef int _port;

    def __init__(self, str addr, int port):
        self._addr = addr
        self._port = port

    cpdef dump(self, obj):
        cdef bytes dump_data = dumps(obj)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((self._addr, self._port))
        sock.send(dump_data)
        sock.close()


# functionality
cpdef dump(obj, stream):
    _dump(obj, stream)

cpdef bytes dumps(obj):
    return _dumps(obj)

cpdef PicualDumpNetConn dumpnc(str addr, int port):
    return PicualDumpNetConn(addr, port)

cpdef PicualDumpNet dumpns(str addr, int port):
    cdef PicualDumpNet dnet = PicualDumpNet(addr, port)
    dnet._writer = new BuffWriter()
    return dnet

cpdef load(stream):
    return _load(stream)

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
