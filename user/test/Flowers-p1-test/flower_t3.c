/*check input valid*/
#include "con_ed.h"

void check_input(void)
{
	int key2 = 0xace;
	char *leucojum = "winter";
	char res[MAX_VAL_SIZE];

	memset(res, 0xff, sizeof(res));
	assert(kkv_init(0) == 0);
    /* using two keys to preserve 2 words*/
	assert(kkv_put(key2, leucojum, 0, 0) != 0);
	assert(errno == EINVAL);
	errno = 0;
	assert(kkv_get(key2, NULL, MAX_VAL_SIZE, KKV_NONBLOCK) != 0);
	assert(errno == EINVAL);
}

int main(void)
{
	RUN_TEST(check_input);
	return 0;
}