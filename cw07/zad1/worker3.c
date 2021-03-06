#include "common.h"



void send(int sem_id, int orders_id){
	struct sembuf sops[3];

	fill_sops_pos(sops, 0, IN_USE, 0, 0);	//wait until nobody modifies orders
	fill_sops_pos(sops, 1, ARE_TO_SEND, 0, 0); //wait until there are orders to send
	fill_sops_pos(sops, 2, IN_USE, 1, 0);	//mark orders as currently being used

	if(semop(sem_id, sops, 3) == -1) error_exit("Could not execute operations on semaphores.");

	orders* orders = shmat(orders_id, NULL, 0);

	orders->vals[orders->first_to_send] *= 3;
	int n = orders->vals[orders->first_to_send];

	orders->num_to_send--;
	orders->first_to_send = (orders->first_to_send + 1) % MAX_ORDERS;

	printf("(%d %ld) Wyslalem zamowienie wielkosci %d. Liczba zamowien do przygotowania: %d. Liczba zamowien do wyslania: %d\n",
		   getpid(), time(NULL), n, orders->num_to_prep, orders->num_to_send);


	struct sembuf finalize[3];

	int semsop_idx = 0;

	if(orders->num_to_send == 0){	//there are no more to send
		fill_sops_pos(finalize, semsop_idx++, ARE_TO_SEND, 1, 0);
	}

	if(semctl(sem_id, 3, GETVAL, NULL) == 1){		//if there were no free places before
		fill_sops_pos(finalize, semsop_idx++, ARE_FREE, -1, 0);
	}

	fill_sops_pos(finalize, semsop_idx++, IN_USE, -1, 0);

	if(semop(sem_id, finalize, semsop_idx) == -1) error_exit("Could not execute operations on semaphores.");
	shmdt(orders);

}

int main(int argc, char** argv){

	srand(time(NULL));

	signal(SIGINT, sigint_handler);

	char cwd[PATH_MAX];
	getcwd(cwd, sizeof cwd);
	key_t sem_key = ftok(cwd, SEM_ID);

	int sem_id = semget(sem_key, 0, 0);
	if(sem_id == -1) error_exit("Could not access semaphores.");

	key_t arr_key = ftok(cwd, ORD_ID);
	int orders_id = shmget(arr_key, 0, 0);


	while(1){
		usleep((rand()%5 + 1) * RAND_TIME_MUL);
		send(sem_id, orders_id);
	}


}