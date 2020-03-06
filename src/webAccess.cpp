#include "webAccess.h"

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdio.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <uri/UriBraces.h>

WebAccess::WebAccess(std::shared_ptr<Storage> storage) : _server(80), _storage(storage) {
    _server.on("/", HTTP_GET, std::bind(&WebAccess::handleRoot, this));     // Call the 'handleRoot' function when a client requests URI "/"
    _server.on(UriBraces("/recordings/{}.csv"), HTTP_GET, std::bind(&WebAccess::handleRecordingCsv, this));
    _server.on(UriBraces("/recordings/{}.csv/remove"), HTTP_GET, std::bind(&WebAccess::handleRemoveRecording, this));
    _server.onNotFound(std::bind(&WebAccess::handleNotFound, this));        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
    _server.begin(); // Actually start the server
}

WebAccess::~WebAccess() {}

void WebAccess::loop() {
	while (true) {
        _server.handleClient();                    // Listen for HTTP requests from clients
	}
}

void WebAccess::handleRoot() {                         // When URI / is requested, send a web page with a button to list the recordings
    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _server.send(200, "text/html",
        "<html>\n" // defines the whole document
        "   <head>\n" // (not shown) contains meta information about the document
        "       <title>ESP-ISD-ECG</title>\n" // specifies a title for the document
        "   </head>\n"
        "   <body>\n" // defines the document body.
        "       <h1>ESP-ISD-ECG</h1>\n" // heading
        "       <ul>\n" // unordered/bullet list, followed by <li> (List Items, empty tag)
    );

    // WiFiClient client = _server.client();

    for (auto& recordings : _storage->list_recordings()) { // return obj
        // <a href='/recordings/000xx.csv'>000xx</a> --- link & Button
        std::string msg = "<li><a href='/recordings/"; // Links
        msg += recordings.get_name();
        msg += ".csv'>"; 
        msg += recordings.get_name();
        msg += "</a>\n";
        
        msg += "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp\n"; // spaces
        msg += "<a href='/recordings/"; // Links
        msg += recordings.get_name();
        msg += ".csv/remove'>"; 
        msg += "Click here to remove";
        msg += "</a>\n";
        
        _server.sendContent(msg.data()); // method = String, parameter = C-string
    }
    
    _server.sendContent("</ul></body></html>\n");
    
    // Send zero length chunk to terminate the HTTP body
    _server.sendContent("");
}    

void WebAccess::handleRecordingCsv() { // If a POST request is made to URI /recordings/000xx.csv
    //   STORAGE_CHECK_STATE(_state, StorageState::Idle, {});
    //   std::lock_guard<std::mutex> lock(_spi_mutex);

    String recording_name = _server.pathArg(0); // TODO: get 000xx tks to UriBraces

    int len;
	float data[7];
	char fl_to_str[10];

    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _server.send(200, "text/csv", "");
    WiFiClient client = _server.client();

    if (_storage->open_recording(recording_name.c_str())) { // if can open
        do {
            // then read, put to "data" & return length
            len = _storage->read_record(data, 7); 
            
            for (int i = 0; i < len; i++) {
                snprintf(fl_to_str, 9, "%f", data[i]); // float to string
                _server.sendContent(fl_to_str);
                if (i == len - 1) { // for every read_record()
                    _server.sendContent("\n"); 
                } else {
                    _server.sendContent(",");
                }
            }
        } while (len >= 1);
        
        // // Send zero length chunk to terminate the HTTP body
        _server.sendContent("");
        _storage->close_recording();
    } else {
        _server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
    }
}

void WebAccess::handleRemoveRecording() { // If a POST request is made to URI /recordings/000xx.csv
    //   STORAGE_CHECK_STATE(_state, StorageState::Idle, {});
    //   std::lock_guard<std::mutex> lock(_spi_mutex);
    String recording_name = _server.pathArg(0); // TODO: get 000xx tks to UriBraces

    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    // _server.send(200, "text/html",
    //     "<html>\n" // defines the whole document
    //     "   <head>\n" // (not shown) contains meta information about the document
    //     "       <title>Confirmation</title>\n" // specifies a title for the document
    //     "   </head>\n"
    //     "   <body>\n" // defines the document body.
    //     "       <h1>Are you sure you want to delete this file?</h1>\n" // heading
    //     "       <ul>\n" // unordered/bullet list, followed by <li> (List Items, empty tag)
    // );

    // WiFiClient client = _server.client();

    // if (_storage->open_recording(recording_name.c_str())) {}      // if can open
    bool isRemoved = _storage->remove_recording(recording_name.c_str());
    if (!isRemoved)
    {
        _storage->close_recording();
        _server.send(503, "text/plain", "500: Internal Server Error"); // SD cannot delete
        // delay(5000); // wait before redirecting to /
        // log_e already inside remove_recording()
        return;
    }
    _storage->close_recording();
    
    _server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
    _server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}

void WebAccess::handleNotFound() {
    _server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
