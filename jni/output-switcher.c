/*
 * Copyright (C) 2012 Collabora Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <string.h>
#include <jni.h>
#include <pulse/pulseaudio.h>

struct userdata {
    pa_threaded_mainloop *mainloop;
    pa_context *context;

    char *server_name;
    uint32_t module_idx;
    uint32_t local_sink_idx;
    uint32_t remote_sink_idx;
};

/* This should likely be passed around in Java land as a long. */
static struct userdata *userdata;

static void context_si_info_cb(pa_context *c, const pa_sink_input_info *i, int eol, void *user)
{
    if (eol)
        return;

    /* XXX: Error handling */
    pa_operation_unref(pa_context_move_sink_input_by_index(c, i->index, (uint32_t) user, NULL, NULL));
}

static void move_all_sink_inputs(struct userdata *u, uint32_t sink_idx)
{
    char buf[256];

    /* XXX: Error handling */
    pa_operation_unref(pa_context_get_sink_input_info_list(u->context, context_si_info_cb, (void *) sink_idx));

    snprintf(buf, sizeof(buf), "%d", userdata->remote_sink_idx);
    pa_operation_unref(pa_context_set_default_sink(userdata->context, buf, NULL, NULL));
}

static void context_state_cb(pa_context *c, void *user)
{
    struct userdata *u = (struct userdata *) user;

    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;

        case PA_CONTEXT_READY:
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            pa_threaded_mainloop_signal(u->mainloop, 0);
            break;

        default:
            /* Not reached */
            break;
    }
}

static void context_subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *user)
{
    struct userdata *u = (struct userdata *) user;

    if (((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SINK) &&
        ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_NEW)) {
        /* New sink */
        /* XXX: This assumes that the only new sinks will be remote, and that
         * there will be only one remote sink */
        u->remote_sink_idx = idx;

        pa_threaded_mainloop_signal(u->mainloop, 0);
    } else if (((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SINK) &&
        ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE)) {
        if (u->remote_sink_idx == idx) {
            /* Module went away */
            /* XXX: Let the UI know */
            u->remote_sink_idx = 0;
            u->module_idx = 0;
            pa_xfree(u->server_name);
            u->server_name = NULL;
        }
    } else if (((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_MODULE) &&
        ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE)) {
        if (u->module_idx == idx) {
            /* Module went away */
            /* XXX: Let the UI know */
            u->remote_sink_idx = 0;
            u->module_idx = 0;
            pa_xfree(u->server_name);
            u->server_name = NULL;
        }
    }
}

static void context_index_cb(pa_context *c, uint32_t idx, void *user)
{
    struct userdata *u = (struct userdata *) user;

    userdata->module_idx = idx;
    pa_threaded_mainloop_signal(u->mainloop, 0);
}

static void init()
{
    int ret;
    pa_context_state_t state;

    if (userdata)
        return;

    userdata = pa_xnew0(struct userdata, 1);

    userdata->mainloop = pa_threaded_mainloop_new();

    userdata->context = pa_context_new(pa_threaded_mainloop_get_api(userdata->mainloop), "PA Output Switcher");

    if ((ret = pa_context_connect(userdata->context, NULL, PA_CONTEXT_NOFLAGS, NULL)) < 0) {
        /* XXX: How do we meaningfully assert here? */
        return;
    }

    pa_context_set_state_callback(userdata->context, context_state_cb, userdata);
    pa_context_set_subscribe_callback(userdata->context, context_subscribe_cb, userdata);

    pa_threaded_mainloop_lock(userdata->mainloop);

    pa_threaded_mainloop_start(userdata->mainloop);

    state = pa_context_get_state(userdata->context);
    while (state != PA_CONTEXT_READY && state != PA_CONTEXT_FAILED && state != PA_CONTEXT_TERMINATED) {
        pa_threaded_mainloop_wait(userdata->mainloop);
        state = pa_context_get_state(userdata->context);
    }

    if (state != PA_CONTEXT_READY) {
        /* XXX: How do we meaningfully assert here? */
        return;
    }

    /* XXX: Error handling */
    pa_operation_unref(pa_context_subscribe(userdata->context, PA_SUBSCRIPTION_MASK_SINK, NULL, NULL));

    pa_threaded_mainloop_unlock(userdata->mainloop);

    /* XXX: we never set local_sink_idx to the actual value, just assume it's 0 */
}

/* XXX: Where do we call this */
void deinit()
{
    pa_threaded_mainloop_lock(userdata->mainloop);

    pa_context_disconnect(userdata->context);
    pa_context_unref(userdata->context);

    pa_threaded_mainloop_stop(userdata->mainloop);
    pa_threaded_mainloop_unlock(userdata->mainloop);
    pa_threaded_mainloop_free(userdata->mainloop);

    pa_xfree(userdata);
}

jboolean Java_org_pulseaudio_outputswitcher_OutputSwitcher_switchToNetwork(JNIEnv* env, jobject thiz, jstring jServerName)
{
    char buf[256];
    const char *server_name = NULL;
    jboolean ret = JNI_FALSE;

    fprintf(stderr, "Something!\n");

    if (jServerName)
        server_name = (*env)->GetStringUTFChars(env, jServerName, NULL);

    if (!server_name)
        return JNI_FALSE;

    if (!userdata)
        init();

    fprintf(stderr, "Inited. Server is %s\n", server_name);

    pa_threaded_mainloop_lock(userdata->mainloop);

    if (!userdata->server_name || strcmp(server_name, userdata->server_name) != 0) {
        pa_operation *o;

        fprintf(stderr, "New server: %s\n", server_name);

        if (userdata->module_idx) {
            fprintf(stderr, "Unloading old module\n");
            pa_context_unload_module(userdata->context, userdata->module_idx, NULL, NULL);
        }

        snprintf(buf, sizeof(buf), "server=%s", server_name);

        userdata->remote_sink_idx = 0;

        fprintf(stderr, "Loading new module\n");

        if (!(o = pa_context_load_module(userdata->context, "module-tunnel-sink", buf, context_index_cb, userdata))) {
            /* XXX: Error handling */
            goto out;
        }

        while (pa_operation_get_state(o) == PA_OPERATION_RUNNING) {
            pa_threaded_mainloop_wait(userdata->mainloop);
        }

	if (userdata->module_idx == -1) {
            /* XXX: Error handling */
            goto out;
        }


        fprintf(stderr, "Waiting for new sink (%d)\n", userdata->module_idx);

        /* XXX: where are the locks, douchebag? */
        while (!userdata->remote_sink_idx) {
            pa_threaded_mainloop_wait(userdata->mainloop);
        }

        if (userdata->server_name)
            pa_xfree(userdata->server_name);
        userdata->server_name = pa_xstrdup(server_name);
    }

    fprintf(stderr, "Setting default sink to %d\n", userdata->remote_sink_idx);
    move_all_sink_inputs(userdata, userdata->remote_sink_idx);

    ret = JNI_TRUE;

out:
    pa_threaded_mainloop_unlock(userdata->mainloop);

    (*env)->ReleaseStringUTFChars(env, jServerName, server_name);

    return ret;
}

jboolean Java_org_pulseaudio_outputswitcher_OutputSwitcher_switchToLocal(JNIEnv* env, jobject thiz)
{
    if (!userdata)
        init();

    pa_threaded_mainloop_lock(userdata->mainloop);

    move_all_sink_inputs(userdata, userdata->local_sink_idx);

    pa_threaded_mainloop_unlock(userdata->mainloop);

    return JNI_TRUE;
}
