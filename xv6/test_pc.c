#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"
#include "umutex.h"

#define NBUF 8
#define PER_PRODUCER 200

static int buf[NBUF];
static int in = 0;
static int out = 0;
static int count = 0;

static int consumed = 0;

static umutex_t m;

void
producer(void *arg)
{
  int base = (int)arg;
  int i;

  for(i = 0; i < PER_PRODUCER; i++){
    int item = base + i;

    while(1){
      mutex_lock(&m);

      if(count < NBUF){
        buf[in] = item;
        in = (in + 1) % NBUF;
        count++;
        mutex_unlock(&m);
        break;
      }

      mutex_unlock(&m);
      thread_yield();
    }

    thread_yield();
  }
}

void
consumer(void *arg)
{
  int item;

  while(consumed < 400){
    mutex_lock(&m);

    if(count > 0){
      item = buf[out];
      out = (out + 1) % NBUF;
      count--;
      consumed++;

      if(consumed % 100 == 0)
        printf(1, "consumer got %d items (last=%d)\n", consumed, item);
    }

    mutex_unlock(&m);
    thread_yield();
  }
}

int
main(void)
{
  int t1, t2, tc;

  thread_init();
  mutex_init(&m);

  t1 = thread_create(producer, (void*)100000);
  t2 = thread_create(producer, (void*)200000);
  tc = thread_create(consumer, 0);

  thread_join(t1);
  thread_join(t2);
  thread_join(tc);

  printf(1, "test_pc: done\n");
  exit();
}
