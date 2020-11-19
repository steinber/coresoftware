#ifndef G4HISTOS_G4VTXNTUPLE_H
#define G4HISTOS_G4VTXNTUPLE_H

#include <fun4all/SubsysReco.h>

#include <map>
#include <set>
#include <string>
#include <vector>

// Forward declerations
class Fun4AllHistoManager;
class PHCompositeNode;
class TFile;
class TH1;
class TNtuple;

class G4VtxNtuple : public SubsysReco
{
 public:
  //! constructor
  G4VtxNtuple(const std::string &name = "G4VtxNtuple", const std::string &filename = "G4VtxNtuple.root");

  //! destructor
  virtual ~G4VtxNtuple();

  //! full initialization
  int Init(PHCompositeNode *);

  //! event processing method
  int process_event(PHCompositeNode *);

  //! end of run method
  int End(PHCompositeNode *);

 protected:
  std::string m_FileName;
  Fun4AllHistoManager *hm;
  TNtuple *ntup;
};

#endif
