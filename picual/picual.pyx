
# cython: language_level=3
# distutils: language = c++

# import dependencies
import datetime, hashlib, importlib, pickle
import threading, socket, struct, sys, time

# extern
cdef extern from "picual_c.cpp":

    cdef int TYPE_LONG_LIST

    cdef cppclass Reader:
        pass

    cdef cppclass ReaderGen(Reader):
        int is_wrong_type
        int g_complete
        gen_next()
        void reset()

    cdef cppclass BuffReaderGen(ReaderGen):
        pass

    cdef cppclass Writer:
        to_bytes()
        void write_bytes(obj)
        void write_obj(obj)

    cdef cppclass BuffWriter(Writer):
        pass

    cdef cppclass StreamWriter(Writer):
        StreamWriter(fileobj)

    void _dump(obj, stream)
    _dumps(obj)
    _load(stream)
    _loads(data)
    ReaderGen* _loadg(data)
    ReaderGen* _loadgs(data)

    void _init(config)
    void _store_refr(name, obj)

    void _add_custom_dumper(cls, func)
    void _add_custom_loader(cls, func)

    void _open_network(Writer* w)
    void _close_network(Writer* w, int count)


# make config
cdef dict picual_config = {}

# generator
cdef class PicualLoadGenerator:

    cdef ReaderGen* reader

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

    cdef int count
    cdef int is_stream
    cdef Writer* writer
    cdef object sock
    cdef int thread_over
    cdef int use_thread
    cdef object thread

    def __init__(self, int is_stream, str addr, int port, int use_thread):
        self.count = 0;
        self.is_stream = is_stream
        self.use_thread = use_thread
        if use_thread:
            self.thread_over = 0
            self.thread = threading.Thread(target=dump_net_thread, args=(self, addr, port, lambda: self.thread_over))
            self.thread.start()
            time.sleep(0.1)
        else:
            self.sock = open_socket_for_dump_net(self, addr, port)

    def __del__(self):
        self.close()

    @property
    def data(self):
        if self.is_stream:
            return b''
        else:
            return struct.pack('<BQ', TYPE_LONG_LIST, self.count) + self.writer.to_bytes()

    @property
    def redumped(self):
        return dumps(loads(self.data))

    cpdef update(self):
        while update_dump_net(self, self.sock):
            pass

    cpdef close(self):
        self.thread_over = 1
        if self.is_stream:
            _close_network(self.writer, self.count)
        if self.use_thread:
            self.thread.join()
        else:
            self.update()
            self.sock.close()

    def __enter__(self):
        return self

    def __exit__(self, *args, **kwargs):
        self.close()

cdef open_socket_for_dump_net(PicualDumpNet net, str addr, int port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((addr, port))
    sock.setblocking(False)
    sock.listen()
    return sock

cpdef dump_net_thread(self, str addr, int port, thread_over):
    sock = open_socket_for_dump_net(self, addr, port)
    while not thread_over():
        while update_dump_net(self, sock):
            pass
    sock.close()

cdef int update_dump_net(PicualDumpNet net, sock):
    cdef bytes data = b''
    cdef bytes get
    try:
        conn, addr = sock.accept()
        while 1:
            get = conn.recv(4096)
            if get:
                data += get
            else:
                break
            net.count += 1
            obj = loads(data)
            net.writer.write_obj(obj)
        return 1
    except BlockingIOError:
        return 0

cdef class PicualDumpNetConn:

    cdef str addr;
    cdef int port;

    def __init__(self, str addr, int port):
        self.addr = addr
        self.port = port

    cpdef dump(self, obj):
        cdef bytes dump_data = dumps(obj)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((self.addr, self.port))
        sock.send(dump_data)
        sock.close()


# custom
def custom_dumper(cls):
    def custom_dumper_wrapper(func):
        _add_custom_dumper(cls, func)
        return func
    return custom_dumper_wrapper

def custom_loader(cls):
    def custom_loader_wrapper(func):
        _add_custom_loader(cls, func)
        return func
    return custom_loader_wrapper


# object
cpdef unpack_object(str c_name):
    cls = get_obj_from_refr(c_name)
    n_obj = object.__new__(cls)
    return n_obj

cpdef get_class_name(obj):
    return obj.__class__.__module__ + '.' + obj.__class__.__name__

picual_config['unpack_object'] = unpack_object
picual_config['get_class_name'] = get_class_name
picual_config['pickle_dump'] = pickle.dumps
picual_config['pickle_load'] = pickle.loads

# datetime
cpdef pack_datetime(obj):
    cdef double tsv = obj.timestamp()
    if obj.microsecond:
        return tsv
    else:
        return int(tsv)

picual_config['datetime_class'] = datetime.datetime
picual_config['pack_datetime'] = pack_datetime
picual_config['unpack_datetime'] = datetime.datetime.fromtimestamp

# timedelta
cpdef pack_timedelta(obj):
    cdef double tsv = obj.total_seconds()
    if obj.microseconds:
        return tsv
    else:
        return int(tsv)

cpdef unpack_timedelta(obj):
    return datetime.timedelta(seconds=obj)

picual_config['timedelta_class'] = datetime.timedelta
picual_config['pack_timedelta'] = pack_timedelta
picual_config['unpack_timedelta'] = unpack_timedelta

# referencing
def store_refr(str name, obj=None):
    r_name = store_refr_name(name)
    if obj is None:
        def store_refr_decor(func):
            _store_refr(r_name, func)
            return func
        return store_refr_decor
    else:
        _store_refr(r_name, obj)
        return obj


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
picual_config['get_obj_from_refr'] = get_obj_from_refr

cpdef bytes store_refr_name(str name):
    if len(name) < 16:
        return name.encode()
    else:
        return hashlib.md5(name.encode()).digest()
picual_config['store_refr_name'] = store_refr_name

# initialize
_init(picual_config)

# picual functionality
cpdef dump(obj, stream):
    _dump(obj, stream)

cpdef bytes dumps(obj):
    return _dumps(obj)

cpdef dump_to(obj, str filename):
    with open(filename, 'wb') as dump_file:
        _dump(obj, dump_file)

cpdef PicualDumpNetConn dumpnc(str addr, int port):
    return PicualDumpNetConn(addr, port)

cpdef PicualDumpNet dumpn(fileobj, str addr, int port, use_thread=False):
    cdef PicualDumpNet dnet = PicualDumpNet(True, addr, port, use_thread)
    dnet.writer = new StreamWriter(fileobj)
    _open_network(dnet.writer)
    return dnet

cpdef PicualDumpNet dumpn_to(str filename, str addr, int port, use_thread=False):
    return dumpn(open(filename, 'wb'), addr, port, use_thread)

cpdef PicualDumpNet dumpns(str addr, int port, use_thread=False):
    cdef PicualDumpNet dnet = PicualDumpNet(False, addr, port, use_thread)
    dnet.writer = new BuffWriter()
    return dnet

cpdef load(stream):
    return _load(stream)

cpdef load_from(str filename):
    with open(filename, 'rb') as load_file:
        return _load(load_file)

cpdef loads(bytes data):
    return _loads(data)

cpdef loadg(stream):
    cdef PicualLoadGenerator gen = PicualLoadGenerator()
    gen.reader = _loadg(stream)
    if gen.reader.is_wrong_type:
        raise TypeError('You can only load a generator from a list, tuple, or dictionary')
    return gen

cpdef loadg_from(str filename):
    return loadg(open(filename, 'rb'))

cpdef loadgs(bytes data):
    cdef PicualLoadGenerator gen = PicualLoadGenerator()
    gen.reader = _loadgs(data)
    if gen.reader.is_wrong_type:
        raise TypeError('You can only load a generator from a list, tuple, or dictionary')
    return gen

