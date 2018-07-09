#include "transaction.h"
#include "db.h"
#include "driver.h"
#include "levtr.h"
#include "cursor.h"
#include "station.h"
#include "repinfo.h"
#include "batch.h"
#include "trace.h"
#include "dballe/core/query.h"
#include "dballe/sql/sql.h"
#include <cassert>
#include <memory>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v7 {

Transaction::Transaction(std::shared_ptr<v7::DB> db, std::unique_ptr<dballe::Transaction> sql_transaction)
    : db(db), sql_transaction(sql_transaction.release()), batch(*this), trc(db->trace->trace_transaction())
{
    m_repinfo = db->driver().create_repinfo(*this).release();
    m_station = db->driver().create_station(*this).release();
    m_levtr = db->driver().create_levtr(*this).release();
    m_station_data = db->driver().create_station_data(*this).release();
    m_data = db->driver().create_data(*this).release();
}

Transaction::~Transaction()
{
    rollback();
    delete m_data;
    delete m_station_data;
    delete m_levtr;
    delete m_station;
    delete m_repinfo;
    delete sql_transaction;
}

v7::Repinfo& Transaction::repinfo()
{
    return *m_repinfo;
}

v7::Station& Transaction::station()
{
    return *m_station;
}

v7::LevTr& Transaction::levtr()
{
    return *m_levtr;
}

v7::StationData& Transaction::station_data()
{
    return *m_station_data;
}

v7::Data& Transaction::data()
{
    return *m_data;
}

void Transaction::commit()
{
    if (fired) return;
    sql_transaction->commit();
    clear_cached_state();
    fired = true;
    trc.done();
}

void Transaction::rollback()
{
    if (fired) return;
    sql_transaction->rollback();
    clear_cached_state();
    fired = true;
    trc.done();
}

void Transaction::clear_cached_state()
{
    repinfo().read_cache();
    levtr().clear_cache();
    station_data().clear_cache();
    data().clear_cache();
    batch.clear();
}

Transaction& Transaction::downcast(dballe::Transaction& transaction)
{
    v7::Transaction* t = dynamic_cast<v7::Transaction*>(&transaction);
    assert(t);
    return *t;
}

void Transaction::remove_all()
{
    auto trc = db->trace->trace_remove_all();
    db->driver().remove_all_v7(); // TODO: pass trace step
    clear_cached_state();
}

void Transaction::insert_station_data(StationValues& vals, bool can_replace, bool station_can_add)
{
    Tracer<> trc(this->trc ? this->trc->trace_insert_station_data() : nullptr);
    batch::Station* st = batch.get_station(trc, vals.info, station_can_add);

    // Add all the variables we find
    batch::StationData& sd = st->get_station_data(trc);
    for (auto& i: vals.values)
        sd.add(i.second.var, can_replace ? batch::UPDATE : batch::ERROR);

    // Perform changes
    batch.write_pending(trc);

    // Read the IDs from the results
    vals.info.id = st->id;
    for (auto& v: vals.values)
    {
        auto i = sd.ids_by_code.find(v.first);
        if (i == sd.ids_by_code.end())
            continue;
        v.second.data_id = i->id;
    }
}

void Transaction::insert_data(DataValues& vals, bool can_replace, bool station_can_add)
{
    if (vals.values.empty())
        throw error_notfound("no variables found in input record");

    Tracer<> trc(this->trc ? this->trc->trace_insert_data() : nullptr);
    batch::Station* st = batch.get_station(trc, vals.info, station_can_add);

    batch::MeasuredData& md = st->get_measured_data(trc, vals.info.datetime);

    // Insert the lev_tr data, and get the ID
    int id_levtr = levtr().obtain_id(trc, LevTrEntry(vals.info.level, vals.info.trange));

    // Add all the variables we find
    for (auto& i: vals.values)
        md.add(id_levtr, i.second.var, can_replace ? batch::UPDATE : batch::ERROR);

    // Perform changes
    batch.write_pending(trc);

    // Read the IDs from the results
    vals.info.id = st->id;
    for (auto& v: vals.values)
    {
        auto i = md.ids_on_db.find(IdVarcode(id_levtr, v.first));
        if (i == md.ids_on_db.end())
            continue;
        v.second.data_id = i->id;
    }
}

void Transaction::remove_station_data(const Query& query)
{
    Tracer<> trc(this->trc ? this->trc->trace_remove_station_data(query) : nullptr);
    cursor::run_delete_query(trc, dynamic_pointer_cast<v7::Transaction>(shared_from_this()), core::Query::downcast(query), true, db->explain_queries);
}

void Transaction::remove(const Query& query)
{
    Tracer<> trc(this->trc ? this->trc->trace_remove(query) : nullptr);
    cursor::run_delete_query(trc, dynamic_pointer_cast<v7::Transaction>(shared_from_this()), core::Query::downcast(query), false, db->explain_queries); // TODO: tracing
}

std::unique_ptr<db::CursorStation> Transaction::query_stations(const Query& query)
{
    Tracer<> trc(this->trc ? this->trc->trace_query_stations(query) : nullptr);
    auto res = cursor::run_station_query(trc, dynamic_pointer_cast<v7::Transaction>(shared_from_this()), core::Query::downcast(query), db->explain_queries);
    return move(res);
}

std::unique_ptr<db::CursorStationData> Transaction::query_station_data(const Query& query)
{
    Tracer<> trc(this->trc ? this->trc->trace_query_station_data(query) : nullptr);
    auto res = cursor::run_station_data_query(trc, dynamic_pointer_cast<v7::Transaction>(shared_from_this()), core::Query::downcast(query), db->explain_queries);
    return move(res);
}

std::unique_ptr<db::CursorData> Transaction::query_data(const Query& query)
{
    Tracer<> trc(this->trc ? this->trc->trace_query_data(query) : nullptr);
    auto res = cursor::run_data_query(trc, dynamic_pointer_cast<v7::Transaction>(shared_from_this()), core::Query::downcast(query), db->explain_queries);
    return move(res);
}

std::unique_ptr<db::CursorSummary> Transaction::query_summary(const Query& query)
{
    Tracer<> trc(this->trc ? this->trc->trace_query_summary(query) : nullptr);
    auto res = cursor::run_summary_query(trc, dynamic_pointer_cast<v7::Transaction>(shared_from_this()), core::Query::downcast(query), db->explain_queries);
    return move(res);
}

void Transaction::attr_query_station(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest)
{
    Tracer<> trc(this->trc ? this->trc->trace_func("attr_query_station") : nullptr);
    // Create the query
    auto& d = station_data();
    d.read_attrs(trc, data_id, dest);
}

void Transaction::attr_query_data(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest)
{
    Tracer<> trc(this->trc ? this->trc->trace_func("attr_query_data") : nullptr);
    // Create the query
    auto& d = data();
    d.read_attrs(trc, data_id, dest);
}

void Transaction::attr_insert_station(int data_id, const Values& attrs)
{
    Tracer<> trc(this->trc ? this->trc->trace_func("attr_insert_station") : nullptr);
    auto& d = station_data();
    d.merge_attrs(trc, data_id, attrs);
}

void Transaction::attr_insert_data(int data_id, const Values& attrs)
{
    Tracer<> trc(this->trc ? this->trc->trace_func("attr_insert_data") : nullptr);
    auto& d = data();
    d.merge_attrs(trc, data_id, attrs);
}

void Transaction::attr_remove_station(int data_id, const db::AttrList& attrs)
{
    Tracer<> trc(this->trc ? this->trc->trace_func("attr_remove_station") : nullptr);
    if (attrs.empty())
    {
        // Delete all attributes
        char buf[64];
        snprintf(buf, 64, "UPDATE station_data SET attrs=NULL WHERE id=%d", data_id);
        Tracer<> trc_upd(trc ? trc->trace_update(buf, 1) : nullptr);
        db->conn->execute(buf);
    } else {
        auto& d = station_data();
        d.remove_attrs(trc, data_id, attrs);
    }
}

void Transaction::attr_remove_data(int data_id, const db::AttrList& attrs)
{
    Tracer<> trc(this->trc ? this->trc->trace_func("attr_remove_data") : nullptr);
    if (attrs.empty())
    {
        // Delete all attributes
        char buf[64];
        snprintf(buf, 64, "UPDATE data SET attrs=NULL WHERE id=%d", data_id);
        Tracer<> trc_upd(trc ? trc->trace_update(buf, 1) : nullptr);
        db->conn->execute(buf);
    } else {
        auto& d = data();
        d.remove_attrs(trc, data_id, attrs);
    }
}

void Transaction::update_repinfo(const char* repinfo_file, int* added, int* deleted, int* updated)
{ // TODO: tracing
    repinfo().update(repinfo_file, added, deleted, updated);
}

void Transaction::dump(FILE* out)
{
    repinfo().dump(out);
    station().dump(out);
    levtr().dump(out);
    station_data().dump(out);
    data().dump(out);
}

void TestTransaction::commit()
{
    throw std::runtime_error("commit attempted while forbidden during tests");
}

}
}
}
