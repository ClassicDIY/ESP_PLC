#pragma once

#include "Arduino.h"

const char home_html[] PROGMEM = R"rawliteral(
	<!DOCTYPE html><html lang=\"en\">
	<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>
	<title>{n}</title>

	<script>

		function initWebSocket() {
		  const socket = new WebSocket('ws://' + window.location.hostname + ':{hp}');
		  socket.onmessage = function(event) {
				const gpioValues = JSON.parse(event.data);
				for (const [key, value] of Object.entries(gpioValues)) {
					// console.log(`Pin: ${key}, State: ${value}`);
					const el = document.getElementById(`${key}`);
					if (el) {
						const isNumeric = !isNaN(`${value}`);
						if (isNumeric) {
							el.innerText = `${key}: ${value}`;
							el.style.backgroundColor = 'yellow';
						} else {
							if (`${value}` === `High` || `${value}` === `On`) {
								el.style.backgroundColor = 'green';
							} else {
								el.style.backgroundColor = 'red';
							}
						}
					}
				}
			};

			socket.onerror = function(error) {
      			console.error('WebSocket error:', error);
    		};

			window.addEventListener('beforeunload', function() {
				if (socket) {
					socket.close();
				}
			});
		}

		window.onload = function() {
		  initWebSocket();
		}

	</script>
	</head>
	
    <style>
        .box {
            width: 100px;
            height: 25px;
            margin: 5px;
            // display: inline-block;
			background-color:grey;
			display: inline-flex; /* Changed to inline-flex to allow the use of Flexbox */
			align-items: center;  /* Vertically center the content */
			justify-content: center; /* Horizontally center the content */
			border: 1px solid #000; /* Added a border for better visibility */
        }
    </style>
	<body>
	<h2>{n}</h2>
	<div style='font-size: .6em;'>Firmware config version '{v}'</div>
	<hr>
	
    <div id="boxes-container">
		Digital Inputs:
		{digitalInputs}
		<hr>
		Analog Inputs:
		{analogInputs}
		<hr>
		Digital Outputs:
		{digitalOutputs}
		<hr>
    </div>
	<p>
	<div style='padding-top:25px;'>
	<p><a href='settings' onclick="javascript:event.target.port={cp}" >View Current Settings</a></p>
	
	</div></body></html>
	)rawliteral";