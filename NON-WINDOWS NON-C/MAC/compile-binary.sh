#! /bin/bash
# This will create a binary called MacChatScript in the BINARIES folder and make it executable

# move up one dir if we're in MAC/
if [[ $PWD =~ MAC ]]; then
  cd ../../;
fi

# keep track of our errors
ERROR=0;

printf "\n"
printf "\e[34m###############################\n";
printf "# Compile MacChatScript start #\n";
printf "###############################\e[0m\n\n";

printf "\e[34mWorking directory is $PWD\e[0m\n";
printf "\n"

printf "\e[34mCompilation in progres...\e[0m\n";
printf "\n"

PROCESSID=`ps -A | pgrep MacChatScript`;

if [ -n "$PROCESSID" ]; then
  kill -9 $PROCESSID;
  printf "\e[34mKilled runnning process MacChatScript PID $PROCESSID\e[0m\n";
  printf "\n";
fi

# Build the binary
g++ -funsigned-char SRC/*.cpp -I/usr/local/opt/curl/include/ -I/usr/local/include/libbson-1.0 -I/usr/local/include/libmongoc-1.0 -o BINARIES/MacChatScript -L/usr/lib -L/usr/local/lib -lcurl -lpthread -lbson-1.0 -lmongoc-1.0 2>LOGS/build-log.txt

# $? is the latest command return state
ERROR+=$?

if [[ ERROR -eq 0 ]]; then
  printf "\e[32mCS Compiling finished with no errors\e[0m\n";
  # Make sure it's executable
  chmod +x BINARIES/MacChatScript;
  printf "\e[32mBinary created: BINARIES/MacChatScript\e[0m\n";
else
  printf "\e[31mCS Compiling finished with errors, see /LOGS/compile.log \e[0m\n";
fi

printf "\n"
printf "\e[34m###############################\n";
printf "# Compile MacChatScript end   #\n";
printf "###############################\e[0m\n";

# Run our cleaning script if we didn't get an error
if [[ ERROR -eq 0 ]]; then
  
  # first we clean temp files
  chmod +x "NON-WINDOWS NON-C"/MAC/clean-files.sh;
  "NON-WINDOWS NON-C"/MAC/clean-files.sh
  ERROR+=$?
  
  # then we build kora
  chmod +x "NON-WINDOWS NON-C"/MAC/build-kora.sh;
  "NON-WINDOWS NON-C"/MAC/build-kora.sh
  ERROR+=$?
  
  # git checkout our DICT files since osx can't fully generate them
  git checkout DICT/ 2>LOGS/build-log.txt;
  ERROR+=$?
  
fi

if [[ ERROR -eq 0 ]]; then
  printf "\e[32m#####\n";
  printf "All done without errors!\n";
  printf "Run with \"BINARIES/MacChatScript local\"\e[0m\n\n";
else
  printf "\e[31m#####\n";
  printf "Errors!\n";
  printf "Inspects the logs.\e[0m\n\n";
fi

exit $ERROR;