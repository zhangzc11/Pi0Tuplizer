#ifndef HLTEFF_HH
#define HLTEFF_HH

#include "Pi0Events.hh" 

#include <TROOT.h>
#include <TChain.h>
#include <TTree.h>
#include <TFile.h>
#include "TLorentzVector.h"

#include <map>
#include <string>
#include <vector>
#include <iostream>
using namespace std;

class HLTeff: public Pi0Events {
    public :
        HLTeff(TTree *tree=0);
	virtual ~HLTeff();
	virtual void Analyze();
};

#endif
