To autostart server using cron, use   crontab  jobs.cron  (if you are in the LINUX directory)

if crontab doesnt seem to work, consider: 
/etc/init.d/crond restart

If you dont have a log file, try   tail /var/log/syslog
or continuous: tail -f /var/log/syslog | grep CRON


in apache the index page (script to run chatbot) can be in /var/www/html/index.php

sudo -s to access the var file

to install g++
sudo apt-get install g++
sudo apt-get install gdb
sudo apt-get install make


to count how many distinct users you have:
find USERS -type f | wc -l