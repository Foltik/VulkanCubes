#pragma once

#include <vulkan/vulkan.h>

template <typename Type, typename VkType>
struct VkTraits {
    constexpr VkTraits() = default;

    constexpr const Type* addr() const {
        return reinterpret_cast<const Type*>(this);
    }

    constexpr const VkType* operator&() const {
        return reinterpret_cast<const VkType*>(this);
    }

    constexpr VkType* operator&() {
        return reinterpret_cast<VkType*>(this);
    }

    constexpr explicit operator const VkType&() const {
        return *reinterpret_cast<const VkType*>(this);
    }
};
