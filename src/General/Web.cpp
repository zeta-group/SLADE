
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Web.cpp
// Description: Utility functions for dealing with web requests
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "Web.h"
#include <SFML/Network.hpp>
#include <thread>

wxDEFINE_EVENT(wxEVT_THREAD_WEBGET_COMPLETED, wxThreadEvent);


// -----------------------------------------------------------------------------
//
// Web Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Sends a HTTP GET request to [host]/[uri] and returns the response (blocking)
// -----------------------------------------------------------------------------
string Web::getHttp(const char* host, const char* uri)
{
	// Setup connection & request
	sf::Http          http(host);
	sf::Http::Request request;
	request.setMethod(sf::Http::Request::Get);
	request.setUri(uri);

	// Send HTTP request
	auto response = http.sendRequest(request);

	switch (response.getStatus())
	{
	case sf::Http::Response::Ok: return response.getBody();
	default: return "connect_failed";
	}
}

// -----------------------------------------------------------------------------
// Sends a HTTP GET request to [host]/[uri].
// Runs asynchronously and sends a wxEVT_THREAD_WEBGET_COMPLETED event to
// [event_handler] with the response when done
// -----------------------------------------------------------------------------
void Web::getHttpAsync(const char* host, const char* uri, wxEvtHandler* event_handler)
{
	std::thread thread([=]() {
		// Queue wx event with http request response
		auto event = new wxThreadEvent(wxEVT_THREAD_WEBGET_COMPLETED);
		event->SetString(getHttp(host, uri));
		wxQueueEvent(event_handler, event);
	});

	thread.detach();
}
