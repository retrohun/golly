                        /*** /

This file is part of Golly, a Game of Life Simulator.
Copyright (C) 2011 Andrew Trevorrow and Tomas Rokicki.

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

#include "wx/filename.h"   // for wxFileName

#include "bigint.h"
#include "lifealgo.h"
#include "qlifealgo.h"
#include "hlifealgo.h"
#include "jvnalgo.h"
#include "generationsalgo.h"
#include "ruletable_algo.h"
#include "ruletreealgo.h"

#include "wxgolly.h"       // for wxGetApp
#include "wxmain.h"        // for ID_ALGO0
#include "wxutils.h"       // for Fatal, Warning
#include "wxrender.h"      // for DrawOneIcon
#include "wxprefs.h"       // for gollydir
#include "wxalgos.h"

// -----------------------------------------------------------------------------

// exported data:

wxMenu* algomenu;                   // menu of algorithm names
algo_type initalgo = QLIFE_ALGO;    // initial layer's algorithm
AlgoData* algoinfo[MAX_ALGOS];      // static info for each algorithm

wxBitmap** hexicons7x7;          // hexagonal icon bitmaps for scale 1:8
wxBitmap** hexicons15x15;        // hexagonal icon bitmaps for scale 1:16

// -----------------------------------------------------------------------------

// These default cell colors were generated by continuously finding the
// color furthest in rgb space from the closest of the already selected
// colors, black, and white.
static unsigned char default_colors[] = {
48,48,48, // better if state 0 is dark gray (was 255,127,0)
0,255,127,127,0,255,148,148,148,128,255,0,255,0,128,
0,128,255,1,159,0,159,0,1,255,254,96,0,1,159,96,255,254,
254,96,255,126,125,21,21,126,125,125,21,126,255,116,116,116,255,116,
116,116,255,228,227,0,28,255,27,255,27,28,0,228,227,227,0,228,
27,28,255,59,59,59,234,195,176,175,196,255,171,194,68,194,68,171,
68,171,194,72,184,71,184,71,72,71,72,184,169,255,188,252,179,63,
63,252,179,179,63,252,80,9,0,0,80,9,9,0,80,255,175,250,
199,134,213,115,100,95,188,163,0,0,188,163,163,0,188,203,73,0,
0,203,73,73,0,203,94,189,0,189,0,94,0,94,189,187,243,119,
55,125,32,125,32,55,32,55,125,255,102,185,102,185,255,120,209,168,
208,166,119,135,96,192,182,255,41,83,153,130,247,88,55,89,247,55,
88,55,247,87,75,0,0,87,75,75,0,87,200,135,59,51,213,127,
255,255,162,255,37,182,37,182,255,228,57,117,142,163,210,57,117,228,
193,255,246,188,107,123,123,194,107,145,59,5,5,145,59,59,5,145,
119,39,198,40,197,23,197,23,40,23,40,197,178,199,158,255,201,121,
134,223,223,39,253,84,149,203,15,203,15,149,15,149,203,152,144,90,
143,75,139,71,97,132,224,65,219,65,219,224,255,255,40,218,223,69,
74,241,0,241,0,74,0,74,241,122,171,51,220,211,227,61,127,87,
90,124,176,36,39,13,165,142,255,255,38,255,38,255,255,83,50,107,
224,142,165,255,181,9,9,255,181,181,9,255,140,238,70,255,74,5,
74,5,255,138,84,51,31,172,101,177,115,17,221,0,0,0,221,0,
0,0,221,220,255,200,0,41,50,255,150,205,178,45,116,113,255,189,
47,0,44,40,119,171,205,107,255,177,115,172,133,73,236,109,0,168,
168,46,207,188,181,203,212,188,35,90,97,52,39,209,184,41,164,152,
227,46,70,46,70,227,211,156,255,98,146,222,136,56,95,102,54,152,
86,142,0,142,0,86,0,86,142,86,223,96,246,135,46,4,208,120,
212,233,158,177,92,214,104,147,88,149,240,147,227,93,148,72,255,133,
209,27,194,147,255,255,44,93,0,160,36,158,182,233,0,96,94,217,
218,103,88,163,154,38,118,114,139,94,0,43,113,164,174,168,188,114,
0,23,119,42,86,93,255,226,202,80,191,155,255,158,136,0,247,62,
234,146,88,0,183,229,110,212,36,0,143,161,105,191,210,133,164,0,
41,30,89,164,0,132,30,89,42,178,222,217,121,22,11,221,107,22,
69,151,255,45,158,3,158,3,45,3,45,158,86,42,29,9,122,22,
213,209,110,53,221,57,159,101,91,93,140,45,247,213,37,185,34,0,
0,185,34,34,0,185,236,0,172,210,180,78,231,107,221,162,49,43,
43,162,49,49,43,162,36,248,213,114,0,214,213,36,248,149,34,243,
185,158,167,144,122,224,34,245,149,255,31,98,31,98,255,152,200,193,
255,80,95,128,123,63,102,62,72,255,62,148,151,226,108,159,99,255,
226,255,126,98,223,136,80,95,255,225,153,15,73,41,211,212,71,41,
83,217,187,180,235,79,0,166,127,251,135,243,229,41,0,41,0,229,
82,255,216,141,174,249,249,215,255,167,31,79,31,79,167,213,102,185,
255,215,83,4,2,40,224,171,220,41,0,4,6,50,90,221,15,113,
15,113,221,33,0,115,108,23,90,182,215,36
};

// -----------------------------------------------------------------------------

// XPM data for default 7x7 icon
static const char* default7x7[] = {
// width height ncolors chars_per_pixel
"7 7 2 1",
// colors
"A c #000000000000",    // black will be transparent
"B c #FFFFFFFFFFFF",    // white
// pixels
"AAABAAA",
"AABBBAA",
"ABBBBBA",
"BBBBBBB",
"ABBBBBA",
"AABBBAA",
"AAABAAA"
};

// XPM data for default 15x15 icon
static const char* default15x15[] = {
// width height ncolors chars_per_pixel
"15 15 2 1",
// colors
"A c #000000000000",    // black will be transparent
"B c #FFFFFFFFFFFF",    // white
// pixels
"AAAAAAAAAAAAAAA",
"AAAAAAABAAAAAAA",
"AAAAAABBBAAAAAA",
"AAAAABBBBBAAAAA",
"AAAABBBBBBBAAAA",
"AAABBBBBBBBBAAA",
"AABBBBBBBBBBBAA",
"ABBBBBBBBBBBBBA",
"AABBBBBBBBBBBAA",
"AAABBBBBBBBBAAA",
"AAAABBBBBBBAAAA",
"AAAAABBBBBAAAAA",
"AAAAAABBBAAAAAA",
"AAAAAAABAAAAAAA",
"AAAAAAAAAAAAAAA"
};

// XPM data for the 7x7 icon used for hexagonal CA
static const char* hex7x7[] = {
// width height ncolors chars_per_pixel
"7 7 2 1",
// colors
"A c #FFFFFFFFFFFF",
"B c #000000000000",    // black will be transparent
// pixels
"BAABBBB",
"AAAAABB",
"AAAAAAB",
"BAAAAAB",
"BAAAAAA",
"BBAAAAA",
"BBBBAAB"};

// XPM data for the 15x15 icon used for hexagonal CA
static const char* hex15x15[] = {
// width height ncolors chars_per_pixel
"15 15 2 1",
// colors
"A c #FFFFFFFFFFFF",
"B c #000000000000",    // black will be transparent
// pixels
"BBBAABBBBBBBBBB",
"BBAAAAABBBBBBBB",
"BAAAAAAAABBBBBB",
"AAAAAAAAAAABBBB",
"AAAAAAAAAAAABBB",
"BAAAAAAAAAAABBB",
"BAAAAAAAAAAAABB",
"BBAAAAAAAAAAABB",
"BBAAAAAAAAAAAAB",
"BBBAAAAAAAAAAAB",
"BBBAAAAAAAAAAAA",
"BBBBAAAAAAAAAAA",
"BBBBBBAAAAAAAAB",
"BBBBBBBBAAAAABB",
"BBBBBBBBBBAABBB"};


// -----------------------------------------------------------------------------

#ifdef __WXGTK__
// this routine is used to fix a wxGTK bug which reverses colors when
// converting a monochrome wxImage to a wxBitmap with depth 1
static void ReverseImageColors(wxImage& image)
{
   // get image data as an array of 3 bytes per pixel (RGBRGBRGB...)
   int numpixels = image.GetWidth() * image.GetHeight();
   unsigned char* newdata = (unsigned char*) malloc(numpixels * 3);
   if (newdata) {
      unsigned char* p = image.GetData();
      unsigned char* n = newdata;
      for (int i = 0; i < numpixels; i++) {
         unsigned char r = *p++;
         unsigned char g = *p++;
         unsigned char b = *p++;
         if (r == 0 && g == 0 && b == 0) {
            // change black to white
            *n++ = 255;
            *n++ = 255;
            *n++ = 255;
         } else {
            // change non-black to black
            *n++ = 0;
            *n++ = 0;
            *n++ = 0;
         }
      }
      image.SetData(newdata);    // image now owns pointer
   } else {
      Fatal(_("Malloc failed in ReverseImageColors!"));
   }
}
#endif

// -----------------------------------------------------------------------------

static wxBitmap** CreateIconBitmaps(const char** xpmdata, int maxstates)
{
   if (xpmdata == NULL) return NULL;
   
   wxImage image(xpmdata);
   image.SetMaskColour(0, 0, 0);    // make black transparent

   #ifdef __WXGTK__
      // fix wxGTK bug
      ReverseImageColors(image);
   #endif

   wxBitmap allicons(image, 1);     // default icons are monochrome

   int wd = allicons.GetWidth();
   int numicons = allicons.GetHeight() / wd;
   if (numicons > 255) numicons = 255;          // play safe
   
   wxBitmap** iconptr = (wxBitmap**) malloc(256 * sizeof(wxBitmap*));
   if (iconptr) {
      // only need to test < maxstates here, but play safe
      for (int i = 0; i < 256; i++) iconptr[i] = NULL;
      
      for (int i = 0; i < numicons; i++) {
         wxRect rect(0, i*wd, wd, wd);
         // add 1 to skip iconptr[0] (ie. dead state)
         iconptr[i+1] = new wxBitmap(allicons.GetSubBitmap(rect));
      }
      
      if (numicons < maxstates-1 && iconptr[numicons]) {
         // duplicate last icon
         wxRect rect(0, (numicons-1)*wd, wd, wd);
         for (int i = numicons; i < maxstates-1; i++) {
            iconptr[i+1] = new wxBitmap(allicons.GetSubBitmap(rect));
         }
      }
   }
   return iconptr;
}

// -----------------------------------------------------------------------------

static wxBitmap** ScaleIconBitmaps(wxBitmap** srcicons, int size)
{
   if (srcicons == NULL) return NULL;
   
   wxBitmap** iconptr = (wxBitmap**) malloc(256 * sizeof(wxBitmap*));
   if (iconptr) {
      for (int i = 0; i < 256; i++) {
         if (srcicons[i] == NULL) {
            iconptr[i] = NULL;
         } else {
            wxImage image = srcicons[i]->ConvertToImage();
            iconptr[i] = new wxBitmap(image.Scale(size, size, wxIMAGE_QUALITY_HIGH));
         }
      }
   }
   return iconptr;
}

// -----------------------------------------------------------------------------

static void CreateDefaultIcons(AlgoData* ad)
{
   if (ad->defxpm7x7 || ad->defxpm15x15) {
      // create icons using given algo's default XPM data
      ad->icons7x7 = CreateIconBitmaps(ad->defxpm7x7, ad->maxstates);
      ad->icons15x15 = CreateIconBitmaps(ad->defxpm15x15, ad->maxstates);

      // create scaled bitmaps if only one size was supplied
      if (!ad->icons15x15) {
         // scale up 7x7 bitmaps (looks ugly)
         ad->icons15x15 = ScaleIconBitmaps(ad->icons7x7, 15);
      }
      if (!ad->icons7x7) {
         // scale down 15x15 bitmaps (not too bad)
         ad->icons7x7 = ScaleIconBitmaps(ad->icons15x15, 7);
      }
   } else {
      // algo didn't supply any icons so use static XPM data defined above
      ad->icons7x7 = CreateIconBitmaps(default7x7, ad->maxstates);
      ad->icons15x15 = CreateIconBitmaps(default15x15, ad->maxstates);
   }
}

// -----------------------------------------------------------------------------

AlgoData::AlgoData() {
   algomem = defbase = 0;
   statusbrush = NULL;
   icons7x7 = icons15x15 = NULL;
   iconfile = wxEmptyString;
}

// -----------------------------------------------------------------------------

AlgoData& AlgoData::tick() {
   AlgoData* r = new AlgoData();
   algoinfo[r->id] = r;
   return *r;
}

// -----------------------------------------------------------------------------

void InitAlgorithms()
{
   // qlife must be 1st and hlife must be 2nd
   qlifealgo::doInitializeAlgoInfo(AlgoData::tick());
   hlifealgo::doInitializeAlgoInfo(AlgoData::tick());
   // nicer if the rest are in alphabetical order
   generationsalgo::doInitializeAlgoInfo(AlgoData::tick());
   jvnalgo::doInitializeAlgoInfo(AlgoData::tick());
   ruletable_algo::doInitializeAlgoInfo(AlgoData::tick());
   ruletreealgo::doInitializeAlgoInfo(AlgoData::tick()) ;

   // algomenu is used when algo button is pressed and for Set Algo submenu
   algomenu = new wxMenu();

   // init algoinfo array
   for (int i = 0; i < NumAlgos(); i++) {
      AlgoData* ad = algoinfo[i];
      if (ad->algoName == 0 || ad->creator == 0)
         Fatal(_("Algorithm did not set name and/or creator"));
      
      wxString name = wxString(ad->algoName, wxConvLocal);
      algomenu->AppendCheckItem(ID_ALGO0 + i, name);
      
      // does algo use hashing?
      ad->canhash = ad->defbase == 8;    //!!! safer method needed???
      
      // set status bar background by cycling thru a few pale colors
      switch (i % 9) {
         case 0: ad->statusrgb.Set(255, 255, 206); break;  // pale yellow
         case 1: ad->statusrgb.Set(226, 250, 248); break;  // pale blue
         case 2: ad->statusrgb.Set(255, 233, 233); break;  // pale pink
         case 3: ad->statusrgb.Set(225, 255, 225); break;  // pale green
         case 4: ad->statusrgb.Set(243, 225, 255); break;  // pale purple
         case 5: ad->statusrgb.Set(255, 220, 180); break;  // pale orange
         case 6: ad->statusrgb.Set(200, 255, 255); break;  // pale aqua
         case 7: ad->statusrgb.Set(200, 200, 200); break;  // pale gray
         case 8: ad->statusrgb.Set(255, 255, 255); break;  // white
      }
      ad->statusbrush = new wxBrush(ad->statusrgb);
      
      // initialize default color scheme
      if (ad->defr[0] == ad->defr[1] &&
          ad->defg[0] == ad->defg[1] &&
          ad->defb[0] == ad->defb[1]) {
         // colors are nonsensical, probably unset, so use above defaults
         unsigned char* rgbptr = default_colors;
         for (int c = 0; c < ad->maxstates; c++) {
            ad->defr[c] = *rgbptr++;
            ad->defg[c] = *rgbptr++;
            ad->defb[c] = *rgbptr++;
         }
      }
      ad->gradient = ad->defgradient;
      ad->fromrgb.Set(ad->defr1, ad->defg1, ad->defb1);
      ad->torgb.Set(ad->defr2, ad->defg2, ad->defb2);
      for (int c = 0; c < ad->maxstates; c++) {
         ad->algor[c] = ad->defr[c];
         ad->algog[c] = ad->defg[c];
         ad->algob[c] = ad->defb[c];
      }
      
      CreateDefaultIcons(ad);
   }
   
   hexicons7x7 = CreateIconBitmaps(hex7x7,256);
   hexicons15x15 = CreateIconBitmaps(hex15x15,256);
}

// -----------------------------------------------------------------------------

lifealgo* CreateNewUniverse(algo_type algotype, bool allowcheck)
{
   lifealgo* newalgo = NULL;
   newalgo = algoinfo[algotype]->creator();

   if (newalgo == NULL) Fatal(_("Failed to create new universe!"));

   if (algoinfo[algotype]->algomem >= 0)
      newalgo->setMaxMemory(algoinfo[algotype]->algomem);

   if (allowcheck) newalgo->setpoll(wxGetApp().Poller());

   return newalgo;
}

// -----------------------------------------------------------------------------

const char* GetAlgoName(algo_type algotype)
{
   return algoinfo[algotype]->algoName;
}

// -----------------------------------------------------------------------------

int NumAlgos()
{
   return staticAlgoInfo::getNumAlgos();
}

// -----------------------------------------------------------------------------

bool LoadIconFile(const wxString& path, int maxstate,
                  wxBitmap*** out15x15, wxBitmap*** out7x7)
{
   wxImage image;
   if (!image.LoadFile(path)) {
      Warning(_("Could not load icon bitmaps from file:\n") + path);
      return false;
   }
   
   #ifdef __WXGTK__
      // need alpha channel on Linux
      image.SetMaskColour(0, 0, 0);    // make black transparent
   #endif
   
   // check for multi-color icons
   int depth = -1;
   if (image.CountColours(2) <= 2) depth = 1;   // monochrome

   #ifdef __WXGTK__
      // fix wxGTK bug
      if (depth == 1) ReverseImageColors(image);
   #endif
   
   wxBitmap allicons(image, depth);
   int wd = allicons.GetWidth();
   int ht = allicons.GetHeight();

   // check dimensions
   if (ht != 15 && ht != 22) {
      Warning(_("Wrong bitmap height in icon file (must be 15 or 22):\n") + path);
      return false;
   }
   if (wd % 15 != 0) {
      Warning(_("Wrong bitmap width in icon file (must be multiple of 15):\n") + path);
      return false;
   }
   
   // first extract 15x15 icons
   int numicons = wd / 15;
   if (numicons > 255) numicons = 255;    // play safe

   wxBitmap** iconptr = (wxBitmap**) malloc(256 * sizeof(wxBitmap*));
   if (iconptr) {
      for (int i = 0; i < 256; i++) iconptr[i] = NULL;
      for (int i = 0; i < numicons; i++) {
         wxRect rect(i*15, 0, 15, 15);
         // add 1 to skip iconptr[0] (ie. dead state)
         iconptr[i+1] = new wxBitmap(allicons.GetSubBitmap(rect));
      }
      if (numicons < maxstate && iconptr[numicons]) {
         // duplicate last icon
         wxRect rect((numicons-1)*15, 0, 15, 15);
         for (int i = numicons; i < maxstate; i++) {
            iconptr[i+1] = new wxBitmap(allicons.GetSubBitmap(rect));
         }
      }
      
      // if there is an extra icon at the right end of the multi-color icons then
      // store it in iconptr[0] -- it will be used later in UpdateCurrentColors()
      // to set the color of state 0
      if (depth != 1 && (wd / 15) > maxstate) {
         wxRect rect(maxstate*15, 0, 15, 15);
         iconptr[0] = new wxBitmap(allicons.GetSubBitmap(rect));
      }
   }
   *out15x15 = iconptr;
   
   if (ht == 22) {
      // extract 7x7 icons (at bottom left corner of each 15x15 icon)
      iconptr = (wxBitmap**) malloc(256 * sizeof(wxBitmap*));
      if (iconptr) {
         for (int i = 0; i < 256; i++) iconptr[i] = NULL;
         for (int i = 0; i < numicons; i++) {
            wxRect rect(i*15, 15, 7, 7);
            // add 1 to skip iconptr[0] (ie. dead state)
            iconptr[i+1] = new wxBitmap(allicons.GetSubBitmap(rect));
         }
         if (numicons < maxstate && iconptr[numicons]) {
            // duplicate last icon
            wxRect rect((numicons-1)*15, 15, 7, 7);
            for (int i = numicons; i < maxstate; i++) {
               iconptr[i+1] = new wxBitmap(allicons.GetSubBitmap(rect));
            }
         }
      }
      *out7x7 = iconptr;
   } else {
      // create 7x7 icons by scaling down 15x15 icons
      *out7x7 = ScaleIconBitmaps(*out15x15, 7);
   }
   
   return true;
}

// -----------------------------------------------------------------------------

void LoadIcons(algo_type algotype)
{
   AlgoData* ad = algoinfo[algotype];

   // deallocate old algo icons if they exist
   if (ad->icons15x15) {
      for (int i = 0; i < 256; i++) delete ad->icons15x15[i];
      free(ad->icons15x15);
      ad->icons15x15 = NULL;
   }
   if (ad->icons7x7) {
      for (int i = 0; i < 256; i++) delete ad->icons7x7[i];
      free(ad->icons7x7);
      ad->icons7x7 = NULL;
   }
   
   if (ad->iconfile.length() > 0) {
      // try to load icons from this file
      if (!wxFileName::FileExists(ad->iconfile)) {
         Warning(_("Icon file does not exist:\n") + ad->iconfile);
      } else if (LoadIconFile(ad->iconfile, ad->maxstates-1, &ad->icons15x15, &ad->icons7x7)) {
         // icons were successfully loaded
         return;
      }
      // an error occurred, so restore default icons
      ad->iconfile = wxEmptyString;
      CreateDefaultIcons(ad);
   } else {
      // iconfile path is empty, so restore default icons
      CreateDefaultIcons(ad);
   }
}

// -----------------------------------------------------------------------------

// define a window for displaying icons:

const int NUMCOLS = 32;       // number of columns
const int NUMROWS = 8;        // number of rows

class IconPanel : public wxPanel
{
public:
   IconPanel(wxWindow* parent, wxWindowID id, int size, int algotype)
      : wxPanel(parent, id, wxPoint(0,0), wxSize(NUMCOLS*size+1,NUMROWS*size+1))
   { boxsize = size; algoindex = algotype; }

   wxStaticText* statebox;    // for showing state of icon under cursor
   int boxsize;               // wd and ht of each icon box
   int algoindex;             // index into algoinfo
   
private:
   void GetGradientColor(int state, unsigned char* r,
                                    unsigned char* g,
                                    unsigned char* b);

   void OnEraseBackground(wxEraseEvent& event);
   void OnPaint(wxPaintEvent& event);
   void OnMouseMotion(wxMouseEvent& event);
   void OnMouseExit(wxMouseEvent& event);

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(IconPanel, wxPanel)
   EVT_ERASE_BACKGROUND (IconPanel::OnEraseBackground)
   EVT_PAINT            (IconPanel::OnPaint)
   EVT_MOTION           (IconPanel::OnMouseMotion)
   EVT_ENTER_WINDOW     (IconPanel::OnMouseMotion)
   EVT_LEAVE_WINDOW     (IconPanel::OnMouseExit)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------

void IconPanel::OnEraseBackground(wxEraseEvent& WXUNUSED(event))
{
   // do nothing
}

// -----------------------------------------------------------------------------

void IconPanel::GetGradientColor(int state, unsigned char* r,
                                            unsigned char* g,
                                            unsigned char* b)
{
   // calculate gradient color for given state (> 0 and < maxstates)
   AlgoData* ad = algoinfo[algoindex];
   if (state == 1) {
      *r = ad->fromrgb.Red();
      *g = ad->fromrgb.Green();
      *b = ad->fromrgb.Blue();
   } else if (state == ad->maxstates - 1) {
      *r = ad->torgb.Red();
      *g = ad->torgb.Green();
      *b = ad->torgb.Blue();
   } else {
      unsigned char r1 = ad->fromrgb.Red();
      unsigned char g1 = ad->fromrgb.Green();
      unsigned char b1 = ad->fromrgb.Blue();
      unsigned char r2 = ad->torgb.Red();
      unsigned char g2 = ad->torgb.Green();
      unsigned char b2 = ad->torgb.Blue();
      int N = ad->maxstates - 2;
      double rfrac = (double)(r2 - r1) / (double)N;
      double gfrac = (double)(g2 - g1) / (double)N;
      double bfrac = (double)(b2 - b1) / (double)N;
      *r = (int)(r1 + (state-1) * rfrac + 0.5);
      *g = (int)(g1 + (state-1) * gfrac + 0.5);
      *b = (int)(b1 + (state-1) * bfrac + 0.5);
   }
}

// -----------------------------------------------------------------------------

void IconPanel::OnPaint(wxPaintEvent& WXUNUSED(event))
{
   wxPaintDC dc(this);
   
   dc.SetPen(*wxBLACK_PEN);

   #ifdef __WXMSW__
      // we have to use theme background color on Windows
      wxBrush bgbrush(GetBackgroundColour());
   #else
      wxBrush bgbrush(*wxTRANSPARENT_BRUSH);
   #endif

   // draw icon boxes
   wxRect r = wxRect(0, 0, boxsize+1, boxsize+1);
   int col = 0;
   for (int state = 0; state < 256; state++) {
      if (state == 0) {
         dc.SetBrush(bgbrush);
         dc.DrawRectangle(r);
         dc.SetBrush(wxNullBrush);

      } else if (state < algoinfo[algoindex]->maxstates) {
         wxBitmap** iconmaps;
         if (boxsize > 8)
            iconmaps = algoinfo[algoindex]->icons15x15;
         else
            iconmaps = algoinfo[algoindex]->icons7x7;
         if (iconmaps && iconmaps[state]) {
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(r);
            dc.SetBrush(wxNullBrush);               
            if (algoinfo[algoindex]->gradient) {
               unsigned char red, green, blue;
               GetGradientColor(state, &red, &green, &blue);
               DrawOneIcon(dc, r.x + 1, r.y + 1, iconmaps[state],
                           algoinfo[algoindex]->algor[0],
                           algoinfo[algoindex]->algog[0],
                           algoinfo[algoindex]->algob[0],
                           red, green, blue);
            } else {
               DrawOneIcon(dc, r.x + 1, r.y + 1, iconmaps[state],
                           algoinfo[algoindex]->algor[0],
                           algoinfo[algoindex]->algog[0],
                           algoinfo[algoindex]->algob[0],
                           algoinfo[algoindex]->algor[state],
                           algoinfo[algoindex]->algog[state],
                           algoinfo[algoindex]->algob[state]);
            }
         } else {
            dc.SetBrush(bgbrush);
            dc.DrawRectangle(r);
            dc.SetBrush(wxNullBrush);
         }

      } else {
         // state >= algoinfo[algoindex]->maxstates
         dc.SetBrush(bgbrush);
         dc.DrawRectangle(r);
         dc.SetBrush(wxNullBrush);
      }
      
      col++;
      if (col < NUMCOLS) {
         r.x += boxsize;
      } else {
         r.x = 0;
         r.y += boxsize;
         col = 0;
      }
   }

   dc.SetPen(wxNullPen);
}

// -----------------------------------------------------------------------------

void IconPanel::OnMouseMotion(wxMouseEvent& event)
{
   int col = event.GetX() / boxsize;
   int row = event.GetY() / boxsize;
   int state = row * NUMCOLS + col;
   if (state < 0 || state > 255) {
      statebox->SetLabel(_(" "));
   } else {
      statebox->SetLabel(wxString::Format(_("%d"),state));
   }
}

// -----------------------------------------------------------------------------

void IconPanel::OnMouseExit(wxMouseEvent& WXUNUSED(event))
{
   statebox->SetLabel(_(" "));
}

// -----------------------------------------------------------------------------

// define a modal dialog for loading icons from graphic files

class IconDialog : public wxDialog
{
public:
   IconDialog(wxWindow* parent, int algotype);

   virtual bool TransferDataFromWindow();       // called when user hits OK

   enum {
      // control ids
      ICON7_PANEL = wxID_HIGHEST + 1,
      ICON15_PANEL,
      STATE_BOX,
      FILE_BOX,
      LOAD_BUTT,
      DEFAULT_BUTT
   };

   void CreateControls();        // initialize controls

   int algoindex;                // index into algoinfo

   IconPanel* iconpanel15;       // for displaying 15x15 icons
   IconPanel* iconpanel7;        // for displaying 7x7 icons
   
   wxStaticText* filebox;        // for displaying icon file path

private:
   void UpdateFileName();        // update icon file path

   // event handlers
   void OnButton(wxCommandEvent& event);

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(IconDialog, wxDialog)
   EVT_BUTTON     (wxID_ANY,  IconDialog::OnButton)
END_EVENT_TABLE()

#include "bitmaps/icon_format.xpm"     // shows icon file format

// -----------------------------------------------------------------------------

// these consts are used to get nicely spaced controls on each platform:

const int HGAP = 12;    // space left and right of vertically stacked boxes

// following ensures OK/Cancel buttons are better aligned
#ifdef __WXMAC__
   const int STDHGAP = 0;
#elif defined(__WXMSW__)
   const int STDHGAP = 9;
#else
   const int STDHGAP = 10;
#endif

#if defined(__WXMAC__) && wxCHECK_VERSION(2,8,0)
   // fix wxALIGN_CENTER_VERTICAL bug in wxMac 2.8.0+;
   // only happens when a wxStaticText/wxButton box is next to a wxChoice box
   #define FIX_ALIGN_BUG wxBOTTOM,4
#else
   #define FIX_ALIGN_BUG wxALL,0
#endif

// -----------------------------------------------------------------------------

IconDialog::IconDialog(wxWindow* parent, int algotype)
{
   Create(parent, wxID_ANY, _("Load Icons"), wxDefaultPosition, wxDefaultSize);
   algoindex = algotype;
   CreateControls();
   Centre();
}

// -----------------------------------------------------------------------------

void IconDialog::CreateControls()
{
   wxString note =
           _("Golly can load icon bitmaps from BMP/GIF/PNG/TIFF files.  The picture below ");
   note += _("shows how the bitmaps must be arranged.  The top row contains icons of size ");
   note += _("15x15.  The bottom row contains icons of size 7x7 and is optional; if it isn't ");
   note += _("supplied then Golly will create 7x7 icons by scaling down the 15x15 icons.");
   wxStaticText* notebox = new wxStaticText(this, wxID_STATIC, note);
   notebox->Wrap(NUMCOLS * 16 + 1);
   
   // create bitmap showing icon format
   wxBitmap bmap(icon_format_xpm);
   wxStaticBitmap* bmapbox = new wxStaticBitmap(this, wxID_STATIC, bmap,
                                                wxDefaultPosition,
                                                wxSize(bmap.GetWidth(), bmap.GetHeight()),
                                                0);

   wxStaticText* wdbox = new wxStaticText(this, wxID_STATIC, _("width = 15xN"));

   wxBoxSizer* htbox1 = new wxBoxSizer(wxHORIZONTAL);
   wxBoxSizer* htbox2 = new wxBoxSizer(wxHORIZONTAL);
   htbox1->Add(new wxStaticText(this, wxID_STATIC, _("height = 22 (15+7)")), 0, 0, 0);
   htbox2->Add(new wxStaticText(this, wxID_STATIC, wxEmptyString), 0, 0, 0);
   htbox2->SetMinSize( htbox1->GetMinSize() );

   wxBoxSizer* bitmapbox = new wxBoxSizer(wxHORIZONTAL);
   bitmapbox->Add(htbox1, 0, wxALIGN_CENTER_VERTICAL, 0);
   bitmapbox->AddSpacer(4);
   bitmapbox->Add(bmapbox, 0, wxALIGN_CENTER_VERTICAL, 0);
   bitmapbox->AddSpacer(4);
   bitmapbox->Add(htbox2, 0, wxALIGN_CENTER_VERTICAL, 0);

   // create buttons
   wxButton* loadbutt = new wxButton(this, LOAD_BUTT, _("Load Icon File..."));
   wxButton* defbutt = new wxButton(this, DEFAULT_BUTT, _("Default Icons"));

   filebox = new wxStaticText(this, FILE_BOX, wxEmptyString);
   UpdateFileName();
   
   wxBoxSizer* loadbox = new wxBoxSizer(wxHORIZONTAL);
   loadbox->Add(loadbutt, 0, wxALIGN_CENTER_VERTICAL, 0);
   loadbox->Add(filebox, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, HGAP);

   // create child windows for displaying icons
   iconpanel15 = new IconPanel(this, ICON15_PANEL, 16, algoindex);
   iconpanel7 = new IconPanel(this, ICON7_PANEL, 8, algoindex);

   wxStaticText* statebox = new wxStaticText(this, STATE_BOX, _("999"));
   iconpanel15->statebox = statebox;
   iconpanel7->statebox = statebox;
   wxBoxSizer* hbox15 = new wxBoxSizer(wxHORIZONTAL);
   hbox15->Add(statebox, 0, 0, 0);

   statebox->SetLabel(_(" "));

   wxBoxSizer* botbox = new wxBoxSizer(wxHORIZONTAL);
   botbox->Add(new wxStaticText(this, wxID_STATIC, _("State: ")), 0, wxALIGN_CENTER_VERTICAL, 0);
   botbox->Add(hbox15, 0, wxALIGN_CENTER_VERTICAL, 0);

   wxBoxSizer* vbox15 = new wxBoxSizer(wxVERTICAL);
   vbox15->Add(loadbox, 0, wxLEFT | wxRIGHT, 0);
   vbox15->AddSpacer(10);
   vbox15->Add(iconpanel15, 0, wxLEFT | wxRIGHT, 0);
   vbox15->AddSpacer(5);
   vbox15->Add(botbox, 0, wxLEFT | wxRIGHT, 0);

   wxBoxSizer* vbox7 = new wxBoxSizer(wxVERTICAL);
   vbox7->Add(iconpanel7, 0, wxLEFT | wxRIGHT, 0);

   wxSizer* stdbutts = CreateButtonSizer(wxOK | wxCANCEL);
   wxBoxSizer* stdhbox = new wxBoxSizer( wxHORIZONTAL );
   stdhbox->Add(defbutt, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, HGAP);
   stdhbox->Add(stdbutts, 1, wxGROW | wxALIGN_CENTER_VERTICAL | wxRIGHT, STDHGAP);

   wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
   topSizer->AddSpacer(10);
   topSizer->Add(notebox, 0, wxLEFT | wxRIGHT, HGAP);
   topSizer->AddSpacer(10);
   topSizer->Add(wdbox, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, HGAP);
   topSizer->Add(bitmapbox, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, HGAP);
   topSizer->AddSpacer(20);
   topSizer->Add(vbox15, 0, wxGROW | wxLEFT | wxRIGHT, HGAP);
   // topSizer->AddSpacer(2);
   topSizer->Add(vbox7, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, HGAP);
   topSizer->AddSpacer(10);
   topSizer->Add(stdhbox, 1, wxGROW | wxTOP | wxBOTTOM, 10);
   SetSizer(topSizer);
   topSizer->SetSizeHints(this);    // calls Fit
}

// -----------------------------------------------------------------------------

void IconDialog::UpdateFileName()
{
   if (algoinfo[algoindex]->iconfile.length() > 0) {
      wxString path = algoinfo[algoindex]->iconfile;
      if (path.StartsWith(gollydir)) {
         // remove gollydir from start of path
         path.erase(0, gollydir.length());
      }
      filebox->SetLabel(path);
   } else {
      filebox->SetLabel(_("(currently using default icons)"));
   }
}

// -----------------------------------------------------------------------------

static void ChooseIconFile(wxWindow* parent, wxString& result)
{
   wxString filetypes = _("All files (*)|*");
   filetypes +=         _("|BMP (*.bmp)|*.bmp");
   filetypes +=         _("|GIF (*.gif)|*.gif");
   filetypes +=         _("|PNG (*.png)|*.png");
   filetypes +=         _("|TIFF (*.tiff;*.tif)|*.tiff;*.tif");
   
   wxFileDialog opendlg(parent, _("Choose an icon file"),
                        wxEmptyString, wxEmptyString, filetypes,
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST);

   opendlg.SetDirectory(icondir);
   
   if ( opendlg.ShowModal() == wxID_OK ) {
      result = opendlg.GetPath();
      wxFileName fullpath(result);
      icondir = fullpath.GetPath();
   } else {
      result = wxEmptyString;
   }
}

// -----------------------------------------------------------------------------

void IconDialog::OnButton(wxCommandEvent& event)
{
   int id = event.GetId();
   
   if ( id == LOAD_BUTT ) {
      // ask user to choose an icon file
      wxString result;
      ChooseIconFile(this, result);
      if ( !result.IsEmpty() ) {
         algoinfo[algoindex]->iconfile = result;
         LoadIcons(algoindex);
         UpdateFileName();
         iconpanel15->Refresh(false);
         iconpanel7->Refresh(false);
      }
   
   } else if ( id == DEFAULT_BUTT ) {
      // restore default icons
      algoinfo[algoindex]->iconfile = wxEmptyString;
      LoadIcons(algoindex);
      UpdateFileName();
      iconpanel15->Refresh(false);
      iconpanel7->Refresh(false);
   
   } else {
      // process other buttons like Cancel and OK
      event.Skip();
   }
}

// -----------------------------------------------------------------------------

bool IconDialog::TransferDataFromWindow()
{
   // no need to do any validation
   return true;
}

// -----------------------------------------------------------------------------

void ChangeIcons(algo_type algotype)
{
   AlgoData* ad = algoinfo[algotype];
   wxString oldiconfile = ad->iconfile;

   IconDialog dialog( wxGetApp().GetTopWindow(), algotype );
   if ( dialog.ShowModal() == wxID_OK ) {
      // no need to do anything
   } else {
      // user hit Cancel so restore icon file if it was changed
      if (ad->iconfile != oldiconfile) {
         ad->iconfile = oldiconfile;
         LoadIcons(algotype);
      }
   }
}
