#include <graphics/device.hpp>

#include <graphics/swapchain.hpp>
#include <systems/engine.hpp>
#include <log.hpp>

#include <map>
#include <set>
#include <string>
#include <cstring>
#include <format>

namespace niqqa
{
namespace graphics
{
bool QueueFamilyIndices::is_complete() const noexcept
{
    return graphics_family.has_value() && present_family.has_value();
}

QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) noexcept
{
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    int32_t i = 0;
    for (const auto &queue_family : queue_families)
    {
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphics_family = i;
        }

        if (surface != VK_NULL_HANDLE)
        {
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

            if (present_support)
            {
                indices.present_family = i;
            }
        }

        if (indices.is_complete())
        {
            break;
        }

        ++i;
    }

    return indices;
}

static bool check_device_extension_support(VkPhysicalDevice device, const std::vector<const char *> &extensions) noexcept
{
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string> required_extensions(extensions.begin(), extensions.end());

    for (const auto &extension : available_extensions)
    {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

bool Device::init(VkInstance instance, VkSurfaceKHR surface) noexcept
{
    m_instance = instance;

    if (!pick_physical_device(surface))
    {
        return false;
    }

    if (!create_logical_device(surface))
    {
        return false;
    }

    return true;
}

void Device::cleanup() noexcept
{
    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
}

VkPhysicalDevice Device::gpu() const noexcept
{
    return m_gpu;
}

VkDevice Device::device() const noexcept
{
    return m_device;
}

uint32_t Device::graphics_queue_family() const noexcept
{
    return m_graphics_family;
}

uint32_t Device::present_queue_family() const noexcept
{
    return m_present_family;
}

VkPhysicalDeviceProperties Device::properties() const noexcept
{
    return m_properties;
}

int32_t Device::rate_device_suitability(VkPhysicalDevice device, VkSurfaceKHR surface) noexcept
{
    QueueFamilyIndices indices = find_queue_families(device, surface);
    
    if (!indices.is_complete())
    {
        return 0;
    }

    if (!check_device_extension_support(device, {REQUIRED_EXTS.begin(), REQUIRED_EXTS.end()}))
    {
        return 0;
    }

    if (surface != VK_NULL_HANDLE)
    {
        SwapchainSupportDetails swapchain_support = query_swapchain_support(device, surface);

        if (swapchain_support.formats.empty() || swapchain_support.present_modes.empty())
        {
            return 0;
        }
    }

    VkPhysicalDeviceProperties device_properties{};
    VkPhysicalDeviceFeatures device_features{};

    vkGetPhysicalDeviceProperties(device, &device_properties);
    vkGetPhysicalDeviceFeatures(device, &device_features);

    int32_t score = 0;

    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }

    score += device_properties.limits.maxImageDimension2D;

    if (check_device_extension_support(device, {OPTIONAL_EXTS.begin(), OPTIONAL_EXTS.end()}))
    {
        score += 200;
    }

    if (check_device_extension_support(device, {RT_EXTS.begin(), RT_EXTS.end()}))
    {
        score += 2000;
    }

    return score;
}

bool Device::pick_physical_device(VkSurfaceKHR surface) noexcept
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

    if (device_count == 0)
    {
        LOG_ERROR("Device", "Failed to find GPUs with Vulkan support");
        return false;
    }

    std::string msg = std::format("Found {} GPU(s) with Vulkan support", device_count);
    LOG_INFO("Device", msg);

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

    std::multimap<int32_t, VkPhysicalDevice> candidates;

    for (const auto &device : devices)
    {
        int32_t score = rate_device_suitability(device, surface);
        candidates.insert(std::make_pair(score, device));
    }

    if (candidates.rbegin()->first > 0)
    {
        m_gpu = candidates.rbegin()->second;
    }
    else 
    {
        LOG_ERROR("Device", "Failed to find a suitable GPU");
        return false;
    }

    vkGetPhysicalDeviceProperties(m_gpu, &m_properties);
    vkGetPhysicalDeviceFeatures(m_gpu, &m_features);

    msg = std::format("Selected GPU: %s", m_properties.deviceName);
    LOG_INFO("Device", msg);

    return true;
}

bool Device::create_logical_device(VkSurfaceKHR surface) noexcept
{
    QueueFamilyIndices queue_families = find_queue_families(m_gpu, surface);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families;

    m_graphics_family = queue_families.graphics_family.value();
    m_present_family = queue_families.present_family.value();

    if (surface != VK_NULL_HANDLE)
    {
        unique_queue_families = {queue_families.graphics_family.value(), queue_families.present_family.value()};
    }
    else 
    {
        unique_queue_families = {queue_families.graphics_family.value()};
    }

    queue_create_infos.reserve(unique_queue_families.size());

    float queue_priority = 1.0f;
    for (uint32_t queue_family : unique_queue_families)
    {
        VkDeviceQueueCreateInfo queue_create_info;
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    std::vector<const char *> enabled_extensions;
    
    VkPhysicalDeviceFeatures2 physical_device_features2{};
    physical_device_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physical_device_features2.features = m_features;

    void *p_next = nullptr;

    if (surface != VK_NULL_HANDLE)
    {
        enabled_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state_features{};

    if (is_device_extension_supported("VK_EXT_extended_dynamic_state"))
    {
        enabled_extensions.push_back("VK_EXT_extended_dynamic_state");

        extended_dynamic_state_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
        extended_dynamic_state_features.extendedDynamicState = VK_TRUE;
        extended_dynamic_state_features.pNext = p_next;

        p_next = &extended_dynamic_state_features;
    }

    VkPhysicalDeviceExtendedDynamicState2FeaturesEXT extended_dynamic_state2_features{};

    if (is_device_extension_supported("VK_EXT_extended_dynamic_state2"))
    {
        enabled_extensions.push_back("VK_EXT_extended_dynamic_state2");

        extended_dynamic_state2_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
        extended_dynamic_state2_features.extendedDynamicState2 = VK_TRUE;
        extended_dynamic_state2_features.pNext = p_next;

        p_next = &extended_dynamic_state2_features;
    }

    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extended_dynamic_state3_features{};

    if (is_device_extension_supported("VK_EXT_extended_dynamic_state3"))
    {
        enabled_extensions.push_back("VK_EXT_extended_dynamic_state3");

        extended_dynamic_state3_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
        extended_dynamic_state3_features.extendedDynamicState3PolygonMode = VK_TRUE;
        extended_dynamic_state3_features.extendedDynamicState3DepthClampEnable = VK_TRUE;
        extended_dynamic_state3_features.extendedDynamicState3DepthClipEnable = VK_TRUE;
        extended_dynamic_state3_features.pNext = physical_device_features2.pNext;

        physical_device_features2.pNext = &extended_dynamic_state3_features;
    }
    
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline_features{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features{};
    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR buffer_device_address_features{};
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptor_indexing_features{};
    VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features;

    if (is_device_extension_supported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) &&
        is_device_extension_supported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) &&
        is_device_extension_supported(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) &&
        is_device_extension_supported(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) &&
        is_device_extension_supported(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) &&
        is_device_extension_supported(VK_KHR_RAY_QUERY_EXTENSION_NAME))
    {
        enabled_extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        enabled_extensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        enabled_extensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        enabled_extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        enabled_extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        enabled_extensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);

        ray_tracing_pipeline_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        ray_tracing_pipeline_features.pNext = physical_device_features2.pNext;
        physical_device_features2.pNext = &ray_tracing_pipeline_features;

        acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        acceleration_structure_features.accelerationStructure = true;
        acceleration_structure_features.pNext = physical_device_features2.pNext;
        physical_device_features2.pNext = &ray_tracing_pipeline_features;

        ray_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        ray_query_features.rayQuery = true;
        ray_query_features.pNext = physical_device_features2.pNext;
        physical_device_features2.pNext = &acceleration_structure_features;

        buffer_device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;
        buffer_device_address_features.bufferDeviceAddress = true;
        buffer_device_address_features.pNext = physical_device_features2.pNext;
        physical_device_features2.pNext = &ray_query_features;

        descriptor_indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
        descriptor_indexing_features.shaderSampledImageArrayNonUniformIndexing = true;
        descriptor_indexing_features.runtimeDescriptorArray = true;
        descriptor_indexing_features.descriptorBindingVariableDescriptorCount = true;
        descriptor_indexing_features.descriptorBindingPartiallyBound = true;
        descriptor_indexing_features.pNext = physical_device_features2.pNext;
        physical_device_features2.pNext = &descriptor_indexing_features;
    }

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pNext = &physical_device_features2;

    create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size());
    create_info.ppEnabledExtensionNames = enabled_extensions.data();

    if constexpr (systems::Engine::enable_validation_layers)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(systems::Engine::validation_layers.size());
        create_info.ppEnabledLayerNames = systems::Engine::validation_layers.data();
    }
    else 
    {
        create_info.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_gpu, &create_info, nullptr, &m_device) != VK_SUCCESS)
    {
        LOG_ERROR("Device", "Failed to create logical device");
        return false;
    }
    
    LOG_INFO("Device", "Logical device created");

    vkGetDeviceQueue(m_device, queue_families.graphics_family.value(), 0, &m_graphics_queue);

    if (surface != VK_NULL_HANDLE)
    {
        vkGetDeviceQueue(m_device, queue_families.present_family.value(), 0, &m_present_queue);
    }

    return true;
}

bool Device::is_device_extension_supported(const std::string &extension_name) noexcept
{
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(m_gpu, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(m_gpu, nullptr, &extension_count, extensions.data());

    for (const auto &extension : extensions)
    {
        if (strcmp(extension.extensionName, extension_name.c_str()) == 0)
        {
            return true;
        }
    }

    return false;
}
} // namespace graphics
} // namespace niqqa