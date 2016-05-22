#include "ofApp.h"
// #include <map>


void ofApp::ofAppDoBTN1(int state){
	// state = 0 is release, state = 1 is press
	msg = ofToString(state);

}



//--------------------------------------------------------------
void ofApp::setup(){
	ofSetWindowTitle("osc receiver");
	cout << "listening for osc messages on port " << PORT << "\n";
	// OSC
	receiver.setup(PORT);

	// CAMERA
	// camera.setup(WIDTH, HEIGHT, true);
	camera.initGrabber(WIDTH, HEIGHT);
	videoInverted.allocate(WIDTH, HEIGHT, OF_PIXELS_RGB);
	videoTexture.allocate(videoInverted);
	// didCamUpdate = false;
	cameraFbo.allocate(640, 480);
	cameraFbo.black();

	// FT
	// process all but the density on 16th resolution
	flowWidth = WIDTH / 4;
	flowHeight = HEIGHT / 4;
	opticalFlow.setup(flowWidth, flowHeight);
	velocityMask.setup(WIDTH, HEIGHT);
	fluidSimulation.setup(flowWidth, flowHeight, WIDTH, HEIGHT);
	particleFlow.setup(flowWidth, flowHeight, WIDTH, HEIGHT);
	velocityDots.setup(flowWidth / 4, flowHeight / 4);

	// parameters for output
	doDrawCamBackground.set(false);

}

//--------------------------------------------------------------
void ofApp::update(){
	// camera.draw(0, 0);
	camera.update();
	// if(camera.isFrameNew()){
	//     ofPixels & pixels = camera.getPixels();
	//     for(int i = 0; i < pixels.size(); i++){
	//         //invert the color of the pixel
	//         videoInverted[i] = 255 - pixels[i];
	//     }
	//     //load the inverted pixels
	//     videoTexture.loadData(videoInverted);
	// }

	// OSC
	while (receiver.hasWaitingMessages()) {
		ofxOscMessage m;
		receiver.getNextMessage(m);

		// TODO: ROUTER?
		if (m.getAddress() == "/reset") {
			// ofAppDoBTN1(m.getArgAsInt32(0));
			fluidSimulation.reset();
		}
		if (m.getAddress() == "/camera") {
			doDrawCamBackground.set(m.getArgAsInt32(0));
		}
		if (m.getAddress() == "/opticalFlow/strength") {
			ofParameter<float> strength = opticalFlow.parameters.getFloat("strength");
			strength.set(m.getArgAsFloat(0));
			ofLogNotice() << opticalFlow.parameters;
		}
		if (m.getAddress() == "/opticalFlow/offset") {
			ofParameter<int> offset = opticalFlow.parameters.getInt("offset");
			offset.set(m.getArgAsInt32(0));
			ofLogNotice() << opticalFlow.parameters;
		}
		if (m.getAddress() == "/opticalFlow/threshold") {
			ofParameter<float> threshold = opticalFlow.parameters.getFloat("threshold");
			threshold.set(m.getArgAsFloat(0));
			ofLogNotice() << opticalFlow.parameters;
		}
		if (m.getAddress() == "/opticalFlow/lambda") {
			ofParameter<float> lambda = opticalFlow.parameters.getFloat("lambda");
			lambda.set(m.getArgAsFloat(0));
			ofLogNotice() << opticalFlow.parameters;
		}
		if (m.getAddress() == "/opticalFlow/inverseX") {
			ofParameter<bool> inverseX = opticalFlow.parameters.getBool("inverse x");
			inverseX.set(!inverseX.get());
			ofLogNotice() << opticalFlow.parameters;
		}
		if (m.getAddress() == "/opticalFlow/inverseY") {
			ofParameter<bool> inverseY = opticalFlow.parameters.getBool("inverse y");
			inverseY.set(!inverseY.get());
			ofLogNotice() << opticalFlow.parameters;
		}

		if (m.getAddress() == "/opticalFlow/decay") {
			ofParameterGroup blurParams = opticalFlow.parameters.getGroup("time decay blur");
			ofParameter<int> decay = blurParams.getInt("decay");
			// Weird that if we get this as int, it issues warning ..
			// but everything is fine if we assign a float to an int ..
			// ofLogNotice() << m.getArgAsInt32(0);
			decay.set(m.getArgAsFloat(0));
			ofLogNotice() << blurParams;
		}


	}
	// end OSC

	if (camera.isFrameNew()) {
		ofPushStyle();
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);

		cameraFbo.begin();
			camera.draw(0, 0, cameraFbo.getWidth(), cameraFbo.getHeight());
		cameraFbo.end();

		ofPopStyle();

		opticalFlow.setSource(cameraFbo.getTexture());
		// opticalFlow.update(deltaTime);
		// use internal deltatime instead
		opticalFlow.update();

		velocityMask.setDensity(cameraFbo.getTexture());
		velocityMask.setVelocity(opticalFlow.getOpticalFlow());
		velocityMask.update();
	}

	fluidSimulation.addVelocity(opticalFlow.getOpticalFlowDecay());
	fluidSimulation.addDensity(velocityMask.getColorMask());
	fluidSimulation.addTemperature(velocityMask.getLuminanceMask());

	// mouseForces removed
	fluidSimulation.update();

	if (particleFlow.isActive()) {
		particleFlow.setSpeed(fluidSimulation.getSpeed());
		particleFlow.setCellSize(fluidSimulation.getCellSize());
		particleFlow.addFlowVelocity(opticalFlow.getOpticalFlow());
		particleFlow.addFluidVelocity(fluidSimulation.getVelocity());
		// particleFlow.addDensity(fluidSimulation.getDensity());
		particleFlow.setObstacle(fluidSimulation.getObstacle());
	}
	particleFlow.update();

}

//--------------------------------------------------------------
// void ofApp::draw(){
// 	ofBackgroundGradient(100, 0);
// 	// camera.draw(0, 0);
//     videoTexture.draw(0, 0);
//     ofDrawBitmapStringHighlight(msg, 50, 50);

// }

void ofApp::draw(){
	ofClear(0,0);
	if (doDrawCamBackground.get()) {
		drawSource();
	}
	drawComposite();
	// drawOpticalFlow();

	// ofShowCursor();
	// switch(drawMode.get()) {
	// 	case DRAW_COMPOSITE: drawComposite(); break;
	// 	case DRAW_PARTICLES: drawParticles(); break;
	// 	case DRAW_FLUID_FIELDS: drawFluidFields(); break;
	// 	case DRAW_FLUID_DENSITY: drawFluidDensity(); break;
	// 	case DRAW_FLUID_VELOCITY: drawFluidVelocity(); break;
	// 	case DRAW_FLUID_PRESSURE: drawFluidPressure(); break;
	// 	case DRAW_FLUID_TEMPERATURE: drawFluidTemperature(); break;
	// 	case DRAW_FLUID_DIVERGENCE: drawFluidDivergence(); break;
	// 	case DRAW_FLUID_VORTICITY: drawFluidVorticity(); break;
	// 	case DRAW_FLUID_BUOYANCY: drawFluidBuoyance(); break;
	// 	case DRAW_FLUID_OBSTACLE: drawFluidObstacle(); break;
	// 	case DRAW_FLOW_MASK: drawMask(); break;
	// 	case DRAW_OPTICAL_FLOW: drawOpticalFlow(); break;
	// 	case DRAW_SOURCE: drawSource(); break;
	// 	// case DRAW_MOUSE: drawMouseForces(); break;
	// 	case DRAW_VELDOTS: drawVelocityDots(); break;
	// }

	// DEBUG
	ofDrawBitmapStringHighlight(msg, 50, 50);

}

//--------------------------------------------------------------
void ofApp::drawComposite(int _x, int _y, int _width, int _height) {
	ofPushStyle();

	ofEnableBlendMode(OF_BLENDMODE_ADD);
	fluidSimulation.draw(_x, _y, _width, _height);

	ofEnableBlendMode(OF_BLENDMODE_ADD);
	// if (particleFlow.isActive())
	particleFlow.draw(_x, _y, _width, _height);

	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawParticles(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	if (particleFlow.isActive())
		particleFlow.draw(_x, _y, _width, _height);
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidFields(int _x, int _y, int _width, int _height) {
	ofPushStyle();

	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	pressureField.setPressure(fluidSimulation.getPressure());
	pressureField.draw(_x, _y, _width, _height);
	velocityTemperatureField.setVelocity(fluidSimulation.getVelocity());
	velocityTemperatureField.setTemperature(fluidSimulation.getTemperature());
	velocityTemperatureField.draw(_x, _y, _width, _height);
	temperatureField.setTemperature(fluidSimulation.getTemperature());

	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidDensity(int _x, int _y, int _width, int _height) {
	ofPushStyle();

	fluidSimulation.draw(_x, _y, _width, _height);

	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidVelocity(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	if (showScalar.get()) {
		ofClear(0,0);
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	//	ofEnableBlendMode(OF_BLENDMODE_DISABLED); // altenate mode
		displayScalar.setSource(fluidSimulation.getVelocity());
		displayScalar.draw(_x, _y, _width, _height);
	}
	if (showField.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		velocityField.setVelocity(fluidSimulation.getVelocity());
		velocityField.draw(_x, _y, _width, _height);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidPressure(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	ofClear(128);
	if (showScalar.get()) {
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);
		displayScalar.setSource(fluidSimulation.getPressure());
		displayScalar.draw(_x, _y, _width, _height);
	}
	if (showField.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		pressureField.setPressure(fluidSimulation.getPressure());
		pressureField.draw(_x, _y, _width, _height);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidTemperature(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	if (showScalar.get()) {
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);
		displayScalar.setSource(fluidSimulation.getTemperature());
		displayScalar.draw(_x, _y, _width, _height);
	}
	if (showField.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		temperatureField.setTemperature(fluidSimulation.getTemperature());
		temperatureField.draw(_x, _y, _width, _height);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidDivergence(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	if (showScalar.get()) {
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);
		displayScalar.setSource(fluidSimulation.getDivergence());
		displayScalar.draw(_x, _y, _width, _height);
	}
	if (showField.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		temperatureField.setTemperature(fluidSimulation.getDivergence());
		temperatureField.draw(_x, _y, _width, _height);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidVorticity(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	if (showScalar.get()) {
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);
		displayScalar.setSource(fluidSimulation.getConfinement());
		displayScalar.draw(_x, _y, _width, _height);
	}
	if (showField.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		ofSetColor(255, 255, 255, 255);
		velocityField.setVelocity(fluidSimulation.getConfinement());
		velocityField.draw(_x, _y, _width, _height);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidBuoyance(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	if (showScalar.get()) {
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);
		displayScalar.setSource(fluidSimulation.getSmokeBuoyancy());
		displayScalar.draw(_x, _y, _width, _height);
	}
	if (showField.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		velocityField.setVelocity(fluidSimulation.getSmokeBuoyancy());
		velocityField.draw(_x, _y, _width, _height);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidObstacle(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	ofEnableBlendMode(OF_BLENDMODE_DISABLED);
	fluidSimulation.getObstacle().draw(_x, _y, _width, _height);
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawMask(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	ofEnableBlendMode(OF_BLENDMODE_DISABLED);
	velocityMask.draw(_x, _y, _width, _height);
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawOpticalFlow(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	if (showScalar.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		displayScalar.setSource(opticalFlow.getOpticalFlowDecay());
		displayScalar.draw(0, 0, _width, _height);
	}
	if (showField.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		velocityField.setVelocity(opticalFlow.getOpticalFlowDecay());
		velocityField.draw(0, 0, _width, _height);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawSource(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	ofEnableBlendMode(OF_BLENDMODE_DISABLED);
	cameraFbo.draw(_x, _y, _width, _height);
	ofPopStyle();
}

// dots

//--------------------------------------------------------------
void ofApp::drawVelocityDots(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	ofEnableBlendMode(OF_BLENDMODE_ADD);
	velocityDots.setVelocity(fluidSimulation.getVelocity());
	velocityDots.draw(_x, _y, _width, _height);
	ofPopStyle();
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	// ofLogNotice() << "keypressed: " << key;

	switch (key) {
		// case 'G':
		// case 'g': toggleGuiDraw = !toggleGuiDraw; break;
		// case 'f':
		// case 'F': doFullScreen.set(!doFullScreen.get()); break;

		case 'c':
		case 'C':
			// ofLogNotice() << "setting" << !doDrawCamBackground.get();
			doDrawCamBackground.set(!doDrawCamBackground.get());
			break;


		case '1': drawMode.set(DRAW_COMPOSITE); break;
		case '2': drawMode.set(DRAW_FLUID_FIELDS); break;
		case '3': drawMode.set(DRAW_FLUID_VELOCITY); break;
		case '4': drawMode.set(DRAW_FLUID_PRESSURE); break;
		case '5': drawMode.set(DRAW_FLUID_TEMPERATURE); break;
		case '6': drawMode.set(DRAW_OPTICAL_FLOW); break;
		case '7': drawMode.set(DRAW_FLOW_MASK); break;
		case '8': drawMode.set(DRAW_MOUSE); break;

		case 'r':
		case 'R':
			fluidSimulation.reset();
			// fluidSimulation.addObstacle(flowToolsLogoImage.getTexture());
			// mouseForces.reset();
			break;

		default: break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

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
