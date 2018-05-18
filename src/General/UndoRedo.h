#pragma once

#include "General/ListenerAnnouncer.h"
#include "common.h"

class UndoStep
{
public:
	UndoStep() {}
	virtual ~UndoStep() {}

	virtual bool doUndo() { return true; }
	virtual bool doRedo() { return true; }
	virtual bool writeFile(MemChunk& mc) { return true; }
	virtual bool readFile(MemChunk& mc) { return true; }
	virtual bool isOk() { return true; }
};

class UndoLevel
{
public:
	UndoLevel(string_view name);
	~UndoLevel();

	string name() const { return name_; }
	bool   doUndo();
	bool   doRedo();
	void   addStep(UndoStep* step) { undo_steps_.push_back(step); }
	string getTimeStamp(bool date, bool time) const;

	bool writeFile(string_view filename);
	bool readFile(string_view filename);
	void createMerged(vector<UndoLevel*>& levels);

private:
	string            name_;
	vector<UndoStep*> undo_steps_;
	wxDateTime        timestamp_;
};

class SLADEMap;
class UndoManager : public Announcer
{
public:
	UndoManager(SLADEMap* map = nullptr);
	~UndoManager();

	SLADEMap*  map() const { return map_; }
	void       getAllLevels(vector<string>& list);
	int        currentIndex() const { return current_level_index_; }
	unsigned   nUndoLevels() const { return undo_levels_.size(); }
	UndoLevel* undoLevel(unsigned index) { return undo_levels_[index]; }

	void   beginRecord(string_view name);
	void   endRecord(bool success);
	bool   currentlyRecording() const;
	bool   recordUndoStep(UndoStep* step) const;
	string undo();
	string redo();

	void clear();
	bool createMergedLevel(UndoManager* manager, string_view name);

	typedef std::unique_ptr<UndoManager> UPtr;

private:
	vector<UndoLevel*> undo_levels_;
	UndoLevel*         current_level_;
	int                current_level_index_;
	bool               undo_running_;
	SLADEMap*          map_;
};

namespace UndoRedo
{
bool         currentlyRecording();
UndoManager* currentManager();
SLADEMap*    currentMap();
} // namespace UndoRedo
