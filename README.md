superfunpack
============

Miscellaneous functions


strpftime
---

Flexible date/time string parsing and conversion

Synopsis
--

```
string strpftime (input_string, input_format, output_format)

input_string: An input string value representing date and/or time.
input_format: A strptime-valid format describing the input string.
output_format: A strftime-valid output format.
```

Description
--
The strpftime function parses and converts dates and times from one string format representation into another. It's useful to pick out portions of a date or time, for example, to pick out the week of the year, or the weekday name, or the name of the month, etc. It can understand any str[pf]time time format.


* Example 1: Pick out the week of the year.

```
iquery -aq "apply(build(<s:string>[i=0:0,1,0],'{0}[(03-Mar-2012)]',true), woy , int64(strpftime(s, '%d-%m-%Y', '%U')))"
{i} s,woy
{0} '03-Mar-2012',1
```

* Example 2: Print the full weekday name of a date in your locale.

```
iquery -aq "apply(build(<s:string>[i=0:0,1,0],'{0}[(03-Mar-2012)]',true), day ,strpftime(s, '%d-%m-%Y', '%A'))"
{i} s,day
{0} '03-Mar-2012','Sunday'
```

Installation
--

You'll need SciDB installed, along with the SciDB development header packages. The names vary depending on your operating system type, but they are the package that have "-dev" in the name. You *don't* need the SciDB source code to compile and install this.

Run `make` and copy  the `libsuperfunpack.so` plugin to the `lib/scidb/plugins` directory on each of your SciDB cluster nodes. Here is an example:

```
git clone https://github.com/Paradigm4/superfunpack.git
cd superfunpack
make
cp libsuperfunpack.so /opt/scidb/13.12/lib/scidb/plugins

iquery -aq "load_library('superfunpack')"
```
Remember to copy the plugin to all you SciDB cluster nodes.
