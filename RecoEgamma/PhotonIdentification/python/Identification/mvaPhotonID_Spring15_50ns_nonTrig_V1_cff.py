from PhysicsTools.SelectorUtils.centralIDRegistry import central_id_registry

import FWCore.ParameterSet.Config as cms

#
# In this file we define the locations of the MVA weights, cuts on the MVA values
# for specific working points, and configure those cuts in VID
#

#
# The following MVA is derived for Spring15 MC samples for non-triggering photons.
# See more documentation in this presentation:
#    https://indico.cern.ch/event/369237/contribution/2/attachments/1128009/1611753/egamma-July17-2015.pdf
#

# This MVA implementation class name
mvaSpring15NonTrigClassName = "PhotonMVAEstimatorRun2Spring15NonTrig"

# There are 2 categories in this MVA. They have to be configured in this strict order
# (cuts and weight files order):
#   0    barrel photons
#   1    endcap photons

mvaSpring15NonTrigWeightFiles_V1 = cms.vstring(
    "RecoEgamma/PhotonIdentification/data/Spring15/photon_general_MVA_Spring15_50ns_EB_V1.weights.xml",
    "RecoEgamma/PhotonIdentification/data/Spring15/photon_general_MVA_Spring15_50ns_EE_V1.weights.xml"
    )

# Load some common definitions for MVA machinery
from RecoEgamma.PhotonIdentification.Identification.mvaPhotonID_tools import *

# The locatoins of value maps with the actual MVA values and categories
# for all particles.
# The names for the maps are "<module name>:<MVA class name>Values" 
# and "<module name>:<MVA class name>Categories"
mvaProducerModuleLabel = "photonMVAValueMapProducer"
mvaValueMapName        = mvaProducerModuleLabel + ":" + mvaSpring15NonTrigClassName + "Values"
mvaCategoriesMapName   = mvaProducerModuleLabel + ":" + mvaSpring15NonTrigClassName + "Categories"

# The working point for this MVA that is expected to have about 90% signal
# efficiency in each category for photons with pt>30 GeV (somewhat lower
# for lower pt photons).
idName = "mvaPhoID-Spring15-50ns-nonTrig-V1-wp90"
MVA_WP90 = PhoMVA_2Categories_WP(
    idName = idName,
    mvaValueMapName = mvaValueMapName,           # map with MVA values for all particles
    mvaCategoriesMapName = mvaCategoriesMapName, # map with category index for all particles
    cutCategory0 = 0.284, # EB
    cutCategory1 = 0.432  # EE
    )

#
# Finally, set up VID configuration for all cuts
#

# Create the PSet that will be fed to the MVA value map producer
mvaPhoID_Spring15_50ns_nonTrig_V1_producer_config = cms.PSet( 
    mvaName            = cms.string(mvaSpring15NonTrigClassName),
    weightFileNames    = mvaSpring15NonTrigWeightFiles_V1,
    #
    # All the event content needed for this MVA implementation follows
    #
    # All the value maps: these are expected to be produced by the
    # PhotonIDValueMapProducer running upstream
    #
    full5x5SigmaIEtaIEtaMap   = cms.InputTag("photonIDValueMapProducer:phoFull5x5SigmaIEtaIEta"),
    full5x5SigmaIEtaIPhiMap   = cms.InputTag("photonIDValueMapProducer:phoFull5x5SigmaIEtaIPhi"),
    full5x5E1x3Map      = cms.InputTag("photonIDValueMapProducer:phoFull5x5E1x3"),
    full5x5E2x2Map      = cms.InputTag("photonIDValueMapProducer:phoFull5x5E2x2"),
    full5x5E2x5MaxMap   = cms.InputTag("photonIDValueMapProducer:phoFull5x5E2x5Max"),
    full5x5E5x5Map      = cms.InputTag("photonIDValueMapProducer:phoFull5x5E5x5"),
    esEffSigmaRRMap     = cms.InputTag("photonIDValueMapProducer:phoESEffSigmaRR"),
    phoChargedIsolation = cms.InputTag("photonIDValueMapProducer:phoChargedIsolation"),
    phoPhotonIsolation  = cms.InputTag("photonIDValueMapProducer:phoPhotonIsolation"),
    phoWorstChargedIsolation = cms.InputTag("photonIDValueMapProducer:phoWorstChargedIsolation"),
    #
    # Original event content: pileup in this case
    # 
    rho                       = cms.InputTag("fixedGridRhoFastjetAll") 
    )
# Create the VPset's for VID cuts
mvaPhoID_Spring15_50ns_nonTrig_V1_wp90 = configureVIDMVAPhoID_V1( MVA_WP90 )

# The MD5 sum numbers below reflect the exact set of cut variables
# and values above. If anything changes, one has to 
# 1) comment out the lines below about the registry, 
# 2) run "calculateMD5 <this file name> <one of the VID config names just above>
# 3) update the MD5 sum strings below and uncomment the lines again.
#

central_id_registry.register( mvaPhoID_Spring15_50ns_nonTrig_V1_wp90.idName,
                              '26d6df804ad7045c1f62d61d02de6ffd')

mvaPhoID_Spring15_50ns_nonTrig_V1_wp90.isPOGApproved = cms.untracked.bool(True)
