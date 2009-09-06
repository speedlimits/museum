/*  Sirikata Graphical Object Host
 *  LightBulb.cpp
 *
 *  Created by Ken Turkowski on 9/4/09.
 *  Copyright (c) 2009, Stanford University.
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

	// Create the mesh via the MeshManager
	MeshPtr msh = Ogre::MeshManager::getSingleton().createManual("LightBulb", "General");

	// Create one submesh
	SubMesh* sub = msh->createSubMesh();

    const float size = 15e-2f;  // 15 cm
	const float sqrt13 = 0.577350269f; /* sqrt(1/3) */

	// Define the vertices (8 vertices, each consisting of 2 groups of 3 floats
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

	// Define 12 triangles (two triangles per cube face)
	// The values in this table refer to vertices in the above table
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

	// Create vertex data structure for 8 vertices shared between submeshes
	msh->sharedVertexData = new VertexData();
	msh->sharedVertexData->vertexCount = nVertices;

	// Create declaration (memory format) of vertex data
	VertexDeclaration* decl = msh->sharedVertexData->vertexDeclaration;
	size_t offset = 0;
	// 1st buffer
	decl->addElement(0, offset, VET_FLOAT3, VES_POSITION);
	offset += VertexElement::getTypeSize(VET_FLOAT3);
	decl->addElement(0, offset, VET_FLOAT3, VES_NORMAL);
	offset += VertexElement::getTypeSize(VET_FLOAT3);
	// Allocate vertex buffer of the requested number of vertices (vertexCount)
	// and bytes per vertex (offset)
	HardwareVertexBufferSharedPtr vbuf =
		HardwareBufferManager::getSingleton().createVertexBuffer(
		offset, msh->sharedVertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
	// Upload the vertex data to the card
	vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);

	// Set vertex buffer binding so buffer 0 is bound to our vertex buffer
	VertexBufferBinding* bind = msh->sharedVertexData->vertexBufferBinding;
	bind->setBinding(0, vbuf);

	// 2nd buffer
	offset = 0;
	decl->addElement(1, offset, VET_COLOUR, VES_DIFFUSE);
	offset += VertexElement::getTypeSize(VET_COLOUR);
	// Allocate vertex buffer of the requested number of vertices (vertexCount)
	// and bytes per vertex (offset)
	vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
		offset, msh->sharedVertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
	// Upload the vertex data to the card
	vbuf->writeData(0, vbuf->getSizeInBytes(), colours, true);

	// Set vertex buffer binding so buffer 1 is bound to our colour buffer
	bind->setBinding(1, vbuf);

	// Allocate index buffer of the requested number of vertices (ibufCount)
	HardwareIndexBufferSharedPtr ibuf = HardwareBufferManager::getSingleton().
		createIndexBuffer(
		HardwareIndexBuffer::IT_16BIT,
		ibufCount,
		HardwareBuffer::HBU_STATIC_WRITE_ONLY);

	// Upload the index data to the card
	ibuf->writeData(0, ibuf->getSizeInBytes(), faces, true);

	// Set parameters of the submesh
	sub->useSharedVertices = true;
	sub->indexData->indexBuffer = ibuf;
	sub->indexData->indexCount = ibufCount;
	sub->indexData->indexStart = 0;

	// Set bounding information (for culling)
	msh->_setBounds(AxisAlignedBox(-size, -size, -size, size, size, size));
	msh->_setBoundingSphereRadius(sqrt(3) * size);

	// Notify Mesh object that it has been loaded
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
