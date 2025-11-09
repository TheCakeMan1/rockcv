#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

int main(void)
{
    VkInstance instance;
    VkInstanceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pApplicationInfo = NULL,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL};

    VkResult res = vkCreateInstance(&info, NULL, &instance);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateInstance failed: %d\n", res);
        return 1;
    }

    uint32_t gpu_count = 0;
    vkEnumeratePhysicalDevices(instance, &gpu_count, NULL);
    printf("Physical devices found: %u\n", gpu_count);

    vkDestroyInstance(instance, NULL);
    return 0;
}
