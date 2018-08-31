#include "ofApp.h"
using namespace ofxCv;
using namespace cv;


//--------------------------------------------------------------
void ofApp::setup(){
//    ofSetWindowShape(1280, 800);
    //load the config file
    config.open("config.json");
    outputFbo.allocate(1280*2, 720*2);
    screenWidth = ofGetWidth();
    screenHeight = ofGetHeight();
    

    
    for(int i = 0; i < 4; i++){
        potVals.push_back(0);
    
    }

    ofLog() << "before grabber init";

    grabber.initGrabber(1280/4, 720/4);
    
    //set up the GUI
    blobGui.setup("blob", "blobGui.xml");
    blobGui.add(blobminArea.set("min area", 5, 1, 100));
    blobGui.add(blobmaxArea.set("max area", 2000, 1, 1000));
    blobGui.add(blobthreshold.set("thresh", 128, 0, 255));
    blobGui.add(persistence.set("persistence", 15, 1, 80));
    blobGui.add(maxDist.set("max dist", 32, 10, 200));
    blobGui.loadFromFile("blobGui.xml");
    blobGui.setPosition(10,ofGetHeight() - 10 - blobGui.getHeight());
    
    
    gui.setup("gui", "settings.xml");
    gui.add(bUseipad.set("use ipad", false));
    gui.add(crop.set("crop", 0, 0, 150));
    gui.add(threshold.set("Threshold", 128, 0, 255));
    gui.add(minArea.set("Min area", 5, 1, 100));
    gui.add(maxArea.set("Max area", 2000, 1, 2000));
    gui.add(holes.set("Holes", false));
    gui.add(smoothingSize.set("smoothing size", 1, 0, 40));
    gui.add(drawNumLines.set("Num Line Draw", 10, 0, 200));
    gui.add(iterations.set("contour iterations", 60, 1, 60));
    gui.add(lerpAmt.set("lerp amt", 0.0, 0.0, 1.0));
    gui.add(maxNumPoints.set("numPoints", 1000, 100, 10000));
    gui.add(framesBetweenCapture.set("frames between", 10, 1, 300));
    gui.loadFromFile("settings.xml");
    gui.setPosition(10, ofGetHeight() - 20 - blobGui.getHeight() - gui.getHeight());
    
    ofLog() << "before osc setp";

    //set our OSC listener
    oscIn.setup(config["oscPort"].asInt());
    oscOut.setup("127.0.0.1", config["oscOutPort"].asInt());
    
    //setup what camera we are
    camNumber = config["camNumber"].asInt();
    
    ofSetWindowTitle(ofToString(camNumber));
    
    
    //lets start with two groups
    LineGroup contours;
    groups.push_back(contours);
    LineGroup lines;
    groups.push_back(lines);
    LineGroup merged;
    groups.push_back(merged);
//    pMerge.setup();
    
    bShowOutput = true;
    
    // This was for trying to initialize the static lines once in the update function (when it was first run)
    bStaticLinesInit = false;
    
    testimg.load("test.jpg");
    
    bShowGui = false;

    ofLog() << "before arduino stuff";

    // arduino.connect("/dev/ttyUSB1", 57600);
    ofAddListener(arduino.EInitialized, this, &ofApp::setupArduino);
    // bSetupArduino	= false;
    // arduino.sendReset();

}

//--------------------------------------------------------------
void ofApp::update(){

    //update the grabber;
    grabber.update();

    // if(ofGetElapsedTimeMillis() > 10000 && !bSetupArduino){
    //         arduino.sendFirmwareVersionRequest();

    //     setupArduino(2);
    // }
    
    //update the artduino
    // updateArduino();
    
    //if we have a new frame
    if(grabber.isFrameNew()){

        for(int i = 0; i < 3; i++){
            auto& g = groups[i];
            g.lines.clear();
        }
        
        //get all of the contours
        for(int i = 0; i < iterations; i++){
            contourFinder.setMinAreaRadius(10);
            contourFinder.setMaxAreaRadius(800);
            threshold = ofMap(i, 0, iterations, 100, 255);
            contourFinder.setThreshold(threshold);
            contourFinder.setFindHoles(holes);
            contourFinder.findContours(grabber);

            for(int j = 0; j < contourFinder.size(); j++){
                ofPolyline p;
                p = contourFinder.getPolyline(j);
                groups[0].lines.push_back(p);
            }
        }
    
    // Static line setup
        int numLines = groups[0].lines.size();
        
        if(numLines > 0){
            float spacing = 5;
            float init_x = grabber.getWidth()/2;
            for(int i = 0; i < numLines; i++){
                ofPolyline pl;
                if((i)%2 == 0) {
                    pl.addVertex(ofPoint(init_x + ((i+1)/2)*spacing, 0));
                    pl.addVertex(ofPoint(init_x + ((i+1)/2)*spacing, grabber.getHeight()));
                } else {
                    pl.addVertex(ofPoint(init_x + (-1)*((i+1)/2)*spacing, 0));
                    pl.addVertex(ofPoint(init_x + (-1)*((i+1)/2)*spacing, grabber.getHeight()));
                }
                //            pl.resize(groups[0].lines[i].size());
                groups[1].lines.push_back(pl);
            }
            
            //merge between the two lines
            for(int i = 0; i < numLines; i++){

                pMerge.setNbPoints(groups[0].lines[i].size());
                pMerge.mergePolyline(groups[0].lines[i], groups[1].lines[i], lerpAmt);
                ofPolyline pl = pMerge.getPolyline();
                groups[2].lines.push_back(pl);
            }
        }
        //resize the contours to original resolution (4x the size we have);
        for(int i = 0; i < groups.size(); i++){
            for(auto& l : groups[i].lines){
                for(auto& p : l){
                    p*=8;
                }
                if(i != 1) l = l.getSmoothed(smoothingSize);
            }
        }
    }
    
    
    //add some info to our info stream
    ss.str("");
    ss << "framerate: " << ofGetFrameRate() << endl;
//    ss << "syphonIn server: " << syphonIn.getServerName() << endl;
//    ss << "syphonOut name: " << syphonOut.getName() << endl;
    ss << "oscIn port: " << config["oscPort"].asInt() << endl;
//    ss << "syphonIn size: " << syphonIn.getWidth() << "x" << syphonIn.getHeight() << endl;
    ss << "output size: " << outputFbo.getWidth() << "x" << outputFbo.getHeight() << endl;
    ss << "lines group A: " << groups[0].lines.size() << endl;
    ss << "lines group B: " << groups[1].lines.size() << endl;
    ss << "lines group C: " << groups[2].lines.size() << endl;
    ss << "pot 0 value: " << ofToString(potVals[0]) << endl;
    ss << "pot 1 value: " << ofToString(potVals[1]) << endl;
    ss << "pot 2 value: " << ofToString(potVals[2]) << endl;
    ss << "pot 3 value: " << ofToString(potVals[3]) << endl;
    
    
    
    
    
    
}

//--------------------------------------------------------------
void ofApp::draw(){


    ofClear(0,0,0,255);
    ofSetColor(255,255);
    ofSetLineWidth(3);


    int numLines = groups[2].lines.size();
    int minNumLines = numLines > (int) drawNumLines ? (int) drawNumLines : numLines;
    for(int i = 0; i < minNumLines; i++) {
        auto &pl = groups[2].lines[i];
        pl.draw();
    }


    ofSetColor(255);
    
    if(bShowGui){

        gui.draw();
        blobGui.draw();
        ofDrawBitmapStringHighlight(ss.str(), gui.getPosition().x + gui.getWidth() + 20 , gui.getPosition().y +5);
//        ofDrawBitmapStringHighlight(camInfo.str(), gui.getPosition().x + gui.getWidth() + 250, gui.getPosition().y + 5);
        grabber.draw(0, 0);
    }

}




//--------------------------------------------------------------
void ofApp::getOsc(){
    
    while(oscIn.hasWaitingMessages()){
        ofxOscMessage m;
        oscIn.getNextMessage(m);
        
        string address = "/contours/";
        address += ofToString(camNumber);
        
        string smoothAddress = address;
        smoothAddress += "/smooth";
        
        string iterAddress = address;
        iterAddress += "/iter";
        
        string lerpAddress = address;
        lerpAddress += "/lerp";
        
        string numAddress = address;
        numAddress += "/numLines";
        
        
        if(m.getAddress() == smoothAddress){
            smoothingSize = (int)ofMap(m.getArgAsFloat(0), 0, 1.0, 1, 60);
        }

        
        if(m.getAddress() == "/contours/bUseIpad"){
            bUseipad = m.getArgAsInt(0);
        }
        
        
        else if(m.getAddress() == iterAddress){
            drawNumLines = (int)ofMap(m.getArgAsFloat(0), 0.0, 1.0, 1, 200);
        }
        
        if(bUseipad){
            if(m.getAddress() == lerpAddress){
                lerpAmt = m.getArgAsFloat(0);
                
            }
            
//            else if(m.getAddress() == iterAddress){
//                drawNumLines = (int)ofMap(m.getArgAsFloat(0), 0.0, 1.0, 1, 200);
//            }
        }
        
        else if(bUseipad == false){
            if(m.getAddress() == "/master/lowEnv"){
                
                float f = 1 - m.getArgAsFloat(0);
                
                if(f  < 0.3) f = 0.0;
                
                lerpAmt = f;
            }
            
//            if(m.getAddress() == "/count"){
//                drawNumLines = m.getArgAsFloat(0);;
//                ofLog() << "got mess";
//            }
            
        }
       
        
    }
    
    
}




//--------------------------------------------------------------
void ofApp::setupArduino(const int & version) {
    
    // remove listener because we don't need it anymore
    ofRemoveListener(arduino.EInitialized, this, &ofApp::setupArduino);
    
    // it is now safe to send commands to the Arduino
    bSetupArduino = true;
    
    // print firmware name and version to the console
    ofLogNotice("arduino") << arduino.getFirmwareName();
    ofLogNotice("arduino") << "firmata v" << arduino.getMajorFirmwareVersion() << "." << arduino.getMinorFirmwareVersion();
    
    // Note: pins A0 - A5 can be used as digital input and output.
    // Refer to them as pins 14 - 19 if using StandardFirmata from Arduino 1.0.
    // If using Arduino 0022 or older, then use 16 - 21.
    // Firmata pin numbering changed in version 2.3 (which is included in Arduino 1.0)
    
    // set pin A0 to analog input
    arduino.sendAnalogPinReporting(0, ARD_ANALOG);
    arduino.sendAnalogPinReporting(1, ARD_ANALOG);
    arduino.sendAnalogPinReporting(2, ARD_ANALOG);
    arduino.sendAnalogPinReporting(3, ARD_ANALOG);
    
      // Listen for changes on the digital and analog pins
    ofAddListener(arduino.EDigitalPinChanged, this, &ofApp::digitalPinChanged);
    ofAddListener(arduino.EAnalogPinChanged, this, &ofApp::analogPinChanged);
}

//--------------------------------------------------------------
void ofApp::updateArduino(){
    
    // update the arduino, get any data or messages.
    // the call to ard.update() is required
    arduino.update();
    
    // do not send anything until the arduino has been set up
    if (bSetupArduino) {
        // fade the led connected to pin D11
        arduino.sendPwm(11, (int)(128 + 128 * sin(ofGetElapsedTimef())));   // pwm...
    }
    
}

// digital pin event handler, called whenever a digital pin value has changed
// note: if an analog pin has been set as a digital pin, it will be handled
// by the digitalPinChanged function rather than the analogPinChanged function.

//--------------------------------------------------------------
void ofApp::digitalPinChanged(const int & pinNum) {
    // do something with the digital input. here we're simply going to print the pin number and
    // value to the screen each time it changes
    buttonState = "digital pin: " + ofToString(pinNum) + " = " + ofToString(arduino.getDigital(pinNum));
}

// analog pin event handler, called whenever an analog pin value has changed

//--------------------------------------------------------------
void ofApp::analogPinChanged(const int & pinNum) {
    // do something with the analog input. here we're simply going to print the pin number and
    // value to the screen each time it changes
    
    if(pinNum < 4){
        potVals[pinNum] = arduino.getAnalog(pinNum);
    }
    
}






//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    if(key == ' '){
        bShowGui = !bShowGui;
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
