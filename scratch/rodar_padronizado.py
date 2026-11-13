import csv
import subprocess
from itertools import product
from pathlib import Path


DIRETORIO_SCRATCH = Path(__file__).resolve().parent
DIRETORIO_NS3 = DIRETORIO_SCRATCH.parent
ANGULOS_CARRO = [90, 75, 60, 45, 30]
ANGULOS_RIS = [15, 30, 45, 60, 90]
CAMPOS_ENLACE = (
    "tempo_s", "distancia_tx_m", "distancia_rx_m", "recebidos"
)


def rodar_simulacoes(diretorio_simulacao, arquivo_csv):
    cabecalho = ["angulo_carro_graus", "angulo_ris_graus"]
    for enlace in ("direto", "ris"):
        cabecalho.extend(f"{enlace}_{campo}" for campo in CAMPOS_ENLACE)
    cabecalho.append("pacotes_enviados")

    linhas = []
    total = len(ANGULOS_CARRO) * len(ANGULOS_RIS)
    combinacoes = product(ANGULOS_CARRO, ANGULOS_RIS)
    for numero, (angulo_carro, angulo_ris) in enumerate(combinacoes, start=1):
        comando = (
            f"padronizado --anguloCarro={angulo_carro} "
            f"--anguloRIS={angulo_ris} --saidaCsv=1"
        )
        print(f"Execução {numero}/{total} | carro={angulo_carro} RIS={angulo_ris}", flush=True)

        resultado = subprocess.run(
            ["python3.11", "waf", f"--cwd={diretorio_simulacao}", "--run", comando],
            cwd=DIRETORIO_NS3,
            stdout=subprocess.PIPE,
            text=True,
            check=True,
        )
        # O Waf também imprime mensagens de compilação no stdout.
        linhas_csv = [
            campos for campos in csv.reader(resultado.stdout.splitlines())
            if len(campos) == len(cabecalho) - 2
        ]
        if len(linhas_csv) != 1:
            raise ValueError(
                f"Esperava uma linha CSV em '{comando}', recebi {len(linhas_csv)}.\n"
                f"{resultado.stdout}"
            )
        linhas.append([angulo_carro, angulo_ris, *linhas_csv[0]])

    with arquivo_csv.open("w", newline="", encoding="utf-8") as saida:
        escritor = csv.writer(saida)
        escritor.writerow(cabecalho)
        escritor.writerows(linhas)


if __name__ == "__main__":
    arquivo_csv = DIRETORIO_SCRATCH / "resultados_padronizado.csv"
    rodar_simulacoes(DIRETORIO_SCRATCH, arquivo_csv)
    print(f"CSV salvo em {arquivo_csv}")
