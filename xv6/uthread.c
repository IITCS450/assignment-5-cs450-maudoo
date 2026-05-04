#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"

enum { T_UNUSED, T_RUNNABLE, T_RUNNING, T_ZOMBIE };

#define MAX_THREADS 16
#define STACK_SIZE 4096

struct thread {
  tid_t tid;
  int state;
  void *stack;
  void *sp;
  void (*fn)(void *);
  void *arg;
};

static struct thread threads[MAX_THREADS];
static int current = 0;
static int initialized = 0;

static void thread_stub(void);
static int pick_next(void);

void
thread_init(void)
{
  int i;

  for(i = 0; i < MAX_THREADS; i++){
    threads[i].tid = i;
    threads[i].state = T_UNUSED;
    threads[i].stack = 0;
    threads[i].sp = 0;
    threads[i].fn = 0;
    threads[i].arg = 0;
  }

  threads[0].state = T_RUNNING;
  current = 0;
  initialized = 1;
}

tid_t
thread_create(void (*fn)(void*), void *arg)
{
  int i;
  char *stack;
  uint *sp;

  if(!initialized)
    thread_init();

  for(i = 1; i < MAX_THREADS; i++){
    if(threads[i].state == T_UNUSED){
      stack = malloc(STACK_SIZE);
      if(stack == 0)
        return -1;

      sp = (uint *)(stack + STACK_SIZE);

      *--sp = (uint)thread_stub;
      *--sp = 0;
      *--sp = 0;
      *--sp = 0;
      *--sp = 0;

      threads[i].state = T_RUNNABLE;
      threads[i].stack = stack;
      threads[i].sp = sp;
      threads[i].fn = fn;
      threads[i].arg = arg;

      return threads[i].tid;
    }
  }

  return -1;
}

void
thread_yield(void)
{
  int prev, next;

  if(!initialized)
    thread_init();

  prev = current;
  next = pick_next();

  if(next == prev)
    return;

  if(threads[prev].state == T_RUNNING)
    threads[prev].state = T_RUNNABLE;

  threads[next].state = T_RUNNING;
  current = next;

  uswtch((struct context **)&threads[prev].sp,
         (struct context *)threads[next].sp);
}

int
thread_join(tid_t tid)
{
  if(!initialized)
    thread_init();

  if(tid <= 0 || tid >= MAX_THREADS)
    return -1;

  while(threads[tid].state != T_ZOMBIE){
    if(threads[tid].state == T_UNUSED)
      return -1;
    thread_yield();
  }

  if(threads[tid].stack)
    free(threads[tid].stack);

  threads[tid].state = T_UNUSED;
  threads[tid].stack = 0;
  threads[tid].sp = 0;
  threads[tid].fn = 0;
  threads[tid].arg = 0;

  return 0;
}

static void
thread_stub(void)
{
  int tid = current;

  threads[tid].fn(threads[tid].arg);
  threads[tid].state = T_ZOMBIE;
  thread_yield();

  while(1)
    ;
}

static int
pick_next(void)
{
  int i;

  for(i = 1; i <= MAX_THREADS; i++){
    int idx = (current + i) % MAX_THREADS;
    if(threads[idx].state == T_RUNNABLE)
      return idx;
  }

  return current;
}
