#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ns2-mobility-helper.h"

#include <iomanip>
#include <iostream>

using namespace ns3;

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

  if (Simulator::Now () < Seconds (33.0))
    {
      Simulator::Schedule (Seconds (1.0), &PrintPositions, carros);
    }
}

int main() {
    NodeContainer carros;

    carros.Create(2);

    Ns2MobilityHelper mobility("mobility.tcl");

    mobility.Install(
        carros.Begin(),
        carros.End()
    );

    Simulator::Schedule (Seconds (0.0), &PrintPositions, carros);

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
