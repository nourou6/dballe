#include "Benchmark.h"

#include <stdarg.h>
#include <unistd.h>

using namespace std;

static Benchmark* _root = 0;
Benchmark* Benchmark::root()
{
	if (!_root)
		_root = new Benchmark("");
	return _root;
}


double Benchmark::tps = sysconf(_SC_CLK_TCK);

dba_err Benchmark::timing(const char* fmt, ...)
{
    struct tms curtms;
    if (times(&curtms) == -1)
		return dba_error_system("reading timing informations");

	string fn = fullName();
	if (!fn.empty())
		fprintf(stdout, "%.*s: ", fn.size(), fn.data());

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);

    fprintf(stdout, ": %.2f user, %.2f system, %.2f total\n",
				    (curtms.tms_utime - lasttms.tms_utime)/tps,\
				    (curtms.tms_stime - lasttms.tms_stime)/tps,\
				    ((curtms.tms_utime - lasttms.tms_utime) + (curtms.tms_stime - lasttms.tms_stime))/tps); \

    if (times(&lasttms) == -1)
		return dba_error_system("reading timing informations");

	return dba_error_ok();
}

// Run only the subtest at the given path
dba_err Benchmark::run(const std::string& path)
{
	size_t split = path.find('/');
	if (split == std::string::npos)
	{
		for (std::vector<Benchmark*>::iterator i = children.begin();
				i != children.end(); i++)
			if ((*i)->name() == path)
				return (*i)->run();
	} else {
		std::string child(path, 0, split);
		std::string nextpath(path, split+1);

		for (std::vector<Benchmark*>::iterator i = children.begin();
				i != children.end(); i++)
			if ((*i)->name() == child)
				return (*i)->run(nextpath);
	}
	return dba_error_notfound("looking for child %s at %s", path.c_str(), fullName().c_str());
}

// Run all subtests and this test
dba_err Benchmark::run()
{
	if (times(&lasttms) == -1)
		return dba_error_system("reading timing informations");

	// First, run children
	if (! children.empty())
	{
		for (std::vector<Benchmark*>::iterator i = children.begin();
				i != children.end(); i++)
			DBA_RUN_OR_RETURN((*i)->run());
		DBA_RUN_OR_RETURN(timing("total"));
	}

	// Then, run self
	DBA_RUN_OR_RETURN(main());

	return dba_error_ok();
}
// vim:set ts=4 sw=4:
