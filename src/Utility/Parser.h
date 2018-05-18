#pragma once

#include "PropertyList/Property.h"
#include "Tree.h"
#include "Utility/StringUtils.h"

class ArchiveTreeNode;
class Parser;
class Tokenizer;

class ParseTreeNode : public STreeNode
{
public:
	ParseTreeNode(
		ParseTreeNode*   parent      = nullptr,
		Parser*          parser      = nullptr,
		ArchiveTreeNode* archive_dir = nullptr,
		string_view      type        = "");
	~ParseTreeNode() = default;

	string_view name() const override { return name_; }
	void        setName(string_view name) override { this->name_.assign(name.data(), name.size()); }

	const string&           inherit() const { return inherit_; }
	const string&           type() const { return type_; }
	const vector<Property>& values() const { return values_; }

	unsigned nValues() const { return values_.size(); }
	Property value(unsigned index = 0);
	string   stringValue(unsigned index = 0);
	int      intValue(unsigned index = 0);
	bool     boolValue(unsigned index = 0);
	double   floatValue(unsigned index = 0);

	// To avoid need for casts everywhere
	ParseTreeNode* getChildPTN(const string& name) { return dynamic_cast<ParseTreeNode*>(child(name)); }
	ParseTreeNode* getChildPTN(unsigned index) { return dynamic_cast<ParseTreeNode*>(childAt(index)); }

	ParseTreeNode* addChildPTN(string_view name, string_view type = "");
	void           addStringValue(const string& value) { values_.emplace_back(value); }
	void           addIntValue(int value) { values_.emplace_back(value); }
	void           addBoolValue(bool value) { values_.emplace_back(value); }
	void           addFloatValue(double value) { values_.emplace_back(value); }

	bool parse(Tokenizer& tz);
	void write(string& out, int indent = 0) const;

	typedef std::unique_ptr<ParseTreeNode> UPtr;

protected:
	STreeNode* createChild(string_view name) override
	{
		auto ret = new ParseTreeNode();
		ret->setName(name);
		ret->parser_ = parser_;
		return ret;
	}

private:
	string           name_;
	string           inherit_;
	string           type_;
	vector<Property> values_;
	Parser*          parser_;
	ArchiveTreeNode* archive_dir_;

	void logError(const Tokenizer& tz, const string& error) const;
	bool parsePreprocessor(Tokenizer& tz);
	bool parseAssignment(Tokenizer& tz, ParseTreeNode* child) const;
};

class Parser
{
public:
	Parser(ArchiveTreeNode* dir_root = nullptr);
	~Parser() = default;

	ParseTreeNode* parseTreeRoot() const { return pt_root_.get(); }

	void setCaseSensitive(bool cs) { case_sensitive_ = cs; }

	bool parseText(MemChunk& mc, string_view source = "memory chunk", bool debug = false) const;
	bool parseText(string_view text, string_view source = "string", bool debug = false) const;
	void define(const string& def) { defines_.push_back(StrUtil::lower(def)); }
	bool defined(const string& def) { return VECTOR_EXISTS(defines_, StrUtil::lower(def)); }

	// To simplify casts from STreeNode to ParseTreeNode
	static ParseTreeNode* node(STreeNode* node) { return dynamic_cast<ParseTreeNode*>(node); }

private:
	ParseTreeNode::UPtr pt_root_;
	vector<string>      defines_;
	ArchiveTreeNode*    archive_dir_root_ = nullptr;
	bool                case_sensitive_   = false;
};
