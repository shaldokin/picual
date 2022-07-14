
// reader
class Reader {
  public:

    // state
    bool is_gen = false;
    PyObject* last = Py_None;

    // classes
    std::unordered_map<std::string, unsigned int> classes;
    std::unordered_map<unsigned int, std::string> class_names;
    unsigned int class_count = 1;
    unsigned int get_class(const char* class_name);

    // points
    std::unordered_map<void*, unsigned int> points;
    std::unordered_map<unsigned int, PyObject*> points_from_indices;
    unsigned int point_count = 1;
    void add_point(PyObject* obj, const unsigned int defp=0);

    // references
    std::unordered_map<unsigned int, PyObject*> refrs;
    unsigned int refr_count = 1;

    // read
    virtual PyObject* read_obj();
    virtual void read(char* buff, const unsigned int size);
    virtual PyObject* gen_next();
    virtual void reset();

};

// buff reader
class BuffReader : public Reader {
  public:

    const char* buff;
    BuffReader();
    BuffReader(const char* buff);

    void read(char* buff, const unsigned int size);
};

// stream reader
class StreamReader : public Reader {
  public:

    // members
    PyObject* stream;

    // construction / destruction
    StreamReader(PyObject* stream);
    ~StreamReader();

    // read
    void read(char* buff, const unsigned int size);

};

// reader generator
class ReaderGen : public Reader {
  public:

    int is_wrong_type = 0;

    char g_type = TYPE_NONE;
    char g_last_type;
    unsigned int g_index = 0;
    long unsigned int g_repeat;
    unsigned int g_length;
    long int g_complete = 0;

    char r_type = TYPE_NONE;
    long unsigned int r_length;
    double r_float = 0.0f;
    long int r_int;
    const char* r_str = nullptr;
    long int r_str_len = 0;
    PyObject* r_obj;

    ReaderGen();

    PyObject* gen_next();
    void reset();
    virtual void _init();
};

class BuffReaderGen : public ReaderGen {
  public:

    const char* buff;
    const char* o_buff;

    BuffReaderGen(const char* buff);
    void read(char* buff, const unsigned int size);
    virtual void _init();

};

class StreamReaderGen : public ReaderGen {
  public:

    PyObject* stream;

    StreamReaderGen(PyObject* stream);
    void read(char* buff, const unsigned int size);
    virtual void _init();

};

// reading helper functions
char* reader_num_buffer;

void add_obj_to_reader(PyObject* obj, Reader* r, const bool is_gen, const unsigned char c_type, PyObject*& cont, unsigned int& index);

PyObject* read_from_branch(Reader* r, const unsigned char branch);

unsigned int read_length(Reader* r, const unsigned char branch, const unsigned char type);

void read_next(Reader* r, const bool is_gen, const unsigned char c_type, PyObject*& cont, unsigned int& index);

template <class n_type>
n_type read_num(Reader* r, const unsigned int size);

template <class d_type>
bool read_repeat(Reader* r, const unsigned int length, const unsigned char c_type, PyObject*& cont, unsigned int& index, const char r_type, const bool is_gen, py_check_func, d_type(*set_rvalue)(BuffReader*, d_type), d_type(py_as_func)(PyObject*), PyObject*(py_from_func)(d_type));

void read_str(const char*& str, unsigned int& str_len, Reader* r, const unsigned char branch, const unsigned char type);

