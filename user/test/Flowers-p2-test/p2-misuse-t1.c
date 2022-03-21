/*kkv_init(), kkv_destroy() called twice*/
#include "con_ed.h"

void flowers_sequential(void)
{
	int key1 = 0xbeef;
	char *sakura = "spring";
	char *lotus = "summer";
	char res[MAX_VAL_SIZE];

	memset(res, 0xff, sizeof(res));

	assert(kkv_init(0) == 0);
	assert(kkv_init(0) == -EPERM);
    /* using two keys to preserve 2 words*/
	assert(kkv_put(key1, sakura, strlen(sakura) + 1, 0) == 0);
	assert(kkv_put(key1, lotus, strlen(sakura) + 1, 0) == 0);
	assert(kkv_get(key1, res, MAX_VAL_SIZE, KKV_NONBLOCK) == 0);
	assert(strcmp(lotus, res) == 0);
	assert(kkv_destroy(0) == 0);
	assert(kkv_destroy(0) == -EPERM);
}

int main(void)
{
	RUN_TEST(flowers_sequential);
	return 0;
}