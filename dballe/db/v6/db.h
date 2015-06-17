/*
 * dballe/v6/db - Archive for point-based meteorological data, db layout version 6
 *
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_DB_V6_H
#define DBA_DB_V6_H

#include <dballe/db/db.h>
#include <dballe/db/trace.h>
#include <wreport/varinfo.h>
#include <string>
#include <vector>
#include <memory>

/** @file
 * @ingroup db
 *
 * Functions used to connect to DB-All.e and insert, query and delete data.
 */

/**
 * Constants used to define what values we should retrieve from a query
 */
/** Retrieve latitude and longitude */
#define DBA_DB_WANT_COORDS      (1 << 0)
/** Retrieve the mobile station identifier */
#define DBA_DB_WANT_IDENT       (1 << 1)
/** Retrieve the level information */
#define DBA_DB_WANT_LEVEL       (1 << 2)
/** Retrieve the time range information */
#define DBA_DB_WANT_TIMERANGE   (1 << 3)
/** Retrieve the date and time information */
#define DBA_DB_WANT_DATETIME    (1 << 4)
/** Retrieve the variable name */
#define DBA_DB_WANT_VAR_NAME    (1 << 5)
/** Retrieve the variable value */
#define DBA_DB_WANT_VAR_VALUE   (1 << 6)
/** Retrieve the report code */
#define DBA_DB_WANT_REPCOD      (1 << 7)
/** Retrieve the station ID */
#define DBA_DB_WANT_ANA_ID      (1 << 8)
/** Retrieve the lev_tr ID */
#define DBA_DB_WANT_CONTEXT_ID  (1 << 9)

namespace dballe {
struct Station;
struct StationValues;
struct DataValues;

namespace db {
struct Connection;
struct Statement;
struct Sequence;

namespace sql {
struct Driver;
struct Repinfo;
struct Station;
struct LevTr;
struct LevTrCache;
struct DataV6;
struct AttrV6;
}

namespace v6 {

/**
 * DB-ALLe database connection
 */
class DB : public dballe::DB
{
public:
    /// Database connection
    db::Connection* conn;
    /// Database query tracing
    Trace trace;

protected:
    /// Store information about the database ID of a variable
    struct VarID
    {
        wreport::Varcode code;
        int id;
        VarID(wreport::Varcode code, int id) : code(code), id(id) {}
    };

    /// Store database variable IDs for all last inserted variables
    std::vector<VarID> last_insert_varids;

    /// SQL driver backend
    sql::Driver* m_driver;

    /**
     * Accessors for the various parts of the database.
     *
     * @warning Before using these 5 pointers, ensure they are initialised
     * using one of the dba_db_need_* functions
     * @{
     */
    /** Report information */
    struct sql::Repinfo* m_repinfo;
    /** Station information */
    struct sql::Station* m_station;
    /** Level/timerange information */
    struct sql::LevTr* m_lev_tr;
    /// Level/timerange cache
    struct sql::LevTrCache* m_lev_tr_cache;
    /** Variable data */
    struct sql::DataV6* m_data;
    /** Variable attributes */
    struct sql::AttrV6* m_attr;
    /** @} */

    int _last_station_id;

    void init_after_connect();

    DB(std::unique_ptr<Connection> conn);

    /*
     * Lookup, insert or replace data in station taking the values from
     * rec.
     *
     * If rec did not contain ana_id, it will be set by this function.
     *
     * @param rec
     *   The record with the station information
     * @param can_add
     *   If true we can insert new stations in the database, if false we
     *   only look up existing records and raise an exception if missing
     * @returns
     *   The station ID
     */
    int obtain_station(const Station& st, bool can_add=true);

public:
    virtual ~DB();

    db::Format format() const { return V6; }

    /// Access the backend DB driver
    sql::Driver& driver();

    /// Access the repinfo table
    sql::Repinfo& repinfo();

    /// Access the station table
    sql::Station& station();

    /// Access the lev_tr table
    sql::LevTr& lev_tr();

    /// Access the lev_tr cache
    sql::LevTrCache& lev_tr_cache();

    /// Access the data table
    sql::DataV6& data();

    /// Access the data table
    sql::AttrV6& attr();

    void disappear();

    /**
     * Reset the database, removing all existing DBALLE tables and re-creating them
     * empty.
     *
     * @param repinfo_file
     *   The name of the CSV file with the report type information data to load.
     *   The file is in CSV format with 6 columns: report code, mnemonic id,
     *   description, priority, descriptor, table A category.
     *   If repinfo_file is NULL, then the default of /etc/dballe/repinfo.csv is
     *   used.
     */
    void reset(const char* repinfo_file = 0);

    /**
     * Delete all the DB-ALLe tables from the database.
     */
    void delete_tables();

    /**
     * Update the repinfo table in the database, with the data found in the given
     * file.
     *
     * @param repinfo_file
     *   The name of the CSV file with the report type information data to load.
     *   The file is in CSV format with 6 columns: report code, mnemonic id,
     *   description, priority, descriptor, table A category.
     *   If repinfo_file is NULL, then the default of /etc/dballe/repinfo.csv is
     *   used.
     * @retval added
     *   The number of repinfo entryes that have been added
     * @retval deleted
     *   The number of repinfo entryes that have been deleted
     * @retval updated
     *   The number of repinfo entryes that have been updated
     */
    void update_repinfo(const char* repinfo_file, int* added, int* deleted, int* updated);

    std::map<std::string, int> get_repinfo_priorities();

    /**
     * Get the report code from a report mnemonic
     */
    int rep_cod_from_memo(const char* memo);

    void insert_station_data(StationValues& vals, bool can_replace, bool station_can_add) override;
    void insert_data(DataValues& vals, bool can_replace, bool station_can_add) override;

    int last_station_id() const;

    void remove_station_data(const Query& query) override;
    void remove(const Query& query);
    void remove_all();

    /**
     * Remove orphan values from the database.
     *
     * Orphan values are currently:
     * \li lev_tr values for which no data exists
     * \li station values for which no lev_tr exists
     *
     * Depending on database size, this routine can take a few minutes to execute.
     */
    void vacuum();

    std::unique_ptr<db::Cursor> query_stations(const Query& query);
    std::unique_ptr<db::Cursor> query_station_data(const Query& query) override;
    std::unique_ptr<db::Cursor> query_data(const Query& query);
    std::unique_ptr<db::Cursor> query_summary(const Query& query);

    /**
     * Query attributes
     *
     * @param id_data
     *   The database id of the data related to the attributes to retrieve
     * @param id_var
     *   The varcode of the variable related to the attributes to retrieve.  See @ref vartable.h (ignored)
     * @param qcs
     *   The WMO codes of the QC values requested.  If it is empty, then all values
     *   are returned.
     * @param dest
     *   The function that will be called on each attribute retrieved
     * @return
     *   Number of attributes returned in attrs
     */
    void query_attrs(int reference_id, wreport::Varcode id_var,
            std::function<void(std::unique_ptr<wreport::Var>)>&& dest) override;

    void attr_insert(wreport::Varcode id_var, const Values& attrs) override;
    void attr_insert(int id_data, wreport::Varcode id_var, const Values& attrs) override;

    /**
     * Delete QC data for the variable `var' in record `rec' (coming from a previous
     * dba_query)
     *
     * @param id_data
     *   The database id of the lev_tr related to the attributes to remove
     * @param id_var
     *   The varcode of the variable related to the attributes to remove.  See @ref vartable.h (ignored)
     * @param qcs
     *   Array of WMO codes of the attributes to delete.  If empty, all attributes
     *   associated to id_data will be deleted.
     */
    void attr_remove(int id_data, wreport::Varcode id_var, const db::AttrList& qcs);

    void import_msg(const Message& msg, const char* repmemo, int flags) override;
    bool export_msgs(const Query& query, std::function<bool(std::unique_ptr<Message>&&)> dest) override;

    /**
     * Dump the entire contents of the database to an output stream
     */
    void dump(FILE* out);

    friend class dballe::DB;
};

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
