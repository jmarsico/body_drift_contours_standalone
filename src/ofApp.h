#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxOsc.h"
#include "ofxPolylineMerger.h"
#include "ofxJSON.h"
#include "ofxFastFboReader.h"
#include "IPVideoGrabber.h"

using namespace ofx;
class ofApp : public ofBaseApp{

public:
    void setup();
    void update();
    void draw();
    void getOsc();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    
    ofxPanel gui;
    ofParameter<bool> bUseipad;
    ofParameter<int> crop;
    ofParameter<bool> holes;
    ofParameter<float> minArea;
    ofParameter<float> maxArea;
    ofParameter<int> threshold;
    ofParameter<int> smoothingSize;
    ofParameter<int> iterations;
    ofParameter<float> lerpAmt;
    ofParameter<int> maxNumPoints;
    ofParameter<int> framesBetweenCapture;
    ofParameter<int> pixBufferSize;
    ofParameter<int> drawNumLines;
    
    ofxPanel blobGui;
    ofParameter<float> blobminArea;
    ofParameter<float> blobmaxArea;
    ofParameter<int> blobthreshold;
    ofParameter<int> persistence;
    ofParameter<int> maxDist;
    
    bool bShowGui;
    
    

    bool bShowOutput;
    bool bStaticLinesInit;
    ofxCv::ContourFinder contourFinder;
    ofxCv::ContourFinder blobTracker;
    
    
    //we want several sets of polylines
    struct LineGroup{
        vector<ofPolyline> lines;
    };
    vector<LineGroup> groups;
    
    //syphon in and out
    ofFbo inputFbo;
    ofPixels inputPix;
    
    ofFbo outputFbo;
    
    
    //fast fbo reader to pull down the pixels from syphon
    ofxFastFboReader reader;
    
    
	//for gaining
    ofxOscReceiver oscIn;
    ofxOscSender oscOut;
    
    //we want to average several incoming images
    deque<ofPixels> pixBuffer;
    vector<int> totalPixVal;
    
    //out polyline merger
    ofxPolylineMerger pMerge;
    
    //config info
    ofxJSONElement config;
    
    //stringstream to capture info as we go
    stringstream ss;
    
    int camNumber;
    
    
    ofImage testimg;
    
    int screenWidth, screenHeight;
    
    ofArduino arduino;
    bool bSetupArduino;
    vector<int> potVals;
    
    //the ip video grabber
//    std::shared_ptr<Video::IPVideoGrabber> grabber;
    ofVideoGrabber grabber;
    ofPixels cropped;
    
private:
    void setupArduino(const int & version);
    void digitalPinChanged(const int & pinNum);
    void analogPinChanged(const int & pinNum);
    void updateArduino();
    
    string buttonState;

    
    
    
};
