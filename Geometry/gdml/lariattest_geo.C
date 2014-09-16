typedef struct _drawopt {
  const char* volume;
  int         color;
} drawopt;

void lariattest_geo(TString volName="volCryostat"){

gSystem->Load("libGeom");
gSystem->Load("libGdml");

TGeoManager::Import("lariattest_flat_pmt_w.gdml");

drawopt optlariattest[] = {
   {"volWorld",                 0},
   {"volDetEnclosure",          kWhite},
   {"volDewar_inLAr",              kOrange},
//   {"volTPCWirePlaneLengthSide", kCyan+3},
//   {"volTPCWirePlaneWidthSide", kRed},
  {"voltpb1", kGreen},
  {"volBeamBoxpp", kBlue},


  
//{"volTPCShieldPlane", kBlue},
//{"volArgon_solid_L", kRed},
//{"volArgon_cap_L", kOrange},
//{"volArgon_cap_front", kOrange},
//{"volTPC", kOrange},
//{"volDetEnclosure", kBlue},
//{"volMND", kBlue},
//{"volTPCActive",kGreen},
  {0, 0}
};

for (int i=0;; ++i) {
  if (optlariattest[i].volume==0) break;
    gGeoManager->FindVolumeFast(optlariattest[i].volume)->SetLineColor(optlariattest[i].color);
}

TList* mat = gGeoManager->GetListOfMaterials();
TIter next(mat);
TObject *obj;
// while (obj = next()) {
//  obj->Print();
// }

 gGeoManager->CheckOverlaps(0.01);
 gGeoManager->PrintOverlaps();
 gGeoManager->SetMaxVisNodes(70000);

 //gGeoManager->GetTopVolume()->Draw();
 //gGeoManager->FindVolumeFast(volName)->Draw();

 TFile *tf = new TFile("csu40.root", "RECREATE");
 gGeoManager->Write();
 tf->Close();
}
