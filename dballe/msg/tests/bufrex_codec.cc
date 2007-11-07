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

#include <test-utils-msg.h>
#include <dballe/msg/file.h>
#include <dballe/msg/bufrex_codec.h>
#include <dballe/msg/msg.h>

namespace tut {
using namespace tut_dballe;

struct bufrex_codec_shar
{
	bufrex_codec_shar()
	{
	}

	~bufrex_codec_shar()
	{
		test_untag();
	}
};
TESTGRP(bufrex_codec);

#define IS(field, val) do { \
		dba_var var = dba_msg_get_##field##_var(msg); \
		gen_ensure(var != 0); \
		gen_ensure_var_equals(var, val); \
	} while (0)
#define UN(field) do { \
		dba_var var = dba_msg_get_##field##_var(msg); \
		if (var != 0) \
			gen_ensure_var_undef(var); \
	} while (0)

template<> template<>
void to::test<1>()
{
	dba_msgs msgs = read_test_msg("crex/test-synop0.crex", CREX);
	dba_msg msg = msgs->msgs[0];
	gen_ensure_equals(msg->type, MSG_SYNOP);

	/* dba_msg_print((dba_msg)synop, stderr); */
	
	IS(block, 10);
	IS(station, 837);
	IS(st_type, 1);
	IS(year, 2004);
	IS(month, 11);
	IS(day, 30);
	IS(hour, 12);
	IS(minute, 0);
	IS(year, 2004);
	IS(latitude, 48.22);
	IS(longitude, 9.92);
	IS(height, 550.0);
	UN(height_baro);
	IS(press, 94340.0);
	IS(press_msl, 100940.0);
	//TESTD(var_press_tend, -170);
	IS(press_tend, 7.0);
	IS(wind_dir, 80.0);
	IS(wind_speed, 6.0);
	IS(temp_2m, 276.2);
	IS(dewpoint_2m, 273.9);
	UN(humidity);
	IS(visibility, 5000.0);
	IS(pres_wtr, 10);
	IS(past_wtr1, 2);
	IS(past_wtr2, 2);
	IS(cloud_n, 100);
	IS(cloud_nh, 8);
	IS(cloud_hh, 450.0);
	IS(cloud_cl, 35);
	IS(cloud_cm, 61);
	IS(cloud_ch, 60);
	IS(cloud_n1, 8);
	IS(cloud_c1, 6);
	IS(cloud_h1, 350.0);
	UN(cloud_n2);
	UN(cloud_c2);
	UN(cloud_h2);
	UN(cloud_n3);
	UN(cloud_c3);
	UN(cloud_h3);
	UN(cloud_n4);
	UN(cloud_c4);
	UN(cloud_h4);
	UN(tot_prec24);
	UN(tot_snow);

	dba_msg_delete(msg);
}

static void relax_bufrex_msg(bufrex_msg b)
{
	int sounding_workarounds = 
		b->type == 2 && (b->subtype == 91 || b->subtype == 101 || b->subtype == 102);

	if (b->rep_year < 100)
		b->rep_year += 2000;
	for (size_t i = 0; i < b->subsets_count; ++i)
	{
		// See what is the index of the first attribute
		size_t first_attr = 0;
		for (; first_attr < b->subsets[i]->vars_count; ++first_attr)
			if (dba_var_code(b->subsets[i]->vars[first_attr]) == DBA_VAR(0, 33, 7))
				break;
	
		// See what is the index of the data present indicator
		size_t first_dds = 0;
		for (; first_dds < b->subsets[i]->vars_count; ++first_dds)
			if (dba_var_code(b->subsets[i]->vars[first_dds]) == DBA_VAR(0, 31, 31))
				break;

		// FIXME: I still haven't figured out a good way of comparing the
		// trailing confidence intervals for temps, so I get rid of them and
		// their leading delayed replication factor 
		if (sounding_workarounds)
			bufrex_subset_truncate(b->subsets[i], first_attr - 1);

		for (size_t j = 0; j < b->subsets[i]->vars_count; ++j)
		{
			switch (dba_var_code(b->subsets[i]->vars[j]))
			{
				case DBA_VAR(0, 8, 2):
					// Vertical significances tend to lose attrs
					dba_var_clear_attrs(b->subsets[i]->vars[j]);
					if (b->type == 0 || b->type == 1)
					{
						// And they are meaningless for SYNOPs
						dba_var_seti(b->subsets[i]->vars[j], 1);
						// Also drop their confidence intervals queued for encoding
						if (first_attr + j < b->subsets[i]->vars_count)
							dba_var_unset(b->subsets[i]->vars[first_attr + j]);
					}
					// For temps, we also need to unset the data present indicators
					if (sounding_workarounds)
						if (first_dds + j < b->subsets[i]->vars_count)
							dba_var_unset(b->subsets[i]->vars[first_dds + j]);
					break;
				case DBA_VAR(0, 31, 1):
					// Delayed replication factors' confidence intervals are
					// SICK and we don't support them
					dba_var_clear_attrs(b->subsets[i]->vars[j]);
					if (first_attr + j < b->subsets[i]->vars_count)
						dba_var_unset(b->subsets[i]->vars[first_attr + j]);
					if (sounding_workarounds)
						if (first_dds + j < b->subsets[i]->vars_count)
							dba_var_unset(b->subsets[i]->vars[first_dds + j]);
					break;
				case DBA_VAR(0, 1,  31):
				case DBA_VAR(0, 1,  32):
				case DBA_VAR(0, 1, 201):
					// Generating centre and application do change
					dba_var_seti(b->subsets[i]->vars[j], 1);
					break;
			}

			if (dba_var_value(b->subsets[i]->vars[j]) == NULL)
			{
				// Remove confidence intervals for unset variables
				if (first_attr + j < b->subsets[i]->vars_count)
					dba_var_unset(b->subsets[i]->vars[first_attr + j]);
				// For temps, we also need to unset the data present indicators
				if (sounding_workarounds)
					if (first_dds + j < b->subsets[i]->vars_count)
						dba_var_unset(b->subsets[i]->vars[first_dds + j]);
			}
		}
	}
	// Some temp ship D table entries are not found in CREX D tables, so we
	// replace them with their expansion.  As a result, we cannot compare the
	// data descriptor sections of temp ship and we throw them away here.
	if (b->type == 2 && b->subtype == 102)
		bufrex_msg_reset_datadesc(b);

}

/* Test going from dba_msg to BUFR and back */
template<> template<>
void to::test<2>()
{
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
//		"bufr/obs3-3.1.bufr",
//		"bufr/obs3-56.2.bufr",
		"bufr/obs4-142.13803.bufr", 
		"bufr/obs4-142.1.bufr", 
		"bufr/obs4-144.4.bufr", 
		"bufr/obs4-145.4.bufr", 
		"bufr/test-airep1.bufr",
		"bufr/test-temp1.bufr", 
		NULL
	};

	for (int i = 0; files[i] != NULL; i++)
	{
		test_tag(files[i]);

		// Read the test message in a bufrex_raw
		bufrex_msg braw1 = read_test_msg_raw(files[i], BUFR);

		// Save category as a reference for reencoding
		int type = braw1->type;
		int subtype = braw1->subtype;
		
		// Finish converting in a dba_msg
		dba_msgs msgs1;
		CHECKED(bufrex_msg_to_dba_msgs(braw1, &msgs1));

		/// Test if reencoded bufr_msg matches

		// Reencode the message to BUFR using the same template
		bufrex_msg b1 = read_test_msg_raw(files[i], BUFR);
		bufrex_msg b2;
		CHECKED(bufrex_msg_create(BUFREX_BUFR, &b2));
		b2->type = b1->type;
		b2->subtype = b1->subtype;
		b2->edition = b1->edition;
		b2->opt.bufr.centre = b1->opt.bufr.centre;
		b2->opt.bufr.subcentre = b1->opt.bufr.subcentre;
		b2->opt.bufr.master_table = b1->opt.bufr.master_table;
		b2->opt.bufr.local_table = b1->opt.bufr.local_table;
		CHECKED(bufrex_msg_load_tables(b2));
		CHECKED(bufrex_msg_from_dba_msg(b2, msgs1->msgs[0]));

		// FIXME: relax checks a bit
		relax_bufrex_msg(b1);
		relax_bufrex_msg(b2);

		// Compare b1 and b2
		int bdiffs = 0;
		// Our metar message sample is different than the official template
		// and test-airep1 uses the wrong varcode for originating appliation
		// and test-temp1 uses a nonstandard template
		if ((b1->type != 0 || b1->subtype != 140)
				&& string(files[i]) != "bufr/test-airep1.bufr"
				&& string(files[i]) != "bufr/test-temp1.bufr")
			bufrex_msg_diff(b1, b2, &bdiffs, stderr);
		if (bdiffs > 0)
		{
			FILE* out1 = fopen("/tmp/bufrexmsg1.txt", "wt");
			FILE* out2 = fopen("/tmp/bufrexmsg2.txt", "wt");
			bufrex_msg_print(b1, out1);
			bufrex_msg_print(b2, out2);
			fclose(out1);
			fclose(out2);
			fprintf(stderr, "Message dumps have been written to /tmp/bufrexmsg1.txt and /tmp/bufrexmsg2.txt\n");
		}
		gen_ensure_equals(bdiffs, 0);

		bufrex_msg_delete(b1);
		bufrex_msg_delete(b2);


		/// Test if reencoded dba_msg match

		// Reencode the dba_msg in another dba_rawmsg
		dba_rawmsg raw2;
		CHECKED(bufrex_encode_bufr(msgs1, type, subtype, &raw2));

		// Parse the second dba_rawmsg
		dba_msgs msgs2;
		CHECKED(bufrex_decode_bufr(raw2, &msgs2));

		if (string(files[i]).find("2-101.16") != string::npos)
		{
			FILE* outraw = fopen("/tmp/1to2.txt", "w");
			bufrex_msg braw;
			CHECKED(bufrex_msg_create(BUFREX_BUFR, &braw));
			braw->edition = 3;
			braw->type = type;
			braw->subtype = subtype;
			braw->opt.bufr.centre = 98;
			braw->opt.bufr.subcentre = 0;
			braw->opt.bufr.master_table = 6;
			braw->opt.bufr.local_table = 1;
			CHECKED(bufrex_msg_load_tables(braw));
			CHECKED(bufrex_msg_from_dba_msgs(braw, msgs1));
			bufrex_msg_print(braw, outraw);
			fclose(outraw);
			bufrex_msg_delete(braw);

			FILE* out1 = fopen("/tmp/msg1.txt", "w");
			FILE* out2 = fopen("/tmp/msg2.txt", "w");
				
			dba_msgs_print(msgs1, out1);
			dba_msgs_print(msgs2, out2);
			fclose(out1);
			fclose(out2);
		}

		// Compare the two dba_msg
		int diffs = 0;
		dba_msgs_diff(msgs1, msgs2, &diffs, stderr);
		gen_ensure_equals(diffs, 0);

		//cerr << files[i] << ": ok" << endl;

		dba_msgs_delete(msgs1);
		dba_msgs_delete(msgs2);
		bufrex_msg_delete(braw1);
		dba_rawmsg_delete(raw2);
	}
	test_untag();
}

/* Test going from CREX to dba_msg and back */
template<> template<>
void to::test<3>()
{
	const char* files[] = {
		"crex/test-mare0.crex",
		"crex/test-mare1.crex",
		"crex/test-mare2.crex",
		"crex/test-synop0.crex",
		"crex/test-synop1.crex",
		"crex/test-synop2.crex",
		"crex/test-synop3.crex",
		"crex/test-temp0.crex",
		NULL
	};

	for (int i = 0; files[i] != NULL; i++)
	{
		test_tag(files[i]);

		// Read the test message in a bufrex_raw
		bufrex_msg braw1 = read_test_msg_raw(files[i], CREX);

		// Save category as a reference for reencoding
		int type = braw1->type;
		int subtype = braw1->subtype;
		if (subtype == 0)
			type = 0;
		
		// Finish converting in a dba_msg
		dba_msgs msgs1;
		CHECKED(bufrex_msg_to_dba_msgs(braw1, &msgs1));

		// Reencode the dba_msg in another dba_rawmsg
		dba_rawmsg raw2;
		CHECKED(bufrex_encode_crex(msgs1, type, subtype, &raw2));

		// Parse the second dba_rawmsg
		dba_msgs msgs2;
		CHECKED(bufrex_decode_crex(raw2, &msgs2));

		/*
		if (string(files[i]).find("mare2") != string::npos)
		{
			dba_msg_print(msg1, stderr);
			dba_msg_print(msg2, stderr);
		}
		*/

		// Compare the two dba_msg
		int diffs = 0;
		dba_msgs_diff(msgs1, msgs2, &diffs, stderr);
		gen_ensure_equals(diffs, 0);

		dba_msgs_delete(msgs1);
		dba_msgs_delete(msgs2);
		bufrex_msg_delete(braw1);
		dba_rawmsg_delete(raw2);
	}
	test_untag();
}

/* Check that a BUFR from a synop high-level station correctly reports isobaric
 * surface and geopotential */
template<> template<>
void to::test<4>()
{
	dba_msgs msgs = read_test_msg("bufr/obs0-1.11188.bufr", BUFR);
	dba_msg src = msgs->msgs[0];
	dba_var var;

	gen_ensure((var = dba_msg_get_isobaric_surface_var(src)) != NULL);
	gen_ensure(dba_var_value(var) != NULL);
	gen_ensure((var = dba_msg_get_geopotential_var(src)) != NULL);
	gen_ensure(dba_var_value(var) != NULL);

	dba_msgs_delete(msgs);
}

/* Test import of environment BUFR4 messages */
template<> template<>
void to::test<5>()
{
	dba_msgs msgs = read_test_msg("bufr/ed4.bufr", BUFR);
	dba_msg src = msgs->msgs[0];
	dba_msg_datum d;
	dba_var var;
	double val;

	gen_ensure((d = dba_msg_find(src, DBA_VAR(0, 15, 193), 105, 3, 0, 3, -3600, 0)) != NULL);
	gen_ensure(dba_var_value(d->var) != NULL);
	CHECKED(dba_var_enqd(d->var, &val));
	gen_ensure_equals(val, 2700000e-14);

	dba_msgs_delete(msgs);
}

/* TODO: add entries for more of the sample messages, taking data from another decoder */

}

/* vim:set ts=4 sw=4: */
