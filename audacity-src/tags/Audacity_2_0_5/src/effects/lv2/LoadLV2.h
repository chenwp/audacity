/**********************************************************************

  Audacity: A Digital Audio Editor

  LoadLV2.h

  Audacity(R) is copyright (c) 1999-2008 Audacity Team.
  License: GPL v2.  See License.txt.

*********************************************************************/

#include <slv2/slv2.h>

extern SLV2World gWorld;

// This is the LV2 Feature array. It is passed to every LV2 plugin on 
// instantiation. So far it only contains the URI Map Feature, which is
// needed to load synths.
extern LV2_Feature*const gLV2Features[];

// These are needed for comparisons
extern SLV2Value gAudioPortClass;
extern SLV2Value gControlPortClass;
extern SLV2Value gMidiPortClass;
extern SLV2Value gInputPortClass;
extern SLV2Value gOutputPortClass;
extern SLV2Value gPortToggled;
extern SLV2Value gPortIsInteger;
extern SLV2Value gPortIsSampleRate;


void LoadLV2Plugins();
void UnloadLV2Plugins();
