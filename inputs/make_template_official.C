{

    double bins[] = 0, 0.5, 0.505102, 0.510309, 0.515625, 0.521053, 0.526596, 0.532258, 0.538043, 0.543956, 0.55, 0.55618, 0.5625, 0.568966, 0.575581, 0.582353, 0.589286, 0.596386, 0.603659, 0.611111, 0.61875, 0.626582, 0.634615, 0.642857, 0.651316, 0.66, 0.668919, 0.678082, 0.6875, 0.697183, 0.707143, 0.717391, 0.727941, 0.738806, 0.75, 0.761538, 0.773438, 0.785714, 0.798387, 0.811475, 0.825, 0.838983, 0.853448, 0.868421, 0.883929, 0.9, 0.916667, 0.933962, 0.951923, 0.970588, 0.99, 1.0102, 1.03125, 1.05319, 1.07609, 1.1, 1.125, 1.15116, 1.17857, 1.20732, 1.2375, 1.26923, 1.30263, 1.33784, 1.375, 1.41429, 1.45588, 1.5, 1.54688, 1.59677, 1.65, 1.7069, 1.76786, 1.83333, 1.90385, 1.98, 2.0625, 2.15217, 2.25, 2.35714, 2.475, 2.60526, 2.75, 2.91176, 3.09375, 3.3, 3.53571, 3.80769, 4.125, 4.5, 4.95, 5.5, 6.1875, 7.07143, 8.25, 9.9, 12.375, 16.5, 24.75, 49.5, 120};

    TH1D * hrecoe_0 = new TH1D("hrecoe_0", "reconstructed E#nu for sample", 100, bins);



    for (int i = 1; i <= hrecoe_0->GetNbinsX(); i++) { 
        cout << i << " : " << hrecoe_0->GetBinLowEdge(i) << "-" << hrecoe_0->GetBinLowEdge(i+1) << endl; 
    }

    TFile *fout = new TFile("DUNE_numu_templates_v0.root", "recreate");
    hrecoe_0->Write();
    fout->Close();


}
