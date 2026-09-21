# Proyecto 1 · Driver de kernel para sensor I2C/SPI
## Semana 1, día 1: entorno, kernel LTS, initramfs con BusyBox y primer módulo

**Objetivo:** arrancar un kernel Linux 6.12 LTS propio (arm64) en QEMU, con un
initramfs mínimo basado en BusyBox, y cargar/descargar un módulo de kernel
escrito por mí.

**Entorno:** Windows 10 + WSL2 (Ubuntu 24.04). Compilación cruzada x86 → arm64,
ejecución en QEMU (`-M virt`).

**Resultado esperado:** en la consola de QEMU aparecen `hello: cargado` y
`hello: descargado`.

---

## Dónde se ejecuta cada cosa

| Sitio | Prompt | Qué se hace aquí |
|---|---|---|
| PowerShell (Windows) | `PS C:\...>` | Solo gestionar WSL (`wsl ...`) |
| Ubuntu (WSL2) | `usuario@PC:~$` | Todo: instalar, compilar, empaquetar, lanzar QEMU |
| Dentro de QEMU | `~ #` | Solo `insmod`, `dmesg`, `rmmod` y pruebas |

Trabajar siempre en `~/` (nunca en `/mnt/c/...`). Para salir de QEMU: `Ctrl-A` y luego `X`.

---

## 0. WSL2 (PowerShell, una sola vez)

```powershell
wsl -l -v                          # lista distros y versión (debe ser VERSION 2)
wsl --unregister Ubuntu            # OJO: borra la distro y todos sus ficheros
wsl --install -d Ubuntu-24.04      # instala Ubuntu 24.04 desde cero
```

## 1. Paquetes (Ubuntu)

```bash
cd ~
sudo apt update && sudo apt install -y build-essential bc bison flex \
  libssl-dev libelf-dev libncurses-dev dwarves cpio xz-utils git wget \
  gcc-aarch64-linux-gnu qemu-system-arm
aarch64-linux-gnu-gcc --version    # comprueba el cross-compilador (x86 -> arm64)
qemu-system-aarch64 --version      # comprueba el emulador arm64
```

## 2. Kernel LTS 6.12

```bash
cd ~
# Busca el nombre de la última 6.12.x y lo guarda en V
V=$(wget -qO- https://cdn.kernel.org/pub/linux/kernel/v6.x/ | grep -o 'linux-6\.12\.[0-9]*\.tar\.xz' | sort -uV | tail -1)
echo $V          # debe mostrar linux-6.12.NN.tar.xz; si sale vacío, parar aquí
wget https://cdn.kernel.org/pub/linux/kernel/v6.x/$V     # descarga el código fuente
tar xf $V                                                # descomprime
mv ${V%.tar.xz} linux                                    # renombra a "linux"
cd linux
export ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-       # compilar para arm64
make defconfig                                           # configuración por defecto (.config)
make -j$(nproc) Image modules                            # compila kernel y módulos
ls -lh ~/linux/arch/arm64/boot/Image                     # comprueba que existe
```

> Los `export` solo valen para la terminal actual. Si la cierras, repítelos
> antes de volver a usar `make` con el kernel o con BusyBox.

## 3. BusyBox estático

```bash
cd ~
wget https://busybox.net/downloads/busybox-1.36.1.tar.bz2
tar xf busybox-1.36.1.tar.bz2 && cd busybox-1.36.1
make defconfig
sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/' .config   # binario estático, sin librerías
sed -i 's/CONFIG_TC=y/# CONFIG_TC is not set/' .config           # el applet "tc" falla al compilar
make -j$(nproc) && make install CONFIG_PREFIX=$HOME/rootfs       # instala en ~/rootfs
file ~/rootfs/bin/busybox        # debe decir: ARM aarch64, statically linked
```

Al terminar `make install` aparece un aviso sobre `setuid root`: es normal y no afecta.
El binario es arm64, así que en Ubuntu (x86) dará `Exec format error`: es lo esperado.

## 4. initramfs

```bash
cd ~/rootfs && mkdir -p dev proc sys etc tmp root
cat > init <<'EOF'
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev
echo "Hola desde initramfs"
exec /bin/sh
EOF
chmod +x init
find . | cpio -o -H newc --owner root:root | gzip > ~/initramfs.cpio.gz
```

`init` es el primer programa que ejecuta el kernel: monta `/proc`, `/sys` y `/dev`,
imprime un mensaje y lanza una shell.

## 5. Arrancar en QEMU

```bash
qemu-system-aarch64 -M virt -cpu max -m 512 -nographic \
  -kernel ~/linux/arch/arm64/boot/Image -initrd ~/initramfs.cpio.gz \
  -append "console=ttyAMA0 rdinit=/init"
```

| Opción | Significado |
|---|---|
| `-M virt` | Máquina virtual genérica arm64 |
| `-cpu max` | CPU con todas las funciones disponibles |
| `-m 512` | 512 MB de RAM |
| `-nographic` | La terminal actúa como pantalla |
| `-kernel` / `-initrd` | Kernel y sistema de ficheros inicial a cargar |
| `-append` | Parámetros del kernel: consola serie y programa `init` |

Debe aparecer `Hola desde initramfs` y el prompt `~ #`. El aviso
`can't access tty; job control turned off` es normal (opcional: cambiar la última
línea de `init` por `exec setsid cttyhack sh`).

## 6. Primer módulo de kernel

En Ubuntu, fuera de QEMU:

```bash
mkdir -p ~/hello && cd ~/hello
cat > hello.c <<'EOF'
#include <linux/module.h>
#include <linux/init.h>

static int __init hello_init(void) { pr_info("hello: cargado\n"); return 0; }
static void __exit hello_exit(void) { pr_info("hello: descargado\n"); }

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Primer modulo");
EOF
# El Makefile necesita un TABULADOR antes de $(MAKE); printf lo genera bien
printf 'obj-m += hello.o\nKDIR ?= $(HOME)/linux\nall:\n\t$(MAKE) -C $(KDIR) M=$(PWD) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules\n' > Makefile
make
file ~/hello/hello.ko            # debe decir: ELF 64-bit ... ARM aarch64
```

Meter el módulo en el initramfs y repackear:

```bash
cp ~/hello/hello.ko ~/rootfs/root/
cd ~/rootfs && find . | cpio -o -H newc --owner root:root | gzip > ~/initramfs.cpio.gz
```

Arrancar QEMU (paso 5) y, **dentro de QEMU** (`~ #`):

```sh
ls /root                 # debe aparecer hello.ko
insmod /root/hello.ko    # carga el módulo
dmesg | tail             # mensajes del kernel: "hello: cargado"
rmmod hello              # descarga el módulo: "hello: descargado"
```

El aviso `loading out-of-tree module taints kernel` es normal: solo indica que el
módulo no forma parte del propio kernel.

---

## Flujo de trabajo cada vez que cambio el módulo

1. Editar `hello.c` y ejecutar `make` en `~/hello`
2. `cp hello.ko ~/rootfs/root/`
3. Repackear el initramfs (`find . | cpio ... > ~/initramfs.cpio.gz`)
4. Lanzar QEMU y probar con `insmod` / `dmesg` / `rmmod`

## Volver a abrir el entorno sin reinstalar nada

Todo lo instalado y compilado sigue en `~` de Ubuntu. Solo hay que entrar y lanzar QEMU.

**1. Entrar en Ubuntu.** Menú Inicio → Ubuntu 24.04, o desde PowerShell:

```powershell
wsl -d Ubuntu-24.04
```

**2. Crear el atajo (una sola vez):**

```bash
cat > ~/run.sh <<'EOF'
#!/bin/sh
qemu-system-aarch64 -M virt -cpu max -m 512 -nographic \
  -kernel ~/linux/arch/arm64/boot/Image -initrd ~/initramfs.cpio.gz \
  -append "console=ttyAMA0 rdinit=/init"
EOF
chmod +x ~/run.sh
```

**3. Desde entonces, para arrancar la VM:**

```bash
~/run.sh
```

**Si cambio el módulo**, antes de arrancar (los `export` hay que repetirlos en cada terminal nueva):

```bash
export ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
cd ~/hello && make && cp hello.ko ~/rootfs/root/
cd ~/rootfs && find . | cpio -o -H newc --owner root:root | gzip > ~/initramfs.cpio.gz
~/run.sh
```

Lo que se hace dentro de QEMU no se guarda: el initramfs se carga en RAM y cada
arranque parte de cero. Lo que se quiera conservar debe estar en `~/rootfs` antes
de repackear.

## Errores que aparecieron y su causa

| Síntoma | Causa | Solución |
|---|---|---|
| `wget` da 404 con `linux-6.12.XX...` | `XX` era un marcador, no un número real | Usar el bloque que calcula `$V` |
| `make: No rule to make target 'defconfig'` | Se ejecutó fuera de `~/linux` | `cd ~/linux` primero |
| `Exec format error` al ejecutar BusyBox en Ubuntu | El binario es arm64 y el PC es x86 | Es normal; solo corre dentro de QEMU |
| `insmod: Permission denied` en `albert@pc:~$` | Se ejecutó en Ubuntu, no en QEMU | Ejecutar en el prompt `~ #` de QEMU |
| `insmod: can't read '/root/hello.ko'` | El módulo no estaba en el initramfs | Copiar `hello.ko` y repackear |

## Entregable del día

- [x] Kernel 6.12 LTS arm64 compilado
- [x] initramfs con BusyBox arrancando en QEMU
- [x] `hello.ko` cargado y descargado con mensajes en `dmesg`
- [ ] Repo Git con `hello.c`, `Makefile` y este documento
- [ ] Captura de pantalla del `insmod` / `dmesg` / `rmmod`

**Siguiente (día 2):** automatizar build y arranque con scripts (`build.sh`, `run.sh`)
y activar `CONFIG_I2C_STUB` en el kernel, base del driver del sensor.
