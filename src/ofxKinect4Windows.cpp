#include "ofxKinect4Windows.h"

SkeletonBone::SkeletonBone ( const Vector4& inPosition, const _NUI_SKELETON_BONE_ORIENTATION& orient) {

	/*mAbsRotQuat	= toQuatf( bone.absoluteRotation.rotationQuaternion );
	mAbsRotMat	= toMatrix44f( bone.absoluteRotation.rotationMatrix );
	mJointEnd	= bone.endJoint;
	mJointStart	= bone.startJoint;
	mPosition	= toVec3f( position );
	mRotQuat	= toQuatf( bone.hierarchicalRotation.rotationQuaternion );
	mRotMat		= toMatrix44f( bone.hierarchicalRotation.rotationMatrix );*/

	cameraRotation.set( orient.absoluteRotation.rotationMatrix.M11, orient.absoluteRotation.rotationMatrix.M12, orient.absoluteRotation.rotationMatrix.M13, orient.absoluteRotation.rotationMatrix.M14,
		orient.absoluteRotation.rotationMatrix.M21, orient.absoluteRotation.rotationMatrix.M22, orient.absoluteRotation.rotationMatrix.M23, orient.absoluteRotation.rotationMatrix.M24,
		orient.absoluteRotation.rotationMatrix.M31, orient.absoluteRotation.rotationMatrix.M32, orient.absoluteRotation.rotationMatrix.M33, orient.absoluteRotation.rotationMatrix.M34,
		orient.absoluteRotation.rotationMatrix.M41, orient.absoluteRotation.rotationMatrix.M42, orient.absoluteRotation.rotationMatrix.M43, orient.absoluteRotation.rotationMatrix.M44);

	//NUI_SKELETON_BONE_ROTATION
	//cout << inPosition.x << " " << inPosition.y << " " << inPosition.z << endl;
	position.set( inPosition.x, inPosition.y, inPosition.z );

	NuiTransformSkeletonToDepthImage(inPosition, &(screenPosition.x), &(screenPosition.y), NUI_IMAGE_RESOLUTION_640x480);

	rotation.set( orient.hierarchicalRotation.rotationMatrix.M11, orient.hierarchicalRotation.rotationMatrix.M12, orient.hierarchicalRotation.rotationMatrix.M13, orient.hierarchicalRotation.rotationMatrix.M14,
		orient.hierarchicalRotation.rotationMatrix.M21, orient.hierarchicalRotation.rotationMatrix.M22, orient.hierarchicalRotation.rotationMatrix.M23, orient.hierarchicalRotation.rotationMatrix.M24,
		orient.hierarchicalRotation.rotationMatrix.M31, orient.hierarchicalRotation.rotationMatrix.M32, orient.hierarchicalRotation.rotationMatrix.M33, orient.hierarchicalRotation.rotationMatrix.M34,
		orient.hierarchicalRotation.rotationMatrix.M41, orient.hierarchicalRotation.rotationMatrix.M42, orient.hierarchicalRotation.rotationMatrix.M43, orient.hierarchicalRotation.rotationMatrix.M44);
	
}

const ofVec3f& SkeletonBone::getStartPosition() const {
		return position;
	}

	//! Returns rotation of the bone relative to the parent bone.
	const ofQuaternion&	SkeletonBone::getRotation() const {
		return rotation.getRotate();
	}
	//! Returns rotation matrix of the bone relative to the parent bone.
	const ofMatrix4x4& SkeletonBone::getRotationMatrix() const {
		return rotation;
	}
	//! Returns index of start joint.
	const int SkeletonBone::getStartJoint() const {
		return startJoint;
	}

	const ofQuaternion SkeletonBone::getCameraRotation() const		{
			return cameraRotation.getRotate();
	}

	const ofMatrix4x4 SkeletonBone::getCameraRotationMatrix() const {
		return rotation;
	}

	int SkeletonBone::getEndJoint() const {
		return endJoint;
	}

	const ofVec3f& SkeletonBone::getScreenPosition() const
	{
		return screenPosition;
	}



ofxKinect4Windows::ofxKinect4Windows(){
	bIsFrameNewVideo = false;
	bNeedsUpdateVideo = false;
	bIsFrameNewDepth = false;
	bNeedsUpdateDepth = false;

	bIsVideoInfrared = false;

	bGrabberInited = false;

  	bUseTexture = true;
	
	setDepthClipping();
}

//---------------------------------------------------------------------------
void ofxKinect4Windows::setDepthClipping(float nearClip, float farClip){
	nearClipping = nearClip;
	farClipping = farClip;
	updateDepthLookupTable();
}

//---------------------------------------------------------------------------
void ofxKinect4Windows::updateDepthLookupTable() {
	unsigned char nearColor = bNearWhite ? 255 : 0;
	unsigned char farColor = bNearWhite ? 0 : 255;
	unsigned int maxDepthLevels = 10001;
	depthLookupTable.resize(maxDepthLevels);
	depthLookupTable[0] = 0;
	for(unsigned int i = 1; i < maxDepthLevels; i++) {
		depthLookupTable[i] = ofMap(i, nearClipping, farClipping, nearColor, farColor, true);
	}
}

bool ofxKinect4Windows::simpleInit()
{
	hKinect = KinectOpenDefaultSensor();

	KINECT_IMAGE_FRAME_FORMAT cf = { sizeof(KINECT_IMAGE_FRAME_FORMAT), 0 };
	
	if( SUCCEEDED(KinectEnableColorStream(hKinect, NUI_IMAGE_RESOLUTION_640x480, &cf)) )
	{
		colorFormat = cf;
		videoPixels.allocate(colorFormat.dwWidth, colorFormat.dwHeight,OF_IMAGE_COLOR_ALPHA);
		videoPixelsBack.allocate(colorFormat.dwWidth, colorFormat.dwHeight,OF_IMAGE_COLOR_ALPHA);
		if(bUseTexture){
			videoTex.allocate(colorFormat.dwWidth, colorFormat.dwHeight, GL_RGBA);
		}
	}
	else{
		ofLogError("ofxKinect4Windows::open") << "Error opening color stream";
		return false;
	}

	KINECT_IMAGE_FRAME_FORMAT df = { sizeof(KINECT_IMAGE_FRAME_FORMAT), 0 };

	if( SUCCEEDED( KinectEnableDepthStream(hKinect, 0, NUI_IMAGE_RESOLUTION_640x480, &df) ) ){
		depthFormat = df;
		depthPixels.allocate(depthFormat.dwWidth, depthFormat.dwHeight, OF_IMAGE_GRAYSCALE);
		depthPixelsBack.allocate(depthFormat.dwWidth, depthFormat.dwHeight, OF_IMAGE_GRAYSCALE);
		depthPixelsRaw.allocate(depthFormat.dwWidth, depthFormat.dwHeight, OF_IMAGE_GRAYSCALE);
		depthPixelsRawBack.allocate(depthFormat.dwWidth, depthFormat.dwHeight, OF_IMAGE_GRAYSCALE);
		if(bUseTexture){
			depthTex.allocate(depthFormat.dwWidth, depthFormat.dwHeight, GL_LUMINANCE);
		}
	} 
	else{
		ofLogError("ofxKinect4Windows::open") << "Error opening depth stream";
		return false;
	}

	startThread(true, false);
	bGrabberInited = true;
	return true;
}

/// is the current frame new?
bool ofxKinect4Windows::isFrameNew(){
	return isFrameNewVideo() || isFrameNewDepth();
}

bool ofxKinect4Windows::isFrameNewVideo(){
	return bIsFrameNewVideo;
}

bool ofxKinect4Windows::isFrameNewDepth(){
	return bIsFrameNewDepth;
}

vector<Skeleton> ofxKinect4Windows::getSkeletons() 
{
	return skeletons;
}

/// updates the pixel buffers and textures
///
/// make sure to call this to update to the latest incoming frames
void ofxKinect4Windows::update(){
	if(!bGrabberInited) {
		return;
	}

	if(bNeedsUpdateVideo){
		bIsFrameNewVideo = true;
		if(bVideoIsIR)
		{
			swap(irPixels, irPixelsBack);
			bNeedsUpdateVideo = false;
			//updateIRPixels();
		}
		else
		{
			swap(videoPixels,videoPixelsBack);
			bNeedsUpdateVideo = false;
		}
		if(bUseTexture) {
			if(bVideoIsIR) 
			{
				irTex.loadData(irPixels.getPixels(), colorFormat.dwWidth, colorFormat.dwHeight, GL_LUMINANCE);
			} 
			else 
			{
				videoTex.loadData(videoPixels.getPixels(), colorFormat.dwWidth, colorFormat.dwHeight, GL_RGBA);
			}
		}
	} else {
		bIsFrameNewVideo = false;
	}

	if(bNeedsUpdateDepth){
		bIsFrameNewDepth = true;
//		if(this->lock()) {
			swap(depthPixelsRaw, depthPixelsRawBack);
			bNeedsUpdateDepth = false;
//			this->unlock();
			updateDepthPixels();
//		}

		if(bUseTexture) {
			depthTex.loadData(depthPixels.getPixels(), depthFormat.dwWidth, depthFormat.dwHeight, GL_LUMINANCE);
		}

	} else {
		bIsFrameNewDepth = false;
	}

	if(bNeedsUpdateSkeleton)
	{	

		bIsSkeletonFrameNew = true;
		bNeedsUpdateSkeleton = false;
		bool foundSkeleton = false;

		// delete every time?
		//skeletons.clear();

		for ( int i = 0; i < NUI_SKELETON_COUNT; i++ ) 
		{
			skeletons.at( i ).clear();
			if (  k4wSkeletons.SkeletonData[ i ].eTrackingState == NUI_SKELETON_TRACKED || k4wSkeletons.SkeletonData[ i ].eTrackingState == NUI_SKELETON_POSITION_ONLY ) {
				//cout << " we have a skeleton " << ofGetElapsedTimeMillis() << endl;
				_NUI_SKELETON_BONE_ORIENTATION bones[ NUI_SKELETON_POSITION_COUNT ];
				if(SUCCEEDED(NuiSkeletonCalculateBoneOrientations( &(k4wSkeletons.SkeletonData[i]), bones ))) {
					//error( hr );
				}

				for ( int j = 0; j < NUI_SKELETON_POSITION_COUNT; j++ ) 
				{
					SkeletonBone bone( k4wSkeletons.SkeletonData[i].SkeletonPositions[j], bones[j] );
					( skeletons.begin())->insert( std::pair<int, SkeletonBone>( j, bone ) );
				}

			}

		}

	} /*else {
		bNeedsUpdateSkeleton = false;
	}*/
}

//----------------------------------------------------------
void ofxKinect4Windows::updateDepthPixels() {
	for(int i = 0; i < depthPixels.getWidth()*depthPixels.getHeight(); i++) {
		//if( i == 160*120) cout << depthPixelsRaw[i] << endl;
		//depthPixels[i] = depthLookupTable[ ofClamp(depthPixelsRaw[i], 0, depthLookupTable.size()-1) ];
		depthPixels[i] = ofClamp(depthPixelsRaw[i] >> 7, 0, depthLookupTable.size()-1);
	}
}

void ofxKinect4Windows::updateIRPixels() {
	for(int i = 0; i < irPixels.getWidth()*irPixels.getHeight(); i++) {
		irPixels[i] =  ofClamp(irPixels[i] >> 1, 0, 255 );
	}
}

//------------------------------------
ofPixels & ofxKinect4Windows::getDepthPixelsRef(){       	///< grayscale values
	return depthPixels;
}

//------------------------------------
ofShortPixels & ofxKinect4Windows::getRawDepthPixelsRef(){
	return depthPixelsRaw;
}

//------------------------------------
void ofxKinect4Windows::setUseTexture(bool bUse){
	bUseTexture = bUse;
}

//----------------------------------------------------------
void ofxKinect4Windows::draw(float _x, float _y, float _w, float _h) {
	if(bUseTexture) {
		videoTex.draw(_x, _y, _w, _h);
	}
}

//----------------------------------------------------------
void ofxKinect4Windows::draw(float _x, float _y) {
	draw(_x, _y, (float)colorFormat.dwWidth, (float)colorFormat.dwHeight);
}

//----------------------------------------------------------
void ofxKinect4Windows::draw(const ofPoint & point) {
	draw(point.x, point.y);
}

//----------------------------------------------------------
void ofxKinect4Windows::draw(const ofRectangle & rect) {
	draw(rect.x, rect.y, rect.width, rect.height);
}

//----------------------------------------------------------
void ofxKinect4Windows::drawDepth(float _x, float _y, float _w, float _h) {
	if(bUseTexture) {
		depthTex.draw(_x, _y, _w, _h);
	}
}

//---------------------------------------------------------------------------
void ofxKinect4Windows::drawDepth(float _x, float _y) {
	drawDepth(_x, _y, (float)depthFormat.dwWidth, (float)depthFormat.dwHeight);
}

//----------------------------------------------------------
void ofxKinect4Windows::drawDepth(const ofPoint & point) {
	drawDepth(point.x, point.y);
}

//----------------------------------------------------------
void ofxKinect4Windows::drawDepth(const ofRectangle & rect) {
	drawDepth(rect.x, rect.y, rect.width, rect.height);
}

void ofxKinect4Windows::drawIR(float _x, float _y, float _w, float _h) {
	if(bUseTexture) {
		irTex.draw(_x, _y, _w, _h);
	}
}

bool ofxKinect4Windows::init( int id )
{
	UINT count = KinectGetSensorCount();
	WCHAR portID[KINECT_MAX_PORTID_LENGTH];

	if( SUCCEEDED(KinectGetPortIDByIndex( 0, _countof(portID), portID ))) {
		
	} else {
		ofLog() << " can't find kinect of ID " << id << endl;
		return false;
	}

	hKinect = KinectOpenSensor(portID);

	if(!hKinect) {
		ofLog() << " can't open kinect of ID " << id << endl;
		return false;
	}

	return true;
}

bool ofxKinect4Windows::startDepthStream( int width, int height, bool nearMode )
{

	_NUI_IMAGE_RESOLUTION res;
	if( width == 320 ) {
		res = NUI_IMAGE_RESOLUTION_320x240;
	} else if( width == 640 ) {
		res = NUI_IMAGE_RESOLUTION_640x480;
	} else {
		ofLog() << " invalid image size passed to startDepthStream() " << endl;
	}

	KINECT_IMAGE_FRAME_FORMAT df = { sizeof(KINECT_IMAGE_FRAME_FORMAT), 0 };

	if( SUCCEEDED( KinectEnableDepthStream(hKinect, nearMode, res, &df) ) ){
//		pDepthBuffer = new BYTE[depthFormat.cbBufferSize];
		depthFormat = df;
		ofLog() << "allocating a buffer of size " << depthFormat.dwWidth*depthFormat.dwHeight*sizeof(unsigned short) << " when k4w wants size " << depthFormat.cbBufferSize << endl;
		depthPixels.allocate(depthFormat.dwWidth, depthFormat.dwHeight, OF_IMAGE_GRAYSCALE);
		depthPixelsBack.allocate(depthFormat.dwWidth, depthFormat.dwHeight, OF_IMAGE_GRAYSCALE);
		depthPixelsRaw.allocate(depthFormat.dwWidth, depthFormat.dwHeight, OF_IMAGE_GRAYSCALE);
		depthPixelsRawBack.allocate(depthFormat.dwWidth, depthFormat.dwHeight, OF_IMAGE_GRAYSCALE);
		if(bUseTexture){
			depthTex.allocate(depthFormat.dwWidth, depthFormat.dwHeight, GL_LUMINANCE);
		}
	} 
	else{
		ofLogError("ofxKinect4Windows::open") << "Error opening depth stream";
		return false;
	}
	return true;
}

bool ofxKinect4Windows::startColorStream( int width, int height )
{
	KINECT_IMAGE_FRAME_FORMAT cf = { sizeof(KINECT_IMAGE_FRAME_FORMAT), 0 };

	_NUI_IMAGE_RESOLUTION res;
	if( width == 320 ) {
		res = NUI_IMAGE_RESOLUTION_320x240;
	} else if( width == 640 ) {
		res = NUI_IMAGE_RESOLUTION_640x480;
	} else if( width == 1280 ) {
		res = NUI_IMAGE_RESOLUTION_1280x960;
	} else {
		ofLog() << " invalid image size passed to startColorStream() " << endl;
	}

	if( SUCCEEDED(KinectEnableColorStream(hKinect, res, &cf)) )
	{
		//BYTE* pColorBuffer = new BYTE[format.cbBufferSize];
		colorFormat = cf;
		ofLog() << "allocating a buffer of size " << colorFormat.dwWidth*colorFormat.dwHeight*sizeof(unsigned char)*4 << " when k4w wants size " << colorFormat.cbBufferSize << endl;
		videoPixels.allocate(colorFormat.dwWidth, colorFormat.dwHeight,OF_IMAGE_COLOR_ALPHA);
		videoPixelsBack.allocate(colorFormat.dwWidth, colorFormat.dwHeight,OF_IMAGE_COLOR_ALPHA);
		if(bUseTexture){
			videoTex.allocate(colorFormat.dwWidth, colorFormat.dwHeight, GL_RGBA);
		}
	}
	else{
		ofLogError("ofxKinect4Windows::open") << "Error opening color stream";
		return false;
	}
	return true;
}

bool ofxKinect4Windows::startIRStream( int width, int height )
{
		_NUI_IMAGE_RESOLUTION res;
	if( width == 320 ) {
		res = NUI_IMAGE_RESOLUTION_320x240;
	} else if( width == 640 ) {
		res = NUI_IMAGE_RESOLUTION_640x480;
	} else if( width == 1280 ) {
		res = NUI_IMAGE_RESOLUTION_1280x960;
	} else {
		ofLog() << " invalid image size passed to startIRStream() " << endl;
	}

	KINECT_IMAGE_FRAME_FORMAT cf = { sizeof(KINECT_IMAGE_FRAME_FORMAT), 0 };


	if( SUCCEEDED(KinectEnableIRStream(hKinect, res, &cf)) )
	{
		//BYTE* pColorBuffer = new BYTE[format.cbBufferSize];
		colorFormat = cf;
		//cout << "allocating a buffer of size " << colorFormat.dwWidth*colorFormat.dwHeight*sizeof(unsigned char)*4 << " when k4w wants size " << colorFormat.cbBufferSize << endl;
		
		cout << colorFormat.dwWidth << " " << colorFormat.dwHeight << endl;
		
		irPixels.allocate(colorFormat.dwWidth, colorFormat.dwHeight, OF_IMAGE_GRAYSCALE);
		irPixelsBack.allocate(colorFormat.dwWidth, colorFormat.dwHeight,OF_IMAGE_GRAYSCALE);
		if(bUseTexture){
			irTex.allocate(colorFormat.dwWidth, colorFormat.dwHeight, GL_LUMINANCE);
		}
	}
	else{
		ofLogError("ofxKinect4Windows::open") << "Error opening color stream";
		return false;
	}
	return true;
}

bool ofxKinect4Windows::startSkeletonStream( bool seated )
{
	NUI_TRANSFORM_SMOOTH_PARAMETERS p = { 0.5f, 0.1f, 0.5f, 0.1f, 0.1f };
	if(SUCCEEDED( KinectEnableSkeletalStream( hKinect, seated, SkeletonSelectionModeDefault, &p))) {
		//cout << " we have skeletons " << endl;

		//vector<Skeleton>::iterator skelIt = skeletons.begin();
		for( int i = 0; i < NUI_SKELETON_COUNT; i++ )  {

			Skeleton s;
			skeletons.push_back(s);

		}

		return true;
	} else {
		ofLog() << " startSkeletonStream() cannot initialize stream " << endl;
		return false;
	}
	return true;
}

bool ofxKinect4Windows::start()
{
	startThread(true, false);
	bGrabberInited = true;
	return true;
}


//----------------------------------------------------------
void ofxKinect4Windows::threadedFunction(){
	bool alignToDepth = false;
	LONGLONG timestamp;
	
	cout << "STARTING THREAD" << endl;

	//JG: DO WE NEED TO BAIL ON THIS LOOP IF THE DEVICE QUITS? 
	//how can we tell?
	while(isThreadRunning()) {
		alignToDepth = !alignToDepth; // flip depth alignment every get - strobing
		//KinectGetColorFrame( _In_ HKINECT hKinect, ULONG cbBufferSize, _Out_cap_(cbBufferSize) BYTE* pColorBuffer, _Out_opt_ LONGLONG* liTimeStamp );
        if( SUCCEEDED( KinectGetDepthFrame(hKinect, depthFormat.cbBufferSize, (BYTE*)depthPixelsRawBack.getPixels(), &timestamp) ) )
		{
			bNeedsUpdateDepth = true;
			//printf("depth Timestamp: %d\r\n", timestamp);
        }

		// KinectGetDepthFrame( _In_ HKINECT hKinect, ULONG cbBufferSize, _Out_cap_(cbBufferSize) BYTE* pDepthBuffer, _Out_opt_ LONGLONG* liTimeStamp );

		if(bVideoIsIR) 
		{
			if( SUCCEEDED( KinectGetColorFrame(hKinect, colorFormat.cbBufferSize, irPixelsBack.getPixels(), &timestamp) ) )
			{
				bNeedsUpdateVideo = true;
				// ProcessColorFrameData(&format, pColorBuffer);
				//printf("Video Timestamp: %d\r\n", timestamp);
			}
		}
		else {
			if( SUCCEEDED( KinectGetColorFrame(hKinect, colorFormat.cbBufferSize, videoPixelsBack.getPixels(), &timestamp) ) )
			{
				bNeedsUpdateVideo = true;
				// ProcessColorFrameData(&format, pColorBuffer);
				//printf("Video Timestamp: %d\r\n", timestamp);
			}
		}

		if( SUCCEEDED ( KinectGetSkeletonFrame(hKinect, &k4wSkeletons )) ) 
		//if( SUCCEEDED ( KinectGetSkeletonDataAlignedToColor( hKinect, sizeof(NUI_SKELETON_DATA) * 6, &k4wSkeletons[0], &timestamp) ))
		{
			//cout << " skeletonia timestamp " << timestamp << endl;
			bNeedsUpdateSkeleton = true;
		}

		//TODO: TILT
		//TODO: ACCEL
		//TODO: SKELETON
		//TODO: FACE
		//TODO: AUDIO
		ofSleepMillis(10);
	}
}