#pragma once
#include <cstdint>
#include <cstddef>
#include "error.hpp"
#include "window.hpp"
struct State
{
	wl_display *display;
	wl_registry *registry;
	wl_shm *shm;
	wl_compositor *compositor;
	xdg_wm_base *xdg_shell;

	explicit operator bool() const;
};

struct App
{

public:
	static App &init(uint8_t bufnum = 2, const char *display_name = nullptr);
	~App();
	explicit operator bool() const;

	Error *error = nullptr;

private:
	App(uint8_t bufnum, const char *display_name);
	State state;
	const wl_display_listener display_listener;
	const wl_registry_listener registry_listener;
	const xdg_wm_base_listener xdg_shell_listener;
	static void ping(void *data, xdg_wm_base *xdg_wm_base, uint32_t serial);
	static void display_handle_error(void *data, wl_display *display, void *object_id, uint32_t code, const char *message);
	static void display_handle_delete_id(void *data, wl_display *display, uint32_t id);
	static void registry_handle_global(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
	static void registry_handle_global_remove(void *data, wl_registry *registry, uint32_t name);
};