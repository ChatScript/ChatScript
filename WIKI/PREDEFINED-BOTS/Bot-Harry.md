# Bot Harry - basic bot

ChatScript comes with a simple bot, already compiled, called Harry. He has a minimal knowledge of
his personal life (childhood) and the ability to quibble and stall in response to most anything else.

# Simple Editing the Script
To change the script, use some word editor (WORD, notepad, whatever) and open up
`RAWDATA/HARRY/introductions.top`. In it find the line at the bottom that says My name is Harry
and change it to My name is Harry Potter. Then, while running the ChatScript engine, (having
provided a user name), enter 
```
:build Harry`
```

This rebuilds the simple script and reinitiates a conversation.

You can then type in What is your name? and get back your new answer. The quibbles are all in files
under `RAWDATA/QUIBBLE`, sorted by kind of sentence.

In` RAWDATA/skeleton.top` there are a whole slew of predefined topic declarations with keywords. You
create some new topic file perhaps based on one of the topics listed and then dump it into the HARRY
folder and do `:build Harry`.
