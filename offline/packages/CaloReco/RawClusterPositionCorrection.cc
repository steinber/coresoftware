#include "RawClusterPositionCorrection.h"

#include <calobase/RawCluster.h>
#include <calobase/RawClusterContainer.h>
#include <calobase/RawTower.h>
#include <calobase/RawTowerContainer.h>
#include <calobase/RawTowerDefs.h>           // for decode_index1, decode_in...
#include <calobase/RawTowerGeomContainer.h>

#include <calobase/TowerInfo.h>
#include <calobase/TowerInfoContainer.h>

#include <phparameter/PHParameters.h>

#include <fun4all/Fun4AllReturnCodes.h>
#include <fun4all/SubsysReco.h>

#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>
#include <phool/PHNode.h>
#include <phool/PHNodeIterator.h>
#include <phool/PHObject.h>
#include <phool/getClass.h>
#include <phool/phool.h>

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>  // for pair

RawClusterPositionCorrection::RawClusterPositionCorrection(const std::string &name)
  : SubsysReco(std::string("RawClusterPositionCorrection_") + name)
  , _eclus_calib_params(std::string("eclus_params_") + name)
  , _ecore_calib_params(std::string("ecore_params_") + name)
  , _det_name(name)
  , bins(17)  // default bins to be 17 to set default recalib parameters to 1
{
  SetDefaultParameters(_eclus_calib_params);
  SetDefaultParameters(_ecore_calib_params);
}

int RawClusterPositionCorrection::InitRun(PHCompositeNode *topNode)
{
  CreateNodeTree(topNode);

  if (Verbosity())
  {
    std::cout << "RawClusterPositionCorrection is running for clusters in the EMCal with eclus parameters:" << std::endl;
    _eclus_calib_params.Print();

    std::cout << "RawClusterPositionCorrection is running for clusters in the EMCal with ecore parameters:" << std::endl;
    _ecore_calib_params.Print();
  }
  // now get the actual number of bins in the calib file
  std::ostringstream paramname;
  paramname.str("");
  paramname << "number_of_bins";

  //+1 because I use bins as the total number of bin boundaries
  // i.e. 16 bins corresponds to 17 bin boundaries
  bins = _eclus_calib_params.get_int_param(paramname.str()) + 1;

  // set bin boundaries

  for (int j = 0; j < bins; j++)
  {
    binvals.push_back(0. + j * 2. / (float) (bins - 1));
  }

  for (int i = 0; i < bins - 1; i++)
  {
    std::vector<double> dumvec;

    for (int j = 0; j < bins - 1; j++)
    {
      std::ostringstream calib_const_name;
      calib_const_name.str("");
      calib_const_name << "recalib_const_eta"
                       << i << "_phi" << j;
      dumvec.push_back(_eclus_calib_params.get_double_param(calib_const_name.str()));
    }
    eclus_calib_constants.push_back(dumvec);
  }

  for (int i = 0; i < bins - 1; i++)
  {
    std::vector<double> dumvec;

    for (int j = 0; j < bins - 1; j++)
    {
      std::ostringstream calib_const_name;
      calib_const_name.str("");
      calib_const_name << "recalib_const_eta"
                       << i << "_phi" << j;
      dumvec.push_back(_ecore_calib_params.get_double_param(calib_const_name.str()));
    }
    ecore_calib_constants.push_back(dumvec);
  }

  return Fun4AllReturnCodes::EVENT_OK;
}

int RawClusterPositionCorrection::process_event(PHCompositeNode *topNode)
{
  if (Verbosity())
  {
    std::cout << "Processing a NEW EVENT" << std::endl;
  }

  std::string rawClusNodeName = "CLUSTER_" + _det_name;
  if (m_UseTowerInfo)
  {
    rawClusNodeName = "CLUSTERINFO_" + _det_name;
  }

  RawClusterContainer *rawclusters = findNode::getClass<RawClusterContainer>(topNode, rawClusNodeName.c_str());
  if (!rawclusters)
  {
    std::cout << "No " << _det_name << " Cluster Container found while in RawClusterPositionCorrection, can't proceed!!!" << std::endl;
    return Fun4AllReturnCodes::ABORTEVENT;
  }

  RawTowerContainer *_towers = nullptr;
  TowerInfoContainer *_towerinfos = nullptr;

  if (!m_UseTowerInfo)
  {
    _towers = findNode::getClass<RawTowerContainer>(topNode, "TOWER_CALIB_" + _det_name);
    if (!_towers)
    {
      std::cout << "No calibrated " << _det_name << " tower info found while in RawClusterPositionCorrection, can't proceed!!!" << std::endl;
      return Fun4AllReturnCodes::ABORTEVENT;
    }
  }
  else
  {
    std::string towerinfoNodename = "TOWERINFO_CALIB_" + _det_name;

    _towerinfos = findNode::getClass<TowerInfoContainer>(topNode, towerinfoNodename);
    if (!_towerinfos)
    {
      std::cerr << Name() << "::" << _det_name << "::" << __PRETTY_FUNCTION__
                << " " << towerinfoNodename << " Node missing, doing bail out!"
                << std::endl;

      return Fun4AllReturnCodes::DISCARDEVENT;
    }
  }

  std::string towergeomnodename = "TOWERGEOM_" + _det_name;
  RawTowerGeomContainer *towergeom = findNode::getClass<RawTowerGeomContainer>(topNode, towergeomnodename);
  if (!towergeom)
  {
    std::cout << PHWHERE << ": Could not find node " << towergeomnodename << std::endl;
    return Fun4AllReturnCodes::ABORTEVENT;
  }
  const int nphibin = towergeom->get_phibins();

  // loop over the clusters
  RawClusterContainer::ConstRange begin_end = rawclusters->getClusters();
  RawClusterContainer::ConstIterator iter;

  for (iter = begin_end.first; iter != begin_end.second; ++iter)
  {
    //    RawClusterDefs::keytype key = iter->first;
    RawCluster *cluster = iter->second;

    float clus_energy = cluster->get_energy();

    std::vector<float> toweretas;
    std::vector<float> towerphis;
    std::vector<float> towerenergies;

    // loop over the towers in the cluster
    RawCluster::TowerConstRange towers = cluster->get_towers();
    RawCluster::TowerConstIterator toweriter;
    for (toweriter = towers.first;
         toweriter != towers.second;
         ++toweriter)
    {
      if (!m_UseTowerInfo)
      {
        RawTower *tower = _towers->getTower(toweriter->first);

        int towereta = tower->get_bineta();
        int towerphi = tower->get_binphi();
        double towerenergy = tower->get_energy();

        // put the etabin, phibin, and energy into the corresponding vectors
        toweretas.push_back(towereta);
        towerphis.push_back(towerphi);
        towerenergies.push_back(towerenergy);
      }
      else
      {
        // std::cout << "clus " << iter->first << " tow key " << toweriter->first << " decoded to " << towerindex << std::endl;

        int iphi = RawTowerDefs::decode_index2(toweriter->first);  // index2 is phi in CYL
        int ieta = RawTowerDefs::decode_index1(toweriter->first);  // index1 is eta in CYL
        unsigned int towerkey = iphi + (ieta << 16U);

        unsigned int towerindex = _towerinfos->decode_key(towerkey);

        TowerInfo *towinfo = _towerinfos->get_tower_at_channel(towerindex);

        double towerenergy = towinfo->get_energy();

        // put the eta, phi, energy into corresponding vectors
        toweretas.push_back(ieta);
        towerphis.push_back(iphi);
        towerenergies.push_back(towerenergy);
      }
    }

    int ntowers = toweretas.size();
    //    std::cout << "jf " <<  ntowers << std::endl;
    assert(ntowers >= 1);

    // loop over the towers to determine the energy
    // weighted eta and phi position of the cluster

    float etamult = 0;
    float etasum = 0;
    float phimult = 0;
    float phisum = 0;

    for (int j = 0; j < ntowers; j++)
    {
      float energymult = towerenergies.at(j) * toweretas.at(j);
      etamult += energymult;
      etasum += towerenergies.at(j);

      int phibin = towerphis.at(j);

      if (phibin - towerphis.at(0) < -nphibin / 2.0)
      {
        phibin += nphibin;
      }
      else if (phibin - towerphis.at(0) > +nphibin / 2.0)
      {
        phibin -= nphibin;
      }
      assert(std::abs(phibin - towerphis.at(0)) <= nphibin / 2.0);

      energymult = towerenergies.at(j) * phibin;
      phimult += energymult;
      phisum += towerenergies.at(j);
    }

    float avgphi = phimult / phisum;
    float avgeta = etamult / etasum;

    if (avgphi < 0)
    {
      avgphi += nphibin;
    }

    // this determines the position of the cluster in the 2x2 block
    float fmodphi = fmod(avgphi, 2.);
    float fmodeta = fmod(avgeta, 2.);

    // determine the bin number
    // 2 is here since we divide the 2x2 block into 16 bins in eta/phi

    int etabin = -99;
    int phibin = -99;
    for (int j = 0; j < bins - 1; j++)
    {
      if (fmodphi >= binvals.at(j) && fmodphi <= binvals.at(j + 1))
      {
        phibin = j;
      }
    }

    for (int j = 0; j < bins - 1; j++)
    {
      if (fmodeta >= binvals.at(j) && fmodeta <= binvals.at(j + 1))
      {
        etabin = j;
      }
    }

    if ((phibin < 0 || etabin < 0) && Verbosity())
    {
      if (Verbosity())
      {
        std::cout << "couldn't recalibrate cluster, something went wrong??" << std::endl;
      }
    }

    float eclus_recalib_val = 1;
    float ecore_recalib_val = 1;
    if (phibin > -1 && etabin > -1)
    {
      eclus_recalib_val = eclus_calib_constants.at(etabin).at(phibin);
      ecore_recalib_val = ecore_calib_constants.at(etabin).at(phibin);
    }
    RawCluster *recalibcluster = dynamic_cast<RawCluster *>(cluster->CloneMe());
    assert(recalibcluster);
    //    if (m_UseTowerInfo)
    //  std::cout << "and here" << std::endl;
    recalibcluster->set_energy(clus_energy / eclus_recalib_val);
    recalibcluster->set_ecore(cluster->get_ecore() / ecore_recalib_val);
    _recalib_clusters->AddCluster(recalibcluster);

    if (Verbosity() && clus_energy > 1)
    {
      std::cout << "Input eclus cluster energy: " << clus_energy << std::endl;
      std::cout << "Recalib value: " << eclus_recalib_val << std::endl;
      std::cout << "Recalibrated eclus cluster energy: "
                << clus_energy / eclus_recalib_val << std::endl;
      std::cout << "Input ecore cluster energy: "
                << cluster->get_ecore() << std::endl;
      std::cout << "Recalib value: " << ecore_recalib_val << std::endl;
      std::cout << "Recalibrated eclus cluster energy: "
                << cluster->get_ecore() / ecore_recalib_val << std::endl;
    }
  }

  return Fun4AllReturnCodes::EVENT_OK;
}

void RawClusterPositionCorrection::CreateNodeTree(PHCompositeNode *topNode)
{
  PHNodeIterator iter(topNode);

  // Get the DST Node
  PHCompositeNode *dstNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "DST"));

  // Check that it is there
  if (!dstNode)
  {
    std::cout << "DST Node missing, quitting" << std::endl;
    throw std::runtime_error("failed to find DST node in RawClusterPositionCorrection::CreateNodeTree");
  }

  // Get the _det_name subnode
  PHCompositeNode *cemcNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", _det_name));

  // Check that it is there
  if (!cemcNode)
  {
    cemcNode = new PHCompositeNode(_det_name);
    dstNode->addNode(cemcNode);
  }

  // Check to see if the cluster recalib node is on the nodetree
  _recalib_clusters = findNode::getClass<RawClusterContainer>(topNode, "CLUSTER_RECALIB_" + _det_name);
  std::string ClusterCorrNodeName = "CLUSTER_POS_COR_" + _det_name;
  ;

  // If not, make it and add it to the _det_name subnode
  if (!_recalib_clusters)
  {
    _recalib_clusters = new RawClusterContainer();
    if (m_UseTowerInfo)
    {
      ClusterCorrNodeName = "CLUSTERINFO_POS_COR_" + _det_name;
    }

    PHIODataNode<PHObject> *clusterNode = new PHIODataNode<PHObject>(_recalib_clusters, ClusterCorrNodeName.c_str(), "PHObject");
    cemcNode->addNode(clusterNode);
  }

  // put the recalib parameters on the node tree
  PHCompositeNode *parNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "RUN"));
  assert(parNode);

  std::string paramNodeName = std::string("eclus_Recalibration_" + _det_name);
  std::string paramNodeName2 = std::string("ecore_Recalibration_" + _det_name);

  if (m_UseTowerInfo)
  {
    paramNodeName = std::string("eclus_RecalibrationInfo_" + _det_name);
    paramNodeName2 = std::string("ecore_RecalibrationInfo_" + _det_name);
  }

  _eclus_calib_params.SaveToNodeTree(parNode, paramNodeName);
  _ecore_calib_params.SaveToNodeTree(parNode, paramNodeName2);
}
int RawClusterPositionCorrection::End(PHCompositeNode * /*topNode*/)
{
  return Fun4AllReturnCodes::EVENT_OK;
}
void RawClusterPositionCorrection::SetDefaultParameters(PHParameters &param)
{
  param.set_int_param("number_of_bins", 17);

  std::ostringstream param_name;
  for (int i = 0; i < bins - 1; i++)
  {
    for (int j = 0; j < bins - 1; j++)
    {
      param_name.str("");
      param_name << "recalib_const_eta"
                 << i << "_phi" << j;

      // default to 1, i.e. no recalibration
      param.set_double_param(param_name.str(), 1.0);
    }
  }
}
