/*
 *  LightBulb.cpp
 *  Sirikata
 *
 *  Created by Ken Turkowski on 9/4/09.
 *  Copyright 2009 Google. All rights reserved.
 *
 */

#include <cmath>
#include <OgreCommon.h>
#include <OgreEntity.h>
#include <OgreHardwareBufferManager.h>
#include <OgreHardwareIndexBuffer.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreMaterialManager.h>
#include <OgreMeshManager.h>
#include <OgrePixelFormat.h>
#include <OgreRenderSystem.h>
#include <OgreResourceManager.h>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OgreSubMesh.h>

#include "CameraEntity.hpp"
#include "Entity.hpp"
#include "LightBulb.hpp"
#include "util/Location.hpp"

using Ogre::AxisAlignedBox;
using Ogre::ColourValue;
using Ogre::HardwareBuffer;
using Ogre::HardwareBufferManager;
using Ogre::HardwareIndexBuffer;
using Ogre::HardwareIndexBufferSharedPtr;
using Ogre::HardwareVertexBufferSharedPtr;
using Ogre::MaterialManager;
using Ogre::MaterialPtr;
using Ogre::MeshPtr;
using Ogre::RGBA;
using Ogre::RenderSystem;
using Ogre::ResourceGroupManager;
using Ogre::Root;
using Ogre::SceneNode;
using Ogre::SubMesh;
using Ogre::TVC_AMBIENT;
using Ogre::VES_DIFFUSE;
using Ogre::VES_NORMAL;
using Ogre::VES_POSITION;
using Ogre::VET_COLOUR;
using Ogre::VET_FLOAT3;
using Ogre::VertexBufferBinding;
using Ogre::VertexData;
using Ogre::VertexDeclaration;
using Ogre::VertexElement;

namespace Sirikata {
namespace Graphics {



void LightBulb::initializeMesh()
{   // This is just a cube now

	/// Create the mesh via the MeshManager
	MeshPtr msh = Ogre::MeshManager::getSingleton().createManual("LightBulb", "General");

	/// Create one submesh
	SubMesh* sub = msh->createSubMesh();

    const float size = 15e-2f;  // 15 cm
	const float sqrt13 = 0.577350269f; /* sqrt(1/3) */

	/// Define the vertices (8 vertices, each consisting of 2 groups of 3 floats
	const size_t nVertices = 8;
	const size_t vbufCount = 3*2*nVertices;
	float vertices[vbufCount] = {
        // ---- point--------       ----------- normal ------------
        -size,  +size,  -size,      -sqrt13,    +sqrt13,    -sqrt13,    //0
        +size,  +size,  -size,      +sqrt13,    +sqrt13,    -sqrt13,    //1
        +size,  -size,  -size,      +sqrt13,    -sqrt13,    -sqrt13,    //2
        -size,  -size,  -size,      -sqrt13,    -sqrt13,    -sqrt13,    //3
        -size,  +size,  +size,      -sqrt13,    +sqrt13,    +sqrt13,    //4
        +size,  +size,  +size,      +sqrt13,    +sqrt13,    +sqrt13,    //5
        +size,  -size,  +size,      +sqrt13,    -sqrt13,    +sqrt13,    //6
        -size,  -size,  +size,      -sqrt13,    -sqrt13,    +sqrt13,    //7
	};

	RenderSystem* rs = Root::getSingleton().getRenderSystem();
	RGBA colours[nVertices];
	RGBA *pColour = colours;
	// Use render system to convert colour value since colour packing varies
	rs->convertColourValue(ColourValue(1.0,0.0,0.0), pColour++); //0 colour
	rs->convertColourValue(ColourValue(0.5,1.0,0.0), pColour++); //1 colour
	rs->convertColourValue(ColourValue(1.0,1.0,1.0), pColour++); //2 colour
	rs->convertColourValue(ColourValue(1.0,1.0,1.0), pColour++); //3 colour
	rs->convertColourValue(ColourValue(0.0,1.0,1.0), pColour++); //4 colour
	rs->convertColourValue(ColourValue(0.0,0.5,1.0), pColour++); //5 colour
	rs->convertColourValue(ColourValue(1.0,1.0,1.0), pColour++); //6 colour
	rs->convertColourValue(ColourValue(1.0,1.0,1.0), pColour++); //7 colour

	/// Define 12 triangles (two triangles per cube face)
	/// The values in this table refer to vertices in the above table
	const size_t ibufCount = 36;
	unsigned short faces[ibufCount] = {
			0,2,3,
			0,1,2,
			1,6,2,
			1,5,6,
			4,6,5,
			4,7,6,
			0,7,4,
			0,3,7,
			0,5,1,
			0,4,5,
			2,7,3,
			2,6,7
	};

	/// Create vertex data structure for 8 vertices shared between submeshes
	msh->sharedVertexData = new VertexData();
	msh->sharedVertexData->vertexCount = nVertices;

	/// Create declaration (memory format) of vertex data
	VertexDeclaration* decl = msh->sharedVertexData->vertexDeclaration;
	size_t offset = 0;
	// 1st buffer
	decl->addElement(0, offset, VET_FLOAT3, VES_POSITION);
	offset += VertexElement::getTypeSize(VET_FLOAT3);
	decl->addElement(0, offset, VET_FLOAT3, VES_NORMAL);
	offset += VertexElement::getTypeSize(VET_FLOAT3);
	/// Allocate vertex buffer of the requested number of vertices (vertexCount) 
	/// and bytes per vertex (offset)
	HardwareVertexBufferSharedPtr vbuf = 
		HardwareBufferManager::getSingleton().createVertexBuffer(
		offset, msh->sharedVertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
	/// Upload the vertex data to the card
	vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);

	/// Set vertex buffer binding so buffer 0 is bound to our vertex buffer
	VertexBufferBinding* bind = msh->sharedVertexData->vertexBufferBinding; 
	bind->setBinding(0, vbuf);

	// 2nd buffer
	offset = 0;
	decl->addElement(1, offset, VET_COLOUR, VES_DIFFUSE);
	offset += VertexElement::getTypeSize(VET_COLOUR);
	/// Allocate vertex buffer of the requested number of vertices (vertexCount) 
	/// and bytes per vertex (offset)
	vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
		offset, msh->sharedVertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
	/// Upload the vertex data to the card
	vbuf->writeData(0, vbuf->getSizeInBytes(), colours, true);

	/// Set vertex buffer binding so buffer 1 is bound to our colour buffer
	bind->setBinding(1, vbuf);

	/// Allocate index buffer of the requested number of vertices (ibufCount) 
	HardwareIndexBufferSharedPtr ibuf = HardwareBufferManager::getSingleton().
		createIndexBuffer(
		HardwareIndexBuffer::IT_16BIT, 
		ibufCount, 
		HardwareBuffer::HBU_STATIC_WRITE_ONLY);

	/// Upload the index data to the card
	ibuf->writeData(0, ibuf->getSizeInBytes(), faces, true);

	/// Set parameters of the submesh
	sub->useSharedVertices = true;
	sub->indexData->indexBuffer = ibuf;
	sub->indexData->indexCount = ibufCount;
	sub->indexData->indexStart = 0;

	/// Set bounding information (for culling)
	msh->_setBounds(AxisAlignedBox(-size, -size, -size, size, size, size));
	msh->_setBoundingSphereRadius(sqrt(3) * size);

	/// Notify Mesh object that it has been loaded
	msh->load();
}


void LightBulb::initializeMaterial() {
    MaterialPtr material = MaterialManager::getSingleton().create(
      "Museum/BulbMaterial", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    material->getTechnique(0)->getPass(0)->setVertexColourTracking(Ogre::TVC_EMISSIVE);
}


void LightBulb::AttachLightBulb(
        Sirikata::Graphics::CameraEntity *camera,
        Sirikata::Graphics::Entity *sirikataEntity,
        const char *name, const Location &loc
) {
    initialize();
    Ogre::Camera *ogreCamera = camera->getOgreCamera(); 
    Ogre::SceneManager* sceneManager = ogreCamera->getSceneManager();
    Ogre::Entity* ogreEntity = sceneManager->createEntity(name, "LightBulb");
    ogreEntity->setMaterialName("Museum/BulbMaterial");

#if 0 // FIXME: Is this redundant, or creating two nodes?
    const Vector3d &pos = loc.getTransform().getPosition();
    SceneNode* thisSceneNode = sceneManager->getRootSceneNode()->createChildSceneNode();
    thisSceneNode->setPosition(pos.x, pos.y, pos.z);
#endif
    sirikataEntity->replaceMoveableObject(ogreEntity);
}


LightBulb::LightBulb() {
}


bool LightBulb::inited = false;


} // namespace Graphics
} // namespace Sirikata


#if 0
 <pathorn_mac> oh right--there's a pointer you have to set on each mesh object
 <kturkowski> Yes, I want to click on it and bring up a dialog to set the prameter
 <pathorn_mac> essentially a pointer from ogre's clss back to sirikata's entity
 <pathorn_mac> mOgreObject->setUserAny(Ogre::Any(static_cast<Entity*>(this)));
 <pathorn_mac> there we go
 <pathorn_mac> where ogreobject is of type Ogre::MovableObject
 <rob___> danx0r, any thoughts on jcaceres 's building? he seems like he's got everything and tried a fresh checkout and rebuild to no luck
 <pathorn_mac> I think Ogre::Entity (different from Entity) is a subclass of that
 <pathorn_mac> well jcaceres crash seems related to the scene file
 <danx0r> rob___: we're in private IM debugging
 <kturkowski> ok, but don't I have to insert a Sirikata proxy too?
 <pathorn_mac> in that there's an object with a bad name
 <rob___> ok
 <pathorn_mac> no you don't want a sirikata proxy
 <pathorn_mac> this is purely for GUI purposes
 <pathorn_mac> so we want to leave this strictly in the gui system
 <kturkowski> can I select it?
 <pathorn_mac> well the way selection works
 <pathorn_mac> is mouse click runs a raytrace
 <pathorn_mac> (an ogre raytrace)
 <kturkowski> I want to group a light with a mesh
 <pathorn_mac> which returns a list of Ogre::MovableObject
 <argenex> ok ddm csv is fixed, lights fixed
 <argenex> scene_ddm_septemberfina_0904b_jasonfix.csv
 <pathorn_mac> and then it calls Entity::fromMovableObject(mymovableobj)
 <pathorn_mac> which uses that same Any value you set on it
 <pathorn_mac> so that's why you must call myMovableObject->setUserAny(Ogre::Any(static_cast<Entity*>(mysirikataentity)));
 <pathorn_mac> when you create the Ogre::Entity class

#endif

