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

/*  The *hyper stats functions below are adapted from the R source code:
 *
 *  R : A Computer Language for Statistical Data Analysis
 *  Copyright (C) 1995, 1996  Robert Gentleman and Ross Ihaka
 *  Copyright (C) 1997--2005  The R Core Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
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
#include "R/fun.h"

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
 * @brief The functions: dumb_hash, dumb_unhash,  strpftime and sleep.
 *
 * @par Synopsis: dumb_hash (string) /dumb_unhash (int64), strpftime (string data,
 * string input_format, string output_format), sleep(uint32 time)
 *
 * @par Examples:
 * <br>
 * apply(apply(build(<s:string>[i=0:9,10,0],i), hash, dumb_hash(s)), hsah,
 * dumb_unhash(hash))
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
  char buf[9];
  memset(buf,0,9);
  snprintf(buf,9,"%s",args[0]->getString()); // <<-- problem ?? !!
  memcpy((void *)&l, (void *)buf, 8);
  res->setInt64(0);
}

static void
l2string(const Value** args, Value *res, void*)
{
  char buf[9];
  memset(buf,0,9);
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
   if(err<0)
   {
     throw PLUGIN_USER_EXCEPTION("superfunpack", SCIDB_SE_UDO, SCIDB_USER_ERROR_CODE_START);
   }
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
  char *data = (char *)args[0]->getString();
  char *informat= (char *)args[1]->getString();
  char *outformat = (char *)args[2]->getString();

  memset(&tm, 0, sizeof(struct tm));
  strptime(data, informat, &tm);

/* It turns out that many implementations of strptime are buggy and have
 * problems with daylight savings time in the locale. We correc for this
 * potential issue by explicitly setting the dst field to 'don't know'
 * and asking mktime to handle it, then reconstruct the time with
 * localtime_r. This approach seems to solve most of the bugs in strptime,
 * and is a lot simpler than, for example, R--which completely replaces
 * strptime with its own implementation!
 */
  tm.tm_isdst = -1;
  time_t t = mktime(&tm);
  memset(&tm, 0, sizeof(struct tm));
  localtime_r(&t, &tm);

  strftime(buf, sizeof(buf), outformat, &tm);
  res->setString(buf);
}


/* ***************************************************************************
 *       Hypergeometric stuff in support of Fisher's exact test 
 *
 * Adapted from the R source code, Copyrigth 1998-2014 R Foundataion.
 * The changes are mainly to use the boost distribution functions and the
 * available boost root-finding method.
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
//      hypergeometric_distribution <> h(m, k, m+n);
      lo = round(max(0.0, k-n));
      hi = round(min(k, m));
      ns = hi - lo + 1;
      if(ns<=0) return 0;
      double relerr = 1.000000001;
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
// Not accurate:
//        logdc[j]   = log(boost::math::pdf(h,support[j]));
        logdc[j]   = dhyper (support[j], m, n, k, 1);
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
//  hypergeometric_distribution <> h(m, k, m+n);
  mnhyper f(m,n,k,x);
  typedef std::pair<double, double> Result;
  boost::uintmax_t max_iter=5000;
  boost::math::tools::eps_tolerance<double> tol(52);

// Check exceptional cases
// x  u
// y  v
  double y = m - x;
  double u = k - x;
  double v = n - u;
  if(x==0 || v==0) return 0;
  if(u==0 || y==0) return INFINITY;

  double mu = f(1);

  double root;
  Result bracket;
  if(mu>x)
  {
    try
    {
      bracket = boost::math::tools::toms748_solve(f, 0.00000001, 1.0, tol, max_iter);
    } catch(...)
    {
      return 0;
    }
    root    = bracket.first;
    if((f(bracket.first) * f(bracket.second)) >0) root = NAN;
  } else if(mu<x)
  {
    f.inv(true);
    try
    {
      bracket = boost::math::tools::toms748_solve(f, 0.00000001, 1.0, tol, max_iter);
    } catch(...)
    {
      return 0;
    }
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
 * @returns The Fisher exact test p-value.
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


/* A support function for the user-visible 'book' function defined
 * below. This function parses the specially-formatted book string defined
 * in the book function below, placing its values into the supplied bid and
 * ask map pointers.
 */
void
parse_book(string X,
           std::map<double,double> *bid,
           std::map<double,double> *ask)
{
  char *comma, *pipe, *comma_saveptr, *pipe_saveptr, *endptr;
  double d, price=NAN;
  int j;
  char *s  = strdup(X.c_str());

  pipe = strtok_r(s, "|", &pipe_saveptr);
  if(!pipe) return;
/* Process the bid values */
  comma = strtok_r(pipe, ",", &comma_saveptr);
  j = 0;
  while(comma)
  {
    d = strtod(comma, &endptr);
    switch(j)
    {
      case 0:
        if(comma != endptr)
        {
          price = d; // cache this price value
        }
        else
        {
          price = NAN;
        }
        break;
      case 1:
        if(comma != endptr)  // add a price/size entry to the book
        {
          if(bid->count(price) > 0)  // entry already on the book, sum
          {
            (*bid)[price] = (*bid)[price] + d;
          } else   // add a new entry
          {
            (*bid)[price] = d;
          }
        }
        break;
    }
    j = (j + 1) % 2;
    comma = strtok_r(NULL, ",", &comma_saveptr);
  }

/* Process the ask values */
  pipe = strtok_r(NULL, "|", &pipe_saveptr);
  if(!pipe) return;
  comma = strtok_r(pipe, ",", &comma_saveptr);
  j = 0;
  while(comma)
  {
    d = strtod(comma, &endptr);
    switch(j)
    {
      case 0:
        if(comma != endptr)
        {
          price = d; // cache this price value
        }
        else
        {
          price = NAN;
        }
        break;
      case 1:
        if(comma != endptr)  // add a price/size entry to the book
        {
          if(ask->count(price) > 0)  // entry already on the book, sum
          {
            (*ask)[price] = (*ask)[price] + d;
          } else   // add a new entry
          {
            (*ask)[price] = d;
          }
        }
        break;
    }
    j = (j + 1) % 2;
    comma = strtok_r(NULL, ",", &comma_saveptr);
  }
}

/* Consolidate two financial market order book strings. This assumes a
 * very special formatting of the strings:
 *
 * bid_price_1, bid_size_1, bid_price_2, bid_size_2, ..., bid_price_m, bid_size_m |
 * ask_price_1, ask_size_1, ask_price_2, ask_size_2, ..., ask_price_n, ask_size_n
 *
 * where the pipe symbol is used to separate the bid and ask sides and they can
 * each have different numbers of entries that are comma-delimited.
 */
/*
 * @brief Consolidate two financial market order book strings. This assumes a
 * very special formatting of each string:
 *
 * bid_price_1, bid_size_1, bid_price_2, bid_size_2, ..., bid_price_m, bid_size_m |
 * ask_price_1, ask_size_1, ask_price_2, ask_size_2, ..., ask_price_n, ask_size_n
 *
 * (note that bid and ask sides are separated by a vertical pipe character).
 * @param x (string) A string order book representation.
 * @param y (string) A string order book representation.
 * @param depth (uint32) The maximum output book depth
 * @returns A string representation of the consolidated book, limited to at
 * most the indicated depth.
 */
static void
book(const Value **args, Value *res, void*)
{
  char buf[128];
  uint32_t j;
  int k;
  string X = (string) args[0]->getString();
  string Y = (string) args[1]->getString();
  uint32_t depth = (uint32_t)args[2]->getUint32();

  std::map<double, double> bid;
  std::map<double, double> ask;

  parse_book(X, &bid, &ask);
  parse_book(Y, &bid, &ask);

  string ans  = "";
  std::map<double,double>::iterator iter;

/* Write out the consolidated bid prices in order up to indicated depth */
  memset(buf, 0, 128);
  j = 0;
  k = 0;
  for(iter = bid.begin(); iter != bid.end(); iter++)
  {
    if(bid.size() - j > depth)
    {
      ++j;
      continue;
    }
    if(k==0)
    {
      snprintf(buf, 128, "%.3f, %.0f", iter->first, iter->second);
      k = 1;
    }
    else     snprintf(buf, 128, ", %.3f, %.0f", iter->first, iter->second);
    ans = ans + buf;
    ++j;
  }
  ans = ans + " | ";

/* Write out the consolidated ask prices in order, up to indicated depth */
  memset(buf, 0, 128);
  j = 0;
  for(iter = ask.begin(); iter != ask.end(); iter++)
  {
    if(j==0) snprintf(buf, 128, "%.3f, %.0f", iter->first, iter->second);
    else     snprintf(buf, 128, ", %.3f, %.0f", iter->first, iter->second);
    ans = ans + buf;
    ++j;
    if(j >= depth) break;
  }
  res->setString(ans.c_str());
}


REGISTER_FUNCTION(book, list_of("string")("string")("uint32"), "string", book);
REGISTER_FUNCTION(strpftime, list_of("string")("string")("string"), "string", pfconvert);
REGISTER_FUNCTION(rsub, list_of("string")("string"), "string", pcrsgsub);
REGISTER_FUNCTION(dumb_hash, list_of("string"), "int64", string2l);
REGISTER_FUNCTION(dumb_unhash, list_of("int64"), "string", l2string);
REGISTER_FUNCTION(sleep, list_of("uint32"), "uint32", dream);
REGISTER_FUNCTION(dhyper, list_of("double")("double")("double")("double"), "double", superfun_dhyper);
REGISTER_FUNCTION(phyper, list_of("double")("double")("double")("double")("bool"), "double", superfun_phyper);
REGISTER_FUNCTION(qhyper, list_of("double")("double")("double")("double")("bool"), "double", superfun_qhyper);
REGISTER_FUNCTION(fishertest_odds_ratio, list_of("double")("double")("double")("double"), "double", superfun_conditional_odds_ratio);
REGISTER_FUNCTION(fishertest_p_value, list_of("double")("double")("double")("double")("string"), "double", superfun_fisher_p_value);

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
