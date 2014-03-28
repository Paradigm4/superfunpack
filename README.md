# superfunpack

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


## fisher\_test\_odds\_ratio

Estimate the conditional odds ratio for Fisher's exact test for testing the
null of independence of rows and columns in a 2x2 contingency table with fixed
marginals.

Use this function together with the `pyhper` hypergeometric cumulative
distribution function described below to conduct Fisher's exact tests on 2x2
contingency tables.

### Synopsis

```
double fisher_test_odds_ratio (double x, double m, double n, double k)
```
> * x: Number of 'yes' events in both classifications (see table below)
> * m: Marginal sum of the 1st column ('yes' events in 1st class)
> * n: Marginal sum of  the 2nd column ('no' events in 1st class)
> * k: Marginal sum of the 1st row ('yes' events in 2nd class)

The following table illustrates the parameters x, m, n, and k in
a contingency table comparing two classifications labeled I and II:

|                  | Class I YES   | Class I NO  | SUM       |
| ---------------- | :-----------: | :---------: | --------: | 
| **Class II YES** | x             | a           | k = x + a |
| **Class II NO**  | b             | c           |           |
| **SUM**          | m = x + b     | n = a + c   |           |


### Example

## pyhper

## dyhper
## qyhper

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


#### Example: Replace the first occurrence of a subtring

```
iquery -aq "apply(build(<s:string>[i=0:0,1,0],'{0}[(\'Paul Brown is a serious serious man.\')]',true), r, rsub(s,'s/serious/silly/'))"
{i} s,r
{0} 'Paul Brown is a serious serious man.','Paul Brown is a silly serious man.'
```

#### Example: Replace all occurrences of a subtring

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
