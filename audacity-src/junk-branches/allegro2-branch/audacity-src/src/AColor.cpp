
/**********************************************************************

  Audacity: A Digital Audio Editor

  AColor.cpp

  Dominic Mazzoni


********************************************************************//**

\class AColor
\brief AColor Manages color brushes and pens

It is also a place to document colour usage policy in Audacity

*//********************************************************************/

#include <wx/dc.h>
#include <wx/settings.h>
#include <wx/utils.h>

#include "AColor.h"
#include "Theme.h"
#include "AllThemeResources.h"

bool AColor::inited = false;
wxBrush AColor::lightBrush[2];
wxBrush AColor::mediumBrush[2];
wxBrush AColor::darkBrush[2];
wxPen AColor::lightPen[2];
wxPen AColor::mediumPen[2];
wxPen AColor::darkPen[2];

wxPen AColor::cursorPen;
wxBrush AColor::indicatorBrush[2];
wxPen AColor::indicatorPen[2];
wxPen AColor::playRegionPen[2];
wxBrush AColor::playRegionBrush[2];

wxBrush AColor::muteBrush[2];
wxBrush AColor::soloBrush;

wxBrush AColor::envelopeBrush;
wxPen AColor::envelopePen;
wxPen AColor::WideEnvelopePen;
wxBrush AColor::tooltipBrush;

//
// Draws a focus rectangle (Taken directly from wxWidgets source)
//
void AColor::DrawFocus(wxDC & dc, wxRect & rect)
{
   // draw the pixels manually: note that to behave in the same manner as
   // DrawRect(), we must exclude the bottom and right borders from the
   // rectangle
   wxCoord x1 = rect.GetLeft(),
         y1 = rect.GetTop(),
         x2 = rect.GetRight(),
         y2 = rect.GetBottom();

   dc.SetPen(wxPen(wxT("MEDIUM GREY"), 0, wxSOLID));

   // this seems to be closer than what Windows does than wxINVERT although
   // I'm still not sure if it's correct
   dc.SetLogicalFunction(wxAND_REVERSE);

   wxCoord z;
   for ( z = x1 + 1; z < x2; z += 2 )
      dc.DrawPoint(z, y1);

   wxCoord shift = z == x2 ? 0 : 1;
   for ( z = y1 + shift; z < y2; z += 2 )
      dc.DrawPoint(x2, z);

   shift = z == y2 ? 0 : 1;
   for ( z = x2 - shift; z > x1; z -= 2 )
      dc.DrawPoint(z, y2);

   shift = z == x1 ? 0 : 1;
   for ( z = y2 - shift; z > y1; z -= 2 )
      dc.DrawPoint(x1, z);

   dc.SetLogicalFunction(wxCOPY);
}

void AColor::Bevel(wxDC & dc, bool up, wxRect & r)
{
   if (up)
      AColor::Light(&dc, false);
   else
      AColor::Dark(&dc, false);

   dc.DrawLine(r.x, r.y, r.x + r.width, r.y);
   dc.DrawLine(r.x, r.y, r.x, r.y + r.height);

   if (!up)
      AColor::Light(&dc, false);
   else
      AColor::Dark(&dc, false);

   dc.DrawLine(r.x + r.width, r.y, r.x + r.width, r.y + r.height);
   dc.DrawLine(r.x, r.y + r.height, r.x + r.width + 1, r.y + r.height);
}

void AColor::Light(wxDC * dc, bool selected)
{
   if (!inited)
      Init();
   int index = (int) selected;
   dc->SetBrush(lightBrush[index]);
   dc->SetPen(lightPen[index]);
}

void AColor::Medium(wxDC * dc, bool selected)
{
   if (!inited)
      Init();
   int index = (int) selected;
   dc->SetBrush(mediumBrush[index]);
   dc->SetPen(mediumPen[index]);
}

void AColor::Dark(wxDC * dc, bool selected)
{
   if (!inited)
      Init();
   int index = (int) selected;
   dc->SetBrush(darkBrush[index]);
   dc->SetPen(darkPen[index]);
}

void AColor::CursorColor(wxDC * dc)
{
   if (!inited)
      Init();
   dc->SetLogicalFunction(wxINVERT);
   dc->SetPen(cursorPen);
}

void AColor::IndicatorColor(wxDC * dc, bool recording)
{
   if (!inited)
      Init();
   int index = (int) recording;
   dc->SetPen(indicatorPen[index]);
   dc->SetBrush(indicatorBrush[index]);
}

void AColor::PlayRegionColor(wxDC * dc, bool locked)
{
   if (!inited)
      Init();
   dc->SetPen(playRegionPen[(int)locked]);
   dc->SetBrush(playRegionBrush[(int)locked]);
}

void AColor::Mute(wxDC * dc, bool on, bool selected, bool soloing)
{
   if (!inited)
      Init();
   int index = (int) selected;
   if (on) {
      dc->SetPen(*wxBLACK_PEN);
      dc->SetBrush(muteBrush[(int) soloing]);
   }
   else {
      dc->SetPen(*wxTRANSPARENT_PEN);
      dc->SetBrush(mediumBrush[index]);
   }
}

void AColor::Solo(wxDC * dc, bool on, bool selected)
{
   if (!inited)
      Init();
   int index = (int) selected;
   if (on) {
      dc->SetPen(*wxBLACK_PEN);
      dc->SetBrush(soloBrush);
   }
   else {
      dc->SetPen(*wxTRANSPARENT_PEN);
      dc->SetBrush(mediumBrush[index]);
   }
}

void AColor::Init()
{
   if (inited)
      return;

   wxColour light =
       wxSystemSettings::GetColour(wxSYS_COLOUR_3DHIGHLIGHT);
   wxColour med = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
   wxColour dark =
       wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW);

   theTheme.SetPenColour(   envelopePen,     clrEnvelope );
   theTheme.SetPenColour(   WideEnvelopePen, clrEnvelope );
   theTheme.SetBrushColour( envelopeBrush,   clrEnvelope );

   WideEnvelopePen.SetWidth( 3 );
   
   // These colors were modified to avoid using reserved colors red and green
   // for the buttons.
   theTheme.SetBrushColour( muteBrush[0],      clrMuteButtonActive);      
   theTheme.SetBrushColour( muteBrush[1],      clrMuteButtonVetoed);     
   theTheme.SetBrushColour( soloBrush,         clrMuteButtonActive);     
                                                                         
   theTheme.SetPenColour(   cursorPen,         clrCursorPen);
   theTheme.SetPenColour(   indicatorPen[0],   clrRecordingPen);
   theTheme.SetPenColour(   indicatorPen[1],   clrPlaybackPen);
   theTheme.SetBrushColour( indicatorBrush[0], clrRecordingBrush);        
   theTheme.SetBrushColour( indicatorBrush[1], clrPlaybackBrush);         
                                                                         
   theTheme.SetBrushColour( playRegionBrush[0],clrRulerRecordingBrush);   
   theTheme.SetPenColour(   playRegionPen[0],  clrRulerRecordingPen);     
   theTheme.SetBrushColour( playRegionBrush[1],clrRulerPlaybackBrush);     
   theTheme.SetPenColour(   playRegionPen[1],  clrRulerPlaybackPen);      

   //Determine tooltip color
   tooltipBrush.SetColour( wxSystemSettingsNative::GetColour(wxSYS_COLOUR_INFOBK) );

#if defined(__WXMSW__) || defined(__WXGTK__)
   // unselected
   lightBrush[0].SetColour(light);
   mediumBrush[0].SetColour(med);
   darkBrush[0].SetColour(dark);
   lightPen[0].SetColour(light);
   mediumPen[0].SetColour(med);
   darkPen[0].SetColour(dark);

   // selected
   lightBrush[1].SetColour(204, 204, 255);
   mediumBrush[1].SetColour(200, 200, 214);
   darkBrush[1].SetColour(148, 148, 170);
   lightPen[1].SetColour(204, 204, 255);
   mediumPen[1].SetColour(200, 200, 214);
   darkPen[1].SetColour(0, 0, 0);

#else

#if defined(__WXMAC__)          // && defined(TARGET_CARBON)

   // unselected
   lightBrush[0].SetColour(246, 246, 255);
   mediumBrush[0].SetColour(220, 220, 220);
   darkBrush[0].SetColour(140, 140, 160);
   lightPen[0].SetColour(246, 246, 255);
   mediumPen[0].SetColour(220, 220, 220);
   darkPen[0].SetColour(140, 140, 160);

   // selected
   lightBrush[1].SetColour(204, 204, 255);
   mediumBrush[1].SetColour(180, 180, 192);
   darkBrush[1].SetColour(148, 148, 170);
   lightPen[1].SetColour(204, 204, 255);
   mediumPen[1].SetColour(180, 180, 192);
   darkPen[1].SetColour(148, 148, 170);

#else

   // unselected
   lightBrush[0].SetColour(255, 255, 255);
   mediumBrush[0].SetColour(204, 204, 204);
   darkBrush[0].SetColour(130, 130, 130);
   lightPen[0].SetColour(255, 255, 255);
   mediumPen[0].SetColour(204, 204, 204);
   darkPen[0].SetColour(130, 130, 130);

   // selected
   lightBrush[1].SetColour(204, 204, 255);
   mediumBrush[1].SetColour(180, 180, 192);
   darkBrush[1].SetColour(148, 148, 170);
   lightPen[1].SetColour(204, 204, 255);
   mediumPen[1].SetColour(180, 180, 192);
   darkPen[1].SetColour(0, 0, 0);

#endif

#endif

   inited = true;
}

const int AColor_midicolors[16][3] = {
   {255, 102, 102},             // 1=salmon
   {204, 0, 0},                 // 2=red
   {255, 117, 23},              // 3=orange
   {255, 255, 0},               // 4=yellow    
   {0, 204, 0},                 // 5=green
   {0, 204, 204},               // 6=turquoise
   {0, 0, 204},                 // 7=blue
   {153, 0, 255},               // 8=blue-violet

   {140, 97, 54},               // 9=brown
   {120, 120, 120},             // 10=gray (drums)
   {255, 175, 40},              // 11=lt orange
   {102, 255, 102},             // 12=lt green
   {153, 255, 255},             // 13=lt turquoise
   {153, 153, 255},             // 14=lt blue
   {204, 102, 255},             // 15=lt blue-violet
   {255, 51, 204}
};                              // 16=lt red-violet

void AColor::MIDIChannel(wxDC * dc, int channel /* 1 - 16 */ )
{
   if (channel >= 1 && channel <= 16) {
      const int *colors = AColor_midicolors[channel - 1];

      dc->SetPen(wxPen(wxColour(colors[0],
                                colors[1], colors[2]), 1, wxSOLID));
      dc->SetBrush(wxBrush(wxColour(colors[0],
                                    colors[1], colors[2]), wxSOLID));
   } else {
      dc->SetPen(wxPen(wxColour(153, 153, 153), 1, wxSOLID));// DONT-THEME Midi, unused.
      dc->SetBrush(wxBrush(wxColour(153, 153, 153), wxSOLID));
   }

}

void AColor::LightMIDIChannel(wxDC * dc, int channel /* 1 - 16 */ )
{
   if (channel >= 1 && channel <= 16) {
      const int *colors = AColor_midicolors[channel - 1];

      dc->SetPen(wxPen(wxColour(127 + colors[0] / 2,
                                127 + colors[1] / 2,
                                127 + colors[2] / 2), 1, wxSOLID));
      dc->SetBrush(wxBrush(wxColour(127 + colors[0] / 2,
                                    127 + colors[1] / 2,
                                    127 + colors[2] / 2), wxSOLID));
   } else {
      dc->SetPen(wxPen(wxColour(204, 204, 204), 1, wxSOLID));
      dc->SetBrush(wxBrush(wxColour(204, 204, 204), wxSOLID));
   }

}

void AColor::DarkMIDIChannel(wxDC * dc, int channel /* 1 - 16 */ )
{
   if (channel >= 1 && channel <= 16) {
      const int *colors = AColor_midicolors[channel - 1];

      dc->SetPen(wxPen(wxColour(colors[0] / 2,
                                colors[1] / 2,
                                colors[2] / 2), 1, wxSOLID));
      dc->SetBrush(wxBrush(wxColour(colors[0] / 2,
                                    colors[1] / 2,
                                    colors[2] / 2), wxSOLID));
   } else {
      dc->SetPen(wxPen(wxColour(102, 102, 102), 1, wxSOLID));
      dc->SetBrush(wxBrush(wxColour(102, 102, 102), wxSOLID));
   }

}

void GetColorGradient(float value,
                      bool selected,
                      bool grayscale,
                      unsigned char *red,
                      unsigned char *green, unsigned char *blue)
{
   float r, g, b;

   if (grayscale) {
      r = g = b = 0.84 - 0.84 * value;
   } else {
      const int gsteps = 4;
      float gradient[gsteps + 1][3] = {
         {float(0.75), float(0.75), float(0.75)},    // lt gray
         {float(0.30), float(0.60), float(1.00)},    // lt blue
         {float(0.90), float(0.10), float(0.90)},    // violet
         {float(1.00), float(0.00), float(0.00)},    // red
         {float(1.00), float(1.00), float(1.00)}     // white
      };                        

      int left = int (value * gsteps);
      int right = (left == gsteps ? gsteps : left + 1);

      float rweight = (value * gsteps) - left;
      float lweight = 1.0 - rweight;

      r = (gradient[left][0] * lweight) + (gradient[right][0] * rweight);
      g = (gradient[left][1] * lweight) + (gradient[right][1] * rweight);
      b = (gradient[left][2] * lweight) + (gradient[right][2] * rweight);
   }

   if (selected) {
      r *= 0.77f;
      g *= 0.77f;
      b *= 0.885f;
   }

   *red = (unsigned char) (255 * r);
   *green = (unsigned char) (255 * g);
   *blue = (unsigned char) (255 * b);
}

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: b3fbaf98-02a1-4baa-bccc-e5dc0ffe007a

