# 📚 Guía de Estudio — Sistemas Operativos (RECU)
> Generada a partir del análisis de: Dic25a, Dic25b, Ene23, Ene24, Jun24, Jun25 + P3, P4, P5, P6, P7

---

## 🔥 Lo que MÁS CAE (por frecuencia en exámenes)

| Tema | Aparece en |
|------|-----------|
| `fork` + `execvp/execl` + `waitpid` | **TODOS** los exámenes ej1 |
| `getopt` para parsear flags | Dic25a, Dic25b, Jun25, P3, P4 |
| Semáforos POSIX con nombre (`sem_open`) | Dic25a ej2, Dic25b ej2, P6 |
| Memoria compartida (`shm_open` + `mmap`) | Dic25a ej2, Dic25b ej2, P6 |
| `pthreads` + mutex + variables de condición | Dic25a ej2, Jun25 ej2, P5 |
| `parse_command` (tokenizar string) | Dic25a ej1, P4 |
| `waitpid(-1, ..., WNOHANG)` polling | Dic25a ej1 |
| `WIFEXITED` / `WIFSIGNALED` / `WTERMSIG` | Dic25b ej1, Jun25 ej1 |
| Señales: `sigaction`, `SIG_IGN`, `SIG_DFL` | Jun25 ej1, Dic25b ej2 |
| Productor-Consumidor (con sems o cond vars) | Jun25 ej2, Dic25b ej2 |
| `open` + `read` + `write` + `lseek` | P3 (ejercicio3: lseek, ejercicio1: copy) |
| Módulos del kernel (chardev) | P7 |

---

## 📦 BLOQUE 1 — E/S de Bajo Nivel (P3)

### Funciones clave
```c
int fd = open(path, O_RDONLY);                          // Abrir lectura
int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644); // Crear/sobreescribir
ssize_t n = read(fd, buf, size);   // 0 = EOF, -1 = error
ssize_t n = write(fd, buf, n);     // Comparar con lo leído
off_t pos = lseek(fd, offset, SEEK_SET/SEEK_END/SEEK_CUR);
close(fd);
```

### ⚠️ TRAMPAS MÁS HABITUALES

**TRAMPA 1 — Prioridad de operadores en `open`:**
```c
// ❌ MAL: el == se evalúa antes que el =
if (fd = open(path, O_RDONLY) == -1)

// ✅ BIEN: paréntesis extra obligatorios
if ((fd = open(path, O_RDONLY)) == -1)
```

**TRAMPA 2 — `lseek` al final: offset NEGATIVO:**
```c
// Para leer los últimos N bytes:
lseek(fd, -N, SEEK_END);  // ← signo negativo, no positivo
```

**TRAMPA 3 — `write` puede escribir menos bytes de los pedidos:**
```c
ssize_t w = write(fd, buf, n_leidos);
if (w != n_leidos) { /* Error o disco lleno */ }
```

**TRAMPA 4 — `read` devuelve 0 en EOF, no -1:**
```c
while ((n = read(fd, buf, BUF_SIZE)) > 0) { ... }
if (n == -1) perror("error"); // Tratar el -1 por separado
```

---

## 🔀 BLOQUE 2 — Procesos: fork, exec, wait (P4, exámenes ej1)

### Patrón estándar que SIEMPRE aparece en el ej1

```c
pid_t launch_command(char **argv) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(EXIT_FAILURE); }
    else if (pid == 0) {
        execvp(argv[0], argv);
        perror("execvp");  // Solo llega aquí si exec falla
        exit(EXIT_FAILURE);
    }
    return pid; // El padre devuelve el PID del hijo
}
```

### `getopt` — Aparece en TODOS los exámenes
```c
int opt;
while ((opt = getopt(argc, argv, "x:s:b")) != -1) {
    switch (opt) {
    case 'x': cmd = optarg; break;  // ← optarg para opciones con argumento
    case 'b': b_flag = 1;   break;  // ← sin : = flag sin argumento
    default:
        fprintf(stderr, "Uso: ...\n");
        return EXIT_FAILURE;
    }
}
// Argumentos no-opción: argv[optind], argv[optind+1], ...
int num_ficheros = argc - optind;
```

### `waitpid` — Variantes clave

```c
// 1. Esperar a UN hijo específico (bloqueante):
waitpid(pid, &status, 0);

// 2. Esperar a CUALQUIER hijo (bloqueante):
pid_t ended = waitpid(-1, &status, 0);

// 3. Polling sin bloqueo (background mode, Dic25a):
pid = waitpid(-1, &status, WNOHANG);
// pid == 0 → ningún hijo terminó aún
// pid > 0  → ese hijo terminó
```

### Macros de estado — MUY preguntadas

```c
if (WIFEXITED(status))   printf("exit: %d", WEXITSTATUS(status));
if (WIFSIGNALED(status)) printf("señal: %d", WTERMSIG(status));
// WIFSTOPPED y WIFCONTINUED existen pero raramente se piden
```

### ⚠️ TRAMPAS

**TRAMPA 1 — `execvp` necesita NULL al final del argv:**
```c
char *args[] = {"ls", "-l", NULL};  // ← El NULL es OBLIGATORIO
execvp(args[0], args);
```

**TRAMPA 2 — El hijo SIEMPRE debe llamar `exit` si exec falla:**
```c
execvp(...);
perror("execvp");
exit(EXIT_FAILURE); // ← Sin esto, el hijo continúa ejecutando código del padre
```

**TRAMPA 3 — `optind` después de `getopt`:**
```c
while (getopt(...) != -1) { ... }
// Ahora argv[optind] es el PRIMER argumento NO-opción
char *fichero = argv[optind]; // ← Correcto
```

**TRAMPA 4 — Modo background: hay que esperar a TODOS al final:**
```c
// Si lanzas en background, al final del programa:
int completo = 0;
while (completo < total_bg) {
    pid = waitpid(-1, &status, WNOHANG);
    if (pid > 0) { /* imprimir resultado y */ completo++; }
    else if (pid == 0) usleep(10000); // Evitar busy-wait
}
```

---

## 🚦 BLOQUE 3 — Señales: sigaction (Jun25 ej1, Dic25b ej2)

### Patrón completo de sigaction

```c
struct sigaction sa = { 0 };
sa.sa_handler = mi_handler;
sa.sa_flags = 0;
sigemptyset(&sa.sa_mask);
sigaction(SIGINT, &sa, NULL);
sigaction(SIGTERM, &sa, NULL);
```

### SIG_IGN + SIG_DFL — Jun25 ej1 (muy específico)

```c
// Patrón del "time" que ignora Ctrl+C pero el hijo no:
// PADRE: ignora señales antes del fork
sigaction(SIGINT, &sa_ignore, NULL);  // sa_handler = SIG_IGN

pid_t pid = fork();
if (pid == 0) {
    // HIJO: restaurar comportamiento por defecto
    sa_default.sa_handler = SIG_DFL;
    sigaction(SIGINT, &sa_default, NULL);
    execvp(...);
}
// Padre espera pero puede ser interrumpido por señal → manejar EINTR
while (waitpid(pid, &status, 0) < 0) {
    if (errno != EINTR) { perror("waitpid"); exit(1); }
}
```

### Handler: `volatile sig_atomic_t`

```c
volatile sig_atomic_t finish = 0;  // ← SIEMPRE esta declaración
void handler(int signo) {
    finish = 1;
    // ¡NO hacer printf en el handler si hay malloc! Es inseguro
}
// En el bucle:
while (!finish) {
    if (sem_wait(sem) == -1) {
        if (errno == EINTR) continue;  // ← Reintentar si interrumpido por señal
        perror("sem_wait");
        break;
    }
}
```

### ⚠️ TRAMPA — `EINTR` en sem_wait y waitpid

```c
// sem_wait puede volver con errno=EINTR si llega una señal
// Si no lo tratas, el programa termina inesperadamente
if (sem_wait(sem) == -1) {
    if (errno == EINTR) continue; // ← Volver al bucle
    perror("error fatal");
    return;
}
```

---

## 🧵 BLOQUE 4 — pthreads: mutex + variables de condición (P5, Jun25 ej2)

### Declaración y ciclo de vida

```c
pthread_mutex_t mutex;
pthread_cond_t condProd, condCons;

pthread_mutex_init(&mutex, NULL);
pthread_cond_init(&condProd, NULL);
pthread_cond_init(&condCons, NULL);

// Al final:
pthread_mutex_destroy(&mutex);
pthread_cond_destroy(&condProd);
pthread_cond_destroy(&condCons);
```

### Patrón CORRECTO de cond_wait — SIEMPRE `while`, NUNCA `if`

```c
pthread_mutex_lock(&mutex);
while (condicion_no_cumplida) {         // ← WHILE, no if (spurious wakeups)
    pthread_cond_wait(&cond, &mutex);   // libera mutex, duerme, readquiere al despertar
}
// ... hacer trabajo ...
pthread_cond_signal(&condOtra);         // Avisar al otro
pthread_mutex_unlock(&mutex);
```

### Patrón Productor-Consumidor (Jun25 ej2)

```c
// PRODUCTOR:
pthread_mutex_lock(&mutex);
while (buffer_lleno) {
    pthread_cond_signal(&condCons);    // Avisar al consumidor
    pthread_cond_wait(&condProd, &mutex);
}
buffer[ind++] = dato;
if (ind == MAX) pthread_cond_signal(&condCons);
pthread_mutex_unlock(&mutex);

// CONSUMIDOR:
pthread_mutex_lock(&mutex);
while (ind == 0) {
    pthread_cond_wait(&condCons, &mutex);
}
procesar(buffer, ind);
ind = 0;
pthread_cond_signal(&condProd);
pthread_mutex_unlock(&mutex);
```

### ⚠️ TRAMPAS threads

**TRAMPA 1 — Pasar argumentos al hilo: USAR malloc, nunca punteros a pila:**
```c
// ❌ MAL: cliente se destruye al salir del for
for (int i = 0; i < n; i++) {
    cliente_t cliente = {i, tipo};
    pthread_create(&hilos[i], NULL, func, &cliente); // PELIGRO
}

// ✅ BIEN: malloc por cada hilo
cliente_t *cliente = malloc(sizeof(cliente_t));
cliente->id = i;
pthread_create(&hilos[i], NULL, func, cliente);
// El hilo hace free(cliente) al terminar
```

**TRAMPA 2 — Olvidar `pthread_join`:**
```c
// Sin join, los hilos quedan como zombies y los recursos no se liberan
for (int i = 0; i < n; i++)
    pthread_join(hilos[i], NULL);
```

**TRAMPA 3 — signal vs broadcast:**
```c
// signal: despierta UNO (cuando solo uno puede proceder)
pthread_cond_signal(&cond);
// broadcast: despierta TODOS (cuando varios pueden proceder o al terminar)
pthread_cond_broadcast(&cond);
// En disco.c: exit llama broadcast para no dejar hilos bloqueados
```

---

## 🔐 BLOQUE 5 — Semáforos POSIX + Memoria compartida (P6, Dic25a ej2, Dic25b ej2)

### Flujo completo (el más repetido en exámenes)

```
PROCESO CREADOR (cocinero/control/cajero):
1. sem_unlink("/nombre")      ← LIMPIAR residuos de ejecuciones anteriores
2. sem_open(O_CREAT, valor)
3. shm_open(O_CREAT|O_RDWR)
4. ftruncate(shd, tamaño)     ← El shm nace con 0 bytes, hay que darle tamaño
5. mmap(NULL, tamaño, PROT_READ|PROT_WRITE, MAP_SHARED, shd, 0)
6. close(shd)                 ← Cerrar fd, el mapeo sigue activo
7. Lógica del programa
8. munmap / sem_close / shm_unlink / sem_unlink  ← LIMPIEZA
```

### Código patrón completo

```c
// CREADOR
sem_unlink("/SEM_A");              // Evitar valores residuales del Kernel
sem_t *sem = sem_open("/SEM_A", O_CREAT, 0644, 1);  // valor inicial = 1
if (sem == SEM_FAILED) { perror("sem_open"); exit(1); }

int shd = shm_open("/BUFFER", O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
ftruncate(shd, sizeof(int));       // ← OBLIGATORIO, nace con 0 bytes
int *mem = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, shd, 0);
close(shd);                        // fd ya no necesario tras mmap

// CONSUMIDOR (proceso distinto, solo abre, no crea)
sem_t *sem = sem_open("/SEM_A", 0);       // ← sin O_CREAT
int shd = shm_open("/BUFFER", O_RDWR, 0); // ← sin O_CREAT
int *mem = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, shd, 0);
```

### Semáforos: operaciones P y V

```c
sem_wait(sem);   // P (bajar): decrementa, bloquea si = 0
sem_post(sem);   // V (subir): incrementa, despierta a uno si hay bloqueados
```

### Problema del Cocinero (Dic25b ej2) — El más complejo

```
Semáforos: empty (init=0), full (init=0)
Cocinero: sem_wait(empty) → cocinar M porciones → sem_post(full)
Salvaje:  sem_wait(m_mutex) → getServing() → si olla vacía: sem_post(empty) + sem_wait(full) → sem_post(m_mutex)
```

### ⚠️ TRAMPAS semáforos/shm

**TRAMPA 1 — El bug más importante del curso: valores residuales:**
```c
// Si O_CREAT se usa con un semáforo YA EXISTENTE, el valor inicial se IGNORA
// → El semáforo tendrá el valor de la ejecución anterior (probablemente 0 → deadlock)
// SOLUCIÓN: Siempre hacer unlink antes:
sem_unlink("/nombre");
sem = sem_open("/nombre", O_CREAT, 0644, valor_inicial);
```

**TRAMPA 2 — `ftruncate` es OBLIGATORIO:**
```c
// shm_open crea objeto de 0 bytes → al acceder → SIGBUS / Segfault
// SIEMPRE después de shm_open, antes de mmap:
ftruncate(shd, sizeof(tipo_que_compartes));
```

**TRAMPA 3 — Diferencia entre close/unlink:**
```c
sem_close(sem);     // Cierra descriptor local (solo este proceso)
sem_unlink("/sem"); // ELIMINA del Kernel (todos los procesos pierden acceso)
// Siempre: primero close, luego unlink (en el proceso creador al final)
```

**TRAMPA 4 — MAP_SHARED vs MAP_PRIVATE:**
```c
// MAP_SHARED: escrituras visibles por todos → es lo que queremos
// MAP_PRIVATE: copia privada → NO comparte datos
mmap(NULL, size, PROT, MAP_SHARED, fd, 0); // ← SIEMPRE MAP_SHARED
```

---

## ⏱️ BLOQUE 6 — gettimeofday + getrusage (Jun25 ej1)

```c
struct timeval start, end, diff;
struct rusage usage;

gettimeofday(&start, NULL);
run_command(argv[1]);           // Ejecutar lo que queremos medir
gettimeofday(&end, NULL);

// Tiempo real (wall clock):
timeval_diff(&diff, &end, &start);
printf("real: %.3f s\n", diff.tv_sec + diff.tv_usec / 1000000.0);

// Tiempo CPU de hijos (llamar DESPUÉS de waitpid):
getrusage(RUSAGE_CHILDREN, &usage);
printf("user: %.3f s\n", usage.ru_utime.tv_sec + usage.ru_utime.tv_usec/1e6);
printf("sys:  %.3f s\n", usage.ru_stime.tv_sec + usage.ru_stime.tv_usec/1e6);
```

---

## 🐧 BLOQUE 7 — Módulos del Kernel (P7)

### Estructura mínima de un chardev

```c
// 1. Registrar número de dispositivo
alloc_chrdev_region(&start, 0, 1, "nombre");

// 2. Crear y registrar cdev
chardev = cdev_alloc();
cdev_init(chardev, &fops);
cdev_add(chardev, start, 1);

// 3. file_operations
static struct file_operations fops = {
    .open    = device_open,
    .release = device_release,
    .read    = device_read,
    .write   = device_write,
    .owner   = THIS_MODULE,
};

// 4. copy_to_user (no memcpy!) para pasar datos al espacio usuario
copy_to_user(buffer, msg_ptr, bytes);

// 5. Al descargar: cdev_del + unregister_chrdev_region
```

### Comandos de kernel modules

```bash
make                        # Compilar el módulo
sudo insmod chardev.ko      # Cargar
sudo rmmod chardev          # Descargar
dmesg | tail                # Ver printk()
sudo mknod /dev/chardev c MAJOR MINOR  # Crear nodo de dispositivo
cat /dev/chardev            # Leer del dispositivo
```

---

## 🪤 PREGUNTAS TRAMPA CLÁSICAS

### 1. ¿Qué pasa si no haces `waitpid` después de `fork`?
> El hijo se convierte en **proceso zombi** (zombie process): ha terminado pero su entrada en la tabla de procesos no se libera porque el padre no ha recogido su código de salida.

### 2. ¿Por qué `pthread_cond_wait` debe estar en un `while` y no en un `if`?
> Por los **spurious wakeups**: el hilo puede despertar sin que se haya enviado una señal real (comportamiento permitido por POSIX). Con `if`, procedería aunque la condición no se cumpla. Con `while`, re-verifica.

### 3. ¿Qué valor devuelve `fork()` en el padre vs en el hijo?
> - **Padre**: recibe el PID del hijo (> 0)
> - **Hijo**: recibe 0
> - **Error**: -1 (solo en el padre)

### 4. ¿Puede `exec` fallar? ¿Qué pasa si falla?
> Sí. Si `execvp` falla (comando no encontrado, etc.), **retorna** al proceso hijo. Por eso SIEMPRE hay que poner `perror` + `exit(EXIT_FAILURE)` después del exec.

### 5. ¿Qué diferencia hay entre `sem_close` y `sem_unlink`?
> - `sem_close`: cierra el descriptor en el proceso actual (como `close` para ficheros)
> - `sem_unlink`: elimina el objeto del kernel (como `unlink` para ficheros). Si no se hace unlink, el semáforo persiste en RAM aunque el proceso muera.

### 6. ¿Qué devuelve `read` cuando llega al EOF?
> Devuelve **0** (no -1). El -1 es error. El 0 es fin de fichero.

### 7. ¿Por qué hay que hacer `ftruncate` después de `shm_open`?
> Porque `shm_open` crea un objeto de **0 bytes**. Si intentas mapearlo sin ajustar el tamaño, obtienes un **SIGBUS** al acceder a la memoria.

### 8. ¿Qué es `optind`?
> Variable global que `getopt` actualiza. Después del bucle, `argv[optind]` es el **primer argumento no-opción** (el fichero, etc.). `argc - optind` = número de argumentos restantes.

### 9. `WNOHANG` en `waitpid`: ¿qué devuelve si ningún hijo ha terminado?
> Devuelve **0** (no bloquea, no hay hijo terminado). Devuelve > 0 si algún hijo terminó, y -1 si error.

### 10. En un proceso hijo después de `fork`, ¿hereda los semáforos del padre?
> **Sí**, hereda todos los descriptores abiertos (semáforos, ficheros, etc.). Eso es precisamente lo que permite la comunicación entre padre e hijo mediante semáforos.

---

## ❌ ERRORES MÁS FRECUENTES DETECTADOS EN TU CÓDIGO

### En los exámenes propios analizados:

1. **Diciembre25a ej1** — No liberar memoria del `parse_command` en el modo `-B` (sí se libera en modo `-b`). Las ramas de código tienen manejo asimétrico de `free`.

2. **Diciembre25a ej2 (control.c)** — `while(finish != 1){}` en el proceso de control es **busy-waiting** ineficiente. Debería usar `pause()` o `sigsuspend()`.

3. **Junio25 ej2** — `if (argc < 1)` siempre es falso (argc nunca puede ser < 1). El programa debería verificar `argc < 2` o simplemente no es necesario ya que todo es opcional.

4. **Enero23 ej1** — El bucle de parseo del grafo quedó vacío (`while(cont < strlen(linea)){}`), el ejercicio no está completado.

5. **Diciembre25b ej2 (cocinero.c)** — Inicializar `empty` con valor 0 en lugar de 1:
   ```c
   // Código con el bug:
   empty = sem_open("/EMPTY", O_CREAT, 0644, 0); // ← ¿0 o 1?
   // El cocinero se bloquea inmediatamente esperando en empty
   // El valor correcto depende de si la olla empieza vacía o llena
   ```

6. **Dic25b ej2 (cajero.c)** — No se llama a `sigaction` para registrar el handler antes de la lógica principal:
   ```c
   // Se configura term e inter pero nunca se llama:
   sigaction(SIGTERM, &term, NULL); // ← FALTA en el main
   sigaction(SIGINT, &inter, NULL); // ← FALTA en el main
   ```

---

## 📋 CHECKLIST PRE-EXAMEN

### Para ejercicios de fork/exec:
- [ ] ¿El hijo llama `exit` después de `exec`?
- [ ] ¿El padre hace `waitpid`?
- [ ] ¿El argv de exec termina en NULL?
- [ ] ¿Tratas el retorno < 0 de fork?
- [ ] ¿Liberas memoria dinámica (`free`) en el padre?

### Para ejercicios de semáforos/shm:
- [ ] ¿Haces `sem_unlink` ANTES de `sem_open` con `O_CREAT`?
- [ ] ¿Haces `ftruncate` antes de `mmap`?
- [ ] ¿Cierras el shd con `close` después de `mmap`?
- [ ] ¿Al terminar: `munmap` + `sem_close` + `shm_unlink` + `sem_unlink`?
- [ ] ¿Los procesos consumidores abren con `sem_open("/nombre", 0)` (sin O_CREAT)?

### Para ejercicios de pthreads:
- [ ] ¿`pthread_cond_wait` dentro de `while`?
- [ ] ¿Los datos al hilo son dinámicos (`malloc`)? ¿El hilo hace `free`?
- [ ] ¿Todos los hilos tienen su `pthread_join`?
- [ ] ¿`pthread_mutex_destroy` + `pthread_cond_destroy` al final?

### Para ejercicios con señales:
- [ ] ¿La bandera global es `volatile sig_atomic_t`?
- [ ] ¿`sem_wait` maneja `errno == EINTR` con `continue`?
- [ ] ¿El padre ignora señales ANTES del fork si el hijo debe recibirlas?
- [ ] ¿El hijo restaura `SIG_DFL` antes del exec?

---

## 🗺️ MAPA DE PRÁCTICAS → EXÁMENES

```
P3 (open/read/write/lseek)
  └─→ Muy poco en exámenes directos, base para entender descriptores

P4 (fork + exec + getopt + parse_command)
  └─→ Dic25a ej1 (casi idéntico a run_commands.c)
  └─→ Dic25b ej1 (fork paralelo + execl)
  └─→ Jun25 ej1 (fork + exec + señales + timing)

P5 (pthreads mutex + cond_var + disco)
  └─→ Jun25 ej2 (productor-consumidor con ficheros)
  └─→ Dic25a ej2 (pthreads en el camión)

P6 (semáforos POSIX + shm + cocinero-salvajes)
  └─→ Dic25b ej2 (cocinero+cajero+salvajes, IDÉNTICO a P6)
  └─→ Dic25a ej2 (control+truck, mismos mecanismos)

P7 (módulos kernel)
  └─→ No ha aparecido directamente en exámenes recientes
```

---

## 🎯 RESUMEN EJECUTIVO PARA EL DÍA DEL EXAMEN

**Ej1 casi siempre:** `fork` + `execvp` + `getopt` + `waitpid`  
**Ej2 casi siempre:** Semáforos POSIX + Memoria compartida (`shm_open`/`mmap`) o pthreads  

Los **3 errores** más comunes que hacen fallar un examen:
1. Olvidar `ftruncate` → SIGBUS
2. No hacer `sem_unlink` al inicio → deadlock por valores residuales  
3. Usar `if` en vez de `while` con `pthread_cond_wait` → race condition
