//Pedro Montarroyos de Pinho RA:10440213
//Gustavo Kiyoshi Ikeda RA:10439179
// Jiye Huang RA:10438990

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define MAX_SENSORES 1000
struct timespec start, end;
int total_alertas = 0;

typedef struct {
    int id;
    double temperatura_soma;
    int count_temperatura;
    double soma_quadrados_temperatura;
    double energia_soma;
} Sensor;

void inicializar_sensores(Sensor sensores[]) {
    for (int i = 0; i < MAX_SENSORES; i++) {
        sensores[i].id = i;
        sensores[i].temperatura_soma = 0;
        sensores[i].count_temperatura = 0;
        sensores[i].soma_quadrados_temperatura = 0;
        sensores[i].energia_soma = 0;
    }
}

void processar_linha(char *linha, Sensor sensores[]) {
    char sensor[20];
    char data[20];
    char hora[20];
    char tipo[20];
    char status[10];
    double valor;

    sscanf(linha, "%s %s %s %s %lf status %s", sensor, data, hora, tipo, &valor, status);

    int id;
    sscanf(sensor, "sensor_%d", &id);
    Sensor *s = &sensores[id];

    if (strcmp(tipo, "temperatura") == 0) {
        s->temperatura_soma += valor;
        s->count_temperatura++;
        s->soma_quadrados_temperatura += valor * valor;
    } else if (strcmp(tipo, "energia") == 0) {
        s->energia_soma += valor;
    }

    if (strcmp(status, "ALERTA") == 0 || strcmp(status, "CRITICO") == 0) {
        total_alertas++;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <arquivo>\n", argv[0]);
        return 1;
    }

    char *nome_arquivo = argv[1];
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) {
        printf("Erro ao abrir arquivo\n");
        return 1;
    }

    Sensor sensores[MAX_SENSORES];
    inicializar_sensores(sensores);

    clock_gettime(CLOCK_MONOTONIC, &start);

    /* Lê e processa o arquivo linha por linha, sem armazenar na memória */
    char buffer[256];
    long total_linhas = 0;
    while (fgets(buffer, sizeof(buffer), arquivo)) {
        processar_linha(buffer, sensores);
        total_linhas++;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double tempo_gasto = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    fclose(arquivo);

    printf("Total de linhas lidas: %ld\n", total_linhas);

    printf("\nMédia de temperatura dos primeiros 10 sensores:\n");
    int count = 0;
    for (int i = 0; i < MAX_SENSORES && count < 10; i++) {
        if (sensores[i].count_temperatura > 0) {
            double media = sensores[i].temperatura_soma / sensores[i].count_temperatura;
            printf("sensor_%03d: %.2f\n", i, media);
            count++;
        }
    }

    double maior_desvio = -1.0;
    int sensor_instavel = -1;
    double total_energia = 0.0;

    for (int i = 0; i < MAX_SENSORES; i++) {
        if (sensores[i].count_temperatura > 1) {
            double media = sensores[i].temperatura_soma / sensores[i].count_temperatura;
            double variancia = (sensores[i].soma_quadrados_temperatura / sensores[i].count_temperatura) - (media * media);
            double desvio = sqrt(variancia);
            if (desvio > maior_desvio) {
                maior_desvio = desvio;
                sensor_instavel = i;
            }
        }
        total_energia += sensores[i].energia_soma;
    }

    if (sensor_instavel != -1) {
        printf("\nSensor mais instável:\n");
        printf("sensor_%03d com desvio padrão = %.4f\n", sensor_instavel, maior_desvio);
    } else {
        printf("\nNenhum sensor com dados suficientes.\n");
    }

    printf("\nTotal de alertas: %d\n", total_alertas);
    printf("\nConsumo total de energia: %.2f\n", total_energia);
    printf("Tempo de execução: %.5f segundos\n", tempo_gasto);
    printf("Pedro Montarroyos de Pinho RA:10440213\n");
    printf("Gustavo Kiyoshi Ikeda RA:10439179\n");
    printf("Jiye Huang RA:10438990\n");

    return 0;
}
