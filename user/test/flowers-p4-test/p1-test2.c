/*inspired by simple sequential*/
#include "con_ed.h"

void flowers_sequential(void)
{
	int key1 = 0xbeef;
	int key2 = 0xace;
	char *cosmos = "fall";
	char *leucojum = "winter";
	char *sakura = "spring";
	char *lotus = "summer";
	char res[MAX_VAL_SIZE];

	memset(res, 0xff, sizeof(res));

	assert(kkv_init(0) == 0);
    /* using two keys to preserve 2 words*/
	assert(kkv_put(key2, cosmos, strlen(cosmos) + 1, 0) == 0);
	assert(kkv_put(key1, sakura, strlen(sakura) + 1, 0) == 0);
	assert(kkv_put(key1, lotus, strlen(sakura) + 1, 0) == 0);
	assert(kkv_get(key1, res, MAX_VAL_SIZE, KKV_NONBLOCK) == 0);
	assert(strcmp(lotus, res) == 0);
	assert(kkv_get(key2, res, MAX_VAL_SIZE, KKV_NONBLOCK) == 0);
	assert(strcmp(cosmos, res) == 0);
	assert(kkv_put(key2, leucojum, strlen(sakura) + 1, 0) == 0);
	assert(kkv_get(key2, res, MAX_VAL_SIZE, KKV_NONBLOCK) == 0);
	assert(strcmp(leucojum, res) == 0);
	/* Sequential get on nonexistent value */
	assert(kkv_get(key2, res, MAX_VAL_SIZE, KKV_NONBLOCK) != 0);
	assert(errno == ENOENT);
	errno = 0;
	assert(kkv_destroy(0) == 0);
}

int main(void)
{
	RUN_TEST(flowers_sequential);
	return 0;
}