//========= Director's Cut - https://github.com/SFMDX ============//
//
// Purpose: Director's Cut VGUI
//
//================================================================//

#ifndef _IDXGUI_H_
#define _IDXGUI_H_

#include <vgui/VGUI.h>

namespace vgui
{
	class Panel;
}

abstract_class IDXGUI
{
public:
	virtual void		Create(vgui::VPANEL parent) = 0;
	virtual void		Destroy(void) = 0;
};

extern IDXGUI* dxgui;

#endif // IFPSPANEL_H