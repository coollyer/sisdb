#include "sis_malloc.h"

void sis_free_call(void *p)
{
	sis_free(p);
}

void sis_sdsfree_call(void *p)
{
	sis_sdsfree((s_sis_sds)p);
}

#if 0
// 这里测试hmem
#define TEST_NUMS 10000
char *testptr[TEST_NUMS];
#include "sis_math.h"
int main()
{

#ifdef HUGE_MEM
    printf("test hmem\n");
    sis_init_random();
    msec_t start = sis_time_get_now_usec();
    for (int i = 0; i < TEST_NUMS; i++)
    {
        int64 len = sis_int_random(1, 4096);
        testptr[i] = sis_malloc(len);
    }
    msec_t costusec = sis_time_get_now_usec() - start;
    start = sis_time_get_now_usec();
    printf("=== malloc %d cost = %lld \n", TEST_NUMS, costusec);

    int freenum = TEST_NUMS / 2;
    for (int i = 0; i < freenum; i++)
    {
        sis_free(testptr[i]);
    }
    costusec = sis_time_get_now_usec() - start;
    start = sis_time_get_now_usec();
    printf("=== free %d cost = %lld \n", freenum, costusec);

    for (int i = 0; i < freenum; i++)
    {
        int64 len = sis_int_random(1, 4096);
        testptr[i] = sis_malloc(len);
    }
    costusec = sis_time_get_now_usec() - start;
    start = sis_time_get_now_usec();
    printf("=== malloc %d cost = %lld \n", freenum, costusec);

    for (int i = 0; i < TEST_NUMS; i++)
    {
        sis_free(testptr[i]);
    }
    costusec = sis_time_get_now_usec() - start;
    start = sis_time_get_now_usec();
    printf("=== free %d cost = %lld \n", TEST_NUMS, costusec);

#endif

return 0;

}
#endif