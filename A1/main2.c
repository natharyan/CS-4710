#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// there will be no race condition as each thread is responsible for a new row of the dist matrix
// and we joining the thread after each iteration of the k variable loop

# define INF 100000
# define MAX_N 10
int dist[MAX_N][MAX_N];
pthread_t tid[MAX_N][MAX_N];

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
    int* distmat_struct = (int*)arg;

    for(int j = 0; j < distmat_struct[2];j++){
        if(dist[distmat_struct[0]][distmat_struct[1]] + dist[distmat_struct[1]][j] < dist[distmat_struct[0]][j]){
                    dist[distmat_struct[0]][j]=dist[distmat_struct[0]][distmat_struct[1]] + dist[distmat_struct[1]][j];
        }
    }
    return NULL;
}

void floydwarshall(int N, int dist[N][N]){
    for(int k = 0; k < N; k++){
        clock_t start_time = clock();
        for(int i = 0; i < N; i++){
            int* threadstruct = (int*)malloc(3*sizeof(int));
            threadstruct[0] = i;
            threadstruct[1] = k;
            threadstruct[2] = N;
            pthread_create(&tid[k][i],NULL,thread_rowcompute,threadstruct);
        }
        clock_t end_time = clock();
        double execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        printf("Execution time: %f seconds\n", execution_time);
        
        for(int i = 0; i < N; i++)
	        pthread_join(tid[k][i], NULL);
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
    // int dist[N][N];

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

    // print the dist matrix
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            printf("%d ", dist[i][j]);
        }
        printf("\n");
    }
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