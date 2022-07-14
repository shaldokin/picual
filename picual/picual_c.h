
// include dependencies
#include <Python.h>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <iostream>
#include <utility>
#include "picual_types.h"

// classes
class Reader;
class BuffReaderGen;
class Writer;
typedef std::pair<PyObject*, PyObject*> custom_cb_pair;
typedef PyObject* (*py_get_function)(PyObject*, Py_ssize_t);
typedef Py_ssize_t (*py_size_function)(PyObject*);
typedef int (*py_check_func)(PyObject*);

// source
#include "picual_writer.h"
#include "picual_reader.h"

// util
void _dump(PyObject* obj, PyObject* stream);
PyObject* _dumps(PyObject* obj);
PyObject* _load(PyObject* stream);
PyObject* _loads(PyObject* data);
ReaderGen* _loadg(PyObject* file);
ReaderGen* _loadgs(PyObject* data);

// custom
PyObject* custom_dumper_args;

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

// network
void _open_network(Writer* w);
void _close_network(Writer* w, const long int count);

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
PyObject* set_state_obj;
PyObject* read_name_obj;
PyObject* write_name_obj;
PyObject* seek_name_obj;
PyObject* pos_1_obj;
PyObject* close_name_obj;

void _init(PyObject* config);
