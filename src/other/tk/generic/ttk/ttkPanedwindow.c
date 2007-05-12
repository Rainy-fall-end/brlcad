/* $Id$
 *
 * Copyright (c) 2005, Joe English.  Freely redistributable.
 *
 * ttk::panedwindow widget implementation.
 *
 * TODO: track active/pressed sash.
 */

#include <string.h>
#include <tk.h>
#include "ttkManager.h"
#include "ttkTheme.h"
#include "ttkWidget.h"

#define MIN_SASH_THICKNESS 5

/*------------------------------------------------------------------------
 * +++ Layout algorithm.
 *
 * (pos=x/y, size=width/height, depending on -orient=horizontal/vertical)
 *
 * Each pane carries two pieces of state: the request size and the
 * position of the following sash.  (The final pane has no sash,
 * its sash position is used as a sentinel value).
 *
 * Pane geometry is determined by the sash positions.
 * When resizing, sash positions are computed from the request sizes,
 * the available space, and pane weights (see ComputePositions()).
 * This ensures continuous resize behavior (that is: changing
 * the size by X pixels then changing the size by Y pixels
 * gives the same result as changing the size by X+Y pixels
 * in one step).
 *
 * The request size is initially set to the slave window's requested size.
 * When the user drags a sash, each pane's request size is set to its
 * actual size.  This ensures that panes "stay put" on the next resize.
 *
 * If reqSize == 0, use 0 for the weight as well.  This ensures that
 * "collapsed" panes stay collapsed during a resize, regardless of
 * their nominal -weight.
 *
 * +++ Invariants.
 *
 * #sash 		=  #pane - 1
 * pos(pane[0]) 	=  0
 * pos(sash[i]) 	=  pos(pane[i]) + size(pane[i]), 0 <= i <= #sash
 * pos(pane[i+1]) 	=  pos(sash[i]) + size(sash[i]), 0 <= i <  #sash
 * pos(sash[#sash])	=  size(pw)   // sentinel value, constraint
 *
 * size(pw) 		=  sum(size(pane(0..#pane))) + sum(size(sash(0..#sash)))
 * size(pane[i]) 	>= 0,  for 0 <= i < #pane
 * size(sash[i]) 	>= 0,  for 0 <= i < #sash
 * ==> pos(pane[i]) <= pos(sash[i]) <= pos(pane[i+1]), for 0 <= i < #sash
 *
 * Assumption: all sashes are the same size.
 */

/*------------------------------------------------------------------------
 * +++ Widget record.
 */

typedef struct {
    Tcl_Obj 	*orientObj;
    int 	orient;
    Ttk_Manager	*mgr;
    Ttk_Layout	sashLayout;
    int 	sashThickness;
} PanedPart;

typedef struct {
    WidgetCore	core;
    PanedPart	paned;
} Paned;

/* @@@ NOTE: -orient is readonly 'cause dynamic oriention changes NYI
 */
static Tk_OptionSpec PanedOptionSpecs[] = {
    {TK_OPTION_STRING_TABLE, "-orient", "orient", "Orient", "vertical",
	Tk_Offset(Paned,paned.orientObj), Tk_Offset(Paned,paned.orient),
	0,(ClientData)ttkOrientStrings,READONLY_OPTION|STYLE_CHANGED },

    WIDGET_INHERIT_OPTIONS(ttkCoreOptionSpecs)
};

/*------------------------------------------------------------------------
 * +++ Slave pane record.
 */
typedef struct {
    int 	reqSize;		/* Pane request size */
    int 	sashPos;		/* Folowing sash position */
    int 	weight; 		/* Pane -weight, for resizing */
} Pane;

static Tk_OptionSpec PaneOptionSpecs[] = {
    {TK_OPTION_INT, "-weight", "weight", "Weight", "0",
	-1,Tk_Offset(Pane,weight), 0,0,GEOMETRY_CHANGED },
    {TK_OPTION_END, 0,0,0, NULL, -1,-1, 0,0,0}
};

/*------------------------------------------------------------------------
 * +++ Layout algorithm.
 */

/* ShoveUp --
 * 	Place sash i at specified position, recursively shoving
 * 	previous sashes upwards as needed, until hitting the top
 * 	of the window.  If that happens, shove back down.
 *
 * 	Returns: final position of sash i.
 */

static int ShoveUp(Paned *pw, int i, int pos)
{
    Pane *pane = Ttk_SlaveData(pw->paned.mgr, i);
    int sashThickness = pw->paned.sashThickness;

    if (i == 0) {
	if (pos < 0)
	    pos = 0;
    } else {
	Pane *prevPane = Ttk_SlaveData(pw->paned.mgr, i-1);
	if (pos < prevPane->sashPos + sashThickness)
	    pos = ShoveUp(pw, i-1, pos - sashThickness) + sashThickness;
    }
    return pane->sashPos = pos;
}

/* ShoveDown --
 * 	Same as ShoveUp, but going in the opposite direction
 * 	and stopping at the sentinel sash.
 */
static int ShoveDown(Paned *pw, int i, int pos)
{
    Pane *pane = Ttk_SlaveData(pw->paned.mgr,i);
    int sashThickness = pw->paned.sashThickness;

    if (i == pw->paned.mgr->nSlaves - 1) {
	pos = pane->sashPos; /* Sentinel value == master window size */
    } else {
	Pane *nextPane = Ttk_SlaveData(pw->paned.mgr,i+1);
	if (pos + sashThickness > nextPane->sashPos)
	    pos = ShoveDown(pw, i+1, pos + sashThickness) - sashThickness;
    }
    return pane->sashPos = pos;
}

/* PanedSize --
 * 	Compute the requested size of the paned widget.
 * 	Used as both the WidgetSpec sizeProc and the ManagerSpec sizeProc.
 */
static int PanedSize(void *recordPtr, int *widthPtr, int *heightPtr)
{
    Paned *pw = recordPtr;
    int nPanes = Ttk_NumberSlaves(pw->paned.mgr);
    int nSashes = nPanes - 1;
    int sashThickness = pw->paned.sashThickness;
    int width = 0, height = 0;
    int index;

    if (pw->paned.orient == TTK_ORIENT_HORIZONTAL) {
	for (index = 0; index < nPanes; ++index) {
	    Pane *pane = Ttk_SlaveData(pw->paned.mgr, index);
	    Tk_Window slaveWindow = Ttk_SlaveWindow(pw->paned.mgr, index);

	    if (height < Tk_ReqHeight(slaveWindow))
		height = Tk_ReqHeight(slaveWindow);
	    width += pane->reqSize;
	}
	width += nSashes * sashThickness;
    } else {
	for (index = 0; index < nPanes; ++index) {
	    Pane *pane = Ttk_SlaveData(pw->paned.mgr, index);
	    Tk_Window slaveWindow = Ttk_SlaveWindow(pw->paned.mgr, index);

	    if (width < Tk_ReqWidth(slaveWindow))
		width = Tk_ReqWidth(slaveWindow);
	    height += pane->reqSize;
	}
	height += nSashes * sashThickness;
    }

    *widthPtr = width;
    *heightPtr = height;
    return 1;
}

/* AdjustPanes --
 * 	Set pane request sizes from sash positions.
 *
 * NOTE:
 * 	AdjustPanes followed by ComputePositions (called during relayout)
 * 	will leave the sashes in the same place, as long as available size
 * 	remains contant.
 */
static void AdjustPanes(Paned *pw)
{
    int sashThickness = pw->paned.sashThickness;
    int pos = 0;
    int index;

    for (index = 0; index < Ttk_NumberSlaves(pw->paned.mgr); ++index) {
	Pane *pane = Ttk_SlaveData(pw->paned.mgr, index);
	int size = pane->sashPos - pos;
	pane->reqSize = size >= 0 ? size : 0;
	pos = pane->sashPos + sashThickness;
    }
}

/* ComputePositions --
 *	Set sash positions from pane request sizes and available space.
 *
 *	Allocate pane->reqSize pixels to each pane, and distribute
 *	the difference = available size - requested size according
 *	to pane->weight.
 *
 *	If there's still some left over, squeeze panes from the bottom up
 *	(This can happen if all weights are zero, or if one or more panes
 *	are too small to absorb the required shrinkage).
 *
 * Notes:
 * 	This doesn't distribute the remainder pixels as evenly as it could
 * 	when more than one pane has weight > 1.
 */
static void ComputePositions(Paned *pw)
{
    Ttk_Manager *mgr = pw->paned.mgr;
    int nPanes = Ttk_NumberSlaves(mgr);
    int sashThickness = pw->paned.sashThickness;
    int available
	= pw->paned.orient == TTK_ORIENT_HORIZONTAL
	? Tk_Width(pw->core.tkwin) : Tk_Height(pw->core.tkwin);
    int reqSize = 0, totalWeight = 0;
    int difference, delta, remainder, pos, i;

    if (nPanes == 0)
	return;

    /* Compute total required size and total available weight:
     */
    for (i = 0; i < nPanes; ++i) {
	Pane *pane = Ttk_SlaveData(mgr, i);
	reqSize += pane->reqSize;
	totalWeight += pane->weight * (pane->reqSize != 0);
    }

    /* Compute difference to be redistributed:
     */
    difference = available - reqSize - sashThickness*(nPanes-1);
    if (totalWeight != 0) {
	delta = difference / totalWeight;
	remainder = difference % totalWeight;
	if (remainder < 0) {
	    --delta;
	    remainder += totalWeight;
	}
    } else {
	delta = remainder = 0;
    }
    /* ASSERT: 0 <= remainder < totalWeight */

    /* Place sashes:
     */
    pos = 0;
    for (i = 0; i < nPanes; ++i) {
	Pane *pane = Ttk_SlaveData(mgr, i);
	int weight = pane->weight * (pane->reqSize != 0);
	int size = pane->reqSize + delta * weight;

	if (weight > remainder)
	    weight = remainder;
	remainder -= weight;
	size += weight;

	if (size < 0)
	    size = 0;

	pane->sashPos = (pos += size);
	pos += sashThickness;
    }

    /* Handle emergency shrink/emergency stretch:
     * Set sentinel sash position to end of widget,
     * shove preceding sashes up.
     */
    ShoveUp(pw, nPanes - 1, available);
}

/* PlacePanes --
 *	Places slave panes based on sash positions.
 */
static void PlacePanes(Paned *pw)
{
    int horizontal = pw->paned.orient == TTK_ORIENT_HORIZONTAL;
    int width = Tk_Width(pw->core.tkwin), height = Tk_Height(pw->core.tkwin);
    int sashThickness = pw->paned.sashThickness;
    int pos = 0;
    int index;

    for (index = 0; index < Ttk_NumberSlaves(pw->paned.mgr); ++index) {
	Pane *pane = Ttk_SlaveData(pw->paned.mgr, index);
	int size = pane->sashPos - pos;

	if (size > 0) {
	    if (horizontal) {
		Ttk_PlaceSlave(pw->paned.mgr, index, pos, 0, size, height);
	    } else {
		Ttk_PlaceSlave(pw->paned.mgr, index, 0, pos, width, size);
	    }
	} else {
	    Ttk_UnmapSlave(pw->paned.mgr, index);
	}

	pos = pane->sashPos + sashThickness;
    }
}

/*------------------------------------------------------------------------
 * +++ Manager specification.
 */

static void PanedPlaceSlaves(void *managerData)
{
    Paned *pw = managerData;
    ComputePositions(pw);
    PlacePanes(pw);
}

static void PaneAdded(Ttk_Manager *mgr, int index)
{
    Pane *pane = Ttk_SlaveData(mgr, index);
    Tk_Window slaveWindow = Ttk_SlaveWindow(mgr, index);
    Paned *pw = mgr->managerData;

    /* See also: PanedGeometryRequestProc */
    pane->reqSize
	= pw->paned.orient == TTK_ORIENT_HORIZONTAL
	? Tk_ReqWidth(slaveWindow) : Tk_ReqHeight(slaveWindow);
}

static void PaneRemoved(Ttk_Manager *mgr, int i) { /*no-op*/ }

static int PaneConfigured(
    Tcl_Interp *interp, Ttk_Manager *mgr, Ttk_Slave *slave, unsigned mask)
{
    Pane *pane = slave->slaveData;
    if (pane->weight < 0) {
	Tcl_AppendResult(interp, "-weight must be nonnegative", NULL);
	pane->weight = 0;
	return TCL_ERROR;
    }
    return TCL_OK;
}

/* PanedGeometryRequestProc --
 * 	Update pane request size, but only if slave is currently unmapped.
 * 	Geometry requests from mapped slaves are not directly honored,
 * 	in order to avoid unexpected pane resizes (esp. while the
 * 	user is dragging a sash [#1325286]).
 */

static void PanedGeometryRequestProc(
    ClientData clientData, Tk_Window slaveWindow)
{
    Ttk_Slave *slave = clientData;
    Pane *pane = slave->slaveData;
    Paned *pw = slave->manager->managerData;

    if (!Tk_IsMapped(slaveWindow)) {
	pane->reqSize
	    = pw->paned.orient == TTK_ORIENT_HORIZONTAL
	    ? Tk_ReqWidth(slaveWindow) : Tk_ReqHeight(slaveWindow);
    }

    /* Continue with default GeometryRequestProc:
     */
    Ttk_GeometryRequestProc(clientData, slaveWindow);
}

static Ttk_ManagerSpec PanedManagerSpec = {
    { "panedwindow", PanedGeometryRequestProc, Ttk_LostSlaveProc },
    PaneOptionSpecs, sizeof(Pane),
    PanedSize,
    PanedPlaceSlaves,
    PaneAdded,
    PaneRemoved,
    PaneConfigured,
};

/*------------------------------------------------------------------------
 * +++ Event handler.
 *
 * <<NOTE-PW-LEAVE-NOTIFYINFERIOR>>
 * Tk does not execute binding scripts for <Leave> events when
 * the pointer crosses from a parent to a child.  This widget
 * needs to know when that happens, though, so it can reset
 * the cursor.
 *
 * This event handler generates an <<EnteredChild>> virtual event
 * on LeaveNotify/NotifyInferior.
 */

static const unsigned PanedEventMask = LeaveWindowMask;
static void PanedEventProc(ClientData clientData, XEvent *eventPtr)
{
    WidgetCore *corePtr = clientData;
    if (   eventPtr->type == LeaveNotify
	&& eventPtr->xcrossing.detail == NotifyInferior)
    {
	TtkSendVirtualEvent(corePtr->tkwin, "EnteredChild");
    }
}

/*------------------------------------------------------------------------
 * +++ Initialization and cleanup hooks.
 */

static int PanedInitialize(Tcl_Interp *interp, void *recordPtr)
{
    Paned *pw = recordPtr;

    Tk_CreateEventHandler(pw->core.tkwin,
	PanedEventMask, PanedEventProc, recordPtr);
    pw->paned.mgr = Ttk_CreateManager(&PanedManagerSpec, pw, pw->core.tkwin);
    pw->paned.sashLayout = 0;
    pw->paned.sashThickness = 1;

    return TCL_OK;
}

static void PanedCleanup(void *recordPtr)
{
    Paned *pw = recordPtr;

    if (pw->paned.sashLayout)
	Ttk_FreeLayout(pw->paned.sashLayout);
    Tk_DeleteEventHandler(pw->core.tkwin,
	PanedEventMask, PanedEventProc, recordPtr);
    Ttk_DeleteManager(pw->paned.mgr);
}

/*------------------------------------------------------------------------
 * +++ Layout management hooks.
 */
static Ttk_Layout PanedGetLayout(
    Tcl_Interp *interp, Ttk_Theme themePtr, void *recordPtr)
{
    Paned *pw = recordPtr;
    Ttk_Layout panedLayout = TtkWidgetGetLayout(interp, themePtr, recordPtr);

    if (panedLayout) {
	int horizontal = pw->paned.orient == TTK_ORIENT_HORIZONTAL;
	const char *layoutName = 
	    horizontal ? ".Vertical.Sash" : ".Horizontal.Sash";
	Ttk_Layout sashLayout = Ttk_CreateSublayout(
	    interp, themePtr, panedLayout, layoutName, pw->core.optionTable);

	if (sashLayout) {
	    int sashWidth, sashHeight;

	    Ttk_LayoutSize(sashLayout, 0, &sashWidth, &sashHeight);
	    pw->paned.sashThickness = horizontal ? sashWidth : sashHeight;

	    if (pw->paned.sashLayout)
		Ttk_FreeLayout(pw->paned.sashLayout);
	    pw->paned.sashLayout = sashLayout;
	} else {
	    Ttk_FreeLayout(panedLayout);
	    return 0;
	}
    }

    /* Sanity-check:
     */
    if (pw->paned.sashThickness < MIN_SASH_THICKNESS)
	pw->paned.sashThickness = MIN_SASH_THICKNESS;

    return panedLayout;
}

static void PanedDisplay(void *recordPtr, Drawable d)
{
    Paned *pw = recordPtr;
    int horizontal = pw->paned.orient == TTK_ORIENT_HORIZONTAL;
    Ttk_Layout sashLayout = pw->paned.sashLayout;
    int sashThickness = pw->paned.sashThickness;
    Ttk_State state = pw->core.state;
    int nPanes = Ttk_NumberSlaves(pw->paned.mgr);
    int i;

    TtkWidgetDisplay(recordPtr, d);

    /* Draw sashes:
     */
    if (horizontal) {
	int height = Tk_Height(pw->core.tkwin);
	for (i = 0; i < nPanes; ++i) {
	    Pane *pane = Ttk_SlaveData(pw->paned.mgr, i);
	    Ttk_PlaceLayout(sashLayout, state,
		Ttk_MakeBox(pane->sashPos, 0, sashThickness, height));
	    Ttk_DrawLayout(sashLayout, state, d);
	}
    } else {
	int width = Tk_Width(pw->core.tkwin);
	for (i = 0; i < nPanes; ++i) {
	    Pane *pane = Ttk_SlaveData(pw->paned.mgr, i);
	    Ttk_PlaceLayout(sashLayout, state,
		Ttk_MakeBox(0, pane->sashPos, width, sashThickness));
	    Ttk_DrawLayout(sashLayout, state, d);
	}
    }
}

/*------------------------------------------------------------------------
 * +++ Widget commands.
 */

/* $pw add window [ options ... ]
 */
static int PanedAddCommand(
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], void *recordPtr)
{
    Paned *pw = recordPtr;
    Tk_Window slaveWindow;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "window");
	return TCL_ERROR;
    }

    slaveWindow = Tk_NameToWindow(
	interp, Tcl_GetString(objv[2]), pw->core.tkwin);

    if (!slaveWindow) {
	return TCL_ERROR;
    }

    return Ttk_AddSlave(interp, pw->paned.mgr, slaveWindow,
	    Ttk_NumberSlaves(pw->paned.mgr), objc - 3, objv + 3);
}

/* $pw insert $index $slave ?options...?
 * 	Insert new slave, or move existing one.
 */
static int PanedInsertCommand(
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], void *recordPtr)
{
    Paned *pw = recordPtr;
    int srcIndex, destIndex;
    Tk_Window slaveWindow;

    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2,objv, "index slave ?options...?");
	return TCL_ERROR;
    }

    slaveWindow = Tk_NameToWindow(
	interp, Tcl_GetString(objv[3]), pw->core.tkwin);
    if (!slaveWindow) {
	return TCL_ERROR;
    }

    if (!strcmp(Tcl_GetString(objv[2]), "end")) {
	destIndex = Ttk_NumberSlaves(pw->paned.mgr);
    } else if (!Ttk_GetSlaveFromObj(interp,pw->paned.mgr,objv[2],&destIndex)) {
	return TCL_ERROR;
    }

    srcIndex = Ttk_SlaveIndex(pw->paned.mgr, slaveWindow);
    if (srcIndex < 0) { /* New slave: */
	return Ttk_AddSlave(interp, pw->paned.mgr, slaveWindow,
		destIndex, objc - 4, objv + 4);
    } /* else -- move existing slave: */

    if (destIndex >= pw->paned.mgr->nSlaves)
	destIndex  = pw->paned.mgr->nSlaves - 1;
    Ttk_ReorderSlave(pw->paned.mgr, srcIndex, destIndex);

    return objc == 4 ? TCL_OK :
	Ttk_ConfigureSlave(interp, pw->paned.mgr,
	     pw->paned.mgr->slaves[destIndex], objc-4,objv+4);
}

/* $pw forget $pane
 */
static int PanedForgetCommand(
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], void *recordPtr)
{
    Paned *pw = recordPtr;
    int paneIndex;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2,objv, "pane");
	return TCL_ERROR;
    }

    if (!Ttk_GetSlaveFromObj(interp, pw->paned.mgr, objv[2], &paneIndex)) {
	return TCL_ERROR;
    }
    Ttk_ForgetSlave(pw->paned.mgr, paneIndex);

    return TCL_OK;
}

/* $pw identify $x $y
 * @@@ TODO: implement as documented, or change documentation.
 */
static int PanedIdentifyCommand(
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], void *recordPtr)
{
    Paned *pw = recordPtr;
    int sashThickness = pw->paned.sashThickness;
    int x, y, pos;
    int index;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2,objv, "x y");
	return TCL_ERROR;
    }
    if (   Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK
	|| Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK
    ) {
	return TCL_ERROR;
    }

    pos = pw->paned.orient == TTK_ORIENT_HORIZONTAL ? x : y;
    for (index = 0; index < pw->paned.mgr->nSlaves - 1; ++index) {
	Pane *pane = Ttk_SlaveData(pw->paned.mgr, index);
	if (pane->sashPos <= pos && pos <= pane->sashPos + sashThickness) {
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(index));
	    return TCL_OK;
	}
    }

    return TCL_OK;	/* empty list */
}

/* $pw pane $pane ?-option ?value -option value ...??
 * 	Query/modify pane options.
 */
static int PanedPaneCommand(
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], void *recordPtr)
{
    Paned *pw = recordPtr;
    int paneIndex;
    Ttk_Slave *slave;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2,objv, "pane ?-option value...?");
	return TCL_ERROR;
    }

    slave = Ttk_GetSlaveFromObj(interp,pw->paned.mgr,objv[2],&paneIndex);
    if (!slave) {
	return TCL_ERROR;
    }

    switch (objc) {
	case 3:
	    return TtkEnumerateOptions(interp, slave->slaveData, PaneOptionSpecs,
			pw->paned.mgr->slaveOptionTable, slave->slaveWindow);
	case 4:
	    return TtkGetOptionValue(interp, slave->slaveData,objv[3],
			pw->paned.mgr->slaveOptionTable, slave->slaveWindow);
	default:
	    return Ttk_ConfigureSlave(
		    interp, pw->paned.mgr, slave, objc-3,objv+3);
    }
}

/* $pw sashpos $index ?$newpos?
 * 	Query or modify sash position.
 */
static int PanedSashposCommand(
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], void *recordPtr)
{
    Paned *pw = recordPtr;
    int sashIndex, position = -1;
    Pane *pane;

    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 2,objv, "index ?newpos?");
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[2], &sashIndex) != TCL_OK) {
	return TCL_ERROR;
    }
    if (sashIndex < 0 || sashIndex >= Ttk_NumberSlaves(pw->paned.mgr) - 1) {
	Tcl_AppendResult(interp,
	    "sash index ", Tcl_GetString(objv[2]), " out of range",
	    NULL);
	return TCL_ERROR;
    }

    pane = Ttk_SlaveData(pw->paned.mgr, sashIndex);

    if (objc == 3) {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(pane->sashPos));
	return TCL_OK;
    }
    /* else -- set new sash position */

    if (Tcl_GetIntFromObj(interp, objv[3], &position) != TCL_OK) {
	return TCL_ERROR;
    }

    if (position < pane->sashPos) {
	ShoveUp(pw, sashIndex, position);
    } else {
	ShoveDown(pw, sashIndex, position);
    }

    AdjustPanes(pw);
    Ttk_ManagerLayoutChanged(pw->paned.mgr);

    Tcl_SetObjResult(interp, Tcl_NewIntObj(pane->sashPos));
    return TCL_OK;
}

static WidgetCommandSpec PanedCommands[] =
{
    { "add", 		PanedAddCommand },
    { "configure",	TtkWidgetConfigureCommand },
    { "cget",		TtkWidgetCgetCommand },
    { "forget", 	PanedForgetCommand },
    { "identify", 	PanedIdentifyCommand },
    { "insert", 	PanedInsertCommand },
    { "instate",	TtkWidgetInstateCommand },
    { "pane",   	PanedPaneCommand },
    { "sashpos",  	PanedSashposCommand },
    { "state",  	TtkWidgetStateCommand },
    { 0,0 }
};

/*------------------------------------------------------------------------
 * +++ Widget specification.
 */

static WidgetSpec PanedWidgetSpec =
{
    "TPanedwindow",		/* className */
    sizeof(Paned),		/* recordSize */
    PanedOptionSpecs,		/* optionSpecs */
    PanedCommands,		/* subcommands */
    PanedInitialize,		/* initializeProc */
    PanedCleanup,		/* cleanupProc */
    TtkCoreConfigure,		/* configureProc */
    TtkNullPostConfigure,		/* postConfigureProc */
    PanedGetLayout,		/* getLayoutProc */
    PanedSize, 			/* sizeProc */
    TtkWidgetDoLayout,		/* layoutProc */
    PanedDisplay		/* displayProc */
};

/*------------------------------------------------------------------------
 * +++ Elements and layouts.
 */

typedef struct {
    Tcl_Obj *thicknessObj;
} SashElement;

static Ttk_ElementOptionSpec SashElementOptions[] = {
    { "-sashthickness", TK_OPTION_INT,
	    Tk_Offset(SashElement,thicknessObj), "5" },
    {NULL}
};

static void SashElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    SashElement *sash = elementRecord;
    int thickness = MIN_SASH_THICKNESS;
    Tcl_GetIntFromObj(NULL, sash->thicknessObj, &thickness);
    *widthPtr = *heightPtr = thickness;
}

static Ttk_ElementSpec SashElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(SashElement),
    SashElementOptions,
    SashElementSize,
    TtkNullElementDraw
};

TTK_BEGIN_LAYOUT(PanedLayout)
    TTK_NODE("Panedwindow.background", 0)/* @@@ BUG: empty layouts don't work */
TTK_END_LAYOUT

TTK_BEGIN_LAYOUT(HorizontalSashLayout)
    TTK_NODE("Sash.hsash", TTK_FILL_X)
TTK_END_LAYOUT

TTK_BEGIN_LAYOUT(VerticalSashLayout)
    TTK_NODE("Sash.vsash", TTK_FILL_Y)
TTK_END_LAYOUT

/*------------------------------------------------------------------------
 * +++ Registration routine.
 */
MODULE_SCOPE 
void TtkPanedwindow_Init(Tcl_Interp *interp)
{
    Ttk_Theme themePtr = Ttk_GetDefaultTheme(interp);
    RegisterWidget(interp, "ttk::panedwindow", &PanedWidgetSpec);

    Ttk_RegisterElement(interp, themePtr, "hsash", &SashElementSpec, 0);
    Ttk_RegisterElement(interp, themePtr, "vsash", &SashElementSpec, 0);

    Ttk_RegisterLayout(themePtr, "TPanedwindow", PanedLayout);
    Ttk_RegisterLayout(themePtr, "Horizontal.Sash", HorizontalSashLayout);
    Ttk_RegisterLayout(themePtr, "Vertical.Sash", VerticalSashLayout);
}
