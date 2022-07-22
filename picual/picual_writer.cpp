
// construction / destruction
BuffWriter::BuffWriter() {
  this->buff = (char*)malloc(0);
  this->_init();
};
BuffWriter::~BuffWriter() {
  this->_init();
  free(this->buff);
};

StreamWriter::StreamWriter(PyObject* stream) {
  Py_INCREF(stream);
  this->stream = stream;
  this->_init();
};
StreamWriter::~StreamWriter() {
  this->_init();
  Py_DECREF(this->stream);
};

// write
void Writer::write(const char* data, const unsigned int len) {};

void BuffWriter::write(const char* data, const unsigned int len) {
  this->buff = (char*)realloc(this->buff, this->size + len);
  #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    for (unsigned int w_cur = 0; w_cur < len; w_cur++)
      this->buff[w_cur] = data[len - 1 - (w_cur)];
  #else
    memcpy((void*)(((const char*)this->buff) + this->size), data, len);
  #endif
  this->size += len;
};

void StreamWriter::write(const char* data, const unsigned int len) {
  PyObject* to_write;
  #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    char* r_data = (char*)malloc(len);
    for (unsigned int w_cur = 0; w_cur < len; w_cur++)
      r_data[w_cur] = data[len - 1 - (w_cur)];
    to_write = PyBytes_FromStringAndSize(r_data, len);
  #else
    to_write = PyBytes_FromStringAndSize(data, len);
  #endif
  PyObject_CallMethodObjArgs(this->stream, write_name_obj, to_write, nullptr);
  Py_CLEAR(to_write);
  #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    free((void*)r_data);
  #endif
};

void Writer::write_bytes(PyObject* obj) {
  char* b_str;
  Py_ssize_t b_size;
  PyBytes_AsStringAndSize(obj, &b_str, &b_size);
  this->write(b_str, b_size);
};

void Writer::write_obj(PyObject* obj) {
  unsigned int index = 0;
  write_branch(this, nullptr, index, 0, nullptr, before_dump(obj));
};

PyObject* Writer::to_bytes() {
  return Py_None;
};

PyObject* BuffWriter::to_bytes() {
  return PyBytes_FromStringAndSize(this->buff, this->size);
};

// references
const int Writer::check_refr(PyObject* obj, unsigned int& refr_id) {
  refr_id = this->refr_indices[obj];
  if (refr_id == 0) {
    const char* refr_name = refr_ids[obj];
    if (refr_name != nullptr) {
      write_str<unsigned char>(this, TYPE_DEFINE_REFR, refr_name, strlen(refr_name), 1);
      refr_id = this->refr_count;
      Py_XINCREF(obj);
      this->all_refrs.push_back(obj);
      this->refr_indices[obj] = refr_id;
      this->refr_count++;
      return 1;
    } else
      return 0;
  } else
    return 1;
};

// custom
const int Writer::check_custom(PyObject* obj, unsigned int& custom_index) {
  PyObject* classobj = PyObject_GetAttrString(obj, "__class__");
  custom_index = this->customs[classobj];
  if (custom_index == 0) {
    PyObject* cust = custom_dumper_func_by_class[classobj];
    if (cust == nullptr)
      return 0;
    else {

      // get name of class
      const char* name_str;
      Py_ssize_t name_len;
      get_class_name(obj, name_str, name_len);

      // define class
      write_num<unsigned char>(this, TYPE_DEFINE_CUSTOM, 1);
      write_num<unsigned char>(this, name_len, 1);
      this->write(name_str, name_len);

      // assign index
      custom_index = this->custom_count;
      this->custom_count++;
      this->customs[classobj] = custom_index;
      this->custom_dumpers[custom_index] = cust;
      Py_XINCREF(obj);
      this->all_customs.push_back(obj);

      // finished!
      return 1;
    }
  } else
    return 1;
};

// classes
unsigned int Writer::get_class(PyObject* cls) {

  // get name
  const char* name_str;
  Py_ssize_t name_len;
  get_class_name(cls, name_str, name_len);

  // get/make index
  auto c_index = this->classes[name_str];

  if (c_index == 0) {
    this->classes[name_str] = this->class_count;
    c_index = this->class_count;
    this->class_count++;
    write_num<unsigned char>(this, TYPE_DEFINE_CLASS, 1);
    write_num<unsigned char>(this, name_len, 1);
    this->write(name_str, name_len);
  };

  // finished!
  return c_index;

};

// points
const int Writer::check_point(PyObject* obj, unsigned int& point_index) {
  point_index = this->points[obj];
  if (point_index == 0) {
    point_index = this->point_count;
    Py_XINCREF(obj);
    this->all_points.push_back(obj);
    this->points[obj] = point_index;
    this->point_count++;
    return 0;
  } else
    return 1;
};

// writing helper functions
void write_branch(Writer* w, PyObject* container, unsigned int& index, const unsigned int size, py_get_function get, PyObject* obj) {

  // start
  unsigned int r_index = 0;
  unsigned int refr_id = 0;
  unsigned int custom_index = 0;

  // none
  if (obj == Py_None)
    write_num<unsigned char>(w, TYPE_NONE, 1);

  // bools
  else if (obj == Py_True)
    write_num<unsigned char>(w, TYPE_TRUE, 1);
  else if (obj == Py_False)
    write_num<unsigned char>(w, TYPE_FALSE, 1);

  // integers
  else if (PyLong_Check(obj)) {
    Py_ssize_t value = PyLong_AsLong(obj);
    write_int(w, TYPE_BYTE, value);

  // float
  } else if (PyFloat_Check(obj)) {
    double value = PyFloat_AsDouble(obj);
    write_num<unsigned char>(w, TYPE_DOUBLE, 1);
    write_num<double>(w, value, 8);
  }

  // string
  else if (PyUnicode_Check(obj)) {
    const char* value = (const char*)PyUnicode_DATA(obj);
    unsigned int length = PyUnicode_GET_LENGTH(obj);
    write_length(w, TYPE_SMALL_STRING, length);
    w->write(value, length);
  }

  // bytes
  else if (PyBytes_Check(obj)) {
    const char* value = PyBytes_AsString(obj);
    unsigned int length = PyBytes_GET_SIZE(obj);
    write_length(w, TYPE_SMALL_BYTES, length);
    w->write(value, length);
  }

  // refr
  else if (w->check_refr(obj, refr_id))
    write_length(w, TYPE_SMALL_REFR, refr_id);

  // point
  else if (w->check_point(obj, r_index))
    write_length(w, TYPE_SMALL_POINT, r_index);

  // custom
  else if (w->check_custom(obj, custom_index)) {
    write_length(w, TYPE_SMALL_CUSTOM, custom_index);
    PyObject* c_data = PyObject_CallFunctionObjArgs(w->custom_dumpers[custom_index], obj, nullptr);
    w->write_obj(c_data);
  }

  // list
  else if (PyList_Check(obj)) {
    unsigned int length = PyList_Size(obj);
    write_length(w, TYPE_SMALL_LIST, length);
    unsigned int i_index = 0;
    while (i_index < length)
      write_branch(w, obj, i_index, length, PyList_GetItem, PyList_GetItem(obj, i_index));
  }

  // tuple
  else if (PyTuple_Check(obj)) {
    unsigned int length = PyTuple_Size(obj);
    write_length(w, TYPE_SMALL_TUPLE, length);
    unsigned int i_index = 0;
    while (i_index < length)
      write_branch(w, obj, i_index, length, PyTuple_GetItem, PyTuple_GetItem(obj, i_index));
  }

  // dict
  else if (PyDict_Check(obj)) {

    // start
    unsigned int length = PyDict_Size(obj);

    // branch
    write_length(w, TYPE_SMALL_DICT, length);

    // dump keys and values
    PyObject* d_key;
    PyObject* d_value;
    Py_ssize_t d_pos = 0;
    unsigned int d_index = 0;
    while (PyDict_Next(obj, &d_pos, &d_key, &d_value)) {
      write_branch(w, nullptr, d_index, 0, nullptr, d_key);
      write_branch(w, nullptr, d_index, 0, nullptr, d_value);
    };
  }

  // check classes
  else {
    PyObject* o_class = PyObject_GetAttrString(obj, "__class__");

    // datetime
    if (o_class == datetime_class) {

      auto dt_get = PyObject_CallFunctionObjArgs(pack_datetime_func, obj, nullptr);

      if (PyFloat_Check(dt_get)) {
        double value = PyFloat_AS_DOUBLE(dt_get);
        write_num<unsigned char>(w, TYPE_PRECISE_DATETIME, 1);
        write_num<double>(w, value, 8);
      } else {
        Py_ssize_t value = PyLong_AsLong(dt_get);
        write_num<unsigned char>(w, TYPE_DATETIME, 1);
        write_num<unsigned int>(w, value, 4);
      }
    }

    // timedelta
    else if (o_class == timedelta_class) {
      auto td_get = PyObject_CallFunctionObjArgs(pack_timedelta_func, obj, nullptr);
      if (PyFloat_Check(td_get)) {
        double value = PyFloat_AS_DOUBLE(td_get);
        write_num<unsigned char>(w, TYPE_PRECISE_TIMEDELTA, 1);
        write_num<double>(w, value, 8);
      } else {
        Py_ssize_t value = PyLong_AsLong(td_get);
        if (value < 256) {
          write_num<unsigned char>(w, TYPE_SMALL_TIMEDELTA, 1);
          write_num<unsigned char>(w, value, 1);
        } else if (value < 65536) {
          write_num<unsigned char>(w, TYPE_SHORT_TIMEDELTA, 1);
          write_num<unsigned short>(w, value, 2);
        } else if (value < 4294967296) {
          write_num<unsigned char>(w, TYPE_MED_TIMEDELTA, 1);
          write_num<unsigned int>(w, value, 4);
        } else {
          unsigned long int l_value = value;
          write_num<unsigned char>(w, TYPE_LONG_TIMEDELTA, 1);
          if (value < 0) {
            auto l_value_ptr = (unsigned int*)(&value);
            l_value = *l_value_ptr;
          };
          write_num<unsigned long int>(w, l_value, 8);
        };
      }
    }

    // object
    else {

      // from state
      if (PyObject_HasAttrString(obj, get_state_name)) {

        // write class
        unsigned int c_index = w->get_class(obj);
        write_length(w, TYPE_SMALL_OBJECT, c_index);

        // write state
        auto g_state = PyObject_CallMethodObjArgs(obj, get_state_obj, nullptr);
        w->write_obj(g_state);
        Py_DECREF(g_state);

      }

      // pickle
      else {
        auto p_get = PyObject_CallFunctionObjArgs(pickle_dump_func, obj, nullptr);
        const char* p_str = PyBytes_AsString(p_get);
        unsigned int length = PyBytes_GET_SIZE(p_get);
        write_length(w, TYPE_SMALL_PICKLED, length);
        w->write(p_str, length);
      };

    }

  }

  // update
  index++;

  // get repetition
  unsigned int repeat = 0;
  if (container != nullptr) {
    while (index < size) {
      PyObject* n_obj = get(container, index);
      if (PyObject_CallMethodObjArgs(obj, eq_name, n_obj, nullptr) == Py_True) {
        repeat++;
        index++;
      } else
        break;
    };
  }

  // repeat
  if (repeat > 0)
    write_length(w, TYPE_SMALL_REPEAT, repeat);

};

void write_int(Writer* w, const unsigned char branch, Py_ssize_t value) {
  if (value < 0) {
    if (value > -128) {
      write_num<unsigned char>(w, branch, 1);
      write_num<char>(w, value, 1);
    } else if (value > -32768) {
      write_num<unsigned char>(w, branch + 2, 1);
      write_num<short>(w, value, 2);
    } else if (value > -2147483648) {
      write_num<unsigned char>(w, branch + 4, 1);
      write_num<int>(w, value, 4);
    } else {
      write_num<unsigned char>(w, branch + 6, 1);
      write_num<long int>(w, value, 8);
    };
  } else {
    if (value < 256) {
      write_num<unsigned char>(w, branch + 1, 1);
      write_num<unsigned char>(w, value, 1);
    } else if (value < 65536) {
      write_num<unsigned char>(w, branch + 3, 1);
      write_num<unsigned short>(w, value, 2);
    } else if (value < 4294967296) {
      write_num<unsigned char>(w, branch + 5, 1);
      write_num<unsigned int>(w, value, 4);
    } else {
      write_num<unsigned char>(w, branch + 7, 1);
      write_num<unsigned long int>(w, value, 8);
    };
  };
};

void write_length(Writer* w, const unsigned char branch, const unsigned int length) {
  if (length < 256) {
    write_num<unsigned char>(w, branch, 1);
    write_num<unsigned char>(w, length, 1);
  } else if (length < 65536) {
    write_num<unsigned char>(w, branch + 1, 1);
    write_num<unsigned short>(w, length, 2);
  } else if (length < 4294967296) {
    write_num<unsigned char>(w, branch + 2, 1);
    write_num<unsigned int>(w, length, 4);
  } else {
    write_num<unsigned char>(w, branch + 3, 1);
    write_num<unsigned long int>(w, length, 8);
  };
};

template <class d_type>
void write_num(Writer* w, const d_type value, const unsigned int size) {
  w->write((char*)&value, size);
};

template <class str_size_type>
void write_str(Writer* w, const char type, const char* str, const unsigned int len, const unsigned int str_size) {
  write_num<unsigned char>(w, type, 1);
  write_num<str_size_type>(w, len, str_size);
  w->write(str, len);
};

// maintainence
void Writer::_init() {

  // classes
  this->class_count = 1;
  this->classes = std::unordered_map<std::string, unsigned int>();

  // points
  for (unsigned int p_index = 0; p_index < this->point_count - 1; p_index++)
    Py_XDECREF(this->all_points[p_index]);
  this->all_points = py_vector();
  this->points = py_obj_uint_map();
  this->point_count = 1;

  // customs
  for (unsigned int c_index = 0; c_index < this->custom_count - 1; c_index++)
    Py_XDECREF(this->all_customs[c_index]);
  this->all_customs = py_vector();
  this->customs = py_obj_uint_map();
  this->custom_dumpers = std::unordered_map<unsigned int, PyObject*>();
  this->custom_count = 1;

  // refrs
  for (unsigned int r_index = 0; r_index < this->refr_count - 1; r_index++)
    Py_XDECREF(this->all_refrs[r_index]);
  this->all_refrs = py_vector();
  this->refr_indices = py_obj_uint_map();
  this->refr_count = 1;

};
