#pragma once

class ArchiveEntry;

namespace StringUtils
{
	// Static common strings
	static string	FULLSTOP = ".";
	static string	COMMA = ",";
	static string	COLON = ":";
	static string	SEMICOLON = ";";
	static string	SLASH_FORWARD = "/";
	static string	SLASH_BACK = "\\";
	static string	QUOTE_SINGLE = "'";
	static string	QUOTE_DOUBLE = "\"";
	static string	CARET = "^";
	static string	ESCAPED_QUOTE_DOUBLE = "\\\"";
	static string	ESCAPED_SLASH_BACK = "\\\\";
	static string	CURLYBRACE_OPEN = "{";
	static string	CURLYBRACE_CLOSE = "}";
	static string	WHITESPACE_CHARACTERS = " \t\n\r\f\v";

	string	escapedString(const string& str, bool swap_backslash = false);

	void	processIncludes(string filename, string& out);
	void	processIncludes(ArchiveEntry* entry, string& out, bool use_res = true);

	bool	isInteger(const string& str, bool allow_hex = true);
	bool	isHex(const string& str);
	bool	isFloat(const string& str);

	bool	startsWith(const string& str, const string& check);

	void	replace(string& str, const string& from, const string& to);

	inline void		lower(string& str) { transform(str.begin(), str.end(), str.begin(), ::tolower); }
	inline void		upper(string& str) { transform(str.begin(), str.end(), str.begin(), ::toupper); }
	inline string	lowerCopy(const string& str) { auto s = str; lower(s); return s; }
	inline string	upperCopy(const string& str) { auto s = str; upper(s); return s; }

	void			ltrim(string& str);
	void			rtrim(string& str);
	inline void		trim(string& str) { ltrim(str); rtrim(str); }
	inline string	ltrimCopy(const string& str) { auto s = str; ltrim(s); return s; }
	inline string	rtrimCopy(const string& str) { auto s = str; rtrim(s); return s; }
	inline string	trimCopy(const string& str) { auto s = str; trim(s); return s; }
}
