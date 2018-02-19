# ChatScript MySQL

© Bruce Wilcox, mailto:gowilcox@gmail.com www.brilligunderstanding.com
<br>Revision 6/25/2017 cs7.51

## Using MySQL as file server

One can also use MySQL as a replacement for the local file system of the USERS directory.
This allows you to scale CS horizontally by having multiple CS servers all connecting to
a common MySQL machine so a user can be routed to any server. To do this, on the ChatScript
command line provide the following command line params:
```
mysqlhost=127.0.0.1 mysqlport=3306 mysqldb=chatscript mysqluser=chatuser mysqlpasswd=password
```
It's probably easier to put these 5 params in a config file and use the config= command line param.

The MySQL code assumes the userfiles table exists in the database.  You can create a database and
the userfiles table using the following statements:
```
create database chatscript character set 'UTF8';
create user chatuser identified by 'password';
grant all on chatscript.* to chatuser;
use chatscript;
create table userfiles (userid varchar(400) PRIMARY KEY, file blob);
```

The MySQL code uses the following prepared statement queries to read, insert, and update a user record:
```
-- read a user
SELECT file FROM userfiles WHERE userid = ?
-- insert a user
INSERT INTO userfiles (file, userid) VALUES (?, ?)
-- update a user
UPDATE userfiles SET file = ? WHERE userid = ?
```

You can override these queries to support alternate schemas.  However, the MySQL module
assumes that any sql used to override the default queries will use the same sequence of arguments.
For example, assume you want to store user data in a table named 'randomusertable.' The following
parameters can be used to override the default MySQL queries:
```
mysqluserread=SELECT userdata FROM randomusertable WHERE username=?
mysqluserinsert=INSERT INTO randomusertable (userdata,username) VALUES (?, ?)
mysqluserupdate=UPDATE randomusertable SET userdata = ? WHERE username = ?
```

Note that the default and override queries use the same arguments in the same order.
