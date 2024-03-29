/**********************************************************************

  Audacity: A Digital Audio Editor

  Reverse.h

  Mark Phillips
  
  This class reverses the selected audio.

**********************************************************************/

#ifndef __AUDACITY_EFFECT_REVERSE__
#define __AUDACITY_EFFECT_REVERSE__

#include <wx/intl.h>

#include "Effect.h"

#define __UNINITIALIZED__ (-1)

class WaveTrack;

class EffectReverse:public Effect {

 public:
   EffectReverse();

   virtual wxString GetEffectName() {
      return wxString(_("Reverse"));
   }
   
   virtual wxString GetEffectAction() {
      return wxString(_("Reversing"));
   }
   
   virtual bool Process();

 private:
   bool ProcessOne(int count, WaveTrack * track,
                   longSampleCount start, sampleCount len);

 };

#endif

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: 99337674-07d6-4291-b855-b0ddf727566f

