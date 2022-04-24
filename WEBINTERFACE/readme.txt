A demo webpage named `index.php` exists in the `WEBINTERFACE/SIMPLE` directory of chatscript
and uses PHP to do the work. You need to set the IP address in the file `WEBINTERFACE/SIMPLE/index.php` to be the IP address of
your instance. If your webpage is on the same machine as your chatscript server, you should use
`localhost` as the ip address, meaning the current machine. My webpage is on a different machine than
my chatscript server. 

Once you have edited that (I did it using a local editor on my laptop and then uploaded the change
along with ChatScript so I didn't have to use a linux editor).
With the file changed, I copied it to the web server page folder via:
```
sudo cp WEBINTERFACE/SIMPLE/*.* /var/www/html
```

The SIMPLE webinterface just shows two lines of text. The BETTER webinterface has a scrollable
text box to see the history and supports the loopback/callback/alarm OOB messages. 
You install that just like you did the SIMPLE interface, by 
```
sudo cp WEBINTERFACE/BETTER/*.* /var/www/html
```

And beyond that is a SPEECH WEB INTERFACE, which requires Chrome.

