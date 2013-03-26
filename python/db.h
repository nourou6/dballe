/*
 * python/db - DB-All.e DB python bindings
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */
#ifndef DBALLE_PYTHON_DB_H
#define DBALLE_PYTHON_DB_H

#include <Python.h>
#include <dballe/db/db.h>
#include "record.h"

extern "C" {

typedef struct {
    PyObject_HEAD
    dballe::DB* db;
    dpy_Record* attr_rec;
} dpy_DB;

PyAPI_DATA(PyTypeObject) dpy_DB_Type;

#define dpy_DB_Check(ob) \
    (Py_TYPE(ob) == &dpy_DB_Type || \
     PyType_IsSubtype(Py_TYPE(ob), &dpy_DB_Type))
}

namespace dballe {
namespace python {

/**
 * Create a new dpy_DB, taking over memory management
 */
dpy_DB* db_create(std::auto_ptr<DB> db);

void register_db(PyObject* m);

}
}

#endif