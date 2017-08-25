//
// ESP_IRKit.ino
//
// Copyright (c) 2017 tako2
//

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

extern "C" {
#include <json/jsonparse.h>
}

#include "irsignal.h"

// WiFi Router SSID and Password
const char *m_SSID = "Your WiFi SSID";
const char *m_Password = "Your WiFi Password";

// Web Server
#define WEB_SERVER_PORT 80
ESP8266WebServer m_Server(WEB_SERVER_PORT);

// Hostname
const char *m_HOSTNAME = "esp_irkit";

IRSignal m_IRSignal;

//=============================================================================
// Setup
//=============================================================================
void setup()
{
  Serial.begin(115200);

  WiFi.begin(m_SSID, m_Password);
  Serial.println("");

  // Connect to WiFi Router
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(m_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start mDNS
  if (MDNS.begin(m_HOSTNAME)) {
    Serial.print("mDNS started (name is \"");
    Serial.print(m_HOSTNAME);
    Serial.println("\")");
    MDNS.addService("irkit", "tcp", WEB_SERVER_PORT);
  }

  // Start Web Server
  m_Server.on("/", handleRoot);
  m_Server.on("/messages", HTTP_POST, handleSend);
  m_Server.on("/messages", HTTP_GET, handleRecv);
  m_Server.on("/keys", HTTP_POST, handleKeys);
  m_Server.onNotFound(handleNotFound);
  m_Server.begin();
  Serial.println("HTTP server started");

  pinMode(IRLEDPIN, OUTPUT);
  pinMode(IRPIN, INPUT);

  pinMode(13, OUTPUT);
}

//=============================================================================
// Loop
//=============================================================================
void loop()
{
  m_Server.handleClient();
}

//=============================================================================
// Handle Root
//=============================================================================
void handleRoot()
{
  const char *message = 
"<html>\
  <head>\
    <title>ESP IRKit Demo</title>\
  <script>\
function createHttpRequest() {\
  if (window.ActiveXObject) { \
    try {\
      return new ActiveXObject(\"Msxml2.XMLHTTP\");\
    } catch (e) {\
      try {\
        return new ActiveXObject(\"Microsoft.XMLHTTP\");\
      } catch (e2) {\
        return null;\
      }\
    }\
  } else if (window.XMLHttpRequest) {\
    try {\
      return new XMLHttpRequest();\
    } catch (e3) {\
      return null;\
    }\
  } else {\
    return null;\
  }\
}\
function send() {\
  var httpObj = createHttpRequest();\
  if (httpObj != null) {\
    document.getElementById(\"message\").innerHTML = \"Sending...\";\
    httpObj.open(\"POST\", \"/messages\", true);\
    httpObj.onreadystatechange = function() {\
      if (httpObj.readyState == 4 && httpObj.status == 200) {\
        send_result(httpObj.responseText);\
      }\
    };\
    httpObj.setRequestHeader(\"Content-type\", \"application/json\");\
    httpObj.send(document.demo.irdata.value);\
  }\
}\
function send_result(resText) {\
  document.getElementById(\"message\").innerHTML = \"Done\";\
}\
function recv() {\
  var httpObj = createHttpRequest();\
  if (httpObj != null) {\
    document.getElementById(\"message\").innerHTML = \"Wait Receiving...\";\
    httpObj.open(\"GET\", \"/messages\", true);\
    httpObj.onreadystatechange = function() {\
      if (httpObj.readyState == 4 && httpObj.status == 200) {\
        recv_result(httpObj.responseText);\
      }\
    };\
    httpObj.send(null);\
  }\
}\
function recv_result(resText) {\
  document.demo.irdata.value = resText;\
  document.getElementById(\"message\").innerHTML = \"Done\";\
}\
  </script>\
  </head>\
  <body>\
    <h1>ESP IRKit DEMO</h1>\
    <form name=\"demo\">\
    <textarea name=\"irdata\" cols=\"80\" rows=\"20\"></textarea><br />\
    <input type=\"button\" value=\"Receive\" onClick=\"recv()\">\
    <input type=\"button\" value=\"Send\" onClick=\"send()\">\
    </form>\
    <br />\
    <div id=\"message\"> </div>\
    <br />\
  </body>\
</html>";

  m_Server.send(200, "text/html", message);
}

//=============================================================================
// Handle Send IR (POST /messages)
//=============================================================================
void handleSend()
{
  if (m_Server.args() < 1 || m_Server.argName(0) != "plain") {
    handleNotFound();
    return;
  }

  jsonparse_state state;
  int res;
  int value_type;
  char token[256];
  long value;

  char *json_text = new char[m_Server.arg(0).length() + 1];

  long freq;
  vector<long> *waveform = new vector<long>();
  int tag = 0;

  if (json_text == NULL || waveform == NULL) {
    if (json_text != NULL) {
      delete json_text;
    }
    if (waveform != NULL) {
      delete waveform;
    }
    m_Server.send(200, "application/json", "{\"status\":\"NG\"}");
    return;
  }

  memcpy(json_text, m_Server.arg(0).c_str(), m_Server.arg(0).length());
  json_text[m_Server.arg(0).length()] = '\0';

  jsonparse_setup(&state, json_text, strlen(json_text));

  while ((res = jsonparse_next(&state)) != JSON_TYPE_ERROR) {
    switch (res) {
    case JSON_TYPE_ARRAY: 
      //Serial.print("ARRAY ");
      //Serial.println(char(res));
      break;
    case JSON_TYPE_OBJECT:
      //Serial.print("OBJECT ");
      //Serial.println(char(res));
      break;
    case JSON_TYPE_PAIR:
      //Serial.print("PAIR ");
      //Serial.println(char(res));
      break;
    case JSON_TYPE_PAIR_NAME:
      //Serial.print("PAIR_NAME ");
      value_type = jsonparse_copy_value(&state, token, 255);
      //Serial.println(String('"') + token + '"');
      if (strcmp(token, "format") == 0) {
	tag = 1;
      } else if (strcmp(token, "freq") == 0) {
	tag = 2;
      } else if (strcmp(token, "data") == 0) {
	tag = 3;
      } else {
	tag = 0;
      }
      break;
    case JSON_TYPE_STRING:
      //Serial.print("STRING ");
      value_type = jsonparse_copy_value(&state, token, 255);
      //Serial.println(String('"') + token + '"');
      if (tag == 1) {
	if (strcmp(token, "raw") == 0) {
	  // format is OK
	} else {
	  // format is NG
	}
      }
      break;
    case JSON_TYPE_INT:
      //Serial.print("INT ");
      //value = jsonparse_get_value_as_long(&state);
      //Serial.println(value, DEC);
      break;
    case JSON_TYPE_NUMBER:
      //Serial.print("NUMBER ");
      value = jsonparse_get_value_as_long(&state);
      //Serial.println(value, DEC);
      if (tag == 2) {
	freq = value * 1000; // kHz => Hz
      } else if (tag == 3) {
	waveform->push_back(value / 2); // IRKit counts with 2MHz
      }
      break;
    case JSON_TYPE_NULL:
      //Serial.println("null");
      break;
    case JSON_TYPE_TRUE:
      //Serial.println("true");
      break;
    case JSON_TYPE_FALSE:
      //Serial.println("false");
      break;
    default:
      //Serial.print("unknown ");
      //Serial.println(char(res));
      break;
    }
  }
  if (res == JSON_TYPE_ERROR) {
    String errStr;
    switch(state.error) {
      case JSON_ERROR_OK: errStr="OK"; break;
      case JSON_ERROR_SYNTAX: errStr="Syntax Error"; break;
      case JSON_ERROR_UNEXPECTED_ARRAY: errStr="Unexpected array"; break;
      case JSON_ERROR_UNEXPECTED_END_OF_ARRAY: errStr="Unexpected end of array"; break;
      case JSON_ERROR_UNEXPECTED_OBJECT: errStr="Unexpected object"; break;
      case JSON_ERROR_UNEXPECTED_STRING: errStr="Unexpected string"; break;
    }
    Serial.println(errStr);
  }

  delete json_text;

  // Send IR Signal
  m_IRSignal.set_carrier(freq);
  m_IRSignal.send(waveform);

  delete waveform;

  m_Server.send(200, "application/json", "{\"status\":\"OK\"}");
}

//=============================================================================
// Handle Receive IR (GET /messages)
//=============================================================================
void handleRecv()
{
  vector<long> *waveform;

  waveform = m_IRSignal.recv();
  if (waveform != NULL) {
    String message = "{\"format\":\"raw\",\"freq\":38,\"data\":[";

    for (int idx = 0; idx < waveform->size(); idx ++) {
      message += (waveform->at(idx) * 2); // IRKit counts with 2MHz
      if ((idx + 1) < waveform->size()) {
	message += ",";
      }
    }
    message += "]}";
    delete waveform;

    m_Server.send(200, "application/json", message);
  } else {
    m_Server.send(200, "application/json",
		  "{\"format\":\"raw\",\"freq\":38,\"data\":[]}");
  }
}

//=============================================================================
// Get Client Token (POST /keys)
//=============================================================================
void handleKeys()
{
  m_Server.send(200, "application/json",
		"{\"clienttoken\":\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXX\"}");
}

//=============================================================================
// Not Found
//=============================================================================
void handleNotFound()
{
  String message = "File Not Found\n\n";

  message += "URI: ";
  message += m_Server.uri();
  message += "\nMethod: ";
  message += ( m_Server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += m_Server.args();
  message += "\n";

  for (uint8_t i = 0; i < m_Server.args(); i++) {
    message += " " + m_Server.argName (i) + ": " + m_Server.arg (i) + "\n";
  }

  Serial.println(message);

  m_Server.send(404, "text/plain", message);
}
