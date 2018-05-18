#pragma once

class SAction;
class SToolBarButton : public wxControl
{
public:
	SToolBarButton(wxWindow* parent, string_view action, string_view icon = "", bool show_name = false);
	SToolBarButton(
		wxWindow*   parent,
		string_view action_id,
		string_view action_name,
		string_view icon,
		string_view help_text,
		bool        show_name = false);
	~SToolBarButton() = default;

	void setIcon(string_view icon);

	enum
	{
		STATE_NORMAL,
		STATE_MOUSEOVER,
		STATE_MOUSEDOWN,
	};

	static int pixelHeight();

private:
	SAction* action_ = nullptr;
	wxBitmap icon_;
	int      state_     = STATE_NORMAL;
	bool     show_name_ = false;

	// For non-SAction buttons
	string action_id_;
	string action_name_;
	string help_text_;

	// Layout
	int pad_outer_  = 3;
	int pad_inner_  = 1;
	int icon_size_  = 16;
	int text_width_ = 0;

	void sendClickedEvent();

	// Events
	void onPaint(wxPaintEvent& e);
	void onMouseEvent(wxMouseEvent& e);
	void onFocus(wxFocusEvent& e);
};

wxDECLARE_EVENT(wxEVT_STOOLBAR_BUTTON_CLICKED, wxCommandEvent);
