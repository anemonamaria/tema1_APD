typedef struct _individual {
	int fitness;
	int *chromosomes;
    int chromosome_length;
	int index;
} individual;

typedef struct _sack_object
{
	int weight;
	int profit;
} sack_object;

typedef struct auxStruct {
	pthread_barrier_t *barrier;
	pthread_mutex_t *mutex;
	sack_object *objects;
	int object_count;
	int generations_count;
	int sack_capacity;
	int P;
} auxStruct;

typedef struct mainStruct {
	int thread_id;
	auxStruct *global;
	individual *dest;
} mainStruct;


int cmpfunc2(individual *a, individual *b)
{
	int i;
	individual *first = a;
	individual *second = b;

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
	return res;
}

void merge(individual *source, int start, int mid, int end, individual *destination) {
	int iA = start;
	int iB = mid;
	int i;

	for (i = start; i < end; i++) {
		if (end == iB || (iA < mid && cmpfunc2(&source[iA], &source[iB]) < 0 )) {
			destination[i] = source[iA];
			iA++;
		} else {
			destination[i] = source[iB];
			iB++;
		}
	}
}




void compare_func_par(individual *current_generation, mainStruct *workStruct)
{
	int thread_id = workStruct->thread_id;
	int width, i;
	individual *aux;

	for (width = 1; width < workStruct->global->object_count; width = width + 1) {

		int start = thread_id *  (workStruct->global->object_count / (1 + width))
						/ workStruct->global->P * (1 + width);
		int end = (thread_id + 1) * (workStruct->global->object_count / (1 + width))
						 / workStruct->global->P *(1 + width);

		for (i = start; i < end; i = i + 1 + width) {
      int right = i + 1 + width;
      if(right > end) right = end;
      int mid = i + width / 2;
      if(mid > end) mid = end;
			merge(current_generation, i, mid, i + 1 + width, workStruct->dest);
		}

		pthread_barrier_wait(workStruct->global->barrier);
		if (thread_id == 0) {
			aux = current_generation;
			current_generation = workStruct->dest;
			workStruct->dest = aux;
		}

		pthread_barrier_wait(workStruct->global->barrier);
	}
}