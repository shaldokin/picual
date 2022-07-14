
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

// references
void _store_refr(PyObject* name, PyObject* obj) {
  char* refr_name = PyBytes_AsString(name);
  refr_ids[obj] = strdup(refr_name);
  refr_objs[refr_name] = obj;
  Py_INCREF(obj);
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

  reader_num_buffer = (char*)malloc(8);

};