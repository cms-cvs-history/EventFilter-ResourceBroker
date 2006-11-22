////////////////////////////////////////////////////////////////////////////////
//
// FEDProvider
// -----------
//
////////////////////////////////////////////////////////////////////////////////


#include "EventFilter/ResourceBroker/interface/FEDProvider.h"

#include <iostream>


using namespace std;
using namespace evf;


////////////////////////////////////////////////////////////////////////////////
// initialze static members
////////////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
FEDProvider* FEDProvider::instance_=0;


////////////////////////////////////////////////////////////////////////////////
// construction/destruction
////////////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
FEDProvider::FEDProvider()
{
  if (0!=instance_) cout<<"WARNING: 2 FEDProvider in the same process!"<<endl;
  instance_=this;
}


//______________________________________________________________________________
FEDProvider::~FEDProvider()
{
  instance_=0;
}

