#include <iostream>
#include <signal.h>
#include <stdlib.h>     /* srand, rand */
#include <unistd.h>
#include <mutex>

#include "CenternetDetection.h"
#include "MobilenetDetection.h"
#include "Yolo3Detection.h"

bool gRun;
bool SAVE_RESULT = false;

void sig_handler(int signo) {
    std::cout<<"request gateway stop\n";
    gRun = false;
}

int main(int argc, char *argv[]) {

    std::cout<<"detection\n";
    signal(SIGINT, sig_handler);


    std::string net = "yolo3_berkeley.rt";
    if(argc > 1)
        net = argv[1]; 
    std::string input = "../demo/yolo_test.mp4";
    if(argc > 2)
        input = argv[2]; 
    char ntype = 'y';
    if(argc > 3)
        ntype = argv[3][0]; 
    int n_classes = 80;
    if(argc > 4)
        n_classes = atoi(argv[4]); 
    int n_batch = 1;
    if(argc > 5)
        n_batch = atoi(argv[5]); 
    bool show = true;
    if(argc > 6)
        show = atoi(argv[6]); 
    float conf_thresh=0.2;
    if(argc > 7)
        conf_thresh = atof(argv[7]);     

    if(n_batch < 1 || n_batch > 64)
        FatalError("Batch dim not supported");

    if(!show)
        SAVE_RESULT = true;

    tk::dnn::Yolo3Detection yolo;
    tk::dnn::CenternetDetection cnet;
    tk::dnn::MobilenetDetection mbnet;  

    tk::dnn::DetectionNN *detNN;  

    switch(ntype)
    {
        case 'y':
            detNN = &yolo;
            break;
        case 'c':
            detNN = &cnet;
            break;
        case 'm':
            detNN = &mbnet;
            n_classes++;
            break;
        default:
        FatalError("Network type not allowed (3rd parameter)\n");
    }

    detNN->init(net, n_classes, n_batch, conf_thresh);

    cv::Mat image;
    std::vector<cv::Mat> dnnInput;
    image = cv::imread(input, cv::IMREAD_COLOR);   // Read the file

    if(! image.data ) {                              // Check for invalid input
        FatalError("Image not valid")
    }
    
    if(show)
        cv::namedWindow("detection", cv::WINDOW_NORMAL);

    std::vector<cv::Mat> batch_frame;
    std::vector<cv::Mat> batch_dnn_input;

    batch_dnn_input.clear();
    batch_frame.clear();
        
            
    batch_frame.push_back(image);

    // this will be resized to the net format
    batch_dnn_input.push_back(image.clone());
         
    
    //inference
    detNN->update(batch_dnn_input, n_batch);
    std::vector<tk::dnn::box> boxes = detNN->detected;
    std::cout << "Number of BBs: " << boxes.size() << std::endl;
    // detNN->draw(batch_frame);

    for (int i = 0; i < boxes.size(); i++) {
    std::cout << "Box " << i << " contents: \n";
    boxes[i].print();
    }

    if(show){
        for(int bi=0; bi< n_batch; ++bi){
            cv::imshow("detection", batch_frame[bi]);
            cv::waitKey(1);
        }
    }
        

    std::cout<<"detection end\n";   
    double mean = 0; 
    
    std::cout<<COL_GREENB<<"\n\nTime stats:\n";
    std::cout<<"Min: "<<*std::min_element(detNN->stats.begin(), detNN->stats.end())/n_batch<<" ms\n";    
    std::cout<<"Max: "<<*std::max_element(detNN->stats.begin(), detNN->stats.end())/n_batch<<" ms\n";    
    for(int i=0; i<detNN->stats.size(); i++) mean += detNN->stats[i]; mean /= detNN->stats.size();
    std::cout<<"Avg: "<<mean/n_batch<<" ms\t"<<1000/(mean/n_batch)<<" FPS\n"<<COL_END;   
    

    return 0;
}