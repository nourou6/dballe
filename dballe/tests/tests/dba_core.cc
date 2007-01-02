/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <extra/test-utils.h>
#include <dballe/core/fast.h>

namespace tut {
using namespace tut_dballe;

struct dba_core_shar
{
	dba_core_shar()
	{
	}

	~dba_core_shar()
	{
	}
};
TESTGRP(dba_core);


// Test variable creation
template<> template<>
void to::test<1>()
{
	gen_ensure_equals(string(itoa(0, 3)), string("0"));
	gen_ensure_equals(string(itoa(1, 3)), string("1"));
	gen_ensure_equals(string(itoa(100, 3)), string("100"));
	gen_ensure_equals(string(itoa(1000, 3)), string("000"));
	gen_ensure_equals(string(itoa(1234567890, 10)), string("1234567890"));
	gen_ensure_equals(string(itoa(45, 2)), string("45"));
	gen_ensure_equals(string(itoa(-1, 2)), string("-1"));
	gen_ensure_equals(string(itoa(-10800, 10)), string("-10800"));
	gen_ensure_equals(string(itoa(-11000000, 7)), string("1000000"));
	gen_ensure_equals(string(itoa(-11000000, 8)), string("-11000000"));
	gen_ensure_equals(string(itoa(-2147483647, 11)), string("-2147483647"));
	gen_ensure_equals(string(itoa(-2147483648l, 11)), string("-2147483648"));
}
	
}

/* vim:set ts=4 sw=4: */
