// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
// #include <opencv2/stitching.hpp>
// #include "opencv2/stitching/warpers.hpp"
// #include "opencv2/stitching/detail/matchers.hpp"
// #include "opencv2/stitching/detail/motion_estimators.hpp"
// #include "opencv2/stitching/detail/exposure_compensate.hpp"
// #include "opencv2/stitching/detail/seam_finders.hpp"
// #include "opencv2/stitching/detail/blenders.hpp"
// #include "opencv2/stitching/detail/camera.hpp"

using namespace cv;
using namespace std;

typedef enum {
    conv,
    scale,
    video,
    image,
    cvt,
    bmcv2cv,
    warp_affine,
    bm_rectangle,
    bm_circle,
    bm_flip,
    // fill_rectangle,
    bm_bitwise_and,
    bm_bitwise_or,
    bm_bitwise_xor,
    bm_absdiff,
    mosaic,
    quantify,
    bm_rotate,
    bm_threshold,
    bm_stitch,
    bm_convertTo,
    bm_addWeighted,
    bm_transpose
} test_case;

enum operation{
    AND = 100,
    OR = 101,
    XOR = 102,
};

int g_device_id = 0;

static int get_plane_width(int format, int plane, int width)
{
    int ret;
    if (plane > 2) return 0;
    switch (format){
        case FORMAT_YUV420P:
        case FORMAT_YUV422P:
            ret = (plane == 0) ? width : width/2;
            break;
        case FORMAT_NV12:
        case FORMAT_NV21:
        case FORMAT_NV16:
        case FORMAT_NV61:
            ret = (plane != 2) ? width : 0;
            break;
        case FORMAT_YUV444P:
            ret = width;
            break;
        case FORMAT_GRAY:
            ret = (plane == 0) ? width : 0;
            break;
        default:
            fprintf(stderr, "ERROR: unsupported format %d\n", format);
            return -1;
    }

    return ret;
}

static int get_plane_height(int format, int plane, int height)
{
    int ret;
    if (plane > 2) return 0;

    switch (format){
        case FORMAT_YUV420P:
            ret = (plane == 0) ? height : height/2;
            break;
        case FORMAT_NV12:
        case FORMAT_NV21:
            ret = (plane == 0) ? height : (plane == 1) ? height/2 : 0;
            break;
        case FORMAT_YUV422P:
            ret = height;
            break;
        case FORMAT_NV16:
        case FORMAT_NV61:
            ret = (plane != 2) ? height : 0;
            break;
        case FORMAT_YUV444P:
            ret = height;
            break;
        case FORMAT_GRAY:
            ret = (plane == 0) ? height : 0;
            break;
        default:
            fprintf(stderr, "ERROR: unsupported format %d\n", format);
            return -1;
    }

    return ret;
}

static int get_plane_size(int format, int plane, int width, int height)
{
    int ret;
    if (plane > 2) return 0;

    switch (format){
        case FORMAT_YUV420P:
            ret = (plane == 0) ? width*height : width*height/4;
            break;
        case FORMAT_NV12:
        case FORMAT_NV21:
            ret = (plane == 0) ? width*height : (plane == 1) ? width*height/2 : 0;
            break;
        case FORMAT_YUV422P:
            ret = (plane == 0) ? width*height : width*height/2;
            break;
        case FORMAT_NV16:
        case FORMAT_NV61:
            ret = (plane != 2) ? width*height : 0;
            break;
        case FORMAT_YUV444P:
            ret = width*height;
            break;
        case FORMAT_GRAY:
            ret = (plane == 0) ? width*height : 0;
            break;
        default:
            fprintf(stderr, "ERROR: unsupported format %d\n", format);
            return -1;
    }

    return ret;
}

static int __attribute__((unused)) dump_data(const char *fn, Mat in, int yuv_enable)
{
  FILE *fp;

  fp = fopen(fn, "wb");
  if (fp == NULL){
    printf("open %s failed\n", fn);
    return -1;
  }

  if (yuv_enable && in.avOK()){
#ifdef USING_SOC
    for (int i = 0; i < in.avRows(); i++){
      fwrite((char*)in.avAddr(0)+i*in.avStep(0),1,in.avCols(),fp);
    }
    for (int i = 0; i < in.avRows()/2; i++){
      fwrite((char*)in.avAddr(1)+i*in.avStep(1),1,in.avCols(),fp);
    }
    /*
    for (int i = 0; i < in.avRows()/2; i++){
      fwrite((char*)in.avAddr(2)+i*in.avStep(2),1,in.avCols()/2,fp);
    }
    */
#else
    bm_handle_t handle = in.u->hid ? in.u->hid : bmcv::getCard();
    bm_device_mem_t mem;
    unsigned char *p;

    p = (unsigned char *)malloc(in.avRows() * in.avStep(0) * 3);
    if (!p){
      printf("malloc failed\n");
      fclose(fp);
      return -1;
    }
    mem = bm_mem_from_device(in.u->addr, in.u->size);
    bm_memcpy_d2s(handle, p, mem);

    for (int i = 0; i < in.avRows(); i++){
      fwrite((char*)p+i*in.avStep(0),1,in.avCols(),fp);
    }
    for (int i = 0; i < in.avRows()/2; i++){
      fwrite((char*)p + in.avRows()*in.avStep(0) + i*in.avStep(1),1,in.avCols(),fp);
    }

    if (p) {free(p); p = NULL;}
#endif
  }else {
    for (int i = 0; i < in.rows; i++)
    {
      fwrite(in.data+i*in.step[0],1,in.cols*in.channels(),fp);
    }
  }

  if (fp) fclose(fp);

  return 0;
}

static int __attribute__((unused)) dump_image(const char *fn, bm_image in)
{
    int i, j;
    bm_status_t ret;
    int src_stride[4];
    FILE *fp;
    void *buf;
    void *data[4] = { NULL };
    int offset = 0;
    int plane_num = bm_image_get_plane_num(in);
    //bm_handle_t handle = bm_image_get_handle(&in);

    fp = fopen(fn, "wb");
    if (!fp) {
        printf("open %s failed\n", fn);
        return -1;
    }

    buf = malloc(in.width * in.height * plane_num);
    if (!buf) {
        printf("malloc failed\n");
        fclose(fp);
        return -1;
    }

    for (i = 0; i < plane_num; i++) {
        data[i] = (char*)buf + offset;
        offset += get_plane_size(in.image_format, i, in.width, in.height);
    }

    ret = bm_image_copy_device_to_host(in, data);
    if (ret != BM_SUCCESS) {
        fclose(fp);
        free(buf);
        return -1;
    }

    ret = bm_image_get_stride(in, src_stride);
    if (ret != BM_SUCCESS) {
        fclose(fp);
        free(buf);
        return -1;
    }

    for (i = 0; i < plane_num; i++){
        int src_offset = 0;
        int plane_width = get_plane_width(in.image_format, i, in.width);
        int plane_height = get_plane_height(in.image_format, i, in.height);

        if (plane_width < 0 || plane_height < 0) {
            fclose(fp);
            free(buf);
            return -1;
        }

        for (j = 0; j < plane_height; j++){
            fwrite((char*)data[i] + src_offset, 1, plane_width, fp);
            src_offset += src_stride[i];
        }
    }

    if (fp) {
        fclose(fp);
    }
    if (buf) {
        free(buf);
    }

    return 0;
}

static void test_conv_1(const char *f0)
{
  Mat m0 = imread(f0, IMREAD_COLOR, g_device_id);
  bmcv::print(m0);

  Mat sub(m0, Rect(16, 16, 128, 128));
  bmcv::print(sub);

  bm_image image;
  bmcv::toBMI(sub, &image);
  bmcv::print(&image);

  Mat d0;
  bmcv::toMAT(&image, d0);
  bmcv::print(d0);

  imwrite("conv_1_0.png", d0);
  bm_image_destroy(&image);
}

// static void test_conv_4(const char *f0, const char *f1, const char *f2, const char *f3)
// {
//   Mat m0 = imread(f0, IMREAD_COLOR, g_device_id);
//   Mat m1 = imread(f1, IMREAD_COLOR, g_device_id);
//   Mat m2 = imread(f2, IMREAD_COLOR, g_device_id);
//   Mat m3 = imread(f3, IMREAD_COLOR, g_device_id);
//   bmcv::print(m0);

//   bm_image image;
//   bmcv::toBMI(m0, m1, m2, m3, &image);
//   bmcv::print(&image);

//   Mat d0, d1, d2, d3;
//   bmcv::toMAT(&image, d0, d1, d2, d3);
//   bmcv::print(d0);

//   imwrite("conv_4_0.png", d0);
//   imwrite("conv_4_1.png", d1);
//   imwrite("conv_4_2.png", d2);
//   imwrite("conv_4_3.png", d3);
//   bm_image_destroy(image);
// }

static void test_size(const char *f0)
{
  Mat m0 = imread(f0, IMREAD_COLOR, g_device_id);
  bmcv::print(m0);

  Mat out(200, 200, CV_8UC3, SophonDevice(g_device_id));
  bmcv::resize(m0, out);
  bmcv::print(out);
  imwrite("size_1_0.png", out);
}

static void test_video(const char *url)
{
  VideoCapture cap(url, cv::CAP_ANY, g_device_id);
  cap.set(cv::CAP_PROP_OUTPUT_YUV, PROP_TRUE);

  Mat frame;
  cap >> frame;
  bmcv::print(frame, true);

  Mat m;
  bmcv::toMAT(frame, m, true);
  imwrite("video_in.png", m);

  Rect rt0(0, 0, 1920, 1080);
  Rect rt1(0, 0, 1280, 720);

  std::vector<Rect> vrt= { rt0, rt1 };

  Size sz0(640, 640);
  Size sz1(128, 128);

  std::vector<Size> vsz= { sz0, sz1 };

  std::vector<Mat> out;
  bmcv::convert(frame, vrt, vsz, out);
  bmcv::print(out[0]);
  bmcv::print(out[1]);

  imwrite("video_0.png", out[0]);
  imwrite("video_1.png", out[1]);
}

static void test_image(const char *f0)
{
  Mat frame = imread(f0, IMREAD_AVFRAME, g_device_id);
  bmcv::print(frame);

  imwrite("image_0.jpg", frame);

  //Mat sub(frame, Rect(16, 16, 128, 128));
  //bmcv::print(sub);
  Mat &sub = frame;

  Mat image;
  bmcv::toMAT(sub, image);
  bmcv::print(image);

  imwrite("image_0.png", image);
}

static void test_cvt(const char *f0)
{
  Mat image = imread(f0, IMREAD_COLOR, g_device_id);
  Mat gray(image.rows, image.cols, CV_8UC1, SophonDevice(g_device_id));

  cvtColor(image, gray, COLOR_BGR2GRAY);

  imwrite("image_cvt.png", gray);
}

static void read_file(const char *filename, void* jpeg_data, size_t* size)
{
    FILE *fp = fopen(filename, "rb+");
    if (fp == NULL) {
        fprintf(stderr, "file not exist or permission not allowed\n");
        exit(-1);
    }
    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fread(jpeg_data, *size, 1, fp);
    printf("read from %s %ld bytes\n", filename, *size);
    fclose(fp);
}

static int bmimage_copyto_bmimage(bm_image in, bm_image out)
{
    int i, j;
    int plane_num = bm_image_get_plane_num(in);
    bm_device_mem_t src_mem[4], dst_mem[4];
    int src_stride[4], dst_stride[4];
    int width = out.width;
    int height = out.height;
    bm_handle_t handle = bm_image_get_handle(&in);
    int total_copy = 0;
    bm_status_t ret;

    assert(width <= in.width);
    assert(height <= in.height);
    ret = bm_image_get_device_mem(in, src_mem);
    if (ret != BM_SUCCESS) {
        printf("bm_image_get_device_mem failed, ret = %d.\n", ret);
        exit(-1);
    }
    ret = bm_image_get_device_mem(out, dst_mem);
    if (ret != BM_SUCCESS) {
        printf("bm_image_get_device_mem failed, ret = %d.\n", ret);
        exit(-1);
    }
    ret = bm_image_get_stride(in, src_stride);
    if (ret != BM_SUCCESS) {
        printf("bm_image_get_stride failed, ret = %d.\n", ret);
        exit(-1);
    }
    ret = bm_image_get_stride(out, dst_stride);
    if (ret != BM_SUCCESS) {
        printf("bm_image_get_stride failed, ret = %d.\n", ret);
        exit(-1);
    }

    for (i = 0; i < plane_num; i++){
        int dst_offset = 0;
        int src_offset = 0;
        int plane_width = get_plane_width(in.image_format, i, width);
        int plane_height = get_plane_height(in.image_format, i, height);

        if (plane_width < 0 || plane_height < 0) return -1;

        for (j = 0; j < plane_height; j++){
            if (BM_SUCCESS != bm_memcpy_d2d_byte(handle, dst_mem[i], dst_offset, src_mem[i], src_offset, plane_width)){
                fprintf(stderr, "ERROR: bm d2d failed\n");
                return -1;
            }
            total_copy += plane_width;
            dst_offset += dst_stride[i];
            src_offset += src_stride[i];
        }
    }
    bm_thread_sync(handle);

    return total_copy;
}

static void test_bmcv2cv(const char *f0)
{
    bm_handle_t handle;
    string prefix;
    bm_status_t ret;
    int width, height;
    // char fn[256];

    ret = bm_dev_request(&handle, BM_CARD_ID( g_device_id ));
    if (ret != BM_SUCCESS) {
        printf("create bm handle failed!");
        exit(-1);
    }
    // read input from picture
    size_t size = 0;
    unsigned char* jpeg_data = (unsigned char*)malloc(4096 * 4096 * 3);
    read_file(f0, jpeg_data, &size);

    // create bm_image used to save output
    bm_image src[1];

start:
    // decode input
    ret = bmcv_image_jpeg_dec(handle, (void**)&jpeg_data, &size, 1, src);
    assert(ret == BM_SUCCESS);
    // sprintf(fn, "%ssrc.yuv", prefix.c_str());
    // dump_image(fn, src[0]);

    width = src[0].width;
    height = src[0].height;

    /* original image */
    printf("step1: test original image\n");
    Mat m;
    cv::bmcv::toMAT(&src[0], m, AVCOL_SPC_BT470BG, AVCOL_RANGE_JPEG, NULL, -1, false, true);
    // sprintf(fn, "%sout.yuv", prefix.c_str());
    // dump_data(fn, m, 1);
    cv::imwrite(prefix+"out"+".jpg", m);

    /* resize to image of different size */
    printf("step2: test 0-63 aligned image\n");
    for (int i = 0; i < 64; i++){
        bm_image out;
        int stride[3];

        stride[0] = (width - i + 15) &~0xf;
        stride[1] = get_plane_width(src->image_format, 1, stride[0]);
        stride[2] = get_plane_width(src->image_format, 2, stride[0]);
        ret = bm_image_create(handle, height-i, width-i, src->image_format, src->data_type, &out, stride);
        assert(ret == BM_SUCCESS);

        ret = bm_image_alloc_contiguous_mem(1, &out, BMCV_HEAP1_ID);
        assert(ret == BM_SUCCESS);

        /* Because some format does not supported by bmcv, we do it by d2d */
        if (bmimage_copyto_bmimage(src[0], out) < 0){
            printf("bm_image copyto bmimage failed\n");
            exit(-1);
        }

        // sprintf(fn, "%ssrc_%d.yuv", prefix.c_str(), i);
        // dump_image(fn, out);

        Mat m0;
        cv::bmcv::toMAT(&out, m0, AVCOL_SPC_BT470BG, AVCOL_RANGE_JPEG, NULL, -1, false, true);
        // sprintf(fn, "%sout_%d.yuv", prefix.c_str(), i);
        // dump_data(fn, m0, 1);
        cv::imwrite(prefix+"out_"+to_string(i)+".jpg", m0);

        bm_image_free_contiguous_mem(1, &out);
        bm_image_destroy(&out);
    }

    /* convert to BGR */
    printf("step3: test BGR png\n");
    {
        bm_image out;

        ret = bm_image_create(handle, height, width, FORMAT_BGR_PACKED, src->data_type, &out);
        assert(ret == BM_SUCCESS);

        ret = bm_image_alloc_contiguous_mem(1, &out);
        assert(ret == BM_SUCCESS);

        ret = bmcv_image_storage_convert_with_csctype(handle, 1, src, &out, CSC_YPbPr2RGB_BT601);
        assert(ret == BM_SUCCESS);

        Mat m1;
        cv::bmcv::toMAT(&out, m1, AVCOL_SPC_BT470BG, AVCOL_RANGE_JPEG, NULL, -1, false, true);
        cv::bmcv::downloadMat(m1);

        cv::imwrite(prefix+"out_bgr"+".png", m1);

        bm_image_free_contiguous_mem(1, &out);
        bm_image_destroy(&out);
    }

    if (src->image_format == FORMAT_YUV420P){
        bm_image_data_format_ext data_type = src->data_type;
        bm_image_destroy(src);

        ret = bm_image_create(handle, height, width, FORMAT_NV12, data_type, src);
        assert(ret == BM_SUCCESS);
        prefix = "nv12_";
        printf("restart for nv12 format\n");
        goto start;
    } else if (src->image_format == FORMAT_YUV422P) {
        bm_image_data_format_ext data_type = src->data_type;
        bm_image_destroy(src);

        ret = bm_image_create(handle, height, width, FORMAT_NV16, data_type, src);
        assert(ret == BM_SUCCESS);
        prefix = "nv16_";
        printf("restart for nv16 format\n");
        goto start;
    }

    /* end */
    bm_image_destroy(src);
    free(jpeg_data);
    bm_dev_free(handle);

    return;
}

static void test_stitch(const char *f, bool default_stitch) {
    Mat frame0 = imread(f, 1, g_device_id);
    Mat frame1 = imread(f, 1, g_device_id);
    Mat output(1920, 2160, frame0.type());
    bool update = true;
    std::vector<Mat> src_img;
    std::vector<Rect> src_vrt;
    std::vector<Rect> dst_vrt;
    if (default_stitch) {
        Rect src_rt(0, 0, frame0.cols, frame0.rows);
        Rect dst_rt0(0, 0, 1920, 1080);
        Rect dst_rt1(0, 1080, 1920, 1080);
        src_img = { frame0, frame1};
        src_vrt = { src_rt, src_rt };
        dst_vrt = { dst_rt0, dst_rt1};
    } else {
        return;
    }
    bmcv::stitch(src_img, src_vrt, dst_vrt, output, update);
    imwrite("dst.png", output);
    return;
}

static void test_mosaic(const char *f, bool default_mosaic, int mosaic_num,
    int start_x, int start_y, int crop_x, int crop_y, int is_expand) {

    Mat frame = imread(f, 1, g_device_id);
    bool update = true;
    // frame.size().width;
    std::vector<Rect> vrt;
    if (default_mosaic) {
        Rect rt(300, 200, 500, 200);
        Rect rt1(100, 200, 300, 400);
        Rect rt2(50, 100, 100, 500);
        vrt= { rt, rt1, rt2 };
    } else {
        Rect rt(start_x, start_y, crop_x, crop_y);
        vrt = { rt };
    }
    bmcv::mosaic(frame, vrt, is_expand, update);
    // imwrite("dst.png", frame);
    return;
}

static void test_quantify(const char *f) {
    Mat frame = imread(f, 1, g_device_id);
    Mat tmp;
    frame.convertTo(tmp, CV_32FC3, 1);
    Mat output(frame.size(), CV_8UC3);
    bool update = true;

    bmcv::quantify(tmp, output, update);
    imwrite("dst.png", output);
    return;
}

static void test_warp_affine(const char* f0, int is_bilinear, int borderMode, int dst_h, int dst_w){
    Mat mat_src = imread(f0, 1, g_device_id);
    Mat mat_dst;

    std::vector<float> data = {
        0.96592583,    0.25881905, -230.06622949,
        -0.25881905,    0.96592583,  172.47349112
    };

    Mat trans_mat1 = Mat(data).reshape(1, 2);

    cv::bmcv::warpAffine(mat_src, mat_dst, trans_mat1, Size(dst_w, dst_h), is_bilinear, borderMode);
    imwrite("dst.png", mat_dst);

    warpAffine(mat_src, mat_dst, trans_mat1, Size(dst_w, dst_h), is_bilinear, borderMode, Scalar(0, 0, 0));
    imwrite("cv_dst.png", mat_dst);
    return;
}

static void test_bitwise_and(const char *f0, const char *f1) {
    Mat frame0 = imread(f0, 1, g_device_id);
    Mat frame1 = imread(f1, 1, g_device_id);
    Mat output(frame0.size(), frame0.type());
    bool update = true;
    bmcv::bitwise_and(frame0, frame1, output, update);
    imwrite("dst.png", output);
    bitwise_and(frame0, frame1, output);
    imwrite("cv_dst.png", output);
    return;
}

static void test_bitwise_or(const char *f0, const char *f1) {
    Mat frame0 = imread(f0, 1, g_device_id);
    Mat frame1 = imread(f1, 1, g_device_id);
    Mat output(frame0.size(), frame0.type());
    bool update = false;
    bmcv::bitwise_or(frame0, frame1, output, update);
    imwrite("dst.png", output);
    bitwise_or(frame0, frame1, output);
    imwrite("cv_dst.png", output);
    return;
}

static void test_bitwise_xor(const char *f0, const char *f1) {
    Mat frame0 = imread(f0, 1, g_device_id);
    Mat frame1 = imread(f1, 1, g_device_id);
    Mat output(frame0.size(), frame0.type());
    bool update = false;
    bmcv::bitwise_xor(frame0, frame1, output, update);
    imwrite("dst.png", output);
    bitwise_xor(frame0, frame1, output);
    imwrite("cv_dst.png", output);
    return;
}

static void test_absdiff(const char *f0, const char *f1) {
    Mat frame0 = imread(f0, 1, g_device_id);
    Mat frame1 = imread(f1, 1, g_device_id);
    Mat output(frame0.size(), frame0.type());
    bool update = true;
    bmcv::absdiff(frame0, frame1, output, update);
    imwrite("dst.png", output);
    absdiff(frame0, frame1, output);
    // imwrite("cv_dst.png", output);
    return;
}

static void test_rotate(const char *f, int rotateCode) {
    Mat frame = imread(f, 1, g_device_id);
    Mat output;

    bool update = true;
    rotate(frame, output, rotateCode);
    imwrite("cv_dst.png", output);
    bmcv::rotate(frame, output, rotateCode, update);
    imwrite("dst.png", output);
    return;
}

static void test_draw_rectangle(const char *f0, bool default_draw, int rect_num,
    int start_x, int start_y, int crop_x, int crop_y, int line_width,
    unsigned char r, unsigned char g, unsigned char b) {
    // one input, method 1:
    Mat frame1 = imread(f0, 1, g_device_id);
    bmcv::rectangle(frame1, Point(start_x, start_y), Point(start_x + crop_x, start_y + crop_y), Scalar(b, g, r), line_width);
    imwrite("dst1.png", frame1);
    rectangle(frame1, Point(start_x, start_y), Point(start_x + crop_x, start_y + crop_y), Scalar(b, g, r), line_width);
    imwrite("cv_dst1.png", frame1);

    // one input, method 2:
    Mat frame2 = imread(f0, 1, g_device_id);
    bmcv::rectangle(frame2, Rect(start_x, start_y, crop_x, crop_y), Scalar(b, g, r), line_width);
    imwrite("dst2.png", frame2);
    rectangle(frame2, Rect(start_x, start_y, crop_x, crop_y), Scalar(b, g, r), line_width);
    imwrite("cv_dst2.png", frame2);

    // multi inputs, method 3;
    Mat frame3 = imread(f0, 1, g_device_id);
    std::vector<Rect> vrt;
    if (default_draw) {
        Rect rt(300, 200, 500, 200);
        Rect rt1(100, 200, 300, 400);
        Rect rt2(50, 100, 100, 500);
        vrt= { rt, rt1, rt2 };
    } else {
        Rect rt(start_x, start_y, crop_x, crop_y);
        vrt = { rt };
    }
    bmcv::rectangle(frame3, vrt, Scalar(b, g, r), line_width);
    imwrite("dst3.png", frame3);
    return;
}

static void test_circle(const char *f0, int center_x, int center_y, int radius,
    int line_width, unsigned char r, unsigned char g, unsigned char b) {
    Mat frame0 = imread(f0, 1, g_device_id);
    bool update = true;
    bmcv::circle(frame0, Point(center_x, center_y), radius, Scalar(b, g, r), line_width, update);
    imwrite("circle.png", frame0);
}

static void test_flip(const char *f, int flipCode) {
    Mat frame = imread(f, 1, g_device_id);
    Mat output, cv_output;

    bool update = true;
    bmcv::flip(frame, output, flipCode, update);
    imwrite("flip.png", output);

    flip(frame, cv_output, flipCode);
    imwrite("cv_flip.png", cv_output);
    return;
}

static void test_convert_to(const char *f, int type, float alpha0,
    float beta0, float alpha1, float beta1, float alpha2, float beta2) {

    Mat frame = imread(f, 1, g_device_id);
    // Mat frame(20, 20, CV_32FC1);
    // cv::randu(frame, 0.0f, 255.0f);
    std::vector<Mat> srcs = { frame };
    std::vector<Mat> dsts(1);
    std::array<float, 3> convert_to_alpha = {alpha0, alpha1, alpha2};
    std::array<float, 3> convert_to_beta = {beta0, beta1, beta2};

    bmcv::convertTo(srcs, dsts, CV_MAKETYPE(type,3), convert_to_alpha, convert_to_beta);
    Mat gray_frame;
    imwrite("dst.png", dsts[0]);

    bmcv::convertTo(frame, dsts[0], CV_MAKETYPE(type,3), 1, 1);
    imwrite("dst1.png", dsts[0]);

    frame.convertTo(dsts[0], CV_MAKETYPE(type,3), 1, 1);
    imwrite("cv_dst.png", dsts[0]);

    return;
}

static void test_threshold(const char *f, unsigned char thresh, unsigned char max_value, int type) {
    Mat frame = imread(f, 0, g_device_id);
    Mat output(frame.size(), frame.type());
    bool update = true;
    bmcv::threshold(frame, output, thresh, max_value, type, update);
    imwrite("dst.png", output);
    threshold(frame, output, thresh, max_value, type);
    imwrite("cv_dst.png", output);
    return;
}

static void test_addWeighted(const char *f0, const char *f1, double alpha, double beta, double gamma) {
    Mat frame0 = imread(f0, 1, g_device_id);
    Mat frame1 = imread(f1, 1, g_device_id);
    Mat output(frame0.size(), frame0.type());
    bool update = true;
    bmcv::addWeighted(frame0, alpha, frame1, beta, gamma, output, update);
    imwrite("dst.png", output);
    addWeighted(frame0, alpha, frame1, beta, gamma, output);
    imwrite("cv_dst.png", output);
    return;
}

static void test_transpose(const char *f) {
    Mat frame = imread(f, IMREAD_GRAYSCALE, g_device_id);
    Mat output;

    bool update = true;
    bmcv::transpose(frame, output, update);
    imwrite("dst.png", output);
    transpose(frame, output);
    imwrite("cv_dst.png", output);
    return;
}

static void Help(const char *programName)
{
    fprintf(stderr, "------------------------------------------------------------------------------\n");
    fprintf(stderr, "%s\n", programName);
    fprintf(stderr, "\tAll rights reserved by Bitmain\n");
    fprintf(stderr, "------------------------------------------------------------------------------\n");
    fprintf(stderr, "USAGE: %s <conv|size> <image file 1> [device_id]\n", programName);
    fprintf(stderr, "       %s <conv> <image file 1> <image file 2> <image file 3> <image file 4> [device_id]\n", programName);
    fprintf(stderr, "       %s <video> <rtsp url> [device_id]\n", programName);
    fprintf(stderr, "       %s <image> <jpeg file> [device_id]\n", programName);
    fprintf(stderr, "       %s <cvt> <jpeg file> [device_id]\n", programName);
    fprintf(stderr, "       %s <bmcv2cv> <jpeg file> [device_id]\n", programName);
    fprintf(stderr, "       %s <warp_affine> <input file> <is_bilinear> <borderMode> <dst_h> <dst_w> [device_id]\n", programName);
    fprintf(stderr, "       %s <rectangle> <image file> <default(1): 3 Rects> [device_id]\n", programName);
    fprintf(stderr, "       %s <rectangle> <image file> <default> <rect_num(1)> <start_x> <start_y> <crop_x> <crop_y> <line_width> <r_value> <g_value> <b_value> [device_id]\n", programName);
    fprintf(stderr, "       %s <circle> <image file> <center_x> <center_y> <radius> <line_width> <r_value> <g_value> <b_value> [device_id]\n", programName);
    fprintf(stderr, "       %s <flip> <image file> <flipCode> [device_id]\n", programName);
    fprintf(stderr, "       %s <bitwise_and> <image file 1> <image file 2> [device_id]\n", programName);
    fprintf(stderr, "       %s <bitwise_or> <image file 1> <image file 2> [device_id]\n", programName);
    fprintf(stderr, "       %s <bitwise_xor> <image file 1> <image file 2> [device_id]\n", programName);
    fprintf(stderr, "       %s <absdiff> <image file 1> <image file 2> [device_id]\n", programName);
    fprintf(stderr, "       %s <mosaic> <image file> <default(1) 3 mosaic Rects> <is_expand> [device_id]\n", programName);
    fprintf(stderr, "       %s <mosaic> <image file> <mosaic num(1)> <start_x> <start_y> <crop_x> <crop_y> <is_expand> [device_id]\n", programName);
    fprintf(stderr, "       %s <quantify> <image file> [device_id]\n", programName);
    fprintf(stderr, "       %s <rotate> <image file> <rotation_angle> [device_id]\n", programName);
    fprintf(stderr, "       %s <threshold> <image file> <thresh> <max_value> <type> [device_id]\n", programName);
    // fprintf(stderr, "       %s <stitch> <image file> <src_rect_num(1)> <src_start_x> <src_start_y> <src_crop_x> <src_crop_y> <dst_rect_num(1)> <dst_start_x> <max_value> <type> [device_id]\n", programName);
    fprintf(stderr, "       %s <stitch> <image file> <default(1)> [device_id]\n", programName);
    fprintf(stderr, "       %s <convertTo> <image file> <type> <alpha0> <beta0> <alpha1> <beta1> <alpha2> <beta2> [device_id]\n", programName);
    fprintf(stderr, "       %s <addWighted> <image file 1> <image file 2> <alpha> <beta> <gamma> [device_id]\n", programName);
    fprintf(stderr, "       %s <transpose> <image file> [device_id]\n", programName);
}

static int parse_args(const char *argv, test_case* tc) {
    if (strcmp(argv, "conv") == 0) {
        *tc = conv;
    } else if (strcmp(argv, "size") == 0) {
        *tc = scale;
    } else if (strcmp(argv, "video") == 0) {
        *tc = video;
    } else if (strcmp(argv, "image") == 0) {
        *tc = image;
    } else if (strcmp(argv, "cvt") == 0) {
        *tc = cvt;
    } else if (strcmp(argv, "bmcv2cv") == 0) {
        *tc = bmcv2cv;
    } else if (strcmp(argv, "warp_affine") == 0) {
        *tc = warp_affine;
    } else if (strcmp(argv, "rectangle") == 0) {
        *tc = bm_rectangle;
    } else if (strcmp(argv, "circle") == 0) {
        *tc = bm_circle;
    } else if (strcmp(argv, "flip") == 0) {
        *tc = bm_flip;
    } else if (strcmp(argv, "bitwise_and") == 0) {
        *tc = bm_bitwise_and;
    } else if (strcmp(argv, "bitwise_or") == 0) {
        *tc = bm_bitwise_or;
    } else if (strcmp(argv, "bitwise_xor") == 0) {
        *tc = bm_bitwise_xor;
    } else if (strcmp(argv, "absdiff") == 0) {
        *tc = bm_absdiff;
    } else if (strcmp(argv, "mosaic") == 0) {
        *tc = mosaic;
    } else if (strcmp(argv, "quantify") == 0) {
        *tc = quantify;
    } else if (strcmp(argv, "rotate") == 0) {
        *tc = bm_rotate;
    } else if (strcmp(argv, "threshold") == 0) {
        *tc = bm_threshold;
    } else if (strcmp(argv, "stitch") == 0) {
        *tc = bm_stitch;
    } else if (strcmp(argv, "convertTo") == 0) {
        *tc = bm_convertTo;
    } else if (strcmp(argv, "addWighted") == 0) {
        *tc = bm_addWeighted;
    } else if (strcmp(argv, "transpose") == 0) {
        *tc = bm_transpose;
    } else {
        printf("Invalid test case: %s\n", argv);
        return -1;
    }
    return 0;
}

int main(int argc, const char** argv)
{
    if (argc < 2) {
        printf("Invalid input!\n");
        Help(argv[0]);
        return -1;
    }

    test_case tc;

    parse_args(argv[1], &tc);

    switch (tc)
    {
    case conv:
        if (argc != 3 && argc != 4 && argc != 6 && argc != 7) {Help(argv[0]); return -1;}
        if (argc == 4 || argc == 7) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_conv_1(argv[2]);
        break;
    case scale:
        if (argc != 3 && argc != 4) {Help(argv[0]); return -1;}
        if (argc == 4) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_size(argv[2]);
        break;
    case video:
        if (argc != 3 && argc != 4) {Help(argv[0]); return -1;}
        if (argc == 4) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_video(argv[2]);
        break;
    case image:
        if (argc != 3 && argc != 4) {Help(argv[0]); return -1;}
        if (argc == 4) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_image(argv[2]);
        break;
    case cvt:
        if (argc != 3 && argc != 4) {Help(argv[0]); return -1;}
        if (argc == 4) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_cvt(argv[2]);
        break;
    case bmcv2cv:
        if (argc != 3 && argc != 4) {Help(argv[0]); return -1;}
        if (argc == 4) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_bmcv2cv(argv[2]);
        break;
    case warp_affine:
        if (argc != 7 && argc != 8) {Help(argv[0]); return -1;}
        if (argc == 8) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_warp_affine(argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
        break;
    case bm_rectangle:
        if (argc != 4 && argc != 5 && argc != 13 && argc != 14) {Help(argv[0]); return -1;}
        if (argc == 5 || argc == 14) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        if (argc < 6) {
            test_draw_rectangle(argv[2], true, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        } else {
            // bool default_ = atoi(argv[3]);
            bool default_ = (strcmp(argv[3], "true") == 0 || strcmp(argv[3], "1") == 0);
            int rect_num = atoi(argv[4]);
            int start_x = atoi(argv[5]);
            int start_y = atoi(argv[6]);
            int crop_x = atoi(argv[7]);
            int crop_y = atoi(argv[8]);
            int line_width = atoi(argv[9]);
            int r = atoi(argv[10]);
            unsigned char g = atoi(argv[11]);
            unsigned char b = atoi(argv[12]);
            test_draw_rectangle(argv[2], default_, rect_num, start_x, start_y, crop_x, crop_y, line_width, r, g, b);
        }
        break;
    case bm_circle: {
        if (argc != 10 && argc != 11) {Help(argv[0]); return -1;}
        if (argc == 11) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        int center_x = atoi(argv[3]);
        int center_y = atoi(argv[4]);
        int radius = atoi(argv[5]);
        int line_width = atoi(argv[6]);
        unsigned char r = atoi(argv[7]);
        unsigned char g = atoi(argv[8]);
        unsigned char b = atoi(argv[9]);
        test_circle(argv[2], center_x, center_y, radius, line_width, r, g, b);
        break;
    }
    case bm_flip:
        if (argc != 4 && argc != 5) {Help(argv[0]); return -1;}
        if (argc == 5) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_flip(argv[2], atoi(argv[3]));
        break;
    case bm_bitwise_and:
        if (argc != 4 && argc != 5) {Help(argv[0]); return -1;}
        if (argc == 5) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_bitwise_and(argv[2], argv[3]);
        break;
    case bm_bitwise_or:
        if (argc != 4 && argc != 5) {Help(argv[0]); return -1;}
        if (argc == 5) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_bitwise_or(argv[2], argv[3]);
        break;
    case bm_bitwise_xor:
        if (argc != 4 && argc != 5) {Help(argv[0]); return -1;}
        if (argc == 5) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_bitwise_xor(argv[2], argv[3]);
        break;
    case bm_absdiff:
        if (argc != 4 && argc != 5) {Help(argv[0]); return -1;}
        if (argc == 5) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_absdiff(argv[2], argv[3]);
        break;
    case mosaic:
        if (argc != 5 && argc != 6 && argc != 9 && argc != 10) {Help(argv[0]); return -1;}
        if (argc == 6 || argc == 10) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        if (argc < 7) {
            test_mosaic(argv[2], true, 0, 0, 0, 0, 0, 0);
        } else {
            int mosaic_num = atoi(argv[3]);
            int start_x = atoi(argv[4]);
            int start_y = atoi(argv[5]);
            int crop_x = atoi(argv[6]);
            int crop_y = atoi(argv[7]);
            int is_expand = atoi(argv[8]);
            test_mosaic(argv[2], false, mosaic_num, start_x, start_y, crop_x, crop_y, is_expand);
        }
        break;
    case quantify:
        if (argc != 3 && argc != 4) {Help(argv[0]); return -1;}
        if (argc == 4) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_quantify(argv[2]);
        break;
    case bm_rotate:
        if (argc != 4 && argc != 5) {Help(argv[0]); return -1;}
        if (argc == 5) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_rotate(argv[2], atoi(argv[3]));
        break;
    case bm_threshold:
        if (argc != 6 && argc != 7) {Help(argv[0]); return -1;}
        if (argc == 7) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_threshold(argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
        break;
    case bm_stitch:
        if (argc != 4 && argc != 5) {Help(argv[0]); return -1;}
        if (argc == 5) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        if (argc < 6)
            test_stitch(argv[2], true);
        // else
        break;
    case bm_convertTo:
        if (argc != 10 && argc != 11) {Help(argv[0]); return -1;}
        if (argc == 11) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_convert_to(argv[2], atoi(argv[3]), atof(argv[4]), atof(argv[5]), atof(argv[6]), atof(argv[7]), atof(argv[8]), atof(argv[9]));
        break;
    case bm_addWeighted: {
        if (argc != 7 && argc != 8) {Help(argv[0]); return -1;}
        if (argc == 8) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        double alpha = atof(argv[4]);
        double beta = atof(argv[5]);
        double gamma = atof(argv[6]);
        test_addWeighted(argv[2], argv[3], alpha, beta, gamma);
        break;
    }
    case bm_transpose:
        if (argc != 3 && argc != 4) {Help(argv[0]); return -1;}
        if (argc == 4) g_device_id = atoi(argv[argc-1]);
        printf("bm_device %d used.\n", g_device_id);
        test_transpose(argv[2]);
        break;
    default:
        printf("Invalid input!\n");
        Help(argv[0]);
        break;
    }

  return 0;
}
