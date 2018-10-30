#os-team20

## How to Build
$ ./build

to build the test code, type
```sh
$ arm-linux-gnueabi-gcc -I /include test.c -o test
```
```sh
$ sdb root on
```
```sh
$ sdb push test /root/test
```

in debug console
```sh
$ direct_set_debug.sh --sdb-set
```
```sh
$ ./test <number> <weight>  weight for 1 to 20
```
  eg> ./test 1000001 1
```sh
$ sdb pull /root/results.txt results
```
## High-Level Design & implementation
  1. register system call sched_getweight, sched_setweight
  2. init_task.h, kernel/kthread.c 을 통해 기본 스케줄려를 SCHED_WRR로 바꿈.
  3. wrr.c을 통해 scheduler를 구현하고 makefile에 등록함.
  4. sched.h를 통해 wrr_scheduler를 위한 구조체를 형성 및 선언.  
  5. use a test investigate implement. finally in debug device result file is stored. results.txt파일에 저장됩니다.

## Investigate

Number = 1000001
weight, execute time  
1,0.706776
2,0.696349
3,0.696559
4,0.694756
5,0.705340
6,0.695877
7,0.696914
8,0.705821
9,0.695844
10,0.700756
11,0.711483
12,0.705468
13,0.695812
14,0.704239
15,0.705817
16,0.704742
17,0.695261
18,0.705653
19,0.704964
20,0.695133

Can see graph in plot.pdf
  원래 이 숫자들은 weight가 증가함에 따라, execution time은 감소해야한다. 하지만 현재 결과값은 그런 사실을 보여주지 못하고있다. 다양한 측면에서 이유를 찾아봤지만 특별한 이유가 보이지 않았다. 특히 time slice가 정확히 setting되어있기 때문에 이를 WRR자체의 문제라기보다 다른 setting문제라고 생각된다. 

## Improve the WRR scheduler

WRR scheduler에서는 worst case latency가 queue의 갯수에 따라 증가하게 된다. 
u_i = fraction wieght of each que such that sum of weight of all task.
u_i는 connection의 갯수가 증가할수록 작아지게 된다.by w_i = u_i*M.    w_i 는 각 큐의 무게합. WRR에서는 M이라는 Constant을 곱함으로 que가 증가할수록 u_i는 감소하고 w_i도 감소하게 된다. 
$ LATENCY_i = W(1-u_i)L_i/r 
이러한 단점을 LOW LATENCY WRR SCHEDULER로 극복할 수 있다. 이는 M과 같이 constant을 곱해서 weight을 구하는것이 아니라, connection의 갯수가 증가함에 따라 감소하는 변수를 두어서 이러한 문제를 해결한다. connection의 갯수가 증가함으로써 total latency을 증가시키는데 이는 N이 증가할수록 r이라는 변수가 더 급격하게 줄어들길떄문제 전체 weight의 합은 감소됨을 이용하는 것이다. r= r_0*(1-k(N/N_max)^2)^(1/2)와 같은 변수를 u_i에 곱해서 weight을 산출한다.

## Lessons Learned
* custom scheduler를 구현하는 방법을 알게되었다.
* test파일을 작성하면서 prime factorization naive tiral division method에 대해 알게되었다.


## How To Register System Call
### 1. Increment the number of System calls
in file: "arch/arm/include/asm/unistd.h"
``` c
#define __NR_syscalls  (N)
```
to
```c
#define __NR_syscalls  (N+4)
```
Total number of system calls must be a multiplication of 4.

### 2. assign system call number
in file: "arch/arm/include/uapi/asm/unistd.h"
add
```c
#define __NR_myfunc      (__NR_SYSCALL_BASE+ #) 
```

### 3. make asmlinkage function
in file: "include/linux/syscalls.h"
```c
asmlinkage int my_func()  // if no parameter then write 'void' 
```

### 4. add to system call table
in file: "arch/arm/kernel/calls.S"
```
call(sys_myfunc)
```

### 5. Revise Makefile
in file: "kernel/Makefile"
```
obj -y = ...  ptree.o
```

### etc.
in file: "kernel/myfunc.c"  
the name of function must be sys_myfunc()
