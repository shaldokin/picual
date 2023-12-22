
// include dependencies
#include <Python.h>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <iostream>
#include <utility>
#include "picual_types.h"

// sys
bool is_big_endian = false;

// debug
template <class p_type>
void print(p_type text);
void print(PyObject* obj);
void print_rcount(PyObject* obj);

// classes
class Reader;
class BuffReaderGen;
class Writer;
typedef std::pair<PyObject*, PyObject*> custom_cb_pair;
typedef PyObject* (*py_get_function)(PyObject*, Py_ssize_t);
typedef Py_ssize_t (*py_size_function)(PyObject*);
typedef int (*py_check_func)(PyObject*);
typedef std::vector<PyObject*> py_vector;
typedef std::unordered_map<PyObject*, unsigned int> py_obj_uint_map;

// source
#include "picual_writer.h"
#include "picual_reader.h"

// util
void _dump(PyObject* obj, PyObject* stream, bool store_points=true);
PyObject* _dumps(PyObject* obj, bool store_points=true);
PyObject* _load(PyObject* stream);
PyObject* _loads(PyObject* data);
ReaderGen* _loadg(PyObject* file);
ReaderGen* _loadgs(PyObject* data);
PyObject* _front_of_buffer_gen(Py_ssize_t size);

// tools
void get_class_name(PyObject* obj, const char*& name, Py_ssize_t& len);

// custom
std::unordered_map<PyObject*, PyObject*> custom_dumper_func_by_class;
std::unordered_map<PyObject*, PyObject*> custom_dumper_class_by_func;
void _add_custom_dumper(PyObject* cls, PyObject* func);

std::unordered_map<PyObject*, PyObject*> custom_loader_func_by_class;
std::unordered_map<PyObject*, PyObject*> custom_loader_class_by_func;
void _add_custom_loader(PyObject* cls, PyObject* func);

// references
std::unordered_map<std::string, PyObject*> refr_objs;
std::unordered_map<PyObject*, const char*> refr_ids;

void _store_refr(PyObject* name, PyObject* obj);

// dump gen
void _open_dump_gen(Writer* w);
void _close_close_gen(Writer* w, const Py_ssize_t count);

// customization
PyObject* before_dump_func = nullptr;
PyObject* before_dump(PyObject* data);
void _set_before_dump(PyObject* func);


// integration
PyObject* eq_name;
const char* get_class_name_name;
PyObject* datetime_class;
PyObject* datetime_to_timestamp_name;
PyObject* pack_datetime_func;
PyObject* unpack_datetime_func;
PyObject* timedelta_class;
PyObject* pack_timedelta_func;
PyObject* unpack_timedelta_func;
PyObject* pickle_dump_func;
PyObject* pickle_load_func;
PyObject* unpack_object_func;
PyObject* get_class_name_func;
const char* get_is_name;
const char* get_name_name;
const char* get_module_name;
const char* get_state_name;
PyObject* get_state_obj;
PyObject* set_state_obj;
PyObject* read_name_obj;
PyObject* write_name_obj;
PyObject* seek_name_obj;
PyObject* pos_1_obj;
PyObject* close_name_obj;
PyObject* get_obj_from_refr;

void _init(PyObject* config);
