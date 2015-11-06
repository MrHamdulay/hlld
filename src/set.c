#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <assert.h>
#include "set.h"
#include "serialize.h"
#include "type_compat.h"

/*
 * Generates the folder name, given a set name.
 */
static const char* SET_FOLDER_NAME = "hlld.%s";

/**
 * Format for the data file names.
 */
static const char* DATA_FILE_NAME = "registers.mmap";

/*
 * Generates the config file name
 */
static const char* CONFIG_FILENAME = "config.ini";

/*
 * Static delarations
 */
static int thread_safe_fault(hlld_set *f);
static int timediff_msec(struct timeval *t1, struct timeval *t2);

static int filter_out_special(CONST_DIRENT_T *d);

// Link the external murmur hash in
extern void MurmurHash3_x64_128(const void * key, const int len, const uint32_t seed, void *out);

/**
 * Initializes a set wrapper.
 * @arg config The configuration to use
 * @arg set_name The name of the set
 * @arg discover Should existing data files be discovered. Otherwise
 * they will be faulted in on-demand.
 * @arg set Output parameter, the new set
 * @return 0 on success
 */
int init_set(hlld_config *config, char *set_name, int discover, hlld_set **set) {
    // Allocate the buffers
    hlld_set *s = *set = calloc(1, sizeof(hlld_set));

    // Initialize
    s->is_dirty = 1;
    s->is_proxied = 1;

    // Store the things
    s->config = config;
    s->set_name = strdup(set_name);

    // Copy set configs
    s->set_config.default_eps = config->default_eps;
    s->set_config.default_precision = config->default_precision;
    s->set_config.in_memory = config->in_memory;
    s->set_config.sliding_period = config->sliding_period;
    s->set_config.sliding_precision = config->sliding_precision;

    // Get the folder name
    char *folder_name = NULL;
    int res;
    res = asprintf(&folder_name, SET_FOLDER_NAME, s->set_name);
    assert(res != -1);

    // Compute the full path
    s->full_path = join_path(config->data_dir, folder_name);
    free(folder_name);

    // Initialize the locks
    INIT_HLLD_SPIN(&s->hll_update);
    pthread_mutex_init(&s->hll_lock, NULL);

    // Try to create the folder path
    res = mkdir(s->full_path, 0755);
    if (res && errno != EEXIST) {
        syslog(LOG_ERR, "Failed to create set directory '%s'. Err: %d [%d]", s->full_path, res, errno);
        return res;
    }

    // Read in the set_config
    char *config_name = join_path(s->full_path, (char*)CONFIG_FILENAME);
    res = set_config_from_filename(config_name, &s->set_config);
    free(config_name);
    if (res && res != -ENOENT) {
        syslog(LOG_ERR, "Failed to read set '%s' configuration. Err: %d [%d]", s->set_name, res, errno);
        return res;
    }

    // Discover the existing set if we need to
    res = 0;
    if (discover) {
        res = thread_safe_fault(s);
        if (res) {
            syslog(LOG_ERR, "Failed to fault in the set '%s'. Err: %d", s->set_name, res);
            printf("Failed to fault in the set '%s'. Err: %d", s->set_name, res);
        }
    }

    // Trigger a flush on first instantiation. This will create
    // a new ini file for first time sets.
    if (!res) {
        res = hset_flush(s);
    }

    return res;
}

/**
 * Destroys a set
 * @arg set The set to destroy
 * @return 0 on success
 */
int destroy_set(hlld_set *set) {
    // Close first
    hset_close(set);

    // Cleanup
    free(set->set_name);
    free(set->full_path);
    free(set);
    return 0;
}

/**
 * Gets the counters that belong to a set
 * @notes Thread safe, but may be inconsistent.
 * @arg set The set
 * @return A reference to the counters of a set
 */
set_counters* hset_counters(hlld_set *set) {
    return &set->counters;
}

/**
 * Checks if a set is currectly mapped into
 * memory or if it is proxied.
 * @notes Thread safe.
 * @return 0 if in-memory, 1 if proxied.
 */
int hset_is_proxied(hlld_set *set) {
    return set->is_proxied;
}

/**
 * Flushes the set. Idempotent if the
 * set is proxied or not dirty.
 * @arg set The set to close
 * @return 0 on success.
 */
int hset_flush(hlld_set *set) {
    // Only do things if we are non-proxied
    if (set->is_proxied)
        return 0;

    // Time how long this takes
    struct timeval start, end;
    gettimeofday(&start, NULL);

    // If we are not dirty, nothing to do
    if (!set->is_dirty)
        return 0;

    // Store our properties for a future unmap
    set->set_config.size = hset_size_total(set);

    // Write out set_config
    char *config_name = join_path(set->full_path, (char*)CONFIG_FILENAME);
    char *serialized_name = join_path(set->full_path, (char*)DATA_FILE_NAME);
    int res = update_filename_from_set_config(config_name, &set->set_config);
    free(config_name);
    if (res) {
        syslog(LOG_ERR, "Failed to write set '%s' configuration. Err: %d.",
                set->set_name, res);
    }

    // Turn dirty off
    set->is_dirty = 0;

    // Flush the set
    res = 0;
    if (!set->set_config.in_memory) {
        res = serialize_hll_to_filename(serialized_name, &set->hll);
    }
    free(serialized_name);

    // Compute the elapsed time
    gettimeofday(&end, NULL);
    syslog(LOG_DEBUG, "Flushed set '%s'. Total time: %d msec.",
            set->set_name, timediff_msec(&start, &end));
    return res;
}

/**
 * Gracefully closes a set.
 * @arg set The set to close
 * @return 0 on success.
 */
int hset_close(hlld_set *set) {
    // Acquire lock
    pthread_mutex_lock(&set->hll_lock);

    // Only act if we are non-proxied
    if (!set->is_proxied) {
        hset_flush(set);
        hll_destroy(&set->hll);
        set->is_proxied = 1;
        set->counters.page_outs += 1;
    }

    // Release lock
    pthread_mutex_unlock(&set->hll_lock);
    return 0;
}

/**
 * Deletes the set with extreme prejudice.
 * @arg set The set to delete
 * @return 0 on success.
 */
int hset_delete(hlld_set *set) {
    // Close first
    hset_close(set);

    // Delete the files
    struct dirent **namelist = NULL;
    int num;

    // Filter only data dirs, in sorted order
    num = scandir(set->full_path, &namelist, filter_out_special, NULL);
    syslog(LOG_INFO, "Deleting %d files for set %s.", num, set->set_name);

    // Free the memory associated with scandir
    for (int i=0; i < num; i++) {
        char *file_path = join_path(set->full_path, namelist[i]->d_name);
        if (unlink(file_path)) {
            syslog(LOG_ERR, "Failed to delete: %s. %s", file_path, strerror(errno));
        }
        free(file_path);
    }

    // Free the memory associated with scandir
    for (int i=0; i < num; i++) {
        free(namelist[i]);
    }
    if (namelist)
        free(namelist);

    // Delete the directory
    if (rmdir(set->full_path)) {
        syslog(LOG_ERR, "Failed to delete: %s. %s", set->full_path, strerror(errno));
    }

    return 0;
}

/**
 * Adds a key to the given set
 * @arg set The set to add to
 * @arg key The key to add
 * @return 0 on success.
 */
int hset_add(hlld_set *set, char *key) {
    if (set->is_proxied) {
        if (thread_safe_fault(set) != 0) return -1;
    }

    // Compute the hash value of the key. We do this
    // so that we can use the hll_add_hash instead of
    // hll_add. This way, the expensive CPU bit can
    // be done without holding a lock
    uint64_t out[2];
    MurmurHash3_x64_128(key, strlen(key), 0, &out);

    // Add the hashed value and update the
    // counters
    LOCK_HLLD_SPIN(&set->hll_update);
    hll_add_hash(&set->hll, out[1]);
    set->counters.sets += 1;
    UNLOCK_HLLD_SPIN(&set->hll_update);

    // Mark as dirty
    set->is_dirty = 1;
    return 0;
}

/**
 * Gets the size of the set
 * @note Thread safe.
 * @arg set The set to check
 * @return The estimated size of the set
 */
uint64_t hset_size_total(hlld_set *set) {
    if (!set->is_proxied) {
        return hll_size_total(&set->hll);
    } else {
        return set->set_config.size;
    }
}

uint64_t hset_size(hlld_set *set, uint64_t time_window, time_t current_time) {
    if (!set->is_proxied) {
        return hll_size(&set->hll, (int)time_window, current_time);
    } else {
        return set->set_config.size;
    }
}

/**
 * Gets the byte size of the set
 * @note Thread safe.
 * @arg set The set
 * @return The total byte size of the set
 */
uint64_t hset_byte_size(hlld_set *set) {
    if (set->bm.size)
        return set->bm.size;
    return hll_bytes_for_precision(set->set_config.default_precision);
}

/**
 * Provides a thread safe faulting of the set.
 */
static int thread_safe_fault(hlld_set *s) {
    // Acquire lock
    int res = 0;
    char *bitmap_path = NULL;
    pthread_mutex_lock(&s->hll_lock);

    // Bail if we already faulted in
    if (!s->is_proxied)
        goto LEAVE;

    // Get the mode for our bitmap
    bitmap_mode mode;
    if (s->set_config.in_memory) {
        mode = ANONYMOUS;

        s->is_proxied = 0;
        res = hll_init(
                s->set_config.default_precision,
                s->set_config.sliding_period, 
                s->set_config.sliding_precision,
                &s->hll); 
        // Skip the fault in
        goto LEAVE;

    } else {
        //mode = PERSISTENT;
        // XXX We no longer have a persistent mode
        mode = SHARED;
    }

    // Get the full path to the bitmap
    bitmap_path = join_path(s->full_path, (char*)DATA_FILE_NAME);

    // Check if the register file exists
    struct stat buf;
    res = stat(bitmap_path, &buf);
    if(res == 0) {
        syslog(LOG_ERR, "Discovered HLL set: %s.", bitmap_path);
    }

    // Handle if the file exists and contains data (read existing hll)
    if (res == 0 && buf.st_size != 0) {
        syslog(LOG_ERR, "Discovered HLL set: %s.", bitmap_path);
        res = unserialize_hll_from_filename(bitmap_path, &s->hll);
        //res = bitmap_from_filename(bitmap_path, buf.st_size, 0, mode, &s->bm);
        if (res) {
            syslog(LOG_ERR, "Failed to load bitmap: %s. %s", bitmap_path, strerror(errno));
            goto LEAVE;
        }

        s->is_proxied = 0;
        // Increase our page ins
        s->counters.page_ins += 1;

    // Handle if it doesn't exist (create the file)
    } else if ((res == -1 && errno == ENOENT) || (res == 0 && buf.st_size == 0)) {
        // We no longer need to create a file before serializing
        s->is_proxied = 0;
        res = hll_init(
                s->set_config.default_precision,
                s->set_config.sliding_period, 
                s->set_config.sliding_precision,
                &s->hll); 
    // Handle any other error
    } else {
        syslog(LOG_ERR, "Failed to query the register file for: %s. %s", bitmap_path, strerror(errno));
        goto LEAVE;
    }

LEAVE:
    // Release lock
    pthread_mutex_unlock(&s->hll_lock);

    // Free the bitmap path if any
    if (bitmap_path) free(bitmap_path);
    return res;
}

/**
 * Works with scandir to filter out special files
 */
static int filter_out_special(CONST_DIRENT_T *d) {
    // Get the file name
    char *name = (char*)d->d_name;

    // Make sure its not speci
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return 0;
    }
    return 1;
}

/**
 * Computes the difference in time in milliseconds
 * between two timeval structures.
 */
static int timediff_msec(struct timeval *t1, struct timeval *t2) {
    uint64_t micro1 = t1->tv_sec * 1000000 + t1->tv_usec;
    uint64_t micro2= t2->tv_sec * 1000000 + t2->tv_usec;
    return (micro2-micro1) / 1000;
}

