## Netedit:
Interface usada para criar o cenário e a simulação.
Ao colocar as ruas e os carros são gerados dois arquivos:
- `.net.xml` para a parte das ruas e cruzamentos (edges e junctions)
- `.rou.xml` para a parte de carros (demand)

Depois para reabrir eles juntos basta rodar 
`netedit -s estrada.net.xml -r carros_demand.rou.xml`

## Sumo
Roda a simulação de fato, executando um arquivo `.sumocfg` que diz ao simulador quais arquivos ele deve usar para executar o teste (geralmente só os dois acima).  
Para gerar a posição dos veículos ao longo do tempo rodar:
`sumo -c sumo_teste.sumocfg --fcd-output sumoTrace.xml`  
Para gerar o arquivo que o ns3 consegue ler com os carros sendo representados por nodos usar: `/usr/bin/python3 $SUMO_HOME/tools/traceExporter.py     --fcd-input sumoTrace.xml     --ns2mobility-output mobility.tcl`

## Simulação Final
- O nodo 1 é o carro da esquerda e o carro 0 é o que vem de baixo
- O RIS é criado na posição fixa (0, 0, 0) 
- Há três canais: 
1. Direto: CHANNEL_DIRECT = TX_DIRECT -> RX_DIRECT
2. Carro esquerda até o RIS: TX_TO_RIS -> RIS_RX
3. RIS até o carro de baixo: RIS_TX -> RX_FROM_RIS

As funções agendadas são:
1. Carro envia pacotes para RIS
2. Para cada aumento do goodput do receptor do RIS envia um pacote do seu transmissor.
3. Imprime-se ao receber em ambos os receptores.


- Atualemente o módulo de vlc faz o cálculo considerando potência e ganho de transmissão e recepção, mas desconsidera LOS, é necessário adpatar para considerar o ângulo de transmissão e recepção.
- `vlc-channel-model.cc ` foi alterado para considerar os ângulos

## Informações
- A velocidade da via é de 11.11 m/s, que é equivalente a 40 km/h
- Os carros partem na velocidade máxima da via
- O nodo 1 é o carro da esquerda e o carro 0 é o que vem de baixo
- Carro 1: (-96.9, 0, 0)
- Carro 0: (0, -96.9, 0)

- Um pacote é enviado a cada 10ms

https://docs.google.com/spreadsheets/d/1mBb2M0FSP_i4I9zUq-5B-Xlbg24znxVZBqEvP3StujY/edit?usp=sharing


## Rodar testes
- `python3.11 scratch/rodar_simulacoes.py`

## TODO
- Obter distância carros quando o primeiro pacote chega
