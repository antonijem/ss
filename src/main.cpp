#include "../h/mem.h"
#include "../h/trap.h"
#include "../h/scheduler.h"
#include "../h/syscall_c.h"
#include "../h/thread.h"

extern void userMain();

void main() {
    init_mem();
    init_trap();
    init_kthread();
    init_scheduler();
    init_timer_thread();
    init_idle_thread();
    init_console();
    userMode();



    userMain();
    shutdown();

}

