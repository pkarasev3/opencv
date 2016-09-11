#ifndef OPENCV_OPENVX_HAL_HPP_INCLUDED
#define OPENCV_OPENVX_HAL_HPP_INCLUDED

#include "opencv2/core/hal/interface.h"

#include "VX/vx.h"
#include "VX/vxu.h"

#include <string>

//==================================================================================================
// utility
// ...

#if 0
#include <cstdio>
#define PRINT(...) printf(__VA_ARGS__)
#else
#define PRINT(...)
#endif

#if __cplusplus >= 201103L
#include <chrono>
struct Tick
{
    typedef std::chrono::time_point<std::chrono::steady_clock> point_t;
    point_t start;
    point_t point;
    Tick()
    {
        start = std::chrono::steady_clock::now();
        point = std::chrono::steady_clock::now();
    }
    inline int one()
    {
        point_t old = point;
        point = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(point - old).count();
    }
    inline int total()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
    }
};
#endif

//==================================================================================================
// One more OpenVX C++ binding :-)
// ...

template <typename T>
struct VX_Traits
{
    enum {
        Type = 0
    };
};

template <>
struct VX_Traits<uchar>
{
    enum {
        Type = VX_DF_IMAGE_U8
    };
};

template <>
struct VX_Traits<ushort>
{
    enum {
        Type = VX_DF_IMAGE_U16
    };
};

template <>
struct VX_Traits<short>
{
    enum {
        Type = VX_DF_IMAGE_S16
    };
};


struct vxContext;
struct vxImage;
struct vxErr;


struct vxErr
{
    vx_status status;
    std::string msg;
    vxErr(vx_status status_, const std::string & msg_) : status(status_), msg(msg_) {}
    void check()
    {
        if (status != VX_SUCCESS)
            throw *this;
    }
    void print()
    {
        PRINT("OpenVX HAL impl error: %d (%s)\n", status, msg.c_str());
    }
    static void check(vx_context ctx)
    {
        vxErr(vxGetStatus((vx_reference)ctx), "context check").check();
    }
    static void check(vx_image img)
    {
        vxErr(vxGetStatus((vx_reference)img), "image check").check();
    }
    static void check(vx_status s)
    {
        vxErr(s, "status check").check();
    }
};


struct vxContext
{
    vx_context ctx;
    static vxContext * getContext();
private:
    vxContext()
    {
        ctx = vxCreateContext();
        vxErr::check(ctx);
    }
    ~vxContext()
    {
        vxReleaseContext(&ctx);
    }
};


struct vxImage
{
    vx_image img;

    template <typename T>
    vxImage(vxContext &ctx, const T *data, size_t step, int w, int h)
    {
        if (h == 1)
            step = w * sizeof(T);
        vx_imagepatch_addressing_t addr;
        addr.dim_x = w;
        addr.dim_y = h;
        addr.stride_x = sizeof(T);
        addr.stride_y = step;
        addr.scale_x = VX_SCALE_UNITY;
        addr.scale_y = VX_SCALE_UNITY;
        addr.step_x = 1;
        addr.step_y = 1;
        void *ptrs[] = { (void*)data };
        img = vxCreateImageFromHandle(ctx.ctx, VX_Traits<T>::Type, &addr, ptrs, VX_MEMORY_TYPE_HOST);
        vxErr::check(img);
    }
    ~vxImage()
    {
        vxSwapImageHandle(img, NULL, NULL, 1);
        vxReleaseImage(&img);
    }
};

//==================================================================================================
// real code starts here
// ...

template <typename T>
inline int ovx_hal_add(const T *a, size_t astep, const T *b, size_t bstep, T *c, size_t cstep, int w, int h)
{
    try
    {
        vxContext * ctx = vxContext::getContext();
        vxImage ia(*ctx, a, astep, w, h);
        vxImage ib(*ctx, b, bstep, w, h);
        vxImage ic(*ctx, c, cstep, w, h);
        vxErr::check(vxuAdd(ctx->ctx, ia.img, ib.img, VX_CONVERT_POLICY_SATURATE, ic.img));
    }
    catch (vxErr & e)
    {
        e.print();
        return CV_HAL_ERROR_UNKNOWN;
    }
    return CV_HAL_ERROR_OK;
}

//==================================================================================================
// functions redefinition
// ...

#undef cv_hal_add8u
#define cv_hal_add8u ovx_hal_add<uchar>
#undef cv_hal_add16s
#define cv_hal_add16s ovx_hal_add<short>

#endif
