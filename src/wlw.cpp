#include <cstring>
#include "wlw.hpp"
State::operator bool() const
{
	return display != nullptr && registry != nullptr && shm != nullptr && compositor != nullptr && xdg_shell;
}
App &App::init(uint8_t bufnum, const char *display_name)
{
	static App instance(bufnum, display_name);
	return instance;
}

App::App(uint8_t bufnum, const char *display_name) : state{}, display_listener{.error = App::display_handle_error, .delete_id = App::display_handle_delete_id}, registry_listener{.global = App::registry_handle_global, .global_remove = App::registry_handle_global_remove}, xdg_shell_listener{.ping = App::ping}
{

	int result = 0;
	if (display_name != nullptr)
	{
		state.display = wl_display_connect(display_name);
	}
	else
	{
		state.display = wl_display_connect(NULL);
	}
	if (!state.display)
	{
		error = new Error(ErrorType::OS, errno);
		return;
	}
	state.registry = wl_display_get_registry(state.display);
	if (!state.registry)
	{
		error = new Error(ErrorType::OS, errno);
		return;
	}
	result = wl_registry_add_listener(state.registry, &registry_listener, this);
	if (result == -1)
	{
		error = new Error(ErrorType::OS, errno);
		return;
	}
	result = wl_display_add_listener(state.display, &display_listener, this);
	if (result == -1)
	{

		error = new Error(ErrorType::OS, errno);
		return;
	}
	result = wl_display_roundtrip(state.display);
	if (result == -1)
	{
		error = new Error(ErrorType::OS, errno);
		return;
	}
	if (state.shm == nullptr)
	{
		error = new Error(ErrorType::USAGE, 0);
		return;
	}
}

App::~App()
{
	if (error)
	{
		delete error;
		error = nullptr;
	}
	if (state.display)
	{
		wl_display_disconnect(state.display);
	}
}

App::operator bool() const
{
	return error == nullptr && state;
}

void App::ping(void *data, xdg_wm_base *xdg_wm_base, uint32_t serial)
{
	xdg_wm_base_pong(xdg_wm_base, serial);
}

void App::display_handle_error(void *data, wl_display *display, void *object_id, uint32_t code, const char *message)
{
	App *app = static_cast<App *>(data);
	app->error = new Error(ErrorType::WL_DISPLAY, code);
}

void App::display_handle_delete_id(void *data, wl_display *display, uint32_t id)
{
}

void App::registry_handle_global(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
	App *app = static_cast<App *>(data);
	if (strcmp(interface, wl_shm_interface.name) == 0)
	{
		app->state.shm = static_cast<wl_shm *>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
	}
	else if (strcmp(interface, wl_compositor_interface.name) == 0)
	{
		app->state.compositor = static_cast<wl_compositor *>(wl_registry_bind(registry, name, &wl_compositor_interface, 4));
	}
	else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
	{
		app->state.xdg_shell = static_cast<xdg_wm_base *>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
		xdg_wm_base_add_listener(app->state.xdg_shell, &app->xdg_shell_listener, app);
	}
}
void App::registry_handle_global_remove(void *data, wl_registry *registry, uint32_t name)
{
}
