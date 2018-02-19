<?php

/**
 *
 * file name: ui.php
 *
 * Credits:  This program is derived from a sample client script by Alejandro Gervasio
 * posted here:  http://www.devshed.com/c/a/PHP/An-Introduction-to-Sockets-in-PHP/
 * Modified 2012 by R. Wade Schuette
 * Augmented 2014 by Bruce Wilcox
 * Refactored 2014 by Dave Morton
 *
 * Function -- a working skeleton of a client to the ChatScript Server
 * NOTES:   Be sure to put in your correct host, port, username, and bot name below!
 *
 * LEGAL STUFF:
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, or sell
 * copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

//  =============  user values ====
$host = "1.22.08.4";  //  <<<<<<<<<<<<<<<<< YOUR CHATSCRIPT SERVER IP ADDRESS OR HOST-NAME GOES HERE
$port = 1024;          // <<<<<<< your port number if different from 1024
$bot  = "";       // <<<<<<< desired botname, or "" for default bot
//=========================

// Please do not change anything below this line.
$null = "\x00";
$postVars = filter_input_array(INPUT_POST, FILTER_SANITIZE_STRING);
extract($postVars);

if (isset($send))
{
    // open client connection to TCP server
	$userip = ($_SERVER['X_FORWARDED_FOR']) ? $_SERVER['X_FORWARDED_FOR'] : $_SERVER['REMOTE_ADDR']; // get actual ip address of user as his id

    $msg = $userip.$null.$bot.$null.$message.$null;

    // fifth parameter in fsockopen is timeout in seconds
    if(!$fp=fsockopen($host,$port,$errstr,$errno,300))
    {
        trigger_error('Error opening socket',E_USER_ERROR);
    }

    // write message to socket server
    fputs($fp,$msg);
    while (!feof($fp))
	{
        $ret .= fgets($fp, 512);
    }

    // close socket connection
    fclose($fp);
    exit($ret);
}
