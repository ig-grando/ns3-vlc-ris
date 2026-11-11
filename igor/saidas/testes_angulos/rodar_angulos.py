import subprocess
from itertools import product

# potencias = [0.1, 0.25, 0.5, 0.75, 1.0]
semi_angulos_direto = [90, 75, 60, 45, 30]
fovs_direto = [90, 75, 60, 45, 30]
semi_angulos_ris = [15, 30, 45, 60, 90]
fovs_ris = [15, 30, 45, 60, 90]

for i in range(5):
    for j in range(5):
        comando = (
            f"carros-vlc --potencia={1} "
            f"--semiAnguloDireto={semi_angulos_direto[i]} "
            f"--fovDireto={fovs_direto[j]} "
            f"--semiAnguloRIS={semi_angulos_ris[i]} "
            f"--fovRIS={fovs_ris[j]}"
        )
        print("\n" + "=" * 100)
        print(f"EXECUCAO {i * 5 + j + 1}/25")
        print(comando)
        print("-" * 100, flush=True)

        resultado = subprocess.run(
            ["python3.11", "waf", "--cwd=scratch", "--run", comando],
            stdout=subprocess.PIPE,
            text=True,
            check=True,
        )
        for linha in resultado.stdout.splitlines():
            if linha.startswith(("TEMPO PRIMEIRO", "PACOTES RECEBIDOS")):
                print(linha)
