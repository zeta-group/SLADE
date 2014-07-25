
#ifndef __DATA_ENTRY_PANEL_H__
#define __DATA_ENTRY_PANEL_H__

#include "EntryPanel.h"
#include <wx/grid.h>

struct int_string_t
{
	int		key;
	string	value;
	int_string_t(int key, string value)
	{
		this->key = key;
		this->value = value;
	}
};

struct dep_column_t
{
	string					name;
	uint8_t					type;
	uint16_t				size;
	uint32_t				row_offset;
	vector<int_string_t>	custom_values;
	dep_column_t(string name, uint8_t type, uint16_t size, uint32_t row_offset)
	{
		this->name = name;
		this->type = type;
		this->size = size;
		this->row_offset = row_offset;
	}

	void addCustomValue(int key, string value)
	{
		custom_values.push_back(int_string_t(key, value));
	}

	string getCustomValue(int key)
	{
		for (unsigned a = 0; a < custom_values.size(); a++)
		{
			if (key == custom_values[a].key)
				return custom_values[a].value;
		}
		return S_FMT("Unknown: %d", key);
	}
};

class DataEntryPanel;
class DataEntryTable : public wxGridTableBase
{
private:
	MemChunk				data;
	vector<dep_column_t>	columns;
	unsigned				row_stride;
	unsigned				data_start;
	DataEntryPanel*			parent;

public:
	DataEntryTable(DataEntryPanel* parent);
	virtual ~DataEntryTable();

	// Column types
	enum
	{
		COL_INT_SIGNED,
		COL_INT_UNSIGNED,
		COL_FIXED,
		COL_STRING,
		COL_BOOLEAN,
		COL_FLOAT,
		COL_CUSTOM_VALUE
	};

	// wxGridTableBase overrides
	int		GetNumberRows();
	int		GetNumberCols();
	string	GetValue(int row, int col);
	void	SetValue(int row, int col, const string& value);
	string	GetColLabelValue(int col);

	MemChunk&	getData() { return data; }
	bool		setupDataStructure(ArchiveEntry* entry);
};

class DataEntryPanel : public EntryPanel
{
private:
	wxGrid*			grid_data;
	DataEntryTable*	table_data;

public:
	DataEntryPanel(wxWindow* parent);
	~DataEntryPanel();

	bool	loadEntry(ArchiveEntry* entry);
	bool	saveEntry();
	void	setModified(bool modified) { EntryPanel::setModified(modified); }
};

#endif//__DATA_ENTRY_PANEL_H__