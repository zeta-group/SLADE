#pragma once

class Tokenizer
{
public:
	enum CommentTypes
	{
		CStyle     = 1,  // C comments (/* */)
		CPPStyle   = 2,  // C++ comments (//)
		Hash       = 4,  // Hash comments (#)
		DoubleHash = 8,  // Double hash comments (##)
		Shell      = 16, // Shell comments (;)

		Default = CStyle | CPPStyle | DoubleHash,
	};

	struct Token
	{
		string   text;
		unsigned line_no       = 0;
		bool     quoted_string = false;
		unsigned pos_start     = 0;
		unsigned pos_end       = 0;
		unsigned length        = 0;
		bool     valid         = false;

		explicit operator string() const { return text; }
		explicit operator const string() const { return text; }
		explicit operator const char*() const { return text.c_str(); }
		bool     operator==(const string& cmp) const { return text == cmp; }
		bool     operator==(const char* cmp) const { return text == cmp; }
		bool     operator==(char cmp) const { return length == 1 && text[0] == cmp; }
		bool     operator!=(const string& cmp) const { return text != cmp; }
		bool     operator!=(const char* cmp) const { return text != cmp; }
		bool     operator!=(char cmp) const { return length != 1 || text[0] != cmp; }
		char     operator[](unsigned index) const { return text[index]; }

		bool isInteger(bool allow_hex = false) const;
		bool isHex() const;
		bool isFloat() const;

		int    asInt() const;
		bool   asBool() const;
		double asFloat() const;

		void toInt(int& val) const;
		void toBool(bool& val) const;
		void toFloat(double& val) const;
		void toFloat(float& val) const;
	};

	struct TokenizeState
	{
		enum class State
		{
			Unknown,
			Token,
			Comment,
			Whitespace
		};

		State    state        = State::Unknown;
		unsigned position     = 0;
		unsigned size         = 0;
		unsigned current_line = 1;
		unsigned comment_type = 0;
		Token    current_token;
		bool     done = false;
	};

	// Constructors
	Tokenizer(int comments = CommentTypes::Default, const string& special_characters = DEFAULT_SPECIAL_CHARACTERS);

	// Accessors
	const string& source() const { return source_; }
	bool          decorate() const { return decorate_; }
	bool          readLowerCase() const { return read_lowercase_; }
	const Token&  current() const { return token_current_; }
	const Token&  peek() const;

	// Modifiers
	void setCommentTypes(int types) { comment_types_ = types; }
	void setSpecialCharacters(string_view characters)
	{
		memset(special_character_, 0, 256);
		for (char c : characters)
			special_character_[c] = true;
		//special_characters_.assign(characters.begin(), characters.end());
	}
	void setSource(const string& source) { source_ = source; }
	void setReadLowerCase(bool lower) { read_lowercase_ = lower; }
	void enableDecorate(bool enable) { decorate_ = enable; }
	void enableDebug(bool enable) { debug_ = enable; }

	// Token Iterating
	const Token&  next();
	void          adv(int inc = 1);
	bool          advIf(const char* check, int inc = 1);
	bool          advIf(const string& check, int inc = 1);
	bool          advIf(char check, int inc = 1);
	bool          advIfNC(const char* check, int inc = 1);
	bool          advIfNC(const string& check, int inc = 1);
	bool          advIfNext(const char* check, int inc = 1);
	bool          advIfNext(const string& check, int inc = 1);
	bool          advIfNext(char check, int inc = 1);
	bool          advIfNextNC(const char* check, int inc = 1);
	void          advToNextLine();
	void          advToEndOfLine();
	void          skipSection(const char* begin, const char* end, bool allow_quoted = false);
	vector<Token> getTokensUntil(const char* end);
	vector<Token> getTokensUntilNC(const char* end);
	vector<Token> getTokensUntilNextLine(bool from_start = false);
	string        getLine(bool from_start = false);

	// Token Checking
	bool check(const char* check) const { return token_current_ == check; }
	bool check(const string& check) const { return token_current_ == check; }
	bool check(char check) const { return token_current_ == check; }
	bool checkOrEnd(const char* check) const;
	bool checkOrEnd(const string& check) const;
	bool checkOrEnd(char check) const;
	bool checkNC(const char* check) const;
	bool checkOrEndNC(const char* check) const;
	bool checkNext(const char* check) const;
	bool checkNext(const string& check) const;
	bool checkNext(char check) const;
	bool checkNextNC(const char* check) const;

	// Load Data
	bool openFile(string_view filename, unsigned offset = 0, unsigned length = 0);
	bool openString(string_view text, unsigned offset = 0, unsigned length = 0, string_view source = "unknown");
	bool openMem(const char* mem, unsigned length, string_view source);
	bool openMem(const MemChunk& mc, string_view source);

	// General
	bool isSpecialCharacter(char p) const { return special_character_[p]; }
	bool atEnd() const { return !token_next_.valid; }
	void reset();

	// Old tokenizer interface bridge (don't use)
	string getToken()
	{
		if (atEnd())
			return "";
		string t = token_current_.text;
		adv();
		return t;
	}
	void getToken(string* str)
	{
		if (atEnd())
			*str = "";
		else
			*str = token_current_.text;
		adv();
	}
	string peekToken() const
	{
		if (atEnd())
			return "";
		return token_next_.text;
	}
	int getInteger()
	{
		if (atEnd())
			return 0;
		int v = token_current_.asInt();
		adv();
		return v;
	}
	double getDouble()
	{
		if (atEnd())
			return 0;
		double v = token_current_.asFloat();
		adv();
		return v;
	}
	double getFloat()
	{
		if (atEnd())
			return 0;
		double v = token_current_.asFloat();
		adv();
		return v;
	}
	void skipToken() { adv(); }
	bool checkToken(const string& cmp)
	{
		next();
		return check(cmp);
	}
	unsigned lineNo() const { return token_current_.line_no; }
	unsigned tokenEnd() const { return token_current_.pos_end; }


	static const string DEFAULT_SPECIAL_CHARACTERS;
	static const Token& invalidToken() { return invalid_token_; }

private:
	vector<char>  data_;
	Token         token_current_;
	Token         token_next_;
	TokenizeState state_;

	// Configuration
	int          comment_types_;      // Types of comments to skip
	//vector<char> special_characters_; // These will always be read as separate tokens
	bool         special_character_[256]; // Table of char values that are considered special characters
	string       source_;             // What file/entry/chunk is being tokenized
	bool         decorate_;           // Special handling for //$ comments
	bool         read_lowercase_;     // If true, tokens will all be read in lowercase
									  // (except for quoted strings, obviously)
	bool debug_;                      // Log each token read

	// Static
	static Token invalid_token_;

	// Tokenizing
	unsigned checkCommentBegin();
	void     tokenizeUnknown();
	void     tokenizeToken();
	void     tokenizeComment();
	void     tokenizeWhitespace();
	bool     readNext(Token* target);
	bool     readNext() { return readNext(&token_next_); }
	void     resetToLineStart();
};
