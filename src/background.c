#include <unistd.h>
#include "background.h"

static void* flush_thread_main(void *in);
static void* unmap_thread_main(void *in);
typedef struct {
    hlld_config *config;
    hlld_setmgr *mgr;
    int *should_run;
} background_thread_args;

/**
 * Helper macro to pack and unpack the arguments
 * to the thread, and free the memory.
 */
# define PACK_ARGS() {                  \
    args = malloc(sizeof(background_thread_args));  \
    args->config = config;              \
    args->mgr = mgr;                    \
    args->should_run = should_run;      \
}
# define UNPACK_ARGS() {                \
    background_thread_args *args = in;  \
    config = args->config;              \
    mgr = args->mgr;                    \
    should_run = args->should_run;      \
    free(args);                         \
}

/**
 * Starts a flushing thread which on every
 * configured flush interval, flushes all the sets.
 * @arg config The configuration
 * @arg mgr The manager to use
 * @arg should_run Pointer to an integer that is set to 0 to
 * indicate the thread should exit.
 * @arg t The output thread
 * @return 1 if the thread was started
 */
int start_flush_thread(hlld_config *config, hlld_setmgr *mgr, int *should_run, pthread_t *t) {
    // Return if we are not scheduled
    if(config->flush_interval <= 0) {
        return 0;
    }

    // Start thread
    background_thread_args *args;
    PACK_ARGS();
    pthread_create(t, NULL, flush_thread_main, args);
    return 1;
}

/**
 * Starts a cold unmap thread which on every
 * cold interval unamps cold sets.
 * @arg config The configuration
 * @arg mgr The manager to use
 * @arg should_run Pointer to an integer that is set to 0 to
 * indicate the thread should exit.
 * @arg t The output thread
 * @return 1 if the thread was started
 */
int start_cold_unmap_thread(hlld_config *config, hlld_setmgr *mgr, int *should_run, pthread_t *t) {
    // Return if we are not scheduled
    if(config->cold_interval <= 0) {
        return 0;
    }

    // Start thread
    background_thread_args *args;
    PACK_ARGS();
    pthread_create(t, NULL, unmap_thread_main, args);
    return 1;
}


static void* flush_thread_main(void *in) {
    hlld_config *config;
    hlld_setmgr *mgr;
    int *should_run;
    UNPACK_ARGS();

    // Perform the initial checkpoint with the manager
    setmgr_client_checkpoint(mgr);

    syslog(LOG_INFO, "Flush thread started. Interval: %d seconds.", config->flush_interval);
    unsigned int ticks = 0;
    while (*should_run) {
        sleep(1);
        setmgr_client_checkpoint(mgr);
        if ((++ticks % config->flush_interval) == 0 && *should_run) {
            // List all the sets
            syslog(LOG_INFO, "Scheduled flush started.");
            hlld_set_list_head *head;
            int res = setmgr_list_sets(mgr, &head);
            if (res != 0) {
                syslog(LOG_WARNING, "Failed to list sets for flushing!");
                continue;
            }

            // Flush all, ignore errors since
            // sets might get deleted in the process
            hlld_set_list *node = head->head;
            while (node) {
                setmgr_flush_set(mgr, node->set_name);
                node = node->next;
            }

            // Cleanup
            setmgr_cleanup_list(head);
        }
    }
    return NULL;
}

static void* unmap_thread_main(void *in) {
    hlld_config *config;
    hlld_setmgr *mgr;
    int *should_run;
    UNPACK_ARGS();

    // Perform the initial checkpoint with the manager
    setmgr_client_checkpoint(mgr);

    syslog(LOG_INFO, "Cold unmap thread started. Interval: %d seconds.", config->cold_interval);
    unsigned int ticks = 0;
    while (*should_run) {
        sleep(1);
        setmgr_client_checkpoint(mgr);
        if ((++ticks % config->cold_interval) == 0 && *should_run) {
            // List the cold sets
            syslog(LOG_INFO, "Cold unmap started.");
            hlld_set_list_head *head;
            int res = setmgr_list_cold_sets(mgr, &head);
            if (res != 0) {
                continue;
            }

            // Close the sets, save memory
            syslog(LOG_INFO, "Cold set count: %d", head->size);
            hlld_set_list *node = head->head;
            while (node) {
                syslog(LOG_INFO, "Unmapping set '%s' for being cold.", node->set_name);
                setmgr_unmap_set(mgr, node->set_name);
                node = node->next;
            }

            // Cleanup
            setmgr_cleanup_list(head);
        }
    }
    return NULL;
}


