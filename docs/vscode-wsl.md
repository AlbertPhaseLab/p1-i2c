# Trabajar con VS Code sobre WSL2

**Objetivo:** editar, compilar y usar Git del proyecto `~/p1-i2c` desde VS Code, con
los ficheros y las herramientas de Ubuntu, sin copiar nada entre Windows y Linux.

**Idea:** VS Code se instala en **Windows**, pero con la extensión **WSL** ejecuta la
terminal, el compilador y el acceso a ficheros **dentro de Ubuntu**.

---

## Lo mínimo para trabajar

1. Instalar **VS Code en Windows** (code.visualstudio.com), no dentro de Ubuntu.
   Dejar marcada la opción "Añadir al PATH".
2. Instalar la extensión **WSL** (de Microsoft).
3. En la terminal de Ubuntu:

```bash
cd ~/p1-i2c
code .
```

4. Abrir la terminal integrada con `` Ctrl+` `` y trabajar como siempre
   (`./build.sh`, `./run.sh`). Para salir de QEMU sigue valiendo `Ctrl-A` y luego `X`.

Abajo a la izquierda debe aparecer **WSL: Ubuntu-24.04**: es la señal de que VS Code
está conectado a Ubuntu.

## El servidor de VS Code

La primera vez que se ejecuta `code .`, VS Code descarga solo un pequeño servidor
dentro de Ubuntu (`~/.vscode-server`). No se instala a mano. Hace de puente: la
ventana está en Windows y el servidor ejecuta terminal, compilador y ficheros en
Linux. Sin él no habría terminal de Ubuntu integrada. Solo necesita internet la
primera vez.

**Abrir siempre la carpeta con `code .` desde Ubuntu.** Abrirla desde
`\\wsl$\Ubuntu-24.04\...` en un VS Code normal de Windows funciona, pero es más
lento, la terminal sería de Windows y el autocompletado del código del kernel
funciona peor.

---

## Opcional 1: extensión C/C++

Instalar **C/C++** (Microsoft). Al estar conectado a WSL, VS Code ofrece
**"Install in WSL"**: aceptar.

## Opcional 2: que entienda el código del kernel

Sin configurar, aparecen errores rojos en `#include <linux/module.h>`. Crear
`.vscode/c_cpp_properties.json`:

```bash
mkdir -p ~/p1-i2c/.vscode && cd ~/p1-i2c/.vscode
cat > c_cpp_properties.json <<'EOF'
{
  "configurations": [{
    "name": "kernel-arm64",
    "compilerPath": "/usr/bin/aarch64-linux-gnu-gcc",
    "intelliSenseMode": "linux-gcc-arm64",
    "cStandard": "gnu11",
    "includePath": [
      "${env:HOME}/linux/include",
      "${env:HOME}/linux/include/uapi",
      "${env:HOME}/linux/include/generated",
      "${env:HOME}/linux/include/generated/uapi",
      "${env:HOME}/linux/arch/arm64/include",
      "${env:HOME}/linux/arch/arm64/include/uapi",
      "${env:HOME}/linux/arch/arm64/include/generated",
      "${env:HOME}/linux/arch/arm64/include/generated/uapi"
    ],
    "defines": ["__KERNEL__", "MODULE", "KBUILD_MODNAME=\"demo\""],
    "forcedInclude": [
      "${env:HOME}/linux/include/linux/kconfig.h",
      "${env:HOME}/linux/include/linux/compiler_types.h"
    ]
  }],
  "version": 4
}
EOF
```

Con eso funcionan el autocompletado, **Ir a definición** (`F12`) y los avisos. Prueba:
`F12` sobre `miscdevice` en `hello_misc.c` debe llevar a la cabecera del kernel.
Puede quedar algún aviso residual: IntelliSense no replica exactamente los flags de
compilación del kernel.

## Opcional 3: compilar con un atajo

```bash
cat > ~/p1-i2c/.vscode/tasks.json <<'EOF'
{
  "version": "2.0.0",
  "tasks": [{
    "label": "build",
    "type": "shell",
    "command": "${workspaceFolder}/build.sh",
    "group": { "kind": "build", "isDefault": true },
    "problemMatcher": ["$gcc"]
  }]
}
EOF
```

`Ctrl+Shift+B` ejecuta `build.sh`. Los errores aparecen en el panel **Problemas** y
un clic lleva a la línea. QEMU se sigue lanzando a mano en la terminal integrada,
porque es interactivo.

## Opcional 4: Git desde VS Code

El icono de **Control de código fuente** (`Ctrl+Shift+G`) muestra los cambios, las
diferencias por línea y permite hacer commit con un mensaje. Al subir a GitHub,
VS Code suele ofrecer iniciar sesión con la cuenta desde el propio panel, lo que
puede evitar el *personal access token*. Si no, el método del token sigue valiendo.

Los ficheros de `.vscode/` pueden entrar en el repositorio: son configuración del
proyecto.

---

## Guardar las guías en el repo (`docs/`)

La carpeta `docs/` no existe hasta que se crea. En Ubuntu:

```bash
mkdir -p ~/p1-i2c/docs
```

Ruta en Ubuntu: `/home/albert/p1-i2c/docs`.
Ruta en Windows: `\\wsl$\Ubuntu-24.04\home\albert\p1-i2c\docs`.

Para copiar los `.md` descargados: arrastrarlos al explorador de VS Code (dentro de
`docs/`) o soltarlos en la ruta de Windows. Después:

```bash
cd ~/p1-i2c
git add docs
git commit -m "Docs: guias de la semana 1"
```

Los `.md` se ven formateados con `Ctrl+Shift+V`.

## Problemas habituales

| Síntoma | Causa | Solución |
|---|---|---|
| `code: command not found` en Ubuntu | VS Code de Windows sin "Añadir al PATH" | Reinstalarlo marcando esa opción, o abrir VS Code y conectar con "WSL: Connect to WSL" (`F1`) |
| No aparece "WSL: Ubuntu-24.04" abajo a la izquierda | Carpeta abierta sin la extensión WSL | Instalar la extensión y abrir con `code .` desde Ubuntu |
| Errores rojos en `#include <linux/...>` | Falta `c_cpp_properties.json` o el kernel no está compilado | Crear el fichero (Opcional 2); el kernel debe haberse compilado una vez |
| `F12` no salta a la cabecera | IntelliSense no ha reindexado | `F1` → "C/C++: Reset IntelliSense Database" |
| Lentitud al abrir ficheros | Carpeta abierta desde `\\wsl$\...` en Windows | Abrir con `code .` desde Ubuntu |
| `Ctrl+Shift+B` no compila | `build.sh` sin permisos de ejecución | `chmod +x ~/p1-i2c/build.sh` |
