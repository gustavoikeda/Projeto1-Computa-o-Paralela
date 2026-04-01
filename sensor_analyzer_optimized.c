#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define MAX_LINHAS 10000000
#define MAX_SENSORES 1000

typedef struct {
    int id;
    double temperatura_soma;
    int count_temperatura;
    double soma_quadrados_temperatura;
    double energia_soma;
    int alertas;
} Sensor;

typedef struct {
    char **linhas;
    int inicio;
    int fim;
    Sensor sensores_locais[MAX_SENSORES]; 
    int alertas_locais;
} Thread_chunk;

Sensor sensores_globais[MAX_SENSORES];
int total_alertas_global = 0;

void processar_linha_otimizado(char *linha, Sensor sensores[], int *alertas){
    char *token;
    char *saveptr;
    token = strtok_r(linha, " ", &saveptr);
    if (!token) return;
    int id = atoi(token + 7);

    strtok_r(NULL, " ", &saveptr);
    strtok_r(NULL, " ", &saveptr);

    char *tipo = strtok_r(NULL, " ", &saveptr);
    
    token = strtok_r(NULL, " ", &saveptr);
    if (!token) return;
    double valor = atof(token);

    strtok_r(NULL, " ", &saveptr);
    char *status = strtok_r(NULL, " \n", &saveptr);

    Sensor *s = &sensores[id];
    if(tipo[0] == 't')
    {
        s->temperatura_soma += valor;
        s->count_temperatura++;
        s->soma_quadrados_temperatura += valor * valor;
    }
    else if(tipo[0] == 'e')
    {
        s->energia_soma += valor;
    }

    if(status && (status[0] == 'A' || status[0] == 'C')) 
    {
        (*alertas)++;
    }
}

void* Thread_func(void* arg) {
    Thread_chunk *chunk = (Thread_chunk*) arg;
    
    for(int i = 0; i < MAX_SENSORES; i++)
    {
        chunk->sensores_locais[i] = (Sensor){i, 0, 0, 0, 0, 0};
    }
    chunk->alertas_locais = 0;
    for(int i = chunk->inicio; i < chunk->fim; i++)
    {
        processar_linha_otimizado(chunk->linhas[i], chunk->sensores_locais, &chunk->alertas_locais);
    }
    return NULL;
}

int main(int argc, char *argv[]){
    if (argc != 3)
    {
        printf("Uso: %s <num_threads> <arquivo>\n", argv[0]);
        return 1;
    }
    int num_threads = atoi(argv[1]);
    char *nome_arquivo = argv[2];
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) return 1;

    char **linhas = malloc(sizeof(char*) * MAX_LINHAS);
    int total_linhas = 0;
    char buffer[256];

    while (fgets(buffer, sizeof(buffer), arquivo) && total_linhas < MAX_LINHAS)
    {
        linhas[total_linhas++] = strdup(buffer);
    }
    fclose(arquivo);

    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    Thread_chunk *chunks = malloc(sizeof(Thread_chunk) * num_threads);
    int bloco = total_linhas / num_threads;

    struct timespec start, end;

    //clock_t inicio = clock();
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int i = 0; i < num_threads; i++)
    {
        chunks[i].linhas = linhas;
        chunks[i].inicio = i * bloco;
        chunks[i].fim = (i == num_threads - 1) ? total_linhas : (i + 1) * bloco;
        pthread_create(&threads[i], NULL, Thread_func, &chunks[i]);
    }

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
        total_alertas_global += chunks[i].alertas_locais;
        for (int j = 0; j < MAX_SENSORES; j++) {
            sensores_globais[j].temperatura_soma += chunks[i].sensores_locais[j].temperatura_soma;
            sensores_globais[j].count_temperatura += chunks[i].sensores_locais[j].count_temperatura;
            sensores_globais[j].soma_quadrados_temperatura += chunks[i].sensores_locais[j].soma_quadrados_temperatura;
            sensores_globais[j].energia_soma += chunks[i].sensores_locais[j].energia_soma;
        }
    }

    
    //clock_t fim = clock();
    clock_gettime(CLOCK_MONOTONIC, &end);
    //double tempo_gasto = (double)(fim - inicio) / CLOCKS_PER_SEC;
    double tempo_gasto = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("\nMédia de temperatura (10 primeiros):\n");
    int exibidos = 0;
    for (int i = 0; i < MAX_SENSORES && exibidos < 10; i++)
    {
        if (sensores_globais[i].count_temperatura > 0)
        {
            printf("sensor_%03d: %.2f\n", i, sensores_globais[i].temperatura_soma / sensores_globais[i].count_temperatura);
            exibidos++;
        }
    }

    double maior_desvio = -1.0;
    int sensor_instavel = -1;
    double total_energia = 0.0;
    for (int i = 0; i < MAX_SENSORES; i++)
    {
        if (sensores_globais[i].count_temperatura > 1)
        {
            double media = sensores_globais[i].temperatura_soma / sensores_globais[i].count_temperatura;
            double variancia = (sensores_globais[i].soma_quadrados_temperatura / sensores_globais[i].count_temperatura) - (media * media);
            double desvio = sqrt(fmax(0, variancia));
            if(desvio > maior_desvio)
            {
                maior_desvio = desvio;
                sensor_instavel = i;
            }
        }
        total_energia += sensores_globais[i].energia_soma;
    }

    printf("\nSensor mais instável: sensor_%03d (DP: %.4f)\n", sensor_instavel, maior_desvio);
    printf("Total de alertas: %d\n", total_alertas_global);
    printf("Consumo total de energia: %.2f\n", total_energia);
    printf("Tempo de execução paralelo: %.5f segundos\n", tempo_gasto);

    // Limpeza
    for(int i = 0; i < total_linhas; i++) free(linhas[i]);
    free(linhas); free(threads); free(chunks);
    return 0;
}
