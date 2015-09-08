#include "memdb/tests.h"
#include "index.h"

using namespace dballe;
using namespace dballe::memdb;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("empty", []() {
        });
    }
} test("memdb_index");

}
