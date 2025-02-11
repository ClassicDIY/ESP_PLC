#pragma once

#include "Arduino.h"

// Store HTML content with JavaScript to receive serial log data via WebSocket
const char web_serial_html[] PROGMEM = R"rawliteral(
	<!DOCTYPE html>
	<html>
	<head>
	  <title>ESP32 Serial Log</title>
	  <script>
		var ws;
	
		function initWebSocket() {
		  ws = new WebSocket('ws://' + window.location.hostname + ':7668/');
		  ws.onmessage = function(event) {
			document.getElementById('log').innerText += event.data;
		  };
		}
		
		window.onload = function() {
		  initWebSocket();
		}
	  </script>
	</head>
	<body>
	  <h1>ESP32 Serial Log</h1>
	  <pre>Connecting to WebSocket...</pre><br>
	  <div id="log"></div>
	</body>
	</html>
	)rawliteral";

const char home_html[] PROGMEM = R"rawliteral(
	<!DOCTYPE html><html lang=\"en\">
	<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>
	<title>{n}</title>
	</head><body>
	<h2>{n}</h2>
	<div style='font-size: .6em;'>Firmware config version '{v}'</div>
	<hr><p>
	<div style='padding-top:25px;'>
	<p><a href='settings' onclick="javascript:event.target.port={p}" >View Current Settings</a></p>
	
	</div></body></html>
	)rawliteral";