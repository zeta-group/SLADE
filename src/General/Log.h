#pragma once

namespace Log
{
	enum class MessageType
	{
		Info,
		Warning,
		Error,
		Debug,
		Console,	// Only displayed in the console
		Script,		// Script output
		Any
	};

	struct Message
	{
		string		message;
		MessageType	type;
		time_t		timestamp;

		string	formattedMessageLine() const;
	};

	const vector<Message>&	history();
	int						verbosity();

	void	setVerbosity(int verbosity);

	void	init();

	void		message(MessageType type, int level, const char* text);
	void		message(MessageType type, const char* text);
	inline void	message(MessageType type, int level, const string& text) { message(type, level, text.c_str()); }
	inline void	message(MessageType type, const string& text) { message(type, text.c_str()); }

	vector<Message*>	since(time_t time, MessageType type = MessageType::Any);

	inline void	info(int level, const char* text) { message(MessageType::Info, level, text); }
	inline void	info(const char* text) { message(MessageType::Info, text); }
	inline void	info(int level, const string& text) { message(MessageType::Info, level, text); }
	inline void	info(const string& text) { message(MessageType::Info, text); }

	inline void	warning(int level, const char* text) { message(MessageType::Warning, level, text); }
	inline void	warning(const char* text) { message(MessageType::Warning, text); }
	inline void	warning(int level, const string& text) { message(MessageType::Warning, level, text); }
	inline void	warning(const string& text) { message(MessageType::Warning, text); }

	inline void	error(int level, const char* text) { message(MessageType::Error, level, text); }
	inline void	error(const char* text) { message(MessageType::Error, text); }
	inline void	error(int level, const string& text) { message(MessageType::Error, level, text); }
	inline void	error(const string& text) { message(MessageType::Error, text); }

	inline void	debug(int level, const char* text) { message(MessageType::Debug, level, text); }
	inline void	debug(const char* text) { message(MessageType::Debug, text); }
	inline void	debug(int level, const string& text) { message(MessageType::Debug, level, text); }
	inline void	debug(const string& text) { message(MessageType::Debug, text); }

	inline void	console(const char* text) { message(MessageType::Console, text); }
	inline void	console(const string& text) { message(MessageType::Console, text); }
}

// Try to avoid using these and use Log::message/error/warning with S_FMT instead
#define LOG_MESSAGE(level, ...) Log::message(Log::MessageType::Info, level, formatString(__VA_ARGS__))
#define LOG_WARNING(level, ...) Log::message(Log::MessageType::Warning, level, formatString(__VA_ARGS__))
#define LOG_ERROR(level, ...) Log::message(Log::MessageType::Error, level, formatString(__VA_ARGS__))
// move LOG_DEBUG here?
