/*
 *	== INCLUDES ==
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/*
 *	== CONSTANTS ==
 */

#ifdef O_CREAT
#undef O_CREAT
#define O_CREAT 0x0040
#endif

#define APPROV_ERROR	5

#define INIT_RANDOM	0
#define INIT_FILE	1

#define DERIVATIVE_DELTA 1

/*
 *	== MACROS ==
 */

#define PRINT_USAGE printf("Usage:\n" \
			"first -r <input>\n" \
			"first -t <input> <expected result>\n")

#define VALIDATE_ARGS (argc > 2 ? (argv[1][0] == '-' ? 1 : 0) : 0)

#define MOD(x) (x >= 0 ? x : -x)

/*
 *	== TYPES ==
 */

typedef struct neuron_t{
	int weight;
	int (* operation)(int, int);
};

/*
 *	== FUNCTION DECLARATIONS ==
 */

void initNeurons();

void updateWeights(int input, int expectedOutput);

int processInput(int input);

int derivative(int (* func)(int, int), int input, int weight);

int power(int, int);

int div2(int, int);

int mul2(int, int);

int add2(int, int);

int sub2(int, int);

/*
 *	== GLOBAL VARIABLES ==
 */

int		 	origWeights[4]	= {0, 0, 0, 0}	;	// weights already calculated, stored in file
int 			fd 		= 0		;	// weights file descriptor
struct neuron_t 	neurons[4]	= {		\
					{ 0, div2 },	\
					{ 0, mul2 },	\
					{ 0, add2 },	\
					{ 0, sub2 },	\
					}		;	// neurons


/*
 *	== CODE ==
 */

int main(int argc, char* argv[]){
	initNeurons();

	if(VALIDATE_ARGS){
		switch(argv[1][1]){
			case 't':
				if(argv[3] == NULL){
					PRINT_USAGE;
					return -1;
				}

				updateWeights(atoi(argv[2]), atoi(argv[3]));
				printf("Lesson learned.\n");
				return 0;
			case 'r':
				printf("result: %d\n", processInput(atoi(argv[2])));
				return 0;
			default:
				PRINT_USAGE;
				return -1;
		}
		return 0;
	} else{
		PRINT_USAGE;
		return -1;
	}
}

int processInput(int input){
	int output = 0;	
	for(int i = 0; i < 4; i++){
		output += neurons[i].operation(input, neurons[i].weight);
	}
	return output;
}

void updateWeights(int input, int expectedOutput){
	int gradient[4][2];
	int delta = expectedOutput - processInput(input);
	
	// count gradient (and sort it to optimize next step)
	for(int i = 0; i < 4; i++){
		int temp = derivative(neurons[i].operation, input, neurons[i].weight);

		gradient[i][0] = temp;
		gradient[i][1] = i;

		for(int j = i - 1; j >= 0; j--){					// worst sorting algorithm ever
			if(MOD(temp) < MOD(gradient[j][0])){
				gradient[j+1][1] = gradient[j][1];
				gradient[j+1][0] = gradient[j][0];
				gradient[j][1] = i;
				gradient[j][0] = temp;
			}
		}
	}

	// decrease delta assuming antigradient = -gradient
	while(delta){
		if(delta > 0){
			for(int i = 3; i >= 0; i--){
				if(delta - MOD(gradient[i][0]) >= 0){
					delta -= MOD(gradient[i][0]);
					if(gradient[i][0] > 0){
						origWeights[gradient[i][1]]++;
					} else{
						origWeights[gradient[i][1]]--;
					}
					break;
				}
			}
		} else{
			for(int i = 3; i >= 0; i--){
				if(delta + MOD(gradient[i][0]) <= 0){
					delta += MOD(gradient[i][0]);
					if(gradient[i][0] > 0){
						origWeights[gradient[i][1]]--;
					} else{
						origWeights[gradient[i][1]]++;
					}
					break;
				}
			}
		}
		int i = 0;
		do{
			if(MOD(delta) < MOD(gradient[i][0])){
				goto update;
			}
		}while((i < 4) && (!gradient[i++][0]));
	}
	update:
	fd = open("weights", O_WRONLY);
	write(fd, origWeights, sizeof(origWeights));
	close(fd);

	return;
}

void initNeurons(){
	fd = open("weights", O_CREAT | O_RDWR, 0666);
	if(fd < 0){
		printf("Failed to open file\n");
		exit(-1);
	}
	if(read(fd, origWeights, sizeof(origWeights)) == 0){
		write(fd, origWeights, sizeof(origWeights));
		close(fd);
		return;
	} else{
		for(int i = 0; i < 4; i++){
			neurons[i].weight = origWeights[i];
		}
		
		close(fd);
		return;
	}
}

int power(int base, int exp){
	if(exp <= 0){
		return 1;
	}

	int tmp = 1;
	for(int i = 0; i < exp; i++){
		tmp *= base;
	}
	return tmp;
}

int derivative(int (* func)(int, int), int input, int weight){
	return (int) (func(input, weight + DERIVATIVE_DELTA) - func(input, weight))/DERIVATIVE_DELTA;
}

/*
 *	== NEURONS' OPERATIONS ==
 */

int div2(int a, int weight){
	return a/power(2, weight);
}

int mul2(int a, int weight){
	return a*power(2, weight);
}

int add2(int a, int weight){
	return a+2*weight;
}

int sub2(int a, int weight){
	return a-2*weight;
}
