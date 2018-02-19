
g++ -funsigned-char SRC/*.cpp  -o BINARIES/ChatScript -L/user/lib -lcurl -lpthread 2>err.txt

#  or maybe below if the above doesnt work for you do this bottom one

#  g++ -funsigned-char SRC/*.cpp  -o BINARIES/ChatScript  -lpthread -lcurl 2>err.txt
