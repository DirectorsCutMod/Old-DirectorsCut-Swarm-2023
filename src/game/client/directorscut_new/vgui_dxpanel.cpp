//========= Director's Cut - https://github.com/SFMDX ============//
//
// Purpose: Director's Cut VGUI
//
//================================================================//

#include "cbase.h"

#include "directorscut_new/dx_service.h"
#include "vgui_dxpanel.h"
#include "iclientmode.h"
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/PropertyPage.h>
#include "vguimatsurface/imatsystemsurface.h"
#include "rendertexture.h"
#include <view.h>
#include "viewrender.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_BUILD_FACTORY(CDXViewportPanel);
DECLARE_BUILD_FACTORY(CDXViewport);

CDXViewportPanel::CDXViewportPanel(Panel* parent, const char* panelName) : EditablePanel(parent, panelName)
{
	SetPaintEnabled(false);
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(true);

	Msg("Started viewport\n");
	
	IScheme* pscheme = scheme()->GetIScheme(GetScheme());
	font = pscheme->GetFont("DefaultLargeOutline");

	m_nEngineOutputTexture = vgui::surface()->CreateNewTextureID();
	vgui::ivgui()->AddTickSignal(GetVPanel());

	KeyValues* pVMTKeyValues = NULL;
	pVMTKeyValues = new KeyValues("UnlitGeneric");
	pVMTKeyValues->SetString("$basetexture", "_rt_FullFrameFB");
	pVMTKeyValues->SetInt("$nofog", 1);
	m_ScreenMaterial.Init("MiniViewportEngineRenderAreaSceneMaterial", pVMTKeyValues);
	m_ScreenMaterial->Refresh();
}


void CDXViewportPanel::OnTick()
{
}

CDXViewportPanel::~CDXViewportPanel(void)
{
}


void CDXViewportPanel::Paint()
{
}

void CDXViewportPanel::GetEngineBounds(int& x, int& y, int& w, int& h)
{
	x = 0;
	y = 0;
	GetSize(w, h);

	// Check aspect ratio
	int sx, sy;
	surface()->GetScreenSize(sx, sy);

	if (sy > 0 &&
		h > 0)
	{
		float screenaspect = (float)sx / (float)sy;
		float aspect = (float)w / (float)h;

		float ratio = screenaspect / aspect;

		// Screen is wider, need bars at top and bottom
		if (ratio > 1.0f)
		{
			int usetall = (float)w / screenaspect;
			y = (h - usetall) / 2;
			h = usetall;
		}
		// Screen is narrower, need bars at left/right
		else
		{
			int usewide = (float)h * screenaspect;
			x = (w - usewide) / 2;
			w = usewide;
		}
	}
}

void CDXViewportPanel::PaintBackground()
{	
	surface()->DrawSetColor(0, 0, 0, 255);
	surface()->DrawFilledRect( 0, 0, GetWide(), GetTall() );
	if (engine->IsInGame())
	{
		int x, y, w, h;
		GetEngineBounds(x, y, w, h);

		CMatRenderContextPtr pRenderContext(g_pMaterialSystem);
		g_pMatSystemSurface->DrawSetTextureMaterial(m_nEngineOutputTexture, m_ScreenMaterial);
		surface()->DrawSetColor(Color(0, 0, 0, 255));
		surface()->DrawTexturedRect(x, y, x + w, y + h);
		pRenderContext.SafeRelease();

		//CMatRenderContextPtr pRenderContext(g_pMaterialSystem);
		//g_pMatSystemSurface->DrawSetTextureMaterial(m_nEngineOutputTexture, m_ScreenMaterial);
		//pRenderContext.SafeRelease();
		//surface()->DrawTexturedRect(0, 0, GetWide(), GetTall());
	}
	// overlay
	DXStatus* status = g_DXService.GetStatus();
	surface()->DrawSetTextColor(status->GetLabelColor());
	surface()->DrawSetTextPos(4, 4);
	const wchar_t* label = status->GetLabel();
	surface()->DrawPrintText(label, wcslen(label));
}

CDXViewport::CDXViewport(Panel* parent, const char* panelName) : Frame(parent, panelName)
{
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/BoxRocket.res", "BoxRocketScheme"));
	LoadControlSettings("resource/dxviewport.res");
	SetTitle("Engine Viewport", true);
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	Activate();
}

CDXViewport::~CDXViewport(void)
{
}

CDXPanel::CDXPanel(vgui::VPANEL parent)
{
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/BoxRocket.res", "BoxRocketScheme"));
	SetParent(parent);
	SetVisible(true);
	ComputeSize();
	SetName("CDXPanel");

	pViewport = new CDXViewport(this, "Viewport");
	
	MakeReadyForUse();
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
}

CDXPanel::~CDXPanel(void)
{
	pViewport->DeletePanel();
}

void CDXPanel::ComputeSize(void)
{
	int wide, tall;
	vgui::ipanel()->GetSize(GetVParent(), wide, tall);

	int x = 0;
	int y = 0;
	if (IsX360())
	{
		x += XBOX_MINBORDERSAFE * wide;
		y += XBOX_MINBORDERSAFE * tall;
	}
	SetPos(x, y);
	SetSize(wide, tall);
}

void CDXPanel::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	ComputeSize();
}

void CDXPanel::OnScreenSizeChanged(int iOldWide, int iOldTall)
{
	BaseClass::OnScreenSizeChanged(iOldWide, iOldTall);
	ComputeSize();
}

void CDXPanel::Paint()
{
	BaseClass::Paint();
}

CDXGUI::CDXGUI(void)
{
	dxPanel = NULL;
}

void CDXGUI::Create(vgui::VPANEL parent)
{
	//dxPanel = new CDXPanel(parent);
}

void CDXGUI::Destroy(void)
{
	if (dxPanel)
	{
		dxPanel->SetParent((vgui::Panel*)NULL);
		delete dxPanel;
	}
}

static CDXGUI g_DXPanel;
IDXGUI* dxgui = (IDXGUI*)&g_DXPanel;
