/*
**
* BEGIN_COPYRIGHT
*
* This file is part of SciDB.  Copyright (C) 2008-2014 SciDB, Inc.
*
* Superfunpack is free software: you can redistribute it and/or modify it under
* the terms of the GNU General Public License version 2 as published by the
* Free Software Foundation.
*
* SciDB is distributed "AS-IS" AND WITHOUT ANY WARRANTY OF ANY KIND, INCLUDING
* ANY IMPLIED WARRANTY OF MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR A
* PARTICULAR PURPOSE. See the GNU General Public License version 2 for the
* complete license terms.
*
* END_COPYRIGHT
*/
#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#include <vector>

#include <boost/assign.hpp>
#include <boost/math/distributions/hypergeometric.hpp>
#include <boost/math/tools/roots.hpp>

#include "query/FunctionLibrary.h"
#include "query/FunctionDescription.h"
#include "system/ErrorsLibrary.h"

#include "pcrs.h"

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
 * - Sleep, per chance to dream.
 *   <br> <br>
 *
 * @brief The functions: hashish and hsihsah,  strpftime and sleep.
 *
 * @par Synopsis: hashish (string) / hsihsah (int64), strpftime (string data,
 * string input_format, string output_format), sleep(uint32 time)
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

using boost::math::hypergeometric_distribution;

static void
string2l(const Value** args, Value *res, void*)
{
  int64_t l;
  char buf[8];
  std::string s = args[0]->getString();
  memset(buf,0,8);
  snprintf(buf,8,"%s",s.c_str());
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
dream(const Value** args, Value *res, void*)
{
  uint32_t l = (uint32_t)args[0]->getUint32();
  res->setUint32(sleep(l));
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
 * @brief  Parse the data string into a time value via strptime format
 *         specified in the informat argument. Then convert the time
 *         value into a formatted string via the outformat argument.
 * @param data (string) input data
 * @param informat (string) input data format, see strptime for details
 * @param outformat (string) output format, see strftime for defails
 * @returns string
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


/* ***************************************************************************
 *       Hypergeometric stuff in support of Fisher's exact test 
 * ***************************************************************************
 */
class mnhyper
{
  double m, n, k, x;
  bool invert;
  bool dnhyper;

  public:
    mnhyper(double,double,double,double);
    void inv(bool);
    void dn(bool);
    double operator()(double ncp)
    {
      int j, ns;
      double logncp, lo, hi, maxd, sumd, sum;
      if(invert && ncp!=0) ncp = 1/ncp;
      hypergeometric_distribution <> h(m, k, m+n);
      lo = round(max(0.0, k-n));
      hi = round(min(k, m));
      ns = hi - lo + 1;
      if(ns<=0) return 0;
      double relerr = 1.0000001;
      double dj, xminuslo = 0;
      vector<double> support(ns);
      vector<double> logdc(ns);
      vector<double> d(ns);
      logncp    = log(ncp);
      maxd    = -INFINITY;
      sumd    = sum = 0;
      xminuslo = x - lo;
      for(j=0;j<ns;++j)
      {
        support[j] = lo + j;
        logdc[j]   = log(boost::math::pdf(h,support[j]));
        d[j]       = logdc[j] + logncp*support[j];
        if(d[j] > maxd) maxd = d[j];
      }
      for(j=0;j<ns;++j)
      {
        d[j] = exp(d[j] - maxd);
        sumd = sumd + d[j];
      }
      if(dnhyper)
      {
        xminuslo = relerr*d[x - lo]/sumd;
      }
      for(j=0;j<ns;++j)
      {
        if(dnhyper)
        {
          dj = d[j]/sumd;
          if(dj < xminuslo) sum = sum + dj;
        }
        else        sum  = sum + support[j]*d[j]/sumd;
      }
      if(!dnhyper) sum = sum - x;
      return sum;
    }
};
mnhyper::mnhyper(double M, double N, double K, double X)
{
  m = M;
  n = N;
  k = K;
  x = X;
  invert = false;
  dnhyper = false;
}
void
mnhyper::inv(bool b)
{
  invert = b;
}
void
mnhyper::dn(bool b)
{
  dnhyper = true;
}

double
hyper_mle(double x, double m, double n, double k)
{
  hypergeometric_distribution <> h(m, k, m+n);
  mnhyper f(m,n,k,x);
  typedef std::pair<double, double> Result;
  boost::uintmax_t max_iter=500;
  boost::math::tools::eps_tolerance<double> tol(30);

  double mu = f(1) + x;

  double root;
  Result bracket;
  if(mu>x)
  {
    bracket = boost::math::tools::toms748_solve(f, 0.0000001, 1.0, tol, max_iter);
    root    = bracket.first;
    if((f(bracket.first) * f(bracket.second)) >0) root = NAN;
  } else if(mu<x)
  {
    f.inv(true);
    bracket = boost::math::tools::toms748_solve(f, 0.0000001, 1.0, tol, max_iter);
    root    = 1/bracket.first;
    if((f(bracket.first) * f(bracket.second)) >0) root = NAN;
  } else
  {
    root = 1;
  }
  return root;
}

/*
 * @brief Fisher exact test conditional odds ratio
 * @param x (double) The number of white balls drawn without replacement
 *           from an urn that contains both black and white balls.
 * @param m (double) The number of white balls in the urn.
 * @param n (double) The number of black balls in the urn.
 * @param k (double) The number of balls drawn from the urn. 
 * @returns The conditional odds ratio for the one-tailed Fisher exact test.
 */
static void
superfun_conditional_odds_ratio(const Value** args, Value *res, void*)
{
  double x = args[0]->getDouble();
  double m = args[1]->getDouble();
  double n = args[2]->getDouble();
  double k = args[3]->getDouble();
  res->setDouble(hyper_mle(x, m, n, k));
}

/*
 * @brief Fisher exact test p-value
 * @param x (double) The number of white balls drawn without replacement
 *           from an urn that contains both black and white balls.
 * @param m (double) The number of white balls in the urn.
 * @param n (double) The number of black balls in the urn.
 * @param k (double) The number of balls drawn from the urn. 
 * @param alternative (string) one of {"less","greater","two.sided"}
 * @returns The conditional odds ratio for the one-tailed Fisher exact test.
 */
static void
superfun_fisher_p_value(const Value** args, Value *res, void*)
{
  double x = args[0]->getDouble();
  double m = args[1]->getDouble();
  double n = args[2]->getDouble();
  double k = args[3]->getDouble();
  string a = args[4]->getString();
  hypergeometric_distribution <> h(m, k, m+n);
  if(a == "less")
  {
    res->setDouble(boost::math::cdf(h, x));
    return;
  }
  if(a == "greater")
  {
    res->setDouble(boost::math::cdf(complement(h, x - 1)));
    return;
  }
// Default to two.sided
  mnhyper f(m,n,k,x);
  f.dn(true);
  res->setDouble(f(1));
}

/*
 * @brief Hypergeometric probability density function
 * @param x (double) The number of white balls drawn without replacement
 *           from an urn that contains both black and white balls.
 * @param m (double) The number of white balls in the urn.
 * @param n (double) The number of black balls in the urn.
 * @param k (double) The number of balls drawn from the urn. 
 * @returns The hypergeometric density at x.
 */
static void
superfun_dhyper(const Value** args, Value *res, void*)
{
  double x = args[0]->getDouble();
  double m = args[1]->getDouble();
  double n = args[2]->getDouble();
  double k = args[3]->getDouble();
  hypergeometric_distribution <> h(m, k, m+n);
  res->setDouble(boost::math::pdf(h, x));
}

/*
 * @brief hypergeometric cumulative distribution
 * @param x (double) The number of white balls drawn without replacement
 *           from an urn that contains both black and white balls.
 * @param m (double) The number of white balls in the urn.
 * @param n (double) The number of black balls in the urn.
 * @param k (double) The number of balls drawn from the urn. 
 * @param lower_tail (boolean) TRUE for lower tail quantile, FALSE for upper.
 * @returns The hypergeometric cumulative distribution up to x
 */
static void
superfun_phyper(const Value** args, Value *res, void*)
{
  double x = args[0]->getDouble();
  double m = args[1]->getDouble();
  double n = args[2]->getDouble();
  double k = args[3]->getDouble();
  bool lower_tail = args[4]->getBool();
  hypergeometric_distribution <> h(m, k, m+n);
  if(lower_tail)
  {
    res->setDouble(boost::math::cdf(h, x));
    return;
  }
  res->setDouble(boost::math::cdf(complement(h, x)));
}

/*
 * @brief hypergeometric quantile function
 * @param p (double) The probability (0 <= p <= 1)
 * @param m (double) The number of white balls in the urn.
 * @param n (double) The number of black balls in the urn.
 * @param k (double) The number of balls drawn from the urn. 
 * @param lower_tail (boolean) TRUE for lowe tail quantile, FALSE for upper.
 * @returns The number of white balls drawn without replacement
 *          from an urn that contains both black and white balls.
 */
static void
superfun_qhyper(const Value** args, Value *res, void*)
{
  double p = args[0]->getDouble();
  double m = args[1]->getDouble();
  double n = args[2]->getDouble();
  double k = args[3]->getDouble();
  bool lower_tail = args[4]->getBool();
  hypergeometric_distribution <> h(m, k, m+n);
  if(lower_tail)
  {
    res->setDouble(boost::math::quantile(h, p));
    return;
  }
  res->setDouble(boost::math::quantile(complement(h, p)));
}


REGISTER_FUNCTION(strpftime, list_of("string")("string")("string"), "string", pfconvert);
REGISTER_FUNCTION(rsub, list_of("string")("string"), "string", pcrsgsub);
REGISTER_FUNCTION(hashish, list_of("string"), "int64", string2l);
REGISTER_FUNCTION(hsihsah, list_of("int64"), "string", l2string);
REGISTER_FUNCTION(sleep, list_of("uint32"), "uint32", dream);
REGISTER_FUNCTION(dhyper, list_of("double")("double")("double")("double"), "double", superfun_dhyper);
REGISTER_FUNCTION(phyper, list_of("double")("double")("double")("double")("bool"), "double", superfun_phyper);
REGISTER_FUNCTION(qhyper, list_of("double")("double")("double")("double")("bool"), "double", superfun_qhyper);
REGISTER_FUNCTION(fisher_test_odds_ratio, list_of("double")("double")("double")("double"), "double", superfun_conditional_odds_ratio);
REGISTER_FUNCTION(fisher_test_p_value, list_of("double")("double")("double")("double")("string"), "double", superfun_fisher_p_value);

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
