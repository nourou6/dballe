#ifndef DBA_VAR_H
#define DBA_VAR_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup core
 * Implement fast access to information about WMO variables.
 */


#include <dballe/err/dba_error.h>
#include <dballe/core/dba_vartable.h>
#include <stdio.h>

/**
 * Holds a DBALLE variable
 */
struct _dba_var;
typedef struct _dba_var* dba_var;

/**
 * Create a new dba_var
 *
 * @param info
 *   The dba_varinfo that describes the variable
 * @retval var
 *   The variable created.  It will need to be deallocated using
 *   dba_var_delete().
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_create(dba_varinfo info, dba_var* var);
dba_err dba_var_createi(dba_varinfo info, dba_var* var, int val);
dba_err dba_var_created(dba_varinfo info, dba_var* var, double val);
dba_err dba_var_createc(dba_varinfo info, dba_var* var, const char* val);

/**
 * Create a variable with informations from the local table
 *
 * @param code
 *   The dba_varcode that identifies the variable in the local B table.
 * @retval var
 *   The variable created.  It will need to be deallocated using
 *   dba_var_delete().
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_create_local(dba_varcode code, dba_var* var);

/**
 * Make an exact copy of a dba_var
 *
 * @param source
 *   The variable to copy
 * @retval dest
 *   The new copy of source.  It will need to be deallocated using
 *   dba_var_delete().
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_copy(dba_var source, dba_var* dest);

void dba_var_delete(dba_var var);

dba_err dba_var_enqi(dba_var var, int* val);
dba_err dba_var_enqd(dba_var var, double* val);
dba_err dba_var_enqc(dba_var var, const char** val);

dba_err dba_var_seti(dba_var var, int val);
dba_err dba_var_setd(dba_var var, double val);
dba_err dba_var_setc(dba_var var, const char* val);

dba_err dba_var_unset(dba_var var);

/**
 * Query variable attributes
 *
 * @param var
 *   The variable to query
 * @param code
 *   The dba_varcode of the attribute requested
 * @retval attr
 *   A pointer to the attribute if it exists, else NULL.  The pointer points to
 *   the internal representation and must not be deallocated by the caller.
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_enqa(dba_var var, dba_varcode code, dba_var* attr);
dba_err dba_var_seta(dba_var var, dba_var attr);
dba_err dba_var_seta_nocopy(dba_var var, dba_var attr);
dba_err dba_var_unseta(dba_var var, dba_varcode code);

/**
 * Retrieve the dba_varcode for a variable.  This function cannot fail, as
 * dba_var always have a varcode value.
 *
 * @param var
 *   Variable to query
 * @returns
 *   The dba_varcode for the variable
 */
dba_varcode dba_var_code(dba_var var);

/**
 * Get informations about the variable
 * 
 * @param var
 *   The variable to query informations for
 * @returns info
 *   The dba_varinfo for the variable
 */
dba_varinfo dba_var_info(dba_var var);

/**
 * Retrieve the internal string representation of the value for a variable.
 *
 * @param var
 *   Variable to query
 * @returns
 *   A const pointer to the internal string representation, or NULL if the
 *   variable is not defined.
 */
const char* dba_var_value(dba_var var);

/**
 * Copy a value from a variable to another, performing conversions if needed
 *
 * @param dest
 *   The variable to write the value to
 * @param orig
 *   The variable to read the value from
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_copy_val(dba_var dest, dba_var orig);

/**
 * Copy all the attributes from one variable to another.
 *
 * @param dest
 *   The variable that will hold the attributes.
 * @param src
 *   The variable with the attributes to copy.
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_copy_attrs(dba_var dest, dba_var src);

/**
 * Convert a variable to an equivalent variable using different informations
 *
 * @param orig
 *   The variable to convert
 * @param info
 *   The ::dba_varinfo describing the target of the conversion
 * @retval conv
 *   The converted variable.  It needs to be deallocated using
 *   dba_var_delete().
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_convert(dba_var orig, dba_varinfo info, dba_var* conv);

/**
 * Encode a double value into an integer value using varinfo encoding
 * informations
 *
 * @param fval
 *   Value to encode
 * @param info
 *   dba_varinfo structure to use for the encoding informations
 * @returns
 *   The double value encoded as an integer
 */
long dba_var_encode_int(double fval, dba_varinfo info);

/**
 * Decode a double value from integer value using varinfo encoding
 * informations
 *
 * @param val
 *   Value to decode
 * @param info
 *   dba_varinfo structure to use for the encoding informations
 * @returns
 *   The decoded double value
 */
double dba_var_decode_int(long val, dba_varinfo info);

/**
 * Print the variable to an output stream
 *
 * @param var
 *   The variable to print
 * @param out
 *   The output stream to use for printing
 */
void dba_var_print(dba_var var, FILE* out);

/**
 * Print the difference between two variables to an output stream.
 * If there is no difference, it does not print anything.
 *
 * @param var1
 *   The first variable to compare
 * @param var2
 *   The second variable to compare
 * @retval diffs
 *   Incremented by 1 if the variables differ
 * @param out
 *   The output stream to use for printing
 */
void dba_var_diff(dba_var var1, dba_var var2, int* diffs, FILE* out);

#ifdef  __cplusplus
}
#endif

#endif
/* vim:set ts=4 sw=4: */
