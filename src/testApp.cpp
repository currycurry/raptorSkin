#include "testApp.h"

const int useKinect = true;

//--------------------------------------------------------------
void testApp::setup() {
	ofSetFrameRate(60);
	
	camWidth = 640;
	camHeight = 480;
	
	raptor.loadImage("images/raptor.jpg");
	//unsigned char * raptorPix = raptor.getPixels();//don't really know how to do this
	
	
	if(useKinect) {
		//kinect.init(true);  //shows infrared image
		kinect.init();
		kinect.setVerbose(true);
		kinect.open();
		
		// this is setting the raw data conversion range
		// we assume that 100 to 300 CM (1 - 3 meters) is good for tracking a person in space
		kinect.getCalibration().setClippingInCentimeters(100, 300);
		
	} else {
		animation.load("janus-1283433262");
		cout << "Animation length is " << animation.size() << " frames." << endl;
	}
	
	depthImage.allocate(camWidth, camHeight);
	colorImage.allocate(camWidth, camHeight);
	grayImage.allocate(camWidth, camHeight);
	backgroundImage.allocate(camWidth, camHeight);
	thresholdImage.allocate(camWidth, camHeight);
	depthForegroundImage.allocate(camWidth, camHeight);
	
	backgroundSet = false;
	
	panel.setup("Control Panel", 5, 5, 300, 600);
	panel.addPanel("Threshold and Scale");
	panel.addPanel("Control");
	panel.addPanel("Capture");
	
	panel.setWhichPanel("Threshold and Scale");
	panel.addSlider("near threshold", "nearThreshold", 255, 0, 255, true);
	panel.addSlider("far threshold", "farThreshold", 0, 0, 255, true);
	panel.addSlider("depth scale", "depthScale", 5, 1, 20);
	panel.addSlider("depth offset", "depthOffset", 156, 0, 255);
	panel.addSlider("step size", "stepSize", 2, 1, 4, true);
	panel.addSlider("point size", "pointSize", 3, 1, 10, true);
	panel.addToggle("draw zeros", "drawZeros", false);
	panel.addSlider("bg subtraction threshold", "bgThreshold", 40, 0, 255);
	panel.addSlider("erode", "erode", 5, 0, 20);
	panel.addSlider("blur", "blur", 0, 0, 50); //just guessing
	panel.addSlider("blurGaussian", "blurGaussian", 0, 0, 50);
	
	panel.setWhichPanel("Control");
	panel.addSlider("rotate y axis", "rotateY", -180, -360, 360, false);	
	panel.addToggle("draw debug", "drawDebug", false);
	
	panel.setWhichPanel("Capture");
	panel.addToggle("set background", "setBackground", false);
	panel.addToggle("capture image", "captureImage", false);
	panel.addToggle("use jpeg", "useJpeg", false);
}

//--------------------------------------------------------------
void testApp::update() {
	if(useKinect) {
		//toby
		if(panel.hasValueChanged("nearThreshold") ||
		   panel.hasValueChanged("farThreshold")) {
			float nearClipping = panel.getValueF("nearThreshold");
			float farClipping = panel.getValueF("farThreshold");
			kinect.getCalibration().setClippingInCentimeters(nearClipping, farClipping);
			panel.clearAllChanged();
		}
		
		kinect.update();
		if(kinect.isFrameNew()) {
			depthImage.setFromPixels(kinect.getDepthPixels(), camWidth, camHeight);
			depthImage.flagImageChanged();
			
			//map rgb to depth pixels
			/* colorImage.setFromPixels(kinect.getPixels(), camWidth, camHeight);
			 colorImage.translate(-12, -30);
			 colorImage.flagImageChanged();  */  
			
			//raptor skin
			 if(panel.getValueB("useJpeg")) {
			 useJpegPix = false;
			 panel.setValueB("useJpeg", false);
			 colorImage.setFromPixels(raptor.getPixels(), 640, 480); 
			 
			 }  
			
			
			
			//snapshot
			/*if (panel.getValueB("captureImage")) {
				captureColorImage = false;
				panel.setValueB("captureImage", false);
				colorImage.setFromPixels(kinect.getPixels(), camWidth, camHeight);
				colorImage.translate(-12, -30);
				colorImage.flagImageChanged();
			}*/
		}
	} 
	else {
		int frame = ofGetFrameNum() % animation.size();
		ofImage& cur = animation.getAlpha(frame);
		depthImage.setFromPixels(cur.getPixels(), cur.getWidth(), cur.getHeight());
		depthImage.flagImageChanged();
	}
	
	// do processing here on depthImage
	grayImage = depthImage;
	
	if (panel.getValueB("setBackground")) {
		backgroundSet = false;
		panel.setValueB("setBackground", false);
	}
	if (!backgroundSet) {
		backgroundImage = grayImage;
		backgroundSet = true;
	}
	
	thresholdImage.absDiff(backgroundImage, grayImage);
	thresholdImage.threshold(panel.getValueI("bgThreshold"));
	
	for (int i = 0; i < panel.getValueI("erode"); i++) {
		thresholdImage.erode();
	}
	
	for (int i = 0; i < panel.getValueI("blur"); i++) {
		thresholdImage.blur();
	}
	
	for (int i = 0; i < panel.getValueI("blurGaussian"); i++) {
		thresholdImage.blurGaussian();
	}
	
	unsigned char* thresholdPixels = thresholdImage.getPixels();
	unsigned char* depthPixels = depthImage.getPixels();
	unsigned char depthForegroundPixels[camWidth*camHeight];
	int nearThresh =  panel.getValueF("nearThreshold");
	int farThresh =  panel.getValueF("farThreshold");
	
	for (int i = 0; i < camWidth * camHeight; i++) {
		if (thresholdPixels[i]) {
			depthForegroundPixels[i] = depthPixels[i];
		} else {
			depthForegroundPixels[i] = 0;
		}
	}
	depthForegroundImage.setFromPixels(depthForegroundPixels, camWidth, camHeight);
	
	
	
	
	
}

//--------------------------------------------------------------
void testApp::draw() {
	
	
	ofBackground(0, 0, 0);
	
	// draw debug or non debug
	
	if (panel.getValueB("drawDebug")){
		ofPushMatrix();
		// center everything
		//ofTranslate((ofGetWidth() - camWidth) / 2, 0, 0);
		ofSetColor(255, 255, 255);
		//depthImage.draw(0, 0);
		depthForegroundImage.draw(0, 0);
		ofPushMatrix();
		ofTranslate(0, camHeight);
		//ofTranslate(camWidth / 2, camHeight / 2);
		ofRotateY(panel.getValueF("rotateY"));
		//ofTranslate(-camWidth / 2, -camHeight / 2);
		ofTranslate(0, 100);
		drawPointCloud();
		ofPopMatrix();
		ofPopMatrix();
	} else {
		ofPushMatrix();
		// center everything
		ofTranslate(ofGetWidth()/2, ofGetWidth()/2, 0);
		ofSetColor(255, 255, 255);
		ofRotateY(panel.getValueF("rotateY"));
		ofTranslate(-camWidth / 2, -camHeight / 2-200);
		drawPointCloud();
		ofPopMatrix();
		
	}
	
}

//--------------------------------------------------------------
void testApp::drawPointCloud() {
	if(useKinect) {
		ofScale(1, 1, panel.getValueF("depthScale"));
		ofTranslate(0, 0, -panel.getValueF("depthOffset"));
	} else {
		ofScale(1, 1, 2.5);
		ofTranslate(0, 0, -128);
	}
	
	unsigned char* depthPixels = depthForegroundImage.getPixels();
	unsigned char* colorPixels = colorImage.getPixels();
	unsigned char* raptorPix = raptor.getPixels();
	
	glEnable(GL_POINT_SMOOTH);
	glPointSize(panel.getValueF("pointSize"));
	glBegin(GL_POINTS);
	//test
	int step = panel.getValueI("stepSize");
	bool drawZeroes = panel.getValueB("drawZeros");
	
	for(int y = 0; y < camHeight; y += step) {
		for(int x = 0; x < camWidth; x += step) {
			int i = y * camWidth + x;
			// this is an orthographic projection,
			// which isn't really 'correct'
			if(!(depthPixels[i] == 0 && !drawZeroes)) {
				
				
				//colors pointcloud
				glColor3ub(colorPixels[i*3], colorPixels[i*3+1], colorPixels[i*3+2]);
				//glColor3ub(44, 122, 205);
				glVertex3f(x, y, depthPixels[i]);
			}
		}
	}
	glEnd();
}

//--------------------------------------------------------------
void testApp::exit() {
	if(useKinect) {
		kinect.close();
	}
}

//--------------------------------------------------------------

void testApp::keyPressed (int key) {
	if(key == 'f') {
		ofToggleFullscreen();
	}
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y) {
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button)
{}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button)
{}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button)
{}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h)
{}

