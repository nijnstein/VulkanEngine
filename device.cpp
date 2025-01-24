#include "defines.h"

namespace vkengine
{       
    void VulkanDevice::initDeviceProperties()
    {
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
        vkGetPhysicalDeviceProperties(physicalDevice, &gpuProperties);
    }

    std::vector<const char*> VulkanDevice::getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (VERBOSE)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        //extensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);

        return extensions;
    }

    void VulkanDevice::initInstance()
    {
        if (VERBOSE && !checkValidationLayerSupport())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = APPLICATION_NAME.c_str();

        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "VulkanEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto glfwExtensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
        createInfo.ppEnabledExtensionNames = glfwExtensions.data();

        static VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

        if (VERBOSE)
        {
            // log extension information to console 
            std::cout << glfwExtensions.size() << " glfw extensions required" << std::endl;
            for (int i = 0; i < glfwExtensions.size(); i++)
            {
                std::cout << '\t' << glfwExtensions[i] << std::endl;
            }

            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> extensions(extensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

            std::cout << extensionCount << " available vulkan extensions:" << std::endl;
            for (const auto& extension : extensions)
            {
                std::cout << '\t' << extension.extensionName << std::endl;
            }

            // enable validation layers 
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            std::cout << createInfo.enabledLayerCount << " validation layers enabled" << std::endl;
            for (int i = 0; i < (int)createInfo.enabledLayerCount; i++)
            {
                std::cout << '\t' << validationLayers[i] << std::endl;
            }

            // debug instance create/destroy 
            auto debugCreateInfo = vkengine::debug::initDebugMessengerCreateInfo(this);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));
    }

    void VulkanDevice::initVMA()
    {
        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        allocatorCreateInfo.physicalDevice = physicalDevice;
        allocatorCreateInfo.device = device;
        allocatorCreateInfo.instance = instance;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

        vmaCreateAllocator(&allocatorCreateInfo, &allocator);
    }

    void VulkanDevice::initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(configuration.frameBufferSize.width, configuration.frameBufferSize.height, configuration.windowName.c_str(), nullptr, nullptr);

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    void VulkanDevice::initExtensions()
    {
        vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(device, "vkCmdPushDescriptorSetKHR");
        if (!vkCmdPushDescriptorSetKHR)
        {
            throw std::runtime_error("Could not get a valid function pointer for vkCmdPushDescriptorSetKHR");
        }

        vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR"));
        if (!vkGetPhysicalDeviceProperties2KHR)
        {
            throw std::runtime_error("Could not get a valid function pointer for vkGetPhysicalDeviceProperties2KHR");
        }

        //vkCmdSetPrimitiveTopologyEXT = (PFN_vkCmdSetPrimitiveTopologyEXT)vkGetInstanceProcAddr(instance, "vkCmdSetPrimitiveTopologyEXT");
        //if (!vkCmdSetPrimitiveTopologyEXT)
        //{
        //	throw std::runtime_error("Could not get a valid function pointer for vkCmdSetPrimitiveTopologyEXT");
        //}
        //
        //vkCmdSetPolygonModeEXT = (PFN_vkCmdSetPolygonModeEXT)vkGetInstanceProcAddr(instance, "vkCmdSetPolygonModeEXT");
        //if (!vkCmdSetPolygonModeEXT)
        //{
        //	throw std::runtime_error("Could not get a valid function pointer for vkCmdSetPolygonModeEXT");
        //}
        //
        //vkCmdSetLineWidth = (PFN_vkCmdSetLineWidth)vkGetInstanceProcAddr(instance, "vkCmdSetLineWidth");
        //if (!vkCmdSetLineWidth)
        //{
        //	throw std::runtime_error("Could not get a valid function pointer for vkCmdSetLineWidth");
        //}
    }

    void VulkanDevice::initPushDescriptors()
    {
        VkPhysicalDeviceProperties2KHR deviceProps2{};
        pushDescriptorProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;
        deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
        deviceProps2.pNext = &pushDescriptorProps;
        vkGetPhysicalDeviceProperties2KHR(physicalDevice, &deviceProps2);
    }

    void VulkanDevice::framebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window))->framebufferResized = true;
    }

    void VulkanDevice::initDebugMessenger()
    {
        if (!VERBOSE) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo = vkengine::debug::initDebugMessengerCreateInfo(this);

        VK_CHECK(vkengine::debug::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger));
    }

    void VulkanDevice::initPhysicalDevice()
    {           
        physicalDevice = VK_NULL_HANDLE;

        VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0)
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        std::multimap<int, VkPhysicalDevice> candidates;
        for (const auto& device : devices)
        {
            int score = vkengine::rateDeviceSuitability(device, surface);
            if (score > 0)
            {
                candidates.insert(std::make_pair(score, device));
            }
        }

        if (candidates.rbegin()->first > 0)
        {
            physicalDevice = candidates.rbegin()->second;
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        initDeviceProperties();

        maxMSAASamples = vkengine::getMaxUsableSampleCount(physicalDevice);
        currentMSAASamples = maxMSAASamples < VK_SAMPLE_COUNT_4_BIT ? maxMSAASamples : VK_SAMPLE_COUNT_4_BIT;

        DEBUG("Vulkan %d, device: %s", gpuProperties.apiVersion, gpuProperties.deviceName);
        DEBUG("	- maxMSAA = %dx, selected %dx MSAA\n", maxMSAASamples, currentMSAASamples)
    }

    void VulkanDevice::initLogicalDevice()
    {
        auto indices = QueueFamilyIndices::find(surface, physicalDevice);

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;

        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceFeatures.sampleRateShading = VK_TRUE;
        deviceFeatures.multiDrawIndirect = VK_TRUE;
        deviceFeatures.depthClamp = VK_TRUE;
        deviceFeatures.fillModeNonSolid = VK_TRUE; 

        VkDeviceCreateInfo createInfo{};

        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (VERBOSE)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void VulkanDevice::initSwapChain()
    {
        auto swapChainSupport = SwapChainSupportDetails::query(physicalDevice, surface);
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(configuration.enableVSync, swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(window, swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // render to seperate image first for post-processing: VK_IMAGE_USAGE_TRANSFER_DST_BIT

        auto indices = QueueFamilyIndices::find(surface, physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // perform an optional transform to the image before writing it to the swap (like 90degree flip)
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ignore alpha channel
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain));

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void VulkanDevice::initSwapChainBuffers(VkRenderPass renderPass)
    {
        swapChainImageViews.resize(swapChainImages.size());

        for (uint32_t i = 0; i < swapChainImages.size(); i++)
        {
            swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }

        VkFormat depthFormat = findDepthFormat(physicalDevice);
        depthImage = createImage(swapChainExtent.width, swapChainExtent.height, 1, getCurrentMSAASamples(), depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "depthbuffer");
        depthImage.view = createImageView(depthImage.img, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

        transitionImageLayout(depthImage.img, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

        VkFormat colorFormat = swapChainImageFormat;
        colorImage = createImage(
            swapChainExtent.width, swapChainExtent.height,
            1,
            getCurrentMSAASamples(),
            colorFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            "colorBuffer");

        colorImage.view = createImageView(colorImage.img, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

        swapChainFramebuffers.resize(swapChainImageViews.size());
        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            std::array<VkImageView, 3> attachments =
            {
                colorImage.view,
                depthImage.view,
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            VK_CHECK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]));
        }
    }

    void VulkanDevice::recreateSwapChain(VkRenderPass renderPass)
    {
        int width = 0, height = 0;
        framebufferResized = false;

        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0)
        {
            // just wait until unminimized
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();
        initSwapChain();
        initSwapChainBuffers(renderPass);

        updateAfterResize();
    }

    void VulkanDevice::invalidateFrameBuffer()
    {
        framebufferResized = true;
    }

    void VulkanDevice::cleanupSwapChain()
    {
        vkDestroyImageView(device, colorImage.view, nullptr);
        vmaDestroyImage(allocator, colorImage.img, colorImage.allocation);

        vkDestroyImageView(device, depthImage.view, nullptr);
        vmaDestroyImage(allocator, depthImage.img, depthImage.allocation);

        for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
        {
            vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
        }

        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            vkDestroyImageView(device, swapChainImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    void VulkanDevice::initIndirectCommandBuffers(PipelineInfo& pipeline)
    {
        destroyIndirectCommandBuffers(pipeline); 

        if (configuration.enableIndirect)
        {   
            pipeline.indirectCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT); 
            pipeline.indirectCommandBufferInvalidations.resize(MAX_FRAMES_IN_FLIGHT); 

            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                createBuffer(
                    configuration.maxIndirectCommandCount * sizeof(RenderInfo),
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                    pipeline.indirectCommandBuffers[i],
                    "indirectCommandBuffer"
                );

                pipeline.indirectCommandBufferInvalidations[i] = true;
            }
        }
    }

    void VulkanDevice::invalidateDrawCommandBuffers(PipelineInfo& pipeline)
    {
        for (int i = 0; i < pipeline.indirectCommandBufferInvalidations.size(); i++)
        {
            pipeline.indirectCommandBufferInvalidations[i] = true;
        }
    }

    void VulkanDevice::invalidateDrawCommandBuffers(RenderSet& set)
    {
        for (auto [m, pi] : set.pipelines)
        {
            invalidateDrawCommandBuffers(pi); 
        }
    }

    void VulkanDevice::destroyIndirectCommandBuffers(PipelineInfo& pipeline)
    {
        for (auto& buffer : pipeline.indirectCommandBuffers)
        {
            if (buffer.isAllocated())
            {
                DESTROY_BUFFER(allocator, buffer);
            }
        }
        pipeline.indirectCommandBuffers.resize(0); 
    }

    void VulkanDevice::initCommandPool()
    {
        auto queueFamilyIndices = vkengine::QueueFamilyIndices::find(surface, physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        VK_CHECK(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool));
    }

    VkCommandBuffer VulkanDevice::beginCommandBuffer(VkCommandBufferLevel level)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = level;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void VulkanDevice::endCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void VulkanDevice::endCommandBuffer(VkCommandBuffer commandBuffer)
    {
        endCommandBuffer(commandBuffer, graphicsQueue); 
    }

    void VulkanDevice::initPipelineCache()
    {
        START_TIMER

        // attempt to load cache data 
        std::vector<uint8_t> data;
        if (io::FileExists(pipelineCacheFilename))
        {
            data = io::ReadFile(pipelineCacheFilename);
        }

        if (data.size() > 0)
        {
            DEBUG("Pipeline cacheData loaded from %s with size %d bytes\n", pipelineCacheFilename.c_str(), data.size());

            // check cache data
            //
            // Offset	 Size            Meaning
            // ------    ------------    ------------------------------------------------------------------
            //      0               4    length in bytes of the entire pipeline cache header written as a
            //                           stream of bytes, with the least significant byte first
            //      4               4    a VkPipelineCacheHeaderVersion value written as a stream of bytes,
            //                           with the least significant byte first
            //      8               4    a vendor ID equal to VkPhysicalDeviceProperties::vendorID written
            //                           as a stream of bytes, with the least significant byte first
            //     12               4    a device ID equal to VkPhysicalDeviceProperties::deviceID written
            //                           as a stream of bytes, with the least significant byte first
            //     16    VK_UUID_SIZE    a pipeline cache ID equal to VkPhysicalDeviceProperties::pipelineCacheUUID
            //

            uint32_t headerLength = 0;
            uint32_t cacheHeaderVersion = 0;
            uint32_t vendorID = 0;
            uint32_t deviceID = 0;
            uint8_t pipelineCacheUUID[VK_UUID_SIZE] = {};

            memcpy(&headerLength, (uint8_t*)data.data() + 0, 4);
            memcpy(&cacheHeaderVersion, (uint8_t*)data.data() + 4, 4);
            memcpy(&vendorID, (uint8_t*)data.data() + 8, 4);
            memcpy(&deviceID, (uint8_t*)data.data() + 12, 4);
            memcpy(pipelineCacheUUID, (uint8_t*)data.data() + 16, VK_UUID_SIZE);

            // check fields 
            bool badCache = false;

            if (headerLength <= 0) 
            {
                badCache = true;
                DEBUG("Bad header length in %s.\n", pipelineCacheFilename.c_str())
                DEBUG("    Cache contains: 0x%.8x\n", headerLength)
            }

            if (cacheHeaderVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE) 
            {
                badCache = true;
                DEBUG("Unsupported cache header version in %s.\n", pipelineCacheFilename.c_str())
                DEBUG("    Cache contains: 0x%.8x\n", cacheHeaderVersion)
            }

            if (vendorID != gpuProperties.vendorID)
            {
                badCache = true;
                DEBUG("Vendor ID mismatch in %s.\n", pipelineCacheFilename.c_str())
                DEBUG("    Cache contains: 0x%.8x\n", vendorID)
                DEBUG("    Driver expects: 0x%.8x\n", gpuProperties.vendorID)
            }

            if (deviceID != gpuProperties.deviceID)
            {
                badCache = true;
                DEBUG("Device ID mismatch in %s.\n", pipelineCacheFilename.c_str())
                DEBUG("    Cache contains: 0x%.8x\n", deviceID)
                DEBUG("    Driver expects: 0x%.8x\n", gpuProperties.deviceID)
            }

            if (memcmp(pipelineCacheUUID, gpuProperties.pipelineCacheUUID, sizeof(pipelineCacheUUID)) != 0)
            {
                badCache = true;
                DEBUG("UUID mismatch in %s.\n", pipelineCacheFilename.c_str())
                DEBUG("    Cache contains: %s\n", uuidToString(pipelineCacheUUID))
                DEBUG("    Driver expects: %s\n", uuidToString(gpuProperties.pipelineCacheUUID))
            }

            if (badCache)
            {
                // Don't submit initial cache data if any version info is incorrect
                data.resize(0);

                // And clear out the old cache file for use in next run
                DEBUG("Deleting cache entry %s to repopulate.\n", pipelineCacheFilename.c_str());
                io::DeleteFile(pipelineCacheFilename);
            }
        }
	 
        // Feed the initial cache data into cache creation
        VkPipelineCacheCreateInfo pipelineCacheCreate;
        pipelineCacheCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pipelineCacheCreate.pNext = NULL;
        pipelineCacheCreate.initialDataSize = data.size();
        pipelineCacheCreate.pInitialData = data.size() > 0 ? data.data() : nullptr;
        pipelineCacheCreate.flags = 0;

        VK_CHECK(vkCreatePipelineCache(device, &pipelineCacheCreate, nullptr, &pipelineCache))
        END_TIMER("Created pipeline cache in ")
    }

    void VulkanDevice::destroyPipelineCache() 
    { 
        size_t endCacheSize = 0;
        void* endCacheData = nullptr;

        // Call with nullptr to get cache size
        VK_CHECK(vkGetPipelineCacheData(device, pipelineCache, &endCacheSize, nullptr))

        // Allocate memory to hold the populated cache data
        endCacheData = (char*)malloc(sizeof(char) * endCacheSize);
        if (!endCacheData)
        {
            DEBUG("Memory error");
            exit(EXIT_FAILURE);
        }

        // Call again with pointer to buffer
        VK_CHECK(vkGetPipelineCacheData(device, pipelineCache, &endCacheSize, endCacheData))
        
        // Write the file to disk, overwriting whatever was there
        io::WriteFile(pipelineCacheFilename.c_str(), endCacheData, endCacheSize); 
        free(endCacheData); 

        vkDestroyPipelineCache(device, pipelineCache, NULL);
    }

    void VulkanDevice::initCommandBuffers()
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
        VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()));
    }

    void VulkanDevice::initSyncObjects()
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]))
            VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]))
            VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]))
        }
    }

    Image VulkanDevice::allocateImage(VkImageCreateInfo imgCreateInfo, const char* debugname)
    {
        START_TIMER

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocCreateInfo.priority = 1.0f;

        Image image{};
        vmaCreateImage(allocator, &imgCreateInfo, &allocCreateInfo, &image.img, &image.allocation, nullptr);

        SET_ALLOCATION_NAME(allocator, image.allocation, debugname)

        image.name = debugname;
        image.mipmapLevels = imgCreateInfo.mipLevels;
        image.width = imgCreateInfo.extent.width;
        image.height = imgCreateInfo.extent.height;
        image.depth = imgCreateInfo.extent.depth;
        image.format = imgCreateInfo.format;
        image.layout = imgCreateInfo.initialLayout;
        image.layerCount = imgCreateInfo.arrayLayers;
        image.flags = imgCreateInfo.flags;

        END_TIMER("Allocating image %s took: ", debugname)
        DEBUG_IMAGE(image)

        return image;
    }

    Image VulkanDevice::allocateStagedImage(VkImageCreateInfo imgCreateInfo, bool autoGenerateMipmaps, void* data, uint32_t dataSize, VkFormat dataFormat, const char* debugname)
    {
        DEBUG("Staging image allocation: %s\n", debugname);
        BEGIN_COMMAND_BUFFER

        VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufCreateInfo.size = dataSize;
        bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer buf;
        VmaAllocation alloc;
        VmaAllocationInfo allocInfo;
        vmaCreateBuffer(allocator, &bufCreateInfo, &allocCreateInfo, &buf, &alloc, &allocInfo);

        memcpy(allocInfo.pMappedData, data, dataSize);

        VkImage img;
        VmaAllocation allocImg;
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocCreateInfo.priority = 1.0f;

        vmaCreateImage(allocator, &imgCreateInfo, &allocCreateInfo, &img, &allocImg, nullptr);

        SET_ALLOCATION_NAME(allocator, allocImg, debugname)

        transitionImageLayout(img, dataFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imgCreateInfo.mipLevels);
        copyBufferToImage(buf, img, static_cast<uint32_t>(imgCreateInfo.extent.width), static_cast<uint32_t>(imgCreateInfo.extent.height));

        // transition not needed when generating mipmaps as the generation also performs the transfer of layout			
        if (autoGenerateMipmaps && imgCreateInfo.mipLevels > 1)
        {
            generateMipmaps(img, imgCreateInfo.format, imgCreateInfo.extent.width, imgCreateInfo.extent.height, imgCreateInfo.mipLevels);
        }
        else
        {
            transitionImageLayout(img, imgCreateInfo.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imgCreateInfo.mipLevels, imgCreateInfo.arrayLayers);
        }

        vmaDestroyBuffer(allocator, buf, alloc);

        Image image{};
        image.name = debugname;
        image.allocation = allocImg;
        image.img = img;

        image.mipmapLevels = imgCreateInfo.mipLevels;
        image.width = imgCreateInfo.extent.width;
        image.height = imgCreateInfo.extent.height;
        image.depth = imgCreateInfo.extent.depth;
        image.format = imgCreateInfo.format;
        image.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image.layerCount = imgCreateInfo.arrayLayers; 
        image.flags = imgCreateInfo.flags;

        END_COMMAND_BUFFER
        DEBUG_IMAGE(image)

        return image;
    }

    void VulkanDevice::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount)
    {
        BEGIN_COMMAND_BUFFER

            transitionImageLayout(commandBuffer, image, format, oldLayout, newLayout, mipLevels, layerCount); 

        END_COMMAND_BUFFER
    }

    void VulkanDevice::transitionImageLayout(Image& image, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        BEGIN_COMMAND_BUFFER

        transitionImageLayout(commandBuffer, image, oldLayout, newLayout); 

        END_COMMAND_BUFFER
    }

    void VulkanDevice::transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layerCount;

        barrier.srcAccessMask = 0; // TODO
        barrier.dstAccessMask = 0; // TODO

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (hasStencilComponent(format))
            {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        else
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = 0;//VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = 0;//VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    void VulkanDevice::transitionImageLayout(VkCommandBuffer commandBuffer, Image& image, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image.img;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = image.mipmapLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        barrier.srcAccessMask = 0; // TODO
        barrier.dstAccessMask = 0; // TODO

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (hasStencilComponent(image.format))
            {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        else
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else
                if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
                {
                    barrier.srcAccessMask = 0;
                    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                    sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                    destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                }
                else
                    if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                    {
                        barrier.srcAccessMask = 0;
                        barrier.dstAccessMask = 0;

                        sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    }
                    else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        image.layout = newLayout;
        image.updateDescriptor(); 
    }

    void VulkanDevice::transitionImageLayout(Image& image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subResourceRange)
    {
        BEGIN_COMMAND_BUFFER
        transitionImageLayout(commandBuffer, image, oldLayout, newLayout, subResourceRange); 
        END_COMMAND_BUFFER
    }
    
    void VulkanDevice::transitionImageLayout(VkCommandBuffer commandBuffer, Image& image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subResourceRange)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image.img;
        barrier.subresourceRange = subResourceRange; 

        barrier.srcAccessMask = 0; // TODO
        barrier.dstAccessMask = 0; // TODO

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;


        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        image.layout = newLayout;
        image.updateDescriptor(); 
    }

    void VulkanDevice::setImageLayout(
        VkCommandBuffer cmdbuffer,
        Image& image,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkImageSubresourceRange subresourceRange,
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask)
    {
        // Create an image barrier object
        VkImageMemoryBarrier imageMemoryBarrier = init::imageMemoryBarrier();
        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = image.img;
        imageMemoryBarrier.subresourceRange = subresourceRange;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (oldImageLayout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            // Image layout is undefined (or does not matter)
            // Only valid as initial layout
            // No flags required, listed only for completeness
            imageMemoryBarrier.srcAccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // Image is preinitialized
            // Only valid as initial layout for linear images, preserves memory contents
            // Make sure host writes have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image is a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image is a depth/stencil attachment
            // Make sure any writes to the depth/stencil buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image is a transfer source
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image is a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image is read by a shader
            // Make sure any shader reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (newImageLayout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image will be used as a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image will be used as a transfer source
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image will be used as a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image layout will be used as a depth/stencil attachment
            // Make sure any writes to depth/stencil buffer have been finished
            imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image will be read in a shader (sampler, input attachment)
            // Make sure any writes to the image have been finished
            if (imageMemoryBarrier.srcAccessMask == 0)
            {
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
        }

        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(
            cmdbuffer,
            srcStageMask,
            dstStageMask,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

        image.layout = newImageLayout;
        image.updateDescriptor(); 
    }

    // Fixed sub resource on first mip level and layer
    void VulkanDevice::setImageLayout(
        VkCommandBuffer cmdbuffer,
        Image& image,
        VkImageAspectFlags aspectMask,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask)
    {
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = aspectMask;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;
        
        setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
    }

    void VulkanDevice::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
    {
        // Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        BEGIN_COMMAND_BUFFER

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        END_COMMAND_BUFFER
    }

    void VulkanDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        BEGIN_COMMAND_BUFFER

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        END_COMMAND_BUFFER
    }

    void VulkanDevice::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        BEGIN_COMMAND_BUFFER

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        END_COMMAND_BUFFER
    }

    void VulkanDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags, Buffer& buffer, const char* debugname, float reserve)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = reserve > 1.0f ? (UINT)(size * reserve) : size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = flags;

        if (flags & VMA_ALLOCATION_CREATE_MAPPED_BIT)
        {
            VmaAllocationInfo allocationInfo{};
            VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.alloc, &allocationInfo));
            buffer.mappedData = allocationInfo.pMappedData;
        }
        else
        {
            VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.alloc, nullptr));
        }

        buffer.info = bufferInfo;
        buffer.updateDescriptor();

        SET_BUFFER_NAME(allocator, buffer, debugname)
        DEBUG_BUFFER(buffer)
    }

    void VulkanDevice::createBuffer(VkDeviceSize size, void* data, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags, Buffer& buffer, const char* debugname, float reserve)
    {
        VkBufferCreateInfo stagingInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        stagingInfo.size = reserve > 1.0f ? (UINT)(size * reserve) : size;
        stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo allocStagedInfo = {};
        allocStagedInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocStagedInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuffer;
        VmaAllocation alloc;
        VmaAllocationInfo allocInfo;
        vmaCreateBuffer(allocator, &stagingInfo, &allocStagedInfo, &stagingBuffer, &alloc, &allocInfo);

        memcpy(allocInfo.pMappedData, data, size);

        createBuffer(stagingInfo.size, usage, flags, buffer, debugname);
        copyBuffer(stagingBuffer, buffer.buffer, size);

        vmaDestroyBuffer(allocator, stagingBuffer, alloc);
    }

    Image VulkanDevice::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, const char* debugname)
    {
        VkImageCreateInfo imageInfo{};

        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = numSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        return allocateImage(imageInfo, debugname);
    }

    Image VulkanDevice::createCubemap(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, const char* debugname)
    {
        VkImageCreateInfo imageInfo{};

        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 6;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; 

        return allocateImage(imageInfo, debugname);
    }

    Image VulkanDevice::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, void* data, uint32_t dataSize, const char* debugname)
    {
        VkImageCreateInfo imageInfo{};

        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = numSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
       
        return allocateStagedImage(imageInfo, true, data, dataSize, format, debugname);
    }

    VkImageView VulkanDevice::createImageView(VkImage img, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipmapLevels, uint32_t layerCount)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = img;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipmapLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = layerCount;
        viewInfo.subresourceRange.aspectMask = aspectFlags;

        VkImageView imageView;
        VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &imageView));

        return imageView;
    }

    void VulkanDevice::createImageView(Image& image, VkImageAspectFlags aspectFlags)
    {
        bool isCubemap = image.layerCount == 6 || image.isCube;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image.img;
        if ((image.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) > 0 || isCubemap)
        {
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        }
        else
        {
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        }
        viewInfo.format = image.format;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = image.mipmapLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = image.layerCount;
        viewInfo.subresourceRange.aspectMask = aspectFlags;

        VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &image.view));
    }

    void VulkanDevice::createImageSampler(Image& image)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0.0f; // Optional
        samplerInfo.maxLod = static_cast<float>(image.mipmapLevels);
        samplerInfo.mipLodBias = 0.0f; // Optional

        VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &image.sampler));
    }

    void VulkanDevice::createImageSampler(Image& image, VkSamplerCreateInfo info)
    {
        VK_CHECK(vkCreateSampler(device, &info, nullptr, &image.sampler));
    }
    
    VkSampleCountFlagBits VulkanDevice::getMaxMSAASamples() const
    {
        return maxMSAASamples;
    }

    VkSampleCountFlagBits VulkanDevice::getCurrentMSAASamples() const
    {
        return currentMSAASamples;
    }

    uint32_t VulkanDevice::getCurrentFrame() const
    {
        return currentFrame;
    }

    bool VulkanDevice::hasFrameBufferResized() const
    {   
        return framebufferResized; 
    }

    VkFormat VulkanDevice::getSwapChainFormat() 
    {
        return swapChainImageFormat;
    }

    TextureInfo VulkanDevice::initTexture(std::string path, VkFormat format, bool ensureWithView, bool ensureWithSampler, bool isColor, bool isCube)
    {
        return getTexture(registerTexture(path, format, isColor, isCube).id, ensureWithView, ensureWithSampler);
    }

    TextureInfo& VulkanDevice::registerTexture(std::string path, VkFormat format, bool isColor, bool isCube)
    {
        for (auto& [id, tex] : textures)
        {
            if (tex.path == path)
            {
                return tex;
            }
        }

        TextureInfo texture{};
        texture.id = (TextureId)textures.size();
        texture.path = path;
        texture.format = format;
        texture.image = {};
        texture.isColor = isColor; 
        texture.isCube = isCube; 

        textures[texture.id] = texture;

        return textures[texture.id];
    }

    TextureInfo& VulkanDevice::getTexture(int32_t textureId, bool ensureWithView, bool ensureWithSampler)
    {
        if (textures.count(textureId) > 0)
        {
            bool update = false;
            auto& tex = textures[textureId];

            if (tex.image.allocation == VK_NULL_HANDLE)
            {
                DEBUG("Loading texture %d from '%s'\n", textureId, tex.path.c_str())
                START_TIMER

                auto extension = std::filesystem::path(tex.path).extension();               
                //std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });
                
                if (extension == ".ktx")
                {
                    auto data = io::ReadFile(tex.path); 
                    tex.image = loadKtx(tex.path, data, tex.format, tex.isColor);
                }
                else
                {
                    // jpeg / png
                    int texWidth, texHeight, texChannels;

                    stbi_uc* pixels = stbi_load(tex.path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
                    if (!pixels)
                    {
                        throw std::runtime_error("failed to load texture image!");
                    }

                    uint32_t dataSize = texWidth * texHeight * 4;
                    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

                    tex.image = createImage(
                        texWidth, texHeight, mipLevels,
                        VK_SAMPLE_COUNT_1_BIT,
                        tex.format,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        pixels,
                        dataSize,
                        tex.path.c_str());

                    stbi_image_free(pixels);
                    update = true;                   
                }
                tex.image.isCube = tex.isCube;

                END_TIMER("Loading texture '%d' : '%s' took: ", textureId, tex.path.c_str())
            }

            if (tex.image.allocation != VK_NULL_HANDLE)
            {
                if (ensureWithView && tex.image.view == VK_NULL_HANDLE)
                {
                    createImageView(tex.image);
                    update = true;
                }
                if (ensureWithSampler && tex.image.sampler == VK_NULL_HANDLE)
                {
                    createImageSampler(tex.image);
                    update = true;
                }
                if (update)
                {
                    textures[textureId] = tex;
                    tex.image.updateDescriptor();
                }
            }
            return tex;
        }
        else
        {
            throw std::runtime_error("failed to load texture");
        }
    }

    TextureInfo& VulkanDevice::getTexture(std::string path, VkFormat format, bool ensureWithView, bool ensureWithSampler, bool isColor)
    {
        return getTexture(initTexture(path, format, ensureWithView, ensureWithSampler, isColor).id, ensureWithView, ensureWithSampler);
    }

    void VulkanDevice::freeTexture(int32_t textureId)
    {
        if (textures.count(textureId) > 0)
        {
            auto& tex = textures[textureId];

            if (tex.image.view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(device, tex.image.view, nullptr);
            }
            if (tex.image.sampler != VK_NULL_HANDLE)
            {
                vkDestroySampler(device, tex.image.sampler, nullptr);
            }
            if (tex.image.allocation != VK_NULL_HANDLE)
            {
                vmaDestroyImage(allocator, tex.image.img, tex.image.allocation);
            }
            tex.image = {};
        }
    }
    
    Material VulkanDevice::initMaterial(std::string name, int albedoId, int normalId, int metallicId, int roughnessId, int vertexShaderId, int fragmentShaderId)
    {
        for (auto& [id, m] : materials)
        {
            if (m.name == name)
            {
                return m;
            }
        }

        Material material{};
        material.name = name;
        material.albedoTextureId = albedoId;
        material.normalTextureId = normalId;
        material.metallicTextureId = metallicId;
        material.roughnessTextureId = roughnessId; 
        material.vertexShaderId = vertexShaderId;
        material.fragmentShaderId = fragmentShaderId;

        material.materialId = (MaterialId)materials.size();
        materials[material.materialId] = material;

        return material;
    }

    // returns a builder for a material that needs to be disposed with delete if not built to completion
    MaterialBuilder* VulkanDevice::initMaterial(std::string name)
    {
        for (auto& [id, m] : materials)
        {
            if (m.name == name)
            {
                std::runtime_error("a material with this name already exists and cannot be built");
            }
        }
        return (new MaterialBuilder(this))->create(name); 
    }

    Material VulkanDevice::getMaterial(MaterialId material)
    {
        if (materials.contains(material))
        {
            return materials[material]; 
        }

        std::runtime_error("a material with this id/name does not exist");        
    }

    ShaderInfo VulkanDevice::initShader(const char* path, VkShaderStageFlagBits stage)
    {
        START_TIMER

            for (auto& [id, info] : shaders)
            {
                if (info.path == path)
                {
                    return info;
                }
            }

        auto code = io::ReadFile(path);
        VkShaderModule shaderModule = assets::createShaderModule(device, code);

        ShaderInfo shader{};
        shader.id = shaders.size();
        shader.path = path;
        shader.module = shaderModule;
        shader.flags = stage;
        shaders[shader.id] = shader;

        END_TIMER("initialized shader: '%s' in ", path);
        return shader;
    }
    
    ShaderInfo VulkanDevice::initVertexShader(const char* path)
    {
        return initShader(path, VK_SHADER_STAGE_VERTEX_BIT);
    }

    ShaderInfo VulkanDevice::initFragmentShader(const char* path)
    {
        return initShader(path, VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    void VulkanDevice::registerModel(ModelInfo& model)
    {
        model.modelId = (ModelId)models.size();
        models[model.modelId] = model;
    }

    MeshId VulkanDevice::registerMesh(MeshInfo& mesh)
    {
        mesh.meshId = (MeshId)meshes.size();

        ensureMeshHashLOD0(mesh); 

        meshes[mesh.meshId] = mesh;
        return mesh.meshId; 
    }

    MaterialId VulkanDevice::registerMaterial(Material& material)
    {
        material.materialId = (MaterialId)materials.size();
        materials[material.materialId] = material;
        return material.materialId;
    }

    MeshInfo& VulkanDevice::getMesh(MeshId id) 
    {
        return meshes[id];
    }

    void VulkanDevice::setMesh(MeshInfo mesh)
    {
        ensureMeshHashLOD0(mesh); 
        meshes[mesh.meshId] = mesh; 
    }

    void VulkanDevice::ensureMeshHashLOD0(MeshInfo& mesh)
    {
        // setup a default lod for meshes without any 
        if (mesh.lods.size() == 0)
        {
            MeshLODLevelInfo lod = {
                .meshId = mesh.meshId,
                .lodLevel = 0,
                .indexOffset = 0,
                .indexCount = (uint32_t)mesh.indices.size()
            };
            mesh.lods.push_back(lod);
        }
    }

    ModelInfo VulkanDevice::loadObj(const char* path, Material material, float scale, bool computeNormals, bool removeDuplicateVertices, bool absoluteScaling, bool computeTangents)
    {
        ModelData model = assets::loadObj(path, material, scale, computeNormals, removeDuplicateVertices, absoluteScaling, computeTangents);

        ModelInfo info{};
        info.aabb = model.calculateAABB();

        for (auto& mesh : model.meshes)
        {
            registerMesh(mesh);

            info.meshes.push_back(mesh.meshId);

            if (mesh.materialId >= 0)
            {
                bool isNew = true;
                for (auto& materialId : info.materialIds)
                {
                    if (materialId == mesh.materialId)
                    {
                        isNew = false;
                        break;
                    }
                }
                if (isNew)
                {
                    info.materialIds.push_back(mesh.materialId);
                }
            }
        }

        registerModel(info);
        return info;
    }

    ImageRaw VulkanDevice::loadRawImageData(const char* path)
    {
        DEBUG("Loading image data from '%s'\n", path)
        START_TIMER

        ImageRaw raw{};
        auto extension = std::filesystem::path(path).extension();

        if (extension == ".ktx")
        {
            raw = loadKtxRaw(path);
        }
        else
        {
            // jpeg / png
            int width, height, depth;

            stbi_uc* pixels = stbi_load(path, &width, &height, &depth, STBI_rgb_alpha);
            if (!pixels)
            {
                throw std::runtime_error("failed to load image!");
            }

            raw.name = path;  
            raw.dataSize = width * height * depth;
            raw.data = (BYTE*)malloc(raw.dataSize); 
            raw.width = width;
            raw.height = height;
            raw.depth = depth; 
            
            memcpy(raw.data, pixels, raw.dataSize); 

            stbi_image_free(pixels);
        }

        END_TIMER("Loading image '%s' took: ", path)
        return raw; 
    }

    void VulkanDevice::resetFrameStats()
    {
        frameStats.triangleCount = 0;
        frameStats.instanceCount = 0;    
        frameStats.drawCount = 0; 
        for (int i = 0; i < LOD_LEVELS; i++) frameStats.lodCounts[i] = 0;
    }
 
    void VulkanDevice::updateFrameStats(UINT instanceCount, UINT triangleCount, UINT lodLevel)
    {
        frameStats.instanceCount += instanceCount;
        frameStats.triangleCount += triangleCount;
        frameStats.lodCounts[lodLevel] += instanceCount;
    }
    
    void VulkanDevice::updateFrameStatsDrawCount(UINT drawCount)
    {
        frameStats.drawCount += drawCount;
    }

    // PBR: Generate a BRDF integration map used as a look-up-table (stores roughness / NdotV)
    Image VulkanDevice::generateBRDFLUT()
    {
        START_TIMER

        const VkFormat format = VK_FORMAT_R16G16_SFLOAT;	// R16G16 is supported pretty much everywhere
        const int32_t dim = 512;

        Image lutBrdf = createImage(dim, dim, 1, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        createImageView(lutBrdf); 

        VkSamplerCreateInfo samplerCI = init::samplerCreateInfo();
        samplerCI.magFilter = VK_FILTER_LINEAR;
        samplerCI.minFilter = VK_FILTER_LINEAR;
        samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.minLod = 0.0f;
        samplerCI.maxLod = 1.0f;
        samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VK_CHECK(vkCreateSampler(device, &samplerCI, nullptr, &lutBrdf.sampler));
        createImageSampler(lutBrdf); 
        
        lutBrdf.updateDescriptor();

        // FB, Att, RP, Pipe, etc.
        VkAttachmentDescription attDesc = {};
        
        // Color attachment
        attDesc.format = format;
        attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // Create the actual renderpass
        VkRenderPassCreateInfo renderPassCI = init::renderPassCreateInfo();
        renderPassCI.attachmentCount = 1;
        renderPassCI.pAttachments = &attDesc;
        renderPassCI.subpassCount = 1;
        renderPassCI.pSubpasses = &subpassDescription;
        renderPassCI.dependencyCount = 2;
        renderPassCI.pDependencies = dependencies.data();

        VkRenderPass renderpass;
        VK_CHECK(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderpass));

        VkFramebufferCreateInfo framebufferCI = init::framebufferCreateInfo();
        framebufferCI.renderPass = renderpass;  // re use renderpass 
        framebufferCI.attachmentCount = 1;
        framebufferCI.pAttachments = &lutBrdf.view;
        framebufferCI.width = dim;
        framebufferCI.height = dim;
        framebufferCI.layers = 1;

        VkFramebuffer framebuffer;
        VK_CHECK(vkCreateFramebuffer(device, &framebufferCI, nullptr, &framebuffer));

        // Descriptors
        VkDescriptorSetLayout descriptorsetlayout;
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};
        VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI = init::descriptorSetLayoutCreateInfo(setLayoutBindings);
        VK_CHECK(vkCreateDescriptorSetLayout(device, &descriptorsetlayoutCI, nullptr, &descriptorsetlayout));

        // Descriptor Pool
        std::vector<VkDescriptorPoolSize> poolSizes = { init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1) };
        VkDescriptorPoolCreateInfo descriptorPoolCI = init::descriptorPoolCreateInfo(poolSizes, 2);
        VkDescriptorPool descriptorpool;
        VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorpool));

        // Descriptor sets
        VkDescriptorSet descriptorset;
        VkDescriptorSetAllocateInfo allocInfo = init::descriptorSetAllocateInfo(descriptorpool, &descriptorsetlayout, 1);
        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptorset));

        // Pipeline layout
        VkPipelineLayout pipelinelayout;
        VkPipelineLayoutCreateInfo pipelineLayoutCI = init::pipelineLayoutCreateInfo(&descriptorsetlayout, 1);
        VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelinelayout));

        // Pipeline
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        VkPipelineRasterizationStateCreateInfo rasterizationState = init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        VkPipelineColorBlendAttachmentState blendAttachmentState = init::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlendState = init::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
        VkPipelineDepthStencilStateCreateInfo depthStencilState = init::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo viewportState = init::pipelineViewportStateCreateInfo(1, 1);
        VkPipelineMultisampleStateCreateInfo multisampleState = init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState = init::pipelineDynamicStateCreateInfo(dynamicStateEnables);
        VkPipelineVertexInputStateCreateInfo emptyInputState = init::pipelineVertexInputStateCreateInfo();
        
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = 
        { 
            initShader("compiled shaders/genbrdflut.vert.spv", VK_SHADER_STAGE_VERTEX_BIT).createInfo(),
            initShader("compiled shaders/genbrdflut.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT).createInfo()
        };

        VkGraphicsPipelineCreateInfo pipelineCI = init::pipelineCreateInfo(pipelinelayout, renderpass);
        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pDepthStencilState = &depthStencilState;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.stageCount = 2;
        pipelineCI.pStages = shaderStages.data();
        pipelineCI.pVertexInputState = &emptyInputState;

        // Look-up-table (from BRDF) pipeline
        VkPipeline pipeline;
        VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));

        // Render
        VkClearValue clearValues[1];
        clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

        VkRenderPassBeginInfo renderPassBeginInfo = init::renderPassBeginInfo();
        renderPassBeginInfo.renderPass = renderpass;
        renderPassBeginInfo.renderArea.extent.width = dim;
        renderPassBeginInfo.renderArea.extent.height = dim;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = clearValues;
        renderPassBeginInfo.framebuffer = framebuffer;

        VkCommandBuffer cmdBuf = beginCommandBuffer();

        vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport = init::viewport((float)dim, (float)dim, 0.0f, 1.0f);
        VkRect2D scissor = init::rect2D(dim, dim, 0, 0);
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdDraw(cmdBuf, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmdBuf);
  
        endCommandBuffer(cmdBuf); 

        vkQueueWaitIdle(graphicsQueue);

        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelinelayout, nullptr);
        vkDestroyRenderPass(device, renderpass, nullptr);
        vkDestroyFramebuffer(device, framebuffer, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorsetlayout, nullptr);
        vkDestroyDescriptorPool(device, descriptorpool, nullptr);

        END_TIMER("Generating BRDF LUT took ")

        return lutBrdf;
    }

    // PBR: Generate an irradiance cube map from the environment cube map
    Image VulkanDevice::generateIrradianceCube(ModelData* skyboxModel, Image environmentCube)
    {
        START_TIMER

        const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
        const int32_t dim = 64;
        const uint32_t numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;

        // Pre-filtered cube map
        Image irradianceCube = createCubemap(dim, dim, numMips, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
       
        VkImageViewCreateInfo viewCI = init::imageViewCreateInfo();
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewCI.format = format;
        viewCI.subresourceRange = {};
        viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCI.subresourceRange.levelCount = numMips;
        viewCI.subresourceRange.layerCount = 6;
        viewCI.image = irradianceCube.img;
        VK_CHECK(vkCreateImageView(device, &viewCI, nullptr, &irradianceCube.view));

        // Sampler
        VkSamplerCreateInfo samplerCI = init::samplerCreateInfo();
        samplerCI.magFilter = VK_FILTER_LINEAR;
        samplerCI.minFilter = VK_FILTER_LINEAR;
        samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.minLod = 0.0f;
        samplerCI.maxLod = static_cast<float>(numMips);
        samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VK_CHECK(vkCreateSampler(device, &samplerCI, nullptr, &irradianceCube.sampler));
        
        irradianceCube.updateDescriptor(); 

        // FB, Att, RP, Pipe, etc.
        VkAttachmentDescription attDesc = {};
        // Color attachment
        attDesc.format = format;
        attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // Renderpass
        VkRenderPassCreateInfo renderPassCI = init::renderPassCreateInfo();
        renderPassCI.attachmentCount = 1;
        renderPassCI.pAttachments = &attDesc;
        renderPassCI.subpassCount = 1;
        renderPassCI.pSubpasses = &subpassDescription;
        renderPassCI.dependencyCount = 2;
        renderPassCI.pDependencies = dependencies.data();
        VkRenderPass renderpass;
        VK_CHECK(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderpass));

        VkFramebuffer offscreenFramebuffer;

        // Offscreen framebuffer
        Image offscreen = createImage(
            dim, dim, 1,
            VK_SAMPLE_COUNT_1_BIT, 
            format,
            VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        createImageView(offscreen);
        offscreen.updateDescriptor(); 

        VkFramebufferCreateInfo fbufCreateInfo = init::framebufferCreateInfo();
        fbufCreateInfo.renderPass = renderpass;
        fbufCreateInfo.attachmentCount = 1;
        fbufCreateInfo.pAttachments = &offscreen.view;
        fbufCreateInfo.width = dim;
        fbufCreateInfo.height = dim;
        fbufCreateInfo.layers = 1;
        VK_CHECK(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offscreenFramebuffer));

        {
            BEGIN_COMMAND_BUFFER
                setImageLayout(commandBuffer, offscreen, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            END_COMMAND_BUFFER
        }

        // Descriptors
        VkDescriptorSetLayout descriptorsetlayout;
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = 
        {
            init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };

        VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI = init::descriptorSetLayoutCreateInfo(setLayoutBindings);
        VK_CHECK(vkCreateDescriptorSetLayout(device, &descriptorsetlayoutCI, nullptr, &descriptorsetlayout));

        // Descriptor Pool
        std::vector<VkDescriptorPoolSize> poolSizes = { init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1) };
        VkDescriptorPoolCreateInfo descriptorPoolCI = init::descriptorPoolCreateInfo(poolSizes, 2);
        VkDescriptorPool descriptorpool;
        VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorpool));

        // Descriptor sets
        VkDescriptorSet descriptorset;
        VkDescriptorSetAllocateInfo allocInfo = init::descriptorSetAllocateInfo(descriptorpool, &descriptorsetlayout, 1);
        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptorset));
        VkWriteDescriptorSet writeDescriptorSet = init::writeDescriptorSet(descriptorset, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &environmentCube.descriptor);
        vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);

        // Pipeline layout
        struct PushBlock
        {
            glm::mat4 mvp;
            // Sampling deltas
            float deltaPhi = (2.0f * glm::pi<float>()) / 180.0f;
            float deltaTheta = (0.5f * glm::pi<float>()) / 64.0f;
        } pushBlock;

        VkPipelineLayout pipelinelayout;
        std::vector<VkPushConstantRange> pushConstantRanges = {
            init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlock), 0),
        };
        VkPipelineLayoutCreateInfo pipelineLayoutCI = init::pipelineLayoutCreateInfo(&descriptorsetlayout, 1);
        pipelineLayoutCI.pushConstantRangeCount = 1;
        pipelineLayoutCI.pPushConstantRanges = pushConstantRanges.data();
        VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelinelayout));

        // Pipeline
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        VkPipelineRasterizationStateCreateInfo rasterizationState = init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        VkPipelineColorBlendAttachmentState blendAttachmentState = init::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlendState = init::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
        VkPipelineDepthStencilStateCreateInfo depthStencilState = init::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo viewportState = init::pipelineViewportStateCreateInfo(1, 1);
        VkPipelineMultisampleStateCreateInfo multisampleState = init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState = init::pipelineDynamicStateCreateInfo(dynamicStateEnables);
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages =
        {
            initShader("compiled shaders/filtercube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT).createInfo(),
            initShader("compiled shaders/irradiancecube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT).createInfo()
        };

        VkPipelineVertexInputStateCreateInfo vertexCI{};
        auto desc = Vertex::getBindingDescription();
        auto attr = Vertex::getAttributeDescriptions(); 
                
        vertexCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexCI.vertexBindingDescriptionCount = 1;
        vertexCI.pVertexBindingDescriptions = &desc;
        vertexCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr.size());
        vertexCI.pVertexAttributeDescriptions = attr.data();            

        VkGraphicsPipelineCreateInfo pipelineCI = init::pipelineCreateInfo(pipelinelayout, renderpass);
        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pDepthStencilState = &depthStencilState;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.stageCount = 2;
        pipelineCI.pStages = shaderStages.data();
        pipelineCI.renderPass = renderpass; // reuses!!!
        pipelineCI.pVertexInputState = &vertexCI;

        VkPipeline pipeline;
        VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));

        // Render
        VkClearValue clearValues[1];
        clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };

        VkRenderPassBeginInfo renderPassBeginInfo = init::renderPassBeginInfo();
        
        // Reuse render pass from example pass
        renderPassBeginInfo.renderPass = renderpass;
        renderPassBeginInfo.framebuffer = offscreenFramebuffer;
        renderPassBeginInfo.renderArea.extent.width = dim;
        renderPassBeginInfo.renderArea.extent.height = dim;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = clearValues;

        std::vector<glm::mat4> matrices =
        {
            // POSITIVE_X
            glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_X
            glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // POSITIVE_Y
            glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_Y
            glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // POSITIVE_Z
            glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_Z
            glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        };

        BEGIN_COMMAND_BUFFER

        VkViewport viewport = init::viewport((float)dim, (float)dim, 0.0f, 1.0f);
        VkRect2D scissor = init::rect2D(dim, dim, 0, 0);

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = numMips;
        subresourceRange.layerCount = 6;

        // Change image layout for all cubemap faces to transfer destination
        setImageLayout(
            commandBuffer,
            irradianceCube,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            subresourceRange);

        //setImageLayout(
        //    commandBuffer,
        //    environmentCube,
        //    VK_IMAGE_LAYOUT_UNDEFINED,
        //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        //    subresourceRange);                    

        // create index and vertex buffer for skybox/cube 
        Buffer vertexBuffer, indexBuffer; 
        
        createBuffer(
            sizeof(skyboxModel->meshes[0].vertices[0]) * skyboxModel->meshes[0].vertices.size(),
            skyboxModel->meshes[0].vertices.data(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
            vertexBuffer, 
            "generateIrradianceCube-skybox-vertexBuffer");

        createBuffer(
            sizeof(skyboxModel->meshes[0].indices[0]) * skyboxModel->meshes[0].indices.size(),
            skyboxModel->meshes[0].indices.data(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
            indexBuffer, 
            "generateIrradianceCube-skybox-indexBuffer");

        for (uint32_t m = 0; m < numMips; m++) 
        {
            for (uint32_t f = 0; f < 6; f++) 
            {
                viewport.width = static_cast<float>(dim * std::pow(0.5f, m));
                viewport.height = static_cast<float>(dim * std::pow(0.5f, m));
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                // Render scene from cube face's point of view
                vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                // Update shader push constant block
                pushBlock.mvp = glm::perspective((float)(glm::pi<float>() / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];

                vkCmdPushConstants(commandBuffer, pipelinelayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlock), &pushBlock);

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinelayout, 0, 1, &descriptorset, 0, NULL);

                // draw skybox
                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &offset);
                vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, indexBuffer.info.size / 4, 1, 0, 0, 0);

                vkCmdEndRenderPass(commandBuffer);

                setImageLayout(
                    commandBuffer, 
                    offscreen, 
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

                // Copy region for transfer from framebuffer to cube face
                VkImageCopy copyRegion = {};

                copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.srcSubresource.baseArrayLayer = 0;
                copyRegion.srcSubresource.mipLevel = 0;
                copyRegion.srcSubresource.layerCount = 1;
                copyRegion.srcOffset = { 0, 0, 0 };

                copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.dstSubresource.baseArrayLayer = f;
                copyRegion.dstSubresource.mipLevel = m;
                copyRegion.dstSubresource.layerCount = 1;
                copyRegion.dstOffset = { 0, 0, 0 };

                copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
                copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
                copyRegion.extent.depth = 1;

                vkCmdCopyImage(
                    commandBuffer,
                    offscreen.img,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    irradianceCube.img,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &copyRegion);

                // Transform framebuffer color attachment back
                setImageLayout(
                    commandBuffer,
                    offscreen,
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            }
        }

        setImageLayout(
            commandBuffer,
            irradianceCube,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            subresourceRange);
                
        END_COMMAND_BUFFER

        vkDestroyRenderPass(device, renderpass, nullptr);
        vkDestroyFramebuffer(device, offscreenFramebuffer, nullptr);

        DESTROY_IMAGE(allocator, offscreen)

        vkDestroyDescriptorPool(device, descriptorpool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorsetlayout, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelinelayout, nullptr);

        END_TIMER("Generating irradiance cube with %d mip levels took ", numMips)

        return irradianceCube;
    }

    // PBR: Prefilter environment cubemap
    // See https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/
    Image VulkanDevice::generatePrefilteredCube(ModelData* skyboxModel, Image environmentCube)
    {
        START_TIMER

        const VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
        const int32_t dim = 512;
        const uint32_t numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;

        // create target image 
        Image prefilteredCube = createCubemap(dim, dim, numMips, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        prefilteredCube.view = createImageView(prefilteredCube.img, prefilteredCube.format, VK_IMAGE_ASPECT_COLOR_BIT, prefilteredCube.mipmapLevels, prefilteredCube.layerCount);
        createImageSampler(prefilteredCube); 
        prefilteredCube.updateDescriptor(); 

        // FB, Att, RP, Pipe, etc.
        VkAttachmentDescription attDesc = {};
        attDesc.format = format;
        attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // Renderpass
        VkRenderPassCreateInfo renderPassCI = init::renderPassCreateInfo();
        renderPassCI.attachmentCount = 1;
        renderPassCI.pAttachments = &attDesc;
        renderPassCI.subpassCount = 1;
        renderPassCI.pSubpasses = &subpassDescription;
        renderPassCI.dependencyCount = 2;
        renderPassCI.pDependencies = dependencies.data();
        VkRenderPass renderpass;
        VK_CHECK(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderpass));

        // framebuffer 
        Image offscreen = createImage(dim, dim, 1, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkFramebuffer framebuffer;
        
        createImageView(offscreen); 
        offscreen.updateDescriptor(); 
        
        VkFramebufferCreateInfo fbufCreateInfo = init::framebufferCreateInfo();
            fbufCreateInfo.renderPass = renderpass;
            fbufCreateInfo.attachmentCount = 1;
            fbufCreateInfo.pAttachments = &offscreen.view;
            fbufCreateInfo.width = dim;
            fbufCreateInfo.height = dim;
            fbufCreateInfo.layers = 1;
            VK_CHECK(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &framebuffer));

            transitionImageLayout(
                offscreen,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            // Descriptors
        VkDescriptorSetLayout descriptorsetlayout;
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI = init::descriptorSetLayoutCreateInfo(setLayoutBindings);
        VK_CHECK(vkCreateDescriptorSetLayout(device, &descriptorsetlayoutCI, nullptr, &descriptorsetlayout));

        // Descriptor Pool
        std::vector<VkDescriptorPoolSize> poolSizes = { init::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1) };
        VkDescriptorPoolCreateInfo descriptorPoolCI = init::descriptorPoolCreateInfo(poolSizes, 2);
        VkDescriptorPool descriptorpool;
        VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorpool));

        // Descriptor sets
        VkDescriptorSet descriptorset;
        VkDescriptorSetAllocateInfo allocInfo = init::descriptorSetAllocateInfo(descriptorpool, &descriptorsetlayout, 1);
        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptorset));
        VkWriteDescriptorSet writeDescriptorSet = init::writeDescriptorSet(descriptorset, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &environmentCube.descriptor);
        vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);

        // Pipeline layout
        struct PushBlock {
            glm::mat4 mvp;
            float roughness;
            uint32_t numSamples = 32u;
        } pushBlock;

        VkPipelineLayout pipelinelayout;
        std::vector<VkPushConstantRange> pushConstantRanges = {
            init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlock), 0),
        };
        VkPipelineLayoutCreateInfo pipelineLayoutCI = init::pipelineLayoutCreateInfo(&descriptorsetlayout, 1);
        pipelineLayoutCI.pushConstantRangeCount = 1;
        pipelineLayoutCI.pPushConstantRanges = pushConstantRanges.data();
        VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelinelayout));

        // Pipeline
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        VkPipelineRasterizationStateCreateInfo rasterizationState = init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        VkPipelineColorBlendAttachmentState blendAttachmentState = init::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlendState = init::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
        VkPipelineDepthStencilStateCreateInfo depthStencilState = init::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo viewportState = init::pipelineViewportStateCreateInfo(1, 1);
        VkPipelineMultisampleStateCreateInfo multisampleState = init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState = init::pipelineDynamicStateCreateInfo(dynamicStateEnables);
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages =
        {
            initShader("compiled shaders/filtercube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT).createInfo(),
            initShader("compiled shaders/prefilterenvmap.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT).createInfo()
        };

        VkPipelineVertexInputStateCreateInfo vertexCI{};
        auto desc = Vertex::getBindingDescription();
        auto attr = Vertex::getAttributeDescriptions();

        vertexCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexCI.vertexBindingDescriptionCount = 1;
        vertexCI.pVertexBindingDescriptions = &desc;
        vertexCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr.size());
        vertexCI.pVertexAttributeDescriptions = attr.data();

        VkGraphicsPipelineCreateInfo pipelineCI = init::pipelineCreateInfo(pipelinelayout, renderpass);
        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pDepthStencilState = &depthStencilState;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.stageCount = 2;
        pipelineCI.pStages = shaderStages.data();
        pipelineCI.renderPass = renderpass;
        pipelineCI.pVertexInputState = &vertexCI;

        VkPipeline pipeline;
        VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));

        // Render

        VkClearValue clearValues[1];
        clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };

        VkRenderPassBeginInfo renderPassBeginInfo = init::renderPassBeginInfo();
        // Reuse render pass from example pass
        renderPassBeginInfo.renderPass = renderpass;
        renderPassBeginInfo.framebuffer = framebuffer;
        renderPassBeginInfo.renderArea.extent.width = dim;
        renderPassBeginInfo.renderArea.extent.height = dim;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = clearValues;

        std::vector<glm::mat4> matrices = 
        {
            // POSITIVE_X
            glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_X
            glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // POSITIVE_Y
            glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_Y
            glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // POSITIVE_Z
            glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_Z
            glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        };

        BEGIN_COMMAND_BUFFER

        VkViewport viewport = init::viewport((float)dim, (float)dim, 0.0f, 1.0f);
        VkRect2D scissor = init::rect2D(dim, dim, 0, 0);

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = numMips;
        subresourceRange.layerCount = 6;

        // Change image layout for all cubemap faces to transfer destination
        transitionImageLayout(
            commandBuffer,
            prefilteredCube,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            subresourceRange);


        // create index and vertex buffer for skybox/cube 
        Buffer vertexBuffer, indexBuffer;

        createBuffer(
            sizeof(skyboxModel->meshes[0].vertices[0])* skyboxModel->meshes[0].vertices.size(),
            skyboxModel->meshes[0].vertices.data(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
            vertexBuffer,
            "prefilter-skybox-vertexBuffer");

        createBuffer(
            sizeof(skyboxModel->meshes[0].indices[0])* skyboxModel->meshes[0].indices.size(),
            skyboxModel->meshes[0].indices.data(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
            indexBuffer,
            "prefilter-skybox-indexBuffer");

        for (uint32_t m = 0; m < numMips; m++) 
        {
            pushBlock.roughness = (float)m / (float)(numMips - 1);
            for (uint32_t f = 0; f < 6; f++) 
            {
                viewport.width = static_cast<float>(dim * std::pow(0.5f, m));
                viewport.height = static_cast<float>(dim * std::pow(0.5f, m));
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                // Render scene from cube face's point of view
                vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                // Update shader push constant block
                pushBlock.mvp = glm::perspective((float)(glm::pi<float>() / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];

                vkCmdPushConstants(commandBuffer, pipelinelayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlock), &pushBlock);

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinelayout, 0, 1, &descriptorset, 0, NULL);

                // draw skybox
                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &offset);
                vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, indexBuffer.info.size / 4, 1, 0, 0, 0);

                vkCmdEndRenderPass(commandBuffer);

                transitionImageLayout(
                    commandBuffer,
                    offscreen,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

                // Copy region for transfer from framebuffer to cube face
                VkImageCopy copyRegion = {};

                copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.srcSubresource.baseArrayLayer = 0;
                copyRegion.srcSubresource.mipLevel = 0;
                copyRegion.srcSubresource.layerCount = 1;
                copyRegion.srcOffset = { 0, 0, 0 };

                copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.dstSubresource.baseArrayLayer = f;
                copyRegion.dstSubresource.mipLevel = m;
                copyRegion.dstSubresource.layerCount = 1;
                copyRegion.dstOffset = { 0, 0, 0 };

                copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
                copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
                copyRegion.extent.depth = 1;

                vkCmdCopyImage(
                    commandBuffer,
                    offscreen.img,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    prefilteredCube.img,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &copyRegion);

                // Transform framebuffer color attachment back
                transitionImageLayout(
                    commandBuffer,
                    offscreen,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            }
        }

        transitionImageLayout(
            commandBuffer,
            prefilteredCube,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            subresourceRange);

        END_COMMAND_BUFFER

        vkDestroyRenderPass(device, renderpass, nullptr);
        vkDestroyFramebuffer(device, framebuffer, nullptr);
        
        DESTROY_IMAGE(allocator, offscreen)

        vkDestroyDescriptorPool(device, descriptorpool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorsetlayout, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelinelayout, nullptr);

        END_TIMER("Generating pre-filtered enivornment cube with %d mip levels took: ", numMips)

        return prefilteredCube;
    }

    void VulkanDevice::initPBR(ModelData* skybox, Image& environmentCube)
    {
        cleanupPBR(); 

        brdfLUT = generateBRDFLUT();

        if (environmentCube.layout != VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL)
        {
           // BEGIN_COMMAND_BUFFER
           // 
           // setImageLayout(
           //     commandBuffer,
           //     environmentCube.img,
           //     VK_IMAGE_ASPECT_COLOR_BIT,
           //     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
           //     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
           // 
           // environmentCube.layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL; 
           // environmentCube.updateDescriptor(); 
           // 
           // END_COMMAND_BUFFER
        }

        irradianceCube = generateIrradianceCube(skybox, environmentCube);
        prefilteredCube = generatePrefilteredCube(skybox, environmentCube);
    }

    void VulkanDevice::cleanupPBR()
    {
        DESTROY_IMAGE(allocator, brdfLUT)
        DESTROY_IMAGE(allocator, irradianceCube)
        DESTROY_IMAGE(allocator, prefilteredCube)
    }
}