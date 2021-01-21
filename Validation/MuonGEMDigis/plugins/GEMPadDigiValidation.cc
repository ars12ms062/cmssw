#include "Validation/MuonGEMDigis/plugins/GEMPadDigiValidation.h"

GEMPadDigiValidation::GEMPadDigiValidation(const edm::ParameterSet& pset)
    : GEMBaseValidation(pset, "GEMPadDigiValidation") {
  const auto& pad_pset = pset.getParameterSet("gemPadDigi");
  const auto& pad_tag = pad_pset.getParameter<edm::InputTag>("inputTag");
  pad_token_ = consumes<GEMPadDigiCollection>(pad_tag);
  geomToken_ = esConsumes<GEMGeometry, MuonGeometryRecord>();
  geomTokenBeginRun_ = esConsumes<GEMGeometry, MuonGeometryRecord, edm::Transition::BeginRun>();
}

void GEMPadDigiValidation::bookHistograms(DQMStore::IBooker& booker,
                                          edm::Run const& Run,
                                          edm::EventSetup const& setup) {
  const GEMGeometry* gem = &setup.getData(geomTokenBeginRun_);

  // NOTE Occupancy
  booker.setCurrentFolder("MuonGEMDigisV/GEMDigisTask/Pad/Occupancy");

  for (const auto& region : gem->regions()) {
    Int_t region_id = region->region();

    if (detail_plot_) me_detail_occ_zr_[region_id] = bookZROccupancy(booker, region_id, "pad", "Pad");

    for (const auto& station : region->stations()) {
      Int_t station_id = station->station();
      ME2IdsKey key2(region_id, station_id);

      if (detail_plot_) me_detail_occ_det_[key2] = bookDetectorOccupancy(booker, key2, station, "pad", "Pad");

      const auto& superChamberVec = station->superChambers();
      if (superChamberVec.empty()) {
        edm::LogError(kLogCategory_) << "Super chambers missing for region = " << region_id
                                     << " and station = " << station_id;
        continue;
      }
      const GEMSuperChamber* super_chamber = superChamberVec.front();
      if (super_chamber == nullptr) {
        edm::LogError(kLogCategory_) << "Failed to find super chamber for region = " << region_id
                                     << " and station = " << station_id;
        continue;
      }
      for (const auto& chamber : super_chamber->chambers()) {
        Int_t layer_id = chamber->id().layer();
        ME3IdsKey key3(region_id, station_id, layer_id);

        const auto& etaPartitionVec = chamber->etaPartitions();
        if (etaPartitionVec.empty() || etaPartitionVec.front() == nullptr) {
          edm::LogError(kLogCategory_)
              << "Eta partition missing or null for region, station, super chamber, chamber = (" << region_id << ", "
              << station_id << ", " << super_chamber->id() << ", " << chamber->id() << ")";
          continue;
        }
        Int_t num_pads = etaPartitionVec.front()->npads();

        if (detail_plot_) {
          me_detail_occ_xy_[key3] = bookXYOccupancy(booker, key3, "pad", "Pad");

          me_detail_occ_phi_pad_[key3] = bookHist2D(booker,
                                                    key3,
                                                    "occ_phi_pad",
                                                    "Pad Occupancy",
                                                    280,
                                                    -M_PI,
                                                    M_PI,
                                                    num_pads / 2,
                                                    0,
                                                    num_pads,
                                                    "#phi [rad]",
                                                    "Pad number");

          me_detail_occ_pad_[key3] =
              bookHist1D(booker, key3, "occ_pad", "Pad Occupancy", num_pads, 0.5, num_pads + 0.5, "Pad number");
        }
      }  // layer loop
    }    // station loop
  }      // region loop

  // NOTE Bunch Crossing
  if (detail_plot_) {
    booker.setCurrentFolder("MuonGEMDigisV/GEMDigisTask/Pad/BunchCrossing");

    for (const auto& region : gem->regions()) {
      Int_t region_id = region->region();
      for (const auto& station : region->stations()) {
        Int_t station_id = station->station();

        const auto& superChamberVec = station->superChambers();
        if (superChamberVec.empty()) {
          edm::LogError(kLogCategory_) << "Super chambers missing for region = " << region_id
                                       << " and station = " << station_id;
          continue;
        }
        const GEMSuperChamber* super_chamber = superChamberVec.front();
        if (super_chamber == nullptr) {
          edm::LogError(kLogCategory_) << "Failed to find super chamber for region = " << region_id
                                       << " and station = " << station_id;
          continue;
        }
        for (const auto& chamber : super_chamber->chambers()) {
          Int_t layer_id = chamber->id().layer();
          ME3IdsKey key3(region_id, station_id, layer_id);

          me_detail_bx_[key3] = bookHist1D(booker, key3, "bx", "Pad Bunch Crossing", 5, -2.5, 2.5, "Bunch crossing");
        }  // chamber loop
      }    // station loop
    }      // region loop
  }        // detail plot
}

GEMPadDigiValidation::~GEMPadDigiValidation() {}

void GEMPadDigiValidation::analyze(const edm::Event& event, const edm::EventSetup& setup) {
  const GEMGeometry* gem = &setup.getData(geomToken_);

  edm::Handle<GEMPadDigiCollection> collection;
  event.getByToken(pad_token_, collection);
  if (not collection.isValid()) {
    edm::LogError(kLogCategory_) << "Cannot get pads by label GEMPadToken.";
    return;
  }

  // type of range_iter: GEMPadDigiCollection::DigiRangeIterator
  for (const auto& pad_pair : *collection) {
    GEMDetId gemid = pad_pair.first;
    const auto& range = pad_pair.second;

    if (gem->idToDet(gemid) == nullptr) {
      edm::LogError(kLogCategory_) << "Getting DetId failed. Discard this gem pad hit. "
                                   << "Maybe it comes from unmatched geometry." << std::endl;
      continue;
    }

    const GEMEtaPartition* roll = gem->etaPartition(gemid);
    const BoundPlane& surface = roll->surface();

    Int_t region_id = gemid.region();
    Int_t station_id = gemid.station();
    Int_t layer_id = gemid.layer();
    Int_t chamber_id = gemid.chamber();
    Int_t roll_id = gemid.roll();
    Int_t num_layers = gemid.nlayers();

    ME2IdsKey key2(region_id, station_id);
    ME3IdsKey key3(region_id, station_id, layer_id);

    for (auto digi = range.first; digi != range.second; ++digi) {
      // ignore 16-partition GE2/1 pads
      if (gemid.isGE21() and digi->nPartitions() == GEMPadDigi::GE21SplitStrip)
        continue;

      Int_t pad = digi->pad();
      Int_t bx = digi->bx();

      const LocalPoint& local_pos = roll->centreOfPad(pad);
      const GlobalPoint& global_pos = surface.toGlobal(local_pos);

      Float_t g_r = global_pos.perp();
      Float_t g_phi = global_pos.phi();
      Float_t g_x = global_pos.x();
      Float_t g_y = global_pos.y();
      Float_t g_abs_z = std::fabs(global_pos.z());


      Int_t bin_x = getDetOccBinX(num_layers, chamber_id, layer_id);

      if (detail_plot_) {
        me_detail_occ_xy_[key3]->Fill(g_x, g_y);
        me_detail_occ_phi_pad_[key3]->Fill(g_phi, pad);
        me_detail_occ_pad_[key3]->Fill(pad);
        me_detail_bx_[key3]->Fill(bx);
        me_detail_occ_zr_[region_id]->Fill(g_abs_z, g_r);
        me_detail_occ_det_[key2]->Fill(bin_x, roll_id);
      }  // if detail_plot
    }    // digi loop
  }      // range loop
}
