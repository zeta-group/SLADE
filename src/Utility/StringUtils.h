#pragma once

#include <sstream>

class ArchiveEntry;

namespace StrUtil
{
// Static common strings
static string FULLSTOP              = ".";
static string COMMA                 = ",";
static string COLON                 = ":";
static string SEMICOLON             = ";";
static string SLASH_FORWARD         = "/";
static string SLASH_BACK            = "\\";
static string QUOTE_SINGLE          = "'";
static string QUOTE_DOUBLE          = "\"";
static string CARET                 = "^";
static string ESCAPED_QUOTE_DOUBLE  = "\\\"";
static string ESCAPED_SLASH_BACK    = "\\\\";
static string CURLYBRACE_OPEN       = "{";
static string CURLYBRACE_CLOSE      = "}";
static string DASH                  = "-";
static string WHITESPACE_CHARACTERS = " \t\n\r\f\v";
static string EMPTY                 = "";
static string SPACE                 = " ";
static string UNDERSCORE            = "_";
static string AMPERSAND             = "&";
static string EQUALS                = "=";
static string BOOL_TRUE             = "true";
static string BOOL_FALSE            = "false";

// String comparisons and checks
// CI = Case-Insensitive
bool isInteger(string_view str, bool allow_hex = true);
bool isHex(string_view str);
bool isFloat(string_view str);
bool equalCI(string_view left, string_view right);
bool equalCI(string_view left, const char* right);
bool startsWith(string_view str, string_view check);
bool startsWithCI(string_view str, string_view check);
bool endsWith(string_view str, string_view check);
bool endsWithCI(string_view str, string_view check);
bool contains(string_view str, char check);
bool containsCI(string_view str, char check);
bool contains(string_view str, string_view check);
bool containsCI(string_view str, string_view check);
bool matches(string_view str, string_view check);
bool matchesCI(string_view str, string_view check);

// String transformations
// IP = In-Place
enum TransformOptions
{
	TrimLeft  = 1,
	TrimRight = 2,
	Trim      = 3,
	UpperCase = 4,
	LowerCase = 8
};
string escapedString(string_view str, bool swap_backslash = false);
void   replaceIP(string& str, string_view from, string_view to);
string replace(string_view str, string_view from, string_view to);
void   replaceFirstIP(string& str, string_view from, string_view to);
string replaceFirst(string_view str, string_view from, string_view to);
void   lowerIP(string& str);
void   upperIP(string& str);
string lower(string_view str);
string upper(string_view str);
void   ltrimIP(string& str);
void   rtrimIP(string& str);
void   trimIP(string& str);
string ltrim(string_view str);
string rtrim(string_view str);
string trim(string_view str);
void   capitalizeIP(string& str);
string capitalize(string_view str);
string wildcardToRegex(string_view str);
string prepend(string_view str, string_view prefix);
void   prependIP(string& str, string_view prefix);
string transform(string_view str, int options);

// Substrings
string              afterLast(string_view str, char chr);
string              afterFirst(string_view str, char chr);
string              beforeLast(string_view str, char chr);
string              beforeFirst(string_view str, char chr);
vector<string>      split(string_view str, char separator);
vector<string_view> splitToViews(string_view str, char separator);
string              truncate(string_view str, unsigned length);
void                truncateIP(string& str, unsigned length);
string              removeLast(string_view str, unsigned n);
void                removeLastIP(string& str, unsigned n);

// Misc
void processIncludes(string filename, string& out);
void processIncludes(ArchiveEntry* entry, string& out, bool use_res = true);

// Joins all given args into a single string
template<typename... Args> string join(const Args&... args)
{
	std::ostringstream stream;

	int a[] = { 0, ((void)(stream << args), 0)... };

	return stream.str();
}

// Path class
class Path
{
public:
	Path(string_view full_path);

	const string& fullPath() const { return full_path_; }

	string_view         path(bool include_end_sep = true) const;
	string_view         fileName(bool include_extension = true) const;
	string_view         extension() const;
	vector<string_view> pathParts() const;

	void set(string_view full_path);
	void setPath(string_view path);
	void setPath(const vector<string_view>& parts);
	void setPath(const vector<string>& parts);
	void setFileName(string_view file_name);
	void setExtension(string_view extension);

	// Static functions
	static string_view fileNameOf(string_view full_path, bool include_extension = true);
	static string_view extensionOf(string_view full_path);
	static string_view pathOf(string_view full_path, bool include_end_sep = true);

private:
	string            full_path_;
	string::size_type filename_start_ = string::npos;
	string::size_type filename_end_   = string::npos;
};

} // namespace StrUtil
