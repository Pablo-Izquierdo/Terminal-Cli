
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <stdbool.h>
#include <iostream>
#include <sys/time.h>
#include <mutex>
#include <condition_variable>
#include <type_traits>
#include <ctype.h>
#include <atomic>


#define DEBUG
#define FEATURE_LOGGER
#define FEATURE_OPTIMIZE
//#ifdef DEBUG

//#endif


typedef enum {SUM, SUB, XOR} operation;
typedef enum {single, multi} mode;


// Structure containing the configuration of the program
struct configuration_data {
  int longArray;    // length
  operation op; // operation type
  int numthreads = 0;   // number of threads
  mode modo;
};

#ifdef FEATURE_LOGGER
// Structure containing the data information of the program optimized
struct data_information {
    double *datos;
    std::atomic<double> resultado;
    int longFragmento;
    int inicioResto;
};
#else
// Structure containing the data information of the program
struct data_information {
    double *datos;
    double resultado;
    int longFragmento;
    int inicioResto;
};
#endif

//Declaración de funciones
mode determinaMode (int numThreads);
void ejecucionThread (int i, struct data_information* data, int inicio, operation op, int longArray);
long timer ();
void ejecucionLogger(struct configuration_data* config, std::condition_variable &variableMain, bool* loggerTerminado);
void loggerThread(double resultadoFragmento, int id);
int readPipe(double buffer[], double dato[], int buffersize);
int writePipe(double buffer[], int id, double dato);

std::mutex mutexOperacion;
#ifdef FEATURE_LOGGER
    std::condition_variable variableRespuestaLogger;
    std::condition_variable variableLogger;
    bool datoListo = false;
    double pipeThread[2] = {0, -100};
#endif

int main (int argc, char *argv[])
{
    //Orden de los argumentos ( nombrefichero, longArray , Operacion, mumThreads )
    // ( nombrefichero, longArray , Operacion, --multithread, numthreads )

    //INICIALIZO VARIABLES
    struct configuration_data config;
    struct data_information data;
    long Tinicial;
    long Tfinal;

    //---------------------------------------PROCEDAR ARGUMENTOS------------------------------------------------------

    if(strcmp(argv[1],"--help") == 0){
        printf("\nModo de empleo: cli [OPTIONS] numDouble operacion\nOpera los n primeros numeros, desde 0 hasta (numDouble - 1)\nOperaciones admitidas: sum, sub, xor\n[Options]\n     --multi-thread  Realiza las operaciones de forma paralela con los threads\n                     que le introduzcas [Maximo threads = 12].\n                     Esta opcion no es obligatoria, en caso de no ponerla o\n                     ponerla igual a 0 el calculo lo realizara el main.\n\n");
        return 0;
    }

    if(argc < 3){
        printf("introduce los argumentos correctamente\n./cli [Options] numDoubles Operacion\n");
        return -1;
    }

    int posLong = 1;
    int posOp = 2;
    int i;
    for(i = 1; i < argc; i++){
        if(strcmp(argv[i],"--multi-thread") == 0){
            i++;
            if(isdigit(*argv[i])){
                config.numthreads = atoi(argv[i]);
            } else {
                printf("Introduce numero de threads correctamente, --multi-thread numThreads\nPara más información : cli --help\n");
                return -1;
            }

            if(config.numthreads > 12) {printf("Excedido el numero de threads\n"); return -1;}
            if(i == 2){
                posLong = 3; posOp = 4;
                //if(DEBUG){
                //printf("PosLOng = %d, PosOp = %d\n", posLong, posOp);}
                }
            if(i == 3){posOp = 4;}
        }else if(i == posLong ){
            if(isdigit(*argv[i])){
                config.longArray = atoi(argv[i]);
            } else {
                printf("Cantidad de doubles no entontrada. Para más información : cli --help\n");
                return -1;
            }

        }else if (i == posOp) {
            if (strcmp(argv[i], "sum") == 0){
                config.op = SUM;
            }
            else if (strcmp(argv[i], "sub") == 0){
                config.op = SUB;
            }
            else if (strcmp(argv[i], "xor") == 0){
                config.op = XOR;
            }else{
                printf("Operación desconocida"); return -1;
            }

        } else{
            printf("Problema al introducir los parametros. Para más información : cli --help");
        }
    }
    //Determino el modo
    config.modo = determinaMode(config.numthreads);

    #ifdef DEBUG
    printf("\n------PARAMETROS DEL PROGRAMA-------\n");
    printf("argc = %d", argc);
    printf("\n0 = %s", argv[0]);
    printf("\n1 = %s", argv[1]);
    printf("\n2 = %s", argv[2]);
    printf("\n3 = %s", argv[3]);
    printf("\n4 = %s", argv[4]);
    printf("\nlonguitud del array = %d", config.longArray);
    printf("\noperacion seleccionada (sum, sub, xor) = %d", config.op);
    printf("\nnumero de threads = %d", config.numthreads);
    printf("\nModo de ejecución (single, multi) = %d\n\n\n", config.modo);
    #endif


    //--------------------------------SETUP DATA------------------------------------
     #ifdef DEBUG
     printf("\n----------DATOS DEL PROGRAMA----------\n");
     #endif

    data.datos = (double*) malloc((sizeof(double) * config.longArray));

    for(i = 0; i < config.longArray; i++){
        data.datos[i] = (double) i;
        #ifdef DEBUG
            printf("datos[%d] = %f\n", i, data.datos[i]);
        #endif
    }

    //Dividimos el array
    if(config.numthreads == 0){
        data.longFragmento = config.longArray;
    }else {
        data.longFragmento = (int) config.longArray/config.numthreads;
        }
    data.inicioResto = (int) (data.longFragmento * config.numthreads);  //if inicio Resto == longarray -> no hay resto
    data.resultado = 0;

    #ifdef DEBUG
        printf("Longitud de los fragmentos = %d\nInicio del resto = %d\n", data.longFragmento, data.inicioResto);
    #endif

    //-------------------------------PROCESADO --------------------------------------

    #ifdef DEBUG
        printf("\n--------------PROCESADO-------------\n");
    #endif
    #ifdef DEBUG
        printf("Inicio Timer\n");
    #endif

    #ifdef FEATURE_LOGGER
    //condicional para notificar al main
    std::condition_variable variableMain;
    auto loggerTerminado = false;


        std::thread logger = std::thread(ejecucionLogger, &config, std::ref(variableMain), &loggerTerminado);
        #ifdef DEBUG
            std::cout << "Creado logger\n";
        #endif
    #endif

    //Cojo el tiempo inicial
    Tinicial = timer();

    if(config.modo == multi){ // si es modo multi thread
        std::thread threads[config.numthreads];
        for (auto i=0; i<config.numthreads; ++i){ // creo lo threads y les pogo su funcion a ejecutar
            threads[i] = std::thread(ejecucionThread, i, &data, (data.longFragmento * i), config.op, config.longArray);
            #ifdef DEBUG
                std::cout << "Creado thread numero " << i << ", con id = " << threads[i].get_id() << "\n";
            #endif
        }


        for (auto i = 0; i < config.numthreads; ++i){ // hago join de los threads creados para que el main los espere.
            threads[i].join();
        }

        Tfinal = timer(); //Tiempo final de ejecucion de los threads

        //join el logger con el resto
        #ifdef FEATURE_LOGGER
            logger.join();
        #endif

        #ifdef DEBUG
            printf("Finalizo Timer\n");
        #endif



    } else if (config.modo == single){
        #ifdef DEBUG
            printf("Soy el main\n");
        #endif

        ejecucionThread (-1, &data, 0, config.op, config.longArray);

        Tfinal = timer(); //Tiempo final de ejecucion

        //join el logger con el resto
        #ifdef FEATURE_LOGGER
            logger.join();

        #endif
        #ifdef DEBUG
            printf("Finalizo Timer\n");
        #endif
    }

    #ifdef FEATURE_LOGGER
    double bothresult[2]  = {0,0}; // recibo el dato y se escribe en index = 0, y escribo el resultado de la suma de los threads en index = 1
    {//Espero al Logger a que me pase el dato final
        std::unique_lock<std::mutex> lk(mutexOperacion);
        variableRespuestaLogger.wait(lk, [loggerTerminado]{return loggerTerminado == true;});

        if(readPipe(pipeThread, bothresult, 2) == 0){
            #ifdef DEBUG
                printf("Soy el Main, Dato del Logger recibido\n");
            #endif
        } else {
            #ifdef DEBUG
                printf("Soy el Main, el dato del Logger ha fallado\n");
            #endif
        }

    }
    bothresult[1] = data.resultado;
    if(bothresult[0] == bothresult[1]){
        //printf("\nSolucion = %f, Tiempo de hilos computando = %ld microsegundos\n", data.resultado, (Tfinal - Tinicial));
        std::cout <<"Solucion = "<< data.resultado <<", Tiempo de hilos computando = "<< (Tfinal - Tinicial) <<" microsegundos\n";
        return 0;
    } else {
        printf("\n Solucion Erronea Solucion Threads = %f, Solucion Logger = %f\n", bothresult[1], bothresult[0]);
        return 1;
    }

    #endif




    //printf("\nSolucion = %f, Tiempo de hilos computando = %ld microsegundos\n", data.resultado, (Tfinal - Tinicial));
    std::cout <<"Solucion = "<< data.resultado <<", Tiempo de hilos computando = "<< (Tfinal - Tinicial)<<" microsegundos\n";
    return 0;


}

void ejecucionThread (int id ,struct data_information* data, int inicio, operation op, int longArray){



    //Recorro el fragmento del array y realizo la operacion
    int i;
    double resultadoFragmento = data->datos[inicio];
    int longitudFragmento = data->longFragmento;
    bool calculoResto= false;

    //Casos Especiales
    //Si hay que restar entonces pongo el inicial negativo
    if(op == SUB){
        resultadoFragmento = -data->datos[inicio];
    }

    //Si estoy operando el resto entonces la longitud del fragmento cambia
    if(data->inicioResto == inicio){
        longitudFragmento = (-inicio + (longArray)); //Desde el inicio del resto hasta el final del array
    }

    //Opero el Fragmento
    if(inicio != (longArray)){ //si no estoy al final del array, calculo
        for(i = (inicio + 1); i < (inicio + longitudFragmento); i++){
            if(op == SUM){
                resultadoFragmento += data->datos[i];
            }else if(op == SUB){
                resultadoFragmento += -data->datos[i];
            }else if(op == XOR){
                resultadoFragmento = (int) resultadoFragmento xor (int) data->datos[i];
            }
        }
    } else {
        resultadoFragmento = 0;
    }

    #ifdef DEBUG
        if(inicio != (longArray)){
            printf("Soy el thread %d, Inicio en %d hasta %d, Resultado del fragmento = %f\n", id, inicio, (inicio + longitudFragmento - 1), resultadoFragmento);
        } else {
            printf("Soy el thread %d, No hay resto, resto = %f.\n", id, resultadoFragmento);
        }
    #endif

    #ifdef FEATURE_OPTIMIZE
        //En el caso de que lo quiera optimizado
        #ifdef DEBUG
            data->resultado.is_lock_free();
        #endif
        if(data->resultado == 0){
                calculoResto = true;
        }

        if(op == SUM){
            data->resultado = data->resultado + resultadoFragmento;
        }else if(op == SUB){
            data->resultado = data->resultado + resultadoFragmento;
        }else if(op == XOR){
            data->resultado = (int) data->resultado xor (int) resultadoFragmento;
        }

         #ifdef DEBUG
            //printf("Soy el thread %d, Desbloqueo el mutex, resultado global = %f\n", id, data->resultado);
            std::cout <<"Soy el thread "<< id <<", Uso Atómicos Resultado global = "<< data->resultado <<"\n";
        #endif

    #else
        mutexOperacion.lock(); //bloqueo mutex para devolver resultado

        #ifdef DEBUG
            printf("Soy el thread %d, Bloqueo el mutex\n", id);
        #endif
        if(data->resultado == 0){
            calculoResto = true;
        }

        if(op == SUM){
            data->resultado += resultadoFragmento;
        }else if(op == SUB){
            data->resultado += resultadoFragmento;
        }else if(op == XOR){
            data->resultado = (int) data->resultado xor (int) resultadoFragmento;
        }


        mutexOperacion.unlock();

        #ifdef DEBUG
            //printf("Soy el thread %d, Desbloqueo el mutex, resultado global = %f\n", id, data->resultado);
            std::cout <<"Soy el thread "<< id <<", Desbloqueo el mutex, resultado global = "<< data->resultado <<"\n";
        #endif
    #endif

    #ifdef FEATURE_LOGGER
        loggerThread(resultadoFragmento, id);
    #endif

    //-----------------------------------------------RESTO--------------------------------------------------------------------------

    if(calculoResto){
        if(data->inicioResto != longArray && data->inicioResto != 0){ //Si hay resto lo calculo
            #ifdef DEBUG
                printf("Soy el thread %d, Calculo el resto\n", id);
            #endif
            ejecucionThread(id, data, data->inicioResto, op , longArray);

        }else if (id != -1){ // en el caso de que no hay resto (y no soy el main (si no la pipe se satura)) le digo al logger que el resultado del resto es 0
            #ifdef FEATURE_LOGGER
                loggerThread(0, id);
            #endif
        }


    }


    #ifdef DEBUG
        printf("Soy el thread %d, HE TERMINADO\n", id);
    #endif


// al intentar hacer thread.terminate() me da un extepcion aborted. Esto creo que es porque como estan join con la main si hago terminate peta
}


mode determinaMode (int numThreads) {
    if(numThreads == 0){
        return single;
    }else {
        return multi;
    }
}


long timer (){
    struct timeval tp;
    gettimeofday(&tp, NULL);
    auto tiempoUs = tp.tv_sec * 1e6 + tp.tv_usec;
    return tiempoUs;
}

#ifdef FEATURE_LOGGER
void loggerThread(double resultadoFragmento, int id){

    //El mutex estará bloqueado durante todo el ambito de las llaves.
    //Con la llave de cierre el mutex se desbloquea
    {
        std::unique_lock<std::mutex> ulk(mutexOperacion);
        #ifdef DEBUG
            printf("Soy el thread %d, Bloqueo el mutexLogger\n", id);
        #endif


        //Espero al que el logger me confirme que esta listo, Si no hago esto
        //puede ocurrir que algun hilo se me cuele
        #ifdef DEBUG
            printf("Soy el thread %d, Espero al Logger \n", id);
        #endif

        variableRespuestaLogger.wait(ulk, []{return datoListo == false;});


        if(writePipe(pipeThread, id, resultadoFragmento) == 0){
            #ifdef DEBUG
                //printf("Soy el thread %d, Envio resultado al logger, buffer = %f, %f\n", id, pipeThread[0], pipeThread[1]);
                printf("Soy el thread %d, Envio resultado al logger\n", id);
            #endif
            datoListo = true;
        } else {
            #ifdef DEBUG
                printf("Soy el thread %d, Ha ocurrido un error el la pipe\n", id);
            #endif
        }
        //mando signa y desbloqueo mutex

    variableLogger.notify_one();
    }


    #ifdef DEBUG
            printf("Soy el thread %d, Suelto el mutexLogger\n", id);
    #endif



}

void ejecucionLogger(struct configuration_data* config, std::condition_variable &variableMain, bool* loggerTerminado){

    double resultados[config->numthreads + 1] = {0};
    int i;
    for(i = 0; i <= config->numthreads; i++){
        std::unique_lock<std::mutex> ulk(mutexOperacion);
        variableLogger.wait(ulk, []{return datoListo == true;});

        //Hay que leer el resultado
        if(readPipe(pipeThread, resultados, config->numthreads + 1) == 0){

            #ifdef DEBUG
                int j;
                    printf("Soy el Logger, Resultado recibidos");

                for (j = 0; j <= config->numthreads; j++ ){
                    printf(" %f", resultados[j]);
                }
                printf("\n");
            #endif
            datoListo = false;

        }else{
            #ifdef DEBUG
                printf("Soy el Logger, Ha ocurrido un error el la pipe\n");
            #endif
        }
        variableRespuestaLogger.notify_one();
    }

    //Opero los Fragmentos
    #ifdef DEBUG
        printf("Soy el Logger\n");
    #endif
    double result = 0;
    for(i = 0; i < config->numthreads; i++){
        printf("Resultado fragmento %d = %f \n", i, resultados[i]);

        if(config->op == SUM){
            result += resultados[i];
        }else if(config->op == SUB){
            result += resultados[i];
        }else if(config->op == XOR){
            result = (int) result xor (int) resultados[i];
        }
    }
    //Opero el resto
        printf("Resultado fragmento resto = %f\n", resultados[config->numthreads]);

        if(config->op == SUM){
            result += resultados[config->numthreads];
        }else if(config->op == SUB){
            result += resultados[config->numthreads];
        }else if(config->op == XOR){
            result = (int) result xor (int) resultados[config->numthreads];
        }

    std::unique_lock<std::mutex> ulk(mutexOperacion);
    if(writePipe(pipeThread, 0, result) == 0){
        #ifdef DEBUG
            printf("Soy el Logger, Dato pasado al main\n");
        #endif
        *loggerTerminado = true;
        variableMain.notify_one();

    } else {
        #ifdef DEBUG
            printf("Soy el Logger, Ha ocurrido un error el la pipe, al comunicarse con el main\n");
        #endif
    }


}


/**
 * Name: writePipe
 * Parameters: buffer a buffer[2] of double, id the id of the thread, dato the data of the thread
 * Descripcion: Write on a buffer[2] of doubles
 * the data in the index 0 and id in the index 1
 *
 * return 0 if the buffer was empty (id -100, data 0), -1 otherwise
 **/
int writePipe(double buffer[], int id, double dato){
    if(buffer[1] == -100 && buffer[0] == 0){
        buffer[0] = dato;
        buffer[1] = id;
        return 0;
    }else{
        return -1;
    }
}


/**
 * Name: readPipe
 * Parameters: buffer a buffer[2] of double, dato the data[] where should be write, datoSize the sizeof dato[].
 * Descripcion: read from a buffer[2] of doubles the information and write
 * the data in the index (on buffer[1]) of dato[].
 * In the case of there is a data (diferent to 0) in the index specify, the data will be writen on the last position of the array dato[].
 * In the case of the index on buffer[1] is < 0 or > (datoSIze - 1) the data will be writen in the dato[0];
 *
 * return 0 if the buffer is empty, (id of thread is not -100 ) or -1 otherwise
 *
 **/
int readPipe(double buffer[], double dato[], int datoSize){

    if(buffer[1] < 0 && buffer[1] > (datoSize - 1)){buffer[1] = 0;}

    if(buffer[1] != -100){
        if(dato[ (int) buffer[1]] == 0){
            dato[ (int) buffer[1]] = buffer[0];
            buffer[1] = -100;
            buffer[0] = 0;
        }else{
            dato[datoSize - 1] = buffer[0];
            buffer[1] = -100;
            buffer[0] = 0;
        }

        return 0;
    }else{
        return -1;
    }
}

#endif
