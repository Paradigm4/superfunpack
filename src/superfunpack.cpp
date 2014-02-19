#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "pcrs.h"

#include <vector>
#include <boost/assign.hpp>

#include "query/Operator.h"
#include "query/FunctionLibrary.h"
#include "query/FunctionDescription.h"
#include "query/TypeSystem.h"
#include "system/ErrorsLibrary.h"

using namespace std;
using namespace scidb;
using namespace boost::assign;

/** @file superfunpack.cpp
 *
 * - An example trivial string to integer hash that supports only up to 7-byte
 *   strings.
 * - A flexible time parser and formatter to/from all valid strptime/strftime
 *   conversions.
 * - A perl-style regular expression substitute.
 *   <br> <br>
 *
 * @brief The functions: hashish() and hsihsah() and strpftime.
 *
 * @par Synopsis: hashish (string) / hsihsah (int64), strpftime (string data,
 * string input_format, string output_format)
 *
 * @par Examples:
 * <br>
 * apply(apply(build(<s:string>[i=0:9,10,0],i),hash,hashish(s)),hsah,
 * hsihsah(hash))
 *
 * apply(build(<s:string>[i=0:0,1,0],'{0}[(03-Mar-2012)]',true),x,strpftime(s, '%d-%m-%Y', '%U'))
 *
 * @par Summary:
 * <br>
 *
 * Convert a string to a 64-bit integer or a 64-bit integer
 * back to a string.  Only up to the first seven characters of the string are
 * uniquely hashed (this is a silly hash).
 **/

static void
string2l(const Value** args, Value *res, void*)
{
  int64_t l;
  char buf[8];
  std::string s = args[0]->getString();
  memset(buf,0,8);
  snprintf(buf,8,s.c_str());
  memcpy(&l, buf, 8);
  res->setInt64(l);
}

static void
l2string(const Value** args, Value *res, void*)
{
  char buf[8];
  int64_t l = (int64_t)args[0]->getInt64();
  memcpy(buf, &l, 8);
  res->setString(buf);
}


static void
pcrsgsub(const Value** args, Value *res, void*)
{
   pcrs_job *job;
   const char *e;
   char *s;
   char *result;
   size_t length;
   int err;

   std::string data = args[0]->getString();
   std::string expr = args[1]->getString();
   s = (char *)data.c_str();
   e = (const char *)expr.c_str();
 
   if (NULL == (job = pcrs_compile_command(e, &err)))
   {
     throw PLUGIN_USER_EXCEPTION("superfunpack", SCIDB_SE_UDO, SCIDB_USER_ERROR_CODE_START);
   }
   length = strlen(data.c_str());
   err = pcrs_execute(job, s, length, &result, &length);
   res->setString(result);
   free(result);
   pcrs_free_job(job);
}

/* 
 * pfconvert (string data, string informat, string outformat) -> string
 * Parse the data string into a time value via strptime format specified
 * in the informat argument. Then convert the time value into a formatted
 * string via the outformat argument.
 */
static void
pfconvert(const Value** args, Value *res, void*)
{
  struct tm tm;
  char buf[255];
  std::string data = args[0]->getString();
  std::string informat= args[1]->getString();
  std::string outformat = args[2]->getString();
  memset(&tm, 0, sizeof(struct tm));
  strptime(data.c_str(), informat.c_str(), &tm);
  strftime(buf, sizeof(buf), outformat.c_str(), &tm);
  res->setString(buf);
}

REGISTER_FUNCTION(strpftime, list_of("string")("string")("string"), "string", pfconvert);
REGISTER_FUNCTION(rsub, list_of("string")("string"), "string", pcrsgsub);
REGISTER_FUNCTION(hashish, list_of("string"), "int64", string2l);
REGISTER_FUNCTION(hsihsah, list_of("int64"), "string", l2string);


// general class for registering/unregistering user defined SciDB objects
static class superfunpack
{
public:
  superfunpack()
  {
    _errors[SCIDB_USER_ERROR_CODE_START] = "Duuuude. Your regular expression failed to compile.";
    scidb::ErrorsLibrary::getInstance()->registerErrors("superfunpack", &_errors);
  }

  ~superfunpack()
  {
    scidb::ErrorsLibrary::getInstance()->unregisterErrors("superfunpack");
  }

private:
    scidb::ErrorsLibrary::ErrorsMessages _errors;
} _instance;
