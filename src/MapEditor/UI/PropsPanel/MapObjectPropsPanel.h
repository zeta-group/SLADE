#pragma once

#include "MOPGProperty.h"
#include "PropsPanelBase.h"
#include "UI/Controls/STabCtrl.h"
#include "MapEditor/SLADEMap/MapObject.h"

class wxPropertyGrid;
class wxPGProperty;
class UDMFProperty;
class MOPGProperty;

class MapObjectPropsPanel : public PropsPanelBase
{
public:
	MapObjectPropsPanel(wxWindow* parent, bool no_apply = false);
	~MapObjectPropsPanel() = default;

	vector<MapObject*>& getObjects() { return objects_; }
	bool                showAll() const;

	void openObject(MapObject* object);
	void openObjects(vector<MapObject*>& objects) override;
	void updateArgs(MOPGIntWithArgsProperty* source);
	void applyChanges() override;
	void clearGrid();
	void hideFlags(bool hide) { hide_flags_ = hide; }
	void hideTriggers(bool hide) { hide_triggers_ = hide; }
	void hideProperty(const string& property) { hide_props_.push_back(property); }
	void clearHiddenProperties() { hide_props_.clear(); }
	bool propHidden(const string& property) { return VECTOR_EXISTS(hide_props_, property); }

private:
	TabControl*           stc_sections_   = nullptr;
	wxPropertyGrid*       pg_properties_  = nullptr;
	wxPropertyGrid*       pg_props_side1_ = nullptr;
	wxPropertyGrid*       pg_props_side2_ = nullptr;
	MapObject::Type       last_type_      = MapObject::Type::Object;
	string                last_config_;
	vector<MOPGProperty*> properties_;
	wxPGProperty*         args_[5]      = { nullptr, nullptr, nullptr, nullptr, nullptr };
	wxButton*             btn_reset_    = nullptr;
	wxButton*             btn_apply_    = nullptr;
	wxCheckBox*           cb_show_all_  = nullptr;
	wxButton*             btn_add_      = nullptr;
	wxPGProperty*         group_custom_ = nullptr;
	bool                  no_apply_     = false;
	bool                  udmf_         = false;

	// Hide properties
	bool           hide_flags_    = false;
	bool           hide_triggers_ = false;
	vector<string> hide_props_;

	MOPGProperty* addBoolProperty(
		wxPGProperty*   group,
		const wxString& label,
		const wxString& propname,
		bool            readonly  = false,
		wxPropertyGrid* grid      = nullptr,
		UDMFProperty*   udmf_prop = nullptr);
	MOPGProperty* addIntProperty(
		wxPGProperty*   group,
		const wxString& label,
		const wxString& propname,
		bool            readonly  = false,
		wxPropertyGrid* grid      = nullptr,
		UDMFProperty*   udmf_prop = nullptr);
	MOPGProperty* addFloatProperty(
		wxPGProperty*   group,
		const wxString& label,
		const wxString& propname,
		bool            readonly  = false,
		wxPropertyGrid* grid      = nullptr,
		UDMFProperty*   udmf_prop = nullptr);
	MOPGProperty* addStringProperty(
		wxPGProperty*   group,
		const wxString& label,
		const wxString& propname,
		bool            readonly  = false,
		wxPropertyGrid* grid      = nullptr,
		UDMFProperty*   udmf_prop = nullptr);
	MOPGProperty* addLineFlagProperty(
		wxPGProperty*   group,
		const wxString& label,
		const wxString& propname,
		int             index,
		bool            readonly  = false,
		wxPropertyGrid* grid      = nullptr,
		UDMFProperty*   udmf_prop = nullptr);
	MOPGProperty* addThingFlagProperty(
		wxPGProperty*   group,
		const wxString& label,
		const wxString& propname,
		int             index,
		bool            readonly  = false,
		wxPropertyGrid* grid      = nullptr,
		UDMFProperty*   udmf_prop = nullptr);
	MOPGProperty* addTextureProperty(
		wxPGProperty*   group,
		const wxString& label,
		const wxString& propname,
		int             textype,
		bool            readonly  = false,
		wxPropertyGrid* grid      = nullptr,
		UDMFProperty*   udmf_prop = nullptr);
	void addUDMFProperty(
		UDMFProperty&   prop,
		MapObject::Type objtype,
		string_view     basegroup = "",
		wxPropertyGrid* grid      = nullptr);

	void setupType(MapObject::Type objtype);
	void setupTypeUDMF(MapObject::Type objtype);

	// Events
	void onBtnApply(wxCommandEvent& e);
	void onBtnReset(wxCommandEvent& e);
	void onShowAllToggled(wxCommandEvent& e);
	void onBtnAdd(wxCommandEvent& e);
	void onPropertyChanged(wxPropertyGridEvent& e);
};
