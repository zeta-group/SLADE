#pragma once

namespace SFileDialog
{
	struct FileInfo
	{
		vector<string>	filenames;
		string			extension;
		int				ext_index;
		string			path;
	};

	bool	openFile(
				FileInfo& info,
				string_view caption,
				string_view extensions,
				wxWindow* parent = nullptr,
				string_view fn_default = "",
				int ext_default = 0
			);
	bool	openFiles(
				FileInfo& info,
				string_view caption,
				string_view extensions,
				wxWindow* parent = nullptr,
				string_view fn_default = "",
				int ext_default = 0
			);
	bool	saveFile(
				FileInfo& info,
				string_view caption,
				string_view extensions,
				wxWindow* parent = nullptr,
				string_view fn_default = "",
				int ext_default = 0
			);
	bool	saveFiles(
				FileInfo& info,
				string_view caption,
				string_view extensions,
				wxWindow* parent = nullptr,
				int ext_default = 0
			);

	string	executableExtensionString();
	string	executableFileName(const string& exe_name);
}
