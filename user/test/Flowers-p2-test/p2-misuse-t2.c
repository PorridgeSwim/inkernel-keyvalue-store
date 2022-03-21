/*kkv_destroy() called while executing kkv_put()*/
#include "con_ed.h"

#define usage(arg0) fprintf(stderr, "usage: %s [<nthreads>]\n", arg0)

#define KEY1 0xabc
#define COLOR "red"
int putend = 0;

pid_t gettid(void)
{
	return syscall(__NR_gettid);
}

void *put_thread(void *ignore)
{
    int putres;

	DEBUG("Thread [%u] init_chameleon initialized\n", gettid());
    for(int i = 0; i < 10000; i ++){
	    putres = kkv_put(KEY1, COLOR, MAX_VAL_SIZE, 0);
        if(putres != 0){
            fprintf(stderr, "put error %s\n", strerror(errno));
	        pthread_exit(NULL);
        }
        DEBUG("[%u] Putting!\n", gettid());
    }
    putend = 1;
	pthread_exit(NULL);
}

void *get_thread(void *ignore)
{
	char res[MAX_VAL_SIZE];
    int getres = 0;
    // int rightcolor = 0;
	DEBUG("Thread [%u] play_hotpotato initialized\n", gettid());

	/* spin wait for the color, =0 means found, result returned to res */
    while (getres == 0 || errno == ENOENT){
        getres = kkv_get(KEY1, res, MAX_VAL_SIZE, KKV_NONBLOCK);
		DEBUG("[%u] Getting!\n", gettid());
        if(putend){
            fprintf(stderr, "no more put\n");
            pthread_exit(NULL);
        }
    }
    DEBUG("[%u] Perm err!\n", gettid());
    fprintf(stderr, "get error %s\n", strerror(errno));
	pthread_exit(NULL);
}

void put_get(int nthreads)
{
	pthread_t threads[nthreads];
	int i, ret;

	kkv_init(0);
    ret = pthread_create((pthread_t *) threads, NULL,
            put_thread, NULL);
    if (ret)
        die("pthread_create() failed");

	for (i = 1; i < nthreads; i++) {
		ret = pthread_create((threads + i), NULL,
				put_thread, NULL);
		if (ret)
			die("pthread_create() failed");
	}

	for (i = 0; i < nthreads; i++)
		pthread_join(threads[i], NULL);
	if(kkv_destroy(0) < 0 ){
		fprintf(stderr, "destroy error: %s\n", strerror(errno));
	}
    fprintf(stderr, "destroyed\n");
}

int main(int argc, char **argv)
{
	int nthreads = argc > 1 ? atoi(argv[1]) : 64;

	if (!nthreads) {
		usage(argv[0]);
		return 2;
	}

	RUN_TEST(put_get, nthreads);
	return 0;
}