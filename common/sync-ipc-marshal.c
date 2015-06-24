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
#include "sync-ipc-marshal.h"


#ifdef __cplusplus
extern "C"
{
#endif

void
bundle_iterate_cb(const char *key, const char *val, void *data)
{
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

