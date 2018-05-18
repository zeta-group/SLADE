#pragma once

class ListView : public wxListCtrl
{
public:
	enum class ItemStatus
	{
		Normal,
		Modified,
		New,
		Locked,
		Error,
		Disabled
	};

	ListView(wxWindow* parent, int id, long style = wxLC_REPORT) :
		wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, style)
	{
	}
	~ListView() = default;

	bool showIcons() const { return icons_; }
	void showIcons(bool show) { icons_ = show; }
	bool enableSizeUpdate() const { return update_width_; }
	void enableSizeUpdate(bool update) { update_width_ = update; }

	bool addItem(int index, string_view text);
	bool addItem(int index, const vector<string>& text);

	bool deleteItems(vector<int> items);

	bool setItemStatus(int index, ItemStatus status);
	bool setItemText(int item, int column, string_view text);

	void clearSelection();
	bool selectItem(int item, bool focus = true);
	bool deSelectItem(int item);

	vector<int> selectedItems() const;

	bool showItem(int item);
	bool swapItems(int item1, int item2);

	bool updateSize();

private:
	bool icons_        = true;
	bool update_width_ = true;
};
