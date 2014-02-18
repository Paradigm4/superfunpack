superfunpack
============

Miscellaneous functions


strpftime
==


```
iquery -aq "apply(build(<s:string>[i=0:0,1,0],'{0}[(03-Mar-2012)]',true),x,strpftime(s, '%d-%m-%Y', '%U'))"
```
