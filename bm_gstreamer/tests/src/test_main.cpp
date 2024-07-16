#include <gst/check/gstcheck.h>

extern Suite*
bmvideodec_suite(void);

extern Suite*
bmvideoenc_suite(void);

int
main(int argc, char** argv)
{
  int ret = 0;

  gst_check_init(&argc, &argv);

  Suite *video_decode;
  video_decode = bmvideodec_suite();
  ret += gst_check_run_suite(video_decode, "bmvideo_dec", __FILE__);

  Suite *video_encode;
  video_encode = bmvideoenc_suite();
  ret += gst_check_run_suite(video_encode, "bmvideo_enc", __FILE__);

  return ret;
}
