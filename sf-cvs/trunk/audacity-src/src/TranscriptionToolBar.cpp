/**********************************************************************

  Audacity: A Digital Audio Editor

  TranscriptionToolBar.cpp

  Shane T. Mueller

  This class manages the miniframe window (aka floating window)
  which contains the tool selection (ibeam, envelope, move, zoom),
  the play/stop/record buttons, and the volume control.  All of the
  controls in this window were custom-written for Audacity - they
  are not native controls on any platform - however, it is intended
  that the images could be easily replaced to allow "skinning" or
  just customization to match the look and feel of each platform.

**********************************************************************/

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/brush.h>
#include <wx/intl.h>
#endif

#include <wx/image.h>

#include <cmath>
#include <iostream>

#include "widgets/AButton.h"
#include "widgets/ASlider.h"

#include "TranscriptionToolBar.h"
#include "AudioIO.h"
#include "Project.h"
#include "WaveTrack.h"
#include "VoiceKey.h"
#include "ImageManipulation.h"
#include "LabelTrack.h"



#ifdef __WXMAC__
#define TRANSCRIPTIONTOOLBAR_HEIGHT_OFFSET 0
#endif

#ifdef __WXGTK__
#define TRANSCRIPTIONTOOLBAR_HEIGHT_OFFSET 22
#endif

#ifdef __WXMSW__
#define TRANSCRIPTIONTOOLBAR_HEIGHT_OFFSET 25
#endif


#include "../images/TranscriptionButtons.h"


const int BUTTON_WIDTH = 27;
const int SEPARATOR_WIDTH = 14;


using std::cout;
using std::flush;
using std::endl;
///////////////////////////////////////////
///  Methods for TranscriptionToolBar
///////////////////////////////////////////
 

BEGIN_EVENT_TABLE(TranscriptionToolBar, wxWindow)
  EVT_PAINT(TranscriptionToolBar::OnPaint)
  EVT_CHAR(TranscriptionToolBar::OnKeyEvent)
  EVT_COMMAND_RANGE(TTB_StartOn, TTB_StartOn,
		    wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnStartOn)
  EVT_COMMAND_RANGE(TTB_StartOff, TTB_StartOff,
		    wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnStartOff)
  EVT_COMMAND_RANGE(TTB_EndOn, TTB_EndOn,
		    wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnEndOn)
  EVT_COMMAND_RANGE(TTB_EndOff, TTB_EndOff,
		    wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnEndOff)
  EVT_COMMAND_RANGE(TTB_Calibrate, TTB_Calibrate,
		    wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnCalibrate)
  EVT_COMMAND_RANGE(TTB_AutomateSelection, TTB_AutomateSelection,
		    wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnAutomateSelection)
  EVT_COMMAND_RANGE(TTB_MakeLabel, TTB_MakeLabel,
		    wxEVT_COMMAND_BUTTON_CLICKED, TranscriptionToolBar::OnMakeLabel)
   EVT_SLIDER(TTB_SensitivitySlider, TranscriptionToolBar::OnSensitivitySlider)
END_EVENT_TABLE()
 
  ////Standard Constructor
  TranscriptionToolBar::TranscriptionToolBar(wxWindow * parent):
    ToolBar(parent, -1, wxPoint(1,1), wxSize(377,27))
{
  InitializeTranscriptionToolBar();
}

//Full-service Constructor
TranscriptionToolBar::TranscriptionToolBar(wxWindow * parent, wxWindowID id,
					   const wxPoint & pos,
					   const wxSize & size):
  ToolBar(parent, id, pos, size)
{

  InitializeTranscriptionToolBar();
}

void TranscriptionToolBar::InitializeTranscriptionToolBar()
{

  mIdealSize = wxSize(550, 27);


  vk = new VoiceKey();
  mBackgroundBrush.SetColour(wxColour(204, 204, 204));
  mBackgroundPen.SetColour(wxColour(204, 204, 204));
  
  mBackgroundBitmap = NULL;
  mBackgroundHeight = 0;
  mBackgroundWidth = 0;
 
  mNumDividers = 0;

  MakeButtons();
 
  mButtons[TTB_StartOn]->Disable();
  mButtons[TTB_StartOff]->Disable();
  mButtons[TTB_EndOn]->Disable();
  mButtons[TTB_EndOff]->Disable();
  mButtons[TTB_Calibrate]->Enable();
  mButtons[TTB_AutomateSelection]->Disable();
  mButtons[TTB_MakeLabel]->Enable();
}



// This is a convenience function that allows for button creation in
// MakeButtons() with fewer arguments

void TranscriptionToolBar::AddButton(const char **fg, const char **disabled, const char **alpha,
				     int id, const char *tooltip)
{

  // Windows (TM) has a little extra room for some reason, so the top of the
  // buttons should be a little lower.
  int buttonTop = 0;
#ifdef __WXMSW__
  buttonTop=0;
#endif

  mButtons[id] = ToolBar::MakeButton(
				     upImage, downImage, hiliteImage, fg,
				     disabled, alpha,
				     wxWindowID(id), wxPoint(mButtonPos, buttonTop),
				     false /*No buttons should process down events.*/,
				     wxSize(BUTTON_WIDTH, BUTTON_WIDTH), 0, 0);
#if wxUSE_TOOLTIPS // Not available in wxX11
  mButtons[id]->SetToolTip(tooltip);
#endif

  mButtonPos += BUTTON_WIDTH;
  mDividers[mNumDividers++] = mButtonPos++;
}




void TranscriptionToolBar::MakeButtons()
{
  wxColour newColour =
    wxSystemSettings::GetSystemColour(wxSYS_COLOUR_3DFACE);
  wxColour baseColour = wxColour(204, 204, 204);

  wxImage *upOriginal = new wxImage(wxBitmap(Up).ConvertToImage());
  wxImage *downOriginal = new wxImage(wxBitmap(Down).ConvertToImage());
  wxImage *hiliteOriginal = new wxImage(wxBitmap(Hilite).ConvertToImage());

#ifdef __WXGTK__
  /* dmazzoni: hack to get around XPM color bugs in GTK */
  unsigned char *data = upOriginal->GetData();
  baseColour.Set(data[28 * 3], data[28 * 3 + 1], data[28 * 3 + 2]);
#endif

  upImage = ChangeImageColour(upOriginal, baseColour, newColour);
  downImage = ChangeImageColour(downOriginal, baseColour, newColour);
  hiliteImage = ChangeImageColour(hiliteOriginal, baseColour, newColour);


  mButtonPos = 0;
  AddButton(StartOn,     StartOnDisabled,   StartOnAlpha,   TTB_StartOn,   _("Adjust left selection to  next onset"));
  AddButton(EndOn,       EndOnDisabled,     EndOnAlpha,     TTB_EndOn,     _("Adjust right selection to previous offset"));
  AddButton(StartOff,    StartOffDisabled,  StartOffAlpha,  TTB_StartOff,  _("Adjust left selection to next offset"));
  AddButton(EndOff,      EndOffDisabled,    EndOffAlpha,    TTB_EndOff,    _("Adjust right selection to previous onset"));
  AddButton(CalibrateUp, CalibrateDisabled, CalibrateAlpha, TTB_Calibrate, _("Calibrate voicekey"));
  AddButton(AutomateSelection,   AutomateSelectionDisabled,   AutomateSelectionAlpha,   TTB_AutomateSelection,   _("Automatically make labels from words\n"));
  AddButton(MakeTag,     MakeTagDisabled,   MakeTagAlpha,   TTB_MakeLabel,  _("Make label from selection.\n"));
  
  mSensitivitySlider = new ASlider(this, TTB_SensitivitySlider, _("Adjust Sensitivity"),
                                   wxPoint(mButtonPos,0),wxSize(100,28));
  
  
  mSensitivitySlider->Set(.5);

}
  

TranscriptionToolBar::~TranscriptionToolBar()
{
   

   delete vk;	
   
   if (mBackgroundBitmap) delete mBackgroundBitmap;
   if(mSensitivitySlider)delete mSensitivitySlider;
   
   for (int i=0; i<TTBNumButtons; i++)
      if(mButtons[i]) delete mButtons[i];
   

}


//This handles key-stroke events????
void TranscriptionToolBar::OnKeyEvent(wxKeyEvent & event)
{
  if (event.ControlDown()) {
    event.Skip();
    return;
  }
  
  if (event.KeyCode() == WXK_SPACE)
    {
      if (gAudioIO->IsBusy()) {
	/*Do Stuff Here*/
	
      } else {
	
	/*Do other stuff Here*/
      }
    }
}



//This changes the state of the various buttons
void TranscriptionToolBar::SetButton(bool down, AButton* button)
{
  if (down)
    button->PushDown();
  else
    button->PopUp();
}



void TranscriptionToolBar::GetSamples(WaveTrack *t, sampleCount *s0, sampleCount *slen)
{
  // GetSamples attempts to translate the start and end selection markers into sample indices 
  // These selection numbers are doubles.


  //First, get the current selection. It is part of the mViewInfo, which is
  //part of the project
   
  AudacityProject *p = GetActiveProject();
  if(p) {
    double start = p->GetSel0();
    double end = p->GetSel1();
   	   
    sampleCount ss0 = sampleCount( (start - t->GetOffset()) * t->GetRate() );
    sampleCount ss1 = sampleCount( (end - t->GetOffset()) * t->GetRate() );

    if (start < t->GetOffset())
      ss0 = 0;
#if 0
    if (ss1 >= t->GetNumSamples())
      ss1 = t->GetNumSamples();
#endif

    if (ss1 < ss0)
      ss1 = ss0;
	    

    *s0 = ss0;
    *slen = ss1 - ss0;
  }
}

void TranscriptionToolBar::OnStartOn()
{
  //If IO is busy, abort immediately
  if (gAudioIO->IsBusy()){
    SetButton(false,mButtons[TTB_StartOn]);
    return;
  }
	
  vk->AdjustThreshold(GetSensitivity());
  AudacityProject *p = GetActiveProject();
	
 
  TrackList *tl = p->GetTracks();
  TrackListIterator iter(tl);

  Track *t = iter.First();   //Make a track
  if(t) 
     {		
        sampleCount start,len;
        GetSamples((WaveTrack*)t, &start,&len);
        
        //Adjust length to end if selection is null
        //if(len == 0)
        //len = (WaveTrack*)t->GetSequence()->GetNumSamples()-start;
        
        sampleCount newstart = vk->OnForward(*(WaveTrack*)t,start,len);
        double newpos = newstart / ((WaveTrack*)t)->GetRate();
        
        p->SetSel0(newpos);
        p->RedrawProject();

        SetButton(false, mButtons[TTB_StartOn]); 
    }
}

void TranscriptionToolBar::OnStartOff()
{
   //If IO is busy, abort immediately
   if (gAudioIO->IsBusy()){
      SetButton(false,mButtons[TTB_StartOff]);
      return;
   }
   vk->AdjustThreshold(GetSensitivity());
   AudacityProject *p = GetActiveProject();
   
   TrackList *tl = p->GetTracks();
   TrackListIterator iter(tl);
   
   SetButton(false, mButtons[TTB_StartOff]);
   Track *t = iter.First();   //Make a track
   if(t) 
      {		
         sampleCount start,len;
         GetSamples((WaveTrack*)t, &start,&len);
         
         //Adjust length to end if selection is null
         //if(len == 0)
         //len = (WaveTrack*)t->GetSequence()->GetNumSamples()-start;
         
         sampleCount newstart = vk->OffForward(*(WaveTrack*)t,start,len);
         double newpos = newstart / ((WaveTrack*)t)->GetRate();
         
         p->SetSel0(newpos);
         p->RedrawProject();
         
         SetButton(false, mButtons[TTB_StartOn]); 
      }
   
	
}

void TranscriptionToolBar::OnEndOn()
{
	
  //If IO is busy, abort immediately
  if (gAudioIO->IsBusy()){
    SetButton(false,mButtons[TTB_EndOn]);
    return;
  }
  
  vk->AdjustThreshold(GetSensitivity());
  AudacityProject *p = GetActiveProject();
  TrackList *tl = p->GetTracks();
  TrackListIterator iter(tl);



  Track *t = iter.First();   //Make a track
  if(t) 
     {		
        sampleCount start,len;
        GetSamples((WaveTrack*)t, &start,&len);
        
        //Adjust length to end if selection is null
        if(len == 0)
           {
              len = start;
              start = 0;
           }
        sampleCount newEnd = vk->OnBackward(*(WaveTrack*)t,start+ len,len);
        double newpos = newEnd / ((WaveTrack*)t)->GetRate();
        
        p->SetSel1(newpos);
        p->RedrawProject();
        
        SetButton(false, mButtons[TTB_EndOn]);		

     }
}



void TranscriptionToolBar::OnEndOff()
{
	
  //If IO is busy, abort immediately
  if (gAudioIO->IsBusy()){
    SetButton(false,mButtons[TTB_EndOff]);
    return;
  }
  vk->AdjustThreshold(GetSensitivity());
  AudacityProject *p = GetActiveProject();
  TrackList *tl = p->GetTracks();
  TrackListIterator iter(tl);

  Track *t = iter.First();   //Make a track
  if(t) 
     {		
        sampleCount start,len;
        GetSamples((WaveTrack*)t, &start,&len);
        
        //Adjust length to end if selection is null
        if(len == 0)
           {
              len = start;
              start = 0;
           }
        sampleCount newEnd = vk->OffBackward(*(WaveTrack*)t,start+ len,len);
        double newpos = newEnd / ((WaveTrack*)t)->GetRate();
        
        p->SetSel1(newpos);
        p->RedrawProject();
        
        SetButton(false, mButtons[TTB_EndOff]);
    }

}




void TranscriptionToolBar::OnCalibrate()
{
  //If IO is busy, abort immediately
  if (gAudioIO->IsBusy()){
    SetButton(false,mButtons[TTB_Calibrate]);
    return;
  }


  AudacityProject *p = GetActiveProject();

  TrackList *tl = p->GetTracks();
  TrackListIterator iter(tl);
  Track *t = iter.First();   //Get a track
  
  if(t) 
     {		
        sampleCount start,len;
        GetSamples((WaveTrack*)t, &start,&len);
        
        vk->CalibrateNoise(*((WaveTrack*)t),start,len);
        vk->AdjustThreshold(3);
		
     mButtons[TTB_StartOn]->Enable();
     mButtons[TTB_StartOff]->Enable();
     mButtons[TTB_EndOn]->Enable();
     mButtons[TTB_EndOff]->Enable();
     //mThresholdSensitivity->Set(3);

     SetButton(false,mButtons[TTB_Calibrate]);    
     }
  
  mButtons[TTB_StartOn]->Enable();
  mButtons[TTB_StartOff]->Enable();
  mButtons[TTB_EndOn]->Enable();
  mButtons[TTB_EndOff]->Enable();
  mButtons[TTB_AutomateSelection]->Enable();
  
}

//This automates selection through a selected region,
//selecting its best guess for words and creating labels at those points.

void TranscriptionToolBar::OnAutomateSelection()
{

	
   //If IO is busy, abort immediately
   if (gAudioIO->IsBusy())
      {
         SetButton(false,mButtons[TTB_EndOff]);
         return;
      }
   vk->AdjustThreshold(GetSensitivity());
   AudacityProject *p = GetActiveProject();
   TrackList *tl = p->GetTracks();
   TrackListIterator iter(tl);
   
   Track *t = iter.First();   //Make a track
   if(t) 
      {		
         sampleCount start,len;
         GetSamples((WaveTrack*)t, &start,&len);
         
         //Adjust length to end if selection is null
         if(len == 0)
            {
               len = start;
               start = 0;
            }
         int lastlen = 0;
         sampleCount newStart, newEnd;
         double newStartPos, newEndPos;
         
         int minWordSize = (int)(((WaveTrack*)t)->GetRate() * .05);
         
         //Continue until we have processed the entire
         //region, or we are making no progress.
         while(len > 0 && lastlen != len)
            {
               
               lastlen = len;
               
               newStart = vk->OnForward(*(WaveTrack*)t,start,len);
               
               //Adjust len by the new start position
               len -= (newStart - start);
               
               //Adjust len by the minimum word size
               len -= minWordSize;
               
               

               //OK, now we have found a new starting point.  A 'word' should be at least 
               //50 ms long, so jump ahead 50 ms.
               
               newEnd   = vk->OffForward(*(WaveTrack*)t,newStart+minWordSize, len);
               
               //Adjust len by the new wod end
               len -= (newEnd - newStart);
               
               //Calculate the start and end of the words, in seconds
               newStartPos = newStart / ((WaveTrack*)t)->GetRate();
               newEndPos = newEnd / ((WaveTrack*)t)->GetRate();
               
               
               //Increment
               start = newEnd;
               
               p->DoAddLabel(newStartPos, newEndPos);
               p->RedrawProject();
               
               SetButton(false, mButtons[TTB_AutomateSelection]);
            }
         
      }

}





void TranscriptionToolBar::OnMakeLabel()
{
   AudacityProject *p = GetActiveProject();
   SetButton(false, mButtons[TTB_MakeLabel]);  
   p->DoAddLabel(p->GetSel0(),  p->GetSel1());
 
}


//This returns a double z-score between 0 and 10.
double TranscriptionToolBar::GetSensitivity()
{
   return (double)mSensitivity;
}




void TranscriptionToolBar::OnPaint(wxPaintEvent & evt)
{
  wxPaintDC dc(this);

  int width, height;
  GetSize(&width, &height);


#if defined(__WXMAC__)          // && defined(TARGET_CARBON)

  if (mBackgroundWidth < width) {
    if (mBackgroundBitmap)
      delete mBackgroundBitmap;

    mBackgroundBitmap = new wxBitmap(width, height);

    wxMemoryDC memDC;
    memDC.SelectObject(*mBackgroundBitmap);

    int y;
    memDC.SetPen(wxPen(wxColour(231, 231, 231), 1, wxSOLID));
    for (y = 0; y < height; y += 4)
      memDC.DrawLine(0, y, width, y);
    memDC.SetPen(wxPen(wxColour(239, 239, 239), 1, wxSOLID));
    for (y = 1; y < height; y += 2)
      memDC.DrawLine(0, y, width, y);
    memDC.SetPen(wxPen(wxColour(255, 255, 255), 1, wxSOLID));
    for (y = 2; y < height; y += 4)
      memDC.DrawLine(0, y, width, y);

  }

  wxMemoryDC memDC;
  memDC.SelectObject(*mBackgroundBitmap);

  dc.Blit(0, 0, width, height, &memDC, 0, 0, wxCOPY, FALSE);

#else

  dc.SetBrush(mBackgroundBrush);
  dc.SetPen(mBackgroundPen);
  dc.DrawRectangle(0, 0, width, height);

  dc.SetPen(*wxBLACK_PEN);

#endif
}

void TranscriptionToolBar::OnSensitivitySlider(wxCommandEvent & event)
{
   mSensitivity = (mSensitivitySlider->Get())* 5;
}

void TranscriptionToolBar::EnableDisableButtons()
{
   AudacityProject *p = GetActiveProject();
   if (!p) return;
  // Is anything selected?
   bool selection = false;
   TrackListIterator iter(p->GetTracks());
   for (Track *t = iter.First(); t; t = iter.Next())
      if (t->GetSelected()) {
         selection = true;
         break;
      }
   selection &= (p->GetSel0() < p->GetSel1());

   mButtons[TTB_Calibrate]->SetEnabled(selection);
   
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
// arch-tag: 

