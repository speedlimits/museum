/*  Sirikata liboh -- Bullet Graphics Plugin
 *  BulletSystem.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn & Daniel Braxton Miller
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

#include <fstream>
#include <oh/Platform.hpp>
#include <oh/SimulationFactory.hpp>
#include <oh/ProxyObject.hpp>
#include <oh/SpaceTimeOffsetManager.hpp>
#include <options/Options.hpp>
#include <transfer/TransferManager.hpp>
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletSystem.hpp"
#include "Bullet_Sirikata.pbj.hpp"
#include "Bullet_Physics.pbj.hpp"
#include "util/RoutableMessageBody.hpp"
#include "util/RoutableMessageHeader.hpp"
#include "util/KnownServices.hpp"
using namespace std;
using std::tr1::placeholders::_1;
static int core_plugin_refcount = 0;

    //#define DEBUG_OUTPUT(x) x
#define DEBUG_OUTPUT(x)

#define DEBUG_ALWAYS(x) x

std::string gWidth="1360";
std::string gHeight="768";
std::string gFullscreen="true";
std::string gMode="";
bool gDebug=false;

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    DEBUG_OUTPUT(cout << "dbm: plugin init" << endl;)
    if (core_plugin_refcount==0)
        SimulationFactory::getSingleton().registerConstructor("bulletphysics",
                &BulletSystem::create,
                true);
    core_plugin_refcount++;
    DEBUG_OUTPUT(cout << "dbm: plugin init return" << endl;)
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++core_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(core_plugin_refcount>0);
    return --core_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (core_plugin_refcount>0) {
        core_plugin_refcount--;
        assert(core_plugin_refcount==0);
        if (core_plugin_refcount==0)
            SimulationFactory::getSingleton().unregisterConstructor("bulletphysics",true);
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "bulletphysics";
}
SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return core_plugin_refcount;
}

namespace Sirikata {

const ObjectReference&BulletObj::getObjectReference()const {
    return mMeshptr->getObjectReference().object();
}
const SpaceID&BulletObj::getSpaceID()const {
    return mMeshptr->getObjectReference().space();
}

void BulletObj::meshChanged (const URI &newMesh) {
    DEBUG_OUTPUT(cout << "dbm:    meshlistener: " << newMesh << endl;)
    mMeshname = newMesh;
}

void BulletObj::setPhysical (const PhysicalParameters &pp) {
    DEBUG_OUTPUT(cout << "dbm: setPhysical: " << this << " mode=" << pp.mode << " name: " << pp.name << " mesh: " << mMeshname << endl);
    mName = pp.name;
    mHull = pp.hull;
    mGravity = system->getGravity() * pp.gravity;
    colMask = pp.colMask;
    colMsg = pp.colMsg;
    switch (pp.mode) {
    case PhysicalParameters::Disabled:
        DEBUG_OUTPUT(cout << "  dbm: debug setPhysical: Disabled" << endl);
        mActive = false;
        mMeshptr->setLocationAuthority(0);
        mDynamic = false;
        break;
    case PhysicalParameters::Static:
        mDynamic = false;
        mShape = ShapeMesh;
        break;
    case PhysicalParameters::DynamicBox:
        mDynamic = true;
        mShape = ShapeBox;
        break;
    case PhysicalParameters::DynamicCylinder:
        mDynamic = true;
        mShape = ShapeCylinder;
        break;
    case PhysicalParameters::DynamicSphere:
        mDynamic = true;
        mShape = ShapeSphere;
        break;
    case PhysicalParameters::Character:
        mDynamic = true;
        mShape = ShapeCharacter;
        break;
    }
    if (mMeshptr) {
        if (mDynamic && (!mMeshptr->isLocal()) ) {      /// for now, physics ignores dynamic objects on other hosts
            DEBUG_OUTPUT(cout << "  dbm: debug setPhysical: disabling dynamic&non-local" << endl);
            mActive = false;
            mMeshptr->setLocationAuthority(0);
            return;
        }
    }
    if (!(pp.mode==PhysicalParameters::Disabled)) {
        DEBUG_OUTPUT(cout << "  dbm: debug setPhysical: adding to bullet" << endl);
        positionOrientation po;
        po.p = mMeshptr->getPosition();
        po.o = mMeshptr->getOrientation();
        Vector3f size = mMeshptr->getScale();
        system->addPhysicalObject(this, po, pp.density, pp.friction, pp.bounce, pp.hull, size.x, size.y, size.z);
        mMeshptr->setLocationAuthority(this);
    }
}

positionOrientation BulletObj::getBulletState() {
    btTransform trans;
    this->mBulletBodyPtr->getMotionState()->getWorldTransform(trans);
    return positionOrientation(system, trans.getOrigin(),trans.getRotation());
}

void BulletObj::setBulletState(positionOrientation po, Vector3f linVel, Vector3f angVel) {
    btTransform trans;
    mBulletBodyPtr->getMotionState()->getWorldTransform(trans);
    trans.setOrigin(btVector3(po.p.x, po.p.y, po.p.z));
    trans.setRotation(btQuaternion(po.o.x, po.o.y, po.o.z, po.o.w));
    /// more Bullet mojo: dynamic vs kinematic
    if (mDynamic) {
        mBulletBodyPtr->proceedToTransform(trans);           /// how to move dynamic objects
        mBulletBodyPtr->setLinearVelocity(btVector3(linVel.x, linVel.y, linVel.z));
        mBulletBodyPtr->setAngularVelocity(btVector3(angVel.x, angVel.y, angVel.z));
    }
    else {
        mBulletBodyPtr->getMotionState()->setWorldTransform(trans);   /// how to move 'kinematic' objects (animated)
    }
    mBulletBodyPtr->activate(true);      /// wake up, you lazy slob!
}

void BulletObj::setScale (const Vector3f &newScale) {
    if (mSizeX == 0)         /// this gets called once before the bullet stuff is ready
        return;
    if (mSizeX==newScale.x && mSizeY==newScale.y && mSizeZ==newScale.z)
        return;
    mSizeX = newScale.x;
    mSizeY = newScale.y;
    mSizeZ = newScale.z;
    float mass;
    btVector3 localInertia(0,0,0);
    buildBulletShape(NULL, 0, mass);        /// null, 0 means re-use original vertices
    if (mDynamic) {                          /// inertia meaningless for static objects
        if (!mShape==ShapeMesh) {
            mColShape->calculateLocalInertia(mass,localInertia);
        }
        else {
            /// note: this code path not tested, as we don't yet support dynamic mesh
            cout << "using bounding box for inertia, Bullet does not calculate for mesh!" << endl;
            localInertia = btVector3(mSizeX, mSizeY, mSizeZ);      /// does this make sense?  it does to me
        }
    }
    mBulletBodyPtr->setCollisionShape(mColShape);
    mBulletBodyPtr->setMassProps(mass, localInertia);
    mBulletBodyPtr->setGravity(btVector3(mGravity.x, mGravity.y, mGravity.z));  /// otherwise gravity assumes old inertia!
    mBulletBodyPtr->activate(true);
    DEBUG_OUTPUT(cout << "dbm: setScale " << newScale << " old X: " << mSizeX << " mass: "
                 << mass << " localInertia: " << localInertia.getX() << "," << localInertia.getY() << "," << localInertia.getZ() << endl);
}

void BulletObj::buildBulletShape(const unsigned char* meshdata, int meshbytes, float &mass) {
    /// if meshbytes = 0, reuse vertices & indices (for rescaling)
    if (mColShape) delete mColShape;
    if (mDynamic) {
        if (mShape == ShapeSphere) {
            DEBUG_OUTPUT(cout << "dbm: shape=sphere " << endl);
            mColShape = new btSphereShape(btScalar(mSizeX*mHull.x));
            mass = mSizeX*mSizeX*mSizeX * mDensity * 4.189;                         /// Thanks, Wolfram Alpha!
        }
        if (mShape == ShapeCharacter) {
            DEBUG_OUTPUT(cout << "dbm: shape=character " << endl);                  /// for now, it's a sphere
            mColShape = new btSphereShape(btScalar(mSizeX*mHull.x));                /// should be ellipsoid or capsule, some day
            mass = mSizeX*mSizeX*mSizeX * mDensity * 20.0;                          /// yeah! Massive, dude!
        }
        else if (mShape == ShapeBox) {
            DEBUG_OUTPUT(cout << "dbm: shape=boxen " << endl);
            mColShape = new btBoxShape(btVector3(mSizeX*mHull.x*.5, mSizeY*mHull.y*.5, mSizeZ*mHull.z*.5));
            mass = mSizeX * mSizeY * mSizeZ * mDensity;
        }
        else if (mShape == ShapeCylinder) {
            DEBUG_OUTPUT(cout << "dbm: shape=cylinder " << endl);
            mColShape = new btCylinderShape(btVector3(mSizeX*mHull.x, mSizeY*mHull.y, mSizeZ*mHull.z));
            mass = mSizeX * mSizeY * mSizeZ * mDensity * 3.1416;
        }
    }
    else {
        /// create a mesh-based static (not dynamic ie forces, though kinematic, ie movable) object
        /// assuming !dynamic; in future, may support dynamic mesh through gimpact collision
        vector<double> bounds;
        if (mBtVertices)
            btAlignedFree(mBtVertices);
        mBtVertices=NULL;
        unsigned int i,j;

        if (meshbytes) {
            mVertices.clear();
            mIndices.clear();
            parseOgreMesh parser;
            parser.parseData(meshdata, meshbytes, mVertices, mIndices, bounds);
        }
        DEBUG_OUTPUT (cout << "dbm:mesh " << mVertices.size() << " vertices:" << endl);
        mBtVertices=(btScalar*)btAlignedAlloc(mVertices.size()/3*sizeof(btScalar)*4,16);
        for (i=0; i<mVertices.size()/3; i+=1) {
            DEBUG_OUTPUT ( cout << "dbm:mesh");
            for (j=0; j<3; j+=1) {
                DEBUG_OUTPUT (cout <<" " << mVertices[i*3+j]);
            }
            mBtVertices[i*4]=mVertices[i*3]*mSizeX;
            mBtVertices[i*4+1]=mVertices[i*3+1]*mSizeY;
            mBtVertices[i*4+2]=mVertices[i*3+2]*mSizeZ;
            mBtVertices[i*4+3]=1;
            DEBUG_OUTPUT (cout << endl);
        }
        DEBUG_OUTPUT (cout << endl);
        DEBUG_OUTPUT (cout << "dbm:mesh " << mIndices.size() << " indices:");
        for (i=0; i<mIndices.size(); i++) {
            DEBUG_OUTPUT (cout << " " << mIndices[i]);
        }
        DEBUG_OUTPUT (cout << endl);
        DEBUG_OUTPUT (cout << "dbm:mesh bounds:");
        for (i=0; i<bounds.size(); i++) {
            DEBUG_OUTPUT (cout << " " << bounds[i]);
        }
        DEBUG_OUTPUT (cout << endl);

        if (mIndexArray) delete mIndexArray;
        mIndexArray = new btTriangleIndexVertexArray(
            mIndices.size()/3,                       // # of triangles (int)
            &(mIndices[0]),                          // ptr to list of indices (int)
            sizeof(int)*3,                          // index stride, in bytes (typically 3X sizeof(int) = 12
            mVertices.size()/3,                      // # of vertices (int)
            mBtVertices,         // (btScalar*) pointer to vertex list
            sizeof(btScalar)*4);                     // vertex stride, in bytes
        btVector3 aabbMin(-10000,-10000,-10000),aabbMax(10000,10000,10000);
        mColShape  = new btBvhTriangleMeshShape(mIndexArray,false, aabbMin, aabbMax);
        DEBUG_OUTPUT(cout << "dbm: shape=trimesh mColShape: " << mColShape <<
                     " triangles: " << mIndices.size()/3 << " verts: " << mVertices.size()/3 << endl);
        mass = 0.0;
    }
}
BulletObj::~BulletObj() {
    DEBUG_OUTPUT(cout << "dbm: BulletObj destructor " << this << endl);
    if (mBtVertices!=NULL)
        btAlignedFree(mBtVertices);
    if (mMotionState!=NULL) delete mMotionState;
    if (mIndexArray!=NULL) delete mIndexArray;
    if (mColShape!=NULL) delete mColShape;
    if (mBulletBodyPtr!=NULL) delete mBulletBodyPtr;
}

void BulletObj::buildBulletBody(const unsigned char* meshdata, int meshbytes) {
    float mass;
    btTransform startTransform;
    btVector3 localInertia(0,0,0);
    btRigidBody* body;

    buildBulletShape(meshdata, meshbytes, mass);

    DEBUG_OUTPUT(cout << "dbm: mass = " << mass << endl;)
    if (mDynamic) {
        mColShape->calculateLocalInertia(mass,localInertia);
    }
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(mInitialPo.p.x, mInitialPo.p.y, mInitialPo.p.z));
    startTransform.setRotation(btQuaternion(mInitialPo.o.x, mInitialPo.o.y, mInitialPo.o.z, mInitialPo.o.w));
    mMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,mMotionState,mColShape,localInertia);
    body = new btRigidBody(rbInfo);
    body->setFriction(mFriction);
    body->setRestitution(mBounce);
    if (!mDynamic) {
        /// voodoo recommendations from the bullet tutorials
        body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
    }
    else {
        if (mShape==ShapeCharacter) {
            body->setAngularFactor(0);  /// for now, all we do with characters is avoid external angular effects
        }
    }
    system->dynamicsWorld->addRigidBody(body);
    body->setGravity(btVector3(mGravity.x, mGravity.y, mGravity.z));
    mBulletBodyPtr=body;
    mActive=true;
    system->bt2siri[body]=this;
}

void BulletObj::requestLocation(TemporalValue<Location>::Time timeStamp, const Protocol::ObjLoc& reqLoc) {
    mPIDControlEnabled = true;      /// need a way to turn this off!
    if (reqLoc.has_velocity()) {
        btVector3 btvel(reqLoc.velocity().x, reqLoc.velocity().y, reqLoc.velocity().z);
//        mBulletBodyPtr->setLinearVelocity(btvel);
        mDesiredLinearVelocity = btvel;
    }
    if (reqLoc.has_angular_speed()) {
        Vector3f axis(0,1,0);
        if (reqLoc.has_rotational_axis()) {
            axis = reqLoc.rotational_axis();
        }
        else if (reqLoc.angular_speed() != 0) {
            cout << "ERROR -- please don't specify an angular speed without an axis" << endl;
            assert(false);
        }
        axis = mMeshptr->getOrientation() * axis;
        axis *= reqLoc.angular_speed();
        btVector3 btangvel(axis.x, axis.y, axis.z);
//        mBulletBodyPtr->setAngularVelocity(btangvel);
        mDesiredAngularVelocity = btangvel;
    }
}

Task::EventResponse BulletSystem::downloadFinished(Task::EventPtr evbase, BulletObj* bullobj) {
    Transfer::DownloadEventPtr ev = std::tr1::static_pointer_cast<Transfer::DownloadEvent> (evbase);
    DEBUG_OUTPUT (cout << "dbm: downloadFinished: status:" << (int)ev->getStatus()
                  << " success: " << (int)Transfer::TransferManager::SUCCESS
                  << " bullet obj: " << bullobj
                  << " length = " << (int)ev->data().length() << endl);
    if (!ev->getStatus()==Transfer::TransferManager::SUCCESS) {
        cout << "BulletSystem::downloadFinished failed, bullet object will not be built" << endl;
    }
    else {
        Transfer::DenseDataPtr flatData = ev->data().flatten();
        const unsigned char* realData = flatData->data();
        DEBUG_OUTPUT (cout << "dbm downloadFinished: data: " << (char*)&realData[2] << endl);
        bullobj->buildBulletBody(realData, ev->data().length());
    }
    return Task::EventResponse::del();
}

void BulletSystem::addPhysicalObject(BulletObj* obj,
                                     positionOrientation po,
                                     float density, float friction, float bounce, Vector3f hull,
                                     float mSizeX, float mSizeY, float mSizeZ) {
    /// a bit annoying -- we have to keep all these around in case our mesh isn't available
    /// note that presently these values are not updated during simulation (particularly po)
    obj->mDensity = density;
    obj->mFriction = friction;
    obj->mBounce = bounce;
    obj->mSizeX = mSizeX;
    obj->mSizeY = mSizeY;
    obj->mSizeZ = mSizeZ;
    obj->mInitialPo = po;
    obj->mHull = hull;
    DEBUG_OUTPUT(cout << "dbm: adding active object: " << obj << " shape: " << (int)obj->mShape << endl);
    if (obj->mDynamic) {
        /// create the object now
        obj->buildBulletBody(NULL, 0);                /// no mesh data
    }
    else {
        /// set up a mesh download; callback (downloadFinished) calls buildBulletBody and completes object
        transferManager->download(obj->mMeshptr->getMesh(), std::tr1::bind(&Sirikata::BulletSystem::downloadFinished,
                                  this, _1, obj), Transfer::Range(true));
    }
}

void BulletSystem::removePhysicalObject(BulletObj* obj) {
    /// this is tricky, and not well tested
    /// memory issues:
    /// there are a number of objects created during the instantiation of a BulletObj
    /// if they really need to be kept around, we should keep track of them & delete them
    DEBUG_OUTPUT(cout << "dbm: removing active object: " << obj << endl;)
    for (unsigned int i=0; i<objects.size(); i++) {
        if (objects[i] == obj) {
            if (objects[i]->mActive) {
                dynamicsWorld->removeRigidBody(obj->mBulletBodyPtr);
            }
            delete obj;
            break;
        }
    }
}

float btMagSq(const btVector3& v) {
    return v.x() * v.x()
           + v.y() * v.y()
           + v.z() * v.z();
}

bool BulletSystem::tick() {
    static Task::LocalTime lasttime = mStartTime;
    static Task::DeltaTime waittime = Task::DeltaTime::seconds(0.02);
    //static int mode = 0;
    static BulletObj* avatar_id=0;
    Task::LocalTime now = Task::LocalTime::now();
    Task::DeltaTime delta;
    positionOrientation po;

    DEBUG_OUTPUT(cout << "dbm: BulletSystem::tick time: " << (now-mStartTime).toSeconds() << endl;)
    if (now > lasttime + waittime) {
        delta = now-lasttime;
        if (delta.toSeconds() > 0.05) delta = delta.seconds(0.05);           /// avoid big time intervals, they are trubble
        lasttime = now;
        if ((now-mStartTime) > Duration::seconds(gDebug? 12.0:30.0)) {                   /// wait for it

            /// main object loop
            for (unsigned int i=0; i<objects.size(); i++) {
                if (objects[i]->mActive) {
                    Location loc=objects[i]->mMeshptr->getLastLocation();

                    /// if object has been moved, reset bullet position accordingly
                    if (objects[i]->mMeshptr->getPosition() != objects[i]->getBulletState().p ||
                            objects[i]->mMeshptr->getOrientation() != objects[i]->getBulletState().o) {
                        DEBUG_OUTPUT(cout << "dbm debug: object, " << objects[i]->mName << " moved" << endl
                                     << "    debug meshpos: " << objects[i]->mMeshptr->getPosition() << endl
                                     << "    debug bulletpos before reset: " << objects[i]->getBulletState().p << endl
                                     << "    debug set vel " << loc.getVelocity() << endl);
                        objects[i]->setBulletState(
                            positionOrientation (
                                loc.getPosition(),
                                loc.getOrientation()),
                            loc.getVelocity(), Vector3f(0,0,0)  /// FIXME: should properly calcualte & set ang vel
                        );
                        DEBUG_OUTPUT(cout << "    debug new pos: " << objects[i]->getBulletState().p << endl);
                        if (gMode=="funmode")
                            /// fun hack: set painting gravity to zero on reset
                            if (objects[i]->mName.size()>7 && objects[i]->mName.substr(0,8)=="artwork_") {
                                objects[i]->mGravity=Vector3f(0,0,0);
                                objects[i]->mBulletBodyPtr->setGravity(
                                    btVector3(objects[i]->mGravity.x, objects[i]->mGravity.y, objects[i]->mGravity.z));
                                cout << "dbm debug gravity set on" << objects[i]->mName << endl;
                            }
                    }

                    /// if object under PID control, control it
                    if (objects[i]->mPIDControlEnabled) {

                        /// this is not yet a real PID controller!  YMMV
                        DEBUG_OUTPUT(cout << "dbm debug PID vel: " <<
                                     objects[i]->mDesiredLinearVelocity.x() <<
                                     objects[i]->mDesiredLinearVelocity.y() <<
                                     objects[i]->mDesiredLinearVelocity.z()
                                     << endl);
                        objects[i]->mBulletBodyPtr->setLinearVelocity(objects[i]->mDesiredLinearVelocity);
                        objects[i]->mBulletBodyPtr->setAngularVelocity(objects[i]->mDesiredAngularVelocity);
                        objects[i]->mBulletBodyPtr->activate(true);

                        /// bit of a hack: if both linear & angular vel are zero, release control (so gravity & inertia can have fun)
                        if ( (btMagSq(objects[i]->mDesiredLinearVelocity) < 0.001f) &&
                                (btMagSq(objects[i]->mDesiredAngularVelocity) < 0.001f) ) {
                            objects[i]->mPIDControlEnabled=false;
                        }
                    }
                    if (objects[i]->mName=="Avatar_fun") {
                        avatar_id = objects[i];
                    }
                }
            }
//            dynamicsWorld->stepSimulation(delta.toSeconds(),Duration::seconds(10).toSeconds());
            dynamicsWorld->stepSimulation(delta.toSeconds(),20, 1.0/300.0);

            for (unsigned int i=0; i<objects.size(); i++) {
                if (objects[i]->mActive) {
                    po = objects[i]->getBulletState();
                    DEBUG_OUTPUT(cout << "    dbm: object, " << objects[i]->mName << ", delta, "
                                 << delta.toSeconds() << ", newpos, " << po.p << "obj: " << objects[i] << endl);
                    Time remoteNow=Time::convertFrom(now,SpaceTimeOffsetManager::getSingleton().getSpaceTimeOffset(objects[i]->mMeshptr->getObjectReference().space()));
                    Location loc (objects[i]->mMeshptr->globalLocation(remoteNow));
                    if (    (po.p.x <= 100000. && po.p.x >= -100000.)==false ||
                            (po.p.y <= 100000. && po.p.y >= -100000.)==false ||
                            (po.p.z <= 100000. && po.p.z >= -100000.)==false ) {
                        cout << "dbm debug BAD POSITION! (won't try to recover)" << objects[i]->mName << " pos: " << po.p << endl;
                        /*
                        Location lastKnown=objects[i]->mMeshptr->extrapolateLocation(remoteNow);
                        po.p.x=lastKnown.getPosition().x+.1*(rand()/(float)RAND_MAX);
                        po.p.y=lastKnown.getPosition().y+.1*(rand()/(float)RAND_MAX);
                        po.p.z=lastKnown.getPosition().z+.1*(rand()/(float)RAND_MAX);
                        po.o=lastKnown.getOrientation();
                        cout << "dbm debug POSITION FIX:" << objects[i]->mName << " pos: " << po.p << endl;
                        loc.setVelocity(lastKnown.getVelocity());
                        loc.setAxisOfRotation(lastKnown.getAxisOfRotation());
                        loc.setAngularSpeed(lastKnown.getAngularSpeed());
                        */
                    }
                    loc.setPosition(po.p);
                    loc.setOrientation(po.o);
                    objects[i]->mMeshptr->setLocation(remoteNow, loc);
                }
            }

            /// test queryRay
            /*
            ProxyMeshObjectPtr bugObj;
            for (unsigned int i=0; i<objects.size(); i++)
                if (objects[i]->mName == "queryRay") bugObj = objects[i]->mMeshptr;
            double dist;
            Vector3f norm;
            SpaceObjectReference sor;
            //queryRay(Vector3d(48.81, 4618.08, 23.31), Vector3f(0,-1,0), 100.0, dist, norm, sor);  /// BORNHOLM SCENE
            queryRay(Vector3d(0, 10, 0), Vector3f(0,-1,0), 20.0, bugObj, dist, norm, sor);
            cout << "dbm debug: queryRay returns distance: " << dist << " normal: " << norm << " object: " << sor << endl;
            */

            /// collision messages
            std::map<ObjectReference,RoutableMessageBody> mBeginCollisionMessagesToSend;
            std::map<ObjectReference,RoutableMessageBody> mEndCollisionMessagesToSend;
            BulletObj* anExampleCollidingMesh=NULL;
            for (customDispatch::CollisionPairMap::iterator i=dispatcher->collisionPairs.begin();
                    i != dispatcher->collisionPairs.end(); /*increment in if*/) {
                BulletObj* b0=anExampleCollidingMesh=i->first.getLower();
                BulletObj* b1=i->first.getHigher();
                ObjectReference b0id=b0->getObjectReference();
                ObjectReference b1id=b1->getObjectReference();
                Time spaceNow=Time::convertFrom(now,SpaceTimeOffsetManager::getSingleton().getSpaceTimeOffset(b0->getSpaceID()));

                if (i->second.collidedThisFrame()) {             /// recently colliding; send msg & change mode
                    if (!i->second.collidedLastFrame()) {
                        if (b1->colMsg & b0->colMask) {
                            RoutableMessageBody *body=&mBeginCollisionMessagesToSend[b1id];

                            Physics::Protocol::CollisionBegin collide;
                            collide.set_timestamp(spaceNow);
                            collide.set_other_object_reference(b0id.getAsUUID());
                            for (std::vector<customDispatch::ActiveCollisionState::PointCollision>::iterator iter=i->second.mPointCollisions.begin(),iterend=i->second.mPointCollisions.end();iter!=iterend;++iter) {
                                collide.add_this_position(iter->mWorldOnHigher);
                                collide.add_other_position(iter->mWorldOnLower);
                                collide.add_this_normal(iter->mNormalWorldOnHigher);
                                collide.add_impulse(iter->mAppliedImpulse);

                            }
                            collide.SerializeToString(body->add_message("BegCol"));
                            cout << "   begin collision msg: " << b0->mName << " --> " << b1->mName
                            << " time: " << (Task::LocalTime::now()-mStartTime).toSeconds() << endl;
                            if (b0->mName.size() >= 8 && b0->mName.substr(0,8)=="artwork_") {
                                b0->mGravity=Vector3f(0,-1,0);
                                b0->mBulletBodyPtr->setGravity(
                                    btVector3(b0->mGravity.x, b0->mGravity.y, b0->mGravity.z));
                                cout << "dbm debug gravity set on" << b0->mName << endl;
                            }
                            if (b1->mName.size() >= 8 && b1->mName.substr(0,8)=="artwork_") {
                                b1->mGravity=Vector3f(0,-1,0);
                                b1->mBulletBodyPtr->setGravity(
                                    btVector3(b1->mGravity.x, b1->mGravity.y, b1->mGravity.z));
                                cout << "dbm debug gravity set on" << b1->mName << endl;
                            }
                        }
                        if (b0->colMsg & b1->colMask) {
                            RoutableMessageBody *body=&mBeginCollisionMessagesToSend[b0id];

                            Physics::Protocol::CollisionBegin collide;
                            collide.set_timestamp(spaceNow);
                            collide.set_other_object_reference(b1id.getAsUUID());
                            for (std::vector<customDispatch::ActiveCollisionState::PointCollision>::iterator iter=i->second.mPointCollisions.begin(),iterend=i->second.mPointCollisions.end();iter!=iterend;++iter) {
                                collide.add_other_position(iter->mWorldOnHigher);
                                collide.add_this_position(iter->mWorldOnLower);
                                collide.add_this_normal(-iter->mNormalWorldOnHigher);
                                collide.add_impulse(iter->mAppliedImpulse);

                            }
                            collide.SerializeToString(body->add_message("BegCol"));
                            if (b0->mName.size() >= 8 && b0->mName.substr(0,8)=="artwork_") {
                                b0->mGravity=Vector3f(0,-1,0);
                                b0->mBulletBodyPtr->setGravity(
                                    btVector3(b0->mGravity.x, b0->mGravity.y, b0->mGravity.z));
                                cout << "dbm debug gravity set on" << b0->mName << endl;
                            }
                            if (b1->mName.size() >= 8 && b1->mName.substr(0,8)=="artwork_") {
                                b1->mGravity=Vector3f(0,-1,0);
                                b1->mBulletBodyPtr->setGravity(
                                    btVector3(b1->mGravity.x, b1->mGravity.y, b1->mGravity.z));
                                cout << "dbm debug gravity set on" << b1->mName << endl;
                            }
                            cout << "   begin collision msg: " << b1->mName << " --> " << b0->mName
                            << " time: " << (Task::LocalTime::now()-mStartTime).toSeconds() << endl;
                        }
                    }
                    i->second.resetCollisionFlag();
                    ++i;
                }
                else {        /// didn't get flagged again; collision now over
                    assert(i->second.collidedLastFrame());
                    if (b1->colMsg & b0->colMask) {
                        RoutableMessageBody *body=&mEndCollisionMessagesToSend[b1id];

                        Physics::Protocol::CollisionEnd collide;
                        collide.set_timestamp(spaceNow);
                        collide.set_other_object_reference(b0id.getAsUUID());
                        collide.SerializeToString(body->add_message("EndCol"));

                        cout << "     end collision msg: " << b0->mName << " --> " << b1->mName
                        << " time: " << (Task::LocalTime::now()-mStartTime).toSeconds() << endl;
                    }
                    if (b0->colMsg & b1->colMask) {
                        RoutableMessageBody *body=&mEndCollisionMessagesToSend[b0id];

                        Physics::Protocol::CollisionEnd collide;
                        collide.set_timestamp(spaceNow);
                        collide.set_other_object_reference(b1id.getAsUUID());
                        collide.SerializeToString(body->add_message("EndCol"));
                        cout << "     end collision msg: " << b1->mName << " --> " << b0->mName
                        << " time: " << (Task::LocalTime::now()-mStartTime).toSeconds() << endl;
                    }
                    dispatcher->collisionPairs.erase(i++);
                }
                for (std::map<ObjectReference,RoutableMessageBody>*whichMessages=&mBeginCollisionMessagesToSend;true;whichMessages=&mEndCollisionMessagesToSend) {//send all items from map 1, then all items from map 2 (for loop of size 2)
                    for (std::map<ObjectReference,RoutableMessageBody>::iterator iter=whichMessages->begin(),iterend=whichMessages->end();iter!=iterend;++iter) {
                        RoutableMessageHeader hdr;
                        if (avatar_id) {
                            /// for fun mode, send all collision msgs to avatar
                            hdr.set_destination_object(avatar_id->getObjectReference());
                        }
                        else {
                            hdr.set_destination_object(iter->first);
                        }
                        hdr.set_destination_space(anExampleCollidingMesh->getSpaceID());
                        hdr.set_source_object(ObjectReference::spaceServiceID());
                        hdr.set_source_port(Services::PHYSICS);
                        std::string body;
                        iter->second.SerializeToString(&body);
                        sendMessage(hdr,MemoryReference(body));
                    }
                    if (whichMessages==&mEndCollisionMessagesToSend) {
                        break;
                    }
                }
            }
        }
    }
    DEBUG_OUTPUT(cout << endl;)
    return true;
}

void customDispatch::ActiveCollisionState::collide(BulletObj* first, BulletObj* second, btPersistentManifold *currentCollisionManifold) {
    bool flipped= !(first<second);
    //so we can save the normals
    mCollisionFlag|=COLLIDED_THIS_FRAME;
    int size=currentCollisionManifold->getNumContacts();
    mPointCollisions.resize(size);
    for (int i=0;i<size;++i) {
        Vector3d posA=positionFromBullet(first->getBulletSystem(),currentCollisionManifold->getContactPoint(i).getPositionWorldOnA());
        Vector3d posB=positionFromBullet(first->getBulletSystem(),currentCollisionManifold->getContactPoint(i).getPositionWorldOnB());
        Vector3f nrmB=normalFromBullet(currentCollisionManifold->getContactPoint(i).m_normalWorldOnB);
        if (flipped) {
            mPointCollisions[i].mWorldOnLower=posB;
            mPointCollisions[i].mWorldOnHigher=posA;
            mPointCollisions[i].mNormalWorldOnHigher=-nrmB;
        }
        else {
            mPointCollisions[i].mWorldOnLower=posA;
            mPointCollisions[i].mWorldOnHigher=posB;
            mPointCollisions[i].mNormalWorldOnHigher=nrmB;
        }
        mPointCollisions[i].mAppliedImpulse=currentCollisionManifold->getContactPoint(i).getAppliedImpulse();
    }
}


void customNearCallback(btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher,
                        const btDispatcherInfo& dispatchInfo) {
    /// we gots to do the stuff Bullet does ourselves to capture the collisionPair

    btCollisionObject* colObj0 = (btCollisionObject*)collisionPair.m_pProxy0->m_clientObject;
    btCollisionObject* colObj1 = (btCollisionObject*)collisionPair.m_pProxy1->m_clientObject;

    if (dispatcher.needsCollision(colObj0,colObj1)) {
        //dispatcher will keep algorithms persistent in the collision pair
        if (!collisionPair.m_algorithm) {
            collisionPair.m_algorithm = dispatcher.findAlgorithm(colObj0,colObj1);
        }

        if (collisionPair.m_algorithm) {
            btManifoldResult contactPointResult(colObj0,colObj1);

            if (dispatchInfo.m_dispatchFunc ==      btDispatcherInfo::DISPATCH_DISCRETE) {
                //discrete collision detection query
                collisionPair.m_algorithm->processCollision(colObj0,colObj1,dispatchInfo,&contactPointResult);
            }
            else {
                //continuous collision detection query, time of impact (toi)
                btScalar toi = collisionPair.m_algorithm->calculateTimeOfImpact(colObj0,colObj1,dispatchInfo,&contactPointResult);
                if (dispatchInfo.m_timeOfImpact > toi)
                    dispatchInfo.m_timeOfImpact = toi;
            }
            btPersistentManifold*persistentManifold=contactPointResult.getPersistentManifold();
            int contacts = persistentManifold->getNumContacts();
            if (contacts) {
                BulletObj* siri0 = ((customDispatch*)(&dispatcher))->bt2siri[0][colObj0];
                BulletObj* siri1 = ((customDispatch*)(&dispatcher))->bt2siri[0][colObj1];
                if (siri0 && siri1) {
                    if (siri0->colMask & siri1->colMask) {
                        ((customDispatch*)(&dispatcher))->collisionPairs[customDispatch::OrderedCollisionPair(siri0,siri1)].collide(siri0,siri1,persistentManifold);
                    }
                }
            }
        }
    }
}

void initBornholm() {
    std::cout << "dbm debug INIT BORNHOLM!!!" << std::endl;
    std::string temp;
    if (gMode=="") {
        std::ifstream ifs( "mode.txt" );
        while (getline(ifs, temp)) {
            int delim = temp.find("=");
            std::string cmd = temp.substr(0,delim);
            std::string val = temp.substr(delim+1,temp.size()-delim-1);
            if (cmd=="mode") {
                gMode=val;
            }
            if (cmd=="width") {
                gWidth=val;
            }
            if (cmd=="height") {
                gHeight=val;
            }
            if (cmd=="fullscreen") {
                if (val=="False") gFullscreen="false";
            }
            if (cmd=="debug") {
                if (val=="True") gDebug=true;
            }
        }
        std::cout << "dbm debug bullet initBornholm width " << gWidth <<" height "<<gHeight<<
        " full "<<gFullscreen<<" debug "<<gDebug<< " mode " << gMode<<std::endl;
    }
}

bool BulletSystem::initialize(Provider<ProxyCreationListener*>*proxyManager, const String&options) {
    DEBUG_OUTPUT(cout << "dbm: BulletSystem::initialize options: " << options << endl);

    initBornholm();

    /// HelloWorld from Bullet/Demos
    mTempTferManager = new OptionValue("transfermanager","0", OptionValueType<void*>(),"dummy");
    mWorkQueue = new OptionValue("workqueue","0",OptionValueType<void*>(),"Memory address of the WorkQueue");
    mEventManager = new OptionValue("eventmanager","0",OptionValueType<void*>(),"Memory address of the EventManager<Event>");
    InitializeClassOptions("bulletphysics",this, mTempTferManager, mWorkQueue, mEventManager, NULL);
    OptionSet::getOptions("bulletphysics",this)->parse(options);
    Transfer::TransferManager* tm = (Transfer::TransferManager*)mTempTferManager->as<void*>();
    this->transferManager = tm;

    groundlevel = -100.0;
    btTransform groundTransform;
    btDefaultMotionState* mMotionState;
    btVector3 worldAabbMin(-10000,-10000,-10000);
    btVector3 worldAabbMax(10000,10000,10000);
    int maxProxies = 1024;
    btVector3 localInertia(0,0,0);

    /// set up bullet stuff
    collisionConfiguration = new btDefaultCollisionConfiguration();
    //dispatcher = new btCollisionDispatcher(collisionConfiguration);
    dispatcher = new customDispatch(collisionConfiguration, &bt2siri);
    dispatcher->setNearCallback(customNearCallback);
    overlappingPairCache= new btAxisSweep3(worldAabbMin,worldAabbMax,maxProxies);
    solver = new btSequentialImpulseConstraintSolver;
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,overlappingPairCache,solver,collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(mGravity.x, mGravity.y, mGravity.z));

    /// create ground
    groundShape= new btBoxShape(btVector3(btScalar(1500.),btScalar(1.0),btScalar(1500.)));
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0,groundlevel-1,0));
    groundShape->calculateLocalInertia(0.0f,localInertia);
    mMotionState = new btDefaultMotionState(groundTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f,mMotionState,groundShape,localInertia);
    groundBody = new btRigidBody(rbInfo);
    groundBody->setRestitution(0.2);
    groundBody->setFriction(0.1);
    dynamicsWorld->addRigidBody(groundBody);
    proxyManager->addListener(this);
    DEBUG_OUTPUT(cout << "dbm: BulletSystem::initialized, including test bullet object" << endl);
    /// we don't delete these, the ProxyManager does (I think -- someone does anyway)
//    delete mTempTferManager;
//    delete mWorkQueue;
//    delete mEventManager;
    return true;
}

BulletSystem::BulletSystem() :
        mGravity(0, GRAVITY, 0),
        mStartTime(Task::LocalTime::now()) {
    DEBUG_OUTPUT(cout << "dbm: I am the BulletSystem constructor!" << endl);

}

BulletSystem::~BulletSystem() {
    DEBUG_OUTPUT(cout << "dbm: BulletSystem destructor" << endl);

    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfiguration;
    delete groundBody;
    delete groundShape;
    DEBUG_OUTPUT(cout << "dbm: BulletSystem destructor finished" << endl;)
}

void BulletSystem::createProxy(ProxyObjectPtr p) {
    ProxyMeshObjectPtr meshptr(tr1::dynamic_pointer_cast<ProxyMeshObject>(p));
    if (meshptr) {
        DEBUG_OUTPUT(cout << "dbm: createProxy ptr:" << meshptr << " mesh: " << meshptr->getMesh() << endl;)
        objects.push_back(new BulletObj(this));     /// clean up memory!!!
        objects.back()->mMeshptr = meshptr;
        meshptr->MeshProvider::addListener(objects.back());
    }
}

void BulletSystem::destroyProxy(ProxyObjectPtr p) {
    ProxyMeshObjectPtr meshptr(tr1::dynamic_pointer_cast<ProxyMeshObject>(p));
    for (unsigned int i=0; i<objects.size(); i++) {
        if (objects[i]->mMeshptr==meshptr) {
            DEBUG_OUTPUT(cout << "dbm: destroyProxy, object=" << objects[i] << endl);
            meshptr->MeshProvider::removeListener(objects[i]);
            removePhysicalObject(objects[i]);
            objects.erase(objects.begin()+i);
            break;
        }
    }
}

struct  raycastCallback : public btCollisionWorld::RayResultCallback {
    btCollisionObject* mSkip;
    btVector3   m_hitNormalWorld;

    raycastCallback(btCollisionObject* skip) :
            mSkip(skip) {
    }

    virtual btScalar    addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace) {
        //caller already does the filter on the m_closestHitFraction
        if (rayResult.m_collisionObject==mSkip) {
            return 1.0;
        }
        m_closestHitFraction = rayResult.m_hitFraction;
        m_collisionObject = rayResult.m_collisionObject;
        if (normalInWorldSpace) {
            m_hitNormalWorld = rayResult.m_hitNormalLocal;
        }
        else {
            ///need to transform normal into worldspace
            m_hitNormalWorld = m_collisionObject->getWorldTransform().getBasis()*rayResult.m_hitNormalLocal;
        }
        //m_hitPointWorld.setInterpolate3(m_rayFromWorld,m_rayToWorld,rayResult.m_hitFraction);
        return rayResult.m_hitFraction;
    }
};
bool BulletSystem::forwardMessagesTo(MessageService*ms) {
    messageServices.push_back(ms);
    return true;
}
bool BulletSystem::endForwardingMessagesTo(MessageService*ms) {
    bool retval=false;
    do {
        std::vector<MessageService*>::iterator where=std::find(messageServices.begin(),messageServices.end(),ms);
        if (where!=messageServices.end()) {
            messageServices.erase(where);
            retval=true;
        }
        else break;
    }
    while (true);
    return retval;
}

void BulletSystem::processMessage(const RoutableMessageHeader&mh, MemoryReference message_body) {

}

void BulletSystem::sendMessage(const RoutableMessageHeader&mh, MemoryReference message_body) {
    for (vector<MessageService*>::iterator i=messageServices.begin(),ie=messageServices.end();i!=ie;++i) {
        (*i)->processMessage(mh,message_body);
    }
}

bool BulletSystem::queryRay(const Vector3d& position,
                            const Vector3f& direction,
                            const double maxDistance,
                            ProxyMeshObjectPtr ignore,
                            double &returnDistance,
                            Vector3f &returnNormal,
                            SpaceObjectReference &returnName) {

    btVector3 start(position.x, position.y, position.z);
    Vector3d temp = position + Vector3d(direction)*maxDistance;
    btVector3 end(temp.x, temp.y, temp.z);
    btCollisionObject* btIgnore=0;
    if (ignore)
        btIgnore = mesh2bullet(ignore)->mBulletBodyPtr;         /// right now this is a slow walk in the park
    raycastCallback cb(btIgnore);
    dynamicsWorld->rayTest (start, end, cb);
    if (cb.hasHit ()) {
        btVector3 norm = cb.m_hitNormalWorld.normalize();
        returnNormal.x = norm.getX();
        returnNormal.y = norm.getY();
        returnNormal.z = norm.getZ();
        BulletObj* obj=bt2siri[cb.m_collisionObject];
        returnDistance = maxDistance * cb.m_closestHitFraction;
        if (obj) {
            /// if not found, it's probably the ground body
            returnName = obj->mMeshptr->getObjectReference();
        }
        return true;
    }
    else {
        return false;
    }
}


}//namespace sirikata
