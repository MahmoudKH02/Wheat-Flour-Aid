#include "headers.h"


int fid;
int sorter_queue, drawer_queue;
int number_of_families;
int starvation_rate_for_families[50];
int family_max_starvation_rate_index;

int get_num_of_bags(int starve_rate, float required_decrease, int starve_rate_decrease);


int main(int argc, char* argv[]) {

    if (argc < 7) {
        perror("Not Enough Args, sorter.c");
        exit(-1);
    }

    fid = atoi(argv[1]);//the id for the families' message queue
    sorter_queue = atoi(argv[2]);//the id for the sorter's message queue
    number_of_families = atoi(argv[3]);

    float required_decrease = ( atoi(argv[4]) / 100.0 );
    int starve_decrease = atoi(argv[5]);
    
    drawer_queue = atoi(argv[6]);

    printf("Hello from sorter with pid %d\n", getpid());
    fflush(NULL);

    for(int i=0; i<50; i++){
        starvation_rate_for_families[i] = 0;
    }

    struct msqid_ds buf;
    familyStruct familia;

    while (1) {

        msgctl(sorter_queue, IPC_STAT, &buf);

        if(buf.msg_qnum > 0) {

            for(int i = 1; i < (number_of_families+1); i++) {

                if (msgrcv(sorter_queue, &familia, sizeof(familyStruct), i, IPC_NOWAIT) != -1) {
                    starvation_rate_for_families[i] = familia.starvationRate;      
                }
            }

            // find the maximum
            family_max_starvation_rate_index = 1;

            for(int i = 1; i < (number_of_families+1); i++) {

                if(starvation_rate_for_families[i] > starvation_rate_for_families[family_max_starvation_rate_index]) {
                    family_max_starvation_rate_index = i;
                }
            }
 
            familyCritical emptyQueue;
            msgrcv(fid, &emptyQueue, sizeof(familyCritical), SORTER_VALUE, IPC_NOWAIT);

            int bags_required = get_num_of_bags(
                starvation_rate_for_families[family_max_starvation_rate_index],
                required_decrease,
                starve_decrease
            );
            
            familyCritical worst_family;
            worst_family.type = SORTER_VALUE;
            worst_family.family_index = family_max_starvation_rate_index;
            worst_family.num_bags_required = bags_required;

            msgsnd(fid, &worst_family, sizeof(familyCritical), 0);

            // send info to drawer (starvation rate after eating)
            MESSAGE msg = {
                SORTER, 0,
                .data.sorter = {worst_family.family_index, starvation_rate_for_families[family_max_starvation_rate_index], worst_family.num_bags_required}
            };

            if (msgsnd(drawer_queue, &msg, sizeof(msg), 0) == -1 ) {
                perror("Child: msgsend");
                exit(-1);
            }
        }
    }
    return 0;
}

int get_num_of_bags(int starve_rate, float required_decrease, int starve_rate_decrease) {

    int num_bags = (int) round( (starve_rate * required_decrease) / starve_rate_decrease );

    return num_bags;
}