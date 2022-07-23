
// include source
#include "picual_c.h"
#include "picual_writer.cpp"
#include "picual_reader.cpp"

// debug
void print(PyObject* obj) {
  std::cout << (const char*)PyUnicode_DATA(PyObject_Repr(obj)) << "\n";
};

template <class p_type>
void print(p_type text) {
  std::cout << text << "\n";
};


void print_rcount(PyObject* obj) {
  std::cout << obj->ob_refcnt << "\n";
};

// functionality
void _dump(PyObject* obj, PyObject* stream, bool store_points) {
  StreamWriter writer(stream);
  writer.store_points = store_points;
  writer.write_start();
  writer.write_obj(obj);
};

PyObject* _dumps(PyObject* obj, bool store_points) {
  BuffWriter writer;
  writer.store_points = store_points;
  writer.write_start();
  writer.write_obj(obj);
  return writer.to_bytes();
};

PyObject* _load(PyObject* stream) {
  StreamReader reader(stream);
  return reader.read_obj();
};

PyObject* _loads(PyObject* py_data) {

  // get data from python
  char* data;
  Py_ssize_t d_len;
  PyBytes_AsStringAndSize(py_data, &data, &d_len);

  // read object from buffer
  BuffReader reader(data);
  return reader.read_obj();

};

ReaderGen* _loadg(PyObject* stream) {
  return new StreamReaderGen(stream);
};

ReaderGen* _loadgs(PyObject* py_data) {

  // get data
  char* data;
  Py_ssize_t d_len;
  PyBytes_AsStringAndSize(py_data, &data, &d_len);

  // create reader
  BuffReaderGen* reader = new BuffReaderGen(data);
  return reader;

};

PyObject* _front_of_buffer_gen(Py_ssize_t size) {
  BuffWriter writer;
  write_length(&writer, TYPE_SMALL_LIST, size);
  return writer.to_bytes();
}

// network
void _open_dump_gen(Writer* w) {
  write_num<unsigned char>(w, TYPE_LONG_LIST, 1);
  write_num<long unsigned int>(w, 0, 8);
};

void _close_dump_gen(Writer* w, const long int count) {
  PyObject_CallMethodObjArgs(((StreamWriter*)w)->stream, seek_name_obj, pos_1_obj, nullptr);
  write_num<long unsigned int>(w, count, 8);
  PyObject_CallMethodObjArgs(((StreamWriter*)w)->stream, close_name_obj, nullptr);
};



// references
void _store_refr(PyObject* name, PyObject* obj) {
  char* refr_name = PyBytes_AsString(name);
  Py_INCREF(obj);
  refr_ids[obj] = strdup(refr_name);
  refr_objs[refr_name] = obj;
};

// tools
void get_class_name(PyObject* cls, const char*& name, Py_ssize_t& len) {
  auto name_obj = PyObject_CallFunctionObjArgs(get_class_name_func, cls, nullptr);
  name = strdup((const char*)PyUnicode_DATA(name_obj));
  len = PyUnicode_GET_LENGTH(name_obj);
};

// custom
void _add_custom_dumper(PyObject* cls, PyObject* func) {
  Py_INCREF(cls);
  Py_INCREF(func);
  custom_dumper_func_by_class[cls] = func;
  custom_dumper_class_by_func[func] = cls;
};

void _add_custom_loader(PyObject* cls, PyObject* func) {
  Py_INCREF(cls);
  Py_INCREF(func);
  custom_loader_func_by_class[cls] = func;
  custom_loader_class_by_func[func] = cls;
};


void _set_before_dump(PyObject* func) {
  Py_XINCREF(func);
  Py_XDECREF(before_dump_func);
  before_dump_func = func;
};

PyObject* before_dump(PyObject* data) {
  if (before_dump_func == nullptr)
    return data;
  else
    return PyObject_CallFunctionObjArgs(before_dump_func, data, nullptr);
};


// integration
void _init(PyObject* config) {

  eq_name = PyUnicode_FromString("__eq__");
  datetime_to_timestamp_name = PyUnicode_FromString("timestamp");
  get_class_name_name = "__class__";

  datetime_class = PyDict_GetItemString(config, "datetime_class");
  pack_datetime_func = PyDict_GetItemString(config, "pack_datetime");
  Py_INCREF(pack_datetime_func);
  unpack_datetime_func = PyDict_GetItemString(config, "unpack_datetime");

  timedelta_class = PyDict_GetItemString(config, "timedelta_class");
  pack_timedelta_func = PyDict_GetItemString(config, "pack_timedelta");
  unpack_timedelta_func = PyDict_GetItemString(config, "unpack_timedelta");

  pickle_dump_func = PyDict_GetItemString(config, "pickle_dump");
  pickle_load_func = PyDict_GetItemString(config, "pickle_load");

  get_obj_from_refr = PyDict_GetItemString(config, "get_obj_from_refr");
  get_class_name_func = PyDict_GetItemString(config, "get_class_name");

  unpack_object_func = PyDict_GetItemString(config, "unpack_object");

  get_name_name = "__name__";
  get_module_name = "__module__";
  get_state_name = "__getstate__";
  get_state_obj = PyUnicode_FromString(get_state_name);
  set_state_obj = PyUnicode_FromString("__setstate__");

  write_name_obj = PyUnicode_FromString("write");
  read_name_obj = PyUnicode_FromString("read");
  seek_name_obj = PyUnicode_FromString("seek");
  close_name_obj = PyUnicode_FromString("close");
  pos_1_obj = PyLong_FromLong(1);

  reader_num_buffer = (char*)malloc(8);

};

// dev
void _test() {

    const char* value = "ilikekake";

    // list
    print("LIST BEFORE ADDING");
    auto cont1 = PyList_New(1);
    print_rcount(cont1);

    print("LIST ITEM BEFORE ADDING");
    auto item1 = PyBytes_FromString(value);
    print_rcount(item1);

    PyList_SetItem(cont1, 0, item1);

    print("LIST AFTER ADDING");
    print_rcount(cont1);

    print("LIST ITEM AFTER ADDING");
    print_rcount(item1);

    print("LIST ITEM WITHIN LIST");
    print_rcount(PyList_GetItem(cont1, 0));

    print("LIST ITEM WITHIN LIST (AFTER DECR ITEM OUTSIDE OF LIST)");
    Py_DECREF(item1);
    print_rcount(PyList_GetItem(cont1, 0));

    print("");

    // tuple
    print("TUPLE BEFORE ADDING");
    auto cont2 = PyTuple_New(1);
    print_rcount(cont2);

    print("TUPLE ITEM BEFORE ADDING");
    auto item2 = PyBytes_FromString(value);
    print_rcount(item2);

    PyTuple_SetItem(cont2, 0, item2);

    print("TUPLE AFTER ADDING");
    print_rcount(cont2);

    print("TUPLE ITEM AFTER ADDING");
    print_rcount(item2);

    print("TUPLE ITEM WITHIN TUPLE");
    print_rcount(PyTuple_GetItem(cont2, 0));

    print("TUPLE ITEM WITHIN TUPLE (AFTER DECR ITEM OUTSIDE OF TUPLE)");
    Py_DECREF(item2);
    print_rcount(PyTuple_GetItem(cont2, 0));

};