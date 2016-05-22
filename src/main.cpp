#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main(){
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofSetupOpenGL(WIDTH, HEIGHT, OF_WINDOW);
	ofRunApp(new ofApp());

}
