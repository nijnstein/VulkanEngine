#pragma once

namespace vkengine
{
    struct QuantizedVertex
    {
        // == pos    = short x 3 = 6 byte
        //    col    = byte  * 3 = 3 byte 
        //    normal = byte  * 3 = 3 byte
        //    uv     = short * 2 = 4 byte  ==  16 bytes = 1 vec4
        alignas(16) UVEC4 Q;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(QUANTIZED_VERTEX);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_UINT;
            attributeDescriptions[0].offset = offsetof(QUANTIZED_VERTEX, Q);

            return attributeDescriptions;
        }
    };

    //
    // small form vertex
    // 
    // 48 bytes
    //
    // 2x smaller due to making use of packing the alignment bytes
    // 
    // as soon as light works as it should and textures are there 
    // drop/replace color for tangent 
    // 
    // should pack vertices in gpu buffer 
    // 
    struct PackedVertex
    {
        alignas(16) VEC4 posAndValue;
        alignas(16) VEC4 colorAndNormal;
        alignas(16) VEC4 uvAndNormal;

        inline QuantizedVertex quantize()
        {
            unsigned short x = (unsigned short)((posAndValue.x + 512.0f) * 64.0f);
            unsigned short y = (unsigned short)((posAndValue.y + 512.0f) * 64.0f);
            unsigned short z = (unsigned short)((posAndValue.z + 512.0f) * 64.0f);

            uint8_t r = (uint8_t)(colorAndNormal.x * 255.0f);
            uint8_t g = (uint8_t)(colorAndNormal.y * 255.0f);
            uint8_t b = (uint8_t)(colorAndNormal.z * 255.0f);

            uint8_t nx = (uint8_t)((colorAndNormal.w + 1.0f) * 127.0f);
            uint8_t ny = (uint8_t)((uvAndNormal.z + 1.0f) * 127.0f);
            uint8_t nz = (uint8_t)((uvAndNormal.w + 1.0f) * 127.0f);

            // range max 0..255 instead of 0..1  -> * 255
            // more precision in uv then normal for our blocky stuff with texture atlasses
            unsigned short u = (unsigned short)(uv().x * 255.0f);
            unsigned short v = (unsigned short)(uv().y * 255.0f);

            UINT xy = x | (y << 16); 
            UINT z_rg = z | (r << 16) | (g << 24); 

            UINT bnxnynz = b | nx << 8 | ny << 16 | nz << 24; 
            UINT uv = u | v << 16; 

            return { UVEC4(
                xy, 
                z_rg,
                bnxnynz,
                uv
            ) };
        }

        inline VEC3 pos() const {
            return VEC3(posAndValue);
        }

        inline FLOAT value() const {
            return posAndValue[3];
        }

        inline VEC3 color() const {
            return VEC3(colorAndNormal);
        }

        inline void setColor(VEC3 color) {
            colorAndNormal.x = color.x; 
            colorAndNormal.y = color.y;
            colorAndNormal.z = color.z; 
        }

        inline VEC3 tangent() const {
            return VEC3(colorAndNormal);
        }

        inline VEC3 normal() const {
            return VEC3(colorAndNormal[3], uvAndNormal[2], uvAndNormal[3]);
        }

        inline void setNormal(VEC3 normal) {
            colorAndNormal[3] = normal.x;
            uvAndNormal[2] = normal.y;
            uvAndNormal[3] = normal.z;
        }
        
        inline void addToNormal(VEC3 addNormal)
        {
            setNormal(addNormal + normal()); 
        }

        inline VEC2 uv() const {
            return VEC2(uvAndNormal[0], uvAndNormal[1]);
        }

        inline void setUv(VEC2 uv) 
        {
            uvAndNormal[0] = uv.x;
            uvAndNormal[1] = uv.y; 
        }

        inline void set(VEC3 pos, VEC3 color, VEC3 normal, VEC2 uv, FLOAT value)
        {
            posAndValue = VEC4(pos, value);
            colorAndNormal = VEC4(color, normal[0]);
            uvAndNormal = VEC4(uv[0], uv[1], normal[1], normal[2]);
        }

        bool operator==(const PackedVertex& other) const
        {
            return posAndValue == other.posAndValue
                && colorAndNormal == other.colorAndNormal
                && uvAndNormal == other.uvAndNormal;               
        }

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(PackedVertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(PackedVertex, posAndValue);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(PackedVertex, colorAndNormal);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(PackedVertex, uvAndNormal);

            return attributeDescriptions;
        }

    };

    // 96 byte / vertex 
    struct Vertex
    {
        alignas(16) VEC3 pos;
        alignas(16) VEC3 color;
        alignas(16) VEC3 normal;
        alignas(16) VEC2 uv;
        alignas(16) VEC3 tangent;
        alignas(16) VEC3 bitangent;

        bool operator==(const Vertex& other) const
        {
            return pos == other.pos 
                && color == other.color 
                && normal == other.normal 
                && uv == other.uv 
                && tangent == other.tangent 
                && bitangent == other.bitangent;
        }

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, normal);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(Vertex, uv);

            attributeDescriptions[4].binding = 0;
            attributeDescriptions[4].location = 4;
            attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[4].offset = offsetof(Vertex, tangent);

            attributeDescriptions[5].binding = 0;
            attributeDescriptions[5].location = 5;
            attributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[5].offset = offsetof(Vertex, bitangent);

            return attributeDescriptions;
        }        

        inline PackedVertex packWithColor(const float withValue = 0)
        {
            return PackedVertex(
                VEC4(pos, withValue),
                VEC4(color, normal[0]),
                VEC4(uv[0], uv[1], normal[1], normal[2]));
        }

        inline PackedVertex packWithTangent(const float withValue = 0)
        {
            return PackedVertex(
                VEC4(pos, withValue),
                VEC4(tangent, normal[0]),
                VEC4(uv[0], uv[1], normal[1], normal[2]));
        }

        inline void unpackWithColor(const PackedVertex& v)
        {
            pos = v.pos();
            color = v.color();
            normal = v.normal();
            uv = v.uv();
            tangent = VEC3(0);
            bitangent = VEC3(0);
        }

        inline void unpackWithTangent(const PackedVertex& v)
        {
            pos = v.pos();
            color = VEC3(1); 
            normal = v.normal();
            uv = v.uv();
            tangent = v.color(); 
            bitangent = VEC3(0); 
        }
    };
}

namespace std 
{
    template<> struct hash<vkengine::Vertex> 
    {
        SIZE operator()(vkengine::Vertex const& vertex) const 
        {
            return (
                (hash<VEC3>()(vertex.pos) ^
                (hash<VEC3>()(vertex.color) << 1)) >> 1) ^
                (hash<VEC3>()(vertex.normal) << 1) ^
                (hash<VEC2>()(vertex.uv) << 1) ^
                (hash<VEC3>()(vertex.tangent) + (hash<VEC3>()(vertex.bitangent)));
        }
    };

    template<> struct hash<vkengine::PackedVertex>
    {
        SIZE operator()(vkengine::PackedVertex const& vertex) const
        {
            return ((hash<VEC4>()(vertex.posAndValue) ^ (hash<VEC4>()(vertex.colorAndNormal) << 1)) >> 1) ^ (hash<VEC4>()(vertex.uvAndNormal) << 1); 
        }
    };
}

