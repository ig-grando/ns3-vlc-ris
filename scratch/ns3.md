# Aprendizado
## NS3
- Um certo programa pode gerar arquivos cwnd ou pcap. O primeiro é de mudança do tamanho de janela para janelas deslizantes e o segundo, que é mais útil, serve para guardar alguns pacotes específicos e posteriormente os analizar com uma aplicação tipo o Wireshark
- Para rodar o ns3 com o vlc precisei usar o python3.11.
- Depois de colocar o arquivo desejado em /scratch rodar com: `python3.11 waf --run scratch/carros`
- Para conseguir importar o mobility.tsl gerado do Sumo precisei rodar com: `python3.11 waf --cwd=scratch --run carros`

Um programa base do ns3 tem a seguinte estrutura:
```
#include "ns3/core-module.h"
#include "ns3/network-module.h"

using namespace ns3;

int
main (int argc, char *argv[])
{
  // 1. Criar nós
  NodeContainer nodes;
  nodes.Create (2);

  // 2. Configurar rede, mobilidade, aplicações etc.
  // Ex.: instalar dispositivos, definir IP, agendar eventos.
  
  Simulator::Schedule(Seconds(5.0), &MinhaFuncao);

  // 3. Definir quando a simulação termina
  Simulator::Stop (Seconds (10.0));

  // 4. Executar os eventos agendados
  Simulator::Run ();

  // 5. Liberar objetos e encerrar
  Simulator::Destroy ();

  return 0;
}
```
Já em relação à configuração dos nodos podemos ter: configuração de mobilidade, dispositivo (WIFI, Ethernet), canal (cabo, vlc), IP, MAC. TUdo isso no fim precisa da pilha da conexão que vai linkar tudo:
```
Aplicação
↓
TCP ou UDP
↓
IP (IPv4/IPv6)
↓
NetDevice
↓
Canal
```
### Exemplos
- No seventh.cc ele tem vários exemplos tratando de gravação de arquivos e de criação de gráfico

## VLCnew
Módulo adicional que possui dispositivos, canal e modelo físico, não substituindo IP, TCP e outros protocolos de mais alto nível.

## Netedit:
Interface usada para criar o cenário e a simulação.
Cria dois arquivos:
- `.net.xml` para a parte das ruas e cruzamentos (edges e junctions)
- `.rou.xml` para a parte de carros (demand)

## Sumo
Roda a simulação de fato, executando um arquivo `.sumocfg` que diz ao simulador quais arquivos ele deve usar para executar o teste (geralmente só os dois acima).  
Para gerar a posição dos veículos ao longo do tempo rodar:
`sumo -c sumo_teste.sumocfg --fcd-output sumoTrace.xml`  
Para gerar o arquivo que o ns3 consegue ler com os carros sendo representados por nodos usar: `/usr/bin/python3 $SUMO_HOME/tools/traceExporter.py     --fcd-input sumoTrace.xml     --ns2mobility-output mobility.tcl
`

## Simulaçao completa
A simulação completa utiliza o arquivo de mobilidade gerado, depois o instala nos carros. O RIS é criado na posição fixa (201.6, -1.6) e tem três canais com dois dispositivos únicos cada. 

- Direto: CHANNEL_DIRECT = TX_DIRECT -> RX_DIRECT
- Com RIS: TX_TO_RIS -> RIS_RX -> RIS_TX -> RX_FROM_RIS

As funções agendadas são:
1. Carro envia pacotes para RIS
2. Para cada aumento do goodput do receptor do RIS envia um pacote do seu transmissor.
3. Imprime-se ao receber em ambos os receptores.
