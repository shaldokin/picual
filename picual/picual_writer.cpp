
// construction / destructions
Writer::Writer(const char type) {
  this->type = type;
  if (type == W_TO_BUFF)
    this->buff = (char*)malloc(0);
  this->size = 0;
  this->stream = stream;
};

Writer::~Writer() {
  if (this->type == W_TO_BUFF)
    free((void*)this->buff);
};

// dumping
void Writer::write(const char* data, const unsigned int size) {
  if (this->type == W_TO_BUFF) {
    this->buff = (char*)realloc(this->buff, this->size + size);
    memcpy((void*)(((const char*)this->buff) + this->size), data, size);
  } else
    this->stream->write(data, size);
  this->size += size;
}

template <class d_type>
void _dump_num(Writer* w, const d_type value, unsigned int size) {
  w->write((char*)&value, size);
};

template <class s_type>
void _dump_str(Writer* w, const char type, const char* str, const unsigned int len, const unsigned int s_size) {
  _dump_num<unsigned char>(w, type, 1);
  _dump_num<s_type>(w, len, s_size);
  w->write(str, len);
};

void _dump_length(Writer* w, const unsigned char branch, const unsigned int length) {
  if (length < 256) {
    _dump_num<unsigned char>(w, branch, 1);
    _dump_num<unsigned char>(w, length, 1);
  } else if (length < 65536) {
    _dump_num<unsigned char>(w, branch + 1, 1);
    _dump_num<unsigned short>(w, length, 2);
  } else if (length < 4294967296) {
    _dump_num<unsigned char>(w, branch + 2, 1);
    _dump_num<unsigned int>(w, length, 4);
  } else {
    _dump_num<unsigned char>(w, branch + 3, 1);
    _dump_num<unsigned long int>(w, length, 8);
  };
};

void _dump_int(Writer* w, const unsigned char branch, long int value) {
  if (value < 0) {
    if (value > -128) {
    _dump_num<unsigned char>(w, branch, 1);
    _dump_num<char>(w, value, 1);
    } else if (value > -32768) {
    _dump_num<unsigned char>(w, branch + 2, 1);
    _dump_num<short>(w, value, 2);
    } else if (value > -2147483648) {
    _dump_num<unsigned char>(w, branch + 4, 1);
    _dump_num<int>(w, value, 4);
    } else {
    _dump_num<unsigned char>(w, branch + 6, 1);
    _dump_num<long int>(w, value, 8);
    };
  } else {
    if (value < 256) {
    _dump_num<unsigned char>(w, branch + 1, 1);
    _dump_num<unsigned char>(w, value, 1);
    } else if (value < 65536) {
    _dump_num<unsigned char>(w, branch + 3, 1);
    _dump_num<unsigned short>(w, value, 2);
    } else if (value < 4294967296) {
    _dump_num<unsigned char>(w, branch + 5, 1);
    _dump_num<unsigned int>(w, value, 4);
    } else {
    _dump_num<unsigned char>(w, branch + 7, 1);
    _dump_num<unsigned long int>(w, value, 8);
    };
  };
}

bool _check_point(Writer* w, PyObject* obj, unsigned int& point_index) {
  point_index = w->points[obj];
  if (point_index == 0) {
    point_index = w->point_count;
    w->points[obj] = point_index;
    w->point_count++;
    return false;
  } else
    return true;
};

bool _check_refr(Writer* w, PyObject* obj, unsigned int& refr_id) {
  refr_id = refr_ids[obj];
  return refr_id != 0;
};

void _dump_branch(Writer* w, PyObject* container, unsigned int& index, const unsigned int size, py_get_function get, PyObject* obj) {

  // start
  unsigned int r_index = 0;
  unsigned int refr_id = 0;

  // none
  if (obj == Py_None)
    _dump_num<unsigned char>(w, TYPE_NONE, 1);

  // bools
  else if (obj == Py_True)
    _dump_num<unsigned char>(w, TYPE_TRUE, 1);
  else if (obj == Py_False)
    _dump_num<unsigned char>(w, TYPE_FALSE, 1);

  // integers
  else if (PyLong_Check(obj)) {
    long int value = PyLong_AsLong(obj);
    _dump_int(w, TYPE_BYTE, value);

  // float
  } else if (PyFloat_Check(obj)) {
    double value = PyFloat_AsDouble(obj);
    _dump_num<unsigned char>(w, TYPE_DOUBLE, 1);
    _dump_num<double>(w, value, 8);
  }

  // string
  else if (PyUnicode_Check(obj)) {
    const char* value = (const char*)PyUnicode_DATA(obj);
    unsigned int length = PyUnicode_GET_LENGTH(obj);
    _dump_length(w, TYPE_SMALL_STRING, length);
    w->write(value, length);
  }

  // bytes
  else if (PyBytes_Check(obj)) {
    const char* value = PyBytes_AsString(obj);
    unsigned int length = PyBytes_GET_SIZE(obj);
    _dump_length(w, TYPE_SMALL_BYTES, length);
    w->write(value, length);
  }

  // refr
  else if (_check_refr(w, obj, refr_id))
    _dump_length(w, TYPE_SMALL_REFR, refr_id);

  // point
  else if (_check_point(w, obj, r_index))
    _dump_length(w, TYPE_SMALL_POINT, r_index);

  // list
  else if (PyList_Check(obj)) {
    unsigned int length = PyList_Size(obj);
    _dump_length(w, TYPE_SMALL_LIST, length);
    unsigned int i_index = 0;
    while (i_index < length)
      _dump_branch(w, obj, i_index, length, PyList_GetItem, PyList_GetItem(obj, i_index));
  }

  // tuple
  else if (PyTuple_Check(obj)) {
    unsigned int length = PyTuple_Size(obj);
    _dump_length(w, TYPE_SMALL_TUPLE, length);
    unsigned int i_index = 0;
    while (i_index < length)
      _dump_branch(w, obj, i_index, length, PyTuple_GetItem, PyTuple_GetItem(obj, i_index));
  }

  // dict
  else if (PyDict_Check(obj)) {

    // start
    unsigned int length = PyDict_Size(obj);

    // branch
    _dump_length(w, TYPE_SMALL_DICT, length);

    // dump keys and values
    PyObject* d_key;
    PyObject* d_value;
    Py_ssize_t d_pos = 0;
    unsigned int d_index = 0;
    while (PyDict_Next(obj, &d_pos, &d_key, &d_value)) {
      _dump_branch(w, nullptr, d_index, 0, nullptr, d_key);
      _dump_branch(w, nullptr, d_index, 0, nullptr, d_value);
    };
  }

  // check classes
  else {
    PyObject* o_class = PyObject_GetAttrString(obj, "__class__");

    // datetime
    if (o_class == datetime_class) {

      PyTuple_SetItem(pack_datetime_func_args, 0, obj);
      auto dt_get = PyObject_CallObject(pack_datetime_func, pack_datetime_func_args);

      if (PyFloat_Check(dt_get)) {
        double value = PyFloat_AS_DOUBLE(dt_get);
        _dump_num<unsigned char>(w, TYPE_PRECISE_DATETIME, 1);
        _dump_num<double>(w, value, 8);
      } else {
        long int value = PyLong_AsLong(dt_get);
        _dump_num<unsigned char>(w, TYPE_DATETIME, 1);
        _dump_num<unsigned int>(w, value, 4);
      }
    }

    // timedelta
    else if (o_class == timedelta_class) {
      PyTuple_SetItem(pack_timedelta_func_args, 0, obj);
      auto td_get = PyObject_CallObject(pack_timedelta_func, pack_timedelta_func_args);
      if (PyFloat_Check(td_get)) {
        double value = PyFloat_AS_DOUBLE(td_get);
        _dump_num<unsigned char>(w, TYPE_PRECISE_TIMEDELTA, 1);
        _dump_num<double>(w, value, 8);
      } else {
        long int value = PyLong_AsLong(td_get);
        if (value < 256) {
          _dump_num<unsigned char>(w, TYPE_SMALL_TIMEDELTA, 1);
          _dump_num<unsigned char>(w, value, 1);
        } else if (value < 65536) {
          _dump_num<unsigned char>(w, TYPE_SHORT_TIMEDELTA, 1);
          _dump_num<unsigned short>(w, value, 2);
        } else if (value < 4294967296) {
          _dump_num<unsigned char>(w, TYPE_MED_TIMEDELTA, 1);
          _dump_num<unsigned int>(w, value, 4);
        } else {
          unsigned long int l_value = value;
          _dump_num<unsigned char>(w, TYPE_LONG_TIMEDELTA, 1);
          if (value < 0)
            l_value = *((unsigned int*)(&value));
          _dump_num<unsigned long int>(w, l_value, 8);
        };
      }
    }

    // object
    else {

      // from state
      if (PyObject_HasAttrString(obj, get_state_name)) {

        // get name of class
        PyTuple_SetItem(get_class_name_func_args, 0, obj);
        auto c_name_obj = PyObject_CallObject(get_class_name_func, get_class_name_func_args);
        auto c_name = strdup((const char*)PyUnicode_DATA(c_name_obj));
        auto c_name_len = strlen(c_name);
        auto c_value = w->classes[c_name];
        if (c_value == 0) {
          w->classes[c_name] = w->class_count;
          c_value = w->class_count;
          w->class_count++;
          _dump_num<unsigned char>(w, TYPE_DEFINE_CLASS, 1);
          _dump_num<unsigned char>(w, c_name_len, 1);
          w->stream->write(c_name, c_name_len);
          w->size += c_name_len;
        };

        // write object
        if (c_value < 256) {
          _dump_num<unsigned char>(w, TYPE_SMALL_OBJECT, 1);
          _dump_num<unsigned char>(w, c_value, 1);
        } else if (c_value < 65536) {
          _dump_num<unsigned char>(w, TYPE_SHORT_OBJECT, 1);
          _dump_num<unsigned short>(w, c_value, 2);
        } else if (c_value < 4294967296) {
          _dump_num<unsigned char>(w, TYPE_MED_OBJECT, 1);
          _dump_num<unsigned int>(w, c_value, 4);
        } else {
          _dump_num<unsigned char>(w, TYPE_LONG_OBJECT, 1);
          _dump_num<unsigned long>(w, c_value, 8);
        };

        auto g_state = PyObject_CallMethodObjArgs(obj, get_state_obj, nullptr);
        w->dump(g_state);
      }

      // pickle
      else {
        PyTuple_SetItem(pickle_dump_func_args, 0, obj);
        auto p_get = PyObject_CallObject(pickle_dump_func, pickle_dump_func_args);
        const char* p_str = PyBytes_AsString(p_get);
        unsigned int length = PyBytes_GET_SIZE(p_get);
        _dump_length(w, TYPE_SMALL_PICKLED, length);
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
    _dump_length(w, TYPE_SMALL_REPEAT, repeat);

};

void Writer::dump(PyObject* obj) {
  unsigned int index = 0;
  _dump_branch(this, nullptr, index, 0, nullptr, obj);
};

