#include "db/tests.h"
#include "db/v6/db.h"
#include "sql/sql.h"
#include "db/v6/driver.h"
#include "db/v6/repinfo.h"
#include "db/v6/station.h"
#include "db/v6/levtr.h"
#include "db/v6/datav6.h"
#include "db/v6/attrv6.h"
#include "config.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct Fixture : DriverFixture
{
    using DriverFixture::DriverFixture;

    unique_ptr<db::v6::AttrV6> attr;

    void reset_attr()
    {
        using namespace dballe::db::v6;

        auto st = driver->create_stationv6();
        auto lt = driver->create_levtrv6();
        auto da = driver->create_datav6();

        int added, deleted, updated;
        driver->create_repinfov6()->update(nullptr, &added, &deleted, &updated);

        // Insert a mobile station
        wassert(actual(st->obtain_id(4500000, 1100000, "ciao")) == 1);

        // Insert a fixed station
        wassert(actual(st->obtain_id(4600000, 1200000)) == 2);

        // Insert a lev_tr
        wassert(actual(lt->obtain_id(Level(1, 2, 0, 3), Trange(4, 5, 6))) == 1);

        // Insert another lev_tr
        wassert(actual(lt->obtain_id(Level(2, 3, 1, 4), Trange(5, 6, 7))) == 2);

        auto t = conn->transaction();
        // Insert a datum
        {
            bulk::InsertV6 vars;
            vars.id_station = 1;
            vars.id_report = 1;
            vars.datetime = Datetime(2001, 2, 3, 4, 5, 6);
            Var var(varinfo(WR_VAR(0, 1, 2)), 123);
            vars.add(&var, 1);
            da->insert(*t, vars, DataV6::ERROR);
        }

        // Insert another datum
        {
            bulk::InsertV6 vars;
            vars.id_station = 2;
            vars.id_report = 2;
            vars.datetime = Datetime(2002, 3, 4, 5, 6, 7);
            Var var(varinfo(WR_VAR(0, 1, 2)), 234);
            vars.add(&var, 2);
            da->insert(*t, vars, DataV6::ERROR);
        }
        t->commit();
    }

    void test_setup()
    {
        DriverFixture::test_setup();
        attr = driver->create_attrv6();
        reset_attr();
    }

    Var query(int id_data, unsigned expected_attr_count)
    {
        Var res(varinfo(WR_VAR(0, 12, 101)));
        unsigned count = 0;
        attr->read(id_data, [&](unique_ptr<Var> attr) { res.seta(move(attr)); ++count; });
        wassert(actual(count) == expected_attr_count);
        return res;
    }
};

class Tests : public FixtureTestCase<Fixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        add_method("insert", [](Fixture& f) {
            using namespace dballe::db::v6;
            auto& at = *f.attr;

            auto t = f.conn->transaction();

            Var var1(varinfo(WR_VAR(0, 12, 101)), 280.0);
            var1.seta(newvar(WR_VAR(0, 33, 7), 50));

            Var var2(varinfo(WR_VAR(0, 12, 101)), 280.0);
            var2.seta(newvar(WR_VAR(0, 33, 7), 75));

            // Insert two attributes
            {
                bulk::InsertAttrsV6 attrs;
                attrs.add_all(var1, 1);
                at.insert(*t, attrs, AttrV6::ERROR);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).istrue());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).isfalse());
            }
            {
                bulk::InsertAttrsV6 attrs;
                attrs.add_all(var2, 2);
                at.insert(*t, attrs, AttrV6::ERROR);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).istrue());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).isfalse());
            }

            // Reinsert the first attribute: it should work, doing no insert/update queries
            {
                bulk::InsertAttrsV6 attrs;
                attrs.add_all(var1, 1);
                at.insert(*t, attrs, AttrV6::IGNORE);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).isfalse());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).isfalse());
            }

            // Reinsert the second attribute: it should work, doing no insert/update queries
            {
                bulk::InsertAttrsV6 attrs;
                attrs.add_all(var2, 2);
                at.insert(*t, attrs, AttrV6::UPDATE);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).isfalse());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).isfalse());
            }

            // Load the attributes for the first variable
            {
                Var var(f.query(1, 1));
                wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
                wassert(actual(*var.next_attr()) == 50);
            }

            // Load the attributes for the second variable
            {
                Var var(f.query(2, 1));
                wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
                wassert(actual(*var.next_attr()) == 75);
            }

            // Update both values
            {
                bulk::InsertAttrsV6 attrs;
                attrs.add_all(var2, 1);
                at.insert(*t, attrs, AttrV6::UPDATE);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).isfalse());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).istrue());
            }
            {
                bulk::InsertAttrsV6 attrs;
                attrs.add_all(var1, 2);
                at.insert(*t, attrs, AttrV6::UPDATE);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).isfalse());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).istrue());
            }
            // Load the attributes again to verify that they changed
            {
                Var var(f.query(1, 1));
                wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
                wassert(actual(*var.next_attr()) == 75);
            }
            {
                Var var(f.query(2, 1));
                wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
                wassert(actual(*var.next_attr()) == 50);
            }

            // TODO: test a mix of update and insert
        });
    }
};

Tests tg1("db_sql_attr_v6_sqlite", "SQLITE", db::V6);
#ifdef HAVE_ODBC
Tests tg2("db_sql_attr_v6_odbc", "ODBC", db::V6);
#endif
#ifdef HAVE_LIBPQ
Tests tg3("db_sql_attr_v6_postgresql", "POSTGRESQL", db::V6);
#endif
#ifdef HAVE_MYSQL
Tests tg4("db_sql_attr_v6_mysql", "MYSQL", db::V6);
#endif

}
