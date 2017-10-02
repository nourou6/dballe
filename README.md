DB-All.e
===============================================================

Build status
------------

| Environment | Status |
| ----------- | ------ |
| CentOS 7    | [![Build Status](https://badges.herokuapp.com/travis/ARPA-SIMC/dballe?branch=travis&env=DOCKER_IMAGE=centos:7&label=fedora24)](https://travis-ci.org/ARPA-SIMC/dballe) |
| Fedora 25   | [![Build Status](https://badges.herokuapp.com/travis/ARPA-SIMC/dballe?branch=travis&env=DOCKER_IMAGE=fedora:25&label=fedora25)](https://travis-ci.org/ARPA-SIMC/dballe) |
| Fedora 26   | [![Build Status](https://badges.herokuapp.com/travis/ARPA-SIMC/dballe?branch=travis&env=DOCKER_IMAGE=fedora:26&label=fedora26)](https://travis-ci.org/ARPA-SIMC/dballe) |


Introduction
------------

DB-All.e is a fast on-disk database where meteorological observed and
forecast data can be stored, searched, retrieved and updated.

This framework allows to manage large amounts of data using its simple
Application Program Interface, and provides tools to visualise, import
and export in the standard formats BUFR, AOF and CREX.

The main characteristics of DB-ALL.e are:

 * Fortran, C, C++ and Python APIs are provided.
 * To make computation easier, data is stored as physical quantities,
   that is, as measures of a variable in a specific point of space and
   time, rather than as a sequence of report.
 * Internal representation is similar to BUFR and CREX WMO standard
   (table code driven) and utility for import and export are included
   (generic and ECMWF template).
 * Representation is in 7 dimensions: latitude and longitude geographic
   coordinates, table driven vertical coordinate, reference time,
   table driven observation and forecast specification, table driven
   data type.
 * It allows to store extra information linked to the data, such as
   confidence intervals for quality control.
 * It allows to store extra information linked to the stations.
 * Variables can be represented as real, integer and characters, with
   appropriate precision for the type of measured value.
 * It is based on physical principles, that is, the data it contains are
   defined in terms of homogeneous and consistent physical data. For
   example, it is impossible for two incompatible values to exist in the
   same point in space and time.
 * It can manage fixed stations and moving stations such as airplanes or
   ships.
 * It can manage both observational and forecast data.
 * It can manage data along all three dimensions in space, such as data
   from soundings and airplanes.
 * Report information is preserved. It can work based on physical
   parameters or on report types.
 * It is temporary, to be used for a limited time and then be deleted.
 * Does not need backup, since it only contains replicated or derived data.
 * Write access is enabled for its users.

Building DB-All.e
-----------------

DB-All.e is already packaged in both .rpm and .deb formats, and that provides
easy installation for most Linux distributions.

If you want to build and install DB-All.e yourself, you'll need to install the
automake/autoconf/libtool packages then you can proceed as in most other Unix 
software:

  autoreconf -if
  ./configure
  make
  make install


Getting started
---------------

DB-All.e requires a database to run. It can create a SQLite database, or access
a PostgreSQL or MySQL database. See doc/fapi_connect.md for details about
connecting to a database.

Once this is set up, you can initialise the DB-All.e database using the command::

  dbadb wipe --url=sqlite:dballe.sqlite3

If you do not already have access to datasets to import, some are available
from http://www.ncar.ucar.edu/tools/datasets/ after registering (for free) on
the website.


Documentation
-------------

Documentation for all commandline tools can be found in their manpages.  All
commandline tools also have extensive commandline help that can be accessed
using the "--help" option.

The Fortran API is documented in the fapi.pdf document.

The C API and all the C internals are documented through Doxygen.

Administration and maintanance of DB-All.e are covered in the guide.pdf
document.


Testing DB-All.e
----------------

Unit testing can be run using "make check", but it requires an existing DSN
connection to a MySQL database, which should be called 'test'.  Please note
that unit testing functions will wipe existing DB-All.e tables on the test DSN
database.


Useful resources
----------------

BUFR decoding:

 * http://www.knmi.nl/~meulenvd/code/bufr/dmtn1.html

AOF decoding:

 * Reading of Fortran "unformatted sequential" from C/C++:
   http://astronomy.swin.edu.au/~pbourke/dataformats/fortran/

ECWMF BUFR template codes:

 * http://www.ecmwf.int/research/ifsdocs/OBSERVATIONS/Chap2_Obs_types3.html


Contact and copyright information
---------------------------------

The author of DB-ALLe is Enrico Zini <enrico@enricozini.com>

DB-ALLe is Copyright (C) 2005-2016 ARPAE-SIMC <urpsim@arpae.it>

DB-ALLe is licensed under the terms of the GNU General Public License version
2.  Please see the file COPYING for details.

Contact informations for ARPAE-SIMC:

  Agenzia Regionale per la Prevenzione, l'Ambiente e l'Energia (ARPAE)
  Servizio Idro-Meteo-Climatologico (SIMC)

  Address: Viale Silvani 6, 40122 Bologna, Italy
  Tel: + 39 051 6497511
  Fax: + 39 051 6497501
  Email: urpsim@arpae.it
  Website: http://www.arpae.it/sim/
