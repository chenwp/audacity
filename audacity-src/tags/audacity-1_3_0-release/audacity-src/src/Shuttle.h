/**********************************************************************

  Audacity: A Digital Audio Editor

  Shuttle.h

  Dominic Mazzoni
  James Crook

**********************************************************************/

#ifndef __AUDACITY_SHUTTLE__
#define __AUDACITY_SHUTTLE__


// Enums is a helper class for Shuttle.
// It defines enumerations which are used in effects dialogs,
// in the effects themselves and in preferences.
// (If it grows big, we will move it out of shuttle.h).
class Enums {
public:
   static const wxString * GetDbChoices();
   static const int    NumDbChoices;
   static const double Db2Signal[];
   static const wxString DbChoices[];
};

class Shuttle {
 public:
   // constructors and destructors
   Shuttle();
   virtual ~Shuttle() {}
      
 public:
   bool mbStoreInClient;
   wxString mValueString;
   // Even though virtual, mostly the transfer functions won't change
   // for special kinds of archive.
   virtual bool TransferBool( const wxString & Name, bool & bValue, const bool & bDefault );
   virtual bool TransferFloat( const wxString & Name, float & fValue, const float &fDefault );
   virtual bool TransferDouble( const wxString & Name, double & dValue, const double &dDefault );
   virtual bool TransferInt( const wxString & Name, int & iValue, const int &iDefault );
   virtual bool TransferLongLong( const wxString & Name, wxLongLong_t & iValue, const wxLongLong_t &iDefault );
   virtual bool TransferEnum( const wxString & Name, int & iValue, 
      const int nChoices, const wxString * pFirstStr);
   // We expect the ExchangeWithMaster function to change from one type of
   // archive to another.
   virtual bool ExchangeWithMaster(const wxString & Name);
};

class ShuttleCli : public Shuttle
{
public:
   wxString mParams;
   ShuttleCli(){ mParams = wxT("") ;}
   virtual ~ShuttleCli() {}
   virtual bool ExchangeWithMaster(const wxString & Name);
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
// arch-tag: TBD

