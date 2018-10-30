#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <gmp.h>
#include <linux/unistd.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sched.h>
#include "../linux-3.10-artik/arch/arm/include/asm/unistd.h"
#include "prime.h"

#define MIN_WEIGHT 1
#define MAX_WEIGHT 20

#define SCHED_NORMAL		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_BATCH		3
#define SCHED_IDLE		5
#define SCHED_WRR		6

static struct timeval start_time;
static void check_primes(mpz_t base);

static int is_number(const char *string)
{
	int i;

	if (string == NULL)
		return 0;

	for (i = 0; string[i] != '\0'; ++i) {
		if (!isdigit(string[i]))
			return 0;
	}
	return 1;
}

static int is_valid_weight(const char *weight)
{
	if (!is_number(weight))
		return 0;

	int wt = atoi(weight);
	if (wt >= MIN_WEIGHT && wt <= MAX_WEIGHT)
		return 1;
	return 0;
}

static char *get_policy_name(int policy)
{
	char *result = calloc(100, sizeof(char));
	if (result == NULL) {
		printf("Memory allocation fault");
		exit(1);
	}

	switch (policy) {
		case SCHED_NORMAL: {
			char *s = "CFS: Normal";
			strncpy(result, s, strlen(s));
			break;
		}
		case SCHED_FIFO: {
			char *s = "FIFO";
			strncpy(result, s, strlen(s));
			break;
		}
		case SCHED_RR: {
			char *s = "Round Robin";
			strncpy(result, s, strlen(s));
			break;
		}
		case SCHED_BATCH: {
			char *s = "Batch";
			strncpy(result, s, strlen(s));
			break;
		}
		case SCHED_IDLE: {
			char *s = "Idle";
			strncpy(result, s, strlen(s));
			break;
		}
		case SCHED_WRR: {
			char *s = "Weighted Round Robin";
			strncpy(result, s, strlen(s));
			break;
		}
		default: {
			char *s = "Unknown";
			strncpy(result, s, strlen(s));
			break;
		}
	}
	return result;
}

static void start_timer()
{
	int ret;
	ret = gettimeofday(&start_time, NULL);
	if (ret != 0)
		return perror("Timer error");
}

static double stop_timer()
{
	int ret;
	struct timeval now, diff;

	ret = gettimeofday(&now, NULL);
	if (ret != 0) {
		perror("Timer stop error");
		return -1;
	}

	diff.tv_usec = now.tv_usec - start_time.tv_usec;
	diff.tv_sec = now.tv_sec - start_time.tv_sec;

	return diff.tv_sec + (diff.tv_usec / pow(10, 6));
}

static void print_scheduler()
{
	int ret;
	char *policy;
	pid_t pid = getpid();

	if (pid < 0) {
		perror("Process calling error");
		exit(-1);
	}

	ret = sched_getscheduler(pid);

	if (ret < 0) {
		perror("Scheduler calling error");
		exit(-1);
	}

	policy = get_policy_name(ret);
	printf("Current Policy: %s\n", policy);
	free(policy);
}

static void print_weight(pid_t pid)
{
	int ret;
	if (pid < 0)
		return;
	ret = syscall(381, pid);
	if (ret < 0)
		return perror("Getting weight error");
	printf("Process %d weight : %d\n", pid, ret);
}

static void print_current_weight()
{
	pid_t pid = getpid();
	if (pid < 0) {
		perror("Process calling error");
		exit(-1);
	}
	print_weight(pid);
}

static void set_weight(pid_t pid, int weight)
{
	int ret;
	if (pid < 0)
		return;
	ret = syscall(380, pid, weight);
	if (ret < 0)
		return perror("Setting weight error");
	printf("Process %d weight is now updated to %d\n", pid, weight);
}

static void scheduler_table(mpz_t number)
{
	double run_time = 0;
	int i = 0;
	FILE *file = fopen("results.txt", "w");
	for (i = MIN_WEIGHT; i <= MAX_WEIGHT; i++) {
		set_weight(0, i);
		printf("Weight : %d\n", i);
		start_timer();
		check_primes(number);
		run_time = stop_timer();
		printf("Found factors in %f secs\n", run_time);
		fprintf(file, "%d,%f\n", i, run_time);
	}
	printf("Test completed.\n");
	fclose(file);
}

static void test(mpz_t number, const char *weight_string)
{
	double run_time;
	char c;
	int wt = atoi(weight_string);
	printf("Input Weight : %d\n", wt);
	printf("Process : %d\n", getpid());

	print_scheduler();
	print_current_weight();

	set_weight(0, wt);
	print_current_weight();

	start_timer();
	check_primes(number);
	run_time = stop_timer();
	printf("Factorization completed in %f seconds.\n", run_time);

	printf("Do you want to do automatic weight testing? Y/N:");
	c = getchar();
	if (c == 'Y' || c == 'y')
		scheduler_table(number);
	else
		printf("Okay. Done\n");

}

int main(int argc, const char *argv[])
{
	mpz_t largenum;

	if (argc < 3) {
		printf("Usage: %s <number> <weight>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (mpz_init_set_str(largenum, argv[1], 10) < 0) {
		perror("Invalid number");
		return EXIT_FAILURE;
	}

	if (!is_valid_weight(argv[2])) {
		printf("Invalid Weight. Only integers 1-20 inclusive allowed\n");
		return EXIT_FAILURE;
	}

	setvbuf(stdout, NULL, _IONBF, 0);
	test(largenum, argv[2]);
	return EXIT_SUCCESS;
}

static void check_primes(mpz_t base)
{
	char *str;
	int res;
	mpz_t i, half, two;

	mpz_init_set_str(two, "2", 10);
	mpz_init_set_str(i, "2", 10);
	mpz_init(half);
	mpz_cdiv_q(half, base, two);

	str = mpz_to_str(base);
	if (!str)
		return;

	res = mpz_probab_prime_p(base, 10);
	if (res) {
		printf("%s is a prime number\n", str);
		free(str);
		return;
	}

	printf("Trial: prime factors for %s are:", str);
	free(str);
	do {
		if (mpz_divisible_p(base, i) && verify_is_prime(i)) {
			str = mpz_to_str(i);
			if (!str)
				return;
			printf(" %s", str);
			free(str);
		}

		mpz_nextprime(i, i);
	} while (mpz_cmp(i, half) <= 0);
	printf("\n");
}
