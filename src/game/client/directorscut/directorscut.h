//========= Director's Cut - https://github.com/teampopplio/directorscut ============//
//
// Purpose: Director's Cut shared game system.
//
// $NoKeywords: $
//=============================================================================//

#ifndef _DIRECTORSCUT_H_
#define _DIRECTORSCUT_H_

#include "igamesystem.h"
#include "physpropclientside.h"
#include "mathlib/vector.h"
#include "dag_entity.h"
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "imgui_public.h"
#include "tutorial.h"
#include "GraphEditor.h"
#include "ImSequencer.h"
#include "ImCurveEdit.h"
#include <array>
#include <algorithm>

// Version information
// Increment for each release
#define DX_VERSION_MAJOR 0
#define DX_VERSION_MINOR 2
#define DX_VERSION_PATCH 0

// These refer to where this build is being uploaded to
#define DX_UNIVERSE_PUBLIC 0
#define DX_UNIVERSE_BETA 1

// Text definitions for each universe
#define DX_UNIVERSE_PUBLIC_TEXT "Public"
#define DX_UNIVERSE_BETA_TEXT "Beta"

// Text origins for each universe
#define DX_UNIVERSE_PUBLIC_ORIGIN "Steam release"
#define DX_UNIVERSE_BETA_ORIGIN "For testing purposes only"

// Preprocessor definition helpers
#ifdef DX_UNIVERSE_SET_PUBLIC
#define DX_UNIVERSE DX_UNIVERSE_PUBLIC
#endif
#ifdef DX_UNIVERSE_SET_BETA
#define DX_UNIVERSE DX_UNIVERSE_BETA
#endif

#ifndef DX_UNIVERSE
#pragma message("DX_UNIVERSE is not defined, defaulting to DX_UNIVERSE_BETA.")
#define DX_UNIVERSE DX_UNIVERSE_BETA
#endif

// Set the universe text
#if DX_UNIVERSE == DX_UNIVERSE_PUBLIC
#define DX_UNIVERSE_TEXT DX_UNIVERSE_PUBLIC_TEXT
#define DX_UNIVERSE_ORIGIN DX_UNIVERSE_PUBLIC_ORIGIN
#endif
#if DX_UNIVERSE == DX_UNIVERSE_BETA
#define DX_UNIVERSE_TEXT DX_UNIVERSE_BETA_TEXT
#define DX_UNIVERSE_ORIGIN DX_UNIVERSE_BETA_ORIGIN
#endif

// Complain to compiler if DX_UNIVERSE is set higher
#if DX_UNIVERSE > DX_UNIVERSE_SET_PUBLIC
#pragma message("DX_UNIVERSE: This build is not intended for public release.")
#endif

#define GRAPH_EDITOR_TEMPLATES_MAX 2

struct GraphEditorDelegate : public GraphEditor::Delegate
{
	bool AllowedLink(GraphEditor::NodeIndex from, GraphEditor::NodeIndex to) override
	{
		return true;
	}

	void SelectNode(GraphEditor::NodeIndex nodeIndex, bool selected) override
	{
		mNodes[nodeIndex].mSelected = selected;
	}

	void MoveSelectedNodes(const ImVec2 delta) override
	{
		for (auto& node : mNodes)
		{
			if (!node.mSelected)
			{
			continue;
			}
			node.x += delta.x;
			node.y += delta.y;
		}
	}

	virtual void RightClick(GraphEditor::NodeIndex nodeIndex, GraphEditor::SlotIndex slotIndexInput, GraphEditor::SlotIndex slotIndexOutput) override
	{
	}

	void AddLink(GraphEditor::NodeIndex inputNodeIndex, GraphEditor::SlotIndex inputSlotIndex, GraphEditor::NodeIndex outputNodeIndex, GraphEditor::SlotIndex outputSlotIndex) override
	{
		mLinks.push_back({ inputNodeIndex, inputSlotIndex, outputNodeIndex, outputSlotIndex });
	}

	void DelLink(GraphEditor::LinkIndex linkIndex) override
	{
		mLinks.erase(mLinks.begin() + linkIndex);
	}

	void CustomDraw(ImDrawList* drawList, ImRect rectangle, GraphEditor::NodeIndex nodeIndex) override
	{
		drawList->AddLine(rectangle.Min, rectangle.Max, IM_COL32(0, 0, 0, 255));
		drawList->AddText(rectangle.Min, IM_COL32(255, 128, 64, 255), "Draw");
	}

	const size_t GetTemplateCount() override
	{
		return sizeof(mTemplates) / sizeof(GraphEditor::Template);
	}

	const GraphEditor::Template GetTemplate(GraphEditor::TemplateIndex index) override
	{
		return mTemplates[index];
	}

	const size_t GetNodeCount() override
	{
		return mNodes.size();
	}

	const GraphEditor::Node GetNode(GraphEditor::NodeIndex index) override
	{
		const auto& myNode = mNodes[index];
		GraphEditor::Node list;
		list.mName = myNode.name;
		list.mTemplateIndex = myNode.templateIndex;
		list.mRect = ImRect(ImVec2(myNode.x, myNode.y), ImVec2(myNode.x + 200, myNode.y + 200));
		list.mSelected = myNode.mSelected;
		return list;
	}

	const size_t GetLinkCount() override
	{
		return mLinks.size();
	}

	const GraphEditor::Link GetLink(GraphEditor::LinkIndex index) override
	{
		return mLinks[index];
	}

	// Graph datas
	std::vector<GraphEditor::Template> mTemplates;

	struct Node
	{
		const char* name;
		GraphEditor::TemplateIndex templateIndex;
		float x, y;
		bool mSelected;
	};

	std::vector<Node> mNodes;

	std::vector<GraphEditor::Link> mLinks;
};

static const char* SequencerItemTypeNames[] = { "Camera","Music", "ScreenEffect", "FadeIn", "Animation" };

struct RampEdit : public ImCurveEdit::Delegate
{
   RampEdit()
   {
      mPts[0][0] = ImVec2(-10.f, 0);
      mPts[0][1] = ImVec2(20.f, 0.6f);
      mPts[0][2] = ImVec2(25.f, 0.2f);
      mPts[0][3] = ImVec2(70.f, 0.4f);
      mPts[0][4] = ImVec2(120.f, 1.f);
      mPointCount[0] = 5;

      mPts[1][0] = ImVec2(-50.f, 0.2f);
      mPts[1][1] = ImVec2(33.f, 0.7f);
      mPts[1][2] = ImVec2(80.f, 0.2f);
      mPts[1][3] = ImVec2(82.f, 0.8f);
      mPointCount[1] = 4;


      mPts[2][0] = ImVec2(40.f, 0);
      mPts[2][1] = ImVec2(60.f, 0.1f);
      mPts[2][2] = ImVec2(90.f, 0.82f);
      mPts[2][3] = ImVec2(150.f, 0.24f);
      mPts[2][4] = ImVec2(200.f, 0.34f);
      mPts[2][5] = ImVec2(250.f, 0.12f);
      mPointCount[2] = 6;
      mbVisible[0] = mbVisible[1] = mbVisible[2] = true;
      mMax = ImVec2(1.f, 1.f);
      mMin = ImVec2(0.f, 0.f);
   }
   size_t GetCurveCount()
   {
      return 3;
   }

   bool IsVisible(size_t curveIndex)
   {
      return mbVisible[curveIndex];
   }
   size_t GetPointCount(size_t curveIndex)
   {
      return mPointCount[curveIndex];
   }

   uint32_t GetCurveColor(size_t curveIndex)
   {
      uint32_t cols[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 };
      return cols[curveIndex];
   }
   ImVec2* GetPoints(size_t curveIndex)
   {
      return mPts[curveIndex];
   }
   virtual ImCurveEdit::CurveType GetCurveType(size_t curveIndex) const { return ImCurveEdit::CurveSmooth; }
   virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value)
   {
      mPts[curveIndex][pointIndex] = ImVec2(value.x, value.y);
      SortValues(curveIndex);
      for (size_t i = 0; i < GetPointCount(curveIndex); i++)
      {
         if (mPts[curveIndex][i].x == value.x)
            return (int)i;
      }
      return pointIndex;
   }
   virtual void AddPoint(size_t curveIndex, ImVec2 value)
   {
      if (mPointCount[curveIndex] >= 8)
         return;
      mPts[curveIndex][mPointCount[curveIndex]++] = value;
      SortValues(curveIndex);
   }
   virtual ImVec2& GetMax() { return mMax; }
   virtual ImVec2& GetMin() { return mMin; }
   virtual unsigned int GetBackgroundColor() { return 0; }
   ImVec2 mPts[3][8];
   size_t mPointCount[3];
   bool mbVisible[3];
   ImVec2 mMin;
   ImVec2 mMax;
private:
   void SortValues(size_t curveIndex)
   {
      auto b = std::begin(mPts[curveIndex]);
      auto e = std::begin(mPts[curveIndex]) + GetPointCount(curveIndex);
      std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });

   }
};

struct MySequence : public ImSequencer::SequenceInterface
{
   // interface with sequencer

   virtual int GetFrameMin() const {
      return mFrameMin;
   }
   virtual int GetFrameMax() const {
      return mFrameMax;
   }
   virtual int GetItemCount() const { return (int)myItems.size(); }

   virtual int GetItemTypeCount() const { return sizeof(SequencerItemTypeNames) / sizeof(char*); }
   virtual const char* GetItemTypeName(int typeIndex) const { return SequencerItemTypeNames[typeIndex]; }
   virtual const char* GetItemLabel(int index) const
   {
      static char tmps[512];
      V_snprintf(tmps, 512, "[%02d] %s", index, SequencerItemTypeNames[myItems[index].mType]);
      return tmps;
   }

   virtual void Get(int index, int** start, int** end, int* type, unsigned int* color)
   {
      MySequenceItem& item = myItems[index];
      if (color)
         *color = 0xFFAA8080; // same color for everyone, return color based on type
      if (start)
         *start = &item.mFrameStart;
      if (end)
         *end = &item.mFrameEnd;
      if (type)
         *type = item.mType;
   }
   virtual void Add(int type) { myItems.push_back(MySequenceItem{ type, 0, 10, false }); };
   virtual void Del(int index) { myItems.erase(myItems.begin() + index); }
   virtual void Duplicate(int index) { myItems.push_back(myItems[index]); }

   virtual size_t GetCustomHeight(int index) { return myItems[index].mExpanded ? 300 : 0; }

   // my datas
   MySequence() : mFrameMin(0), mFrameMax(0) {}
   int mFrameMin, mFrameMax;
   struct MySequenceItem
   {
      int mType;
      int mFrameStart, mFrameEnd;
      bool mExpanded;
   };
   std::vector<MySequenceItem> myItems;
   RampEdit rampEdit;

   virtual void DoubleClick(int index) {
      if (myItems[index].mExpanded)
      {
         myItems[index].mExpanded = false;
         return;
      }
      for (auto& item : myItems)
         item.mExpanded = false;
      myItems[index].mExpanded = !myItems[index].mExpanded;
   }

   virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect)
   {
      static const char* labels[] = { "Translation", "Rotation" , "Scale" };

      rampEdit.mMax = ImVec2(float(mFrameMax), 1.f);
      rampEdit.mMin = ImVec2(float(mFrameMin), 0.f);
      draw_list->PushClipRect(legendClippingRect.Min, legendClippingRect.Max, true);
      for (int i = 0; i < 3; i++)
      {
         ImVec2 pta(legendRect.Min.x + 30, legendRect.Min.y + i * 14.f);
         ImVec2 ptb(legendRect.Max.x, legendRect.Min.y + (i + 1) * 14.f);
         draw_list->AddText(pta, rampEdit.mbVisible[i] ? 0xFFFFFFFF : 0x80FFFFFF, labels[i]);
         if (ImRect(pta, ptb).Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(0))
            rampEdit.mbVisible[i] = !rampEdit.mbVisible[i];
      }
      draw_list->PopClipRect();

      ImGui::SetCursorScreenPos(rc.Min);
      ImCurveEdit::Edit(rampEdit, rc.Max - rc.Min, 137 + index, &clippingRect);
   }

   virtual void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect)
   {
      rampEdit.mMax = ImVec2(float(mFrameMax), 1.f);
      rampEdit.mMin = ImVec2(float(mFrameMin), 0.f);
      draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
      for (int i = 0; i < 3; i++)
      {
         for (int j = 0; j < rampEdit.mPointCount[i]; j++)
         {
            float p = rampEdit.mPts[i][j].x;
            if (p < myItems[index].mFrameStart || p > myItems[index].mFrameEnd)
               continue;
            float r = (p - mFrameMin) / float(mFrameMax - mFrameMin);
            float x = ImLerp(rc.Min.x, rc.Max.x, r);
            draw_list->AddLine(ImVec2(x, rc.Min.y + 6), ImVec2(x, rc.Max.y - 4), 0xAA000000, 4.f);
         }
      }
      draw_list->PopClipRect();
   }
};

class Version
{
public:
	Version(const int major, const int minor, const int patch) : m_major(major), m_minor(minor), m_patch(patch)
	{
		char* version = new char[16];
		sprintf(version, "%d.%d.%d", m_major, m_minor, m_patch);
		m_version = version;
	}
	~Version()
	{
		delete[] m_version;
	}
	const char* GetVersion() const
	{
		return m_version;
	}
	int m_major;
	int m_minor;
	int m_patch;
protected:
	char* m_version;
};

class CDirectorsCutSystem : public CAutoGameSystemPerFrame
{
public:

	CDirectorsCutSystem() : CAutoGameSystemPerFrame("CDirectorsCutSystem")
	{
	}

	virtual bool InitAllSystems()
	{
		return true;
	}
	
	virtual void PostInit();
	virtual void Shutdown();
	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPreEntity();
	virtual void Update(float frametime);
	void SetupEngineView(Vector &origin, QAngle &angles, float &fov);
	void Frustum(float left, float right, float bottom, float top, float znear, float zfar, float* m16);
	void Perspective(float fov, float aspect, float znear, float zfar, float* m16);
	void OrthoGraphic(const float l, float r, float b, const float t, float zn, const float zf, float* m16);
	void LookAt(const float* eye, const float* at, const float* up, float* m16);
	void SetDefaultSettings();
	void InitSceneMaterials();
	float cameraView[16];
	float cameraProjection[16];
	float identityMatrix[16];
	float snap[3];
	float distance = 100.f;
	float fov = 93; // dunno why this works
	float fovDefault = 93;
	float fovAdjustment = 2;
	float playerFov;
	float camYAngle = 165.f / 180.f * M_PI_F;
	float camXAngle = 32.f / 180.f * M_PI_F;
	float gridSize = 500.f;
	float currentTimeScale = 1.f;
	float timeScale = 1.f;
	Vector pivot;
	Vector newPivot;
	Vector engineOrigin;
	Vector playerOrigin;
	Vector deltaOrigin;
	Vector poseBoneOrigin;
	QAngle engineAngles;
	QAngle playerAngles;
	QAngle deltaAngles;
	QAngle poseBoneAngles;
	CUtlVector < CElementPointer* > elements;
	Version directorcut_version = Version(DX_VERSION_MAJOR, DX_VERSION_MINOR, DX_VERSION_PATCH);
	InputContextHandle_t inputContext;
	CUtlVector<TutorialSection*> tutorialSections;
	char* directorscut_author = "KiwifruitDev";
	char modelName[CHAR_MAX];
	char lightTexture[CHAR_MAX];
	char savePath[MAX_PATH];
	int elementIndex = -1;
	int nextElementIndex = -1;
	int boneIndex = -1;
	int poseIndex = -1;
	int nextPoseIndex = -1;
	int flexIndex = -1;
	int operation = 2;
	int oldOperation = 2;
	int hoveringInfo[3];
	int tutorialSectionIndex = 0;
	bool useSnap = false;
	bool orthographic = false;
	bool firstEndScene = true;
	bool cursorState = false;
	bool imguiActive = false;
	bool levelInit = false;
	bool mainMenu = true;
	bool selecting = false;
	bool justSetPivot = false;
	bool pivotMode = false;
	bool spawnAtPivot = false;
	bool windowVisibilities[7];
	bool inspectorDocked = true;
	bool gotInput = false;
	bool savedOnce = false;
	ImFont* fontTahoma;
	CTextureReference m_ScreenBuffer;
	GraphEditor::Options options;
	GraphEditorDelegate delegate;
	GraphEditor::ViewState viewState;
	GraphEditor::FitOnScreen fit = GraphEditor::Fit_None;
	int graphEditorTab = 0;
	MySequence mySequence;
	int selectedEntry = -1;
	int firstFrame = 0;
    bool expanded = true;
    int currentFrame = 100;
};

// singleton
CDirectorsCutSystem &DirectorsCutGameSystem();

// TODO: move this somewhere sensible

// TF2 proxy material dummies
// eventually support will be added to modify these proxies in-editor
// but as it stands, these classes just output "normal" values to solve texture issues

class CDummyProxy : public IMaterialProxy
{
public:
	virtual bool Init(IMaterial* pMaterial, KeyValues* pKeyValues);
	virtual void OnBind(void*) {};
	virtual void Release() {};
	virtual IMaterial* GetMaterial();
	IMaterial* mat;
};

class CDummyProxyResultFloat : public CDummyProxy
{
public:
	virtual bool Init(IMaterial* pMaterial, KeyValues* pKeyValues);
	IMaterialVar* resultVar;
};

class CDummyProxyResultFloatInverted : public CDummyProxy
{
public:
	virtual bool Init(IMaterial* pMaterial, KeyValues* pKeyValues);
	IMaterialVar* resultVar;
};

class CDummyProxyResultRGB : public CDummyProxy
{
public:
	virtual bool Init(IMaterial* pMaterial, KeyValues* pKeyValues);
	IMaterialVar* resultVar;
};

class CDummyProxyResultRGBInverted : public CDummyProxy
{
public:
	virtual bool Init(IMaterial* pMaterial, KeyValues* pKeyValues);
	IMaterialVar* resultVar;
};

class CDummyInvisProxy : public CDummyProxy {};
class CDummySpyInvisProxy : public CDummyProxy {};
class CDummyWeaponInvisProxy : public CDummyProxy {};
class CDummyVmInvisProxy : public CDummyProxy {};
class CDummyBuildingInvisProxy : public CDummyProxy {};
class CDummyCommunityWeaponProxy : public CDummyProxy {};
class CDummyInvulnLevelProxy : public CDummyProxy {};
class CDummyBurnLevelProxy : public CDummyProxyResultFloat {};
class CDummyYellowLevelProxy : public CDummyProxyResultFloatInverted {};
class CDummyModelGlowColorProxy : public CDummyProxyResultRGB {};
class CDummyItemTintColorProxy : public CDummyProxyResultRGB {};
class CDummyBuildingRescueLevelProxy : public CDummyProxy {};
class CDummyTeamTextureProxy : public CDummyProxy {};
class CDummyAnimatedWeaponSheenProxy : public CDummyProxy {};
class CDummyWeaponSkinProxy : public CDummyProxy {};
class CDummyShieldFalloffProxy : public CDummyProxy {};
class CDummyStatTrakIllumProxy : public CDummyProxy {};
class CDummyStatTrakDigitProxy : public CDummyProxy {};
class CDummyStatTrakIconProxy : public CDummyProxy {};
class CDummyStickybombGlowColorProxy : public CDummyProxy {};
class CDummySniperRifleChargeProxy : public CDummyProxy {};
class CDummyHeartbeatProxy : public CDummyProxy {};
class CDummyWheatlyEyeGlowProxy : public CDummyProxy {};
class CDummyBenefactorLevelProxy : public CDummyProxy {};

EXPOSE_INTERFACE(CDummyInvisProxy, IMaterialProxy, "invis" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummySpyInvisProxy, IMaterialProxy, "spy_invis" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyWeaponInvisProxy, IMaterialProxy, "weapon_invis" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyVmInvisProxy, IMaterialProxy, "vm_invis" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyBuildingInvisProxy, IMaterialProxy, "building_invis" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyCommunityWeaponProxy, IMaterialProxy, "CommunityWeapon" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyInvulnLevelProxy, IMaterialProxy, "InvulnLevel" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyBurnLevelProxy, IMaterialProxy, "BurnLevel" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyYellowLevelProxy, IMaterialProxy, "YellowLevel" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyModelGlowColorProxy, IMaterialProxy, "ModelGlowColor" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyItemTintColorProxy, IMaterialProxy, "ItemTintColor" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyBuildingRescueLevelProxy, IMaterialProxy, "BuildingRescueLevel" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyTeamTextureProxy, IMaterialProxy, "TeamTexture" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyAnimatedWeaponSheenProxy, IMaterialProxy, "AnimatedWeaponSheen" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyWeaponSkinProxy, IMaterialProxy, "WeaponSkin" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyShieldFalloffProxy, IMaterialProxy, "ShieldFalloff" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyStatTrakIllumProxy, IMaterialProxy, "StatTrakIllum" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyStatTrakDigitProxy, IMaterialProxy, "StatTrakDigit" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyStatTrakIconProxy, IMaterialProxy, "StatTrakIcon" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyStickybombGlowColorProxy, IMaterialProxy, "StickybombGlowColor" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummySniperRifleChargeProxy, IMaterialProxy, "SniperRifleCharge" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyHeartbeatProxy, IMaterialProxy, "Heartbeat" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyWheatlyEyeGlowProxy, IMaterialProxy, "WheatlyEyeGlow" IMATERIAL_PROXY_INTERFACE_VERSION);
EXPOSE_INTERFACE(CDummyBenefactorLevelProxy, IMaterialProxy, "BenefactorLevel" IMATERIAL_PROXY_INTERFACE_VERSION);

#endif
