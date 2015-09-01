/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include "sync-log.h"
#include "sync_manager.h"
#include "sync-ipc-marshal.h"


#ifdef __cplusplus
extern "C"
{
#endif


void
bundle_iterate_cb(const char *key, const char *val, void *data)
{
	// LOG_LOGD("marshal bundle key %s val %s", key, val);
	g_variant_builder_add((GVariantBuilder *)data, "{sv}",
			key,
			g_variant_new_string(val));
}


GVariant*
marshal_bundle(bundle *extras)
{
	GVariantBuilder builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);

	bundle_iterate(extras, bundle_iterate_cb, (void *)&builder);
	return g_variant_builder_end(&builder);
}


void
umarshal_sync_job_list(GVariant* variant, sync_manager_sync_job_cb callback, void* user_data)
{
	GVariantIter iter;
	GVariantIter* iter_job = NULL;
	const gchar *key = NULL;
	GVariant *value = NULL;
	g_variant_iter_init (&iter, variant);

	while (g_variant_iter_loop (&iter, "a{sv}", &iter_job))
	{
		int sync_job_id = 0;
		account_h account = NULL;
		char* sync_job_name = NULL;
		char* sync_capability = NULL;
		bundle* job_user_data = NULL;

		while (g_variant_iter_loop(iter_job, "{sv}", &key, &value))
		{
			if (!g_strcmp0(key, KEY_SYNC_JOB_ID))
			{
				sync_job_id = g_variant_get_int32 (value);
			}

			if (!g_strcmp0(key, KEY_SYNC_JOB_ACC_ID))
			{
				int account_id = g_variant_get_int32 (value);
				if (account_id != -1)
				{
					account_create(&account);
					account_query_account_by_account_id(account_id, &account);
				}
			}

			if (!g_strcmp0(key, KEY_SYNC_JOB_NAME))
			{
				sync_job_name = (char*) g_variant_get_string(value, NULL);
			}
			else if (!g_strcmp0(key, KEY_SYNC_JOB_CAPABILITY))
			{
				sync_capability = (char*) g_variant_get_string (value, NULL);
			}
			else if (!g_strcmp0(key, KEY_SYNC_JOB_USER_DATA))
			{
				job_user_data = umarshal_bundle(value);
			}
		}

		if (!callback(account, sync_job_name, sync_capability, sync_job_id, job_user_data, user_data))
		{
			account_destroy(account);
			bundle_free(job_user_data);
			break;
		}

		account_destroy(account);
		bundle_free(job_user_data);
	}
}



bundle*
umarshal_bundle(GVariant *in_data)
{
	if (in_data == NULL) {
		LOG_LOGD(" Null input");
		return NULL;
	}

	GVariant *temp_var = in_data;
	gchar *print_type = NULL;
	print_type = g_variant_print(temp_var, TRUE);
	if (print_type == NULL)	{
		LOG_LOGD(" Invalid input");
		return NULL;
	}

	bundle *extras = bundle_create();
	if (extras == NULL)
		LOG_LOGD(" bundle_create returned null");

	GVariantIter iter;
	gchar *key = NULL;
	/*gchar *value = NULL;*/
	GVariant *value = NULL;

	g_variant_iter_init(&iter, in_data);;

	while (g_variant_iter_next(&iter, "{sv}", &key, &value))
	{
		/*LOG_LOGD("umarshal bundle key %s val %s", key, (char*) g_variant_get_string (value, NULL));*/
		if (bundle_add(extras, key, g_variant_get_string(value, NULL)) != 0)
			LOG_LOGD(" error in bundle_add");
	}

	return extras;
}

#ifdef __cplusplus
}
#endif

