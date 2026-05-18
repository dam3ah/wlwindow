#pragma once
#include <wayland-client.h>
#include <xdg-shell-protocol.h>
#include <functional>
#include "error.hpp"
namespace
{
    struct BufferNode
    {
        wl_buffer *buffer = nullptr;
        BufferNode *next = nullptr;
        Error *error = nullptr;
        void draw_buffer(ssize_t size, uint16_t width, uint16_t height, uint16_t stride, wl_shm_format shm_fmt, wl_shm *shm, std::function<void(void *data, uint16_t width, uint16_t height)> draw);
        ~BufferNode();
        explicit operator bool();
        BufferNode &operator=(const BufferNode &other) = delete;

    private:
        int create_shm_file();
        int allocate_shm_file(size_t size);
    };
    struct BufferRing
    {
    public:
        BufferRing(uint8_t bufnum);
        ~BufferRing();
        BufferNode *get_head();
        BufferNode *get_next();
        BufferNode *step();
        uint8_t get_bufnum() const;
        Error *error = nullptr;

    private:
        BufferNode *head;
        uint8_t bufnum;
        BufferRing &operator=(const BufferRing &other) = delete;
        explicit operator bool();
        const wl_buffer_listener buffer_listener;
        static void buffer_release(void *data, wl_buffer *buffer);
    };

}
enum class ColorFormat
{
    ARGB,
    ABGR,
    XRGB,
    XBGR,
    RGB_24,
    BGR_24,
    YUYV,
    YVYU,
    UYVY,
    VYUY,
    RGB_8,
    BGR_8
};

// TODO: implement windwos
struct Surface
{
    Surface(wl_compositor *compositor, xdg_wm_base *xdg_shell, wl_shm *shm, uint16_t width, uint16_t height, uint8_t bufnum, ColorFormat cf = ColorFormat::ARGB);
    ~Surface();
    void draw(std::function<void(void *data, uint16_t width, uint16_t height)> surf_draw);

private:
    uint8_t bufnum;
    static wl_shm *shm;
    BufferRing *buf_ring;
    wl_surface *surf;
    xdg_surface *xdg_surf;
    uint16_t min_width = 0, min_height = 0, width, height, max_width = UINT16_MAX, max_height = UINT16_MAX, get_stride(), get_width(), get_height();
    uint8_t bpp;
    wl_shm_format shm_format;
    Error *error = nullptr;
    uint32_t get_buffer_size();
    void set_shm_fmt(ColorFormat cf);
    void resize(uint16_t width, uint16_t height);
    void set_max_dimesnions(uint16_t max_width, uint16_t max_height);
    void set_min_dimensions(uint16_t min_width, uint16_t min_height);
    const xdg_surface_listener xdg_surf_listener;
    static void xdg_surface_configure(void *data, xdg_surface *surface, uint32_t serial);
};
