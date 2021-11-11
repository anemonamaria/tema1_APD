#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <string.h>
#include "objects.h"
#include "pthread_barrier_mac.h"


// TODO - de facut output-ul bun
//  - de paralelizat qsort

//mine from here
#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define MAX(A, B) (((A) < (B)) ? (B) : (A))


typedef struct auxStruct {
	pthread_barrier_t *barrier;
	pthread_mutex_t *mutex;
	//int thread_id;
	sack_object *objects;
	int object_count;
	int generations_count;
	int sack_capacity;
	int P;
} auxStruct;

typedef struct mainStruct {
	int thread_id;
	auxStruct *global;
} mainStruct;

// alocam memorie
mainStruct *init(pthread_barrier_t *barrier, int thread_id, sack_object *objects,
				int object_count, int generations_count, int sack_capacity, int P, pthread_mutex_t *mutex) {
	mainStruct *myStruct;
	myStruct = malloc(sizeof(mainStruct));

	myStruct->thread_id = thread_id;

	auxStruct *aux_struct = malloc(sizeof(auxStruct));

	aux_struct->barrier = barrier;
	//myStruct->thread_id = thread_id;
	aux_struct->objects = objects;
	aux_struct->object_count = object_count;
	aux_struct->generations_count = generations_count;
	aux_struct->sack_capacity = sack_capacity;
	aux_struct->P = P;
	aux_struct->mutex = mutex;

	myStruct->global = aux_struct;


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

void compute_fitness_function(mainStruct *myStruct, individual *generation)//, int start, int end)
{
	int weight;
	int profit;

	// int start = myStruct->global->object_count * myStruct->thread_id / myStruct->global->P;
	// int end = MIN(myStruct->global->object_count, myStruct->global->object_count * (myStruct->thread_id + 1) / myStruct->global->P);
	//pthread_barrier_wait(myStruct->barrier);    i < end; myStruct->global->object_count;
	for (int i = 0; i < myStruct->global->object_count; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += myStruct->global->objects[j].weight;
				profit += myStruct->global->objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= myStruct->global->sack_capacity) ? profit : 0;
	}
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

int *intersection(int a1, int b1, int a2, int b2) {
	int *rez = calloc(2, sizeof(int));
	if(a2 < b1) {
		rez[0] = MAX(a1, a2);
		rez[1] = MIN(b1, b2);
	}

	return rez;

}

void *thread_func2(void *arg) {

	mainStruct *workStruct = (mainStruct *)arg;

	// printf("%d tid %p pointer struct mare %p pointer struct inclusa\n", workStruct->thread_id, workStruct, workStruct->global);
	int count, cursor;
	individual *current_generation = (individual*) calloc(workStruct->global->object_count, sizeof(individual));
	individual *next_generation = (individual*) calloc(workStruct->global->object_count, sizeof(individual));
	individual *tmp = NULL;
	// int *intersc;

	// set initial generation (composed of object_count individuals with a single item in the sack)

	for (int i = 0; i < workStruct->global->object_count; ++i) {
		current_generation[i].fitness = 0;
		current_generation[i].chromosomes = (int*) calloc(workStruct->global->object_count, sizeof(int));
		current_generation[i].chromosomes[i] = 1;
		current_generation[i].index = i;
		current_generation[i].chromosome_length = workStruct->global->object_count;

		next_generation[i].fitness = 0;
		next_generation[i].chromosomes = (int*) calloc(workStruct->global->object_count, sizeof(int));
		next_generation[i].index = i;
		next_generation[i].chromosome_length = workStruct->global->object_count;
	}
	// iterate for each generation

	// int start_c1 = workStruct->global->object_count * workStruct->thread_id / workStruct->global->P;
	// int end_c1 = MIN(workStruct->global->object_count, workStruct->global->object_count * (workStruct->thread_id + 1) / workStruct->global->P);

	// count = workStruct->global->object_count * 3 / 10;
	// int start1 = count * workStruct->thread_id / workStruct->global->P;
	// int end1 = MIN(count, count * (workStruct->thread_id + 1) / workStruct->global->P);

	count = workStruct->global->object_count * 2 / 10;
	int start2 = count * workStruct->thread_id / workStruct->global->P;
	int end2 = MIN(count, count * (workStruct->thread_id + 1) / workStruct->global->P);

	count = workStruct->global->object_count * 2 / 10;
	int start3 = count * workStruct->thread_id / workStruct->global->P;
	int end3 = MIN(count, count * (workStruct->thread_id + 1) / workStruct->global->P);

	int start5 = workStruct->global->object_count * workStruct->thread_id / workStruct->global->P;
	int end5 = MIN(workStruct->global->object_count, (workStruct->thread_id + 1) * workStruct->global->object_count / workStruct->global->P);

	for (int k = 0; k < workStruct->global->generations_count; ++k) {
		cursor = 0;

		compute_fitness_function(workStruct, current_generation);//, start_c1, end_c1);

		// for(int j = 0; j < workStruct->global->object_count; j++) {
		// 	printf("%d  j  %d fit  %d thre %d gen\n",j, current_generation[j].fitness, workStruct->thread_id, k);
		// }
		// printf("\n\n");

		if(workStruct->thread_id == 0) {
			qsort(current_generation, workStruct->global->object_count, sizeof(individual), cmpfunc);

		}


		count = workStruct->global->object_count * 3 / 10;
		// TODO asta paralelizat nu intra in timp pentru ./in1 10 dar celelalte dau corect
		for (int i = 0; i < count; ++i) {
			copy_individual(current_generation + i, next_generation + i);
		}
		pthread_barrier_wait(workStruct->global->barrier);

		cursor = count;

		count = workStruct->global->object_count * 2 / 10;
		for (int i = start2; i < end2; ++i) {
			copy_individual(current_generation + i, next_generation + cursor + i);
			mutate_bit_string_1(next_generation + cursor + i, k);
		}
		pthread_barrier_wait(workStruct->global->barrier);

		cursor += count;
		count = workStruct->global->object_count * 2 / 10;
		for (int i = start3; i < end3; ++i) {
			copy_individual(current_generation + i + count, next_generation + cursor + i);
			mutate_bit_string_2(next_generation + cursor + i, k);
		}
		pthread_barrier_wait(workStruct->global->barrier);
		cursor += count;

		count = workStruct->global->object_count * 3 / 10;
		if (count % 2 == 1) {
			copy_individual(current_generation + workStruct->global->object_count - 1, next_generation + cursor + count - 1);
			count--;
		}

		for (int i = 0; i < count; i += 2) {
			crossover(current_generation + i, next_generation + cursor + i, k);
		}
		pthread_barrier_wait(workStruct->global->barrier);

		tmp = current_generation;
		current_generation = next_generation;
		next_generation = tmp;
		pthread_barrier_wait(workStruct->global->barrier);

		for (int i = start5; i < end5; ++i) {
			current_generation[i].index = i;
		}

		if (k % 5 == 0 && workStruct->thread_id == 0) {
			print_best_fitness(current_generation);
		}
		pthread_barrier_wait(workStruct->global->barrier);

	}

	pthread_barrier_wait(workStruct->global->barrier);

	// TODO aici rezolva compute !!!
	compute_fitness_function(workStruct, current_generation);//, start_c1, end_c1);
	pthread_barrier_wait(workStruct->global->barrier);
	if(workStruct->thread_id == 0) {
		qsort(current_generation, workStruct->global->object_count, sizeof(individual), cmpfunc);
	}

	if(workStruct->thread_id == 0) {
		print_best_fitness(current_generation);
	}

	// free resources for old generation
	free_generation(current_generation);
	free_generation(next_generation);

	// free resources
	free(current_generation);
	free(next_generation);

	pthread_exit(NULL);
}

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
	mainStruct **aux = malloc(sizeof(mainStruct *) * P);

	// create the threads
	for (int i = 0; i < P; i++) {
		aux[i] = init(&barrier, i, objects, object_count, generations_count, sack_capacity, P, &mutex);
	}

	for (int i = 0; i < P; i++) {
		pthread_create(&tid[i], NULL, thread_func2, (void *)aux[i]);
	}

	for (int i = 0; i < P; i++) {
		pthread_join(tid[i], NULL);
	}

	pthread_mutex_destroy(&mutex);
	pthread_barrier_destroy(&barrier);
	// to here


	free(objects);
	for(int i = 0; i < P; i ++) {
		free(aux[i]->global);
		free(aux[i]);
	}
	free(aux);

	return 0;
}