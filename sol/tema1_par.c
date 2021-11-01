#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <string.h>
#include "objects.h"
#include "pthread_barrier_mac.h"


//mine from here
#define MIN(A, B) (((A) < (B)) ? (A) : (B))

typedef struct auxStruct {
	pthread_barrier_t *barrier;
	pthread_mutex_t *mutex;
	int thread_id;
	sack_object *objects;
	int object_count;
	int generations_count;
	int sack_capacity;
	individual *current_generation, *next_generation;
	int P;
} auxStruct;

// alocam memorie
auxStruct *init(pthread_barrier_t *barrier, pthread_mutex_t *mutex, int thread_id, sack_object *objects,
				int object_count, int generations_count, int sack_capacity, int P) {
	auxStruct *myStruct;
	myStruct = malloc(sizeof(auxStruct));

	myStruct->current_generation = (individual*) calloc(object_count, sizeof(individual));
	myStruct->next_generation = (individual*) calloc(object_count, sizeof(individual));

	for (int i = 0; i < object_count; ++i) {
		myStruct->current_generation[i].fitness = 0;
		myStruct->current_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		myStruct->current_generation[i].chromosomes[i] = 1;
		myStruct->current_generation[i].index = i;
		myStruct->current_generation[i].chromosome_length = object_count;

		myStruct->next_generation[i].fitness = 0;
		myStruct->next_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		myStruct->next_generation[i].index = i;
		myStruct->next_generation[i].chromosome_length = object_count;
	}

	myStruct->barrier = barrier;
	myStruct->mutex = mutex;
	myStruct->thread_id = thread_id;
	myStruct->objects = objects;
	myStruct->object_count = object_count;
	myStruct->generations_count = generations_count;
	myStruct->sack_capacity = sack_capacity;
	myStruct->P = P;


	return myStruct;
}

// mine to here

int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int *P, int argc, char *argv[])
{
	FILE *fp;

	if (argc < 4) {
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);

	*P = atoi(argv[3]); // mine

	if (*generations_count == 0) {
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function(const sack_object *objects, individual *generation, int object_count, int sack_capacity)
{
	int weight;
	int profit;

	for (int i = 0; i < object_count; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc2(individual *a, individual *b)
{
	int i;
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;

		for (i = 0; i < first->chromosome_length && i < second->chromosome_length; ++i) {
			first_count += first->chromosomes[i];
			second_count += second->chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			//printf("%d index\n", second->index - first->index);
			return second->index - first->index;
		}
	}
	//printf("%d res\n", res);
	return res;
}

int cmpfunc(const void *a, const void *b)
{
	int i;
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;

		for (i = 0; i < first->chromosome_length && i < second->chromosome_length; ++i) {
			first_count += first->chromosomes[i];
			second_count += second->chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			//printf("%d index\n", second->index - first->index);
			return second->index - first->index;
		}
	}
	//printf("%d res\n", res);
	return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{

	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

// mine from here
void *thread_function (void *arg) {

	auxStruct *workStruct = (auxStruct *)arg;
	int count, cursor;
	// individual *current_generation = (individual*) calloc(workStruct->object_count, sizeof(individual));
	// individual *next_generation = (individual*) calloc(workStruct->object_count, sizeof(individual));
	individual *tmp = NULL;
	/*// printf("aaaaaaaaaaaaaaaa");
	// if(workStruct->thread_id == 0) {
	// 	current_generation = (individual*) calloc(workStruct->object_count, sizeof(individual));
	// 	next_generation = (individual*) calloc(workStruct->object_count, sizeof(individual));

		// for (int i = 0; i < workStruct->object_count; ++i) {
		// 	current_generation[i].fitness = 0;
		// 	current_generation[i].chromosomes = (int*) calloc(workStruct->object_count, sizeof(int));
		// 	current_generation[i].chromosomes[i] = 1;
		// 	current_generation[i].index = i;
		// 	current_generation[i].chromosome_length = workStruct->object_count;

		// 	next_generation[i].fitness = 0;
		// 	next_generation[i].chromosomes = (int*) calloc(workStruct->object_count, sizeof(int));
		// 	next_generation[i].index = i;
		// 	next_generation[i].chromosome_length = workStruct->object_count;
		// }
	//} */

	int start =  workStruct->thread_id * (double)workStruct->generations_count / workStruct->P;
	int end = MIN(workStruct->generations_count, (workStruct->thread_id + 1) *
				(double)workStruct->generations_count / workStruct->P);
	// int start_qs = workStruct->thread_id * workStruct->object_count / workStruct->P;
	// int end_qs = MIN(workStruct->object_count, (workStruct->thread_id + 1)
	// 			* workStruct->object_count / workStruct->P);
	// int start_imp = start_qs, start_par = start_qs;
	// individual aux;
	for (int k = start; k < end; k++) {
		cursor = 0;

		// compute fitness and sort by it
		//pthread_mutex_lock(workStruct->mutex);
		compute_fitness_function(workStruct->objects, workStruct->current_generation,
			workStruct->object_count, workStruct->sack_capacity);
		qsort(workStruct->current_generation, workStruct->object_count, sizeof(individual), cmpfunc); // TODO par this?
		// pthread_mutex_unlock(workStruct->mutex);

		 //my sort
		/*if (start_qs % 2)
			start_par = start_par + 1;
		else
			start_imp = start_imp + 1;
		for (int l = 0; l < workStruct->object_count; l++) {
			for (int n = start_par; n < workStruct->object_count - 1 && n < end_qs; n +=2) {
				if (cmpfunc2 (&current_generation[n], &current_generation[n+1]) > 0) {
					aux = current_generation[n];
					current_generation[n] = current_generation[n+1];
					current_generation[n+1] = aux;
				}
			}

			pthread_barrier_wait(workStruct->barrier);

			for(int n = start_imp; n < workStruct->object_count - 1 && n < end_qs; n += 2) {
				if (cmpfunc2 (&current_generation[n], &current_generation[n+1]) > 0) {

					aux = current_generation[n];
					current_generation[n] = current_generation[n+1];
					current_generation[n+1] = aux;
				}
			}
			pthread_barrier_wait(workStruct->barrier);
		}*/
		//wndofmysort

		// keep first 30% children (elite children selection)
		count = workStruct->object_count * 3 / 10;
		//	pthread_mutex_lock(workStruct->mutex);
		for (int i = 0; i < count; ++i) {
			copy_individual(workStruct->current_generation + i, workStruct->next_generation + i);
		}
		cursor = count;
		//	pthread_mutex_unlock(workStruct->mutex);
		//pthread_barrier_wait(workStruct->barrier);


		// mutate first 20% children with the first version of bit string mutation
		// pthread_mutex_lock(workStruct->mutex);
		count = workStruct->object_count * 2 / 10;
		for (int i = 0; i < count; ++i) {


			copy_individual(workStruct->current_generation + i, workStruct->next_generation + cursor + i);
			mutate_bit_string_1(workStruct->next_generation + cursor + i, k);
		}
		cursor += count;
		//pthread_barrier_wait(workStruct->barrier);
			// pthread_mutex_unlock(workStruct->mutex);


		// mutate next 20% children with the second version of bit string mutation
		count = workStruct->object_count * 2 / 10;
		for (int i = 0; i < count; ++i) {
	//	pthread_mutex_lock(workStruct->mutex);
			copy_individual(workStruct->current_generation + i + count, workStruct->next_generation + cursor + i);
			mutate_bit_string_2(workStruct->next_generation + cursor + i, k);
		// pthread_mutex_unlock(workStruct->mutex);
		}
		cursor += count;
		//pthread_barrier_wait(workStruct->barrier);


		// crossover first 30% parents with one-point crossover
		// (if there is an odd number of parents, the last one is kept as such)
		count = workStruct->object_count * 3 / 10;

		if (count % 2 == 1) {
			copy_individual(workStruct->current_generation + workStruct->object_count - 1,
				workStruct->next_generation + cursor + count - 1);
			count--;
			//pthread_barrier_wait(workStruct->barrier);
		}


		//printf("%d\n", count);
		for (int i = 0; i < count; i += 2) {
			crossover(workStruct->current_generation + i, workStruct->next_generation + cursor + i, k);
		}
		//pthread_barrier_wait(workStruct->barrier);

		// pthread_mutex_lock(workStruct->mutex);

		// switch to new generation
		tmp = workStruct->current_generation;
		workStruct->current_generation = workStruct->next_generation;
		workStruct->next_generation = tmp;
		// pthread_mutex_unlock(workStruct->mutex);
		//printf("%d ffffff  tid %d \n", current_generation[0].fitness, workStruct->thread_id);
		for (int i = 0; i < workStruct->object_count; ++i) {
			workStruct->current_generation[i].index = i;

		}
		pthread_barrier_wait(workStruct->barrier);
		// pthread_mutex_lock(workStruct->mutex);

		if (k % 5 == 0) {
			print_best_fitness(workStruct->current_generation);
		}
		// pthread_mutex_unlock(workStruct->mutex);

	}
	//pthread_barrier_wait(workStruct->barrier);

	if (workStruct->thread_id == 0) {
		compute_fitness_function(workStruct->objects, workStruct->current_generation,
			workStruct->object_count, workStruct->sack_capacity);
		qsort(workStruct->current_generation, workStruct->object_count, sizeof(individual), cmpfunc);
		print_best_fitness(workStruct->current_generation);

		// free resources for old generation
		free_generation(workStruct->current_generation);
		free_generation(workStruct->next_generation);

		// free resources
		free(workStruct->current_generation);
		free(workStruct->next_generation);
	}

	pthread_exit(NULL);
}
//to here

int main(int argc, char *argv[])
{
	// array with all the objects that can be placed in the sack
	sack_object *objects = NULL;

	// number of objects
	int object_count = 0;

	// maximum weight that can be carried in the sack
	int sack_capacity = 0;

	// number of generations
	int generations_count = 0;

	// mine nr threads
	int P = 0;
	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, &P, argc, argv)) {
		return 0;
	}

	// mine from here
	pthread_t tid[P];
	pthread_barrier_t barrier;
	pthread_mutex_t mutex;

	pthread_barrier_init(&barrier, NULL, P);
	pthread_mutex_init(&mutex, NULL);

	// alocam un vector de pointeri la structuri
	auxStruct **aux = malloc(sizeof(auxStruct *) * P);

	// create the threads
	for (int i = 0; i < P; i++) {
		aux[i] = init(&barrier, &mutex, i, objects, object_count, generations_count, sack_capacity, P);
	}

	for (int i = 0; i < P; i++) {
		pthread_create(&tid[i], NULL, thread_function, (void *)aux[i]);
	}

	for (int i = 0; i < P; i++) {
		pthread_join(tid[i], NULL);
	}

	pthread_mutex_destroy(&mutex);
	pthread_barrier_destroy(&barrier);
	// to here

	free(objects);
	free(aux);

	return 0;
}