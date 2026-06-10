#include <stdlib.h>     // malloc, free, EXIT_*
#include <stdio.h>      // printf, perror
#include <pthread.h>    // pthread_t, pthread_create, pthread_join, pthread_self


/* Estructura que se pasará como argumento a cada hilo:
 *  - num: número de hilo (0, 1, 2, ...)
 *  - P  : prioridad del hilo ('P' prioritario, 'N' no prioritario)
 */
typedef struct {
	int num;
	char P;
}datosHilos;

/* Función que ejecutará cada hilo (start_routine para pthread_create)
 * Recibe un void* que en realidad es un puntero a datosHilos, ya que como no se sabe que datos se pasan es un puntero sin etiqueta 
 */
void *thread_usuario(void *arg)
{
	datosHilos *datos = (datosHilos *) arg;  // hacemos el cast para darle la etiqueta del tipo al puntero 
	int num = datos->num;
	char p = datos->P;
    // Obtenemos el identificador del hilo actual
    pthread_t Id = pthread_self();
    // Liberamos la memoria dinámica reservada en main para este hilo
    free(datos);

    printf("Soy el hilo %ld, con numero %d y con preferencia %c\n", Id, num, p);
}
	


int main(int argc, char* argv[])
{	
	pthread_t hilos[5];

	for(int i = 0; i < 5; i++){
		// Reservar memoria dinámica para los argumentos del hilo i
        datosHilos *datosH = malloc(sizeof(datosHilos));  // Llamamos a malloc cada vex ya que cada hilo tiene que tener su propia direccion de memoria en el heap
		if(datosH == NULL){
			perror("Error con malloc");
			return EXIT_FAILURE;
		}
		datosH->num = i;
		datosH->P = (i %2 == 0) ? 'P' : 'N';

		if(pthread_create(&hilos[i], NULL, thread_usuario, datosH) != 0){
			fprintf(stderr, "Error al crear el hilo %d", i);
			free(datosH);
			return EXIT_FAILURE;
		}
	}

	for(int i = 0; i < 5; i++){
		pthread_join(hilos[i], NULL);

	}

	return 0;
}

/*
 * FUNCIONES CLAVE EXPLICADAS:
 *
 * pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
 * - Crea un nuevo hilo de ejecución dentro del mismo proceso.
 * - Parámetros:
 * thread: puntero donde se guarda el identificador del hilo creado.
 * attr: atributos del hilo (normalmente NULL para valores por defecto).
 * start_routine: función que ejecutará el hilo. Debe tener la forma:
 * 
 * void *func(void *arg).
 * arg: puntero pasado como argumento inicial a la función (puede ser NULL).
 * - Devuelve:
 * 0 : si la creación del hilo tiene éxito.
 * != 0 : indica error.
 *
 * pthread_join(pthread_t thread, void **retval)
 * - Bloquea el hilo que llama hasta que termina el hilo identificado por 'thread'.
 * - Parámetros:
 * thread: identificador de hilo devuelto previamente por pthread_create.
 * retval: puntero para recoger el valor devuelto por la función del hilo.
 * > Si no es NULL: contendrá el puntero retornado por start_routine.
 * > Si es NULL: se ignora el resultado.
 * - Devuelve:
 * 0 : si la espera se realiza correctamente.
 * != 0 : indica error.
 * - Es la forma estándar de sincronizar la finalización de hilos y liberar sus recursos.
 *
 * pthread_self(void)
 * - Devuelve el identificador (pthread_t) del hilo que realiza la llamada.
 * - No recibe argumentos.
 * - Se usa típicamente dentro de la función de hilo para:
 * - Identificar el hilo actual en mensajes de depuración o logs.
 * - Comparar si un determinado pthread_t corresponde al hilo que ejecuta el código.
 *
 * malloc(size_t size) / free(void *ptr)
 * - malloc reserva 'size' bytes de memoria dinámica y devuelve un puntero.
 * - free libera memoria previamente reservada, evitando fugas (memory leaks).
 * - En este ejercicio:
 * - Se usa malloc para reservar una estructura de argumentos (datosHilos)
 * distinta para cada hilo antes de crearlo.
 * - Se usa free dentro de la función del hilo, una vez copiados los datos
 * a variables locales, para liberar esa memoria asignada.
 */
/*
 * =========================================================================
 * RESUMEN GLOBAL: ARQUITECTURA Y MECÁNICA DE HILOS EN C (POSIX THREADS)
 * =========================================================================
 *
 * 1. MODELO DE MEMORIA COMPARTIDA (HEAP vs STACK)
 * -------------------------------------------------------------------------
 * - A diferencia de fork() (que clona el proceso y duplica toda la memoria),
 * los hilos viven dentro del mismo proceso padre.
 * - Todos los hilos COMPARTEN el mismo espacio de variables globales,
 * descriptores de ficheros y la memoria dinámica (el HEAP donde trabaja malloc).
 * - Sin embargo, cada hilo tiene su propia pila local (STACK) privada para
 * sus variables internas de función.
 * - Por ello, para pasar datos entre el main y los hilos sin que se destruyan,
 * es obligatorio comunicarse mediante direcciones de memoria (punteros).
 *
 * 2. ¿POR QUÉ EN 'pthread_create' SE PASA '&hilos[i]' CON AMPERSAND?
 * -------------------------------------------------------------------------
 * - En C, si pasas una variable a una función a secas (ej: hilos[i]), la función
 * recibe una copia. Cualquier cambio que haga se pierde al salir.
 * - pthread_create tiene la misión de registrar el hilo en el Kernel de Linux
 * y devolverle al main el número de ID asignado a ese hilo.
 * - Al anteponer el ampersand '&', le estamos pasando la DIRECCIÓN DE MEMORIA
 * real y exacta de la casilla de nuestro array. Así, pthread_create puede
 * escribir el ID directamente en la memoria de nuestro main.
 *
 * 3. EL MISTERIO DEL PUNTERO UNIVERSAL 'void *'
 * -------------------------------------------------------------------------
 * - La librería pthread es genérica: los diseñadores no sabían si tú ibas a querer
 * pasarle a un hilo un entero, un float, un array o un struct personalizado.
 * - Como en C no existe la programación genérica pura, se usa 'void *'.
 * - Un 'void *' es un puntero comodín: representa una dirección de memoria pura,
 * una "flecha" que apunta a la RAM pero sin ninguna "etiqueta" de tipo de datos.
 * - Protocolo de uso:
 * a) En el main, transformamos nuestro struct al comodín: (void *)datosH.
 * b) Dentro del hilo, lo primero que hacemos es "devolverle la etiqueta" real
 * haciendo un casteo para poder usar sus campos: (datosHilos *) arg.
 *
 * 4. LA OBLIGACIÓN ABSOLUTA DE USAR 'malloc()' DENTRO DE LOS BUCLES 'for'
 * -------------------------------------------------------------------------
 * - Si en el bucle 'for' creáramos una variable local normal en el Stack y
 * pasáramos su dirección (&variable), todos los hilos recibirían LA MISMA
 * dirección de memoria.
 * - Como el hilo main se ejecuta en la CPU a la velocidad de la luz, el bucle
 * cambiaría el valor de la variable (de 0 a 1, luego a 2...) antes de que
 * los hilos secundarios tuvieran tiempo de despertar. Al final, todos los hilos
 * mirarían la misma dirección y leerían el mismo número corrupto (el último).
 * - Solución con 'malloc()': En cada vuelta del bucle for, le pedimos al Sistema
 * Operativo una "caja de memoria exclusiva e independiente" en el HEAP. 
 * - El Hilo 0 recibe una flecha a la Caja A, el Hilo 1 a la Caja B, etc. Ya no se
 * pisan los datos entre ellos.
 * - Responsabilidad: Como esa caja está en el Heap compartido, una vez que el
 * hilo secundario ha copiado los datos a sus variables locales, debe destruirla
 * haciendo 'free(datos)' para evitar fugas de memoria (Memory Leaks).
 */