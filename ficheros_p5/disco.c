#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // Para sleep()
#include <pthread.h> //Libreria para trabajar con hilos

#define CAPACITY 2
#define VIPSTR(vip) ((vip) ? "  vip  " : "not vip")


//Control de concurrencia
pthread_mutex_t mutex;       // El "cerrojo" para la sección crítica
pthread_cond_t condVip;      // Sala de espera para VIPs
pthread_cond_t condNormal;   // Sala de espera para Normales




typedef struct {
    int id;
    int isvip;
	int cont;   // Para que sigan un orden de entrada
} cliente_t;

// Region Critica // --- VARIABLES GLOBALES (RECURSOS COMPARTIDOS) ---
int aforo_actual;            // Personas dentro de la disco actualmente
int colaVip;                 // Cuántos VIPs están esperando fuera

int contV;
int contN;

void enter_normal_client(int id, int cont)
{
	pthread_mutex_lock(&mutex);
	printf("Cliente NORMAL id: %d, quiere entrar a la disco, es el %d de los normales, aforo actual:%d, con colaVIP:%d\n",id, cont ,aforo_actual, colaVip);  
	while(aforo_actual >= CAPACITY || colaVip > 0 || contN != cont){
		pthread_cond_wait(&condNormal, &mutex);
	}
	aforo_actual++;
	contN++;
	printf("Cliente NORMAL id: %d, entrando a la disco, es el %d de los normales aforo actual:%d, con colaVIP:%d\n",id, cont ,aforo_actual, colaVip);  

	pthread_mutex_unlock(&mutex);
}

void enter_vip_client(int id, int cont)
{
	pthread_mutex_lock(&mutex);
	colaVip++;
	printf("Cliente VIP id: %d, quiere entrar a la disco, es el %d de los VIP, aforo actual:%d, con colaVIP:%d\n",id, cont ,aforo_actual, colaVip);  
	while(aforo_actual >= CAPACITY || contV != cont){
		pthread_cond_wait(&condVip, &mutex);
	}

	aforo_actual++;
	contV++;
	colaVip--;
	pthread_mutex_unlock(&mutex);
}

void dance(int id, int isvip)
{
	printf("Client %2d (%s) dancing in disco\n", id, VIPSTR(isvip));
	sleep((rand() % 3) + 1);
}

void disco_exit(int id, int isvip)
{
	pthread_mutex_lock(&mutex);
	printf("Client %2d (%s) saliendo de la disco\n", id, VIPSTR(isvip));
	aforo_actual--;
	if(colaVip > 0){
		pthread_cond_signal(&condVip);
	}
	else {
		// Si no hay VIPs, despertamos a todos para que revisen condiciones.
        // Broadcast es más seguro aquí para evitar bloqueos si hay mezcla de hilos
		pthread_cond_broadcast(&condVip);
		pthread_cond_broadcast(&condNormal);
	}



	pthread_mutex_unlock(&mutex);

}

void *client(void *arg)
{
	cliente_t *cliente = (cliente_t *) arg;


	int id = cliente->id;
	int isVip = cliente->isvip;
	int cont = cliente->cont;

	// IMPORTANTE: Liberar la memoria de la estructura aquí o en el main (con join).
    // En este código se hace free en el main después del join, lo cual es arriesgado 
    // si usas la variable 'cliente' aquí. Como ya copiaste los datos a variables locales,
    // es seguro.
	printf("Cliente registrado con id: %d, isvip: %d, cont: %d\n", cliente->id, cliente->isvip, cliente->cont);  
	if(isVip){
		enter_vip_client(id, cont);
	}
	else enter_normal_client(id, cont);

	dance(id, isVip);
	disco_exit(id, isVip);
	free(cliente);
	return NULL;
}

int main(int argc, char *argv[])
{
	if(argc < 2){
		fprintf(stderr, "Error falta el archvio de antrada\n");
		return EXIT_FAILURE;
	}

	FILE *f = fopen(argv[1], "r");
	if(f == NULL){
		fprintf(stderr, "Error al abrir el archivo: %s\n", argv[1]);
		return EXIT_FAILURE;
	}

	int m = 0; // Numero de clientes totales
	char c;
	if(fscanf(f, "%d", &m) != 1 || m <= 0){
		fprintf(stderr, "Error al escanear M\n");
		fclose(f);
		return EXIT_FAILURE;
	}

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&condVip, NULL);
	pthread_cond_init(&condNormal, NULL);

	pthread_t *hilos = malloc(sizeof(cliente_t) * m);

	aforo_actual= 0;
	colaVip = 0;


	int contNormal = 0;
	int contVip = 0; 
	contN = 0;
	contV = 0; 


	for(int i = 0; i < m; i++){
		pthread_mutex_lock(&mutex); //bloqueo para poner el contador bien
		cliente_t *cliente = malloc(sizeof(cliente_t)); 
		cliente->id = i + 1;
		if(fscanf(f, "%d", &cliente->isvip)!= 1){
			fprintf(stderr, "Error al escanear\n");
			return EXIT_FAILURE;
		}

		if(cliente->isvip){
			cliente->cont = contVip;
			contVip++;
		}
		else {
			cliente->cont = contNormal;
			contNormal++;
		}

		pthread_mutex_unlock(&mutex);
		
		if(pthread_create(&hilos[i], NULL, client, cliente ) != 0){
			fprintf(stderr, "Error pthread create");
			free(cliente);
			fclose(f);
			return EXIT_FAILURE;
		}
	}

	fclose(f);
	for(int i = 0; i < m; i++){
		pthread_join(hilos[i], NULL);
	}


	free(hilos);
	pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condVip);
	pthread_cond_destroy(&condNormal);
    
    printf("Todos los clientes han salido. Programa terminando.\n");
    return 0;
}

/*
 * LÓGICA DEL PROGRAMA (SIMULACIÓN DE DISCOTECA):
 *
 * Objetivo: Gestionar el acceso concurrente a un recurso limitado (aforo).
 * Reglas de Sincronización implementadas:
 * 1. Aforo limitado: Máximo 3 personas dentro (CAPACITY).
 * 2. Prioridad VIP: Si hay VIPs esperando, los normales no pueden entrar
 * aunque haya sitio.
 * 3. Orden estricto (FIFO): Los hilos deben entrar en orden numérico secuencial
 * dentro de su categoría (VIP o Normal), controlado por 'contV' y 'contN'.
 *
 * Herramientas:
 * - Mutex: Protege las variables globales (aforo, contadores).
 * - Variables de Condición:
 * > condVip: Cola de espera para clientes VIP.
 * > condNormal: Cola de espera para clientes normales.
 */
/*
Ejecucion : gcc -Wall -pthread -o discoteca tu_archivo.c 


*/



/*
 * ============================================================================
 * FSCANF - Función para leer datos FORMATEADOS desde un ARCHIVO
 * ============================================================================
 * 
 * Sintaxis:
 *   int fscanf(FILE *stream, const char *format, ...);
 * 
 * Parámetros:
 *   - FILE *stream: Puntero al archivo abierto con fopen()
 *   - const char *format: Cadena de formato (como en scanf)
 *   - ...: Direcciones de variables donde guardar los datos
 * 
 * Valor de retorno:
 *   - Número > 0: Cantidad de elementos leídos correctamente
 *   - 0: No se leyó nada
 *   - EOF (-1): Error o fin de archivo
 * 
 * DIFERENCIA CON SCANF:
 *   - scanf() lee del TECLADO (stdin)
 *   - fscanf() lee de un ARCHIVO
 * 
 * ============================================================================
 * PTHREAD_MUTEX_INIT - Inicializar un MUTEX (exclusión mutua)
 * ============================================================================
 * 
 * Sintaxis:
 *   int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
 * 
 * Parámetros:
 *   - pthread_mutex_t *mutex: Puntero al mutex a inicializar
 *   - const pthread_mutexattr_t *attr: Atributos del mutex (NULL = valores por defecto)
 * 
 * Valor de retorno:
 *   - 0: Éxito
 *   - Valor != 0: Código de error
 * 
 * ¿QUÉ ES UN MUTEX?
 *   Un mutex (mutual exclusion) es un mecanismo de sincronización que permite
 *   que solo UN hilo acceda a una sección crítica (código que modifica recursos
 *   compartidos) a la vez. Previene condiciones de carrera (race conditions).
 * 
 * USO TÍPICO:
 *   pthread_mutex_t mutex;
 *   pthread_mutex_init(&mutex, NULL);
 * 
 * IMPORTANTE:
 *   - Debe llamarse ANTES de usar el mutex
 *   - Cada mutex inicializado debe destruirse con pthread_mutex_destroy()
 * 
 * ============================================================================
 * PTHREAD_MUTEX_LOCK - Bloquear/Adquirir un MUTEX
 * ============================================================================
 * 
 * Sintaxis:
 *   int pthread_mutex_lock(pthread_mutex_t *mutex);
 * 
 * Parámetros:
 *   - pthread_mutex_t *mutex: Puntero al mutex a bloquear
 * 
 * Valor de retorno:
 *   - 0: Éxito (mutex adquirido)
 *   - Valor != 0: Código de error
 * 
 * COMPORTAMIENTO:
 *   - Si el mutex está LIBRE: El hilo lo adquiere inmediatamente y continúa
 *   - Si el mutex está OCUPADO: El hilo se BLOQUEA esperando hasta que esté libre
 * 
 * SECCIÓN CRÍTICA:
 *   Todo el código entre pthread_mutex_lock() y pthread_mutex_unlock() es una
 *   sección crítica donde solo puede estar UN hilo a la vez.
 * 
 * IMPORTANTE:
 *   - Siempre debe ir seguido de pthread_mutex_unlock()
 *   - Si olvidas hacer unlock, otros hilos quedarán bloqueados (DEADLOCK)
 * 
 * ============================================================================
 * PTHREAD_MUTEX_UNLOCK - Desbloquear/Liberar un MUTEX
 * ============================================================================
 * 
 * Sintaxis:
 *   int pthread_mutex_unlock(pthread_mutex_t *mutex);
 * 
 * Parámetros:
 *   - pthread_mutex_t *mutex: Puntero al mutex a desbloquear
 * 
 * Valor de retorno:
 *   - 0: Éxito (mutex liberado)
 *   - Valor != 0: Código de error
 * 
 * COMPORTAMIENTO:
 *   - Libera el mutex para que otros hilos puedan adquirirlo
 *   - Si hay hilos esperando, uno de ellos será despertado
 * 
 * REGLA DE ORO:
 *   Por cada pthread_mutex_lock() debe haber EXACTAMENTE un pthread_mutex_unlock()
 * 
 * IMPORTANTE:
 *   - Solo el hilo que hizo lock puede hacer unlock
 *   - Hacer unlock de un mutex que no posees es un error
 * 
 * ============================================================================
 * PTHREAD_MUTEX_DESTROY - Destruir un MUTEX
 * ============================================================================
 * 
 * Sintaxis:
 *   int pthread_mutex_destroy(pthread_mutex_t *mutex);
 * 
 * Parámetros:
 *   - pthread_mutex_t *mutex: Puntero al mutex a destruir
 * 
 * Valor de retorno:
 *   - 0: Éxito
 *   - EBUSY: El mutex está bloqueado o siendo usado
 *   - Valor != 0: Código de error
 * 
 * CUÁNDO USAR:
 *   - Al final del programa, después de que todos los hilos hayan terminado
 *   - Libera recursos asociados al mutex
 * 
 * IMPORTANTE:
 *   - El mutex debe estar desbloqueado (unlocked) antes de destruirlo
 *   - No destruir un mutex que está siendo usado causa comportamiento indefinido
 *   - Por cada pthread_mutex_init() debe haber un pthread_mutex_destroy()
 * 
 * ============================================================================
 * PTHREAD_COND_INIT - Inicializar una VARIABLE DE CONDICIÓN
 * ============================================================================
 * 
 * Sintaxis:
 *   int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
 * 
 * Parámetros:
 *   - pthread_cond_t *cond: Puntero a la variable de condición a inicializar
 *   - const pthread_condattr_t *attr: Atributos (NULL = valores por defecto)
 * 
 * Valor de retorno:
 *   - 0: Éxito
 *   - Valor != 0: Código de error
 * 
 * ¿QUÉ ES UNA VARIABLE DE CONDICIÓN?
 *   Mecanismo de sincronización que permite a los hilos ESPERAR hasta que
 *   se cumpla una CONDICIÓN específica. Evita busy-waiting (espera activa).
 * 
 * USO TÍPICO:
 *   pthread_cond_t cond;
 *   pthread_cond_init(&cond, NULL);
 * 
 * IMPORTANTE:
 *   - SIEMPRE se usa junto con un mutex
 *   - Debe destruirse con pthread_cond_destroy() al terminar
 * 
 * ============================================================================
 * PTHREAD_COND_WAIT - Esperar en una VARIABLE DE CONDICIÓN
 * ============================================================================
 * 
 * Sintaxis:
 *   int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
 * 
 * Parámetros:
 *   - pthread_cond_t *cond: Variable de condición en la que esperar
 *   - pthread_mutex_t *mutex: Mutex asociado (debe estar BLOQUEADO)
 * 
 * Valor de retorno:
 *   - 0: Éxito (condición señalada)
 *   - Valor != 0: Código de error
 * 
 * COMPORTAMIENTO (3 PASOS ATÓMICOS):
 *   1. Libera el mutex automáticamente
 *   2. Bloquea el hilo hasta recibir señal (signal/broadcast)
 *   3. Re-adquiere el mutex antes de retornar
 * 
 * PATRÓN DE USO CORRECTO:
 *   pthread_mutex_lock(&mutex);
 *   while (condicion_no_cumplida) {    // ¡Usar WHILE, no IF!
 *       pthread_cond_wait(&cond, &mutex);
 *   }
 *   // ... hacer trabajo ...
 *   pthread_mutex_unlock(&mutex);
 * 
 * ¿POR QUÉ WHILE Y NO IF?
 *   - Puede haber "despertares espurios" (spurious wakeups)
 *   - Otro hilo puede cambiar la condición antes de que este se ejecute
 * 
 * IMPORTANTE:
 *   - El mutex DEBE estar bloqueado antes de llamar a pthread_cond_wait()
 *   - pthread_cond_wait() lo desbloquea mientras espera
 *   - Cuando retorna, el mutex vuelve a estar bloqueado
 * 
 * ============================================================================
 * PTHREAD_COND_SIGNAL - Despertar UN hilo esperando en una condición
 * ============================================================================
 * 
 * Sintaxis:
 *   int pthread_cond_signal(pthread_cond_t *cond);
 * 
 * Parámetros:
 *   - pthread_cond_t *cond: Variable de condición a señalar
 * 
 * Valor de retorno:
 *   - 0: Éxito
 *   - Valor != 0: Código de error
 * 
 * COMPORTAMIENTO:
 *   - Despierta AL MENOS UN hilo que esté esperando en pthread_cond_wait()
 *   - Si NO hay hilos esperando, la señal se pierde (no se guarda)
 *   - Si hay varios hilos esperando, se despierta UNO (no específico)
 * 
 * USO TÍPICO:
 *   pthread_mutex_lock(&mutex);
 *   // ... modificar estado compartido ...
 *   pthread_cond_signal(&cond);
 *   pthread_mutex_unlock(&mutex);
 * 
 * DIFERENCIA CON BROADCAST:
 *   - signal: Despierta UN hilo
 *   - broadcast: Despierta TODOS los hilos
 * 
 * CUÁNDO USAR:
 *   - Cuando solo UN hilo puede proceder (ej: un recurso disponible)
 *   - Más eficiente que broadcast si solo uno puede continuar
 * 
 * ============================================================================
 * PTHREAD_COND_BROADCAST - Despertar TODOS los hilos esperando
 * ============================================================================
 * 
 * Sintaxis:
 *   int pthread_cond_broadcast(pthread_cond_t *cond);
 * 
 * Parámetros:
 *   - pthread_cond_t *cond: Variable de condición a señalar
 * 
 * Valor de retorno:
 *   - 0: Éxito
 *   - Valor != 0: Código de error
 * 
 * COMPORTAMIENTO:
 *   - Despierta a TODOS los hilos que están esperando en pthread_cond_wait()
 *   - Todos compiten por el mutex al despertar
 *   - Solo uno conseguirá el mutex primero, los demás esperan
 * 
 * USO TÍPICO:
 *   pthread_mutex_lock(&mutex);
 *   // ... modificar estado compartido ...
 *   pthread_cond_broadcast(&cond);
 *   pthread_mutex_unlock(&mutex);
 * 
 * CUÁNDO USAR:
 *   - Cuando VARIOS hilos pueden proceder
 *   - Cuando no sabes cuál debe despertar
 *   - Al finalizar programa (despertar a todos para que terminen)
 * 
 * IMPORTANTE:
 *   - Más costoso que signal (despierta a todos)
 *   - Los hilos despertados re-verificarán su condición en el while
 * 
 * ============================================================================
 * PTHREAD_COND_DESTROY - Destruir una VARIABLE DE CONDICIÓN
 * ============================================================================
 * 
 * Sintaxis:
 *   int pthread_cond_destroy(pthread_cond_t *cond);
 * 
 * Parámetros:
 *   - pthread_cond_t *cond: Puntero a la variable de condición a destruir
 * 
 * Valor de retorno:
 *   - 0: Éxito
 *   - EBUSY: Hay hilos esperando en esta variable de condición
 *   - Valor != 0: Código de error
 * 
 * CUÁNDO USAR:
 *   - Al final del programa, después de que todos los hilos hayan terminado
 *   - Libera recursos asociados a la variable de condición
 * 
 * IMPORTANTE:
 *   - No debe haber hilos esperando en pthread_cond_wait()
 *   - Por cada pthread_cond_init() debe haber un pthread_cond_destroy()
 *   - Destruir una variable de condición en uso causa comportamiento indefinido
 * 
 * ============================================================================
 * PTHREAD_CREATE - Crear un NUEVO HILO
 * ============================================================================
 * 
 * Sintaxis:
 *   int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
 *                      void *(*start_routine)(void *), void *arg);
 * 
 * Parámetros:
 *   - pthread_t *thread: Puntero donde guardar el ID del hilo creado
 *   - const pthread_attr_t *attr: Atributos del hilo (NULL = por defecto)
 *   - void *(*start_routine)(void *): Función que ejecutará el hilo
 *   - void *arg: Argumento que se pasa a start_routine
 * 
 * Valor de retorno:
 *   - 0: Éxito (hilo creado)
 *   - Valor != 0: Código de error (EAGAIN, EINVAL, EPERM, etc.)
 * 
 * COMPORTAMIENTO:
 *   - Crea un nuevo hilo de ejecución
 *   - El hilo empieza ejecutando start_routine(arg)
 *   - El hilo padre continúa su ejecución inmediatamente (concurrente)
 * 
 * LA FUNCIÓN start_routine:
 *   - Debe tener firma: void *nombre_funcion(void *arg)
 *   - Puede retornar un valor con return (se recupera con pthread_join)
 *   - Cuando retorna, el hilo termina
 * 
 * IMPORTANTE:
 *   - El argumento 'arg' se pasa por REFERENCIA (cuidado con variables locales)
 *   - Usa malloc() para datos que pasas al hilo si son locales
 *   - Cada hilo creado debe ser esperado con pthread_join() o detached
 * 
 * ============================================================================
 * PTHREAD_JOIN - Esperar a que un HILO TERMINE
 * ============================================================================
 * 
 * Sintaxis:
 *   int pthread_join(pthread_t thread, void **retval);
 * 
 * Parámetros:
 *   - pthread_t thread: ID del hilo a esperar (obtenido de pthread_create)
 *   - void **retval: Puntero donde guardar el valor de retorno del hilo
 *                    (NULL si no interesa el valor)
 * 
 * Valor de retorno:
 *   - 0: Éxito (hilo terminado)
 *   - Valor != 0: Código de error (EDEADLK, EINVAL, ESRCH, etc.)
 * 
 * COMPORTAMIENTO:
 *   - BLOQUEA el hilo que llama hasta que el hilo especificado termine
 *   - Recupera el valor de retorno del hilo (si se especifica retval)
 *   - Libera los recursos asociados al hilo terminado
 * 
 * ANALOGÍA:
 *   Es como wait() para procesos, pero para hilos
 * 
 * USO TÍPICO:
 *   pthread_t hilo;
 *   pthread_create(&hilo, NULL, funcion, arg);
 *   // ... hacer otras cosas ...
 *   pthread_join(hilo, NULL);  // Esperar a que termine
 * 
 * IMPORTANTE:
 *   - Solo se puede hacer join UNA VEZ por hilo
 *   - Si no haces join, el hilo queda como "zombie" (recursos no liberados)
 *   - No puedes hacer join de un hilo "detached"
 *   - Hacer join del mismo hilo dos veces causa comportamiento indefinido
 * 
 * ============================================================================
 * FOPEN - Abrir un ARCHIVO
 * ============================================================================
 * 
 * Sintaxis:
 *   FILE *fopen(const char *filename, const char *mode);
 * 
 * Parámetros:
 *   - const char *filename: Ruta del archivo a abrir
 *   - const char *mode: Modo de apertura
 *       "r"  = lectura (el archivo debe existir)
 *       "w"  = escritura (crea o sobreescribe)
 *       "a"  = append (añadir al final)
 *       "r+" = lectura y escritura (debe existir)
 *       "w+" = lectura y escritura (crea o sobreescribe)
 *       "a+" = lectura y append
 * 
 * Valor de retorno:
 *   - Puntero FILE* válido: Éxito
 *   - NULL: Error al abrir (archivo no existe, sin permisos, etc.)
 * 
 * USO TÍPICO:
 *   FILE *fd = fopen("datos.txt", "r");
 *   if (fd == NULL) {
 *       perror("Error al abrir");
 *       exit(EXIT_FAILURE);
 *   }
 * 
 * IMPORTANTE:
 *   - Siempre verificar que el retorno NO sea NULL
 *   - Cerrar el archivo con fclose() cuando termines
 *   - Usar perror() para ver el error específico
 * 
 * ============================================================================
 * FCLOSE - Cerrar un ARCHIVO
 * ============================================================================
 * 
 * Sintaxis:
 *   int fclose(FILE *stream);
 * 
 * Parámetros:
 *   - FILE *stream: Puntero al archivo a cerrar (obtenido con fopen)
 * 
 * Valor de retorno:
 *   - 0: Éxito
 *   - EOF: Error al cerrar
 * 
 * COMPORTAMIENTO:
 *   - Escribe cualquier dato pendiente (flush)
 *   - Libera recursos asociados al archivo
 *   - El puntero FILE* queda inválido
 * 
 * IMPORTANTE:
 *   - Por cada fopen() debe haber un fclose()
 *   - No usar el puntero después de fclose()
 *   - Cerrar archivos al terminar libera recursos del sistema
 * 
 * ============================================================================
 * MALLOC - Asignar MEMORIA DINÁMICA
 * ============================================================================
 * 
 * Sintaxis:
 *   void *malloc(size_t size);
 * 
 * Parámetros:
 *   - size_t size: Número de bytes a asignar
 * 
 * Valor de retorno:
 *   - Puntero void* a la memoria asignada: Éxito
 *   - NULL: No hay memoria suficiente
 * 
 * COMPORTAMIENTO:
 *   - Reserva 'size' bytes en el HEAP
 *   - La memoria NO está inicializada (contiene basura)
 *   - La memoria persiste hasta que se libere con free()
 * 
 * USO TÍPICO:
 *   int *array = malloc(10 * sizeof(int));
 *   if (array == NULL) {
 *       perror("Error en malloc");
 *       exit(EXIT_FAILURE);
 *   }
 * 
 * IMPORTANTE:
 *   - Siempre verificar que NO retorne NULL
 *   - Usar sizeof() para calcular el tamaño correcto
 *   - Liberar con free() cuando ya no se necesite
 *   - No liberar causa memory leak (fuga de memoria)
 * 
 * ============================================================================
 * FREE - Liberar MEMORIA DINÁMICA
 * ============================================================================
 * 
 * Sintaxis:
 *   void free(void *ptr);
 * 
 * Parámetros:
 *   - void *ptr: Puntero a memoria previamente asignada con malloc/calloc/realloc
 * 
 * Valor de retorno:
 *   - Ninguno (void)
 * 
 * COMPORTAMIENTO:
 *   - Libera la memoria apuntada por ptr
 *   - El puntero queda inválido (dangling pointer)
 * 
 * IMPORTANTE:
 *   - Por cada malloc() debe haber un free()
 *   - Hacer free(NULL) es seguro (no hace nada)
 *   - Hacer free() dos veces del mismo puntero es ERROR GRAVE (double free)
 *   - Usar un puntero después de free() es ERROR (use after free)
 * 
 * BUENA PRÁCTICA:
 *   free(ptr);
 *   ptr = NULL;  // Evita usar el puntero por error
 * 
 */