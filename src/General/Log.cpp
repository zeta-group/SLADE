
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Log.cpp
// Description: The SLADE logging implementation
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
#include "App.h"
#include "External/fmt/time.h"
#include <fstream>


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace Log
{
vector<Message> log;
std::ofstream   log_file;
} // namespace Log
CVAR(Int, log_verbosity, 1, CVAR_SAVE)


// -----------------------------------------------------------------------------
// Formatter for fmt so that Log::MessageType can be written to a string
// -----------------------------------------------------------------------------
namespace Log
{
void format_arg(fmt::BasicFormatter<char>& f, const char*& format_str, const Log::MessageType& s)
{
	switch (s)
	{
	case Log::MessageType::Info: f.writer().write(" [Info]"); break;
	case Log::MessageType::Warning: f.writer().write(" [Warn]"); break;
	case Log::MessageType::Error: f.writer().write("[Error]"); break;
	case Log::MessageType::Debug: f.writer().write("[Debug]"); break;
	case Log::MessageType::Script: f.writer().write("[Script]"); break;
	default: f.writer().write("  [Log]"); break;
	}
}
}

// -----------------------------------------------------------------------------
//
// FreeImage Error Handler
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Allows us to catch FreeImage errors and log them
// -----------------------------------------------------------------------------
void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char* message)
{
	string error = "FreeImage: ";
	if (fif != FIF_UNKNOWN)
		error += fmt::format("[{}] ", FreeImage_GetFormatFromFIF(fif));
	error += message;

	Log::error(error);
}


// -----------------------------------------------------------------------------
//
// Log::Message Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the log entry as a formatted string:
// HH:MM:SS: <message>
// -----------------------------------------------------------------------------
string Log::Message::formattedMessageLine() const
{
	return fmt::format("{:%H:%M:%S}: {} {}", timestamp, type, message);
}


// -----------------------------------------------------------------------------
//
// Log Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Initialises the log file and logging stuff
// -----------------------------------------------------------------------------
void Log::init()
{
	// Redirect sf::err output to the log file
	log_file.open(App::path("slade3.log", App::Dir::User));
	sf::err().rdbuf(log_file.rdbuf());

	// Write logfile header
	auto t = std::time(nullptr);
	auto tm = std::localtime(&t);
	info("SLADE - It's a Doom Editor");
	info(fmt::format("Version {}", Global::version));
	if (!Global::sc_rev.empty())
		info(fmt::format("Git Revision {}", Global::sc_rev));
	info(fmt::format("Written by Simon Judd, 2008-{:%Y}", *tm));
#ifdef SFML_VERSION_MAJOR
	info(fmt::format(
		"Compiled with wxWidgets {}.{}.{} and SFML {}.{}.{}",
		wxMAJOR_VERSION,
		wxMINOR_VERSION,
		wxRELEASE_NUMBER,
		SFML_VERSION_MAJOR,
		SFML_VERSION_MINOR,
		SFML_VERSION_PATCH));
#else
	info(fmt::format("Compiled with wxWidgets {}.{}.{}", wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER));
#endif
	info("--------------------------------");

	// Set up FreeImage to use our log:
	FreeImage_SetOutputMessage(FreeImageErrorHandler);
}

// -----------------------------------------------------------------------------
// Returns the log message history
// -----------------------------------------------------------------------------
const vector<Log::Message>& Log::history()
{
	return log;
}

// -----------------------------------------------------------------------------
// Returns the current log verbosity level, log messages with a higher level
// than the current verbosity will not be logged
// -----------------------------------------------------------------------------
int Log::verbosity()
{
	return log_verbosity;
}

// -----------------------------------------------------------------------------
// Sets the log verbosity level to [verbosity]
// -----------------------------------------------------------------------------
void Log::setVerbosity(int verbosity)
{
	log_verbosity = verbosity;
}

// -----------------------------------------------------------------------------
// Logs a message [text] of [type]
// -----------------------------------------------------------------------------
void Log::message(MessageType type, const char* text)
{
	// Add log message
	auto t = std::time(nullptr);
	log.push_back({ text, type, *std::localtime(&t) });

	// Write to log file
	if (log_file.is_open() && type != MessageType::Console)
		sf::err() << log.back().formattedMessageLine() << "\n";
}

// -----------------------------------------------------------------------------
// Returns a list of log messages of [type] that have been recorded since [time]
// -----------------------------------------------------------------------------
vector<Log::Message*> Log::since(time_t time, MessageType type)
{
	vector<Message*> list;
	for (auto& msg : log)
		if (mktime(&msg.timestamp) >= time && (type == MessageType::Any || msg.type == type))
			list.push_back(&msg);
	return list;
}

// -----------------------------------------------------------------------------
// Logs a debug message [text] at verbosity [level] only if debug mode is on
// -----------------------------------------------------------------------------
void Log::debug(int level, const char* text)
{
	if (Global::debug)
		message(MessageType::Debug, level, text);
}

// -----------------------------------------------------------------------------
// Logs a debug message [text] only if debug mode is on
// -----------------------------------------------------------------------------
void Log::debug(const char* text)
{
	if (Global::debug)
		message(MessageType::Debug, text);
}

// -----------------------------------------------------------------------------
// Logs a message [text] of [type] at verbosity [level]
// -----------------------------------------------------------------------------
void Log::message(MessageType type, int level, const char* text)
{
	if (level > log_verbosity)
		return;

	// Add log message
	auto t = std::time(nullptr);
	log.push_back({ text, type, *std::localtime(&t) });

	// Write to log file
	if (log_file.is_open() && type != MessageType::Console)
		sf::err() << log.back().formattedMessageLine() << "\n";
}
