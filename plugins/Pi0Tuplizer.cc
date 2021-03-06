// -*- C++ -*-
/*
 *   Description: reconstruction of pi0/eta and save the ntuple
*/
// Author: Zhicai Zhang
// Created: Fri Mar 24 11:01:09 CET 2017


#include "Pi0Tuplizer.h"

using namespace std;

#define DEBUG


class ecalRecHitLess : public std::binary_function<EcalRecHit, EcalRecHit, bool>
{
public:
  bool operator()(EcalRecHit x, EcalRecHit y)
  {
    return (x.energy() > y.energy());
  }
};



Pi0Tuplizer::Pi0Tuplizer(const edm::ParameterSet& iConfig)
{
	//get parameters from iConfig
	isMC_	= iConfig.getUntrackedParameter<bool>("isMC",false);
	isPi0_	= iConfig.getUntrackedParameter<bool>("isPi0",false);
	FillL1SeedFinalDecision_	= iConfig.getUntrackedParameter<bool>("FillL1SeedFinalDecision",false);
	FillDiPhotonNtuple_	= iConfig.getUntrackedParameter<bool>("FillDiPhotonNtuple",false);
	FillPhotonNtuple_	= iConfig.getUntrackedParameter<bool>("FillPhotonNtuple",false);

        if(FillL1SeedFinalDecision_)   uGtAlgToken_ = consumes<BXVector<GlobalAlgBlk>>(iConfig.getUntrackedParameter<edm::InputTag>("uGtAlgInputTag"));	

	EBRecHitCollectionToken_      	= consumes<EBRecHitCollection>(iConfig.getUntrackedParameter<edm::InputTag>("EBRecHitCollectionTag"));
    	EERecHitCollectionToken_      	= consumes<EERecHitCollection>(iConfig.getUntrackedParameter<edm::InputTag>("EERecHitCollectionTag"));
    	ESRecHitCollectionToken_      	= consumes<ESRecHitCollection>(iConfig.getUntrackedParameter<edm::InputTag>("ESRecHitCollectionTag"));

	PhotonOrderOption_		= iConfig.getUntrackedParameter<std::string>("PhotonOrderOption");
	EB_Seed_E_ 			= iConfig.getUntrackedParameter<double>("EB_Seed_E",0.2);
	EE_Seed_E_ 			= iConfig.getUntrackedParameter<double>("EE_Seed_E",0.5);

	pi0PtCut_barrel1 		= iConfig.getUntrackedParameter<double>("pi0PtCut_barrel1",2.6);
	pi0PtCut_barrel2 		= iConfig.getUntrackedParameter<double>("pi0PtCut_barrel2",2.6);
	pi0PtCut_endcap1 		= iConfig.getUntrackedParameter<double>("pi0PtCut_endcap1",3.0);
	pi0PtCut_endcap2 		= iConfig.getUntrackedParameter<double>("pi0PtCut_endcap2",1.5);
	
 	gPtCut_barrel1 			= iConfig.getUntrackedParameter<double>("gPtCut_barrel1",1.3);
	gPtCut_barrel2 			= iConfig.getUntrackedParameter<double>("gPtCut_barrel2",1.3);
	gPtCut_endcap1 			= iConfig.getUntrackedParameter<double>("gPtCut_endcap1",0.95);
	gPtCut_endcap2 			= iConfig.getUntrackedParameter<double>("gPtCut_endcap2",0.65);
	
 	s4s9Cut_barrel1 		= iConfig.getUntrackedParameter<double>("s4s9Cut_barrel1",0.83);
	s4s9Cut_barrel2 		= iConfig.getUntrackedParameter<double>("s4s9Cut_barrel2",0.83);
	s4s9Cut_endcap1 		= iConfig.getUntrackedParameter<double>("s4s9Cut_endcap1",0.95);
	s4s9Cut_endcap2 		= iConfig.getUntrackedParameter<double>("s4s9Cut_endcap2",0.95);
	
 	nxtal1Cut_barrel1 		= iConfig.getUntrackedParameter<double>("nxtal1Cut_barrel1",0.);
	nxtal1Cut_barrel2 		= iConfig.getUntrackedParameter<double>("nxtal1Cut_barrel2",0.);
	nxtal1Cut_endcap1 		= iConfig.getUntrackedParameter<double>("nxtal1Cut_endcap1",0.);
	nxtal1Cut_endcap2 		= iConfig.getUntrackedParameter<double>("nxtal1Cut_endcap2",0.);

	nxtal2Cut_barrel1 		= iConfig.getUntrackedParameter<double>("nxtal2Cut_barrel1",0.);
	nxtal2Cut_barrel2 		= iConfig.getUntrackedParameter<double>("nxtal2Cut_barrel2",0.);
	nxtal2Cut_endcap1 		= iConfig.getUntrackedParameter<double>("nxtal2Cut_endcap1",0.);
	nxtal2Cut_endcap2 		= iConfig.getUntrackedParameter<double>("nxtal2Cut_endcap2",0.);
		
	ebtopology_ = new CaloTopology();
    	EcalBarrelHardcodedTopology* ebHTopology = new EcalBarrelHardcodedTopology();
    	ebtopology_->setSubdetTopology(DetId::Ecal,EcalBarrel,ebHTopology);

	eetopology_ = new CaloTopology();
    	EcalEndcapHardcodedTopology* eeHTopology=new EcalEndcapHardcodedTopology();
    	eetopology_->setSubdetTopology(DetId::Ecal,EcalEndcap,eeHTopology);

	
	//create output file and tree	
	edm::Service<TFileService> fs;
	if(FillDiPhotonNtuple_) Pi0Events = fs->make<TTree>("Pi0Events", "reconstructed pi0/eta ntuple");	
	if(FillPhotonNtuple_) PhoEvents = fs->make<TTree>("PhoEvents", "reconstructed photon ntuple");	

}


Pi0Tuplizer::~Pi0Tuplizer()
{

}


//------ Method called for each event ------//
void Pi0Tuplizer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{

	resetBranches();
	loadEvent(iEvent, iSetup); 

//fill event info
  	runNum = iEvent.id().run();
  	lumiNum = iEvent.luminosityBlock();
  	eventNum = iEvent.id().event();
	eventTime = iEvent.eventAuxiliary().time().unixTime();

//call specific functions
	if(FillL1SeedFinalDecision_) GetL1SeedBit();
	recoPhoCluster_EB();//reconstruct photon clusters in EB
	resetPhoBranches();
	recoPhoCluster_EE();//reconstruct photon clusters in EE

	if(FillDiPhotonNtuple_) recoDiPhoEvents_EB();//reconstruct pi0/eta events from the photon clusters in EB
	if(FillDiPhotonNtuple_) recoDiPhoEvents_EE();//reconstruct pi0/eta events from the photon clusters in EE
	
//fill ntuple
	if(FillDiPhotonNtuple_ && N_Pi0_rec > 0) Pi0Events->Fill();

}


//
void Pi0Tuplizer::recoPhoCluster_EB()
{
	std::vector<EcalRecHit> ebseeds;
	for(EBRecHitCollection::const_iterator itb= ebRecHit->begin(); itb != ebRecHit->end(); ++itb)
	{
		if(itb->energy() > EB_Seed_E_)  ebseeds.push_back( *itb );	
	}
	
	if(ebseeds.size() < 2) return;
	
	sort(ebseeds.begin(), ebseeds.end(), ecalRecHitLess());	
	//make photon clusters
	std::set<EBDetId> isUsed;
	for (std::vector<EcalRecHit>::iterator itseed=ebseeds.begin(); itseed!=ebseeds.end(); itseed++)
	{
		EBDetId seed_id( itseed->id() );
		if(isUsed.count(seed_id)!=0) continue;

		int seed_ieta = seed_id.ieta();
    		int seed_iphi = seed_id.iphi();
		
		std::vector<DetId> clus_v = ebtopology_->getWindow(seed_id,3,3);
		vector<const EcalRecHit*> RecHitsInWindow;
			
		float simple_energy = 0.0;
		float posTotalEnergy = 0.0;
		for (std::vector<DetId>::const_iterator det=clus_v.begin(); det!=clus_v.end(); det++)
		{
			EBDetId thisId( *det );
			if(isUsed.count(thisId)!=0) continue; 			
			EBRecHitCollection::const_iterator ixtal = ebRecHit->find( thisId );
			if( ixtal == ebRecHit->end() ) continue; 
			RecHitsInWindow.push_back( &(*ixtal) );
			simple_energy +=  ixtal->energy();
			if(ixtal->energy()>0.) posTotalEnergy += ixtal->energy();	
		}
	
		if(simple_energy <= 0) continue;

		//variables to be saved in the 3x3 cluster
		float e3x3 = 0.0;
		float e2x2 = 0.0;
		float s4s9_tmp[4]={0.,0.,0.,0.};

		float maxEne = itseed->energy();
		float maxEne2 = -999.;

		std::vector<std::pair<DetId,float> > enFracs;

		float total_weight = 0.0;
		float xclu(0.), yclu(0.), zclu(0.);
		
		for(unsigned int j=0; j<RecHitsInWindow.size();j++)
    		{
			EBDetId det(RecHitsInWindow[j]->id());	
			int ieta = det.ieta();
	        	int iphi = det.iphi();
			int deta = diff_neta_s(seed_ieta,ieta);
        		int dphi = diff_nphi_s(seed_iphi,iphi);
			float en = RecHitsInWindow[j]->energy();
	
			if(abs(deta)<=1 && abs(dphi)<=1)
        		{
				e3x3 +=  en;
				if(en<maxEne && en>maxEne2) maxEne2 = en;

				if(deta <= 0 && dphi <=0){ s4s9_tmp[0] += en; }
          			if(deta >= 0 && dphi <=0){ s4s9_tmp[1] += en; }
          			if(deta <= 0 && dphi >=0){ s4s9_tmp[2] += en; }
          			if(deta >= 0 && dphi >=0){ s4s9_tmp[3] += en; }

				isUsed.insert(RecHitsInWindow[j]->id());
		
				enFracs.push_back( std::make_pair( RecHitsInWindow[j]->id(), en ) );
	
				float weight = std::max( float(0.), PCparams_.param_W0_ + log(en/posTotalEnergy) );

				float maxDepth = PCparams_.param_X0_ * ( PCparams_.param_T0_barl_ + log( posTotalEnergy ) );
				float maxToFront;
				float pos_geo;
				
				const CaloCellGeometry* cell=geometry->getGeometry(det);
            			GlobalPoint posit = ( dynamic_cast<const TruncatedPyramid*>(cell) )->getPosition( 0. );
            			pos_geo = posit.mag();
			
				const CaloCellGeometry* cell_seed=geometry->getGeometry( seed_id );
        			GlobalPoint posit_seed = ( dynamic_cast<const TruncatedPyramid*>(cell_seed) )->getPosition( 0. );
        			maxToFront = posit_seed.mag();
		
				float depth = maxDepth + maxToFront - pos_geo;
				GlobalPoint posThis = ( dynamic_cast<const TruncatedPyramid*>(cell) )->getPosition( depth );
				xclu += weight*posThis.x();
          			yclu += weight*posThis.y();
          			zclu += weight*posThis.z();
          			total_weight += weight;
			}	
		}//end loop of xtals in 3x3 clusters

		e2x2 = *max_element( s4s9_tmp,s4s9_tmp+4);
	
		math::XYZPoint clusPos( xclu/total_weight, yclu/total_weight, zclu/total_weight );	
		
		//apply pt and s4s9 cut on photon cluster
		float s4s9 = e2x2/e3x3;
		float s2s9 =maxEne/e3x3;
		if(maxEne2 > -900.0 ) s2s9 = (maxEne+maxEne2)/e3x3;

		float ptClus = e3x3/cosh(clusPos.eta());
		if(fabs( clusPos.eta() ) < 1.0 ) 
		{
			if(ptClus<gPtCut_barrel1) continue;
			if(s4s9<s4s9Cut_barrel1) continue;
		}
		else if (fabs( clusPos.eta() ) < 1.5 ) 
		{
			if(ptClus<gPtCut_barrel2) continue;
			if(s4s9<s4s9Cut_barrel2) continue;
		}
		else 
		{
			continue;
		}
		
		ebclusters.push_back( CaloCluster( e3x3, clusPos, CaloID(CaloID::DET_ECAL_BARREL), enFracs, CaloCluster::undefined, seed_id ) );
		ebSeedTime.push_back( itseed->time() );	
		ebNxtal.push_back(RecHitsInWindow.size());
		ebS4S9.push_back(s4s9);		
		ebS2S9.push_back(s2s9);		
		ebS1S9.push_back((maxEne)/e3x3);		

		//fill photon cluster
		pho_E = e3x3;
		pho_Eta = clusPos.eta();
		pho_iEta = seed_id.ieta();
		pho_Phi = clusPos.phi();
		pho_iPhi = seed_id.iphi();
		pho_Pt = ptClus;
		pho_SeedTime = itseed->time();
		pho_ClusterTime = -999.9;//to be constructed
		pho_S4S9 = s4s9;
		pho_S2S9 = s2s9;
		pho_S1S9 = (maxEne)/e3x3; 
		pho_Nxtal = RecHitsInWindow.size();
		pho_x = clusPos.x();
		pho_y = clusPos.y();
		pho_z = clusPos.z();
		if(FillPhotonNtuple_ ) PhoEvents->Fill();
		
	}//end loop of all seed xtals	
	
}

void Pi0Tuplizer::recoPhoCluster_EE()
{

	std::vector<EcalRecHit> eeseends;
	for(EERecHitCollection::const_iterator itb= eeRecHit->begin(); itb != eeRecHit->end(); ++itb)
	{
		if(itb->energy() > EE_Seed_E_)  eeseends.push_back( *itb );	
	}
	
	if(eeseends.size() < 2) return;
	
	sort(eeseends.begin(), eeseends.end(), ecalRecHitLess());	
	//make photon clusters
	std::set<EEDetId> isUsed;
	for (std::vector<EcalRecHit>::iterator itseed=eeseends.begin(); itseed!=eeseends.end(); itseed++)
	{
		EEDetId seed_id( itseed->id() );
		if(isUsed.count(seed_id)!=0) continue;

		int seed_ix = seed_id.ix();
    		int seed_iy = seed_id.iy();
		
		std::vector<DetId> clus_v = eetopology_->getWindow(seed_id,3,3);
		vector<const EcalRecHit*> RecHitsInWindow;
			
		float simple_energy = 0.0;
		float posTotalEnergy = 0.0;
		for (std::vector<DetId>::const_iterator det=clus_v.begin(); det!=clus_v.end(); det++)
		{
			EEDetId thisId( *det );
			if(isUsed.count(thisId)!=0) continue; 			
			EERecHitCollection::const_iterator ixtal = eeRecHit->find( thisId );
			if( ixtal == eeRecHit->end() ) continue; 
			RecHitsInWindow.push_back( &(*ixtal) );
			simple_energy +=  ixtal->energy();
			if(ixtal->energy()>0.) posTotalEnergy += ixtal->energy();	
		}
	
		if(simple_energy <= 0) continue;

		//variables to be saved in the 3x3 cluster
		float e3x3 = 0.0;
		float e2x2 = 0.0;
		float s4s9_tmp[4]={0.,0.,0.,0.};

		float maxEne = itseed->energy();
		float maxEne2 = -999.;

		std::vector<std::pair<DetId,float> > enFracs;

		float total_weight = 0.0;
		float xclu(0.), yclu(0.), zclu(0.);
		
		for(unsigned int j=0; j<RecHitsInWindow.size();j++)
    		{
			EEDetId det(RecHitsInWindow[j]->id());	
			int ix = det.ix();
	        	int iy = det.iy();
			int dix = seed_ix-ix;
        		int diy = seed_iy-iy;
			float en = RecHitsInWindow[j]->energy();
	
			if(abs(dix)<=1 && abs(diy)<=1)
        		{
				e3x3 +=  en;
				if(en<maxEne && en>maxEne2) maxEne2 = en;

				if(dix <= 0 && diy <=0){ s4s9_tmp[0] += en; }
          			if(dix >= 0 && diy <=0){ s4s9_tmp[1] += en; }
          			if(dix <= 0 && diy >=0){ s4s9_tmp[2] += en; }
          			if(dix >= 0 && diy >=0){ s4s9_tmp[3] += en; }

				isUsed.insert(RecHitsInWindow[j]->id());
		
				enFracs.push_back( std::make_pair( RecHitsInWindow[j]->id(), en ) );
	
				float weight = std::max( float(0.), PCparams_.param_W0_ + log(en/posTotalEnergy) );

				float maxDepth = PCparams_.param_X0_ * ( PCparams_.param_T0_barl_ + log( posTotalEnergy ) );
				float maxToFront;
				float pos_geo;
				
				const CaloCellGeometry* cell=geometry->getGeometry(det);
            			GlobalPoint posit = ( dynamic_cast<const TruncatedPyramid*>(cell) )->getPosition( 0. );
            			pos_geo = posit.mag();
			
				const CaloCellGeometry* cell_seed=geometry->getGeometry( seed_id );
        			GlobalPoint posit_seed = ( dynamic_cast<const TruncatedPyramid*>(cell_seed) )->getPosition( 0. );
        			maxToFront = posit_seed.mag();
		
				float depth = maxDepth + maxToFront - pos_geo;
				GlobalPoint posThis = ( dynamic_cast<const TruncatedPyramid*>(cell) )->getPosition( depth );
				xclu += weight*posThis.x();
          			yclu += weight*posThis.y();
          			zclu += weight*posThis.z();
          			total_weight += weight;
			}	
		}//end loop of xtals in 3x3 clusters

		e2x2 = *max_element( s4s9_tmp,s4s9_tmp+4);
	
		math::XYZPoint clusPos( xclu/total_weight, yclu/total_weight, zclu/total_weight );	
		
		//apply pt and s4s9 cut on photon cluster
		float s4s9 = e2x2/e3x3;
		float s2s9 =maxEne/e3x3;
		if(maxEne2 > -900.0 ) s2s9 = (maxEne+maxEne2)/e3x3;

		float ptClus = e3x3/cosh(clusPos.eta());
		if(fabs( clusPos.eta() ) < 1.5 ) continue;
		else if(fabs( clusPos.eta() ) < 1.8 ) 
		{
			if(ptClus<gPtCut_endcap1) continue;
			if(s4s9<s4s9Cut_endcap1) continue;
		}
		else if (fabs( clusPos.eta() ) < 3.0 ) 
		{
			if(ptClus<gPtCut_endcap2) continue;
			if(s4s9<s4s9Cut_endcap2) continue;
		}
		else 
		{
			continue;
		}
		
		eeclusters.push_back( CaloCluster( e3x3, clusPos, CaloID(CaloID::DET_ECAL_ENDCAP), enFracs, CaloCluster::undefined, seed_id ) );
		eeSeedTime.push_back( itseed->time() );	
		eeNxtal.push_back(RecHitsInWindow.size());
		eeS4S9.push_back(s4s9);		
		eeS2S9.push_back(s2s9);		
		eeS1S9.push_back((maxEne)/e3x3);		

		//fill photon cluster
		pho_E = e3x3;
		pho_Eta = clusPos.eta();
		pho_iX = seed_id.ix();
		pho_Phi = clusPos.phi();
		pho_iY = seed_id.iy();
		pho_Pt = ptClus;
		pho_SeedTime = itseed->time();
		pho_ClusterTime = -999.9;//to be constructed
		pho_S4S9 = s4s9;
		pho_S2S9 = s2s9;
		pho_S1S9 = (maxEne)/e3x3; 
		pho_Nxtal = RecHitsInWindow.size();
		pho_x = clusPos.x();
		pho_y = clusPos.y();
		pho_z = clusPos.z();
		if(FillPhotonNtuple_ ) PhoEvents->Fill();
		
	}//end loop of all seed xtals	
	

}


void Pi0Tuplizer::recoDiPhoEvents_EB()
{
	N_Pi0_rec = 0;

	int i=0;
	
	for(std::vector<CaloCluster>::const_iterator g1  = ebclusters.begin(); g1 != ebclusters.end(); ++g1, ++i)
  	{
		if(g1->seed().subdetId()!=1) continue;

		int j=i+1;
		for(std::vector<CaloCluster>::const_iterator g2 = g1+1; g2 != ebclusters.end(); ++g2, ++j ) 
		{
			if(g2->seed().subdetId()!=1) continue;
			int ind1 = i;//
        		int ind2 = j;//
        		bool Inverted=false;// if pt(i)>pt(j), Inverted = false; else true
			 
 			//if keep this part, then the leading photon (g1) is the one with higher pt; 
 			//otherwise the leading photon is the one with higher seed energy
			if( PhotonOrderOption_ == "PhoPtBased" && g1->energy()/cosh(g1->eta()) < g2->energy()/cosh(g2->eta()) ) 
			{
				Inverted=true;
				ind1 = j;
				ind2 = i;
			}
		
			float Corr1 = 1.0;
			float Corr2 = 1.0;// whatever corrections to be added in the future
	
		 	math::PtEtaPhiMLorentzVector g1P4( (Corr1*g1->energy())/cosh(g1->eta()), g1->eta(), g1->phi(), 0. );
        		math::PtEtaPhiMLorentzVector g2P4( (Corr2*g2->energy())/cosh(g2->eta()), g2->eta(), g2->phi(), 0. );
			math::PtEtaPhiMLorentzVector pi0P4 = g1P4 + g2P4;
			math::PtEtaPhiMLorentzVector g1P4_nocor( (g1->energy())/cosh(g1->eta()), g1->eta(), g1->phi(), 0. );
        		math::PtEtaPhiMLorentzVector g2P4_nocor( (g2->energy())/cosh(g2->eta()), g2->eta(), g2->phi(), 0. );
        		math::PtEtaPhiMLorentzVector pi0P4_nocor = g1P4_nocor + g2P4_nocor;

			if( pi0P4_nocor.mass()<0.03 && pi0P4.mass() < 0.03 ) continue;
			//apply kinamatics cut on diphoton and nxtal cut			
			if(fabs( pi0P4.eta() ) < 1.0 ) 
			{
				if(pi0P4.Pt()<pi0PtCut_barrel1) continue;
				if(ebNxtal[ind1]<nxtal1Cut_barrel1) continue;
				if(ebNxtal[ind2]<nxtal2Cut_barrel1) continue;
			}
			else if (fabs( pi0P4.eta() ) < 1.5 ) 
			{
				if(pi0P4.Pt()<pi0PtCut_barrel2) continue;
				if(ebNxtal[ind1]<nxtal1Cut_barrel2) continue;
				if(ebNxtal[ind2]<nxtal2Cut_barrel2) continue;
			}
			else 
			{
				continue;
			}

			if( g1P4.eta() == g2P4.eta() && g1P4.phi() == g2P4.phi() ) continue;
			
			//fill pi0/eta ntuple
			if(N_Pi0_rec >= NPI0MAX-1) break; // too many pi0s
			if( FillDiPhotonNtuple_ && pi0P4.mass() > ((isPi0_)?0.03:0.2) && pi0P4.mass() < ((isPi0_)?0.25:1.) )
			{
				mPi0_rec[N_Pi0_rec]  =  pi0P4.mass();
				ptPi0_rec[N_Pi0_rec] =  pi0P4.Pt();
				etaPi0_rec[N_Pi0_rec] =  pi0P4.Eta();
				phiPi0_rec[N_Pi0_rec] =  pi0P4.Phi();
	
				deltaRG1G2_rec[N_Pi0_rec] = GetDeltaR( g1P4.eta(), g2P4.eta(), g1P4.phi(), g2P4.phi() );

				EBDetId  id_1(g1->seed());
              			EBDetId  id_2(g2->seed());
              			if(Inverted)
                		{
                  			id_1 = g2->seed();
                  			id_2 = g1->seed();
                		}

				if(!Inverted)
				{
					enG1_rec[N_Pi0_rec] =  g1P4.E();
					enG2_rec[N_Pi0_rec] =  g2P4.E();
					etaG1_rec[N_Pi0_rec] =  g1P4.Eta();
					etaG2_rec[N_Pi0_rec] =  g2P4.Eta();
					phiG1_rec[N_Pi0_rec] =  g1P4.Phi();
					phiG2_rec[N_Pi0_rec] =  g2P4.Phi();
					ptG1_rec[N_Pi0_rec] =  g1P4.Pt();
					ptG2_rec[N_Pi0_rec] =  g2P4.Pt();
				}
				else
				{
					enG1_rec[N_Pi0_rec] =  g2P4.E();
					enG2_rec[N_Pi0_rec] =  g1P4.E();
					etaG1_rec[N_Pi0_rec] =  g2P4.Eta();
					etaG2_rec[N_Pi0_rec] =  g1P4.Eta();
					phiG1_rec[N_Pi0_rec] =  g2P4.Phi();
					phiG2_rec[N_Pi0_rec] =  g1P4.Phi();
					ptG1_rec[N_Pi0_rec] =  g2P4.Pt();
					ptG2_rec[N_Pi0_rec] =  g1P4.Pt();
				}
				
				iEtaG1_rec[N_Pi0_rec] =  id_1.ieta();
				iEtaG2_rec[N_Pi0_rec] =  id_2.ieta();
				iPhiG1_rec[N_Pi0_rec] =  id_1.iphi();
				iPhiG1_rec[N_Pi0_rec] =  id_2.iphi();

				seedTimeG1_rec[N_Pi0_rec] = ebSeedTime[ind1];	
				seedTimeG2_rec[N_Pi0_rec] = ebSeedTime[ind2];	
				s4s9G1_rec[N_Pi0_rec] = ebS4S9[ind1];	
				s4s9G2_rec[N_Pi0_rec] = ebS4S9[ind2];	
				s2s9G1_rec[N_Pi0_rec] = ebS2S9[ind1];	
				s2s9G2_rec[N_Pi0_rec] = ebS2S9[ind2];	
				s1s9G1_rec[N_Pi0_rec] = ebS1S9[ind1];	
				s1s9G2_rec[N_Pi0_rec] = ebS1S9[ind2];	
				nxtalG1_rec[N_Pi0_rec] = ebNxtal[ind1];	
				nxtalG2_rec[N_Pi0_rec] = ebNxtal[ind2];	
				N_Pi0_rec ++;			
			}
			if(N_Pi0_rec >= NPI0MAX-1) break; // too many pi0s
		}//end loop of g2	
	}//end loop of g1
}

void Pi0Tuplizer::recoDiPhoEvents_EE()
{


	int i=0;
	
	for(std::vector<CaloCluster>::const_iterator g1  = eeclusters.begin(); g1 != eeclusters.end(); ++g1, ++i)
  	{
		if(g1->seed().subdetId()!=2) continue;

		int j=i+1;
		for(std::vector<CaloCluster>::const_iterator g2 = g1+1; g2 != eeclusters.end(); ++g2, ++j ) 
		{
			if(g2->seed().subdetId()!=2) continue;
			int ind1 = i;//
        		int ind2 = j;//
        		bool Inverted=false;// if pt(i)>pt(j), Inverted = false; else true
			 
 			//if keep this part, then the leading photon (g1) is the one with higher pt; 
 			//otherwise the leading photon is the one with higher seed energy
			if( PhotonOrderOption_ == "PhoPtBased" && g1->energy()/cosh(g1->eta()) < g2->energy()/cosh(g2->eta()) ) 
			{
				Inverted=true;
				ind1 = j;
				ind2 = i;
			}
		
			float Corr1 = 1.0;
			float Corr2 = 1.0;// whatever corrections to be added in the future
	
		 	math::PtEtaPhiMLorentzVector g1P4( (Corr1*g1->energy())/cosh(g1->eta()), g1->eta(), g1->phi(), 0. );
        		math::PtEtaPhiMLorentzVector g2P4( (Corr2*g2->energy())/cosh(g2->eta()), g2->eta(), g2->phi(), 0. );
			math::PtEtaPhiMLorentzVector pi0P4 = g1P4 + g2P4;
			math::PtEtaPhiMLorentzVector g1P4_nocor( (g1->energy())/cosh(g1->eta()), g1->eta(), g1->phi(), 0. );
        		math::PtEtaPhiMLorentzVector g2P4_nocor( (g2->energy())/cosh(g2->eta()), g2->eta(), g2->phi(), 0. );
        		math::PtEtaPhiMLorentzVector pi0P4_nocor = g1P4_nocor + g2P4_nocor;

			if( pi0P4_nocor.mass()<0.03 && pi0P4.mass() < 0.03 ) continue;
			//apply kinamatics cut on diphoton and nxtal cut			
			if(fabs( pi0P4.eta() ) < 1.5 ) continue;
			else if(fabs( pi0P4.eta() ) < 1.8 ) 
			{
				if(pi0P4.Pt()<pi0PtCut_endcap1) continue;
				if(eeNxtal[ind1]<nxtal1Cut_endcap1) continue;
				if(eeNxtal[ind2]<nxtal2Cut_endcap1) continue;
			}
			else if (fabs( pi0P4.eta() ) < 3.0 ) 
			{
				if(pi0P4.Pt()<pi0PtCut_endcap2) continue;
				if(eeNxtal[ind1]<nxtal1Cut_endcap2) continue;
				if(eeNxtal[ind2]<nxtal2Cut_endcap2) continue;
			}
			else 
			{
				continue;
			}

			if( g1P4.eta() == g2P4.eta() && g1P4.phi() == g2P4.phi() ) continue;
			
			//fill pi0/eta ntuple
			if(N_Pi0_rec >= NPI0MAX-1) break; // too many pi0s
			if( FillDiPhotonNtuple_ && pi0P4.mass() > ((isPi0_)?0.03:0.2) && pi0P4.mass() < ((isPi0_)?0.25:1.) )
			{
				mPi0_rec[N_Pi0_rec]  =  pi0P4.mass();
				ptPi0_rec[N_Pi0_rec] =  pi0P4.Pt();
				etaPi0_rec[N_Pi0_rec] =  pi0P4.Eta();
				phiPi0_rec[N_Pi0_rec] =  pi0P4.Phi();
	
				deltaRG1G2_rec[N_Pi0_rec] = GetDeltaR( g1P4.eta(), g2P4.eta(), g1P4.phi(), g2P4.phi() );

				EEDetId  id_1(g1->seed());
              			EEDetId  id_2(g2->seed());
              			if(Inverted)
                		{
                  			id_1 = g2->seed();
                  			id_2 = g1->seed();
                		}

				if(!Inverted)
				{
					enG1_rec[N_Pi0_rec] =  g1P4.E();
					enG2_rec[N_Pi0_rec] =  g2P4.E();
					etaG1_rec[N_Pi0_rec] =  g1P4.Eta();
					etaG2_rec[N_Pi0_rec] =  g2P4.Eta();
					phiG1_rec[N_Pi0_rec] =  g1P4.Phi();
					phiG2_rec[N_Pi0_rec] =  g2P4.Phi();
					ptG1_rec[N_Pi0_rec] =  g1P4.Pt();
					ptG2_rec[N_Pi0_rec] =  g2P4.Pt();
				}
				else
				{
					enG1_rec[N_Pi0_rec] =  g2P4.E();
					enG2_rec[N_Pi0_rec] =  g1P4.E();
					etaG1_rec[N_Pi0_rec] =  g2P4.Eta();
					etaG2_rec[N_Pi0_rec] =  g1P4.Eta();
					phiG1_rec[N_Pi0_rec] =  g2P4.Phi();
					phiG2_rec[N_Pi0_rec] =  g1P4.Phi();
					ptG1_rec[N_Pi0_rec] =  g2P4.Pt();
					ptG2_rec[N_Pi0_rec] =  g1P4.Pt();
				}
				
				iXG1_rec[N_Pi0_rec] =  id_1.ix();
				iXG2_rec[N_Pi0_rec] =  id_2.ix();
				iYG1_rec[N_Pi0_rec] =  id_1.iy();
				iYG1_rec[N_Pi0_rec] =  id_2.iy();

				seedTimeG1_rec[N_Pi0_rec] = eeSeedTime[ind1];	
				seedTimeG2_rec[N_Pi0_rec] = eeSeedTime[ind2];	
				s4s9G1_rec[N_Pi0_rec] = eeS4S9[ind1];	
				s4s9G2_rec[N_Pi0_rec] = eeS4S9[ind2];	
				s2s9G1_rec[N_Pi0_rec] = eeS2S9[ind1];	
				s2s9G2_rec[N_Pi0_rec] = eeS2S9[ind2];	
				s1s9G1_rec[N_Pi0_rec] = eeS1S9[ind1];	
				s1s9G2_rec[N_Pi0_rec] = eeS1S9[ind2];	
				nxtalG1_rec[N_Pi0_rec] = eeNxtal[ind1];	
				nxtalG2_rec[N_Pi0_rec] = eeNxtal[ind2];	
				N_Pi0_rec ++;			
			}
			if(N_Pi0_rec >= NPI0MAX-1) break; // too many pi0s
		}//end loop of g2	
	}//end loop of g1

}



void Pi0Tuplizer::GetL1SeedBit()
{
	if(uGtAlg.isValid()) 
	{
        	for(int bx=uGtAlg->getFirstBX(); bx<=uGtAlg->getLastBX(); bx++) 
		{
        		for(std::vector<GlobalAlgBlk>::const_iterator algBlk = uGtAlg->begin(bx); algBlk != uGtAlg->end(bx); ++algBlk) 
			{
                		for(uint i=0;i<NL1SEED;i++)
				{
                			if(algBlk->getAlgoDecisionFinal(i)) 
					{
					allL1SeedFinalDecision[i] = true;
					L1SeedBitFinalDecision->push_back(i);
					}
                		}
              		}
           	}
        }
	
}


//load all collection from cfg file
void Pi0Tuplizer::loadEvent(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
	ebclusters.clear();
	eeclusters.clear();
	ebSeedTime.clear();
	eeSeedTime.clear();
	ebNxtal.clear();
	eeNxtal.clear();
	ebS4S9.clear();
	eeS4S9.clear();
	ebS2S9.clear();
	eeS2S9.clear();
	ebS1S9.clear();
	eeS1S9.clear();

	if(FillL1SeedFinalDecision_) iEvent.getByToken(uGtAlgToken_,uGtAlg);	
	iEvent.getByToken ( EBRecHitCollectionToken_, ebRecHit);
  	iEvent.getByToken ( EERecHitCollectionToken_, eeRecHit);
  	iEvent.getByToken ( ESRecHitCollectionToken_, esRecHit);
	
	edm::ESHandle<CaloGeometry> geoHandle;
  	iSetup.get<CaloGeometryRecord>().get(geoHandle);
  	geometry = geoHandle.product();
  	estopology_ = new EcalPreshowerTopology(geoHandle);


}



// ------------ method called once each job just before starting event loop  ------------
void Pi0Tuplizer::beginJob()
{

setBranches();

}

//------ Method called once each job just after ending the event loop ------//
void Pi0Tuplizer::endJob()
{

}

// ------------ method called when starting to processes a run  ------------
void Pi0Tuplizer::beginRun(edm::Run const&, edm::EventSetup const& iSetup)
{

}

// ------------ method called when ending the processing of a run  ------------
void Pi0Tuplizer::endRun(edm::Run const&, edm::EventSetup const&)
{

}


// set branch address
void Pi0Tuplizer::setBranches()
{
	//initiate vectors
	L1SeedBitFinalDecision = new std::vector<int>;
	L1SeedBitFinalDecision->clear();
	
//pi0 ntuple
  	Pi0Events->Branch("runNum", &runNum, "runNum/i");
  	Pi0Events->Branch("lumiNum", &lumiNum, "lumiNum/i");
  	Pi0Events->Branch("eventNum", &eventNum, "eventNum/i");
  	Pi0Events->Branch("eventTime", &eventTime, "eventTime/i");
  	Pi0Events->Branch( "allL1SeedFinalDecision", allL1SeedFinalDecision, "allL1SeedFinalDecision[300]/O");
	Pi0Events->Branch("L1SeedBitFinalDecision", "vector<int>", &L1SeedBitFinalDecision);

	Pi0Events->Branch( "N_Pi0_rec", &N_Pi0_rec, "N_Pi0_rec/I");
	Pi0Events->Branch( "mPi0_rec", mPi0_rec, "mPi0_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "ptPi0_rec", ptPi0_rec, "ptPi0_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "etaPi0_rec", etaPi0_rec, "etaPi0_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "phiPi0_rec", phiPi0_rec, "phiPi0_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "enG1_rec", enG1_rec, "enG1_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "enG2_rec", enG2_rec, "enG2_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "etaG1_rec", etaG1_rec, "etaG1_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "etaG2_rec", etaG2_rec, "etaG2_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "phiG1_rec", phiG1_rec, "phiG1_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "phiG2_rec", phiG2_rec, "phiG2_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "ptG1_rec", ptG1_rec, "ptG1_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "ptG2_rec", ptG2_rec, "ptG2_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "iEtaG1_rec", iEtaG1_rec, "iEtaG1_rec[N_Pi0_rec]/I");		
	Pi0Events->Branch( "iXG1_rec", iXG1_rec, "iXG1_rec[N_Pi0_rec]/I");		
	Pi0Events->Branch( "iEtaG2_rec", iEtaG2_rec, "iEtaG2_rec[N_Pi0_rec]/I");		
	Pi0Events->Branch( "iXG2_rec", iXG2_rec, "iXG2_rec[N_Pi0_rec]/I");		
	Pi0Events->Branch( "iPhiG1_rec", iPhiG1_rec, "iPhiG1_rec[N_Pi0_rec]/I");		
	Pi0Events->Branch( "iYG1_rec", iYG1_rec, "iYG1_rec[N_Pi0_rec]/I");		
	Pi0Events->Branch( "iPhiG2_rec", iPhiG2_rec, "iPhiG2_rec[N_Pi0_rec]/I");		
	Pi0Events->Branch( "iYG2_rec", iYG2_rec, "iYG2_rec[N_Pi0_rec]/I");		
	Pi0Events->Branch( "deltaRG1G2_rec", deltaRG1G2_rec, "deltaRG1G2_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "nxtalG1_rec", nxtalG1_rec, "nxtalG1_rec[N_Pi0_rec]/I");		
	Pi0Events->Branch( "nxtalG2_rec", nxtalG2_rec, "nxtalG2_rec[N_Pi0_rec]/I");		
	Pi0Events->Branch( "seedTimeG1_rec", seedTimeG1_rec, "seedTimeG1_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "seedTimeG2_rec", seedTimeG2_rec, "seedTimeG2_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "s4s9G1_rec", s4s9G1_rec, "s4s9G1_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "s4s9G2_rec", s4s9G2_rec, "s4s9G2_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "s2s9G1_rec", s2s9G1_rec, "s2s9G1_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "s2s9G2_rec", s2s9G2_rec, "s2s9G2_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "s1s9G1_rec", s1s9G1_rec, "s1s9G1_rec[N_Pi0_rec]/F");		
	Pi0Events->Branch( "s1s9G2_rec", s1s9G2_rec, "s1s9G2_rec[N_Pi0_rec]/F");		
//photon ntuple
	PhoEvents->Branch("runNum", &runNum, "runNum/i");
  	PhoEvents->Branch("lumiNum", &lumiNum, "lumiNum/i");
  	PhoEvents->Branch("eventNum", &eventNum, "eventNum/i");
  	PhoEvents->Branch("eventTime", &eventTime, "eventTime/i");
  	PhoEvents->Branch("pho_E", &pho_E, "pho_E/F");
  	PhoEvents->Branch("pho_Eta", &pho_Eta, "pho_Eta/F");
  	PhoEvents->Branch("pho_iEta", &pho_iEta, "pho_iEta/I");
  	PhoEvents->Branch("pho_iX", &pho_iX, "pho_iX/I");
  	PhoEvents->Branch("pho_Phi", &pho_Phi, "pho_Phi/F");
  	PhoEvents->Branch("pho_iPhi", &pho_iPhi, "pho_iPhi/I");
  	PhoEvents->Branch("pho_iY", &pho_iY, "pho_iY/I");
  	PhoEvents->Branch("pho_Pt", &pho_Pt, "pho_Pt/F");
  	PhoEvents->Branch("pho_SeedTime", &pho_SeedTime, "pho_SeedTime/F");
  	PhoEvents->Branch("pho_ClusterTime", &pho_ClusterTime, "pho_ClusterTime/F");
  	PhoEvents->Branch("pho_S4S9", &pho_S4S9, "pho_S4S9/F");
  	PhoEvents->Branch("pho_S2S9", &pho_S2S9, "pho_S2S9/F");
  	PhoEvents->Branch("pho_S1S9", &pho_S1S9, "pho_S1S9/F");
  	PhoEvents->Branch("pho_Nxtal", &pho_Nxtal, "pho_Nxtal/I");
  	PhoEvents->Branch("pho_x", &pho_x, "pho_x/F");
  	PhoEvents->Branch("pho_y", &pho_y, "pho_y/F");
  	PhoEvents->Branch("pho_z", &pho_z, "pho_z/F");
	
}

// clear the content of all variables in the output ntuple
void Pi0Tuplizer::resetBranches()
{
	runNum = -1;
	lumiNum = -1;
	eventNum = -1;
	eventTime = -1;
	for(int i=0;i<NL1SEED;i++)
	{
	allL1SeedFinalDecision[i] = false;
	}	
	L1SeedBitFinalDecision->clear();
	N_Pi0_rec = 0;
	for(int i=0;i<NPI0MAX;i++)
	{
		mPi0_rec[i] = 0;
		ptPi0_rec[i] = 0;
		etaPi0_rec[i] = 0;
		phiPi0_rec[i] = 0;
		enG1_rec[i] = 0;
		enG2_rec[i] = 0;
		etaG1_rec[i] = 0;
		etaG2_rec[i] = 0;
		phiG1_rec[i] = 0;
		phiG2_rec[i] = 0;
		ptG1_rec[i] = 0;
		ptG2_rec[i] = 0;
		iEtaG1_rec[i] = -999;
		iXG1_rec[i] = -999;
		iEtaG2_rec[i] = -999;
		iXG2_rec[i] = -999;
		iPhiG1_rec[i] = -999;
		iYG1_rec[i] = -999;
		iPhiG2_rec[i] = -999;
		iYG2_rec[i] = -999;
		deltaRG1G2_rec[i] = 0;
		nxtalG1_rec[i] = 0;
		nxtalG2_rec[i] = 0;
		seedTimeG1_rec[i] = 0;
		seedTimeG2_rec[i] = 0;
		s4s9G1_rec[i] = 0;
		s4s9G2_rec[i] = 0;
		s2s9G1_rec[i] = 0;
		s2s9G2_rec[i] = 0;
		s1s9G1_rec[i] = 0;
		s1s9G2_rec[i] = 0;
	}

	pho_E=0;
	pho_Eta=0;
	pho_iEta=-999;
	pho_iX=-999;
	pho_Phi=0;
	pho_iPhi=-999;
	pho_iY=-999;
	pho_Pt=0;
	pho_SeedTime=0;
	pho_ClusterTime=0;
	pho_S4S9=0;
	pho_S2S9=0;
	pho_S1S9=0;
	pho_Nxtal=0;
	pho_x=0;
	pho_y=0;
	pho_z=0;
}

void Pi0Tuplizer::resetPhoBranches()
{
	pho_E=0;
        pho_Eta=0;
        pho_iEta=-999;
        pho_iX=-999;
        pho_Phi=0;
        pho_iPhi=-999;
        pho_iY=-999;
        pho_Pt=0;
        pho_SeedTime=0;
        pho_ClusterTime=0;
        pho_S4S9=0;
        pho_S2S9=0;
        pho_S1S9=0;
        pho_Nxtal=0;
        pho_x=0;
        pho_y=0;
        pho_z=0;
}


void Pi0Tuplizer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
      edm::ParameterSetDescription desc;
      desc.setUnknown();
      descriptions.addDefault(desc);
}

int Pi0Tuplizer::diff_neta_s(int neta1, int neta2)
{
	int diff = neta1 - neta2;
	return diff;
}

int Pi0Tuplizer::diff_nphi_s(int nphi1, int nphi2)
{
	int diff;
	if(abs(nphi1-nphi2) < (360-abs(nphi1-nphi2))) diff = nphi1 - nphi2;
	
	else
	{
		diff=360-abs(nphi1-nphi2);
		if(nphi1>nphi2) diff=-diff;
	}	
	return diff;
}


float Pi0Tuplizer::GetDeltaR(float eta1, float eta2, float phi1, float phi2){

  return sqrt( (eta1-eta2)*(eta1-eta2)
        + DeltaPhi(phi1, phi2)*DeltaPhi(phi1, phi2) );

}


float Pi0Tuplizer::DeltaPhi(float phi1, float phi2){
  float diff = fabs(phi2 - phi1);
  while (diff >acos(-1)) diff -= 2*acos(-1);
  while (diff <= -acos(-1)) diff += 2*acos(-1);
  
  return diff;

}


//define this as a plug-in
DEFINE_FWK_MODULE(Pi0Tuplizer);

