//========= Director's Cut - https://github.com/SFMDX ============//
//
// Purpose: Director's Cut Service
//
//================================================================//

#include "cbase.h"

#include "directorscut_new/dx_service.h"

#ifdef CLIENT_DLL
#include <vgui_controls/Panel.h>
#include <vgui/isurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include "ienginevgui.h"

#include "directorscut_new/vgui_dxpanel.h"
#include "view.h"
#include "viewrender.h"

#include "filesystem.h"

#ifdef WIN32
#undef MAX_VALUE
#undef INVALID_HANDLE_VALUE
#include <atlstr.h>
#endif

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/// <summary>
/// This is a layer for DMX that provides individual classes in place of factories.
/// Similar to CDmElement (engine tools) where elements can have their own classes.
/// </summary>
template<class T>
CDmxElement* DX_Meta_Create(int flags = 0)
{
	T* proxy = new T(flags);
	CDmxElement* dmx = CreateDmxElement(proxy->GetClass());
	proxy->SetParent(dmx);
	CDmxAttribute* dx = dmx->AddAttribute("dxmeta");
	dx->SetValue<int>((int)proxy);
	proxy->OnInitialize();
	return dmx;
}

// Proxy for adding attributes
template<class T>
CDmxAttribute* DX_Meta_Add_Attribute(CDmxElement* dmx, const char* name, T value)
{
	// Prevent "dxmeta" attribute
	if (strcmp(name, "dxmeta") == 0)
	{
		Warning("DX_Meta_Add_Attribute: Cannot add attribute named \"dxmeta\" on %s\n", dmx->GetName());
		return NULL;
	}
	CDmxAttribute* dx = dmx->AddAttribute(name);
	// Notify proxy
	DXMeta_Proxy* proxy = (DXMeta_Proxy*)dmx->GetValue<int>("dxmeta");
	if (!proxy)
	{
		Warning("DX_Meta_Add_Attribute: Invalid metadata element on %s\n", dmx->GetName());
		return NULL;
	}
	if(proxy->OnAttributeAdded(name, dx->GetType()))
		dx->SetValue<T>(value);
}

// Proxy for removing attributes
void DX_Meta_Remove_Attribute(CDmxElement* dmx, const char* name)
{
	// Prevent "dxmeta" attribute
	if (strcmp(name, "dxmeta") == 0)
	{
		Warning("DX_Meta_Remove_Attribute: Cannot remove attribute named \"dxmeta\" on %s\n", dmx->GetName());
		return;
	}
	// Notify proxy
	DXMeta_Proxy* proxy = (DXMeta_Proxy*)dmx->GetValue<int>("dxmeta");
	if (!proxy)
	{
		Warning("DX_Meta_Remove_Attribute: Invalid metadata element on %s\n", dmx->GetName());
		return;
	}
	if(proxy->OnAttributeRemoved(name))
		dmx->RemoveAttribute(name);
}

// Proxy for renaming attributes
void DX_Meta_Rename_Attribute(CDmxElement* dmx, const char* oldName, const char* newName)
{
	// Prevent "dxmeta" attribute
	if (strcmp(oldName, "dxmeta") == 0 || strcmp(newName, "dxmeta") == 0)
	{
		Warning("DX_Meta_Rename_Attribute: Cannot rename attribute named \"dxmeta\" on %s\n", dmx->GetName());
		return;
	}
	// Notify proxy
	DXMeta_Proxy* proxy = (DXMeta_Proxy*)dmx->GetValue<int>("dxmeta");
	if (!proxy)
	{
		Warning("DX_Meta_Rename_Attribute: Invalid metadata element on %s\n", dmx->GetName());
		return;
	}
	if(proxy->OnAttributeRenamed(oldName, newName))
		dmx->RenameAttribute(oldName, newName);
}

// Proxy for changing attributes
template<class T>
void DX_Meta_Change_Attribute(CDmxElement* dmx, const char* name, T value)
{
	// Prevent "dxmeta" attribute
	if (strcmp(name, "dxmeta") == 0)
	{
		Warning("DX_Meta_Change_Attribute: Cannot change attribute named \"dxmeta\" on %s\n", dmx->GetName());
		return;
	}
	// Notify proxy
	DXMeta_Proxy* proxy = (DXMeta_Proxy*)dmx->GetValue<int>("dxmeta");
	if (!proxy)
	{
		Warning("DX_Meta_Change_Attribute: Invalid metadata element on %s\n", dmx->GetName());
		return;
	}
	if(proxy->OnAttributeChanged(name, value))
		dmx->SetValue<T>(name, value);
}

// proxy for adding an element to an array attribute
template<class T>
void DX_Meta_Array_AddToTail(CDmxElement* dmx, const char* name, T value)
{
	CDmxAttribute* dx = dmx->GetAttribute(name);
	// Notify proxy
	DXMeta_Proxy* proxy = (DXMeta_Proxy*)dmx->GetValue<int>("dxmeta");
	if (!proxy)
	{
		Warning("DX_Meta_Array_AddToTail: Invalid metadata element on %s\n", dmx->GetName());
		return;
	}
	int index = dx->GetArrayForEdit<T>().AddToTail(value);
	proxy->OnAttributeArrayElementAdded(name, index);
}

// proxy for removing an element from an array attribute
template<class T>
void DX_Meta_Array_Remove(CDmxElement* dmx, const char* name, int index)
{
	CDmxAttribute* dx = dmx->GetAttribute(name);
	// Notify proxy
	DXMeta_Proxy* proxy = (DXMeta_Proxy*)dmx->GetValue<int>("dxmeta");
	if (!proxy)
	{
		Warning("DX_Meta_Array_Remove: Invalid metadata element on %s\n", dmx->GetName());
		return;
	}
	dx->GetArrayForEdit<T>().Remove(index);
	proxy->OnAttributeArrayElementRemoved(name, index);
}

// proxy for inserting an element into an array attribute
template<class T>
void DX_Meta_Array_AddToHead(CDmxElement* dmx, const char* name, T value)
{
	CDmxAttribute* dx = dmx->GetAttribute(name);
	// Notify proxy
	DXMeta_Proxy* proxy = (DXMeta_Proxy*)dmx->GetValue<int>("dxmeta");
	if (!proxy)
	{
		Warning("DX_Meta_Array_AddToHead: Invalid metadata element on %s\n", dmx->GetName());
		return;
	}
	int index = dx->GetArrayForEdit<T>().AddToHead(value);
	proxy->OnAttributeArrayElementAdded(name, index);
}

DXMeta_Proxy_Generic::DXMeta_Proxy_Generic(int flags)
{
	this->flags = flags;
	this->classname = "DmElement"; // compatibility with datamodel
}
DXMeta_Proxy_Generic::~DXMeta_Proxy_Generic()
{
	// if necessary, non-generic types should now delete attributes
}
void DXMeta_Proxy_Generic::OnInitialize()
{
	// non-generic types should set initial attributes here
}
void DXMeta_Proxy_Generic::SetParent(CDmxElement *parent)
{
	this->parent = parent;
}
CDmxElement* DXMeta_Proxy_Generic::GetParent()
{
	return parent;
}
const char* DXMeta_Proxy_Generic::GetClass()
{
	return classname;
}
int DXMeta_Proxy_Generic::GetFlags()
{
	return flags;
}
void DXMeta_Proxy_Generic::SetFlags(int flags)
{
	this->flags = flags;
}
void DXMeta_Proxy_Generic::OnUnserialized()
{
	// non-generic types should use this to set attributes using existing data
}
bool DXMeta_Proxy_Generic::OnAttributeAdded(const char* attribute)
{
	// non-generic types should set initial values here
	return true; // return false to cancel addition
}
bool DXMeta_Proxy_Generic::OnAttributeRemoved(const char* attribute)
{
	// non-generic types should reset values here
	return true; // return false to cancel removal
}
bool DXMeta_Proxy_Generic::OnAttributeRenamed(const char* attribute, const char* newName)
{
	// this will break matching internal/dmx attributes if not overridden
	return true; // return false to cancel rename (preferred if trying to rename internal attributes)
}
bool DXMeta_Proxy_Generic::OnAttributeChanged(const char* attribute, void* value)
{
	// non-generic types should update values here
	return true; // return false to cancel change
}
void DXMeta_Proxy_Generic::OnAttributeArrayElementAdded(const char* attribute, int index)
{
	// not implemented
}
void DXMeta_Proxy_Generic::OnAttributeArrayElementRemoved(const char* attribute, int index)
{
	// not implemented
}
void DXMeta_Proxy_Generic::OnAttributeArrayElementChanged(const char* attribute, int index)
{
	// not implemented
}
void DXMeta_Proxy_Generic::OnPreRender()
{
	// if present in the render stack, this will be called before rendering
}

DXMeta_Proxy_DmeTimeFrame::DXMeta_Proxy_DmeTimeFrame(int flags) : DXMeta_Proxy_Generic(flags)
{
	this->classname = "DmeTimeFrame";
	this->attr_start = 0.0f;
	this->attr_duration = 12.8333f;
	this->attr_offset = 0.0f;
	this->attr_scale = 1.0f;
}
void DXMeta_Proxy_DmeTimeFrame::OnInitialize()
{
	BaseClass::OnInitialize();
	DmeTime_t* start = new DmeTime_t();
	start->SetSeconds(attr_start);
	DmeTime_t* duration = new DmeTime_t();
	duration->SetSeconds(attr_duration);
	DmeTime_t* offset = new DmeTime_t();
	offset->SetSeconds(attr_offset);
	this->parent->AddAttribute("start")->SetValue<DmeTime_t>(*start);
	this->parent->AddAttribute("duration")->SetValue<DmeTime_t>(*duration);
	this->parent->AddAttribute("offset")->SetValue<DmeTime_t>(*offset);
	this->parent->AddAttribute("scale")->SetValue<float>(attr_scale);
}
void DXMeta_Proxy_DmeTimeFrame::OnUnserialized()
{
	BaseClass::OnUnserialized();
	this->attr_start = this->parent->GetValue<float>("start");
	this->attr_duration = this->parent->GetValue<float>("duration");
	this->attr_offset = this->parent->GetValue<float>("offset");
	this->attr_scale = this->parent->GetValue<float>("scale");
}
bool DXMeta_Proxy_DmeTimeFrame::OnAttributeChanged(const char* attribute, void* value)
{
	BaseClass::OnAttributeChanged(attribute, value);
	if (strcmp(attribute, "start") == 0)
	{
		this->attr_start = this->parent->GetValue<float>("start");
	}
	else if (strcmp(attribute, "duration") == 0)
	{
		this->attr_duration = this->parent->GetValue<float>("duration");
	}
	else if (strcmp(attribute, "offset") == 0)
	{
		this->attr_offset = this->parent->GetValue<float>("offset");
	}
	else if (strcmp(attribute, "scale") == 0)
	{
		this->attr_scale = this->parent->GetValue<float>("scale");
	}
	return true;
}
bool DXMeta_Proxy_DmeTimeFrame::OnAttributeRenamed(const char* attribute, const char* newName)
{
	BaseClass::OnAttributeRenamed(attribute, newName);
	// list of internal attributes
	const char** internal_attributes = new const char*[4] { "start", "duration", "offset", "scale" };
	for (int i = 0; i < 4; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot rename internal attribute %s\n", attribute);
			return false; // cancel rename
		}
	}
	return true;
}
bool DXMeta_Proxy_DmeTimeFrame::OnAttributeRemoved(const char* attribute)
{
	BaseClass::OnAttributeRemoved(attribute);
	// list of internal attributes
	const char** internal_attributes = new const char*[4] { "start", "duration", "offset", "scale" };
	for (int i = 0; i < 4; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot remove internal attribute %s\n", attribute);
			return false; // cancel removal
		}
	}
	return true;
}

DXMeta_Proxy_DmeClip::DXMeta_Proxy_DmeClip(int flags) : DXMeta_Proxy_Generic(flags)
{
	this->classname = "DmeClip";
	//this->color = Color();
	this->text = "";
	this->mute = false;
	this->trackGroups = CUtlVector<DXMeta_Proxy_DmeTrackGroup*>();
	this->displayScale = 1.0f;
}
void DXMeta_Proxy_DmeClip::OnInitialize()
{
	BaseClass::OnInitialize();
	CDmxElement* timeFrame_element = DX_Meta_Create<DXMeta_Proxy_DmeTimeFrame>();
	timeFrame_element->SetName("unnamed");
	this->timeFrame = (DXMeta_Proxy_DmeTimeFrame*)timeFrame_element->GetValue<int>("dxmeta");
	this->parent->AddAttribute("timeFrame")->SetValue<CDmxElement*>(timeFrame_element);
	this->parent->AddAttribute("color")->SetValue<Color>(this->color);
	this->parent->AddAttribute("text")->SetValue(this->text);
	this->parent->AddAttribute("mute")->SetValue<bool>(this->mute);
	CDmxAttribute* trackGroups_attr = this->parent->AddAttribute("trackGroups");
	trackGroups_attr->GetArrayForEdit<CDmxElement*>();
	this->parent->AddAttribute("displayScale")->SetValue<float>(this->displayScale);
}
void DXMeta_Proxy_DmeClip::OnUnserialized()
{
	BaseClass::OnUnserialized();
	// TODO: Update timeFrame
	this->color = this->parent->GetValue<Color>("color");
	this->text = this->parent->GetValue<CUtlString>("text").Get();
	this->mute = this->parent->GetValue<bool>("mute");
	// TODO: Update trackGroups
	this->displayScale = this->parent->GetValue<float>("displayScale");
}
bool DXMeta_Proxy_DmeClip::OnAttributeChanged(const char* attribute, void* value)
{
	BaseClass::OnAttributeChanged(attribute, value);
	if (strcmp(attribute, "timeFrame") == 0)
	{
		return false; // cancel change
	}
	else if (strcmp(attribute, "color") == 0)
	{
		this->color = this->parent->GetValue<Color>("color");
	}
	else if (strcmp(attribute, "text") == 0)
	{
		this->text = this->parent->GetValue<CUtlString>("text").Get();
	}
	else if (strcmp(attribute, "mute") == 0)
	{
		this->mute = this->parent->GetValue<bool>("mute");
	}
	else if (strcmp(attribute, "trackGroups") == 0)
	{
		return false; // cancel change
	}
	else if (strcmp(attribute, "displayScale") == 0)
	{
		this->displayScale = this->parent->GetValue<float>("displayScale");
	}
	return true;
}
bool DXMeta_Proxy_DmeClip::OnAttributeRenamed(const char* attribute, const char* newName)
{
	BaseClass::OnAttributeRenamed(attribute, newName);
	// list of internal attributes
	const char** internal_attributes = new const char*[6] { "timeFrame", "color", "text", "mute", "trackGroups", "displayScale" };
	for (int i = 0; i < 5; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot rename internal attribute %s\n", attribute);
			return false; // cancel rename
		}
	}
	return true;
}
bool DXMeta_Proxy_DmeClip::OnAttributeRemoved(const char* attribute)
{
	BaseClass::OnAttributeRemoved(attribute);
	// list of internal attributes
	const char** internal_attributes = new const char*[6] { "timeFrame", "color", "text", "mute", "trackGroups", "displayScale" };
	for (int i = 0; i < 5; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot remove internal attribute %s\n", attribute);
			return false; // cancel removal
		}
	}
	return true;
}
void DXMeta_Proxy_DmeClip::OnAttributeArrayElementAdded(const char* attribute, int index)
{
	BaseClass::OnAttributeArrayElementAdded(attribute, index);
	if (strcmp(attribute, "trackGroups") == 0)
	{
		CDmxElement* trackGroup_element = parent->GetAttribute("trackGroups")->GetArray<CDmxElement*>()[index];
		DXMeta_Proxy_DmeTrackGroup* trackGroup = (DXMeta_Proxy_DmeTrackGroup*)trackGroup_element->GetValue<int>("dxmeta");
		this->trackGroups.AddToTail(trackGroup);
	}
}
void DXMeta_Proxy_DmeClip::OnAttributeArrayElementRemoved(const char* attribute, int index) 
{
	BaseClass::OnAttributeArrayElementRemoved(attribute, index);
	if (strcmp(attribute, "trackGroups") == 0)
	{
		CDmxElement* trackGroup_element = parent->GetAttribute("trackGroups")->GetArray<CDmxElement*>()[index];
		DXMeta_Proxy_DmeTrackGroup* trackGroup = (DXMeta_Proxy_DmeTrackGroup*)trackGroup_element->GetValue<int>("dxmeta");
		this->trackGroups.FindAndRemove(trackGroup);
	}
}
void DXMeta_Proxy_DmeClip::OnAttributeArrayElementChanged(const char* attribute, int index)
{
	BaseClass::OnAttributeArrayElementChanged(attribute, index);
	// do nothing
}

DXMeta_Proxy_DmeFilmClip::DXMeta_Proxy_DmeFilmClip(int flags) : DXMeta_Proxy_DmeClip(flags)
{
	/*
	const char* mapname;
	int activeMonitor;
	DXMeta_Proxy_DmeDag* scene;
	const char* aviFile;
	float fadeIn;
	float fadeOut;
	bool useAviFile;
	DXMeta_Proxy_DmeTrackGroup* subClipTrackGroup;
	float volume;
	CUtlVector<CUtlString> concommands;
	CUtlVector<CUtlString> convars;
	*/
	this->classname = "DmeFilmClip";
	//this->materialOverlay
	this->mapname = "";
	//this->camera
	//this->monitorCameras
	this->activeMonitor = 0;
	this->aviFile = "";
	this->fadeIn = 0.0f;
	this->fadeOut = 0.0f;
	//this->inputs
	//this->operators
	this->useAviFile = false;
	//this->animationSets
	//this->bookmarkSets
	//this->activeBookmarkSet
	this->volume = 1.0f;
	this->concommands = CUtlVector<CUtlString>();
	this->convars = CUtlVector<CUtlString>();
}
void DXMeta_Proxy_DmeFilmClip::OnInitialize()
{
	BaseClass::OnInitialize();
	this->parent->AddAttribute("mapname")->SetValue(this->mapname);
	this->parent->AddAttribute("activeMonitor")->SetValue<int>(this->activeMonitor);
	this->parent->AddAttribute("aviFile")->SetValue(this->aviFile);
	this->parent->AddAttribute("fadeIn")->SetValue<float>(this->fadeIn);
	this->parent->AddAttribute("fadeOut")->SetValue<float>(this->fadeOut);
	this->parent->AddAttribute("useAviFile")->SetValue<bool>(this->useAviFile);
	this->parent->AddAttribute("volume")->SetValue<float>(this->volume);
}
void DXMeta_Proxy_DmeFilmClip::OnUnserialized()
{
	BaseClass::OnUnserialized();
	this->mapname = this->parent->GetValue<CUtlString>("mapname").Get();
	this->activeMonitor = this->parent->GetValue<int>("activeMonitor");
	this->aviFile = this->parent->GetValue<CUtlString>("aviFile").Get();
	this->fadeIn = this->parent->GetValue<float>("fadeIn");
	this->fadeOut = this->parent->GetValue<float>("fadeOut");
	this->useAviFile = this->parent->GetValue<bool>("useAviFile");
	this->volume = this->parent->GetValue<float>("volume");
}
bool DXMeta_Proxy_DmeFilmClip::OnAttributeChanged(const char* attribute, void* value)
{
	BaseClass::OnAttributeChanged(attribute, value);
	if (strcmp(attribute, "mapname") == 0)
	{
		this->mapname = this->parent->GetValue<CUtlString>("mapname").Get();
	}
	else if (strcmp(attribute, "activeMonitor") == 0)
	{
		this->activeMonitor = this->parent->GetValue<int>("activeMonitor");
	}
	else if (strcmp(attribute, "aviFile") == 0)
	{
		this->aviFile = this->parent->GetValue<CUtlString>("aviFile").Get();
	}
	else if (strcmp(attribute, "fadeIn") == 0)
	{
		this->fadeIn = this->parent->GetValue<float>("fadeIn");
	}
	else if (strcmp(attribute, "fadeOut") == 0)
	{
		this->fadeOut = this->parent->GetValue<float>("fadeOut");
	}
	else if (strcmp(attribute, "useAviFile") == 0)
	{
		this->useAviFile = this->parent->GetValue<bool>("useAviFile");
	}
	else if (strcmp(attribute, "volume") == 0)
	{
		this->volume = this->parent->GetValue<float>("volume");
	}
	return true;
}
bool DXMeta_Proxy_DmeFilmClip::OnAttributeRenamed(const char* attribute, const char* newName)
{
	BaseClass::OnAttributeRenamed(attribute, newName);
	// list of internal attributes
	const char** internal_attributes = new const char*[9] { "mapname", "activeMonitor", "aviFile", "fadeIn", "fadeOut", "useAviFile", "volume", "concommands", "convars" };
	for (int i = 0; i < 6; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot rename internal attribute %s\n", attribute);
			return false; // cancel rename
		}
	}
	return true;
}
bool DXMeta_Proxy_DmeFilmClip::OnAttributeRemoved(const char* attribute)
{
	BaseClass::OnAttributeRemoved(attribute);
	// list of internal attributes
	const char** internal_attributes = new const char*[9] { "mapname", "activeMonitor", "aviFile", "fadeIn", "fadeOut", "useAviFile", "volume", "concommands", "convars" };
	for (int i = 0; i < 6; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot remove internal attribute %s\n", attribute);
			return false; // cancel removal
		}
	}
	return true;
}

DXMeta_Proxy_DmeTrack::DXMeta_Proxy_DmeTrack(int flags) : DXMeta_Proxy_Generic(flags)
{
	this->classname = "DmeTrack";
	this->children = CUtlVector<DXMeta_Proxy_DmeClip*>();
	this->collapsed = false;
	this->mute = false;
	this->synched = false;
	this->clipType = 0;
	this->volume = 1.0f;
	this->displayScale = 1.0f;
}
void DXMeta_Proxy_DmeTrack::OnInitialize()
{
	BaseClass::OnInitialize();
	CDmxAttribute* children_attr = this->parent->AddAttribute("children");
	children_attr->GetArrayForEdit<CDmxElement*>();
	this->parent->AddAttribute("collapsed")->SetValue<bool>(this->collapsed);
	this->parent->AddAttribute("mute")->SetValue<bool>(this->mute);
	this->parent->AddAttribute("synched")->SetValue<bool>(this->synched);
	this->parent->AddAttribute("clipType")->SetValue<int>(this->clipType);
	this->parent->AddAttribute("volume")->SetValue<float>(this->volume);
	this->parent->AddAttribute("displayScale")->SetValue<float>(this->displayScale);
}
void DXMeta_Proxy_DmeTrack::OnUnserialized()
{
	BaseClass::OnUnserialized();
	// TODO: children
	this->collapsed = this->parent->GetValue<bool>("collapsed");
	this->mute = this->parent->GetValue<bool>("mute");
	this->synched = this->parent->GetValue<bool>("synched");
	this->clipType = this->parent->GetValue<int>("clipType");
	this->volume = this->parent->GetValue<float>("volume");
	this->displayScale = this->parent->GetValue<float>("displayScale");
}
bool DXMeta_Proxy_DmeTrack::OnAttributeChanged(const char* attribute, void* value)
{
	BaseClass::OnAttributeChanged(attribute, value);
	if (strcmp(attribute, "children") == 0)
	{
		return false; // cancel change
	}
	else if (strcmp(attribute, "collapsed") == 0)
	{
		this->collapsed = this->parent->GetValue<bool>("collapsed");
	}
	else if (strcmp(attribute, "mute") == 0)
	{
		this->mute = this->parent->GetValue<bool>("mute");
	}
	else if (strcmp(attribute, "synched") == 0)
	{
		this->synched = this->parent->GetValue<bool>("synched");
	}
	else if (strcmp(attribute, "clipType") == 0)
	{
		this->clipType = this->parent->GetValue<int>("clipType");
	}
	else if (strcmp(attribute, "volume") == 0)
	{
		this->volume = this->parent->GetValue<float>("volume");
	}
	else if (strcmp(attribute, "displayScale") == 0)
	{
		this->displayScale = this->parent->GetValue<float>("displayScale");
	}
	return true;
}
bool DXMeta_Proxy_DmeTrack::OnAttributeRenamed(const char* attribute, const char* newName)
{
	BaseClass::OnAttributeRenamed(attribute, newName);
	// list of internal attributes
	const char** internal_attributes = new const char*[7] { "children", "collapsed", "mute", "synched", "clipType", "volume", "displayScale" };
	for (int i = 0; i < 6; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot rename internal attribute %s\n", attribute);
			return false; // cancel rename
		}
	}
	return true;
}
bool DXMeta_Proxy_DmeTrack::OnAttributeRemoved(const char* attribute)
{
	BaseClass::OnAttributeRemoved(attribute);
	// list of internal attributes
	const char** internal_attributes = new const char*[7] { "children", "collapsed", "mute", "synched", "clipType", "volume", "displayScale" };
	for (int i = 0; i < 6; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot remove internal attribute %s\n", attribute);
			return false; // cancel removal
		}
	}
	return true;
}

DXMeta_Proxy_DmeTrackGroup::DXMeta_Proxy_DmeTrackGroup(int flags)
{
	this->classname = "DmeTrackGroup";
	this->children = CUtlVector<DXMeta_Proxy_DmeTrack*>();
	this->visible = true;
	this->mute = false;
	this->displayScale = 1.0f;
	this->minimized = false;
	this->volume = 1.0f;
	this->forcemultitrack = false;
}
void DXMeta_Proxy_DmeTrackGroup::OnInitialize()
{
	BaseClass::OnInitialize();
	CDmxAttribute* children_attr = this->parent->AddAttribute("children");
	children_attr->GetArrayForEdit<CDmxElement*>();
	this->parent->AddAttribute("visible")->SetValue<bool>(this->visible);
	this->parent->AddAttribute("mute")->SetValue<bool>(this->mute);
	this->parent->AddAttribute("displayScale")->SetValue<float>(this->displayScale);
	this->parent->AddAttribute("minimized")->SetValue<bool>(this->minimized);
	this->parent->AddAttribute("volume")->SetValue<float>(this->volume);
	this->parent->AddAttribute("forcemultitrack")->SetValue<bool>(this->forcemultitrack);
}
void DXMeta_Proxy_DmeTrackGroup::OnUnserialized()
{
	BaseClass::OnUnserialized();
	this->visible = this->parent->GetValue<bool>("visible");
	this->mute = this->parent->GetValue<bool>("mute");
	this->displayScale = this->parent->GetValue<float>("displayScale");
	this->minimized = this->parent->GetValue<bool>("minimized");
	this->volume = this->parent->GetValue<float>("volume");
	this->forcemultitrack = this->parent->GetValue<bool>("forcemultitrack");
}
bool DXMeta_Proxy_DmeTrackGroup::OnAttributeChanged(const char* attribute, void* value)
{
	BaseClass::OnAttributeChanged(attribute, value);
	if (strcmp(attribute, "children") == 0)
	{
		return false; // cancel change
	}
	else if (strcmp(attribute, "visible") == 0)
	{
		this->visible = this->parent->GetValue<bool>("visible");
	}
	else if (strcmp(attribute, "mute") == 0)
	{
		this->mute = this->parent->GetValue<bool>("mute");
	}
	else if (strcmp(attribute, "displayScale") == 0)
	{
		this->displayScale = this->parent->GetValue<float>("displayScale");
	}
	else if (strcmp(attribute, "minimized") == 0)
	{
		this->minimized = this->parent->GetValue<bool>("minimized");
	}
	else if (strcmp(attribute, "volume") == 0)
	{
		this->volume = this->parent->GetValue<float>("volume");
	}
	else if (strcmp(attribute, "forcemultitrack") == 0)
	{
		this->forcemultitrack = this->parent->GetValue<bool>("forcemultitrack");
	}
	return true;
}
bool DXMeta_Proxy_DmeTrackGroup::OnAttributeRenamed(const char* attribute, const char* newName)
{
	BaseClass::OnAttributeRenamed(attribute, newName);
	// list of internal attributes
	const char** internal_attributes = new const char*[7] { "children", "visible", "mute", "displayScale", "minimized", "volume", "forcemultitrack" };
	for (int i = 0; i < 7; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot rename internal attribute %s\n", attribute);
			return false; // cancel rename
		}
	}
	return true;
}
bool DXMeta_Proxy_DmeTrackGroup::OnAttributeRemoved(const char* attribute)
{
	BaseClass::OnAttributeRemoved(attribute);
	// list of internal attributes
	const char** internal_attributes = new const char*[7] { "children", "visible", "mute", "displayScale", "minimized", "volume", "forcemultitrack" };
	for (int i = 0; i < 7; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot remove internal attribute %s\n", attribute);
			return false; // cancel removal
		}
	}
	return true;
}

DXMeta_Proxy_DmeTransform::DXMeta_Proxy_DmeTransform(int flags)
{
	this->classname = "DmeTransform";
	/*
	Vector position;
	Quaternion orientation;
	*/
	this->position = Vector(0, 0, 0);
	this->orientation = Quaternion(0, 0, 0, 0);
}
void DXMeta_Proxy_DmeTransform::OnInitialize()
{
	BaseClass::OnInitialize();
	this->parent->AddAttribute("position")->SetValue<Vector>(this->position);
	this->parent->AddAttribute("orientation")->SetValue<Quaternion>(this->orientation);
}
void DXMeta_Proxy_DmeTransform::OnUnserialized()
{
	BaseClass::OnUnserialized();
	this->position = this->parent->GetValue<Vector>("position");
	this->orientation = this->parent->GetValue<Quaternion>("orientation");
}
bool DXMeta_Proxy_DmeTransform::OnAttributeChanged(const char* attribute, void* value)
{
	BaseClass::OnAttributeChanged(attribute, value);
	if (strcmp(attribute, "position") == 0)
	{
		this->position = this->parent->GetValue<Vector>("position");
	}
	else if (strcmp(attribute, "orientation") == 0)
	{
		this->orientation = this->parent->GetValue<Quaternion>("orientation");
	}
	return true;
}
bool DXMeta_Proxy_DmeTransform::OnAttributeRenamed(const char* attribute, const char* newName)
{
	BaseClass::OnAttributeRenamed(attribute, newName);
	// list of internal attributes
	const char** internal_attributes = new const char*[2] { "position", "orientation" };
	for (int i = 0; i < 2; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot rename internal attribute %s\n", attribute);
			return false; // cancel rename
		}
	}
	return true;
}
bool DXMeta_Proxy_DmeTransform::OnAttributeRemoved(const char* attribute)
{
	BaseClass::OnAttributeRemoved(attribute);
	// list of internal attributes
	const char** internal_attributes = new const char*[2] { "position", "orientation" };
	for (int i = 0; i < 2; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot remove internal attribute %s\n", attribute);
			return false; // cancel removal
		}
	}
	return true;
}

DXMeta_Proxy_DmeShape::DXMeta_Proxy_DmeShape(int flags)
{
	this->classname = "DmeShape";
	/*
	bool visible;
	*/
	this->visible = true;
}
void DXMeta_Proxy_DmeShape::OnInitialize()
{
	BaseClass::OnInitialize();
	this->parent->AddAttribute("visible")->SetValue<bool>(this->visible);
}
void DXMeta_Proxy_DmeShape::OnUnserialized()
{
	BaseClass::OnUnserialized();
	this->visible = this->parent->GetValue<bool>("visible");
}
bool DXMeta_Proxy_DmeShape::OnAttributeChanged(const char* attribute, void* value)
{
	BaseClass::OnAttributeChanged(attribute, value);
	if (strcmp(attribute, "visible") == 0)
	{
		this->visible = this->parent->GetValue<bool>("visible");
	}
	return true;
}
bool DXMeta_Proxy_DmeShape::OnAttributeRenamed(const char* attribute, const char* newName)
{
	BaseClass::OnAttributeRenamed(attribute, newName);
	// list of internal attributes
	const char** internal_attributes = new const char*[1] { "visible" };
	for (int i = 0; i < 1; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot rename internal attribute %s\n", attribute);
			return false; // cancel rename
		}
	}
	return true;
}
bool DXMeta_Proxy_DmeShape::OnAttributeRemoved(const char* attribute)
{
	BaseClass::OnAttributeRemoved(attribute);
	// list of internal attributes
	const char** internal_attributes = new const char*[1] { "visible" };
	for (int i = 0; i < 1; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot remove internal attribute %s\n", attribute);
			return false; // cancel removal
		}
	}
	return true;
}

DXMeta_Proxy_DmeDag::DXMeta_Proxy_DmeDag(int flags)
{
	this->classname = "DmeDag";
	/*
	DXMeta_Proxy_DmeTransform* transform;
	DXMeta_Proxy_DmeShape* shape;
	bool visible;
	CUtlVector<DXMeta_Proxy_DmeDag*> children;
	*/
	this->transform = new DXMeta_Proxy_DmeTransform();
	this->shape = new DXMeta_Proxy_DmeShape();
	this->visible = true;
	this->children = CUtlVector<DXMeta_Proxy_DmeDag*>();
}
void DXMeta_Proxy_DmeDag::OnInitialize()
{
	BaseClass::OnInitialize();
	this->parent->AddAttribute("visible")->SetValue<bool>(this->visible);
	CDmxAttribute* children_attr = this->parent->AddAttribute("children");
	children_attr->GetArrayForEdit<CDmxElement*>();
}
void DXMeta_Proxy_DmeDag::OnUnserialized()
{
	BaseClass::OnUnserialized();
	this->visible = this->parent->GetValue<bool>("visible");
}
bool DXMeta_Proxy_DmeDag::OnAttributeChanged(const char* attribute, void* value)
{
	BaseClass::OnAttributeChanged(attribute, value);
	if (strcmp(attribute, "visible") == 0)
	{
		this->visible = this->parent->GetValue<bool>("visible");
	}
	return true;
}
bool DXMeta_Proxy_DmeDag::OnAttributeRenamed(const char* attribute, const char* newName)
{
	BaseClass::OnAttributeRenamed(attribute, newName);
	// list of internal attributes
	const char** internal_attributes = new const char*[1] { "visible" };
	for (int i = 0; i < 1; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot rename internal attribute %s\n", attribute);
			return false; // cancel rename
		}
	}
	return true;
}
bool DXMeta_Proxy_DmeDag::OnAttributeRemoved(const char* attribute)
{
	BaseClass::OnAttributeRemoved(attribute);
	// list of internal attributes
	const char** internal_attributes = new const char*[1] { "visible" };
	for (int i = 0; i < 1; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot remove internal attribute %s\n", attribute);
			return false; // cancel removal
		}
	}
	return true;
}

DXMeta_Proxy_DmeGlobalFlexControllerOperator::DXMeta_Proxy_DmeGlobalFlexControllerOperator(int flags)
{
	this->classname = "DmeGlobalFlexControllerOperator";
	/*
	float flexWeight;
	DXMeta_Proxy_DmeGameModel* gameModel;
	*/
	this->flexWeight = 0.0f;
}
void DXMeta_Proxy_DmeGlobalFlexControllerOperator::OnInitialize()
{
	BaseClass::OnInitialize();
	this->parent->AddAttribute("flexWeight")->SetValue<float>(this->flexWeight);
}
void DXMeta_Proxy_DmeGlobalFlexControllerOperator::OnUnserialized()
{
	BaseClass::OnUnserialized();
	this->flexWeight = this->parent->GetValue<float>("flexWeight");
}
bool DXMeta_Proxy_DmeGlobalFlexControllerOperator::OnAttributeChanged(const char* attribute, void* value)
{
	BaseClass::OnAttributeChanged(attribute, value);
	if (strcmp(attribute, "flexWeight") == 0)
	{
		this->flexWeight = this->parent->GetValue<float>("flexWeight");
	}
	return true;
}
bool DXMeta_Proxy_DmeGlobalFlexControllerOperator::OnAttributeRenamed(const char* attribute, const char* newName)
{
	BaseClass::OnAttributeRenamed(attribute, newName);
	// list of internal attributes
	const char** internal_attributes = new const char*[1] { "flexWeight" };
	for (int i = 0; i < 1; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot rename internal attribute %s\n", attribute);
			return false; // cancel rename
		}
	}
	return true;
}
bool DXMeta_Proxy_DmeGlobalFlexControllerOperator::OnAttributeRemoved(const char* attribute)
{
	BaseClass::OnAttributeRemoved(attribute);
	// list of internal attributes
	const char** internal_attributes = new const char*[1] { "flexWeight" };
	for (int i = 0; i < 1; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot remove internal attribute %s\n", attribute);
			return false; // cancel removal
		}
	}
	return true;
}

DXMeta_Proxy_DmeGameModel::DXMeta_Proxy_DmeGameModel(int flags)
{
	this->classname = "DmeGameModel";
	/*
	CUtlVector<float> flexWeights;
	char* modelName;
	int skin;
	int body;
	int sequence;
	int attr_flags;
	CUtlVector<DXMeta_Proxy_DmeTransform*> bones;
	CUtlVector<DXMeta_Proxy_DmeGlobalFlexControllerOperator*> globalFlexControllers;
	bool computeBounds;
	bool evaluateProcedualBones;
	*/
	this->flexWeights = CUtlVector<float>();
	this->modelName = "";
	this->skin = 0;
	this->body = 0;
	this->sequence = 0;
	this->attr_flags = 0;
	this->bones = CUtlVector<DXMeta_Proxy_DmeTransform*>();
	this->globalFlexControllers = CUtlVector<DXMeta_Proxy_DmeGlobalFlexControllerOperator*>();
	this->computeBounds = false;
	this->evaluateProcedualBones = false;
}
void DXMeta_Proxy_DmeGameModel::OnInitialize()
{
	BaseClass::OnInitialize();
	this->parent->AddAttribute("modelName")->SetValue<CUtlString>(CUtlString(this->modelName));
	this->parent->AddAttribute("skin")->SetValue<int>(this->skin);
	this->parent->AddAttribute("body")->SetValue<int>(this->body);
	this->parent->AddAttribute("sequence")->SetValue<int>(this->sequence);
	this->parent->AddAttribute("attr_flags")->SetValue<int>(this->attr_flags);
	this->parent->AddAttribute("computeBounds")->SetValue<bool>(this->computeBounds);
	this->parent->AddAttribute("evaluateProcedualBones")->SetValue<bool>(this->evaluateProcedualBones);
}
void DXMeta_Proxy_DmeGameModel::OnUnserialized()
{
	BaseClass::OnUnserialized();
	this->modelName = this->parent->GetValue<CUtlString>("modelName").Get();
	this->skin = this->parent->GetValue<int>("skin");
	this->body = this->parent->GetValue<int>("body");
	this->sequence = this->parent->GetValue<int>("sequence");
	this->attr_flags = this->parent->GetValue<int>("attr_flags");
	this->computeBounds = this->parent->GetValue<bool>("computeBounds");
	this->evaluateProcedualBones = this->parent->GetValue<bool>("evaluateProcedualBones");
}
bool DXMeta_Proxy_DmeGameModel::OnAttributeChanged(const char* attribute, void* value)
{
	BaseClass::OnAttributeChanged(attribute, value);
	if (strcmp(attribute, "modelName") == 0)
	{
		this->modelName = this->parent->GetValue<CUtlString>("modelName").Get();
	}
	else if (strcmp(attribute, "skin") == 0)
	{
		this->skin = this->parent->GetValue<int>("skin");
	}
	else if (strcmp(attribute, "body") == 0)
	{
		this->body = this->parent->GetValue<int>("body");
	}
	else if (strcmp(attribute, "sequence") == 0)
	{
		this->sequence = this->parent->GetValue<int>("sequence");
	}
	else if (strcmp(attribute, "attr_flags") == 0)
	{
		this->attr_flags = this->parent->GetValue<int>("attr_flags");
	}
	else if (strcmp(attribute, "computeBounds") == 0)
	{
		this->computeBounds = this->parent->GetValue<bool>("computeBounds");
	}
	else if (strcmp(attribute, "evaluateProcedualBones") == 0)
	{
		this->evaluateProcedualBones = this->parent->GetValue<bool>("evaluateProcedualBones");
	}
	return true;
}
bool DXMeta_Proxy_DmeGameModel::OnAttributeRenamed(const char* attribute, const char* newName)
{
	BaseClass::OnAttributeRenamed(attribute, newName);
	// list of internal attributes
	const char** internal_attributes = new const char*[7] { "modelName", "skin", "body", "sequence", "attr_flags", "computeBounds", "evaluateProcedualBones" };
	for (int i = 0; i < 7; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot rename internal attribute %s\n", attribute);
			return false; // cancel rename
		}
	}
	return true;
}
bool DXMeta_Proxy_DmeGameModel::OnAttributeRemoved(const char* attribute)
{
	BaseClass::OnAttributeRemoved(attribute);
	// list of internal attributes
	const char** internal_attributes = new const char*[7] { "modelName", "skin", "body", "sequence", "attr_flags", "computeBounds", "evaluateProcedualBones" };
	for (int i = 0; i < 7; i++)
	{
		if (strcmp(attribute, internal_attributes[i]) == 0)
		{
			Warning("Cannot remove internal attribute %s\n", attribute);
			return false; // cancel removal
		}
	}
	return true;
}

bool DXService::Init()
{
	status->SetLabel(L"NO MAP LOADED!");
	status->SetLabelColor(255, 0, 0);
	return true;
}

void DXService::PostInit()
{
#ifdef CLIENT_DLL
	DECLARE_DMX_CONTEXT();
#endif
	Msg("Initialized DX Service on ");
#ifdef CLIENT_DLL
	Msg("client.\n");
	vgui::VPANEL toolParent = enginevgui->GetPanel(PANEL_ROOT);
	dxgui->Create(toolParent);

	status->SetLabel(L"Camera View");
	status->SetLabelColor(255, 255, 255);
	
	// Delete hl2_singleton mutex
#ifdef WIN32
	if(CommandLine()->FindParm("-multirun") || CommandLine()->FindParm("-allowmultiple"))
	{
		HANDLE handle = OpenMutex(MUTEX_ALL_ACCESS, false, "hl2_singleton_mutex");
		if (handle != NULL && ReleaseMutex(handle))
		{
			Msg("Released hl2_singleton_mutex\n");
		}
		else
		{
			Msg("Failed to release hl2_singleton_mutex\n");
		}
	}
#endif
	root = CreateDmxElement("DmElement");
	root->SetName("session");

	CDmxAttribute* activeClip = root->AddAttribute("activeClip");

	CDmxElement* clip = DX_Meta_Create<DXMeta_Proxy_DmeFilmClip>();
	clip->SetName("Test");
	activeClip->SetValue<CDmxElement*>(clip);

	CDmxAttribute* miscBin = root->AddAttribute("miscBin");
	CDmxAttribute* cameraBin = root->AddAttribute("cameraBin");
	CDmxAttribute* clipBin = root->AddAttribute("clipBin");
	cameraBin->GetArrayForEdit<CDmxElement*>();
	miscBin->GetArrayForEdit<CDmxElement*>();
	clipBin->GetArrayForEdit<CDmxElement*>().AddToTail(clip);
	
	CDmxAttribute* settings = root->AddAttribute("settings");
	CDmxElement* sessionSettings = CreateDmxElement("DmeSettings");
	sessionSettings->SetName("sessionSettings");
	settings->SetValue<CDmxElement*>(sessionSettings);
	
	CDmxElement* trackGroup = DX_Meta_Create<DXMeta_Proxy_DmeTrackGroup>();
	trackGroup->SetName("Sound");
	DX_Meta_Array_AddToTail(clip, "trackGroups", trackGroup);

	CDmxElement* trackGroup2 = DX_Meta_Create<DXMeta_Proxy_DmeTrackGroup>();
	trackGroup2->SetName("Overlay");
	DX_Meta_Array_AddToTail(clip, "trackGroups", trackGroup2);

	CDmxElement* subClipTrackGroup = DX_Meta_Create<DXMeta_Proxy_DmeTrackGroup>();
	subClipTrackGroup->SetName("subClipTrackGroup");
	DX_Meta_Change_Attribute(clip, "subClipTrackGroup", subClipTrackGroup);
	
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	SerializeDMX(buf, root);

	FileHandle_t fh = filesystem->Open("test.dmx","w", "MOD");
	filesystem->Write(buf.Base(),buf.TellPut(),fh);
	filesystem->Close(fh);
	Msg("Wrote test.dmx\n");
#else
	Msg("server.\n");
#endif
}

void DXService::Shutdown()
{
#ifdef CLIENT_DLL
	dxgui->Destroy();
#endif
}

void DXService::Update(float frametime)
{
#ifdef CLIENT_DLL
#endif
}

void DXService::LevelInitPostEntity()
{
}

#ifdef CLIENT_DLL
void DXService::PreRender()
{
}

void DXService::SetupEngineView(Vector& origin, QAngle& angles, float& fov)
{
	if (!active)
		return;
	origin = this->origin;
	angles = this->angles;
	fov = this->fov;
}

DXStatus* DXService::GetStatus()
{
	return status;
}
void DXService::RenderStackAddToHead(DXMeta_Proxy* proxy)
{
	renderStack.AddToHead(proxy);
}
void DXService::RenderStackAddToTail(DXMeta_Proxy* proxy)
{
	renderStack.AddToTail(proxy);
}
void DXService::RenderStackRemove(DXMeta_Proxy* proxy)
{
	renderStack.FindAndRemove(proxy);
}
void DXService::RenderStackClear()
{
	renderStack.RemoveAll();
}
#endif



DXService g_DXService("DXService");
