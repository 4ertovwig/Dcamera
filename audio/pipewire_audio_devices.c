/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <wp/wp.h>
#include <stdio.h>
#include <locale.h>
#include <libintl.h>

#include <glib-2.0/glib.h>
#include <pipewire/pipewire.h>
#include <pipewire/keys.h>
#include <pipewire/extensions/session-manager/keys.h>

#include "pipewire_audio_devices.h"

// On my machine wireplumber use next plugins
// from here /usr/lib/x86_64-linux-gnu/wireplumber-0.4/:

// /usr/lib/x86_64-linux-gnu/wireplumber-0.4/libwireplumber-module-mixer-api.so         // get volume for devices
// /usr/lib/x86_64-linux-gnu/wireplumber-0.4/libwireplumber-module-default-nodes-api
// /usr/lib/x86_64-linux-gnu/pipewire-0.3/libpipewire-module-session-manager.so
// /usr/lib/x86_64-linux-gnu/pipewire-0.3/libpipewire-module-adapter.so
// /usr/lib/x86_64-linux-gnu/pipewire-0.3/libpipewire-module-client-node.so
// /usr/lib/x86_64-linux-gnu/pipewire-0.3/libpipewire-module-protocol-native.so
// /usr/lib/x86_64-linux-gnu/pipewire-0.3/libpipewire-module-metadata.so
// /usr/lib/x86_64-linux-gnu/pipewire-0.3/libpipewire-module-client-device.so
// /usr/lib/x86_64-linux-gnu/spa-0.2/support/libspa-support.so

    static const gchar *AUDIO_CLASSES[] = {
        "Audio/Sink",
        "Audio/Source",
    };

    typedef struct _WpCtl
    {
        GOptionContext *context;
        GMainLoop *loop;
        GPtrArray *apis;
        WpCore *core;
        WpObjectManager *om;
        guint pending_plugins;
        gint exit_code;
    } WpCtl;

    static struct
    {
        struct
        {
            gboolean display_nicknames;
            gboolean display_names;
        } status;
    } cmdline;

    struct print_context
    {
        WpCtl *self;
        guint32 default_node;
        WpPlugin *mixer_api;
        GHashTable *printed_filters;
    };

    const GOptionEntry entries[3] = {
        {"nick", 'k', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
         &cmdline.status.display_nicknames,
         "Display device and node nicknames instead of descriptions", NULL},
        {"name", 'n', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
         &cmdline.status.display_names,
         "Display device and node names instead of descriptions", NULL},
        {NULL}};

    static void wp_ctl_clear(WpCtl *self)
    {
        g_clear_pointer(&self->apis, g_ptr_array_unref);
        g_clear_object(&self->om);
        g_clear_object(&self->core);
        g_clear_pointer(&self->loop, g_main_loop_unref);
        g_clear_pointer(&self->context, g_option_context_free);
    }

    static void print_controls(guint32 id, struct print_context *context)
    {
        g_autoptr(GVariant) dict = NULL;

        if (context->mixer_api)
            g_signal_emit_by_name(context->mixer_api, "get-volume", id, &dict);

        if (dict)
        {
            gboolean mute = FALSE;
            gdouble volume = 1.0;
            if (g_variant_lookup(dict, "mute", "b", &mute) &&
                g_variant_lookup(dict, "volume", "d", &volume))
                LOG_INFO(" [vol: %.2f%s", volume, mute ? " MUTED]" : "]");
        }
        LOG_INFO("\n");
    }

    static void print_device(const GValue *item, gpointer data)
    {
        WpPipewireObject *obj = g_value_get_object(item);
        guint32 id = wp_proxy_get_bound_id(WP_PROXY(obj));
        const gchar *api = wp_pipewire_object_get_property(obj, PW_KEY_DEVICE_API);
        const gchar *name = NULL;

        if (cmdline.status.display_nicknames)
            name = wp_pipewire_object_get_property(obj, PW_KEY_DEVICE_NICK);
        else if (cmdline.status.display_names)
            name = wp_pipewire_object_get_property(obj, PW_KEY_DEVICE_NAME);

        if (!name)
            name = wp_pipewire_object_get_property(obj, PW_KEY_DEVICE_DESCRIPTION);

        LOG_INFO("      %4u. %-35s [%s]\n", id, name, api);
    }

    static void print_dev_node(const GValue *item, gpointer data)
    {
        WpPipewireObject *obj = g_value_get_object(item);
        struct print_context *context = data;
        guint32 id = wp_proxy_get_bound_id(WP_PROXY(obj));
        gboolean is_default = (context->default_node == id);
        const gchar *name = NULL;

        if (cmdline.status.display_nicknames)
            name = wp_pipewire_object_get_property(obj, PW_KEY_NODE_NICK);
        else if (cmdline.status.display_names)
            name = wp_pipewire_object_get_property(obj, PW_KEY_NODE_NAME);

        if (!name)
            name = wp_pipewire_object_get_property(obj, PW_KEY_NODE_DESCRIPTION);

        LOG_INFO("    %c %4u. %-35s", is_default ? '*' : ' ', id, name);
        print_controls(id, context);
    }

    static void print_filter_node(const GValue *item, gpointer data)
    {
        struct print_context *context = data;
        g_autoptr(WpPlugin) def_nodes_api = NULL;
        WpPipewireObject *obj = g_value_get_object(item);
        g_autoptr(WpIterator) it = NULL;
        g_auto(GValue) val = G_VALUE_INIT;
        const gchar *link_group;

        // Skip already printed filters
        link_group = wp_pipewire_object_get_property(obj, PW_KEY_NODE_LINK_GROUP);
        if (g_hash_table_contains(context->printed_filters, link_group))
            return;

        def_nodes_api = wp_plugin_find(context->self->core, "default-nodes-api");

        // Print all nodes for this link_group
        LOG_INFO("      - %-60s\n", link_group);
        it = wp_object_manager_new_filtered_iterator(context->self->om,
                                                     WP_TYPE_NODE,
                                                     WP_CONSTRAINT_TYPE_PW_PROPERTY, PW_KEY_NODE_LINK_GROUP, "=s", link_group,
                                                     NULL);
        for (; wp_iterator_next(it, &val); g_value_unset(&val))
        {
            WpPipewireObject *node = g_value_get_object(&val);
            guint32 id = wp_proxy_get_bound_id(WP_PROXY(node));
            const gchar *name, *media_class;

            name = wp_pipewire_object_get_property(node, PW_KEY_NODE_NAME);
            if (cmdline.status.display_nicknames)
                name = wp_pipewire_object_get_property(node, PW_KEY_NODE_NICK);
            else if (cmdline.status.display_names)
                name = wp_pipewire_object_get_property(node, PW_KEY_NODE_NAME);
            if (!name)
                name = wp_pipewire_object_get_property(node, PW_KEY_NODE_DESCRIPTION);
            media_class = wp_pipewire_object_get_property(node, PW_KEY_MEDIA_CLASS);

            context->default_node = -1;
            if (def_nodes_api)
                g_signal_emit_by_name(def_nodes_api, "get-default-node", media_class,
                                      &context->default_node);

            LOG_INFO("    %c %4u. %-60s [%s]\n",
                     context->default_node == id ? '*' : ' ', id, name, media_class);
        }
        g_clear_pointer(&it, wp_iterator_unref);

        // Insert link-group in table to not print them again
        g_hash_table_insert(context->printed_filters, g_strdup(link_group),
                            NULL);
    }

    static void print_endpoint(const GValue *item, gpointer data)
    {
        WpPipewireObject *obj = g_value_get_object(item);
        struct print_context *context = data;
        guint32 id = wp_proxy_get_bound_id(WP_PROXY(obj));
        guint32 node_id = -1;
        gboolean is_default = (context->default_node == id);
        const gchar *str, *name;

        if ((str = wp_pipewire_object_get_property(obj, "node.id")))
            node_id = atoi(str);

        name = wp_pipewire_object_get_property(obj, "endpoint.description");
        if (!name)
            name = wp_pipewire_object_get_property(obj, "endpoint.name");

        LOG_INFO("    %c %4u. %-35s", is_default ? '*' : ' ', id, name);
        print_controls(node_id, context);
    }

    static void status_run(WpCtl *self)
    {
        g_autoptr(WpIterator) it = NULL;
        g_auto(GValue) val = G_VALUE_INIT;
        g_autoptr(WpPlugin) def_nodes_api = NULL;
        struct print_context context = {.self = self};

        def_nodes_api = wp_plugin_find(self->core, "default-nodes-api");
        context.mixer_api = wp_plugin_find(self->core, "mixer-api");

        /* server + clients */
        LOG_INFO("PipeWire '%s' [%s, %s@%s, cookie:%u]\n=============================\n",
                 wp_core_get_remote_name(self->core),
                 wp_core_get_remote_version(self->core),
                 wp_core_get_remote_user_name(self->core),
                 wp_core_get_remote_host_name(self->core),
                 wp_core_get_remote_cookie(self->core));

        LOG_INFO("  Clients:\n");
        it = wp_object_manager_new_filtered_iterator(self->om, WP_TYPE_CLIENT, NULL);
        for (; wp_iterator_next(it, &val); g_value_unset(&val))
        {
            WpProxy *client = g_value_get_object(&val);
            g_autoptr(WpProperties) properties =
                wp_pipewire_object_get_properties(WP_PIPEWIRE_OBJECT(client));

            LOG_INFO("    "
                     "  %4u. %-35s [%s, %s@%s, pid:%s]\n=============================\n",
                     wp_proxy_get_bound_id(client),
                     wp_properties_get(properties, PW_KEY_APP_NAME),
                     wp_properties_get(properties, PW_KEY_CORE_VERSION),
                     wp_properties_get(properties, PW_KEY_APP_PROCESS_USER),
                     wp_properties_get(properties, PW_KEY_APP_PROCESS_HOST),
                     wp_properties_get(properties, PW_KEY_APP_PROCESS_ID));
        }
        g_clear_pointer(&it, wp_iterator_unref);
        LOG_INFO("\n");
        {
            const gchar *media_type = "Audio";
            g_autoptr(WpIterator) child_it = NULL;
            if (media_type && *media_type != '\0')
            {
                gchar media_type_glob[16];
                gchar media_class[24];

                g_snprintf(media_type_glob, sizeof(media_type_glob), "*%s*", media_type);

                LOG_INFO("############################## Devices: ##############################\n");
                child_it = wp_object_manager_new_filtered_iterator(self->om,
                                                                   WP_TYPE_DEVICE,
                                                                   WP_CONSTRAINT_TYPE_PW_PROPERTY, PW_KEY_MEDIA_CLASS, "#s", media_type_glob,
                                                                   NULL);
                wp_iterator_foreach(child_it, print_device, self);
                g_clear_pointer(&child_it, wp_iterator_unref);

                LOG_INFO("######################################################################\n");

                LOG_INFO("\n__________________________________________________________________________________\n    Sinks:\n");
                g_snprintf(media_class, sizeof(media_class), "%s/Sink", media_type);
                context.default_node = -1;
                if (def_nodes_api)
                    g_signal_emit_by_name(def_nodes_api, "get-default-node", media_class,
                                          &context.default_node);
                child_it = wp_object_manager_new_filtered_iterator(self->om,
                                                                   WP_TYPE_NODE,
                                                                   WP_CONSTRAINT_TYPE_PW_PROPERTY, PW_KEY_MEDIA_CLASS, "#s", "*/Sink*",
                                                                   WP_CONSTRAINT_TYPE_PW_PROPERTY, PW_KEY_MEDIA_CLASS, "#s", media_type_glob,
                                                                   NULL);
                wp_iterator_foreach(child_it, print_dev_node, (gpointer)&context);
                g_clear_pointer(&child_it, wp_iterator_unref);

                LOG_INFO("\n\n__________________________________________________________________________________\n    Sink endpoints:\n");
                child_it = wp_object_manager_new_filtered_iterator(self->om,
                                                                   WP_TYPE_ENDPOINT,
                                                                   WP_CONSTRAINT_TYPE_PW_PROPERTY, PW_KEY_MEDIA_CLASS, "#s", "*/Sink*",
                                                                   WP_CONSTRAINT_TYPE_PW_PROPERTY, PW_KEY_MEDIA_CLASS, "#s", media_type_glob,
                                                                   NULL);
                wp_iterator_foreach(child_it, print_endpoint, (gpointer)&context);
                g_clear_pointer(&child_it, wp_iterator_unref);

                LOG_INFO("\n\n__________________________________________________________________________________\n    Sources:\n");
                g_snprintf(media_class, sizeof(media_class), "%s/Source", media_type);
                context.default_node = -1;
                if (def_nodes_api)
                    g_signal_emit_by_name(def_nodes_api, "get-default-node", media_class,
                                          &context.default_node);
                child_it = wp_object_manager_new_filtered_iterator(self->om,
                                                                   WP_TYPE_NODE,
                                                                   WP_CONSTRAINT_TYPE_PW_PROPERTY, PW_KEY_MEDIA_CLASS, "#s", "*/Source*",
                                                                   WP_CONSTRAINT_TYPE_PW_PROPERTY, PW_KEY_MEDIA_CLASS, "#s", media_type_glob,
                                                                   NULL);
                wp_iterator_foreach(child_it, print_dev_node, (gpointer)&context);
                g_clear_pointer(&child_it, wp_iterator_unref);

                LOG_INFO("\n\n__________________________________________________________________________________\n    Source endpoints:\n");
                child_it = wp_object_manager_new_filtered_iterator(self->om,
                                                                   WP_TYPE_ENDPOINT,
                                                                   WP_CONSTRAINT_TYPE_PW_PROPERTY, PW_KEY_MEDIA_CLASS, "#s", "*/Source*",
                                                                   WP_CONSTRAINT_TYPE_PW_PROPERTY, PW_KEY_MEDIA_CLASS, "#s", media_type_glob,
                                                                   NULL);
                wp_iterator_foreach(child_it, print_endpoint, (gpointer)&context);
                g_clear_pointer(&child_it, wp_iterator_unref);
            }

            LOG_INFO("\n");
        }

        // Default settings
        LOG_INFO("Settings\n");
        if (def_nodes_api)
        {
            LOG_INFO("      Default Configured Node Names:\n");
            for (guint i = 0; i < G_N_ELEMENTS(AUDIO_CLASSES); i++)
            {
                const gchar *name = NULL;
                g_signal_emit_by_name(def_nodes_api, "get-default-configured-node-name",
                                      AUDIO_CLASSES[i], &name);
                if (name)
                    LOG_INFO("      %4u. %-12s  %s\n", i,
                             AUDIO_CLASSES[i], name);
            }
        }

        g_clear_object(&context.mixer_api);
        g_main_loop_quit(self->loop);
    }

    static gboolean status_prepare(WpCtl *self, GError **error)
    {
        wp_object_manager_add_interest(self->om, WP_TYPE_CLIENT, NULL);
        wp_object_manager_add_interest(self->om, WP_TYPE_DEVICE, NULL);
        wp_object_manager_add_interest(self->om, WP_TYPE_NODE, NULL);
        wp_object_manager_add_interest(self->om, WP_TYPE_PORT, NULL);
        wp_object_manager_add_interest(self->om, WP_TYPE_LINK, NULL);
        wp_object_manager_request_object_features(self->om, WP_TYPE_GLOBAL_PROXY,
                                                  WP_PIPEWIRE_OBJECT_FEATURES_MINIMAL);
        wp_object_manager_add_interest(self->om, WP_TYPE_METADATA, NULL);
        return TRUE;
    }

    static void on_plugin_activated(WpObject *p, GAsyncResult *res, WpCtl *ctl)
    {
        g_autoptr(GError) error = NULL;
        if (!wp_object_activate_finish(p, res, &error))
        {
            LOG_ERROR("%s\n", error->message);
            ctl->exit_code = 1;
            g_main_loop_quit(ctl->loop);
            return;
        }

        if (--ctl->pending_plugins == 0)
            wp_core_install_object_manager(ctl->core, ctl->om);
    }

    // main method
    int getAudioDevicesInfo()
    {
        WpCtl ctl = {0};
        g_autoptr(GError) error = NULL;
        g_autofree gchar *summary = NULL;

        // I don't think so...
        // setlocale (LC_ALL, "");
        // setlocale (LC_NUMERIC, "C");
        wp_init(WP_INIT_ALL);

        ctl.context = g_option_context_new(
            "COMMAND [COMMAND_OPTIONS] - WirePlumber Control CLI");
        ctl.loop = g_main_loop_new(NULL, FALSE);
        ctl.core = wp_core_new(NULL, NULL);
        ctl.apis = g_ptr_array_new_with_free_func(g_object_unref);
        ctl.om = wp_object_manager_new();

        {
            // Option 'status'
            GOptionGroup *group;

            /* options */
            group = g_option_group_new("status", NULL, NULL, &ctl, NULL);
            g_option_group_add_entries(group, entries);
            g_option_context_set_main_group(ctl.context, group);

            /* summary */
            summary = g_strdup_printf("Command: status\n Displays the current state of objects in PipeWire");
            g_option_context_set_summary(ctl.context, summary);
        }

        if (!status_prepare(&ctl, &error))
        {
            LOG_ERROR("%s\n", error->message);
            return 1;
        }

        // load required API modules
        if (!wp_core_load_component(ctl.core,
                                    "libwireplumber-module-default-nodes-api", "module", NULL, &error))
        {
            LOG_ERROR("%s\n", error->message);
            return 1;
        }
        if (!wp_core_load_component(ctl.core,
                                    "libwireplumber-module-mixer-api", "module", NULL, &error))
        {
            LOG_ERROR("%s\n", error->message);
            return 1;
        }

        g_ptr_array_add(ctl.apis, wp_plugin_find(ctl.core, "default-nodes-api"));
        g_ptr_array_add(ctl.apis, ({
                            WpPlugin *p = wp_plugin_find(ctl.core, "mixer-api");
                            g_object_set(G_OBJECT(p), "scale", 1 /* cubic */, NULL);
                            p;
                        }));

        // connect to core
        if (!wp_core_connect(ctl.core))
        {
            LOG_ERROR("Could not connect to PipeWire\n");
            return 1;
        }

        // run
        g_signal_connect_swapped(ctl.core, "disconnected",
                                 (GCallback)g_main_loop_quit, ctl.loop);
        g_signal_connect_swapped(ctl.om, "installed",
                                 (GCallback)status_run, &ctl);

        for (guint i = 0; i < ctl.apis->len; i++)
        {
            WpPlugin *plugin = g_ptr_array_index(ctl.apis, i);
            ctl.pending_plugins++;
            wp_object_activate(WP_OBJECT(plugin), WP_PLUGIN_FEATURE_ENABLED, NULL,
                               (GAsyncReadyCallback)on_plugin_activated, &ctl);
        }

        g_main_loop_run(ctl.loop);

        wp_ctl_clear(&ctl);
        return ctl.exit_code;
    }
