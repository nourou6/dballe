/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2009  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <test-utils-db.h>
#include <dballe/db/import.h>
#include <dballe/db/export.h>

namespace tut {
using namespace tut_dballe;

struct db_import_shar : public db_test
{
	TestMsgEnv testenv;

	db_import_shar()
	{
	}

	~db_import_shar()
	{
		test_untag();
	}
};
TESTGRP(db_import);

static const char* rep_memo_from_msg(dba_msg msg)
{
	return dba_msg_repmemo_from_type(msg->type);
}


static dba_err msg_collector(dba_msgs msgs, void* data)
{
//	cerr << "MSG COLLECTOR";
	vector<dba_msg>* vec = static_cast<vector<dba_msg>*>(data);
	for (int i = 0; i < msgs->len; ++i)
	{
//		cerr << " got " << i << "/" << msgs->len << ":" << (int)msgs->msgs[i];
		(*vec).push_back(msgs->msgs[i]);
		// Detach the message from the msgs
		msgs->msgs[i] = NULL;
	}
//	cerr << endl;
	dba_msgs_delete(msgs);
	return dba_error_ok();
}

static dba_err msgs_collector(dba_msgs msgs, void* data)
{
//	cerr << "MSG COLLECTOR";
	msg_vector* vec = static_cast<msg_vector*>(data);
	vec->push_back(msgs);
	return dba_error_ok();
}

template<> template<>
void to::test<1>()
{
	use_db();

	const char* files[] = {
		"crex/test-mare0.crex",
		"crex/test-mare1.crex",
		"crex/test-synop0.crex",
		"crex/test-synop2.crex",
		"crex/test-temp0.crex",
		NULL
	};

	/* Create the records we use to work */
	dba_record query;
	CHECKED(dba_record_create(&query));

	for (int i = 0; files[i] != NULL; i++)
	{
		test_tag(files[i]);
		dba_msgs inmsgs = read_test_msg(files[i], CREX);
		dba_msg msg = inmsgs->msgs[0];

		CHECKED(dba_db_reset(db, NULL));
		CHECKED(dba_import_msg(db, msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_DATETIME_ATTRS));
		// Explicitly set rep_memo so that the messages later match
		CHECKED(dba_msg_set_rep_memo(msg, rep_memo_from_msg(msg), -1));

		vector<dba_msg> msgs;
		CHECKED(dba_record_key_setc(query, DBA_KEY_REP_MEMO, rep_memo_from_msg(msg)));
		CHECKED(dba_db_export(db, query, msg_collector, &msgs));
		gen_ensure_equals(msgs.size(), 1u);
		gen_ensure(msgs[0] != NULL);

		/*
		if (string(files[i]).find("temp0") != string::npos)
		{
			dba_msg_print(msg, stderr);
			dba_msg_print(msgs[0], stderr);
		}
		*/

		int diffs = 0;
		dba_msg_diff(msg, msgs[0], &diffs, stderr);
		if (diffs != 0) track_different_msgs(msg, msgs[0], "crex-old");
		gen_ensure_equals(diffs, 0);

		dba_msgs_delete(inmsgs);
		for (vector<dba_msg>::iterator i = msgs.begin(); i != msgs.end(); i++)
			dba_msg_delete(*i);
	}

	dba_record_delete(query);
	test_untag();
}

template<> template<>
void to::test<2>()
{
	use_db();

	const char* files[] = {
		"bufr/obs0-1.22.bufr", 
		"bufr/obs0-1.11188.bufr", 
		"bufr/obs0-3.504.bufr", 
		"bufr/obs1-9.2.bufr", 
		"bufr/obs1-11.16.bufr", 
		"bufr/obs1-13.36.bufr", 
		"bufr/obs1-19.3.bufr", 
		"bufr/obs1-21.1.bufr", 
		"bufr/obs1-140.454.bufr", 
		"bufr/obs2-101.16.bufr", 
		"bufr/obs2-102.1.bufr", 
		"bufr/obs2-91.2.bufr", 
		"bufr/obs3-3.1.bufr",
//		"bufr/obs3-56.2.bufr",
		"bufr/obs4-142.13803.bufr", 
		"bufr/obs4-142.1.bufr", 
		"bufr/obs4-144.4.bufr", 
		"bufr/obs4-145.4.bufr", 
		"bufr/obs255-255.0.bufr", 
		"bufr/test-airep1.bufr",
		"bufr/test-temp1.bufr", 
		"bufr/ed4.bufr",
		NULL
	};

	dba_record query;
	CHECKED(dba_record_create(&query));

	for (int i = 0; files[i] != NULL; i++)
	{
		test_tag(files[i]);
		dba_msgs inmsgs = read_test_msg(files[i], BUFR);
		dba_msg msg = inmsgs->msgs[0];

		CHECKED(dba_db_reset(db, NULL));
		CHECKED(dba_import_msg(db, msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_DATETIME_ATTRS));
		// Explicitly set rep_memo so that the messages later match
		CHECKED(dba_msg_set_rep_memo(msg, rep_memo_from_msg(msg), -1));

		vector<dba_msg> msgs;
		CHECKED(dba_record_key_setc(query, DBA_KEY_REP_MEMO, rep_memo_from_msg(msg)));
		CHECKED(dba_db_export(db, query, msg_collector, &msgs));
		gen_ensure_equals(msgs.size(), 1u);
		gen_ensure(msgs[0] != NULL);

		/*
		if (string(files[i]).find("1-21") != string::npos)
		{
			dba_msg_print(msg, stderr);
			dba_msg_print(msgs[0], stderr);
		}
		*/

		// Compare the two dba_msg
		int diffs = 0;
		dba_msg_diff(msg, msgs[0], &diffs, stderr);
		if (diffs != 0) track_different_msgs(msg, msgs[0], "bufr-old");
		gen_ensure_equals(diffs, 0);

		dba_msgs_delete(inmsgs);
		for (vector<dba_msg>::iterator i = msgs.begin(); i != msgs.end(); i++)
			dba_msg_delete(*i);
	}

	dba_record_delete(query);
	test_untag();
}

template<> template<>
void to::test<3>()
{
	use_db();

	const char* files[] = {
		"aof/obs1-14.63.aof",
		"aof/obs1-21.1.aof",
		"aof/obs1-24.2104.aof",
		"aof/obs1-24.34.aof",
		"aof/obs2-144.2198.aof",
		"aof/obs4-165.2027.aof",
		"aof/obs5-35.61.aof",
		"aof/obs5-36.30.aof",
		"aof/obs6-32.1573.aof",
		NULL
	};
	/*	"aof/test-01.aof", */

	dba_record query;
	CHECKED(dba_record_create(&query));

	for (int i = 0; files[i] != NULL; i++)
	{
		test_tag(files[i]);
		dba_msgs inmsgs = read_test_msg(files[i], AOF);
		dba_msg msg = inmsgs->msgs[0];

		CHECKED(dba_db_reset(db, NULL));
		CHECKED(dba_import_msg(db, msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_DATETIME_ATTRS));
		// Explicitly set rep_memo so that the messages later match
		CHECKED(dba_msg_set_rep_memo(msg, rep_memo_from_msg(msg), -1));

		vector<dba_msg> msgs;
		CHECKED(dba_record_key_setc(query, DBA_KEY_REP_MEMO, rep_memo_from_msg(msg)));
		CHECKED(dba_db_export(db, query, msg_collector, &msgs));
		gen_ensure_equals(msgs.size(), 1u);
		gen_ensure(msgs[0] != NULL);

		/*
		if (string(files[i]).find("1-21") != string::npos)
		{
			dba_msg_print(msg, stderr);
			dba_msg_print(msgs[0], stderr);
		}
		*/

		// Compare the two dba_msg
		int diffs = 0;
		dba_msg_diff(msg, msgs[0], &diffs, stderr);
		if (diffs != 0) track_different_msgs(msg, msgs[0], "aof-old");
		gen_ensure_equals(diffs, 0);

		dba_msgs_delete(inmsgs);
		for (vector<dba_msg>::iterator i = msgs.begin(); i != msgs.end(); i++)
			dba_msg_delete(*i);
	}

	dba_record_delete(query);
	test_untag();
}

// Check that multiple messages are correctly identified during export
template<> template<>
void to::test<4>()
{
	use_db();

	// msg1 has latitude 33.88
	// msg2 has latitude 46.22
	dba_msgs msgs1 = read_test_msg("bufr/obs0-1.22.bufr", BUFR);
	dba_msgs msgs2 = read_test_msg("bufr/obs0-3.504.bufr", BUFR);
	dba_msg msg1 = msgs1->msgs[0];
	dba_msg msg2 = msgs2->msgs[0];

	CHECKED(dba_db_reset(db, NULL));
	CHECKED(dba_import_msg(db, msg1, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_DATETIME_ATTRS));
	CHECKED(dba_import_msg(db, msg2, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_DATETIME_ATTRS));
	// Explicitly set rep_memo so that the messages later match
	CHECKED(dba_msg_set_rep_memo(msg1, rep_memo_from_msg(msg1), -1));
	CHECKED(dba_msg_set_rep_memo(msg2, rep_memo_from_msg(msg2), -1));

	dba_record query;
	CHECKED(dba_record_create(&query));
	CHECKED(dba_record_key_setc(query, DBA_KEY_REP_MEMO, rep_memo_from_msg(msg1)));

	// fprintf(stderr, "Queried: %d\n", rep_cod_from_msg(msg1));

	// Export with the old algorithm
	vector<dba_msg> msgs;
	CHECKED(dba_db_export(db, query, msg_collector, &msgs));
	// Warning: this test fails on Debian: http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=397597
	gen_ensure_equals(msgs.size(), 2u);
	gen_ensure(msgs[0] != NULL);
	gen_ensure(msgs[1] != NULL);

	// Compare the two dba_msg
	int diffs = 0;
	dba_msg_diff(msg1, msgs[0], &diffs, stderr);
	if (diffs != 0) track_different_msgs(msg1, msgs[0], "synop1-old");
	gen_ensure_equals(diffs, 0);

	diffs = 0;
	dba_msg_diff(msg2, msgs[1], &diffs, stderr);
	if (diffs != 0) track_different_msgs(msg2, msgs[1], "synop2-old");
	gen_ensure_equals(diffs, 0);

	dba_msgs_delete(msgs1);
	dba_msgs_delete(msgs2);
	for (vector<dba_msg>::iterator i = msgs.begin(); i != msgs.end(); i++)
		dba_msg_delete(*i);

	dba_record_delete(query);
}

// Check that all imported messages are found on export
template<> template<>
void to::test<5>()
{
	use_db();

	msg_generator gen;

	CHECKED(dba_db_reset(db, NULL));

	/* Fix the seed so that we always get predictable results */
	srand(1);
	
	/* Import 100 random messages */
	for (int i = 0; i < 100; i++)
	{
		dba_msg msg;
		CHECKED(dba_msg_create(&msg));
		CHECKED(gen.fill_message(msg, rnd(0.8)));
		CHECKED(dba_import_msg(db, msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_DATETIME_ATTRS));
		dba_msg_delete(msg);
	}

	// Prepare the query
	dba_record query;
	CHECKED(dba_record_create(&query));
	CHECKED(dba_record_key_seti(query, DBA_KEY_REP_COD, 255));

	// Export
	vector<dba_msg> msgs;
	CHECKED(dba_db_export(db, query, msg_collector, &msgs));
	gen_ensure_equals(msgs.size(), 100u);
	for (vector<dba_msg>::iterator i = msgs.begin(); i != msgs.end(); ++i)
	{
		gen_ensure(*i != NULL);
		dba_msg_delete(*i);
	}

	dba_record_delete(query);
}

static dba_err msg_counter(dba_msgs msgs, void* data)
{
	(*(int*)data) += msgs->len;
	dba_msgs_delete(msgs);
	return dba_error_ok();
}

// Check that the right messages are exported
template<> template<>
void to::test<6>()
{
	use_db();

	msg_vector msgs;

	// All the various input messages with unique data
	static const char* bufr_files[] = {
		"bufr/obs0-1.22.bufr",
		"bufr/obs0-3.504.bufr",
		"bufr/obs1-11.16.bufr",
		"bufr/obs1-13.36.bufr",
		"bufr/obs1-19.3.bufr",
		"bufr/obs1-21.1.bufr",
		"bufr/obs1-9.2.bufr",
		"bufr/obs1-140.454.bufr",
		"bufr/obs2-101.16.bufr",
		"bufr/obs2-102.1.bufr",
		"bufr/obs2-91.2.bufr",
		"bufr/obs4-142.13803.bufr",
		"bufr/obs4-142.1.bufr",
		"bufr/obs4-144.4.bufr",
		"bufr/obs4-145.4.bufr",
	};
	static const char* crex_files[] = {
		"crex/test-mare1.crex",
		"crex/test-synop0.crex",
		"crex/test-synop2.crex",
		"crex/test-temp0.crex",
	};
	static const char* aof_files[] = {
		"aof/obs1-14.63.aof",
		"aof/obs1-21.1.aof",
		"aof/obs1-24.2104.aof",
		"aof/obs1-24.34.aof",
		"aof/obs2-144.2198.aof",
		"aof/obs4-165.2027.aof",
		"aof/obs5-35.61.aof",
		"aof/obs6-32.1573.aof",
	};

	for (size_t i = 0; i < sizeof(bufr_files) / sizeof(const char*); i++)
		CHECKED(read_file(BUFR, bufr_files[i], msgs));
	for (size_t i = 0; i < sizeof(crex_files) / sizeof(const char*); i++)
		CHECKED(read_file(CREX, crex_files[i], msgs));
	for (size_t i = 0; i < sizeof(aof_files) / sizeof(const char*); i++)
		CHECKED(read_file(AOF, aof_files[i], msgs));

	CHECKED(dba_db_reset(db, NULL));

	map<dba_msg_type, int> rep_cods;
	for (msg_vector::const_iterator i = msgs.begin(); i != msgs.end(); i++)
	{
		CHECKED(dba_import_msgs(db, *i, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_DATETIME_ATTRS | DBA_IMPORT_OVERWRITE));
		rep_cods[(*i)->msgs[0]->type]++;
	}

	dba_record query;
	CHECKED(dba_record_create(&query));
	for (map<dba_msg_type, int>::const_iterator i = rep_cods.begin(); i != rep_cods.end(); i++)
	{
		test_tag(dba_msg_type_name(i->first));

		int count = 0;
		CHECKED(dba_record_key_setc(query, DBA_KEY_REP_MEMO, dba_msg_repmemo_from_type(i->first)));
		CHECKED(dba_db_export(db, query, msg_counter, &count));
		gen_ensure_equals(count, i->second);
	}

	test_untag();
}

static void clear_datetime_attrs(dba_msg msg)
{
	for (int i = 0; i < msg->data_count; i++)
	{
		dba_msg_level lev = msg->data[i];
		if (lev->ltype1 != 257) continue;
		for (int j = 0; j < lev->data_count; j++)
		{
			dba_msg_datum dat = lev->data[j];
			if (DBA_VAR_X(dba_var_code(dat->var)) != 4) continue;
			dba_var_clear_attrs(dat->var);
		}
    }
}

// Check a case when two AOF messages cannot be exported after import
template<> template<>
void to::test<7>()
{
	use_db();

	msg_vector msgs;
	const char* fname = "aof/err1.aof";

	CHECKED(read_file(AOF, fname, msgs));

	CHECKED(dba_db_reset(db, NULL));

	//map<dba_msg_type, int> rep_cods;
	for (msg_vector::const_iterator i = msgs.begin(); i != msgs.end(); i++)
	{
		CHECKED(dba_import_msgs(db, *i, NULL, DBA_IMPORT_ATTRS));
		//rep_cods[(*i)->msgs[0]->type]++;
	}

	dba_record query;
	CHECKED(dba_record_create(&query));
	msg_vector msgs1;
	CHECKED(dba_db_export(db, query, msgs_collector, &msgs1));
	gen_ensure_equals(msgs1.size(), 2u);

	clear_datetime_attrs(msgs[0]->msgs[0]);
	clear_datetime_attrs(msgs[1]->msgs[0]);

	#if 0
	fprintf(stderr, "msgs[0]\n");
	dba_msg_print(msgs[0]->msgs[0], stderr);
	fprintf(stderr, "msgs[1]\n");
	dba_msg_print(msgs[1]->msgs[0], stderr);
	fprintf(stderr, "msgs1[0]\n");
	dba_msg_print(msgs1[0]->msgs[0], stderr);
	fprintf(stderr, "msgs1[1]\n");
	dba_msg_print(msgs1[1]->msgs[0], stderr);
	#endif

	#if 0
	// Compare the two dba_msg
	int diffs = 0;
	dba_msg_diff(msgs[0]->msgs[0], msgs1[0]->msgs[0], &diffs, stderr);
	if (diffs != 0) track_different_msgs(msgs[0], msgs1[0], "aof-reexported1");
	gen_ensure_equals(diffs, 0);

	diffs = 0;
	dba_msg_diff(msgs[1]->msgs[0], msgs1[1]->msgs[0], &diffs, stderr);
	if (diffs != 0) track_different_msgs(msgs[1], msgs1[1], "aof-reexported2");
	gen_ensure_equals(diffs, 0);
	#endif

	dba_rawmsg rmsg;
	CHECKED(dba_marshal_encode(msgs[0], BUFR, &rmsg));
	dba_rawmsg_delete(rmsg);
	CHECKED(dba_marshal_encode(msgs[1], BUFR, &rmsg));
	dba_rawmsg_delete(rmsg);
	CHECKED(dba_marshal_encode(msgs1[0], BUFR, &rmsg));
	dba_rawmsg_delete(rmsg);
	CHECKED(dba_marshal_encode(msgs1[1], BUFR, &rmsg));
	dba_rawmsg_delete(rmsg);


	dba_record_delete(query);


	#if 0
	dba_record query;
	CHECKED(dba_record_create(&query));
	for (map<dba_msg_type, int>::const_iterator i = rep_cods.begin(); i != rep_cods.end(); i++)
	{
		test_tag(dba_msg_type_name(i->first));

		int count = 0;
		CHECKED(dba_record_key_seti(query, DBA_KEY_REP_COD, dba_msg_repcod_from_type(i->first)));
		CHECKED(dba_db_export(db, query, msg_counter, &count));
		gen_ensure_equals(count, i->second);
	}
	#endif
}

// Check a case when two messages imported get mangled and export exports
// different messages
template<> template<>
void to::test<8>()
{
	use_db();

	msg_vector msgs;
	const char* fname = "bufr/synotemp.bufr";

	CHECKED(read_file(BUFR, fname, msgs));

	CHECKED(dba_db_reset(db, NULL));

	//map<dba_msg_type, int> rep_cods;
	for (msg_vector::const_iterator i = msgs.begin(); i != msgs.end(); i++)
	{
		CHECKED(dba_import_msgs(db, *i, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_DATETIME_ATTRS));
		//rep_cods[(*i)->msgs[0]->type]++;
	}

	dba_record query;
	CHECKED(dba_record_create(&query));
	msg_vector msgs1;
	CHECKED(dba_db_export(db, query, msgs_collector, &msgs1));
	gen_ensure_equals(msgs1.size(), 2u);

	//clear_datetime_attrs(msgs[0]->msgs[0]);
	//clear_datetime_attrs(msgs[1]->msgs[0]);

	#if 0
	fprintf(stderr, "msgs[0]\n");
	dba_msg_print(msgs[0]->msgs[0], stderr);
	fprintf(stderr, "msgs[1]\n");
	dba_msg_print(msgs[1]->msgs[0], stderr);
	fprintf(stderr, "msgs1[0]\n");
	dba_msg_print(msgs1[0]->msgs[0], stderr);
	fprintf(stderr, "msgs1[1]\n");
	dba_msg_print(msgs1[1]->msgs[0], stderr);
	#endif

	// Explicitly set rep_memo so that the messages later match
	CHECKED(dba_msg_set_rep_memo(msgs[0]->msgs[0], rep_memo_from_msg(msgs[0]->msgs[0]), -1));
	CHECKED(dba_msg_set_rep_memo(msgs[1]->msgs[0], rep_memo_from_msg(msgs[1]->msgs[0]), -1));

	// Compare the two dba_msg
	int diffs = 0;
	dba_msg_diff(msgs[0]->msgs[0], msgs1[1]->msgs[0], &diffs, stderr);
	if (diffs != 0) track_different_msgs(msgs[0], msgs1[1], "synotemp-reexported1");
	gen_ensure_equals(diffs, 0);

	diffs = 0;
	dba_msg_diff(msgs[1]->msgs[0], msgs1[0]->msgs[0], &diffs, stderr);
	if (diffs != 0) track_different_msgs(msgs[1], msgs1[0], "synotemp-reexported2");
	gen_ensure_equals(diffs, 0);

	//dba_rawmsg rmsg;
	//CHECKED(dba_marshal_encode(msgs[0], BUFR, &rmsg));
	//dba_rawmsg_delete(rmsg);
	//CHECKED(dba_marshal_encode(msgs[1], BUFR, &rmsg));
	//dba_rawmsg_delete(rmsg);
	//CHECKED(dba_marshal_encode(msgs1[0], BUFR, &rmsg));
	//dba_rawmsg_delete(rmsg);
	//CHECKED(dba_marshal_encode(msgs1[1], BUFR, &rmsg));
	//dba_rawmsg_delete(rmsg);


	dba_record_delete(query);


	#if 0
	dba_record query;
	CHECKED(dba_record_create(&query));
	for (map<dba_msg_type, int>::const_iterator i = rep_cods.begin(); i != rep_cods.end(); i++)
	{
		test_tag(dba_msg_type_name(i->first));

		int count = 0;
		CHECKED(dba_record_key_seti(query, DBA_KEY_REP_COD, dba_msg_repcod_from_type(i->first)));
		CHECKED(dba_db_export(db, query, msg_counter, &count));
		gen_ensure_equals(count, i->second);
	}
	#endif
}

}

// vim:set ts=4 sw=4:
