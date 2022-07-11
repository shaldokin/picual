
// include header
#include "picual_c.h"

// writer
#include "picual_writer.cpp"
#include "picual_reader.cpp"

// functionality
PyObject* _dumps(PyObject* obj) {
  std::stringstream buff("");
  Writer writer(W_TO_BUFF);
  writer.dump(obj);
  return PyBytes_FromStringAndSize(writer.buff, writer.size);
};

PyObject* _loads(PyObject* py_data) {

  char* data;
  long int d_len;
  PyBytes_AsStringAndSize(py_data, &data, &d_len);

  Reader reader(R_FROM_BUFF);
  reader.buff = data;

  PyObject* l_obj = Py_None;
  unsigned int l_index = 0;
  _load_next(&reader, 0, l_obj, l_index);
  return l_obj;
}


// integration
void _init(PyObject* get_class_name_func_, PyObject* pickle_dump_func_, PyObject* pickle_load_func_, PyObject* datetime_class_, PyObject* pack_datetime_func_, PyObject* unpack_datetime_func_, PyObject* timedelta_class_, PyObject* pack_timedelta_func_, PyObject* unpack_timedelta_func_, PyObject* unpack_object_func_) {
  eq_name = PyUnicode_FromString("__eq__");
  datetime_to_timestamp_name = PyUnicode_FromString("timestamp");
  get_class_name = "__class__";

  datetime_class = datetime_class_;
  pack_datetime_func = pack_datetime_func_;
  pack_datetime_func_args = PyTuple_New(1);
  unpack_datetime_func = unpack_datetime_func_;
  unpack_datetime_func_args = PyTuple_New(1);

  timedelta_class = timedelta_class_;
  pack_timedelta_func = pack_timedelta_func_;
  pack_timedelta_func_args = PyTuple_New(1);
  unpack_timedelta_func = unpack_timedelta_func_;
  unpack_timedelta_func_args = PyTuple_New(1);

  pickle_dump_func = pickle_dump_func_;
  pickle_dump_func_args = PyTuple_New(1);
  pickle_load_func = pickle_load_func_;
  pickle_load_func_args = PyTuple_New(1);

  get_class_name_func = get_class_name_func_;
  get_class_name_func_args = PyTuple_New(1);

  unpack_object_func = unpack_object_func_;
  unpack_object_func_args = PyTuple_New(2);

  get_name_name = "__name__";
  get_module_name = "__module__";
  get_state_name = "__getstate__";
  get_state_obj = PyUnicode_FromString(get_state_name);


};