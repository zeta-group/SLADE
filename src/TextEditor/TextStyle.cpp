
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextStyle.cpp
// Description: Classes which handle font and colour settings for the text
//              editor. TextStyle contains the actual font and colour settings
//              for a particular 'style' (eg. keywords, comments, etc).
//              StyleSet is just a set of these styles that can be loaded to
//              the scintilla 'styles' in the text editor
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
#include "Archive/ArchiveManager.h"
#include "Lexer.h"
#include "TextStyle.h"
#include "UI/TextEditorCtrl.h"
#include "UI/WxUtils.h"
#include "Utility/Parser.h"
#include "Utility/Tokenizer.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(String, txed_override_font, "", CVAR_SAVE)
CVAR(Int, txed_override_font_size, 0, CVAR_SAVE)
vector<TextEditorCtrl*> StyleSet::editors_;
namespace
{
vector<StyleSet*> style_sets;
StyleSet*         ss_current = nullptr;
} // namespace


// -----------------------------------------------------------------------------
//
// TextStyle Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextStyle class constructor
// -----------------------------------------------------------------------------
TextStyle::TextStyle(string_view name, string_view description, int style_id) :
	name_{ name.data(), name.size() },
	description_{ description.data(), description.size() }
{
	// Init variables
	if (style_id >= 0)
		wx_styles_.push_back(style_id);
}

// -----------------------------------------------------------------------------
// Adds a wxSTC style id to the list (used for applying style to the
// wxStyledTextCtrl, in case this style replaces multiple)
// -----------------------------------------------------------------------------
void TextStyle::addWxStyleId(int style)
{
	wx_styles_.push_back(style);
}

// -----------------------------------------------------------------------------
// Reads text style information from a parse tree
// -----------------------------------------------------------------------------
bool TextStyle::parse(ParseTreeNode* node)
{
	// Check any info was given
	if (!node)
		return false;

	// Go through info nodes
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto child = node->getChildPTN(a);
		auto name  = child->name();

		// Font name
		if (StrUtil::equalCI(name, "font"))
			font_ = child->stringValue();

		// Font size
		if (StrUtil::equalCI(name, "size"))
			size_ = child->intValue();

		// Foreground colour
		if (StrUtil::equalCI(name, "foreground"))
		{
			foreground_.set(child->intValue(0), child->intValue(1), child->intValue(2), 255);
			fg_defined_ = true;
		}

		// Background colour
		if (StrUtil::equalCI(name, "background"))
		{
			background_.set(child->intValue(0), child->intValue(1), child->intValue(2), 255);
			bg_defined_ = true;
		}

		// Bold
		if (StrUtil::equalCI(name, "bold"))
			bold_ = (int)child->boolValue();

		// Italic
		if (StrUtil::equalCI(name, "italic"))
			italic_ = (int)child->boolValue();

		// Underlined
		if (StrUtil::equalCI(name, "underlined"))
			underlined_ = (int)child->boolValue();
	}

	return true;
}

// -----------------------------------------------------------------------------
// Applies the style settings to the scintilla text control [stc]
// -----------------------------------------------------------------------------
void TextStyle::applyTo(wxStyledTextCtrl* stc)
{
	for (int wx_style : wx_styles_)
	{
		// Set font face
		if (!txed_override_font.value.empty())
			stc->StyleSetFaceName(wx_style, txed_override_font.value);
		else if (!font_.empty())
			stc->StyleSetFaceName(wx_style, font_);

		// Set font size
		if (txed_override_font_size > 0)
			stc->StyleSetSize(wx_style, txed_override_font_size);
		else if (size_ > 0)
			stc->StyleSetSize(wx_style, size_);

		// Set foreground
		if (fg_defined_)
			stc->StyleSetForeground(wx_style, WXCOL(foreground_));

		// Set background
		if (bg_defined_)
			stc->StyleSetBackground(wx_style, WXCOL(background_));

		// Set bold
		if (bold_ > 0)
			stc->StyleSetBold(wx_style, true);
		else if (bold_ == 0)
			stc->StyleSetBold(wx_style, false);

		// Set italic
		if (italic_ > 0)
			stc->StyleSetItalic(wx_style, true);
		else if (italic_ == 0)
			stc->StyleSetItalic(wx_style, false);

		// Set underlined
		if (underlined_ > 0)
			stc->StyleSetUnderline(wx_style, true);
		else if (underlined_ == 0)
			stc->StyleSetUnderline(wx_style, false);
	}
}

// -----------------------------------------------------------------------------
// Copies style info from [copy]
// -----------------------------------------------------------------------------
bool TextStyle::copyStyle(TextStyle* copy)
{
	if (!copy)
		return false;

	// Copy data straight
	font_       = copy->font_;
	size_       = copy->size_;
	foreground_ = copy->foreground_;
	fg_defined_ = copy->fg_defined_;
	background_ = copy->background_;
	bg_defined_ = copy->bg_defined_;
	bold_       = copy->bold_;
	italic_     = copy->italic_;
	underlined_ = copy->underlined_;

	return true;
}

// -----------------------------------------------------------------------------
// Returns a formatted string defining this style
// -----------------------------------------------------------------------------
string TextStyle::getDefinition(unsigned tabs) const
{
	fmt::memory_buffer ret;

	// Write font
	if (!font_.empty())
	{
		for (unsigned t = 0; t < tabs; t++)
			format_to(ret, "\t");
		format_to(ret, "font = \"{}\";\n", font_);
	}

	// Write size
	if (size_ >= 0)
	{
		for (unsigned t = 0; t < tabs; t++)
			format_to(ret, "\t");
		format_to(ret, "size = {};\n", size_);
	}

	// Write foreground
	if (fg_defined_)
	{
		for (unsigned t = 0; t < tabs; t++)
			format_to(ret, "\t");
		format_to(ret, "foreground = {}, {}, {};\n", foreground_.r, foreground_.g, foreground_.b);
	}

	// Write background
	if (bg_defined_)
	{
		for (unsigned t = 0; t < tabs; t++)
			format_to(ret, "\t");
		format_to(ret, "background = {}, {}, {};\n", background_.r, background_.g, background_.b);
	}

	// Write bold
	if (bold_ >= 0)
	{
		for (unsigned t = 0; t < tabs; t++)
			format_to(ret, "\t");
		format_to(ret, "bold = {};\n", bold_);
	}

	// Write italic
	if (italic_ >= 0)
	{
		for (unsigned t = 0; t < tabs; t++)
			format_to(ret, "\t");
		format_to(ret, "italic = {};\n", italic_);
	}

	// Write underlined
	if (underlined_ >= 0)
	{
		for (unsigned t = 0; t < tabs; t++)
			format_to(ret, "\t");
		format_to(ret, "underlined = {};\n", underlined_);
	}

	return to_string(ret);
}


// -----------------------------------------------------------------------------
//
// StyleSet Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// StyleSet class constructor
// -----------------------------------------------------------------------------
StyleSet::StyleSet(string_view name) :
	name_{ name.data(), name.size() },
	ts_default_("default", "Default", wxSTC_STYLE_DEFAULT),
	ts_selection_("selection", "Selected Text")
{
	// Init default style
	wxFont f(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	ts_default_.font_ = f.GetFaceName();
	ts_default_.size_ = 10;
	ts_default_.foreground_.set(0, 0, 0, 255);
	ts_default_.fg_defined_ = true;
	ts_default_.background_.set(255, 255, 255, 255);
	ts_default_.bg_defined_   = true;
	ts_default_.bold_         = 0;
	ts_default_.italic_       = 0;
	ts_default_.underlined_   = 0;
	ts_selection_.bg_defined_ = true;
	ts_selection_.background_.set(150, 150, 150, 255);
	ts_selection_.fg_defined_ = false;

	// Init styles
	styles_.push_back(new TextStyle("preprocessor", "Preprocessor", Lexer::Style::Preprocessor));
	styles_.push_back(new TextStyle("comment", "Comment", Lexer::Style::Comment));
	styles_.push_back(new TextStyle("comment_doc", "Comment (Doc)", Lexer::Style::CommentDoc));
	styles_.push_back(new TextStyle("string", "String", Lexer::Style::String));
	styles_.push_back(new TextStyle("character", "Character", Lexer::Style::Char));
	styles_.push_back(new TextStyle("keyword", "Keyword", Lexer::Style::Keyword));
	styles_.push_back(new TextStyle("constant", "Constant", Lexer::Style::Constant));
	styles_.push_back(new TextStyle("type", "Type", Lexer::Style::Type));
	styles_.push_back(new TextStyle("property", "Property", Lexer::Style::Property));
	styles_.push_back(new TextStyle("function", "Function", Lexer::Style::Function));
	styles_.push_back(new TextStyle("number", "Number", Lexer::Style::Number));
	styles_.push_back(new TextStyle("operator", "Operator", Lexer::Style::Operator));
	styles_.push_back(new TextStyle("bracematch", "Brace Match", wxSTC_STYLE_BRACELIGHT));
	styles_.push_back(new TextStyle("bracebad", "Brace Mismatch", wxSTC_STYLE_BRACEBAD));
	styles_.push_back(new TextStyle("linenum", "Line Numbers", wxSTC_STYLE_LINENUMBER));
	styles_.push_back(new TextStyle("calltip", "Call Tip", wxSTC_STYLE_CALLTIP));
	styles_.push_back(new TextStyle("calltip_hl", "Call Tip Highlight"));
	styles_.push_back(new TextStyle("foldmargin", "Code Folding Margin"));
	styles_.push_back(new TextStyle("guides", "Indent/Right Margin Guide"));
	styles_.push_back(new TextStyle("wordmatch", "Word Match"));
	styles_.push_back(new TextStyle("current_line", "Current Line"));
}

// -----------------------------------------------------------------------------
// StyleSet class destructor
// -----------------------------------------------------------------------------
StyleSet::~StyleSet()
{
	for (auto& style : styles_)
		delete style;
}

// -----------------------------------------------------------------------------
// Reads style set info from a parse tree
// -----------------------------------------------------------------------------
bool StyleSet::parseSet(ParseTreeNode* root)
{
	if (!root)
		return false;

	// Get name
	auto node = root->getChildPTN("name");
	if (node)
		name_ = node->stringValue();

	// Parse styles
	ts_default_.parse(root->getChildPTN("default"));     // Default style
	ts_selection_.parse(root->getChildPTN("selection")); // Selection style
	for (auto& style : styles_)                          // Other styles
	{
		if (ParseTreeNode* child = root->getChildPTN(style->name_))
			style->parse(child);
		else
		{
			if (style->name_ == "foldmargin")
			{
				// No 'foldmargin' style defined, copy it from line numbers style
				style->foreground_ = styleForeground("linenum");
				style->background_ = styleBackground("linenum");
				style->fg_defined_ = true;
				style->bg_defined_ = true;
			}
			else if (style->name_ == "guides")
			{
				// No 'guides' style defined, use the default foreground colour
				style->foreground_ = ts_default_.foreground();
				style->fg_defined_ = true;
			}
			else if (style->name_ == "type" || style->name_ == "property")
			{
				// No 'type' or 'property' style defined, copy it from keyword style
				style->copyStyle(styleFor("keyword"));
			}
			else if (style->name_ == "comment_doc")
			{
				// No 'comment_doc' style defined, copy it from comment style
				style->copyStyle(styleFor("comment"));
			}
			else if (style->name_ == "current_line")
			{
				// No 'currentline' style defined, use the default background and darken/lighten it a little
				int fgm = -20;
				int bgm = -10;
				if (ts_default_.background_.greyscale().r < 100)
				{
					fgm = 30;
					bgm = 15;
				}
				style->foreground_ = ts_default_.background().amp(fgm, fgm, fgm, 0);
				style->fg_defined_ = true;
				style->background_ = ts_default_.background().amp(bgm, bgm, bgm, 0);
				style->bg_defined_ = true;
			}
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Applies all the styles in this set to the text styles in scintilla text
// control [stc]
// -----------------------------------------------------------------------------
void StyleSet::applyTo(TextEditorCtrl* stc)
{
	applyToWx(stc);

	// Update code folding margin
	stc->setupFoldMargin(styleFor("foldmargin"));
}

void StyleSet::applyToWx(wxStyledTextCtrl* stc)
{
	// Set default style
	ts_default_.applyTo(stc);

	// Apply default style to all
	stc->StyleClearAll();

	// Apply other styles
	for (auto& style : styles_)
		style->applyTo(stc);

	// Set selection background if customised
	if (ts_selection_.hasBackground())
		stc->SetSelBackground(true, WXCOL(ts_selection_.background_));
	else
		stc->SetSelBackground(false, wxColour("red"));

	// Set selection foreground if customised
	if (ts_selection_.hasForeground())
		stc->SetSelForeground(true, WXCOL(ts_selection_.foreground_));
	else
		stc->SetSelForeground(false, wxColour("red"));

	// Set caret colour to text foreground colour
	stc->SetCaretForeground(WXCOL(ts_default_.foreground_));

	// Set indent and right margin line colour
	stc->SetEdgeColour(WXCOL(styleFor("guides")->foreground()));
	stc->StyleSetBackground(wxSTC_STYLE_INDENTGUIDE, WXCOL(styleBackground("guides")));
	stc->StyleSetForeground(wxSTC_STYLE_INDENTGUIDE, WXCOL(styleForeground("guides")));

	// Set word match indicator colour
	stc->SetIndicatorCurrent(8);
	stc->IndicatorSetForeground(8, WXCOL(styleForeground("wordmatch")));

	// Set current line colour
	stc->SetCaretLineBackground(WXCOL(styleBackground("current_line")));
	stc->MarkerDefine(
		1, wxSTC_MARK_BACKGROUND, WXCOL(styleBackground("current_line")), WXCOL(styleBackground("current_line")));
	stc->MarkerDefine(
		2, wxSTC_MARK_UNDERLINE, WXCOL(styleForeground("current_line")), WXCOL(styleForeground("current_line")));
}

// -----------------------------------------------------------------------------
// Copies all styles in [copy] to this set
// -----------------------------------------------------------------------------
bool StyleSet::copySet(StyleSet* copy)
{
	if (!copy)
		return false;

	// Copy all styles
	ts_default_.copyStyle(&(copy->ts_default_));
	ts_selection_.copyStyle(&(copy->ts_selection_));
	for (unsigned a = 0; a < copy->styles_.size(); a++)
		styles_[a]->copyStyle(copy->styles_[a]);

	return true;
}

// -----------------------------------------------------------------------------
// Returns the text style associated with [name] (these are hard coded), or
// nullptr if [name] was invalid
// -----------------------------------------------------------------------------
TextStyle* StyleSet::styleFor(string_view name)
{
	// Return style matching name given
	if (StrUtil::equalCI(name, "default"))
		return &ts_default_;
	else if (StrUtil::equalCI(name, "selection"))
		return &ts_selection_;
	else
	{
		for (auto& style : styles_)
		{
			if (style->name_ == name)
				return style;
		}
	}

	// Not a valid style
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the extra text style at [index]
// -----------------------------------------------------------------------------
TextStyle* StyleSet::styleAt(unsigned index)
{
	if (index < styles_.size())
		return styles_[index];
	else
		return nullptr;
}

// -----------------------------------------------------------------------------
// Writes this style set as a text definition to a file [filename]
// -----------------------------------------------------------------------------
bool StyleSet::writeFile(string_view filename)
{
	// Open file for writing
	wxFile file(WxUtils::stringFromView(filename), wxFile::write);

	if (!file.IsOpened())
		return false;

	// Write opening
	file.Write("styleset {\n");

	// Name
	file.Write(wxString::Format("\tname = \"%s\";\n\n", name_));

	// Default style
	file.Write("\tdefault {\n");
	file.Write(ts_default_.getDefinition(2));
	file.Write("\t}\n\n");

	// Selection style
	file.Write("\tselection {\n");
	file.Write(ts_selection_.getDefinition(2));
	file.Write("\t}\n\n");

	// Other styles
	for (auto& style : styles_)
	{
		file.Write(wxString::Format("\t%s {\n", style->name_));
		file.Write(style->getDefinition(2));
		file.Write("\t}\n\n");
	}

	// Write end
	file.Write("}\n");

	// Close file
	file.Close();

	return true;
}

// -----------------------------------------------------------------------------
// Returns the foreground colour of [style], or the default style's foreground
// colour if it is not set
// -----------------------------------------------------------------------------
const ColRGBA& StyleSet::styleForeground(string_view style)
{
	TextStyle* s = this->styleFor(style);
	if (s && s->hasForeground())
		return s->foreground();
	else
		return ts_default_.foreground();
}

// -----------------------------------------------------------------------------
// Returns the background colour of [style], or the default style's background
// colour if it is not set
// -----------------------------------------------------------------------------
const ColRGBA& StyleSet::styleBackground(string_view style)
{
	TextStyle* s = this->styleFor(style);
	if (s && s->hasBackground())
		return s->background();
	else
		return ts_default_.background();
}

// -----------------------------------------------------------------------------
// Returns the default style font face
// -----------------------------------------------------------------------------
const string& StyleSet::defaultFontFace()
{
	if (!txed_override_font.value.empty())
		return txed_override_font.value;
	else
		return styleFor("default")->fontFace();
}

// -----------------------------------------------------------------------------
// Returns the default style font size
// -----------------------------------------------------------------------------
int StyleSet::defaultFontSize()
{
	if (!txed_override_font.value.empty() && txed_override_font_size > 0)
		return txed_override_font_size;
	else
		return styleFor("default")->fontSize();
}


// -----------------------------------------------------------------------------
//
// StyleSet Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Initialises the 'current' style set from the previously saved 'current.sss'
// file, or uses the default set if the file does not exist
// -----------------------------------------------------------------------------
void StyleSet::initCurrent()
{
	// Create 'current' styleset
	ss_current        = new StyleSet();
	ss_current->name_ = "<current styleset>";

	// First up, check if "<userdir>/current.sss" exists
	string path = App::path("current.sss", App::Dir::User);
	if (wxFileExists(path))
	{
		// Read it in
		Tokenizer tz;
		tz.openFile(path);

		// Parse it
		ParseTreeNode root;
		root.allowDup(true);
		root.parse(tz);

		// Find definition
		auto node = root.getChildPTN("styleset");
		if (node)
		{
			// If found, load it into the current set
			ss_current->parseSet(node);
			return;
		}
	}

	// Unable to load from userdir, just load first styleset (should be default)
	if (!style_sets.empty())
		ss_current->copySet(style_sets[0]);
}

// -----------------------------------------------------------------------------
// Writes the current style set to the 'current.sss' file
// -----------------------------------------------------------------------------
void StyleSet::saveCurrent()
{
	if (!ss_current)
		return;

	ss_current->writeFile(App::path("current.sss", App::Dir::User));
}

// -----------------------------------------------------------------------------
// Returns the current style set
// -----------------------------------------------------------------------------
StyleSet* StyleSet::currentSet()
{
	if (!ss_current)
		initCurrent();

	return ss_current;
}

// -----------------------------------------------------------------------------
// Loads the style set matching [name] to the current style set.
// Returns false if no match was found, true otherwise
// -----------------------------------------------------------------------------
bool StyleSet::loadSet(string_view name)
{
	// Search for set matching name
	for (auto& style_set : style_sets)
	{
		if (StrUtil::equalCI(style_set->name_, name))
		{
			ss_current->copySet(style_set);
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Loads the style set at [index] to the current style set.
// Returns false if [index] was out of bounds, true otherwise
// -----------------------------------------------------------------------------
bool StyleSet::loadSet(unsigned index)
{
	// Check index
	if (index >= style_sets.size())
		return false;

	currentSet()->copySet(style_sets[index]);

	return true;
}

// -----------------------------------------------------------------------------
// Applies the current style set to the scintilla text control [stc]
// -----------------------------------------------------------------------------
void StyleSet::applyCurrent(TextEditorCtrl* stc)
{
	currentSet()->applyTo(stc);
}

// -----------------------------------------------------------------------------
// Returns the name of the style set at [index], or an empty string if [index]
// is out of bounds
// -----------------------------------------------------------------------------
string StyleSet::name(unsigned index)
{
	// Check index
	if (index >= style_sets.size())
		return "";

	return style_sets[index]->name_;
}

// -----------------------------------------------------------------------------
// Returns the number of loaded style sets
// -----------------------------------------------------------------------------
unsigned StyleSet::numSets()
{
	return style_sets.size();
}

// -----------------------------------------------------------------------------
// Returns the style set at [index], or nullptr if [index] is out of bounds
// -----------------------------------------------------------------------------
StyleSet* StyleSet::getSet(unsigned index)
{
	// Check index
	if (index >= style_sets.size())
		return nullptr;

	return style_sets[index];
}

// -----------------------------------------------------------------------------
// Adds [stc] to the current list of text editors
// -----------------------------------------------------------------------------
void StyleSet::addEditor(TextEditorCtrl* stc)
{
	editors_.push_back(stc);
}

// -----------------------------------------------------------------------------
// Removes [stc] from the current list of text editors
// -----------------------------------------------------------------------------
void StyleSet::removeEditor(TextEditorCtrl* stc)
{
	VECTOR_REMOVE(editors_, stc);
}

// -----------------------------------------------------------------------------
// Applies the current style set to all text editors in the list
// -----------------------------------------------------------------------------
void StyleSet::applyCurrentToAll()
{
	for (auto& editor : editors_)
		applyCurrent(editor);
}

// -----------------------------------------------------------------------------
// Adds [set] to the list of text styles (makes a copy). If a custom set with
// [set]'s name already exists, copy [set] to it
// -----------------------------------------------------------------------------
void StyleSet::addSet(StyleSet* set)
{
	// Find existing custom set with same name
	for (auto s : style_sets)
		if (s->name_ == set->name_)
		{
			// Non-custom set exists, need to create a copy instead
			if (s->built_in_)
			{
				set->name_ += " (Copy)";
				break;
			}

			// Copy set
			s->copySet(set);
			return;
		}

	// Not found, add new set
	auto new_set = new StyleSet(set->name_);
	new_set->copySet(set);
	style_sets.push_back(new_set);
}

// -----------------------------------------------------------------------------
// Loads all text styles from the slade resource archive (slade.pk3)
// -----------------------------------------------------------------------------
bool StyleSet::loadResourceStyles()
{
	// Get 'config/text_styles' directory in slade.pk3
	ArchiveTreeNode* dir = App::archiveManager().programResourceArchive()->getDir("config/text_styles");

	// Check it exists
	if (!dir)
	{
		Log::info(1, "Warning: No 'config/text_styles' directory exists in slade.pk3");
		return false;
	}

	// Read default style set first
	ArchiveEntry* default_style = dir->entry("default.sss");
	if (default_style)
	{
		// Read entry data into tokenizer
		Tokenizer tz;
		tz.openMem(default_style->data(), default_style->name());

		// Parse it
		ParseTreeNode root;
		root.allowDup(true);
		root.parse(tz);

		// Read any styleset definitions
		vector<STreeNode*> nodes = root.children("styleset");
		for (auto& node : nodes)
		{
			StyleSet* newset  = new StyleSet();
			newset->built_in_ = true;
			if (newset->parseSet((ParseTreeNode*)node))
				style_sets.push_back(newset);
			else
				delete newset;
		}
	}

	// Go through all entries within it
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		ArchiveEntry* entry = dir->entryAt(a);

		// Skip default
		if (entry->nameNoExt() == "default")
			continue;

		// Read entry data into tokenizer
		Tokenizer tz;
		tz.openMem(entry->data(), entry->name());

		// Parse it
		ParseTreeNode root;
		root.allowDup(true);
		root.parse(tz);

		// Read any styleset definitions
		vector<STreeNode*> nodes = root.children("styleset");
		for (auto& node : nodes)
		{
			StyleSet* newset  = new StyleSet();
			newset->built_in_ = true;
			if (newset->parseSet((ParseTreeNode*)node))
				style_sets.push_back(newset);
			else
				delete newset;
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Loads all text styles from the user text style directory
// -----------------------------------------------------------------------------
bool StyleSet::loadCustomStyles()
{
	// If the custom stylesets directory doesn't exist, create it
	if (!wxDirExists(App::path("text_styles", App::Dir::User)))
		wxMkdir(App::path("text_styles", App::Dir::User));

	// Open the custom stylesets directory
	wxDir res_dir;
	res_dir.Open(App::path("text_styles", App::Dir::User));
	string dir_base = res_dir.GetName().ToStdString();
	dir_base.push_back('/');

	// Go through each file in the directory
	wxString filename = wxEmptyString;
	bool     files    = res_dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
	while (files)
	{
		// Read file into tokenizer
		Tokenizer tz;
		tz.openFile(dir_base + filename.ToStdString());

		// Parse it
		ParseTreeNode root;
		root.allowDup(true);
		root.parse(tz);

		// Read any styleset definitions
		vector<STreeNode*> nodes = root.children("styleset");
		for (auto& node : nodes)
		{
			StyleSet* newset = new StyleSet();
			if (newset->parseSet((ParseTreeNode*)node))
				style_sets.push_back(newset);
			else
				delete newset;
		}

		// Next file
		files = res_dir.GetNext(&filename);
	}

	return true;
}
