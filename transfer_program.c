#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "declarations.h"

int main(int argc, char* argv[]) 
{
	if(argc != 3) {
		printf("format: ./executable input_file num_workers\n");
		return 0;
	}
	else{
		char* input_file = argv[1];
		int num_threads = atoi(argv[2]);
		
		FILE *fp;    
		char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(input_file, "r");
    if (fp == NULL){
		  printf("failure in creation of file pointer\n");
			exit(1);
		}
      
		//gauge size of Account and Transfer arrays
		int accounts_size = 0;
		int transfers_size = 0;
    while ((read = getline(&line, &len, fp)) != -1) {
			line = strtok(line, "\n");
			char** tokens = str_split(line, ' ');
			char* first_token = *tokens;
			if(strcmp(first_token, "Transfer") == 0)
				++transfers_size;
			else
				++accounts_size;
    }

		Account accounts = (Account)malloc(accounts_size * sizeof(struct account));
		Transfer transfers = (Transfer) malloc(transfers_size * sizeof(struct transfer));

		rewind(fp);

		//populating accounts and transfers
		int accounts_index = 0;
		int transfers_index = 0;
		while ((read = getline(&line, &len, fp)) != -1) {
			line = strtok(line, "\n");
			char** tokens = str_split(line, ' ');
			char* first_token = *tokens;
			//2 ways of doing the same thing, first way better
			if(strcmp(first_token, "Transfer") == 0){
				Transfer t = &(transfers[transfers_index]);
				t->out = *(tokens + 1);
				t->in = *(tokens + 2);
				t->amount = atoi(*(tokens + 3));
				++transfers_index;			
			}
			else{
				Account a = malloc(sizeof(struct account));
				a->name = *tokens;
				a->balance = atoi(*(tokens + 1));
				accounts[accounts_index] = *a;
				++accounts_index;
			}
    }
		//close file 
		fclose(fp);
    if (line)
        free(line);

		//initialize mutex and cond variable: Just 1 mutex and 1 cond var shared by all
		int ret;
		pthread_mutex_t lock; 
		ret = pthread_mutex_init(&lock, NULL) ;
		if (ret) {
			printf("Mutex initialization failed:  %d", ret);
			exit(1);
		}

		pthread_cond_t cond;
		ret = pthread_cond_init( &cond, NULL);
		if (ret) {
			printf("Condition variable initialization failed:  %d", ret);
			exit(1);
		}

		//initializing accesses map
		Account_Access accesses = (Account_Access)malloc(accounts_size * sizeof(struct account_access));
		for(int i = 0; i < accounts_size; ++i){
			Account_Access aa = &(accesses[i]);
			aa->name = accounts[i].name;
			aa->access = 0;
		}
		
		//precomputing states to maximize concurrency
		State states = (State)malloc(num_threads * sizeof(struct state));
		for(int i=0;i<num_threads;i++){
			State st = (State)malloc(sizeof(struct state));
			st->start_index = i;
			st->num_threads = num_threads;
			st->transfers_size = transfers_size;
			st->accounts_size = accounts_size;
			st->accounts = accounts;
			st->transfers = transfers;
			st->lock_ptr = &lock;
			st->cond_ptr = &cond;
			st->accesses = accesses;
			states[i] = *st;
		}
		
		pthread_t threads[num_threads];

		//running threads
    for(int i=0;i<num_threads;i++){	
      ret = pthread_create(&threads[i], NULL, transfer_amount, (void *)&states[i]);
      if (ret){
      	printf("ERROR %d",ret);
        exit(1);
    	}
		}
	
		//joining threads
		for (int i = 0; i < num_threads; i++) {
       ret = pthread_join(threads[i], NULL);
       if (ret) {
				printf("ERROR %d", ret);
				exit(1);
			}
		}

		//print accounts
		for(int i = 0; i < accounts_size; ++i){
			printf("%s %d\n", accounts[i].name, accounts[i].balance);
		}
	}	
}
