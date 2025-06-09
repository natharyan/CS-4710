#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// counters to track how many atoms of each type are waiting
int h_count = 0;    // hydrogen atoms counter
int o_count = 0;    // oxygen atoms counter
int c_count = 0;    // carbon atoms counter (for alcohol)
// mutex to protect access to the shared counters
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// condition variables for signaling between threads
pthread_cond_t h_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t o_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t c_cond = PTHREAD_COND_INITIALIZER;
// variables for tracking molecule formation
pthread_t water_h_ids[2];
pthread_t water_o_id;
int water_ready = 0;    // flag for water molecule readiness
int water_h_departed[2] = {0, 0};  // track hydrogen departures from water
int water_o_departed = 0;          // track oxygen departure from water
pthread_t ozone_o_ids[3];
int ozone_ready = 0;    // flag for ozone molecule readiness
int ozone_o_departed[3] = {0, 0, 0};  // track oxygen departures from ozone
pthread_t alcohol_c_ids[2];
pthread_t alcohol_h_ids[6];
pthread_t alcohol_o_id;
int alcohol_ready = 0;  // flag for alcohol molecule readiness
int alcohol_c_departed[2] = {0, 0};    // track carbon departures from alcohol
int alcohol_h_departed[6] = {0, 0, 0, 0, 0, 0};  // track hydrogen departures
int alcohol_o_departed = 0;             // track oxygen departure from alcohol
// simulates the creation of a water molecule
void MakeWater(pthread_t caller_id) {
    printf("\nWater molecule (H2O) formed by H:%lu, H:%lu, O:%lu (called by thread %lu)\n", 
           (unsigned long)water_h_ids[0], (unsigned long)water_h_ids[1], 
           (unsigned long)water_o_id, (unsigned long)caller_id);
    sleep(1); // simulate processing time
}
// simulates the creation of an ozone molecule
void MakeOzone(pthread_t caller_id) {
    printf("\nOzone molecule (O3) formed by O:%lu, O:%lu, O:%lu (called by thread %lu)\n", 
           (unsigned long)ozone_o_ids[0], (unsigned long)ozone_o_ids[1], 
           (unsigned long)ozone_o_ids[2], (unsigned long)caller_id);
    sleep(1); // simulate processing time
}
// simulates the creation of an alcohol molecule
void MakeAlcohol(pthread_t caller_id) {
    printf("\nAlcohol molecule (C2H6O) formed by C:%lu, C:%lu, H:%lu, H:%lu, H:%lu, H:%lu, H:%lu, H:%lu, O:%lu (called by thread %lu)\n",
           (unsigned long)alcohol_c_ids[0], (unsigned long)alcohol_c_ids[1],
           (unsigned long)alcohol_h_ids[0], (unsigned long)alcohol_h_ids[1], 
           (unsigned long)alcohol_h_ids[2], (unsigned long)alcohol_h_ids[3], 
           (unsigned long)alcohol_h_ids[4], (unsigned long)alcohol_h_ids[5], 
           (unsigned long)alcohol_o_id, (unsigned long)caller_id);
    sleep(1); // simulate processing time
}
// function for hydrogen atom threads
void* HarrivesTh(void* arg) {
    pthread_t thread_id = pthread_self();
    int id = *(int*)arg;
    int in_molecule = 0;
    int molecule_type = 0; // 0: none, 1: water, 2: alcohol
    int h_index = -1;      // index in water or alcohol arrays
    pthread_mutex_lock(&mutex);
    printf("Hydrogen thread %lu has arrived\n", (unsigned long)thread_id);
    h_count++;
    printf("Currently have %d hydrogen, %d oxygen, and %d carbon atoms\n", 
           h_count, o_count, c_count);
    // keep checking for molecule formation opportunities
    while (1) {
        // check for alcohol formation (highest priority due to complexity)
        if (!alcohol_ready && c_count >= 2 && h_count >= 6 && o_count >= 1) {
            // find an empty slot for this hydrogen
            for (int i = 0; i < 6 && !in_molecule; i++) {
                if (alcohol_h_ids[i] == 0) {
                    alcohol_h_ids[i] = thread_id;
                    in_molecule = 1;
                    molecule_type = 2;
                    h_index = i;
                    break;
                }
            }
            // check if this completes the alcohol formation
            int complete = 1;
            for (int i = 0; i < 2; i++) {
                if (alcohol_c_ids[i] == 0) complete = 0;
            }
            for (int i = 0; i < 6; i++) {
                if (alcohol_h_ids[i] == 0) complete = 0;
            }
            if (alcohol_o_id == 0) complete = 0;
            
            if (complete && !alcohol_ready) {
                alcohol_ready = 1;
                h_count -= 6;
                c_count -= 2;
                o_count -= 1;
                // this thread will make the alcohol if it's the last hydrogen
                if (alcohol_h_ids[5] == thread_id) {
                    MakeAlcohol(thread_id);
                }
                // signal all waiting threads
                pthread_cond_broadcast(&h_cond);
                pthread_cond_broadcast(&o_cond);
                pthread_cond_broadcast(&c_cond);
                break; // this hydrogen can now depart
            }
        }
        // check for water formation (if not already part of alcohol)
        if (!in_molecule && !water_ready && h_count >= 2 && o_count >= 1) {
            if (water_h_ids[0] == 0) {
                water_h_ids[0] = thread_id;
                in_molecule = 1;
                molecule_type = 1;
                h_index = 0;
            } else if (water_h_ids[1] == 0) {
                water_h_ids[1] = thread_id;
                in_molecule = 1;
                molecule_type = 1;
                h_index = 1;
            }
            // check if this completes the water formation
            if (water_h_ids[0] != 0 && water_h_ids[1] != 0 && water_o_id != 0 && !water_ready) {
                water_ready = 1;
                h_count -= 2;
                o_count -= 1;
                // this thread will make the water if it's the second hydrogen
                if (water_h_ids[1] == thread_id) {
                    MakeWater(thread_id);
                }
                // signal all waiting threads
                pthread_cond_broadcast(&h_cond);
                pthread_cond_broadcast(&o_cond);
                break; // this hydrogen can now depart
            }
        }
        // if not part of any molecule yet or molecule not complete, wait
        if (!in_molecule || 
            (molecule_type == 1 && !water_ready) || 
            (molecule_type == 2 && !alcohol_ready)) {
            pthread_cond_wait(&h_cond, &mutex);
        } else {
            break; // part of a ready molecule, can depart
        }
    }
    // mark this thread as departed
    if (molecule_type == 1 && water_ready) {
        water_h_departed[h_index] = 1;
        // print departure message
        printf("\n---Hydrogen thread %lu is leaving, %d hydrogen atoms remaining---\n", 
               (unsigned long)thread_id, h_count);
        // reset water molecule if all atoms have departed
        if (water_h_departed[0] && water_h_departed[1] && water_o_departed) {
            water_h_ids[0] = water_h_ids[1] = 0;
            water_o_id = 0;
            water_h_departed[0] = water_h_departed[1] = 0;
            water_o_departed = 0;
            water_ready = 0;
        }
    } else if (molecule_type == 2 && alcohol_ready) {
        alcohol_h_departed[h_index] = 1;
        // print departure message
        printf("\n---Hydrogen thread %lu is leaving, %d hydrogen atoms remaining---\n", 
               (unsigned long)thread_id, h_count);
        // check if all atoms have departed from alcohol
        int all_departed = alcohol_o_departed;
        for (int i = 0; i < 2; i++) {
            if (!alcohol_c_departed[i]) all_departed = 0;
        }
        for (int i = 0; i < 6; i++) {
            if (!alcohol_h_departed[i]) all_departed = 0;
        }
        // reset alcohol molecule if all atoms have departed
        if (all_departed) {
            for (int i = 0; i < 2; i++) {
                alcohol_c_ids[i] = 0;
                alcohol_c_departed[i] = 0;
            }
            for (int i = 0; i < 6; i++) {
                alcohol_h_ids[i] = 0;
                alcohol_h_departed[i] = 0;
            }
            alcohol_o_id = 0;
            alcohol_o_departed = 0;
            alcohol_ready = 0;
        }
    } else {
        // not part of any molecule, just leave
        printf("\n---Hydrogen thread %lu is leaving, %d hydrogen atoms remaining---\n", 
               (unsigned long)thread_id, h_count);
    }
    pthread_mutex_unlock(&mutex);
    free(arg);
    return NULL;
}
// function for oxygen atom threads
void* OarrivesTh(void* arg) {
    pthread_t thread_id = pthread_self();
    int id = *(int*)arg;
    int in_molecule = 0;
    int molecule_type = 0; // 0: none, 1: water, 2: ozone, 3: alcohol
    int o_index = -1;      // index in ozone array
    pthread_mutex_lock(&mutex);
    printf("Oxygen thread %lu has arrived\n", (unsigned long)thread_id);
    o_count++;
    printf("Currently have %d hydrogen, %d oxygen, and %d carbon atoms\n", 
           h_count, o_count, c_count);
    // keep checking for molecule formation opportunities
    while (1) {
        // check for alcohol formation (if not already part of ozone)
        if (!in_molecule && !alcohol_ready && c_count >= 2 && h_count >= 6 && o_count >= 1) {
            if (alcohol_o_id == 0) {
                alcohol_o_id = thread_id;
                in_molecule = 1;
                molecule_type = 3;
            }
            // check if this completes the alcohol formation
            int complete = 1;
            for (int i = 0; i < 2; i++) {
                if (alcohol_c_ids[i] == 0) complete = 0;
            }
            for (int i = 0; i < 6; i++) {
                if (alcohol_h_ids[i] == 0) complete = 0;
            }
            if (alcohol_o_id == 0) complete = 0;
            
            if (complete && !alcohol_ready) {
                alcohol_ready = 1;
                h_count -= 6;
                c_count -= 2;
                o_count -= 1;
                // this thread will make the alcohol
                MakeAlcohol(thread_id);
                // signal all waiting threads
                pthread_cond_broadcast(&h_cond);
                pthread_cond_broadcast(&o_cond);
                pthread_cond_broadcast(&c_cond);
                break; // this oxygen can now depart
            }
        }
        // check for ozone formation (highest priority for oxygen)
        if (!ozone_ready && o_count >= 3) {
            for (int i = 0; i < 3 && !in_molecule; i++) {
                if (ozone_o_ids[i] == 0) {
                    ozone_o_ids[i] = thread_id;
                    in_molecule = 1;
                    molecule_type = 2;
                    o_index = i;
                    break;
                }
            }
            // check if this completes the ozone formation
            if (ozone_o_ids[0] != 0 && ozone_o_ids[1] != 0 && ozone_o_ids[2] != 0 && !ozone_ready) {
                ozone_ready = 1;
                o_count -= 3;
                // this thread will make the ozone if it's the third oxygen
                if (ozone_o_ids[2] == thread_id) {
                    MakeOzone(thread_id);
                }
                // signal all waiting threads
                pthread_cond_broadcast(&o_cond);
                break; // this oxygen can now depart
            }
        }
        // check for water formation (if not already part of ozone or alcohol)
        if (!in_molecule && !water_ready && h_count >= 2 && o_count >= 1) {
            if (water_o_id == 0) {
                water_o_id = thread_id;
                in_molecule = 1;
                molecule_type = 1;
            }
            // check if this completes the water formation
            if (water_h_ids[0] != 0 && water_h_ids[1] != 0 && water_o_id != 0 && !water_ready) {
                water_ready = 1;
                h_count -= 2;
                o_count -= 1;
                // this thread will make the water
                MakeWater(thread_id);
                // signal all waiting threads
                pthread_cond_broadcast(&h_cond);
                pthread_cond_broadcast(&o_cond);
                break; // this oxygen can now depart
            }
        }
        // if not part of any molecule yet or molecule not complete, wait
        if (!in_molecule || 
            (molecule_type == 1 && !water_ready) || 
            (molecule_type == 2 && !ozone_ready) ||
            (molecule_type == 3 && !alcohol_ready)) {
            pthread_cond_wait(&o_cond, &mutex);
        } else {
            break; // part of a ready molecule, can depart
        }
    }
    // mark this thread as departed
    if (molecule_type == 1 && water_ready) {
        water_o_departed = 1;
        // print departure message
        printf("\n---Oxygen thread %lu is leaving, %d oxygen atoms remaining---\n", 
               (unsigned long)thread_id, o_count);
        // reset water molecule if all atoms have departed
        if (water_h_departed[0] && water_h_departed[1] && water_o_departed) {
            water_h_ids[0] = water_h_ids[1] = 0;
            water_o_id = 0;
            water_h_departed[0] = water_h_departed[1] = 0;
            water_o_departed = 0;
            water_ready = 0;
        }
    } else if (molecule_type == 2 && ozone_ready) {
        ozone_o_departed[o_index] = 1;
        // print departure message
        printf("\n---Oxygen thread %lu is leaving, %d oxygen atoms remaining---\n", 
               (unsigned long)thread_id, o_count);
        // reset ozone if all oxygens have departed
        if (ozone_o_departed[0] && ozone_o_departed[1] && ozone_o_departed[2]) {
            ozone_o_ids[0] = ozone_o_ids[1] = ozone_o_ids[2] = 0;
            ozone_o_departed[0] = ozone_o_departed[1] = ozone_o_departed[2] = 0;
            ozone_ready = 0;
        }
    } else if (molecule_type == 3 && alcohol_ready) {
        alcohol_o_departed = 1;
        // print departure message
        printf("\n---Oxygen thread %lu is leaving, %d oxygen atoms remaining---\n", 
               (unsigned long)thread_id, o_count);
        // check if all atoms have departed from alcohol
        int all_departed = alcohol_o_departed;
        for (int i = 0; i < 2; i++) {
            if (!alcohol_c_departed[i]) all_departed = 0;
        }
        for (int i = 0; i < 6; i++) {
            if (!alcohol_h_departed[i]) all_departed = 0;
        }
        // reset alcohol molecule if all atoms have departed
        if (all_departed) {
            for (int i = 0; i < 2; i++) {
                alcohol_c_ids[i] = 0;
                alcohol_c_departed[i] = 0;
            }
            for (int i = 0; i < 6; i++) {
                alcohol_h_ids[i] = 0;
                alcohol_h_departed[i] = 0;
            }
            alcohol_o_id = 0;
            alcohol_o_departed = 0;
            alcohol_ready = 0;
        }
    } else {
        // not part of any molecule, just leave
        printf("\n---Oxygen thread %lu is leaving, %d oxygen atoms remaining---\n", 
               (unsigned long)thread_id, o_count);
    }
    pthread_mutex_unlock(&mutex);
    free(arg);
    return NULL;
}
// function for carbon atom threads
void* CarrivesTh(void* arg) {
    pthread_t thread_id = pthread_self();
    int id = *(int*)arg;
    int in_molecule = 0;
    int c_index = -1;      // index in alcohol array
    pthread_mutex_lock(&mutex);
    printf("Carbon thread %lu has arrived\n", (unsigned long)thread_id);
    c_count++;
    printf("Currently have %d hydrogen, %d oxygen, and %d carbon atoms\n", 
           h_count, o_count, c_count);
    // keep checking for alcohol formation opportunities
    while (1) {
        if (!alcohol_ready && c_count >= 2 && h_count >= 6 && o_count >= 1) {
            if (alcohol_c_ids[0] == 0) {
                alcohol_c_ids[0] = thread_id;
                in_molecule = 1;
                c_index = 0;
            } else if (alcohol_c_ids[1] == 0) {
                alcohol_c_ids[1] = thread_id;
                in_molecule = 1;
                c_index = 1;
            }
            // check if this completes the alcohol formation
            int complete = 1;
            for (int i = 0; i < 2; i++) {
                if (alcohol_c_ids[i] == 0) complete = 0;
            }
            for (int i = 0; i < 6; i++) {
                if (alcohol_h_ids[i] == 0) complete = 0;
            }
            if (alcohol_o_id == 0) complete = 0;
            if (complete && !alcohol_ready) {
                alcohol_ready = 1;
                h_count -= 6;
                c_count -= 2;
                o_count -= 1;
                // this thread will make the alcohol if it's the second carbon
                if (alcohol_c_ids[1] == thread_id) {
                    MakeAlcohol(thread_id);
                }
                // signal all waiting threads
                pthread_cond_broadcast(&h_cond);
                pthread_cond_broadcast(&o_cond);
                pthread_cond_broadcast(&c_cond);
                break; // this carbon can now depart
            }
        }
        // if not part of any molecule yet or molecule not complete, wait
        if (!in_molecule || !alcohol_ready) {
            pthread_cond_wait(&c_cond, &mutex);
        } else {
            break; // part of a ready molecule, can depart
        }
    }
    // mark this thread as departed
    if (in_molecule && alcohol_ready) {
        alcohol_c_departed[c_index] = 1;
        // print departure message
        printf("\n---Carbon thread %lu is leaving, %d carbon atoms remaining---\n", 
               (unsigned long)thread_id, c_count);
        // check if all atoms have departed from alcohol
        int all_departed = alcohol_o_departed;
        for (int i = 0; i < 2; i++) {
            if (!alcohol_c_departed[i]) all_departed = 0;
        }
        for (int i = 0; i < 6; i++) {
            if (!alcohol_h_departed[i]) all_departed = 0;
        }
        // reset alcohol molecule if all atoms have departed
        if (all_departed) {
            for (int i = 0; i < 2; i++) {
                alcohol_c_ids[i] = 0;
                alcohol_c_departed[i] = 0;
            }
            for (int i = 0; i < 6; i++) {
                alcohol_h_ids[i] = 0;
                alcohol_h_departed[i] = 0;
            }
            alcohol_o_id = 0;
            alcohol_o_departed = 0;
            alcohol_ready = 0;
        }
    } else {
        // not part of any molecule, just leave
        printf("\n---Carbon thread %lu is leaving, %d carbon atoms remaining---\n", 
               (unsigned long)thread_id, c_count);
    }
    pthread_mutex_unlock(&mutex);
    free(arg);
    return NULL;
}
// wrapper function to create a hydrogen atom thread
void Harrives() {
    int* id = malloc(sizeof(int));
    static int h_id = 0;
    *id = ++h_id;
    pthread_t thread;
    pthread_create(&thread, NULL, HarrivesTh, id);
    pthread_detach(thread);
}
// wrapper function to create an oxygen atom thread
void Oarrives() {
    int* id = malloc(sizeof(int));
    static int o_id = 0;
    *id = ++o_id;
    pthread_t thread;
    pthread_create(&thread, NULL, OarrivesTh, id);
    pthread_detach(thread);
}
// wrapper function to create a carbon atom thread
void Carrives() {
    int* id = malloc(sizeof(int));
    static int c_id = 0;
    *id = ++c_id;
    pthread_t thread;
    pthread_create(&thread, NULL, CarrivesTh, id);
    pthread_detach(thread);
}
// main function for testing the molecule formation simulation
int main() {
    printf("Starting molecule formation simulation...\n");
    printf("----------------------------------------\n");
    // create a mix of hydrogen, oxygen, and carbon atoms
    for (int i = 0; i < 30; i++) {
        
        int choice = rand() % 10 < 6 ? 0 : (rand() % 10 < 8 ? 1 : 2);
        if (choice == 0) {
            Harrives();
        } 
        else if (choice == 1) {
            Oarrives();
        }
        else {
            Carrives();
        }
        usleep(500000);  // space out the arrivals to make output readable
    }
    
    // wait to allow the simulation to complete
    printf("\nWaiting for simulation to complete...\n");
    sleep(10);
    // clean up resources
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&h_cond);
    pthread_cond_destroy(&o_cond);
    pthread_cond_destroy(&c_cond);
    printf("\nSimulation complete!\n");
    return 0;
}