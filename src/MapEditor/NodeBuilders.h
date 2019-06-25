#pragma once

namespace NodeBuilders
{
struct Builder
{
	string         id;
	string         name;
	string         command;
	string         exe;
	vector<string> options;
	vector<string> option_desc;
};

void     init();
unsigned nNodeBuilders();
Builder& builder(string_view id);
Builder& builder(unsigned index);
string   builderPath(string_view id);
string   builderPath(unsigned index);
void     setBuilderPath(string_view id, string_view path);
} // namespace NodeBuilders
