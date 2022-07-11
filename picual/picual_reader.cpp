
// construction / destruction
Reader::Reader(const char type) {
  this->type = type;
};
Reader::~Reader() {};


// reading
const char* Reader::read(const unsigned int size) {
  auto o_buff = (char*)malloc(size);
  if (this->type == R_FROM_BUFF) {
    memcpy(o_buff, this->buff, size);
    this->buff += size;
  } else
    this->stream->read(o_buff, size);
  return (const char*)o_buff;
};

template <class i_type>
i_type _load_num(Reader* r, const unsigned int size) {
    i_type* o_buff = (i_type*)r->read(size);
    i_type o_val = *o_buff;
    free(o_buff);
    return o_val;
};

unsigned int _load_length(Reader* r, const unsigned char branch, const unsigned char type) {
  if (branch == type)
    return _load_num<unsigned char>(r, 1);
  else if (branch == type + 1)
    return _load_num<unsigned short>(r, 2);
  else if (branch == type + 2)
    return _load_num<unsigned int>(r, 4);
  else
    return _load_num<unsigned long int>(r, 8);
};

void _load_str(const char*& str, unsigned int& str_len, Reader* r, const unsigned char branch, const unsigned char type) {
  str_len = _load_length(r, branch, type);
  str = r->read(str_len);
  str = (const char*)realloc((void*)str, str_len + 1);
  ((char*)str)[str_len] = 0;
};

PyObject* _load_from_branch(Reader* r, const unsigned char branch) {

  // none
  if (branch == TYPE_NONE) {
    return Py_None;
  }

  // bool
  else if (branch == TYPE_TRUE) {
    return Py_True;
  } else if (branch == TYPE_FALSE) {
    return Py_False;
  }

  // int
  else if (branch == TYPE_BYTE) {
    char value = _load_num<char>(r, 1);
    return PyLong_FromLong(value);
  } else if (branch == TYPE_UBYTE) {
    unsigned char value = _load_num<unsigned char>(r, 1);
    return PyLong_FromLong(value);
  } else if (branch == TYPE_SHORT) {
    short value = _load_num<short>(r, 2);
    return PyLong_FromLong(value);
  } else if (branch == TYPE_USHORT) {
    unsigned short value = _load_num<unsigned short>(r, 2);
    return PyLong_FromLong(value);
  } else if (branch == TYPE_INT) {
    int value = _load_num<int>(r, 4);
    return PyLong_FromLong(value);
  } else if (branch == TYPE_UINT) {
    unsigned int value = _load_num<unsigned int>(r, 4);
    return PyLong_FromLong(value);
  } else if (branch == TYPE_LONG) {
    long int value = _load_num<long int>(r, 8);
    return PyLong_FromLong(value);
  } else if (branch == TYPE_ULONG) {
    unsigned long int value = _load_num<unsigned long int>(r, 8);
    return PyLong_FromUnsignedLong(value);
  }

  // float
  else if (branch == TYPE_FLOAT) {
    float value = _load_num<float>(r, 4);
    return PyFloat_FromDouble(value);
  } else if (branch == TYPE_DOUBLE) {
    double value = _load_num<double>(r, 8);
    return PyFloat_FromDouble(value);
  }

  // string / bytes
  else if (branch >= TYPE_SMALL_STRING && branch <= TYPE_LONG_STRING) {
   const char* str_val;
   unsigned int str_len;
   _load_str(str_val, str_len, r, branch, TYPE_SMALL_STRING);
   return PyUnicode_FromStringAndSize(str_val, str_len);
  } else if (branch >= TYPE_SMALL_BYTES && branch <= TYPE_LONG_BYTES) {
   const char* str_val;
   unsigned int str_len;
   _load_str(str_val, str_len, r, branch, TYPE_SMALL_BYTES);
   return PyBytes_FromStringAndSize(str_val, str_len);
  }

  // list / tuple
  else if (branch >= TYPE_SMALL_LIST && branch <= TYPE_LONG_LIST) {
    unsigned int i_size = _load_length(r, branch, TYPE_SMALL_LIST);
    PyObject* i_out = PyList_New(i_size);
    r->points.push_back(i_out);
    unsigned int i_index = 0;
    while (i_index < i_size)
      _load_next(r, TYPE_SMALL_LIST, i_out, i_index);
    return i_out;
  } else if (branch >= TYPE_SMALL_TUPLE && branch <= TYPE_LONG_TUPLE) {
    unsigned int i_size = _load_length(r, branch, TYPE_SMALL_TUPLE);
    PyObject* i_out = PyTuple_New(i_size);
    r->points.push_back(i_out);
    unsigned int i_index = 0;
    while (i_index < i_size)
      _load_next(r, TYPE_SMALL_TUPLE, i_out, i_index);
    return i_out;
  }

  // dict
  else if (branch >= TYPE_SMALL_DICT && branch <= TYPE_LONG_DICT) {
    PyObject* d_out = PyDict_New();
    r->points.push_back(d_out);
    unsigned int d_size = _load_length(r, branch, TYPE_SMALL_DICT);
    PyObject* d_key;
    PyObject* d_value;
    unsigned int d_dummy_index = 0;
    for (unsigned int d_index = 0; d_index < d_size; d_index++) {
      _load_next(r, 0, d_key, d_dummy_index);
      _load_next(r, 0, d_value, d_dummy_index);
      PyDict_SetItem(d_out, d_key, d_value);
    };
    return d_out;
  }

  // datetime
  else if (branch == TYPE_DATETIME) {
    PyObject* ts_value = PyLong_FromLong(_load_num<unsigned int>(r, 4));
    PyTuple_SetItem(unpack_datetime_func_args, 0, ts_value);
    PyObject* dt_out = PyObject_CallObject(unpack_datetime_func, unpack_datetime_func_args);
    r->points.push_back(dt_out);
    return dt_out;
  } else if (branch == TYPE_PRECISE_DATETIME) {
    PyObject* ts_value = PyFloat_FromDouble(_load_num<double>(r, 8));
    PyTuple_SetItem(unpack_datetime_func_args, 0, ts_value);
    PyObject* dt_out = PyObject_CallObject(unpack_datetime_func, unpack_datetime_func_args);
    r->points.push_back(dt_out);
    return dt_out;
  }

  // timedelta
  else if (branch == TYPE_PRECISE_TIMEDELTA) {
    PyObject* td_value = PyLong_FromLong(_load_num<double>(r, 8));
    PyTuple_SetItem(unpack_timedelta_func_args, 0, td_value);
    PyObject* td_out = PyObject_CallObject(unpack_timedelta_func, unpack_timedelta_func_args);
    r->points.push_back(td_out);
    return td_out;
  } else if (branch >= TYPE_SMALL_TIMEDELTA && branch <= TYPE_LONG_TIMEDELTA) {
    PyObject* td_value = PyLong_FromLong(_load_length(r, branch, TYPE_SMALL_TIMEDELTA));
    PyTuple_SetItem(unpack_timedelta_func_args, 0, td_value);
    PyObject* td_out = PyObject_CallObject(unpack_timedelta_func, unpack_timedelta_func_args);
    r->points.push_back(td_out);
    return td_out;
  }

  // object
  else if (branch >= TYPE_SMALL_OBJECT && branch <= TYPE_LONG_OBJECT) {

    // get name
    unsigned int c_index = _load_length(r, branch, TYPE_SMALL_OBJECT);
    std::string c_name = r->class_names[c_index];

    // load the object
    PyObject* o_obj = Py_None;
    unsigned int o_index = 0;
    _load_next(r, 0, o_obj, o_index);

    // build the object
    PyObject* c_name_py = PyUnicode_FromString(c_name.c_str());
    PyTuple_SetItem(unpack_object_func_args, 0, c_name_py);
    PyTuple_SetItem(unpack_object_func_args, 1, o_obj);

    // finished!
    PyObject* o_out = PyObject_CallObject(unpack_object_func, unpack_object_func_args);
    r->points.push_back(o_out);
    return o_out;

  }

  // pickle
  else if (branch >= TYPE_SMALL_PICKLED && branch <= TYPE_LONG_PICKLED) {
    const char* str_val;
    unsigned int str_len;
    _load_str(str_val, str_len, r, branch, TYPE_SMALL_PICKLED);
    PyObject* p_bytes = PyBytes_FromStringAndSize(str_val, str_len);
    PyTuple_SetItem(pickle_load_func_args, 0, p_bytes);
    PyObject* p_obj = PyObject_CallObject(pickle_load_func, pickle_load_func_args);
    free((void*)str_val);
    r->points.push_back(p_obj);
    return p_obj;
  }

  // refr
  else if (branch >= TYPE_SMALL_REFR && branch <= TYPE_LONG_REFR) {
    unsigned int r_id = _load_length(r, branch, TYPE_SMALL_REFR);
    return refr_objs[r_id];
  }

  // point
  else if (branch >= TYPE_SMALL_POINT && branch <= TYPE_LONG_POINT) {
    unsigned int r_index = _load_length(r, branch, TYPE_SMALL_POINT);
    return r->points[r_index - 1];
  }

}

void _add_obj_to_reader(PyObject* obj, Reader* r, const unsigned char c_type, PyObject*& cont, unsigned int& index) {

  // start
  r->last = obj;

  // add to list
  if (c_type == TYPE_SMALL_LIST) {
    PyList_SetItem(cont, index, obj);
    index++;

    // add to tuple
  } else if (c_type == TYPE_SMALL_TUPLE) {
    PyTuple_SetItem(cont, index, obj);
    index++;
  }

  // normal
  else
    cont = obj;

};

void _load_next(Reader* r, const unsigned char c_type, PyObject*& cont, unsigned int& index) {

  // get branch
  unsigned char br = _load_num<unsigned char>(r, 1);

  // class definition?
  if (br == TYPE_DEFINE_CLASS) {
    unsigned char c_len = _load_num<unsigned char>(r, 1);
    char* c_name = (char*)realloc((char*)(r->read(c_len)), c_len + 1);
    c_name[c_len] = 0;
    r->classes[c_name] = r->class_count;
    r->class_names[r->class_count] = c_name;
    r->class_count++;
    free(c_name);
    _load_next(r, c_type, cont, index);
  }

  // repeat
  else if (br >= TYPE_SMALL_REPEAT && br <= TYPE_LONG_REPEAT) {

    // get size of repeat
    auto r_length = _load_length(r, br, TYPE_SMALL_REPEAT);

    // float
    if (PyFloat_Check(r->last)) {
      double f_value = PyFloat_AsDouble(r->last);
      for (unsigned int r_index = 0; r_index < r_length; r_index++)
        _add_obj_to_reader(PyFloat_FromDouble(f_value), r, c_type, cont, index);
    }

    // int
    else if (PyLong_Check(r->last)) {
      long int i_value = PyLong_AsLong(r->last);
      for (unsigned int r_index = 0; r_index < r_length; r_index++)
        _add_obj_to_reader(PyLong_FromLong(i_value), r, c_type, cont, index);
    }

    // string
    else if (PyUnicode_Check(r->last)) {
      const char* s_value = (const char*)PyUnicode_DATA(r->last);
      for (unsigned int r_index = 0; r_index < r_length; r_index++)
        _add_obj_to_reader(PyUnicode_FromString(s_value), r, c_type, cont, index);
    }

    // bytes
    else if (PyFloat_Check(r->last)) {
      const char* s_value = (const char*)PyBytes_AsString(r->last);
      for (unsigned int r_index = 0; r_index < r_length; r_index++)
        _add_obj_to_reader(PyBytes_FromString(s_value), r, c_type, cont, index);
    }

    // the same
    else {
      for (unsigned int r_index = 0; r_index < r_length; r_index++)
        _add_obj_to_reader(r->last, r, c_type, cont, index);
    };


  }

  // load object
  else {
    auto load_obj = _load_from_branch(r, br);
    _add_obj_to_reader(load_obj, r, c_type, cont, index);
  };

};

PyObject* Reader::load() {

  return Py_None;
};
