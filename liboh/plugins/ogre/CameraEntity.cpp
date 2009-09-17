/*  Sirikata Graphical Object Host
 *  CameraEntity.cpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <oh/Platform.hpp>
#include "CameraEntity.hpp"
#include <options/Options.hpp>

#include <Ogre.h>
#include <OgreFrameListener.h>
namespace Sirikata {
namespace Graphics {
	class HDRListener: public Ogre::CompositorInstance::Listener
	{
	protected:
		int mVpWidth, mVpHeight;
		int mBloomSize;
		// Array params - have to pack in groups of 4 since this is how Cg generates them
		// also prevents dependent texture read problems if ops don't require swizzle
		float mBloomTexWeights[15][4];
		float mBloomTexOffsetsHorz[15][4];
		float mBloomTexOffsetsVert[15][4];
	public:
		HDRListener();
		virtual ~HDRListener();
		void notifyViewportSize(int width, int height);
		void notifyCompositor(Ogre::CompositorInstance* instance);
		virtual void notifyMaterialSetup(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat);
		virtual void notifyMaterialRender(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat);
	};

	/*************************************************************************
	HDRListener Methods
	*************************************************************************/
	//---------------------------------------------------------------------------
	HDRListener::HDRListener()
	{
	}
	//---------------------------------------------------------------------------
	HDRListener::~HDRListener()
	{
	}
	//---------------------------------------------------------------------------
	void HDRListener::notifyViewportSize(int width, int height)
	{
		mVpWidth = width;
		mVpHeight = height;
	}
	//---------------------------------------------------------------------------
	void HDRListener::notifyCompositor(Ogre::CompositorInstance* instance)
	{
		// Get some RTT dimensions for later calculations
		Ogre::CompositionTechnique::TextureDefinitionIterator defIter =
			instance->getTechnique()->getTextureDefinitionIterator();
		while (defIter.hasMoreElements())
		{
			Ogre::CompositionTechnique::TextureDefinition* def =
				defIter.getNext();
			if(def->name == "rt_bloom0")
			{
				mBloomSize = (int)def->width; // should be square
				// Calculate gaussian texture offsets & weights
				float deviation = 3.0f;
				float texelSize = 1.0f / (float)mBloomSize;

				// central sample, no offset
				mBloomTexOffsetsHorz[0][0] = 0.0f;
				mBloomTexOffsetsHorz[0][1] = 0.0f;
				mBloomTexOffsetsVert[0][0] = 0.0f;
				mBloomTexOffsetsVert[0][1] = 0.0f;
				mBloomTexWeights[0][0] = mBloomTexWeights[0][1] =
					mBloomTexWeights[0][2] = Ogre::Math::gaussianDistribution(0, 0, deviation);
				mBloomTexWeights[0][3] = 1.0f;

				// 'pre' samples
				for(int i = 1; i < 8; ++i)
				{
					mBloomTexWeights[i][0] = mBloomTexWeights[i][1] =
						mBloomTexWeights[i][2] = 1.25f * Ogre::Math::gaussianDistribution(i, 0, deviation);
					mBloomTexWeights[i][3] = 1.0f;
					mBloomTexOffsetsHorz[i][0] = i * texelSize;
					mBloomTexOffsetsHorz[i][1] = 0.0f;
					mBloomTexOffsetsVert[i][0] = 0.0f;
					mBloomTexOffsetsVert[i][1] = i * texelSize;
				}
				// 'post' samples
				for(int i = 8; i < 15; ++i)
				{
					mBloomTexWeights[i][0] = mBloomTexWeights[i][1] =
						mBloomTexWeights[i][2] = mBloomTexWeights[i - 7][0];
					mBloomTexWeights[i][3] = 1.0f;

					mBloomTexOffsetsHorz[i][0] = -mBloomTexOffsetsHorz[i - 7][0];
					mBloomTexOffsetsHorz[i][1] = 0.0f;
					mBloomTexOffsetsVert[i][0] = 0.0f;
					mBloomTexOffsetsVert[i][1] = -mBloomTexOffsetsVert[i - 7][1];
				}

			}
		}
	}
	//---------------------------------------------------------------------------
	void HDRListener::notifyMaterialSetup(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat)
	{
		// Prepare the fragment params offsets
		switch(pass_id)
		{
		//case 994: // rt_lum4
		case 993: // rt_lum3
		case 992: // rt_lum2
		case 991: // rt_lum1
		case 990: // rt_lum0
			break;
		case 800: // rt_brightpass
			break;
		case 701: // rt_bloom1
			{
				// horizontal bloom
				mat->load();
				Ogre::GpuProgramParametersSharedPtr fparams =
					mat->getBestTechnique()->getPass(0)->getFragmentProgramParameters();
				const Ogre::String& progName = mat->getBestTechnique()->getPass(0)->getFragmentProgramName();
				fparams->setNamedConstant("sampleOffsets", mBloomTexOffsetsHorz[0], 15);
				fparams->setNamedConstant("sampleWeights", mBloomTexWeights[0], 15);

				break;
			}
		case 700: // rt_bloom0
			{
				// vertical bloom
				mat->load();
				Ogre::GpuProgramParametersSharedPtr fparams =
					mat->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
				const Ogre::String& progName = mat->getBestTechnique()->getPass(0)->getFragmentProgramName();
				fparams->setNamedConstant("sampleOffsets", mBloomTexOffsetsVert[0], 15);
				fparams->setNamedConstant("sampleWeights", mBloomTexWeights[0], 15);

				break;
			}
		}
	}
	//---------------------------------------------------------------------------
	void HDRListener::notifyMaterialRender(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat)
	{
	}

static void ProcessCompositorEffects(OgreSystem *root, Ogre::Viewport*vp) {
    static HDRListener hdrListener;
    String allCompositors=root->getCompositors();
    std::string::size_type where=0;
    do {
        std::string::size_type where2=allCompositors.find_first_of(",",where+1);
        std::string nam;
        if (where2!=std::string::npos)
            nam=allCompositors.substr(where,where2-where);
        else
            nam=allCompositors.substr(where);
        where=where2;
        if (nam.length()) {
            Ogre::CompositorInstance *instance = Ogre::CompositorManager::getSingleton().addCompositor(vp, nam);
            Ogre::CompositorManager::getSingleton().setCompositorEnabled(vp,nam,true);
            if (nam=="HDR") {
                instance->addListener(&hdrListener);
                hdrListener.notifyViewportSize(vp->getActualWidth(),vp->getActualHeight());
                hdrListener.notifyCompositor(instance);
            }
        }
    }while (where!=std::string::npos);
}

CameraEntity::CameraEntity(OgreSystem *scene,
                           const std::tr1::shared_ptr<ProxyCameraObject> &pco,
                           std::string ogreName)
    : Entity(scene,
             pco,
             ogreName.length()?ogreName:ogreName=ogreCameraName(pco->getObjectReference()),
             NULL),
      mRenderTarget(NULL),
      mViewport(NULL) {
    mAttachedIter=scene->mAttachedCameras.end();
    getProxy().CameraProvider::addListener(this);
    String cameraName = ogreName;
    if (scene->getSceneManager()->hasCamera(cameraName)) {
        init(scene->getSceneManager()->getCamera(cameraName));
    } else {
        init(scene->getSceneManager()->createCamera(cameraName));
    }
    getOgreCamera()->setNearClipDistance(scene->getOptions()->referenceOption("nearplane")->as<float32>());
    getOgreCamera()->setFarClipDistance(scene->getOptions()->referenceOption("farplane")->as<float32>());
}

void CameraEntity::attach (const String&renderTargetName,
                     uint32 width,
                     uint32 height){
    this->detach();
    mRenderTarget = mScene->createRenderTarget(renderTargetName,
                                               width,
                                               height);
    mViewport= mRenderTarget->addViewport(getOgreCamera());
    mViewport->setBackgroundColour(Ogre::ColourValue(0,.125,.25,1));
    ProcessCompositorEffects(mScene,mViewport);
    getOgreCamera()->setAspectRatio((float32)mViewport->getActualWidth()/(float32)mViewport->getActualHeight());
    mAttachedIter = mScene->attachCamera(renderTargetName,this);
}
void CameraEntity::detach() {
    if (mViewport&&mRenderTarget) {
        mRenderTarget->removeViewport(mViewport->getZOrder());
/*
  unsigned int numViewports=sm->getNumViewports();
  for (unsigned int i=0;i<numViewports;++i){
  if (sm->getViewport(i)==mViewport) {
  sm->removeViewport(i);
  break;
  }
  }
*/
    }else {
        assert(!mViewport);
    }
    if (mRenderTarget) {
        mScene->destroyRenderTarget(mRenderTarget->getName());
        mRenderTarget=NULL;
    }
    mAttachedIter=mScene->detachCamera(mAttachedIter);

}

CameraEntity::~CameraEntity() {
    if ((!mViewport) || (mViewport && mRenderTarget)) {
        detach();
    }
    if (mAttachedIter != mScene->mAttachedCameras.end()) {
        mScene->mAttachedCameras.erase(mAttachedIter);
    }
    Ogre::Camera*toDestroy=getOgreCamera();
    init(NULL);
    mScene->getSceneManager()->destroyCamera(toDestroy);
    getProxy().CameraProvider::removeListener(this);
}
std::string CameraEntity::ogreCameraName(const SpaceObjectReference&ref) {
    return "Camera:"+ref.toString();
}
std::string CameraEntity::ogreMovableName()const{
    return ogreCameraName(id());
}

}
}
