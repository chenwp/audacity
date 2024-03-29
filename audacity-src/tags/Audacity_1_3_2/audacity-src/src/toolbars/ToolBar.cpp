/**********************************************************************

  Audacity: A Digital Audio Editor

  ToolBar.cpp
  
  Dominic Mazzoni
  Shane T. Mueller
  Leland Lucius

  See ToolBar.h for details.

*******************************************************************//**

\file ToolBar.cpp

  Implements ToolBar

*//*******************************************************************//**

\class ToolBar
\brief Works with ToolManager and ToolDock to provide a dockable window 
in which buttons can be placed.

*//**********************************************************************/

#include "../Audacity.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/defs.h>
#include <wx/gdicmn.h>
#include <wx/image.h>
#include <wx/intl.h>
#include <wx/sizer.h>
#include <wx/sysopt.h>
#include <wx/window.h>
#endif  /*  */

#include "ToolBar.h"

#include "../AllThemeResources.h"
#include "../AColor.h"
#include "../ImageManipulation.h"
#include "../Project.h"
#include "../Theme.h"
#include "../widgets/AButton.h"
#include "../widgets/Grabber.h"

////////////////////////////////////////////////////////////
/// Methods for ToolBar
////////////////////////////////////////////////////////////

//
// Define class to RTTI
//
IMPLEMENT_CLASS( ToolBar, wxPanel );

//
// Custom event
//
DEFINE_EVENT_TYPE(EVT_TOOLBAR_UPDATED)

//
// Width of the resize grab area
//
#define RWIDTH 4

//
// Event table
//
BEGIN_EVENT_TABLE( ToolBar, wxPanel )
   EVT_PAINT( ToolBar::OnPaint )
   EVT_ERASE_BACKGROUND( ToolBar::OnErase )
   EVT_LEFT_DOWN( ToolBar::OnLeftDown )
   EVT_LEFT_UP( ToolBar::OnLeftUp )
   EVT_MOTION( ToolBar::OnMotion )
END_EVENT_TABLE()  

//
// Constructor
//
ToolBar::ToolBar( int type,
                  const wxString &label,
                  bool resizable )
: wxPanel()
{
   // Save parameters
   mType = type;
   mLabel = label;
   mTitle.Printf( _("Audacity %s ToolBar"), mLabel.c_str() );
   mResizable = resizable;

   // Initialize everything
   mParent = NULL;
   mHSizer = NULL;
   mSpacer = NULL;
   mDocked = true;
   mVisible = false;
}

//
// Destructor
//
ToolBar::~ToolBar()
{
}

//
// Returns the toolbar title
//
wxString ToolBar::GetTitle()
{
   return mTitle;
}

//
// Returns the toolbar label
//
wxString ToolBar::GetLabel()
{
   return mLabel;
}

//
// Returns the toolbar type
//
int ToolBar::GetType()
{
   return mType;
}

//
// Returns whether the toolbar is resizable or not
//
bool ToolBar::IsResizable()
{
   return mResizable;
}

//
// Returns the dock state of the toolbar
//
bool ToolBar::IsDocked()
{
   return mDocked;
}

//
// Returns the visibility of the toolbar
//
bool ToolBar::IsVisible()
{
   return mVisible;
}

//
// Show or hide the toolbar
//
bool ToolBar::Expose( bool show )
{
   bool was = mVisible;

   mVisible = show;

   if( IsDocked() )
   {
      Show( show );
   }
   else
   {
      GetParent()->Show( show );
   }

   return was;
}

//
// Initialize the toolbar
//
void ToolBar::Create( wxWindow *parent )
{
   // Save parameters
   mParent = parent;

   // Create the window and label it
   wxPanel::Create( mParent,
                    mType,
                    wxDefaultPosition,
                    wxDefaultSize,
                    wxNO_BORDER | wxTAB_TRAVERSAL,
                    mTitle );
   SetLabel( mLabel );

   // Go do the rest of the creation
   ReCreateButtons();

   // Let the user see it in all its glory
   Show();
   mVisible = true;
}

void ToolBar::ReCreateButtons()
{
   // SetSizer(NULL) detaches mHSizer and deletes it.
   // Do not use Detach() here, as that attempts to detach mHSizer from itself!
   SetSizer( NULL );

   // Get rid of any children we may have
   DestroyChildren();

   // Create the main sizer
   wxBoxSizer *ms = new wxBoxSizer( wxHORIZONTAL );

   // Create the grabber and add it to the main sizer
   mGrabber = new Grabber( this, mType );
   ms->Add( mGrabber, 0, wxEXPAND | wxALIGN_LEFT | wxALIGN_TOP | wxRIGHT, 1 );

   // Use a box sizer for laying out controls
   mHSizer = new wxBoxSizer( wxHORIZONTAL );
   ms->Add( mHSizer, 1, wxEXPAND );

   // (Re)Establish dock state
   SetDocked( IsDocked(), false );

   // Go add all the rest of the gadgets
   Populate();

   // Add some space for the resize border
   if( IsResizable() )
   {
      mSpacer = ms->Add( RWIDTH, 1 );
   }

   // Set the sizer and do initial layout
   SetSizerAndFit( ms );
   Layout();

   // Recalculate the height to be a multiple of toolbarSingle
#define tbs ( toolbarSingle + toolbarGap )
   wxSize sz = GetSize();
   sz.y = ( ( ( sz.y + tbs ) / tbs ) * tbs ) - 1;
#undef tbs

   // Set the true AND minimum sizes and do final layout
   SetBestFittingSize(sz);
   Layout();
}

ToolDock *ToolBar::GetDock()
{
   return (ToolDock *)mParent;
};

//
// Toggle the docked/floating state
//
void ToolBar::SetDocked( bool dock, bool pushed )
{
   // Remember it
   mDocked = dock;

   // Change the tooltip of the grabber
#if wxUSE_TOOLTIPS
   mGrabber->SetToolTip( GetTitle() );
#endif

   // Set the grabber button state
   mGrabber->PushButton( pushed );
}

//
// Notify parent of changes
//
void ToolBar::Updated()
{
   wxCommandEvent e( EVT_TOOLBAR_UPDATED, GetId() );
   mParent->GetEventHandler()->AddPendingEvent( e );
}

//
// Returns a pointer to the main sizer
//
wxBoxSizer *ToolBar::GetSizer()
{
   return mHSizer;
}

//
// Add a window to the main sizer
//
void ToolBar::Add( wxWindow *window,
                   int proportion,
                   int flag,
                   int border,
                   wxObject* userData )
{
   mHSizer->Add( window,
                 proportion,
                 flag,
                 border,
                 userData );
}

//
// Add a child sizer to the main sizer
//
void ToolBar::Add( wxSizer *sizer,
                   int proportion,
                   int flag,
                   int border,
                   wxObject* userData )
{
   mHSizer->Add( sizer,
                 proportion,
                 flag,
                 border,
                 userData );
}

//
// Add some space to the main sizer
//
void ToolBar::Add( int width,
                   int height,
                   int proportion,
                   int flag,
                   int border,
                   wxObject* userData )
{
   mHSizer->Add( width,
                 height,
                 proportion,
                 flag,
                 border,
                 userData );
}

//
// Adds a spacer to the main sizer
//
void ToolBar::AddSpacer( int size )
{
   mHSizer->AddSpacer( size );
}

//
// Adds a strechable spacer to the main sizer
//
void ToolBar::AddStretchSpacer( int prop )
{
   mHSizer->AddStretchSpacer( prop );
}

//
// Detach a window from the main sizer
//
void ToolBar::Detach( wxWindow *window )
{
   mHSizer->Detach( window );
}

//
// Detach a child sizer from the main sizer
//
void ToolBar::Detach( wxSizer *sizer )
{
   mHSizer->Detach( sizer );
}

void ToolBar::MakeMacRecoloredImage(teBmps eBmpOut, teBmps eBmpIn )
{
   theTheme.ReplaceImage( eBmpOut, &theTheme.Image( eBmpIn ));
}

void ToolBar::MakeRecoloredImage( teBmps eBmpOut, teBmps eBmpIn )
{
   wxImage * pPattern;
   wxImage * pSrc = &theTheme.Image( eBmpIn );
   wxColour newColour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
   wxColour baseColour = wxColour(204, 204, 204);

   pPattern = ChangeImageColour( pSrc,     baseColour, newColour );
   theTheme.ReplaceImage( eBmpOut, pPattern);
   delete pPattern;
}

void ToolBar:: MakeButtonBackgroundsLarge()
{
#ifdef USE_AQUA_THEME
   MakeMacRecoloredImage( bmpRecoloredUpLarge,     bmpMacUpButton );
   MakeMacRecoloredImage( bmpRecoloredDownLarge,   bmpMacDownButton );
   MakeMacRecoloredImage( bmpRecoloredHiliteLarge, bmpMacHiliteButton );
#else
   MakeRecoloredImage( bmpRecoloredUpLarge,     bmpUpButtonLarge );
   MakeRecoloredImage( bmpRecoloredDownLarge,   bmpDownButtonLarge );
   MakeRecoloredImage( bmpRecoloredHiliteLarge, bmpHiliteButtonLarge );
#endif
}

void ToolBar::MakeButtonBackgroundsSmall()
{
#ifdef USE_AQUA_THEME
   MakeMacRecoloredImage( bmpRecoloredUpSmall,     bmpMacUpButtonSmall );
   MakeMacRecoloredImage( bmpRecoloredDownSmall,   bmpMacDownButtonSmall );
   MakeMacRecoloredImage( bmpRecoloredHiliteSmall, bmpMacHiliteButtonSmall );
#else
   MakeRecoloredImage( bmpRecoloredUpSmall,     bmpUpButtonSmall );
   MakeRecoloredImage( bmpRecoloredDownSmall,   bmpDownButtonSmall );
   MakeRecoloredImage( bmpRecoloredHiliteSmall, bmpHiliteButtonSmall );
#endif
}

/// Makes a button and its four different state bitmaps
/// @param eUp        Background for when button is Up.
/// @param eDown      Background for when button is Down.
/// @param eHilite    Background for when button is Hilit.
/// @param eStandard  Foreground when enabled.
/// @param eDisabled  Foreground when disabled.
/// @param id         Windows Id.
/// @param placement  Placement position
/// @param processdownevents true iff button handles down events.
/// @param size       Size of the background.
AButton * ToolBar::MakeButton(teBmps eUp,
                              teBmps eDown,
                              teBmps eHilite,
                              teBmps eStandard,
                              teBmps eDisabled,
                              wxWindowID id,
                              wxPoint placement,
                              bool processdownevents, 
                              wxSize size) 
{
   int xoff = (size.GetWidth() - theTheme.Image(eStandard).GetWidth())/2;
   int yoff = (size.GetHeight() - theTheme.Image(eStandard).GetHeight())/2;
   
   wxImage * up2        = OverlayImage(eUp,     eStandard, xoff, yoff);
   wxImage * hilite2    = OverlayImage(eHilite, eStandard, xoff, yoff);
   wxImage * down2      = OverlayImage(eDown,   eStandard, xoff + 1, yoff + 1);
   wxImage * disable2   = OverlayImage(eUp,     eDisabled, xoff, yoff);

   AButton * button =
      new AButton(this, id, placement, size, up2, hilite2, down2,
            disable2, processdownevents);

   delete up2;
   delete down2;
   delete hilite2;
   delete disable2;
   return button;
}


//
// This changes the state a button (from up to down or vice versa)
//
void ToolBar::SetButton( bool down, AButton * button )
{
   if( down )
   {
      button->PushDown();
   }
   else
   {
      button->PopUp();
   }
}

//
// Handle background erasure
//
void ToolBar::OnErase( wxEraseEvent & event )
{
   // Ignore it to prevent flashing
}

//
// This draws the background of a toolbar
//
void ToolBar::OnPaint( wxPaintEvent & event )
{
   wxPaintDC dc( (wxWindow *) event.GetEventObject() );

   // Start with a clean background
   //
   // Under GTK, clearing will cause the background to be white and
   // rather than setting a background color, just bypass the clear.
#if !defined(__WXGTK__)
   dc.Clear();
#endif

   // Go repaint the rest
   Repaint( &dc );

   if( IsResizable() && IsDocked() )
   {
      wxSize sz = GetSize();

      AColor::Dark( &dc, false );
      dc.DrawLine( sz.x - 4,  0, sz.x - 4, sz.y );
      dc.DrawLine( sz.x - 1,  0, sz.x - 1, sz.y );
   }
}

//
// Handle toolbar resizing
//
void ToolBar::OnLeftDown( wxMouseEvent & event )
{
   // Go ahead and set the event to propagate
   event.Skip();

   // Don't do anything if we're not docked
   if( !IsDocked() )
   {
      return;
   }

   // Can we be resized?
   if( IsResizable() )
   {
      wxPoint pos = event.GetPosition();
      wxRect rect = GetRect();

      // Adjust to size of resize grabber
      rect.x = rect.width - RWIDTH;
      rect.y = 0;
      rect.width = RWIDTH;

      // Is left click within resize grabber?
      if( rect.Inside( pos ) )
      {
         // Retrieve the mouse position
         mResizeStart = ClientToScreen( pos );

         // We want all of the mouse events
         CaptureMouse();
      }
   }
}

void ToolBar::OnLeftUp( wxMouseEvent & event )
{
   // Go ahead and set the event to propagate
   event.Skip();

   if( HasCapture() )
   {
      ReleaseMouse();
   }

   SetCursor( wxCURSOR_ARROW );
}

void ToolBar::OnMotion( wxMouseEvent & event )
{
   // Go ahead and set the event to propagate
   event.Skip();

   // Don't do anything if we're not docked
   if( !IsDocked() )
   {
      return;
   }

   // Retrieve the mouse position
   wxPoint pos = ClientToScreen( event.GetPosition() );

   if( !HasCapture() )
   {
      if( IsResizable() )
      {
         wxPoint pos = event.GetPosition();
         wxRect rect = GetRect();

         // Adjust to size of resize grabber
         rect.x = rect.width - RWIDTH;
         rect.y = 0;
         rect.width = RWIDTH;

         // Is left click within resize grabber?
         if( rect.Inside( pos ) )
         {
            SetCursor( wxCURSOR_SIZEWE );
         }
         else
         {
            SetCursor( wxCURSOR_ARROW );
         }
      }
   }
   else if( event.Dragging() )
   {
      wxRect r = GetRect();
      wxSize msz = GetMinSize();
      wxSize psz = GetParent()->GetClientSize();

      // Adjust the size by the difference between the
      // last mouse and current mouse positions.
      r.width += ( pos.x - mResizeStart.x );

      // Constrain
      if( r.width < msz.x )
      {
         // Don't allow resizing to go too small
         r.width = msz.x;
      }
      else if( r.GetRight() > psz.x - 3 )
      {
         // Don't allow resizing to go too large
         //
         // The 3 magic pixels are because I'm too chicken to change the
         // calculations in ToolDock::LayoutToolBars() even though I'm
         // the one that set them up.  :-)
         r.SetRight( psz.x - 3 );
      }
      else
      {         
         // Remember for next go round
         mResizeStart = pos;
      }

      // Resize the bar
      SetSize( r.GetSize() );

      // Tell everyone we've changed sizes
      Updated();

      // Refresh our world
      GetParent()->Refresh();
      GetParent()->Update();
   }
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
// arch-tag: 2f4ec75c-bdb7-4889-96d1-5d00abc41027

