
// include source
#include "picual_c.h"
#include "picual_writer.cpp"
#include "picual_reader.cpp"

// functionality
void _dump(PyObject* obj, PyObject* stream) {
  StreamWriter writer(stream);
  writer.write_obj(obj);
};

PyObject* _dumps(PyObject* obj) {
  BuffWriter writer;
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
  long int d_len;
  PyBytes_AsStringAndSize(py_data, &data, &d_len);

  // read object from buffer
  BuffReader reader(data);
  return reader.read_obj();

};

BuffReaderGen* _loadgs(PyObject* py_data) {

  // get data
  char* data;
  long int d_len;
  PyBytes_AsStringAndSize(py_data, &data, &d_len);

  // create reader
  BuffReaderGen* reader = new BuffReaderGen(data);
  return reader;

};

// network
void _open_network(Writer* w) {
  write_num<unsigned char>(w, TYPE_LONG_LIST, 1);
  write_num<long unsigned int>(w, 0, 8);
};

void _close_network(Writer* w, const long int count) {
  PyObject_CallMethodObjArgs(((StreamWriter*)w)->stream, seek_name_obj, pos_1_obj, nullptr);
  write_num<long unsigned int>(w, count, 8);
  PyObject_CallMethodObjArgs(((StreamWriter*)w)->stream, close_name_obj, nullptr);
};

// references
void _store_refr(PyObject* name, PyObject* obj) {
  char* refr_name = PyBytes_AsString(name);
  refr_ids[obj] = strdup(refr_name);
  refr_objs[refr_name] = obj;
  Py_INCREF(obj);
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

// integration
void _init(PyObject* config) {

  eq_name = PyUnicode_FromString("__eq__");
  datetime_to_timestamp_name = PyUnicode_FromString("timestamp");
  get_class_name = "__class__";

  datetime_class = PyDict_GetItemString(config, "datetime_class");
  pack_datetime_func = PyDict_GetItemString(config, "pack_datetime");
  Py_INCREF(pack_datetime_func);
  pack_datetime_func_args = PyTuple_New(1);
  unpack_datetime_func = PyDict_GetItemString(config, "unpack_datetime");
  unpack_datetime_func_args = PyTuple_New(1);

  timedelta_class = PyDict_GetItemString(config, "timedelta_class");
  pack_timedelta_func = PyDict_GetItemString(config, "pack_timedelta");
  pack_timedelta_func_args = PyTuple_New(1);
  unpack_timedelta_func = PyDict_GetItemString(config, "unpack_timedelta");
  unpack_timedelta_func_args = PyTuple_New(1);

  pickle_dump_func = PyDict_GetItemString(config, "pickle_dump");
  pickle_dump_func_args = PyTuple_New(1);
  pickle_load_func = PyDict_GetItemString(config, "pickle_load");
  pickle_load_func_args = PyTuple_New(1);

  get_class_name_func = PyDict_GetItemString(config, "get_class_name");
  get_class_name_func_args = PyTuple_New(1);

  unpack_object_func = PyDict_GetItemString(config, "unpack_object");
  unpack_object_func_args = PyTuple_New(1);

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

  custom_dumper_args = PyTuple_New(1);

};