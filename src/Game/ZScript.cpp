
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ZScript.cpp
// Description: ZScript definition classes and parsing
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
#include "ZScript.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveManager.h"
#include "General/Database.h"
#include "General/Library.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"

using namespace slade;
using namespace zscript;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::zscript
{
EntryType* etype_zscript = nullptr;

// ZScript keywords (can't be function/variable names)
vector<string> keywords = { "class",      "default", "private",  "static", "native",   "return",       "if",
							"else",       "for",     "while",    "do",     "break",    "continue",     "deprecated",
							"state",      "null",    "readonly", "true",   "false",    "struct",       "extend",
							"clearscope", "vararg",  "ui",       "play",   "virtual",  "virtualscope", "meta",
							"Property",   "version", "in",       "out",    "states",   "action",       "override",
							"super",      "is",      "let",      "const",  "replaces", "protected",    "self" };

// For test_parse_zscript console command
bool dump_parsed_blocks    = false;
bool dump_parsed_states    = false;
bool dump_parsed_functions = false;

string db_comment = "//$";

auto sql_insert_source     = "INSERT INTO zs_source (archive_file_id, entry_path) VALUES (?,?)";
auto sql_insert_identifier = "INSERT INTO zs_identifier (source_id, type_id, name, parent_id) VALUES (?,?,?,?)";
auto sql_insert_enum_value = "INSERT INTO zs_enumerator_value (identifier_id, name, value) VALUES (?,?,?)";
auto sql_insert_class =
	"INSERT INTO zs_class (identifier_id, scope_id, base_class, abstract, native, replaces, version)"
	"VALUES (?,?,?,?,?,?,?)";
auto sql_insert_class_default     = "INSERT INTO zs_class_default_property (identifier_id, name, value) VALUES (?,?,?)";
auto sql_insert_class_editor_prop = "INSERT INTO zs_class_editor_property (identifier_id, name, value) VALUES (?,?,?)";
auto sql_insert_struct            = "INSERT INTO zs_struct (identifier_id, scope_id, native, version) VALUES (?,?,?,?)";
auto sql_insert_function =
	"INSERT INTO zs_function (identifier_id, scope_id, return_type, visibility, action, action_scope, const, final, "
	"native, override, static, vararg, virtual, virtualscope, deprecated, version) "
	"VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
auto sql_insert_function_parameter =
	"INSERT INTO zs_function_parameter (identifier_id, [index], name, type, default_value) "
	"VALUES (?,?,?,?,?)";
auto sql_insert_state_frame =
	"INSERT INTO zs_state_frame (identifier_id, sprite_base, sprite_frames, duration) "
	"VALUES (?,?,?,?)";

} // namespace slade::zscript


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace slade::zscript
{
// -----------------------------------------------------------------------------
// Writes a log [message] of [type] beginning with the location of [statement]
// -----------------------------------------------------------------------------
void logParserMessage(const ParsedStatement& statement, log::MessageType type, string_view message)
{
	string location = "<unknown location>";
	if (statement.entry)
		location = statement.entry->path(true);

	log::message(type, fmt::format("{}:{}: {}", location, statement.line, message));
}

// -----------------------------------------------------------------------------
// Parses a ZScript type (eg. 'class<Actor>') from [tokens] beginning at [index]
// -----------------------------------------------------------------------------
string parseType(const vector<string>& tokens, unsigned& index)
{
	string type;

	// Qualifiers
	while (index < tokens.size())
	{
		if (strutil::equalCI(tokens[index], "in") || strutil::equalCI(tokens[index], "out"))
			type.append(tokens[index++]).append(" ");
		else
			break;
	}

	type += tokens[index];

	// Check for ...
	if (index + 2 < tokens.size() && tokens[index] == '.' && tokens[index + 1] == '.' && tokens[index + 2] == '.')
	{
		type = "...";
		index += 2;
	}

	// Check for <>
	if (tokens[index + 1] == '<')
	{
		type += '<';
		index += 2;
		while (index < tokens.size() && tokens[index] != '>')
			type += tokens[index++];
		type += '>';
		++index;
	}

	++index;

	return type;
}

// -----------------------------------------------------------------------------
// Parses a ZScript value from [tokens] beginning at [index]
// -----------------------------------------------------------------------------
string parseValue(const vector<string>& tokens, unsigned& index)
{
	string value;
	while (true)
	{
		// Read between ()
		if (tokens[index] == '(')
		{
			int level = 1;
			value += tokens[index++];
			while (level > 0)
			{
				if (tokens[index] == '(')
					++level;
				if (tokens[index] == ')')
					--level;

				value += tokens[index++];
			}

			continue;
		}

		if (tokens[index] == ',' || tokens[index] == ';' || tokens[index] == ')')
			break;

		value += tokens[index++];

		if (index >= tokens.size())
			break;
	}

	return value;
}

// -----------------------------------------------------------------------------
// Checks for a ZScript keyword+value statement in [tokens] beginning at
// [index], eg. deprecated("#.#") or version("#.#")
// Returns true if there is a keyword+value statement and writes the value to
// [value]
// -----------------------------------------------------------------------------
bool checkKeywordValueStatement(const vector<string>& tokens, unsigned index, string_view word, string& value)
{
	if (index + 3 >= tokens.size())
		return false;

	if (strutil::equalCI(tokens[index], word) && tokens[index + 1] == '(' && tokens[index + 3] == ')')
	{
		value = tokens[index + 2];
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Parses all statements/blocks in [entry], adding them to [parsed]
// -----------------------------------------------------------------------------
void parseBlocks(ArchiveEntry& entry, vector<ParsedStatement>& parsed, vector<ArchiveEntry*>& entry_stack)
{
	Tokenizer tz;
	tz.setSpecialCharacters(Tokenizer::DEFAULT_SPECIAL_CHARACTERS + "()+-[]&!?.");
	tz.enableDecorate(true);
	tz.setCommentTypes(Tokenizer::CommentTypes::CPPStyle | Tokenizer::CommentTypes::CStyle);
	tz.openMem(entry.data(), "ZScript");

	entry_stack.push_back(&entry);

	while (!tz.atEnd())
	{
		// Preprocessor
		if (strutil::startsWith(tz.current().text, '#'))
		{
			if (tz.checkNC("#include"))
			{
				auto inc_entry = entry.relativeEntry(tz.next().text);

				// Check #include path could be resolved
				if (!inc_entry)
				{
					log::warning(
						"Warning parsing ZScript entry {}: "
						"Unable to find #included entry \"{}\" at line {}, skipping",
						entry.name(),
						tz.current().text,
						tz.current().line_no);
				}
				else if (VECTOR_EXISTS(entry_stack, inc_entry))
				{
					log::warning(
						"Warning parsing ZScript entry {}: "
						"Detected circular #include \"{}\" on line {}, skipping",
						entry.name(),
						tz.current().text,
						tz.current().line_no);
				}
				else
					parseBlocks(*inc_entry, parsed, entry_stack);
			}

			tz.advToNextLine();
			continue;
		}

		// Version
		else if (tz.checkNC("version"))
		{
			tz.advToNextLine();
			continue;
		}

		// ZScript
		parsed.push_back({});
		parsed.back().entry = &entry;
		if (!parsed.back().parse(tz))
			parsed.pop_back();
	}

	// Set entry type
	if (etype_zscript && entry.type() != etype_zscript)
		entry.setType(etype_zscript);

	entry_stack.pop_back();
}

// -----------------------------------------------------------------------------
// Returns true if [word] is a ZScript keyword
// -----------------------------------------------------------------------------
bool isKeyword(string_view word)
{
	for (auto& kw : keywords)
		if (strutil::equalCI(word, kw))
			return true;

	return false;
}

int findClassIdentifierId(const string& class_name, int source_id, database::Context* db)
{
	int id = 0;

	if (auto* ps = db->cacheQuery(
			"zs_find_class_id", "SELECT id FROM zs_identifier WHERE type_id = 2 AND name = ? AND source_id = ?", true))
	{
		ps->bind(1, class_name);
		ps->bind(2, source_id);
		if (ps->executeStep())
			id = ps->getColumn(0).getInt();
		ps->reset();
	}

	return id;
}

} // namespace slade::zscript


// -----------------------------------------------------------------------------
//
// Enumerator Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Parses an enumerator block [statement]
// -----------------------------------------------------------------------------
bool Enumerator::parse(ParsedStatement& statement)
{
	// Check valid statement
	if (statement.block.empty())
		return false;
	if (statement.tokens.size() < 2)
		return false;

	// Parse name
	name_ = statement.tokens[1];

	// Parse values
	unsigned index = 0;
	unsigned count = statement.block[0].tokens.size();
	while (index < count)
	{
		auto val_name = statement.block[0].tokens[index];

		// TODO: Parse value

		values_.push_back({ val_name, 0 });

		// Skip past next ,
		while (index + 1 < count)
			if (statement.block[0].tokens[++index] == ',')
				break;

		index++;
	}

	return true;
}


// -----------------------------------------------------------------------------
//
// Function Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Parses a function parameter from [tokens] beginning at [index]
// -----------------------------------------------------------------------------
unsigned Function::Parameter::parse(const vector<string>& tokens, unsigned start_index)
{
	// Type
	type = parseType(tokens, start_index);

	// Special case - '...'
	if (type == "...")
	{
		name = "...";
		type.clear();
		return start_index;
	}

	// Name
	if (start_index >= tokens.size() || tokens[start_index] == ')')
		return start_index;
	name = tokens[start_index++];

	// Default value
	if (start_index < tokens.size() && tokens[start_index] == '=')
	{
		++start_index;
		default_value = parseValue(tokens, start_index);
	}

	return start_index;
}

// -----------------------------------------------------------------------------
// Parses a function declaration [statement]
// -----------------------------------------------------------------------------
bool Function::parse(ParsedStatement& statement)
{
	unsigned index;
	int      last_qualifier = -1;
	for (index = 0; index < statement.tokens.size(); index++)
	{
		if (strutil::equalCI(statement.tokens[index], "virtual"))
		{
			virtual_       = true;
			last_qualifier = index;
		}
		else if (strutil::equalCI(statement.tokens[index], "static"))
		{
			static_        = true;
			last_qualifier = index;
		}
		else if (strutil::equalCI(statement.tokens[index], "native"))
		{
			native_        = true;
			last_qualifier = index;
		}
		else if (strutil::equalCI(statement.tokens[index], "action"))
		{
			action_        = true;
			last_qualifier = index;
		}
		else if (strutil::equalCI(statement.tokens[index], "override"))
		{
			override_      = true;
			last_qualifier = index;
		}
		else if ((int)index > last_qualifier + 2 && statement.tokens[index] == '(')
		{
			name_        = statement.tokens[index - 1];
			return_type_ = statement.tokens[index - 2];
			break;
		}
		else if (checkKeywordValueStatement(statement.tokens, index, "deprecated", deprecated_))
			index += 3;
		else if (checkKeywordValueStatement(statement.tokens, index, "version", version_))
			index += 3;
	}

	if (name_.empty() || return_type_.empty())
	{
		logParserMessage(statement, log::MessageType::Warning, "Function parse failed");
		return false;
	}

	// Name can't be a keyword
	if (isKeyword(name_))
	{
		logParserMessage(statement, log::MessageType::Warning, "Function name can't be a keyword");
		return false;
	}

	// Parse parameters
	while (statement.tokens[index] != '(')
	{
		if (index == statement.tokens.size())
			return true;
		++index;
	}
	++index; // Skip (

	while (statement.tokens[index] != ')' && index < statement.tokens.size())
	{
		parameters_.emplace_back();
		index = parameters_.back().parse(statement.tokens, index);

		if (statement.tokens[index] == ',')
			++index;
	}

	if (dump_parsed_functions)
		log::debug(asString());

	return true;
}

// -----------------------------------------------------------------------------
// Returns a string representation of the function
// -----------------------------------------------------------------------------
string Function::asString()
{
	string str;
	if (!deprecated_.empty())
		str += fmt::format("deprecated v{} ", deprecated_);
	if (static_)
		str += "static ";
	if (native_)
		str += "native ";
	if (virtual_)
		str += "virtual ";
	if (action_)
		str += "action ";

	str += fmt::format("{} {}(", return_type_, name_);

	for (auto& p : parameters_)
	{
		str += fmt::format("{} {}", p.type, p.name);
		if (!p.default_value.empty())
			str.append(" = ").append(p.default_value);

		if (&p != &parameters_.back())
			str += ", ";
	}
	str += ")";

	return str;
}

// -----------------------------------------------------------------------------
// Returns true if [statement] is a valid function declaration
// -----------------------------------------------------------------------------
bool Function::isFunction(ParsedStatement& statement)
{
	// Need at least type, name, (, )
	if (statement.tokens.size() < 4)
		return false;

	// Check for ( before =
	bool special_func = false;
	for (auto& token : statement.tokens)
	{
		if (token == '=')
			return false;

		if (!special_func && token == '(')
			return true;

		if (strutil::equalCI(token, "deprecated") || strutil::equalCI(token, "version"))
			special_func = true;
		else if (special_func && token == ')')
			special_func = false;
	}

	// No ( found
	return false;
}


// -----------------------------------------------------------------------------
//
// State Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the first valid frame sprite (eg. TNT1 A -> TNT1A?)
// -----------------------------------------------------------------------------
string State::editorSprite()
{
	if (frames.empty())
		return "";

	for (auto& f : frames)
		if (!f.sprite_frame.empty())
			return fmt::format("{}{}?", f.sprite_base, f.sprite_frame[0]);

	return "";
}


// -----------------------------------------------------------------------------
//
// StateTable Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Parses a states definition statement/block [states]
// -----------------------------------------------------------------------------
bool StateTable::parse(ParsedStatement& states)
{
	vector<string> current_states;
	for (auto& statement : states.block)
	{
		if (statement.tokens.empty())
			continue;

		auto states_added = false;
		auto index        = 0u;

		// Check for state labels
		for (auto a = 0u; a < statement.tokens.size(); ++a)
		{
			if (statement.tokens[a] == ':')
			{
				// Ignore ::
				if (a + 1 < statement.tokens.size() && statement.tokens[a + 1] == ':')
				{
					++a;
					continue;
				}

				if (!states_added)
					current_states.clear();

				string state;
				for (auto b = index; b < a; ++b)
					state += statement.tokens[b];

				strutil::lowerIP(state);
				current_states.push_back(state);
				if (state_first_.empty())
					state_first_ = state;
				states_added = true;

				index = a + 1;
			}
		}

		if (index >= statement.tokens.size())
		{
			logParserMessage(
				statement,
				log::MessageType::Warning,
				fmt::format("Failed to parse states block beginning on line {}", states.line));
			continue;
		}

		// Ignore state commands
		if (strutil::equalCI(statement.tokens[index], "stop") || strutil::equalCI(statement.tokens[index], "goto")
			|| strutil::equalCI(statement.tokens[index], "loop") || strutil::equalCI(statement.tokens[index], "wait")
			|| strutil::equalCI(statement.tokens[index], "fail"))
			continue;

		if (index + 2 < statement.tokens.size())
		{
			// Parse duration
			int duration = 0;
			if (statement.tokens[index + 2] == '-' && index + 3 < statement.tokens.size())
			{
				// Negative number
				strutil::toInt(statement.tokens[index + 3], duration);
				duration = -duration;
			}
			else
				strutil::toInt(statement.tokens[index + 2], duration);

			for (auto& state : current_states)
				states_[state].frames.push_back({ statement.tokens[index], statement.tokens[index + 1], duration });
		}
	}

	states_.erase(strutil::EMPTY);

	if (dump_parsed_states)
	{
		for (auto& state : states_)
		{
			log::debug("State {}:", state.first);
			for (auto& frame : state.second.frames)
				log::debug(
					"Sprite: {}, Frames: {}, Duration: {}", frame.sprite_base, frame.sprite_frame, frame.duration);
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Returns the most appropriate sprite from the state table to use for the
// editor.
// Uses a state priority: Idle > See > Inactive > Spawn > [first defined]
// -----------------------------------------------------------------------------
string StateTable::editorSprite()
{
	if (!states_["idle"].frames.empty())
		return states_["idle"].editorSprite();
	if (!states_["see"].frames.empty())
		return states_["see"].editorSprite();
	if (!states_["inactive"].frames.empty())
		return states_["inactive"].editorSprite();
	if (!states_["spawn"].frames.empty())
		return states_["spawn"].editorSprite();
	if (!states_[state_first_].frames.empty())
		return states_[state_first_].editorSprite();

	return "";
}


// -----------------------------------------------------------------------------
//
// Class Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Parses a class definition statement/block [class_statement]
// -----------------------------------------------------------------------------
bool Class::parse(ParsedStatement& class_statement, const vector<Class>& parsed_classes)
{
	if (class_statement.tokens.size() < 2)
	{
		logParserMessage(class_statement, log::MessageType::Warning, "Class parse failed");
		return false;
	}

	name_ = class_statement.tokens[1];

	for (unsigned a = 0; a < class_statement.tokens.size(); a++)
	{
		// Inherits
		if (class_statement.tokens[a] == ':' && a < class_statement.tokens.size() - 1)
		{
			inherits_class_ = class_statement.tokens[a + 1];
			for (const auto& pclass : parsed_classes)
				if (strutil::equalCI(pclass.name_, inherits_class_))
				{
					inherit(pclass);
					break;
				}
		}

		// Native
		else if (strutil::equalCI(class_statement.tokens[a], "native"))
			native_ = true;

		// Deprecated
		else if (checkKeywordValueStatement(class_statement.tokens, a, "deprecated", deprecated_))
			a += 3;

		// Version
		else if (checkKeywordValueStatement(class_statement.tokens, a, "version", version_))
			a += 3;
	}

	if (!parseClassBlock(class_statement.block))
	{
		logParserMessage(class_statement, log::MessageType::Warning, "Class parse failed");
		return false;
	}

	// Set editor sprite from parsed states
	default_properties_["sprite"] = states_.editorSprite();

	// Add DB comment props to default properties
	for (auto& i : db_properties_)
	{
		// Sprite
		if (strutil::equalCI(i.first, "EditorSprite") || strutil::equalCI(i.first, "Sprite"))
			default_properties_["sprite"] = i.second;

		// Angled
		else if (strutil::equalCI(i.first, "Angled"))
			default_properties_["angled"] = true;
		else if (strutil::equalCI(i.first, "NotAngled"))
			default_properties_["angled"] = false;

		// Is Decoration
		else if (strutil::equalCI(i.first, "IsDecoration"))
			default_properties_["decoration"] = true;

		// Icon
		else if (strutil::equalCI(i.first, "Icon"))
			default_properties_["icon"] = i.second;

		// DB2 Color
		else if (strutil::equalCI(i.first, "Color"))
			default_properties_["color"] = i.second;

		// SLADE 3 Colour (overrides DB2 color)
		// Good thing US spelling differs from ABC (Aussie/Brit/Canuck) spelling! :p
		else if (strutil::equalCI(i.first, "Colour"))
			default_properties_["colour"] = i.second;

		// Obsolete thing
		else if (strutil::equalCI(i.first, "Obsolete"))
			default_properties_["obsolete"] = true;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Parses a class definition block [block] only (ignores the class declaration
// statement line, used for 'extend class')
// -----------------------------------------------------------------------------
bool Class::extend(ParsedStatement& block)
{
	return parseClassBlock(block.block);
}

// ----------------------------------------------------------------------------
// 'Inherits' data from the given [parent] class
// ----------------------------------------------------------------------------
void Class::inherit(const Class& parent)
{
	// variables_ = parent.variables_;
	// functions_ = parent.functions_;
	// enumerators_ = parent.enumerators_;
	default_properties_ = parent.default_properties_;
	states_             = parent.states_;
	db_properties_      = parent.db_properties_;
}

// ----------------------------------------------------------------------------
// Adds this class as a ThingType to [parsed], or updates an existing ThingType
// definition in [types] or [parsed]
// ----------------------------------------------------------------------------
void Class::toThingType(std::map<int, game::ThingType>& types, vector<game::ThingType>& parsed)
{
	// Find existing definition
	game::ThingType* def = nullptr;

	// Check types with ednums first
	for (auto& type : types)
		if (strutil::equalCI(name_, type.second.className()))
		{
			def = &type.second;
			break;
		}
	if (!def)
	{
		// Check all parsed types
		for (auto& type : parsed)
			if (strutil::equalCI(name_, type.className()))
			{
				def = &type;
				break;
			}
	}

	// Create new type if it didn't exist
	if (!def)
	{
		parsed.emplace_back(name_, "ZScript", name_);
		def = &parsed.back();
	}

	// Set properties from DB comments
	auto   title = name_;
	string group = "ZScript";
	for (auto& prop : db_properties_)
	{
		if (strutil::equalCI(prop.first, "Title"))
			title = prop.second;
		else if (strutil::equalCI(prop.first, "Group") || strutil::equalCI(prop.first, "Category"))
			group = "ZScript/" + prop.second;
	}
	def->define(def->number(), title, group);

	// Set properties from defaults section
	def->loadProps(default_properties_, true, true);
}

// -----------------------------------------------------------------------------
// Parses a class definition from statements in [block]
// -----------------------------------------------------------------------------
bool Class::parseClassBlock(vector<ParsedStatement>& block)
{
	for (auto& statement : block)
	{
		if (statement.tokens.empty())
			continue;

		// Default block
		string_view first_token = statement.tokens[0];
		if (strutil::equalCI(first_token, "default"))
		{
			if (!parseDefaults(statement.block))
				return false;
		}

		// Enum
		else if (strutil::equalCI(first_token, "enum"))
		{
			Enumerator e;
			if (!e.parse(statement))
				return false;

			enumerators_.push_back(e);
		}

		// States
		else if (strutil::equalCI(first_token, "states"))
			states_.parse(statement);

		// DB property comment
		else if (strutil::startsWith(first_token, db_comment))
		{
			if (statement.tokens.size() > 1)
				db_properties_.emplace_back(first_token.substr(3), statement.tokens[1]);
			else
				db_properties_.emplace_back(first_token.substr(3), "true");
		}

		// Function
		else if (Function::isFunction(statement))
		{
			Function fn{ {}, name_ };
			if (fn.parse(statement))
				functions_.push_back(fn);
		}

		// Variable
		else if (statement.tokens.size() >= 2 && statement.block.empty())
			continue;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Parses a 'default' block from statements in [block]
// -----------------------------------------------------------------------------
bool Class::parseDefaults(vector<ParsedStatement>& defaults)
{
	for (auto& statement : defaults)
	{
		if (statement.tokens.empty())
			continue;

		// DB property comment
		if (strutil::startsWith(statement.tokens[0], db_comment))
		{
			string_view prop = statement.tokens[0];
			prop.remove_prefix(3);
			if (statement.tokens.size() > 1)
				db_properties_.emplace_back(prop, statement.tokens[1]);
			else
				db_properties_.emplace_back(prop, "true");
			continue;
		}

		// Flags
		unsigned t     = 0;
		unsigned count = statement.tokens.size();
		while (t < count)
		{
			if (statement.tokens[t] == '+')
				default_properties_[strutil::lower(statement.tokens[++t])] = true;
			else if (statement.tokens[t] == '-')
				default_properties_[strutil::lower(statement.tokens[++t])] = false;
			else
				break;

			t++;
		}

		if (t >= count)
			continue;

		// Name
		auto name = statement.tokens[t];
		if (t + 2 < count && statement.tokens[t + 1] == '.')
		{
			name.append(".").append(statement.tokens[t + 2]);
			t += 2;
		}

		// Value
		// For now ignore anything after the first whitespace/special character
		// so stuff like arithmetic expressions or comma separated lists won't
		// really work properly yet
		if (t + 1 < count)
			default_properties_[strutil::lower(name)] = statement.tokens[t + 1];

		// Name only (no value), set as boolean true
		else if (t < count)
			default_properties_[strutil::lower(name)] = true;
	}

	return true;
}


// -----------------------------------------------------------------------------
//
// Definitions Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Clears all definitions
// -----------------------------------------------------------------------------
void Definitions::clear()
{
	classes_.clear();
	enumerators_.clear();
	variables_.clear();
}

// -----------------------------------------------------------------------------
// Parses ZScript in [entry]
// -----------------------------------------------------------------------------
bool Definitions::parseZScript(ArchiveEntry& entry)
{
	// Parse into tree of expressions and blocks
	auto                    start = app::runTimer();
	vector<ParsedStatement> parsed;
	vector<ArchiveEntry*>   entry_stack;
	parseBlocks(entry, parsed, entry_stack);
	log::debug(2, "parseBlocks: {}ms", app::runTimer() - start);
	start = app::runTimer();

	for (auto& block : parsed)
	{
		if (block.tokens.empty())
			continue;

		if (dump_parsed_blocks)
			block.dump();

		// Class
		if (strutil::equalCI(block.tokens[0], "class"))
		{
			Class nc(Class::Type::Class);

			if (!nc.parse(block, classes_))
				return false;

			classes_.push_back(nc);
		}

		// Struct
		else if (strutil::equalCI(block.tokens[0], "struct"))
		{
			Class nc(Class::Type::Struct);

			if (!nc.parse(block, classes_))
				return false;

			classes_.push_back(nc);
		}

		// Extend Class
		else if (
			block.tokens.size() > 2 && strutil::equalCI(block.tokens[0], "extend")
			&& strutil::equalCI(block.tokens[1], "class"))
		{
			for (auto& c : classes_)
				if (strutil::equalCI(c.name(), block.tokens[2]))
				{
					c.extend(block);
					break;
				}
		}

		// Enum
		else if (strutil::equalCI(block.tokens[0], "enum"))
		{
			Enumerator e;

			if (!e.parse(block))
				return false;

			enumerators_.push_back(e);
		}
	}

	log::debug(2, "ZScript: {}ms", app::runTimer() - start);

	return true;
}

// -----------------------------------------------------------------------------
// Parses all ZScript entries in [archive]
// -----------------------------------------------------------------------------
bool Definitions::parseZScript(const Archive& archive)
{
	// Get base ZScript file
	Archive::SearchOptions opt;
	opt.match_name      = "zscript";
	opt.ignore_ext      = true;
	auto zscript_enries = archive.findAll(opt);
	if (zscript_enries.empty())
		return false;

	log::info(2, "Parsing ZScript entries found in archive {}", archive.filename());

	// Get ZScript entry type (all parsed ZScript entries will be set to this)
	etype_zscript = EntryType::fromId("zscript");
	if (etype_zscript == EntryType::unknownType())
		etype_zscript = nullptr;

	// Parse ZScript entries
	bool ok = true;
	for (auto entry : zscript_enries)
		if (!parseZScript(*entry))
			ok = false;

	return ok;
}

// -----------------------------------------------------------------------------
// Exports all classes to ThingTypes in [types] and [parsed] (from a
// Game::Configuration object)
// -----------------------------------------------------------------------------
void Definitions::exportThingTypes(std::map<int, game::ThingType>& types, vector<game::ThingType>& parsed)
{
	for (auto& cdef : classes_)
		cdef.toThingType(types, parsed);
}


// -----------------------------------------------------------------------------
//
// ParsedStatement Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Parses a ZScript 'statement'. This isn't technically correct but suits our
// purposes well enough
//
// tokens
// {
//     block[0].tokens
//     {
//         block[0].block[0].tokens;
//         ...
//     }
//
//     block[1].tokens;
//     ...
// }
// -----------------------------------------------------------------------------
bool ParsedStatement::parse(Tokenizer& tz)
{
	// Check for unexpected token
	if (tz.check('}'))
	{
		tz.adv();
		return false;
	}

	line = tz.lineNo();

	// Tokens
	bool in_initializer = false;
	while (true)
	{
		// End of statement (;)
		if (tz.advIf(';'))
			return true;

		// DB comment
		if (strutil::startsWith(tz.current().text, db_comment))
		{
			tokens.emplace_back(tz.current().text);
			tokens.emplace_back(tz.getLine());
			return true;
		}

		if (tz.check('}'))
		{
			// End of array initializer
			if (in_initializer)
			{
				in_initializer = false;
				tokens.emplace_back("}");
				tz.adv();
				continue;
			}

			// End of statement
			return true;
		}

		if (tz.atEnd())
		{
			log::debug("Failed parsing zscript statement/block beginning line {}", line);
			return false;
		}

		// Beginning of block
		if (tz.advIf('{'))
			break;

		// Array initializer: ... = { ... }
		if (tz.current() == '=' && tz.peek() == '{')
		{
			tokens.emplace_back("=");
			tokens.emplace_back("{");
			tz.adv(2);
			in_initializer = true;
			continue;
		}

		tokens.emplace_back(tz.current().text);
		tz.adv();
	}

	// Block
	while (true)
	{
		if (tz.advIf('}'))
			return true;

		if (tz.atEnd())
		{
			log::debug("Failed parsing zscript statement/block beginning line {}", line);
			return false;
		}

		block.push_back({});
		block.back().entry = entry;
		if (!block.back().parse(tz) || block.back().tokens.empty())
			block.pop_back();
	}
}

// -----------------------------------------------------------------------------
// Dumps this statement to the log (debug), indenting by 2*[indent] spaces
// -----------------------------------------------------------------------------
void ParsedStatement::dump(int indent)
{
	string line;
	for (int a = 0; a < indent; a++)
		line += "  ";

	// Tokens
	for (auto& token : tokens)
		line += token + " ";
	log::debug(line);

	// Blocks
	for (auto& b : block)
		b.dump(indent + 1);
}



Parser::Parser()
{
	bool ok = db_.open(database::programDatabasePath());

	ps_insert_identifier_     = std::make_unique<SQLite::Statement>(*db_.connectionRW(), sql_insert_identifier);
	ps_insert_enum_value_     = std::make_unique<SQLite::Statement>(*db_.connectionRW(), sql_insert_enum_value);
	ps_insert_class_          = std::make_unique<SQLite::Statement>(*db_.connectionRW(), sql_insert_class);
	ps_insert_class_default_  = std::make_unique<SQLite::Statement>(*db_.connectionRW(), sql_insert_class_default);
	ps_insert_class_ed_prop_  = std::make_unique<SQLite::Statement>(*db_.connectionRW(), sql_insert_class_editor_prop);
	ps_insert_struct_         = std::make_unique<SQLite::Statement>(*db_.connectionRW(), sql_insert_struct);
	ps_insert_function_       = std::make_unique<SQLite::Statement>(*db_.connectionRW(), sql_insert_function);
	ps_insert_function_param_ = std::make_unique<SQLite::Statement>(*db_.connectionRW(), sql_insert_function_parameter);
	ps_insert_state_frame_    = std::make_unique<SQLite::Statement>(*db_.connectionRW(), sql_insert_state_frame);
}

bool Parser::parseZScript(ArchiveEntry& entry, bool base_source)
{
	int source_id;
	if (base_source)
	{
		source_id = 0;

		// Delete everything defined in the 'base' source (ie. gzdoom.pk3, source_id 0)
		db_.enableForeignKeyConstraints(true);
		db_.exec("DELETE FROM zs_identifier WHERE source_id = 0");
		db_.enableForeignKeyConstraints(false);
	}
	else
	{
		auto archive_id = library::archiveFileId(*entry.parent(), &db_);

		db_.enableForeignKeyConstraints(true);
		SQLite::Statement ps_remove_source(
			*db_.connectionRW(), "DELETE FROM zs_source WHERE archive_file_id = ? AND entry_path = ?");
		ps_remove_source.bind(1, archive_id);
		ps_remove_source.bind(2, entry.path(true));
		ps_remove_source.exec();
		db_.enableForeignKeyConstraints(false);

		SQLite::Statement ps_insert_source(*db_.connectionRW(), sql_insert_source);
		ps_insert_source.bind(1, archive_id);
		ps_insert_source.bind(2, entry.path(true));
		ps_insert_source.exec();

		source_id = db_.lastRowId();
	}

	// Parse into tree of expressions and blocks
	auto                    start = app::runTimer();
	vector<ParsedStatement> parsed;
	vector<ArchiveEntry*>   entry_stack;
	parseBlocks(entry, parsed, entry_stack);
	log::info(2, "parseBlocks (DB): {}ms", app::runTimer() - start);
	start = app::runTimer();

	auto transaction = SQLite::Transaction(*db_.connectionRW());

	for (auto& statement : parsed)
	{
		if (statement.tokens.empty())
			continue;

		if (dump_parsed_blocks)
			statement.dump();


		// Class
		if (strutil::equalCI(statement.tokens[0], "class"))
		{
			if (!parseClass(statement, source_id))
				return false;
		}

		// Struct
		else if (strutil::equalCI(statement.tokens[0], "struct"))
		{
			if (!parseStruct(statement, source_id, 0))
				return false;
		}

		// Extend Class
		else if (
			statement.tokens.size() > 2 && strutil::equalCI(statement.tokens[0], "extend")
			&& strutil::equalCI(statement.tokens[1], "class"))
		{
			if (auto id = findClassIdentifierId(statement.tokens[2], source_id, &db_); id > 0)
			{
				if (!parseClassBlock(statement.block, source_id, id))
					return false;
			}
			else
				logParserMessage(
					statement,
					log::MessageType::Warning,
					fmt::format("Unknown class \"{}\" for extend class", statement.tokens[2]));
		}

		// Enum
		else if (strutil::equalCI(statement.tokens[0], "enum"))
		{
			if (!parseEnum(statement, source_id, 0))
				return false;
		}

		// Const
		else if (strutil::equalCI(statement.tokens[0], "const"))
		{
			if (!parseConst(statement, source_id, 0))
				return false;
		}

		// Static Array
		else if (
			statement.tokens.size() > 2 && strutil::equalCI(statement.tokens[0], "static")
			&& strutil::equalCI(statement.tokens[1], "const"))
		{
			// TODO
		}

		// Unknown
		else
			zscript::logParserMessage(
				statement, log::MessageType::Warning, fmt::format("Unknown keyword {}", statement.tokens[0]));
	}

	transaction.commit();

	log::info(2, "ZScript (DB): {}ms", app::runTimer() - start);

	return true;
}

bool Parser::parseZScript(shared_ptr<Archive> archive)
{
	// Clear definitions from archive in the database
	auto archive_id = library::archiveFileId(*archive, &db_);
	if (archive_id > 0)
		db_.exec(fmt::format("DELETE FROM zs_source WHERE archive_file_id = {}", archive_id));

	// Get base ZScript file
	Archive::SearchOptions opt;
	opt.match_name      = "zscript";
	opt.ignore_ext      = true;
	auto zscript_enries = archive->findAll(opt);
	if (zscript_enries.empty())
		return false;

	log::info(2, "Parsing ZScript entries found in archive {}", archive->filename());

	// Get ZScript entry type (all parsed ZScript entries will be set to this)
	etype_zscript = EntryType::fromId("zscript");
	if (etype_zscript == EntryType::unknownType())
		etype_zscript = nullptr;

	// Parse ZScript entries
	bool ok = true;
	for (auto entry : zscript_enries)
	{
		auto e_shared = entry->getShared();
		if (!parseZScript(*e_shared))
			ok = false;
	}

	return ok;
}

bool Parser::parseEnum(ParsedStatement& enum_statement, int source_id, int parent_id)
{
	// Check valid statement
	if (enum_statement.block.empty())
		return false;
	if (enum_statement.tokens.size() < 2)
		return false;

	// Parse name
	ps_insert_identifier_->bind(1, source_id);
	ps_insert_identifier_->bind(2, static_cast<int>(IdentifierType::Enumerator));
	ps_insert_identifier_->bind(3, enum_statement.tokens[1]);
	ps_insert_identifier_->bind(4, parent_id);
	ps_insert_identifier_->exec();
	ps_insert_identifier_->reset();
	auto identifier_id = db_.lastRowId();

	// Parse values
	unsigned index = 0;
	unsigned count = enum_statement.block[0].tokens.size();
	while (index < count)
	{
		ps_insert_enum_value_->bind(1, identifier_id);
		ps_insert_enum_value_->bind(2, enum_statement.block[0].tokens[index]);
		ps_insert_enum_value_->bind(3, 0); // TODO: Parse value
		ps_insert_enum_value_->exec();
		ps_insert_enum_value_->reset();

		// Skip past next ,
		while (index + 1 < count)
			if (enum_statement.block[0].tokens[++index] == ',')
				break;

		index++;
	}

	return true;
}

bool zscript::Parser::parseClass(ParsedStatement& class_statement, int source_id)
{
	const auto& tokens       = class_statement.tokens;
	const auto  tokens_count = class_statement.tokens.size();

	if (tokens_count < 2)
	{
		logParserMessage(class_statement, log::MessageType::Warning, "Class parse failed");
		return false;
	}

	auto&  name = tokens[1];
	string base_class, version, replaces_class;
	bool   native   = false;
	bool   abstract = false;
	auto   scope    = ObjectScope::Data;

	for (unsigned a = 2; a < tokens_count; a++)
	{
		// Inherits
		if (tokens[a] == ':' && a < tokens_count - 1)
		{
			base_class = tokens[a + 1];
			++a;
		}

		// Native
		else if (strutil::equalCI(tokens[a], "native"))
			native = true;

		// Abstract
		else if (strutil::equalCI(tokens[a], "abstract"))
			abstract = true;

		// Version
		else if (checkKeywordValueStatement(tokens, a, "version", version))
			a += 3;

		// Replaces
		else if (strutil::equalCI(tokens[a], "replaces") && a < tokens_count - 1)
		{
			replaces_class = tokens[a + 1];
			++a;
		}

		// Play scope
		else if (strutil::equalCI(tokens[a], "play"))
			scope = ObjectScope::Play;

		// UI scope
		else if (strutil::equalCI(tokens[a], "ui"))
			scope = ObjectScope::UI;

		// Unknown
		else
			logParserMessage(
				class_statement,
				log::MessageType::Warning,
				fmt::format("Unexpected token \"{}\" in class definition", tokens[a]));
	}

	// Add identifier row
	ps_insert_identifier_->bind(1, source_id);
	ps_insert_identifier_->bind(2, static_cast<int>(IdentifierType::Class));
	ps_insert_identifier_->bind(3, name);
	ps_insert_identifier_->bind(4, 0);
	ps_insert_identifier_->exec();
	ps_insert_identifier_->reset();
	auto identifier_id = db_.lastRowId();

	// Add class row
	ps_insert_class_->bind(1, identifier_id);
	ps_insert_class_->bind(2, static_cast<int>(scope));
	ps_insert_class_->bind(3, base_class);
	ps_insert_class_->bind(4, abstract);
	ps_insert_class_->bind(5, native);
	ps_insert_class_->bind(6, replaces_class);
	ps_insert_class_->bind(7, version);
	ps_insert_class_->exec();
	ps_insert_class_->reset();

	if (!parseClassBlock(class_statement.block, source_id, identifier_id))
	{
		logParserMessage(class_statement, log::MessageType::Warning, "Class parse failed");
		return false;
	}

	return true;
}

bool zscript::Parser::parseClassBlock(vector<ParsedStatement>& block, int source_id, int class_id)
{
	for (auto& statement : block)
	{
		if (statement.tokens.empty())
			continue;

		// Default block
		string_view first_token = statement.tokens[0];
		if (strutil::equalCI(first_token, "default"))
		{
			if (!parseClassDefaults(statement.block, class_id))
				return false;
		}

		// States
		else if (strutil::equalCI(first_token, "states"))
		{
			if (!parseStateTable(statement, source_id, class_id))
				return false;
		}

		// Enum
		else if (strutil::equalCI(first_token, "enum"))
		{
			if (!parseEnum(statement, source_id, class_id))
				return false;
		}

		// Struct
		else if (strutil::equalCI(first_token, "struct"))
		{
			if (!parseStruct(statement, source_id, class_id))
				return false;
		}

		// DB property comment
		else if (strutil::startsWith(first_token, db_comment))
		{
			/*if (statement.tokens.size() > 1)
				db_properties_.emplace_back(first_token.substr(3), statement.tokens[1]);
			else
				db_properties_.emplace_back(first_token.substr(3), "true");*/
		}

		// Function
		else if (Function::isFunction(statement))
		{
			if (!parseFunction(statement, source_id, class_id))
				return false;
		}

		// Variable
		else if (statement.tokens.size() >= 2 && statement.block.empty())
			continue;
	}

	return true;
}

bool zscript::Parser::parseClassDefaults(vector<ParsedStatement>& defaults, int class_id)
{
	for (auto& statement : defaults)
	{
		if (statement.tokens.empty())
			continue;

		// DB property comment
		if (strutil::startsWith(statement.tokens[0], db_comment))
		{
			string_view prop = statement.tokens[0];
			prop.remove_prefix(3);

			ps_insert_class_ed_prop_->bind(1, class_id);
			ps_insert_class_ed_prop_->bind(2, string{ prop });

			if (statement.tokens.size() > 1)
				ps_insert_class_ed_prop_->bind(3, statement.tokens[1]);
			else
				ps_insert_class_ed_prop_->bind(3, "true");

			ps_insert_class_ed_prop_->exec();
			ps_insert_class_ed_prop_->reset();

			continue;
		}

		// Flags
		unsigned t     = 0;
		unsigned count = statement.tokens.size();
		while (t < count)
		{
			if (statement.tokens[t] == '+')
				ps_insert_class_default_->bind(3, "true");
			else if (statement.tokens[t] == '-')
				ps_insert_class_default_->bind(3, "false");
			else
				break;

			ps_insert_class_default_->bind(1, class_id);
			ps_insert_class_default_->bind(2, statement.tokens[++t]);
			ps_insert_class_default_->exec();
			ps_insert_class_default_->reset();

			t++;
		}

		if (t >= count)
			continue;

		// Name
		auto name = statement.tokens[t];
		if (t + 2 < count && statement.tokens[t + 1] == '.')
		{
			name.append(".").append(statement.tokens[t + 2]);
			t += 2;
		}

		ps_insert_class_default_->bind(1, class_id);
		ps_insert_class_default_->bind(2, name);

		// Value
		// For now ignore anything after the first whitespace/special character
		// so stuff like arithmetic expressions or comma separated lists won't
		// really work properly yet
		if (t + 1 < count)
			ps_insert_class_default_->bind(3, statement.tokens[t + 1]);

		// Name only (no value), set as boolean true
		else if (t < count)
			ps_insert_class_default_->bind(3, "true");

		else
			continue;

		ps_insert_class_default_->exec();
		ps_insert_class_default_->reset();
	}

	return true;
}

bool zscript::Parser::parseStruct(ParsedStatement& struct_statement, int source_id, int parent_id)
{
	const auto tokens_count = struct_statement.tokens.size();

	if (tokens_count < 2)
	{
		logParserMessage(struct_statement, log::MessageType::Warning, "Struct parse failed");
		return false;
	}

	auto&  name = struct_statement.tokens[1];
	string version;
	bool   native = false;
	auto   scope  = ObjectScope::Data;

	for (unsigned a = 2; a < tokens_count; a++)
	{
		// Native
		if (strutil::equalCI(struct_statement.tokens[a], "native"))
			native = true;

		// Version
		else if (checkKeywordValueStatement(struct_statement.tokens, a, "version", version))
			a += 3;

		// Play scope
		else if (strutil::equalCI(struct_statement.tokens[a], "play"))
			scope = ObjectScope::Play;

		// UI scope
		else if (strutil::equalCI(struct_statement.tokens[a], "ui"))
			scope = ObjectScope::UI;

		// Data scope
		else if (strutil::equalCI(struct_statement.tokens[a], "clearscope"))
			scope = ObjectScope::Data;
	}

	// Add identifier row
	ps_insert_identifier_->bind(1, source_id);
	ps_insert_identifier_->bind(2, static_cast<int>(IdentifierType::Struct));
	ps_insert_identifier_->bind(3, name);
	ps_insert_identifier_->bind(4, parent_id);
	ps_insert_identifier_->exec();
	ps_insert_identifier_->reset();
	auto identifier_id = db_.lastRowId();

	// Add struct row
	ps_insert_struct_->bind(1, identifier_id);
	ps_insert_struct_->bind(2, static_cast<int>(scope));
	ps_insert_struct_->bind(3, native);
	ps_insert_struct_->bind(4, version);
	ps_insert_struct_->exec();
	ps_insert_struct_->reset();

	// TODO: Parse struct block

	return true;
}

bool zscript::Parser::parseConst(ParsedStatement& const_statement, int source_id, int parent_id)
{
	// Check valid statement
	if (const_statement.tokens.size() < 4)
		return false;

	// Add identifier row
	ps_insert_identifier_->bind(1, source_id);
	ps_insert_identifier_->bind(2, static_cast<int>(IdentifierType::Const));
	ps_insert_identifier_->bind(3, const_statement.tokens[1]);
	ps_insert_identifier_->bind(4, parent_id);
	ps_insert_identifier_->exec();
	ps_insert_identifier_->reset();
	// auto identifier_id = db_.lastRowId();

	// TODO: Parse/store value?

	return true;
}

bool zscript::Parser::parseFunction(ParsedStatement& func_statement, int source_id, int parent_id)
{
	struct Parameter
	{
		string name;
		string type;
		string default_value;
	};

	string      name, returns, deprecated, version, action_scope;
	Visibility  visibility = Visibility::Public;
	ObjectScope scope      = ObjectScope::Data;
	bool        action, bconst, final, native, override, bstatic, vararg, bvirtual, virtualscope;

	action = bconst = final = native = override = bstatic = vararg = bvirtual = virtualscope = false;

	// Parse from last to first token (easier this way)
	const auto& tokens      = func_statement.tokens;
	unsigned    token_count = tokens.size();
	unsigned    i           = token_count - 1;

	// Find end of parameters
	while (tokens[i] != ")")
	{
		// Const
		if (strutil::equalCI(tokens.back(), "const"))
			bconst = true;

		--i;
	}
	auto params_end = i;

	// Find start of parameters
	while (tokens[i] != "(")
		--i;

	// Parse parameters
	vector<Parameter> params;
	unsigned          pi = i + 1;
	while (pi < params_end)
	{
		auto type = parseType(tokens, pi);

		if (type == "...")
		{
			params.push_back({ "...", {}, {} });
			break;
		}
		else
		{
			params.push_back({ tokens[pi++], type, {} });
			if (pi < params_end && tokens[pi] == '=')
			{
				++pi;
				params.back().default_value = parseValue(tokens, pi);
			}
		}

		if (pi < params_end && tokens[pi] == strutil::COMMA)
			++pi;
	}

	// Name
	name = tokens[--i];

	// Returns
	returns = tokens[--i];
	while (i >= 1 && tokens[i - 1] == ",")
	{
		i -= 2;
		returns = tokens[i] + ", " + returns;
	}
	auto att_end = i;

	// Attributes/flags
	for (unsigned ti = 0; ti < att_end; ++ti)
	{
		if (checkKeywordValueStatement(tokens, ti, "action", action_scope))
			ti += 3;
		else if (strutil::equalCI(tokens[ti], "action"))
			action = true;
		else if (strutil::equalCI(tokens[ti], "final"))
			final = true;
		else if (strutil::equalCI(tokens[ti], "native"))
			native = true;
		else if (strutil::equalCI(tokens[ti], "override"))
			override = true;
		else if (strutil::equalCI(tokens[ti], "static"))
			bstatic = true;
		else if (strutil::equalCI(tokens[ti], "vararg"))
			vararg = true;
		else if (strutil::equalCI(tokens[ti], "virtual"))
			bvirtual = true;
		else if (strutil::equalCI(tokens[ti], "virtualscope"))
			virtualscope = true;
		else if (strutil::equalCI(tokens[ti], "private"))
			visibility = Visibility::Private;
		else if (strutil::equalCI(tokens[ti], "protected"))
			visibility = Visibility::Protected;
		else if (strutil::equalCI(tokens[ti], "play"))
			scope = ObjectScope::Play;
		else if (strutil::equalCI(tokens[ti], "ui"))
			scope = ObjectScope::UI;
		else if (checkKeywordValueStatement(tokens, ti, "deprecated", deprecated))
			ti += 3;
		else if (checkKeywordValueStatement(tokens, ti, "version", version))
			ti += 3;
	}

	// Add identifier row
	ps_insert_identifier_->bind(1, source_id);
	ps_insert_identifier_->bind(2, static_cast<int>(IdentifierType::Function));
	ps_insert_identifier_->bind(3, name);
	ps_insert_identifier_->bind(4, parent_id);
	ps_insert_identifier_->exec();
	ps_insert_identifier_->reset();
	auto identifier_id = db_.lastRowId();

	// Add function row
	ps_insert_function_->bind(1, identifier_id);
	ps_insert_function_->bind(2, static_cast<int>(scope));
	ps_insert_function_->bind(3, returns);
	ps_insert_function_->bind(4, static_cast<int>(visibility));
	ps_insert_function_->bind(5, action);
	ps_insert_function_->bind(6, action_scope);
	ps_insert_function_->bind(7, bconst);
	ps_insert_function_->bind(8, final);
	ps_insert_function_->bind(9, native);
	ps_insert_function_->bind(10, override);
	ps_insert_function_->bind(11, bstatic);
	ps_insert_function_->bind(12, vararg);
	ps_insert_function_->bind(13, bvirtual);
	ps_insert_function_->bind(14, virtualscope);
	ps_insert_function_->bind(15, deprecated);
	ps_insert_function_->bind(16, version);
	ps_insert_function_->exec();
	ps_insert_function_->reset();

	// Add parameters
	auto index = 0;
	for (const auto& param : params)
	{
		ps_insert_function_param_->bind(1, identifier_id);
		ps_insert_function_param_->bind(2, index++);
		ps_insert_function_param_->bind(3, param.name);
		ps_insert_function_param_->bind(4, param.type);
		ps_insert_function_param_->bind(5, param.default_value);
		ps_insert_function_param_->exec();
		ps_insert_function_param_->reset();
	}

	return true;
}

bool zscript::Parser::parseStateTable(const ParsedStatement& states_statement, int source_id, int parent_id)
{
	std::map<string, State> parsed_states;
	vector<string>          current_states;
	for (const auto& statement : states_statement.block)
	{
		if (statement.tokens.empty())
			continue;

		auto states_added = false;
		auto index        = 0u;

		// Check for state labels
		for (auto a = 0u; a < statement.tokens.size(); ++a)
		{
			if (statement.tokens[a] == ':')
			{
				// Ignore ::
				if (a + 1 < statement.tokens.size() && statement.tokens[a + 1] == ':')
				{
					++a;
					continue;
				}

				if (!states_added)
					current_states.clear();

				string state;
				for (auto b = index; b < a; ++b)
					state += statement.tokens[b];

				strutil::lowerIP(state);
				current_states.push_back(state);
				// if (state_first_.empty())
				//	state_first_ = state;
				states_added = true;

				index = a + 1;
			}
		}

		if (index >= statement.tokens.size())
		{
			logParserMessage(
				statement,
				log::MessageType::Warning,
				fmt::format("Failed to parse states block beginning on line {}", states_statement.line));
			continue;
		}

		// Ignore state commands
		if (strutil::equalCI(statement.tokens[index], "stop") || strutil::equalCI(statement.tokens[index], "goto")
			|| strutil::equalCI(statement.tokens[index], "loop") || strutil::equalCI(statement.tokens[index], "wait")
			|| strutil::equalCI(statement.tokens[index], "fail"))
			continue;

		if (index + 2 < statement.tokens.size())
		{
			// Parse duration
			int duration = 0;
			if (statement.tokens[index + 2] == '-' && index + 3 < statement.tokens.size())
			{
				// Negative number
				strutil::toInt(statement.tokens[index + 3], duration);
				duration = -duration;
			}
			else
				strutil::toInt(statement.tokens[index + 2], duration);

			for (auto& state : current_states)
				parsed_states[state].frames.push_back(
					{ statement.tokens[index], statement.tokens[index + 1], duration });
		}
	}

	parsed_states.erase(strutil::EMPTY);

	if (dump_parsed_states)
	{
		for (auto& state : parsed_states)
		{
			log::debug("State {}:", state.first);
			for (auto& frame : state.second.frames)
				log::debug(
					"Sprite: {}, Frames: {}, Duration: {}", frame.sprite_base, frame.sprite_frame, frame.duration);
		}
	}

	// Write states to database
	for (const auto& state : parsed_states)
	{
		// Add identifier row
		ps_insert_identifier_->bind(1, source_id);
		ps_insert_identifier_->bind(2, static_cast<int>(IdentifierType::State));
		ps_insert_identifier_->bind(3, state.first);
		ps_insert_identifier_->bind(4, parent_id);
		ps_insert_identifier_->exec();
		ps_insert_identifier_->reset();
		auto identifier_id = db_.lastRowId();

		// Add frame rows
		for (const auto& frame : state.second.frames)
		{
			ps_insert_state_frame_->bind(1, identifier_id);
			ps_insert_state_frame_->bind(2, frame.sprite_base);
			ps_insert_state_frame_->bind(3, frame.sprite_frame);
			ps_insert_state_frame_->bind(4, frame.duration);
			ps_insert_state_frame_->exec();
			ps_insert_state_frame_->reset();
		}
	}

	return true;
}








// TESTING CONSOLE COMMANDS
#include "General/Console.h"
#include "MainEditor/MainEditor.h"

CONSOLE_COMMAND(test_parse_zscript, 0, false)
{
	dump_parsed_blocks    = false;
	dump_parsed_states    = false;
	dump_parsed_functions = false;
	ArchiveEntry* entry   = nullptr;

	for (auto& arg : args)
	{
		if (strutil::equalCI(arg, "dump"))
			dump_parsed_blocks = true;
		else if (strutil::equalCI(arg, "states"))
			dump_parsed_states = true;
		else if (strutil::equalCI(arg, "func"))
			dump_parsed_functions = true;
		else if (!entry)
			entry = maineditor::currentArchive()->entryAtPath(arg);
	}

	if (!entry)
		entry = maineditor::currentEntry();

	if (entry)
	{
		Definitions test;
		if (test.parseZScript(*entry))
			log::console("Parsed Successfully");
		else
			log::console("Parsing failed");
	}
	else
		log::console("Select an entry or enter a valid entry name/path");

	dump_parsed_blocks    = false;
	dump_parsed_states    = false;
	dump_parsed_functions = false;
}


CONSOLE_COMMAND(test_parseblocks, 1, false)
{
	int  num   = strutil::asInt(args[0]);
	auto entry = maineditor::currentEntry();
	if (!entry)
		return;

	auto                    start = app::runTimer();
	vector<ParsedStatement> parsed;
	vector<ArchiveEntry*>   entry_stack;
	for (auto a = 0; a < num; ++a)
	{
		parseBlocks(*entry, parsed, entry_stack);
		parsed.clear();
	}
	log::console(fmt::format("Took {}ms", app::runTimer() - start));
}
