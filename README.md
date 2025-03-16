OneMustSee -- a OMF2097 memory viewer
=====================================

This is an application for viewing memory used by the 1994 DOS game One Must
Fall 2097. It has some requirements to use:

* You need to be on linux
* You need to be willing to run this as root
* You have a working C compiler
* You are running the game in DosBox/DosBox-Staging/DosBox-X
* You are using OMF version 2.1

Compiling
---------

> gcc onemust.c -o OneMustSee

Usage
-----

> ./onemustsee <dosbox pid> [--csv]

In CSV mode it will emit a CSV header of the enabled fields, and then after each
tick it will add a row to the field.

In non CSV mode it will print some initial information, then show whenever each
field changes.


Configuration
-------------

None. Edit the source if you want to add/remove fields.


Problems
---------

Me too, buddy.


Bonus Software
--------------

Also included, at no extra cost (just pay shipping and handling) is a bash
script to use OpenOMF's rectool to annotate REC files using the CSV output of
onemustsee. You use `omf play <recfile>` in dosbox while capturing the output
with onemustsee in CSV mode. Then you can do something like this:

```
cp <rec file you just played in OMF> .
bash add_assertions.sh <csv from onemustsee> <copy of the rec file>
```

OpenOMF can then play the REC file and check the assertions against its own game
state.
