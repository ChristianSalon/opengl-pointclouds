# opengl-pointclouds
# VGE Projekt – Rekonštrukcia povrchu z mračna bodov

## Riešitelia

- Matej Koscelník (`xkosce01`)
- Christian Saloň (`xsalon02`)


# Projekt zahŕňa:

- interaktívnu aplikáciu na rekonštrukciu 3D povrchu z mračna bodov pomocou klasických algoritmov:
  - Poisson Surface Reconstruction
  - Rolling Ball Surface Reconstruction
  - Gaussian Kernel surface reconstruction
- využitie neurónových sietí na rekonštrukciu povrchu:
  - príprava datasetu
  - trénovanie modelu
  - inferencia

# Použité technológie

## Interaktívna aplikácia

- **C++20**
- **CMake**
- **OpenMP**
- **OpenGL** – vykresľovanie 3D scény
- **GLFW** – správa okna
- **GLM** – matematické operácie
- **Happly** – načítanie `.ply` súborov
- **ImGUI**, **imgui-filebrowser** – používateľské rozhranie
- **CGAL** – algoritmy rekonštrukcie povrchu (inštalácia cez vcpkg)
- **GLSL Compute Shaders** – výpočet hustotného poľa

## Trénovanie a inferencia neurónovej siete

- **PyTorch**
- **NumPy**
- **SciPy**
- **Trimesh** – načítanie mračien bodov a export meshov
- **PyMCubes** – Marching Cubes extrakcia povrchu

# Najdôležitejšie výstupy

## Interaktívna rekonštrukcia

Používateľ môže experimentovať s parametrami rekonštrukcie a sledovať ich vplyv na výsledný mesh.

## GPU vykresľovanie

Implementované vykresľovanie pomocou:

- OpenGL
- Compute Shaderov

Aplikácia obsahuje:

- 3 režimy vykresľovania bodov
- 3 klasické algoritmy rekonštrukcie povrchu
- 1 ML/AI rekonštrukčný algoritmus

## NeuralPull implementácia

Implementovaný model:

- trénovanie na datasete ModelNet40
- inferencia nad vlastným point cloudom
- export rekonštruovaného mesh povrchu

# Ovládanie programu

# Spustenie ML prostredia

## Vytvorenie virtuálneho prostredia

```bash
cd python
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```
## Trénovanie modelu

```bash
python train.py neural_pull single \
    --dataset-path path/to/pointcloud \
    --batch-size 1 \
    --save-path path/to/save/folder
```

## Inferencia

```bash
python inference.py neural_pull \
    --model-path path/to/checkpoint \
    --output path/to/output_mesh.ply
```