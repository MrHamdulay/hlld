#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include "config.h"
#include "set.h"
#include "set_manager.h"

START_TEST(test_mgr_init_destroy)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_mgr_create_drop)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"foo1", NULL);
    fail_unless(res == 0);

    res = setmgr_drop_set(mgr, (char*)"foo1");
    fail_unless(res == 0);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_mgr_create_double_drop)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"dub1", NULL);
    fail_unless(res == 0);

    res = setmgr_drop_set(mgr, (char*)"dub1");
    fail_unless(res == 0);

    res = setmgr_drop_set(mgr, (char*)"dub1");
    fail_unless(res == -1);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_mgr_list)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"bar1", NULL);
    fail_unless(res == 0);
    res = setmgr_create_set(mgr, (char*)"bar2", NULL);
    fail_unless(res == 0);

    hlld_set_list_head *head;
    res = setmgr_list_sets(mgr, NULL, &head);
    fail_unless(res == 0);
    fail_unless(head->size == 2);

    int has_bar1 = 0;
    int has_bar2 = 0;

    hlld_set_list *node = head->head;
    while (node) {
        if (strcmp(node->set_name, (char*)"bar1") == 0)
            has_bar1 = 1;
        else if (strcmp(node->set_name, (char*)"bar2") == 0)
            has_bar2 = 1;
        node = node->next;
    }
    fail_unless(has_bar1);
    fail_unless(has_bar2);

    res = setmgr_drop_set(mgr, (char*)"bar1");
    fail_unless(res == 0);
    res = setmgr_drop_set(mgr, (char*)"bar2");
    fail_unless(res == 0);

    setmgr_cleanup_list(head);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_mgr_list_prefix)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"bar1", NULL);
    fail_unless(res == 0);
    res = setmgr_create_set(mgr, (char*)"bar2", NULL);
    fail_unless(res == 0);
    res = setmgr_create_set(mgr, (char*)"junk1", NULL);
    fail_unless(res == 0);

    hlld_set_list_head *head;
    res = setmgr_list_sets(mgr, (char*)"bar", &head);
    fail_unless(res == 0);
    fail_unless(head->size == 2);

    int has_bar1 = 0;
    int has_bar2 = 0;

    hlld_set_list *node = head->head;
    while (node) {
        if (strcmp(node->set_name, (char*)"bar1") == 0)
            has_bar1 = 1;
        else if (strcmp(node->set_name, (char*)"bar2") == 0)
            has_bar2 = 1;
        node = node->next;
    }
    fail_unless(has_bar1);
    fail_unless(has_bar2);

    res = setmgr_drop_set(mgr, (char*)"bar1");
    fail_unless(res == 0);
    res = setmgr_drop_set(mgr, (char*)"bar2");
    fail_unless(res == 0);
    res = setmgr_drop_set(mgr, (char*)"junk1");
    fail_unless(res == 0);

    setmgr_cleanup_list(head);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST


START_TEST(test_mgr_list_no_sets)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    hlld_set_list_head *head;
    res = setmgr_list_sets(mgr, NULL, &head);
    fail_unless(res == 0);
    fail_unless(head->size == 0);
    setmgr_cleanup_list(head);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST


START_TEST(test_mgr_add_keys)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"zab1", NULL);
    fail_unless(res == 0);

    char *keys[] = {(char*)"hey",(char*)"there",(char*)"person"};
    res = setmgr_set_keys(mgr, (char*)"zab1", (char**)&keys, 3, 100);
    fail_unless(res == 0);

    res = setmgr_drop_set(mgr, (char*)"zab1");
    fail_unless(res == 0);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_mgr_add_no_set)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    char *keys[] = {(char*)"hey",(char*)"there",(char*)"person"};
    res = setmgr_set_keys(mgr, (char*)"noop1", (char**)&keys, 3, 100);
    fail_unless(res == -1);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

/* Flush */
START_TEST(test_mgr_flush_no_set)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_flush_set(mgr, (char*)"noop1");
    fail_unless(res == -1);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_mgr_flush)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"zab3", NULL);
    fail_unless(res == 0);

    res = setmgr_flush_set(mgr, (char*)"zab3");
    fail_unless(res == 0);

    res = setmgr_drop_set(mgr, (char*)"zab3");
    fail_unless(res == 0);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

/* Unmap */
START_TEST(test_mgr_unmap_no_set)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_unmap_set(mgr, (char*)"noop2");
    fail_unless(res == -1);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_mgr_unmap)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"zab4", NULL);
    fail_unless(res == 0);

    res = setmgr_unmap_set(mgr, (char*)"zab4");
    fail_unless(res == 0);

    res = setmgr_drop_set(mgr, (char*)"zab4");
    fail_unless(res == 0);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_mgr_unmap_add_keys)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"zab5", NULL);
    fail_unless(res == 0);

    res = setmgr_unmap_set(mgr, (char*)"zab5");
    fail_unless(res == 0);

    // Try to add keys now
    char *keys[] = {(char*)"hey",(char*)"there",(char*)"person"};
    res = setmgr_set_keys(mgr, (char*)"zab5", (char**)&keys, 3, 100);
    fail_unless(res == 0);

    res = setmgr_drop_set(mgr, (char*)"zab5");
    fail_unless(res == 0);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

/* Clear command */
START_TEST(test_mgr_clear_no_set)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_clear_set(mgr, (char*)"noop2");
    fail_unless(res == -1);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_mgr_clear_not_proxied)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"dub1", NULL);
    fail_unless(res == 0);

    // Should be not proxied still
    res = setmgr_clear_set(mgr, (char*)"dub1");
    fail_unless(res == -2);

    res = setmgr_drop_set(mgr, (char*)"dub1");
    fail_unless(res == 0);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_mgr_clear)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"dub2", NULL);
    fail_unless(res == 0);

    res = setmgr_unmap_set(mgr, (char*)"dub2");
    fail_unless(res == 0);

    // Should be not proxied still
    res = setmgr_clear_set(mgr, (char*)"dub2");
    fail_unless(res == 0);

    // Force a vacuum
    setmgr_vacuum(mgr);

    res = setmgr_create_set(mgr, (char*)"dub2", NULL);
    fail_unless(res == 0);

    res = setmgr_drop_set(mgr, (char*)"dub2");
    fail_unless(res == 0);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_mgr_clear_reload)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"zab9", NULL);
    fail_unless(res == 0);

    // Try to add keys now
    char *keys[] = {(char*)"hey",(char*)"there",(char*)"person"};
    res = setmgr_set_keys(mgr, (char*)"zab9", (char**)&keys, 3, 100);
    fail_unless(res == 0);

    res = setmgr_unmap_set(mgr, (char*)"zab9");
    fail_unless(res == 0);

    res = setmgr_clear_set(mgr, (char*)"zab9");
    fail_unless(res == 0);

    // Force a vacuum
    setmgr_vacuum(mgr);

    // This should rediscover
    res = setmgr_create_set(mgr, (char*)"zab9", NULL);
    fail_unless(res == 0);

    // Try to check keys now
    uint64_t size;
    res = setmgr_set_size_total(mgr, (char*)"zab9", &size);
    fail_unless(res == 0);
    fail_unless(size == 3, 100);

    res = setmgr_drop_set(mgr, (char*)"zab9");
    fail_unless(res == 0);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

/* List Cold */
START_TEST(test_mgr_list_cold_no_sets)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    hlld_set_list_head *head;
    res = setmgr_list_cold_sets(mgr, &head);
    fail_unless(res == 0);
    fail_unless(head->size == 0);
    setmgr_cleanup_list(head);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_mgr_list_cold)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"zab6", NULL);
    fail_unless(res == 0);
    res = setmgr_create_set(mgr, (char*)"zab7", NULL);
    fail_unless(res == 0);

    // Force vacuum so that these are noticed by the cold list
    setmgr_vacuum(mgr);

    hlld_set_list_head *head;
    res = setmgr_list_cold_sets(mgr, &head);
    fail_unless(res == 0);
    fail_unless(head->size == 0);

    // Check the keys in one, so that it stays hot
    char *keys[] = {(char*)"hey",(char*)"there",(char*)"person"};
    res = setmgr_set_keys(mgr, (char*)"zab6", (char**)&keys, 3, 100);
    fail_unless(res == 0);

    // Check cold again
    res = setmgr_list_cold_sets(mgr, &head);
    fail_unless(res == 0);
    fail_unless(head->size == 1);

    int has_zab6 = 0;
    int has_zab7 = 0;

    hlld_set_list *node = head->head;
    while (node) {
        if (strcmp(node->set_name, (char*)"zab6") == 0)
            has_zab6 = 1;
        else if (strcmp(node->set_name, (char*)"zab7") == 0)
            has_zab7 = 1;
        node = node->next;
    }
    fail_unless(!has_zab6);
    fail_unless(has_zab7);

    res = setmgr_drop_set(mgr, (char*)"zab6");
    fail_unless(res == 0);
    res = setmgr_drop_set(mgr, (char*)"zab7");
    fail_unless(res == 0);
    setmgr_cleanup_list(head);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

/* Unmap in memory */
START_TEST(test_mgr_unmap_in_mem)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);
    config.in_memory = 1;

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"mem1", NULL);
    fail_unless(res == 0);

    // Try to add keys now
    char *keys[] = {(char*)"hey",(char*)"there",(char*)"person"};
    res = setmgr_set_keys(mgr, (char*)"mem1", (char**)&keys, 3, 100);
    fail_unless(res == 0);

    res = setmgr_unmap_set(mgr, (char*)"mem1");
    fail_unless(res == 0);

    // Try to check keys now
    uint64_t size;
    res = setmgr_set_size_total(mgr, (char*)"mem1", &size);
    fail_unless(res == 0);
    fail_unless(size == 3, 100);

    res = setmgr_drop_set(mgr, (char*)"mem1");
    fail_unless(res == 0);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

/* Custom config */
START_TEST(test_mgr_create_custom_config)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    // Custom config
    hlld_config *custom = (hlld_config*)malloc(sizeof(hlld_config));
    memcpy(custom, &config, sizeof(hlld_config));
    custom->in_memory = 1;

    res = setmgr_create_set(mgr, (char*)"custom1", custom);
    fail_unless(res == 0);

    res = setmgr_drop_set(mgr, (char*)"custom1");
    fail_unless(res == 0);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

/* Close & Restore */

START_TEST(test_mgr_restore)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"zab8", NULL);
    fail_unless(res == 0);

    char *keys[] = {(char*)"hey",(char*)"there",(char*)"person"};
    res = setmgr_set_keys(mgr, (char*)"zab8", (char**)&keys, 3, 100);
    fail_unless(res == 0);

    // Shutdown
    res = destroy_set_manager(mgr);
    fail_unless(res == 0);

   // Restrore
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    // Try to check keys now
    uint64_t size;
    res = setmgr_set_size_total(mgr, (char*)"zab8", &size);
    fail_unless(res == 0);
    fail_unless(size == 3, 100);

    res = setmgr_drop_set(mgr, (char*)"zab8");
    fail_unless(res == 0);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

void test_mgr_cb(void *data, char *set_name, hlld_set* set) {
    (void)set_name;
    (void)set;
    int *out = (int*)data;
    *out = 1;
}

START_TEST(test_mgr_callback)
{
    hlld_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);
    config.in_memory = 1;

    hlld_setmgr *mgr;
    res = init_set_manager(&config, 0, &mgr);
    fail_unless(res == 0);

    res = setmgr_create_set(mgr, (char*)"cb1", NULL);
    fail_unless(res == 0);

    int val = 0;
    res = setmgr_set_cb(mgr, (char*)"cb1", test_mgr_cb, &val);
    fail_unless(val == 1);

    res = setmgr_drop_set(mgr, (char*)"cb1");
    fail_unless(res == 0);

    res = destroy_set_manager(mgr);
    fail_unless(res == 0);
}
END_TEST

