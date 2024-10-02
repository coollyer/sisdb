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

// sort old
// hmem memory begin.
// === malloc 10000 cost = 3543 
// === free 5000 cost = 170176 
// === malloc 5000 cost = 356 
// === free 10000 cost = 100523 

// new no sort
// hmem memory begin.
// === malloc 10000 cost = 21213 
// === free 5000 cost = 3111 
// === malloc 5000 cost = 645 
// === free 10000 cost = 4344 
// === 1 ========================== 
// === malloc 10000 cost = 1134 
// === free 5000 cost = 1377 
// === malloc 5000 cost = 351 
// === free 10000 cost = 2172 
// === 2 ========================== 
// === malloc 10000 cost = 742 
// === free 5000 cost = 1138 
// === malloc 5000 cost = 359 
// === free 10000 cost = 3043 
// === 3 ========================== 
// === malloc 10000 cost = 901 
// === free 5000 cost = 1249 
// === malloc 5000 cost = 354 
// === free 10000 cost = 2327 
// === 4 ========================== 
// === malloc 10000 cost = 707 
// === free 5000 cost = 1189 
// === malloc 5000 cost = 412 
// === free 10000 cost = 2454 
// === 5 ========================== 
// === malloc 10000 cost = 672 
// === free 5000 cost = 1096 
// === malloc 5000 cost = 340 
// === free 10000 cost = 2453 
// === 6 ========================== 
// === malloc 10000 cost = 723 
// === free 5000 cost = 1468 
// === malloc 5000 cost = 389 
// === free 10000 cost = 2469 
// === 7 ========================== 
// === malloc 10000 cost = 688 
// === free 5000 cost = 1235 
// === malloc 5000 cost = 400 
// === free 10000 cost = 2579 
// === 8 ========================== 
// === malloc 10000 cost = 688 
// === free 5000 cost = 1341 
// === malloc 5000 cost = 351 
// === free 10000 cost = 2305 
// === 9 ========================== 
// === malloc 10000 cost = 668 
// === free 5000 cost = 1262 
// === malloc 5000 cost = 515 
// === free 10000 cost = 2465 


// test debug mem
// === 0 ========================== 
// === malloc 10000 cost = 15592 
// === free 5000 cost = 802 
// === malloc 5000 cost = 3915 
// === free 10000 cost = 7490 
// === 1 ========================== 
// === malloc 10000 cost = 10770 
// === free 5000 cost = 1700 
// === malloc 5000 cost = 5020 
// === free 10000 cost = 6567 
// === 2 ========================== 
// === malloc 10000 cost = 7744 
// === free 5000 cost = 1164 
// === malloc 5000 cost = 1977 
// === free 10000 cost = 2041 
// === 3 ========================== 
// === malloc 10000 cost = 5684 
// === free 5000 cost = 2146 
// === malloc 5000 cost = 1960 
// === free 10000 cost = 1626 
// === 4 ========================== 
// === malloc 10000 cost = 4694 
// === free 5000 cost = 1739 
// === malloc 5000 cost = 3386 
// === free 10000 cost = 1737 
// === 5 ========================== 
// === malloc 10000 cost = 2015 
// === free 5000 cost = 900 
// === malloc 5000 cost = 1011 
// === free 10000 cost = 2927 
// === 6 ========================== 
// === malloc 10000 cost = 4751 
// === free 5000 cost = 4004 
// === malloc 5000 cost = 1665 
// === free 10000 cost = 5612 
// === 7 ========================== 
// === malloc 10000 cost = 2902 
// === free 5000 cost = 2335 
// === malloc 5000 cost = 1757 
// === free 10000 cost = 4294 
// === 8 ========================== 
// === malloc 10000 cost = 4542 
// === free 5000 cost = 2658 
// === malloc 5000 cost = 1260 
// === free 10000 cost = 5562 
// === 9 ========================== 
// === malloc 10000 cost = 4349 
// === free 5000 cost = 712 
// === malloc 5000 cost = 2751 
// === free 10000 cost = 1421 


// test standed mem
// === 0 ========================== 
// === malloc 10000 cost = 1764 
// === free 5000 cost = 1117 
// === malloc 5000 cost = 3457 
// === free 10000 cost = 4093 
// === 1 ========================== 
// === malloc 10000 cost = 2823 
// === free 5000 cost = 1235 
// === malloc 5000 cost = 1001 
// === free 10000 cost = 2413 
// === 2 ========================== 
// === malloc 10000 cost = 4517 
// === free 5000 cost = 2330 
// === malloc 5000 cost = 1499 
// === free 10000 cost = 3181 
// === 3 ========================== 
// === malloc 10000 cost = 7033 
// === free 5000 cost = 595 
// === malloc 5000 cost = 3346 
// === free 10000 cost = 11306 
// === 4 ========================== 
// === malloc 10000 cost = 3694 
// === free 5000 cost = 537 
// === malloc 5000 cost = 2785 
// === free 10000 cost = 3919 
// === 5 ========================== 
// === malloc 10000 cost = 4734 
// === free 5000 cost = 1212 
// === malloc 5000 cost = 1165 
// === free 10000 cost = 2833 
// === 6 ========================== 
// === malloc 10000 cost = 3572 
// === free 5000 cost = 540 
// === malloc 5000 cost = 1013 
// === free 10000 cost = 5066 
// === 7 ========================== 
// === malloc 10000 cost = 4327 
// === free 5000 cost = 2180 
// === malloc 5000 cost = 2363 
// === free 10000 cost = 5302 
// === 8 ========================== 
// === malloc 10000 cost = 4292 
// === free 5000 cost = 1817 
// === malloc 5000 cost = 3913 
// === free 10000 cost = 1532 
// === 9 ========================== 
// === malloc 10000 cost = 2911 
// === free 5000 cost = 548 
// === malloc 5000 cost = 1941 
// === free 10000 cost = 1913 
// 这里测试hmem

#define TEST_NUMS 10000
char *testptr[TEST_NUMS];
#include "sis_math.h"
int main()
{
    safe_memory_start();
#ifdef HUGE_MEM
    printf("test hmem\n");
#else 
#ifdef __RELEASE__
    printf("test standed mem\n");
#else
    printf("test debug mem\n");
#endif
#endif
    sis_init_random();
    
    for (int i = 0; i < 10; i++)
    {
        printf("=== %d ========================== \n", i);
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
    }
    sis_sleep(3000);
    printf("=== 1 ========================== \n");
    safe_memory_stop();
    printf("=== 2 ========================== \n");

return 0;

}
#endif