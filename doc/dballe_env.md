# DB-All.e environment variables

## `DBA_DB`

Default URL to use to connect to a database. See [Connection
methods](fapi_connect.md) for details.

## `TMPDIR`

Directory used for writing temporary files.

## `DBA_REPINFO`

Default `repinfo.csv` file to use when creating a new database, without
explicitly providing one.

By default, `/usr/share/wreport/repinfo.csv` is used.

## `DBA_TABLES`

Directory where the file with B table definitions `dballe.txt` is
searched for.

By default, `/usr/share/wreport` is used.

## `DBA_DB_FORMAT`

Override the database format to use when creating new databases.

Possible values:

 * `V6`: current stable format (the default)
 * `V7`: new format still being worked on

## `DBA_EXPLAIN`

If present in the environment, then on SQL databases `EXPLAIN` is run before
each nontrivial query, and its output is printed to standard error.

This is used to debug SQL performance problems and help design better queries.

## `DBA_PROFILE`

If present in the environment, then DB-All.e collects profiling information and
prints them to standard error.

## `DBA_LOGDIR`

If present in the environment, its value points to a directory where DB-All.e
will write a file for each database session, with a list of all nontrivial
queries that are run and their timing information.

This is used to debug performance problems.

## `DBA_INSECURE_SQLITE`

If present in the environment, speed is preferred over integrity when using
SQLite databases.

The actual details of what this means can vary. At the moment, the journal is
disabled entirely, and interrupting a running program can result in a corrupted
database file.

## `DBA_AOF_ENDIANNESS`

This controls the endianness used when writing `AOF` files.

The possible values are:

* `ARCH`: use the endianness of the current system
* `LE`: Little Endian
* `BE`: Big Endian
`

By default when the variable is not set, `ARCH` is used.

## `DBA_FORTRAN_TRACE` or `DBALLE_TRACE_FORTRAN`

If present in the environment, the variable points to a file where DB-All.e
will write all Fortran API operations that are performed, in a format that can
be easily turned into a `C++` unit test case.

This is used when debugging unexpected behaviour, to help reproduce bugs in C++
and contribute to regression testing.

`DBALLE_TRACE_FORTRAN` is the old name for this variable, still supported for
compatibility.

## `DBA_FORTRAN_TRANSACTION`

Wrap in a single database transaction everything that happens on a session
between [idba_preparati][] and [idba_fatto][], unless [idba_preparati][] is
called with only `"read"` access levels.

This should make execution faster at least on PostgreSQL and MySQL, and if
[idba_fatto][] is not called, like if the program aborts because of an error,
then the partial work is rolled back rather than kept in the database.

[idba_preparati]: fapi_reference.md#idba_preparati
[idba_messaggi]: fapi_reference.md#idba_messaggi
[idba_fatto]: fapi_reference.md#idba_fatto