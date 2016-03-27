#include "attr.h"
#include "dballe/db/v7/internals.h"
#include "dballe/sql/querybuf.h"
#include "dballe/sql/sqlite.h"
#include "dballe/var.h"

using namespace std;
using namespace wreport;
using dballe::sql::SQLiteConnection;
using dballe::sql::SQLiteStatement;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v7 {
namespace sqlite {

SQLiteAttr::SQLiteAttr(SQLiteConnection& conn, const std::string& table_name, std::unordered_set<int> State::* new_ids)
    : Attr(table_name, new_ids), conn(conn)
{
    // Precompile the statement for select
    sstm = conn.sqlitestatement("SELECT code, value FROM " + table_name + " WHERE id_data=?").release();
}

SQLiteAttr::~SQLiteAttr()
{
    delete sstm;
    delete istm;
    delete ustm;
}

void SQLiteAttr::read(int id_data, function<void(unique_ptr<Var>)> dest)
{
    sstm->bind_val(1, id_data);
    sstm->execute([&]() {
        if (sstm->column_isnull(1))
            dest(newvar(sstm->column_int(0)));
        else
            dest(newvar(sstm->column_int(0), sstm->column_string(1)));
    });
}

void SQLiteAttr::insert(dballe::db::v7::Transaction& t, v7::bulk::InsertAttrsV7& attrs, UpdateMode update_mode)
{
    Querybuf select_query;
    select_query.appendf("SELECT id_data, code, value FROM %s WHERE id_data IN (", table_name.c_str());
    select_query.start_list(",");
    int last_data_id = -1;
    bool do_select = false;
    for (const auto& a: attrs)
    {
        if (a.id_data == last_data_id) continue;
        last_data_id = a.id_data;
        if (attrs.id_data_new.find(a.id_data) != attrs.id_data_new.end()) continue;
        select_query.append_listf("%d", a.id_data);
        do_select = true;
    }
    select_query.append(") ORDER BY id_data, code");

    v7::bulk::AnnotateAttrsV7 todo(attrs);
    if (do_select)
    {
        // Get the current status of variables for this context
        auto sstm = conn.sqlitestatement(select_query);

        // Scan the result in parallel with the variable list, annotating changed
        // items with their data ID
        sstm->execute([&]() {
            todo.annotate(
                    sstm->column_int(0),
                    sstm->column_int(1),
                    sstm->column_string(2));
        });
    }
    todo.annotate_end();

    // We now have a todo-list

    switch (update_mode)
    {
        case UPDATE:
            if (todo.do_update)
            {
                if (!ustm) ustm = conn.sqlitestatement("UPDATE " + table_name + " SET value=? WHERE id_data=? AND code=?").release();
                for (auto& v: attrs)
                {
                    if (!v.needs_update()) continue;
                    // Warning: we do not know if v.var is a string, so the result of
                    // enqc is only valid until another enqc is called
                    ustm->bind(v.attr->enqc(), v.id_data, v.attr->code());
                    ustm->execute();
                    v.set_updated();
                }
            }
            break;
        case IGNORE:
            break;
        case ERROR:
            if (todo.do_update)
                throw error_consistency("refusing to overwrite existing data");
    }

    if (todo.do_insert)
    {
        if (!istm) istm = conn.sqlitestatement("INSERT INTO " + table_name + " (id_data, code, value) VALUES (?, ?, ?)").release();
        for (auto& v: attrs)
        {
            if (!v.needs_insert()) continue;
            // Warning: we do not know if v.var is a string, so the result of
            // enqc is only valid until another enqc is called
            istm->bind(v.id_data, v.attr->code(), v.attr->enqc());
            istm->execute();
            v.set_inserted();
        }
    }
}

void SQLiteAttr::_dump(std::function<void(int, wreport::Varcode, const char*)> out)
{
    auto stm = conn.sqlitestatement("SELECT id_data, code, value FROM " + table_name);
    stm->execute([&]() {
        const char* val = stm->column_isnull(2) ? nullptr : stm->column_string(2);
        out(stm->column_int(0), stm->column_int(1), val);
    });
}

}
}
}
}
