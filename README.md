#  superfunpack

Miscellaneous scalar functions for SciDB that are super fun.

## Installing the plug in

You'll need SciDB installed, along with the SciDB development header packages.
The names vary depending on your operating system type, but they are the
package that have "-dev" in the name. You *don't* need the SciDB source code to
compile and install this.

Run `make` and copy  the `libsuperfunpack.so` plugin to the `lib/scidb/plugins`
directory on each of your SciDB cluster nodes. Here is an example:

```
git clone https://github.com/Paradigm4/superfunpack.git
cd superfunpack
make
cp libsuperfunpack.so /opt/scidb/13.12/lib/scidb/plugins

iquery -aq "load_library('superfunpack')"
```
Remember to copy the plugin to all you SciDB cluster nodes.

Note that if you're *re-installing* superfunpack, you'll need to restart
SciDB for the new plugin to take effect.


## fisher\_test\_odds\_ratio and fisher\_test\_p\_value

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

      Fisher's Exact Test for Count Data

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


## sleep 

SciDB nap time.

### Synopsis

```
uint32 (time)
```

> * time: a unit32 value specifying time to sleep in seconds.

### Description

Invoke the sleep system call. Returns zero if the requested time has elapsed, or the number of seconds left to sleep, if the call was interrupted by a signal handler.


#### Example

```
# Sleep for 6 seconds (1 + 2 + 3):

iquery -aq "apply(build(<t: uint64>[i=1:3,3,0], i), zzz, sleep(t))"
{i} t,zzz
{1} 1,0
{2} 2,0
{3} 3,0
```

## Licenses

Superfunpack is Copyright (c) 2014 by Paradigm4, Inc., contact Bryan Lewis
<blewis@paradigm4.com>, and is licenensed under GPL 2.

The code in superfunpack incorporates pcrs code Copyright (C) 2000, 2001 by
Andreas S. Oesterhelt  <andreas@oesterhelt.org> (LGPL 2), R Copyright (C)
1992-1996, 1998-2012 Free Software Foundation, Inc. (GPL 2), and boost (Boost
Software License - Version 1.0 - August 17, 2003).
