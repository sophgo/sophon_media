#include "ff_video_filter.h"
#include "bmlib_runtime.h"

int initFilter(ffmpegFilterContext *filter, char* args_main, char** args_overlay_pics, char** args_overlay) {

    int i = 0;

    AVFilterGraph *filter_graph = avfilter_graph_alloc();

    const AVFilter *src = avfilter_get_by_name("buffer");
    const AVFilter *overlay = avfilter_get_by_name("overlay_bm");
    const AVFilter *sink = avfilter_get_by_name("buffersink");

    avfilter_graph_create_filter(&filter->src_ctx_main, src, "src_main", args_main, nullptr, filter_graph);
    avfilter_graph_create_filter(&filter->sink_ctx, sink, "sink", nullptr, nullptr, filter_graph);

    for(i=0; i<filter->overlay_num; i++)
    {
        avfilter_graph_create_filter(&filter->src_ctx_overlay[i], src, "src_overlay", args_overlay_pics[i], nullptr, filter_graph);
        avfilter_graph_create_filter(&filter->overlay_ctx[i], overlay, "overlay", args_overlay[i], nullptr, filter_graph);
    }

    avfilter_link(filter->src_ctx_main, 0, filter->overlay_ctx[0], 0);
    avfilter_link(filter->src_ctx_overlay[0], 0, filter->overlay_ctx[0], 1);

    for(i=1; i<filter->overlay_num; i++)
    {
        avfilter_link(filter->overlay_ctx[i-1], 0, filter->overlay_ctx[i], 0);
        avfilter_link(filter->src_ctx_overlay[i], 0, filter->overlay_ctx[i], 1);
    }

    avfilter_link(filter->overlay_ctx[filter->overlay_num-1], 0, filter->sink_ctx, 0);

    avfilter_graph_config(filter_graph, nullptr);
end:
    if (filter->filterGraph) {
        avfilter_graph_free(&filter->filterGraph);
    }
    return -1;
}

AVFrame* getFilterFrame(ffmpegFilterContext filter, AVFrame *main, AVFrame **overlay){
    int i = 0;
    AVFrame* frame = av_frame_alloc();
    int ret = av_buffersrc_write_frame(filter.src_ctx_main, main);
    if(ret < 0)
        return NULL;

    for(i=0; i<filter.overlay_num; i++)
    {
        ret = av_buffersrc_write_frame(filter.src_ctx_overlay[i], overlay[i]);
        if(ret < 0)
            return NULL;
    }

    ret = av_buffersink_get_frame(filter.sink_ctx, frame);
    if(ret < 0)
        return NULL;

    return frame;
}

int deInitFilter(ffmpegFilterContext filter){
    avfilter_graph_free(&(filter.filterGraph));
    return 0;
}
