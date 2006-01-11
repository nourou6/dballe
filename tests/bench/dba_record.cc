#include "Benchmark.h"

#include <dballe/core/dba_record.h>

#include <vector>

using namespace std;

namespace bench_dba_record {

class create : public Benchmark
{
protected:
	virtual dba_err main()
	{
		static const int iterations = 1500000;

		for (int i = 0; i < iterations; i++)
		{
			dba_record rec;
			DBA_RUN_OR_RETURN(dba_record_create(&rec));
			dba_record_delete(rec);
		}
		timing("%d dba_record_create", iterations);

		return dba_error_ok();
	}

public:
	create() : Benchmark("create") {}
};

class enqset : public Benchmark
{
	vector<dba_keyword> int_keys;
	vector<dba_keyword> float_keys;
	vector<dba_keyword> char_keys;
	vector<dba_varcode> int_vars;
	vector<dba_varcode> float_vars;
	vector<dba_varcode> char_vars;

protected:
	virtual dba_err main()
	{
		static const int iterations = 1500000;
		int ival; double dval; const char* cval;
		dba_record rec;

		DBA_RUN_OR_RETURN(dba_record_create(&rec));

		for (int i = 0; i < iterations; i++)
			for (vector<dba_keyword>::const_iterator j = int_keys.begin(); j != int_keys.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_key_seti(rec, *j, 42));
		timing("%d dba_record_key_seti", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_keyword>::const_iterator j = float_keys.begin(); j != float_keys.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_key_setd(rec, *j, 42.42));
		timing("%d dba_record_key_setd", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_keyword>::const_iterator j = char_keys.begin(); j != char_keys.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_key_setc(rec, *j, "foobar"));
		timing("%d dba_record_key_setc", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_keyword>::const_iterator j = int_keys.begin(); j != int_keys.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, *j, &ival));
		timing("%d dba_record_key_enqi", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_keyword>::const_iterator j = float_keys.begin(); j != float_keys.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_key_enqd(rec, *j, &dval));
		timing("%d dba_record_key_enqd", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_keyword>::const_iterator j = char_keys.begin(); j != char_keys.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_key_enqc(rec, *j, &cval));
		timing("%d dba_record_key_enqc", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_varcode>::const_iterator j = int_vars.begin(); j != int_vars.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_var_seti(rec, *j, 42));
		timing("%d dba_record_var_seti", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_varcode>::const_iterator j = float_vars.begin(); j != float_vars.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_var_setd(rec, *j, 42.42));
		timing("%d dba_record_var_setd", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_varcode>::const_iterator j = char_vars.begin(); j != char_vars.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_var_setc(rec, *j, "foobar"));
		timing("%d dba_record_var_setc", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_varcode>::const_iterator j = int_vars.begin(); j != int_vars.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_var_enqi(rec, *j, &ival));
		timing("%d dba_record_var_enqi", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_varcode>::const_iterator j = float_vars.begin(); j != float_vars.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_var_enqd(rec, *j, &dval));
		timing("%d dba_record_var_enqd", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_varcode>::const_iterator j = char_vars.begin(); j != char_vars.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_var_enqc(rec, *j, &cval));
		timing("%d dba_record_var_enqc", iterations);

		dba_record_delete(rec);

		return dba_error_ok();
	}

public:
	enqset() : Benchmark("enqset")
	{
		int_keys.push_back(DBA_KEY_PRIORITY);
		int_keys.push_back(DBA_KEY_PRIOMAX);
		int_keys.push_back(DBA_KEY_PRIOMIN);
		int_keys.push_back(DBA_KEY_LEVELTYPE);
		int_keys.push_back(DBA_KEY_DATA_ID);
		float_keys.push_back(DBA_KEY_LAT);
		float_keys.push_back(DBA_KEY_LON);
		float_keys.push_back(DBA_KEY_LATMAX);
		float_keys.push_back(DBA_KEY_LATMIN);
		float_keys.push_back(DBA_KEY_LONMAX);
		char_keys.push_back(DBA_KEY_REP_MEMO);
		char_keys.push_back(DBA_KEY_IDENT);
		char_keys.push_back(DBA_KEY_NAME);
		char_keys.push_back(DBA_KEY_DATETIME);
		char_keys.push_back(DBA_KEY_VARLIST);
		int_vars.push_back(  DBA_VAR(0, 1,   1));
		int_vars.push_back(  DBA_VAR(0, 1,   2));
		int_vars.push_back(  DBA_VAR(0, 1,   4));
		int_vars.push_back(  DBA_VAR(0, 1,   5));
		int_vars.push_back(  DBA_VAR(0, 1,  20));
		float_vars.push_back(DBA_VAR(0, 1,  41));
		float_vars.push_back(DBA_VAR(0, 1,  42));
		float_vars.push_back(DBA_VAR(0, 1,  43));
		float_vars.push_back(DBA_VAR(0, 1, 208));
		float_vars.push_back(DBA_VAR(0, 1, 209));
		char_vars.push_back( DBA_VAR(0, 0,   1));
		char_vars.push_back( DBA_VAR(0, 0,   5));
		char_vars.push_back( DBA_VAR(0, 0,  10));
		char_vars.push_back( DBA_VAR(0, 0,  11));
		char_vars.push_back( DBA_VAR(0, 0,  12));
	}
};

class top : public Benchmark
{
public:
	top() : Benchmark("dba_record")
	{
		addChild(new create());
		addChild(new enqset());
	}
};

static RegisterRoot r(new top());

}

/* vim:set ts=4 sw=4: */
