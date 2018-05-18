
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Lexer.cpp
// Description: A lexer class to handle syntax highlighting and code folding
//              for the text editor
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
#include "Lexer.h"
#include "TextLanguage.h"
#include "UI/TextEditorCtrl.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, debug_lexer, false, CVAR_SECRET)


// -----------------------------------------------------------------------------
//
// Lexer Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Lexer class constructor
// -----------------------------------------------------------------------------
Lexer::Lexer()
{
	// Default word characters
	setWordChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");

	// Default operator characters
	setOperatorChars("+-*/=><|~&!");
}

// -----------------------------------------------------------------------------
// Loads settings and word lists from [language]
// -----------------------------------------------------------------------------
void Lexer::loadLanguage(TextLanguage* language)
{
	language_ = language;
	clearWords();

	if (!language)
	{
		comment_begin_     = "";
		comment_doc_       = "";
		comment_line_      = "";
		comment_block_end_ = "";
		block_begin_       = "";
		block_end_         = "";

		return;
	}

	// Load language words
	for (const auto& word : language->wordList(TextLanguage::WordType::Constant))
		addWord(word, Lexer::Style::Constant);
	for (const auto& word : language->wordList(TextLanguage::WordType::Property))
		addWord(word, Lexer::Style::Property);
	for (const auto& word : language->functionsList())
		addWord(word, Lexer::Style::Function);
	for (const auto& word : language->wordList(TextLanguage::WordType::Type))
		addWord(word, Lexer::Style::Type);
	for (const auto& word : language->wordList(TextLanguage::WordType::Keyword))
		addWord(word, Lexer::Style::Keyword);

	// Load language info
	preprocessor_char_ = language->preprocessor().empty() ? 0 : language->preprocessor()[0];
	comment_begin_     = language_->commentBegin();
	comment_doc_       = language_->docComment();
	comment_line_      = language_->lineComment();
	comment_block_end_ = language_->commentEnd();
	block_begin_       = language_->blockBegin();
	block_end_         = language_->blockEnd();
}

// -----------------------------------------------------------------------------
// Performs text styling on [editor], for characters from [start] to [end].
// Returns true if the next line needs to be styled (eg. for multi-line
// comments)
// -----------------------------------------------------------------------------
bool Lexer::doStyling(TextEditorCtrl* editor, int start, int end)
{
	if (start < 0)
		start = 0;

	int        line = editor->LineFromPosition(start);
	LexerState state{ start, end, line, lines_[line].commented ? State::Comment : State::Unknown, 0, 0, false, editor };

	editor->StartStyling(start, 31);
	if (debug_lexer)
		Log::debug(S_FMT("START STYLING FROM %d TO %d (LINE %d)", start, end, line + 1));

	bool done = false;
	while (!done)
	{
		switch (state.state)
		{
		case State::Whitespace: done = processWhitespace(state); break;
		case State::Comment: done = processComment(state); break;
		case State::String: done = processString(state); break;
		case State::Char: done = processChar(state); break;
		case State::Word: done = processWord(state); break;
		case State::Operator: done = processOperator(state); break;
		default: done = processUnknown(state); break;
		}
	}

	// Set current & next line's info
	lines_[line].fold_increment = state.fold_increment;
	lines_[line].has_word       = state.has_word;
	lines_[line + 1].commented  = (state.state == State::Comment);

	// Return true if we are still inside a comment
	return (state.state == State::Comment);
}

// -----------------------------------------------------------------------------
// Sets the [style] for [word]
// -----------------------------------------------------------------------------
void Lexer::addWord(const string& word, int style)
{
	word_list_[language_->caseSensitive() ? word : StrUtil::lower(word)].style = style;
}

// -----------------------------------------------------------------------------
// Applies a style to [word] in [editor], depending on if it is in the word
// list, a number or begins with the preprocessor character
// -----------------------------------------------------------------------------
void Lexer::styleWord(LexerState& state, string word)
{
	if (!language_->caseSensitive())
		StrUtil::lowerIP(word);

	if (word_list_[word].style > 0)
		state.editor->SetStyling(word.length(), word_list_[word].style);
	else if (StrUtil::startsWith(word, language_->preprocessor()))
		state.editor->SetStyling(word.length(), Style::Preprocessor);
	else
	{
		// Check for number
		if (StrUtil::isInteger(word) || StrUtil::isFloat(word))
			state.editor->SetStyling(word.length(), Style::Number);
		else
			state.editor->SetStyling(word.length(), Style::Default);
	}
}

// -----------------------------------------------------------------------------
// Sets the valid word characters to [chars]
// -----------------------------------------------------------------------------
void Lexer::setWordChars(string_view chars)
{
	word_chars_.clear();
	for (char c : chars)
		word_chars_.push_back(c);
}

// -----------------------------------------------------------------------------
// Sets the valid operator characters to [chars]
// -----------------------------------------------------------------------------
void Lexer::setOperatorChars(string_view chars)
{
	operator_chars_.clear();
	for (char c : chars)
		operator_chars_.push_back(c);
}

// -----------------------------------------------------------------------------
// Process unknown characters, updating [state].
// Returns true if the end of the current text range was reached
// -----------------------------------------------------------------------------
bool Lexer::processUnknown(LexerState& state)
{
	int  u_length = 0;
	bool end      = false;
	bool pp       = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		char c = state.editor->GetCharAt(state.position);

		// Start of string
		if (c == '"')
		{
			state.state = State::String;
			state.position++;
			state.length   = 1;
			state.has_word = true;
			break;
		}

		// No language set, only process strings
		else if (!language_)
		{
			u_length++;
			state.position++;
			continue;
		}

		// Start of char
		else if (c == '\'')
		{
			state.state = State::Char;
			state.position++;
			state.length   = 1;
			state.has_word = true;
			break;
		}

		// Start of doc line comment
		else if (checkToken(state, state.position, comment_doc_))
		{
			// Format as comment to end of line
			state.editor->SetStyling(u_length, Style::Default);
			state.editor->SetStyling((state.end - state.position) + 1, Style::CommentDoc);
			return true;
		}

		// Start of line comment
		else if (checkToken(state, state.position, comment_line_))
		{
			// Format as comment to end of line
			state.editor->SetStyling(u_length, Style::Default);
			state.editor->SetStyling((state.end - state.position) + 1, Style::Comment);
			return true;
		}

		// Start of block comment
		else if (checkToken(state, state.position, comment_begin_))
		{
			state.state = State::Comment;
			state.position += comment_begin_.size();
			state.length = comment_begin_.size();
			if (fold_comments_)
			{
				state.fold_increment++;
				state.has_word = true;
			}
			break;
		}

		// Whitespace
		else if (VECTOR_EXISTS(whitespace_chars_, c))
		{
			state.state = State::Whitespace;
			state.position++;
			state.length = 1;
			break;
		}

		// Preprocessor
		else if (c == language_->preprocessor()[0])
		{
			pp = true;
			u_length++;
			state.position++;
			continue;
		}

		// Operator
		else if (VECTOR_EXISTS(operator_chars_, c))
		{
			state.position++;
			state.state    = State::Operator;
			state.length   = 1;
			state.has_word = true;
			break;
		}

		// Word
		else if (VECTOR_EXISTS(word_chars_, c))
		{
			// Include preprocessor character if it was the previous character
			if (pp)
			{
				state.position--;
				u_length--;
			}

			state.state    = State::Word;
			state.length   = 0;
			state.has_word = true;
			break;
		}

		// Block begin
		else if (checkToken(state, state.position, block_begin_))
			state.fold_increment++;

		// Block end
		else if (checkToken(state, state.position, block_end_))
			state.fold_increment--;

		// LOG_MESSAGE(4, "unknown char '%c' (%d)", c, c);
		u_length++;
		state.position++;
		pp = false;
	}

	// LOG_MESSAGE(4, "unknown:%d", u_length);
	state.editor->SetStyling(u_length, Style::Default);

	return end;
}

// -----------------------------------------------------------------------------
// Process comment characters, updating [state].
// Returns true if the end of the current text range was reached
// -----------------------------------------------------------------------------
bool Lexer::processComment(LexerState& state) const
{
	bool end = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		// End of comment
		if (checkToken(state, state.position, comment_block_end_))
		{
			state.length += comment_block_end_.size();
			state.position += comment_block_end_.size();
			state.state = State::Unknown;
			if (fold_comments_)
				state.fold_increment--;
			break;
		}

		state.length++;
		state.position++;
	}

	if (debug_lexer)
		Log::debug(S_FMT("comment:%d", state.length));

	state.editor->SetStyling(state.length, Style::Comment);

	return end;
}

// -----------------------------------------------------------------------------
// Process word characters, updating [state].
// Returns true if the end of the current text range was reached
// -----------------------------------------------------------------------------
bool Lexer::processWord(LexerState& state)
{
	string word;
	bool   end = false;

	// Add first letter
	word.push_back(state.editor->GetCharAt(state.position++));

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		char c = state.editor->GetCharAt(state.position);
		if (VECTOR_EXISTS(word_chars_, c))
		{
			word.push_back(c);
			state.position++;
		}
		else
		{
			state.state = State::Unknown;
			break;
		}
	}

	// Check for preprocessor folding word
	if (fold_preprocessor_ && !word.empty() && word[0] == preprocessor_char_)
	{
		string word_lower = StrUtil::afterFirst(StrUtil::lower(word), preprocessor_char_);
		if (VECTOR_EXISTS(language_->ppBlockBegin(), word_lower))
			state.fold_increment++;
		else if (VECTOR_EXISTS(language_->ppBlockEnd(), word_lower))
			state.fold_increment--;
	}
	else
	{
		string word_lower = StrUtil::lower(word);
		if (VECTOR_EXISTS(language_->wordBlockBegin(), word_lower))
			state.fold_increment++;
		else if (VECTOR_EXISTS(language_->wordBlockEnd(), word_lower))
			state.fold_increment--;
	}

	if (debug_lexer)
		Log::debug(S_FMT("word:%s", word));

	styleWord(state, word);

	return end;
}

// -----------------------------------------------------------------------------
// Process string characters, updating [state].
// Returns true if the end of the current text range was reached
// -----------------------------------------------------------------------------
bool Lexer::processString(LexerState& state) const
{
	bool end = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		// End of string
		char c = state.editor->GetCharAt(state.position);
		if (c == '"')
		{
			state.length++;
			state.position++;
			state.state = State::Unknown;
			break;
		}

		state.length++;
		state.position++;
	}

	if (debug_lexer)
		Log::debug(S_FMT("string:%d", state.length));

	state.editor->SetStyling(state.length, Style::String);

	return end;
}

// -----------------------------------------------------------------------------
// Process char characters, updating [state].
// Returns true if the end of the current text range was reached
// -----------------------------------------------------------------------------
bool Lexer::processChar(LexerState& state) const
{
	bool end = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		// End of string
		char c = state.editor->GetCharAt(state.position);
		if (c == '\'')
		{
			state.length++;
			state.position++;
			state.state = State::Unknown;
			break;
		}

		state.length++;
		state.position++;
	}

	if (debug_lexer)
		Log::debug(S_FMT("char:%d", state.length));

	state.editor->SetStyling(state.length, Style::Char);

	return end;
}

// -----------------------------------------------------------------------------
// Process operator characters, updating [state].
// Returns true if the end of the current text range was reached
// -----------------------------------------------------------------------------
bool Lexer::processOperator(LexerState& state)
{
	bool end = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		char c = state.editor->GetCharAt(state.position);
		if (VECTOR_EXISTS(operator_chars_, c))
		{
			state.length++;
			state.position++;
		}
		else
		{
			state.state = State::Unknown;
			break;
		}
	}

	if (debug_lexer)
		Log::debug(S_FMT("operator:%d", state.length));

	state.editor->SetStyling(state.length, Style::Operator);

	return end;
}

// -----------------------------------------------------------------------------
// Process whitespace characters, updating [state].
// Returns true if the end of the current text range was reached
// -----------------------------------------------------------------------------
bool Lexer::processWhitespace(LexerState& state)
{
	bool end = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		char c = state.editor->GetCharAt(state.position);
		if (VECTOR_EXISTS(whitespace_chars_, c))
		{
			state.length++;
			state.position++;
		}
		else
		{
			state.state = State::Unknown;
			break;
		}
	}

	if (debug_lexer)
		Log::debug(S_FMT("whitespace:%d", state.length));

	state.editor->SetStyling(state.length, Style::Default);

	return end;
}

// -----------------------------------------------------------------------------
// Checks if the text in [editor] starting from [pos] matches [token]
// -----------------------------------------------------------------------------
bool Lexer::checkToken(LexerState& state, int pos, const string& token)
{
	if (!token.empty())
	{
		size_t token_size = token.size();
		for (unsigned a = 0; a < token_size; a++)
			if (state.editor->GetCharAt(pos + a) != (int)token[a])
				return false;

		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Updates code folding levels in [editor], starting from line [line_start]
// -----------------------------------------------------------------------------
void Lexer::updateFolding(TextEditorCtrl* editor, int line_start)
{
	int fold_level = editor->GetFoldLevel(line_start) & wxSTC_FOLDLEVELNUMBERMASK;

	for (int l = line_start; l < editor->GetLineCount(); l++)
	{
		// Determine next line's fold level
		int next_level = fold_level + lines_[l].fold_increment;
		if (next_level < wxSTC_FOLDLEVELBASE)
			next_level = wxSTC_FOLDLEVELBASE;

		// Check if we are going up a fold level
		if (next_level > fold_level)
		{
			if (!lines_[l].has_word)
			{
				// Line doesn't have any words (eg. only has an opening brace),
				// move the fold header up a line
				editor->SetFoldLevel(l - 1, fold_level | wxSTC_FOLDLEVELHEADERFLAG);
				editor->SetFoldLevel(l, next_level);
			}
			else
				editor->SetFoldLevel(l, fold_level | wxSTC_FOLDLEVELHEADERFLAG);
		}
		else
			editor->SetFoldLevel(l, fold_level);

		fold_level = next_level;
	}
}

// -----------------------------------------------------------------------------
// Returns true if the word from [start_pos] to [end_pos] in [editor] is a
// function
// -----------------------------------------------------------------------------
bool Lexer::isFunction(TextEditorCtrl* editor, int start_pos, int end_pos)
{
	string word = editor->GetTextRange(start_pos, end_pos).ToStdString();
	if (!language_->caseSensitive())
		StrUtil::lowerIP(word);
	return word_list_[word].style == (int)Style::Function;
}


// -----------------------------------------------------------------------------
//
// ZScriptLexer Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Sets the [style] for [word], or adds it to the functions list if [style] is
// Function
// -----------------------------------------------------------------------------
void ZScriptLexer::addWord(const string& word, int style)
{
	if (style == Style::Function)
		functions_.push_back(language_->caseSensitive() ? word : StrUtil::lower(word));
	else
		Lexer::addWord(word, style);
}

// -----------------------------------------------------------------------------
// ZScript version of Lexer::styleWord - functions require a following '('
// -----------------------------------------------------------------------------
void ZScriptLexer::styleWord(LexerState& state, string word)
{
	// Skip whitespace after word
	auto index = state.position;
	while (index < state.end)
	{
		if (!(VECTOR_EXISTS(whitespace_chars_, state.editor->GetCharAt(index))))
			break;
		++index;
	}

	// Check for '(' (possible function)
	if (state.editor->GetCharAt(index) == '(')
	{
		if (!language_->caseSensitive())
			StrUtil::lowerIP(word);

		if (VECTOR_EXISTS(functions_, word))
		{
			state.editor->SetStyling(word.length(), Style::Function);
			return;
		}
	}

	Lexer::styleWord(state, word);
}

// -----------------------------------------------------------------------------
// Clears out all defined words
// -----------------------------------------------------------------------------
void ZScriptLexer::clearWords()
{
	functions_.clear();
	Lexer::clearWords();
}

// -----------------------------------------------------------------------------
// Returns true if the word from [start_pos] to [end_pos] in [editor] is a
// function
// -----------------------------------------------------------------------------
bool ZScriptLexer::isFunction(TextEditorCtrl* editor, int start_pos, int end_pos)
{
	// Check for '(' after word

	// Skip whitespace
	auto index = end_pos;
	auto end   = editor->GetTextLength();
	while (index < end)
	{
		if (!(VECTOR_EXISTS(whitespace_chars_, editor->GetCharAt(index))))
			break;
		++index;
	}
	if (editor->GetCharAt(index) != '(')
		return false;

	// Check if word is a function name
	string word = editor->GetTextRange(start_pos, end_pos).ToStdString();
	if (!language_->caseSensitive())
		StrUtil::lowerIP(word);
	return VECTOR_EXISTS(functions_, word);
}
