<!DOCTYPE HTML>
<html>
  <head>
    <title>
      CHATSCRIPT SERVER
    </title>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <style type="text/css">
	
      #topPanel {
        min-width: 200px;
        min-height: 200px;
        width: 80vw;
        height: 50vh;
        overflow: auto;
        margin: 0px auto;
      }	

      #responseHolder {
        min-width: 400px;
        min-height: 250px;
        width: 90%;
        height: 250px;
        overflow: auto;
        margin: 10px auto;
        background-color: lightgrey;
		border-radius: 12px;
		padding: 10px;
		font-family: Arial;
		font-size: 10pt;
		font-style: normal;
		font-weight: normal;
      }
	    
		#avatarHolder {
        height: 200px;
        width: 200px;       
        margin: 0px auto;
        background-color: darkgrey;
		border-radius: 12px;
		padding: 0px;
		float: right;
		font-family: Arial;
		font-size: 12pt;
		font-style: normal;
		font-weight: normal;
	}

    #formpanel {
	min-width: 400px;
	min-height: 200px;
	width: 80%;
    overflow: auto;
	margin: 0px auto;
	font-family: Arial;
	font-size: 10pt;
	font-style: normal;
	font-weight: normal;
      }
      
	  #form_table {
        width: 100%;
		overflow: auto;
		margin: 0px auto;
       
      }
      #txtUser {
	padding: 2px;
      }
      #txtMessage {
	width: 95%;
	right-margin: 5px;
	padding: 2px;
      }
      #speechcontainer {
	margin: 10px auto;
        border-style: solid;
        border-width: 1px;
	border-radius: 12px;
	border-color: darkgrey;
	width: 110%;
	height: 80px;
	padding: 10px;
	font: Arial;
	font-size: 10pt;
	font-style: normal;
	font-weight: normal;
	color: black;
      }
      #button_panel {
        width: 80%;
      }
      #btnMicrophone { 
	margin: 10px;
	float: left;	
	}
      #results {
	font-family: cursive,Arial;
	font-size: 14pt;
	font-style: italic;
	font-weight: normal;
	color: darkgrey;
        margin: 10px;

      }
    </style>
    <script type="text/javascript">
	var cbAutoSend = 'checked';
 	var cbAutoSpeech = 'checked';
     </script>
  </head>
  <body>
    <div id="responseHolder"></div>
     <div id="avatarHolder">
          <img src="rose.gif" alt="rose image" height="200px" width="200px" style="border-radius: 12px;" > 
      </div>
      <div id="formpanel">
    	<div>
		<input type="checkbox" name="autosend" value="checked" checked onclick="if (this.checked) {cbAutoSend = this.value} else { cbAutoSend = '' }"/> Continuous Speech Input
  	    <input type="checkbox" name="speech" value="checked" checked onclick="if (this.checked) {cbAutoSpeech = this.value} else { cbAutoSpeech = '' }"/> Speech Output
     	</div>
    <form id="frmChat" action="#" >
    <p>
      Hi. My name is Rose. I care about security, so while I'm happy to chat with you, it will have to be through this untrackable interface.
   </p>
   <table id="form_table" >
      <tr>
        <td>Name:</td>
        <td>
          <input type="text" id="txtUser" name="user" size="20" value="User" />
          <input type="hidden" name="send" />
        </td>
      </tr>
      <tr>
        <td>Message:</td>
        <td><input type="text" name="message" id="txtMessage" size="80" /></td>
	<td colspan="2"><input type="submit" name="send" value="Send Input" /></td>
      </tr>

      <!-----Added this row for Speech Recognition-------------------------------------------------------->
      <tr>
        <td colspan="2">
	<div id="speechcontainer" >
  		<div id="info">Or click on the microphone icon and begin speaking.</div>
  		<div id="button_panel">
    			<button id="btnMicrophone" type="button" value="microphone" onclick="microphoneClick()">
    			<img id="start_img" src="mic.gif" alt="Start"></button> 
  		</div>
  		<div id="results">
    			<span id="final_span" class="final"></span>
    			<span id="interim_span" class="interim"></span><p>
  		</div>
	</div>
	
        </td>
      </tr>
      <!-------------------------------------------------------------------------------------------------->
	
    </table>
    </form>
   </div>

<script type="text/javascript" src="//ajax.googleapis.com/ajax/libs/jquery/1.11.0/jquery.min.js"></script>
<script type="text/javascript">

var botName = 'Rose';		// change this to your bot name

// declare timer variables
var alarm = null;
var callback = null;
var loopback = null;

$(function(){
	$('#frmChat').submit(function(e){
	// this function overrides the form's submit() method, allowing us to use AJAX calls to communicate with the ChatScript server
	e.preventDefault();  // Prevent the default submit() method
	var name = $('#txtUser').val();
        if (name == '') {
		alert('Please provide your name.');
		document.getElementById('txtUser').focus();
         }
	var chatLog = $('#responseHolder').html();
	var youSaid = '<strong>' + name + ':</strong> ' + $('#txtMessage').val() + "<br>\n";
	update(youSaid);
	var data = $(this).serialize();
	sendMessage(data);
	$('#txtMessage').val('').focus();
	});


	// any user typing cancels loopback or callback for this round 
	$('#txtMessage').keypress(function(){
        window.clearInterval(loopback);
        window.clearTimeout(callback);
        });
		
});


function sendMessage(data){ //Sends inputs to the ChatScript server, and returns the response-  data - a JSON string of input information
$.ajax({
	url: 'ui.php',
	dataType: 'text',
	data: data,
    type: 'post',
    success: function(response){
		processResponse(parseCommands(response));
    },
    error: function(xhr, status, error){
		alert('oops? Status = ' + status + ', error message = ' + error + "\nResponse = " + xhr.responseText);
    }
  });
}


function parseCommands(response){ // Response is data from CS server. This processes OOB commands sent from the CS server returning the remaining response w/o oob commands

	var len  = response.length;
	var i = -1;
	while (++i < len )
	{
		if (response.charAt(i) == ' ' || response.charAt(i) == '\t') continue; // starting whitespace
		if (response.charAt(i) == '[') break;	// we have an oob starter
		return response;						// there is no oob data 
	}
	if ( i == len) return response; // no starter found
	var user = $('#txtUser').val();
     
	// walk string to find oob data and when ended return rest of string
	var start = 0;
	while (++i < len )
	{
		if (response.charAt(i) == ' ' || response.charAt(i) == ']') // separation
		{
			if (start != 0) // new oob chunk
			{
				var blob = response.slice(start,i);
				start = 0;

				var commandArr = blob.split('=');
				if (commandArr.length == 1) continue;	// failed to split left=right

				var command = commandArr[0]; // left side is command 
				var interval = (commandArr.length > 1) ? commandArr[1].trim() : -1; // right side is millisecond count
				if (interval == 0)  /* abort timeout item */
				{
					switch (command){
						case 'alarm':
							window.clearTimeout(alarm);
							alarm = null;
							break;
						case 'callback':
							window.clearTimeout(callback);
							callback = null;
							break;
						case 'loopback':
							window.clearInterval(loopback);
							loopback = null;
							break;
					}
				}
				else if (interval == -1) interval = -1; // do nothing
				else
				{
					var timeoutmsg = {user: user, send: true, message: '[' + command + ' ]'}; // send naked command if timer goes off 
					switch (command) {
						case 'alarm':
							alarm = setTimeout(function(){sendMessage(timeoutmsg );}, interval);
							break;
						case 'callback':
							callback = setTimeout(function(){sendMessage(timeoutmsg );}, interval);
							break;
						case 'loopback':
							loopback = setInterval(function(){sendMessage(timeoutmsg );}, interval);
							break;
					}
				}
			} // end new oob chunk
			if (response.charAt(i) == ']') return response.slice(i + 2); // return rest of string, skipping over space after ] 
		} // end if
		else if (start == 0) start = i;	// begin new text blob
	} // end while
	return response;	// should never get here
 }
 
function update(text){ // text is  HTML code to append to the 'chat log' div. This appends the input text to the response div
	var chatLog = $('#responseHolder').html();
	$('#responseHolder').html(chatLog + text);
	var rhd = $('#responseHolder');
	var h = rhd.get(0).scrollHeight;
	rhd.scrollTop(h);
}

// TTS code taken and modified from here:
// http://stephenwalther.com/archive/2015/01/05/using-html5-speech-recognition-and-text-to-speech
//---------------------------------------------------------------------------------------------------
// say a message
function speak(text, callback) {
    var u = new SpeechSynthesisUtterance();
    u.text = text;
    u.lang = 'en-US';
    //u.lang = 'en-GB';
    u.voice = 4;   // 3 = american female  0 worse female 1 = not best femal
    u.rate = 1.0; // .85
    u.pitch = .9;  
    u.volume = 1.0;  // .5
 
    u.onend = function () {
        if (callback) {
            callback();
        }
    };
 
    u.onerror = function (e) {
        if (callback) {
            callback(e);
        }
    };
 
    speechSynthesis.speak(u);
}
//-----End of TTS Code Block-----------------------------------------------------------------------------

function processResponse(response) { // given the final CS text, converts the parsed response from the CS server into HTML code for adding to the response holder div
	var botSaid = '<strong>' + botName + ':</strong> ' + response + "<br>\n";
	update(botSaid);
	if (cbAutoSpeech == 'checked') { speak(response); }
}



// Continuous Speech recognition code taken and modified from here:
// https://github.com/GoogleChrome/webplatform-samples/tree/master/webspeechdemo
//----------------------------------------------------------------------------------------------------
var final_transcript = '';
var recognizing = false;
var ignore_onend;
var start_timestamp;

if (!('webkitSpeechRecognition' in window)) {
  info.innerHTML = "Speech only works if you use Chrome or Safari. Works best with headphones and mic for separation of signals";
} else {
  btnMicrophone.style.display = 'inline-block';
  var recognition = new webkitSpeechRecognition();
  recognition.continuous = true;
  recognition.interimResults = true;
  recognition.lang = 'en-US';
  recognition.onstart = function() {
    recognizing = true;
    info.innerHTML =  " Speak now.";
    start_img.src = 'mic-animate.gif';
  };

  recognition.onerror = function(event) {
    if (event.error == 'no-speech') {
      start_img.src = 'mic.gif';
      info.innerHTML = "You did not say anything.";
      ignore_onend = true;
    }
    if (event.error == 'audio-capture') {
      start_img.src = 'mic.gif';
      info.innerHTML = "You need a microphone.";
      ignore_onend = true;
    }
    if (event.error == 'not-allowed') {
      if (event.timeStamp - start_timestamp < 100) {
	//Added more detailed message to unblock access to microphone.
        info.innerHTML = " I am blocked. In Chrome go to settings. Click Advanced Settings at the bottom. Under Privacy click the Content Settings button. Under Media click Manage Exceptions Button. Remove this site from the blocked sites list. ";
      } else {
        info.innerHTML = "You did not click the allow button."
      }
      ignore_onend = true;
    }
  };

  recognition.onend = function() {
    recognizing = false;
    if (ignore_onend) {
      return;
    }
    start_img.src = 'mic.gif';
    if (!final_transcript) {
      info.innerHTML = "Click on the microphone icon and begin speaking.";
      return;
    }
    info.innerHTML = "";
   
  };

  recognition.onresult = function(event) {
    var interim_transcript = '';
    for (var i = event.resultIndex; i < event.results.length; ++i) {
      if (event.results[i].isFinal) {
        final_transcript += event.results[i][0].transcript;
	//----Added this section to integrate with Chatscript submit functionality-----
	txtMessage.value = final_transcript;
	final_transcript ='';
	final_span.innerHTML = '';
        interim_span.innerHTML = '';
	if (cbAutoSend == 'checked') { $('#frmChat').submit(); }
	//-----------------------------------------------------------------------------
      } else {
        interim_transcript += event.results[i][0].transcript;
      }
    } 
    final_span.innerHTML = final_transcript;
    interim_span.innerHTML = interim_transcript;  
  };



}

function microphoneClick(event) {
  if (recognizing) {
    recognition.stop();
    return;
  }
  final_transcript = '';
  txtMessage.value = '';
  recognition.start();
  ignore_onend = false;
  final_span.innerHTML = '';
  interim_span.innerHTML = '';
  start_img.src = 'mic-slash.gif';
  info.innerHTML = " Click the Allow button above to enable your microphone.";
  start_timestamp = event.timeStamp;
  
}
//End of Continuous Speech Recognition Block
//----------------------------------------------------------------------------------------------------


</script>
</body>
</html>