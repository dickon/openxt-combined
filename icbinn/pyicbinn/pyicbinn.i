/* pyicbinn.i */
%module pyicbinn
%include "stdint.i"
%{
#include <icbinn.h>
%}

/* Typemaps based on the read/write example from the SWIG documentation:
 *
 * http://www.swig.org/Doc1.3/SWIGDocumentation.html#Typemaps_multi_argument_typemaps
 *
 */

/* For icbinn_pread and icbinn_rand: */

%typemap(in) (void *buf, size_t count) {
    if (!PyInt_Check($input)) {
        PyErr_SetString(PyExc_ValueError, "Expecting an integer");
        return NULL;
    }
    $2 = PyInt_AsLong($input);
    if ($2 < 0) {
        PyErr_SetString(PyExc_ValueError, "Positive integer expected");
        return NULL;
    }
    $1 = (void *)malloc($2);
}

%typemap(out) ssize_t {
    $result = PyLong_FromSsize_t($1);
}

%typemap(argout) (void *buf, size_t count) {
    Py_XDECREF($result);
    if (result < 0) {
        free($1);
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    $result = PyString_FromStringAndSize($1, result);
    free($1);
}

/* For icbinn_pwrite: */

%typemap(in) (const void *buf, size_t count) {
    if (!PyString_Check($input)) {
        PyErr_SetString(PyExc_ValueError, "Expecting a string");
        return NULL;
    }
    $1 = (void *)PyString_AsString($input);
    $2 = PyString_Size($input);
}

/* For icbinn_readent: */

%typemap(in, numinputs=0) struct icbinn_dirent *buf (struct icbinn_dirent temp) {
    $1 = &temp;
}

%typemap(argout) struct icbinn_dirent *buf {
    PyObject *tuple;
    Py_XDECREF($result);
    if (result < 0) {
        return Py_None;
    }
    tuple = PyTuple_New(2);
    PyTuple_SetItem(tuple, 0, PyString_FromString($1->name));
    PyTuple_SetItem(tuple, 1, PyInt_FromLong($1->type));
    $result = tuple;
}

/* For icbinn_stat: */

%typemap(in, numinputs=0) struct icbinn_stat *buf (struct icbinn_stat temp) {
    $1 = &temp;
}

%typemap(argout) struct icbinn_stat *buf {
    PyObject *tuple;
    Py_XDECREF($result);
    if (result < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    tuple = PyTuple_New(2);
    PyTuple_SetItem(tuple, 0, PyLong_FromLongLong($1->size));
    PyTuple_SetItem(tuple, 1, PyInt_FromLong($1->type));
    $result = tuple;
}

/* TODO: can swig #include a header file to avoid repeating these
   declarations? */

ICBINN *icbinn_clnt_create_tcp(const char *host, int port);
ICBINN *icbinn_clnt_create_v4v(int domid, int port);
void icbinn_clnt_destroy(ICBINN *icb);
int icbinn_close(ICBINN *icb, int fd);
int icbinn_lock(ICBINN *icb, int fd, int type);
int icbinn_mkdir(ICBINN *icb, const char *path);
int icbinn_null(ICBINN *icb);
int icbinn_open(ICBINN *icb, char *name, int mode);
ssize_t icbinn_pread(ICBINN *icb, int fd, void *buf, size_t count, uint64_t offset);
int icbinn_pwrite(ICBINN *icb, int fd, const void *buf, size_t count, uint64_t offset);
ssize_t icbinn_readdir(ICBINN *icb, const char *dir, size_t offset, size_t count, struct icbinn_dirent *buf);
int icbinn_readent(ICBINN *icb, const char *dir, size_t offset, struct icbinn_dirent *buf);
int icbinn_rename(ICBINN *icb, const char *from, const char *to);
int icbinn_rmdir(ICBINN *icb, const char *path);
int icbinn_stat(ICBINN *icb, const char *path, struct icbinn_stat *buf);
int icbinn_unlink(ICBINN *icb, const char *path);
int icbinn_ftruncate(ICBINN *icb, int fd, uint64_t length);
ssize_t icbinn_rand(ICBINN *icb, int src, void *buf, size_t count);
