#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define MAX_LINHAS 10000000
#define MAX_SENSORES 1000
int total_alertas = 0;
pthread_mutex_t mutex;

typedef struct{
    int id;
    double temperatura_soma;
    int count_temperatura;
    double soma_quadrados_temperatura;
    double energia_soma;
} Sensor;

typedef struct{
    char **linhas;
    int inicio;
    int fim;
    Sensor *sensores;
} Thread_chunk;


void inicializar_sensores(Sensor sensores[]){
    for (int i = 0; i < 1000; i++)
    {
    sensores[i].id = i;
    sensores[i].temperatura_soma = 0;
    sensores[i].count_temperatura = 0;
    sensores[i].soma_quadrados_temperatura = 0;
    sensores[i].energia_soma = 0;
    }
}

void processar_linha(char *linha, Sensor sensores[]){
    char sensor[20];
    char data[20];
    char hora[20];
    char tipo[20];
    char status[10];
    double valor;

    sscanf(linha, "%s %s %s %s %lf status %s", sensor, data, hora, tipo, &valor, status);
    int id;
    sscanf(sensor, "sensor_%d" , &id);
    Sensor *s = &sensores[id];

    //pthread_mutex_lock(&mutex);
    if(strcmp(tipo, "temperatura") == 0)
    {
        s->temperatura_soma += valor;
        s->count_temperatura++;
        s->soma_quadrados_temperatura += valor * valor;
    }
    else if(strcmp(tipo, "energia") == 0)
    {
        pthread_mutex_lock(&mutex);
        s->energia_soma += valor;
        pthread_mutex_unlock(&mutex);
    }

    if (strcmp(status, "ALERTA") == 0 || strcmp(status, "CRITICO") == 0)
    {
        pthread_mutex_lock(&mutex);
        total_alertas++;
        pthread_mutex_unlock(&mutex);
    }
    //pthread_mutex_unlock(&mutex);
}

void* Thread(void* arg){
    Thread_chunk *chunk = (Thread_chunk*) arg;
    for(int i = chunk -> inicio; i < chunk -> fim; i++)
    {
        processar_linha(chunk -> linhas[i], chunk -> sensores);
    }

    return NULL;
}
struct timespec inicio, fim;

int main(int argc, char *argv[]){
    if (argc != 3)
    {
    printf("Uso: %s <num_threads> <arquivo>\n", argv[0]);
    return 1;
    }

    int num_threads = atoi(argv[1]);

    if (num_threads <= 0)
    {
        printf("Número de threads inválido\n");
        return 1;
    }
    char *nome_arquivo = argv[2];
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo)
    {
        printf("Erro ao abrir arquivo\n");
        return 1;
    }
    
    char **linhas = malloc(sizeof(char*) * MAX_LINHAS);
    int total_linhas = 0;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), arquivo) && total_linhas < MAX_LINHAS)
    {
        linhas[total_linhas] = strdup(buffer);
        total_linhas++;
    }
    fclose(arquivo);

    Sensor sensores[MAX_SENSORES];
    inicializar_sensores(sensores);
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    Thread_chunk *chunk = malloc(sizeof(Thread_chunk) * num_threads);
    int bloco = total_linhas / num_threads;
    pthread_mutex_init(&mutex, NULL);
    clock_t inicio = clock();

    for (int i = 0; i < num_threads; i++)
    {
        chunk[i].linhas = linhas;
        chunk[i].inicio = i * bloco;
        chunk[i].fim = (i == num_threads - 1) ? total_linhas : (i + 1) * bloco;
        chunk[i].sensores = sensores;

        pthread_create(&threads[i], NULL, Thread, &chunk[i]);
    }

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
    clock_t fim = clock();
    double tempo_gasto = (double)(fim - inicio) / CLOCKS_PER_SEC;

    printf("\nMédia de temperatura dos primeiros 10 sensores:\n");
    int count = 0;
    for (int i = 0; i < MAX_SENSORES && count < 10; i++)
    {
        if (sensores[i].count_temperatura > 0)
        {
            double media = sensores[i].temperatura_soma / sensores[i].count_temperatura;
            printf("sensor_%03d: %.2f\n", i, media);
            count++;
        }
    }

    double maior_desvio = -1.0;
    int sensor_instavel = -1;
    double total_energia = 0.0;
    for (int i = 0; i < MAX_SENSORES; i++)
    {
        if (sensores[i].count_temperatura > 1)
        {
            double media = sensores[i].temperatura_soma / sensores[i].count_temperatura;
            double variancia = (sensores[i].soma_quadrados_temperatura / sensores[i].count_temperatura)- (media * media);
            double desvio = sqrt(variancia);
            if (desvio > maior_desvio)
            {
                maior_desvio = desvio;
                sensor_instavel = i;
            }
        }
        total_energia += sensores[i].energia_soma;
    }
    if (sensor_instavel != -1)
    {
        printf("\nSensor mais instável:\n");
        printf("sensor_%03d com desvio padrão = %.4f\n", sensor_instavel, maior_desvio);
    } 
    else 
    {
        printf("\nNenhum sensor com dados suficientes.\n");
    }
    printf("\nTotal de alertas: %d\n", total_alertas);
    printf("\nConsumo total de energia: %.2f\n", total_energia);
    
    printf("Tempo de execução: %.5f segundos\n", tempo_gasto);
    for(int i = 0; i < total_linhas; i++)
    {
        free(linhas[i]);
    }
    free(linhas);
    free(threads);
    free(chunk);
}
