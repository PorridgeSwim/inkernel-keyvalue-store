/*try kkv_destroy() while doing kkv_put()*/
#include "con_ed.h"

#define usage(arg0) fprintf(stderr, "usage: %s [<nthreads>]\n", arg0)

#define KEY1 0xabc
#define COLOR "red"
int putend;
int test_times;

pid_t gettid(void)
{
	return syscall(__NR_gettid);
}

void *put_thread(void *ignore)
{
	DEBUG("Thread [%u] init_chameleon initialized\n", gettid());
	for (int i = 0; i < test_times; i++) {
		kkv_put(KEY1, COLOR, MAX_VAL_SIZE, 0);
		DEBUG("[%u] Putting!\n", gettid());
	}
	putend = 1;
	pthread_exit(NULL);
}

void *destroy_thread(void *ignore)
{
	int desres;

    for (int i = 0; i < test_times; i++) {
        desres = kkv_destroy(0);
		if (desres != 0) {
			fprintf(stderr, "destroy error %s, Stop destroying\n", strerror(errno));
			pthread_exit(NULL);
		}
        kkv_init(0);
    }
	pthread_exit(NULL);
}

void destroy_vs_put(int nthreads)
{
	pthread_t threads[nthreads];
	int i, ret;

	kkv_init(0);
	for (i = 0; i < nthreads/2; i++) {
		ret = pthread_create((pthread_t *) threads + i, NULL, put_thread, NULL);
		if (ret)
			die("pthread_create() failed");
	}
	for (i = nthreads/2 ; i < nthreads; i++) {
		ret = pthread_create((threads + i), NULL, destroy_thread, NULL);
		if (ret)
			die("pthread_create() failed");
	}

	for (i = 0; i < nthreads; i++)
		pthread_join(threads[i], NULL);
	if (kkv_destroy(0) < 0)
		fprintf(stderr, "destroy error: %s\n", strerror(errno));
	fprintf(stderr, "kkv destroyed ");
}

int main(int argc, char **argv)
{
	int nthreads = argc > 1 ? atoi(argv[1]) : 64;

	putend = 0;
	test_times = 200;
	if (!nthreads) {
		usage(argv[0]);
		return 2;
	}

	RUN_TEST(destroy_vs_put, nthreads);
	return 0;
}