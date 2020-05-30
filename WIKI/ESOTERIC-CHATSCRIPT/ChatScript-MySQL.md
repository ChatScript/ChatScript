# ChatScript MySQL

Copyright Bruce Wilcox, mailto:gowilcox@gmail.com www.brilligunderstanding.com
<br>Revision 5/30/2020 cs10.4

# Overview
There are two different capabilities for using MySQL, as a file server and as a database.
As a database, a script can access MySQL on demand (described later). As a file server, this
allows infinite scaling of CS.

## MySQL Fileserver 

By default, a CS server records the current state of a user in a file stored in the local 
filesystem under the name topic_username_botname.txt. 
This works fine for single-server systems. In a world where one uses a second 
server as a hot backup and transfer users to it while redeploying the primary server, user information gets lost. The solution to this (and the ability to scale CS with an arbitrary number of servers) is to have some kind of networked file server.

CS directly supports use of Mongo, Postgres, and MySQL databases as file servers. 
Instead of writing the users’s state record to the local file system, 
it writes it as a record in the database. 
That record will be overwritten each volley with the changed state. 
There is no accumulation of prior state data. Nor are user logs ever written there. 
If user logging is on, such logs are written to the machine performing the chat 
(which may scatter them over many machines). Likewise server logging is kept per machine.

# Setup
To run CS MySQL you need two things. First, you need to run an executable that has been compiled with MySQL enabled.  
Second, you need to supply parameters for CS startup in the cs_init.txt file or cs_initmore.txt file. A line entry looks like this:
```
mysql="host=localhost port=3306 user=root password=admin db=chatscript" 
```
where the host and port refer to the MySQL server access, user and password refer to some userid on the mysql server allowed to access the named db.  
The database itself needs to already exist, it is not automatically created by CS.  It should be defined with the following fields in the schema/table called chatscript using this statement:
The keys (hose, port, user, password, db, appendport) should be in lowercase.
```
create database chatscript character set 'UTF8';
create user chatuser identified by 'password';    or whatever
grant all on chatscript.* to chatuser;
use chatscript;
CREATE TABLE userfiles (userid varchar(100) PRIMARY KEY, file MEDIUMBLOB, when DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP)
```

The table name “userfiles” and the field names are fixed. ChatScript assumes them.
userid  - a limit of 64 is more than enough
file: – in our world we are going to have less than 500K bytes of data
when – CS never sends or uses this value. It is there to allow an external program to query the database and delete old records since our users do not continue forever with Pearl.

An optional key is `appendport` which takes no value. It means to add the current CS port as a
suffix onto the db name. So
```
mysql="host=localhost port=3306 user=root password=admin db=chatscript appendport" 
```
will yield a db name of `chatscript1024` for a default CS server.

# CS Termination: 
If at any time a call to utilize MySQL fails, the CS server will abort itself. If you have an 
automatic restart and retry mechanism, then maybe things will just work next time, or a different
CS server will get invoked.

## MySQL database

A script may also access MySQL using 3 functions: ^MySQLInit, ^MySQLQuery, ^MySQLClose.

### ^MySQLInit("information string")

To open a database access, you need to supply a string of information. The data is similar to the
cs_init.txt mysql line, e.g.,
```
^mysqlinit("host=localhost port=3306 user=root password=admin db=chatscript")
```
This implies you must have already created the database to be used.

### ^MySQLClose()

When you are done accessing the database, this will end the connection.

### ^MySQLQuery( "query string" {'^fn})

You may provide a query in the query string. The output, if there is any, will be returned from this
function. You have the option to provide a quoted function name to execute on each row of data 
returned.  If your function only takes 1 argument, all output from a row will be sent to it as a 
single quoted string. If your function takes multiple arguments, the separate columns of a row will
be passed to the corresponding arguments.

# Sample Bot

There is a sample bot compilable with :build mysql.