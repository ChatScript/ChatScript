# ChatScript
Natural Language tool/dialog manager

ChatScript is the next generation chatbot engine that has won the Loebner's 4 times and is the basis for natural language company for a variety of tech startups.

ChatScript is a rule-based engine, where rules are created by humans writers in program scripts through a process called dialog flow scripting. These use a scripting metalanguage (simply called a "script") as their source code. 
Here what a ChatScript script file looks like:
```
#
# file: food.top
#
topic: ~food []

#! I like spinach
s: ( I like spinach ) Are you a fan of the Popeye cartoons?
	
	a: ( ~yes )  I used to watch him as a child. Did you lust after Olive Oyl?
    	    b: ( ~no ) Me neither. She was too skinny.
    	    b: ( yes ) You probably like skinny models.
	
	a: ( ~no ) What cartoons do you watch?
     		b: ( none ) You lead a deprived life.
     		b: ( Mickey Mouse ) The Disney icon.

#! I often eat chicken
u: ( ![ not never rarely ] I * ~ingest * ~meat ) You eat meat.

#! I really love chicken
u: ( !~negativeWords I * ~like * ~meat ) You like meat.

#! do you eat bacon?
?: ( do you eat _ [ ham eggs bacon] ) I eat '_0

#! do you like eggs or sushi?
?: ( do you like _* or _* ) I don't like '_0 so I guess that means I prefer '_1.

#! I adore kiwi.
s: ( ~like ~fruit ![~animal _bear] )  Vegan, you too...

#! do you eat steak?
?: ( do you eat _~meat ) No, I hate _0.

#! I eat fish.
s: ( I eat _*1 > ) 
  $food = '_0 
  I eat oysters.
```

Above example mentioned in article [How to build your first chatbot using ChatScript](https://medium.freecodecamp.com/chatscript-for-beginners-chatbots-developers-c58bb591da8#.2qdxjuyvs).


## Basic Features

* Powerful pattern matching aimed at detecting meaning.
* Simple rule layout combined with C-style general scripting.
* Built-in WordNet dictionary for ontology and spell-checking.
* Extensive extensible ontology of nouns, verbs, adjectives, adverbs.
* Data as fact triples enables inferencing and supports JSON representation.
* Rules can examine and alter engine and script behavior.
* Planner capabilities allow a bot to act in real/virtual worlds.
* Remembers user interactions across conversations.
* Document mode allows you to scan documents for content.
* Ability to control local machines via popen/tcpopen/jsonopen.
* Ability to read structured JSON data from websites.
* [Postgres](https://www.postgresql.org/) and [Mongo](https://www.mongodb.com/) databases support for big data or large-user-volume chatbots.

## OS Features

* Runs on Windows or Linux or Mac or iOS or Android
* Fast server performance supports a thousand simultaneous users.
* Multiple bots can cohabit on the same server.

## Support Features

* Mature technology in use by various parties around the world.
* Integrated tools to support maintaining and testing large systems.
* UTF8 support allows scripts written in any language
* User support forum on [chatbots.org](https://www.chatbots.org/ai_zone/viewforum/44/) 
* Issues or bugs on this [repo](https://github.com/bwilcox-1234/ChatScript/issues)


# Getting started

## Installation

Take this project and put it into some directory on your machine (typically we call the directory ChatScript, but you can name it whatever). That takes care of installation.

    git clone https://github.com/bwilcox-1234/ChatScript


## Standalone mode - run locally on a console (for developement/test)

From your ChatScript home directory, go to the BINARIES directory: 
```bash
cd BINARIES
```
And run the ChatScript engine

### Windows
```bash
ChatScript
```

### Linux
```bash
./LinuxChatScript64 local
```
Note: to set the file executable: `chmod a+x ./LinuxChatScript64`

### MacOS
```bash
./MacChatScript local
```

This will cause ChatScript to load and ask you for a username. Enter whatever you want. 
You are then talking to the default demo bot `Harry`.

## Server Mode (for production)
From your ChatScript home directory, go to the BINARIES directory and run the ChatScript engine as server
### Run the server on Windows
```bash
ChatScript port=1024
```
### Run the server on Linux
```bash
./LinuxChatScript64
```
### Run the server on MacOS
```bash
./MacChatScript
```

This will cause ChatScript to load as a server.  
But you also need a client (to test client-server communication). 
You can run a separate command window and go to the BINARIES directory and type 

### Run a client (test) on Windows
```bash
ChatScript client=localhost:1024 
```

### Run a client (test) on Linux
```bash
./LinuxChatScript64 client=localhost:1024
```

### Run a client (test) on MacOS
```bash
./MacChatScript client=localhost:1024
```

This will cause ChatScript to load as a client and you can talk to the server. 


## How to build a bot
Run ChatScript locally. From the ChatScript command prompt, type 

    :build Harry

or whatever other preinstalled bot exists. If you have revised basic data, you can first:

    :build 0

## How to compile the engine.
On windows if you have Visual Studio installed, launch `VS2010/chatscript.sln` or `VS2015/chatscript.sln` and do a build. 
The result will go in the `BINARIES` directory.

On Linux, go stand in the SRC directory and type `make server` (assuming you have make and g++ installed). This creates BINARIES/ChatScript, which can run as a server or locally. There are other make choices for installing PostGres or Mongo.


# Full Documentation

[ChatScript Wiki (user guides, tutorials, papers)](/WIKI/README.md)


# Contributing

1. Fork it
2. Create your feature branch (git checkout -b my-new-feature)
3. Commit your changes (git commit -am 'Add some feature')
4. Push to the branch (git push origin my-new-feature)
5. Create new Pull Request


# Last releases

[changes.md](/changes.md)


# Author 

* Bruce Wilcox
  * home website: [BrilligUnderstanding.com](http://www.brilligunderstanding.com)
  * mail:  [gowilcox@gmail.com](mailto:gowilcox@gmail.com )  
