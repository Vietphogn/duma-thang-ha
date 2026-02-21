#include <core/windows/window.hpp>

#include <log.hpp>

namespace niqqa
{
namespace core
{
bool Window::should_close() noexcept
{
    return glfwWindowShouldClose(m_window);
}

bool Window::create_surface(VkInstance instance, VkSurfaceKHR &surface) noexcept
{
    LOG_INFO("Surface", "Creating surface");

    if (!glfwCreateWindowSurface(instance, m_window, nullptr, &surface))
    {
        LOG_ERROR("Surface", "Failed to create surface");
        return false;
    }

    LOG_INFO("Surface", "Surface created");

    return true;
}

void Window::poll_events() noexcept
{
    glfwPollEvents();
}

void Window::cleanup() noexcept
{
    glfwDestroyWindow(m_window);
}

bool Window::init(uint32_t width, uint32_t height, const std::string &title, bool resizable, bool fullscreen) noexcept
{
    if (!glfwInit())
    {
        LOG_ERROR("GLFW", "Failed to initialize GLFW");
        return false;
    }

    LOG_INFO("GLFW", "GLFW initialized");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, m_resizable);

    m_window = glfwCreateWindow(static_cast<int>(m_extent.width),
                                static_cast<int>(m_extent.height),
                                m_title.c_str(),
                                nullptr, 
                                nullptr);

    LOG_INFO("Window", "Creating window");

    if (!m_window)
    {
        LOG_ERROR("Window", "Failed to create window");
        return false;
    }

    LOG_INFO("Window", "Window created");

    return true;
}
} // namespace core
} // namespace niqqa