#pragma once

namespace vkengine 
{
    struct BoundingBox2D
    {
        VEC2 Min;
        VEC2 Max;
    };

    struct BBox
    {
        VEC4 min;
        VEC4 max;

        VEC4 Center()
        { 
            return (min + max) * 0.5f; 
        }

        // ensure min/max is in correct order 
        void align()
        {
            VEC3 minp;
            VEC3 maxp;

            if (min.x > max.x && min.y > max.y && min.z > max.z)
            {
                minp = max;
                maxp = min;
            }
            else
            {
                minp = min;
                maxp = max;
            }

            min = VEC4(minp, 1); 
            max = VEC4(maxp, 1); 
        }
    };

    struct Ray2D
    {
        VEC2 BasePoint;
        VEC2 Heading;
    };

    struct Ray
    {
        VEC4 BasePoint;
        VEC4 Heading;
    };

    struct Plane2D
    {
        float_t OrigoDistance;
        VEC2 Normal;
    };

    struct Plane
    {
        VEC3 Normal; 
        float_t OrigoDistance;
    };
} 

namespace OrthoTree
{
    namespace vkengineAdaptor2D
    {
        using xy_geometry_type = float;
        using XYPoint2D = VEC2;
        using XYBoundingBox2D = vkengine::BoundingBox2D;
        using XYRay2D = vkengine::Ray2D;
        using XYPlane2D = vkengine::Plane2D;

        struct XYAdaptorBasics
        {
            static constexpr xy_geometry_type GetPointC(XYPoint2D const& pt, dim_t dimensionID)
            {
                switch (dimensionID)
                {
                case 0: return pt.x;
                case 1: return pt.y;
                default: assert(false); std::terminate();
                }
            }


            static constexpr void SetPointC(XYPoint2D& pt, dim_t dimensionID, xy_geometry_type value)
            {
                switch (dimensionID)
                {
                case 0: pt.x = value; break;
                case 1: pt.y = value; break;
                default: assert(false); std::terminate();
                }
            }


            static constexpr void SetBoxMinC(XYBoundingBox2D& box, dim_t dimensionID, xy_geometry_type value) { SetPointC(box.Min, dimensionID, value); }
            static constexpr void SetBoxMaxC(XYBoundingBox2D& box, dim_t dimensionID, xy_geometry_type value) { SetPointC(box.Max, dimensionID, value); }
            static constexpr xy_geometry_type GetBoxMinC(XYBoundingBox2D const& box, dim_t dimensionID) { return GetPointC(box.Min, dimensionID); }
            static constexpr xy_geometry_type GetBoxMaxC(XYBoundingBox2D const& box, dim_t dimensionID) { return GetPointC(box.Max, dimensionID); }

            static constexpr XYPoint2D const& GetRayDirection(XYRay2D const& ray) noexcept { return ray.Heading; }
            static constexpr XYPoint2D const& GetRayOrigin(XYRay2D const& ray) noexcept { return ray.BasePoint; }

            static constexpr XYPoint2D const& GetPlaneNormal(XYPlane2D const& plane) noexcept { return plane.Normal; }
            static constexpr xy_geometry_type GetPlaneOrigoDistance(XYPlane2D const& plane) noexcept { return plane.OrigoDistance; }
        };

        using XYAdaptorGeneral = AdaptorGeneralBase<2, XYPoint2D, XYBoundingBox2D, XYRay2D, XYPlane2D, xy_geometry_type, XYAdaptorBasics>;
    } // namespace XYAdaptor2D


    namespace vkengineAdaptor3D
    {
        using xyz_geometry_type = float;
        using XYZPoint3D = VEC4;
        using XYZBoundingBox3D = vkengine::BBox;
        using XYZRay3D = vkengine::Ray;
        using XYZPlane3D = vkengine::Plane;

        struct XYZAdaptorBasics
        {
            static constexpr xyz_geometry_type GetPointC(XYZPoint3D const& pt, dim_t dimensionID) noexcept
            {
                switch (dimensionID)
                {
                case 0: return pt.x;
                case 1: return pt.y;
                case 2: return pt.z;
                default: assert(false); std::terminate();
                }
            }

            static constexpr void SetPointC(XYZPoint3D& pt, dim_t dimensionID, xyz_geometry_type value) noexcept
            {
                switch (dimensionID)
                {
                case 0: pt.x = value; break;
                case 1: pt.y = value; break;
                case 2: pt.z = value; break;
                default: assert(false); std::terminate();
                }
            }

            static constexpr void SetBoxMinC(XYZBoundingBox3D& box, dim_t dimensionID, xyz_geometry_type value) { SetPointC(box.min, dimensionID, value); }
            static constexpr void SetBoxMaxC(XYZBoundingBox3D& box, dim_t dimensionID, xyz_geometry_type value) { SetPointC(box.max, dimensionID, value); }
            static constexpr xyz_geometry_type GetBoxMinC(XYZBoundingBox3D const& box, dim_t dimensionID) { return GetPointC(box.min, dimensionID); }
            static constexpr xyz_geometry_type GetBoxMaxC(XYZBoundingBox3D const& box, dim_t dimensionID) { return GetPointC(box.max, dimensionID); }

            static constexpr XYZPoint3D const& GetRayDirection(XYZRay3D const& ray) noexcept { return ray.Heading; }
            static constexpr XYZPoint3D const& GetRayOrigin(XYZRay3D const& ray) noexcept { return ray.BasePoint; }

            static XYZPoint3D const& GetPlaneNormal(XYZPlane3D const& plane) noexcept { return VEC4(plane.Normal, 0); }
            static constexpr xyz_geometry_type GetPlaneOrigoDistance(XYZPlane3D const& plane) noexcept { return plane.OrigoDistance; }
        };

        using XYZAdaptorGeneral = AdaptorGeneralBase<3, XYZPoint3D, XYZBoundingBox3D, XYZRay3D, XYZPlane3D, xyz_geometry_type, XYZAdaptorBasics>;


    } // namespace XYZAdaptor3D
} // namespace OrthoTree

namespace vkengine
{
    using namespace OrthoTree;
    using namespace OrthoTree::vkengineAdaptor2D;
    using namespace OrthoTree::vkengineAdaptor3D;

    using QuadtreePoint =
        OrthoTreePoint<2, XYPoint2D, XYBoundingBox2D, XYRay2D, XYPlane2D, xy_geometry_type, XYAdaptorGeneral, std::span<XYPoint2D const>>;

    template<uint32_t SPLIT_DEPTH_INCREASEMENT = 2>
    using QuadtreeBox =
        OrthoTreeBoundingBox<2, XYPoint2D, XYBoundingBox2D, XYRay2D, XYPlane2D, xy_geometry_type, SPLIT_DEPTH_INCREASEMENT, XYAdaptorGeneral, std::span<XYBoundingBox2D const>>;

    using QuadtreePointC = OrthoTreeContainerPoint<QuadtreePoint>;

    template<uint32_t SPLIT_DEPTH_INCREASEMENT = 2>
    using QuadtreeBoxCs = OrthoTreeContainerBox<QuadtreeBox<SPLIT_DEPTH_INCREASEMENT>>;
    using QuadtreeBoxC = QuadtreeBoxCs<2>;

    using OctreePoint =
        OrthoTreePoint<3, XYZPoint3D, XYZBoundingBox3D, XYZRay3D, XYZPlane3D, xyz_geometry_type, XYZAdaptorGeneral, std::span<XYZPoint3D const>>;

    template<uint32_t SPLIT_DEPTH_INCREASEMENT = 2>
    using OctreeBox =
        OrthoTreeBoundingBox<3, XYZPoint3D, XYZBoundingBox3D, XYZRay3D, XYZPlane3D, xyz_geometry_type, SPLIT_DEPTH_INCREASEMENT, XYZAdaptorGeneral, std::span<XYZPoint3D const>>;


    using OcreePointC = OrthoTreeContainerPoint<OctreePoint>;

    template<uint32_t SPLIT_DEPTH_INCREASEMENT = 2>
    using OctreeBoxCs = OrthoTreeContainerBox<OctreeBox<SPLIT_DEPTH_INCREASEMENT>>;
    using OctreeBoxC = OctreeBoxCs<2>;


    // Map types
    template<typename TEntity>
    using Map = std::unordered_map<int, TEntity>;

    using QuadtreePointMap = OrthoTreePoint<2, XYPoint2D, XYBoundingBox2D, XYRay2D, XYPlane2D, xy_geometry_type, XYAdaptorGeneral, Map<XYPoint2D>>;

    template<uint32_t SPLIT_DEPTH_INCREASEMENT = 2>
    using QuadtreeBoxMap =
        OrthoTreeBoundingBox<2, XYPoint2D, XYBoundingBox2D, XYRay2D, XYPlane2D, xy_geometry_type, SPLIT_DEPTH_INCREASEMENT, XYAdaptorGeneral, Map<XYBoundingBox2D>>;

    using QuadtreePointMapC = OrthoTreeContainerPoint<QuadtreePointMap>;

    template<uint32_t SPLIT_DEPTH_INCREASEMENT = 2>
    using QuadtreeBoxMapCs = OrthoTreeContainerBox<QuadtreeBoxMap<SPLIT_DEPTH_INCREASEMENT>>;
    using QuadtreeBoxMapC = QuadtreeBoxMapCs<2>;

    using OctreePointMap = OrthoTreePoint<3, XYZPoint3D, XYZBoundingBox3D, XYZRay3D, XYZPlane3D, xyz_geometry_type, XYZAdaptorGeneral, Map<XYZPoint3D>>;

    template<uint32_t SPLIT_DEPTH_INCREASEMENT = 2>
    using OctreeBoxMap =
        OrthoTreeBoundingBox<3, XYZPoint3D, XYZBoundingBox3D, XYZRay3D, XYZPlane3D, xyz_geometry_type, SPLIT_DEPTH_INCREASEMENT, XYZAdaptorGeneral, Map<XYZBoundingBox3D>>;

    using OcreePointMapC = OrthoTreeContainerPoint<OctreePointMap>;

    template<uint32_t SPLIT_DEPTH_INCREASEMENT = 2>
    using OctreeBoxMapCs = OrthoTreeContainerBox<OctreeBoxMap<SPLIT_DEPTH_INCREASEMENT>>;
    using OctreeBoxMapC = OctreeBoxMapCs<2>;
}
