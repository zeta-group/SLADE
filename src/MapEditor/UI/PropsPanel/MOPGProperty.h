#pragma once

#include "common.h"

#include "Game/Args.h"

class MapObject;
class MapObjectPropsPanel;
class UDMFProperty;
class MOPGProperty
{
public:
	MOPGProperty()          = default;
	virtual ~MOPGProperty() = default;

	enum class Type
	{
		Bool = 0,
		Int,
		Float,
		String,
		ActionSpecial,
		SectorSpecial,
		ThingType,
		LineFlag,
		ThingFlag,
		Angle,
		Colour,
		Texture,
		SpecialActivation,
		Id,
	};

	const string& propName() const { return prop_name_; }
	void          setParent(MapObjectPropsPanel* parent) { this->parent_ = parent; }
	virtual void  setUDMFProp(UDMFProperty* prop) { this->udmf_prop_ = prop; }

	virtual Type type()                                   = 0;
	virtual void openObjects(vector<MapObject*>& objects) = 0;
	virtual void updateVisibility()                       = 0;
	virtual void applyValue() {}
	virtual void resetValue();

protected:
	MapObjectPropsPanel* parent_    = nullptr;
	bool                 no_update_ = false;
	UDMFProperty*        udmf_prop_ = nullptr;
	string               prop_name_;
};

class MOPGBoolProperty : public MOPGProperty, public wxBoolProperty
{
public:
	MOPGBoolProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::Bool; }
	void openObjects(vector<MapObject*>& objects) override;
	void updateVisibility() override;
	void applyValue() override;
};

class MOPGIntProperty : public MOPGProperty, public wxIntProperty
{
public:
	MOPGIntProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::Int; }
	void openObjects(vector<MapObject*>& objects) override;
	void updateVisibility() override;
	void applyValue() override;
};

class MOPGFloatProperty : public MOPGProperty, public wxFloatProperty
{
public:
	MOPGFloatProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::Float; }
	void openObjects(vector<MapObject*>& objects) override;
	void updateVisibility() override;
	void applyValue() override;
};

class MOPGStringProperty : public MOPGProperty, public wxStringProperty
{
public:
	MOPGStringProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	void setUDMFProp(UDMFProperty* prop) override;

	Type type() override { return Type::String; }
	void openObjects(vector<MapObject*>& objects) override;
	void updateVisibility() override;
	void applyValue() override;
};

class MOPGIntWithArgsProperty : public MOPGIntProperty
{
public:
	MOPGIntWithArgsProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	void applyValue() override;
	bool hasArgs();
	void updateArgs(wxPGProperty* args[5]);

	// wxPGProperty overrides
	void OnSetValue() override;

protected:
	virtual const Game::ArgSpec& argSpec() = 0;
};

class MOPGActionSpecialProperty : public MOPGIntWithArgsProperty
{
public:
	MOPGActionSpecialProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL) :
		MOPGIntWithArgsProperty(label, name)
	{
	}

	Type type() override { return Type::ActionSpecial; }

	// wxPGProperty overrides
	wxString ValueToString(wxVariant& value, int argFlags = 0) const override;
	bool     OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e) override;

protected:
	const Game::ArgSpec& argSpec() override;
};

class MOPGThingTypeProperty : public MOPGIntWithArgsProperty
{
public:
	MOPGThingTypeProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL) :
		MOPGIntWithArgsProperty(label, name)
	{
	}

	Type type() override { return Type::ThingType; }

	// wxPGProperty overrides
	wxString ValueToString(wxVariant& value, int argFlags = 0) const override;
	bool     OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e) override;

protected:
	const Game::ArgSpec& argSpec() override;
};

class MOPGLineFlagProperty : public MOPGBoolProperty
{
public:
	MOPGLineFlagProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL, int index = -1);

	Type type() override { return Type::LineFlag; }
	void openObjects(vector<MapObject*>& objects) override;
	void applyValue() override;

private:
	int index_;
};

class MOPGThingFlagProperty : public MOPGBoolProperty
{
public:
	MOPGThingFlagProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL, int index = -1);

	Type type() override { return Type::ThingFlag; }
	void openObjects(vector<MapObject*>& objects) override;
	void applyValue() override;

private:
	int index_;
};

class MOPGAngleProperty : public MOPGProperty, public wxEditEnumProperty
{
public:
	MOPGAngleProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::Angle; }
	void openObjects(vector<MapObject*>& objects) override;
	void updateVisibility() override;
	void applyValue() override;

	// wxPGProperty overrides
	wxString ValueToString(wxVariant& value, int argFlags = 0) const override;
};

class MOPGColourProperty : public MOPGProperty, public wxColourProperty
{
public:
	MOPGColourProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::Colour; }
	void openObjects(vector<MapObject*>& objects) override;
	void updateVisibility() override;
	void applyValue() override;
};

class MOPGTextureProperty : public MOPGStringProperty
{
public:
	MOPGTextureProperty(int textype = 0, const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::Texture; }
	void openObjects(vector<MapObject*>& objects) override;

	// wxPGProperty overrides
	bool OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e) override;

private:
	int tex_type_;
};

class MOPGSPACTriggerProperty : public MOPGProperty, public wxEnumProperty
{
public:
	MOPGSPACTriggerProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::SpecialActivation; }
	void openObjects(vector<MapObject*>& objects) override;
	void updateVisibility() override;
	void applyValue() override;
};

class MOPGTagProperty : public MOPGIntProperty
{
public:
	enum class TagType
	{
		Sector,
		Line,
		Thing
	};

	MOPGTagProperty(TagType tagtype, const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::Id; }
	void openObjects(vector<MapObject*>& objects) override;

	// wxPGProperty overrides
	bool OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e) override;

private:
	TagType tag_type_;
};

class MOPGSectorSpecialProperty : public MOPGIntProperty
{
public:
	MOPGSectorSpecialProperty(const wxString& label = wxPG_LABEL, const wxString& name = wxPG_LABEL);

	Type type() override { return Type::SectorSpecial; }
	void openObjects(vector<MapObject*>& objects) override;

	// wxPGProperty overrides
	wxString ValueToString(wxVariant& value, int argFlags = 0) const override;
	bool     OnEvent(wxPropertyGrid* propgrid, wxWindow* window, wxEvent& e) override;
};
