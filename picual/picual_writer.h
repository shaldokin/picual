
// writer
class Writer {
  public:

    // classes
    std::unordered_map<std::string, unsigned int> classes;
    unsigned int class_count = 1;
    unsigned int get_class(PyObject* cls);

    // points
    std::unordered_map<PyObject*, unsigned int> points;
    unsigned int point_count = 1;
    const int check_point(PyObject* obj, unsigned int& point_index);

    // state
    PyObject* obj = Py_None;

    // funcationality
    virtual void write(const char* data, const unsigned int len);
    void write_obj(PyObject* obj);
    virtual PyObject* to_bytes();
    void write_bytes(PyObject* obj);

};

// buff writer
class BuffWriter : public Writer {
  public:

    // buffer
    char* buff;
    unsigned int size = 0;

    // construction / destruction
    BuffWriter();
    ~BuffWriter();

    // write
    void write(const char* data, const unsigned int len);
    PyObject* to_bytes();

};

// stream writer
class StreamWriter : public Writer {
  public:

    PyObject* stream;

    // construction / destruction
    StreamWriter(PyObject* stream);
    ~StreamWriter();

    // write
    void write(const char* data, const unsigned int len);

};

// writing helper functions
bool check_refr(Writer* w, PyObject* obj, unsigned int& refr_id);

void write_branch(Writer* w, PyObject* container, unsigned int& index, const unsigned int size, py_get_function get, PyObject* obj);

void write_int(Writer* w, const unsigned char branch, long int value);

void write_length(Writer* w, const unsigned char branch, const unsigned int length);

template <class d_type>
void write_num(Writer* w, const d_type value, const unsigned int size);

template <class str_size_type>
void write_str(Writer* w, const char type, const char* str, const unsigned int len, const unsigned int str_size);
