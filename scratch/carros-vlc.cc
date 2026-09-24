#include <iomanip>
#include <iostream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ns2-mobility-helper.h"
//#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/vlc-channel-helper.h"
#include "ns3/vlc-device-helper.h"
#include "ns3/netanim-module.h"


using namespace ns3;

// Cria um transmissor com a configuracao padrao usada neste experimento.
static void CreateVlcTransmitter (
    VlcDeviceHelper &deviceHelper,
    const std::string &name,
    double peakPower = 0.05,
    double azimuth=0)
{
  deviceHelper.CreateTransmitter (name);
  deviceHelper.SetTXSignal (name, 1000, 0.5, 0, peakPower, 0);
  deviceHelper.SetTrasmitterParameter (name, "Bias", 0);
  deviceHelper.SetTrasmitterParameter (name, "SemiAngle", 35);
  deviceHelper.SetTrasmitterParameter (name, "Azimuth", azimuth);
  deviceHelper.SetTrasmitterParameter (name, "Elevation", 180.0);
  deviceHelper.SetTrasmitterParameter (name, "Gain", 70);
  deviceHelper.SetTrasmitterParameter (name, "DataRateInMBPS", 5);
}

// Cria um receptor com a configuracao padrao usada neste experimento.
static void CreateVlcReceiver (
    VlcDeviceHelper &deviceHelper,
    const std::string &name,
    double azimuth = 90.0)
{
  deviceHelper.CreateReceiver (name);
  deviceHelper.SetReceiverParameter (name, "FilterGain", 1);
  deviceHelper.SetReceiverParameter (name, "RefractiveIndex", 1.5);
  deviceHelper.SetReceiverParameter (name, "FOVAngle", 40.5);
  deviceHelper.SetReceiverParameter (name, "ConcentrationGain", 0);
  deviceHelper.SetReceiverParameter (name, "PhotoDetectorArea", 1.3e-5);
  deviceHelper.SetReceiverParameter (name, "RXGain", 0);
  deviceHelper.SetReceiverParameter (name, "Beta", 1);
  deviceHelper.SetReceiverParameter (name, "SetModulationScheme",
                                     VlcErrorModel::PSK4);
  deviceHelper.GetReceiver (name)->SetAzmuth (azimuth);
}

// Cria um enlace VLC entre um transmissor e um receptor ja criados.
static void CreateVlcChannel (
    VlcChannelHelper &channelHelper,
    VlcDeviceHelper &deviceHelper,
    const std::string &channelName,
    const std::string &transmitterName,
    const std::string &receiverName)
{
  channelHelper.CreateChannel (channelName);
  channelHelper.SetPropagationLoss (channelName, "VlcPropagationLoss");
  channelHelper.SetPropagationDelay (channelName, 2);
  channelHelper.AttachTransmitter (channelName, transmitterName, &deviceHelper);
  channelHelper.AttachReceiver (channelName, receiverName, &deviceHelper);
  channelHelper.SetChannelParameter (channelName, "TEMP", 295);
  channelHelper.SetChannelWavelength (channelName, 380, 780);
  channelHelper.SetChannelParameter (channelName, "ElectricNoiseBandWidth", 3e5);
}

static void PrintPositions (NodeContainer carros) {
  std::cout << std::fixed << std::setprecision (2)
            << "tempo=" << Simulator::Now ().GetSeconds () << " s" << std::endl;

  for (NodeContainer::Iterator it = carros.Begin (); it != carros.End (); ++it)
    {
      Ptr<MobilityModel> mobility = (*it)->GetObject<MobilityModel> ();
      Vector position = mobility->GetPosition ();

      std::cout << "  no=" << (*it)->GetId ()
                << "  x=" << position.x
                << "  y=" << position.y
                << std::endl;
    }
      Simulator::Schedule (Seconds (1.0), &PrintPositions, carros);
}

// Copia a mobilidade de um no para o dispositivo VLC associado a ele.
static void SyncVlcPosition (Ptr<Node> node, Ptr<VlcNetDevice> device) {
    Ptr<MobilityModel> nodeMobility = node->GetObject<MobilityModel> ();
    device->SetPosition (nodeMobility->GetPosition ());

    Simulator::Schedule (MilliSeconds (100), &SyncVlcPosition, node, device);
}

static void SendVlcPacket (Ptr<VlcTxNetDevice> tx) {
    Ptr<Packet> packet = Create<Packet> (512); // pacote de 512 bytes
    tx->EnqueueDataPacket (packet);

    Simulator::Schedule (Seconds (1.0), &SendVlcPacket, tx);

}

static void ForwardPacketsFromRis (Ptr<VlcRxNetDevice> risReceiver, Ptr<VlcTxNetDevice> risTransmitter) {
  static uint32_t forwardedBytes = 0; // esse static garante que não vai virar 0 novamente
  const uint32_t receivedBytes = risReceiver->ComputeGoodPut ();

  while (forwardedBytes + 512 <= receivedBytes) // se receber mais um envia
    {
      Ptr<Packet> packet = Create<Packet> (512);

      // 100 ms aproximam o atraso de chegada usado pelo vlcnew;
      // 1 ms representa o processamento do repetidor.
      Simulator::Schedule (MilliSeconds (101),
                           &VlcTxNetDevice::EnqueueDataPacket,
                           risTransmitter, packet);

      forwardedBytes += packet->GetSize ();

      std::cout << "t=" << Simulator::Now ().GetSeconds ()
                << " RIS encaminhou 512 bytes" << std::endl;
    }


  Simulator::Schedule (MilliSeconds (10), &ForwardPacketsFromRis,
                        risReceiver, risTransmitter);
}

static void PrintVlcMetrics (Ptr<VlcRxNetDevice> rx, std::string receiverName) {
    std::cout << "t=" << Simulator::Now ().GetSeconds ()
            << " goodput acumulado em " << receiverName << " = "
            << rx->ComputeGoodPut () << " bytes"
            << std::endl;

    Simulator::Schedule (Seconds (1.0), &PrintVlcMetrics, rx, receiverName);
}


int main() {
    NodeContainer carros;

    carros.Create(2);

    Ns2MobilityHelper mobility("mobility.tcl");

    mobility.Install(
        carros.Begin(),
        carros.End()
    );

    // O RIS e fixo, mas precisa ser um no para receber seus dois dispositivos.
    NodeContainer ris;
    ris.Create (1);

    MobilityHelper risMobility;
    risMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    risMobility.Install (ris);
    ris.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (201.6, -1.6, 0));

    VlcDeviceHelper devHelperVPPM; //VLC Device Helper to manage the VLC Device and device properties.

    // Enlace direto: carro transmissor -> carro receptor.
    CreateVlcTransmitter (devHelperVPPM, "TX_DIRECT");
    CreateVlcReceiver (devHelperVPPM, "RX_DIRECT");

    // Primeiro salto do RIS: carro transmissor -> receptor do RIS.
    CreateVlcTransmitter (devHelperVPPM, "TX_TO_RIS");
    CreateVlcReceiver (devHelperVPPM, "RIS_RX", 180);

    // Segundo salto do RIS: transmissor do RIS -> carro receptor.
    CreateVlcTransmitter (devHelperVPPM, "RIS_TX", 0.05, 270);
    CreateVlcReceiver (devHelperVPPM, "RX_FROM_RIS");

    devHelperVPPM.SetTrasmitterPosition ("RIS_TX", 201.6, -1.6, 0);
    devHelperVPPM.SetReceiverPosition ("RIS_RX", 201.6, -1.6, 0);

    VlcChannelHelper chHelper;
    CreateVlcChannel (chHelper, devHelperVPPM, "CHANNEL_DIRECT", "TX_DIRECT", "RX_DIRECT");
    CreateVlcChannel (chHelper, devHelperVPPM, "CHANNEL_TO_RIS", "TX_TO_RIS", "RIS_RX");
    CreateVlcChannel (chHelper, devHelperVPPM, "CHANNEL_FROM_RIS", "RIS_TX", "RX_FROM_RIS");

    // Cada Install associa exatamente um TX e um RX ao seu proprio canal.
    NetDeviceContainer directDevices = chHelper.Install (
        carros.Get (1), carros.Get (0), &devHelperVPPM, &chHelper,
        "TX_DIRECT", "RX_DIRECT", "CHANNEL_DIRECT");
    NetDeviceContainer toRisDevices = chHelper.Install (
        carros.Get (1), ris.Get (0), &devHelperVPPM, &chHelper,
        "TX_TO_RIS", "RIS_RX", "CHANNEL_TO_RIS");
    NetDeviceContainer fromRisDevices = chHelper.Install (
        ris.Get (0), carros.Get (0), &devHelperVPPM, &chHelper,
        "RIS_TX", "RX_FROM_RIS", "CHANNEL_FROM_RIS");

    // Dispositivos montados nos carros acompanham a mobilidade do NS-2.
    Simulator::Schedule (Seconds (0.0), &SyncVlcPosition, carros.Get (1), devHelperVPPM.GetTransmitter ("TX_DIRECT"));
    Simulator::Schedule (Seconds (0.0), &SyncVlcPosition, carros.Get (0), devHelperVPPM.GetReceiver ("RX_DIRECT"));
    Simulator::Schedule (Seconds (0.0), &SyncVlcPosition, carros.Get (1), devHelperVPPM.GetTransmitter ("TX_TO_RIS"));
    Simulator::Schedule (Seconds (0.0), &SyncVlcPosition, carros.Get (0), devHelperVPPM.GetReceiver ("RX_FROM_RIS"));

    // envio e recebimento
    Simulator::Schedule (Seconds (1.0), &SendVlcPacket, devHelperVPPM.GetTransmitter ("TX_TO_RIS"));

    Simulator::Schedule (Seconds (1.0), &ForwardPacketsFromRis, devHelperVPPM.GetReceiver ("RIS_RX"), devHelperVPPM.GetTransmitter ("RIS_TX"));

    Simulator::Schedule (Seconds (1.0), &PrintVlcMetrics, devHelperVPPM.GetReceiver ("RIS_RX"), "RIS_RX");

    Simulator::Schedule (Seconds (1.0), &PrintVlcMetrics, devHelperVPPM.GetReceiver ("RX_FROM_RIS"), "RX_FROM_RIS");

    Simulator::Stop(Seconds (35.0)); // para garantir que vai parar

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
