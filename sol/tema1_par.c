#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include "objects.h"
#include "pthread_barrier_mac.h"

#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define MAX(A, B) (((A) < (B)) ? (B) : (A))

typedef struct mainStruct {
	int thread_id;
	individual *dest;
	pthread_barrier_t *barrier;
	sack_object *objects;
	int object_count;
	int generations_count;
	int sack_capacity;
	int P;
	individual *current_generation;
	individual *next_generation;
} mainStruct;

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

	*P = atoi(argv[3]);

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

void compute_fitness_function(mainStruct *myStruct, int start, int end)
{
	int weight;
	int profit;

	for (int i = start; i < end; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < myStruct->current_generation[i].chromosome_length; ++j) {
			if (myStruct->current_generation[i].chromosomes[j]) {
				weight += myStruct->objects[j].weight;
				profit += myStruct->objects[j].profit;
				myStruct->current_generation[i].count_chromosomes++;
			}
		}
		myStruct->current_generation[i].fitness = (weight <= myStruct->sack_capacity) ? profit : 0;
	}
}

int cmpfunc(const void *a, const void *b)
{
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {

		res = first->count_chromosomes - second->count_chromosomes;

		if (res == 0) {
			return second->index - first->index;
		}
	}
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
		generation[i].count_chromosomes = 0;
	}
}

void *thread_func2(void *arg) {

	mainStruct *workStruct = (mainStruct *)arg;
	int count, cursor;

	individual *tmp = NULL;

	int start_c1 = workStruct->object_count * workStruct->thread_id / workStruct->P;
	int end_c1 = MIN(workStruct->object_count, workStruct->object_count * (workStruct->thread_id + 1) / workStruct->P);

	count = workStruct->object_count * 2 / 10;
	int start2 = count * workStruct->thread_id / workStruct->P;
	int end2 = MIN(count, count * (workStruct->thread_id + 1) / workStruct->P);

	count = workStruct->object_count * 2 / 10;
	int start3 = count * workStruct->thread_id / workStruct->P;
	int end3 = MIN(count, count * (workStruct->thread_id + 1) / workStruct->P);

	int start5 = workStruct->object_count * workStruct->thread_id / workStruct->P;
	int end5 = MIN(workStruct->object_count, (workStruct->thread_id + 1) *
					 workStruct->object_count / workStruct->P);

	for (int k = 0; k < workStruct->generations_count; ++k) {
		cursor = 0;

		compute_fitness_function(workStruct, start_c1, end_c1);

		pthread_barrier_wait(workStruct->barrier);

		if(workStruct->thread_id == 0) {
			qsort(workStruct->current_generation, workStruct->object_count, sizeof(individual), cmpfunc);
		}
		pthread_barrier_wait(workStruct->barrier);

		count = workStruct->object_count * 3 / 10;
		for (int i = 0; i < count; ++i) {
			copy_individual(workStruct->current_generation + i, workStruct->next_generation + i);
			(workStruct->next_generation + i)->count_chromosomes = (workStruct->current_generation + i)->count_chromosomes;
		}

		cursor = count;

		count = workStruct->object_count * 2 / 10;
		for (int i = start2; i < end2; ++i) {
			copy_individual(workStruct->current_generation + i, workStruct->next_generation + cursor + i);
			(workStruct->next_generation + cursor + i)->count_chromosomes = (workStruct->current_generation + i)->count_chromosomes;
			mutate_bit_string_1(workStruct->next_generation + cursor + i, k);
		}

		cursor += count;
		count = workStruct->object_count * 2 / 10;
		for (int i = start3; i < end3; ++i) {
			copy_individual(workStruct->current_generation + i + count, workStruct->next_generation + cursor + i);
			(workStruct->next_generation + i + cursor)->count_chromosomes = (workStruct->current_generation + i + count)->count_chromosomes;
			mutate_bit_string_2(workStruct->next_generation + cursor + i, k);
		}
		cursor += count;

		count = workStruct->object_count * 3 / 10;
		if (count % 2 == 1) {
			copy_individual(workStruct->current_generation + workStruct->object_count - 1, workStruct->next_generation + cursor + count - 1);
			(workStruct->next_generation + cursor + count - 1)->count_chromosomes = (workStruct->current_generation + workStruct->object_count - 1)->count_chromosomes;
			count--;
		}

		for (int i = 0; i < count; i += 2) {
			crossover(workStruct->current_generation + i, workStruct->next_generation + cursor + i, k);
		}
		pthread_barrier_wait(workStruct->barrier);

		tmp = workStruct->current_generation;
		workStruct->current_generation = workStruct->next_generation;
		workStruct->next_generation = tmp;

		for (int i = start5; i < end5; ++i) {
			workStruct->current_generation[i].index = i;
		}

		if (k % 5 == 0 && workStruct->thread_id == 0) {
			print_best_fitness(workStruct->current_generation);
		}
		pthread_barrier_wait(workStruct->barrier);

	}

	compute_fitness_function(workStruct, start_c1, end_c1);
	pthread_barrier_wait(workStruct->barrier);
	if(workStruct->thread_id == 0) {
		qsort(workStruct->current_generation, workStruct->object_count, sizeof(individual), cmpfunc);
	}
	pthread_barrier_wait(workStruct->barrier);

	if(workStruct->thread_id == 0) {
		print_best_fitness(workStruct->current_generation);
		free_generation(workStruct->current_generation);
		free_generation(workStruct->next_generation);

		// free resources
		free(workStruct->current_generation);
		free(workStruct->next_generation);
	}
	pthread_barrier_wait(workStruct->barrier);

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

	pthread_t tid[P];
	pthread_barrier_t barrier;

	pthread_barrier_init(&barrier, NULL, P);

	mainStruct aux[P];
	individual *current_generation = (individual*) calloc(object_count, sizeof(individual));
	individual *next_generation = (individual*) calloc(object_count, sizeof(individual));
	for (int i = 0; i < object_count; ++i) {
		current_generation[i].fitness = 0;
		current_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		current_generation[i].chromosomes[i] = 1;
		current_generation[i].count_chromosomes = 1;
		current_generation[i].index = i;
		current_generation[i].chromosome_length = object_count;

		next_generation[i].fitness = 0;
		next_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		next_generation[i].index = i;
		next_generation[i].count_chromosomes = 1;
		next_generation[i].chromosome_length = object_count;
	}
	for (int i = 0; i < P; i++) {
		aux[i].P = P;
		aux[i].thread_id = i;
		aux[i].sack_capacity = sack_capacity;
		aux[i].objects = objects;
		aux[i].object_count=object_count;
		aux[i].next_generation = next_generation;
		aux[i].current_generation = current_generation;
		aux[i].dest =  malloc(sizeof(individual) * object_count);
		aux[i].barrier = &barrier;
		aux[i].generations_count = generations_count;

		pthread_create(&tid[i], NULL, thread_func2, &aux[i]);
	}

	for (int i = 0; i < P; i++) {
		pthread_join(tid[i], NULL);
	}

	pthread_barrier_destroy(&barrier);

	free(objects);

	for(int i = 0; i < P; i ++) {
		free(aux[i].dest);
	}

	return 0;
}