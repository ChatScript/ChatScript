#compile on osx

note: Please read the 'ChatScript on a Mac' for more options, including xcode and make.


Make sure that cURL is installed on your machine:
```bash
curl --version
```
If it is not, either install it ([instructions](http://macappstore.org/curl/)) or disable it by passing the `-DDISCARDJSON=1` flag into the g++ commands.

Either run the compile script or execute this command from the root of your ChatScript folder:
```bash
g++ -funsigned-char SRC/*.cpp -I/usr/local/opt/curl/include/ -I/usr/local/include/libbson-1.0 -I/usr/local/include/libmongoc-1.0 -o BINARIES/MacChatScript -L/usr/lib -L/usr/local/lib -lcurl -lpthread -lbson-1.0 -lmongoc-1.0 2>LOGS/build-log.txt
```

If the above command does not work, you can try with these parameters instead:
```bash
g++ -funsigned-char SRC/*.cpp  -o BINARIES/MacChatScript  -lpthread -lcurl 2>LOGS/build-log.txt
```

Make sure that your binary is executable:
```bash
chmod +x BINARIES/MacChatScript
```

Run ChatScript in local mode:
```bash
./BINARIES/MacChatScript local
```
