#define _POSIX_C_SOURCE 200112L
#include "window.hpp"
#include <unistd.h>
#include <sys/mman.h>
#include <sys/random.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <cerrno>

wl_shm *Surface::shm = nullptr;

namespace
{
    void BufferNode::draw_buffer(ssize_t size, uint16_t width, uint16_t height, uint16_t stride, wl_shm_format shm_fmt, wl_shm *shm, std::function<void(void *data, uint16_t width, uint16_t height)> draw)
    {

        int shm_fd = allocate_shm_file(size);
        if (shm_fd == -1)
        {
            error = new Error(ErrorType::OS, errno);
            return;
        }
        void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (data == MAP_FAILED)
        {
            close(shm_fd);
            error = new Error(ErrorType::OS, errno);
            return;
        }
        wl_shm_pool *pool = wl_shm_create_pool(shm, shm_fd, size);
        close(shm_fd);
        this->buffer = wl_shm_pool_create_buffer(pool, 0,
                                                 width, height, stride, shm_fmt);
        wl_shm_pool_destroy(pool);
        close(shm_fd);
        // TODO: pass more shit to the user he cant just imagine a data array and write shit
        draw(data, width, height);

        munmap(data, size);
    }

    BufferNode::~BufferNode()
    {
        if (buffer != nullptr)
        {
            wl_buffer_destroy(buffer);
            buffer = nullptr;
        }
        if (next != nullptr)
        {
            next = nullptr;
        }
    }
    // Only checks if the buffer is full of data or not
    BufferNode::operator bool()
    {
        return buffer != nullptr;
    }
    int BufferNode::create_shm_file()
    {
        char name[48];
        uint32_t cookie = 0;
        ssize_t ret = getrandom(&cookie, sizeof(cookie), 0);
        if (ret == -1)
        {
            error = new Error(ErrorType::OS, errno);
            return -1;
        }
        snprintf(name, sizeof(name), "/wl_shm-%d%d", getpid(), cookie);
        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0)
        {
            shm_unlink(name);
            return fd;
        }
        error = new Error(ErrorType::OS, errno);
        return -1;
    }

    int BufferNode::allocate_shm_file(size_t size)
    {
        int fd = create_shm_file();
        if (fd < 0)
        {
            return -1;
        }
        int ret;
        do
        {
            ret = ftruncate(fd, size);
        } while (ret < 0 && errno == EINTR);
        if (ret < 0)
        {
            close(fd);
            error = new Error(ErrorType::OS, errno);
            return -1;
        }
        return fd;
    }

    BufferRing::BufferRing(uint8_t bufnum) : head(nullptr), bufnum(bufnum), buffer_listener{.release = BufferRing::buffer_release}
    {

        BufferNode *current = head = new BufferNode();
        int result = wl_buffer_add_listener(current->buffer, &buffer_listener, this);
        if (result == -1)
        {
            error = new Error(ErrorType::OS, errno);
            return;
        }
        for (int i = 1; i < bufnum; ++i)
        {
            current->next = new BufferNode();
            current = current->next;
            int result = wl_buffer_add_listener(current->buffer, &buffer_listener, this);
            if (result == -1)
            {
                error = new Error(ErrorType::OS, errno);
                return;
            }
        }
        current->next = head;
    }
    BufferRing::~BufferRing()
    {
        BufferNode *current = head;
        for (int i = 0; i < bufnum; ++i)
        {
            BufferNode *next = this->get_next();
            if (current)
            {
                current->~BufferNode();
            }
            current = next;
        }
        if (error != nullptr)
        {
            delete error;
            error = nullptr;
        }
    }

    BufferNode *BufferRing::get_head()
    {
        if (head == nullptr)
            error = new Error(ErrorType::LOGIC, 0);
        return head;
    }

    BufferNode *BufferRing::get_next()
    {
        if (head->next == nullptr)
            error = new Error(ErrorType::LOGIC, 0);
        return head;
    }

    BufferNode *BufferRing::step()
    {
        if (head->next == nullptr)
        {
            error = new Error(ErrorType::LOGIC, 0);
            return nullptr;
        }
        head = head->next;
        return head;
    }

    uint8_t BufferRing::get_bufnum() const
    {
        return bufnum;
    }

    /*
     *Checks if the linked list is connected to each other
     *Doesn't care if the buffer value is null or not(BufferNode does)
     */
    BufferRing::operator bool()
    {
        BufferNode *current = head;
        for (int i = 0; i < bufnum; ++i)
        {
            if (current->next == nullptr)
                return false;
            current = current->next;
        }
        if (current->next != head)
            return false;
        return true;
    }

    // TODO: Moz Defnitly needs extra work
    void BufferRing::buffer_release(void *data, wl_buffer *buffer)
    {
        BufferRing *list = static_cast<BufferRing *>(data);
        list->step();
    }
}

Surface::Surface(wl_compositor *compositor, xdg_wm_base *xdg_shell, wl_shm *shm, uint16_t width, uint16_t height, uint8_t bufnum, ColorFormat cf) : width(width), height(height), bufnum(bufnum), xdg_surf_listener{.configure = Surface::xdg_surface_configure}
{
    // TODO:Check if those objects need to be checked if they null or not
    Surface::shm = shm;
    if (bufnum < 2)
    {
        bufnum = 2;
    }
    buf_ring = new BufferRing(bufnum);
    surf = wl_compositor_create_surface(compositor);
    xdg_surf = xdg_wm_base_get_xdg_surface(xdg_shell, surf);
    int result = xdg_surface_add_listener(xdg_surf, &xdg_surf_listener, this);
    if (result == -1)
    {
        error = new Error(ErrorType::OS, errno);
        return;
    }
    this->set_shm_fmt(cf);
}
Surface::~Surface()
{
    delete buf_ring;
    wl_surface_destroy(surf);
    xdg_surface_destroy(xdg_surf);
}

void Surface::draw(std::function<void(void *data, uint16_t width, uint16_t height)> surf_draw)
{
    // TODO: logic isnt logicing
    buf_ring->get_next()->draw_buffer(get_buffer_size(), width, height, get_stride(), shm_format, shm, surf_draw);
    // wl_surface_attach(surf, current_buffer->buffer, 0, 0);
    // wl_surface_commit(surf);
}
uint16_t Surface::get_stride()
{
    if (width == 0)
    {
        error = new Error(ErrorType::USAGE, 1);
        return 0;
    }
    uint8_t row = width * bpp;
    uint8_t cache_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    uint8_t alignment = 4;
    if (cache_size != -1 || cache_size > alignment)
    {
        alignment = cache_size;
    }
    return ((row + alignment - 1) & ~(alignment - 1));
}

uint16_t Surface::get_width()
{
    return width;
}
uint16_t Surface::get_height()
{
    return height;
}

uint32_t Surface::get_buffer_size()
{
    if (height == 0 || width == 0)
    {
        error = new Error(ErrorType::USAGE, 1);
        return 0;
    }
    return get_stride() * height;
}

void Surface::set_shm_fmt(ColorFormat cf)
{
    switch (cf)
    {
    case ColorFormat::ARGB:
        bpp = 4;
        shm_format = WL_SHM_FORMAT_ARGB8888;
        break;
    case ColorFormat::XRGB:
        bpp = 4;
        shm_format = WL_SHM_FORMAT_XRGB8888;
        break;
    case ColorFormat::ABGR:
        bpp = 4;
        shm_format = WL_SHM_FORMAT_ABGR8888;
        break;
    case ColorFormat::XBGR:
        bpp = 4;
        shm_format = WL_SHM_FORMAT_XBGR8888;
        break;
    case ColorFormat::RGB_24:
        bpp = 4;
        shm_format = WL_SHM_FORMAT_RGB888;
        break;
    case ColorFormat::BGR_24:
        bpp = 3;
        shm_format = WL_SHM_FORMAT_BGR888;
        break;
    case ColorFormat::YUYV:
        bpp = 2;
        shm_format = WL_SHM_FORMAT_YUYV;
        break;
    case ColorFormat::YVYU:
        bpp = 2;
        shm_format = WL_SHM_FORMAT_YVYU;
        break;
    case ColorFormat::UYVY:
        bpp = 2;
        shm_format = WL_SHM_FORMAT_UYVY;
        break;
    case ColorFormat::VYUY:
        bpp = 2;
        shm_format = WL_SHM_FORMAT_VYUY;
        break;
    case ColorFormat::RGB_8:
        bpp = 1;
        shm_format = WL_SHM_FORMAT_RGB332;
        break;
    case ColorFormat::BGR_8:
        bpp = 1;
        shm_format = WL_SHM_FORMAT_BGR233;
        break;
    }
}
void Surface::resize(uint16_t width, uint16_t height)
{
    if (max_width >= width >= min_width && max_height >= height >= min_height)
    {
        this->width = width;
        this->height = height;
        // xdg_surface_set_window_geometry(xdg_surf, 0, 0, width, height);
    }
    else
    {

        error = new Error(ErrorType::USAGE, 1);
    }
}
void Surface::set_max_dimesnions(uint16_t max_width, uint16_t max_height)
{
    this->max_width = max_width;
    this->max_height = max_height;
}

void Surface::set_min_dimensions(uint16_t min_width, uint16_t min_height)
{
    this->min_width = min_width;
    this->min_height = min_height;
}

void Surface::xdg_surface_configure(void *data, xdg_surface *surface, uint32_t serial)
{
    // TODO: SOMESHIT GOTA BE HERE
    Surface *surf = static_cast<Surface *>(data);
    xdg_surface_ack_configure(surface, serial);
}
