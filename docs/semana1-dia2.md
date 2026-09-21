# Proyecto 1 · Driver de kernel para sensor I2C/SPI
## Semana 1, día 2: scripts, bus I2C simulado y repositorio Git

**Objetivo:** automatizar build y arranque con scripts, tener un bus I2C simulado
(`i2c-stub`) y guardar todo en un repositorio Git.

**Resultado esperado:** `i2cget` devuelve `0x1234` en el "sensor" simulado y el
primer commit del repo está hecho.

**Requisito:** día 1 completado (kernel, BusyBox e initramfs funcionando).

---

## Dónde se ejecuta cada cosa

| Sitio | Prompt | Qué se hace aquí |
|---|---|---|
| Ubuntu (WSL2) | `usuario@PC:~$` | Compilar, scripts, Git, lanzar QEMU |
| Dentro de QEMU | `~ #` | Solo `insmod`, `i2cset`, `i2cget`, `dmesg`... |

Git y el repo **no existen dentro de QEMU**. Para salir de QEMU: `Ctrl-A` y luego `X`.

---

## 1. Activar I2C_STUB en el kernel (Ubuntu)

```bash
cd ~/linux
export ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-   # compilar para arm64
scripts/config --module I2C_STUB --enable I2C_CHARDEV
      # i2c-stub como módulo (bus I2C falso) y /dev/i2c-N para i2cset/i2cget
make olddefconfig                # aplica el cambio a la configuración
make -j$(nproc) Image modules    # recompila solo lo afectado
ls -l drivers/i2c/i2c-stub.ko    # comprueba que el módulo existe
```

## 2. Repositorio con scripts (Ubuntu)

```bash
mkdir -p ~/p1-i2c/modules && cd ~/p1-i2c && git init
mv ~/hello ~/p1-i2c/modules/hello                        # el módulo del día 1 entra al repo
sed -i 's/\$(PWD)/$(CURDIR)/' modules/hello/Makefile     # funciona también con make -C
```

**`env.sh`**: variables comunes, para no repetir los `export` a mano.

```bash
cat > env.sh <<'EOF'
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
export KDIR=$HOME/linux
export ROOTFS=$HOME/rootfs
export INITRAMFS=$HOME/initramfs.cpio.gz
EOF
```

**`build.sh`**: compila todos los módulos, los copia al rootfs y repackea el initramfs.

```bash
cat > build.sh <<'EOF'
#!/bin/sh
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
. "$HERE/env.sh"
for d in "$HERE"/modules/*/; do
  make -C "$d"
  cp "$d"/*.ko "$ROOTFS/root/"
done
cp "$KDIR/drivers/i2c/i2c-stub.ko" "$ROOTFS/root/"
(cd "$ROOTFS" && find . | cpio -o -H newc --owner root:root | gzip > "$INITRAMFS")
echo "OK: initramfs actualizado"
EOF
```

**`run.sh`**: arranca QEMU.

```bash
cat > run.sh <<'EOF'
#!/bin/sh
. "$(dirname "$0")/env.sh"
exec qemu-system-aarch64 -M virt -cpu max -m 512 -nographic \
  -kernel "$KDIR/arch/arm64/boot/Image" -initrd "$INITRAMFS" \
  -append "console=ttyAMA0 rdinit=/init"
EOF
```

**`.gitignore`** y permisos:

```bash
printf '*.ko\n*.o\n*.mod\n*.mod.c\n*.symvers\n*.order\n.*.cmd\n' > .gitignore
chmod +x build.sh run.sh
```

| Fichero | Para qué sirve |
|---|---|
| `env.sh` | Variables comunes: arquitectura, compilador, rutas del kernel y del rootfs |
| `build.sh` | Compila `modules/*`, copia los `.ko` (e `i2c-stub.ko`) al rootfs y repackea |
| `run.sh` | Lanza QEMU con el kernel y el initramfs |
| `.gitignore` | Evita subir ficheros generados |

## 3. Compilar y arrancar

```bash
~/p1-i2c/build.sh
find ~/rootfs -name 'i2c*'       # deben salir i2cdetect, i2cget, i2cset... (en usr/sbin)
~/p1-i2c/run.sh
```

BusyBox instala los applets I2C en `usr/sbin`, no en `bin`. Si `find` no muestra nada,
entonces sí hay que activarlos en BusyBox y recompilarlo.

## 4. Probar el bus simulado (dentro de QEMU, `~ #`)

```sh
insmod /root/i2c-stub.ko chip_addr=0x48   # crea un "sensor" falso en la dirección 0x48
i2cdetect -l                              # lista buses (letra L minúscula, no el número 1)
i2cset -y 0 0x48 0x00 0x1234 w            # escribe 0x1234 en el registro 0
i2cget -y 0 0x48 0x00 w                   # debe devolver 0x1234
```

Si `i2cdetect -l` muestra otro número que `i2c-0`, usar ese en `i2cset`/`i2cget`.
Este registro simulado es lo que leerá el driver en la semana 2.

## 5. Git (Ubuntu, fuera de QEMU)

```bash
git config --global user.name "Tu Nombre"        # identidad, una sola vez
git config --global user.email "tu@email.com"
cd ~/p1-i2c
git add .                                        # prepara todos los ficheros
git commit -m "Scripts and simulated I2C bus"    # guarda una versión
git log --oneline                                # lista de commits
```

El commit queda **solo en local**, en la carpeta oculta `.git` del repo. El email
queda visible en cada commit si se sube a GitHub; GitHub ofrece una dirección
privada `ID+usuario@users.noreply.github.com` (Settings → Emails).

### Subirlo a GitHub (copia de seguridad y portfolio)

1. En github.com, crear un repositorio `p1-i2c` **vacío** (sin README ni `.gitignore`).
2. En Ubuntu:

```bash
cd ~/p1-i2c
git branch -M main
git remote add origin https://github.com/<tu-usuario>/p1-i2c.git
git push -u origin main
```

GitHub no acepta la contraseña de la cuenta: pide un *personal access token*
(Settings → Developer settings → Personal access tokens).

## Dónde está la carpeta en Windows

Vive dentro del disco de Ubuntu (WSL), no en `C:`:

```
/home/albert/p1-i2c
```

Dos formas de abrirla desde Windows:

```bash
cd ~/p1-i2c && explorer.exe .    # desde Ubuntu: abre el Explorador en esa carpeta
```

O pegando en la barra de direcciones del Explorador:

```
\\wsl$\Ubuntu-24.04\home\albert\p1-i2c
```

(También vale `\\wsl.localhost\Ubuntu-24.04\...`.) Conviene no mover ni renombrar
cosas del kernel o del rootfs desde Windows, por los permisos de Linux.

**Aviso:** todo ese disco se borra con `wsl --unregister`. Subir el repo a GitHub
es la copia de seguridad.

## Volver a trabajar otro día

```bash
wsl -d Ubuntu-24.04              # desde PowerShell (o abrir Ubuntu desde el menú Inicio)
cd ~/p1-i2c
./build.sh                       # solo si he cambiado módulos
./run.sh                         # arranca QEMU (salir: Ctrl-A y luego X)
```

## Errores que aparecieron y su causa

| Síntoma | Causa | Solución |
|---|---|---|
| `i2cdetect: invalid option -- '1'` | Se escribió el número 1 en vez de la letra L | `i2cdetect -l` |
| `cinsmod: not found` | Se coló una `c` al teclear | `insmod ...` |
| `cd: can't cd to //p1-i2c` dentro de QEMU | Se ejecutó en QEMU, donde no existe el repo | Salir con `Ctrl-A` `X` y hacerlo en Ubuntu |
| `insmod: can't insert ...: File exists` | El módulo ya estaba cargado | No hace falta cargarlo otra vez (o `rmmod` antes) |
| `insmod: Permission denied` en Ubuntu | El módulo es arm64 y solo se carga en QEMU | Ejecutarlo en el prompt `~ #` |
| `git: 'add.' is not a git command` | Faltaba el espacio | `git add .` |
| `Author identity unknown` | Git no sabía quién soy | `git config --global user.name/user.email` |

## Entregable del día

- [x] Kernel con `i2c-stub` activado
- [x] Scripts `env.sh`, `build.sh` y `run.sh`
- [x] `i2cget` devolviendo `0x1234` desde el sensor simulado
- [x] Primer commit del repo (`fd15159`)
- [ ] Repo subido a GitHub
- [ ] Guías `semana1-dia1.md` y `semana1-dia2.md` copiadas a `docs/` en el repo

**Siguiente (día 3):** por definir dentro de la semana 1.
