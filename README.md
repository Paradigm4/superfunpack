#  superfunpack

Miscellaneous scalar functions for SciDB that are super fun.

# Superfunpack functions

## Really trivial hash

The `dumb_hash(string)` function converts up to the first seven bytes of
its string argument to a signed int64 value. Characters past the seventh
are ignored in the string and not hashed.

The `dumb_unhash(int64)` function takes a signed int64 value and returns
a string.

Here is an example:
```
iquery -aq "
  apply(
    apply(
      build(<s:string>[i=0:9,10,0],i),
      hash, dumb_hash(s)),
    hsah, dumb_unhash(hash))"
```

Another:
```
iquery -aq "apply(build(<s:string>[i=1:1,1,0],'{1}[(AAPL)]',true),hash,dumb_hash(s))"

{i} s,hash
{1} 'AAPL',1280328001
```


## fisher\_test\_odds\_ratio and fisher\_test\_p\_value

See http://paradigm4.github.io/SciDBR/fisher.html  for the R package support
for this function.

Estimate the conditional odds ratio or p value Fisher's exact test for testing
the null of independence of rows and columns in a 2x2 contingency table with
fixed marginals.  Use these functions together to conduct Fisher's exact tests
on 2x2 contingency tables. 

From the R documentation of fisher.test:

For 2 by 2 tables, the null of conditional independence is equivalent to the
hypothesis that the odds ratio equals one.  "Exact" inference can be based on
observing that in general, given all marginal totals fixed, the first element
of the contingency table has a non-central hypergeometric distribution with
non-centrality parameter given by the odds ratio (Fisher, 1935).  The
alternative hypothesis for a one-sided test is based on the odds ratio being
smaller than the estimated value.


### Synopsis

```
double fisher_test_odds_ratio (double x, double m, double n, double k)
double fisher_test_p_value (double x, double m, double n, double k, string alternative)
```
> * x: Number of 'yes' events in both classifications (see table below)
> * m: Marginal sum of the 1st column ('yes' events in 1st class)
> * n: Marginal sum of  the 2nd column ('no' events in 1st class)
> * k: Marginal sum of the 1st row ('yes' events in 2nd class)
> * alternative: indicates the alternative hypothesis and must be one of "two.sided", "less", or "greater"

The following table illustrates the parameters x, m, n, and k in
a contingency table comparing two classifications labeled I and II:

|                  | Class I YES   | Class I NO  | SUM       |
| ---------------- | :-----------: | :---------: | --------: | 
| **Class II YES** | x             | a           | k = x + a |
| **Class II NO**  | b             | c           |           |
| **SUM**          | m = x + b     | n = a + c   |           |


### Examples

Consider the following three examples computed by R:
```R
TeaTasting <- matrix(c(3, 1, 1, 3), nrow = 2)

# The data look like:
TeaTasting
```
```
     [,1] [,2]
[1,]    3    1
[2,]    1    3
```
OK, let's run a Fisher exact test:
```R
fisher.test(TeaTasting,alternative="less")

      Fishers Exact Test for Count Data

data:  TeaTasting
p-value = 0.9857
alternative hypothesis: true odds ratio is less than 1
95 percent confidence interval:
   0.0000 306.2469
sample estimates:
odds ratio 
  6.408309 
```
Here is a second example, also taken from the R documentation:
```R
Convictions <- matrix(c(2, 10, 15, 3), nrow = 2)

# These data look like:
Convictions
```
```
     [,1] [,2]
[1,]    2   15
[2,]   10    3
```
Compute a one-sided exact test:
```R
fisher.test(Convictions, alternative="less")
```
```
      Fisher's Exact Test for Count Data

data:  Convictions
p-value = 0.0004652
alternative hypothesis: true odds ratio is less than 1
95 percent confidence interval:
 0.0000000 0.2849601
sample estimates:
odds ratio 
0.04693661 
```
Let's compute a two-sided test on the same convictions table data:
```R
fisher.test(Convictions, alternative="two.sided")
```
```
        Fisher's Exact Test for Count Data

data:  Convictions
p-value = 0.0005367
alternative hypothesis: true odds ratio is not equal to 1
95 percent confidence interval:
 0.003325764 0.363182271
sample estimates:
odds ratio 
0.04693661 
```

Let's compute the same tests in SciDB. First we use the table above to compute
the x, m, n, and k values for each example. For the TeaTasting data, we have
x=3, m = n = k = 4. And for the Convictions table, we get x=2, m=12, n=18, k=17.

TeaTasting table result:
```
apply(
  apply(build(<x:int64>[i=0:0,1,0],3),m,4,n,4,k,4),
  pvalue, fisher_test_p_value(x,m,n,k,'less'),
  odds_ratio_estimate, fisher_test_odds_ratio(x,m,n,k)
)
{i} x, m, n, k, pvalue,   odds_ratio_estimate
{0} 3, 4, 4, 4, 0.985714, 6.40832
```

One-sided test convictions table result:
```
apply(
  apply(build(<x:int64>[i=0:0,1,0],2),m,12,n,18,k,17),
  pvalue, fisher_test_p_value(x,m,n,k,'less'),
  odds_ratio_estimate, fisher_test_odds_ratio(x,m,n,k)
)

{i} x, m,  n,  k,  pvalue,      odds_ratio_estimate
{0} 2, 12, 18, 17, 0.000465181, 0.0469366
```

Two-sided test convictions table result:
```
apply(
  apply(build(<x:int64>[i=0:0,1,0],2),m,12,n,18,k,17),
  pvalue, fisher_test_p_value(x,m,n,k,'two.sided'),
  odds_ratio_estimate, fisher_test_odds_ratio(x,m,n,k)
)
{i} x, m,  n,  k,  pvalue,      odds_ratio_estimate
{0} 2, 12, 18, 17, 0.000536724, 0.0469366
```

In practice SciDB can compute Fisher's exact test across many contingency tables in one
step using combinations of aggregate and apply.

### References

1.     R Core Team (2013). R: A language and environment for statistical
     computing. R Foundation for Statistical Computing, Vienna, Austria.
     URL http://www.R-project.org/.
2. http://www.boost.org/doc/libs/1_55_0/libs/math/doc/html/dist.html
3. http://www.boost.org/doc/libs/1_55_0/libs/math/doc/html/math_toolkit/internals1/roots2.html
4.     Fisher, R. A. (1935) The logic of inductive inference.  _Journal of the Royal Statistical Society Series A_ *98*, 39-54.


## phyper, dhyper, and qhyper

The hypergeometric distribution is used for sampling *without*
replacement.  The density of this distribution with parameters
m, n and k is:

>       p(x) =   choose(m, x) * choose(n, k-x) / choose(m+n, k)      
>       for x = 0, ..., k.  

The p-quantile is defined as the smallest value x such that F(x) >= p, where F
is the cumulative distribution function and 0 <= p <= 1.

Use the phyper function together with the fisher_test_odds_ratio function above
to compute Fishers exact tests on 2x2 contingency tables.

### Synopsis

```
double dhyper(x, m, n, k)
double phyper(x, m, n, k, lower_tail)
double qhyper(p, m, n, k, lower_tail)
```
where,

> * x: the number of white balls drawn without replacement from an urn which contains both black and white balls.
> * p: probability, it must be between 0 and 1.
> * m: the number of white balls in the urn.
> * m: the number of black balls in the urn.
> * k: the number of balls drawn from the urn.
> * lower_tail: (boolean) If true, return the lower tail value, otherwise the upper tail value.


### Details

dhyper gives the cumulative density function, phyper gives the distribution
function, and qhyper gives the quantile function.

### Examples

```
apply(
  apply(build(<x:int64>[i=0:0,1,0],3),m,10,n,7,k,8),
  pvalue, phyper(x,m,n,k,true),
  cdf, dhyper(x,m,n,k)
)

{i} x, m,  n, k, pvalue,            cdf
{0} 3, 10, 7, 8, 0.117030028794735, 0.103661044837515



apply(
  apply(build(<q:double>[i=0:0,1,0],0.117030028794735),m,10,n,7,k,8),
  quantile, qhyper(q,m,n,k,true)
)

{i} q,       m,  n, k, quantile
{0} 0.11703, 10, 7, 8, 3
```


## strpftime

The strpftime function is a flexible date/time string parsing and conversion
tool.

### Synopsis

```
string strpftime (input_string, input_format, output_format)
```
> * input_string: An input string value representing date and/or time (usually a SciDB string attribute).
> * input_format: A strptime-valid format describing the input string.
> * output_format: A strftime-valid output format.

### Description

The strpftime function parses and converts dates and times from one string
format representation into another. It's useful to pick out portions of a date
or time, for example, to pick out the week of the year, or the weekday name, or
the name of the month, etc. It can understand any str[pf]time time format.


#### Example 1: Pick out the week of the year.

```
iquery -aq "apply(build(<s:string>[i=0:0,1,0],'{0}[(03-Mar-2012)]',true), woy , int64(strpftime(s, '%d-%m-%Y', '%U')))"
{i} s,woy
{0} '03-Mar-2012',1
```

#### Example 2: Print the full weekday name of a date in your locale.

```
iquery -aq "apply(build(<s:string>[i=0:0,1,0],'{0}[(03-Mar-2012)]',true), day ,strpftime(s, '%d-%m-%Y', '%A'))"
{i} s,day
{0} '03-Mar-2012','Sunday'
```



## rsub

Perl-style regular expression substring replacement.

### Synopsis

```
string rsub (input_string, replacement_expression)
```

> * input_string: A string value (usually a SciDB attribute)
> * replacement_expression: A perl-like replacement regular expression.

### Description

Search for and replace substrings.


#### Example: Replace the first occurrence of a substring

```
iquery -aq "apply(build(<s:string>[i=0:0,1,0],'{0}[(\'Paul Brown is a serious serious man.\')]',true), r, rsub(s,'s/serious/silly/'))"
{i} s,r
{0} 'Paul Brown is a serious serious man.','Paul Brown is a silly serious man.'
```


#### Example: Replace all occurrences of a substring

```
iquery -aq "apply(build(<s:string>[i=0:0,1,0],'{0}[(\'Paul Brown is a serious serious man.\')]',true), r, rsub(s,'s/serious/silly/g'))"
{i} s,r
{0} 'Paul Brown is a serious serious man.','Paul Brown is a silly silly man.'
```

#### Example: Search and replace characters by hexadecimal code
We search for hex code 61, which corresponds to the 'a' character, replacing
all occurences with 'o':
```
iquery -aq "apply(build(<s:string>[i=0:0,1,0],'{0}[(\'The cat in the hat.\')]',true), r, rsub(s,'s/\x61/o/g'))"
{i} s,r
{0} 'The cat in the hat.','The cot in the hot.'
```

#### Example: Backreferences
```
iquery -aq "apply(build(<s:string>[i=1:1,1,0],'we,are,devo'),a,rsub(s,'s/([a-z]*),([a-z]*),([a-z]*)/\$3 \$2 \$1/'))"
{i} s,a
{1} 'we,are,devo','devo are we'
```


## sleep 

SciDB nap time.

### Synopsis

```
uint32 (time)
```

> * time: a unit32 value specifying time to sleep in seconds.

### Description

Invoke the sleep system call. Returns zero if the requested time has elapsed,
or the number of seconds left to sleep, if the call was interrupted by a signal
handler.


#### Example

```
# Sleep for 6 seconds (1 + 2 + 3):

iquery -aq "apply(build(<t: uint64>[i=1:3,3,0], i), zzz, sleep(t))"
{i} t,zzz
{1} 1,0
{2} 2,0
{3} 3,0
```

## book

An example financial market order book support function.

### Synopsis
```
book( book_string_1,  book_string_2,  depth)
```

### Description
The book function merges two market order books up to an indicated depth.
The input order books are supplied as specially-formatted strings in the
form:

> bid&#95;price&#95;1, bid&#95;size&#95;1, bid&#95;price&#95;2, bid&#95;size&#95;2, ..., bid&#95;price&#95;m, bid&#95;size&#95;m  | ask&#95;price&#95;1, ask&#95;size&#95;1, ask&#95;price&#95;2, ask&#95;size&#95;2, ..., ask&#95;price&#95;n, ask&#95;size&#95;n

That is the bid prices and sizes are listed as comma-separated values, followed
by a vertical pipe symbol, followed by the ask prices and sizes. The bid and ask
sides may contain different numbers of entries. 

The two book entries are combined into a single book entry, summing sizes
associated with common prices on the bid and ask sides (see the examples).

The depth parameter controls the maximum book depth returned. The returned
value is a string formatted as the input book values. Only the *best*
(highest bid and lowest ask) prices up to the depth are reported.


### Examples

#### Merge two books, returning the full consolidated book
(Well technically, up to depth 100000000.) Note that the books have
different numbers of entries on their buy and ask sides.

```
iquery -aq "apply(apply(build(<a:string>[i=1:1,1,0],
                     '1.0,100, 2.0,50, 3.0,25 | 4.0,100, 5.0,50'),
                  b, '1.0,100, 2.0,25, 3.5,50| 5.0,100, 6.0,55, 7.0,100'),i
             c, book(a, b, 100000000))"
{i} a,b,c
{1} '1.0,100, 2.0,50, 3.0,25 | 4.0,100, 5.0,50',
    '1.0,100, 2.0,25, 3.5,50| 5.0,100, 6.0,55, 7.0,100',
    '1.000, 200, 2.000, 75, 3.000, 25, 3.500, 50 | 4.000, 100, 5.000, 150, 6.000, 55, 7.000, 100'
```

#### A repeat of the last example, limiting the output book depth to two

```
iquery -aq "apply(apply(build(<a:string>[i=1:1,1,0],
                     '1.0,100, 2.0,50, 3.0,25 | 4.0,100, 5.0,50'),
                  b, '1.0,100, 2.0,25, 3.5,50| 5.0,100, 6.0,55, 7.0,100'),
             c, book(a,b, 2))"
{i} a,b,c
{1} '1.0,100, 2.0,50, 3.0,25 | 4.0,100, 5.0,50',
    '1.0,100, 2.0,25, 3.5,50| 5.0,100, 6.0,55, 7.0,100',
    '3.000, 25, 3.500, 50 | 4.000, 100, 5.000, 150'
```

### Notes

Bid and ask price and size data are likely to occur as attributes in a SciDB
array. In that case, you need to assemble the required input format string for
the book type. Here is an example that first sets up an example array with data
in attributes, and then creates a new array with the book representation of the
same data:
```
iquery -aq "store( 
              apply(
                build(<bid_price_1:double>[i=1:1,1,0],10.0),
                       bid_size_1, int16(100),
                       bid_price_2, double(10.5),
                       bid_size_2, int16(200),
                       ask_price_1, double(10.9),
                       ask_size_1, int16(150),
                       ask_price_2, double(11.1),
                       ask_size_2, int16(200)),
              example_data)"

{i} bid_price_1,bid_size_1,bid_price_2,bid_size_2,ask_price_1,ask_size_1,ask_price_2,ask_size_2
{1} 10,         100,       10.5,       200,       10.9,       150,       11.1,       200

# Now let's make this into a book-typed array:
iquery -aq "project(
              apply(example_data, book,
                  string(bid_price_1) + ',' + string(bid_size_1) + ',' +
                  string(bid_price_2) + ',' + string(bid_size_2) + '|' +
                  string(ask_price_1) + ',' + string(ask_size_1) + ',' +
                  string(ask_price_2) + ',' + string(ask_size_2)), book)"

{i} book
{1} '10,100,10.5,200,10.9,150,11.1,200'
```

## Installing the plug in

You'll need SciDB installed, along with the SciDB development header packages.
The names vary depending on your operating system type, but they are the
package that have "-dev" in the name. You *don't* need the SciDB source code to
compile and install this. Replace 'scidb-14.3' in the instructions below with your
SciDB version.

#### Required development packages on RHEL and CentOS systems:
```
yum install scidb-14.3-dev.x86_64 scidb-14.3-dev-tools.x86_64 scidb-14.3-dev-tools-dbg.x86_64 scidb-14.3-plugins-dbg.x86_64 scidb-14.3-libboost-devel.x86_64 scidb-14.3-libboost-static.x86_64 log4cxx-devel pcre-devel.x86_64
```

#### Required development packages on Ubuntu systems:
```
apt-get install scidb-14.3-dev scidb-14.3-dev-tools scidb-14.3-libboost1.54-dev scidb-14.3-libmpich2-dev scidb-14.3-libboost1.54-all-dev liblog4cxx10-dev libpcre3-dev
```

## Installing the plug in

You'll need SciDB installed. The easiest way to install and load the plugin is by using https://github.com/paradigm4/dev_tools

Otherwise, you can build manually using the SciDB development header packages. The names vary depending on your operating system type, but they are the package that have "-dev" in the name. You *don't* need the SciDB source code to compile and install this.

Run `make` and copy  `*.so` to the `lib/scidb/plugins`
directory on each of your SciDB cluster nodes. Here is an example:

```
cd superfunpack
make
cp *.so /opt/scidb/14.8/lib/scidb/plugins

iquery -aq "load_library('superfunpack')"
```
Remember to copy the plugin to __all__ your SciDB cluster nodes.

## Licenses

Superfunpack is Copyright (c) 2014 by Paradigm4, Inc., contact Bryan Lewis
<blewis@paradigm4.com>, and is licenensed under GPL 2.

The code in superfunpack incorporates pcrs code Copyright (C) 2000, 2001 by
Andreas S. Oesterhelt  <andreas@oesterhelt.org> (LGPL 2), R Copyright (C)
1992-1996, 1998-2012 Free Software Foundation, Inc. (GPL 2), and boost (Boost
Software License - Version 1.0 - August 17, 2003).

