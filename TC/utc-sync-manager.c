//
// Copyright (c) 2014 Samsung Electronics Co., Ltd.
//
// Licensed under the Apache License, Version 2.0 (the License);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#include "assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <account.h>
#include <account-types.h>
#include <account-error.h>
#include <bundle.h>
#include <bundle_internal.h>
#include "sync_adapter.h"
#include "sync_manager.h"
#include "sync-error.h"


const char *capability_cal = "http://tizen.org/account/capability/calendar";
const char *capability_invalid = "http://tizen.org/account/capability_invalid";
const char *package_name = "core-sync-manager-tests";
const char *name = "test_name";
const char *email_address = "test_email@samsung.com";

static bool connected_sync = false;
static bool connected_acc = false;
static bool created_acc = false;
static bool existed_acc = false;
static bool set_cb = false;
static account_h account = NULL;
static int account_id = -1;
bundle *extra = NULL;


static bool on_start_cb(account_h account, bundle* extras, const char* capability)
{
	return true;
}


static void on_cancel_cb(account_h account, const char* capability)
{
	return;
}


void sync_manager_setup_account(void)
{
	int ret = ACCOUNT_ERROR_NONE;

	ret = account_connect();
	if (ret == ACCOUNT_ERROR_NONE) {
		connected_acc = true;
		ret = account_create(&account);
		if (ret == ACCOUNT_ERROR_NONE) {
			created_acc = true;
		}
	}

	ret = account_set_user_name(account, name);
	assert_eq(ret, ACCOUNT_ERROR_NONE);

	ret = account_set_email_address(account, email_address);
	assert_eq(ret, ACCOUNT_ERROR_NONE);

	ret = account_set_package_name(account, package_name);
	assert_eq(ret, ACCOUNT_ERROR_NONE);

	ret = account_set_sync_support(account, ACCOUNT_SUPPORTS_SYNC);
	assert_eq(ret, ACCOUNT_ERROR_NONE);

	ret = account_insert_to_db(account, &account_id);
	assert_eq(ret, ACCOUNT_ERROR_NONE);

	if (account_type_query_app_id_exist(package_name) == ACCOUNT_ERROR_NONE) {
		existed_acc = true;
	}

	account_disconnect();
}


void sync_manager_cleanup_account(void)
{
	int ret = ACCOUNT_ERROR_NONE;

	ret = account_connect();
	assert_eq(ret, ACCOUNT_ERROR_NONE);

	ret = account_get_account_id(account, &account_id);
	assert_eq(ret, ACCOUNT_ERROR_NONE);

	ret = account_delete_from_db_by_id(account_id);
	assert_eq(ret, ACCOUNT_ERROR_NONE);

	ret = account_destroy(account);
	if (ret == ACCOUNT_ERROR_NONE) {
		created_acc = false;
	}

	if (account_type_query_app_id_exist(package_name) == ACCOUNT_ERROR_NONE) {
		existed_acc = false;
	}

	account_disconnect();
}


int utc_sync_adapter_init_p(void)
{
	int ret = SYNC_ERROR_NONE;

	ret = sync_adapter_init(capability_cal);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_adapter_destroy();

	return 0;
}


int utc_sync_adapter_init_p2(void)
{
	int ret = SYNC_ERROR_NONE;

	ret = sync_adapter_init(NULL);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_adapter_destroy();

	return 0;
}


int utc_sync_adapter_init_n(void)
{
	int ret = SYNC_ERROR_NONE;

	ret = sync_adapter_init(capability_invalid);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


int utc_sync_adapter_destroy_p(void)
{
	int ret = SYNC_ERROR_NONE;

	ret = sync_adapter_init(capability_cal);
	sync_adapter_destroy();
	assert_eq(ret, SYNC_ERROR_NONE);

	return 0;
}


int utc_sync_adapter_destroy_p2(void)
{
	int ret = SYNC_ERROR_NONE;

	ret = sync_adapter_init(NULL);
	sync_adapter_destroy();
	assert_eq(ret, SYNC_ERROR_NONE);

	return 0;
}


int utc_sync_adapter_set_callbacks_p(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_adapter_init(capability_cal);
	ret = sync_adapter_set_callbacks(on_start_cb, on_cancel_cb);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_adapter_unset_callbacks();
	sync_adapter_destroy();

	return 0;
}


int utc_sync_adapter_set_callbacks_p2(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_adapter_init(NULL);
	ret = sync_adapter_set_callbacks(on_start_cb, on_cancel_cb);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_adapter_unset_callbacks();
	sync_adapter_destroy();

	return 0;
}


int utc_sync_adapter_set_callbacks_n(void)
{
	int ret = SYNC_ERROR_NONE;

	sync_adapter_init(NULL);
	ret = sync_adapter_set_callbacks(NULL, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	sync_adapter_destroy();

	return 0;
}


void utc_sync_manager_startup(void)
{
	int ret = SYNC_ERROR_NONE;

	ret = sync_adapter_init(capability_cal);
	assert_eq(ret, SYNC_ERROR_NONE);

	if (!set_cb) {
		sync_adapter_set_callbacks(on_start_cb, on_cancel_cb);
		set_cb = true;
	}

	ret = sync_manager_connect();
	if (ret == SYNC_ERROR_NONE) {
		connected_sync = true;
	}

	return;
}


void utc_sync_manager_cleanup(void)
{
	sync_manager_disconnect();

	if (set_cb) {
		sync_adapter_unset_callbacks();
		set_cb = false;
	}

	sync_adapter_destroy();

	connected_sync = false;

	return;
}


int utc_sync_manager_connect_p(void)
{
	assert(connected_sync);

	return 0;
}


int utc_sync_manager_disconnect_p(void)
{
	utc_sync_manager_cleanup();
	assert(!connected_sync);

	return 0;
}


int utc_sync_manager_add_sync_job_p(void)
{
	assert(connected_sync);

	sync_manager_setup_account();
	assert(connected_acc);
	assert(created_acc);
	assert(existed_acc);

	int ret = SYNC_ERROR_NONE;

	extra = bundle_create();
	bundle_add(extra, SYNC_OPTION_NO_RETRY, "false");
	bundle_add(extra, SYNC_OPTION_EXPEDITED, "false");

	ret = sync_manager_add_sync_job(account, capability_cal, extra);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_remove_sync_job(account, capability_cal);

	sync_manager_cleanup_account();

	return 0;
}


int utc_sync_manager_add_sync_job_p2(void)
{
	assert(connected_sync);

	int ret = SYNC_ERROR_NONE;

	extra = bundle_create();
	bundle_add(extra, SYNC_OPTION_NO_RETRY, "false");
	bundle_add(extra, SYNC_OPTION_EXPEDITED, "false");

	ret = sync_manager_add_sync_job(NULL, NULL, extra);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_remove_sync_job(NULL, NULL);

	return 0;
}


int utc_sync_manager_add_sync_job_n(void)
{
	assert(connected_sync);

	int ret = SYNC_ERROR_NONE;

	ret = sync_manager_add_sync_job(NULL, capability_invalid, NULL);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


int utc_sync_manager_remove_sync_job_p(void)
{
	assert(connected_sync);

	sync_manager_setup_account();
	assert(connected_acc);
	assert(created_acc);
	assert(existed_acc);

	int ret = SYNC_ERROR_NONE;

	extra = bundle_create();
	bundle_add(extra, SYNC_OPTION_NO_RETRY, "false");
	bundle_add(extra, SYNC_OPTION_EXPEDITED, "false");

	sync_manager_add_sync_job(account, capability_cal, extra);

	ret = sync_manager_remove_sync_job(account, capability_cal);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_cleanup_account();

	return 0;
}


int utc_sync_manager_remove_sync_job_p2(void)
{
	int ret = SYNC_ERROR_NONE;

	extra = bundle_create();
	bundle_add(extra, SYNC_OPTION_NO_RETRY, "false");
	bundle_add(extra, SYNC_OPTION_EXPEDITED, "false");
	sync_manager_add_sync_job(NULL, NULL, extra);

	ret = sync_manager_remove_sync_job(NULL, NULL);
	assert_eq(ret, SYNC_ERROR_NONE);

	return 0;
}


int utc_sync_manager_remove_sync_job_n(void)
{
	assert(connected_sync);

	sync_manager_setup_account();
	assert(connected_acc);
	assert(created_acc);
	assert(existed_acc);

	int ret = SYNC_ERROR_NONE;

	ret = sync_manager_remove_sync_job(account, capability_invalid);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	sync_manager_cleanup_account();

	return 0;
}


int utc_sync_manager_add_periodic_sync_job_p(void)
{
	assert(connected_sync);

	sync_manager_setup_account();
	assert(connected_acc);
	assert(created_acc);
	assert(existed_acc);

	int ret = SYNC_ERROR_NONE;

	extra = bundle_create();
	bundle_add(extra, SYNC_OPTION_NO_RETRY, "false");
	bundle_add(extra, SYNC_OPTION_EXPEDITED, "false");

	ret = sync_manager_add_periodic_sync_job(account, capability_cal, extra, SYNC_PERIOD_INTERVAL_5MIN);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_remove_periodic_sync_job(account, capability_cal);

	sync_manager_cleanup_account();

	return 0;
}


int utc_sync_manager_add_periodic_sync_job_p2(void)
{
	assert(connected_sync);

	int ret = SYNC_ERROR_NONE;

	extra = bundle_create();
	bundle_add(extra, SYNC_OPTION_NO_RETRY, "false");
	bundle_add(extra, SYNC_OPTION_EXPEDITED, "false");

	ret = sync_manager_add_periodic_sync_job(NULL, NULL, extra, SYNC_PERIOD_INTERVAL_5MIN);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_remove_periodic_sync_job(NULL, NULL);

	return 0;
}


int utc_sync_manager_add_periodic_sync_job_n(void)
{
	assert(connected_sync);

	int ret = SYNC_ERROR_NONE;

	ret = sync_manager_add_periodic_sync_job(NULL, capability_invalid, NULL, SYNC_PERIOD_INTERVAL_5MIN);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


int utc_sync_manager_add_periodic_sync_job_n2(void)
{
	assert(connected_sync);

	int ret = SYNC_ERROR_NONE;

	extra = bundle_create();
	bundle_add(extra, SYNC_OPTION_NO_RETRY, "false");
	bundle_add(extra, SYNC_OPTION_EXPEDITED, "false");

	ret = sync_manager_add_periodic_sync_job(NULL, NULL, extra, -1);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	return 0;
}


int utc_sync_manager_remove_periodic_sync_job_p(void)
{
	assert(connected_sync);

	sync_manager_setup_account();
	assert(connected_acc);
	assert(created_acc);
	assert(existed_acc);

	int ret = SYNC_ERROR_NONE;

	extra = bundle_create();
	bundle_add(extra, SYNC_OPTION_NO_RETRY, "false");
	bundle_add(extra, SYNC_OPTION_EXPEDITED, "false");

	sync_manager_add_periodic_sync_job(account, capability_cal, extra, SYNC_PERIOD_INTERVAL_5MIN);

	ret = sync_manager_remove_periodic_sync_job(account, capability_cal);
	assert_eq(ret, SYNC_ERROR_NONE);

	sync_manager_cleanup_account();

	return 0;
}


int utc_sync_manager_remove_periodic_sync_job_p2(void)
{
	assert(connected_sync);

	int ret = SYNC_ERROR_NONE;

	extra = bundle_create();
	bundle_add(extra, SYNC_OPTION_NO_RETRY, "false");
	bundle_add(extra, SYNC_OPTION_EXPEDITED, "false");

	sync_manager_add_periodic_sync_job(NULL, NULL, extra, SYNC_PERIOD_INTERVAL_5MIN);

	ret = sync_manager_remove_periodic_sync_job(NULL, NULL);
	assert_eq(ret, SYNC_ERROR_NONE);

	return 0;
}


int utc_sync_manager_remove_periodic_sync_job_n(void)
{
	assert(connected_sync);

	sync_manager_setup_account();
	int ret = SYNC_ERROR_NONE;

	ret = sync_manager_remove_sync_job(account, capability_invalid);
	assert_eq(ret, SYNC_ERROR_INVALID_PARAMETER);

	sync_manager_cleanup_account();

	return 0;
}

