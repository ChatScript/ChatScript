
read this from Todd Kuebler:

It should be possible to create wrapper code for any of the 19 languages swig supports (http://www.swig.org/compare.html). 

http://vinterstitial.blogspot.com/2014/05/a-swig-of-chatbot-making-unity-chatter.html

https://github.com/tkuebler/ChatScriptBrains/tree/master/swig


for .NET/mono/c#:

Wrap ChatScript with swig ( creates a .cpp <—> .cs P/Invoke wrapper )
Generate DLL (windows),  .so ( linux ) or .dylib (mac) with chat script source + the wrapper and put it someplace 
libraryish /usr/local/lib
Use the swig generated .cs files to call the InitSystem and PerformChat methods of chat script. 

for unity:

Create a bundle instead of a .dylib ( on windows DLL should work )
put in /Assets/Plugins folder
use generated .cs files to call chat script from your unity scripts
