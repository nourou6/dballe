#ifndef DBA_ARRAY_H
#define DBA_ARRAY_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup core
 * Dynamic array for various types
 *
 * 
 */

#include <dballe/err/dba_error.h>

/*
 * Implementation note: this array is to be kept as simple as possible, since
 * code generated by a macro is harder than usual to maintain.
 */

#define DBA_ARR_DECLARE(type, name) \
	struct _dba_arr_##name; \
	typedef struct _dba_arr_##name* dba_arr_##name; \
	dba_err dba_arr_##name##_create(dba_arr_##name* arr); \
	void dba_arr_##name##_delete(dba_arr_##name arr); \
	dba_err dba_arr_##name##_append(dba_arr_##name arr, type val); \
	type* dba_arr_##name##_data(dba_arr_##name arr); \
	int dba_arr_##name##_size(dba_arr_##name arr);

#define DBA_ARR_DEFINE(type, name) \
    struct _dba_arr_##name { \
		type* arr; \
		int len; \
		int alloclen; \
	}; \
    \
    dba_err dba_arr_##name##_create(dba_arr_##name* arr) { \
		*arr = (dba_arr_##name)calloc(1, sizeof(struct _dba_arr_##name)); \
		if (*arr == NULL) \
			return dba_error_alloc("allocating new dba_arr_" #name); \
		return dba_error_ok(); \
	} \
    void dba_arr_##name##_delete(dba_arr_##name arr) { \
		if (arr->arr != NULL) free(arr->arr); \
		free(arr); \
	} \
    dba_err dba_arr_##name##_append(dba_arr_##name arr, type val) {\
		if (arr->len == arr->alloclen) { \
			if (arr->alloclen == 0) { \
				arr->alloclen = 4; \
				if ((arr->arr = (type*)malloc(arr->alloclen * sizeof(type))) == NULL) \
					return dba_error_alloc("allocating memory for data in dba_arr_" #name); \
			} else { \
				type* newarr; \
				/* Double the size of the arr buffer */ \
				arr->alloclen <<= 1; \
				if ((newarr = (type*)realloc(arr->arr, arr->alloclen)) == NULL) \
					return dba_error_alloc("allocating memory for expanding data in dba_arr_" #name); \
				arr->arr = newarr; \
			} \
		} \
		arr->arr[arr->len++] = val; \
		return dba_error_ok(); \
	} \
    type* dba_arr_##name##_data(dba_arr_##name arr) { return arr->arr; } \
    int dba_arr_##name##_size(dba_arr_##name arr) { return arr->len; }


DBA_ARR_DECLARE(void*, ptr);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
