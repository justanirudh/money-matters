#include<stdio.h>
#include"declarations.h"
#include<string.h>
#include<pthread.h>

void* transfer_amount(void* st){
	State state = (State)st;
	int index = state->start_index;
	int num_threads = state->num_threads;
	int transfers_size = state->transfers_size;
	int accounts_size = state->accounts_size;
	Account accounts = state->accounts;
	Transfer transfers = state->transfers;
	pthread_mutex_t* lock_ptr = state->lock_ptr; 
	pthread_cond_t* cond_ptr = state->cond_ptr;
	Account_Access accesses = state->accesses;

	//round-robin logic here	
	while(index < transfers_size){
		Transfer transfer = &(transfers[index]);
		char* out_account_name = transfer->out;
		char* in_account_name = transfer->in;
		int amount = transfer->amount;

		Account out = find_account(accounts, accounts_size, out_account_name);
		Account in = find_account(accounts, accounts_size, in_account_name);
	
		//ensuring data race freedom
		access_account(accesses, accounts_size, out_account_name, in_account_name, lock_ptr, cond_ptr);

		out->balance = out->balance - amount;
		in->balance = in->balance + amount;

		release_account(accesses, accounts_size, out_account_name, in_account_name, lock_ptr, cond_ptr);
	
		index = index + num_threads;
	}	

	return 0;
}

Account find_account(Account accounts, int size, char* name){
	int i;
	for(i = 0; i < size; ++i){
		if(strcmp(name, accounts[i].name) == 0)
			break;	
	}
	return &(accounts[i]);
}

Account_Access find_account_access(Account_Access accesses, int size, char* name){
	int i;
	for(i = 0; i < size; ++i){
		if(strcmp(name, accesses[i].name) == 0)
			break;	
	}
	return &(accesses[i]);
}

void access_account(Account_Access accesses,int size, char* out, char* in, pthread_mutex_t* lock_ptr, pthread_cond_t* cond_ptr){
	pthread_mutex_lock(lock_ptr);
	Account_Access a1 = find_account_access(accesses, size, out);
	Account_Access a2 = find_account_access(accesses, size, in);
	while(!(a1->access == 0 && a2->access == 0))
		pthread_cond_wait(cond_ptr, lock_ptr); 
	++(a1->access);
	++(a2->access);
	pthread_mutex_unlock(lock_ptr);
}

void release_account(Account_Access accesses, int size, char* out, char* in, pthread_mutex_t* lock_ptr, pthread_cond_t* cond_ptr){
	pthread_mutex_lock(lock_ptr);
	Account_Access a1 = find_account_access(accesses, size, out);
	Account_Access a2 = find_account_access(accesses, size, in);
	--(a1->access);
	--(a2->access);
	pthread_cond_broadcast(cond_ptr);	
	pthread_mutex_unlock(lock_ptr);
}

