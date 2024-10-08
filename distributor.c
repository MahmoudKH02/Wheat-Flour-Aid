#include "headers.h"
#include "functions.h"

int energy;
int news_queue;
int my_number;
int drawer_queue;

void got_shot(int sig);


int main(int argc, char *argv[]) {

    if (argc < 9) {
        perror("Not enough arguments\n");
        exit(-1);
    }

    if (signal(SIGUSR2, got_shot) == SIG_ERR) {
        perror("SIGUSR2 in (DISTRIBUTOR)");
        exit(SIGQUIT);
    }
    
    int family_queue = atoi(argv[4]);
    int safe_queue = atoi(argv[1]);
    news_queue = atoi(argv[6]);
    my_number = atoi(argv[7]);
    drawer_queue = atoi(argv[8]);

    int min_energy_decay = atoi( strtok(argv[2], "-") );
    int max_energy_decay = atoi( strtok('\0', "-") );

    int min_start_energy = atoi( strtok(argv[5], "-") );
    int max_start_energy = atoi( strtok('\0', "-") );
    energy = select_from_range(min_start_energy, max_start_energy);

    int DISTRIBUTOR_BAGS_TRIP_hold = atoi(argv[3]);

    printf("(distributor) with pid (%d) is ready to receive bag information ...\n",getpid());
    fflush(NULL);

    // send info to drawer (initial values)
    MESSAGE msg = {DISTRIBUTOR, 0, .data.distributor = {energy, my_number, 0, false, getpid()}};

    if (msgsnd(drawer_queue, &msg, sizeof(msg), 0) == -1 ) {
        perror("Child: msgsend");
    }

    AidPackage bags[ DISTRIBUTOR_BAGS_TRIP_hold ];

    while (1) {
        int count = 0; // Counter to keep track of the number of 10 kg bags stored

        for (int i = 0; i < DISTRIBUTOR_BAGS_TRIP_hold; i++) {
    
            if (msgrcv(safe_queue, &bags[i], sizeof(AidPackage), KG_BAG, 0) == -1) {
                perror("msgrcv ddddd");
            }

            printf(
                "(Distributor) have Bag Information: Type: %ld, Weight: %d count = %d\n",
                bags[count].package_type, bags[count].weight, count
            );
            fflush(NULL);
            count++;

            // send info to drawer (take bag from safe house)
            MESSAGE msg = {DISTRIBUTOR, 1, .data.distributor = {energy, my_number, count, false, getpid()}};

            if (msgsnd(drawer_queue, &msg, sizeof(msg), 0) == -1 ) {
                perror("Child: msgsend");
            }
        }

        sleep( get_sleep_duration(energy) );
        printf("*****************************(Distributor) %d has reached family neighborhood*****************************\n", getpid());
        fflush(NULL);

        for (int i = 0; i < DISTRIBUTOR_BAGS_TRIP_hold; i++) {
            printf("-----(Distributor) Going to families-------\n");
            fflush(NULL);

            // read from msg queue
            familyCritical worst_family;

            if (msgrcv(family_queue, &worst_family, sizeof(familyCritical), SORTER_VALUE, 0) == -1) {
                perror("msgrcv fffff");
            }

            printf("-----(Distributor) Has Received the most starving family from (sorter), bags: %d-------, index %d\n", 
                    worst_family.num_bags_required, worst_family.family_index);
            fflush(NULL);

            int sent_bags = (count < worst_family.num_bags_required)? count : worst_family.num_bags_required;

            bags[count-1].package_type = worst_family.family_index;
            bags[count-1].weight = sent_bags;

            if (msgsnd(family_queue, &bags[count-1], sizeof(AidPackage), 0) == -1) {
                perror("msgsnd");
            }
            count -= sent_bags;

            printf(
                "(Distributor) feeds family %d\n",
                worst_family.family_index
            );
            fflush(NULL);
        
            energy -= select_from_range(min_energy_decay, max_energy_decay);

            // send info to drawer (give bags to families)
            MESSAGE msg = {DISTRIBUTOR, 0, .data.distributor = {energy, my_number, count, false, getpid()}};

            if (msgsnd(drawer_queue, &msg, sizeof(msg), 0) == -1 ) {
                perror("Child: msgsend");
            }

            sleep( get_sleep_duration(energy) );

            if (count == 0)
                break;
        }
    }
    return 0;
}


void got_shot(int sig) {
    int die_probability = 100 - energy;

    bool die = select_from_range(1, 100) <= die_probability;

    if (die) {
        alert_news(news_queue, DISTRIBUTOR, my_number);
        printf("Worker %d is killed\n", getpid());

        // send info to drawer
        MESSAGE msg = {DISTRIBUTOR, 0, .data.distributor = {energy, my_number, 0, true, getpid()}};

        if (msgsnd(drawer_queue, &msg, sizeof(msg), 0) == -1 ) {
            perror("Child: msgsend");
        }
        exit(-1);
    }
}

