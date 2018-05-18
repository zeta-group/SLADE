#pragma once

#include "Utility/PropertyList/Property.h"

class MobjPropertyList
{
public:
	struct Entry
	{
		string		name;
		Property	value;

		Entry(string_view name) : name{ name.data(), name.size() } {}
		Entry(string_view name, const Property& value) : name{ name.data(), name.size() }, value{ value } {}
	};

	MobjPropertyList() = default;
	~MobjPropertyList() = default;

	// Operator for direct access to hash map
	Property& operator[](string_view key)
	{
		for (auto& entry : properties_)
			if (entry.name == key)
				return entry.value;

		properties_.emplace_back(key);
		return properties_.back().value;
	}

	vector<Entry>&	allProperties() { return properties_; }

	void	clear() { properties_.clear(); }
	bool	propertyExists(string_view key) const;
	bool	removeProperty(string_view key);
	void	copyTo(MobjPropertyList& list) const;
	void	addFlag(string_view key) { properties_.emplace_back(key, Property{}); }
	bool	isEmpty() const { return properties_.empty(); }

	string	toString(bool condensed = false) const;

private:
	vector<Entry>	properties_;
};
