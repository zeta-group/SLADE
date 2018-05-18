#pragma once

#include "common.h"

class ConsolePanel : public wxPanel
{
public:
	ConsolePanel(wxWindow* parent, int id);
	~ConsolePanel() = default;

	void initLayout();
	void setupTextArea() const;
	void update();

private:
	wxStyledTextCtrl* text_log_;
	wxTextCtrl*       text_command_;
	int               cmd_log_index_ = 0;
	wxTimer           timer_update_;
	unsigned          next_message_index_ = 0;

	// Events
	void onCommandEnter(wxCommandEvent& e);
	void onCommandKeyDown(wxKeyEvent& e);
};
