
#include "Main.h"
#include "MainEditor.h"
#include "Archive/ArchiveManager.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "UI/ArchiveManagerPanel.h"
#include "UI/Controls/PaletteChooser.h"
#include "UI/MainWindow.h"

namespace MainEditor
{
MainWindow* main_window = nullptr;
}

bool MainEditor::init()
{
	main_window = new MainWindow();
	return true;
}

MainWindow* MainEditor::window()
{
	return main_window;
}

wxWindow* MainEditor::windowWx()
{
	return main_window;
}

// -----------------------------------------------------------------------------
// Returns the currently open archive (ie the current tab's archive, if any)
// -----------------------------------------------------------------------------
Archive* MainEditor::currentArchive()
{
	return main_window->archiveManagerPanel()->currentArchive();
}

// -----------------------------------------------------------------------------
// Returns the currently open entry (current tab -> current entry panel)
// -----------------------------------------------------------------------------
ArchiveEntry* MainEditor::currentEntry()
{
	return main_window->archiveManagerPanel()->currentEntry();
}

// -----------------------------------------------------------------------------
// Returns a list of all currently selected entries in the current archive panel
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> MainEditor::currentEntrySelection()
{
	return main_window->archiveManagerPanel()->currentEntrySelection();
}

// -----------------------------------------------------------------------------
// Opens the texture editor for the current archive tab
// -----------------------------------------------------------------------------
void MainEditor::openTextureEditor(Archive* archive, ArchiveEntry* entry)
{
	main_window->archiveManagerPanel()->openTextureTab(App::archiveManager().archiveIndex(archive), entry);
}

// -----------------------------------------------------------------------------
// Opens the map editor for the current archive tab
// -----------------------------------------------------------------------------
void MainEditor::openMapEditor(Archive* archive)
{
	MapEditor::chooseMap(archive);
}

void ::MainEditor::openArchiveTab(Archive* archive)
{
	main_window->archiveManagerPanel()->openTab(archive);
}

// -----------------------------------------------------------------------------
// Opens [entry] in its own tab
// -----------------------------------------------------------------------------
void MainEditor::openEntry(ArchiveEntry* entry)
{
	main_window->archiveManagerPanel()->openEntryTab(entry);
}

void MainEditor::setGlobalPaletteFromArchive(Archive* archive)
{
	main_window->paletteChooser()->setGlobalFromArchive(archive);
}

Palette* MainEditor::currentPalette(ArchiveEntry* entry)
{
	return main_window->paletteChooser()->selectedPalette(entry);
}

EntryPanel* MainEditor::currentEntryPanel()
{
	return main_window->archiveManagerPanel()->currentArea();
}

#ifdef USE_WEBVIEW_STARTPAGE
void MainEditor::openDocs(string_view page_name)
{
	main_window->openDocs(page_name);
}
#endif
