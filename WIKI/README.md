# ChatScript Guide To Documentation (user guides, tutorials, papers)
<br>© Bruce Wilcox, mailto:gowilcox@gmail.com www.brilligunderstanding.com
<br>Revision CS7.6

ChatScript has a lot of documentation in various manuals, so knowing what to read may seem daunting.
Here is an overview.

## Basic ChatScript for starters

* [What is ChatScript?](OVERVIEWS-AND-TUTORIALS/What-is-ChatScript.md)
<br>Overview of the design goals and abilities of CS. Not necessary to read.

* [ChatScript Basic User Manual](ChatScript-Basic-User-Manual.md)
<br>This explains how to run CS, how to understand basic CS ideas like rules, topics, and
concepts. A must starting place.

* [ChatScript Tutorial](OVERVIEWS-AND-TUTORIALS/ChatScript-Tutorial.md)
<br>A briew step-by-step on creating a chatbot travel agent, written by a CS user.

* [ChatScript Memorization](ChatScript-Memorization.md)
<br>A simple explanation of how to “learn” data about the user.

* [ChatScript Common Beginner Mistakes](ChatScript-Common-Beginner-Mistakes.md)
<br>Here are a collection of common beginner mistakes.


## Predefined Bots

* [Bot Harry - basic bot](PREDEFINED-BOTS/Bot-Harry.md)
<br>A brief overview of the simple Harry bot and how to make simple modifications.
Potentially useful for a beginner read.

* [Bot NLTK – NL analysis bot](PREDEFINED-BOTS/Bot-NLTK.md)
<br>A brief description of how to run the NLTK bot. Not useful for most people, especially if
NLTK means nothing to you.

* [Bot Stockpile – planner bot](PREDEFINED-BOTS/Bot-Stockpile.md)
<br>A brief description of how to run the Stockpile bot. Not useful for most people. It's about planner capabilities of CS.

* [Bot Postgres – postgres bot](PREDEFINED-BOTS/Bot-Postgres.md)
<br>Illustration of using Postgres database.

* [Bot German](PREDEFINED-BOTS/Bot-German.md)
<br>An illustration of hooking in an external pos-tagger for foreign language support.


## Advanced ChatScript

* [ChatScript Advanced User Manual](ChatScript-Advanced-User-Manual.md)
<br>Once you've master basic CS, this is the place to go next.

* [ChatScript System Functions Manual](ChatScript-System-Functions-Manual.md)
<br>A listing of all the functions of CS.

* [ChatScript Fact Manual](ChatScript-Fact-Manual.md)
<br>A discussion of how to manipulate facts in CS.

* [ChatScript Json](ChatScript-Json.md)
<br>ChatScript support for Json

* [ChatScript Overview Input to Output](ChatScript-Overview-of-Input-to-Output.md)
<br>An overview of the process of converting input to output. Not necessary except for
really advanced users wanting the appropriate mental model.

* [ChatScript Pattern Redux](ChatScript-Pattern-Redux.md)
<br>A terse but detailed look at everything involving rule patterns.

* [ChatScript System Variables and Engine-defined Concepts](ChatScript-System-Variables-and-Engine-defined-Concepts.md)
<br> Engine-defined Concepts. System Variables. Control over Input. Interchange Variables.

* [ChatScript Command Line Parameters](ChatScript-Command-Line-Parameters.md)
<br> Command line parameters details and usage.

* [ChatScript Multiple Bots](ChatScript-Multiple-Bots.md)
<br> How to control multiple bots in a single server.

* [Installing and Updating ChatScript](Installing-and-Updating-ChatScript.md)
<br>Installing on Windows, Mac, Linux. Updating ChatScript (advanced).

* [ChatScript Coding Standards](ChatScript-Coding-Standards.md)


## Test and Debug

* [ChatScript Finalizing a Bot](ChatScript-Finalizing-A-Bot.md)
<br>Once you have built a bot, how to polish it and make sure it is “ready”. A bot will likely
never be complete because you will want to keep improving it.

* [ChatScript Testing Manual](ChatScript-Debugging-Manual.md)
<br>The features of CS that support debugging, including tracing.

 * [ChatScript Debugger](ChatScript-Debugger.md)
<br>Built-in text oriented step debugger.


## Specialized ChatScript

* ### Servers and Clients

 * [ChatScript ClientServer Manual](CLIENTS-AND-SERVERS/ChatScript-ClientServer-Manual.md)
<br>How to configure and run CS as a server. And thinking about CS on mobile.

 * [ChatScript External Communications](CLIENTS-AND-SERVERS/ChatScript-External-Communications.md)
<br> How to embedding ChatScript inside another main program, calling programs on the OS from ChatScript, and getting services via the Internet from ChatScript.

 * [ChatScript Amazon Server](CLIENTS-AND-SERVERS/ChatScript-Amazon-Server.md)
<br>How to install CS as a server on Amazon AWS.


* ### Esoteric ChatScript

 * [ChatScript Control Scripts](ESOTERIC-CHATSCRIPT/ChatScript-Control-Scripts.md)
<br>Brief overview of writing your own control scripts

 * [ChatScript Analytics](ESOTERIC-CHATSCRIPT/ChatScript-Analytics-Manual.md)
<br>Debug functions that can dissect log files.

 * [ChatScript Document Reader](ESOTERIC-CHATSCRIPT/ChatScript-Document-Reader.md)
<br>How to use CS to acquire information from a document.

 * [ChatScript Javascript](ESOTERIC-CHATSCRIPT/ChatScript-Javascript.md)
<br>How to write outputmacros in Javascript and call them.

 * [ChatScript Mongo](ESOTERIC-CHATSCRIPT/ChatScript-MongoDB.md)
<br>How to use the Mongo db directly from CS

 * [ChatScript Planning](ESOTERIC-CHATSCRIPT/ChatScript-Planning.md)
<br>How to use CS as an HTN (hierarchical task network) planner.

 * [ChatScript PosParser](ESOTERIC-CHATSCRIPT/ChatScript-PosParser.md)
<br>How to use grammar/parsing in CS patterns.

 * [ChatScript PostgreSQL](ESOTERIC-CHATSCRIPT/ChatScript-PostgreSQL.md)
<br>How to use the Postgres database directly from CS.

 * [ChatScript MySQL](ESOTERIC-CHATSCRIPT/ChatScript-MySQL.md)
<br>How to use the MySQL database directly from CS.

 * [ChatScript Exotica](ESOTERIC-CHATSCRIPT/ChatScript-Exotica-Examples.md)
<br>Brief old interesting scripting tips

 * [ChatScript Foreign Languages](ESOTERIC-CHATSCRIPT/ChatScript-Foreign-Languages.md)
<br>Running CS in a language other than English.

 * [ChatScript Engine](ESOTERIC-CHATSCRIPT/ChatScript-Engine.md)
<br>How the internals of the engine work and how to extend it with private code.


## Papers in order

* [Paper - ChatBots 102](../PAPERS/Paper-%20ChatBots%20102.pdf)
<br>My first paper, looking at the flaws of AIML and why I felt I could do better (before Suzette won anything).

* [Paper - Pattern Matching for Natural Language](PAPERS/Paper-Pattern-Matching-for-Natural-Language-Applications.md)
<br>Compares CS, AIML, and Facade

* [Paper - Suzette The Most Human Computer](PAPERS/Suzette-The-Most-Human-Computer.md)
<br>How our first chatbot came about, won the Loebner's, and differed from AIML.

* [Paper - Speaker for the Dead](../PAPERS/Paper-%20Speaker%20for%20theDead.pdf)
<br>Applying chatbots to manage people's accumulations of papers, photos, etc.

* [Paper - Google Talk](PAPERS/Paper-Google-Talk.md)
<br>A talk I gave at Google about my history, CS, and writing code to act out stories

* [Paper - Writing a Chatbot](PAPERS/Writing-a-Chatbot.md)
<br>Useful discussion on how to think about writing a chatbot

* [Paper - ARBOR_ MakingItReal](PAPERS/Paper-ARBOR-MakingItReal.md)
<br>Useful discussion on how to think about writing a chatbot

* [Paper - Winning 15 Minute Conversation](../PAPERS/Paper-%20Winning%2015%20Minute%20Conversation.pdf)
<br>The conversation (1 of 2) that had our chatbot easily win best 15 minute conversation at ChatBot Battles 2012.

* [Paper - Winning the Loebner's](PAPERS/Paper%20-%20WinningTheLoebners.md)
<br>Realities of the Loebner competition and additional ideas of english applied to chatbots

* [ChatScript Training](PAPERS/ChatScript-Training.md)
<br>A slide series on how CS works and how the engine works.
