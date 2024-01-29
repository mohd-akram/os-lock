#include <stdlib.h>
#include <node_api.h>
#include <uv.h>

#include "addon.h"
#include "lock.h"

#define NAPI_CALL(env, call)							\
	do {									\
		napi_status status = (call);					\
		if (status != napi_ok) {					\
			const napi_extended_error_info *error_info = NULL;	\
			napi_get_last_error_info((env), &error_info);		\
			const char *err_message = error_info->error_message;	\
			bool is_pending;					\
			napi_is_exception_pending((env), &is_pending);		\
			if (!is_pending) {					\
				const char *message = (err_message == NULL)	\
					? "empty error message"			\
					: err_message;				\
				napi_throw_error((env), NULL, message);		\
			}							\
			return NULL;						\
		}								\
	} while(0)

struct lock_work {
	napi_async_work async;
	napi_ref cb;
	int res;

	int fd;
	long long start;
	long long length;
	bool exclusive;
	bool immediate;
};

void lock_do(napi_env env, void *data) {
	struct lock_work *work = data;
	work->res = uv_translate_sys_error(lock(
		work->fd, work->start, work->length, work->exclusive,
		work->immediate
	));
}

void unlock_do(napi_env env, void *data) {
	struct lock_work *work = data;
	work->res = uv_translate_sys_error(unlock(
		work->fd, work->start, work->length
	));
}

static napi_value lock_finish(napi_env env, napi_status status, void *data) {
	struct lock_work *work = data;

	napi_value err;

	napi_value msg;
	napi_value code;
	if (work->res) {
		NAPI_CALL(env, napi_create_string_utf8(
			env, uv_strerror(work->res), NAPI_AUTO_LENGTH, &msg
		));
		NAPI_CALL(env, napi_create_string_utf8(
			env, uv_err_name(work->res), NAPI_AUTO_LENGTH, &code
		));
		NAPI_CALL(env, napi_create_error(env, code, msg, &err));
	}
	else NAPI_CALL(env, napi_get_null(env, &err));

	napi_value argv[] = { err };

	napi_value cb;
	NAPI_CALL(env, napi_get_reference_value(env, work->cb, &cb));
	napi_value recv;
	NAPI_CALL(env, napi_get_null(env, &recv));
	NAPI_CALL(env, napi_call_function(env, recv, cb, 1, argv, NULL));

	NAPI_CALL(env, napi_delete_reference(env, work->cb));
	NAPI_CALL(env, napi_delete_async_work(env, work->async));
	free(work);

	return NULL;
}

napi_value lock_async(napi_env env, napi_callback_info info) {
	size_t argc = 6;
	napi_value argv[6];

	NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

	struct lock_work *work = malloc(sizeof *work);

	NAPI_CALL(env, napi_get_value_int32(env, argv[0], &work->fd));
	NAPI_CALL(env, napi_get_value_int64(env, argv[1], &work->start));
	NAPI_CALL(env, napi_get_value_int64(env, argv[2], &work->length));
	NAPI_CALL(env, napi_get_value_bool(env, argv[3], &work->exclusive));
	NAPI_CALL(env, napi_get_value_bool(env, argv[4], &work->immediate));
	NAPI_CALL(env, napi_create_reference(env, argv[5], 1, &work->cb));

	napi_value name;
	NAPI_CALL(env, napi_create_string_utf8(
		env, "lock_async", NAPI_AUTO_LENGTH, &name
	));
	NAPI_CALL(env, napi_create_async_work(
		env, NULL, name, lock_do,
		(napi_async_complete_callback)lock_finish, work, &work->async
	));
	NAPI_CALL(env, napi_queue_async_work(env, work->async));

	return NULL;
}

napi_value unlock_async(napi_env env, napi_callback_info info) {
	size_t argc = 4;
	napi_value argv[4];

	NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

	struct lock_work *work = malloc(sizeof *work);

	NAPI_CALL(env, napi_get_value_int32(env, argv[0], &work->fd));
	NAPI_CALL(env, napi_get_value_int64(env, argv[1], &work->start));
	NAPI_CALL(env, napi_get_value_int64(env, argv[2], &work->length));
	NAPI_CALL(env, napi_create_reference(env, argv[3], 1, &work->cb));

	napi_value name;
	NAPI_CALL(env, napi_create_string_utf8(
		env, "unlock_async", NAPI_AUTO_LENGTH, &name
	));
	NAPI_CALL(env, napi_create_async_work(
		env, NULL, name, unlock_do,
		(napi_async_complete_callback)lock_finish, work, &work->async
	));
	NAPI_CALL(env, napi_queue_async_work(env, work->async));

	return NULL;
}

napi_value create_addon(napi_env env) {
	napi_value result;
	NAPI_CALL(env, napi_create_object(env, &result));

	napi_value lock;
	NAPI_CALL(env, napi_create_function(
		env, "lock", NAPI_AUTO_LENGTH, lock_async, NULL, &lock
	));
	NAPI_CALL(env, napi_set_named_property(
		env, result, "lock", lock
	));

	napi_value unlock;
	NAPI_CALL(env, napi_create_function(
		env, "unlock", NAPI_AUTO_LENGTH, unlock_async, NULL, &unlock
	));
	NAPI_CALL(env, napi_set_named_property(
		env, result, "unlock", unlock
	));

	return result;
}
