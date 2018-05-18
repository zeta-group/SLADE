#pragma once

#include "MapEditor/MapEditor.h"

class MapEditContext;

class MoveObjects
{
public:
	MoveObjects(MapEditContext& context) : context_{ context } {}

	const vector<MapEditor::Item>& items() const { return items_; }
	fpoint2_t                      offset() const { return offset_; }

	bool begin(const fpoint2_t& mouse_pos);
	void update(const fpoint2_t& mouse_pos);
	void end(bool accept = true);

private:
	MapEditContext&         context_;
	fpoint2_t               origin_;
	fpoint2_t               offset_;
	vector<MapEditor::Item> items_;
	MapEditor::Item         item_closest_ = 0;
};
