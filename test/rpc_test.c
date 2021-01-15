#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/time.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<getopt.h>
#include <time.h>
#include<pthread.h>
#include "rpc_dev.h"
#include "ipc_cmd_type.h"

#define	CMD_HIFI_DSPA	0x1
#define CMD_HIFI_DSPB	0x2

#define RPCUINT_SIZE	64
#define RPCUINT3_CNT	100
#define RPCUINT4_CNT	10
#define RPCUINT5_CNT	10
#define RPCUINT6_CNT	100
#define RPCUINT7_CNT	10
#define RPCUINT8_CNT	10

extern char *optarg;
extern int optind, opterr, optopt;
int lopt;
static char opt_string[]="d:12h";

struct mbox_uint {
	uint32_t uintcmd;
	char data[RPCUINT_SIZE];
	uint32_t sumdata;
} __attribute__((packed));

#define MSECS_PER_SEC (1000L)
#define NSECS_PER_MSEC (1000000L)

static void rpc_get_cur_timestamp(struct timespec* ts)
{
	clock_gettime(CLOCK_MONOTONIC, ts);
	return;
}

static uint32_t rpc_msec_duration(struct timespec* tsEnd, struct timespec* tsStart)
{
	uint32_t uEndMSec = (uint32_t)(tsEnd->tv_sec*MSECS_PER_SEC) + (uint32_t)(tsEnd->tv_nsec/NSECS_PER_MSEC);
	uint32_t uStartMSec = (uint32_t)(tsStart->tv_sec*MSECS_PER_SEC) + (uint32_t)(tsStart->tv_nsec/NSECS_PER_MSEC);

	return (uEndMSec - uStartMSec);
}
#define TIC              \
    struct timespec bgn, end; \
    rpc_get_cur_timestamp(&bgn)

#define TOC                            \
    rpc_get_cur_timestamp(&end); \
    uint32_t ms = rpc_msec_duration(&end, &bgn)


static struct option long_opts[] = {
	{ "dsp", required_argument, NULL, 'd' },
	{ "test-1", no_argument, NULL, '1' },
	{ "test-2", no_argument, NULL, '2' },
	{ "test-3", no_argument, NULL, '3' },
	{ "test-4", no_argument, NULL, '4' },
	{ "test-5", no_argument, NULL, '5' },
	{ "test-6", no_argument, NULL, '6' },
	{ "test-7", no_argument, NULL, '7' },
	{ "test-8", no_argument, NULL, '8' },
	{ "test-9", no_argument, NULL, '9' },
	{ "help", no_argument, NULL, 'h' },
	{ 0, 0, 0, 0 }
};

void show_usage() {
	printf("Usage:[options]\n");
	printf(" -d, --dsp=DSPNAME    The dspname, hifi4a or hifi4b\n");
	printf(" --test-1             Test 1\n");
	printf(" --test-2             Test 2\n");
	printf(" --test-3             test uint3: ap 2 dsp notfeedback 100 times\n");
	printf(" --test-4             test uint4: ap 2 dsp, dsp 2 risc-v sycn test 10 times\n");
	printf(" --test-5             test uint5: ap 2 dsp, dsp 2 dsp sync and async test 10 times\n");
	printf(" --test-6             test uint6: ap 2 dsp feedback 100 times\n");
	printf(" --test-7             test uint7: ap 2 dsp feedback api mutil-task call rpc test 3*10 times\n");
	printf(" --test-8             test uint8: ap 2 dsp, resume dsploop test thread\n");
	printf(" --test-9             test uint9: ap 2 dsp, suspend dsploop test thread\n");
	printf(" -h, --help           Print this message and exit.\n");
}

int dsp_dev_is_exist(const char *path)
{
	if (access(path,F_OK) != -1) {
		printf("dsp dev %s is exist\n", path);
	} else {
		printf("dsp dev %s is not exist\n", path);
		return -1;
	}
	return 0;
}


int mbox_uint3(const char *path) {
	uint32_t i, sumdata = 0;
	int testcnt = RPCUINT3_CNT;
	struct mbox_uint sendbuf;
	int fd =  RPC_init(path, O_RDWR, 0);

	TIC;

	srand((unsigned)time(NULL));

	while (testcnt--) {
		sendbuf.uintcmd = 0x3;
		sumdata = 0;
		for (i = 0; i < RPCUINT_SIZE; i++) {
			sendbuf.data[i] = rand() % 0xff - 1;
			sumdata += sendbuf.data[i];
		}
		sendbuf.sumdata = sumdata;
		RPC_invoke(fd, MBX_CMD_RPCUINT_TEST, &sendbuf, sizeof(sendbuf));
		if (sendbuf.sumdata != sumdata - 1)
			break;
	}

	TOC;

	if (testcnt <= 0)
		printf("mbox unit3 test pass: %d, time: %u ms\n", RPCUINT3_CNT, ms);
	else
		printf("mbox unit3 test fail: %d, sumdata:0x%x\n", testcnt, sendbuf.sumdata);

	RPC_deinit(fd);

	return 0;
}

int mbox_uint4(const char *path) {
	int testcnt = RPCUINT4_CNT;
	struct mbox_uint sendbuf;
	int fd =  RPC_init(path, O_RDWR, 0);

	TIC;

	while (testcnt--) {
		memset(&sendbuf, 0x0, sizeof(sendbuf));
		sendbuf.uintcmd = 0x4;
		RPC_invoke(fd, MBX_CMD_RPCUINT_TEST, &sendbuf, sizeof(sendbuf));
		if (sendbuf.sumdata != 0x4)
			break;
	}

	TOC;

	if (testcnt <= 0)
		printf("mbox unit4 test pass: %d, time: %u ms\n", RPCUINT4_CNT, ms);
	else
		printf("mbox unit4 test fail: %d, sumdata:0x%x\n", testcnt, sendbuf.sumdata);

	RPC_deinit(fd);

	return 0;
}

int mbox_uint5(const char *path) {
	int testcnt = RPCUINT5_CNT;
	struct mbox_uint sendbuf;
	int fd =  RPC_init(path, O_RDWR, 0);

	TIC;

	while (testcnt--) {
		memset(&sendbuf, 0x0, sizeof(sendbuf));
		sendbuf.uintcmd = 0x5;
		RPC_invoke(fd, MBX_CMD_RPCUINT_TEST, &sendbuf, sizeof(sendbuf));
		if (sendbuf.sumdata != 0x5)
			break;
	}

	TOC;

	if (testcnt <= 0)
		printf("mbox unit5 test pass: %d, time: %u ms\n", RPCUINT5_CNT, ms);
	else
		printf("mbox unit5 test fail: %d, sumdata:0x%x\n", testcnt, sendbuf.sumdata);

	RPC_deinit(fd);

	return 0;
}

int mbox_uint6(const char *path) {
	uint32_t i, sumdata = 0;
	int testcnt = RPCUINT6_CNT;
	struct mbox_uint sendbuf;
	int fd =  RPC_init(path, O_RDWR, 0);

	TIC;

	srand((unsigned)time(NULL));

	while (testcnt--) {
		sendbuf.uintcmd = 0x6;
		sumdata = 0;
		for (i = 0; i < RPCUINT_SIZE; i++) {
			sendbuf.data[i] = rand() % 0xff - 1;
			sumdata += sendbuf.data[i];
		}
		sendbuf.sumdata = sumdata;
		RPC_invoke(fd, MBX_CMD_RPCUINT_TESTA, &sendbuf, sizeof(sendbuf));
		if (sendbuf.sumdata != sumdata - 1)
			break;
	}

	TOC;

	if (testcnt <= 0)
		printf("mbox unit6 test pass: %d, time: %u ms\n", RPCUINT6_CNT, ms);
	else
		printf("mbox unit6 test fail: %d, sumdata:0x%x\n", testcnt, sendbuf.sumdata);

	RPC_deinit(fd);

	return 0;
}


struct thread_t {
	int tid;
	int data;
	const char *path;
};

static int uint7_r[3] = {0};

static void *mbox_uint7thread(void *arg)

{
	struct thread_t *p = (struct thread_t *)arg;
	struct mbox_uint sendbuf;
	int testcnt = RPCUINT7_CNT;
	int fd = 0;
	uint32_t rdata, sumdata, i;

	fd =  RPC_init(p->path, O_RDWR, 0);
	while (testcnt--) {
		memset(&sendbuf, 0x0, sizeof(sendbuf));
		rdata = rand() % 0xff - 1;
		sendbuf.uintcmd = 0x7;
		sumdata = 0;
		if (rdata % 2 == 0)
			sendbuf.data[0] = 0x71;
		else
			sendbuf.data[0] = 0x7;
		for (i = 1; i < RPCUINT_SIZE; i++) {
			sendbuf.data[i] = rand() % 0xff - 1;
			sumdata += sendbuf.data[i];
		}
		sumdata +=sendbuf.data[0];
		sendbuf.sumdata = sumdata;
		RPC_invoke(fd, MBX_CMD_RPCUINT_TESTA, &sendbuf, sizeof(sendbuf));
		if (sendbuf.sumdata != sumdata - 1) {
			printf("thread-%d test fail\n", p->tid);
			uint7_r[p->tid] = -1;
			break;
		}
	}
	RPC_deinit(fd);

	if (uint7_r[p->tid] == 0) {
		uint7_r[p->tid] = 1;
		printf("thread-%d test pass\n", p->tid);
	}
	return NULL;
}

int mbox_uint7(const char *path) {
	int testcnt = RPCUINT7_CNT * 100;
	pthread_t test_thread_1;
	pthread_t test_thread_2;
	pthread_t test_thread_3;
	struct thread_t *p1;
	struct thread_t *p2;
	struct thread_t *p3;
	int ret = 0;
	int st = 0;


	TIC;
	p1 = malloc(sizeof(struct thread_t));
	if (!p1) {
		printf("mbox_uint7 cannot alloc mem!\n");
		ret = 1;
		goto err;
	}
	p1->tid = 0x0;
	p1->path = path;
	ret = pthread_create(&test_thread_1, NULL, mbox_uint7thread, p1);
	if (ret) {
		printf("thread create error...\n");
		ret = 1;
		goto err1;
	}
	p2 = malloc(sizeof(struct thread_t));
	p2->tid = 0x1;
	p2->path = path;
	if (!p2) {
		printf("mbox_uint7 cannot alloc mem!\n");
		ret = 1;
		goto err1;
	}
	ret = pthread_create(&test_thread_2, NULL, mbox_uint7thread, p2);
	if (ret) {
		printf("thread create error...\n");
		ret = 1;
		goto err2;
	}
	p3 = malloc(sizeof(struct thread_t));
	p3->tid = 0x2;
	p3->path = path;
	if (!p2) {
		printf("mbox_uint7 cannot alloc mem!\n");
		ret = 1;
		goto err2;
	}
	ret = pthread_create(&test_thread_3, NULL, mbox_uint7thread, p3);
	if (ret) {
		printf("thread create error...\n");
		ret = 1;
		goto err3;
	}

	while (testcnt--) {
		if ((uint7_r[0] == 1) && (uint7_r[1] == 1) &&(uint7_r[2] == 1)) {
			st = 0xff;
			break;
		}
		usleep(10*1000);
	}
	TOC;

	if (st == 0xff)
		printf("mbox unit7 test pass: %d, time: %u ms\n", RPCUINT7_CNT, ms);
	else
		printf("mbox unit7 test fail: %d, time: %u ms\n", RPCUINT7_CNT, ms);

err3:
	free(p3);
err2:
	free(p2);
err1:
	free(p1);
err:
	return ret;
}

int mbox_uint89(const char *path, int flag) {
	struct mbox_uint sendbuf;
	int fd =  RPC_init(path, O_RDWR, 0);

	memset(&sendbuf, 0x0, sizeof(sendbuf));
	sendbuf.sumdata =  flag;
	sendbuf.uintcmd = 0x89;
	RPC_invoke(fd, MBX_CMD_RPCUINT_TESTA, &sendbuf, sizeof(sendbuf));
	if (sendbuf.sumdata != 0x89) {
		printf("mbox unit89 start or stop dsplooptest fail, sumdata:0x%x\n", sendbuf.sumdata);
	} else {
		printf("mbox unit89 start or stop dsplooptest ok, sumdata:0x%x\n", sendbuf.sumdata);
	}

	RPC_deinit(fd);

	return 0;
}

int do_cmd(int cmd, int digit)
{
	int fd;
	char path[64];

	switch (cmd) {
	case CMD_HIFI_DSPA:
		sprintf(path, "/dev/dsp_dev");
	break;
	case CMD_HIFI_DSPB:
		sprintf(path, "/dev/dspb_dev");
	break;
	default:
	break;
	}

	if (dsp_dev_is_exist(path) < 0)
		return -1;
	/*
	 * 1, open /dev/dsp_dev or /dev/dspb_dev
	 * 2, access right, support rw
	 * 3, access group, current not use
	 * */

	switch (digit) {
	case '1':
		fd = RPC_init(path, O_RDWR, 0);
		RPC_invoke(fd, MBX_TEST_DEMO, path, sizeof(path));
		printf("rpc uint case0 %s\n", path);
	break;
	case '2':
		fd = RPC_init(path, O_RDWR, 0);
		for (;;) {
			/*
			 * 1, fd get from RPC_init
			 * 2, MBX_LISTEN_DEMO cmd defined in ipc_cmd_type.h,
			 *    use to recognize handle thing, rtos also need
			 *    add this cmd
			 * 3, s, buffer base addr
			 * 4, size, max size is 240 bytes
			 * */
			RPC_invoke(fd, MBX_LISTEN_DEMO, path, sizeof(path));
			printf("Data %s\n", path);
		}
	break;
	case '3':
		mbox_uint3(path);
	break;
	case '4':
		mbox_uint4(path);
	break;
	case '5':
		mbox_uint5(path);
	break;
	case '6':
		mbox_uint6(path);
	break;
	case '7':
		mbox_uint7(path);
	break;
	case '8':
		mbox_uint89(path, 1);
	break;
	case '9':
		mbox_uint89(path, 0);
	break;
	default:
	break;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int c;
	int digit_optind = 0;
	int cmd = 0;
	int option_index;

	if (argc < 2) {
		show_usage();
		return -1;
	}

	while (1) {
		c = getopt_long(argc, argv, opt_string,
				long_opts, &option_index);
		if (c < 0)
			break;

		switch (c) {
		case 'd':
			printf("dspname %s\n", optarg);
			if (!strcmp(optarg, "hifi4a")) {
				cmd |= CMD_HIFI_DSPA;
			} else if (!strcmp(optarg, "hifi4b")) {
				cmd |= CMD_HIFI_DSPB;
			} else {
				printf("not support dsp name\n");
				show_usage();
				return 0;
			}
		break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (digit_optind != 0) {
				printf("two digit\n");
				break;
			}
			printf("option %c\n", c);
			digit_optind = c;
		break;
		case 'h':
			show_usage();
			return 0;
		break;
		default:
			show_usage();
			return -1;
		break;
		};
	}

	do_cmd(cmd, digit_optind);
	return 0;
}
