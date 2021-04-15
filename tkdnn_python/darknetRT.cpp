#include "darknetRT.h"

bool gRun;
bool SAVE_RESULT = false;

void sig_handler(int signo) {
    std::cout<<"request gateway stop\n";
    gRun = false;
}

extern "C"
{


void copy_image_from_bytes(image im, unsigned char *pdata)
{
    int w = im.w;
    int h = im.h;
    int c = im.c;

    memcpy(im.data, pdata, h * w * c);
}

image make_empty_image(int w, int h, int c)
{
    image out;
    out.data = 0;
    out.h = h;
    out.w = w;
    out.c = c;
    return out;
}

image make_image(int w, int h, int c)
{
    image out = make_empty_image(w,h,c);
    out.data = (float*)xcalloc(h * w * c, sizeof(float));
    return out;
}

tk::dnn::Yolo3Detection* load_network(char* net_cfg, int n_classes, int n_batch)
{
    std::string net;
    net = net_cfg;
    tk::dnn::Yolo3Detection *detNN = new tk::dnn::Yolo3Detection;
    detNN->init(net, n_classes, n_batch);

    return detNN;
}
#include <typeinfo>
void do_inference(tk::dnn::Yolo3Detection *net, image im)
{
    std::vector<cv::Mat> batch_dnn_input;

    cv::Mat frame(im.h, im.w, CV_8UC3, (unsigned char*)im.data);
    batch_dnn_input.push_back(frame.clone());
    net->update(batch_dnn_input, 1);

}

detection* get_network_boxes(tk::dnn::Yolo3Detection *net, float thresh, int *pnum)
{
    std::vector<tk::dnn::box> detected;
    detected = net->get_detected();
    int nboxes =0;
    std::vector<std::string> classesName = net->get_classesName();
    detection* dets = (detection*)xcalloc(detected.size(), sizeof(detection));
    for (int i = 0; i < detected.size(); ++i)
    {
        if (detected[i].prob > thresh)
        {
            dets[nboxes].cl = detected[i].cl;
            strcpy(dets[nboxes].name,classesName[dets[nboxes].cl].c_str());
            dets[nboxes].bbox.x = detected[i].x;
            dets[nboxes].bbox.y = detected[i].y;
            dets[nboxes].bbox.w = detected[i].w;
            dets[nboxes].bbox.h = detected[i].h;
            dets[nboxes].prob = detected[i].prob;
            nboxes += 1;
        }
    }
    if (pnum) *pnum = nboxes;
    return dets;
}

}