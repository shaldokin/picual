
// include dependencies
#include <Python.h>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <iostream>
#include "picual_types.h"

// classes
class Writer;
typedef PyObject* (*py_get_function)(PyObject*, Py_ssize_t);
typedef Py_ssize_t (*py_size_function)(PyObject*);

// writer
#define W_TO_BUFF 1
#define W_TO_STREAM 2

class Writer {
  public:

    // members
    char type;
    char* buff;
    const char* r_buff;
    std::ostream* stream = nullptr;
    unsigned int size = 0;

    std::unordered_map<std::string, unsigned int> classes;
    unsigned int class_count = 1;

    std::unordered_map<PyObject*, unsigned int> points;
    unsigned int point_count = 1;

    PyObject* obj = Py_None;

    // construction / destruction
    Writer(const char type);
    ~Writer();

    // funcationality
    void write(const char* data, const unsigned int size);
    void dump(PyObject* obj);

};

// reader
#define R_FROM_BUFF 1
#define R_FROM_STREAM 2

class Reader {
  public:

    char type;
    std::istream* stream;
    const char* buff;

    PyObject* last = Py_None;

    std::unordered_map<std::string, unsigned int> classes;
    std::unordered_map<unsigned int, std::string> class_names;
    unsigned int class_count = 1;

    std::vector<PyObject*> points;

    Reader(const char type);
    ~Reader();

    const char* read(const unsigned int size);
    PyObject* load();

};

void _load_next(Reader* r, const unsigned char c_type, PyObject*& cont, unsigned int& index);

// util
PyObject* _dumps(PyObject* obj);
PyObject* _loads(PyObject* data);

// references
std::unordered_map<unsigned int, PyObject*> refr_objs;
std::unordered_map<PyObject*, unsigned int> refr_ids;
unsigned int refr_count = 1;

void _store_refr(PyObject* obj);

// integration
PyObject* eq_name;
const char* get_class_name;
PyObject* datetime_class;
PyObject* datetime_to_timestamp_name;
PyObject* pack_datetime_func;
PyObject* pack_datetime_func_args;
PyObject* unpack_datetime_func;
PyObject* unpack_datetime_func_args;
PyObject* timedelta_class;
PyObject* pack_timedelta_func;
PyObject* pack_timedelta_func_args;
PyObject* unpack_timedelta_func;
PyObject* unpack_timedelta_func_args;
PyObject* pickle_dump_func;
PyObject* pickle_dump_func_args;
PyObject* pickle_load_func;
PyObject* pickle_load_func_args;
PyObject* unpack_object_func;
PyObject* unpack_object_func_args;
PyObject* get_class_name_func;
PyObject* get_class_name_func_args;
const char* get_is_name;
const char* get_name_name;
const char* get_module_name;
const char* get_state_name;
PyObject* get_state_obj;

void _init(PyObject* get_class_name_func, PyObject* pickle_dump_func, PyObject* pickle_load_func, PyObject* datetime_class, PyObject* pack_datetime_func, PyObject* unpack_datetime_func, PyObject* timedelta_class, PyObject* pack_timedelta_func, PyObject* unpack_object_func);
