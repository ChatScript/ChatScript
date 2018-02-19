#! /bin/bash
# Simple build script that will make sure that :build 0 and :build kora execute without problems

printf "\n"
printf "\e[34m###############################\n";
printf "# Building KORA start         #\n";
printf "###############################\e[0m\n\n";

# move up one dir if we're in MAC/
if [[ $PWD =~ MAC ]]; then
  cd ../../;
fi

# $? is the latest command return state
ERROR=0

printf "\e[34mWorking directory is $PWD\e[0m\n";
printf "\n";

# regenerate build0 folders
BINARIES/MacChatScript build0=RAWDATA/files0.txt >> /dev/null;
ERROR+=$?;

if   [[ ERROR -eq 0 ]]; then
  printf "\e[32mCS build0 finished with no errors\e[0m\n";
else
  printf "\e[31mCS build0 finished with errors, see /LOGS/bugs.txt \e[0m\n";
fi

# regenerate build1 folders
BINARIES/MacChatScript build1=private/filesKora.txt >> /dev/null;
ERROR+=$?;

if   [[ ERROR -eq 0 ]]; then
  printf "\e[32mCS build1 finished with no errors\e[0m\n";  
else
  printf "\e[31mCS build1 finished with errors, see /LOGS/bugs.txt \e[0m\n\n";
fi

printf "\n"
printf "\e[34m###############################\n";
printf "# Building KORA end           #\n";
printf "###############################\e[0m\n";

printf "\n";
exit $ERROR;