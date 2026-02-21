#include <core/vulkan/instance.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <systems/engine.hpp>
#include <log.hpp>

#include <cstdint>

namespace niqqa
{
namespace core
{
bool Instance::init() noexcept
{
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "app";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Vulkan Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    std::vector<const char *> extensions = get_required_extensions();

    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    if constexpr (systems::Engine::enable_validation_layers)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(systems::Engine::validation_layers.size());
        create_info.ppEnabledLayerNames = systems::Engine::validation_layers.data();
    }
    else  
    {
        create_info.enabledLayerCount = 0;
    }

    LOG_INFO("Instance", "Creating instance");

    if (vkCreateInstance(&create_info, nullptr, &m_instance) != VK_SUCCESS)
    {
        LOG_ERROR("Instance", "Failed to create instance");
        return false;
    }

    LOG_INFO("Instance", "Instance created");

    return true;
}

void Instance::cleanup() noexcept
{
    vkDestroyInstance(m_instance, nullptr);
}

std::vector<const char *> Instance::get_required_extensions()
{
    uint32_t extension_count = 0;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    std::vector<const char *> extensions(glfw_extensions, glfw_extensions + extension_count);

    return extensions;
}
} // namespace core
} // namespace niqqa