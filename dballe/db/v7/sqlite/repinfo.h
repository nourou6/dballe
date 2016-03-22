#ifndef DBALLE_DB_V7_SQLITE_REPINFO_H
#define DBALLE_DB_V7_SQLITE_REPINFO_H

#include <dballe/db/v7/repinfo.h>
#include <dballe/sql/fwd.h>
#include <vector>
#include <string>
#include <map>

namespace dballe {
struct Record;

namespace db {
namespace v7 {
namespace sqlite {

/**
 * Fast cached access to the repinfo table
 */
struct SQLiteRepinfoBase : public v7::Repinfo
{
    /**
     * DB connection. The pointer is assumed always valid during the
     * lifetime of the object
     */
    dballe::sql::SQLiteConnection& conn;

    SQLiteRepinfoBase(dballe::sql::SQLiteConnection& conn);
    SQLiteRepinfoBase(const SQLiteRepinfoBase&) = delete;
    SQLiteRepinfoBase(const SQLiteRepinfoBase&&) = delete;
    virtual ~SQLiteRepinfoBase();
    SQLiteRepinfoBase& operator=(const SQLiteRepinfoBase&) = delete;

    void dump(FILE* out) override;

protected:
    /// Return how many time this ID is used in the database
    int id_use_count(unsigned id, const char* name) override;
    void delete_entry(unsigned id) override;
    void update_entry(const v7::repinfo::Cache& entry) override;
    void insert_entry(const v7::repinfo::Cache& entry) override;
    void read_cache() override;
    void insert_auto_entry(const char* memo) override;
};

struct SQLiteRepinfoV7 : public SQLiteRepinfoBase
{
    SQLiteRepinfoV7(dballe::sql::SQLiteConnection& conn);

protected:
    int id_use_count(unsigned id, const char* name) override;
};

}
}
}
}
#endif
