// Patrick Dunne, p.dunne12@imperial.ac.uk
// August 11, 2021
//
// Adapted from T2K  XsecResponse/src_bin/make_xsec_response_sk_2019.cc
//
// Usage:
//  ./bin/make_xsec_response_1d -w wtfile.root -m mtuple.root -o outfile.root -selec {nue, numu, cc1pi}
//
#include <stdlib.h>
#include <cstdlib>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>
#include <string.h>

#include <iostream>

#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2F.h"
#include "TMatrixDSym.h"
#include "TVectorD.h"
#include "TAxis.h"

#include "XsecVary.h"

using std::cout;
using std::cerr;
using std::endl;

bool nue = false;
bool NHC = true;
bool byEvent_splines = false;
std::string fTempFilenue =  "inputs/DUNE_numu_templates_v0.root";
std::string fTempFilenumu =  "inputs/DUNE_numu_templates_v0.root";
std::string fTempFilenue_erec =  "inputs/DUNE_numu_templates_v0_erec.root";
std::string fTempFilenumu_erec =  "inputs/DUNE_numu_templates_v0_erec.root";
std::string fCovFile = "inputs/flux_covariance_13av1_prelim.root";
std::string wtfile = "";
std::string mtuple = "";
std::string outfile = "";
std::string selection = "";
void Usage();
void ParseArgs(int argc, char **argv);
void ReadTemplates(TFile *ftemp, std::vector<TH1F*> &recoe_temps);


int main(int argc, char *argv[])
{
    //Get the arguments
    ParseArgs(argc, argv);

	std::cout << "~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
	std::cout << "Starting spline generation using " << wtfile << " and selection " << selection << std::endl;
    // Check name of file to see if it's numode or anti-numode, and whether nue or numu selection is required //!!PD NEEDS UPDATING FOR DUNE
    std::string RHC_mode("m250");
	std::string alt_RHC_mode("rhc");//2020 OA mtuples labelled with fhc/rhc
    std::string mc_version(mtuple);
    std::string nue_selec("nue");
    std::string cc1pi_selec("cc1pi");
    std::string numu_selec("numu");
    std::string str_selec(selection);
    std::size_t mc_found_RHC = mc_version.find(RHC_mode);
    std::size_t alt_mc_found_RHC = mc_version.find(alt_RHC_mode);//2020 OA mtuples labelled with fhc/rhc
    if (mc_found_RHC != std::string::npos || alt_mc_found_RHC != std::string::npos)
      NHC = false;
    std::size_t selec_found_nue = str_selec.find(nue_selec);
    std::size_t selec_found_numu = str_selec.find(numu_selec);
    std::size_t selec_found_cc1pi = str_selec.find(cc1pi_selec);
    if (selec_found_nue != std::string::npos)
      nue = true;
    else if (selec_found_numu != std::string::npos)
      nue = false;
    else if (selec_found_cc1pi != std::string::npos)
      nue = true;
    else{
      std::cerr << "Incompatible selection given: " << selection << ". Defaulting to numu selection" << std::endl;
      std::cerr << "ERROR: Incompatible selection given: " << selection << ". Something has gone wrong..." << std::endl;
      std::cerr << "ERROR: You gave me a weight file " << wtfile << std::endl;
      std::cerr << "ERROR: You gave me a mtuple file " << mtuple << std::endl;
      std::cerr << "ERROR: You gave me a selection of " << selection << std::endl;
	  throw;
	}
    std::cout << "Detected from filename: Numode = " << NHC << std::endl;
    if (nue) std::cout << "Using nue selection" << std::endl;
    else std::cout << "Using numu selection" << std::endl;

    //Change File paths
    outfile = "../m3_dune/inputs/mtuples/MAQESplines/" + outfile;
    std::cout << "THE OUTPUT FILE IS: " << outfile << std::endl; 
    //Open template files and covariance files
    TFile *ftemp_nue = new TFile(fTempFilenue.c_str()); 
    TFile *ftemp_numu = new TFile(fTempFilenumu.c_str());
    
    TFile *ftemp_nue_erec = new TFile(fTempFilenue_erec.c_str()); 
    TFile *ftemp_numu_erec = new TFile(fTempFilenumu_erec.c_str());

    //Load the templates
    std::vector<TH1F*> recoe_temps;
    std::vector<TH1F*> recoe_temps_erec;
    if (nue)
      ReadTemplates(ftemp_nue, recoe_temps);
    else
      ReadTemplates(ftemp_numu, recoe_temps);
    std::cout << "Creating splines with ";
    if (nue) std::cout << "nue etrue binning" << std::endl;
    else std::cout << "numu etrue binning" << std::endl;
    std::cout << "Found " << recoe_temps.size() << " etrue binning templates" << std::endl;

    if (nue)
      ReadTemplates(ftemp_nue_erec, recoe_temps_erec);
    else
      ReadTemplates(ftemp_numu_erec, recoe_temps_erec);
    std::cout << "Creating splines with ";
    if (nue) std::cout << "nue erec binning" << std::endl;
    else std::cout << "numu erec binning" << std::endl;
    std::cout << "Found " << recoe_temps_erec.size() << " erec binning templates" << std::endl;

    //Load the xsec variations, generate splines and graphs for xsec variations

    //For the nue sample ('0' is code for nue, '1' is code for numu). Actually this doesn't do anything in XsecVary at the moment, but could be used to put in the selections
    int nutype;
    if (nue) nutype = 0;
    else nutype = 1;


    // ---------------------------------------------------------------------------------- //
    //                                                                                    //
    //               Start block for specifying systematics treatments.                   //
    //                                                                                    //
    // ---------------------------------------------------------------------------------- //

    // Vectors containing interaction mode codes that will eventually be associated with a
    // particular systematic. These are the modes this systematic affects.
    std::vector<int> ccqemodes; ccqemodes.push_back(1);
    std::vector<int> resmodes; resmodes.push_back(4);
    std::vector<int> dismodes; dismodes.push_back(3);
    std::vector<int> disresmodes; disresmodes.push_back(3); disresmodes.push_back(4);
    std::vector<int> mostmodes; mostmodes.push_back(1); mostmodes.push_back(3); mostmodes.push_back(4); mostmodes.push_back(10);
    std::vector<int> allmodes; 
    for(int i = 0; i <= 11; i++) 
      allmodes.push_back(i);

    // Specify for which systematics we would like splines produced.
    // Constructor requires 4 inputs, with an optional 5th. In order:
    // (1) Name of systematic, formatted in correspondes with the input weight file
    //     tree name for that systematic, with "_tree" removed
    // (2) A short name for this systematic that will be featured in the name of
    //     output splines and graphs, along mode and bin information
    // (3) List of interaction modes affected by this parameter, built above
    // (4) The position of the nominal knot within the arrays contained in the
    //     supplied weight file
    // (5) [optional] Should extra knots be made to extend the range of systematic
    //      values in the splines, creating flat(ish) edges? (boolean)
	//
	///    SystematicProperties pfo_sysprop("NIWG2014a_pF_O16","pfo",ccqemode,6,true);
    //    SystematicProperties maqe_sf_sysprop("NXSec_MaCCQE_SF","maqesf",ccqemode,14);
	//    SystematicProperties dismpishp_sysprop("NIWG2012a_dismpishp","dismpishp",ccothmode,6);
	//    SystematicProperties berpaA_sysprop("NIWG_Effective_rpaCCQE_A","erpa_A",ccqemode,6);
	//    SystematicProperties berpaB_sysprop("NIWG_Effective_rpaCCQE_B","erpa_B",ccqemode,6);
	//    SystematicProperties berpaC_sysprop("NIWG_Effective_rpaCCQE_C","erpa_D",ccqemode,6);
	//    SystematicProperties berpaD_sysprop("NIWG_Effective_rpaCCQE_D","erpa_E",ccqemode,6);
   
   // DUNE SYSTEMATICS
 
    SystematicProperties maqe_sysprop("MaCCQE","maqe",ccqemodes,3);  //1
    SystematicProperties vecffqeshape_sysprop("VecFFCCQEshape","vecffqeshape", ccqemodes,3); //1
    //SystematicProperties mancel_sysprop("MaNCEL","mancel", allmodes,3); //? **
    //SystematicProperties etancel_sysprop("EtaNCEL","etancel", allmodes,3);//? **
    SystematicProperties mares_sysprop("MaCCRES","mares", resmodes,3);//4
    SystematicProperties mvres_sysprop("MvCCRES","mvres", resmodes,3);//4
    SystematicProperties mancres_sysprop("MaNCRES","mancres", resmodes,3);//4
    SystematicProperties mvncres_sysprop("MvNCRES","mvncres", resmodes,3);//4
    //SystematicProperties rdecbr1gamma_sysprop("RDecBR1gamma", "rdecbr1gamma", resmodes,3);//4 **
    //SystematicProperties rdecbr1eta_sysprop("RDecBR1eta", "rdecbr1eta", resmodes,3);//4 **
    SystematicProperties thetadelta_sysprop("Theta_Delta2Npi", "thetadelta", resmodes,3); //4
    SystematicProperties ahtby_sysprop("AhtBY", "ahtby", dismodes,3); //3
    SystematicProperties bhtby_sysprop("BhtBY", "bhtby", dismodes,3); //3
    SystematicProperties cv1uby_sysprop("CV1uBY", "cv1uby", dismodes,3); //3
    SystematicProperties cv2uby_sysprop("CV2uBY", "cv2uby", dismodes,3); //3
    //SystematicProperties formzone_sysprop("FormZone", "formzone", dismodes,3); //3 **
    //SystematicProperties mfppi_sysprop("MFP_pi", "mfppi", dismodes,3);//3 **
    SystematicProperties frcexpi_sysprop("FrCEx_pi", "frcexpi", disresmodes,3);//3,4
    SystematicProperties frelaspi_sysprop("FrElas_pi", "frelaspi", disresmodes,3);//3,4
    SystematicProperties frinelpi_sysprop("FrInel_pi", "frinelpi", disresmodes,3);//3,4
    SystematicProperties frabspi_sysprop("FrAbs_pi", "frabspi", disresmodes,3);//3,4
    SystematicProperties frpiprodpi_sysprop("FrPiProd_pi", "frpiprodpi", disresmodes,3);//3,4
    //SystematicProperties mfpn_sysprop("MFP_N", "mfpn", mostmodes,3);//1,3,4,10 **
    SystematicProperties frcexn_sysprop("FrCEx_N", "frcexn", mostmodes,3);//1,3,4,10
    SystematicProperties frelasn_sysprop("FrElas_N", "frelasn", mostmodes,3);//1,3,4,10
    SystematicProperties frineln_sysprop("FrInel_N", "frineln", mostmodes,3);//1,3,4,10
    SystematicProperties frabsn_sysprop("FrAbs_N", "frabsn", mostmodes,3);//1,3,4,10
    SystematicProperties frpiprodn_sysprop("FrPiProd_N", "frpiprodn", mostmodes,3);//1,3,4,10
    SystematicProperties pauli_sysprop("CCQEPauliSupViaKF", "pauli", ccqemodes,3);//1
    //SystematicProperties gauss_sysprop("Mnv2p2hGaussEnhancement", "gauss", allmodes,3);//1,3,4,5,10 *
    //SystematicProperties mkspp_sysprop("MKSPP_ReWeight", "mkspp", allmodes,3);//1,3,4,5,10**
    SystematicProperties e2anu_sysprop("E2p2h_A_nu", "e2anu", allmodes,3);//1,3,4,5,7,10 
    SystematicProperties e2bnu_sysprop("E2p2h_B_nu", "e2bnu", allmodes,3);//1,3,4,5,7,10
    SystematicProperties e2anubar_sysprop("E2p2h_A_nubar", "e2anubar", allmodes,3);//1,3,4,5,7,10
    SystematicProperties e2bnubar_sysprop("E2p2h_B_nubar", "e2bnubar", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nuncc2_sysprop("NR_nu_n_CC_2Pi", "nuncc2", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nuncc3_sysprop("NR_nu_n_CC_3Pi", "nuncc3", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nupcc2_sysprop("NR_nu_p_CC_2Pi", "nupcc2", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nupcc3_sysprop("NR_nu_p_CC_3Pi", "nupcc3", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nunpcc1_sysprop("NR_nu_np_CC_1Pi", "nunpcc1", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nunnc1_sysprop("NR_nu_n_NC_1Pi", "nunnc1", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nunnc2_sysprop("NR_nu_n_NC_2Pi", "nunnc2", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nunnc3_sysprop("NR_nu_n_NC_3Pi", "nunnc3", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nupnc1_sysprop("NR_nu_p_NC_1Pi", "nupnc1", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nupnc2_sysprop("NR_nu_p_NC_2Pi", "nupnc2", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nupnc3_sysprop("NR_nu_p_NC_3Pi", "nupnc3", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nubarncc1_sysprop("NR_nubar_n_CC_1Pi", "nubarncc1", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nubarncc2_sysprop("NR_nubar_n_CC_2Pi", "nubarncc2", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nubarncc3_sysprop("NR_nubar_n_CC_3Pi", "nubarncc3", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nubarpcc1_sysprop("NR_nubar_p_CC_1Pi", "nubarpcc1", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nubarpcc2_sysprop("NR_nubar_p_CC_2Pi", "nubarpcc2", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nubarpcc3_sysprop("NR_nubar_p_CC_3Pi", "nubarpcc3", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nubarnnc1_sysprop("NR_nubar_n_NC_1Pi", "nubarnnc1", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nubarnnc2_sysprop("NR_nubar_n_NC_2Pi", "nubarnnc2", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nubarnnc3_sysprop("NR_nubar_n_NC_3Pi", "nubarnnc3", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nubarpnc1_sysprop("NR_nubar_p_NC_1Pi", "nubarpnc1", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nubarpnc2_sysprop("NR_nubar_p_NC_2Pi", "nubarpnc2", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nubarpnc3_sysprop("NR_nubar_p_NC_3Pi", "nubarpnc3", allmodes,3);//1,3,4,5,7,10
    SystematicProperties berpaa_sysprop("BeRPA_A", "berpaa", allmodes,3);//1,3,4,5,7,10
    SystematicProperties berpab_sysprop("BeRPA_B", "berpab", allmodes,3);//1,3,4,5,7,10
    SystematicProperties berpad_sysprop("BeRPA_D", "berpad", allmodes,3);//1,3,4,5,7,10
    //SystematicProperties berpae_sysprop("BeRPA_E", "berpae", allmodes,3);//1,3,4,5,7,10 **
    //SystematicProperties lepmom_sysprop("EbFSLepMomShift", "lepmom", allmodes,3);//1,3,4,5,7,10 **
    SystematicProperties c12nu_sysprop("C12ToAr40_2p2hScaling_nu", "c12nu", allmodes,3);//1,3,4,5,7,10
    SystematicProperties c12nubar_sysprop("C12ToAr40_2p2hScaling_nubar", "c12nubar", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nuexsec_sysprop("nuenuebar_xsec_ratio", "nuexsec", allmodes,3);//1,3,4,5,7,10
    SystematicProperties nuemuxsec_sysprop("nuenumu_xsec_ratio", "nuemuxsec", allmodes,3);//1,3,4,5,7,10
    //SystematicProperties q2sup_sysprop("SPPLowQ2Suppression", "q2sup", allmodes,3);//1,3,4,5,10 **
    //SystematicProperties fsismear_sysprop("FSILikeEAvailSmearing", "fsismear", allmodes,3);//1,3,4,5,7,10 **

    //SystematicProperties mec_sysprop("NIWGMEC_PDDWeight_O16","mecpdd",mecmode,3);
    //SystematicProperties ca5_sysprop("NXSec_CA5RES","ca5",pimodes,3);
    //SystematicProperties manff_sysprop("NXSec_MaRES","manff",pimodes,3);
    //SystematicProperties bgscl_sysprop("NXSec_BgSclRES","bgscl",pimodes,3);
	//ETA adding I-1/2 anti-nu parameter
	//SystematicProperties bgsclbar_sysprop("NXSec_BgSclLMCPiBarRES", "bgsclbar", pimodes, 3);

	// Due to special treatment of FSI params the nominal knot for FSIPIPROD is knot 1
	//SystematicProperties FSI_NCasc_FrAbs_pi_sysprop("NCasc_FrAbs_pi","FSIPIABS",allmodes,3);
	//SystematicProperties FSI_NCasc_FrInelLow_pi_sysprop("NCasc_FrInelLow_pi","FSIINELLO",allmodes,3);
	//SystematicProperties FSI_NCasc_FrCExLow_pi_sysprop("NCasc_FrCExLow_pi","FSICEXLO",allmodes,3);
	//SystematicProperties FSI_NCasc_FrCExHigh_pi_sysprop("NCasc_FrCExHigh_pi","FSICEXHI",allmodes,3);  //ETA- NOT USUALLY IN THE OA
	//SystematicProperties FSI_NCasc_FrInelHigh_pi_sysprop("NCasc_FrInelHigh_pi","FSIINELHI",allmodes,3);
	//SystematicProperties FSI_NCasc_FrPiProd_pi_sysprop("NCasc_FrPiProd_pi","FSIPIPROD",allmodes,1);
	// ETA CCDIS and CCMpi parameters for 2020OA
	//SystematicProperties BY_DIS_sysprop("NIWG_DIS_BY", "DISBY", ccdismode,3);
	//SystematicProperties BY_MPi_sysprop("NIWG_MultiPi_BY", "MPiBY", ccmpimode,3);
	//SystematicProperties Xsec_AGKY_sysprop("NIWG_MultiPi_Xsec_AGKY", "MPiAGKYXsec", ccmpimode,3);
	//ETA adding 2p2h which wasn't in this branch...
	//SystematicProperties Edep2p2h_lowEnu_sysprop("NIWG_2p2hEdep_lowEnu","2p2hedeplowenu", mecmode, 1);
	//SystematicProperties Edep2p2h_highEnu_sysprop("NIWG_2p2hEdep_highEnu", "2p2hedephienu", mecmode, 1);
	//SystematicProperties Edep2p2h_lowEnubar_sysprop("NIWG_2p2hEdep_lowEnubar", "2p2hedeplowenubar", mecmode, 1);
	//SystematicProperties Edep2p2h_highEnubar_sysprop("NIWG_2p2hEdep_highEnubar", "2p2hedephienubar", mecmode, 1);
	//ETA adding low Q2 params for VALOR
	//Changing the name of the spline to be consistent with ND280
	// i.e. move to C++ indexing in the name                     
	//SystematicProperties LowQ2Suppression_1_sysprop("NIWGQETwk_LowQ2Suppression1", "LowQ2Suppression0", ccqemode, 3);
	//SystematicProperties LowQ2Suppression_2_sysprop("NIWGQETwk_LowQ2Suppression2", "LowQ2Suppression1", ccqemode, 3);
	//SystematicProperties LowQ2Suppression_3_sysprop("NIWGQETwk_LowQ2Suppression3", "LowQ2Suppression2", ccqemode, 3);
	//SystematicProperties LowQ2Suppression_4_sysprop("NIWGQETwk_LowQ2Suppression4", "LowQ2Suppression3", ccqemode, 3);
	//SystematicProperties LowQ2Suppression_5_sysprop("NIWGQETwk_LowQ2Suppression5", "LowQ2Suppression4", ccqemode, 3);
	//SystematicProperties LowQ2Suppression_6_sysprop("NIWGQETwk_LowQ2Suppression6", "LowQ2Suppression5", ccqemode, 3);
	//SystematicProperties LowQ2Suppression_7_sysprop("NIWGQETwk_LowQ2Suppression7", "LowQ2Suppression6", ccqemode, 3);
	//SystematicProperties LowQ2Suppression_8_sysprop("NIWGQETwk_LowQ2Suppression8", "LowQ2Suppression7", ccqemode, 3);

    // Put these systematics in a list
    std::vector<SystematicProperties> systProps;   
    systProps.push_back(maqe_sysprop);
    systProps.push_back(vecffqeshape_sysprop);
    //systProps.push_back(mancel_sysprop);
    //systProps.push_back(etancel_sysprop);
    systProps.push_back(mares_sysprop);
    systProps.push_back(mvres_sysprop);
    systProps.push_back(mancres_sysprop);
    systProps.push_back(mvncres_sysprop);
    //systProps.push_back(rdecbr1gamma_sysprop);
    //systProps.push_back(rdecbr1eta_sysprop);
    systProps.push_back(thetadelta_sysprop);
    systProps.push_back(ahtby_sysprop);
    systProps.push_back(bhtby_sysprop);
    systProps.push_back(cv1uby_sysprop);
    systProps.push_back(cv2uby_sysprop);
    //systProps.push_back(formzone_sysprop);
    //systProps.push_back(mfppi_sysprop);
    systProps.push_back(frcexpi_sysprop);
    systProps.push_back(frelaspi_sysprop);
    systProps.push_back(frinelpi_sysprop);
    systProps.push_back(frabspi_sysprop);
    systProps.push_back(frpiprodpi_sysprop);
    //systProps.push_back(mfpn_sysprop);
    systProps.push_back(frcexn_sysprop);
    systProps.push_back(frelasn_sysprop);
    systProps.push_back(frineln_sysprop);
    systProps.push_back(frabsn_sysprop);
    systProps.push_back(frpiprodn_sysprop);
    systProps.push_back(pauli_sysprop);
    //systProps.push_back(gauss_sysprop);
    //systProps.push_back(mkspp_sysprop);
    systProps.push_back(e2anu_sysprop);
    systProps.push_back(e2bnu_sysprop);
    systProps.push_back(e2anubar_sysprop);
    systProps.push_back(e2bnubar_sysprop);
    systProps.push_back(nuncc2_sysprop);
    systProps.push_back(nuncc3_sysprop);
    systProps.push_back(nupcc2_sysprop);
    systProps.push_back(nupcc3_sysprop);
    systProps.push_back(nunpcc1_sysprop);
    systProps.push_back(nunnc1_sysprop);
    systProps.push_back(nunnc2_sysprop);
    systProps.push_back(nunnc3_sysprop);
    systProps.push_back(nupnc1_sysprop);
    systProps.push_back(nupnc2_sysprop);
    systProps.push_back(nupnc3_sysprop);
    systProps.push_back(nubarncc1_sysprop);
    systProps.push_back(nubarncc2_sysprop);
    systProps.push_back(nubarncc3_sysprop);
    systProps.push_back(nubarpcc1_sysprop);
    systProps.push_back(nubarpcc2_sysprop);
    systProps.push_back(nubarpcc3_sysprop);
    systProps.push_back(nubarnnc1_sysprop);
    systProps.push_back(nubarnnc2_sysprop);
    systProps.push_back(nubarnnc3_sysprop);
    systProps.push_back(nubarpnc1_sysprop);
    systProps.push_back(nubarpnc2_sysprop);
    systProps.push_back(nubarpnc3_sysprop);
    systProps.push_back(berpaa_sysprop);
    systProps.push_back(berpab_sysprop);
    systProps.push_back(berpad_sysprop);
    //systProps.push_back(berpae_sysprop);
    //systProps.push_back(lepmom_sysprop);
    systProps.push_back(c12nu_sysprop);
    systProps.push_back(c12nubar_sysprop);
    systProps.push_back(nuexsec_sysprop);
    systProps.push_back(nuemuxsec_sysprop);
    //systProps.push_back(q2sup_sysprop);
    //systProps.push_back(fsismear_sysprop);

    // Call the XSecVary class constructor
    XsecVary xs(wtfile, mtuple, systProps, nutype);

    // ---------------------------------------------------------------------------------- //
    //                                                                                    //
    //                End block for specifying systematics treatments.                    //
    //                                                                                    //
    // ---------------------------------------------------------------------------------- //

 
    // For binned splines: set binning then make variations.
    if (byEvent_splines == false)
      {
	// Load the flux covariance matrix and extract energy binning
	// Determine which true energy binning based on the first event
	// in the file
	xs.GetEntry(0);

	const int ebins = recoe_temps[0]->GetXaxis()->GetNbins();
        const Double_t *ebinrange = recoe_temps[0]->GetXaxis()->GetXbins()->GetArray();
	std::cout << "Number of true energy bins: " << ebins << "." << std::endl;
	std::cout << "True energy bins: {";
	for (int ibin=0; ibin<ebins; ibin++)
	  std::cout << ebinrange[ibin] << ", ";
	std::cout << ebinrange[ebins] << "}" << std::endl;
	
	const int rebins = recoe_temps_erec[0]->GetXaxis()->GetNbins();
	//const Double_t *rebinrange = recoe_temps_erec[0]->GetXaxis()->GetXbins()->GetArray();
        const Double_t *rebinrange = recoe_temps_erec[0]->GetXaxis()->GetXbins()->GetArray();
        std::cout << "Number of reco energy bins: " << rebins << "." << std::endl;
	std::cout << "Reco energy bins: {";
	for (int ibin=0; ibin<rebins; ibin++)
	  std::cout << rebinrange[ibin] << ", ";
	std::cout << rebinrange[rebins] << "}" << std::endl;
 
	
	xs.SetBinning(ebinrange,ebins,rebinrange,rebins);
	
	std::cout <<  "Binning implemented successfully" << std::endl; 

	xs.MakeVariations();

	xs.WriteGraphs(outfile);
        
        //Code to create bin Template
         
        //double bins[] = {0.,  0.5,  1.,  1.25, 1.5, 1.75, 2.,  2.25, 2.5, 2.75, 3.,  3.25, 3.5, 3.75, 4.,  5.,   6.,  10.};       

        //TH1D * hrecoe_0 = new TH1D("hrecoe_0", "reconstructed E#nu for sample", 17, bins);



        //for (int i = 1; i <= hrecoe_0->GetNbinsX(); i++) { 
          //cout << i << " : " << hrecoe_0->GetBinLowEdge(i) << "-" << hrecoe_0->GetBinLowEdge(i+1) << endl; 
    //}

        //TFile *fout = new TFile("DUNE_numu_templates_v0_etrue.root", "recreate");
        //hrecoe_0->Write();
        //fout->Write();
        //fout->Close();


      }
/*
    else // Event-by-event splines 
      {
	xs.InitByEventTree(13, outfile);
	xs.MakeVariations_byEvent();

	xs.WriteByEventTree();
      }
*/

    return 0;
}

// Print the cmd line syntax
void Usage(){
    cout << "Cmd line syntax should be:" << endl;
    cout << "./bin/make_xsec_response.exe [-t 'byEv' (for event by event splines)] -w wtfile -m mtuple -o outfile [-nue (for nue selection, as opposed to numu)]" << endl;
}

// Messy way to process cmd line arguments.
void ParseArgs(int argc, char **argv){
    //  std::cout << "parse args" << std::endl;
    int nargs = 1; 
    if(argc<(nargs*2+1)){ Usage(); exit(1); }
    for(int i = 1; i < argc; i+=2){
        if(std::string(argv[i]) == "-selec") selection = argv[i+1];
        else if(std::string(argv[i]) == "-w") wtfile = argv[i+1];
        else if(std::string(argv[i]) == "-m") mtuple = argv[i+1];
        else if(std::string(argv[i]) == "-o") outfile = argv[i+1];
	else if(std::string(argv[i]) == "-t") 
	  {
	    if (std::string(argv[i+1])=="byEv") byEvent_splines=true;
	    else byEvent_splines=false;
	  }
	else if(std::string(argv[i]) == "-f")
	  {
	    if(std::string(argv[i+1])=="fine")
	      {
		fTempFilenue =  "inputs/sk_nuepi0_templates_narrow.root";
		fTempFilenumu =  "inputs/sk_numu_templates.root";
	      }
	  }
        else 
	  {  
            cout << "Invalid argument:" << argv[i] << " "<< argv[i+1] << endl;
            Usage();
            exit(1);
	  }
    } 
}


void ReadTemplates(TFile *ftemp, std::vector<TH1F*> &recoe_temps){
    bool found_temp = true;
    int temp_iter=0;
    while(found_temp){
        char temp_name[50];
        sprintf(temp_name,"hrecoe_%d",temp_iter); 
        TH1F *blah = NULL;
        blah = (TH1F*)(ftemp->Get(temp_name));
        if(blah!=NULL){
            recoe_temps.push_back(blah);
            temp_iter++;
	    std::cout << temp_name << std::endl
		      << blah << std::endl
		      << blah->GetName() << std::endl
		      << blah->GetXaxis()->GetNbins()
		      << std::endl;
        } else found_temp = false;
    }
}
