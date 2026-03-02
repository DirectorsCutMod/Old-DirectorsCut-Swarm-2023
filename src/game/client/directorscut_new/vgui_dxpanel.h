//========= Director's Cut - https://github.com/SFMDX ============//
//
// Purpose: Director's Cut VGUI
//
//================================================================//

#ifndef _VGUI_DXPANEL_H_
#define _VGUI_DXPANEL_H_

#include "idxgui.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/PropertyPage.h>

using namespace vgui;

class CDXViewportPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE(CDXViewportPanel, EditablePanel);
public:
	CDXViewportPanel(Panel* parent, const char* panelName);
	virtual			~CDXViewportPanel(void);
	virtual void	Paint();
	virtual void	PaintBackground();
	virtual void OnTick();

private:
	int m_nEngineOutputTexture;
	vgui::HFont	font;
	CMaterialReference			m_ScreenMaterial;
	QAngle curAngle;
	void GetEngineBounds(int& x, int& y, int& w, int& h);
};

class CDXViewport : public Frame
{
	DECLARE_CLASS_SIMPLE(CDXViewport, Frame);
public:
	CDXViewport(Panel* parent, const char* panelName);
	virtual			~CDXViewport(void);
};

/*
class CDXPage : public PropertyPage
{
	DECLARE_CLASS_SIMPLE(CDXPage, PropertyPage);
public:
	CDXPage(Panel* parent, const char* panelName) : PropertyPage(parent, panelName) {}
	~CDXPage();
protected:
	MESSAGE_FUNC_PARAMS(OpenContextMenu, "OpenContextMenu", params);
private:
	DHANDLE<Menu> contextMenu;
};
*/

class CDXPanel : public Panel
{
	DECLARE_CLASS_SIMPLE(CDXPanel, Panel);
public:
	CDXPanel(VPANEL parent);
	virtual			~CDXPanel(void);
	virtual void	ApplySchemeSettings(IScheme* pScheme);
	virtual void	Paint();

protected:
	MESSAGE_FUNC_INT_INT(OnScreenSizeChanged, "OnScreenSizeChanged", oldwide, oldtall);

private:
	void ComputeSize(void);
	CDXViewport* pViewport;
};

class CDXGUI : public IDXGUI
{
private:
	CDXPanel* dxPanel;
public:
	CDXGUI(void);
	void Create(VPANEL parent);
	void Destroy(void);
};

#endif
