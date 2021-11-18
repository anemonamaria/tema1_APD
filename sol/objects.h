#ifndef OBJECTS_H
#define OBJECTS_H

// structure for an object to be placed in the sack, with its weight and profit
typedef struct _sack_object
{
	int weight;
	int profit;
} sack_object;


// structure for an individual in the genetic algorithm; the chromosomes are an array corresponding to each sack
// object, in order, where 1 means that the object is in the sack, 0 that it is not
typedef struct _individual {
	int fitness;
	int *chromosomes;
    int chromosome_length;
	int count_chromosomes;
	int index;
} individual;

#endif