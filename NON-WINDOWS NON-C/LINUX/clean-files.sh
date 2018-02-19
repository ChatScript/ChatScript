#! /bin/bash
# Simple cleaning script that get rid of build, log and temporary files

printf "\n"
printf "\e[34m###############################\n";
printf "# Clean files start           #\n";
printf "###############################\e[0m\n\n";

# move up dirs if we're in LINUX/
if [[ $PWD =~ LINUX ]]; then
  cd ../..;
fi

printf "\e[34mWorking directory is $PWD\e[0m\n";
printf "\n";

# clean dictionary and facts binaries
rm -rf DICT/ENGLISH/dict.bin;
rm -rf DICT/ENGLISH/facts.bin;
printf "\e[32mDeleted dict.bin and facts.bin\e[0m\n";

# clean LOGS but keep compile.log
rm -rf LOGS/*.txt;
printf "\e[32mDeleted LOGS/*.txt\e[0m\n";

# clean TMP
rm -rf TMP/*.txt;
printf "\e[32mDeleted TMP/\*.txt\e[0m\n";
rm -rf TMP/*.bin;
printf "\e[32mDeleted TMP/\*.bin\e[0m\n";

# clean TOPIC
rm -rf TOPIC/BUILD*;
printf "\e[32mDeleted TOPIC/BUILD\*\e[0m\n";

# clean USERS
rm -rf USERS/*.txt;
printf "\e[32mDeleted USERS/\*.txt\e[0m\n";

# clean VERIFY
rm -rf VERIFY/*.txt;
printf "\e[32mDeleted VERIFY/\*.txt\e[0m\n";

# clean err.txt file
if [[ -f err.txt ]]; then
  rm err.txt
  printf "\e[32mDeleted err.txt\e[0m\n";
fi

printf "\n"
printf "\e[34m###############################\n";
printf "# Clean files end             #\n";
printf "###############################\e[0m\n";

exit 0;