#pragma once

#include "ofMain.h"

#include "KinectWrapperLib.h"
#pragma comment (lib, "KinectWrapper.lib") // add path to lib additional dependency dir $(TargetDir)


class SkeletonBone
{
public:
	// lots of constness because we're putting these in a map and that
	// copies stuff all over the place
	const ofQuaternion getCameraRotation() const;
	const ofMatrix4x4 getCameraRotationMatrix() const;
	int getEndJoint() const;
	const ofVec3f& getStartPosition() const;
	const ofVec3f& getScreenPosition() const;
	const ofQuaternion&	getRotation() const;
	const ofMatrix4x4& getRotationMatrix() const;
	const int getStartJoint() const;
	SkeletonBone( const Vector4& inPosition, const _NUI_SKELETON_BONE_ORIENTATION& bone );

private:

	ofMatrix4x4 cameraRotation;
	int	endJoint;
	int	startJoint;
	ofVec3f	position;
	ofMatrix4x4	rotation;
	ofVec2f screenPosition;
};

typedef map<int, SkeletonBone> Skeleton;

class ofxKinect4Windows : protected ofThread {
  public:
	ofxKinect4Windows();

	bool simpleInit();

	// new API
	bool init( int id );
	bool startDepthStream( int width, int height, bool nearMode );
	bool startColorStream( int width, int height );
	bool startIRStream( int width, int height );
	bool startSkeletonStream( bool seated );
	bool start();

  	/// is the current frame new?
	bool isFrameNew();
	bool isFrameNewVideo();
	bool isFrameNewDepth();
	bool isFrameNewSkeleton();

	void setDepthClipping(float nearClip=500, float farClip=4000);
	
	/// updates the pixel buffers and textures
	///
	/// make sure to call this to update to the latest incoming frames
	void update();
	ofPixels & getDepthPixelsRef();       	///< grayscale values
	ofShortPixels & getRawDepthPixelsRef();	///< raw 11 bit values

	/// enable/disable frame loading into textures on update()
	void setUseTexture(bool bUse);

	/// draw the video texture
	void draw(float x, float y, float w, float h);
	void draw(float x, float y);
	void draw(const ofPoint& point);
	void draw(const ofRectangle& rect);

	/// draw the grayscale depth texture
	void drawDepth(float x, float y, float w, float h);
	void drawDepth(float x, float y);
	void drawDepth(const ofPoint& point);
	void drawDepth(const ofRectangle& rect);

	void drawIR( float x, float y, float w, float h );

	vector<Skeleton> getSkeletons();

  private:

	HKINECT hKinect;
	KINECT_IMAGE_FRAME_FORMAT depthFormat;
	KINECT_IMAGE_FRAME_FORMAT colorFormat;
	NUI_SKELETON_FRAME k4wSkeletons;

	bool bVideoIsIR;
  	bool bGrabberInited;
	vector<Skeleton> skeletons;

	//quantize depth buffer to 8 bit range
	vector<unsigned char> depthLookupTable;
	void updateDepthLookupTable();
	void updateDepthPixels();
	void updateIRPixels();
	bool bNearWhite;
	float nearClipping, farClipping;

  	bool bUseTexture;
	ofTexture depthTex; ///< the depth texture
	ofTexture videoTex; ///< the RGB texture
	ofTexture irTex;

	ofPixels videoPixels;
	ofPixels videoPixelsBack;			///< rgb back
	ofPixels depthPixels;
	ofPixels depthPixelsBack;
	ofShortPixels depthPixelsRaw;
	ofShortPixels depthPixelsRawBack;	///< depth back

	ofShortPixels irPixelsRaw;
	ofShortPixels irPixelsBackRaw;
	ofPixels irPixels;
	ofPixels irPixelsBack;

	bool bIsFrameNewVideo;
	bool bNeedsUpdateVideo;
	bool bIsFrameNewDepth;
	bool bNeedsUpdateDepth;
	bool bNeedsUpdateSkeleton;
	bool bIsSkeletonFrameNew;

	bool bIsVideoInfrared;
	void threadedFunction();

};