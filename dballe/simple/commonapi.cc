#include "commonapi.h"
#include "dballe/var.h"
#include "dballe/types.h"
#include "dballe/core/record-access.h"
#include "dballe/db/db.h"
#include <stdio.h>  // snprintf
#include <limits>
#include <cstdlib>
#include <cstring>
#include <strings.h>

using namespace dballe;
using namespace wreport;
using namespace std;

namespace dballe {
namespace fortran {

Operation::~Operation() {}
void Operation::set_varcode(wreport::Varcode varcode) {}
wreport::Varcode Operation::dammelo(dballe::Record& output) { throw error_consistency("dammelo called without a previous voglioquesto"); }


struct VaridOperation : public Operation
{
    /// Varcode of the data variable
    wreport::Varcode varcode;
    /// Database ID of the data variable
    int varid;

    VaridOperation(int varid) : varid(varid) {}
    void set_varcode(wreport::Varcode varcode) override { this->varcode = varcode; }
    void voglioancora(db::Transaction& tr, std::vector<wreport::Var>& dest) override
    {
        if (!varid)
            throw error_consistency("voglioancora called with an invalid *context_id");
        // Retrieve the varcodes of the attributes that we want
        function<void(unique_ptr<Var>&&)> consumer;
        if (selected_attr_codes.empty())
        {
            consumer = [&](unique_ptr<Var>&& var) {
                // TODO: move into qcoutput
                dest.emplace_back(std::move(*var));
            };
        } else {
            consumer = [&](unique_ptr<Var>&& var) {
                for (auto code: selected_attr_codes)
                    if (code == var->code())
                    {
                        // TODO: move into qcoutput
                        dest.emplace_back(std::move(*var));
                        break;
                    }
            };
        }
        tr.attr_query_data(varid, consumer);
    }
    void critica(db::Transaction& tr, const Values& qcinput) override
    {
        tr.attr_insert_data(varid, qcinput);
    }
    void scusa(db::Transaction& tr) override
    {
        tr.attr_remove_data(varid, selected_attr_codes);
    }
};



CommonAPIImplementation::CommonAPIImplementation()
    : perms(0), qc_iter(-1), qc_count(0)
{
}

CommonAPIImplementation::~CommonAPIImplementation()
{
    delete operation;
}

unsigned CommonAPIImplementation::compute_permissions(const char* anaflag, const char* dataflag, const char* attrflag)
{
    unsigned perms = 0;

    if (strcasecmp("read",  anaflag) == 0)
        perms |= PERM_ANA_RO;
    if (strcasecmp("write", anaflag) == 0)
        perms |= PERM_ANA_WRITE;
    if (strcasecmp("read",  dataflag) == 0)
        perms |= PERM_DATA_RO;
    if (strcasecmp("add",   dataflag) == 0)
        perms |= PERM_DATA_ADD;
    if (strcasecmp("write", dataflag) == 0)
        perms |= PERM_DATA_WRITE;
    if (strcasecmp("read",  attrflag) == 0)
        perms |= PERM_ATTR_RO;
    if (strcasecmp("write", attrflag) == 0)
        perms |= PERM_ATTR_WRITE;

    if ((perms & (PERM_ANA_RO | PERM_ANA_WRITE)) == 0)
        throw error_consistency("pseudoana should be opened in either 'read' or 'write' mode");
    if ((perms & (PERM_DATA_RO | PERM_DATA_ADD | PERM_DATA_WRITE)) == 0)
        throw error_consistency("data should be opened in one of 'read', 'add' or 'write' mode");
    if ((perms & (PERM_ATTR_RO | PERM_ATTR_WRITE)) == 0)
        throw error_consistency("attr should be opened in either 'read' or 'write' mode");

    if (perms & PERM_ANA_RO && perms & PERM_DATA_WRITE)
        throw error_consistency("when data is 'write' ana must also be set to 'write', because deleting data can potentially also delete pseudoana");
    /*
    // Check disabled: allowing importing data without attributes is more
    // important than a dubious corner case
    if (perms & PERM_ATTR_RO && perms & PERM_DATA_WRITE)
        throw error_consistency("when data is 'write' attr must also be set to 'write', because deleting data also deletes its attributes");
    */

    return perms;
}

void CommonAPIImplementation::test_input_to_output()
{
    output = input;
}

int CommonAPIImplementation::enqi(const char* param)
{
    if (param[0] == '*')
    {
        if (strcmp(param + 1, "context_id") == 0)
            // TODO: it seems that this always returned missing, by querying a Record that was never set
            return missing_int;
        else
        {
            wreport::Varcode code = resolve_varcode(param + 1);
            for (const auto& var: qcoutput)
                if (var.code() == code)
                    return var.enqi();
            return missing_int;
        }
    }
    return record_enqi(output, param, missing_int);
}

signed char CommonAPIImplementation::enqb(const char* param)
{
    int value = missing_int;
    if (param[0] == '*')
    {
        wreport::Varcode code = resolve_varcode(param + 1);
        for (const auto& var: qcoutput)
            if (var.code() == code)
                value = var.enqi();
    } else {
        value = record_enqi(output, param, missing_int);
    }

    if (value == missing_int)
        return missing_byte;

    if (value < numeric_limits<signed char>::min()
            || value > numeric_limits<signed char>::max())
        error_consistency::throwf("value queried (%d) does not fit in a byte", value);
    return value;
}

float CommonAPIImplementation::enqr(const char* param)
{
    double value = missing_double;
    if (param[0] == '*')
    {
        wreport::Varcode code = resolve_varcode(param + 1);
        for (const auto& var: qcoutput)
            if (var.code() == code)
                value = var.enqd();
    } else {
        value = record_enqd(output, param, missing_double);
    }

    if (value == missing_double)
        return missing_float;

    if (value < -numeric_limits<float>::max()
            || value > numeric_limits<float>::max())
        error_consistency::throwf("value queried (%f) does not fit in a real", value);
    return value;
}

double CommonAPIImplementation::enqd(const char* param)
{
    if (param[0] == '*')
    {
        wreport::Varcode code = resolve_varcode(param + 1);
        for (const auto& var: qcoutput)
            if (var.code() == code)
                return var.enqd();
        return missing_double;
    }
    return record_enqd(output, param, missing_double);
}

std::string CommonAPIImplementation::enqc(const char* param)
{
    if (param[0] == '*')
    {
        wreport::Varcode code = resolve_varcode(param + 1);
        for (const auto& var: qcoutput)
            if (var.code() == code)
                return var.enqc();
        return std::string();
    }
    return record_enqs(output, param, std::string());
}

bool CommonAPIImplementation::enqc(const char* param, std::string& res)
{
    if (param[0] == '*')
    {
        wreport::Varcode code = resolve_varcode(param + 1);
        for (const auto& var: qcoutput)
            if (var.code() == code)
            {
                res = var.enqc();
                return true;
            }
        return false;
    }
    return record_enqsb(output, param, res);
}

void CommonAPIImplementation::seti(const char* param, int value)
{
    if (param[0] == '*')
    {
        if (strcmp(param + 1, "context_id") == 0)
        {
            delete operation;
            if (value == MISSING_INT)
                operation = nullptr;
            else
                operation = new VaridOperation(value);
        } else {
            qcinput.set(resolve_varcode(param + 1), value);
        }
        return;
    }
    record_seti(input, param, value);
}

void CommonAPIImplementation::setb(const char* param, signed char value)
{
    return seti(param, value);
}

void CommonAPIImplementation::setr(const char* param, float value)
{
    return setd(param, value);
}

void CommonAPIImplementation::setd(const char* param, double value)
{
    if (param[0] == '*')
    {
        qcinput.set(resolve_varcode(param + 1), value);
        return;
    }
    record_setd(input, param, value);
}

void CommonAPIImplementation::setc(const char* param, const char* value)
{
    if (param[0] == '*')
    {
        if (strcmp(param + 1, "var_related") == 0)
        {
            if (!operation)
                throw error_consistency("*var_related set without context_id, or before any dammelo or prendilo");
            operation->set_varcode(resolve_varcode(value));
        } else if (strcmp(param + 1, "var") == 0) {
            if (!operation)
                throw error_consistency("*var set without context_id, or before any dammelo or prendilo");
            std::vector<wreport::Varcode> varcodes { resolve_varcode(value + 1) };
            operation->select_attrs(varcodes);
        } else if (strcmp(param + 1, "varlist") == 0) {
            if (!operation)
                throw error_consistency("*varlist set without context_id, or before any dammelo or prendilo");
            std::vector<wreport::Varcode> varcodes;
            size_t pos = 0;
            while (true)
            {
                size_t len = strcspn(value + pos, ",");
                if (len == 0) break;

                if (*(value + pos) != '*')
                    throw error_consistency("QC value names must start with '*'");
                varcodes.push_back(resolve_varcode(value + pos + 1));

                if (!*(value + pos + len))
                    break;
                pos += len + 1;
            }
            operation->select_attrs(varcodes);
        } else {
            // Set varcode=value
            qcinput.set(resolve_varcode(param + 1), value);
        }
        return;
    }
    record_setc(input, param, value);
}

void CommonAPIImplementation::setcontextana()
{
    input.set_datetime(Datetime());
    input.set_level(Level());
    input.set_trange(Trange());
    station_context = true;
}

void CommonAPIImplementation::enqlevel(int& ltype1, int& l1, int& ltype2, int& l2)
{
    Level lev = output.get_level(); // TODO: access Record directly
    ltype1 = lev.ltype1 != MISSING_INT ? lev.ltype1 : API::missing_int;
    l1     = lev.l1     != MISSING_INT ? lev.l1     : API::missing_int;
    ltype2 = lev.ltype2 != MISSING_INT ? lev.ltype2 : API::missing_int;
    l2     = lev.l2     != MISSING_INT ? lev.l2     : API::missing_int;
}

void CommonAPIImplementation::setlevel(int ltype1, int l1, int ltype2, int l2)
{
    Level level(ltype1, l1, ltype2, l2);  // TODO: access Record directly
    if (!level.is_missing())
        station_context = false;
    input.set_level(level);
}

void CommonAPIImplementation::enqtimerange(int& ptype, int& p1, int& p2)
{
    Trange tr = output.get_trange(); // TODO: access Record directly
    ptype = tr.pind != MISSING_INT ? tr.pind : API::missing_int;
    p1    = tr.p1   != MISSING_INT ? tr.p1   : API::missing_int;
    p2    = tr.p2   != MISSING_INT ? tr.p2   : API::missing_int;
}

void CommonAPIImplementation::settimerange(int ptype, int p1, int p2)
{
    Trange trange(ptype, p1, p2);  // TODO: access Record directly
    if (!trange.is_missing())
        station_context = false;
    input.set_trange(trange);
}

void CommonAPIImplementation::enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec)
{
    Datetime dt = output.get_datetime();  // TODO: Access Record directly
    year = dt.year != 0xffff ? dt.year : API::missing_int;
    month = dt.month != 0xff ? dt.month : API::missing_int;
    day = dt.day != 0xff ? dt.day : API::missing_int;
    hour = dt.hour != 0xff ? dt.hour : API::missing_int;
    min = dt.minute != 0xff ? dt.minute : API::missing_int;
    sec = dt.second != 0xff ? dt.second : API::missing_int;
}

void CommonAPIImplementation::setdate(int year, int month, int day, int hour, int min, int sec)
{
    Datetime dt(year, month, day, hour, min, sec);
    if (!dt.is_missing())
        station_context = false;
    input.set_datetime(dt);
}

void CommonAPIImplementation::setdatemin(int year, int month, int day, int hour, int min, int sec)
{
    DatetimeRange dtr = input.get_datetimerange();
    dtr.min = Datetime(year, month, day, hour, min, sec);
    if (!dtr.min.is_missing())
        station_context = true;
    input.set_datetimerange(dtr);
}

void CommonAPIImplementation::setdatemax(int year, int month, int day, int hour, int min, int sec)
{
    DatetimeRange dtr = input.get_datetimerange();
    dtr.max = Datetime(year, month, day, hour, min, sec);
    if (!dtr.max.is_missing())
        station_context = true;
    input.set_datetimerange(dtr);
}

void CommonAPIImplementation::unset(const char* param)
{
    if (param[0] == '*')
    {
        if (strcmp(param + 1, "var_related") == 0)
        {
            if (!operation)
                throw error_consistency("*var_related set without context_id, or before any dammelo or prendilo");
            operation->set_varcode(0);
        } else if (strcmp(param + 1, "var") == 0) {
            if (!operation) return;
            operation->select_attrs(std::vector<wreport::Varcode>());
        } else if (strcmp(param + 1, "varlist") == 0) {
            if (!operation) return;
            operation->select_attrs(std::vector<wreport::Varcode>());
        } else {
            // Set varcode=value
            qcinput.erase(resolve_varcode(param + 1));
        }
        return;
    }
    record_unset(input, param);
}

void CommonAPIImplementation::unsetall()
{
    qcinput.clear();
    input.clear();
    if (operation)
    {
        operation->set_varcode(0);
        operation->select_attrs(std::vector<wreport::Varcode>());
    }
    station_context = false;
}

void CommonAPIImplementation::unsetb()
{
    qcinput.clear();
    input.clear_vars();
}

const char* CommonAPIImplementation::spiegal(int ltype1, int l1, int ltype2, int l2)
{
    cached_spiega = Level(ltype1, l1, ltype2, l2).describe();
    return cached_spiega.c_str();
}

const char* CommonAPIImplementation::spiegat(int ptype, int p1, int p2)
{
    cached_spiega = Trange(ptype, p1, p2).describe();
    return cached_spiega.c_str();
}

const char* CommonAPIImplementation::spiegab(const char* varcode, const char* value)
{
    Varinfo info = varinfo(WR_STRING_TO_VAR(varcode + 1));
    Var var(info, value);

    char buf[1024];
    switch (info->type)
    {
        case Vartype::String:
            snprintf(buf, 1023, "%s (%s) %s", var.enqc(), info->unit, info->desc);
            break;
        case Vartype::Binary:
            snprintf(buf, 1023, "%s (%s) %s", var.enqc(), info->unit, info->desc);
            break;
        case Vartype::Integer:
        case Vartype::Decimal:
            snprintf(buf, 1023, "%.*f (%s) %s", info->scale > 0 ? info->scale : 0, var.enqd(), info->unit, info->desc);
            break;
    }
    cached_spiega = buf;
    return cached_spiega.c_str();
}

const char* CommonAPIImplementation::ancora()
{
    static char parm[10] = "*";

    if (qc_iter < 0)
        throw error_consistency("ancora called without a previous voglioancora");
    if ((unsigned)qc_iter >= qcoutput.size())
        throw error_notfound("ancora called with no (or no more) results available");

    Varcode var = qcoutput[qc_iter].code();
    format_bcode(var, parm + 1);

    /* Get next value from qc */
    ++qc_iter;

    return parm;
}

void CommonAPIImplementation::fatto()
{
}

}
}
