
#ifndef __LUA_H__
#define __LUA_H__

class wxWindow;
namespace sol { class state; }

namespace Lua
{
	bool	init();
	void 	initLibs();
	void	close();

	struct Error
	{
		string	type;
		string	message;
		int		line_no;
	};
	Error&	error();
	void	showErrorDialog(
		wxWindow* parent = nullptr,
		const string& title = "Script Error",
		const string& message = "An error occurred running the script, see details below"
	);

	bool	run(string program, bool sandbox = true);
	bool	runFile(string filename, bool sandbox = true);
	bool	runArchiveScript(const string& script, Archive* archive);

	string	serializeTable(const string& table_name);
	string	serializeTable(sol::table table);

	sol::state&	state();

	wxWindow*	currentWindow();
	void		setCurrentWindow(wxWindow* window);
}

#endif//__LUA_H__
