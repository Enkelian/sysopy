#include "common.h"

sem_t* in_use_sem;
sem_t* are_to_prep_sem;
sem_t* are_to_send_sem;


void exit_fun_worker(){
	sem_close(in_use_sem);
	sem_close(are_to_prep_sem);
	sem_close(are_to_send_sem);
}


void prepare(int orders_fd){

	sem_wait(are_to_prep_sem);

	if(sem_wait(in_use_sem) == -1) error_exit("Could not wait in_use_sem.");

	orders* orders =  mmap(NULL, sizeof(orders), PROT_READ | PROT_WRITE, MAP_SHARED, orders_fd, 0);

	sem_post(in_use_sem);

	orders->vals[orders->first_to_prep] *= 2;
	int n = orders->vals[orders->first_to_prep];

	orders->num_to_prep--;
	orders->num_to_send++;
	orders->first_to_prep = (orders->first_to_prep + 1) % MAX_ORDERS;

	printf("(%d %ld) Przygotowalem liczbe wielkosci %d. Liczba zamowien do przygotowania: %d. Liczba zamowien do wyslania: %d\n",
		   getpid(), time(NULL), n, orders->num_to_prep, orders->num_to_send);



	if(orders->num_to_prep != 0){	//if there are still any to prepare
		sem_post(are_to_prep_sem);
	}

	if(munmap(orders, sizeof(orders)) == -1) error_exit("Could not unmount shared memory.");


	int val;

	sem_getvalue(are_to_send_sem, &val);
	if(val == 0){				//if there were no more to send before
		sem_post(are_to_send_sem);
	}

	sem_post(in_use_sem);
}

int main(int argc, char** argv){

	signal(SIGINT, sigint_handler);
	if(atexit(exit_fun_worker) == -1) error_exit("atexit failed.");

	srand(time(NULL));

	in_use_sem = open_sem(IN_USE);
	are_to_prep_sem = open_sem(ARE_TO_PREP);
	are_to_send_sem = open_sem(ARE_TO_SEND);

	int orders_fd = shm_open(ORD_NAME, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
	if(orders_fd == -1) error_exit("Could not open shared memory.");


	while(1){
		usleep((rand()%5 + 1) * RAND_TIME_MUL);
		prepare(orders_fd);
	}
}