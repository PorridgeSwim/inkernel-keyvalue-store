/*kkv_destroy() called while executing kkv_put()*/
#include "con_ed.h"

#define usage(arg0) fprintf(stderr, "usage: %s [<nthreads>]\n", arg0)

#define KEY1 0xabc
#define KEY2 0xdef
#define COLOR "red"
int putting = 0;

pid_t gettid(void)
{
	return syscall(__NR_gettid);
}

void *putting_thread(void *ignosre)
{
	DEBUG("Thread [%u] init_chameleon initialized\n", gettid());

    for(int i = 1; i < 100; i ++){
        // fprintf(stderr, "start putting -->> ");
        putting ++ ;
	    assert(kkv_put(KEY1, COLOR, MAX_VAL_SIZE, 0) == 0);
        putting -- ;
        // fprintf(stderr, "-->> end putting \n");
    }
	pthread_exit(NULL);
}

void *change_color_thread(void *ignore)
{
	char res[MAX_VAL_SIZE];
	DEBUG("Thread [%u] play_hotpotato initialized\n", gettid());

    while (kkv_get(KEY1, res, MAX_VAL_SIZE, KKV_NONBLOCK) != 0)
		DEBUG("[%u] Getting!\n", gettid());
    if(putting > 0){
        fprintf(stderr, "get when putting\n ");
    }
    else{
        fprintf(stderr, "%d \n", putting);
    }
	DEBUG("[%u] Hot potato!\n", gettid());
	pthread_exit(NULL);
}

void chameleon(int nthreads)
{
	pthread_t threads[nthreads];
	int i, ret;

	kkv_init(0);
	// for (i = 0; i < 10; i++) {
        ret = pthread_create((pthread_t *) threads, NULL,
                putting_thread, NULL);
        if (ret)
            die("pthread_create() failed");
	// }

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