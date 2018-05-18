#pragma once

#include "Utility/Parser.h"

class TextEditorCtrl;
class TextStyle
{
	friend class StyleSet;

public:
	TextStyle(string_view name, string_view description, int style_id = -1);
	~TextStyle() = default;

	void addWxStyleId(int style);

	const string& description() const { return description_; }
	const string& fontFace() const { return font_; }
	int           fontSize() const { return size_; }
	bool          hasForeground() const { return fg_defined_; }
	bool          hasBackground() const { return bg_defined_; }
	int           bold() const { return bold_; }
	int           italic() const { return italic_; }
	int           underlined() const { return underlined_; }

	void setFontFace(string_view font) { S_SET_VIEW(font_, font); }
	void setFontSize(int size) { this->size_ = size; }
	void setBold(int bold) { this->bold_ = bold; }
	void setItalic(int italic) { this->italic_ = italic; }
	void setUnderlined(int underlined) { this->underlined_ = underlined; }
	void setForeground(const ColRGBA& col)
	{
		foreground_.set(col);
		fg_defined_ = true;
	}
	void clearForeground() { fg_defined_ = false; }
	void setBackground(const ColRGBA& col)
	{
		background_.set(col);
		bg_defined_ = true;
	}
	void clearBackground() { bg_defined_ = false; }

	const ColRGBA& foreground() const { return foreground_; }
	const ColRGBA& background() const { return background_; }

	bool   parse(ParseTreeNode* node);
	void   applyTo(wxStyledTextCtrl* stc);
	bool   copyStyle(TextStyle* copy);
	string getDefinition(unsigned tabs = 0) const;

private:
	string      name_;
	string      description_;
	vector<int> wx_styles_;

	string font_;
	int    size_ = -1;
	ColRGBA foreground_;
	bool   fg_defined_ = false;
	ColRGBA background_;
	bool   bg_defined_ = false;
	int    bold_       = -1;
	int    italic_     = -1;
	int    underlined_ = -1;
};

class StyleSet
{
public:
	StyleSet(string_view name = "Unnamed Style");
	~StyleSet();

	const string& name() const { return name_; }
	unsigned      nStyles() const { return styles_.size(); }

	bool       parseSet(ParseTreeNode* root);
	void       applyTo(TextEditorCtrl* stc);
	void       applyToWx(wxStyledTextCtrl* stc);
	bool       copySet(StyleSet* copy);
	TextStyle* styleFor(string_view name);
	TextStyle* styleAt(unsigned index);
	bool       writeFile(string_view filename);

	const ColRGBA& styleForeground(string_view style);
	const ColRGBA& styleBackground(string_view style);
	const string& defaultFontFace();
	int           defaultFontSize();

	// Static functions for styleset management
	static void      initCurrent();
	static void      saveCurrent();
	static StyleSet* currentSet();
	static bool      loadSet(string_view name);
	static bool      loadSet(unsigned index);
	static void      applyCurrent(TextEditorCtrl* stc);
	static string    name(unsigned index);
	static unsigned  numSets();
	static StyleSet* getSet(unsigned index);
	static void      addEditor(TextEditorCtrl* stc);
	static void      removeEditor(TextEditorCtrl* stc);
	static void      applyCurrentToAll();
	static void      addSet(StyleSet* set);

	static bool loadResourceStyles();
	static bool loadCustomStyles();

private:
	string    name_;
	TextStyle ts_default_;
	TextStyle ts_selection_;
	bool      built_in_ = false;

	vector<TextStyle*> styles_;

	static vector<TextEditorCtrl*> editors_;
};
