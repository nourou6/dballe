#ifndef DBALLE_DB_DATA_H
#define DBALLE_DB_DATA_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup db
 *
 * Pseudoana table management used by the db module, but not
 * exported as official API.
 */

#include <dballe/db/internals.h>

struct _dba_db;
	
/**
 * Precompiled query to insert a value in data
 */
struct _dba_db_data
{
	struct _dba_db* db;
	SQLHSTMT sstm;
	SQLHSTMT istm;
	SQLHSTMT ustm;

	int id;
	int id_context;
	dba_varcode id_var;
	char value[255];
	SQLINTEGER value_ind;
};
typedef struct _dba_db_data* dba_db_data;


dba_err dba_db_data_create(dba_db db, dba_db_data* ins);
void dba_db_data_delete(dba_db_data ins);
dba_err dba_db_data_get_id(dba_db_data ins, int *id);
dba_err dba_db_data_insert(dba_db_data ins, int rewrite, int* id);


#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
