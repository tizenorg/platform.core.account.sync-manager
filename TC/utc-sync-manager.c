/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <account.h>
#include <account-types.h>
#include <account-error.h>
#include <bundle.h>
#include "sync_adapter.h"
#include "sync_manager.h"
#include "sync-error.h"


#ifdef MOBILE
const char *capability_calendar = SYNC_SUPPORTS_CAPABILITY_CALENDAR;
const char *capability_contact = SYNC_SUPPORTS_CAPABILITY_CONTACT;
#endif
const char *capability_image = SYNC_SUPPORTS_CAPABILITY_IMAGE;
const char *capability_video = SYNC_SUPPORTS_CAPABILITY_VIDEO;
const char *capability_sound = SYNC_SUPPORTS_CAPABILITY_SOUND;
const char *capability_music = SYNC_SUPPORTS_CAPABILITY_MUSIC;
const char *capability_invalid = "http://tizen.org/sync/capability/invalid";
const char *name_on_demand = "name_of_on_demand_sync_job";
const char *name_periodic = "name_of_periodic_sync_job";

const char *app_id = "core.sync-manager-tests";
const char *name = "test_name";
const char *email_address = "test_email@samsung.com";

static bool using_acc = false;
static bool created_acc = false;
static bool existed_acc = false;
static bool set_cb = false;
static account_h account = NULL;
static int account_id = -1;

bundle *user_data = NULL;

static GMainLoop *g_mainloop = NULL;
static bool is_finished = false;


static void timeout_func()
{
	g_main_loop_quit(g_mainloop);
	g_mainloop = NULL;
}


static void wait_for_async()
{
	g_mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(g_mainloop);
}


static int is_callback_finished()
{
	if (is_finished == true)
		return 0;

	return 1;
}


static bool on_start_cb(account_h account, const char *sync_job_name, const char *sync_capability, bundle *sync_job_user_data)
{
	if (!is_finished)
		is_finished = true;

	if (g_mainloop)
		timeout_func(g_mainloop);

	return true;
}


static void on_cancel_cb(account_h account, const char *sync_job_name, const char *sync_capability, bundle *sync_job_user_data)
{
	if (!is_finished)
		is_finished = true;

	if (g_mainloop)
		timeout_func(g_mainloop);

	return;
}


static bool sync_job_cb(account_h account, const char *sync_job_name, const char *sync_capability, int sync_job_id, bundle *sync_job_user_data, void *user_data)
{
	return true;
}


void sync_manager_setup_account(void)
{
	int ret = ACCOUNT_ERROR_NONE;

	using_acc = true;
	ret = account_create(&account);
	if (ret == ACCOUNT_ERROR_NONE)
		created_acc = true;

	account_set_user_name(account, name);
	account_set_email_address(account, email_address);
	account_set_package_name(account, app_id);
	account_set_sync_support(account, ACCOUNT_SUPPORTS_SYNC);
	account_insert_to_db(account, &account_id);

	if (account_type_query_app_id_exist(app_id) == ACCOUNT_ERROR_NONE)
		existed_acc = true;
}


void sync_manager_cleanup_account(void)
{
	int ret = ACCOUNT_ERROR_NONE;

	ret = account_delete_from_db_by_package_name(app_id);
	if (ret == ACCOUNT_ERROR_NONE)
		existed_acc = false;

	ret = account_destroy(account);
	if (ret == ACCOUNT_ERROR_NONE)
		created_acc = false;

	using_acc = false;
}


void sync_manager_setup_adapter(void)
{
	int ret = SYNC_ERROR_NONE;

	if (!set_cb) {
		ret = sync_adapter_set_callbacks(on_start_cb, on_cancel_cb);
		if (ret == SYNC_ERROR_NONE)
			set_cb = true;
	}
}


/**
 * @testcase		utc_sync_adapter_set_callbacks_p
 * @since_tizen		2.4
 * @description		Positive test case of setting callbacks to be invoked when a sync job is operated
 */
int utc_sync_adapter_set_callbacks_p(void)
{
	int ret = SYNC_ERROR_NONE;

	ret = sync_adapter_set_callbacks(on_start_cb, on_cancel_cb);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_adapter_unset_callbacks();
	assert_eq(ret, SYNC_ERROR_NONE);

	return 0;
}


/**
 * @testcase		utc_sync_adapter_set_callbacks_n
 * @since_tizen		2.4
 * @description		Negative test case of setting callbacks without the name of the start callback as mandatory parameter
 */
int utc_sync_adapter_set_callbacks_n(void)
{
	int ret = SYNC_ERROR_NONE;

	ret = sync_adapter_set_callbacks(NULL, on_cancel_cb);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


/**
 * @testcase		utc_sync_adapter_set_callbacks_n2
 * @since_tizen		2.4
 * @description		Negative test case of setting callbacks without the name of the cancel callback as mandatory parameter
 */
int utc_sync_adapter_set_callbacks_n2(void)
{
	int ret = SYNC_ERROR_NONE;

	ret = sync_adapter_set_callbacks(on_start_cb, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


/**
 * @function		utc_sync_manager_startup
 * @description		Called before each test
 * @parameter		NA
 * @return			NA
 */
void utc_sync_manager_startup(void)
{
	return;
}


/**
 * @function		utc_sync_manager_cleanup
 * @description		Called after each test
 * @parameter		NA
 * @return			NA
 */
void utc_sync_manager_cleanup(void)
{
	if (existed_acc)
		sync_manager_cleanup_account();

	int ret = SYNC_ERROR_NONE;

	if (set_cb) {
		ret = sync_adapter_unset_callbacks();
		if (ret == SYNC_ERROR_NONE)
			set_cb = false;
	}

	return;
}


void sync_manager_setup_interval(void)
{
	utc_sync_manager_cleanup();

	sync_manager_setup_adapter();
	sync_manager_setup_account();
}


/**
 * @testcase		utc_sync_manager_on_demand_sync_job_p
 * @since_tizen		2.4
 * @description		Positive test case of adding on-demand sync jobs
 */
int utc_sync_manager_on_demand_sync_job_p(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_on_demand_sync_job(account, name_on_demand, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_on_demand_sync_job(account, name_on_demand, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_on_demand_sync_job(account, name_on_demand, SYNC_OPTION_EXPEDITED, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_on_demand_sync_job(account, name_on_demand, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	return 0;
}


/**
 * @testcase		utc_sync_manager_on_demand_sync_job_n
 * @since_tizen		2.4
 * @description		Negative test case of adding on-demand sync jobs without sync job name as mandatory parameter
 */
int utc_sync_manager_on_demand_sync_job_n(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_on_demand_sync_job(account, NULL, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_on_demand_sync_job(NULL, NULL, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_on_demand_sync_job(account, NULL, SYNC_OPTION_EXPEDITED, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_on_demand_sync_job(NULL, NULL, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


/**
 * @testcase		utc_sync_manager_on_demand_sync_job_n2
 * @since_tizen		2.4
 * @description		Negative test case of adding on-demand sync jobs with invalid sync option
 */
int utc_sync_manager_on_demand_sync_job_n2(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_on_demand_sync_job(account, name_on_demand, -1, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_on_demand_sync_job(NULL, name_on_demand, -1, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_on_demand_sync_job(account, name_on_demand, -1, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_on_demand_sync_job(NULL, name_on_demand, -1, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


/**
 * @testcase		utc_sync_manager_on_demand_sync_job_p2
 * @since_tizen		2.4
 * @description		Positive test case of adding on-demand sync jobs with @c NULL parameters
 */
int utc_sync_manager_on_demand_sync_job_p2(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_on_demand_sync_job(NULL, name_on_demand, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_on_demand_sync_job(account, name_on_demand, SYNC_OPTION_EXPEDITED, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_on_demand_sync_job(NULL, name_on_demand, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	return 0;
}


/**
 * @testcase		utc_sync_manager_on_demand_sync_job_n3
 * @since_tizen		2.4
 * @description		Negative test case of adding on-demand sync jobs without sync job id as mandatory parameter
 */
int utc_sync_manager_on_demand_sync_job_n3(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	ret = sync_manager_on_demand_sync_job(account, name_on_demand, SYNC_OPTION_NONE, user_data, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_on_demand_sync_job(NULL, name_on_demand, SYNC_OPTION_NO_RETRY, user_data, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_on_demand_sync_job(account, name_on_demand, SYNC_OPTION_EXPEDITED, NULL, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_on_demand_sync_job(NULL, name_on_demand, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


/**
 * @testcase		utc_sync_manager_on_demand_sync_job_n4
 * @since_tizen		2.4
 * @description		Negative test case of adding on-demand sync jobs without calling sync_adapter_set_callbacks()
 */
int utc_sync_manager_on_demand_sync_job_n4(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_on_demand_sync_job(account, name_on_demand, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	ret = sync_manager_on_demand_sync_job(NULL, name_on_demand, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	ret = sync_manager_on_demand_sync_job(account, name_on_demand, SYNC_OPTION_EXPEDITED, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	ret = sync_manager_on_demand_sync_job(NULL, name_on_demand, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	return 0;
}


/**
 * @testcase		utc_sync_manager_add_periodic_sync_job_p
 * @since_tizen		2.4
 * @description		Positive test case of adding periodic sync jobs
 */
int utc_sync_manager_add_periodic_sync_job_p(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_30MIN, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_1H, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_2H, SYNC_OPTION_EXPEDITED, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_3H, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_6H, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_12H, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_1DAY, SYNC_OPTION_EXPEDITED, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	return 0;
}


/**
 * @testcase		utc_sync_manager_add_periodic_sync_job_n
 * @since_tizen		2.4
 * @description		Negative test case of adding periodic sync jobs without sync job name as mandatory parameter
 */
int utc_sync_manager_add_periodic_sync_job_n(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_add_periodic_sync_job(account, NULL, SYNC_PERIOD_INTERVAL_30MIN, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_periodic_sync_job(NULL, NULL, SYNC_PERIOD_INTERVAL_1H, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_periodic_sync_job(account, NULL, SYNC_PERIOD_INTERVAL_2H, SYNC_OPTION_EXPEDITED, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_periodic_sync_job(NULL, NULL, SYNC_PERIOD_INTERVAL_3H, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


/**
 * @testcase		utc_sync_manager_add_periodic_sync_job_n2
 * @since_tizen		2.4
 * @description		Negative test case of adding periodic sync jobs with invalid sync interval
 */
int utc_sync_manager_add_periodic_sync_job_n2(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, -1, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, -1, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, -1, SYNC_OPTION_EXPEDITED, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, -1, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


/**
 * @testcase		utc_sync_manager_add_periodic_sync_job_n3
 * @since_tizen		2.4
 * @description		Negative test case of adding periodic sync jobs with invalid sync option
 */
int utc_sync_manager_add_periodic_sync_job_n3(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_6H, -1, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_12H, -1, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_1DAY, -1, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_30MIN, -1, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


/**
 * @testcase		utc_sync_manager_add_periodic_sync_job_p2
 * @since_tizen		2.4
 * @description		Positive test case of adding periodic sync jobs with @c NULL parameters
 */
int utc_sync_manager_add_periodic_sync_job_p2(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_30MIN, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_1H, SYNC_OPTION_NO_RETRY, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_2H, SYNC_OPTION_EXPEDITED, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_3H, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_6H, SYNC_OPTION_NONE, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_12H, SYNC_OPTION_NO_RETRY, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_1DAY, SYNC_OPTION_EXPEDITED, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	return 0;
}


/**
 * @testcase		utc_sync_manager_add_periodic_sync_job_n4
 * @since_tizen		2.4
 * @description		Negative test case of adding periodic sync jobs without sync job id as mandatory parameter
 */
int utc_sync_manager_add_periodic_sync_job_n4(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_1H, SYNC_OPTION_NONE, user_data, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_2H, SYNC_OPTION_NO_RETRY, user_data, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_3H, SYNC_OPTION_EXPEDITED, NULL, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_6H, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


/**
 * @testcase		utc_sync_manager_add_periodic_sync_job_n5
 * @since_tizen		2.4
 * @description		Negative test case of adding periodic sync jobs without calling sync_adapter_set_callbacks()
 */
int utc_sync_manager_add_periodic_sync_job_n5(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_30MIN, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_1H, SYNC_OPTION_NO_RETRY, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_2H, SYNC_OPTION_EXPEDITED, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_3H, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_6H, SYNC_OPTION_NONE, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_12H, SYNC_OPTION_NO_RETRY, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_1DAY, SYNC_OPTION_EXPEDITED, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	return 0;
}


/**
 * @testcase		utc_sync_manager_add_data_change_sync_job_p
 * @since_tizen		2.4
 * @description		Positive test case of adding data change sync jobs
 */
int utc_sync_manager_add_data_change_sync_job_p(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

#ifdef MOBILE
	ret = sync_manager_add_data_change_sync_job(account, capability_calendar, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_add_data_change_sync_job(account, capability_contact, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);
#endif

	ret = sync_manager_add_data_change_sync_job(account, capability_image, SYNC_OPTION_EXPEDITED, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_add_data_change_sync_job(account, capability_video, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_add_data_change_sync_job(account, capability_sound, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_add_data_change_sync_job(account, capability_music, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	return 0;
}


/**
 * @testcase		utc_sync_manager_add_data_change_sync_job_n
 * @since_tizen		2.4
 * @description		Negative test case of adding data change sync jobs with invalid sync capability or without sync capability as mandatory parameter
 */
int utc_sync_manager_add_data_change_sync_job_n(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_add_data_change_sync_job(account, NULL, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_data_change_sync_job(NULL, NULL, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_data_change_sync_job(account, NULL, SYNC_OPTION_EXPEDITED, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_data_change_sync_job(NULL, NULL, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_data_change_sync_job(account, capability_invalid, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_data_change_sync_job(NULL, capability_invalid, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_data_change_sync_job(account, capability_invalid, SYNC_OPTION_EXPEDITED, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_data_change_sync_job(NULL, capability_invalid, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


/**
 * @testcase		utc_sync_manager_add_data_change_sync_job_n2
 * @since_tizen		2.4
 * @description		Negative test case of adding data change sync jobs with invalid sync option
 */
int utc_sync_manager_add_data_change_sync_job_n2(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

#ifdef MOBILE
	ret = sync_manager_add_data_change_sync_job(account, capability_calendar, -1, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_data_change_sync_job(NULL, capability_contact, -1, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);
#endif

	ret = sync_manager_add_data_change_sync_job(account, capability_image, -1, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_data_change_sync_job(NULL, capability_video, -1, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


/**
 * @testcase		utc_sync_manager_add_data_change_sync_job_p2
 * @since_tizen		2.4
 * @description		Positive test case of adding data change sync jobs with @c NULL parameters
 */
int utc_sync_manager_add_data_change_sync_job_p2(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

#ifdef MOBILE
	ret = sync_manager_add_data_change_sync_job(NULL, capability_calendar, SYNC_OPTION_EXPEDITED, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_add_data_change_sync_job(account, capability_contact, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);
#endif

	ret = sync_manager_add_data_change_sync_job(NULL, capability_image, SYNC_OPTION_NONE, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_add_data_change_sync_job(NULL, capability_video, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_add_data_change_sync_job(account, capability_sound, SYNC_OPTION_EXPEDITED, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_add_data_change_sync_job(NULL, capability_music, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	return 0;
}


/**
 * @testcase		utc_sync_manager_add_data_change_sync_job_n3
 * @since_tizen		2.4
 * @description		Negative test case of adding data change sync jobs without sync job id as mandatory parameter
 */
int utc_sync_manager_add_data_change_sync_job_n3(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	ret = sync_manager_add_data_change_sync_job(account, capability_sound, SYNC_OPTION_NONE, user_data, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_data_change_sync_job(NULL, capability_music, SYNC_OPTION_NO_RETRY, user_data, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

#ifdef MOBILE
	ret = sync_manager_add_data_change_sync_job(account, capability_calendar, SYNC_OPTION_EXPEDITED, NULL, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_add_data_change_sync_job(NULL, capability_contact, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);
#endif

	return 0;
}


/**
 * @testcase		utc_sync_manager_add_data_change_sync_job_n4
 * @since_tizen		2.4
 * @description		Negative test case of adding data change sync jobs without calling sync_adapter_set_callbacks()
 */
int utc_sync_manager_add_data_change_sync_job_n4(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_add_data_change_sync_job(account, capability_music, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

#ifdef MOBILE
	ret = sync_manager_add_data_change_sync_job(NULL, capability_calendar, SYNC_OPTION_EXPEDITED, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	ret = sync_manager_add_data_change_sync_job(account, capability_contact, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);
#endif

	ret = sync_manager_add_data_change_sync_job(NULL, capability_image, SYNC_OPTION_NONE, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	ret = sync_manager_add_data_change_sync_job(NULL, capability_video, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	ret = sync_manager_add_data_change_sync_job(account, capability_sound, SYNC_OPTION_EXPEDITED, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	ret = sync_manager_add_data_change_sync_job(NULL, capability_music, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND);

	return 0;
}


/**
 * @testcase		utc_sync_manager_remove_sync_job_p
 * @since_tizen		2.4
 * @description		Positive test case of removing on-demand sync jobs
 */
int utc_sync_manager_remove_sync_job_p(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_on_demand_sync_job(account, name_on_demand, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_on_demand_sync_job(NULL, name_on_demand, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_on_demand_sync_job(account, name_on_demand, SYNC_OPTION_EXPEDITED, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_on_demand_sync_job(NULL, name_on_demand, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	return 0;
}


/**
 * @testcase		utc_sync_manager_remove_sync_job_p2
 * @since_tizen		2.4
 * @description		Positive test case of removing periodic sync jobs
 */
int utc_sync_manager_remove_sync_job_p2(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_30MIN, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_2H, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_6H, SYNC_OPTION_EXPEDITED, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_setup_interval();

	ret = sync_manager_add_periodic_sync_job(NULL, name_periodic, SYNC_PERIOD_INTERVAL_1DAY, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	return 0;
}


/**
 * @testcase		utc_sync_manager_remove_sync_job_p3
 * @since_tizen		2.4
 * @description		Positive test case of removing data change sync jobs
 */
int utc_sync_manager_remove_sync_job_p3(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

#ifdef MOBILE
	ret = sync_manager_add_data_change_sync_job(account, capability_calendar, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_add_data_change_sync_job(NULL, capability_contact, SYNC_OPTION_NO_RETRY, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);
#endif

	ret = sync_manager_add_data_change_sync_job(account, capability_image, SYNC_OPTION_EXPEDITED, NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_add_data_change_sync_job(NULL, capability_video, (SYNC_OPTION_NO_RETRY | SYNC_OPTION_EXPEDITED), NULL, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	wait_for_async();

	ret = is_callback_finished();
	assert_eq(ret, SYNC_ERROR_NONE);
	is_finished = false;

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	return 0;
}


/**
 * @testcase		utc_sync_manager_remove_sync_job_n
 * @since_tizen		2.4
 * @description		Negative test case of removing various sync jobs with invalid sync job id
 */
int utc_sync_manager_remove_sync_job_n(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id = 0;

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_30MIN, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(-1);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_add_data_change_sync_job(account, capability_image, SYNC_OPTION_NONE, user_data, &sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(-1);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_remove_sync_job(sync_job_id);
	assert_eq(ret, SYNC_ERROR_NONE);

	return 0;
}


/**
 * @testcase		utc_sync_manager_foreach_sync_job_p
 * @since_tizen		2.4
 * @description		Positive test case of getting all of sync jobs
 */
int utc_sync_manager_foreach_sync_job_p(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id1 = 0;
	int sync_job_id2 = 0;

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_30MIN, SYNC_OPTION_NONE, user_data, &sync_job_id1);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_add_data_change_sync_job(account, capability_sound, SYNC_OPTION_NONE, user_data, &sync_job_id2);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_foreach_sync_job(sync_job_cb, NULL);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id1);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id2);
	assert_eq(ret, SYNC_ERROR_NONE);

	return 0;
}


/**
 * @testcase		utc_sync_manager_foreach_sync_job_n
 * @since_tizen		2.4
 * @description		Negative test case of getting all of sync jobs without the name of callback as mandatory parameter
 */
int utc_sync_manager_foreach_sync_job_n(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_manager_setup_adapter();
	assert(set_cb);

	sync_manager_setup_account();
	assert(using_acc);
	assert(created_acc);
	assert(existed_acc);

	user_data = bundle_create();
	bundle_add_str(user_data, "str", "String user_data sample.");

	int sync_job_id1 = 0;
	int sync_job_id2 = 0;

	ret = sync_manager_add_periodic_sync_job(account, name_periodic, SYNC_PERIOD_INTERVAL_30MIN, SYNC_OPTION_NONE, user_data, &sync_job_id1);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_add_data_change_sync_job(account, capability_music, SYNC_OPTION_NONE, user_data, &sync_job_id2);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_foreach_sync_job(NULL, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	ret = sync_manager_remove_sync_job(sync_job_id1);
	assert_eq(ret, SYNC_ERROR_NONE);

	ret = sync_manager_remove_sync_job(sync_job_id2);
	assert_eq(ret, SYNC_ERROR_NONE);

	return 0;
}
