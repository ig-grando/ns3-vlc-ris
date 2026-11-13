import argparse
import csv
from pathlib import Path


DIRETORIO = Path(__file__).resolve().parent
CAMPOS_PRIMEIRO_PACOTE = ("tempo_s", "distancia_tx_m", "distancia_rx_m")


def ler_resultados(arquivo):
    resultados = {}
    with arquivo.open(encoding="utf-8-sig", newline="") as entrada:
        leitor = csv.DictReader(entrada)
        colunas = {"angulo_carro_graus", "angulo_ris_graus"}
        for enlace in ("direto", "ris"):
            colunas.update(f"{enlace}_{campo}" for campo in CAMPOS_PRIMEIRO_PACOTE)
        faltantes = colunas.difference(leitor.fieldnames or [])
        if faltantes:
            raise ValueError(f"Colunas ausentes no CSV: {', '.join(sorted(faltantes))}")

        for linha in leitor:
            carro = int(linha["angulo_carro_graus"])
            ris = int(linha["angulo_ris_graus"])
            if (carro, ris) in resultados:
                raise ValueError(f"Combinação repetida: carro {carro}°, RIS {ris}°.")
            resultados[carro, ris] = linha

    if not resultados:
        raise ValueError("O CSV não contém resultados.")
    return resultados


def tabela_primeiro_pacote(resultados):
    linhas = []
    for (carro, ris), resultado in sorted(resultados.items()):
        celulas = [f'<th scope="row">{carro}°</th>', f"<td>{ris}°</td>"]
        for enlace in ("direto", "ris"):
            for campo in CAMPOS_PRIMEIRO_PACOTE:
                valor = resultado[f"{enlace}_{campo}"]
                texto = str(float(valor)).replace(".", ",") if valor else "—"
                celulas.append(f"<td>{texto}</td>")
        linhas.append(f'<tr>{"".join(celulas)}</tr>')

    colunas = """
        <th scope="col">Instante (s)</th>
        <th scope="col">TX à origem (m)</th>
        <th scope="col">RX à origem (m)</th>
    """
    return f"""
        <section class="painel">
          <h1>Primeiro pacote recebido</h1>
          <p>O instante é contado desde o início da simulação. TX e RX são os carros
             transmissor e receptor. Um travessão indica ausência de recepção.</p>
          <div class="rolagem">
            <table class="dados">
              <thead>
                <tr>
                  <th scope="col" rowspan="2">Ângulo do carro</th>
                  <th scope="col" rowspan="2">Ângulo do RIS</th>
                  <th scope="colgroup" colspan="3">Direto</th>
                  <th scope="colgroup" colspan="3">Via RIS</th>
                </tr>
                <tr>{colunas}{colunas}</tr>
              </thead>
              <tbody>{"".join(linhas)}</tbody>
            </table>
          </div>
        </section>
    """


def gerar_relatorio(resultados):
    tabela = tabela_primeiro_pacote(resultados)

    return f"""<!doctype html>
<html lang="pt-BR">
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Primeiro pacote recebido</title>
<style>
  * {{ box-sizing: border-box; }}
  body {{ margin: 0; background: #f2f5f8; color: #172b3a; font: 16px/1.5 Inter, system-ui, sans-serif; }}
  main {{ max-width: 1180px; margin: auto; padding: 32px 20px; }}
  h1 {{ font-size: 1.5rem; line-height: 1.2; margin: 0 0 12px; }}
  p {{ margin: 0 0 16px; }}
  .painel {{ margin-bottom: 16px; padding: 20px; background: white; border: 1px solid #dce4eb; border-radius: 12px; }}
  .rolagem {{ overflow-x: auto; }}
  table {{ border-collapse: collapse; width: 100%; font-variant-numeric: tabular-nums; }}
  th, td {{ padding: 8px; text-align: center; white-space: nowrap; }}
  thead, tbody th {{ background: #eaf0f5; }}
  .dados td {{ border-bottom: 1px solid #e0e8ef; }}
  .dados tbody tr:nth-child(even) {{ background: #f6f8fa; }}
</style>
<main>
  {tabela}
</main>
</html>
"""


def main():
    parser = argparse.ArgumentParser(description="Gera uma tabela HTML do primeiro pacote recebido.")
    parser.add_argument("--entrada", type=Path, default=DIRETORIO / "resultados_padronizado.csv")
    parser.add_argument("--saida", type=Path, default=DIRETORIO / "graficos_padronizado.html")
    args = parser.parse_args()

    try:
        resultados = ler_resultados(args.entrada)
        args.saida.write_text(gerar_relatorio(resultados), encoding="utf-8")
    except (OSError, ValueError) as erro:
        parser.exit(1, f"Erro: {erro}\n")
    print(f"Relatório salvo em {args.saida}")


if __name__ == "__main__":
    main()
