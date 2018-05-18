#pragma once

#include "UI/Browser/BrowserWindow.h"

class SLADEMap;
class Archive;

class MapTexBrowserItem : public BrowserItem
{
public:
	MapTexBrowserItem(string_view name, int type, unsigned index = 0);
	~MapTexBrowserItem() = default;

	bool	loadImage() override;
	string	itemInfo() override;
	int		usageCount() const { return usage_count_; }
	void	setUsage(int count) { usage_count_ = count; }

private:
	int	usage_count_ = 0;
};

class MapTextureBrowser : public BrowserWindow
{
public:
	MapTextureBrowser(wxWindow* parent, int type = 0, string_view texture = "", SLADEMap* map = nullptr);
	~MapTextureBrowser() = default;

	void	doSort(unsigned sort_type) override;
	void	updateUsage() const;

private:
	int			type_;
	SLADEMap*	map_;
};
