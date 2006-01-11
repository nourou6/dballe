#ifndef DBA_BTABLE_H
#define DBA_BTABLE_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup core
 * Implement fast access to information about WMO variables.
 */


#include <dballe/err/dba_error.h>
#include <dballe/core/dba_array.h>

/**
 * Holds the WMO variable code of a variable
 */
typedef short unsigned int dba_varcode;

DBA_ARR_DECLARE(dba_varcode, varcode);

/**
 * Holds the information about a DBALLE variable.
 *
 * It never needs to be deallocated, as all the dba_varinfo returned by DB-ALLe
 * are pointers to memory-cached versions that are guaranteed to exist for all
 * the lifetime of the program.
 */
struct _dba_varinfo
{
	/** The variable code.  @see DBA_VAR, DBA_VAR_X, DBA_VAR_Y. */
	dba_varcode var;
	/** The variable description. */
	char desc[64];
	/** The measurement unit of the variable. */
	char unit[24];
	/** The scale of the variable.  When the variable is represented as an
	 * integer, it is multiplied by 10**scale */
	int scale;
	/** The reference value for the variable.  When the variable is represented
	 * as an integer, and after scaling, it is added this value */
	int ref;
	/** The length in digits of the integer representation of this variable
	 * (after scaling and changing reference value) */
	int len;
	/** The reference value for bit-encoding.  When the variable is encoded in
	 * a bit string, it is added this value */
	int bit_ref;
	/** The length in bits of the variable when encoded in a bit string (after
	 * scaling and changing reference value) */
	int bit_len;
	/** True if the variable is a string; false if it is a numeric value */
	int is_string;
};
typedef struct _dba_varinfo* dba_varinfo;

/**
 * Holds a variable information table
 *
 * It never needs to be deallocated, as all the dba_vartable returned by
 * DB-ALLe are pointers to memory-cached versions that are guaranteed to exist
 * for all the lifetime of the program.
 */
struct _dba_vartable;
typedef struct _dba_vartable* dba_vartable;

/**
 * Create a WMO variable code from its F, X and Y components.
 */
#define DBA_VAR(f, x, y) ((dba_varcode)( ((unsigned)(f)<<14) | ((unsigned)(x)<<8) | (unsigned)(y) ))

/**
 * Convert a XXYYY string to a WMO variable code.
 *
 * This is useful only in rare cases, such as when parsing tables; use
 * dba_descriptor_code to parse proper entry names such as "B01003" or "D21301".
 */
#define DBA_STRING_TO_VAR(str) ((dba_varcode)( \
		(( ((str)[0] - '0')*10 + ((str)[1] - '0') ) << 8) | \
		( ((str)[2] - '0')*100 + ((str)[3] - '0')*10 + ((str)[4] - '0') ) \
))

/**
 * Get the F part of a WMO variable code.
 */
#define DBA_VAR_F(code) ((code) >> 14)
/**
 * Get the X part of a WMO variable code.
 */
#define DBA_VAR_X(code) ((code) >> 8 & 0x3f)
/**
 * Get the Y part of a WMO variable code.
 */
#define DBA_VAR_Y(code) ((code) & 0xff)


/**
 * Query informations about the DBALLE variable definitions
 * 
 * @param code
 *   The ::dba_varcode of the variable to query
 * @retval info
 *   The ::dba_varinfo structure with the variable results.  It needs to be
 *   deallocated using dba_varinfo_delete()
 * @return
 *   The error indicator for this function (@see dba_err)
 */
dba_err dba_varinfo_query_local(dba_varcode code, dba_varinfo* info);

/**
 * Get a copy of the local B table
 *
 * @retval table
 *   The copy of the B table.  It needs to be deallocated with
 *   dba_varinfo_delete()
 * @return
 *   The error indicator for this function (@see dba_err)
 */
dba_err dba_varinfo_get_local_table(dba_vartable* table);

/**
 * Convert a FXXYYY string descriptor code into its short integer
 * representation.
 *
 * @param desc
 *   The 6-byte string descriptor as FXXYYY
 *
 * @return
 *   The short integer code that can be queried with the DBA_GET_* macros
 */
dba_varcode dba_descriptor_code(const char* desc);

/**
 * Create a new vartable structure
 *
 * @param id
 *   ID of the vartable data to access
 * @retval table
 *   The vartable to access the data
 * @return
 *   The error status (@see dba_err)
 */
dba_err dba_vartable_create(const char* id, dba_vartable* table);

/**
 * Return the ID for the given table
 *
 * @param table
 *   The table to query
 * @return
 *   The table id.  It is a pointer to the internal value, and does not need to
 *   be deallocated.
 */
const char* dba_vartable_id(dba_vartable table);

/**
 * Query the vartable
 *
 * @param table
 *   vartable to query
 * @param var
 *   vartable entry number (i.e. the XXYYY number in BXXYYY)
 * @retval info
 *   the ::dba_varinfo structure with the results of the query.  The returned
 *   dba_varinfo needs to be deallocated using dba_varinfo_delete()
 * @return
 *   The error status (@see dba_err)
 */
dba_err dba_vartable_query(dba_vartable table, dba_varcode var, dba_varinfo* info);

typedef void (*dba_vartable_iterator)(dba_varinfo info, void* data);

/**
 * Iterate through all elements in a dba_vartable
 */
dba_err dba_vartable_iterate(dba_vartable table, dba_vartable_iterator func, void* data);

#ifdef  __cplusplus
}
#endif

#endif
/* vim:set ts=4 sw=4: */
