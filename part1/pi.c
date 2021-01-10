#include <stdio.h>
#include <pthread.h>
#include <immintrin.h>

struct info {
	unsigned long long end;
	unsigned long long *local_number_in_circle;
};
pthread_t *threads_id;
void *toss(void *info);
unsigned rand_num(unsigned *seed);

int main(int argc, char *argv[]) {
	int threads_numbers = 1;
	unsigned long long number_of_tosses = 0;
	unsigned long long number_in_circle = 0;

    char *end;
	threads_numbers = atoi(argv[1]);  // get user input
	number_of_tosses = strtoull(argv[2], &end, 10);
	threads_id = (pthread_t *)malloc(threads_numbers * sizeof(pthread_t));
	
	unsigned long long *local_number_in_circle_ptr = (unsigned long long *)malloc(threads_numbers * sizeof(unsigned long long));
	unsigned long long step = number_of_tosses / threads_numbers;
	struct info info_arr[threads_numbers];

	for (register unsigned i = threads_numbers; i--;) {
      info_arr[i].end = step;
	  info_arr[i].local_number_in_circle = local_number_in_circle_ptr + i;
      pthread_create(&threads_id[i], NULL, toss, (void *)&info_arr[i]);
	}
	for (register unsigned i = threads_numbers; i--;)
		pthread_join(threads_id[i], NULL);
	
	for (register unsigned i = threads_numbers; i--;)
		number_in_circle += local_number_in_circle_ptr[i];

	float pi_estimate = number_in_circle << 2;  // equal to number_in_circle * 4
	pi_estimate /= number_of_tosses;
	printf("%.12f\n", pi_estimate);

	free(threads_id);
	free(local_number_in_circle_ptr);
	return 0;
}

void *toss(void *info) {
	struct info *data = (struct info *)info;
	unsigned seed = 1;
	unsigned long long local_number_in_circle = 0;
	for(register int end = (*data).end; end > 0; end -= 8) {
		float x1 = rand_num(&seed), x2 = rand_num(&seed), x3 = rand_num(&seed), x4 = rand_num(&seed),
			x5 = rand_num(&seed), x6 = rand_num(&seed), x7 = rand_num(&seed), x8 = rand_num(&seed);
		float y1 = rand_num(&seed), y2 = rand_num(&seed), y3 = rand_num(&seed), y4 = rand_num(&seed),
			y5 = rand_num(&seed), y6 = rand_num(&seed), y7 = rand_num(&seed), y8 = rand_num(&seed);
		float input_x[8] = { x1, x2, x3, x4, x1, x2, x3, x4 };
		float input_y[8] = { y1, y2, y3, y4, y5, y6, y7, y8 };
		float output[8] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
		float base[8] = { RAND_MAX, RAND_MAX, RAND_MAX, RAND_MAX, RAND_MAX, RAND_MAX, RAND_MAX, RAND_MAX };
		__m256 x = _mm256_load_ps(input_x);
		__m256 y = _mm256_load_ps(input_y);
		__m256 b = _mm256_load_ps(base);
		__m256 out = _mm256_load_ps(output);
		x = _mm256_div_ps(x, b);
		y = _mm256_div_ps(y, b);
		x =_mm256_mul_ps(x, x);
		y = _mm256_mul_ps(y, y);
		out = _mm256_add_ps(x, y);
		for (register unsigned i = 8; i--;)
			if (out[i] <= 1)
				++local_number_in_circle;
	}
	*(data->local_number_in_circle) = local_number_in_circle;
}

unsigned rand_num(unsigned *seed) {  // Lehmer random number generator
	long product = (long)*seed;
	product *= (0x00000001 << 0x00000002) + 0x00000001;
	int x = (product & 0x7fffffff);
	x += (product >> 0x0000001f);
	x &= 0x7fffffff; 
	x += (x >> 0x0000001f);
	return *seed = x;
}