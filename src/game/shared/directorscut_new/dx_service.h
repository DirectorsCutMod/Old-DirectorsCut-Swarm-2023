//========= Director's Cut - https://github.com/SFMDX ============//
//
// Purpose: Director's Cut Service
//
//================================================================//

#ifndef _DX_SERVICE_H_
#define _DX_SERVICE_H_

#include "igamesystem.h"
#include "uniqueid.h"

#include "dmxloader/dmxloader.h"
#include "dmxloader/dmxelement.h"

#ifdef CLIENT_DLL

/// <summary>
/// DXStatus bundles up client status information.
/// </summary>
class DXStatus
{
public:
	void SetLabel(const wchar_t* label)
	{
		viewport_label = label;
	}
	void SetLabelColor(Color color)
	{
		viewport_label_color = color;
	}
	void SetLabelColor(int r, int g, int b)
	{
		viewport_label_color = Color(r, g, b, 255);
	}
	void SetActive(bool active)
	{
		this->active = active;
	}
	const wchar_t* GetLabel()
	{
		return viewport_label;
	}
	Color GetLabelColor()
	{
		return viewport_label_color;
	}
	bool IsActive()
	{
		return active;
	}
private:
	const wchar_t* viewport_label = L"";
	Color viewport_label_color = Color(255, 255, 255, 255);
	bool active = false;
};
#endif

abstract_class DXMeta_Proxy
{
public:
	virtual void SetParent(CDmxElement *parent) = 0;
	virtual CDmxElement* GetParent() = 0;
	virtual const char* GetClass() = 0;
	virtual int GetFlags() = 0;
	virtual void SetFlags(int flags) = 0;
	virtual void OnInitialize() = 0;
	virtual void OnUnserialized() = 0;
	virtual bool OnAttributeAdded(const char* attribute) = 0;
	virtual bool OnAttributeRemoved(const char* attribute) = 0;
	virtual bool OnAttributeRenamed(const char* attribute, const char* newName) = 0;
	// below does not fire if attribute is an array
	virtual bool OnAttributeChanged(const char* attribute, void* value) = 0;
	// array functions
	virtual void OnAttributeArrayElementAdded(const char* attribute, int index) = 0;
	virtual void OnAttributeArrayElementRemoved(const char* attribute, int index) = 0;
	virtual void OnAttributeArrayElementChanged(const char* attribute, int index) = 0;
	virtual void OnPreRender() = 0;
};

class DXMeta_Proxy_Generic : public DXMeta_Proxy
{
public:
	DXMeta_Proxy_Generic(int flags = 0);
	~DXMeta_Proxy_Generic();
	virtual void OnInitialize();
	virtual void SetParent(CDmxElement *parent);
	virtual CDmxElement* GetParent();
	virtual const char* GetClass();
	virtual int GetFlags();
	virtual void SetFlags(int flags);
	virtual void OnUnserialized();
	virtual bool OnAttributeAdded(const char* attribute);
	virtual bool OnAttributeRemoved(const char* attribute);
	virtual bool OnAttributeRenamed(const char* attribute, const char* newName);
	virtual bool OnAttributeChanged(const char* attribute, void* value);
	virtual void OnAttributeArrayElementAdded(const char* attribute, int index);
	virtual void OnAttributeArrayElementRemoved(const char* attribute, int index);
	virtual void OnAttributeArrayElementChanged(const char* attribute, int index);
	virtual void OnPreRender();
protected:
	CDmxElement *parent;
	int flags;
	const char* classname;
	// If this wasn't a generic proxy, a list of internal attributes would be here.
};

class DXMeta_Proxy_DmeTimeFrame : public DXMeta_Proxy_Generic 
{
	DECLARE_CLASS(DXMeta_Proxy_DmeTimeFrame, DXMeta_Proxy_Generic);
public:
	DXMeta_Proxy_DmeTimeFrame(int flags = 0);
	virtual void OnInitialize();
	virtual void OnUnserialized();
	virtual bool OnAttributeChanged(const char* attribute, void* value);
	virtual bool OnAttributeRenamed(const char* attribute, const char* newName);
	virtual bool OnAttributeRemoved(const char* attribute);
protected:
	float attr_start;
	float attr_duration;
	float attr_offset;
	float attr_scale;
};

class DXMeta_Proxy_DmeTrackGroup;

class DXMeta_Proxy_DmeClip : public DXMeta_Proxy_Generic 
{
	DECLARE_CLASS(DXMeta_Proxy_DmeClip, DXMeta_Proxy_Generic);
public:
	DXMeta_Proxy_DmeClip(int flags = 0);
	virtual void OnInitialize();
	virtual void OnUnserialized();
	virtual bool OnAttributeChanged(const char* attribute, void* value);
	virtual bool OnAttributeRenamed(const char* attribute, const char* newName);
	virtual bool OnAttributeRemoved(const char* attribute);
	virtual void OnAttributeArrayElementAdded(const char* attribute, int index);
	virtual void OnAttributeArrayElementRemoved(const char* attribute, int index);
	virtual void OnAttributeArrayElementChanged(const char* attribute, int index);
protected:
	DXMeta_Proxy_DmeTimeFrame* timeFrame;
	Color color;
	const char* text;
	bool mute;
	CUtlVector<DXMeta_Proxy_DmeTrackGroup*> trackGroups;
	float displayScale;
};

class DXMeta_Proxy_DmeDag;

class DXMeta_Proxy_DmeFilmClip : public DXMeta_Proxy_DmeClip
{
	DECLARE_CLASS(DXMeta_Proxy_DmeFilmClip, DXMeta_Proxy_DmeClip);
public:
	DXMeta_Proxy_DmeFilmClip(int flags = 0);
	virtual void OnInitialize();
	virtual void OnUnserialized();
	virtual bool OnAttributeChanged(const char* attribute, void* value);
	virtual bool OnAttributeRenamed(const char* attribute, const char* newName);
	virtual bool OnAttributeRemoved(const char* attribute);
protected:
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
};

class DXMeta_Proxy_DmeTrack : public DXMeta_Proxy_Generic 
{
	DECLARE_CLASS(DXMeta_Proxy_DmeTrack, DXMeta_Proxy_Generic);
public:
	DXMeta_Proxy_DmeTrack(int flags = 0);
	virtual void OnInitialize();
	virtual void OnUnserialized();
	virtual bool OnAttributeChanged(const char* attribute, void* value);
	virtual bool OnAttributeRenamed(const char* attribute, const char* newName);
	virtual bool OnAttributeRemoved(const char* attribute);
protected:
	CUtlVector<DXMeta_Proxy_DmeClip*> children;
	bool collapsed;
	bool mute;
	bool synched;
	int clipType;
	float volume;
	float displayScale;
};

class DXMeta_Proxy_DmeTrackGroup : public DXMeta_Proxy_Generic 
{
	DECLARE_CLASS(DXMeta_Proxy_DmeTrackGroup, DXMeta_Proxy_Generic);
public:
	DXMeta_Proxy_DmeTrackGroup(int flags = 0);
	virtual void OnInitialize();
	virtual void OnUnserialized();
	virtual bool OnAttributeChanged(const char* attribute, void* value);
	virtual bool OnAttributeRenamed(const char* attribute, const char* newName);
	virtual bool OnAttributeRemoved(const char* attribute);
protected:
	CUtlVector<DXMeta_Proxy_DmeTrack*> children;
	bool visible;
	bool mute;
	float displayScale;
	bool minimized;
	float volume;
	bool forcemultitrack;
};

class DXMeta_Proxy_DmeTransform : public DXMeta_Proxy_Generic 
{
	DECLARE_CLASS(DXMeta_Proxy_DmeTransform, DXMeta_Proxy_Generic);
public:
	DXMeta_Proxy_DmeTransform(int flags = 0);
	virtual void OnInitialize();
	virtual void OnUnserialized();
	virtual bool OnAttributeChanged(const char* attribute, void* value);
	virtual bool OnAttributeRenamed(const char* attribute, const char* newName);
	virtual bool OnAttributeRemoved(const char* attribute);
protected:
	Vector position;
	Quaternion orientation;
};

class DXMeta_Proxy_DmeShape : public DXMeta_Proxy_Generic 
{
	DECLARE_CLASS(DXMeta_Proxy_DmeShape, DXMeta_Proxy_Generic);
public:
	DXMeta_Proxy_DmeShape(int flags = 0);
	virtual void OnInitialize();
	virtual void OnUnserialized();
	virtual bool OnAttributeChanged(const char* attribute, void* value);
	virtual bool OnAttributeRenamed(const char* attribute, const char* newName);
	virtual bool OnAttributeRemoved(const char* attribute);
protected:
	bool visible;
};

class DXMeta_Proxy_DmeDag : public DXMeta_Proxy_Generic 
{
	DECLARE_CLASS(DXMeta_Proxy_DmeDag, DXMeta_Proxy_Generic);
public:
	DXMeta_Proxy_DmeDag(int flags = 0);
	virtual void OnInitialize();
	virtual void OnUnserialized();
	virtual bool OnAttributeChanged(const char* attribute, void* value);
	virtual bool OnAttributeRenamed(const char* attribute, const char* newName);
	virtual bool OnAttributeRemoved(const char* attribute);
protected:
	DXMeta_Proxy_DmeTransform* transform;
	DXMeta_Proxy_DmeShape* shape;
	bool visible;
	CUtlVector<DXMeta_Proxy_DmeDag*> children;
};

class DXMeta_Proxy_DmeGameModel;

class DXMeta_Proxy_DmeGlobalFlexControllerOperator : public DXMeta_Proxy_Generic 
{
	DECLARE_CLASS(DXMeta_Proxy_DmeGlobalFlexControllerOperator, DXMeta_Proxy_Generic);
public:
	DXMeta_Proxy_DmeGlobalFlexControllerOperator(int flags = 0);
	virtual void OnInitialize();
	virtual void OnUnserialized();
	virtual bool OnAttributeChanged(const char* attribute, void* value);
	virtual bool OnAttributeRenamed(const char* attribute, const char* newName);
	virtual bool OnAttributeRemoved(const char* attribute);
protected:
	float flexWeight;
	DXMeta_Proxy_DmeGameModel* gameModel;
};

class DXMeta_Proxy_DmeGameModel : public DXMeta_Proxy_DmeDag
{
	DECLARE_CLASS(DXMeta_Proxy_DmeGameModel, DXMeta_Proxy_DmeDag);
public:
	DXMeta_Proxy_DmeGameModel(int flags = 0);
	virtual void OnInitialize();
	virtual void OnUnserialized();
	virtual bool OnAttributeChanged(const char* attribute, void* value);
	virtual bool OnAttributeRenamed(const char* attribute, const char* newName);
	virtual bool OnAttributeRemoved(const char* attribute);
protected:
	CUtlVector<float> flexWeights;
	const char* modelName;
	int skin;
	int body;
	int sequence;
	int attr_flags;
	CUtlVector<DXMeta_Proxy_DmeTransform*> bones;
	CUtlVector<DXMeta_Proxy_DmeGlobalFlexControllerOperator*> globalFlexControllers;
	bool computeBounds;
	bool evaluateProcedualBones;
};

class DXService : public CAutoGameSystemPerFrame
{
public:
	DXService(char const* name) : CAutoGameSystemPerFrame(name)
	{
	}
	virtual bool InitAllSystems()
	{
		return true;
	}
	virtual bool Init();
	virtual void PostInit();
	virtual void Shutdown();
	virtual void Update(float frametime);
	virtual void LevelInitPostEntity();
#ifdef CLIENT_DLL
	virtual void PreRender();
	void SetupEngineView(Vector& origin, QAngle& angles, float& fov);
	DXStatus* GetStatus();
	void RenderStackAddToHead(DXMeta_Proxy* proxy);
	void RenderStackAddToTail(DXMeta_Proxy* proxy);
	void RenderStackRemove(DXMeta_Proxy* proxy);
	void RenderStackClear();
	Vector origin;
	QAngle angles;
	float fov;
	bool active = false;
#endif
private:
#ifdef CLIENT_DLL
	DXStatus* status = new DXStatus();
#endif
	CDmxElement* root;
	CUtlVector<DXMeta_Proxy*> renderStack;
};

extern DXService g_DXService;

#endif
