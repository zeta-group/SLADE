#pragma once

#include "BrowserCanvas.h"
#include "BrowserItem.h"
#include "Graphics/Palette/Palette.h"
#include "OpenGL/Drawing.h"
#include "Utility/Tree.h"

class wxChoice;
class wxTextCtrl;
class wxSlider;
class wxStaticText;

class BrowserTreeNode : public STreeNode
{
public:
	BrowserTreeNode(BrowserTreeNode* parent = nullptr) : STreeNode(parent) {}
	~BrowserTreeNode();

	string_view    name() const override { return name_; }
	wxTreeListItem treeId() const { return tree_id_; }
	void           setName(string_view name) override { S_SET_VIEW(name_, name); }
	void           setTreeId(wxTreeListItem id) { this->tree_id_ = id; }

	void         clearItems();
	unsigned     nItems() const { return items_.size(); }
	BrowserItem* getItem(unsigned index);
	void         addItem(BrowserItem* item, unsigned index = 0xFFFFFFFF);

private:
	string               name_;
	vector<BrowserItem*> items_;
	wxTreeListItem       tree_id_;

	STreeNode* createChild(string_view name) override
	{
		auto node = new BrowserTreeNode();
		S_SET_VIEW(node->name_, name);
		return node;
	}
};

class BrowserWindow : public wxDialog
{
public:
	BrowserWindow(wxWindow* parent);
	~BrowserWindow();

	bool truncateNames() const { return truncate_names_; }

	Palette* getPalette() { return &palette_; }
	void     setPalette(Palette* pal) { palette_.copyPalette(pal); }

	bool         addItem(BrowserItem* item, string_view where = "");
	void         addGlobalItem(BrowserItem* item);
	void         clearItems(BrowserTreeNode* node = nullptr) const;
	void         reloadItems(BrowserTreeNode* node = nullptr) const;
	BrowserItem* getSelectedItem() const { return canvas_->selectedItem(); }
	bool         selectItem(string_view name, BrowserTreeNode* node = nullptr);

	unsigned     addSortType(string_view name) const;
	virtual void doSort(unsigned sort_type = 0);
	void         setSortType(int type);

	void openTree(BrowserTreeNode* node, bool clear = true);
	void populateItemTree(bool collapse_all = true);
	void addItemTree(BrowserTreeNode* node, wxTreeListItem& item) const;

	// Canvas display options
	void setFont(Drawing::Font font) const;
	void setItemNameType(BrowserItem::NameType type) const;
	void setItemSize(int size);
	void setItemViewType(BrowserItem::ViewType type) const;

protected:
	BrowserTreeNode*     items_root_   = nullptr;
	wxBoxSizer*          sizer_bottom_ = nullptr;
	Palette              palette_;
	BrowserCanvas*       canvas_ = nullptr;
	vector<BrowserItem*> items_global_;
	bool                 truncate_names_ = false;

private:
	wxTreeListCtrl* tree_items_  = nullptr;
	wxChoice*       choice_sort_ = nullptr;
	wxTextCtrl*     text_filter_ = nullptr;
	wxSlider*       slider_zoom_ = nullptr;
	wxStaticText*   label_info_  = nullptr;

	// Events
	void onTreeItemSelected(wxTreeListEvent& e);
	void onChoiceSortChanged(wxCommandEvent& e);
	void onCanvasDClick(wxMouseEvent& e);
	void onTextFilterChanged(wxCommandEvent& e);
	void onZoomChanged(wxCommandEvent& e);
	void onCanvasSelectionChanged(wxEvent& e);
	void onCanvasKeyChar(wxKeyEvent& e);
};
