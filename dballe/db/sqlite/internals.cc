/*
 * db/sqlite/internals - Implementation infrastructure for the SQLite DB connection
 *
 * Copyright (C) 2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "internals.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include "dballe/core/vasprintf.h"
#include "dballe/db/querybuf.h"
#include <cstdlib>
#if 0
#include <cstring>
#include <limits.h>
#include <unistd.h>
#include "dballe/core/verbose.h"
#include <iostream>
#endif

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {

namespace {

}


error_sqlite::error_sqlite(sqlite3* db, const std::string& msg)
{
    this->msg = msg;
    this->msg += ":";
    this->msg += sqlite3_errmsg(db);
}

error_sqlite::error_sqlite(const std::string& dbmsg, const std::string& msg)
{
    this->msg = msg;
    this->msg += ":";
    this->msg += dbmsg;
}

void error_sqlite::throwf(sqlite3* db, const char* fmt, ...)
{
    // Format the arguments
    va_list ap;
    va_start(ap, fmt);
    char* cmsg;
    vasprintf(&cmsg, fmt, ap);
    va_end(ap);

    // Convert to string
    std::string msg(cmsg);
    free(cmsg);
    throw error_sqlite(db, msg);
}

SQLiteConnection::SQLiteConnection()
{
}

SQLiteConnection::~SQLiteConnection()
{
    if (db) sqlite3_close(db);
}

void SQLiteConnection::open_file(const std::string& pathname, int flags)
{
    int res = sqlite3_open_v2(pathname.c_str(), &db, flags, nullptr);
    if (res != SQLITE_OK)
    {
        // From http://www.sqlite.org/c3ref/open.html
        // Whether or not an error occurs when it is opened, resources
        // associated with the database connection handle should be
        // released by passing it to sqlite3_close() when it is no longer
        // required.
        std::string errmsg(sqlite3_errmsg(db));
        sqlite3_close(db);
        db = nullptr;
        throw error_sqlite(errmsg, "opening " + pathname);
    }
    init_after_connect();
}

void SQLiteConnection::open_memory(int flags)
{
    open_file(":memory:", flags);
}

void SQLiteConnection::open_private_file(int flags)
{
    open_file("", flags);
}

void SQLiteConnection::init_after_connect()
{
    server_type = ServerType::SQLITE;
    // autocommit is off by default when inside a transaction
    // set_autocommit(false);
}

void SQLiteConnection::wrap_sqlite3_exec(const std::string& query)
{
    char* errmsg;
    int res = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errmsg);
    if (res != SQLITE_OK && errmsg)
    {
        // http://www.sqlite.org/c3ref/exec.html
        //
        // If the 5th parameter to sqlite3_exec() is not NULL then any error
        // message is written into memory obtained from sqlite3_malloc() and
        // passed back through the 5th parameter. To avoid memory leaks, the
        // application should invoke sqlite3_free() on error message strings
        // returned through the 5th parameter of of sqlite3_exec() after the
        // error message string is no longer needed.·
        std::string msg(errmsg);
        sqlite3_free(errmsg);
        throw error_sqlite(errmsg, "executing " + query);
    }
}

void SQLiteConnection::wrap_sqlite3_exec_nothrow(const std::string& query) noexcept
{
    char* errmsg;
    int res = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errmsg);
    if (res != SQLITE_OK && errmsg)
    {
        // http://www.sqlite.org/c3ref/exec.html
        //
        // If the 5th parameter to sqlite3_exec() is not NULL then any error
        // message is written into memory obtained from sqlite3_malloc() and
        // passed back through the 5th parameter. To avoid memory leaks, the
        // application should invoke sqlite3_free() on error message strings
        // returned through the 5th parameter of of sqlite3_exec() after the
        // error message string is no longer needed.·
        fprintf(stderr, "cannot execute '%s': %s\n", query.c_str(), errmsg);
        sqlite3_free(errmsg);
    }
}

struct SQLiteTransaction : public Transaction
{
    SQLiteConnection& conn;
    bool fired = false;

    SQLiteTransaction(SQLiteConnection& conn) : conn(conn)
    {
    }
    ~SQLiteTransaction() { if (!fired) rollback_nothrow(); }

    void commit() override
    {
        conn.wrap_sqlite3_exec("COMMIT");
        fired = true;
    }
    void rollback() override
    {
        conn.wrap_sqlite3_exec("ROLLBACK");
        fired = true;
    }
    void rollback_nothrow()
    {
        conn.wrap_sqlite3_exec_nothrow("ROLLBACK");
        fired = true;
    }
};

std::unique_ptr<Transaction> SQLiteConnection::transaction()
{
    wrap_sqlite3_exec("BEGIN");
    return unique_ptr<Transaction>(new SQLiteTransaction(*this));
}

std::unique_ptr<SQLiteStatement> SQLiteConnection::sqlitestatement(const std::string& query)
{
    return unique_ptr<SQLiteStatement>(new SQLiteStatement(*this, query));
}

void SQLiteConnection::impl_exec_void(const std::string& query)
{
    wrap_sqlite3_exec(query);
}

void SQLiteConnection::impl_exec_void_int(const std::string& query, int arg1)
{
    auto s = sqlitestatement(query);
    s->bind_val(1, arg1);
    s->execute();
}

void SQLiteConnection::impl_exec_void_string(const std::string& query, const std::string& arg1)
{
    auto s = sqlitestatement(query);
    s->bind_val(1, arg1);
    s->execute();
}

void SQLiteConnection::impl_exec_void_string_string(const std::string& query, const std::string& arg1, const std::string& arg2)
{
    auto s = sqlitestatement(query);
    s->bind_val(1, arg1);
    s->bind_val(2, arg2);
    s->execute();
}

void SQLiteConnection::drop_table_if_exists(const char* name)
{
    exec(string("DROP TABLE IF EXISTS ") + name);
}

void SQLiteConnection::drop_sequence_if_exists(const char* name)
{
    // There are no sequences in sqlite, so we just do nothing
}

int SQLiteConnection::get_last_insert_id()
{
    int res = sqlite3_last_insert_rowid(db);
    if (res == 0)
        throw error_consistency("no last insert ID value returned from database");
    return res;
}

bool SQLiteConnection::has_table(const std::string& name)
{
    auto stm = sqlitestatement("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=?");
    stm->bind(name);

    bool found = false;
    int count = 0;;
    stm->execute_one([&]() {
        found = true;
        count += stm->column_int(0);
    });

    return count > 0;
}

std::string SQLiteConnection::get_setting(const std::string& key)
{
    if (!has_table("dballe_settings"))
        return string();

    auto stm = sqlitestatement("SELECT value FROM dballe_settings WHERE \"key\"=?");
    stm->bind(key);
    string res;
    stm->execute_one([&]() {
        res = stm->column_string(0);
    });

    return res;
}

void SQLiteConnection::set_setting(const std::string& key, const std::string& value)
{
    if (!has_table("dballe_settings"))
        exec("CREATE TABLE dballe_settings (\"key\" CHAR(64) NOT NULL PRIMARY KEY, value CHAR(64) NOT NULL)");

    exec("INSERT OR REPLACE INTO dballe_settings (\"key\", value) VALUES (?, ?)", key, value);
}

void SQLiteConnection::drop_settings()
{
    drop_table_if_exists("dballe_settings");
}

void SQLiteConnection::add_datetime(Querybuf& qb, const int* dt) const
{
    qb.appendf("'%04d-%02d-%02d %02d:%02d:%02d'", dt[0], dt[1], dt[2], dt[3], dt[4], dt[5]);
}

int SQLiteConnection::changes()
{
    return sqlite3_changes(db);
}

SQLiteStatement::SQLiteStatement(SQLiteConnection& conn, const std::string& query)
    : conn(conn)
{
    // From http://www.sqlite.org/c3ref/prepare.html:
    // If the caller knows that the supplied string is nul-terminated, then
    // there is a small performance advantage to be gained by passing an nByte
    // parameter that is equal to the number of bytes in the input string
    // including the nul-terminator bytes as this saves SQLite from having to
    // make a copy of the input string.
    int res = sqlite3_prepare_v2(conn, query.c_str(), query.size() + 1, &stm, nullptr);
    if (res != SQLITE_OK)
        error_sqlite::throwf(conn, "cannot compile query '%s'", query.c_str());
}

SQLiteStatement::~SQLiteStatement()
{
    // Invoking sqlite3_finalize() on a NULL pointer is a harmless no-op.
    sqlite3_finalize(stm);
}

Datetime SQLiteStatement::column_datetime(int col)
{
    Datetime res;
    string dt = column_string(col);
    sscanf(dt.c_str(), "%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu",
            &res.date.year,
            &res.date.month,
            &res.date.day,
            &res.time.hour,
            &res.time.minute,
            &res.time.second);
    return res;
}

void SQLiteStatement::execute(std::function<void()> on_row)
{
    while (true)
    {
        switch (sqlite3_step(stm))
        {
            case SQLITE_ROW:
                try {
                    on_row();
                } catch (...) {
                    wrap_sqlite3_reset_nothrow();
                    throw;
                }
                break;
            case SQLITE_DONE:
                wrap_sqlite3_reset();
                return;
            case SQLITE_BUSY:
            case SQLITE_MISUSE:
            default:
                reset_and_throw("cannot execute the query");
        }
    }
}

void SQLiteStatement::execute_one(std::function<void()> on_row)
{
    bool has_result = false;
    while (true)
    {
        switch (sqlite3_step(stm))
        {
            case SQLITE_ROW:
                if (has_result)
                {
                    wrap_sqlite3_reset();
                    throw error_consistency("query result has more than the one expected row");
                }
                on_row();
                has_result = true;
                break;
            case SQLITE_DONE:
                wrap_sqlite3_reset();
                return;
            case SQLITE_BUSY:
            case SQLITE_MISUSE:
            default:
                reset_and_throw("cannot execute the query");
        }
    }
}

void SQLiteStatement::execute()
{
    while (true)
    {
        switch (sqlite3_step(stm))
        {
            case SQLITE_ROW:
            case SQLITE_DONE:
                wrap_sqlite3_reset();
                return;
            case SQLITE_BUSY:
            case SQLITE_MISUSE:
            default:
                reset_and_throw("cannot execute the query");
        }
    }
}

void SQLiteStatement::bind_null_val(int idx)
{
    if (sqlite3_bind_null(stm, idx) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind a NULL input column");
}

void SQLiteStatement::bind_val(int idx, int val)
{
    if (sqlite3_bind_int(stm, idx, val) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind an int input column");
}

void SQLiteStatement::bind_val(int idx, unsigned val)
{
    if (sqlite3_bind_int64(stm, idx, val) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind an int64 input column");
}

void SQLiteStatement::bind_val(int idx, unsigned short val)
{
    if (sqlite3_bind_int(stm, idx, val) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind an int input column");
}

void SQLiteStatement::bind_val(int idx, const Datetime& val)
{
    char* buf;
    int size = asprintf(&buf, "%04d-%02d-%02d %02d:%02d:%02d",
            val.date.year, val.date.month, val.date.day,
            val.time.hour, val.time.minute, val.time.second);
    if (sqlite3_bind_text(stm, idx, buf, size, free) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind a text (from Datetime) input column");
}

void SQLiteStatement::bind_val(int idx, const char* val)
{
    if (sqlite3_bind_text(stm, idx, val, -1, SQLITE_STATIC))
        throw error_sqlite(conn, "cannot bind a text input column");
}

void SQLiteStatement::bind_val(int idx, const std::string& val)
{
    if (sqlite3_bind_text(stm, idx, val.data(), val.size(), SQLITE_STATIC))
        throw error_sqlite(conn, "cannot bind a text input column");
}

void SQLiteStatement::wrap_sqlite3_reset()
{
    if (sqlite3_reset(stm) != SQLITE_OK)
        throw error_sqlite(conn, "cannot reset the query");
}

void SQLiteStatement::wrap_sqlite3_reset_nothrow() noexcept
{
    sqlite3_reset(stm);
}

void SQLiteStatement::reset_and_throw(const std::string& errmsg)
{
    std::string sqlite_errmsg(sqlite3_errmsg(conn));
    wrap_sqlite3_reset_nothrow();
    throw error_sqlite(sqlite_errmsg, errmsg);
}

#if 0
bool ODBCStatement::fetch()
{
    int sqlres = SQLFetch(stm);
    if (sqlres == SQL_NO_DATA)
    {
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
        debug_reached_completion = true;
#endif
        return false;
    }
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "fetching data");
    return true;
}

bool ODBCStatement::fetch_expecting_one()
{
    if (!fetch())
    {
        close_cursor();
        return false;
    }
    if (fetch())
        throw error_consistency("expecting only one result from statement");
    close_cursor();
    return true;
}

size_t ODBCStatement::select_rowcount()
{
    if (conn.server_quirks & DBA_DB_QUIRK_NO_ROWCOUNT_IN_DIAG)
        return rowcount();

    SQLLEN res;
    int sqlres = SQLGetDiagField(SQL_HANDLE_STMT, stm, 0, SQL_DIAG_CURSOR_ROW_COUNT, &res, 0, NULL);
    // SQLRowCount is broken in newer sqlite odbc
    //int sqlres = SQLRowCount(stm, &res);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "reading row count");
    return res;
}

size_t ODBCStatement::rowcount()
{
    SQLLEN res;
    int sqlres = SQLRowCount(stm, &res);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "reading row count");
    return res;
}
#endif

#if 0
void ODBCStatement::close_cursor()
{
    int sqlres = SQLCloseCursor(stm);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "closing cursor");
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = true;
#endif
}

int ODBCStatement::execute()
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    if (!debug_reached_completion)
    {
        string msg = "Statement " + debug_query + " restarted before reaching completion";
        fprintf(stderr, "-- %s\n", msg.c_str());
        //throw error_consistency(msg);
    }
#endif
    int sqlres = SQLExecute(stm);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "executing precompiled query");
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = false;
#endif
    return sqlres;
}

int ODBCStatement::exec_direct(const char* query)
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_query = query;
#endif
    // Casting out 'const' because ODBC API is not const-conscious
    int sqlres = SQLExecDirect(stm, (SQLCHAR*)query, SQL_NTS);
    if (is_error(sqlres))
        error_odbc::throwf(SQL_HANDLE_STMT, stm, "executing query \"%s\"", query);
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = false;
#endif
    return sqlres;
}

int ODBCStatement::exec_direct(const char* query, int qlen)
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_query = string(query, qlen);
#endif
    // Casting out 'const' because ODBC API is not const-conscious
    int sqlres = SQLExecDirect(stm, (SQLCHAR*)query, qlen);
    if (is_error(sqlres))
        error_odbc::throwf(SQL_HANDLE_STMT, stm, "executing query \"%.*s\"", qlen, query);
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = false;
#endif
    return sqlres;
}

void ODBCStatement::close_cursor_if_needed()
{
    /*
    // If the query raised an error that we are ignoring, closing the cursor
    // would raise invalid cursor state
    if (sqlres != SQL_ERROR)
        close_cursor();
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    else if (sqlres == SQL_ERROR && error_is_ignored())
        debug_reached_completion = true;
#endif
*/
    SQLCloseCursor(stm);
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = true;
#endif
}
#endif

#if 0
int ODBCStatement::execute_and_close()
{
    int sqlres = SQLExecute(stm);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "executing precompiled query");
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = false;
#endif
    close_cursor_if_needed();
    return sqlres;
}

int ODBCStatement::exec_direct_and_close(const char* query)
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_query = query;
#endif
    // Casting out 'const' because ODBC API is not const-conscious
    int sqlres = SQLExecDirect(stm, (SQLCHAR*)query, SQL_NTS);
    if (is_error(sqlres))
        error_odbc::throwf(SQL_HANDLE_STMT, stm, "executing query \"%s\"", query);
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = false;
#endif
    close_cursor_if_needed();
    return sqlres;
}

int ODBCStatement::exec_direct_and_close(const char* query, int qlen)
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_query = string(query, qlen);
#endif
    // Casting out 'const' because ODBC API is not const-conscious
    int sqlres = SQLExecDirect(stm, (SQLCHAR*)query, qlen);
    if (is_error(sqlres))
        error_odbc::throwf(SQL_HANDLE_STMT, stm, "executing query \"%.*s\"", qlen, query);
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = false;
#endif
    close_cursor_if_needed();
    return sqlres;
}

int ODBCStatement::columns_count()
{
    SQLSMALLINT res;
    int sqlres = SQLNumResultCols(stm, &res);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "querying number of columns in the result set");
    return res;
}

Sequence::Sequence(ODBCConnection& conn, const char* name)
    : ODBCStatement(conn)
{
    char qbuf[100];
    int qlen;

    bind_out(1, out);
    if (conn.server_type == ServerType::ORACLE)
        qlen = snprintf(qbuf, 100, "SELECT %s.CurrVal FROM dual", name);
    else
        qlen = snprintf(qbuf, 100, "SELECT last_value FROM %s", name);
    prepare(qbuf, qlen);
}

Sequence::~Sequence() {}

const int& Sequence::read()
{
    if (is_error(SQLExecute(stm)))
        throw error_odbc(SQL_HANDLE_STMT, stm, "reading sequence value");
    /* Get the result */
    if (SQLFetch(stm) == SQL_NO_DATA)
        throw error_notfound("fetching results of sequence value reads");
    if (is_error(SQLCloseCursor(stm)))
        throw error_odbc(SQL_HANDLE_STMT, stm, "closing sequence read cursor");
    return out;
}

std::ostream& operator<<(std::ostream& o, const SQL_TIMESTAMP_STRUCT& t)
{
    char buf[20];
    snprintf(buf, 20, "%04d-%02d-%02d %02d:%02d:%02d.%d", t.year, t.month, t.day, t.hour, t.minute, t.second, t.fraction);
    o << buf;
    return o;
}
#endif

}
}