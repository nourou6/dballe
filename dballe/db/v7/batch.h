#ifndef DBALLE_DB_V7_BATCH_H
#define DBALLE_DB_V7_BATCH_H

#include <dballe/core/values.h>
#include <dballe/db/v7/fwd.h>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>

namespace dballe {
namespace db {
namespace v7 {
struct Transaction;

class Batch
{
protected:
    std::vector<batch::Station> stations;
    std::unordered_map<int, std::vector<size_t>> stations_by_lon;

    batch::Station* find_existing(const std::string& report, const Coords& coords, const Ident& ident);
    void index_existing(batch::Station* st, size_t pos);

public:
    Transaction& transaction;
    unsigned count_select_stations = 0;
    unsigned count_select_station_data = 0;
    unsigned count_select_data = 0;

    Batch(Transaction& transaction) : transaction(transaction) {}

    batch::Station* get_station(const dballe::Station& station, bool station_can_add);
    batch::Station* get_station(const std::string& report, const Coords& coords, const Ident& ident);

    void write_pending(bool with_attrs);
    void clear();
};


namespace batch {

enum UpdateMode {
    UPDATE,
    IGNORE,
    ERROR,
};

struct StationDatum
{
    int id = MISSING_INT;
    const wreport::Var* var;

    StationDatum(const wreport::Var* var)
        : var(var) {}
    StationDatum(int id, const wreport::Var* var)
        : id(id), var(var) {}

    void dump(FILE* out) const;
};

struct StationData
{
    std::unordered_map<wreport::Varcode, int> ids_by_code;
    std::vector<StationDatum> to_insert;
    std::vector<StationDatum> to_update;
    bool loaded = false;

    void add(const wreport::Var* var, UpdateMode on_conflict);
    void write_pending(Transaction& tr, int station_id, bool with_attrs);
};

struct MeasuredDatum
{
    int id = MISSING_INT;
    int id_levtr;
    const wreport::Var* var;

    MeasuredDatum(int id_levtr, const wreport::Var* var)
        : id_levtr(id_levtr), var(var) {}
    MeasuredDatum(int id, int id_levtr, const wreport::Var* var)
        : id(id), id_levtr(id_levtr), var(var) {}

    void dump(FILE* out) const;
};

struct IdVarcode
{
    int id;
    wreport::Varcode varcode;

    IdVarcode(int id, wreport::Varcode varcode)
        : id(id), varcode(varcode)
    {
    }

    bool operator==(const IdVarcode& o) const { return o.id == id && o.varcode == varcode; }
};

} } } }

namespace std
{
    template<> struct hash<dballe::db::v7::batch::IdVarcode>
    {
        typedef dballe::db::v7::batch::IdVarcode argument_type;
        typedef std::size_t result_type;
        result_type operator()(argument_type const& s) const noexcept
        {
            result_type const h1 ( std::hash<int>{}(s.id) );
            result_type const h2 ( std::hash<wreport::Varcode>{}(s.varcode) );
            return h1 ^ (h2 << 1);
        }
    };
}

namespace dballe { namespace db { namespace v7 { namespace batch {

struct MeasuredData
{
    Datetime datetime;
    std::unordered_map<IdVarcode, int> ids_on_db;
    std::vector<MeasuredDatum> to_insert;
    std::vector<MeasuredDatum> to_update;

    MeasuredData(Datetime datetime)
        : datetime(datetime)
    {
    }

    void add(int id_levtr, const wreport::Var* var, UpdateMode on_conflict);
    void write_pending(Transaction& tr, int station_id, bool with_attrs);
};

struct Station : public dballe::Station
{
    Batch& batch;
    bool is_new = true;
    StationData station_data;
    std::map<Datetime, MeasuredData> measured_data;

    Station(Batch& batch)
        : batch(batch) {}

    StationData& get_station_data();
    MeasuredData& get_measured_data(const Datetime& datetime);

    void write_pending(bool with_attrs);
};

}
}
}
}

#endif