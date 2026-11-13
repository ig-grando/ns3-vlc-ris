// uso: python3.11 waf --cwd=scratch --run "padronizado --anguloCarro=60 --anguloRIS=45"

#include <cmath>
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
using namespace std;

int pacotesEnviadosDireto = 0;
int pacotesEnviadosRIS = 0;

struct RegistroPrimeiroPacote {
    bool recebido = false;
    Time instante;
    Vector posicaoTransmissor;
    Vector posicaoReceptor;
};

// Cria um transmissor com a configuracao padrao usada neste experimento.
static void CreateVlcTransmitter (
    VlcDeviceHelper &deviceHelper,
    const string &name,
    double peakPower,
    double azimuth,
    double semiAngle)
{
  deviceHelper.CreateTransmitter (name);
  deviceHelper.SetTXSignal (name, 1000, 0.5, 0, peakPower, 0);
  deviceHelper.SetTrasmitterParameter (name, "Bias", 0);
  deviceHelper.SetTrasmitterParameter (name, "SemiAngle", semiAngle);
  deviceHelper.SetTrasmitterParameter (name, "Azimuth", azimuth);
  deviceHelper.SetTrasmitterParameter (name, "Elevation", 90.0);
  deviceHelper.SetTrasmitterParameter (name, "Gain", 70);
  deviceHelper.SetTrasmitterParameter (name, "DataRateInMBPS", 5);
}

// Cria um receptor com a configuracao padrao usada neste experimento.
static void CreateVlcReceiver (
    VlcDeviceHelper &deviceHelper,
    const string &name,
    double azimuth,
    double fovAngle)
{
  deviceHelper.CreateReceiver (name);
  deviceHelper.SetReceiverParameter (name, "FilterGain", 1);
  deviceHelper.SetReceiverParameter (name, "RefractiveIndex", 1.5);
  deviceHelper.SetReceiverParameter (name, "FOVAngle", fovAngle);
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
    const string &channelName,
    const string &transmitterName,
    const string &receiverName)
{
  channelHelper.CreateChannel (channelName);
  channelHelper.GetChannel (channelName)->SetAttribute (
      "DirectionalCheckEnabled", BooleanValue (true));
  channelHelper.SetPropagationLoss (channelName, "VlcPropagationLoss");
  channelHelper.SetPropagationDelay (channelName, 2);
  channelHelper.AttachTransmitter (channelName, transmitterName, &deviceHelper);
  channelHelper.AttachReceiver (channelName, receiverName, &deviceHelper);
  channelHelper.SetChannelParameter (channelName, "TEMP", 295);
  channelHelper.SetChannelWavelength (channelName, 380, 780);
  channelHelper.SetChannelParameter (channelName, "ElectricNoiseBandWidth", 3e5);
}

// Copia a mobilidade de um no para o dispositivo VLC associado a ele.

// Por mais que o mobility.tcl defina posições a cada segundo temos uma função update que roda com o GetPosition e que atualiza posições intermediárias. 
//Se rodar o printPositions vc confirma isso
static void SyncVlcPosition (Ptr<Node> node, Ptr<VlcNetDevice> device) {
    Ptr<MobilityModel> nodeMobility = node->GetObject<MobilityModel> ();
    device->SetPosition (nodeMobility->GetPosition ());

    Simulator::Schedule (MilliSeconds (10), &SyncVlcPosition, node, device);
}

static void SendVlcPacket (Ptr<VlcTxNetDevice> tx, string nomerRx) {
    if(nomerRx == "TX_DIRECT") {
        pacotesEnviadosDireto++;

    } else if(nomerRx == "TX_TO_RIS") {
        pacotesEnviadosRIS++;

    }

    Ptr<Packet> packet = Create<Packet> (512); // pacote de 512 bytes
    tx->EnqueueDataPacket (packet);

    Simulator::Schedule (MilliSeconds (10), &SendVlcPacket, tx, nomerRx);

}

static void ForwardPacketsFromRis (Ptr<VlcRxNetDevice> risReceiver, Ptr<VlcTxNetDevice> risTransmitter) {
  static uint32_t forwardedBytes = 0; // esse static garante que não vai virar 0 novamente
  const uint32_t receivedBytes = risReceiver->ComputeGoodPut ();

  while (forwardedBytes + 512 <= receivedBytes) // se receber mais um envia
    {
      Ptr<Packet> packet = Create<Packet> (512);

      // Espera de 1 ms apos detectar o pacote pelo goodput.
      Simulator::Schedule (MilliSeconds (1),
                           &VlcTxNetDevice::EnqueueDataPacket,
                           risTransmitter, packet);

      forwardedBytes += packet->GetSize ();
    }


  Simulator::Schedule (MilliSeconds (10), &ForwardPacketsFromRis,
                        risReceiver, risTransmitter);
}

static void PrimeiroPacoteRecebido (const NodeContainer *carros, RegistroPrimeiroPacote *registro, Ptr<const Packet>) {
    if (registro->recebido) {
        return;
    }

    registro->instante = Simulator::Now ();
    registro->posicaoTransmissor = carros->Get (1)->GetObject<MobilityModel> ()->GetPosition ();
    registro->posicaoReceptor = carros->Get (0)->GetObject<MobilityModel> ()->GetPosition ();
    registro->recebido = true;
}

static void ImprimirSaida (const RegistroPrimeiroPacote &primeiroDireto,
                           const RegistroPrimeiroPacote &primeiroRIS,
                           int diretoRecebidos, int risRecebidos) {
    cout << "TEMPO PRIMEIRO PACOTE DIRETO: ";
    if (primeiroDireto.recebido) {
        cout << primeiroDireto.instante.GetSeconds () << "s" << endl;
        const Vector &transmissor = primeiroDireto.posicaoTransmissor;
        const Vector &receptor = primeiroDireto.posicaoReceptor;
        cout << "COORDENADAS PRIMEIRO PACOTE DIRETO: "
             << "carro transmissor (" << transmissor.x << ", " << transmissor.y << ")"
             << ", carro receptor (" << receptor.x << ", " << receptor.y << ")"
             << endl;
    } else {
        cout << "nenhum pacote detectado" << endl;
    }

    cout << "TEMPO PRIMEIRO PACOTE RIS: ";
    if (primeiroRIS.recebido) {
        cout << primeiroRIS.instante.GetSeconds () << "s" << endl;
        const Vector &transmissor = primeiroRIS.posicaoTransmissor;
        const Vector &receptor = primeiroRIS.posicaoReceptor;
        cout << "COORDENADAS PRIMEIRO PACOTE RIS: "
             << "carro transmissor (" << transmissor.x << ", " << transmissor.y << ")"
             << ", carro receptor (" << receptor.x << ", " << receptor.y << ")"
             << endl;
    } else {
        cout << "nenhum pacote detectado" << endl;
    }

    cout << "PACOTES RECEBIDOS DIRETO: " << diretoRecebidos << "/" << pacotesEnviadosDireto << endl;
    cout << "PACOTES RECEBIDOS RIS: " << risRecebidos << "/" << pacotesEnviadosRIS << endl;
}

static void ImprimirSaidaCsv (const RegistroPrimeiroPacote &primeiroDireto,
                              const RegistroPrimeiroPacote &primeiroRIS,
                              int diretoRecebidos, int risRecebidos) {
    // O transmissor percorre o eixo X e o receptor percorre o eixo Y.
    if (primeiroDireto.recebido) {
        cout << primeiroDireto.instante.GetSeconds () << ','
             << std::abs (primeiroDireto.posicaoTransmissor.x) << ','
             << std::abs (primeiroDireto.posicaoReceptor.y);
    } else {
        cout << ",,";
    }
    cout << ',' << diretoRecebidos << ',';

    if (primeiroRIS.recebido) {
        cout << primeiroRIS.instante.GetSeconds () << ','
             << std::abs (primeiroRIS.posicaoTransmissor.x) << ','
             << std::abs (primeiroRIS.posicaoReceptor.y);
    } else {
        cout << ",,";
    }
    cout << ',' << risRecebidos << ',' << pacotesEnviadosDireto << endl;
}

int main(int argc, char *argv[]) {
    const double potencia = 1.0;
    double anguloCarro = 30.0;
    double anguloRIS = 30.0;
    bool saidaCsv = false;

    CommandLine cmd;
    cmd.AddValue ("anguloCarro", "Semiangulo dos transmissores e FOV dos receptores dos carros (graus)",
                  anguloCarro);
    cmd.AddValue ("anguloRIS", "Semiangulo do transmissor e FOV do receptor do RIS (graus)",
                  anguloRIS);
    cmd.AddValue ("saidaCsv", "Imprime uma linha CSV sem cabecalho", saidaCsv);
    cmd.Parse (argc, argv);


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
    ris.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0,  0, 0));

    VlcDeviceHelper devHelperVPPM; //VLC Device Helper to manage the VLC Device and device properties.

    // Enlace direto: carro transmissor -> carro receptor.
    CreateVlcTransmitter (devHelperVPPM, "TX_DIRECT", potencia, 0, anguloCarro);
    CreateVlcReceiver (devHelperVPPM, "RX_DIRECT", 90, anguloCarro);

    // Primeiro salto do RIS: carro transmissor -> receptor do RIS.
    CreateVlcTransmitter (devHelperVPPM, "TX_TO_RIS", potencia, 0, anguloCarro);
    CreateVlcReceiver (devHelperVPPM, "RIS_RX", 180, anguloRIS);

    // Segundo salto do RIS: transmissor do RIS -> carro receptor.
    CreateVlcTransmitter (devHelperVPPM, "RIS_TX", potencia, 270, anguloRIS);
    CreateVlcReceiver (devHelperVPPM, "RX_FROM_RIS", 90.0, anguloCarro);

    devHelperVPPM.SetTrasmitterPosition ("RIS_TX", 0,  0, 0);
    devHelperVPPM.SetReceiverPosition ("RIS_RX", 0,  0, 0);

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
    Simulator::Schedule (Seconds (1.0), &SendVlcPacket, devHelperVPPM.GetTransmitter ("TX_TO_RIS"), "TX_TO_RIS");
    Simulator::Schedule (Seconds (1.0), &SendVlcPacket, devHelperVPPM.GetTransmitter ("TX_DIRECT"), "TX_DIRECT");

    Simulator::Schedule (Seconds (1.0), &ForwardPacketsFromRis, devHelperVPPM.GetReceiver ("RIS_RX"), devHelperVPPM.GetTransmitter ("RIS_TX"));

    RegistroPrimeiroPacote primeiroDireto;
    RegistroPrimeiroPacote primeiroRIS;
    Ptr<VlcRxNetDevice> rxDireto = devHelperVPPM.GetReceiver ("RX_DIRECT");
    Ptr<VlcRxNetDevice> rxViaRIS = devHelperVPPM.GetReceiver ("RX_FROM_RIS");
    const bool diretoConectado = rxDireto->TraceConnectWithoutContext (
        "RxGoodPacket", MakeBoundCallback (&PrimeiroPacoteRecebido, &carros, &primeiroDireto));
    const bool risConectado = rxViaRIS->TraceConnectWithoutContext (
        "RxGoodPacket", MakeBoundCallback (&PrimeiroPacoteRecebido, &carros, &primeiroRIS));
    NS_ABORT_MSG_UNLESS (diretoConectado && risConectado,
                         "Falha ao conectar o registro de recepcao VLC");

    Simulator::Stop(Seconds (35.0)); // para garantir que vai parar

    Simulator::Run();

    const int diretoRecebidos = rxDireto->ComputeGoodPut () / 512;
    const int risRecebidos = rxViaRIS->ComputeGoodPut () / 512;

    if (saidaCsv) {
        ImprimirSaidaCsv (primeiroDireto, primeiroRIS, diretoRecebidos, risRecebidos);
    } else {
        ImprimirSaida (primeiroDireto, primeiroRIS, diretoRecebidos, risRecebidos);
    }

    Simulator::Destroy();
    return 0;
}






// funções não mais usadas, jogue para antes da main pra usar
static void PrintPositions (NodeContainer carros) {
  cout << fixed << setprecision (2)
            << "tempo=" << Simulator::Now ().GetSeconds () << " s" << endl;

  for (NodeContainer::Iterator it = carros.Begin (); it != carros.End (); ++it)
    {
      Ptr<MobilityModel> mobility = (*it)->GetObject<MobilityModel> ();
      Vector position = mobility->GetPosition ();

      cout << "  no=" << (*it)->GetId ()
                << "  x=" << position.x
                << "  y=" << position.y
                << endl;
    }
      Simulator::Schedule (MilliSeconds (10.0), &PrintPositions, carros);
}
