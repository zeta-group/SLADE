#pragma once

class TextEditorCtrl;
class TextLanguage;
class Lexer
{
public:
	Lexer();
	virtual ~Lexer() = default;

	enum Style
	{
		Default      = wxSTC_STYLE_DEFAULT,
		Comment      = wxSTC_C_COMMENT,
		CommentDoc   = wxSTC_C_COMMENTDOC,
		String       = wxSTC_C_STRING,
		Char         = wxSTC_C_CHARACTER,
		Number       = wxSTC_C_NUMBER,
		Operator     = wxSTC_C_OPERATOR,
		Preprocessor = wxSTC_C_PREPROCESSOR,

		// Words
		Keyword  = wxSTC_C_WORD,
		Function = wxSTC_C_WORD2,
		Constant = wxSTC_C_GLOBALCLASS,
		Type     = wxSTC_C_IDENTIFIER,
		Property = wxSTC_C_UUID,
	};

	virtual void loadLanguage(TextLanguage* language);

	virtual bool doStyling(TextEditorCtrl* editor, int start, int end);
	virtual void addWord(const string& word, int style);
	virtual void clearWords() { word_list_.clear(); }

	void setWordChars(string_view chars);
	void setOperatorChars(string_view chars);

	void updateFolding(TextEditorCtrl* editor, int line_start);
	void foldComments(bool fold) { fold_comments_ = fold; }
	void foldPreprocessor(bool fold) { fold_preprocessor_ = fold; }

	virtual bool isFunction(TextEditorCtrl* editor, int start_pos, int end_pos);

protected:
	enum class State
	{
		Unknown,
		Word,
		Comment,
		String,
		Char,
		Number,
		Operator,
		Whitespace,
	};

	vector<char>  word_chars_;
	vector<char>  operator_chars_;
	vector<char>  whitespace_chars_  = { ' ', '\n', '\r', '\t' };
	TextLanguage* language_          = nullptr;
	bool          fold_comments_     = false;
	bool          fold_preprocessor_ = false;
	char          preprocessor_char_ = '#';

	// Language strings
	string comment_begin_;
	string comment_doc_;
	string comment_line_;
	string block_begin_;
	string block_end_;
	string comment_block_end_;

	struct WLIndex
	{
		char style;
		WLIndex() : style(0) {}
	};
	std::map<string, WLIndex> word_list_;

	struct LineInfo
	{
		bool commented;
		int  fold_increment;
		bool has_word;
		LineInfo() : commented{ false }, fold_increment{ 0 }, has_word{ false } {}
	};
	std::map<int, LineInfo> lines_;

	struct LexerState
	{
		int             position;
		int             end;
		int             line;
		State           state;
		int             length;
		int             fold_increment;
		bool            has_word;
		TextEditorCtrl* editor;
	};
	bool processUnknown(LexerState& state);
	bool processComment(LexerState& state) const;
	bool processWord(LexerState& state);
	bool processString(LexerState& state) const;
	bool processChar(LexerState& state) const;
	bool processOperator(LexerState& state);
	bool processWhitespace(LexerState& state);

	virtual void styleWord(LexerState& state, string word);
	static bool  checkToken(LexerState& state, int pos, const string& token);
};

class ZScriptLexer : public Lexer
{
public:
	ZScriptLexer()          = default;
	virtual ~ZScriptLexer() = default;

protected:
	void addWord(const string& word, int style) override;
	void styleWord(LexerState& state, string word) override;
	void clearWords() override;
	bool isFunction(TextEditorCtrl* editor, int start_pos, int end_pos) override;

private:
	vector<string> functions_;
};
