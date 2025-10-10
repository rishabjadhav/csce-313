#include "threading.h"

void t_init()
{
        // Define contexts, set state to INVALID, and context to null/default
        for (int i = 0; i < NUM_CTX; i++) {
                contexts[i].state = INVALID;
                memset(&contexts[i].context, 0, sizeof(ucontext_t));
        }

        // Current context index is 0, we can start saving snapshots there.
        current_context_idx = 0;

        // Initialize main thread
        // Calling getcontext() on a pointer copies the context of machine to the pointer provided as function argument
        getcontext(&contexts[current_context_idx].context);
        contexts[current_context_idx].state = VALID;

}

int32_t t_create(fptr foo, int32_t arg1, int32_t arg2)
{
        // Find next non-empty entry in contexts
        uint8_t idx = 19;
        for (uint8_t i = 1; i < NUM_CTX; i++) {
                if (contexts[i].state == INVALID) {
                        // found an empty entry!
                        idx = i;
                        break;
                }
        }

        // No space left in contexts
        if (idx == 19) { return 1; }

        // Init context within this open entry in contexts
        getcontext(&contexts[idx].context);

        // Allocate a new stack for our new context
        // Pointer to the top of the stack is returned in uc_stack's stack pointer now.
        contexts[idx].context.uc_stack.ss_sp = malloc(STK_SZ);

        // Set stack size
        contexts[idx].context.uc_stack.ss_size = STK_SZ;

        // Call makecontext, creating the worker task at function pointer foo, with args passed as well
        makecontext(&contexts[idx].context, (ctx_ptr)foo, 2, arg1, arg2);

        contexts[idx].state = VALID;
        return 0;
}

int32_t t_yield()
{
        // Find a valid context to yield control over to
        uint8_t next_idx = (uint8_t)((current_context_idx + 1) % NUM_CTX);
        while (contexts[next_idx].state != VALID) {
                next_idx = (uint8_t)((next_idx + 1) % NUM_CTX);
        
                if (next_idx == current_context_idx) { return -1; }
        }

        // next_idx holds a valid context's index that we can swap to now
        uint8_t prev_idx = current_context_idx;

        // this task needs to become the new task, so we must switch our current context index
        current_context_idx = next_idx;

        // Swap contexts, save current task, load new task
        swapcontext(&contexts[prev_idx].context, &contexts[next_idx].context);
        return 1;
}

void t_finish()
{
        // Free stack that was allocated for this task
        if (contexts[current_context_idx].context.uc_stack.ss_sp != NULL) {
                free(contexts[current_context_idx].context.uc_stack.ss_sp);
                contexts[current_context_idx].context.uc_stack.ss_sp = NULL;
        }

        // Reset context entry to zeroes
        memset(&contexts[current_context_idx].context, 0, sizeof(ucontext_t));

        // Find a valid context to yield control over to, now that this task is done.
        uint8_t next_idx = (uint8_t)((current_context_idx + 1) % NUM_CTX);

        // Same logic as in t_yield, but we don't swap contexts this time
        while (contexts[next_idx].state != VALID) {
                next_idx = (uint8_t)((next_idx + 1) % NUM_CTX);
        
                // No more valid contexts to switch to, go to main task
                if (next_idx == current_context_idx) {
                        next_idx = 0;
                        break;
                }
        }

        // Set this context to done
        contexts[current_context_idx].state = DONE;

        // Switch to new context or main task
        current_context_idx = next_idx;

        setcontext(&contexts[next_idx].context);
}