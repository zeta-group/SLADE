
#include "Main.h"
#include "SLADEMap/SLADEMap.h"
#include "UndoSteps.h"

using namespace MapEditor;

PropertyChangeUS::PropertyChangeUS(MapObject* object)
{
	backup_ = std::make_unique<MapObject::Backup>();
	object->backup(backup_.get());
}

void PropertyChangeUS::doSwap(MapObject* obj)
{
	auto temp = std::make_unique<MapObject::Backup>();
	obj->backup(temp.get());
	obj->loadFromBackup(backup_.get());
	backup_.swap(temp);
}

bool PropertyChangeUS::doUndo()
{
	MapObject* obj = UndoRedo::currentMap()->getObjectById(backup_->id);
	if (obj)
		doSwap(obj);

	return true;
}

bool PropertyChangeUS::doRedo()
{
	MapObject* obj = UndoRedo::currentMap()->getObjectById(backup_->id);
	if (obj)
		doSwap(obj);

	return true;
}


MapObjectCreateDeleteUS::MapObjectCreateDeleteUS()
{
	SLADEMap* map = UndoRedo::currentMap();
	map->getObjectIdList(MapObject::Type::Vertex, vertices_);
	map->getObjectIdList(MapObject::Type::Line, lines_);
	map->getObjectIdList(MapObject::Type::Side, sides_);
	map->getObjectIdList(MapObject::Type::Sector, sectors_);
	map->getObjectIdList(MapObject::Type::Thing, things_);
}

void MapObjectCreateDeleteUS::swapLists()
{
	// Backup
	vector<unsigned> vertices;
	vector<unsigned> lines;
	vector<unsigned> sides;
	vector<unsigned> sectors;
	vector<unsigned> things;
	SLADEMap*        map = UndoRedo::currentMap();
	if (isValid(this->vertices_))
		map->getObjectIdList(MapObject::Type::Vertex, vertices);
	if (isValid(this->lines_))
		map->getObjectIdList(MapObject::Type::Line, lines);
	if (isValid(this->sides_))
		map->getObjectIdList(MapObject::Type::Side, sides);
	if (isValid(this->sectors_))
		map->getObjectIdList(MapObject::Type::Sector, sectors);
	if (isValid(this->things_))
		map->getObjectIdList(MapObject::Type::Thing, things);

	// Restore
	if (isValid(this->vertices_))
	{
		map->restoreObjectIdList(MapObject::Type::Vertex, this->vertices_);
		this->vertices_ = vertices;
		map->updateGeometryInfo(0);
	}
	if (isValid(this->lines_))
	{
		map->restoreObjectIdList(MapObject::Type::Line, this->lines_);
		this->lines_ = lines;
		map->updateGeometryInfo(0);
	}
	if (isValid(this->sides_))
	{
		map->restoreObjectIdList(MapObject::Type::Side, this->sides_);
		this->sides_ = sides;
	}
	if (isValid(this->sectors_))
	{
		map->restoreObjectIdList(MapObject::Type::Sector, this->sectors_);
		this->sectors_ = sectors;
	}
	if (isValid(this->things_))
	{
		map->restoreObjectIdList(MapObject::Type::Thing, this->things_);
		this->things_ = things;
	}
}

bool MapObjectCreateDeleteUS::doUndo()
{
	swapLists();
	return true;
}

bool MapObjectCreateDeleteUS::doRedo()
{
	swapLists();
	return true;
}

void MapObjectCreateDeleteUS::checkChanges()
{
	SLADEMap* map = UndoRedo::currentMap();

	// Check vertices changed
	bool vertices_changed = false;
	if (map->nVertices() != vertices_.size())
		vertices_changed = true;
	else
		for (unsigned a = 0; a < map->nVertices(); a++)
			if (map->vertex(a)->objId() != vertices_[a])
			{
				vertices_changed = true;
				break;
			}
	if (!vertices_changed)
	{
		// No change, clear
		vertices_.clear();
		vertices_.push_back(0);
		LOG_MESSAGE(3, "MapObjectCreateDeleteUS: No vertices added/deleted");
	}

	// Check lines changed
	bool lines_changed = false;
	if (map->nLines() != lines_.size())
		lines_changed = true;
	else
		for (unsigned a = 0; a < map->nLines(); a++)
			if (map->line(a)->objId() != lines_[a])
			{
				lines_changed = true;
				break;
			}
	if (!lines_changed)
	{
		// No change, clear
		lines_.clear();
		lines_.push_back(0);
		LOG_MESSAGE(3, "MapObjectCreateDeleteUS: No lines added/deleted");
	}

	// Check sides changed
	bool sides_changed = false;
	if (map->nSides() != sides_.size())
		sides_changed = true;
	else
		for (unsigned a = 0; a < map->nSides(); a++)
			if (map->side(a)->objId() != sides_[a])
			{
				sides_changed = true;
				break;
			}
	if (!sides_changed)
	{
		// No change, clear
		sides_.clear();
		sides_.push_back(0);
		LOG_MESSAGE(3, "MapObjectCreateDeleteUS: No sides added/deleted");
	}

	// Check sectors changed
	bool sectors_changed = false;
	if (map->nSectors() != sectors_.size())
		sectors_changed = true;
	else
		for (unsigned a = 0; a < map->nSectors(); a++)
			if (map->sector(a)->objId() != sectors_[a])
			{
				sectors_changed = true;
				break;
			}
	if (!sectors_changed)
	{
		// No change, clear
		sectors_.clear();
		sectors_.push_back(0);
		LOG_MESSAGE(3, "MapObjectCreateDeleteUS: No sectors added/deleted");
	}

	// Check things changed
	bool things_changed = false;
	if (map->nThings() != things_.size())
		things_changed = true;
	else
		for (unsigned a = 0; a < map->nThings(); a++)
			if (map->thing(a)->objId() != things_[a])
			{
				things_changed = true;
				break;
			}
	if (!things_changed)
	{
		// No change, clear
		things_.clear();
		things_.push_back(0);
		LOG_MESSAGE(3, "MapObjectCreateDeleteUS: No things added/deleted");
	}
}

bool MapObjectCreateDeleteUS::isOk()
{
	// Check for any changes at all
	return !(
		vertices_.size() == 1 && vertices_[0] == 0 && lines_.size() == 1 && lines_[0] == 0 && sides_.size() == 1
		&& sides_[0] == 0 && sectors_.size() == 1 && sectors_[0] == 0 && things_.size() == 1 && things_[0] == 0);
}



MultiMapObjectPropertyChangeUS::MultiMapObjectPropertyChangeUS()
{
	// Get backups of recently modified map objects
	vector<MapObject*> objects = UndoRedo::currentMap()->allModifiedObjects(MapObject::propBackupTime());
	for (auto& object : objects)
	{
		auto bak = object->getBackup(true);
		if (bak)
			backups_.emplace_back(bak);
	}

	if (Log::verbosity() >= 2)
	{
		string msg = "Modified ids: ";
		for (auto& backup : backups_)
			msg += S_FMT("%d, ", backup->id);
		Log::info(msg);
	}
}

void MultiMapObjectPropertyChangeUS::doSwap(MapObject* obj, unsigned index)
{
	auto temp = std::make_unique<MapObject::Backup>();
	obj->backup(temp.get());
	obj->loadFromBackup(backups_[index].get());
	backups_[index].swap(temp);
}

bool MultiMapObjectPropertyChangeUS::doUndo()
{
	for (unsigned a = 0; a < backups_.size(); a++)
	{
		MapObject* obj = UndoRedo::currentMap()->getObjectById(backups_[a]->id);
		if (obj)
			doSwap(obj, a);
	}

	return true;
}

bool MultiMapObjectPropertyChangeUS::doRedo()
{
	// LOG_MESSAGE(2, "Restore %lu objects", backups.size());
	for (unsigned a = 0; a < backups_.size(); a++)
	{
		MapObject* obj = UndoRedo::currentMap()->getObjectById(backups_[a]->id);
		if (obj)
			doSwap(obj, a);
	}

	return true;
}
