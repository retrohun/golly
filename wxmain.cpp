                        /*** /

This file is part of Golly, a Game of Life Simulator.
Copyright (C) 2007 Andrew Trevorrow and Tomas Rokicki.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 Web site:  http://sourceforge.net/projects/golly
 Authors:   rokicki@gmail.com  andrew@trevorrow.com

                        / ***/

#include "wx/wxprec.h"     // for compilers that support precompilation
#ifndef WX_PRECOMP
   #include "wx/wx.h"      // for all others include the necessary headers
#endif

#include "wx/dnd.h"        // for wxFileDropTarget
#include "wx/filename.h"   // for wxFileName
#include "wx/clipbrd.h"    // for wxTheClipboard
#if wxUSE_TOOLTIPS
   #include "wx/tooltip.h" // for wxToolTip
#endif

#include "bigint.h"
#include "lifealgo.h"
#include "qlifealgo.h"
#include "hlifealgo.h"

#include "wxgolly.h"       // for wxGetApp, statusptr, viewptr, bigview
#include "wxutils.h"       // for Warning, Fatal, etc
#include "wxprefs.h"       // for gollydir, SavePrefs, SetPasteMode, etc
#include "wxinfo.h"        // for ShowInfo, GetInfoFrame
#include "wxhelp.h"        // for ShowHelp, GetHelpFrame
#include "wxstatus.h"      // for statusptr->...
#include "wxview.h"        // for viewptr->...
#include "wxrender.h"      // for InitDrawingData, DestroyDrawingData
#include "wxscript.h"      // for inscript
#include "wxlayer.h"       // for AddLayer, maxlayers, currlayer, etc
#include "wxundo.h"        // for currlayer->undoredo->...
#include "wxmain.h"

#ifdef __WXMAC__
   #include <Carbon/Carbon.h>    // for GetCurrentProcess, etc
#endif

// -----------------------------------------------------------------------------

// IDs for timer and menu commands
enum {
   // one-shot timer
   ID_ONE_TIMER = wxID_HIGHEST,

   // File menu
   // wxID_NEW,
   // wxID_OPEN,
   ID_OPEN_CLIP,
   ID_OPEN_RECENT,
   // last 2 items in Open Recent submenu
   ID_CLEAR_MISSING_PATTERNS = ID_OPEN_RECENT + MAX_RECENT + 1,
   ID_CLEAR_ALL_PATTERNS,
   ID_SHOW_PATTERNS,
   ID_PATTERN_DIR,
   // wxID_SAVE,
   ID_SAVE_XRLE,
   ID_RUN_SCRIPT,
   ID_RUN_CLIP,
   ID_RUN_RECENT,
   // last 2 items in Run Recent submenu
   ID_CLEAR_MISSING_SCRIPTS = ID_RUN_RECENT + MAX_RECENT + 1,
   ID_CLEAR_ALL_SCRIPTS,
   ID_SHOW_SCRIPTS,
   ID_SCRIPT_DIR,
   // wxID_PREFERENCES,
   // wxID_EXIT,
   
   // Edit menu
   // wxID_UNDO,
   // wxID_REDO,
   ID_CUT,
   ID_COPY,
   ID_NO_UNDO,
   ID_CLEAR,
   ID_OUTSIDE,
   ID_PASTE,
   ID_PMODE,
   ID_PLOCATION,
   ID_PASTE_SEL,
   ID_SELALL,
   ID_REMOVE,
   ID_SHRINK,
   ID_RANDOM,
   ID_FLIPTB,
   ID_FLIPLR,
   ID_ROTATEC,
   ID_ROTATEA,
   ID_CMODE,

   // Paste Location submenu
   ID_PL_TL,
   ID_PL_TR,
   ID_PL_BR,
   ID_PL_BL,
   ID_PL_MID,

   // Paste Mode submenu
   ID_PM_COPY,
   ID_PM_OR,
   ID_PM_XOR,

   // Cursor Mode submenu
   ID_DRAW,
   ID_SELECT,
   ID_MOVE,
   ID_ZOOMIN,
   ID_ZOOMOUT,

   // Control menu
   ID_START,
   ID_NEXT,
   ID_STEP,
   ID_RESET,
   ID_SETGEN,
   ID_FASTER,
   ID_SLOWER,
   ID_AUTO,
   ID_HASH,
   ID_HYPER,
   ID_HINFO,
   ID_RULE,
   
   // View menu
   ID_FULL,
   ID_FIT,
   ID_FIT_SEL,
   ID_MIDDLE,
   ID_RESTORE00,
   // wxID_ZOOM_IN,
   // wxID_ZOOM_OUT,
   ID_SET_SCALE,
   ID_TOOL_BAR,
   ID_LAYER_BAR,
   ID_STATUS_BAR,
   ID_EXACT,
   ID_GRID,
   ID_COLORS,
   ID_BUFF,
   ID_INFO,

   // Set Scale submenu
   ID_SCALE_1,
   ID_SCALE_2,
   ID_SCALE_4,
   ID_SCALE_8,
   ID_SCALE_16,
   
   // Help menu
   ID_HELP_INDEX,
   ID_HELP_INTRO,
   ID_HELP_TIPS,
   ID_HELP_SHORTCUTS,
   ID_HELP_PERL,
   ID_HELP_PYTHON,
   ID_HELP_LEXICON,
   ID_HELP_FILE,
   ID_HELP_EDIT,
   ID_HELP_CONTROL,
   ID_HELP_VIEW,
   ID_HELP_LAYER,
   ID_HELP_HELP,
   ID_HELP_REFS,
   ID_HELP_PROBLEMS,
   ID_HELP_CHANGES,
   ID_HELP_CREDITS,
   ID_SHOW_HELP,        // for help button in tool bar

   // Layer menu
   ID_ADD_LAYER,
   ID_CLONE,
   ID_DUPLICATE,
   ID_DEL_LAYER,
   ID_DEL_OTHERS,
   ID_MOVE_LAYER,
   ID_NAME_LAYER,
   ID_SYNC_VIEW,
   ID_SYNC_CURS,
   ID_STACK,
   ID_TILE,
   ID_LAYER0,
   ID_LAYERMAX = ID_LAYER0 + maxlayers - 1
};

// -----------------------------------------------------------------------------

// static routines used by GetPrefs() to get IDs for items in Open/Run Recent submenus;
// can't be MainFrame methods because GetPrefs() is called before creating main window
// and I'd rather not expose the IDs in a header file

int GetID_CLEAR_MISSING_PATTERNS()  { return ID_CLEAR_MISSING_PATTERNS; }
int GetID_CLEAR_ALL_PATTERNS()      { return ID_CLEAR_ALL_PATTERNS; }
int GetID_OPEN_RECENT()             { return ID_OPEN_RECENT; }
int GetID_CLEAR_MISSING_SCRIPTS()   { return ID_CLEAR_MISSING_SCRIPTS; }
int GetID_CLEAR_ALL_SCRIPTS()       { return ID_CLEAR_ALL_SCRIPTS; }
int GetID_RUN_RECENT()              { return ID_RUN_RECENT; }

// static routines used to post commands to the event queue

int GetID_START() { return ID_START; }
int GetID_RESET() { return ID_RESET; }
int GetID_HASH()  { return ID_HASH; }

// -----------------------------------------------------------------------------

// one-shot timer to fix problems in wxMac and wxGTK -- see OnOneTimer;
// must be static because it's used in DnDFile::OnDropFiles
wxTimer* onetimer;

#ifdef __WXMSW__
bool callUnselect = false;    // OnIdle needs to call Unselect?
#endif

// ids for bitmap buttons in tool bar
enum {
   START_TOOL = 0,
   STOP_TOOL,
   HASH_TOOL,
   NEW_TOOL,
   OPEN_TOOL,
   SAVE_TOOL,
   PATTERNS_TOOL,
   SCRIPTS_TOOL,
   DRAW_TOOL,
   SELECT_TOOL,
   MOVE_TOOL,
   ZOOMIN_TOOL,
   ZOOMOUT_TOOL,
   INFO_TOOL,
   HELP_TOOL      // if moved then change NUM_BUTTONS
};

const int NUM_BUTTONS = HELP_TOOL + 1;

#ifdef __WXMSW__
   // bitmaps are loaded via .rc file
#else
   // bitmaps for tool bar buttons
   #include "bitmaps/play.xpm"
   #include "bitmaps/stop.xpm"
   #include "bitmaps/hash.xpm"
   #include "bitmaps/new.xpm"
   #include "bitmaps/open.xpm"
   #include "bitmaps/save.xpm"
   #include "bitmaps/patterns.xpm"
   #include "bitmaps/scripts.xpm"
   #include "bitmaps/draw.xpm"
   #include "bitmaps/select.xpm"
   #include "bitmaps/move.xpm"
   #include "bitmaps/zoomin.xpm"
   #include "bitmaps/zoomout.xpm"
   #include "bitmaps/info.xpm"
   #include "bitmaps/help.xpm"
   // bitmaps for down state of toggle buttons
   #include "bitmaps/hash_down.xpm"
   #include "bitmaps/patterns_down.xpm"
   #include "bitmaps/scripts_down.xpm"
   #include "bitmaps/draw_down.xpm"
   #include "bitmaps/select_down.xpm"
   #include "bitmaps/move_down.xpm"
   #include "bitmaps/zoomin_down.xpm"
   #include "bitmaps/zoomout_down.xpm"
#endif

// -----------------------------------------------------------------------------

// Define our own vertical tool bar to avoid bugs and limitations in wxToolBar:

// derive from wxPanel so we get current theme's background color on Windows
class ToolBar : public wxPanel
{
public:
   ToolBar(wxWindow* parent, wxCoord xorg, wxCoord yorg, int wd, int ht);
   ~ToolBar() {}

   // add a bitmap button to tool bar
   void AddButton(int id, const wxString& tip);

   // add a vertical gap between buttons
   void AddSeparator();
   
   // enable/disable button
   void EnableButton(int id, bool enable);

   // set state of start/stop button
   void SetStartStopButton();
   
   // set state of a toggle button
   void SelectButton(int id, bool select);

   // detect press and release of a bitmap button
   void OnButtonDown(wxMouseEvent& event);
   void OnButtonUp(wxMouseEvent& event);
   void OnKillFocus(wxFocusEvent& event);
   
private:
   // any class wishing to process wxWidgets events must use this macro
   DECLARE_EVENT_TABLE()

   // event handlers
   void OnPaint(wxPaintEvent& event);
   void OnMouseDown(wxMouseEvent& event);
   void OnButton(wxCommandEvent& event);
   
   // bitmaps for normal or down state
   wxBitmap normtool[NUM_BUTTONS];
   wxBitmap downtool[NUM_BUTTONS];

   #ifdef __WXMSW__
      // on Windows we need bitmaps for disabled buttons
      wxBitmap disnormtool[NUM_BUTTONS];
      wxBitmap disdowntool[NUM_BUTTONS];
   #endif
   
   // positioning data used by AddButton and AddSeparator
   int ypos, xpos, smallgap, biggap;
};

BEGIN_EVENT_TABLE(ToolBar, wxPanel)
   EVT_PAINT            (           ToolBar::OnPaint)
   EVT_LEFT_DOWN        (           ToolBar::OnMouseDown)
   EVT_BUTTON           (wxID_ANY,  ToolBar::OnButton)
END_EVENT_TABLE()

ToolBar* toolbarptr = NULL;      // global pointer to tool bar
const int toolbarwd = 32;        // width of (vertical) tool bar

// tool bar buttons (must be global to use Connect/Disconect on Windows)
wxBitmapButton* tbbutt[NUM_BUTTONS];

// -----------------------------------------------------------------------------

ToolBar::ToolBar(wxWindow* parent, wxCoord xorg, wxCoord yorg, int wd, int ht)
   : wxPanel(parent, wxID_ANY, wxPoint(xorg,yorg), wxSize(wd,ht),
             wxNO_FULL_REPAINT_ON_RESIZE)
{
   #ifdef __WXGTK__
      // avoid erasing background on GTK+
      SetBackgroundStyle(wxBG_STYLE_CUSTOM);
   #endif

   // init bitmaps for normal state
   normtool[START_TOOL] =     wxBITMAP(play);
   normtool[STOP_TOOL] =      wxBITMAP(stop);
   normtool[HASH_TOOL] =      wxBITMAP(hash);
   normtool[NEW_TOOL] =       wxBITMAP(new);
   normtool[OPEN_TOOL] =      wxBITMAP(open);
   normtool[SAVE_TOOL] =      wxBITMAP(save);
   normtool[PATTERNS_TOOL] =  wxBITMAP(patterns);
   normtool[SCRIPTS_TOOL] =   wxBITMAP(scripts);
   normtool[DRAW_TOOL] =      wxBITMAP(draw);
   normtool[SELECT_TOOL] =    wxBITMAP(select);
   normtool[MOVE_TOOL] =      wxBITMAP(move);
   normtool[ZOOMIN_TOOL] =    wxBITMAP(zoomin);
   normtool[ZOOMOUT_TOOL] =   wxBITMAP(zoomout);
   normtool[INFO_TOOL] =      wxBITMAP(info);
   normtool[HELP_TOOL] =      wxBITMAP(help);
   
   // toggle buttons also have a down state
   downtool[HASH_TOOL] =      wxBITMAP(hash_down);
   downtool[PATTERNS_TOOL] =  wxBITMAP(patterns_down);
   downtool[SCRIPTS_TOOL] =   wxBITMAP(scripts_down);
   downtool[DRAW_TOOL] =      wxBITMAP(draw_down);
   downtool[SELECT_TOOL] =    wxBITMAP(select_down);
   downtool[MOVE_TOOL] =      wxBITMAP(move_down);
   downtool[ZOOMIN_TOOL] =    wxBITMAP(zoomin_down);
   downtool[ZOOMOUT_TOOL] =   wxBITMAP(zoomout_down);

   #ifdef __WXMSW__
      for (int i = 0; i < NUM_BUTTONS; i++) {
         CreatePaleBitmap(normtool[i], disnormtool[i]);
      }
      CreatePaleBitmap(downtool[HASH_TOOL],       disdowntool[HASH_TOOL]);
      CreatePaleBitmap(downtool[PATTERNS_TOOL],   disdowntool[PATTERNS_TOOL]);
      CreatePaleBitmap(downtool[SCRIPTS_TOOL],    disdowntool[SCRIPTS_TOOL]);
      CreatePaleBitmap(downtool[DRAW_TOOL],       disdowntool[DRAW_TOOL]);
      CreatePaleBitmap(downtool[SELECT_TOOL],     disdowntool[SELECT_TOOL]);
      CreatePaleBitmap(downtool[MOVE_TOOL],       disdowntool[MOVE_TOOL]);
      CreatePaleBitmap(downtool[ZOOMIN_TOOL],     disdowntool[ZOOMIN_TOOL]);
      CreatePaleBitmap(downtool[ZOOMOUT_TOOL],    disdowntool[ZOOMOUT_TOOL]);
   #endif

   // init position variables used by AddButton and AddSeparator
   ypos = 4;
   #ifdef __WXGTK__
      xpos = 3;
      smallgap = 6;
   #else
      xpos = 4;
      smallgap = 4;
   #endif
   biggap = 16;
}

// -----------------------------------------------------------------------------

void ToolBar::OnPaint(wxPaintEvent& WXUNUSED(event))
{
   wxPaintDC dc(this);

   int wd, ht;
   GetClientSize(&wd, &ht);
   if (wd < 1 || ht < 1 || !showtool) return;
      
   wxRect r = wxRect(0, 0, wd, ht);   
   #ifdef __WXMSW__
      dc.Clear();
      // draw gray line at top edge
      dc.SetPen(*wxGREY_PEN);
      dc.DrawLine(0, 0, r.width, 0);
      dc.SetPen(wxNullPen);
   #else
      // draw gray line at right edge
      #ifdef __WXMAC__
         wxBrush brush(wxColor(202,202,202));
         FillRect(dc, r, brush);
         wxPen linepen(wxColor(140,140,140));
         dc.SetPen(linepen);
      #else
         dc.SetPen(*wxLIGHT_GREY_PEN);
      #endif
      dc.DrawLine(r.GetRight(), 0, r.GetRight(), r.height);
      dc.SetPen(wxNullPen);
   #endif
}

// -----------------------------------------------------------------------------

void ToolBar::OnMouseDown(wxMouseEvent& WXUNUSED(event))
{
   // this is NOT called if user clicks a tool bar button;
   // on Windows we need to reset keyboard focus to viewport window
   viewptr->SetFocus();
}

// -----------------------------------------------------------------------------

void ToolBar::OnButton(wxCommandEvent& event)
{
   #ifdef __WXMAC__
      // close any open tool tip window (fixes wxMac bug?)
      wxToolTip::RemoveToolTips();
   #endif

   int id = event.GetId();

   int cmdid;
   switch (id) {
      case START_TOOL:     cmdid = ID_START; break;
      case HASH_TOOL:      cmdid = ID_HASH; break;
      case NEW_TOOL:       cmdid = wxID_NEW; break;
      case OPEN_TOOL:      cmdid = wxID_OPEN; break;
      case SAVE_TOOL:      cmdid = wxID_SAVE; break;
      case PATTERNS_TOOL:  cmdid = ID_SHOW_PATTERNS; break;
      case SCRIPTS_TOOL:   cmdid = ID_SHOW_SCRIPTS; break;
      case DRAW_TOOL:      cmdid = ID_DRAW; break;
      case SELECT_TOOL:    cmdid = ID_SELECT; break;
      case MOVE_TOOL:      cmdid = ID_MOVE; break;
      case ZOOMIN_TOOL:    cmdid = ID_ZOOMIN; break;
      case ZOOMOUT_TOOL:   cmdid = ID_ZOOMOUT; break;
      case INFO_TOOL:      cmdid = ID_INFO; break;
      case HELP_TOOL:      cmdid = ID_SHOW_HELP; break;
      default:             Warning(_("Unexpected button id!")); return;
   }
   
   // call MainFrame::OnMenu after OnButton finishes;
   // this avoids start/stop button problem in GTK app
   wxCommandEvent cmdevt(wxEVT_COMMAND_MENU_SELECTED, cmdid);
   wxPostEvent(mainptr->GetEventHandler(), cmdevt);
}

// -----------------------------------------------------------------------------

void ToolBar::OnKillFocus(wxFocusEvent& event)
{
   int id = event.GetId();
   tbbutt[id]->SetFocus();   // don't let button lose focus
}

// -----------------------------------------------------------------------------

void ToolBar::OnButtonDown(wxMouseEvent& event)
{
   // a tool bar button has been pressed
   int id = event.GetId();
   
   // connect a handler that keeps focus with the pressed button
   tbbutt[id]->Connect(id, wxEVT_KILL_FOCUS,
                       wxFocusEventHandler(ToolBar::OnKillFocus));
        
   event.Skip();
}

// -----------------------------------------------------------------------------

void ToolBar::OnButtonUp(wxMouseEvent& event)
{
   // a tool bar button has been released
   int id = event.GetId();

   wxPoint pt = tbbutt[id]->ScreenToClient( wxGetMousePosition() );

   int wd, ht;
   tbbutt[id]->GetClientSize(&wd, &ht);
   wxRect r(0, 0, wd, ht);

   // diconnect kill-focus handler
   tbbutt[id]->Disconnect(id, wxEVT_KILL_FOCUS,
                          wxFocusEventHandler(ToolBar::OnKillFocus));
   viewptr->SetFocus();

#if wxCHECK_VERSION(2,7,0)
// Inside is deprecated
if ( r.Contains(pt) ) {
#else
if ( r.Inside(pt) ) {
#endif
      // call OnButton
      wxCommandEvent buttevt(wxEVT_COMMAND_BUTTON_CLICKED, id);
      buttevt.SetEventObject(tbbutt[id]);
      tbbutt[id]->ProcessEvent(buttevt);
   }
}

// -----------------------------------------------------------------------------

void ToolBar::AddButton(int id, const wxString& tip)
{
   tbbutt[id] = new wxBitmapButton(this, id, normtool[id], wxPoint(xpos,ypos));
   if (tbbutt[id] == NULL) {
      Fatal(_("Failed to create tool bar button!"));
   } else {
      const int BUTTON_HT = 24;        // nominal height of bitmap buttons
      ypos += BUTTON_HT + smallgap;
      tbbutt[id]->SetToolTip(tip);
      #ifdef __WXMSW__
         // fix problem with tool bar buttons when generating/inscript
         // due to focus being changed to viewptr
         tbbutt[id]->Connect(id, wxEVT_LEFT_DOWN, wxMouseEventHandler(ToolBar::OnButtonDown));
         tbbutt[id]->Connect(id, wxEVT_LEFT_UP, wxMouseEventHandler(ToolBar::OnButtonUp));
      #endif
   }
}

// -----------------------------------------------------------------------------

void ToolBar::AddSeparator()
{
   ypos += biggap - smallgap;
}

// -----------------------------------------------------------------------------

void ToolBar::EnableButton(int id, bool enable)
{
   if (enable == tbbutt[id]->IsEnabled()) return;

   #ifdef __WXMSW__
      if (id == START_TOOL && (inscript || mainptr->generating)) {
         tbbutt[id]->SetBitmapDisabled(disnormtool[STOP_TOOL]);
         
      } else if (id == HASH_TOOL && currlayer->hash) {
         tbbutt[id]->SetBitmapDisabled(disdowntool[id]);
         
      } else if (id == PATTERNS_TOOL && showpatterns) {
         tbbutt[id]->SetBitmapDisabled(disdowntool[id]);
         
      } else if (id == SCRIPTS_TOOL && showscripts) {
         tbbutt[id]->SetBitmapDisabled(disdowntool[id]);
         
      } else if (id == DRAW_TOOL && currlayer->curs == curs_pencil) {
         tbbutt[id]->SetBitmapDisabled(disdowntool[id]);
         
      } else if (id == SELECT_TOOL && currlayer->curs == curs_cross) {
         tbbutt[id]->SetBitmapDisabled(disdowntool[id]);
         
      } else if (id == MOVE_TOOL && currlayer->curs == curs_hand) {
         tbbutt[id]->SetBitmapDisabled(disdowntool[id]);
         
      } else if (id == ZOOMIN_TOOL && currlayer->curs == curs_zoomin) {
         tbbutt[id]->SetBitmapDisabled(disdowntool[id]);
         
      } else if (id == ZOOMOUT_TOOL && currlayer->curs == curs_zoomout) {
         tbbutt[id]->SetBitmapDisabled(disdowntool[id]);
         
      } else {
         tbbutt[id]->SetBitmapDisabled(disnormtool[id]);
      }
   #endif

   tbbutt[id]->Enable(enable);
}

// -----------------------------------------------------------------------------

void ToolBar::SetStartStopButton()
{
   if (inscript || mainptr->generating) {
      // show stop bitmap
      tbbutt[START_TOOL]->SetBitmapLabel(normtool[STOP_TOOL]);
      if (inscript)
         tbbutt[START_TOOL]->SetToolTip(_("Stop script"));
      else
         tbbutt[START_TOOL]->SetToolTip(_("Stop generating"));
   } else {
      // show start bitmap
      tbbutt[START_TOOL]->SetBitmapLabel(normtool[START_TOOL]);
      tbbutt[START_TOOL]->SetToolTip(_("Start generating"));
   }

   #ifdef __WXX11__
      tbbutt[START_TOOL]->ClearBackground();    // fix wxX11 problem
   #endif

   tbbutt[START_TOOL]->Refresh(false);
}

// -----------------------------------------------------------------------------

void ToolBar::SelectButton(int id, bool select)
{
   if (select) {
      tbbutt[id]->SetBitmapLabel(downtool[id]);
   } else {
      tbbutt[id]->SetBitmapLabel(normtool[id]);
   }

   #ifdef __WXX11__
      tbbutt[id]->ClearBackground();    // fix wxX11 problem
   #endif

   tbbutt[id]->Refresh(false);
}

// -----------------------------------------------------------------------------

void MainFrame::CreateToolbar()
{
   int wd, ht;
   GetClientSize(&wd, &ht);

   toolbarptr = new ToolBar(this, 0, 0, toolbarwd, ht);
   if (toolbarptr == NULL) Fatal(_("Failed to create tool bar!"));

   // add buttons to tool bar
   toolbarptr->AddButton(START_TOOL,      _("Start generating"));
   toolbarptr->AddButton(HASH_TOOL,       _("Toggle hashing"));
   toolbarptr->AddSeparator();
   toolbarptr->AddButton(NEW_TOOL,        _("New pattern"));
   toolbarptr->AddButton(OPEN_TOOL,       _("Open pattern"));
   toolbarptr->AddButton(SAVE_TOOL,       _("Save pattern"));
   toolbarptr->AddSeparator();
   toolbarptr->AddButton(PATTERNS_TOOL,   _("Show/hide patterns"));
   toolbarptr->AddButton(SCRIPTS_TOOL,    _("Show/hide scripts"));
   toolbarptr->AddSeparator();
   toolbarptr->AddButton(DRAW_TOOL,       _("Draw"));
   toolbarptr->AddButton(SELECT_TOOL,     _("Select"));
   toolbarptr->AddButton(MOVE_TOOL,       _("Move"));
   toolbarptr->AddButton(ZOOMIN_TOOL,     _("Zoom in"));
   toolbarptr->AddButton(ZOOMOUT_TOOL,    _("Zoom out"));
   toolbarptr->AddSeparator();
   toolbarptr->AddButton(INFO_TOOL,       _("Show pattern information"));
   toolbarptr->AddButton(HELP_TOOL,       _("Show help window"));
      
   toolbarptr->Show(showtool);
}

// -----------------------------------------------------------------------------

void MainFrame::UpdateToolBar(bool active)
{
   // update tool bar buttons according to the current state
   if (toolbarptr && showtool) {
      if (viewptr->waitingforclick) active = false;
      bool busy = generating || inscript;

      // set state of start/stop button
      toolbarptr->SetStartStopButton();

      // set state of toggle buttons
      toolbarptr->SelectButton(HASH_TOOL,       currlayer->hash);
      toolbarptr->SelectButton(PATTERNS_TOOL,   showpatterns);
      toolbarptr->SelectButton(SCRIPTS_TOOL,    showscripts);
      toolbarptr->SelectButton(DRAW_TOOL,       currlayer->curs == curs_pencil);
      toolbarptr->SelectButton(SELECT_TOOL,     currlayer->curs == curs_cross);
      toolbarptr->SelectButton(MOVE_TOOL,       currlayer->curs == curs_hand);
      toolbarptr->SelectButton(ZOOMIN_TOOL,     currlayer->curs == curs_zoomin);
      toolbarptr->SelectButton(ZOOMOUT_TOOL,    currlayer->curs == curs_zoomout);
      
      toolbarptr->EnableButton(START_TOOL,      active);
      toolbarptr->EnableButton(HASH_TOOL,       active && !inscript);   // allow while generating
      toolbarptr->EnableButton(NEW_TOOL,        active && !busy);
      toolbarptr->EnableButton(OPEN_TOOL,       active && !busy);
      toolbarptr->EnableButton(SAVE_TOOL,       active && !busy);
      toolbarptr->EnableButton(PATTERNS_TOOL,   active);
      toolbarptr->EnableButton(SCRIPTS_TOOL,    active);
      toolbarptr->EnableButton(DRAW_TOOL,       active);
      toolbarptr->EnableButton(SELECT_TOOL,     active);
      toolbarptr->EnableButton(MOVE_TOOL,       active);
      toolbarptr->EnableButton(ZOOMIN_TOOL,     active);
      toolbarptr->EnableButton(ZOOMOUT_TOOL,    active);
      toolbarptr->EnableButton(INFO_TOOL,       active && !currlayer->currfile.IsEmpty());
      toolbarptr->EnableButton(HELP_TOOL,       active);
   }
}

// -----------------------------------------------------------------------------

bool MainFrame::ClipboardHasText()
{
   #ifdef __WXX11__
      return wxFileExists(clipfile);
   #else
      bool hastext = false;
      if ( wxTheClipboard->Open() ) {
         hastext = wxTheClipboard->IsSupported( wxDF_TEXT );
         if (!hastext) {
            // we'll try to convert bitmap data to text pattern
            hastext = wxTheClipboard->IsSupported( wxDF_BITMAP );
         }
         wxTheClipboard->Close();
      }
      return hastext;
   #endif
}

// -----------------------------------------------------------------------------

void MainFrame::EnableAllMenus(bool enable)
{
   #ifdef __WXMAC__
      // enable/disable all menus, including Help menu and items in app menu
      if (enable)
         EndAppModalStateForWindow( (OpaqueWindowPtr*)this->MacGetWindowRef() );
      else
         BeginAppModalStateForWindow( (OpaqueWindowPtr*)this->MacGetWindowRef() );
   #else
      wxMenuBar* mbar = GetMenuBar();
      if (mbar) {
         int count = mbar->GetMenuCount();
         int i;
         for (i = 0; i < count; i++) {
            mbar->EnableTop(i, enable);
         }
      }
   #endif
}

// -----------------------------------------------------------------------------

// update menu bar items according to the given state
void MainFrame::UpdateMenuItems(bool active)
{
   wxMenuBar* mbar = GetMenuBar();
   if (mbar) {
      bool textinclip = ClipboardHasText();
      bool selexists = viewptr->SelectionExists();
      bool busy = generating || inscript;

      if (viewptr->waitingforclick) active = false;
      
      mbar->Enable(wxID_NEW,           active && !busy);
      mbar->Enable(wxID_OPEN,          active && !busy);
      mbar->Enable(ID_OPEN_CLIP,       active && !busy && textinclip);
      mbar->Enable(ID_OPEN_RECENT,     active && !busy && numpatterns > 0);
      mbar->Enable(ID_SHOW_PATTERNS,   active);
      mbar->Enable(ID_PATTERN_DIR,     active);
      mbar->Enable(wxID_SAVE,          active && !busy);
      mbar->Enable(ID_SAVE_XRLE,       active);
      mbar->Enable(ID_RUN_SCRIPT,      active && !busy);
      mbar->Enable(ID_RUN_CLIP,        active && !busy && textinclip);
      mbar->Enable(ID_RUN_RECENT,      active && !busy && numscripts > 0);
      mbar->Enable(ID_SHOW_SCRIPTS,    active);
      mbar->Enable(ID_SCRIPT_DIR,      active);
      mbar->Enable(wxID_PREFERENCES,   !busy);

      mbar->Enable(wxID_UNDO,    active && currlayer->undoredo->CanUndo());
      mbar->Enable(wxID_REDO,    active && currlayer->undoredo->CanRedo());
      mbar->Enable(ID_NO_UNDO,   active && !busy);
      mbar->Enable(ID_CUT,       active && !busy && selexists);
      mbar->Enable(ID_COPY,      active && !busy && selexists);
      mbar->Enable(ID_CLEAR,     active && !busy && selexists);
      mbar->Enable(ID_OUTSIDE,   active && !busy && selexists);
      mbar->Enable(ID_PASTE,     active && !busy && textinclip);
      mbar->Enable(ID_PASTE_SEL, active && !busy && textinclip && selexists);
      mbar->Enable(ID_PLOCATION, active);
      mbar->Enable(ID_PMODE,     active);
      mbar->Enable(ID_SELALL,    active);
      mbar->Enable(ID_REMOVE,    active && selexists);
      mbar->Enable(ID_SHRINK,    active && selexists);
      mbar->Enable(ID_RANDOM,    active && !busy && selexists);
      mbar->Enable(ID_FLIPTB,    active && !busy && selexists);
      mbar->Enable(ID_FLIPLR,    active && !busy && selexists);
      mbar->Enable(ID_ROTATEC,   active && !busy && selexists);
      mbar->Enable(ID_ROTATEA,   active && !busy && selexists);
      mbar->Enable(ID_CMODE,     active);

      if (inscript) {
         // don't use DO_STARTSTOP key to abort a running script
         #ifdef __WXMAC__
            // on Mac we need to clear the accelerator first because "\tEscape" doesn't really
            // change the accelerator (it just looks like it does!) -- this is because escape
            // (key code 27) is used by SetItemCmd to indicate the item has a submenu;
            // see UMASetMenuItemShortcut in wx/src/mac/carbon/uma.cpp
            mbar->SetLabel(ID_START, _("xxx"));
         #endif
         mbar->SetLabel(ID_START, _("Stop Script\tEscape"));
      } else if (generating) {
         mbar->SetLabel(ID_START, _("Stop Generating") + GetAccelerator(DO_STARTSTOP));
      } else {
         mbar->SetLabel(ID_START, _("Start Generating") + GetAccelerator(DO_STARTSTOP));
      }

      mbar->Enable(ID_START,     active);
      mbar->Enable(ID_NEXT,      active && !busy);
      mbar->Enable(ID_STEP,      active && !busy);
      mbar->Enable(ID_RESET,     active && !inscript &&     // allow reset while generating
                                 (generating || currlayer->algo->getGeneration() > currlayer->startgen));
      mbar->Enable(ID_SETGEN,    active && !busy);
      mbar->Enable(ID_FASTER,    active);
      mbar->Enable(ID_SLOWER,    active && currlayer->warp > minwarp);
      mbar->Enable(ID_AUTO,      active);
      mbar->Enable(ID_HASH,      active && !inscript);      // allow toggling while generating
      mbar->Enable(ID_HYPER,     active);
      mbar->Enable(ID_HINFO,     active);
      mbar->Enable(ID_RULE,      active && !busy);

      mbar->Enable(ID_FULL,      active);
      mbar->Enable(ID_FIT,       active);
      mbar->Enable(ID_FIT_SEL,   active && selexists);
      mbar->Enable(ID_MIDDLE,    active);
      mbar->Enable(ID_RESTORE00, active && (currlayer->originx != bigint::zero ||
                                            currlayer->originy != bigint::zero));
      mbar->Enable(wxID_ZOOM_IN, active && viewptr->GetMag() < MAX_MAG);
      mbar->Enable(wxID_ZOOM_OUT, active);
      mbar->Enable(ID_SET_SCALE, active);
      mbar->Enable(ID_TOOL_BAR,  active);
      mbar->Enable(ID_LAYER_BAR, active);
      mbar->Enable(ID_STATUS_BAR,active);
      mbar->Enable(ID_EXACT,     active);
      mbar->Enable(ID_GRID,      active);
      mbar->Enable(ID_COLORS,    active);
      #if defined(__WXMAC__) || defined(__WXGTK__)
         // windows on Mac OS X and GTK+ 2.0 are automatically buffered
         mbar->Enable(ID_BUFF,   false);
         mbar->Check(ID_BUFF,    true);
      #else
         mbar->Enable(ID_BUFF,   active);
         mbar->Check(ID_BUFF,    buffered);
      #endif
      mbar->Enable(ID_INFO,      !currlayer->currfile.IsEmpty());

      mbar->Enable(ID_ADD_LAYER,    active && !busy && numlayers < maxlayers);
      mbar->Enable(ID_CLONE,        active && !busy && numlayers < maxlayers);
      mbar->Enable(ID_DUPLICATE,    active && !busy && numlayers < maxlayers);
      mbar->Enable(ID_DEL_LAYER,    active && !busy && numlayers > 1);
      mbar->Enable(ID_DEL_OTHERS,   active && !inscript && numlayers > 1);
      mbar->Enable(ID_MOVE_LAYER,   active && !busy && numlayers > 1);
      mbar->Enable(ID_NAME_LAYER,   active && !busy);
      mbar->Enable(ID_SYNC_VIEW,    active);
      mbar->Enable(ID_SYNC_CURS,    active);
      mbar->Enable(ID_STACK,        active);
      mbar->Enable(ID_TILE,         active);
      for (int i = 0; i < numlayers; i++)
         mbar->Enable(ID_LAYER0 + i, active && CanSwitchLayer(i));

      // tick/untick menu items created using AppendCheckItem
      mbar->Check(ID_SAVE_XRLE,     savexrle);
      mbar->Check(ID_SHOW_PATTERNS, showpatterns);
      mbar->Check(ID_SHOW_SCRIPTS,  showscripts);
      mbar->Check(ID_NO_UNDO,       !allowundo);
      mbar->Check(ID_AUTO,       currlayer->autofit);
      mbar->Check(ID_HASH,       currlayer->hash);
      mbar->Check(ID_HYPER,      currlayer->hyperspeed);
      mbar->Check(ID_HINFO,      currlayer->showhashinfo);
      mbar->Check(ID_TOOL_BAR,   showtool);
      mbar->Check(ID_LAYER_BAR,  showlayer);
      mbar->Check(ID_STATUS_BAR, showstatus);
      mbar->Check(ID_EXACT,      showexact);
      mbar->Check(ID_GRID,       showgridlines);
      mbar->Check(ID_COLORS,     swapcolors);
      mbar->Check(ID_PL_TL,      plocation == TopLeft);
      mbar->Check(ID_PL_TR,      plocation == TopRight);
      mbar->Check(ID_PL_BR,      plocation == BottomRight);
      mbar->Check(ID_PL_BL,      plocation == BottomLeft);
      mbar->Check(ID_PL_MID,     plocation == Middle);
      mbar->Check(ID_PM_COPY,    pmode == Copy);
      mbar->Check(ID_PM_OR,      pmode == Or);
      mbar->Check(ID_PM_XOR,     pmode == Xor);
      mbar->Check(ID_DRAW,       currlayer->curs == curs_pencil);
      mbar->Check(ID_SELECT,     currlayer->curs == curs_cross);
      mbar->Check(ID_MOVE,       currlayer->curs == curs_hand);
      mbar->Check(ID_ZOOMIN,     currlayer->curs == curs_zoomin);
      mbar->Check(ID_ZOOMOUT,    currlayer->curs == curs_zoomout);
      mbar->Check(ID_SCALE_1,    viewptr->GetMag() == 0);
      mbar->Check(ID_SCALE_2,    viewptr->GetMag() == 1);
      mbar->Check(ID_SCALE_4,    viewptr->GetMag() == 2);
      mbar->Check(ID_SCALE_8,    viewptr->GetMag() == 3);
      mbar->Check(ID_SCALE_16,   viewptr->GetMag() == 4);
      mbar->Check(ID_SYNC_VIEW,  syncviews);
      mbar->Check(ID_SYNC_CURS,  synccursors);
      mbar->Check(ID_STACK,      stacklayers);
      mbar->Check(ID_TILE,       tilelayers);
      for (int i = 0; i < numlayers; i++)
         mbar->Check(ID_LAYER0 + i, currindex == i);
   }
}

// -----------------------------------------------------------------------------

void MainFrame::UpdateUserInterface(bool active)
{
   UpdateToolBar(active);
   UpdateLayerBar(active);
   UpdateMenuItems(active);
   viewptr->CheckCursor(active);
   statusptr->CheckMouseLocation(active);

   #ifdef __WXMSW__
      // ensure viewport window has keyboard focus if main window is active
      if (active) viewptr->SetFocus();
   #endif
}

// -----------------------------------------------------------------------------

// update everything in main window, and menu bar and cursor
void MainFrame::UpdateEverything()
{
   if (IsIconized()) {
      // main window has been minimized, so only update menu bar items
      UpdateMenuItems(false);
      return;
   }

   // update tool bar, layer bar, menu bar and cursor
   UpdateUserInterface(IsActive());

   if (inscript) {
      // make sure scroll bars are accurate while running script
      bigview->UpdateScrollBars();
      return;
   }

   int wd, ht;
   GetClientSize(&wd, &ht);      // includes status bar and viewport

   if (wd > 0 && ht > statusptr->statusht) {
      UpdateView();
      bigview->UpdateScrollBars();
   }
   
   if (wd > 0 && ht > 0 && showstatus) {
      statusptr->Refresh(false);
      statusptr->Update();
   }
}

// -----------------------------------------------------------------------------

// only update viewport and status bar
void MainFrame::UpdatePatternAndStatus()
{
   if (inscript || currlayer->undoredo->doingscriptchanges) return;

   if (!IsIconized()) {
      UpdateView();
      if (showstatus) {
         statusptr->CheckMouseLocation(IsActive());
         statusptr->Refresh(false);
         statusptr->Update();
      }
   }
}

// -----------------------------------------------------------------------------

// only update status bar
void MainFrame::UpdateStatus()
{
   if (!IsIconized()) {
      if (showstatus) {
         statusptr->CheckMouseLocation(IsActive());
         statusptr->Refresh(false);
         statusptr->Update();
      }
   }
}

// -----------------------------------------------------------------------------

void MainFrame::SimplifyTree(wxString& dir, wxTreeCtrl* treectrl, wxTreeItemId root)
{
   // delete old tree (except root)
   treectrl->DeleteChildren(root);

   // append dir as only child
   wxDirItemData* diritem = new wxDirItemData(dir, dir, true);
   wxTreeItemId id;
   id = treectrl->AppendItem(root, dir.AfterLast(wxFILE_SEP_PATH), 0, 0, diritem);
   if ( diritem->HasFiles() || diritem->HasSubDirs() ) {
      treectrl->SetItemHasChildren(id);
      treectrl->Expand(id);
      
      // nicer to expand Perl & Python subdirs inside Scripts
      if ( dir == gollydir + _("Scripts") ) {
         wxTreeItemId child;
         wxTreeItemIdValue cookie;
         child = treectrl->GetFirstChild(id, cookie);
         while ( child.IsOk() ) {
            wxString name = treectrl->GetItemText(child);
            if ( name == _("Perl") || name == _("Python") ) {
               treectrl->Expand(child);
            }
            child = treectrl->GetNextChild(id, cookie);
         }
      }
      
      #ifndef __WXMSW__
         // causes crash on Windows
         treectrl->ScrollTo(root);
      #endif
   }
}

// -----------------------------------------------------------------------------

void MainFrame::DeselectTree(wxTreeCtrl* treectrl, wxTreeItemId root)
{
   // recursively traverse tree and reset each file item background to white
   wxTreeItemIdValue cookie;
   wxTreeItemId id = treectrl->GetFirstChild(root, cookie);
   while ( id.IsOk() ) {
      if ( treectrl->ItemHasChildren(id) ) {
         DeselectTree(treectrl, id);
      } else {
         wxColor currcolor = treectrl->GetItemBackgroundColour(id);
         if ( currcolor != *wxWHITE ) {
            treectrl->SetItemBackgroundColour(id, *wxWHITE);
         }
      }
      id = treectrl->GetNextChild(root, cookie);
   }
}

// -----------------------------------------------------------------------------

// Define a window for right pane of split window:

class RightWindow : public wxWindow
{
public:
   RightWindow(wxWindow* parent, wxCoord xorg, wxCoord yorg, int wd, int ht)
      : wxWindow(parent, wxID_ANY, wxPoint(xorg,yorg), wxSize(wd,ht),
                 wxNO_BORDER |
                 // need this to avoid layer bar buttons flashing on Windows
                 wxNO_FULL_REPAINT_ON_RESIZE)
   {
      // avoid erasing background on GTK+
      SetBackgroundStyle(wxBG_STYLE_CUSTOM);
   }
   ~RightWindow() {}

   // event handlers
   void OnSize(wxSizeEvent& event);
   void OnEraseBackground(wxEraseEvent& event);

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(RightWindow, wxWindow)
   EVT_SIZE             (RightWindow::OnSize)
   EVT_ERASE_BACKGROUND (RightWindow::OnEraseBackground)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------

void RightWindow::OnSize(wxSizeEvent& event)
{
   int wd, ht;
   GetClientSize(&wd, &ht);
   if (wd > 0 && ht > 0 && bigview) {
      // resize layer bar and main viewport window
      ResizeLayerBar(wd);
      bigview->SetSize(0, showlayer ? layerbarht : 0,
                       wd, showlayer ? ht - layerbarht : ht);
   }
   event.Skip();
}

// -----------------------------------------------------------------------------

void RightWindow::OnEraseBackground(wxEraseEvent& WXUNUSED(event))
{
   // do nothing because layer bar and viewport cover all of right pane
}

// -----------------------------------------------------------------------------

RightWindow* rightpane;

wxWindow* MainFrame::RightPane()
{
   return rightpane;
}

// -----------------------------------------------------------------------------

void MainFrame::ResizeSplitWindow(int wd, int ht)
{
   splitwin->SetSize(showtool ? toolbarwd : 0,
                     statusptr->statusht,
                     showtool ? wd - toolbarwd : wd,
                     ht > statusptr->statusht ? ht - statusptr->statusht : 0);

   // wxSplitterWindow automatically resizes left and right panes;
   // note that RightWindow::OnSize has now been called
}

// -----------------------------------------------------------------------------

void MainFrame::ResizeStatusBar(int wd, int ht)
{
   wxUnusedVar(ht);
   // assume showstatus is true
   statusptr->statusht = showexact ? STATUS_EXHT : STATUS_HT;
   statusptr->SetSize(showtool ? toolbarwd : 0, 0,
                      showtool ? wd - toolbarwd : wd, statusptr->statusht);
}

// -----------------------------------------------------------------------------

void MainFrame::ToggleStatusBar()
{
   int wd, ht;
   GetClientSize(&wd, &ht);
   showstatus = !showstatus;
   if (showstatus) {
      ResizeStatusBar(wd, ht);
   } else {
      statusptr->statusht = 0;
      statusptr->SetSize(0, 0, 0, 0);
      #ifdef __WXX11__
         // move so we don't see small portion
         statusptr->Move(-100, -100);
      #endif
   }
   ResizeSplitWindow(wd, ht);
   UpdateEverything();
}

// -----------------------------------------------------------------------------

void MainFrame::ToggleExactNumbers()
{
   int wd, ht;
   GetClientSize(&wd, &ht);
   showexact = !showexact;
   if (showstatus) {
      ResizeStatusBar(wd, ht);
      ResizeSplitWindow(wd, ht);
      UpdateEverything();
   } else {
      // show the status bar using new size
      ToggleStatusBar();
   }
}

// -----------------------------------------------------------------------------

void MainFrame::ToggleToolBar()
{
   showtool = !showtool;
   int wd, ht;
   GetClientSize(&wd, &ht);
   if (showstatus) {
      ResizeStatusBar(wd, ht);
   }
   if (showtool) {
      // resize tool bar in case window was made larger while tool bar hidden
      toolbarptr->SetSize(0, 0, toolbarwd, ht);
   }
   ResizeSplitWindow(wd, ht);
   toolbarptr->Show(showtool);
}

// -----------------------------------------------------------------------------

void MainFrame::ToggleFullScreen()
{
   #ifdef __WXX11__
      // ShowFullScreen(true) does nothing
      statusptr->ErrorMessage(_("Sorry, full screen mode is not implemented for X11."));
   #else
      static bool restorestatusbar; // restore status bar at end of full screen mode?
      static bool restorelayerbar;  // restore layer bar?
      static bool restoretoolbar;   // restore tool bar?
      static bool restorepattdir;   // restore pattern directory?
      static bool restorescrdir;    // restore script directory?

      if (!fullscreen) {
         // save current location and size for use in SavePrefs
         wxRect r = GetRect();
         mainx = r.x;
         mainy = r.y;
         mainwd = r.width;
         mainht = r.height;
      }

      fullscreen = !fullscreen;
      ShowFullScreen(fullscreen,
         wxFULLSCREEN_NOMENUBAR | wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);

      if (fullscreen) {
         // hide scroll bars
         bigview->SetScrollbar(wxHORIZONTAL, 0, 0, 0, true);
         bigview->SetScrollbar(wxVERTICAL, 0, 0, 0, true);
         
         // hide status bar if necessary
         restorestatusbar = showstatus;
         if (restorestatusbar) {
            showstatus = false;
            statusptr->statusht = 0;
            statusptr->SetSize(0, 0, 0, 0);
         }
         
         // hide layer bar if necessary
         restorelayerbar = showlayer;
         if (restorelayerbar) {
            ToggleLayerBar();
         }
         
         // hide tool bar if necessary
         restoretoolbar = showtool;
         if (restoretoolbar) {
            ToggleToolBar();
         }
         
         // hide pattern/script directory if necessary
         restorepattdir = showpatterns;
         restorescrdir = showscripts;
         if (restorepattdir) {
            dirwinwd = splitwin->GetSashPosition();
            splitwin->Unsplit(patternctrl);
            showpatterns = false;
         } else if (restorescrdir) {
            dirwinwd = splitwin->GetSashPosition();
            splitwin->Unsplit(scriptctrl);
            showscripts = false;
         }

      } else {
         // first show tool bar if necessary
         if (restoretoolbar && !showtool) {
            ToggleToolBar();
            if (showstatus) {
               // reduce width of status bar below
               restorestatusbar = true;
            }
         }
         
         // show status bar if necessary;
         // note that even if it's visible we may have to resize width
         if (restorestatusbar) {
            showstatus = true;
            int wd, ht;
            GetClientSize(&wd, &ht);
            ResizeStatusBar(wd, ht);
         }

         // show layer bar if necessary
         if (restorelayerbar && !showlayer) {
            ToggleLayerBar();
         }

         // restore pattern/script directory if necessary
         if ( restorepattdir && !splitwin->IsSplit() ) {
            splitwin->SplitVertically(patternctrl, RightPane(), dirwinwd);
            showpatterns = true;
         } else if ( restorescrdir && !splitwin->IsSplit() ) {
            splitwin->SplitVertically(scriptctrl, RightPane(), dirwinwd);
            showscripts = true;
         }
      }

      if (!fullscreen) {
         // restore scroll bars BEFORE setting viewport size
         bigview->UpdateScrollBars();
      }
      
      // adjust size of viewport (and pattern/script directory if visible)
      int wd, ht;
      GetClientSize(&wd, &ht);
      ResizeSplitWindow(wd, ht);
      UpdateEverything();
   #endif
}

// -----------------------------------------------------------------------------

void MainFrame::ToggleAllowUndo()
{
   allowundo = !allowundo;
   if (allowundo) {
      if (currlayer->algo->getGeneration() > currlayer->startgen) {
         // undo list is empty but user can Reset, so add a generating change
         // to undo list so user can Undo or Reset (and Redo if they wish)
         currlayer->undoredo->AddGenChange();
      }
   } else {
      currlayer->undoredo->ClearUndoRedo();
      // don't clear undo/redo history for other layers here; only do it
      // if allowundo is false when user switches to another layer
   }
}

// -----------------------------------------------------------------------------

void MainFrame::ShowPatternInfo()
{
   if (viewptr->waitingforclick || currlayer->currfile.IsEmpty()) return;
   ShowInfo(currlayer->currfile);
}

// -----------------------------------------------------------------------------

// event table and handlers for main window:

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
   EVT_MENU                (wxID_ANY,        MainFrame::OnMenu)
   EVT_SET_FOCUS           (                 MainFrame::OnSetFocus)
   EVT_ACTIVATE            (                 MainFrame::OnActivate)
   EVT_SIZE                (                 MainFrame::OnSize)
   EVT_IDLE                (                 MainFrame::OnIdle)
#ifdef __WXMAC__
   EVT_TREE_ITEM_EXPANDED  (wxID_TREECTRL,   MainFrame::OnDirTreeExpand)
   // wxMac bug??? EVT_TREE_ITEM_COLLAPSED doesn't get called
   EVT_TREE_ITEM_COLLAPSING (wxID_TREECTRL,  MainFrame::OnDirTreeCollapse)
#endif
   EVT_TREE_SEL_CHANGED    (wxID_TREECTRL,   MainFrame::OnDirTreeSelection)
   EVT_SPLITTER_DCLICK     (wxID_ANY,        MainFrame::OnSashDblClick)
   EVT_TIMER               (ID_ONE_TIMER,    MainFrame::OnOneTimer)
   EVT_CLOSE               (                 MainFrame::OnClose)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------

void MainFrame::OnMenu(wxCommandEvent& event)
{
   showbanner = false;
   statusptr->ClearMessage();
   int id = event.GetId();
   switch (id) {
      // File menu
      case wxID_NEW:          NewPattern(); break;
      case wxID_OPEN:         OpenPattern(); break;
      case ID_OPEN_CLIP:      OpenClipboard(); break;
      case ID_CLEAR_MISSING_PATTERNS:  ClearMissingPatterns(); break;
      case ID_CLEAR_ALL_PATTERNS:      ClearAllPatterns(); break;
      case ID_SHOW_PATTERNS:  ToggleShowPatterns(); break;
      case ID_PATTERN_DIR:    ChangePatternDir(); break;
      case wxID_SAVE:         SavePattern(); break;
      case ID_SAVE_XRLE:      savexrle = !savexrle; break;
      case ID_RUN_SCRIPT:     OpenScript(); break;
      case ID_RUN_CLIP:       RunClipboard(); break;
      case ID_CLEAR_MISSING_SCRIPTS:   ClearMissingScripts(); break;
      case ID_CLEAR_ALL_SCRIPTS:       ClearAllScripts(); break;
      case ID_SHOW_SCRIPTS:   ToggleShowScripts(); break;
      case ID_SCRIPT_DIR:     ChangeScriptDir(); break;
      case wxID_PREFERENCES:  ShowPrefsDialog(); break;
      case wxID_EXIT:         QuitApp(); break;
      // Edit menu
      case wxID_UNDO:         currlayer->undoredo->UndoChange(); break;
      case wxID_REDO:         currlayer->undoredo->RedoChange(); break;
      case ID_NO_UNDO:        ToggleAllowUndo(); break;
      case ID_CUT:            viewptr->CutSelection(); break;
      case ID_COPY:           viewptr->CopySelection(); break;
      case ID_CLEAR:          viewptr->ClearSelection(); break;
      case ID_OUTSIDE:        viewptr->ClearOutsideSelection(); break;
      case ID_PASTE:          viewptr->PasteClipboard(false); break;
      case ID_PASTE_SEL:      viewptr->PasteClipboard(true); break;
      case ID_PL_TL:          SetPasteLocation("TopLeft"); break;
      case ID_PL_TR:          SetPasteLocation("TopRight"); break;
      case ID_PL_BR:          SetPasteLocation("BottomRight"); break;
      case ID_PL_BL:          SetPasteLocation("BottomLeft"); break;
      case ID_PL_MID:         SetPasteLocation("Middle"); break;
      case ID_PM_COPY:        SetPasteMode("Copy"); break;
      case ID_PM_OR:          SetPasteMode("Or"); break;
      case ID_PM_XOR:         SetPasteMode("Xor"); break;
      case ID_SELALL:         viewptr->SelectAll(); break;
      case ID_REMOVE:         viewptr->RemoveSelection(); break;
      case ID_SHRINK:         viewptr->ShrinkSelection(false); break;
      case ID_RANDOM:         viewptr->RandomFill(); break;
      case ID_FLIPTB:         viewptr->FlipSelection(true); break;
      case ID_FLIPLR:         viewptr->FlipSelection(false); break;
      case ID_ROTATEC:        viewptr->RotateSelection(true); break;
      case ID_ROTATEA:        viewptr->RotateSelection(false); break;
      case ID_DRAW:           viewptr->SetCursorMode(curs_pencil); break;
      case ID_SELECT:         viewptr->SetCursorMode(curs_cross); break;
      case ID_MOVE:           viewptr->SetCursorMode(curs_hand); break;
      case ID_ZOOMIN:         viewptr->SetCursorMode(curs_zoomin); break;
      case ID_ZOOMOUT:        viewptr->SetCursorMode(curs_zoomout); break;
      // Control menu
      case ID_START:          if (inscript || generating) {
                                 Stop();
                              } else {
                                 GeneratePattern();
                              }
                              break;
      case ID_NEXT:           NextGeneration(false); break;
      case ID_STEP:           NextGeneration(true); break;
      case ID_RESET:          ResetPattern(); break;
      case ID_SETGEN:         SetGeneration(); break;
      case ID_FASTER:         GoFaster(); break;
      case ID_SLOWER:         GoSlower(); break;
      case ID_AUTO:           ToggleAutoFit(); break;
      case ID_HASH:           ToggleHashing(); break;
      case ID_HYPER:          ToggleHyperspeed(); break;
      case ID_HINFO:          ToggleHashInfo(); break;
      case ID_RULE:           ShowRuleDialog(); break;
      // View menu
      case ID_FULL:           ToggleFullScreen(); break;
      case ID_TOOL_BAR:       ToggleToolBar(); break;
      case ID_LAYER_BAR:      ToggleLayerBar(); break;
      case ID_STATUS_BAR:     ToggleStatusBar(); break;
      case ID_EXACT:          ToggleExactNumbers(); break;
      case ID_INFO:           ShowPatternInfo(); break;
      case ID_FIT:            viewptr->FitPattern(); break;
      case ID_FIT_SEL:        viewptr->FitSelection(); break;
      case ID_MIDDLE:         viewptr->ViewOrigin(); break;
      case ID_RESTORE00:      viewptr->RestoreOrigin(); break;
      case wxID_ZOOM_IN:      viewptr->ZoomIn(); break;
      case wxID_ZOOM_OUT:     viewptr->ZoomOut(); break;
      case ID_SCALE_1:        viewptr->SetPixelsPerCell(1); break;
      case ID_SCALE_2:        viewptr->SetPixelsPerCell(2); break;
      case ID_SCALE_4:        viewptr->SetPixelsPerCell(4); break;
      case ID_SCALE_8:        viewptr->SetPixelsPerCell(8); break;
      case ID_SCALE_16:       viewptr->SetPixelsPerCell(16); break;
      case ID_GRID:           viewptr->ToggleGridLines(); break;
      case ID_COLORS:         viewptr->ToggleCellColors(); break;
      case ID_BUFF:           viewptr->ToggleBuffering(); break;
      // Layer menu
      case ID_ADD_LAYER:      AddLayer(); break;
      case ID_CLONE:          CloneLayer(); break;
      case ID_DUPLICATE:      DuplicateLayer(); break;
      case ID_DEL_LAYER:      DeleteLayer(); break;
      case ID_DEL_OTHERS:     DeleteOtherLayers(); break;
      case ID_MOVE_LAYER:     MoveLayerDialog(); break;
      case ID_NAME_LAYER:     NameLayerDialog(); break;
      case ID_SYNC_VIEW:      ToggleSyncViews(); break;
      case ID_SYNC_CURS:      ToggleSyncCursors(); break;
      case ID_STACK:          ToggleStackLayers(); break;
      case ID_TILE:           ToggleTileLayers(); break;
      // Help menu
      case ID_HELP_INDEX:     ShowHelp(_("Help/index.html")); break;
      case ID_HELP_INTRO:     ShowHelp(_("Help/intro.html")); break;
      case ID_HELP_TIPS:      ShowHelp(_("Help/tips.html")); break;
      case ID_HELP_SHORTCUTS: ShowHelp(_("Help/shortcuts.html")); break;
      case ID_HELP_PERL:      ShowHelp(_("Help/perl.html")); break;
      case ID_HELP_PYTHON:    ShowHelp(_("Help/python.html")); break;
      case ID_HELP_LEXICON:   ShowHelp(_("Help/Lexicon/lex.htm")); break;
      case ID_HELP_FILE:      ShowHelp(_("Help/file.html")); break;
      case ID_HELP_EDIT:      ShowHelp(_("Help/edit.html")); break;
      case ID_HELP_CONTROL:   ShowHelp(_("Help/control.html")); break;
      case ID_HELP_VIEW:      ShowHelp(_("Help/view.html")); break;
      case ID_HELP_LAYER:     ShowHelp(_("Help/layer.html")); break;
      case ID_HELP_HELP:      ShowHelp(_("Help/help.html")); break;
      case ID_HELP_REFS:      ShowHelp(_("Help/refs.html")); break;
      case ID_HELP_PROBLEMS:  ShowHelp(_("Help/problems.html")); break;
      case ID_HELP_CHANGES:   ShowHelp(_("Help/changes.html")); break;
      case ID_HELP_CREDITS:   ShowHelp(_("Help/credits.html")); break;
      case ID_SHOW_HELP:      ShowHelp(wxEmptyString); break;
      case wxID_ABOUT:        ShowAboutBox(); break;
      default:
         if ( id > ID_OPEN_RECENT && id <= ID_OPEN_RECENT + numpatterns ) {
            OpenRecentPattern(id);
         } else if ( id > ID_RUN_RECENT && id <= ID_RUN_RECENT + numscripts ) {
            OpenRecentScript(id);
         } else if ( id >= ID_LAYER0 && id <= ID_LAYERMAX ) {
            SetLayer(id - ID_LAYER0);
            if (inscript) {
               // update window title, viewport and status bar
               inscript = false;
               SetWindowTitle(wxEmptyString);
               UpdatePatternAndStatus();
               inscript = true;
            }
         }
   }
   UpdateUserInterface(IsActive());
}

// -----------------------------------------------------------------------------

void MainFrame::OnSetFocus(wxFocusEvent& WXUNUSED(event))
{
   // this is never called in Mac app, presumably because it doesn't
   // make any sense for a wxFrame to get the keyboard focus

   #ifdef __WXMSW__
      // fix wxMSW problem: don't let main window get focus after being minimized
      if (viewptr) viewptr->SetFocus();
   #endif

   #ifdef __WXX11__
      // make sure viewport keeps keyboard focus whenever main window is active
      if (viewptr && IsActive()) viewptr->SetFocus();
      // fix problems after modal dialog or help window is closed
      UpdateUserInterface(IsActive());
   #endif
}

// -----------------------------------------------------------------------------

void MainFrame::OnActivate(wxActivateEvent& event)
{
   // this is never called in X11 app;
   // note that IsActive() doesn't always match event.GetActive()

   #ifdef __WXMAC__
      if (!event.GetActive()) wxSetCursor(*wxSTANDARD_CURSOR);
      // to avoid disabled menu items after a modal dialog closes
      // don't call UpdateMenuItems on deactivation
      if (event.GetActive()) UpdateUserInterface(true);
   #else
      UpdateUserInterface(event.GetActive());
   #endif
   
   #ifdef __WXGTK__
      if (event.GetActive()) onetimer->Start(20, wxTIMER_ONE_SHOT);
      // OnOneTimer will be called after delay of 0.02 secs
   #endif
   
   event.Skip();
}

// -----------------------------------------------------------------------------

void MainFrame::OnSize(wxSizeEvent& event)
{
   #ifdef __WXMSW__
      // save current location and size for use in SavePrefs if app
      // is closed when window is minimized
      wxRect r = GetRect();
      mainx = r.x;
      mainy = r.y;
      mainwd = r.width;
      mainht = r.height;
   #endif

   int wd, ht;
   GetClientSize(&wd, &ht);
   if (wd > 0 && ht > 0) {
      // note that toolbarptr/statusptr/viewptr might be NULL if OnSize is
      // called from MainFrame::MainFrame (true if X11)
      if (toolbarptr && showtool) {
         // adjust size of tool bar
         toolbarptr->SetSize(0, 0, toolbarwd, ht);
      }
      if (statusptr && showstatus) {
         // adjust size of status bar
         ResizeStatusBar(wd, ht);
      }
      if (viewptr && statusptr && ht > statusptr->statusht) {
         // adjust size of viewport (and pattern/script directory if visible)
         ResizeSplitWindow(wd, ht);
      }
   }
   
   #if defined(__WXX11__) || defined(__WXGTK__)
      // need to do default processing for menu bar and tool bar
      event.Skip();
   #else
      wxUnusedVar(event);
   #endif
}

// -----------------------------------------------------------------------------

#if defined(__WXX11__) || defined(__WXGTK__)
   // handle recursive OnIdle call (probably via Yield call in checkevents)
   bool inidle = false;
#endif

void MainFrame::OnIdle(wxIdleEvent& WXUNUSED(event))
{
   #if defined(__WXX11__) || defined(__WXGTK__)
      if (inidle) return;
   #endif

   // process any pending script/pattern files passed via command line
   if ( pendingfiles.GetCount() > 0 ) {
      #if defined(__WXX11__) || defined(__WXGTK__)
         inidle = true;
      #endif
      for ( size_t n = 0; n < pendingfiles.GetCount(); n++ ) {
         OpenFile(pendingfiles[n]);
      }
      #if defined(__WXX11__) || defined(__WXGTK__)
         inidle = false;
      #endif
      pendingfiles.Clear();
   }

   #ifdef __WXX11__
      // don't change focus here because it prevents menus staying open
      return;
   #endif
   
   #ifdef __WXMSW__
      if ( callUnselect ) {
         // deselect file/folder so user can click the same item
         if (showpatterns) patternctrl->GetTreeCtrl()->Unselect();
         if (showscripts) scriptctrl->GetTreeCtrl()->Unselect();
         callUnselect = false;
         
         // calling SetFocus once doesn't stuff up layer bar buttons
         if ( IsActive() && viewptr ) viewptr->SetFocus();
      }
   #else
      // ensure viewport window has keyboard focus if main window is active;
      // note that we can't do this on Windows because it stuffs up clicks
      // in layer bar buttons
      if ( IsActive() && viewptr ) viewptr->SetFocus();
   #endif
}

// -----------------------------------------------------------------------------

void MainFrame::OnDirTreeExpand(wxTreeEvent& WXUNUSED(event))
{
   if ((generating || inscript) && (showpatterns || showscripts)) {
      // send idle event so directory tree gets updated
      wxIdleEvent idleevent;
      wxGetApp().SendIdleEvents(this, idleevent);
   }
}

// -----------------------------------------------------------------------------

void MainFrame::OnDirTreeCollapse(wxTreeEvent& WXUNUSED(event))
{
   if ((generating || inscript) && (showpatterns || showscripts)) {
      // send idle event so directory tree gets updated
      wxIdleEvent idleevent;
      wxGetApp().SendIdleEvents(this, idleevent);
   }
}

// -----------------------------------------------------------------------------

void MainFrame::OnDirTreeSelection(wxTreeEvent& event)
{
   // note that viewptr will be NULL if called from MainFrame::MainFrame
   if ( viewptr ) {
      wxTreeItemId id = event.GetItem();
      if ( !id.IsOk() ) return;

      wxGenericDirCtrl* dirctrl = NULL;
      if (showpatterns) dirctrl = patternctrl;
      if (showscripts) dirctrl = scriptctrl;
      if (dirctrl == NULL) return;
      
      wxString filepath = dirctrl->GetFilePath();

      // deselect file/folder so this handler will be called if user clicks same item
      wxTreeCtrl* treectrl = dirctrl->GetTreeCtrl();
      #ifdef __WXMSW__
         // can't call UnselectAll() or Unselect() here
      #else
         treectrl->UnselectAll();
      #endif

      if ( filepath.IsEmpty() ) {
         // user clicked on a folder name so expand or collapse it???
         // unfortunately, using Collapse/Expand causes this handler to be
         // called again and there's no easy way to distinguish between
         // a click in the folder name or a dbl-click (or a click in the
         // +/-/arrow image)
         /*
         if ( treectrl->IsExpanded(id) ) {
            treectrl->Collapse(id);
         } else {
            treectrl->Expand(id);
         }
         */
      } else {
         // user clicked on a file name
         if ( inscript ) {
            // use Warning because statusptr->ErrorMessage does nothing if inscript
            if ( showpatterns )
               Warning(_("Cannot load pattern while a script is running."));
            else
               Warning(_("Cannot run script while another one is running."));
         } else if ( generating ) {
            if ( showpatterns )
               statusptr->ErrorMessage(_("Cannot load pattern while generating."));
            else
               statusptr->ErrorMessage(_("Cannot run script while generating."));
         } else {
            // reset background of previously selected file by traversing entire tree;
            // we can't just remember previously selected id because ids don't persist
            // after a folder has been collapsed and expanded
            DeselectTree(treectrl, treectrl->GetRootItem());

            // indicate the selected file
            treectrl->SetItemBackgroundColour(id, *wxLIGHT_GREY);
            
            #ifdef __WXX11__
               // needed for scripts which prompt user to enter a string
               viewptr->SetFocus();
            #endif

            #ifdef __WXMAC__
               if ( !wxFileName::FileExists(filepath) ) {
                  // avoid wxMac bug in wxGenericDirCtrl::GetFilePath; ie. file name
                  // can contain "/" rather than ":" (but directory path is okay)
                  wxFileName fullpath(filepath);
                  wxString dir = fullpath.GetPath();
                  wxString name = fullpath.GetFullName();
                  wxString newpath = dir + wxT(":") + name;
                  if ( wxFileName::FileExists(newpath) ) filepath = newpath;
               }
            #endif
            
            // load pattern or run script
            OpenFile(filepath);
         }
      }

      #ifdef __WXMSW__
         // calling Unselect() here causes a crash so do later in OnIdle
         callUnselect = true;
      #endif

      // changing focus here works on X11 but not on Mac or Windows
      // (presumably because they set focus to treectrl after this call)
      viewptr->SetFocus();
   }
}

// -----------------------------------------------------------------------------

void MainFrame::OnSashDblClick(wxSplitterEvent& WXUNUSED(event))
{
   // splitwin's sash was double-clicked
   if (showpatterns) ToggleShowPatterns();
   if (showscripts) ToggleShowScripts();
   UpdateMenuItems(IsActive());
   UpdateToolBar(IsActive());
}

// -----------------------------------------------------------------------------

void MainFrame::OnOneTimer(wxTimerEvent& WXUNUSED(event))
{
   // fix drag and drop problem on Mac -- see DnDFile::OnDropFiles
   #ifdef __WXMAC__
      // remove colored frame
      if (viewptr) RefreshView();
   #endif
   
   // fix menu item problem on Linux after modal dialog has closed
   #ifdef __WXGTK__
      UpdateMenuItems(true);
   #endif
}

// -----------------------------------------------------------------------------

bool MainFrame::SaveCurrentLayer()
{
   if (currlayer->algo->isEmpty()) return true;    // no need to save empty universe
   wxString query;
   if (numlayers > 1) {
      // make it clear which layer we're asking about
      query.Printf(_("Save the changes to layer %d: \"%s\"?"),
                   currindex, currlayer->currname.c_str());
   } else {
      query.Printf(_("Save the changes to \"%s\"?"), currlayer->currname.c_str());
   }
   int answer = SaveChanges(query, _("If you don't save, your changes will be lost."));
   switch (answer) {
      case 2:  SavePattern(); return true;
      case 1:  return true;   // don't save changes
      default: return false;  // answer == 0 (ie. user selected Cancel)
   }
}

// -----------------------------------------------------------------------------

void MainFrame::OnClose(wxCloseEvent& event)
{
   if (event.CanVeto() && askonquit) {
      // keep track of which unique clones have been seen;
      // we add 1 below to allow for cloneseen[0] (always false)
      const int maxseen = maxlayers/2 + 1;
      bool cloneseen[maxseen];
      for (int i = 0; i < maxseen; i++) cloneseen[i] = false;
   
      // for each dirty layer, ask user if they want to save changes
      int oldindex = currindex;
      for (int i = 0; i < numlayers; i++) {
         // only ask once for each unique clone (cloneid == 0 for non-clone)
         int cid = GetLayer(i)->cloneid;
         if (!cloneseen[cid]) {
            if (cid > 0) cloneseen[cid] = true;
            if (GetLayer(i)->dirty) {
               // temporarily turn off inscript and generating flags
               bool oldscr = inscript;
               bool oldgen = generating;
               inscript = false;
               generating = false;
               
               if (i != currindex) SetLayer(i);
               if (!SaveCurrentLayer()) {
                  // user cancelled "save changes" dialog;
                  // restore current layer and inscript and generating flags
                  SetLayer(oldindex);
                  inscript = oldscr;
                  generating = oldgen;
                  UpdateUserInterface(IsActive());
                  event.Veto();
                  return;
               }
               
               inscript = oldscr;
               generating = oldgen;
            }
         }
      }
   }

   if (GetHelpFrame()) GetHelpFrame()->Close(true);
   if (GetInfoFrame()) GetInfoFrame()->Close(true);
   
   if (splitwin->IsSplit()) dirwinwd = splitwin->GetSashPosition();
   
   #ifndef __WXMAC__
      // if script is running we need to call exit below
      bool wasinscript = inscript;
   #endif

   // abort any running script and tidy up; also restores current directory
   // to location of Golly app so prefs file will be saved in correct place
   FinishScripting();

   // save main window location and other user preferences
   SavePrefs();
   
   // delete any temporary files
   if (wxFileExists(perlfile)) wxRemoveFile(perlfile);
   if (wxFileExists(pythonfile)) wxRemoveFile(pythonfile);
   for (int i = 0; i < numlayers; i++) {
      Layer* layer = GetLayer(i);
      if (wxFileExists(layer->tempstart)) wxRemoveFile(layer->tempstart);
      // also clear undo/redo history
      layer->undoredo->ClearUndoRedo();
   }
   
   #ifndef __WXMAC__
      // avoid error message on Windows or seg fault on Linux
      if (wasinscript) exit(0);
   #endif

   #if defined(__WXX11__) || defined(__WXGTK__)
      // avoid seg fault on Linux
      if (generating) exit(0);
   #else
      if (generating) {
         allowundo = false;   // prevent RememberGenFinish being called
         Stop();
      }
   #endif
   
   Destroy();
}

// -----------------------------------------------------------------------------

void MainFrame::QuitApp()
{
   Close(false);   // false allows OnClose handler to veto close
}

// -----------------------------------------------------------------------------

// note that drag and drop is not supported by wxX11

#if wxUSE_DRAG_AND_DROP

// derive a simple class for handling dropped files
class DnDFile : public wxFileDropTarget
{
public:
   DnDFile() {}
   virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);
};

bool DnDFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString& filenames)
{
   if (mainptr->generating) return false;

   // is there a wx call to bring app to front???
   #ifdef __WXMAC__
      ProcessSerialNumber process;
      if ( GetCurrentProcess(&process) == noErr )
         SetFrontProcess(&process);
   #endif
   #ifdef __WXMSW__
      SetForegroundWindow( (HWND)mainptr->GetHandle() );
   #endif
   mainptr->Raise();
   
   size_t numfiles = filenames.GetCount();
   for ( size_t n = 0; n < numfiles; n++ ) {
      mainptr->OpenFile(filenames[n]);
   }

   #ifdef __WXMAC__
      // need to call Refresh a bit later to remove colored frame on Mac
      onetimer->Start(10, wxTIMER_ONE_SHOT);
      // OnOneTimer will be called once after a delay of 0.01 sec
   #endif
   
   return true;
}

wxDropTarget* MainFrame::NewDropTarget()
{
   return new DnDFile();
}

#endif // wxUSE_DRAG_AND_DROP

// -----------------------------------------------------------------------------

void MainFrame::SetRandomFillPercentage()
{
   // update Random Fill menu item to show randomfill value
   wxMenuBar* mbar = GetMenuBar();
   if (mbar) {
      wxString randlabel;
      randlabel.Printf(_("Random Fill (%d%c)"), randomfill, '%');
      randlabel += GetAccelerator(DO_RANDFILL);
      mbar->SetLabel(ID_RANDOM, randlabel);
   }
}

// -----------------------------------------------------------------------------

void MainFrame::UpdateLayerItem(int index)
{
   // update name in given layer's menu item
   Layer* layer = GetLayer(index);
   wxMenuBar* mbar = GetMenuBar();
   if (mbar) {
      wxString label;
      label.Printf(_("%d: "), index);

      // display asterisk if pattern has been modified
      if (layer->dirty) label += wxT('*');
      
      int cid = layer->cloneid;
      while (cid > 0) {
         // display one or more "=" chars to indicate this is a cloned layer
         label += wxT('=');
         cid--;
      }
      
      label += layer->currname;
      mbar->SetLabel(ID_LAYER0 + index, label);
   }
}

// -----------------------------------------------------------------------------

void MainFrame::AppendLayerItem()
{
   wxMenuBar* mbar = GetMenuBar();
   if (mbar) {
      wxMenu* layermenu = mbar->GetMenu( mbar->FindMenu(_("Layer")) );
      if (layermenu) {
         // no point setting the item name here because UpdateLayerItem
         // will be called very soon
         layermenu->AppendCheckItem(ID_LAYER0 + numlayers - 1, _("foo"));
      } else {
         Warning(_("Could not find Layer menu!"));
      }
   }
}

// -----------------------------------------------------------------------------

void MainFrame::RemoveLayerItem()
{
   wxMenuBar* mbar = GetMenuBar();
   if (mbar) {
      wxMenu* layermenu = mbar->GetMenu( mbar->FindMenu(_("Layer")) );
      if (layermenu) {
         layermenu->Delete(ID_LAYER0 + numlayers);
      } else {
         Warning(_("Could not find Layer menu!"));
      }
   }
}

// -----------------------------------------------------------------------------

void MainFrame::CreateMenus()
{
   wxMenu* fileMenu = new wxMenu();
   wxMenu* editMenu = new wxMenu();
   wxMenu* controlMenu = new wxMenu();
   wxMenu* viewMenu = new wxMenu();
   wxMenu* layerMenu = new wxMenu();
   wxMenu* helpMenu = new wxMenu();

   // create submenus
   wxMenu* plocSubMenu = new wxMenu();
   wxMenu* pmodeSubMenu = new wxMenu();
   wxMenu* cmodeSubMenu = new wxMenu();
   wxMenu* scaleSubMenu = new wxMenu();

   plocSubMenu->AppendCheckItem(ID_PL_TL,       _("Top Left"));
   plocSubMenu->AppendCheckItem(ID_PL_TR,       _("Top Right"));
   plocSubMenu->AppendCheckItem(ID_PL_BR,       _("Bottom Right"));
   plocSubMenu->AppendCheckItem(ID_PL_BL,       _("Bottom Left"));
   plocSubMenu->AppendCheckItem(ID_PL_MID,      _("Middle"));

   pmodeSubMenu->AppendCheckItem(ID_PM_COPY,    _("Copy"));
   pmodeSubMenu->AppendCheckItem(ID_PM_OR,      _("Or"));
   pmodeSubMenu->AppendCheckItem(ID_PM_XOR,     _("Xor"));

   cmodeSubMenu->AppendCheckItem(ID_DRAW,       _("Draw") + GetAccelerator(DO_CURSDRAW));
   cmodeSubMenu->AppendCheckItem(ID_SELECT,     _("Select") + GetAccelerator(DO_CURSSEL));
   cmodeSubMenu->AppendCheckItem(ID_MOVE,       _("Move") + GetAccelerator(DO_CURSMOVE));
   cmodeSubMenu->AppendCheckItem(ID_ZOOMIN,     _("Zoom In") + GetAccelerator(DO_CURSIN));
   cmodeSubMenu->AppendCheckItem(ID_ZOOMOUT,    _("Zoom Out") + GetAccelerator(DO_CURSOUT));

   scaleSubMenu->AppendCheckItem(ID_SCALE_1,    _("1:1") + GetAccelerator(DO_SCALE1));
   scaleSubMenu->AppendCheckItem(ID_SCALE_2,    _("1:2") + GetAccelerator(DO_SCALE2));
   scaleSubMenu->AppendCheckItem(ID_SCALE_4,    _("1:4") + GetAccelerator(DO_SCALE4));
   scaleSubMenu->AppendCheckItem(ID_SCALE_8,    _("1:8") + GetAccelerator(DO_SCALE8));
   scaleSubMenu->AppendCheckItem(ID_SCALE_16,   _("1:16") + GetAccelerator(DO_SCALE16));

   fileMenu->Append(wxID_NEW,                   _("New Pattern") + GetAccelerator(DO_NEWPATT));
   fileMenu->AppendSeparator();
   fileMenu->Append(wxID_OPEN,                  _("Open Pattern...") + GetAccelerator(DO_OPENPATT));
   fileMenu->Append(ID_OPEN_CLIP,               _("Open Clipboard") + GetAccelerator(DO_OPENCLIP));
   fileMenu->Append(ID_OPEN_RECENT,             _("Open Recent"), patternSubMenu);
   fileMenu->AppendSeparator();
   fileMenu->AppendCheckItem(ID_SHOW_PATTERNS,  _("Show Patterns") + GetAccelerator(DO_PATTERNS));
   fileMenu->Append(ID_PATTERN_DIR,             _("Set Pattern Folder...") + GetAccelerator(DO_PATTDIR));
   fileMenu->AppendSeparator();
   fileMenu->Append(wxID_SAVE,                  _("Save Pattern...") + GetAccelerator(DO_SAVE));
   fileMenu->AppendCheckItem(ID_SAVE_XRLE,      _("Save Extended RLE") + GetAccelerator(DO_SAVEXRLE));
   fileMenu->AppendSeparator();
   fileMenu->Append(ID_RUN_SCRIPT,              _("Run Script...") + GetAccelerator(DO_RUNSCRIPT));
   fileMenu->Append(ID_RUN_CLIP,                _("Run Clipboard") + GetAccelerator(DO_RUNCLIP));
   fileMenu->Append(ID_RUN_RECENT,              _("Run Recent"), scriptSubMenu);
   fileMenu->AppendSeparator();
   fileMenu->AppendCheckItem(ID_SHOW_SCRIPTS,   _("Show Scripts") + GetAccelerator(DO_SCRIPTS));
   fileMenu->Append(ID_SCRIPT_DIR,              _("Set Script Folder...") + GetAccelerator(DO_SCRIPTDIR));
   fileMenu->AppendSeparator();
   fileMenu->Append(wxID_PREFERENCES,           _("Preferences...") + GetAccelerator(DO_PREFS));
   fileMenu->AppendSeparator();
   // on the Mac the item is moved to the app menu and the app name is appended to "Quit "
   fileMenu->Append(wxID_EXIT,                  _("Quit") + GetAccelerator(DO_QUIT));

   editMenu->Append(wxID_UNDO,                  _("Undo") + GetAccelerator(DO_UNDO));
   editMenu->Append(wxID_REDO,                  _("Redo") + GetAccelerator(DO_REDO));
   editMenu->AppendCheckItem(ID_NO_UNDO,        _("Disable Undo/Redo") + GetAccelerator(DO_DISABLE));
   editMenu->AppendSeparator();
   editMenu->Append(ID_CUT,                     _("Cut") + GetAccelerator(DO_CUT));
   editMenu->Append(ID_COPY,                    _("Copy") + GetAccelerator(DO_COPY));
   editMenu->Append(ID_CLEAR,                   _("Clear") + GetAccelerator(DO_CLEAR));
   editMenu->Append(ID_OUTSIDE,                 _("Clear Outside") + GetAccelerator(DO_CLEAROUT));
   editMenu->AppendSeparator();
   editMenu->Append(ID_PASTE,                   _("Paste") + GetAccelerator(DO_PASTE));
   editMenu->Append(ID_PMODE,                   _("Paste Mode"), pmodeSubMenu);
   editMenu->Append(ID_PLOCATION,               _("Paste Location"), plocSubMenu);
   editMenu->Append(ID_PASTE_SEL,               _("Paste to Selection") + GetAccelerator(DO_PASTESEL));
   editMenu->AppendSeparator();
   editMenu->Append(ID_SELALL,                  _("Select All") + GetAccelerator(DO_SELALL));
   editMenu->Append(ID_REMOVE,                  _("Remove Selection") + GetAccelerator(DO_REMOVESEL));
   editMenu->Append(ID_SHRINK,                  _("Shrink Selection") + GetAccelerator(DO_SHRINK));
   // full label will be set later by SetRandomFillPercentage
   editMenu->Append(ID_RANDOM,                  _("Random Fill") + GetAccelerator(DO_RANDFILL));
   editMenu->Append(ID_FLIPTB,                  _("Flip Top-Bottom") + GetAccelerator(DO_FLIPTB));
   editMenu->Append(ID_FLIPLR,                  _("Flip Left-Right") + GetAccelerator(DO_FLIPLR));
   editMenu->Append(ID_ROTATEC,                 _("Rotate Clockwise") + GetAccelerator(DO_ROTATECW));
   editMenu->Append(ID_ROTATEA,                 _("Rotate Anticlockwise") + GetAccelerator(DO_ROTATEACW));
   editMenu->AppendSeparator();
   editMenu->Append(ID_CMODE,                   _("Cursor Mode"), cmodeSubMenu);

   controlMenu->Append(ID_START,                _("Start Generating") + GetAccelerator(DO_STARTSTOP));
   controlMenu->Append(ID_NEXT,                 _("Next Generation") + GetAccelerator(DO_NEXTGEN));
   controlMenu->Append(ID_STEP,                 _("Next Step") + GetAccelerator(DO_NEXTSTEP));
   controlMenu->AppendSeparator();
   controlMenu->Append(ID_RESET,                _("Reset") + GetAccelerator(DO_RESET));
   controlMenu->Append(ID_SETGEN,               _("Set Generation...") + GetAccelerator(DO_SETGEN));
   controlMenu->AppendSeparator();
   controlMenu->Append(ID_FASTER,               _("Faster") + GetAccelerator(DO_FASTER));
   controlMenu->Append(ID_SLOWER,               _("Slower") + GetAccelerator(DO_SLOWER));
   controlMenu->AppendSeparator();
   controlMenu->AppendCheckItem(ID_AUTO,        _("Auto Fit") + GetAccelerator(DO_AUTOFIT));
   controlMenu->AppendCheckItem(ID_HASH,        _("Use Hashing") + GetAccelerator(DO_HASHING));
   controlMenu->AppendCheckItem(ID_HYPER,       _("Hyperspeed") + GetAccelerator(DO_HYPER));
   controlMenu->AppendCheckItem(ID_HINFO,       _("Show Hash Info") + GetAccelerator(DO_HASHINFO));
   controlMenu->AppendSeparator();
   controlMenu->Append(ID_RULE,                 _("Set Rule...") + GetAccelerator(DO_RULE));

   viewMenu->Append(ID_FULL,                    _("Full Screen") + GetAccelerator(DO_FULLSCREEN));
   viewMenu->AppendSeparator();
   viewMenu->Append(ID_FIT,                     _("Fit Pattern") + GetAccelerator(DO_FIT));
   viewMenu->Append(ID_FIT_SEL,                 _("Fit Selection") + GetAccelerator(DO_FITSEL));
   viewMenu->Append(ID_MIDDLE,                  _("Middle") + GetAccelerator(DO_MIDDLE));
   viewMenu->Append(ID_RESTORE00,               _("Restore Origin") + GetAccelerator(DO_RESTORE00));
   viewMenu->AppendSeparator();
   viewMenu->Append(wxID_ZOOM_IN,               _("Zoom In") + GetAccelerator(DO_ZOOMIN));
   viewMenu->Append(wxID_ZOOM_OUT,              _("Zoom Out") + GetAccelerator(DO_ZOOMOUT));
   viewMenu->Append(ID_SET_SCALE,               _("Set Scale"), scaleSubMenu);
   viewMenu->AppendSeparator();
   viewMenu->AppendCheckItem(ID_TOOL_BAR,       _("Show Tool Bar") + GetAccelerator(DO_SHOWTOOL));
   viewMenu->AppendCheckItem(ID_LAYER_BAR,      _("Show Layer Bar") + GetAccelerator(DO_SHOWLAYER));
   viewMenu->AppendCheckItem(ID_STATUS_BAR,     _("Show Status Bar") + GetAccelerator(DO_SHOWSTATUS));
   viewMenu->AppendCheckItem(ID_EXACT,          _("Show Exact Numbers") + GetAccelerator(DO_SHOWEXACT));
   viewMenu->AppendCheckItem(ID_GRID,           _("Show Grid Lines") + GetAccelerator(DO_SHOWGRID));
   viewMenu->AppendCheckItem(ID_COLORS,         _("Swap Cell Colors") + GetAccelerator(DO_SWAPCOLORS));
   viewMenu->AppendCheckItem(ID_BUFF,           _("Buffered") + GetAccelerator(DO_BUFFERED));
   viewMenu->AppendSeparator();
   viewMenu->Append(ID_INFO,                    _("Pattern Info") + GetAccelerator(DO_INFO));

   layerMenu->Append(ID_ADD_LAYER,              _("Add Layer") + GetAccelerator(DO_ADD));
   layerMenu->Append(ID_CLONE,                  _("Clone Layer") + GetAccelerator(DO_CLONE));
   layerMenu->Append(ID_DUPLICATE,              _("Duplicate Layer") + GetAccelerator(DO_DUPLICATE));
   layerMenu->AppendSeparator();
   layerMenu->Append(ID_DEL_LAYER,              _("Delete Layer") + GetAccelerator(DO_DELETE));
   layerMenu->Append(ID_DEL_OTHERS,             _("Delete Other Layers") + GetAccelerator(DO_DELOTHERS));
   layerMenu->AppendSeparator();
   layerMenu->Append(ID_MOVE_LAYER,             _("Move Layer...") + GetAccelerator(DO_MOVELAYER));
   layerMenu->Append(ID_NAME_LAYER,             _("Name Layer...") + GetAccelerator(DO_NAMELAYER));
   layerMenu->AppendSeparator();
   layerMenu->AppendCheckItem(ID_SYNC_VIEW,     _("Synchronize Views") + GetAccelerator(DO_SYNCVIEWS));
   layerMenu->AppendCheckItem(ID_SYNC_CURS,     _("Synchronize Cursors") + GetAccelerator(DO_SYNCCURS));
   layerMenu->AppendSeparator();
   layerMenu->AppendCheckItem(ID_STACK,         _("Stack Layers") + GetAccelerator(DO_STACK));
   layerMenu->AppendCheckItem(ID_TILE,          _("Tile Layers") + GetAccelerator(DO_TILE));
   layerMenu->AppendSeparator();
   layerMenu->AppendCheckItem(ID_LAYER0,        _("0"));
   // UpdateLayerItem will soon change the above item name

   helpMenu->Append(ID_HELP_INDEX,              _("Contents"));
   helpMenu->Append(ID_HELP_INTRO,              _("Introduction"));
   helpMenu->Append(ID_HELP_TIPS,               _("Hints and Tips"));
   helpMenu->Append(ID_HELP_SHORTCUTS,          _("Shortcuts"));
   helpMenu->Append(ID_HELP_PERL,               _("Perl Scripting"));
   helpMenu->Append(ID_HELP_PYTHON,             _("Python Scripting"));
   helpMenu->Append(ID_HELP_LEXICON,            _("Life Lexicon"));
   helpMenu->AppendSeparator();
   helpMenu->Append(ID_HELP_FILE,               _("File Menu"));
   helpMenu->Append(ID_HELP_EDIT,               _("Edit Menu"));
   helpMenu->Append(ID_HELP_CONTROL,            _("Control Menu"));
   helpMenu->Append(ID_HELP_VIEW,               _("View Menu"));
   helpMenu->Append(ID_HELP_LAYER,              _("Layer Menu"));
   helpMenu->Append(ID_HELP_HELP,               _("Help Menu"));
   helpMenu->AppendSeparator();
   helpMenu->Append(ID_HELP_REFS,               _("References"));
   helpMenu->Append(ID_HELP_PROBLEMS,           _("Known Problems"));
   helpMenu->Append(ID_HELP_CHANGES,            _("Changes"));
   helpMenu->Append(ID_HELP_CREDITS,            _("Credits"));
   #ifndef __WXMAC__
      helpMenu->AppendSeparator();
   #endif
   // on the Mac the wxID_ABOUT item gets moved to the app menu
   helpMenu->Append(wxID_ABOUT,                 _("About Golly") + GetAccelerator(DO_ABOUT));

   // create the menu bar and append menus
   wxMenuBar* menuBar = new wxMenuBar();
   if (menuBar == NULL) Fatal(_("Failed to create menu bar!"));
   menuBar->Append(fileMenu,     _("&File"));
   menuBar->Append(editMenu,     _("&Edit"));
   menuBar->Append(controlMenu,  _("&Control"));
   menuBar->Append(viewMenu,     _("&View"));
   menuBar->Append(layerMenu,    _("&Layer"));
   menuBar->Append(helpMenu,     _("&Help"));
   
   #ifdef __WXMAC__
      // prevent Window menu being added automatically by wxMac 2.6.1+
      menuBar->SetAutoWindowMenu(false);
   #endif

   // attach menu bar to the frame
   SetMenuBar(menuBar);
}

// -----------------------------------------------------------------------------

// note that we need to remove any accelerator string from GetLabel text
#define SET_ACCEL(i,a) mbar->SetLabel(i, wxMenuItem::GetLabelFromText(mbar->GetLabel(i)) \
                                         + GetAccelerator(a))

void MainFrame::UpdateMenuAccelerators()
{
   // keyboard shortcuts have changed, so update all menu item accelerators
   wxMenuBar* mbar = GetMenuBar();
   if (mbar) {
      //!!! following app menu items aren't updated due to wxMac bug???
      SET_ACCEL(wxID_ABOUT,         DO_ABOUT);
      SET_ACCEL(wxID_PREFERENCES,   DO_PREFS);
      SET_ACCEL(wxID_EXIT,          DO_QUIT);
      
      SET_ACCEL(ID_DRAW,            DO_CURSDRAW);
      SET_ACCEL(ID_SELECT,          DO_CURSSEL);
      SET_ACCEL(ID_MOVE,            DO_CURSMOVE);
      SET_ACCEL(ID_ZOOMIN,          DO_CURSIN);
      SET_ACCEL(ID_ZOOMOUT,         DO_CURSOUT);
      
      SET_ACCEL(ID_SCALE_1,         DO_SCALE1);
      SET_ACCEL(ID_SCALE_2,         DO_SCALE2);
      SET_ACCEL(ID_SCALE_4,         DO_SCALE4);
      SET_ACCEL(ID_SCALE_8,         DO_SCALE8);
      SET_ACCEL(ID_SCALE_16,        DO_SCALE16);
      
      SET_ACCEL(wxID_NEW,           DO_NEWPATT);
      SET_ACCEL(wxID_OPEN,          DO_OPENPATT);
      SET_ACCEL(ID_OPEN_CLIP,       DO_OPENCLIP);
      SET_ACCEL(ID_SHOW_PATTERNS,   DO_PATTERNS);
      SET_ACCEL(ID_PATTERN_DIR,     DO_PATTDIR);
      SET_ACCEL(wxID_SAVE,          DO_SAVE);
      SET_ACCEL(ID_SAVE_XRLE,       DO_SAVEXRLE);
      SET_ACCEL(ID_RUN_SCRIPT,      DO_RUNSCRIPT);
      SET_ACCEL(ID_RUN_CLIP,        DO_RUNCLIP);
      SET_ACCEL(ID_SHOW_SCRIPTS,    DO_SCRIPTS);
      SET_ACCEL(ID_SCRIPT_DIR,      DO_SCRIPTDIR);
      
      SET_ACCEL(wxID_UNDO,          DO_UNDO);
      SET_ACCEL(wxID_REDO,          DO_REDO);
      SET_ACCEL(ID_NO_UNDO,         DO_DISABLE);
      SET_ACCEL(ID_CUT,             DO_CUT);
      SET_ACCEL(ID_COPY,            DO_COPY);
      SET_ACCEL(ID_CLEAR,           DO_CLEAR);
      SET_ACCEL(ID_OUTSIDE,         DO_CLEAROUT);
      SET_ACCEL(ID_PASTE,           DO_PASTE);
      SET_ACCEL(ID_PASTE_SEL,       DO_PASTESEL);
      SET_ACCEL(ID_SELALL,          DO_SELALL);
      SET_ACCEL(ID_REMOVE,          DO_REMOVESEL);
      SET_ACCEL(ID_SHRINK,          DO_SHRINK);
      SET_ACCEL(ID_RANDOM,          DO_RANDFILL);
      SET_ACCEL(ID_FLIPTB,          DO_FLIPTB);
      SET_ACCEL(ID_FLIPLR,          DO_FLIPLR);
      SET_ACCEL(ID_ROTATEC,         DO_ROTATECW);
      SET_ACCEL(ID_ROTATEA,         DO_ROTATEACW);
      
      SET_ACCEL(ID_START,           DO_STARTSTOP);
      SET_ACCEL(ID_NEXT,            DO_NEXTGEN);
      SET_ACCEL(ID_STEP,            DO_NEXTSTEP);
      SET_ACCEL(ID_RESET,           DO_RESET);
      SET_ACCEL(ID_SETGEN,          DO_SETGEN);
      SET_ACCEL(ID_FASTER,          DO_FASTER);
      SET_ACCEL(ID_SLOWER,          DO_SLOWER);
      SET_ACCEL(ID_AUTO,            DO_AUTOFIT);
      SET_ACCEL(ID_HASH,            DO_HASHING);
      SET_ACCEL(ID_HYPER,           DO_HYPER);
      SET_ACCEL(ID_HINFO,           DO_HASHINFO);
      SET_ACCEL(ID_RULE,            DO_RULE);
      
      SET_ACCEL(ID_FULL,            DO_FULLSCREEN);
      SET_ACCEL(ID_FIT,             DO_FIT);
      SET_ACCEL(ID_FIT_SEL,         DO_FITSEL);
      SET_ACCEL(ID_MIDDLE,          DO_MIDDLE);
      SET_ACCEL(ID_RESTORE00,       DO_RESTORE00);
      SET_ACCEL(wxID_ZOOM_IN,       DO_ZOOMIN);
      SET_ACCEL(wxID_ZOOM_OUT,      DO_ZOOMOUT);
      SET_ACCEL(ID_TOOL_BAR,        DO_SHOWTOOL);
      SET_ACCEL(ID_LAYER_BAR,       DO_SHOWLAYER);
      SET_ACCEL(ID_STATUS_BAR,      DO_SHOWSTATUS);
      SET_ACCEL(ID_EXACT,           DO_SHOWEXACT);
      SET_ACCEL(ID_GRID,            DO_SHOWGRID);
      SET_ACCEL(ID_COLORS,          DO_SWAPCOLORS);
      SET_ACCEL(ID_BUFF,            DO_BUFFERED);
      SET_ACCEL(ID_INFO,            DO_INFO);
      
      SET_ACCEL(ID_ADD_LAYER,       DO_ADD);
      SET_ACCEL(ID_CLONE,           DO_CLONE);
      SET_ACCEL(ID_DUPLICATE,       DO_DUPLICATE);
      SET_ACCEL(ID_DEL_LAYER,       DO_DELETE);
      SET_ACCEL(ID_DEL_OTHERS,      DO_DELOTHERS);
      SET_ACCEL(ID_MOVE_LAYER,      DO_MOVELAYER);
      SET_ACCEL(ID_NAME_LAYER,      DO_NAMELAYER);
      SET_ACCEL(ID_SYNC_VIEW,       DO_SYNCVIEWS);
      SET_ACCEL(ID_SYNC_CURS,       DO_SYNCCURS);
      SET_ACCEL(ID_STACK,           DO_STACK);
      SET_ACCEL(ID_TILE,            DO_TILE);
   }
}

// -----------------------------------------------------------------------------

void MainFrame::CreateDirControls()
{
   patternctrl = new wxGenericDirCtrl(splitwin, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize,
                                      #ifdef __WXMSW__
                                         // speed up a bit
                                         wxDIRCTRL_DIR_ONLY | wxNO_BORDER,
                                      #else
                                         wxNO_BORDER,
                                      #endif
                                      wxEmptyString   // see all file types
                                     );
   if (patternctrl == NULL) Fatal(_("Failed to create pattern directory control!"));

   scriptctrl = new wxGenericDirCtrl(splitwin, wxID_ANY, wxEmptyString,
                                     wxDefaultPosition, wxDefaultSize,
                                     #ifdef __WXMSW__
                                        // speed up a bit
                                        wxDIRCTRL_DIR_ONLY | wxNO_BORDER,
                                     #else
                                        wxNO_BORDER,
                                     #endif
                                     _T("Perl/Python scripts|*.pl;*.py")
                                    );
   if (scriptctrl == NULL) Fatal(_("Failed to create script directory control!"));
   
   #ifdef __WXMSW__
      // now remove wxDIRCTRL_DIR_ONLY so we see files
      patternctrl->SetWindowStyle(wxNO_BORDER);
      scriptctrl->SetWindowStyle(wxNO_BORDER);
   #endif

   #if defined(__WXGTK__)
      // make sure background is white when using KDE's GTK theme
      patternctrl->GetTreeCtrl()->SetBackgroundStyle(wxBG_STYLE_COLOUR);
      scriptctrl->GetTreeCtrl()->SetBackgroundStyle(wxBG_STYLE_COLOUR);
      patternctrl->GetTreeCtrl()->SetBackgroundColour(*wxWHITE);
      scriptctrl->GetTreeCtrl()->SetBackgroundColour(*wxWHITE);
      // reduce indent a bit
      patternctrl->GetTreeCtrl()->SetIndent(8);
      scriptctrl->GetTreeCtrl()->SetIndent(8);
   #elif defined(__WXMAC__)
      // reduce indent a bit more
      patternctrl->GetTreeCtrl()->SetIndent(6);
      scriptctrl->GetTreeCtrl()->SetIndent(6);
   #else
      // reduce indent a lot
      patternctrl->GetTreeCtrl()->SetIndent(4);
      scriptctrl->GetTreeCtrl()->SetIndent(4);
   #endif
   
   #ifdef __WXMAC__
      // reduce font size (to get this to reduce line height we had to
      // make a few changes to wxMac/src/generic/treectlg.cpp)
      wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
      font.SetPointSize(12);
      patternctrl->GetTreeCtrl()->SetFont(font);
      scriptctrl->GetTreeCtrl()->SetFont(font);
   #endif
   
   if ( wxFileName::DirExists(patterndir) ) {
      // only show patterndir and its contents
      SimplifyTree(patterndir, patternctrl->GetTreeCtrl(), patternctrl->GetRootId());
   }
   if ( wxFileName::DirExists(scriptdir) ) {
      // only show scriptdir and its contents
      SimplifyTree(scriptdir, scriptctrl->GetTreeCtrl(), scriptctrl->GetRootId());
   }
}

// -----------------------------------------------------------------------------

// create the main window
MainFrame::MainFrame()
   : wxFrame(NULL, wxID_ANY, wxEmptyString, wxPoint(mainx,mainy), wxSize(mainwd,mainht))
{
   wxGetApp().SetFrameIcon(this);

   // initialize hidden files to be in same folder as Golly app;
   // they must be absolute paths in case they are used from a script command when
   // the current directory has been changed to the location of the script file
   clipfile = gollydir + wxT(".golly_clipboard");
   perlfile = gollydir + wxT(".golly_clip.pl");
   pythonfile = gollydir + wxT(".golly_clip.py");

   // create one-shot timer (see OnOneTimer)
   onetimer = new wxTimer(this, ID_ONE_TIMER);

   CreateMenus();
   CreateToolbar();
   
   // if tool bar is visible then adjust position of other child windows
   int toolwd = showtool ? toolbarwd : 0;

   int wd, ht;
   GetClientSize(&wd, &ht);
   // wd or ht might be < 1 on Win/X11 platforms
   if (wd < 1) wd = 1;
   if (ht < 1) ht = 1;

   // wxStatusBar can only appear at bottom of frame so we use our own
   // status bar class which creates a child window at top of frame
   // but to the right of the tool bar
   int statht = showexact ? STATUS_EXHT : STATUS_HT;
   if (!showstatus) statht = 0;
   statusptr = new StatusBar(this, toolwd, 0, wd - toolwd, statht);
   if (statusptr == NULL) Fatal(_("Failed to create status bar!"));
   
   // create a split window with pattern/script directory in left pane
   // and layer bar plus pattern viewport in right pane
   splitwin = new wxSplitterWindow(this, wxID_ANY,
                                   wxPoint(toolwd, statht),
                                   wxSize(wd - toolwd, ht - statht),
                                   #ifdef __WXMSW__
                                      wxSP_BORDER |
                                   #endif
                                   wxSP_3DSASH | wxSP_NO_XP_THEME | wxSP_LIVE_UPDATE);
   if (splitwin == NULL) Fatal(_("Failed to create split window!"));

   // create patternctrl and scriptctrl in left pane
   CreateDirControls();
   
   // create a window for right pane which contains layer bar and pattern viewport
   rightpane = new RightWindow(splitwin, 0, 0, wd - toolwd, ht - statht);
   if (rightpane == NULL) Fatal(_("Failed to create right pane!"));
   
   // create layer bar and initial layer
   CreateLayerBar(rightpane);
   AddLayer();

   // enable/disable tool tips after creating tool bar and layer bar
   #if wxUSE_TOOLTIPS
      wxToolTip::Enable(showtips);
      wxToolTip::SetDelay(1500);    // 1.5 secs
   #endif
   
   // create viewport at minimum size to avoid scroll bars being clipped on Mac
   viewptr = new PatternView(rightpane, 0, showlayer ? layerbarht : 0, 40, 40,
                             wxNO_BORDER |
                             wxWANTS_CHARS |              // receive all keyboard events
                             wxFULL_REPAINT_ON_RESIZE |
                             wxVSCROLL | wxHSCROLL);
   if (viewptr == NULL) Fatal(_("Failed to create viewport window!"));
   
   // this is the main viewport window (tile windows have a tileindex >= 0)
   viewptr->tileindex = -1;
   bigview = viewptr;
   
   #if wxUSE_DRAG_AND_DROP
      // let users drop files onto viewport
      viewptr->SetDropTarget(new DnDFile());
   #endif
   
   // these seemingly redundant steps are needed to avoid problems on Windows
   splitwin->SplitVertically(patternctrl, rightpane, dirwinwd);
   splitwin->SetSashPosition(dirwinwd);
   splitwin->SetMinimumPaneSize(50);
   splitwin->Unsplit(patternctrl);
   splitwin->UpdateSize();

   splitwin->SplitVertically(scriptctrl, rightpane, dirwinwd);
   splitwin->SetSashPosition(dirwinwd);
   splitwin->SetMinimumPaneSize(50);
   splitwin->Unsplit(scriptctrl);
   splitwin->UpdateSize();

   if (showpatterns) splitwin->SplitVertically(patternctrl, rightpane, dirwinwd);
   if (showscripts) splitwin->SplitVertically(scriptctrl, rightpane, dirwinwd);

   InitDrawingData();      // do this after viewport size has been set

   pendingfiles.Clear();   // no pending script/pattern files
   generating = false;     // not generating pattern
   fullscreen = false;     // not in full screen mode
   showbanner = true;      // avoid first file clearing banner message
}

// -----------------------------------------------------------------------------

MainFrame::~MainFrame()
{
   delete onetimer;
   DestroyDrawingData();
}
