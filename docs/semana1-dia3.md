# Proyecto 1 · Driver de kernel para sensor I2C/SPI
## Semana 1, día 3: módulo con parámetro y dispositivo `/dev`

**Objetivo:** un módulo que acepta un parámetro al cargarlo, crea un dispositivo de
caracteres (`/dev/hello_misc`) y permite leer y escribir datos desde la shell.

**Por qué:** el driver I2C de la semana 2 necesita el mismo patrón: registrar en
`init`, desregistrar en `exit`, gestionar errores y pasar datos entre kernel y
espacio de usuario (`copy_from_user`, `simple_read_from_buffer`).

**Requisito:** días 1 y 2 completados (repo `~/p1-i2c` con `env.sh`, `build.sh` y `run.sh`).

---

## Dónde se ejecuta cada cosa

| Sitio | Prompt | Qué se hace aquí |
|---|---|---|
| Ubuntu (WSL2) | `usuario@PC:~$` | Crear ficheros, compilar, Git, lanzar QEMU |
| Dentro de QEMU | `~ #` | `insmod`, `rmmod`, `cat`, `echo`, `dmesg` |

Dentro de QEMU ya eres root (uid 0): no hace falta `sudo`.
Para salir de QEMU: `Ctrl-A` y luego `X`.

---

## 1. Crear el módulo (Ubuntu)

```bash
mkdir -p ~/p1-i2c/modules/hello_misc && cd ~/p1-i2c/modules/hello_misc
```

Crear `hello_misc.c` con este contenido (con VS Code es lo más cómodo; si se pega
en la terminal con `cat > ... <<'EOF'`, comprobar después con `wc -l hello_misc.c`
que tiene unas 70 líneas y no una sola):

```c
// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define BUF_SZ 128

static char *greeting = "hola";
module_param(greeting, charp, 0444);
MODULE_PARM_DESC(greeting, "Texto inicial del dispositivo");

static char buf[BUF_SZ];
static size_t buf_len;
static DEFINE_MUTEX(buf_lock);

static ssize_t hm_read(struct file *f, char __user *ubuf, size_t count, loff_t *ppos)
{
    ssize_t ret;

    mutex_lock(&buf_lock);
    ret = simple_read_from_buffer(ubuf, count, ppos, buf, buf_len);
    mutex_unlock(&buf_lock);
    return ret;
}

static ssize_t hm_write(struct file *f, const char __user *ubuf, size_t count, loff_t *ppos)
{
    if (count >= BUF_SZ)
        return -EINVAL;

    mutex_lock(&buf_lock);
    if (copy_from_user(buf, ubuf, count)) {
        mutex_unlock(&buf_lock);
        return -EFAULT;
    }
    buf_len = count;
    mutex_unlock(&buf_lock);
    return count;
}

static const struct file_operations hm_fops = {
    .owner = THIS_MODULE,
    .read  = hm_read,
    .write = hm_write,
};

static struct miscdevice hm_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "hello_misc",
    .fops  = &hm_fops,
};

static int __init hm_init(void)
{
    int ret;

    buf_len = scnprintf(buf, BUF_SZ, "%s\n", greeting);
    ret = misc_register(&hm_dev);
    if (ret) {
        pr_err("hello_misc: misc_register fallo (%d)\n", ret);
        return ret;
    }
    pr_info("hello_misc: /dev/%s listo\n", hm_dev.name);
    return 0;
}

static void __exit hm_exit(void)
{
    misc_deregister(&hm_dev);
    pr_info("hello_misc: descargado\n");
}

module_init(hm_init);
module_exit(hm_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Dia 3: misc device con parametro");
```

Crear el `Makefile` (necesita un TABULADOR antes de `$(MAKE)`; `printf` lo genera bien):

```bash
printf 'obj-m += hello_misc.o\nKDIR ?= $(HOME)/linux\nall:\n\t$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules\n' > Makefile
```

## 2. Compilar y arrancar (Ubuntu)

```bash
~/p1-i2c/build.sh
ls ~/rootfs/root       # deben aparecer hello.ko, hello_misc.ko e i2c-stub.ko
~/p1-i2c/run.sh
```

`build.sh` recorre `modules/*`, así que recoge el módulo nuevo sin tocar el script.

## 3. Probar (dentro de QEMU, `~ #`)

```sh
insmod /root/hello_misc.ko
ls -l /dev/hello_misc                              # el nodo lo crea devtmpfs
cat /dev/hello_misc                                # hola
echo "adios" > /dev/hello_misc
cat /dev/hello_misc                                # adios
cat /sys/module/hello_misc/parameters/greeting     # el parámetro también está en sysfs
rmmod hello_misc
insmod /root/hello_misc.ko greeting=otro           # cargar con otro valor
cat /dev/hello_misc                                # otro
dmesg | tail
```

Prueba extra: una escritura de 128 bytes o más debe fallar con `Invalid argument`.

```sh
head -c 200 /dev/zero | tr '\0' 'a' > /dev/hello_misc
```

---

## Cómo funciona el código

| Pieza | Qué hace |
|---|---|
| `module_param(greeting, charp, 0444)` | Parámetro de tipo texto; aparece en `/sys/module/hello_misc/parameters/greeting`. `0444` = solo lectura |
| `buf`, `buf_len` | Almacén de 128 bytes en el kernel y cuántos bytes son válidos |
| `buf_lock` (mutex) | Evita que dos procesos lean y escriban a la vez y vean un texto a medias |
| `hm_read` | Se ejecuta con `cat`. `simple_read_from_buffer` copia al espacio de usuario y avanza el offset; al llegar al final devuelve 0 y `cat` termina |
| `hm_write` | Se ejecuta con `echo >`. Rechaza `count >= BUF_SZ` (`-EINVAL`), copia con `copy_from_user` (`-EFAULT` si falla) y devuelve `count` |
| `hm_fops` | Enlaza las operaciones de fichero con las funciones. `.owner = THIS_MODULE` impide descargar el módulo mientras está abierto |
| `miscdevice` + `MISC_DYNAMIC_MINOR` | El kernel asigna un número libre; `devtmpfs` crea `/dev/hello_misc` |
| `hm_init` / `hm_exit` | Registran y desregistran el dispositivo. Si `misc_register` falla, se devuelve el error y `insmod` falla |

El kernel nunca debe desreferenciar directamente un puntero de usuario: por eso se
usan `copy_from_user` y `simple_read_from_buffer`.

---

## Cambiar el parámetro como root

**Al cargar (lo soportado con `0444`):**

```sh
rmmod hello_misc
insmod /root/hello_misc.ko greeting=otro
cat /dev/hello_misc                 # otro
```

Con espacios: `greeting="hola mundo"`.

**Con el módulo ya cargado**, cambiar los permisos a `0644` (escritura solo para root),
recompilar (`build.sh`) y arrancar:

```c
module_param(greeting, charp, 0644);
```

```sh
insmod /root/hello_misc.ko
echo otro > /sys/module/hello_misc/parameters/greeting
cat /sys/module/hello_misc/parameters/greeting   # otro
cat /dev/hello_misc                              # sigue mostrando "hola"
```

El último `cat` sigue mostrando "hola" porque el buffer se rellena una sola vez en
`hm_init`. Para que el dispositivo refleje el cambio, variante opcional:

```c
static bool written;                                            /* NUEVO */

/* en hm_read, dentro del lock, antes de simple_read_from_buffer: */
    if (!written)                                               /* NUEVO */
        buf_len = scnprintf(buf, BUF_SZ, "%s\n", greeting);     /* NUEVO */

/* en hm_write, junto a buf_len = count: */
    written = true;                                             /* NUEVO */
```

En un driver real esto se haría con `module_param_cb` y una función que valide y
bloquee al cambiar el valor, porque sustituir el puntero mientras otro proceso lo
lee es una carrera. Para practicar, así vale.

---

## Git (Ubuntu, fuera de QEMU)

```bash
cd ~/p1-i2c
git add .
git commit -m "Dia 3: modulo misc con parametro"
git log --oneline
```

## Errores habituales

| Síntoma | Causa | Solución |
|---|---|---|
| El `.c` no compila, todo parece un comentario | Se pegó el código en una sola línea y el `//` del principio lo comenta | Crear el fichero con VS Code o comprobar `wc -l hello_misc.c` |
| `make: *** missing separator` | El Makefile no tiene tabulador antes de `$(MAKE)` | Regenerarlo con el `printf` de arriba |
| `Permission denied` al escribir en `.../parameters/greeting` | Permisos `0444` (solo lectura) | Cambiar a `0644` y recompilar |
| `insmod: File exists` | El módulo ya estaba cargado | `rmmod hello_misc` y volver a cargar |
| `Invalid argument` al escribir 200 bytes | Es lo esperado (`count >= BUF_SZ`) | Nada que corregir |
| `insmod: Permission denied` en Ubuntu | Se ejecutó fuera de QEMU; el módulo es arm64 | Ejecutarlo en el prompt `~ #` |

## Entregable del día

- [ ] `hello_misc.ko` cargado, con `cat` mostrando `hola` y luego `adios`
- [ ] Carga con `greeting=otro` y comprobación en `/dev` y en sysfs
- [ ] La escritura de 200 bytes falla con `Invalid argument`
- [ ] Commit del día 3
- [ ] Repo subido a GitHub y guías copiadas a `docs/`
