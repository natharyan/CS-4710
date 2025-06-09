#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// there will be no race condition as each thread is responsible for a new row of the dist matrix
// and we joining the thread after each iteration of the k variable loop

# define INF 100000

typedef struct {
    int row;
    int (*distmatrix)[];
    int squaremat_width;
    int intermediate_node;
} distrow;

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

void writetofile(FILE *output, int N, int dist[N][N]){
    if (output == NULL) {
        perror("Error opening file");
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fprintf(output, "%d ", dist[i][j]);
        }
        fprintf(output, "\n");
    }
    fclose(output);
}

void* thread_rowcompute(void*arg){
    distrow* distmat_struct = (distrow*)arg;
    int (*distmat)[distmat_struct->squaremat_width] = distmat_struct->distmatrix;
    for(int j = 0; j < distmat_struct->squaremat_width;j++){
        if(distmat[distmat_struct->row][distmat_struct->intermediate_node] + distmat[distmat_struct->intermediate_node][j] < distmat[distmat_struct->row][j]){
                    distmat[distmat_struct->row][j] = distmat[distmat_struct->row][distmat_struct->intermediate_node] + distmat[distmat_struct->intermediate_node][j];
        }
    }
    free(distmat_struct);
    return NULL;
}

void floydwarshall(int N, int dist[N][N]){
    for(int k = 0; k < N; k++){
        pthread_t *tid = malloc(N * sizeof(pthread_t));
        clock_t start_time = clock();
        for(int i = 0; i < N; i++){
             distrow *distmatstruct = malloc(sizeof(distrow));
            distmatstruct->distmatrix = dist;
            distmatstruct->row = i;
            distmatstruct->intermediate_node = k;
            distmatstruct->squaremat_width = N;
            pthread_create(&tid[i],NULL,thread_rowcompute,distmatstruct);
        }
        clock_t end_time = clock();
        double execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        printf("Execution time: %f seconds\n", execution_time);
        for(int i = 0; i < N; i++ ) {
            pthread_join(tid[i], NULL);
        }
        free(tid);
    }

}

void floydwarshallwithoutthreading(int N, int dist[N][N]){
    for(int k = 0; k < N; k++){
        for(int i = 0; i < N; i++){
            for(int j = 0; j < N; j++){
                if(dist[i][k] + dist[k][j] < dist[i][j]){
                    dist[i][j] = dist[i][k] + dist[k][j];
                }
            }
        }
    }
}



int main(){
    FILE *file = fopen("input.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    int N,M;
    fscanf(file, "%d %d", &N, &M);
    int dist[N][N];

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (i == j) {
                dist[i][j] = 0;
            } else {
                dist[i][j] = INF;
            }
        }
    }

    // deep copy the dist matrix
    int distcopy[N][N];
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            distcopy[i][j] = dist[i][j];
        }
    }

    initdist(N, M, dist, file);
    fclose(file);

    // clock_t start_time = clock();
    floydwarshall(N, dist);
    // clock_t end_time = clock();
    // double execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    // printf("Execution time: %f seconds\n", execution_time);
    clock_t start_time,end_time;
    double execution_time;
    start_time = clock();
    floydwarshallwithoutthreading(N, distcopy);
    end_time = clock();
    execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Execution tim(without threading): %f seconds\n", execution_time);

    FILE *output = fopen("output.txt", "w");
    writetofile(output,N,dist);

    return 1;
}