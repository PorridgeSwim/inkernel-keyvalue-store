/*inspired by hot potato, test get and put in different threads*/
#include "con_ed.h"

#define usage(arg0) fprintf(stderr, "usage: %s [<nthreads>]\n", arg0)

#define KEY1 0xabc
#define KEY2 0xdef
#define COLOR "red"
#define RED "red"
#define GREEN "green"

pid_t gettid(void)
{
	return syscall(__NR_gettid);
}

void *init_color_thread(void *ignore)
{
	DEBUG("Thread [%u] init_hotpotato initialized\n", gettid());

	/* let loose the hot potato! put "hot potato inside the key"*/
	assert(kkv_put(KEY1, COLOR, strlen(COLOR) + 1, 0) == 0);

	pthread_exit(NULL);
}

void *change_color_thread(void *ignore)
{
	char res[MAX_VAL_SIZE];
    int rightcolor = 0;
	DEBUG("Thread [%u] play_hotpotato initialized\n", gettid());

	/* spin wait for the color, =0 means found, result returned to res */
	while (kkv_get(KEY1, res, MAX_VAL_SIZE, KKV_NONBLOCK) != 0)
		DEBUG("[%u] Cold potato!\n", gettid());

	DEBUG("[%u] Hot potato!\n", gettid());

	/* check we have a right color and change the color */
    if(strcmp(res,RED) == 0){
        fprintf(stderr, "%s ", res);
        assert(kkv_put(KEY1, GREEN, strlen(GREEN) + 1, 0) == 0);
        rightcolor = 1;
    }
    else if(strcmp(res,GREEN) == 0){
        fprintf(stderr, "%s ", res);
        assert(kkv_put(KEY1, RED, strlen(RED) + 1, 0) == 0);
        rightcolor = 1;
    }
	assert(rightcolor == 1);

	pthread_exit(NULL);
}

void chameleon(int nthreads)
{
	pthread_t threads[nthreads];
	int i, ret;

	kkv_init(0);

	ret = pthread_create((pthread_t *) threads, NULL,
			init_color_thread, NULL);
	if (ret)
		die("pthread_create() failed");

	for (i = 1; i < nthreads; i++) {
		ret = pthread_create((threads + i), NULL,
				change_color_thread, NULL);
		if (ret)
			die("pthread_create() failed");
	}

	for (i = 0; i < nthreads; i++)
		pthread_join(threads[i], NULL);

	kkv_destroy(0);
}

int main(int argc, char **argv)
{
	int nthreads = argc > 1 ? atoi(argv[1]) : 64;

	if (!nthreads) {
		usage(argv[0]);
		return 2;
	}

	RUN_TEST(chameleon, nthreads);
	return 0;
}
