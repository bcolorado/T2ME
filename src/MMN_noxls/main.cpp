/* Definiciones externas para el sistema de colas simple */

#include "lcgrand.h" /* Encabezado para el generador de numeros aleatorios */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#define LIMITE_COLA 1000 /* Capacidad maxima de la cola */
#define OCUPADO 1       /* Indicador de Servidor Ocupado */
#define LIBRE 0         /* Indicador de Servidor Libre */

int sig_tipo_evento, num_clientes_espera, num_esperas_requerido, num_eventos,
    num_entra_cola, numero_servidores, servidor_salida;

float area_num_entra_cola, media_entre_llegadas,
    media_atencion, tiempo_simulacion, tiempo_llegada[LIMITE_COLA + 1],
    tiempo_ultimo_evento, total_de_esperas, tiempo_simulacion_maxima, tiempo_servidores_ocupados;

std::vector<float> tiempo_sig_evento, area_estado_servidor;
std::vector<int> estado_servidores;

FILE *parametros, *resultados;

void inicializar(void);
void controltiempo(void);
void llegada(void);
void salida(void);
void reportes(void);
void actualizar_estad_prom_tiempo(void);
float expon(float mean);
float sum(std::vector<float> array);

int main(void) /* Funcion Principal */
{
  /* Abre los archivos de entrada y salida */

  parametros = fopen("param.txt", "r");
  resultados = fopen("result.txt", "w");

  /* Especifica el numero de eventos para la funcion controltiempo. */

  /* Lee los parametros de entrada. */

  fscanf(parametros, "%f %f %f %d %d", &media_entre_llegadas, &media_atencion,
         &tiempo_simulacion_maxima,
         &num_esperas_requerido, &numero_servidores);

  num_eventos = 2;

  /* Escribe en el archivo de salida los encabezados del reporte y los
   * parametros iniciales */

  fprintf(resultados, "Sistema de Colas Simple\n\n");
  fprintf(resultados, "Tiempo promedio de llegada%11.3f minutos\n\n",
          media_entre_llegadas);
  fprintf(resultados, "Tiempo promedio de atencion%16.3f minutos\n\n",
          media_atencion);
  fprintf(resultados, "Tiempo de simulacion%16.3f minutos\n\n", tiempo_simulacion_maxima);
  fprintf(resultados, "Numero de clientes%14d\n\n", num_esperas_requerido);
  fprintf(resultados, "Numero de servidores%14d\n\n", numero_servidores);

  /* iInicializa la simulacion. */

  inicializar();

  /* Corre la simulacion mientras no se llegue al numero de clientes
   * especificaco en el archivo de entrada*/

  while (num_clientes_espera < num_esperas_requerido && tiempo_simulacion < tiempo_simulacion_maxima)
  {

    /* Determina el siguiente evento */
    controltiempo();

    /* Actualiza los acumuladores estadisticos de tiempos promedios */
    actualizar_estad_prom_tiempo();

    /* Invoca la funcion del evento adecuado. */
    switch (sig_tipo_evento)
    {
    case 1:
      llegada();
      break;
    case 2:
      break;
    default:
      salida();
    }
  }

  /* Invoca el generador de reportes y termina la simulacion. */
  reportes();

  fclose(parametros);
  fclose(resultados);

  return 0;
}

void inicializar(void) /* Funcion de inicializacion. */
{
  /* Inicializa el reloj de la simulacion. */

  tiempo_simulacion = 0.0;

  /* Inicializa las variables de estado */

  estado_servidores.resize(numero_servidores + 1);

  for (int i = 1; i <= numero_servidores; i++)
  {
    estado_servidores[i] = LIBRE;
  }

  num_entra_cola = 0;
  tiempo_ultimo_evento = 0.0;
  tiempo_servidores_ocupados = 0.0;

  /* Inicializa los contadores estadisticos. */

  num_clientes_espera = 0;
  total_de_esperas = 0.0;
  area_num_entra_cola = 0.0;

  area_estado_servidor.resize(numero_servidores + 1);

  /* Inicializa la lista de eventos. Ya que no hay clientes, el evento salida
     (terminacion del servicio) no se tiene en cuenta */

  tiempo_sig_evento.resize(num_eventos + numero_servidores + 1);

  tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
  tiempo_sig_evento[2] = tiempo_simulacion_maxima;
  for (int i = 3; i <= num_eventos + numero_servidores; i++)
  {
    tiempo_sig_evento[i] = 1.0e+30;
  }
}

void controltiempo(void) /* Funcion controltiempo */
{
  int i;
  float min_tiempo_sig_evento = 1.0e+29;

  sig_tipo_evento = 0;

  /*  Determina el tipo de evento del evento que debe ocurrir. */

  for (i = 1; i <= num_eventos + numero_servidores; ++i)
    if (tiempo_sig_evento[i] < min_tiempo_sig_evento)
    {
      min_tiempo_sig_evento = tiempo_sig_evento[i];
      sig_tipo_evento = i;
    }

  /* Revisa si la lista de eventos esta vacia. */

  if (sig_tipo_evento == 0)
  {

    /* La lista de eventos esta vacia, se detiene la simulacion. */

    fprintf(resultados, "\nLa lista de eventos esta vacia %f",
            tiempo_simulacion);
    exit(1);
  }

  /* TLa lista de eventos no esta vacia, adelanta el reloj de la simulacion. */

  tiempo_simulacion = min_tiempo_sig_evento;
}

void llegada(void) /* Funcion de llegada */
{
  float espera;

  tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);

  int servidor_libre;
  bool servidor_desocupado_encontrado = false;

  /* Revisa si los servidores estan en modo OCUPADO. */
  for (int i = 1; i <= numero_servidores; i++)
  {
    if (estado_servidores[i] == LIBRE)
    {
      servidor_desocupado_encontrado = true;
      servidor_libre = i;
      break;
    }
  }

  if (servidor_desocupado_encontrado == false)
  {

    /* Servidores en modo OCUPADO, aumenta el numero de clientes en cola */

    ++num_entra_cola;

    /* Verifica si hay condici�n de desbordamiento */

    if (num_entra_cola > LIMITE_COLA)
    {

      /* Se ha desbordado la cola, detiene la simulacion */

      fprintf(resultados,
              "\nDesbordamiento del arreglo tiempo_llegada a la hora");
      fprintf(resultados, "%f", tiempo_simulacion);
      exit(2);
    }

    /* Todavia hay espacio en la cola, se almacena el tiempo de llegada del
            cliente en el ( nuevo ) fin de tiempo_llegada */

    tiempo_llegada[num_entra_cola] = tiempo_simulacion;
  }

  else
  {
    /*  El servidor esta LIBRE, por lo tanto el cliente que llega tiene tiempo
       de espera=0 (Las siguientes dos lineas del programa son para claridad, y
       no afectan el reultado de la simulacion ) */

    espera = 0.0;
    total_de_esperas += espera;

    /* Incrementa el numero de clientes en espera, y pasa el servidor a ocupado
     */
    ++num_clientes_espera;
    estado_servidores[servidor_libre] = OCUPADO;

    /* Programa una salida ( servicio terminado ). */
    tiempo_sig_evento[num_eventos + servidor_libre] = tiempo_simulacion + expon(media_atencion);
  }
}

void salida(void) /* Funcion de Salida. */
{
  int i;
  float espera;

  /* Revisa si la cola esta vacia */

  if (num_entra_cola == 0)
  {

    /* La cola esta vacia, pasa el servidor a LIBRE y
    no considera el evento de salida*/
    estado_servidores[sig_tipo_evento - num_eventos] = LIBRE;
    tiempo_sig_evento[sig_tipo_evento] = 1.0e+30;
  }

  else
  {

    /* La cola no esta vacia, disminuye el numero de clientes en cola. */
    --num_entra_cola;

    /*Calcula la espera del cliente que esta siendo atendido y
    actualiza el acumulador de espera */

    espera = tiempo_simulacion - tiempo_llegada[1];
    total_de_esperas += espera;

    /*Incrementa el numero de clientes en espera, y programa la salida. */
    ++num_clientes_espera;
    tiempo_sig_evento[sig_tipo_evento] = tiempo_simulacion + expon(media_atencion);

    /* Mueve cada cliente en la cola ( si los hay ) una posicion hacia adelante
     */
    for (i = 1; i <= num_entra_cola; ++i)
      tiempo_llegada[i] = tiempo_llegada[i + 1];
  }
}

void reportes(void) /* Funcion generadora de reportes. */
{
  if (tiempo_simulacion > tiempo_simulacion_maxima)
  {
    tiempo_simulacion = tiempo_simulacion_maxima;
  }

  /* Calcula y estima los estimados de las medidas deseadas de desempe�o */
  fprintf(resultados, "\n\nEspera promedio en la cola%11.3f minutos\n\n",
          total_de_esperas / num_clientes_espera);
  fprintf(resultados, "Numero promedio en cola%10.6f\n\n",
          area_num_entra_cola / tiempo_simulacion);
  fprintf(resultados, "Uso promedio de los servidores%15.3f\n\n",
          (sum(area_estado_servidor) / numero_servidores) / tiempo_simulacion);
  fprintf(resultados, "Tiempo de terminacion de la simulacion%12.3f minutos\n\n",
          tiempo_simulacion);
  fprintf(resultados, "Valor formula C de Erlang (simulado)%12.6f",
          tiempo_servidores_ocupados / tiempo_simulacion);
}

void actualizar_estad_prom_tiempo(
    void) /* Actualiza los acumuladores de
                                                                                 area para las estadisticas de tiempo promedio. */
{
  float time_since_last_event;

  /* Calcula el tiempo desde el ultimo evento, y actualiza el marcador
      del ultimo evento */

  time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
  tiempo_ultimo_evento = tiempo_simulacion;

  /* Actualiza el area bajo la funcion de numero_en_cola */
  area_num_entra_cola += num_entra_cola * time_since_last_event;

  /*Actualiza el area bajo la funcion indicadora de servidor ocupado*/
  bool hacer_suma = true;
  for (int i = 1; i <= numero_servidores; i++)
  {
    area_estado_servidor[i] += estado_servidores[i] * time_since_last_event;
    if (estado_servidores[i] == LIBRE && hacer_suma)
    {
      hacer_suma = false;
    }
  }

  if (hacer_suma == true)
  {
    tiempo_servidores_ocupados += time_since_last_event;
  }
}

float expon(float media) /* Funcion generadora de la exponencias */
{
  /* Retorna una variable aleatoria exponencial con media "media"*/

  return -media * log(lcgrand(1));
}

float sum(std::vector<float> array)
{
  float sum = 0;
  for (int i = 1; i <= numero_servidores; i++)
  {
    sum += array[i];
  }
  return sum;
}
