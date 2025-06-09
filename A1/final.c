#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define INF 100000
#define MAX_N 10

int dist[MAX_N][MAX_N];
int dist_seq[MAX_N][MAX_N]; // For sequential execution
pthread_t tid[MAX_N];
pthread_mutex_t lock;

void initdist(int N, int M, int dist[N][N], FILE *file){
    for (int i = 0; i < M; i++) {
        int u, v, w;
        fscanf(file, "%d %d %d", &u, &v, &w);
        u--;
        v--;
        dist[u][v] = w;
        dist[v][u] = w;
    }
}

void copyMatrix(int N, int src[N][N], int dest[N][N]){
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            dest[i][j] = src[i][j];
}

typedef struct {
    int row;
    int k;
    int N;
} ThreadData;

void* thread_rowcompute(void*arg){
    ThreadData* data = (ThreadData*)arg;
    int i = data->row;
    int k = data->k;
    int N = data->N;
    int temp_row[MAX_N];

    for(int j = 0; j < N; j++) {
        temp_row[j] = dist[i][j];
    }

    for(int j = 0; j < N; j++) {
        if (dist[i][k] + dist[k][j] < temp_row[j]) {
            temp_row[j] = dist[i][k] + dist[k][j];
        }
    }

    pthread_mutex_lock(&lock);
    for(int j = 0; j < N; j++) {
        dist[i][j] = temp_row[j];
    }
    pthread_mutex_unlock(&lock);

    free(data);
    return NULL;
}

void floydwarshall_threaded(int N){
    pthread_mutex_init(&lock, NULL);

    for(int k = 0; k < N; k++){
        for(int i = 0; i < N; i++){
            ThreadData* data = (ThreadData*)malloc(sizeof(ThreadData));
            data->row = i;
            data->k = k;
            data->N = N;
            pthread_create(&tid[i], NULL, thread_rowcompute, data);
        }

        for(int i = 0; i < N; i++) {
            pthread_join(tid[i], NULL);
        }
    }

    pthread_mutex_destroy(&lock);
}

void floydwarshall_sequential(int N){
    for(int k = 0; k < N; k++){
        for(int i = 0; i < N; i++){
            for(int j = 0; j < N; j++){
                if (dist_seq[i][k] + dist_seq[k][j] < dist_seq[i][j]) {
                    dist_seq[i][j] = dist_seq[i][k] + dist_seq[k][j];
                }
            }
        }
    }
}

void writeOutputToFile(int N, double time_threaded, double time_sequential){
    FILE *outputFile = fopen("output.txt", "w");
    if (outputFile == NULL) {
        perror("Error opening output.txt for writing");
        return;
    }

    for(int i = 0; i < N; i++){
        for(int j = 0; j < N; j++){
            if(dist[i][j] == INF)
                fprintf(outputFile, "INF ");
            else
                fprintf(outputFile, "%d ", dist[i][j]);
        }
        fprintf(outputFile, "\n");
    }

    fclose(outputFile);
}

int main(){
    FILE *file = fopen("input.txt", "r");
    if (file == NULL) {
        perror("Error opening input.txt");
        return 1;
    }
    int N, M;
    fscanf(file, "%d %d", &N, &M);

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (i == j) {
                dist[i][j] = 0;
                dist_seq[i][j] = 0;
            } else {
                dist[i][j] = INF;
                dist_seq[i][j] = INF;
            }
        }
    }

    initdist(N, M, dist, file);
    fclose(file);

    // Copy matrix for sequential processing
    copyMatrix(N, dist, dist_seq);

    // Threaded Execution
    clock_t start_threaded = clock();
    floydwarshall_threaded(N);
    clock_t end_threaded = clock();
    double time_threaded = ((double)(end_threaded - start_threaded)) / CLOCKS_PER_SEC;

    // Sequential Execution
    clock_t start_sequential = clock();
    floydwarshall_sequential(N);
    clock_t end_sequential = clock();
    double time_sequential = ((double)(end_sequential - start_sequential)) / CLOCKS_PER_SEC;

    // Write output to file
    writeOutputToFile(N, time_threaded, time_sequential);

    return 0;
}
