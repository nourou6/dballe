#include <Python.h>
#include "db.h"
#include "record.h"
#include "cursor.h"
#include "common.h"
#include "types.h"
#include "dballe/types.h"
#include "dballe/file.h"
#include "dballe/core/query.h"
#include "dballe/core/values.h"
#include "dballe/message.h"
#include "dballe/importer.h"
#include "dballe/exporter.h"
#include "dballe/msg/msg.h"
#include "dballe/db/defs.h"
#include <algorithm>
#include <wreport/bulletin.h>
#include "impl-utils.h"

#if PY_MAJOR_VERSION <= 2
    #define PyLong_FromLong PyInt_FromLong
#endif

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {
PyTypeObject* dpy_DB_Type = nullptr;
PyTypeObject* dpy_Transaction_Type = nullptr;
}

namespace {

/**
 * call o.fileno() and return its result.
 *
 * In case of AttributeError and IOError (parent of UnsupportedOperation, not
 * available from C), it clear the error indicator.
 *
 * Returns -1 if fileno() was not available or some other exception happened.
 * Use PyErr_Occurred to tell between the two.
 */
int file_get_fileno(PyObject* o)
{
    // fileno_value = obj.fileno()
    pyo_unique_ptr fileno_meth(PyObject_GetAttrString(o, "fileno"));
    if (!fileno_meth) return -1;
    pyo_unique_ptr fileno_args(Py_BuildValue("()"));
    if (!fileno_args) return -1;
    PyObject* fileno_value = PyObject_Call(fileno_meth, fileno_args, NULL);
    if (!fileno_value)
    {
        if (PyErr_ExceptionMatches(PyExc_AttributeError) || PyErr_ExceptionMatches(PyExc_IOError))
            PyErr_Clear();
        return -1;
    }

    // fileno = int(fileno_value)
    if (!PyObject_TypeCheck(fileno_value, &PyLong_Type)) {
        PyErr_SetString(PyExc_ValueError, "fileno() function must return an integer");
        return -1;
    }

    return PyLong_AsLong(fileno_value);
}

/**
 * call o.data() and return its result, both as a PyObject and as a buffer.
 *
 * The data returned in buf and len will be valid as long as the returned
 * object stays valid.
 */
PyObject* file_get_data(PyObject* o, char*&buf, Py_ssize_t& len)
{
    // Use read() instead
    pyo_unique_ptr read_meth(PyObject_GetAttrString(o, "read"));
    pyo_unique_ptr read_args(Py_BuildValue("()"));
    pyo_unique_ptr data(PyObject_Call(read_meth, read_args, NULL));
    if (!data) return nullptr;

#if PY_MAJOR_VERSION >= 3
    if (!PyObject_TypeCheck(data, &PyBytes_Type)) {
        PyErr_SetString(PyExc_ValueError, "read() function must return a bytes object");
        return nullptr;
    }
    if (PyBytes_AsStringAndSize(data, &buf, &len))
        return nullptr;
#else
    if (!PyObject_TypeCheck(data, &PyString_Type)) {
        Py_DECREF(data);
        PyErr_SetString(PyExc_ValueError, "read() function must return a string object");
        return nullptr;
    }
    if (PyString_AsStringAndSize(data, &buf, &len))
        return nullptr;
#endif

    return data.release();
}

template<typename Vals>
static PyObject* get_insert_ids(const Vals& vals)
{
    pyo_unique_ptr res(throw_ifnull(PyDict_New()));
    pyo_unique_ptr ana_id(throw_ifnull(PyLong_FromLong(vals.info.id)));
    if (PyDict_SetItemString(res, "ana_id", ana_id))
        throw PythonException();

    for (const auto& v: vals.values)
    {
        pyo_unique_ptr id(throw_ifnull(PyLong_FromLong(v.second.data_id)));
        pyo_unique_ptr varcode(to_python(v.first));

        if (PyDict_SetItem(res, varcode, id))
            throw PythonException();
    }

    return res.release();
}

template<typename Impl>
struct insert_station_data : MethKwargs<Impl>
{
    constexpr static const char* name = "insert_station_data";
    constexpr static const char* doc = "Insert station values in the database";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "record", "can_replace", "can_add_stations", NULL };
        PyObject* record;
        int can_replace = 0;
        int station_can_add = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O|ii", const_cast<char**>(kwlist), &record, &can_replace, &station_can_add))
            return nullptr;

        try {
            RecordAccess rec(record);
            ReleaseGIL gil;
            StationValues vals(rec);
            self->db->insert_station_data(vals, can_replace, station_can_add);
            gil.lock();
            return get_insert_ids(vals);
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct insert_data : MethKwargs<Impl>
{
    constexpr static const char* name = "insert_data";
    constexpr static const char* doc = "Insert data values in the database";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "record", "can_replace", "can_add_stations", NULL };
        PyObject* record;
        int can_replace = 0;
        int station_can_add = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O|ii", const_cast<char**>(kwlist), &record, &can_replace, &station_can_add))
            return nullptr;

        try {
            RecordAccess rec(record);
            ReleaseGIL gil;
            DataValues vals(rec);
            self->db->insert_data(vals, can_replace, station_can_add);
            gil.lock();
            return get_insert_ids(vals);
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct remove_station_data : MethKwargs<Impl>
{
    constexpr static const char* name = "remove_station_data";
    constexpr static const char* doc = "Remove station variables from the database";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "query", NULL };
        PyObject* pyquery;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O", const_cast<char**>(kwlist), &pyquery))
            return nullptr;

        try {
            core::Query query;
            read_query(pyquery, query);
            ReleaseGIL gil;
            self->db->remove_station_data(query);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Base, typename Impl>
struct MethQuery : public MethKwargs<Impl>
{
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "query", NULL };
        PyObject* pyquery;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O", const_cast<char**>(kwlist), &pyquery))
            return nullptr;

        try {
            core::Query query;
            read_query(pyquery, query);
            return Base::run_query(self, query);
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct remove : MethQuery<remove<Impl>, Impl>
{
    constexpr static const char* name = "remove";
    constexpr static const char* doc = "Remove data variables from the database";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        ReleaseGIL gil;
        self->db->remove(query);
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct remove_all : MethNoargs<Impl>
{
    constexpr static const char* name = "remove_all";
    constexpr static const char* doc = "Remove all data from the database";
    static PyObject* run(Impl* self)
    {
        try {
            ReleaseGIL gil;
            self->db->remove_all();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct query_stations : MethQuery<query_stations<Impl>, Impl>
{
    constexpr static const char* name = "query_stations";
    constexpr static const char* doc = "Query the station archive in the database; returns a Cursor";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        ReleaseGIL gil;
        std::unique_ptr<db::Cursor> res = self->db->query_stations(query);
        gil.lock();
        return (PyObject*)cursor_create(move(res));
    }
};

template<typename Impl>
struct query_station_data : MethQuery<query_station_data<Impl>, Impl>
{
    constexpr static const char* name = "query_station_data";
    constexpr static const char* doc = "Query the station variables in the database; returns a Cursor";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        ReleaseGIL gil;
        std::unique_ptr<db::Cursor> res = self->db->query_station_data(query);
        gil.lock();
        return (PyObject*)cursor_create(move(res));
    }
};

template<typename Impl>
struct query_data : MethQuery<query_data<Impl>, Impl>
{
    constexpr static const char* name = "query_data";
    constexpr static const char* doc = "Query the variables in the database; returns a Cursor";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        ReleaseGIL gil;
        std::unique_ptr<db::Cursor> res = self->db->query_data(query);
        gil.lock();
        return (PyObject*)cursor_create(move(res));
    }
};

template<typename Impl>
struct query_summary : MethQuery<query_summary<Impl>, Impl>
{
    constexpr static const char* name = "query_summary";
    constexpr static const char* doc = "Query the summary of the results of a query; returns a Cursor";
    static PyObject* run_query(Impl* self, dballe::Query& query)
    {
        ReleaseGIL gil;
        std::unique_ptr<db::Cursor> res = self->db->query_summary(query);
        gil.lock();
        return (PyObject*)cursor_create(move(res));
    }
};

template<typename Impl>
struct query_attrs : MethKwargs<Impl>
{
    constexpr static const char* name = "query_attrs";
    constexpr static const char* doc = "Query attributes (deprecated)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use DB.attr_query_station or DB.attr_query_data instead of DB.query_attrs", 1))
            return nullptr;

        static const char* kwlist[] = { "varcode", "reference_id", "attrs", NULL };
        int reference_id;
        const char* varname;
        PyObject* attrs = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "si|O", const_cast<char**>(kwlist), &varname, &reference_id, &attrs))
            return nullptr;

        try {
            // Read the attribute list, if provided
            db::AttrList codes = db_read_attrlist(attrs);
            py_unique_ptr<dpy_Record> rec(record_create());

            ReleaseGIL gil;
            self->db->attr_query_data(reference_id, [&](unique_ptr<Var>&& var) {
                if (!codes.empty() && find(codes.begin(), codes.end(), var->code()) == codes.end())
                    return;
                rec->rec->set(move(var));
            });
            gil.lock();
            return (PyObject*)rec.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};


template<typename Impl>
struct attr_query_station : MethKwargs<Impl>
{
    constexpr static const char* name = "attr_query_station";
    constexpr static const char* doc = "query station data attributes";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "varid", NULL };
        int varid;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "i", const_cast<char**>(kwlist), &varid))
            return nullptr;

        try {
            py_unique_ptr<dpy_Record> rec(record_create());
            ReleaseGIL gil;
            self->db->attr_query_station(varid, [&](unique_ptr<Var> var) {
                rec->rec->set(move(var));
            });
            gil.lock();
            return (PyObject*)rec.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct attr_query_data : MethKwargs<Impl>
{
    constexpr static const char* name = "attr_query_data";
    constexpr static const char* doc = "query data attributes";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "varid", NULL };
        int varid;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "i", const_cast<char**>(kwlist), &varid))
            return nullptr;

        try {
            py_unique_ptr<dpy_Record> rec(record_create());
            ReleaseGIL gil;
            self->db->attr_query_data(varid, [&](unique_ptr<Var>&& var) {
                rec->rec->set(move(var));
            });
            gil.lock();
            return (PyObject*)rec.release();
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct attr_insert : MethKwargs<Impl>
{
    constexpr static const char* name = "attr_insert";
    constexpr static const char* doc = "Insert new attributes into the database (deprecated)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use DB.attr_insert_station or DB.attr_insert_data instead of DB.attr_insert", 1))
            return nullptr;

        static const char* kwlist[] = { "varcode", "attrs", "varid", NULL };
        int varid = -1;
        const char* varname;
        PyObject* record;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "sO|i", const_cast<char**>(kwlist),
                    &varname,
                    &record,
                    &varid))
            return nullptr;

        if (varid == -1)
        {
            PyErr_SetString(PyExc_ValueError, "please provide a reference_id argument: implicitly reusing the one from the last insert is not supported anymore");
            return nullptr;
        }

        try {
            RecordAccess rec(record);
            ReleaseGIL gil;
            self->db->attr_insert_data(varid, Values(rec));
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct attr_insert_station : MethKwargs<Impl>
{
    constexpr static const char* name = "attr_insert_station";
    constexpr static const char* doc = "Insert new attributes into the database";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "varid", "attrs", NULL };
        int varid;
        PyObject* attrs;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "iO", const_cast<char**>(kwlist), &varid, &attrs))
            return nullptr;

        try {
            RecordAccess rec(attrs);
            ReleaseGIL gil;
            self->db->attr_insert_station(varid, Values(rec));
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct attr_insert_data : MethKwargs<Impl>
{
    constexpr static const char* name = "attr_insert_data";
    constexpr static const char* doc = "Insert new attributes into the database";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "varid", "attrs", NULL };
        int varid;
        PyObject* attrs;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "iO", const_cast<char**>(kwlist), &varid, &attrs))
            return nullptr;

        try {
            RecordAccess rec(attrs);
            ReleaseGIL gil;
            self->db->attr_insert_data(varid, Values(rec));
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct attr_remove : MethKwargs<Impl>
{
    constexpr static const char* name = "attr_remove";
    constexpr static const char* doc = "Remove attributes (deprecated)";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        if (PyErr_WarnEx(PyExc_DeprecationWarning, "please use DB.attr_remove_station or DB.attr_remove_data instead of DB.attr_remove", 1))
            return nullptr;

        static const char* kwlist[] = { "varcode", "varid", "attrs", NULL };
        int varid;
        const char* varname;
        PyObject* attrs = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "si|O", const_cast<char**>(kwlist), &varname, &varid, &attrs))
            return nullptr;

        try {
            // Read the attribute list, if provided
            db::AttrList codes = db_read_attrlist(attrs);
            ReleaseGIL gil;
            self->db->attr_remove_data(varid, codes);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct attr_remove_station : MethKwargs<Impl>
{
    constexpr static const char* name = "attr_remove_station";
    constexpr static const char* doc = "Remove attributes from station variables";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "varid", "attrs", NULL };
        int varid;
        PyObject* attrs;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "i|O", const_cast<char**>(kwlist), &varid, &attrs))
            return nullptr;

        try {
            db::AttrList codes = db_read_attrlist(attrs);
            ReleaseGIL gil;
            self->db->attr_remove_station(varid, codes);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename Impl>
struct attr_remove_data : MethKwargs<Impl>
{
    constexpr static const char* name = "attr_remove_data";
    constexpr static const char* doc = "Remove attributes from data variables";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "varid", "attrs", NULL };
        int varid;
        PyObject* attrs;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "i|O", const_cast<char**>(kwlist), &varid, &attrs))
            return nullptr;

        try {
            db::AttrList codes = db_read_attrlist(attrs);
            ReleaseGIL gil;
            self->db->attr_remove_data(varid, codes);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

template<typename DB>
static unsigned db_load_file_enc(DB& db, Encoding encoding, FILE* file, bool close_on_exit, const std::string& name, int flags)
{
    std::unique_ptr<File> f = File::create(encoding, file, close_on_exit, name);
    std::unique_ptr<Importer> imp = Importer::create(f->encoding());
    unsigned count = 0;
    f->foreach([&](const BinaryMessage& raw) {
        Messages msgs = imp->from_binary(raw);
        db.import_msgs(msgs, NULL, flags);
        ++count;
        return true;
    });
    return count;
}

template<typename DB>
static unsigned db_load_file(DB& db, FILE* file, bool close_on_exit, const std::string& name, int flags)
{
    std::unique_ptr<File> f = File::create(file, close_on_exit, name);
    std::unique_ptr<Importer> imp = Importer::create(f->encoding());
    unsigned count = 0;
    f->foreach([&](const BinaryMessage& raw) {
        Messages msgs = imp->from_binary(raw);
        db.import_msgs(msgs, NULL, flags);
        ++count;
        return true;
    });
    return count;
}

template<typename Impl>
struct load : MethKwargs<Impl>
{
    constexpr static const char* name = "load";
    constexpr static const char* doc = R"(
        load(fp, encoding=None, attrs=False, full_pseudoana=False, overwrite=False)

        Load a file object in the database. An encoding can optionally be
        provided as a string ("BUFR", "CREX"). If encoding is None then
        load will try to autodetect based on the first byte of the file.
    )";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = {"fp", "encoding", "attrs", "full_pseudoana", "overwrite", NULL};
        PyObject* obj;
        const char* encoding = nullptr;
        int attrs = 0;
        int full_pseudoana = 0;
        int overwrite = 0;
        int flags = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O|siii", const_cast<char**>(kwlist), &obj, &encoding, &attrs, &full_pseudoana, &overwrite))
            return nullptr;

        try {
            string repr = object_repr(obj);

            flags = (attrs ? DBA_IMPORT_ATTRS : 0) | (full_pseudoana ? DBA_IMPORT_FULL_PSEUDOANA : 0) | (overwrite ? DBA_IMPORT_OVERWRITE : 0);

            int fileno = file_get_fileno(obj);
            if (fileno == -1)
            {
                if (PyErr_Occurred()) return nullptr;

                char* buf;
                Py_ssize_t len;
                pyo_unique_ptr data = file_get_data(obj, buf, len);
                if (!data) return nullptr;

                FILE* f = fmemopen(buf, len, "r");
                if (!f) return nullptr;
                unsigned count;
                if (encoding)
                {
                    count = db_load_file_enc(*self->db, File::parse_encoding(encoding), f, true, repr, flags);
                } else
                    count = db_load_file(*self->db, f, true, repr, flags);
                return PyLong_FromLong(count);
            } else {
                // Duplicate the file descriptor because both python and libc will want to
                // close it
                fileno = dup(fileno);
                if (fileno == -1)
                {
                    PyErr_Format(PyExc_OSError, "cannot dup() the file handle from %s", repr.c_str());
                    return nullptr;
                }

                FILE* f = fdopen(fileno, "rb");
                if (f == nullptr)
                {
                    close(fileno);
                    PyErr_Format(PyExc_OSError, "cannot fdopen() the dup()ed file handle from %s", repr.c_str());
                    return nullptr;
                }

                unsigned count;
                if (encoding)
                {
                    count = db_load_file_enc(*self->db, File::parse_encoding(encoding), f, true, repr, flags);
                } else
                    count = db_load_file(*self->db, f, true, repr, flags);
                return PyLong_FromLong(count);
            }
        } DBALLE_CATCH_RETURN_PYO
    }
};

template<typename Impl>
struct export_to_file : MethKwargs<Impl>
{
    constexpr static const char* name = "export_to_file";
    constexpr static const char* doc = "Export data matching a query as bulletins to a named file";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "query", "format", "filename", "generic", NULL };
        dpy_Record* query;
        const char* format;
        PyObject* file;
        int as_generic = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "O!sO|i", const_cast<char**>(kwlist), dpy_Record_Type, &query, &format, &file, &as_generic))
            return NULL;

        try {
            Encoding encoding = Encoding::BUFR;
            if (strcmp(format, "BUFR") == 0)
                encoding = Encoding::BUFR;
            else if (strcmp(format, "CREX") == 0)
                encoding = Encoding::CREX;
            else
            {
                PyErr_SetString(PyExc_ValueError, "encoding must be one of BUFR or CREX");
                return NULL;
            }

            if (pyobject_is_string(file))
            {
                std::string filename = string_from_python(file);
                std::unique_ptr<File> out = File::create(encoding, filename, "wb");
                ExporterOptions opts;
                if (as_generic)
                    opts.template_name = "generic";
                auto exporter = Exporter::create(out->encoding(), opts);
                auto q = Query::create();
                q->set_from_record(*query->rec);
                ReleaseGIL gil;
                self->db->export_msgs(*q, [&](unique_ptr<Message>&& msg) {
                    Messages msgs;
                    msgs.emplace_back(move(msg));
                    out->write(exporter->to_binary(msgs));
                    return true;
                });
                gil.lock();
                Py_RETURN_NONE;
            } else {
                ExporterOptions opts;
                if (as_generic)
                    opts.template_name = "generic";
                auto exporter = Exporter::create(encoding, opts);
                auto q = Query::create();
                q->set_from_record(*query->rec);
                pyo_unique_ptr res(nullptr);
                bool has_error = false;
                self->db->export_msgs(*q, [&](unique_ptr<Message>&& msg) {
                    Messages msgs;
                    msgs.emplace_back(move(msg));
                    std::string encoded = exporter->to_binary(msgs);
#if PY_MAJOR_VERSION >= 3
                    res = pyo_unique_ptr(PyObject_CallMethod(file, (char*)"write", (char*)"y#", encoded.data(), (int)encoded.size()));
#else
                    res = pyo_unique_ptr(PyObject_CallMethod(file, (char*)"write", (char*)"s#", encoded.data(), (int)encoded.size()));
#endif
                    if (!res)
                    {
                        has_error = true;
                        return false;
                    }
                    return true;
                });
                if (has_error)
                    return nullptr;
                Py_RETURN_NONE;
            }
        } DBALLE_CATCH_RETURN_PYO
    }
};

namespace pydb {

struct get_default_format : ClassMethNoargs
{
    constexpr static const char* name = "get_default_format";
    constexpr static const char* doc = "get the default DB format";
    static PyObject* run(PyTypeObject* cls)
    {
        try {
            string format = db::format_format(DB::get_default_format());
            return PyUnicode_FromString(format.c_str());
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct set_default_format : ClassMethKwargs
{
    constexpr static const char* name = "set_default_format";
    constexpr static const char* doc = "set the default DB format";
    static PyObject* run(PyTypeObject* cls, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "format", nullptr };
        const char* format;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &format))
            return nullptr;

        try {
            DB::set_default_format(db::format_parse(format));
            Py_RETURN_NONE;
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct connect_from_file : ClassMethKwargs
{
    constexpr static const char* name = "connect_from_file";
    constexpr static const char* doc = "create a DB to access a SQLite file";
    static PyObject* run(PyTypeObject* cls, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "name", nullptr };
        const char* name;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &name))
            return nullptr;

        try {
            ReleaseGIL gil;
            std::shared_ptr<DB> db = DB::connect_from_file(name);
            gil.lock();
            return (PyObject*)db_create(db);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct connect_from_url : ClassMethKwargs
{
    constexpr static const char* name = "connect_from_url";
    constexpr static const char* doc = "create a DB to access a database identified by a DB-All.e URL";
    static PyObject* run(PyTypeObject* cls, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "url", nullptr };
        const char* url;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &url))
            return nullptr;

        try {
            ReleaseGIL gil;
            shared_ptr<DB> db = DB::connect_from_url(url);
            gil.lock();
            return (PyObject*)db_create(db);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct connect_test : ClassMethNoargs
{
    constexpr static const char* name = "connect_test";
    constexpr static const char* doc = "Create a DB for running the test suite, as configured in the test environment";
    static PyObject* run(PyTypeObject* cls)
    {
        try {
            ReleaseGIL gil;
            std::shared_ptr<DB> db = DB::connect_test();
            gil.lock();
            return (PyObject*)db_create(db);
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct is_url : ClassMethKwargs
{
    constexpr static const char* name = "is_url";
    constexpr static const char* doc = "Checks if a string looks like a DB-All.e DB url";
    static PyObject* run(PyTypeObject* cls, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "url", nullptr };
        const char* url;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "s", const_cast<char**>(kwlist), &url))
            return nullptr;

        try {
            if (DB::is_url(url))
                Py_RETURN_TRUE;
            else
                Py_RETURN_FALSE;
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct transaction : MethKwargs<dpy_DB>
{
    constexpr static const char* name = "transaction";
    constexpr static const char* doc = "Create a new database transaction";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "readonly", nullptr };
        int readonly = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "|p", const_cast<char**>(kwlist), &readonly))
            return nullptr;

        try {
            auto res = self->db->transaction(readonly);
            return (PyObject*)transaction_create(move(res));
        } DBALLE_CATCH_RETURN_PYO
    }
};

struct disappear : MethNoargs<dpy_DB>
{
    constexpr static const char* name = "disappear";
    constexpr static const char* doc = "Remove all DB-All.e tables and data from the database, if possible";
    static PyObject* run(Impl* self)
    {
        try {
            ReleaseGIL gil;
            self->db->disappear();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

struct reset : MethKwargs<dpy_DB>
{
    constexpr static const char* name = "reset";
    constexpr static const char* doc = "Reset the database, removing all existing Db-All.e tables and re-creating them empty.";
    static PyObject* run(Impl* self, PyObject* args, PyObject* kw)
    {
        static const char* kwlist[] = { "repinfo_file", nullptr };
        const char* repinfo_file = 0;
        if (!PyArg_ParseTupleAndKeywords(args, kw, "|s", const_cast<char**>(kwlist), &repinfo_file))
            return nullptr;

        try {
            ReleaseGIL gil;
            self->db->reset(repinfo_file);
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

struct vacuum : MethNoargs<dpy_DB>
{
    constexpr static const char* name = "vacuum";
    constexpr static const char* doc = "Perform database cleanup operations";
    static PyObject* run(Impl* self)
    {
        try {
            ReleaseGIL gil;
            self->db->vacuum();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};


struct Definition : public Binding<Definition, dpy_DB>
{
    constexpr static const char* name = "DB";
    constexpr static const char* qual_name = "dballe.DB";
    constexpr static const char* doc = "DB-All.e database access";

    GetSetters<> getsetters;
    Methods<
        get_default_format, set_default_format,
        connect_from_file, connect_from_url, connect_test, is_url,
        disappear, reset, vacuum,
        transaction,
        insert_station_data<Impl>, insert_data<Impl>,
        remove_station_data<Impl>, remove<Impl>, remove_all<Impl>,
        query_stations<Impl>, query_station_data<Impl>, query_data<Impl>, query_summary<Impl>, query_attrs<Impl>,
        attr_query_station<Impl>, attr_query_data<Impl>,
        attr_insert<Impl>, attr_insert_station<Impl>, attr_insert_data<Impl>,
        attr_remove<Impl>, attr_remove_station<Impl>, attr_remove_data<Impl>,
        load<Impl>, export_to_file<Impl>
        > methods;

    static void _dealloc(Impl* self)
    {
        self->db.~shared_ptr<DB>();
        Py_TYPE(self)->tp_free(self);
    }
};

Definition* definition = nullptr;

}


namespace pytr {

typedef MethGenericEnter<dpy_Transaction> __enter__;

struct __exit__ : MethVarargs<dpy_Transaction>
{
    constexpr static const char* name = "__exit__";
    constexpr static const char* doc = "Context manager __exit__";
    static PyObject* run(Impl* self, PyObject* args)
    {
        PyObject* exc_type;
        PyObject* exc_val;
        PyObject* exc_tb;
        if (!PyArg_ParseTuple(args, "OOO", &exc_type, &exc_val, &exc_tb))
            return nullptr;

        try {
            ReleaseGIL gil;
            if (exc_type == Py_None)
                self->db->commit();
            else
                self->db->rollback();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

struct commit : MethNoargs<dpy_Transaction>
{
    constexpr static const char* name = "commit";
    constexpr static const char* doc = "commit the transaction";
    static PyObject* run(Impl* self)
    {
        try {
            ReleaseGIL gil;
            self->db->commit();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};

struct rollback : MethNoargs<dpy_Transaction>
{
    constexpr static const char* name = "rollback";
    constexpr static const char* doc = "roll back the transaction";
    static PyObject* run(Impl* self)
    {
        try {
            ReleaseGIL gil;
            self->db->rollback();
        } DBALLE_CATCH_RETURN_PYO
        Py_RETURN_NONE;
    }
};


struct Definition : public Binding<Definition, dpy_Transaction>
{
    constexpr static const char* name = "Transaction";
    constexpr static const char* qual_name = "dballe.Transaction";
    constexpr static const char* doc = "DB-All.e transaction";

    GetSetters<> getsetters;
    Methods<
        insert_station_data<Impl>, insert_data<Impl>,
        remove_station_data<Impl>, remove<Impl>, remove_all<Impl>,
        query_stations<Impl>, query_station_data<Impl>, query_data<Impl>, query_summary<Impl>,
        attr_query_station<Impl>, attr_query_data<Impl>,
        attr_insert_station<Impl>, attr_insert_data<Impl>,
        attr_remove_station<Impl>, attr_remove_data<Impl>,
        load<Impl>, export_to_file<Impl>,
        __enter__, __exit__, commit, rollback
        > methods;

    static void _dealloc(Impl* self)
    {
        self->db.~shared_ptr<dballe::db::Transaction>();
        Py_TYPE(self)->tp_free(self);
    }
};

Definition* definition = nullptr;

}

}

namespace dballe {
namespace python {

db::AttrList db_read_attrlist(PyObject* attrs)
{
    db::AttrList res;
    if (!attrs) return res;
    pyo_unique_ptr iter(throw_ifnull(PyObject_GetIter(attrs)));

    while (PyObject* iter_item = PyIter_Next(iter)) {
        pyo_unique_ptr item(iter_item);
        string name = string_from_python(item);
        res.push_back(resolve_varcode(name));
    }
    return res;
}

dpy_DB* db_create(std::shared_ptr<DB> db)
{
    py_unique_ptr<dpy_DB> res = throw_ifnull(PyObject_New(dpy_DB, dpy_DB_Type));
    new (&(res->db)) std::shared_ptr<DB>(db);
    return res.release();
}

dpy_Transaction* transaction_create(std::shared_ptr<db::Transaction> transaction)
{
    py_unique_ptr<dpy_Transaction> res = throw_ifnull(PyObject_New(dpy_Transaction, dpy_Transaction_Type));
    new (&(res->db)) std::shared_ptr<db::Transaction>(transaction);
    return res.release();
}

void register_db(PyObject* m)
{
    common_init();

    pydb::definition = new pydb::Definition;
    dpy_DB_Type = pydb::definition->activate(m);

    pytr::definition = new pytr::Definition;
    dpy_Transaction_Type = pytr::definition->activate(m);
}

}
}
