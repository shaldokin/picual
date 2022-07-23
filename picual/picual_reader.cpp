
// construction
void Reader::reset() {};

BuffReader::BuffReader() {};
BuffReader::BuffReader(const char* buff) {
  this->buff = buff;
};

StreamReader::StreamReader(PyObject* stream) {
  this->stream = stream;
  Py_INCREF(this->stream);
};
StreamReader::~StreamReader() {
  Py_DECREF(this->stream);
};

ReaderGen::ReaderGen() {};

BuffReaderGen::BuffReaderGen(const char* buff) {
  this->buff = buff;
  this->o_buff = buff;
  this->is_gen = true;
  this->_init();
};

StreamReaderGen::StreamReaderGen(PyObject* stream) {
  Py_INCREF(stream);
  this->stream = stream;
  this->is_gen = true;
  this->_init();
};

void ReaderGen::_init() {

  // set buffer to beginning
  this->g_complete = 0;
  this->g_type = TYPE_NONE;

  // reset refrs
  this->refr_count = 1;
  this->refrs = std::unordered_map<unsigned int, PyObject*>();

  // reset points
  this->store_points = true;
  this->point_count = 1;
  this->points = std::unordered_map<void*, unsigned int>();
  this->points_from_indices = std::unordered_map<unsigned int, PyObject*>();

  // detect generator type
  this->add_point(Py_None);
  unsigned char branch = read_num<unsigned char>(this, 1);
  if (branch >= TYPE_SMALL_LIST && branch <= TYPE_LONG_LIST) {
    this->g_length = read_length(this, branch, TYPE_SMALL_LIST);
    this->g_index = 0;
    this->g_type = TYPE_SMALL_LIST;
   } else if (branch >= TYPE_SMALL_TUPLE && branch <= TYPE_LONG_TUPLE) {
    this->g_length = read_length(this, branch, TYPE_SMALL_TUPLE);
    this->g_index = 0;
    this->g_type = TYPE_SMALL_LIST;
   } else if (branch >= TYPE_SMALL_DICT && branch <= TYPE_LONG_DICT) {
    this->g_length = read_length(this, branch, TYPE_SMALL_DICT);
    this->g_index = 0;
    this->g_type = TYPE_SMALL_DICT;
  } else
    this->is_wrong_type = 1;

  // finished!
  this->g_last_type = this->g_type;

};

void ReaderGen::reset() {
  this->_init();
};

void BuffReaderGen::_init() {
  this->buff = this->o_buff;
  ReaderGen::_init();
};

void StreamReaderGen::_init() {
  PyObject* zero = PyLong_FromLong(0);
  PyObject_CallMethodObjArgs(this->stream, seek_name_obj, zero, nullptr);
  ReaderGen::_init();
  Py_CLEAR(zero);
};

// points
void Reader::add_point(PyObject* obj, const unsigned int defp) {
  if (this->store_points) {
    unsigned int p_index = this->points[obj];
    if (p_index == 0) {
      if (defp == 0) {
        p_index = this->point_count;
        this->point_count++;
      } else
        p_index = defp;
      Py_INCREF(obj);
      this->points[obj] = p_index;
      this->points_from_indices[p_index] = obj;
    };
  };
};

// read
unsigned int Reader::get_class(const char* class_name) {
  unsigned int c_id = classes[class_name];
  if (c_id == 0) {
    c_id = this->class_count;
    class_names[c_id] = class_name;
    this->class_count++;
  };
  return c_id;
};

void Reader::read(char* buff, const unsigned int size) {};

void BuffReader::read(char* buff, const unsigned int size) {
  memcpy(buff, this->buff, size);
  this->buff += size;
};

void BuffReaderGen::read(char* buff, const unsigned int size) {
  memcpy(buff, this->buff, size);
  this->buff += size;
};

void StreamReader::read(char* buff, const unsigned int size) {
  PyObject* p_size = PyLong_FromLong(size);
  PyObject* r_data = PyObject_CallMethodObjArgs(this->stream, read_name_obj, p_size, nullptr);
  char* bytes = PyBytes_AsString(r_data);
  memcpy(buff, bytes, size);
  Py_CLEAR(p_size);
  Py_CLEAR(r_data);
};

void StreamReaderGen::read(char* buff, const unsigned int size) {
  PyObject* p_size = PyLong_FromLong(size);
  PyObject* r_data = PyObject_CallMethodObjArgs(this->stream, read_name_obj, p_size, nullptr);
  char* bytes = PyBytes_AsString(r_data);
  memcpy(buff, bytes, size);
  Py_CLEAR(p_size);
  Py_CLEAR(r_data);
};

PyObject* Reader::read_obj() {
  PyObject* r_obj = Py_None;
  unsigned int r_index = 0;
  read_next(this, false, 0, r_obj, r_index);
  return r_obj;
};

// general
void add_obj_to_reader(PyObject* obj, Reader* r, const bool is_gen, const unsigned char c_type, PyObject*& cont, unsigned int& index) {

  // start
  r->last = obj;
  Py_INCREF(obj);

  // add to list
  if (c_type == TYPE_SMALL_LIST && !is_gen)
    PyList_SetItem(cont, index, obj);

    // add to tuple
  else if (c_type == TYPE_SMALL_TUPLE && !is_gen)
    PyTuple_SetItem(cont, index, obj);

  // normal
  else
    cont = obj;

  // finished
  index++;

};

unsigned int read_length(Reader* r, const unsigned char branch, const unsigned char type) {
  if (branch == type)
    return read_num<unsigned char>(r, 1);
  else if (branch == type + 1)
    return read_num<unsigned short>(r, 2);
  else if (branch == type + 2)
    return read_num<unsigned int>(r, 4);
  else
    return read_num<unsigned long int>(r, 8);
};

void read_next(Reader* r, const bool is_gen, const unsigned char c_type, PyObject*& cont, unsigned int& index) {

  // get branch
  unsigned char branch = read_num<unsigned char>(r, 1);

  // don't store refrs?
  if (branch == TYPE_SETTING_DONT_STORE_POINTS) {
    r->store_points = false;
    read_next(r, is_gen, c_type, cont, index);
  }

  // class definition?
  else if (branch == TYPE_DEFINE_CLASS) {
    unsigned char c_len = read_num<unsigned char>(r, 1);
    char* c_name = (char*)malloc(c_len + 1);
    r->read(c_name, c_len);
    c_name[c_len] = 0;
    r->get_class(c_name);
    read_next(r, is_gen, c_type, cont, index);
    free(c_name);
  }

  // custom load definition?
  else if (branch == TYPE_DEFINE_CUSTOM) {

    unsigned char c_len = read_num<unsigned char>(r, 1);
    char* c_name = (char*)malloc(c_len + 1);
    r->read(c_name, c_len);
    c_name[c_len] = 0;

    unsigned int c_id = r->custom_count;
    PyObject* str_arg = PyUnicode_FromStringAndSize(c_name, c_len);
    PyObject* c_obj = PyObject_CallFunctionObjArgs(get_obj_from_refr, str_arg, nullptr);
    Py_CLEAR(str_arg);
    r->custom_loaders[c_id] = custom_loader_func_by_class[c_obj];
    r->custom_count++;

    read_next(r, is_gen, c_type, cont, index);
    free(c_name);
  }

  // refr definition
  else if (branch == TYPE_DEFINE_REFR) {

    // get name of reference
    unsigned char r_len = read_num<unsigned char>(r, 1);
    char* r_name = (char*)malloc(r_len + 1);
    r->read(r_name, r_len);
    r_name[r_len] = 0;

    // store reference
    auto refr = refr_objs[r_name];
    Py_INCREF(refr);
    r->refrs[r->refr_count] = refr;
    r->refr_count++;

    // clean up
    free((void*)r_name);

    // get next object
    read_next(r, is_gen, c_type, cont, index);
  }

  // repeat
  else if (branch >= TYPE_SMALL_REPEAT && branch <= TYPE_LONG_REPEAT) {

    // start
    BuffReaderGen* gen;
    unsigned int r_length = read_length(r, branch, TYPE_SMALL_REPEAT);
    if (is_gen) {
      gen = (BuffReaderGen*)r;
      gen->g_repeat = r_length - 1;
      gen->g_type = TYPE_SMALL_REPEAT;
    };

    // float
    if (PyFloat_Check(r->last)) {

      // generator?
      if (is_gen) {
        gen->r_type = TYPE_FLOAT;
        gen->r_float = PyFloat_AsDouble(gen->last);
        PyObject* n_value = PyFloat_FromDouble(gen->r_float);
        add_obj_to_reader(n_value, r, is_gen, c_type, cont, index);
        Py_DECREF(n_value);
      }

      // normal
      else {
        double r_value = PyFloat_AsDouble(r->last);
        for (unsigned int r_index = 0; r_index < r_length; r_index++) {
          PyObject* n_value = PyFloat_FromDouble(r_value);
          add_obj_to_reader(n_value, r, is_gen, c_type, cont, index);
          Py_DECREF(n_value);
        };
      };
    }

    // int
    else if (PyLong_Check(r->last)) {

      // generator?
      if (is_gen) {
        gen->r_type = TYPE_INT;
        gen->r_int = PyLong_AsLong(gen->last);
        PyObject* n_value = PyLong_FromLong(gen->r_int);
        add_obj_to_reader(n_value, r, is_gen, c_type, cont, index);
        Py_DECREF(n_value);
      }

      // normal
      else {
        Py_ssize_t r_value = PyLong_AsLong(r->last);
        for (unsigned int r_index = 0; r_index < r_length; r_index++) {
          PyObject* n_value = PyLong_FromLong(r_value);
          add_obj_to_reader(n_value, r, is_gen, c_type, cont, index);
          Py_DECREF(n_value);
        };
      };
    }

    // string
    else if (PyUnicode_Check(r->last)) {

      // generator?
      if (is_gen) {
        gen->r_type = TYPE_SMALL_STRING;
        gen->r_str = PyUnicode_AsUTF8AndSize(gen->last, &(gen->r_str_len));
        PyObject* n_value = PyUnicode_FromStringAndSize(gen->r_str, gen->r_str_len);
        add_obj_to_reader(n_value, r, is_gen, c_type, cont, index);
        Py_DECREF(n_value);
      }

      // normal
      else {
        Py_ssize_t r_str_len = 0;
        const char* r_value = PyUnicode_AsUTF8AndSize(gen->last, &r_str_len);
        for (unsigned int r_index = 0; r_index < r_length; r_index++) {
          PyObject* n_value = PyUnicode_FromStringAndSize(r_value, r_str_len);
          add_obj_to_reader(n_value, r, is_gen, c_type, cont, index);
          Py_DECREF(n_value);
        };
      };
    }

    // bytes
    else if (PyBytes_Check(r->last)) {

      // generator?
      if (is_gen) {
        gen->r_type = TYPE_SMALL_BYTES;
        char* r_str;
        PyBytes_AsStringAndSize(gen->last, &r_str, &(gen->r_str_len));
        if (gen->r_str != nullptr)
          free((void*)gen->r_str);
        gen->r_str = (const char*)malloc(gen->r_str_len);
        memcpy((char*)gen->r_str, r_str, gen->r_str_len);
        PyObject* n_value = PyBytes_FromStringAndSize(r_str, gen->r_str_len);
        add_obj_to_reader(n_value, r, is_gen, c_type, cont, index);
        Py_DECREF(n_value);
      }

      // normal
      else {
        Py_ssize_t r_str_len = 0;
        char* r_value;
        PyBytes_AsStringAndSize(gen->last, &r_value, &r_str_len);
        for (unsigned int r_index = 0; r_index < r_length; r_index++) {
          PyObject* n_value = PyBytes_FromStringAndSize(r_value, r_str_len);
          add_obj_to_reader(n_value, r, is_gen, c_type, cont, index);
          Py_DECREF(n_value);
        };
      };
    }

    // the same
    else {

      // generator?
      if (is_gen) {
        gen->r_type = TYPE_NONE;
        Py_INCREF(gen->last);
        add_obj_to_reader(gen->last, r, is_gen, c_type, cont, index);
        Py_DECREF(gen->last);
      }

      // normal
      else {
        for (unsigned int r_index = 0; r_index < r_length; r_index++)
          add_obj_to_reader(r->last, r, is_gen, c_type, cont, index);
      };

    };


  }

  // load object
  else {
    auto load_obj = read_from_branch(r, branch);
    add_obj_to_reader(load_obj, r, is_gen, c_type, cont, index);
    Py_DECREF(load_obj);
  };

};

template <class n_type>
n_type read_num(Reader* r, const unsigned int size) {
  r->read(reader_num_buffer, size);
  n_type o_val = *((n_type*)reader_num_buffer);
  return o_val;
};

void read_str(const char*& str, unsigned int& str_len, Reader* r, const unsigned char branch, const unsigned char type) {
  str_len = read_length(r, branch, type);
  str = (const char*)malloc(str_len + 1);
  r->read((char*)str, str_len);
  ((char*)str)[str_len] = 0;
};

PyObject* read_from_branch(Reader* r, const unsigned char branch) {

  // none
  if (branch == TYPE_NONE) {
    Py_INCREF(Py_None);
    return Py_None;
  }

  // bool
  else if (branch == TYPE_TRUE) {
    Py_INCREF(Py_True);
    return Py_True;
  } else if (branch == TYPE_FALSE) {
    Py_INCREF(Py_False);
    return Py_False;
  }

  // int
  else if (branch == TYPE_BYTE) {
    char value = read_num<char>(r, 1);
    return PyLong_FromLong(value);
  } else if (branch == TYPE_UBYTE) {
    unsigned char value = read_num<unsigned char>(r, 1);
    return PyLong_FromLong(value);
  } else if (branch == TYPE_SHORT) {
    short value = read_num<short>(r, 2);
    return PyLong_FromLong(value);
  } else if (branch == TYPE_USHORT) {
    unsigned short value = read_num<unsigned short>(r, 2);
    return PyLong_FromLong(value);
  } else if (branch == TYPE_INT) {
    int value = read_num<int>(r, 4);
    return PyLong_FromLong(value);
  } else if (branch == TYPE_UINT) {
    unsigned int value = read_num<unsigned int>(r, 4);
    return PyLong_FromLong(value);
  } else if (branch == TYPE_LONG) {
    Py_ssize_t value = read_num<long int>(r, 8);
    return PyLong_FromLong(value);
  } else if (branch == TYPE_ULONG) {
    unsigned long int value = read_num<unsigned long int>(r, 8);
    return PyLong_FromUnsignedLong(value);
  }

  // float
  else if (branch == TYPE_FLOAT) {
    float value = read_num<float>(r, 4);
    return PyFloat_FromDouble(value);
  } else if (branch == TYPE_DOUBLE) {
    double value = read_num<double>(r, 8);
    return PyFloat_FromDouble(value);
  }

  // string / bytes
  else if (branch >= TYPE_SMALL_STRING && branch <= TYPE_LONG_STRING) {
   const char* str_val;
   unsigned int str_len;
   read_str(str_val, str_len, r, branch, TYPE_SMALL_STRING);
   return PyUnicode_FromStringAndSize(str_val, str_len);
  } else if (branch >= TYPE_SMALL_BYTES && branch <= TYPE_LONG_BYTES) {
   const char* str_val;
   unsigned int str_len;
   read_str(str_val, str_len, r, branch, TYPE_SMALL_BYTES);
   return PyBytes_FromStringAndSize(str_val, str_len);
  }

  // list / tuple
  else if (branch >= TYPE_SMALL_LIST && branch <= TYPE_LONG_LIST) {
    unsigned int i_size = read_length(r, branch, TYPE_SMALL_LIST);
    PyObject* i_out = PyList_New(i_size);
    r->add_point(i_out);
    unsigned int i_index = 0;
    while (i_index < i_size)
      read_next(r, false, TYPE_SMALL_LIST, i_out, i_index);
    return i_out;
  } else if (branch >= TYPE_SMALL_TUPLE && branch <= TYPE_LONG_TUPLE) {
    unsigned int i_size = read_length(r, branch, TYPE_SMALL_TUPLE);
    PyObject* i_out = PyTuple_New(i_size);
    unsigned int p_index = r->point_count;
    r->point_count++;
    unsigned int i_index = 0;
    while (i_index < i_size)
      read_next(r, false, TYPE_SMALL_TUPLE, i_out, i_index);
    r->add_point(i_out, p_index);
    return i_out;
  }

  // dict
  else if (branch >= TYPE_SMALL_DICT && branch <= TYPE_LONG_DICT) {
    PyObject* d_out = PyDict_New();
    r->add_point(d_out);
    unsigned int d_size = read_length(r, branch, TYPE_SMALL_DICT);
    PyObject* d_key;
    PyObject* d_value;
    unsigned int d_dummy_index = 0;
    for (unsigned int d_index = 0; d_index < d_size; d_index++) {
      read_next(r, false, 0, d_key, d_dummy_index);
      read_next(r, false, 0, d_value, d_dummy_index);
      PyDict_SetItem(d_out, d_key, d_value);
    };
    return d_out;
  }

  // datetime
  else if (branch == TYPE_DATETIME) {
    PyObject* ts_value = PyLong_FromLong(read_num<unsigned int>(r, 4));
    PyObject* dt_out = PyObject_CallFunctionObjArgs(unpack_datetime_func, ts_value, nullptr);
    Py_CLEAR(ts_value);
    r->add_point(dt_out);
    return dt_out;
  } else if (branch == TYPE_PRECISE_DATETIME) {
    PyObject* ts_value = PyFloat_FromDouble(read_num<double>(r, 8));
    PyObject* dt_out = PyObject_CallFunctionObjArgs(unpack_datetime_func, ts_value, nullptr);
    Py_CLEAR(ts_value);
    r->add_point(dt_out);
    return dt_out;
  }

  // timedelta
  else if (branch == TYPE_PRECISE_TIMEDELTA) {
    PyObject* td_value = PyLong_FromLong(read_num<double>(r, 8));
    PyObject* td_out = PyObject_CallFunctionObjArgs(unpack_timedelta_func, td_value, nullptr);
    Py_CLEAR(td_value);
    r->add_point(td_out);
    return td_out;
  } else if (branch >= TYPE_SMALL_TIMEDELTA && branch <= TYPE_LONG_TIMEDELTA) {
    PyObject* td_value = PyLong_FromLong(read_length(r, branch, TYPE_SMALL_TIMEDELTA));
    PyObject* td_out = PyObject_CallFunctionObjArgs(unpack_timedelta_func, td_value, nullptr);
    Py_CLEAR(td_value);
    r->add_point(td_out);
    return td_out;
  }

  // object
  else if (branch >= TYPE_SMALL_OBJECT && branch <= TYPE_LONG_OBJECT) {

    // get name
    unsigned int c_index = read_length(r, branch, TYPE_SMALL_OBJECT);
    std::string c_name = r->class_names[c_index];
    PyObject* c_name_py = PyUnicode_FromString(c_name.c_str());

    // create new object
    PyObject* o_out = PyObject_CallFunctionObjArgs(unpack_object_func, c_name_py, nullptr);
    Py_DECREF(c_name_py);
    r->add_point(o_out);

    // load the state
    PyObject* o_state = Py_None;
    unsigned int o_index = 0;
    read_next(r, false, 0, o_state, o_index);

    // apply the state
    PyObject_CallMethodObjArgs(o_out, set_state_obj, o_state, nullptr);

    // finished!
    Py_DECREF(o_state);
    return o_out;

  }

  // pickle
  else if (branch >= TYPE_SMALL_PICKLED && branch <= TYPE_LONG_PICKLED) {
    const char* str_val;
    unsigned int str_len;
    read_str(str_val, str_len, r, branch, TYPE_SMALL_PICKLED);
    PyObject* p_bytes = PyBytes_FromStringAndSize(str_val, str_len);
    PyObject* p_obj = PyObject_CallFunctionObjArgs(pickle_load_func, p_bytes, nullptr);
    Py_DECREF(p_bytes);
    r->add_point(p_obj);
    free((void*)str_val);
    return p_obj;
  }

  // refr
  else if (branch >= TYPE_SMALL_REFR && branch <= TYPE_LONG_REFR) {
    unsigned int r_id = read_length(r, branch, TYPE_SMALL_REFR);
    PyObject* r_value = r->refrs[r_id];
    Py_INCREF(r_value);
    return r_value;
  }

  // point
  else if (branch >= TYPE_SMALL_POINT && branch <= TYPE_LONG_POINT) {
    unsigned int r_index = read_length(r, branch, TYPE_SMALL_POINT);
    PyObject* point = r->points_from_indices[r_index];
    Py_INCREF(point);
    return point;
  }

  // custom
  else if (branch >= TYPE_SMALL_CUSTOM && branch <= TYPE_LONG_CUSTOM) {

    // get loader
    unsigned int c_index = read_length(r, branch, TYPE_SMALL_CUSTOM);
    PyObject* c_loader = r->custom_loaders[c_index];

    // load the state
    PyObject* o_state = Py_None;
    unsigned int o_index = 0;
    read_next(r, false, 0, o_state, o_index);

    // return the object
    PyObject* obj_return = PyObject_CallFunctionObjArgs(c_loader, o_state, nullptr);
    Py_DECREF(o_state);
    return obj_return;

  };

}


// generator
PyObject* Reader::gen_next() {
  Py_INCREF(Py_None);
  return Py_None;
};

PyObject* ReaderGen::gen_next() {

  // start
  PyObject* n_obj;

  // normal
  if (this->g_type == TYPE_NONE)
    read_next(this, true, 0, n_obj, this->g_index);

  // list / tuple
  else if (this->g_type == TYPE_SMALL_LIST)
    read_next(this, true, 0, n_obj, this->g_index);

  // dict
  else if (this->g_type == TYPE_SMALL_DICT) {

    // load values
    unsigned int d_dummy_index;
    PyObject* d_key;
    read_next(this, false, 0, d_key, d_dummy_index);
    PyObject* d_value;
    read_next(this, false, 0, d_value, d_dummy_index);

    // create tuple
    n_obj = PyTuple_New(2);
    PyTuple_SetItem(n_obj, 0, d_key);
    PyTuple_SetItem(n_obj, 1, d_value);

    // finished!
    this->g_index++;

  }

  // repeat
  else if (this->g_type == TYPE_SMALL_REPEAT) {

    if (this->r_type == TYPE_FLOAT)
      n_obj = PyFloat_FromDouble(this->r_float);
    else if (this->r_type == TYPE_INT)
      n_obj = PyLong_FromLong(this->r_int);
    else if (this->r_type == TYPE_SMALL_STRING)
      n_obj = PyUnicode_FromStringAndSize(this->r_str, this->r_str_len);
    else if (this->r_type == TYPE_SMALL_BYTES)
      n_obj = PyBytes_FromStringAndSize(this->r_str, this->r_str_len);
    else {
      n_obj = this->last;
      Py_INCREF(n_obj);
    };

    this->g_index++;
    this->g_repeat--;
    if (this->g_repeat == 0)
      this->g_type = this->g_last_type;

  };

  // is this complete?
  if (this->g_last_type == TYPE_SMALL_LIST || this->g_last_type == TYPE_SMALL_DICT)
    this->g_complete = this->g_index == this->g_length;

  // finished!
  return n_obj;

};