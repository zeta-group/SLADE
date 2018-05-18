#pragma once

#include "Utility/StringUtils.h"

#define KPM_CTRL 0x01
#define KPM_ALT 0x02
#define KPM_SHIFT 0x04

class Tokenizer;

struct KeyPress
{
	string key;
	bool   alt   = false;
	bool   ctrl  = false;
	bool   shift = false;

	KeyPress(string_view key, bool alt, bool ctrl, bool shift) :
		key{ key.data(), key.size() },
		alt{ alt },
		ctrl{ ctrl },
		shift{ shift }
	{
	}

	KeyPress(string_view key = "", int modifiers = 0) : key{ key.data(), key.size() }
	{
		if (modifiers & KPM_CTRL)
			ctrl = true;
		if (modifiers & KPM_ALT)
			alt = true;
		if (modifiers & KPM_SHIFT)
			shift = true;
	}

	string toString() const
	{
		if (key.empty())
			return "";

		string ret;
		if (ctrl)
			ret += "Ctrl+";
		if (alt)
			ret += "Alt+";
		if (shift)
			ret += "Shift+";

		string keyname = key;
		StrUtil::replaceIP(keyname, "_", " ");
		StrUtil::capitalizeIP(keyname);
		ret += keyname;

		return ret;
	}
};

class KeyBind
{
public:
	KeyBind(string_view name) :
		name_{ name.data(), name.size() },
		pressed_{ false },
		ignore_shift_{ false },
		priority_{ 0 }
	{
	}

	~KeyBind() = default;

	// Operators
	bool operator>(const KeyBind r) const
	{
		return priority_ == r.priority_ ? name_ < r.name_ : priority_ < r.priority_;
	}
	bool operator<(const KeyBind r) const
	{
		return priority_ == r.priority_ ? name_ > r.name_ : priority_ > r.priority_;
	}

	void     clear() { keys_.clear(); }
	void     addKey(string_view key, bool alt = false, bool ctrl = false, bool shift = false);
	string   name() const { return name_; }
	string   group() const { return group_; }
	string   description() const { return description_; }
	string   keysAsString();
	KeyPress firstKey() { return keys_.empty() ? KeyPress{} : keys_[0]; }
	KeyPress firstDefault() { return defaults_.empty() ? KeyPress{} : defaults_[0]; }

	const vector<KeyPress>& keys() const { return keys_; }
	const vector<KeyPress>& defaults() const { return defaults_; }

	// Static functions
	static KeyBind&       getBind(string_view name);
	static vector<string> getBinds(const KeyPress& key);
	static bool           isPressed(string_view name);
	static bool           addBind(
				  string_view name,
				  KeyPress    key,
				  string_view desc         = "",
				  string_view group        = "",
				  bool        ignore_shift = false,
				  int         priority     = -1);
	static string   keyName(int key);
	static string   mbName(int button);
	static bool     keyPressed(KeyPress key);
	static bool     keyReleased(string_view key);
	static KeyPress asKeyPress(int keycode, int modifiers);
	static void     allKeyBinds(vector<KeyBind*>& list);
	static void     releaseAll();
	static void     pressBind(string_view name);

	static void   initBinds();
	static string writeBinds();
	static bool   readBinds(Tokenizer& tz);
	static void   updateSortedBindsList();

private:
	string           name_;
	vector<KeyPress> keys_;
	vector<KeyPress> defaults_;
	bool             pressed_;
	string           description_;
	string           group_;
	bool             ignore_shift_;
	int              priority_;
};


class KeyBindHandler
{
public:
	KeyBindHandler();
	virtual ~KeyBindHandler();

	virtual void onKeyBindPress(string_view name) {}
	virtual void onKeyBindRelease(string_view name) {}
};
