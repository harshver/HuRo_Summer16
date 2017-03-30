/*Following are the copyright details of the code that was used as reference*/
/*
 * Copyright (c) 2011. Philipp Wagner <bytefish[at]gmx[dot]de>.
 * Released to public domain under terms of the BSD Simplified license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the organization nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 *   See <http://www.opensource.org/licenses/bsd-license>
 */

#include "opencv2/core.hpp"
#include "opencv2/face.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/objdetect.hpp"
#include <time.h>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace cv;
using namespace cv::face;
using namespace std;

const int PREDICTION_NUM = 40;
int arr[PREDICTION_NUM] = {0};
int num=0;
int f=1;
int found1=0;
int prediction=-1;

ofstream fout;
ifstream fin;

static void read_csv(const string& filename, vector<Mat>& images, vector<int>& labels, char separator = ';') {
    std::ifstream file(filename.c_str(), ifstream::in);
    if (!file) {
        string error_message = "No valid input file was given, please check the given filename.";
        CV_Error(Error::StsBadArg, error_message);
    }
    string line, path, classlabel;
    while (getline(file, line)) {
        stringstream liness(line);
        getline(liness, path, separator);
        getline(liness, classlabel);
        if(!path.empty() && !classlabel.empty()) {
            images.push_back(imread(path, 0));
            labels.push_back(atoi(classlabel.c_str()));
        }
    }
}

int main(int argc, const char *argv[]) {
    // Check for valid command line arguments, print usage
    // if no arguments were given.
    /*if (argc != 4) {
        cout << "usage: " << argv[0] << " </path/to/haar_cascade> </path/to/csv.ext> </path/to/device id>" << endl;
        cout << "\t </path/to/haar_cascade> -- Path to the Haar Cascade for face detection." << endl;
        cout << "\t </path/to/csv.ext> -- Path to the CSV file with the face database." << endl;
        cout << "\t <device id> -- The webcam device id to grab frames from." << endl;
        exit(1);
    }*/
    // Get the path to your CSV:
    string fn_haar = string("haarcascade_frontalface_alt.xml");//argv[1]);
    string fn_csv = string("database.csv");//argv[2]);
    int deviceId = 0;//atoi(argv[3]);
    // These vectors hold the images and corresponding labels:
    vector<Mat> images;
    vector<int> labels;
    // Read in the data (fails if no valid input filename is given, but you'll get an error message):
    try {
        read_csv(fn_csv, images, labels);
    } catch (cv::Exception& e) {
        cerr << "Error opening file \"" << fn_csv << "\". Reason: " << e.msg << endl;
        // nothing more we can do
        exit(1);
    }
    // Get the height from the first image. We'll need this
    // later in code to reshape the images to their original
    // size AND we need to reshape incoming faces to this size:
    int im_width = images[0].cols;
    int im_height = images[0].rows;
    // Create a FaceRecognizer and train it on the given images:
    Ptr<BasicFaceRecognizer> model = createFisherFaceRecognizer();
    // imshow("1",images.at(0));
    model->train(images, labels);
    // That's it for learning the Face Recognition model. You now
    // need to create the classifier for the task of Face Detection.
    // We are going to use the haar cascade you have specified in the
    // command line arguments:
    //
    CascadeClassifier haar_cascade;
    haar_cascade.load(fn_haar);
    // Get a handle to the Video device:
    VideoCapture cap(deviceId);
    // Check if we can use this device at all:
    if(!cap.isOpened()) {
        cerr << "Capture Device ID " << deviceId << "cannot be opened." << endl;
        return -1;
    }
    // Holds the current frame from the Video device:
    int max, found, face_max_num=0;
    Mat frame;
    Rect face_i;
    Rect minSize(0,0,80,80); Rect maxSize(0,0,250,250);
    clock_t tim=clock();
    for(;;) {

        cap >> frame;
        // Clone the current frame:
        Mat original = frame.clone();
        // Convert the current frame to grayscale:
        Mat gray;
        Mat temp;
        Rect roi(0,frame.rows/5,frame.cols,3*frame.rows/5);
        cvtColor(temp=original(roi), gray, COLOR_BGR2GRAY);
        
        rectangle(original,roi,Scalar(128,128,0),1);
        imshow("1",gray);
        waitKey(10);
        // Find the faces in the frame:
        vector< Rect_<int> > faces;
        double scaleFactor;
        int minNeighbours, flags;
        haar_cascade.detectMultiScale(gray, faces, scaleFactor=1.3, minNeighbours=5,flags=0,minSize.size(),maxSize.size());
        //cerr<<"face_num"<<faces.size()<<'\t'<<"area"<<faces[0].size();
        // At this point you have the position of the faces in
        // faces. Now we'll get the faces, make a prediction and
        // annotate it in the video. Cool or what?
        max = 3000,found =0;
        for(size_t i = 0; i < faces.size(); i++) {
            // Process face by face:
            face_i = faces[i];
            cerr<<"area"<<face_i.size().height*face_i.size().width;

            if(face_i.size().height*face_i.size().width>max){
                max=face_i.size().height*face_i.size().width;
                face_max_num=i; 
                found=1; 
                found1++; 
            }
        }

        if(found){
            // Crop the face from the image. So simple with OpenCV C++:
            Mat face = gray(faces[face_max_num]);
            // Resizing the face is necessary for Eigenfaces and Fisherfaces. You can easily
            // verify this, by reading through the face recognition tutorial coming with OpenCV.
            // Resizing IS NOT NEEDED for Local Binary Patterns Histograms, so preparing the
            // input data really depends on the algorithm used.
            //
            // I strongly encourage you to play around with the algorithms. See which work best
            // in your scenario, LBPH should always be a contender for robust face recognition.
            //
            // Since I am showing the Fisherfaces algorithm here, I also show how to resize the
            // face you have just found:
            Mat face_resized;
            cv::resize(face, face_resized, Size(im_width, im_height), 1.0, 1.0, INTER_CUBIC);
            // Now perform the prediction, see how easy that is:
            int prediction1 = model->predict(face_resized);

            arr[prediction1]+=1;
            num++;

            if(num%51==0)
            {
                int mode =0;
                for(int j=1;j<PREDICTION_NUM;j++){
                    if(arr[j] > arr[mode])
                        mode=j;
                }
                prediction = mode;
                num=0;
            }
            // And finally write all we've found out to the original image!
            // First of all draw a green rectangle around the detected face:
            rectangle(temp, faces[face_max_num], Scalar(0, 0, 255), 1);
            // Create the text we will annotate the box with:
            
            string box_text;
            if(prediction==0)
                box_text = format("Hi Bhuvi!");
            if(prediction==1)
                box_text = format("Hi Ujjwal!");
            if(prediction==2)
                box_text = format("Hi Shruti!");
            if(prediction==3)
                box_text = format("Hi Nitish!");
            if(prediction==4)
                box_text = format("Hi Saurabh!");
            if(prediction==5)
                box_text = format("Hi Rijak!");
            if(prediction==6)
                box_text = format("Hi Mrinaal!");

            // Calculate the position for annotated text (make sure we don't
            // put illegal values in there):
            int pos_x = std::max(faces[face_max_num].tl().x - 10, 0);
            int pos_y = std::max(faces[face_max_num].tl().y - 10, 0);
            // And now put it into the image:
            cerr<<"prediction"<<prediction<<'\t'<<"X,Y:"<<pos_x<<'\t'<<pos_y<<'\n';
            putText(original, box_text, Point(pos_x, pos_y), FONT_HERSHEY_PLAIN, 1.0, Scalar(0,0,255), 2);
        }
        faces.clear();        
        // Show the result:
        imshow("face_recognizer", original);
        if(found1==f)
        {
            f++;
            tim=clock();
            char ch='z';
            fout.open("/dev/ttyACM0");
            fout<<ch;
            cerr<<ch<<"\n";
            fout.flush();
            fout.close();
                                    
        }
        // And display it:
        char key = (char) waitKey(20);
        // Exit this loop on escape:
        if(key == 27)
            break;
        
    }
    return 0;
}
