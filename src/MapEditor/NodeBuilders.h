#pragma once

#include "common.h"

namespace NodeBuilders
{
struct Builder
{
	string         id;
	string         name;
	string         path;
	string         command;
	string         exe;
	vector<string> options;
	vector<string> option_desc;
};

void     init();
void     addBuilderPath(string_view builder, string_view path);
void     saveBuilderPaths(wxFile& file);
unsigned nNodeBuilders();
Builder& getBuilder(string_view id);
Builder& getBuilder(unsigned index);
} // namespace NodeBuilders
