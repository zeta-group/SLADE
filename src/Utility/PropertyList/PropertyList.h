#pragma once

#include "common.h"
#include "Property.h"

class PropertyList
{
public:
	PropertyList() = default;
	~PropertyList() = default;

	// Operator for direct access to hash map
	Property& operator[](const string& key) { return properties_[key]; }

	void	clear() { properties_.clear(); }
	bool	propertyExists(const string& key);
	bool    removeProperty(const string& key);
	void	copyTo(PropertyList& list);
	void    addFlag(const string& key);
	void	allProperties(vector<Property>& list);
	void	allPropertyNames(vector<string>& list);
	bool	isEmpty() const { return properties_.empty(); }

	string	toString(bool condensed = false);

private:
	std::map<string, Property> properties_;
};
