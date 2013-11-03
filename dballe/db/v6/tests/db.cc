/*
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "db/test-utils-db.h"
#include "db/querybuf.h"
#include "db/v6/db.h"
#include "db/v6/cursor.h"
#include "db/internals.h"

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace std;

namespace tut {

struct dbv6_shar : public dballe::tests::DB_test_base
{
    dbv6_shar() : dballe::tests::DB_test_base(db::V6)
    {
    }

    ~dbv6_shar()
    {
    }
};
TESTGRP(dbv6);

// Ensure that reset will work on an empty database
template<> template<>
void to::test<1>()
{
    use_db();
    v6::DB& db = v6();

    db.delete_tables();
    db.reset();
    // Run twice to see if it is idempotent
    db.reset();
}

}

/* vim:set ts=4 sw=4: */
