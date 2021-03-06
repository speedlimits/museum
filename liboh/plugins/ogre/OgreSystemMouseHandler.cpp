/*  Sirikata liboh -- Ogre Graphics Plugin
 *  OgreSystemMouseHandler.cpp
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

#include <set>

#include <SDL_keysym.h>
#include <oh/Platform.hpp>
#include <oh/ProxyLightObject.hpp>
#include <oh/ProxyManager.hpp>
#include <oh/ProxyMeshObject.hpp>
#include <oh/ProxyObject.hpp>
#include <oh/SpaceTimeOffsetManager.hpp>
#include <task/Event.hpp>
#include <task/EventManager.hpp>
#include <task/Time.hpp>
#include <util/Standard.hh>

#include "CameraEntity.hpp"
#include "CameraPath.hpp"
#include "DragActions.hpp"
#include "InputBinding.hpp"
#include "InputResponse.hpp"
#include "LightBulb.hpp"
#include "LightEntity.hpp"
#include "MeshEntity.hpp"
#include "OgreSystem.hpp"
#include "Ogre_Sirikata.pbj.hpp"
#include "WebViewManager.hpp"
#include "input/InputEvents.hpp"
#include "input/SDLInputDevice.hpp"
#include "input/SDLInputManager.hpp"
#include "util/RoutableMessageBody.hpp"

namespace Sirikata {
namespace Graphics {
using namespace Input;
using namespace Task;
using namespace std;

#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)
#ifdef _WIN32
#undef SDL_SCANCODE_UP
#define SDL_SCANCODE_UP 0x60
#undef SDL_SCANCODE_RIGHT
#define SDL_SCANCODE_RIGHT 0x5e
#undef SDL_SCANCODE_DOWN
#define SDL_SCANCODE_DOWN 0x5a
#undef SDL_SCANCODE_LEFT
#define SDL_SCANCODE_LEFT 0x5c
#undef SDL_SCANCODE_PAGEUP
#define SDL_SCANCODE_PAGEUP 0x61
#undef SDL_SCANCODE_PAGEDOWN
#define SDL_SCANCODE_PAGEDOWN 0x5b
#endif // _WIN32

//------------------------------------------------------------------------------

extern String gWidth, gHeight, gFullscreen, gMode;
extern bool gDebug;

bool compareEntity (const Entity* one, const Entity* two) {

    ProxyObject *pp = one->getProxyPtr().get();

    ProxyCameraObject* camera1 = dynamic_cast<ProxyCameraObject*>(pp);
    ProxyLightObject* light1 = dynamic_cast<ProxyLightObject*>(pp);
    ProxyMeshObject* mesh1 = dynamic_cast<ProxyMeshObject*>(pp);
    Time now = SpaceTimeOffsetManager::getSingleton().now(pp->getObjectReference().space());
    Location loc1 = pp->globalLocation(now);
    pp = two->getProxyPtr().get();
    Location loc2 = pp->globalLocation(now);
    ProxyCameraObject* camera2 = dynamic_cast<ProxyCameraObject*>(pp);
    ProxyLightObject* light2 = dynamic_cast<ProxyLightObject*>(pp);
    ProxyMeshObject* mesh2 = dynamic_cast<ProxyMeshObject*>(pp);
    if (camera1 && !camera2) return true;
    if (camera2 && !camera1) return false;
    if (camera1 && camera2) {
        return loc1.getPosition().x < loc2.getPosition().x;
    }
    if (light1 && mesh2) return true;
    if (mesh1 && light2) return false;
    if (light1 && light2) {
        return loc1.getPosition().x < loc2.getPosition().x;
    }
    if (mesh1 && mesh2) {
        return mesh1->getPhysical().name < mesh2->getPhysical().name;
    }
    return one<two;
}


//------------------------------------------------------------------------------
// Break a string into a vector of tokens.
//------------------------------------------------------------------------------

vector<String> tokenizeString (const String& str)
{
    vector<String> tokens;
    String delimiters(" ");
    String::size_type lastPos = str.find_first_not_of(delimiters, 0);
    String::size_type pos = str.find_first_of(delimiters, lastPos);
    while (String::npos != pos || String::npos != lastPos)
    {
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = str.find_first_not_of(delimiters, pos);
        pos = str.find_first_of(delimiters, lastPos);
    }
    return tokens;
}


//------------------------------------------------------------------------------
// Appends a sprintf of its argument list according to format,
// returning the formatted result.
// from http://www.codecodex.com/wiki/String_printf
//------------------------------------------------------------------------------

void string_vappendf(std::string *result, const char *format, va_list args) {
    char *tempBuf = NULL;

#ifndef _WIN32
    int tempBufSize = vasprintf(&tempBuf, format, args);
    if (tempBufSize < 0)
        return; // Cannot allocate enough memory.
#else // _WIN32
    int tempBufSize = 128;            // something reasonable to begin with
    for (;;) {
        tempBuf = (char*)malloc(tempBufSize);
        if (tempBuf == NULL)
            break;
        const int bufSizeNeeded = vsnprintf(tempBuf, tempBufSize, format, args);    // not including trailing null
        if (bufSizeNeeded >= 0 && bufSizeNeeded < tempBufSize) {                    // the whole thing did fit
            tempBufSize = bufSizeNeeded;        // not including trailing null
            break;
        }
        free(tempBuf);
        if (bufSizeNeeded < 0) {                // glibc < 2.1
            tempBufSize *= 2;                   // try something bigger
        }
        else {                                  // glibc >= 2.1
            tempBufSize = bufSizeNeeded + 1;    // now I know how much I need, including trailing null
        }
    }
#endif // _WIN32
    if (tempBuf != NULL) {
        result->append(tempBuf, tempBufSize);
        free(tempBuf);
    }
}


//------------------------------------------------------------------------------
// Returns a sprintf of its remaining arguments according to format.
//------------------------------------------------------------------------------

std::string string_printf(const char *format, ...) {
    std::string result;
    va_list args;
    va_start(args, format);
    string_vappendf(&result, format, args);
    va_end(args);
    return result;
} // string_printf


//------------------------------------------------------------------------------
// Appends a sprintf of its remaining arguments according to format.
//------------------------------------------------------------------------------

void string_appendf(std::string *result, const char *format, ...) {
    va_list args;
    va_start(args, format);
    string_vappendf(result, format, args);
    va_end(args);
} // string_appendf


// Defined in DragActions.cpp. (FIXME this comment)


////////////////////////////////////////////////////////////////////////////////
//                          Mousehandler event handler                        //
////////////////////////////////////////////////////////////////////////////////

class OgreSystem::MouseHandler {
    OgreSystem *mParent;
    std::vector<SubscriptionId> mEvents;
    typedef std::multimap<InputDevice*, SubscriptionId> DeviceSubMap;
    DeviceSubMap mDeviceSubscriptions;

    SpaceObjectReference mCurrentGroup;
    typedef std::set<ProxyObjectWPtr> SelectedObjectSet;
    SelectedObjectSet mSelectedObjects;
    SpaceObjectReference mLastShiftSelected;
    int mLastHitCount;
    float mLastHitX;
    float mLastHitY;
    // map from mouse button to drag for that mouse button.
    /* as far as multiple cursors are concerned,
       each cursor should have its own MouseHandler instance */
    std::map<int, DragAction> mDragAction;
    std::map<int, ActiveDrag*> mActiveDrag;
    std::set<int> mWebViewActiveButtons;
    /*
        typedef EventResponse (MouseHandler::*ClickAction) (EventPtr evbase);
        std::map<int, ClickAction> mClickAction;
    */
    float mCamSpeed;
    CameraPath mCameraPath;
    bool mRunningCameraPath;
    uint32 mCameraPathIndex;
    Task::DeltaTime mCameraPathTime;
    Task::LocalTime mLastCameraTime;

    typedef std::map<String, InputResponse*> InputResponseMap;
    InputResponseMap mInputResponses;

    InputBinding mInputBinding;

    int mWhichRayObject;

    // Class members
    static const char   mWhiteSpace[];                  // space between tokens
    static const char   mArraySpace[];                  // space between array elements
    static LightInfo    mSpotLightMoods[4];             // moods for spot lights        - call initLightMoods() before using
    static LightInfo    mPointLightMoods[4];            // moods for point lights       - call initLightMoods() before using
    static LightInfo    mDirectionalLightMoods[4];      // moods for directionallights  - call initLightMoods() before using
    static bool         mMoodLightsInited;              // indicates when the above moods have been initialized
    static float        mDefaultSpotLightInclination;   // default inclination of spotlights from normal to the wall
    static float        mDefaultLightHeight;            // default height of lights

    class SubObjectIterator {
        typedef Entity* value_type;
        //typedef ssize_t difference_type;
        typedef size_t size_type;
        OgreSystem::SceneEntitiesMap::const_iterator mIter;
        Entity *mParentEntity;
        OgreSystem *mOgreSys;
        void findNext() {
            while (!end() && !((*mIter).second->getProxy().getParent() == mParentEntity->id())) {
                ++mIter;
            }
        }
    public:
        SubObjectIterator(Entity *parent) :
                mParentEntity(parent),
                mOgreSys(parent->getScene()) {
            mIter = mOgreSys->mSceneEntities.begin();
            findNext();
        }
        SubObjectIterator &operator++() {
            ++mIter;
            findNext();
            return *this;
        }
        Entity *operator*() const {
            return (*mIter).second;
        }
        bool end() const {
            return (mIter == mOgreSys->mSceneEntities.end());
        }
    };


    /////////////////// HELPER FUNCTIONS ///////////////


    //--------------------------------------------------------------------------
    // Return the object under the mouse at the given location.
    //--------------------------------------------------------------------------

    Entity *hoverEntity (CameraEntity *cam, Time time, float xPixel, float yPixel, int *hitCount,int which=0) {
        Location location(cam->getProxy().globalLocation(time));
        Vector3f dir (pixelToDirection(cam, location.getOrientation(), xPixel, yPixel));
        SILOG(input,info,"X is "<<xPixel<<"; Y is "<<yPixel<<"; pos = "<<location.getPosition()<<"; dir = "<<dir);

        double dist;
        Vector3f normal;        
        Entity *mouseOverEntity = mParent->rayTrace(location.getPosition(), dir, *hitCount, dist, normal, which);
        if (mouseOverEntity) {
            while (!(mouseOverEntity->getProxy().getParent() == mCurrentGroup)) {
                mouseOverEntity = mParent->getEntity(mouseOverEntity->getProxy().getParent());
                if (mouseOverEntity == NULL) {
                    return NULL; // FIXME: should try again.
                }
            }
            return mouseOverEntity;
        }
        return NULL;
    }

    ///////////////////// CLICK HANDLERS /////////////////////
public:

    //--------------------------------------------------------------------------
    // Change the state of all of the selected items to deselected.
    //--------------------------------------------------------------------------
    void clearSelection() {
        for (SelectedObjectSet::const_iterator selectIter = mSelectedObjects.begin();
                selectIter != mSelectedObjects.end(); ++selectIter) {
            ProxyObjectPtr obj(selectIter->lock());
            Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (ent) {
                ent->setSelected(false);
            }
            // Fire deselected event.
        }
        mSelectedObjects.clear();
    }

private:

    //--------------------------------------------------------------------------
    bool recentMouseInRange(float x, float y, float *lastX, float *lastY) {
        float delx = x-*lastX;
        float dely = y-*lastY;

        if (delx<0) delx=-delx;
        if (dely<0) dely=-dely;
        if (delx>.03125||dely>.03125) {
            *lastX=x;
            *lastY=y;

            return false;
        }
        return true;
    }


    //--------------------------------------------------------------------------
    // Given the screen coordinate, hitQuery will return the entity, the position of intersection,
    // and the normal of the surface at the point of intersection.
    //--------------------------------------------------------------------------

    bool getCurrentGlobalCameraLocation(Location *location) {
        ProxyObjectPtr camera = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
        if (!camera)
            return false;
        Time now(SpaceTimeOffsetManager::getSingleton().now(camera->getObjectReference().space()));
        *location = camera->globalLocation(now);
        return true;
    }


    //--------------------------------------------------------------------------
    // Given the screen coordinate, hitQuery will return the entity, the position of intersection,
    // and the normal of the surface at the point of intersection.
    //--------------------------------------------------------------------------

    bool hitQuery(double screenX, double screenY, Entity const **entity, Vector3d *position, Vector3f *normal) {
        // Look for an object intersected by the ray.
        Location location;
        if (!getCurrentGlobalCameraLocation(&location))
            return false;
        Vector3f viewDirection(pixelToDirection(mParent->mPrimaryCamera, location.getOrientation(), screenX, screenY));
        double distance;
        Vector3f myNormal(0, 0, 0);
        bool success = false;
        int numHits = 1, i;
        const Entity *obj;
        for (i = 0; i < numHits; i++) { // FIXME: Does this really need to iterate?
            obj = mParent->rayTrace(location.getPosition(), viewDirection, numHits, distance, myNormal, i);
            if (obj == NULL) {      // No object found
                if (i < numHits)    // Why not?
                    continue;       // Still more objects: keep looking
                break;              // No more objects: return failure
            }
            success = true;
            break;
        }
        if (!success || !(myNormal.normalizeThis() > 0))
            return false;

        if (entity)
            *entity = obj;
        if (position)
            *position = location.getPosition() + distance * Vector3d(viewDirection.x, viewDirection.y, viewDirection.z);
        if (normal) {
            if (viewDirection.dot(myNormal) > 0)    // backfacing normal
                myNormal = -myNormal;               // make it front-facing
            *normal = myNormal;
        }
        return true;
    }


    //--------------------------------------------------------------------------
    // Determine whether the given entity is artwork.
    // FIXME: total kluge!  Need a way to not select walls etc
    //--------------------------------------------------------------------------

    bool isArtWork(const Entity *obj) {
        bool isArt = false;
        ProxyMeshObject* overMesh = dynamic_cast<ProxyMeshObject*>(obj->getProxyPtr().get());
        if (overMesh) {
            const String &s = overMesh->getPhysical().name;
            isArt = (s.size() >= 8 && s.substr(0,8)=="artwork_");
        }
        return isArt;
    }


    //--------------------------------------------------------------------------
    // Walk from the current location to the specified poition,
    // but aim towards the specified lookat direction.
    //--------------------------------------------------------------------------

    void walkTo(const Vector3d &position, Vector3f lookat) {
        if (lookat.x == 0 && lookat.z == 0) {
            SILOG(input, error, "walkTo: lookat vector is vertical");
            return;
        }
        lookat.y = 0;
        lookat.normalizeThis();

        /*
        // Here is where we would call python magic to do an animation. Right now we just set it.
        Protocol::ObjLoc rloc;
        rloc.set_position(position);
        rloc.set_orientation(Quaternion(Vector3f::unitY().cross(lookat), Vector3f::unitY(), lookat));
        rloc.set_velocity(Vector3f::zero());
        rloc.set_rotational_axis(Vector3f::unitY());
        rloc.set_angular_speed(0);
        ProxyObjectPtr camera = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
        Time now(SpaceTimeOffsetManager::getSingleton().now(camera->getObjectReference().space()));
    //    camera->requestLocation(now, rloc);
        camera->requestLocation(now, rloc);
        */
        ProxyObjectPtr camera = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
        Time now(SpaceTimeOffsetManager::getSingleton().now(camera->getObjectReference().space()));
        Location loc = camera->extrapolateLocation(now);

        loc.setPosition( position );
        loc.setOrientation( Quaternion(Vector3f::unitY().cross(lookat), Vector3f::unitY(), lookat) );
        loc.setVelocity(Vector3f(0,0,0));
        loc.setAngularSpeed(0);

        camera->setLocation(now, loc);
    }


    //--------------------------------------------------------------------------
    void selectObjectAction(Vector2f p, int direction) {
        CameraEntity *camera = mParent->mPrimaryCamera;
//        ProxyObjectPtr camera = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
        if (!camera) {
            return;
        }
        
        /// we don't want all these fancy selection modes for non-debug mode
        if (gDebug && mParent->mInputManager->isModifierDown(Input::MOD_SHIFT)) {
            // add object.
            int numObjectsUnderCursor=0;
            Entity *mouseOver = hoverEntity(camera, SpaceTimeOffsetManager::getSingleton().now(camera->getProxy().getObjectReference().space()), p.x, p.y, &numObjectsUnderCursor, mWhichRayObject);
            if (!mouseOver) {
                return;
            }
            if (mouseOver->id() == mLastShiftSelected && numObjectsUnderCursor==mLastHitCount ) {
                SelectedObjectSet::iterator selectIter = mSelectedObjects.find(mouseOver->getProxyPtr());
                if (selectIter != mSelectedObjects.end()) {
                    ProxyObjectPtr obj(selectIter->lock());
                    Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
                    if (ent) {
                        ent->setSelected(false);
                    }
                    mSelectedObjects.erase(selectIter);
                }
                mWhichRayObject+=direction;
                mLastShiftSelected = SpaceObjectReference::null();
            }else {
                mWhichRayObject=0;
            }
            mouseOver = hoverEntity(camera, SpaceTimeOffsetManager::getSingleton().now(camera->getProxy().getObjectReference().space()), p.x, p.y, &mLastHitCount, mWhichRayObject);
            if (!mouseOver) {
                return;
            }

            SelectedObjectSet::iterator selectIter = mSelectedObjects.find(mouseOver->getProxyPtr());
            if (selectIter == mSelectedObjects.end()) {
                SILOG(input,info,"Added selection " << mouseOver->id());
                mSelectedObjects.insert(mouseOver->getProxyPtr());
                mouseOver->setSelected(true);
                mLastShiftSelected = mouseOver->id();
                // Fire selected event.
            }
            else {

                ProxyObjectPtr obj(selectIter->lock());
                Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
                if (ent) {
                    SILOG(input,info,"Deselected " << ent->id());
                    ent->setSelected(false);
                }
                mSelectedObjects.erase(selectIter);
                // Fire deselected event.
            }
        }
        /*
        /// this screws things up
        else if (mParent->mInputManager->isModifierDown(Input::MOD_CTRL)) {
            SILOG(input,info,"Cleared selection");
            clearSelection();
            mLastShiftSelected = SpaceObjectReference::null();
        }
        */
        else
        
#if 0
        /// this never really worked all that well
        if (mParent->mInputManager->isModifierDown(Input::MOD_CTRL)) {   // click to go
            // Look for an object intersected by the ray.
            const Entity *obj = NULL;
            Vector3d position;
            Vector3f normal;
            if (!hitQuery(p.x, p.y, &obj, &position, &normal))
                return;

            Location location;
            getCurrentGlobalCameraLocation(&location);  // This will succeed since hitQuery succeeded.
            Vector3f viewDirection(pixelToDirection(mParent->mPrimaryCamera, location.getOrientation(), p.x, p.y));
            Vector3f lookat = viewDirection;            // By default, look in the direction that we clicked.
            if (fabs(normal.y) < 1e-2) {                // Clicked on a wall or picture.
                double backOffDistance = 2;
                Vector3d fromWall(normal.x, 0, normal.z);
                fromWall.normalizeThis();
                position += backOffDistance * fromWall;
                if (isArtWork(obj)) lookat = -normal;   // Clicked on a picture: look at it.
            }
            else {                                      // Clicked on the floor
            }
            walkTo(position, lookat);
            return;     // Since this is not a selection, do not do the selection epilogue.
        }
        else
#endif
        {
            // reset selection.
            clearSelection();
            mWhichRayObject+=direction;
            int numObjectsUnderCursor=0;
            Entity *mouseOver = hoverEntity(camera, SpaceTimeOffsetManager::getSingleton().now(camera->getProxy().getObjectReference().space()), p.x, p.y, &numObjectsUnderCursor, mWhichRayObject);
            /// force selection even if already selected
//            if (recentMouseInRange(p.x, p.y, &mLastHitX, &mLastHitY)==false||numObjectsUnderCursor!=mLastHitCount){
            mouseOver = hoverEntity(camera, SpaceTimeOffsetManager::getSingleton().now(camera->getProxy().getObjectReference().space()), p.x, p.y, &mLastHitCount, mWhichRayObject=0);
//            }
            if (mouseOver) {
                if (gMode=="dev" || (isArtWork(mouseOver) && (gMode != "funmode"))) {
                    mSelectedObjects.insert(mouseOver->getProxyPtr());
                    mouseOver->setSelected(true);
                    SILOG(input,info,"Replaced selection with " << mouseOver->id());
                    // Fire selected event.
                }
            }
            mLastShiftSelected = SpaceObjectReference::null();
        }
        getSelectedIDHandler(); // Update the list of selected items in the Javascript world.
        return;
    }

    ///////////////// KEYBOARD HANDLERS /////////////////

    //--------------------------------------------------------------------------
    void deleteObjectsAction() {
        Task::LocalTime now(Task::LocalTime::now());
        while (doUngroupObjects(now)) {
        }
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (ent) {
                ent->getProxy().getProxyManager()->destroyObject(ent->getProxyPtr());
            }
        }
        mSelectedObjects.clear();
    }

    //--------------------------------------------------------------------------
    Entity *doCloneObject(Entity *ent, const ProxyObjectPtr &parentPtr, Time now) {
        SpaceObjectReference newId = SpaceObjectReference(ent->id().space(), ObjectReference(UUID::random()));
        Location loc = ent->getProxy().globalLocation(now);
        Location localLoc = ent->getProxy().extrapolateLocation(now);
        ProxyManager *proxyMgr = ent->getProxy().getProxyManager();

        std::tr1::shared_ptr<ProxyMeshObject> meshObj(
            std::tr1::dynamic_pointer_cast<ProxyMeshObject>(ent->getProxyPtr()));
        std::tr1::shared_ptr<ProxyLightObject> lightObj(
            std::tr1::dynamic_pointer_cast<ProxyLightObject>(ent->getProxyPtr()));
        ProxyObjectPtr newObj;
        if (meshObj) {
            std::tr1::shared_ptr<ProxyMeshObject> newMeshObject (new ProxyMeshObject(proxyMgr, newId));
            newObj = newMeshObject;
            proxyMgr->createObject(newMeshObject);
            newMeshObject->setMesh(meshObj->getMesh());
            newMeshObject->setScale(meshObj->getScale());
        }
        else if (lightObj) {
            std::tr1::shared_ptr<ProxyLightObject> newLightObject (new ProxyLightObject(proxyMgr, newId));
            newObj = newLightObject;
            proxyMgr->createObject(newLightObject);
            newLightObject->update(lightObj->getLastLightInfo());
        }
        else {
            newObj = ProxyObjectPtr(new ProxyMeshObject(proxyMgr, newId));
            proxyMgr->createObject(newObj);
        }
        if (newObj) {
            if (parentPtr) {
                newObj->setParent(parentPtr, now, loc, localLoc);
                newObj->resetLocation(now, localLoc);
            }
            else {
                newObj->resetLocation(now, loc);
            }
        }
        {
            std::list<Entity*> toClone;
            for (SubObjectIterator subIter (ent); !subIter.end(); ++subIter) {
                toClone.push_back(*subIter);
            }
            for (std::list<Entity*>::const_iterator iter = toClone.begin(); iter != toClone.end(); ++iter) {
                doCloneObject(*iter, newObj, now);
            }
        }
        return mParent->getEntity(newId);
    }

    //--------------------------------------------------------------------------
    void cloneObjectsAction() {
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();
        Task::LocalTime now(Task::LocalTime::now());
        SelectedObjectSet newSelectedObjects;
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (!ent) {
                continue;
            }
            Entity *newEnt = doCloneObject(ent, ent->getProxy().getParentProxy(), Time::convertFrom(now,SpaceTimeOffsetManager::getSingleton().getSpaceTimeOffset(ent->getProxy().getObjectReference().space())));
            Time objnow=Time::convertFrom(now,SpaceTimeOffsetManager::getSingleton().getSpaceTimeOffset(ent->getProxy().getObjectReference().space()));
            Location loc (ent->getProxy().extrapolateLocation(objnow));
            loc.setPosition(loc.getPosition() + Vector3d(WORLD_SCALE/2.,0,0));
            newEnt->getProxy().resetLocation(objnow, loc);
            newSelectedObjects.insert(newEnt->getProxyPtr());
            newEnt->setSelected(true);
            ent->setSelected(false);
        }
        mSelectedObjects.swap(newSelectedObjects);
    }

    //--------------------------------------------------------------------------
    void groupObjectsAction() {
        if (mSelectedObjects.size()<2) {
            return;
        }
        if (!mParent->mPrimaryCamera) {
            return;
        }
        SpaceObjectReference parentId = mCurrentGroup;

        ProxyManager *proxyMgr = mParent->mPrimaryCamera->getProxy().getProxyManager();
        Time now(SpaceTimeOffsetManager::getSingleton().now(mParent->mPrimaryCamera->getProxy().getObjectReference().space()));
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (!ent) continue;
            if (ent->getProxy().getProxyManager() != proxyMgr) {
                SILOG(input,error,"Attempting to group objects owned by different proxy manager!");
                return;
            }
            if (!(ent->getProxy().getParent() == parentId)) {
                SILOG(input,error,"Multiple select "<< ent->id() <<
                      " has parent  "<<ent->getProxy().getParent() << " instead of " << mCurrentGroup);
                return;
            }
        }
        Vector3d totalPosition (averageSelectedPosition(now, mSelectedObjects.begin(), mSelectedObjects.end()));
        Location totalLocation (totalPosition,Quaternion::identity(),Vector3f(0,0,0),Vector3f(0,0,0),0);
        Entity *parentEntity = mParent->getEntity(parentId);
        if (parentEntity) {
            totalLocation = parentEntity->getProxy().globalLocation(now);
            totalLocation.setPosition(totalPosition);
        }

        SpaceObjectReference newParentId = SpaceObjectReference(mCurrentGroup.space(), ObjectReference(UUID::random()));
        proxyMgr->createObject(ProxyObjectPtr(new ProxyMeshObject(proxyMgr, newParentId)));
        Entity *newParentEntity = mParent->getEntity(newParentId);
        newParentEntity->getProxy().resetLocation(now, totalLocation);

        if (parentEntity) {
            newParentEntity->getProxy().setParent(parentEntity->getProxyPtr(), now);
        }
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (!ent) continue;
            ent->getProxy().setParent(newParentEntity->getProxyPtr(), now);
            ent->setSelected(false);
        }
        mSelectedObjects.clear();
        mSelectedObjects.insert(newParentEntity->getProxyPtr());
        newParentEntity->setSelected(true);
    }

    //--------------------------------------------------------------------------
    bool doUngroupObjects(Task::LocalTime now) {
        int numUngrouped = 0;
        SelectedObjectSet newSelectedObjects;
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *parentEnt = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (!parentEnt) {
                continue;
            }
            ProxyManager *proxyMgr = parentEnt->getProxy().getProxyManager();
            ProxyObjectPtr parentParent (parentEnt->getProxy().getParentProxy());
            mCurrentGroup = parentEnt->getProxy().getParent(); // parentParent may be NULL.
            bool hasSubObjects = false;
            for (SubObjectIterator subIter (parentEnt); !subIter.end(); ++subIter) {
                hasSubObjects = true;
                Entity *ent = *subIter;
                ent->getProxy().setParent(parentParent, Time::convertFrom(now,SpaceTimeOffsetManager::getSingleton().getSpaceTimeOffset(ent->getProxy().getObjectReference().space())));
                newSelectedObjects.insert(ent->getProxyPtr());
                ent->setSelected(true);
            }
            if (hasSubObjects) {
                parentEnt->setSelected(false);
                proxyMgr->destroyObject(parentEnt->getProxyPtr());
                parentEnt = NULL; // dies.
                numUngrouped++;
            }
            else {
                newSelectedObjects.insert(parentEnt->getProxyPtr());
            }
        }
        mSelectedObjects.swap(newSelectedObjects);
        return (numUngrouped>0);
    }

    //--------------------------------------------------------------------------
    void ungroupObjectsAction() {
        Task::LocalTime now(Task::LocalTime::now());
        doUngroupObjects(now);
    }

    //--------------------------------------------------------------------------
    void enterObjectAction() {
        Task::LocalTime now(Task::LocalTime::now());
        if (mSelectedObjects.size() != 1) {
            return;
        }
        Entity *parentEnt = NULL;
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            parentEnt = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
        }
        if (parentEnt) {
            SelectedObjectSet newSelectedObjects;
            bool hasSubObjects = false;
            for (SubObjectIterator subIter (parentEnt); !subIter.end(); ++subIter) {
                hasSubObjects = true;
                Entity *ent = *subIter;
                newSelectedObjects.insert(ent->getProxyPtr());
                ent->setSelected(true);
            }
            if (hasSubObjects) {
                mSelectedObjects.swap(newSelectedObjects);
                mCurrentGroup = parentEnt->id();
            }
        }
    }

    //--------------------------------------------------------------------------
    void leaveObjectAction() {
        Task::LocalTime now(Task::LocalTime::now());
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *selent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (selent) {
                selent->setSelected(false);
            }
        }
        mSelectedObjects.clear();
        Entity *ent = mParent->getEntity(mCurrentGroup);
        if (ent) {
            mCurrentGroup = ent->getProxy().getParent();
            Entity *parentEnt = mParent->getEntity(mCurrentGroup);
            if (parentEnt) {
                mSelectedObjects.insert(parentEnt->getProxyPtr());
            }
        }
        else {
            mCurrentGroup = SpaceObjectReference::null();
        }
    }


    //--------------------------------------------------------------------------
    // If nothing is currently selected, throw a lightbulb in front of the user.
    // Otherwise, shine a spotlight on the selected object.
    //--------------------------------------------------------------------------
#define ATTACH_MESH_TO_LIGHTS 0

    void createLightAction() {
        if (mSelectedObjects.size() == 0) {
            float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();

            CameraEntity *camera = mParent->mPrimaryCamera;
            if (!camera) return;
            SpaceObjectReference newId = SpaceObjectReference(camera->id().space(), ObjectReference(UUID::random()));
            ProxyManager *proxyMgr = camera->getProxy().getProxyManager();
            Time now(SpaceTimeOffsetManager::getSingleton().now(newId.space()));

            Location loc (camera->getProxy().globalLocation(now));
            loc.setPosition(loc.getPosition() + Vector3d(direction(loc.getOrientation()))*WORLD_SCALE);
            loc.setOrientation(Quaternion(0.886995, 0.000000, -0.461779, 0.000000, Quaternion::WXYZ()));

            std::tr1::shared_ptr<ProxyLightObject> newLightObject (new ProxyLightObject(proxyMgr, newId));
            proxyMgr->createObject(newLightObject);
            {
                LightInfo li;
                li.setLightType(LightInfo::POINT);
                li.setLightDiffuseColor(Color(0.976471, 0.992157, 0.733333));
                li.setLightAmbientColor(Color(.24,.25,.18));
                li.setLightSpecularColor(Color(0,0,0));
                li.setLightShadowColor(Color(0,0,0));
                li.setLightPower(1.0);
                li.setLightRange(75);
                li.setLightFalloff(1,0,0.03);
                li.setLightSpotlightCone(30,40,1);
                li.setCastsShadow(true);
                /* set li according to some sample light in the scene file! */
                newLightObject->update(li);
            }

            Entity *ent = mParent->getEntity(newId);
            if (ent) {
#if ATTACH_MESH_TO_LIGHTS && 0
                // Attach a light bulb mesh
                LightBulb::AttachLightBulb(camera, ent, String("LightMesh" + newId.toString()).c_str(), loc);
#endif // ATTACH_MESH_TO_LIGHTS
                // FIXME: What good does it do to select a light if you can't do anything with it such as move it?
                ent->setSelected(true);
            }

            Entity *parentent = mParent->getEntity(mCurrentGroup);
            if (parentent) {
                Location localLoc = loc.toLocal(parentent->getProxy().globalLocation(now));
                newLightObject->setParent(parentent->getProxyPtr(), now, loc, localLoc);
                newLightObject->resetLocation(now, localLoc);
            }
            else {
                newLightObject->resetLocation(now, loc);
            }
            mSelectedObjects.clear();
            mSelectedObjects.insert(newLightObject);
        }
        else {
            lightSelected("", 0);
        }
    }


    //--------------------------------------------------------------------------
    ProxyObjectPtr getTopLevelParent(ProxyObjectPtr camProxy) {
        ProxyObjectPtr parentProxy;
        while ((parentProxy=camProxy->getParentProxy())) {
            camProxy=parentProxy;
        }
        return camProxy;
    }


    //--------------------------------------------------------------------------
    void toggleLightVisibilityAction() {
        bool foundFirstLight = false;
        bool visibility = true;
        OgreSystem::SceneEntitiesMap::const_iterator iter;
        for (OgreSystem::SceneEntitiesMap::const_iterator iter = mParent->mSceneEntities.begin();
            iter != mParent->mSceneEntities.end(); ++iter
        ) {
            Entity *ent = iter->second;
            ProxyObject *obj = ent->getProxyPtr().get();
            ProxyLightObject *light = dynamic_cast<ProxyLightObject*>(obj);
            if (light) {
                if (!foundFirstLight) {
                    visibility = !ent->getVisible();
                    foundFirstLight = true;
                }
                ent->setVisible(visibility);
            }
        }
    }


    //--------------------------------------------------------------------------
    void moveAction(Vector3f dir, float amount) {
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();

        ProxyObjectPtr cam = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
        if (!cam) return;

        Time now(SpaceTimeOffsetManager::getSingleton().now(cam->getObjectReference().space()));
        Location loc = cam->extrapolateLocation(now);
        const Quaternion &orient = loc.getOrientation();
        Protocol::ObjLoc rloc;
        rloc.set_velocity((orient * dir) * amount * WORLD_SCALE * mCamSpeed);
        rloc.set_angular_speed(0);
        cam->requestLocation(now, rloc);
    }


    //--------------------------------------------------------------------------
    void rotateAction(Vector3f about, float amount) {

        ProxyObjectPtr cam = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
        if (!cam) return;
        Time now(SpaceTimeOffsetManager::getSingleton().now(cam->getObjectReference().space()));
        Location loc = cam->extrapolateLocation(now);
        const Quaternion &orient = loc.getOrientation();

        Protocol::ObjLoc rloc;
        rloc.set_rotational_axis(about);
        rloc.set_angular_speed(amount*sqrt(mCamSpeed));
        cam->requestLocation(now, rloc);
    }


    //--------------------------------------------------------------------------
    void stableRotateAction(float dir, float amount) {

        ProxyObjectPtr cam = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
        if (!cam) return;
        Time now(SpaceTimeOffsetManager::getSingleton().now(cam->getObjectReference().space()));
        Location loc = cam->extrapolateLocation(now);
        const Quaternion &orient = loc.getOrientation();

        double p, r, y;
        quat2Euler(orient, p, r, y);
        Vector3f raxis;
        raxis.x = 0;
        raxis.y =  std::cos(p*DEG2RAD);
        raxis.z = -std::sin(p*DEG2RAD);

        Protocol::ObjLoc rloc;
        rloc.set_rotational_axis(raxis);
        rloc.set_angular_speed(dir*amount*sqrt(gMode=="funmode"? mCamSpeed*2:mCamSpeed));
        cam->requestLocation(now, rloc);
    }


    //--------------------------------------------------------------------------
    void setDragModeAction(const String& modename) {
        if (modename == "")
            mDragAction[1] = 0;
        mDragAction[1] = DragActionRegistry::get(modename);
    }


    //--------------------------------------------------------------------------
    void setCameraSpeed(const float& speed) {
        mCamSpeed = speed;
    }


    //--------------------------------------------------------------------------
    void importAction() {
        std::cout << "input path name for import: " << std::endl;
        std::string filename;
        // a bit of a cludge right now, type name into console.
        fflush(stdin);
        while (!feof(stdin)) {
            int c = fgetc(stdin);
            if (c == '\r') {
                c = fgetc(stdin);
            }
            if (c=='\n') {
                break;
            }
            if (c=='\033' || c <= 0) {
                std::cout << "<escape>\n";
                return;
            }
            std::cout << (unsigned char)c;
            filename += (unsigned char)c;
        }
        std::cout << '\n';
        std::vector<std::string> files;
        files.push_back(filename);
        mParent->mInputManager->filesDropped(files);
    }


    //--------------------------------------------------------------------------
    void saveSceneAction() {
        std::set<std::string> saveSceneNames;
        std::cout << "saving new scene as scene_new.csv: " << std::endl;
        FILE *output = fopen("scene_new.csv", "wt");
        if (!output) {
            perror("Failed to open scene_new.csv");
            return;
        }
        fprintf(output, "objtype,subtype,name,parent,script,scriptparams,");
        fprintf(output, "pos_x,pos_y,pos_z,orient_x,orient_y,orient_z,orient_w,");
        fprintf(output, "vel_x,vel_y,vel_z,rot_axis_x,rot_axis_y,rot_axis_z,rot_speed,");
        fprintf(output, "scale_x,scale_y,scale_z,hull_x,hull_y,hull_z,");
        fprintf(output, "density,friction,bounce,colMask,colMsg,meshURI,diffuse_x,diffuse_y,diffuse_z,ambient,");
        fprintf(output, "specular_x,specular_y,specular_z,shadowpower,");
        fprintf(output, "range,constantfall,linearfall,quadfall,cone_in,cone_out,power,cone_fall,shadow\n");
        OgreSystem::SceneEntitiesMap::const_iterator iter;
        vector<Entity*> entlist;
        entlist.clear();
        for (iter = mParent->mSceneEntities.begin(); iter != mParent->mSceneEntities.end(); ++iter) {
            entlist.push_back(iter->second);
        }
        std::sort(entlist.begin(), entlist.end(), compareEntity);
        for (unsigned int i=0; i<entlist.size(); i++)
            dumpObject(output, entlist[i], saveSceneNames);
        fclose(output);
    }


    //--------------------------------------------------------------------------
    bool quat2Euler(Quaternion q, double& pitch, double& roll, double& yaw) {
        /// note that in the 'gymbal lock' situation, we will get nan's for pitch.
        /// for now, in that case we should revert to quaternion
        double q1,q2,q3,q0;
        q2=q.x;
        q3=q.y;
        q1=q.z;
        q0=q.w;
        roll = std::atan2((2*((q0*q1)+(q2*q3))), (1-(2*(std::pow(q1,2.0)+std::pow(q2,2.0)))));
        pitch = (2*((q0*q2)-(q3*q1)));
        pitch = (pitch <= -1.0) ? -M_PI_2 :
                (pitch >= +1.0) ? +M_PI_2 :
                std::asin((pitch));
        yaw = std::atan2((2*((q0*q3)+(q1*q2))), (1-(2*(std::pow(q2,2.0)+std::pow(q3,2.0)))));
        pitch *= RAD2DEG;
        roll  *= RAD2DEG;
        yaw   *= RAD2DEG;
        if (std::abs(pitch) > 89.0) {
            return false;
        }
        return true;
    }


    //--------------------------------------------------------------------------
    string physicalName(ProxyMeshObject *obj, std::set<std::string> &saveSceneNames) {
        std::string name = obj->getPhysical().name;
        if (name.empty()) {
            name = obj->getMesh().filename();
            if (name.size() < 5) {
                name = "NONAME_NOMESH";
            }
            else {
                name.resize(name.size()-5);
            }
            //name += ".0";
        }
//        if (name.find(".") < name.size()) {             /// remove any enumeration
//            name.resize(name.find("."));
//        }
        int basesize = name.size();
        int count = 1;
        while (saveSceneNames.count(name)) {
            name.resize(basesize);
            std::ostringstream os;
            os << name << "." << count;
            name = os.str();
            count++;
        }
        saveSceneNames.insert(name);
        return name;
    }


    //--------------------------------------------------------------------------
    void dumpObject(FILE* fp, Entity* e, std::set<std::string> &saveSceneNames) {
        ProxyObject *pp = e->getProxyPtr().get();
        Time now(SpaceTimeOffsetManager::getSingleton().now(pp->getObjectReference().space()));
        Location loc = pp->globalLocation(now);
        ProxyCameraObject* camera = dynamic_cast<ProxyCameraObject*>(pp);
        ProxyLightObject* light = dynamic_cast<ProxyLightObject*>(pp);
        ProxyMeshObject* mesh = dynamic_cast<ProxyMeshObject*>(pp);

        double x,y,z;
        std::string w("");
        /// if feasible, use Eulers: (not feasible == potential gimbal confusion)
        /// forcing Quats for now because I need quaternions for initial condition reset
        if (1) {//(!quat2Euler(loc.getOrientation(), x, z, y)) {
            x=loc.getOrientation().x;
            y=loc.getOrientation().y;
            z=loc.getOrientation().z;
            std::stringstream temp;
            temp << loc.getOrientation().w;
            w = temp.str();
        }

        Vector3f angAxis(loc.getAxisOfRotation());
        float angSpeed(loc.getAngularSpeed());

        string parent;
        ProxyObjectPtr parentObj = pp->getParentProxy();
        if (parentObj) {
            ProxyMeshObject *parentMesh = dynamic_cast<ProxyMeshObject*>(parentObj.get());
            if (parentMesh) {
                parent = physicalName(parentMesh, saveSceneNames);
            }
        }
        if (light) {
            const char *typestr = "directional";
            const LightInfo &linfo = light->getLastLightInfo();
            if (linfo.mType == LightInfo::POINT) {
                typestr = "point";
            }
            if (linfo.mType == LightInfo::SPOTLIGHT) {
                typestr = "spotlight";
            }
            float32 ambientPower, shadowPower;
            ambientPower = LightEntity::computeClosestPower(linfo.mDiffuseColor, linfo.mAmbientColor, linfo.mPower);
            shadowPower = LightEntity::computeClosestPower(linfo.mSpecularColor, linfo.mShadowColor,  linfo.mPower);
            fprintf(fp, "light,%s,,%s,,,%f,%f,%f,%f,%f,%f,%s,%f,%f,%f,%f,%f,%f,%f,,,,,,,,,,,,,",typestr,parent.c_str(),
                    loc.getPosition().x,loc.getPosition().y,loc.getPosition().z,x,y,z,w.c_str(),
                    loc.getVelocity().x, loc.getVelocity().y, loc.getVelocity().z, angAxis.x, angAxis.y, angAxis.z, angSpeed);
            fprintf(fp, "%f,%f,%f,%f,%f,%f,%f,%f,%lf,%f,%f,%f,%f,%f,%f,%f,%d\n",
                    linfo.mDiffuseColor.x,linfo.mDiffuseColor.y,linfo.mDiffuseColor.z,ambientPower,
                    linfo.mSpecularColor.x,linfo.mSpecularColor.y,linfo.mSpecularColor.z,shadowPower,
                    linfo.mLightRange,linfo.mConstantFalloff,linfo.mLinearFalloff,linfo.mQuadraticFalloff,
                    linfo.mConeInnerRadians,linfo.mConeOuterRadians,linfo.mPower,linfo.mConeFalloff,
                    (int)linfo.mCastsShadow);
        }
        else if (mesh) {
            URI uri = mesh->getMesh();
            std::string uristr = uri.toString();
            if (uri.proto().empty()) {
                uristr = "";
            }
            const PhysicalParameters &phys = mesh->getPhysical();
            std::string subtype;
            switch (phys.mode) {
            case PhysicalParameters::Disabled:
                subtype="graphiconly";
                break;
            case PhysicalParameters::Static:
                subtype="staticmesh";
                break;
            case PhysicalParameters::DynamicBox:
                subtype="dynamicbox";
                break;
            case PhysicalParameters::DynamicSphere:
                subtype="dynamicsphere";
                break;
            case PhysicalParameters::DynamicCylinder:
                subtype="dynamiccylinder";
                break;
            case PhysicalParameters::Character:
                subtype="character";
                break;
            default:
                std::cout << "unknown physical mode! " << (int)phys.mode << std::endl;
            }
            std::string name = physicalName(mesh, saveSceneNames);
            fprintf(fp, "mesh,%s,%s,%s,,,%f,%f,%f,%f,%f,%f,%s,%f,%f,%f,%f,%f,%f,%f,",subtype.c_str(),name.c_str(),parent.c_str(),
                    loc.getPosition().x,loc.getPosition().y,loc.getPosition().z,x,y,z,w.c_str(),
                    loc.getVelocity().x, loc.getVelocity().y, loc.getVelocity().z, angAxis.x, angAxis.y, angAxis.z, angSpeed);

            fprintf(fp, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%s\n",
                    mesh->getScale().x,mesh->getScale().y,mesh->getScale().z,
                    phys.hull.x, phys.hull.y, phys.hull.z,
                    phys.density, phys.friction, phys.bounce, phys.colMask, phys.colMsg, uristr.c_str());
        }
        else if (camera) {
            fprintf(fp, "camera,,,%s,,,%f,%f,%f,%f,%f,%f,%s,%f,%f,%f,%f,%f,%f,%f\n",parent.c_str(),
                    loc.getPosition().x,loc.getPosition().y,loc.getPosition().z,x,y,z,w.c_str(),
                    loc.getVelocity().x, loc.getVelocity().y, loc.getVelocity().z, angAxis.x, angAxis.y, angAxis.z, angSpeed);
        }
        else {
            fprintf(fp, "#unknown object type in dumpObject\n");
        }
    }


    //--------------------------------------------------------------------------
    void zoomAction(float value, Vector2f axes) {
        zoomInOut(value, axes, mParent->mPrimaryCamera, mSelectedObjects, mParent);
    }

    ///// Top Level Input Event Handlers //////


    //--------------------------------------------------------------------------
    EventResponse keyHandler(EventPtr ev) {
        std::tr1::shared_ptr<ButtonEvent> buttonev (
            std::tr1::dynamic_pointer_cast<ButtonEvent>(ev));
        if (!buttonev) {
            return EventResponse::nop();
        }
    
        // Give the browsers a chance to use this input first
        // but some kbd stuff should only go to ogre (not sure how, but these keys still get to webview)
        if (    buttonev->mButton != SDL_SCANCODE_UP &&
                buttonev->mButton != SDL_SCANCODE_DOWN &&
                buttonev->mButton != SDL_SCANCODE_LEFT &&
                buttonev->mButton != SDL_SCANCODE_RIGHT &&
                buttonev->mButton != SDL_SCANCODE_PAGEUP &&
                buttonev->mButton != SDL_SCANCODE_PAGEDOWN &&
                buttonev->mButton != SDL_SCANCODE_SPACE &&
                buttonev->mButton != SDL_SCANCODE_Q
           ) {
            EventResponse browser_resp = WebViewManager::getSingleton().onButton(buttonev);
            if (browser_resp == EventResponse::cancel()) {
                return EventResponse::cancel();
            }
        }
    
        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);
    
        return EventResponse::nop();
    }


    //--------------------------------------------------------------------------
    EventResponse axisHandler(EventPtr ev) {
        std::tr1::shared_ptr<AxisEvent> axisev (
            std::tr1::dynamic_pointer_cast<AxisEvent>(ev));
        if (!axisev)
            return EventResponse::nop();

        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);

        return EventResponse::cancel();
    }


    //--------------------------------------------------------------------------
    EventResponse textInputHandler(EventPtr ev) {
        std::tr1::shared_ptr<TextInputEvent> textev (
            std::tr1::dynamic_pointer_cast<TextInputEvent>(ev));
        if (!textev)
            return EventResponse::nop();

        // Give the browsers a chance to use this input first
        EventResponse browser_resp = WebViewManager::getSingleton().onKeyTextInput(textev);
        if (browser_resp == EventResponse::cancel())
            return EventResponse::cancel();

        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);

        return EventResponse::nop();
    }


    //--------------------------------------------------------------------------
    EventResponse mouseHoverHandler(EventPtr ev) {
        std::tr1::shared_ptr<MouseHoverEvent> mouseev (
            std::tr1::dynamic_pointer_cast<MouseHoverEvent>(ev));
        if (!mouseev)
            return EventResponse::nop();

        // Give the browsers a chance to use this input first
        EventResponse browser_resp = WebViewManager::getSingleton().onMouseMove(mouseev);
        if (browser_resp == EventResponse::cancel())
            return EventResponse::cancel();

        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);

        return EventResponse::nop();
    }


    //--------------------------------------------------------------------------
    EventResponse mousePressedHandler(EventPtr ev) {
        std::tr1::shared_ptr<MousePressedEvent> mouseev (
            std::tr1::dynamic_pointer_cast<MousePressedEvent>(ev));
        if (!mouseev)
            return EventResponse::nop();

        // Give the browsers a chance to use this input first
        EventResponse browser_resp = WebViewManager::getSingleton().onMousePressed(mouseev);
        if (browser_resp == EventResponse::cancel()) {
            mWebViewActiveButtons.insert(mouseev->mButton);
            return EventResponse::cancel();
        }

        selectObjectAction(Vector2f(mouseev->mX, mouseev->mY), 1);
        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);

        return EventResponse::nop();
    }


    //--------------------------------------------------------------------------
    EventResponse mouseClickHandler(EventPtr ev) {
        std::tr1::shared_ptr<MouseClickEvent> mouseev (
            std::tr1::dynamic_pointer_cast<MouseClickEvent>(ev));
        if (!mouseev)
            return EventResponse::nop();

        // Give the browsers a chance to use this input first
        EventResponse browser_resp = WebViewManager::getSingleton().onMouseClick(mouseev);

        if (mWebViewActiveButtons.find(mouseev->mButton) != mWebViewActiveButtons.end()) {
            mWebViewActiveButtons.erase(mouseev->mButton);
            return EventResponse::cancel();
        }
        if (browser_resp == EventResponse::cancel()) {
            return EventResponse::cancel();
        }
        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);

        return EventResponse::nop();
    }


    //--------------------------------------------------------------------------
    EventResponse mouseDragHandler(EventPtr evbase) {
        MouseDragEventPtr ev (std::tr1::dynamic_pointer_cast<MouseDragEvent>(evbase));
        if (!ev) {
            return EventResponse::nop();
        }

        std::set<int>::iterator iter = mWebViewActiveButtons.find(ev->mButton);
        if (iter != mWebViewActiveButtons.end()) {
            // Give the browser a chance to use this input
            EventResponse browser_resp = WebViewManager::getSingleton().onMouseDrag(ev);

            if (ev->mType == Input::DRAG_END) {
                mWebViewActiveButtons.erase(iter);
            }
            if (browser_resp == EventResponse::cancel()) {
                return EventResponse::cancel();
            }
        }

        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(evbase));
        mInputBinding.handle(inputev);

        ActiveDrag * &drag = mActiveDrag[ev->mButton];
        if (ev->mType == Input::DRAG_START) {
            if (drag) {
                delete drag;
            }
            DragStartInfo info = {
                /*.sys = */ mParent,
                /*.camera = */ mParent->mPrimaryCamera, // for now...
                /*.objects = */ mSelectedObjects,
                /*.ev = */ ev
            };
            if (mDragAction[ev->mButton]) {
                drag = mDragAction[ev->mButton](info);
            }
        }
        if (drag) {
            drag->mouseMoved(ev);

            if (ev->mType == Input::DRAG_END) {
                delete drag;
                drag = 0;
            }
        }
        return EventResponse::nop();
    }


    //--------------------------------------------------------------------------
    EventResponse webviewHandler(EventPtr ev) {
        WebViewEventPtr webview_ev (std::tr1::dynamic_pointer_cast<WebViewEvent>(ev));
        if (!webview_ev)
            return EventResponse::nop();

        // For everything else we let the browser go first, but in this case it should have
        // had its chance, so we just let it go
        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);

        return EventResponse::nop();
    }


    /// Camera Path Utilities
#define CAMERA_PATH_FILE "camera_path.csv"

    //--------------------------------------------------------------------------
    void cameraPathSetCamera(const Vector3d& pos, const Quaternion& orient) {

        ProxyObjectPtr cam = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
        if (!cam) return;
        Time now(SpaceTimeOffsetManager::getSingleton().now(cam->getObjectReference().space()));
        Location loc = cam->extrapolateLocation(now);

        loc.setPosition( pos );
        loc.setOrientation( orient );
        loc.setVelocity(Vector3f(0,0,0));
        loc.setAngularSpeed(0);

        cam->setLocation(now, loc);
    }

    //--------------------------------------------------------------------------
    void cameraPathSetToKeyFrame(uint32 idx) {
        if (idx < 0 || idx >= mCameraPath.numPoints()) return;

        CameraPoint keypoint = mCameraPath[idx];
        cameraPathSetCamera(mCameraPath[idx].position, mCameraPath[idx].orientation);
        mCameraPathTime = mCameraPath[idx].time;
    }

    //--------------------------------------------------------------------------
    void cameraPathLoad() {
        std::cout << "dbm debug: cameraPathLoad" << std::endl;
        mCameraPath.load(CAMERA_PATH_FILE);
    }

    //--------------------------------------------------------------------------
    void cameraPathSave() {
        std::cout << "dbm debug: cameraPathSave" << std::endl;
        mCameraPath.save(CAMERA_PATH_FILE);
    }

    //--------------------------------------------------------------------------
    void cameraPathNext() {
        mCameraPathIndex = mCameraPath.clampKeyIndex(mCameraPathIndex+1);
        cameraPathSetToKeyFrame(mCameraPathIndex);
    }

    //--------------------------------------------------------------------------
    void cameraPathPrevious() {
        mCameraPathIndex = mCameraPath.clampKeyIndex(mCameraPathIndex-1);
        cameraPathSetToKeyFrame(mCameraPathIndex);
    }

    //--------------------------------------------------------------------------
    void cameraPathInsert() {
        std::cout << "dbm debug: cameraPathInsert" << std::endl;

        ProxyObjectPtr cam = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
        if (!cam) return;
        Time now(SpaceTimeOffsetManager::getSingleton().now(cam->getObjectReference().space()));
        Location loc = cam->extrapolateLocation(now);

        mCameraPathIndex = mCameraPath.insert(mCameraPathIndex, loc.getPosition(), loc.getOrientation(), Task::DeltaTime::seconds(1.0), "");
        mCameraPathTime = mCameraPath.keyFrameTime(mCameraPathIndex);
    }

    //--------------------------------------------------------------------------
    void cameraPathDelete() {
        mCameraPathIndex = mCameraPath.remove(mCameraPathIndex);
        mCameraPathTime = mCameraPath.keyFrameTime(mCameraPathIndex);
    }

    //--------------------------------------------------------------------------
    void cameraPathRun() {
        mRunningCameraPath = !mRunningCameraPath;
        if (mRunningCameraPath) {
            std::cout << "dbm debug: cameraPathRun start" << std::endl;
            mCameraPathTime = Task::DeltaTime::zero();
        }
        else {
            std::cout << "dbm debug: cameraPathRun end" << std::endl;
        }
    }

    //--------------------------------------------------------------------------
    void cameraPathChangeSpeed(float factor) {
        mCameraPath.changeTimeDelta(mCameraPathIndex, Task::DeltaTime::seconds(factor));
    }

    //--------------------------------------------------------------------------
    void cameraPathTick(const Task::LocalTime& t) {
        Task::DeltaTime dt = t - mLastCameraTime;
        mLastCameraTime = t;

        if (mRunningCameraPath) {
            mCameraPathTime += dt;

            Vector3d pos;
            Quaternion orient;
            static String oldmsg;
            static String oldAnim;
            String msg, animation;
            int tx, ty;
            bool success = mCameraPath.evaluate(mCameraPathTime, &pos, &orient, msg, &tx, &ty, animation);
            if (msg != oldmsg && msg != "") {
                std::ostringstream ss;
                //ss << "document.selected='" << msg << "'; debug(document.selected);";
                ss << "popUpMessage(" << '"' << msg << '"' << "," << tx << ","<< ty << ");";
                std::cout << "cameraPathTick msg to JS:" << ss.str() << std::endl;
                WebViewManager::getSingleton().evaluateJavaScript("__chrome", ss.str());
            }
            oldmsg=msg;

            if (animation != oldAnim && animation != "") {
                std::cout << "dbm debug animate: " << animation << std::endl;
                ProxyObjectPtr cam = mParent->mPrimaryCamera->getProxyPtr();
                assert(cam);
                RoutableMessageBody msg;
                ostringstream ss;
                ss << "flythru animation " << animation;
                msg.add_message("JavascriptMessage", ss.str());
                String smsg;
                msg.SerializeToString(&smsg);
                cam->sendMessage(MemoryReference(smsg));
            }
            oldAnim=animation;
            if (!success) {
            /// That was fun! Let's do it again!
                std::cout << "dbm debug: flythru again" << std::endl;
                WebViewManager::getSingleton().evaluateJavaScript("__chrome",
                                             "setTimeout('flythruMode(); ', 5000);"
                                                                 );
                mRunningCameraPath = false;
                return;
            }

            cameraPathSetCamera(pos, orient);
        }
    }

    /// Bornholm actions

    /// fun mode

    //--------------------------------------------------------------------------
    void fireAction() {
        ProxyObjectPtr cam = mParent->mPrimaryCamera->getProxyPtr();
        assert(cam);
        RoutableMessageBody msg;
        ostringstream ss;
        ss << "funmode fire ammo_cannon ";
        ProxyObjectPtr avatar = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
        assert(avatar);
        Time now(SpaceTimeOffsetManager::getSingleton().now(avatar->getObjectReference().space()));
        Location location(avatar->globalLocation(now));
        Vector3d pos = location.getPosition();
        Quaternion rot = location.getOrientation();
        Vector3f z_axis = rot.zAxis();
        ss << pos.x <<" "<< pos.y <<" "<< pos.z <<" "<< rot.x <<" "<< rot.y <<" "<< rot.z <<" "<< rot.w
                <<" "<< z_axis.x <<" "<< z_axis.y <<" "<< z_axis.z;
        msg.add_message("JavascriptMessage", ss.str());
        String smsg;
        msg.SerializeToString(&smsg);
//        cam->sendMessage(MemoryReference(smsg));
        avatar->sendMessage(MemoryReference(smsg));
    }

    //--------------------------------------------------------------------------
    void resetAction() {
        /// fun mode has script on avatar
        ProxyObjectPtr cam = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
            
        /// other modes just have camera script
        ProxyObjectPtr cam2 = mParent->mPrimaryCamera->getProxyPtr();
        
        assert(cam);
        RoutableMessageBody msg;
        ostringstream ss;
        ss << "reset";
        ProxyObjectPtr avatar = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
        assert(avatar);
        msg.add_message("JavascriptMessage", ss.str());
        String smsg;
        msg.SerializeToString(&smsg);
        cam->sendMessage(MemoryReference(smsg));
        cam2->sendMessage(MemoryReference(smsg));
    }

    /// curator mode

    //--------------------------------------------------------------------------
    void debugAction(int num) {
        ProxyObjectPtr cam = mParent->mPrimaryCamera->getProxyPtr();
        assert(cam);
        RoutableMessageBody msg;
        ostringstream ss;
        if (num==1) {
            ss << "inventory loadState 82c60703a082460713a065e616d656a00723a06516274777f627b6f50343a00733a0376507f637a00743a08264d273e2535303030303139303733343836333a064d223e21313939393938383535353930383a06433e21393030303030353732323034363a0470753a0376527f647a00763a08264d203e27303731303637363930383439333034323a06403e203a06403e203a06403e27303731303637393332383831363433393a0470773a0371682460783a07623a06516274777f627b6f50333a00793a0377643a08264d273e24363939393937393031393136353a064d213e2037303030303035323435323038373a064d203e2035393939393939383635383839353439333a047071303a0377663a0826403e203a064d203e27333133353337303031363039383032323a06403e203a06403e26383139393833363136323632313630383a047071313a037168246071323a07623a06516274777f627b6f50323a0071333a0377643a08264d293e2334303030303135323538373839313a064d213e203a06413e253a047071343a0377663a0826403e20303031373434323636303436343433393534313a064d203e2033343839393439353534323034393430383a06463e20393131313036363239363436333833356d2030363a06403e29393933393038313138313934363435343a047071353a0371665465637362796074796f6e6c20216c637f6027796478602370716365637024594d454354514d405a364279602355607021383021303a34353a333730223030393a0071363a016e2";
            msg.add_message("JavascriptMessage", ss.str());
            String smsg;
            msg.SerializeToString(&smsg);
            cam->sendMessage(MemoryReference(smsg));
        }
        else if (num==2) {
            initLightMoods();
            lightSelectMood("0", 0);
        }
        
        if (num==3) {
            /// place art
            static int pic=0;
            pic++;
            WebViewManager::NavigationAction act;
            ostringstream ss;
            ss << "inventory placeObject artwork_0" << pic << " 400 300";
            inventoryHandler(act, ss.str());
        }
        
        if (num==-1) {
            mParent->quitRequest=true;
        }
    }

    /// WebView Actions

    //--------------------------------------------------------------------------
    void webViewNavigateAction(WebViewManager::NavigationAction action) {
        WebViewManager::getSingleton().navigate(action);
    }

    //--------------------------------------------------------------------------
    void webViewNavigateStringAction(WebViewManager::NavigationAction action, const String& arg) {
        WebViewManager::getSingleton().navigate(action, arg);
    }

    //--------------------------------------------------------------------------
    void inventoryHandler(WebViewManager::NavigationAction action, const String& arg) {
        ProxyObjectPtr cam = mParent->mPrimaryCamera->getProxyPtr();
        if (!cam) return;
        RoutableMessageBody msg;
        String tok;
        size_t pos=0;
        double mx, my;
        std::ostringstream fullmsg;
        getNextToken(arg, &pos, &tok);          //  inventory
        getNextToken(arg, &pos, &tok);          //  placeObject, saveState, loadState
        if (tok=="placeObject") {
            getNextToken(arg, &pos, &tok);          //  artwork_xx
            getNextTokenAsDouble(arg, &pos, &mx);   // mouse x
            getNextTokenAsDouble(arg, &pos, &my);   // mouse y
            double width  = mParent->mPrimaryCamera->getViewport()->getActualWidth();
            double height = mParent->mPrimaryCamera->getViewport()->getActualHeight();
            mx = (mx-width*.5) / (width*.5);
            my = -((my-20)-height*.5) / (height*.5);    // 20 pixels for title bar?
            Vector3d position;
            Quaternion orientation;
            bool err=getPositionAndOrientationForNewArt(mx, my, 0.1, false, &position, &orientation);
            if (!err) {
                std::cout << "dbm debug ERROR --getPos-etc failed" << std::endl;
                position = Vector3d(0,2,0);
            }
            fullmsg << arg << " " << position.x <<" "<< position.y <<" "<< position.z <<" "<<
            orientation.x <<" "<< orientation.y <<" "<< orientation.z <<" "<< orientation.w;
        }
        else if (tok=="saveState") {
            fullmsg << arg << " lightmood " << mLastMoodSelected; //getMoodStringForSaving();
        }
        else if (tok=="loadState") {
            fullmsg << arg;
        }
        msg.add_message("JavascriptMessage", fullmsg.str());
        String smsg;
        msg.SerializeToString(&smsg);
        cam->sendMessage(MemoryReference(smsg));
    }

    void resetHandler(WebViewManager::NavigationAction action, const String& arg) {
        resetAction();
        if (arg == "reset flythru") {
            cameraPathRun();
        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // Utilities for parsing text tokens
    ////////////////////////////////////////////////////////////////////////////


    //--------------------------------------------------------------------------
    // Get the next token or string. If the token starts with a quote, it is matched with
    // an ending quote, and the string in between is returned. Escaped quotes are recognized
    // in the middle of such a string.
    //
    // Parameters:
    //   - str    = the string to be searched
    //   - *pos   = the position in the string to start the search,
    //              updated to the position after the found token after the call
    //   - *token = found token is stored here
    // Returns true if a token was found, false otherwise.
    //--------------------------------------------------------------------------

    static bool getNextToken(const String &str, size_t *pos, String *token) {
        if (*pos >= str.length())
            return false;
        size_t first = str.find_first_not_of(mWhiteSpace, *pos);
        if (first == String::npos)
            return false;
        size_t last = first;
        if (str[first] == '"' || str[first] == '\'') {              // Quoted token
            char quoteChar = str[first++];
            last = str.find_first_of(quoteChar, first);
            while (last != String::npos && str[last - 1] == '\\')   // Quoted quote
                last = str.find_first_of(quoteChar, last + 1);
            if (last == String::npos)
                *pos = last = str.length();
            else
                *pos = last + 1;
        }
        else {                                                      // Unquoted token
            last = str.find_first_of(mWhiteSpace, first);
            if (last == String::npos)
                last = str.length();
            *pos = last;
        }
        *token = str.substr(first, last - first);
        return true;
    }


    //--------------------------------------------------------------------------
    // Get the next double.
    //
    // Parameters:
    //   - str    = the string to be searched
    //   - *pos   = the position in the string to start the search,
    //              updated to the position after the found number after the call
    //   - *d     = found number is store here. Note that NaNs are not recognized.
    // Returns true if a floating-point number was found, false otherwise.
    //--------------------------------------------------------------------------

    static bool getNextTokenAsDouble(const String &str, size_t *pos, double *d) {
        const char *start = str.c_str() + *pos;
        char *end;
        *d = strtod(start, &end);
        *pos = end - str.c_str();
        return start != end;
    }


    //--------------------------------------------------------------------------
    // Get the next float.
    //
    // Parameters:
    //   - str    = the string to be searched
    //   - *pos   = the position in the string to start the search,
    //              updated to the position after the found number after the call
    //   - *d     = found number is store here. Note that NaNs are not recognized.
    // Returns true if a floating-point number was found, false otherwise.
    //--------------------------------------------------------------------------

    static bool getNextTokenAsFloat(const String &str, size_t *pos, float *f) {
        const char *start = str.c_str() + *pos;
        char *end;
        *f = strtof(start, &end);
        *pos = end - str.c_str();
        return start != end;
    }


    //--------------------------------------------------------------------------
    // Get the next long integer.
    //
    // Parameters:
    //   - str    = the string to be searched
    //   - *pos   = the position in the string to start the search,
    //              updated to the position after the found integer after the call
    //   - *d     = found integer is store here.
    // Returns true if an integer was found, false otherwise.
    //--------------------------------------------------------------------------

    static bool getNextTokenAsLong(const String &str, size_t *pos, long *i) {
        const char *start = str.c_str() + *pos;
        char *end;
        *i = strtol(start, &end, 10);
        *pos = end - str.c_str();
        return start != end;
    }


    //--------------------------------------------------------------------------
    // Get the next integer.
    //
    // Parameters:
    //   - str    = the string to be searched
    //   - *pos   = the position in the string to start the search,
    //              updated to the position after the found integer after the call
    //   - *d     = found integer is store here.
    // Returns true if an integer was found, false otherwise.
    //--------------------------------------------------------------------------

    static bool getNextTokenAsInt(const String &str, size_t *pos, int *i) {
    #if LONG_MAX == INT_MAX
        return getNextTokenAsLong(str, pos, reinterpret_cast<long*>(i));
    #else // LONG_MAX != INT_MAX
        long l;
        bool success = getNextTokenAsLong(str, pos, &l);
        *i = l;
        return success;
    #endif // LONG_MAX != INT_MAX
    }


    //--------------------------------------------------------------------------
    // Make a substring between the first '{' and its matching '}', advancing the caret.
    // Typical use parses a string of the form:
    //    [ { ... }, { ... }, ... { ... } ]
    //
    // Parameters:
    //   - str    = the string to be searched
    //   - *pos   = the position in the string to start the search,
    //              updated to the position after the found integer after the call
    //   - *d     = found object is store here.
    // Returns true is an object was found, false otherwise.
    //--------------------------------------------------------------------------

    static bool getNextObject(const String& arg, size_t *pos, String *obj) {
        const char *argcstr = arg.c_str();
        const char *s = argcstr + *pos;
        s += strspn(s, mArraySpace);
        if (*s != '{')
            return false;
        const char *start = s++, *end;
        int nest = 1;
        for (; *s != 0; ++s) {
            switch (*s) {
                case '{':
                   ++nest;
                   break;
                case '}':
                    if (--nest == 0) {
                        end = s + 1;
                        *obj = arg.substr(start - argcstr, end - start);
                        *pos = end - argcstr;
                        return true;
                    }
                    break;
            }
        }
        return false;
    }


    //--------------------------------------------------------------------------
    // walk [ straight | turn ] speed
    //--------------------------------------------------------------------------

    void walkHandler(WebViewManager::NavigationAction action, const String& arg) {
        String token;
        size_t ix = 0;
        bool success = true, rotate = false;
        double speed;
        getNextToken(arg, &ix, &token);                                 // walk (already parsed)
        success = success && getNextToken(arg, &ix, &token);            // straight | turn
        if      (token == "straight")   rotate = false;
        else if (token == "turn")       rotate = true;
        else                            success = false;
        success = success && getNextTokenAsDouble(arg, &ix, &speed);    // distance
        if (!success)
            return;

        if (!rotate)    moveAction(Vector3f(0, 0, -1), speed);
        else            stableRotateAction(1, speed);
    }


    //--------------------------------------------------------------------------
    // step [ straight | turn ] distance
    //--------------------------------------------------------------------------

    void stepHandler(WebViewManager::NavigationAction action, const String& arg) {
        String token;
        size_t ix = 0;
        bool success = true, rotate = false;
        double distance;
        getNextToken(arg, &ix, &token);                                 // step (already parsed)
        success = success && getNextToken(arg, &ix, &token);            // straight | turn
        if      (token == "straight")   rotate = false;
        else if (token == "turn")       rotate = true;
        else                            success = false;
        success = success && getNextTokenAsDouble(arg, &ix, &distance); // distance
        if (!success)
            return;

        // FIXME: This should move by the specified amount, but below we incorrectly set the speed.
        if (!rotate)    moveAction(Vector3f(0, 0, -1), distance);
        else            stableRotateAction(1, distance);
    }


    //--------------------------------------------------------------------------
    // Determine the appropriate position and orientation for a piece of artwork
    // to be placed at the given mouse coordinates.
    //
    // Given a point in screen space, fire a ray to find the first object hit.
    // Compute the location and normal of the intersection with the object.
    // Offset the intersection location by the specified surfaceOffset in the
    // outward direction of the surface normal. Orientation is computed
    // appropriate for a painting or a sculpture. It is assumed that the struck
    // surface is vertical for a painting and horizontal for a sculpture. It is
    // also assumed that paintings are done in the X-Y plane, and that a
    // sculpture's vertical axis is the Y axis, with the primary view points its
    // Z axis toward the user.
    //--------------------------------------------------------------------------

    bool getPositionAndOrientationForNewArt(
        double screenX, double screenY, // Location on screen
        double surfaceOffset,           // Optional offset from surface of ray intersection
        bool isSculpture,               // Is the artwork a painting or a sculpture?
        Vector3d *position,             // Position for the new artwork
        Quaternion *orientation         // Orientation of the new artwork
    ) {
        // Look for an object intersected by the ray.
        ProxyObjectPtr camera = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
        if (!camera)
            return false;
        Time now(SpaceTimeOffsetManager::getSingleton().now(camera->getObjectReference().space()));
        Location location(camera->globalLocation(now));
        Vector3f viewDirection(pixelToDirection(mParent->mPrimaryCamera, location.getOrientation(), screenX, screenY));
        double distance;
        Vector3f normal(0, 0, 0);
        bool success = false;
        int numHits = 1, i;
        for (i = 0; i < numHits; i++) {
            const Entity *obj = mParent->rayTrace(location.getPosition(), viewDirection, numHits, distance, normal, i);
            if (obj == NULL) {      // No object found
                if (i < numHits)    // Why not?
                    continue;       // Still more objects: keep looking
                break;              // No more objects: return failure
            }
            success = true;
            break;
        }
        if (!success || !(normal.normalizeThis() > 0))
            return false;

        // Compute the position, offset from the surface by the specified amount
        *position = location.getPosition() + distance * Vector3d(viewDirection.x, viewDirection.y, viewDirection.z);
        if (viewDirection.dot(normal) > 0)  // backfacing normal
            normal = -normal;               // make it front-facing
        *position += surfaceOffset * Vector3d(normal.x, normal.y, normal.z);

        // Compute the orientation
        Vector3f xAxis, yAxis, zAxis;
        if (!isSculpture) {                         //------ A Painting ------//
            zAxis = normal;                         // The painting is flush against the wall
            xAxis = Vector3f::unitY().cross(zAxis);
            if (xAxis.normalizeThis() == 0) {       // Oops: hit a horizontal surface
                xAxis = viewDirection.cross(Vector3f::unitY()); // Face the viewer
                if (xAxis.normalizeThis() == 0)     // Oops: looking straight up or down
                    xAxis = Vector3f::unitX();      // Align to the world's coordinate axes, since the normal and view are pathological
            }
            yAxis = zAxis.cross(xAxis);
        }
        else {                                      //------ A Sculpture ------//
            yAxis = Vector3f::unitY();              // Sculpture always stands upright
            xAxis = viewDirection.cross(yAxis);     // Point toward the viewer
            if (xAxis.normalizeThis() == 0)         // Oops: looking straight up or down
                xAxis = Vector3f::unitX();          // Align to the world's coordinate axes, since the current view is pathological
            zAxis = xAxis.cross(yAxis);
        }
        *orientation = Quaternion(xAxis, yAxis, zAxis);
        return true;
    }

    //--------------------------------------------------------------------------
    // Return to Javascript the id of the currently select item, or the first one
    // if there are multiple selections.
    //--------------------------------------------------------------------------

    void getSelectedIDHandler(WebViewManager::NavigationAction action = WebViewManager::NavigateCommand, const String& arg = "") {
        // Neither the action nor the are are used, yet.
        String id;
        if (mSelectedObjects.size() > 0) {
            SelectedObjectSet::const_iterator it = mSelectedObjects.begin();
            ProxyObjectPtr obj(it->lock());
            // id = obj->getObjectReference().toString();
            id = "\"" + dynamic_cast<ProxyMeshObject*>(obj.get())->getPhysical().name + "\"";
        }
        else {
            id = "null";
        }
        WebViewManager::getSingleton().evaluateJavaScript("__chrome",
                                                          "debug('" + id + "');" +
                                                          "pictureSelected(" + id + ");"
        );
    }


    //--------------------------------------------------------------------------
    // Case-insensitive string comparison
    //--------------------------------------------------------------------------

    static bool strcaseeq(const char *s1, const char *s2) {
        #ifdef _WIN32
            return stricmp(s1, s2) == 0;
        #else // _WIN32
            return strcasecmp(s1, s2) == 0;
        #endif // _WIN32
    }

    static bool strncaseeq(const char *s1, const char *s2, size_t n) {
        #ifdef _WIN32
            return strnicmp(s1, s2, n) == 0;
        #else // _WIN32
            return strncasecmp(s1, s2, n) == 0;
        #endif // _WIN32
    }


    //--------------------------------------------------------------------------
    // This provides an easy and lightweight way to get values from a string of the form
    //    name=value name=value ...
    // or
    //    name:value name:value ...
    // Suitable for parsing element atributes or JSON members.
    //
    // Usage:
    //    JavascriptArgumentParser parser(argString);
    //    char attrName[] = "sampleAttributeName";
    //    AttrType attrValue;
    //    if (!getAttributeValue(attrName, &attrValue)
    //        printf("Cannot find attribute \"%s\"\n", attrName);
    //    getAttributeValue(anotherAttr, arrayOf10Values, 10);
    // When the array form is used, an optional leading '[' is skipped.
    //--------------------------------------------------------------------------

    class JavascriptArgumentParser {
    public:
        JavascriptArgumentParser(const String &str) {
            mString = &str;
        }

        size_t hasAttributeName(const char *name) {
            size_t ix = 0;
            while ((ix = mString->find(name, ix)) != String::npos) {    // FIXME: Prefer a case-insensitive search
                if (ix > 0) {
                    if ((*mString)[ix-1] == '"' || (*mString)[ix-1] == '\'') {  // Quoted attribute name
                        char quoteChar = (*mString)[ix-1];
                        if ((*mString)[ix + strlen(name)] != quoteChar) {   // Quoting a different name
                            ix += strlen(name);                         // Skip over the name
                            continue;                                   // Keep looking
                        }
                        ++ix;                                           // Skip over the quote
                    }
                    else if (!isspace((*mString)[ix-1])) {              // No whitespace before the name
                        ix += strlen(name);                             // Skip over name
                        continue;                                       // Keep looking
                    }
                }
                ix += strlen(name);                                     // Skip over name
                ix = mString->find_first_not_of(mWhiteSpace, ix);       // Skip spaces
                if ((*mString)[ix] != '=' && (*mString)[ix] != ':')     // There should have been an '=' or ':' here
                    continue;                                           // Keep looking
                ix = mString->find_first_not_of(mWhiteSpace, ix + 1);   // Skip over the '=' or ':'
                // We know that we have <whitespace> <name> [ '=' | ':' ], and the index is positioned after the '=' or ':'
                break;
            }
            if (ix != String::npos) {
                if ((*mString)[ix] == '[')  // Javascript array
                    ++ix;                   // Position at the first element
            }
            return ix;
        }

        // Get string value
        bool getAttributeValue(const char *name, String *value, int numValues = 1) {
            size_t ix = hasAttributeName(name);
            if (ix == String::npos)
                return false;
            for (; numValues--; ++value, ix = mString->find_first_not_of(mArraySpace, ix))
                if (!getNextToken(*mString, &ix, value))
                    return false;
            return true;
        }

        // Get int value
        bool getAttributeValue(const char *name, int *value, int numValues = 1) {
            size_t ix = hasAttributeName(name);
            if (ix == String::npos)
                return false;
            const char *s0 = &(*mString)[ix];
            for (char *s1; numValues--; value++) {
                *value = strtol(s0, &s1, 10);
                if (s0 == s1 || *s1 == 0)           // Didn't find a number or it is the end of the string
                    break;
                s0 = s1 + strspn(s1, mArraySpace);  // Skip spaces and commas
            }
            return numValues == -1;
        }

        // Get double value
        bool getAttributeValue(const char *name, double *value, int numValues = 1) {
            size_t ix = hasAttributeName(name);
            if (ix == String::npos)
                return false;
            const char *s0 = &(*mString)[ix];
            for (char *s1; numValues--; value++) {
                *value = strtod(s0, &s1);
                if (s0 == s1 || *s1 == 0)           // Didn't find a number or it is the end of the string
                    break;
                s0 = s1 + strspn(s1, mArraySpace);  // Skip spaces and commas
            }
            return numValues == -1;
        }

        // Get float value
        bool getAttributeValue(const char *name, float *value, int numValues = 1) {
            size_t ix = hasAttributeName(name);
            if (ix == String::npos)
                return false;
            const char *s0 = &(*mString)[ix];
            for (char *s1; numValues--; value++) {
                *value = strtod(s0, &s1);
                if (s0 == s1 || *s1 == 0)           // Didn't find a number or it is the end of the string
                    break;
                s0 = s1 + strspn(s1, mArraySpace);  // Skip spaces and commas
            }
            return numValues == -1;
        }

        // Get bool value
        bool getAttributeValue(const char *name, bool *value, int numValues = 1) {
            size_t ix = hasAttributeName(name);
            if (ix == String::npos)
                return false;
            const char *s0 = &(*mString)[ix];
            for (const char *s1; numValues--; value++) {
                *value = strtobool(s0, &s1);
                if (s0 == s1 || *s1 == 0)           // Didn't find a Boolean or it is the end of the string
                    break;
                s0 = s1 + strspn(s1, mArraySpace);  // Skip spaces and commas
            }
            return numValues == -1;
        }

    private:
        static bool strtobool(char const *s0, char const **s1) {
            s0 += strspn(s0, mWhiteSpace);
            *s1 = s0;

            if (strncaseeq(s0, "true", 4)) {
                *s1 += 4;
                return true;
            } else if (s0[0] == '1') {
                *s1 += 1;
                return true;
            }
            if (strncaseeq(s0, "false", 5)) {
                *s1 += 5;
                return false;
            } else if (s0[0] == '0') {
                *s1 += 1;
                return false;
            }
            return static_cast<bool>(-1);
        }

        const String *mString;
    };


    static LightInfo::LightTypes getLightTypeEnum(const String typeString) {
        LightInfo::LightTypes type = static_cast<LightInfo::LightTypes>(-1);
        if       (typeString == "point")                                type = LightInfo::POINT;
        else if  (typeString == "directional")                          type = LightInfo::DIRECTIONAL;
        else if ((typeString == "spot") || (typeString == "spotlight")) type = LightInfo::SPOTLIGHT;
        else     SILOG(input, error, "getLightTypeEnum: unknown light type \"" + typeString + "\"");
        return type;
    }


    //--------------------------------------------------------------------------
    // This parses a string of the form:
    // diffusecolor=1.0,1.0,0.95 specularcolor=1,1,1 ambientcolor=.05,.05,.05 shadowcolor=.02,.02,.02
    // falloff=1.,.01,.02 cone=0,3,0 power=1e2 lightrange=256 castsshadow=true type=spot
    //--------------------------------------------------------------------------

    static void setLightInfoFromString(const String &str, LightInfo *lightInfo) {
        JavascriptArgumentParser jap(str);
        String type;
        if (jap.getAttributeValue("diffusecolor",  &lightInfo->mDiffuseColor[0],  3))   lightInfo->mWhichFields |= LightInfo::DIFFUSE_COLOR;   // R, G, B
        if (jap.getAttributeValue("specularcolor", &lightInfo->mSpecularColor[0], 3))   lightInfo->mWhichFields |= LightInfo::SPECULAR_COLOR;  // R, G, B
        if (jap.getAttributeValue("ambientcolor",  &lightInfo->mAmbientColor[0],  3))   lightInfo->mWhichFields |= LightInfo::AMBIENT_COLOR;   // R, G, B
        if (jap.getAttributeValue("shadowcolor",   &lightInfo->mShadowColor[0],   3))   lightInfo->mWhichFields |= LightInfo::SHADOW_COLOR;    // R, G. B
        if (jap.getAttributeValue("falloff",       &lightInfo->mConstantFalloff,  3))   lightInfo->mWhichFields |= LightInfo::FALLOFF;         // constant, linear, quadratic
        if (jap.getAttributeValue("cone",          &lightInfo->mConeInnerRadians, 3))   lightInfo->mWhichFields |= LightInfo::CONE;            // cone inner radians, outer radians, falloff
        if (jap.getAttributeValue("power",         &lightInfo->mPower,            1))   lightInfo->mWhichFields |= LightInfo::POWER;           // exponent
        if (jap.getAttributeValue("lightrange",    &lightInfo->mLightRange,       1))   lightInfo->mWhichFields |= LightInfo::LIGHT_RANGE;     // range
        if (jap.getAttributeValue("castsshadow",   &lightInfo->mCastsShadow,      1))   lightInfo->mWhichFields |= LightInfo::CAST_SHADOW;     // bool
        if (jap.getAttributeValue("type",             &type)) {                         lightInfo->mWhichFields |= LightInfo::TYPE;            // type
            lightInfo->mType = getLightTypeEnum(type);
            if (lightInfo->mType < 0) lightInfo->mWhichFields &= ~LightInfo::TYPE;
        }
    }


    //--------------------------------------------------------------------------
    // Find the height of the floor underneath the camera
    //--------------------------------------------------------------------------

    double getFloorHeight() const {
        CameraEntity *camera = mParent->mPrimaryCamera;
        Vector3d cameraPosition = camera->getOgrePosition();
        double distance;
        Vector3f normal;
        int numHits = 1, i;
        const Entity *floorObj = mParent->rayTrace(cameraPosition, Vector3f(0, -1, 0), numHits, distance, normal, 0);
        double floorY = 0;
        if (floorObj != NULL)    // No floor
            floorY = cameraPosition.y - distance;
        return floorY;
    }


    //--------------------------------------------------------------------------
    // Get front facing normal of a painting
    //--------------------------------------------------------------------------

    static Vector3f getPaintingNormal(const Entity *paintingEntity) {
        return paintingEntity->getOgreOrientation().zAxis();
    }


    //--------------------------------------------------------------------------
    // Set the position and orientation of a light on a painting entity.
    // The light is mounted on the ceiling
    // and aims at the painting by the given angle from normal.
    //--------------------------------------------------------------------------

    void initPaintingLightLocation(Entity *ent, float angle, float mDefaultLightHeight, Location *lightLocation, LightInfo *lightInfo) {
        // Find wall normal and object bounding sphere.
        // We assume that the object is in front of the wall

        // Get an object's center and radius
        BoundingSphere<double> objSphere = ent->getOgreWorldBoundingSphere<double>();

        // Get the floor coordinate
        double floorY = getFloorHeight();

        // Determine the normal to the wall behind the object
        Vector3f normal = getPaintingNormal(ent);

        // Move the light out from the wall, at the given angle
        Vector3d lightPosition;
        lightPosition.y = floorY + mDefaultLightHeight;
        double f = ((lightPosition.y - objSphere.center().y) / tan(angle));
        lightPosition.x = objSphere.center().x + normal.x * f;
        lightPosition.z = objSphere.center().z + normal.z * f;
        lightLocation->setPosition(lightPosition);

        // Aim it at the artwork
        Vector3f lightZ(objSphere.center() - lightPosition);
        if (lightZ.normalizeThis() == 0)
            lightZ = Vector3f::unitNegY();
        Vector3f lightX(Vector3f::unitY().cross(lightZ));
        if (lightX.normalizeThis() == 0)    // Looking straight down or up
            lightX = Vector3f::unitX();
        Vector3f lightY(lightZ.cross(lightX));
        if (lightY.normalizeThis() == 0)    // Looking straight down or up
            lightY = Vector3f::unitZ();
        lightLocation->setOrientation(Quaternion(lightX, lightY, lightZ));

        // Adjust the cone.
        const float coneFactor = 2.718281828;
        float coneAngle = atan(objSphere.radius() * coneFactor / (objSphere.center() - lightPosition).length());
        float coneInnerAngle = 0;
        float coneOuterAngle = coneAngle;   // Spotlights don't look as expected
        float coneFalloff = 0.5;
        lightInfo->setLightSpotlightCone(0, coneAngle, coneFalloff);
    }


    //--------------------------------------------------------------------------
    // Set the position and orientation of the light based on
    // the location, size and orientation on the selected object.
    //--------------------------------------------------------------------------

    void initSelectedObjectLightLocation(float angle, float mDefaultLightHeight, Location *lightLocation, LightInfo *lightInfo) {
        // Find wall normal and object bounding sphere.
        // We assume that the object is in front of the wall
        Entity *sel = NULL;
        // Get an object's center and radius
        for (SelectedObjectSet::const_iterator it = mSelectedObjects.begin(); it != mSelectedObjects.end(); ++it) {
            ProxyObjectPtr obj(it->lock());
            sel = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (sel == NULL)
                continue;
        }
        if (sel == NULL) {
            SILOG(input, error, "initSelectedObjectLightLocation: no selected object");
            return;
        }
        initPaintingLightLocation(sel, angle, mDefaultLightHeight, lightLocation, lightInfo);
    }


    //--------------------------------------------------------------------------
    // Print the light info in JSON-compatible format,
    // with the given prelude and postlude.
    //--------------------------------------------------------------------------

    static String printLightInfoToString(const char *prelude, const LightInfo &li, const char *postlude) {
        String info;
        info += '{';
        if (prelude) info += prelude;
        if (li.mWhichFields & LightInfo::TYPE)              string_appendf(&info, " 'type':"         "'%s',",  (li.mType == LightInfo::POINT)       ? "point" :
                                                                                                               (li.mType == LightInfo::DIRECTIONAL) ? "directional" :
                                                                                                               (li.mType == LightInfo::SPOTLIGHT)   ? "spotlight" :
                                                                                                               "unknown");
        if (li.mWhichFields & LightInfo::DIFFUSE_COLOR)     string_appendf(&info, " 'diffusecolor':" "[%.7g, %.7g, %.7g],", li.mDiffuseColor[0],  li.mDiffuseColor[1],  li.mDiffuseColor[2]);
        if (li.mWhichFields & LightInfo::SPECULAR_COLOR)    string_appendf(&info, " 'specularcolor':""[%.7g, %.7g, %.7g],", li.mSpecularColor[0], li.mSpecularColor[1], li.mSpecularColor[2]);
        if (li.mWhichFields & LightInfo::AMBIENT_COLOR)     string_appendf(&info, " 'ambientcolor':" "[%.7g, %.7g, %.7g],", li.mAmbientColor[0],  li.mAmbientColor[1],  li.mAmbientColor[2]);
        if (li.mWhichFields & LightInfo::SHADOW_COLOR)      string_appendf(&info, " 'shadowcolor':"  "[%.7g, %.7g, %.7g],", li.mShadowColor[0],   li.mShadowColor[1],   li.mShadowColor[2]);
        if (li.mWhichFields & LightInfo::FALLOFF)           string_appendf(&info, " 'falloff':"      "[%.7g, %.7g, %.7g],", li.mConstantFalloff,  li.mLinearFalloff,    li.mQuadraticFalloff);
        if (li.mWhichFields & LightInfo::CONE)              string_appendf(&info, " 'cone':"         "[%.7g, %.7g, %.7g],", li.mConeInnerRadians, li.mConeOuterRadians, li.mConeFalloff);
        if (li.mWhichFields & LightInfo::POWER)             string_appendf(&info, " 'power':"        "%.7g,", li.mPower);
        if (li.mWhichFields & LightInfo::LIGHT_RANGE)       string_appendf(&info, " 'lightrange':"   "%.7g,", li.mLightRange);
        if (li.mWhichFields & LightInfo::CAST_SHADOW)       string_appendf(&info, " 'castsshadow':"  "%s,",   li.mCastsShadow ? "true" : "false");
        if (postlude) info += postlude;
        if (info[info.size() - 1] == ',')
            info.erase(info.size() - 1);   // Remove trailing comma
        info += " }";
        return info;
    }


    //--------------------------------------------------------------------------
    // Print the light info in JSON-compatible format,
    // with the given prelude and postlude.
    //
    // FIXME: Shouldn't we only print the parameters that have been set?
    //--------------------------------------------------------------------------

    static String printLightInfoToString(const ProxyLightObject *light) {
        const LightInfo &li = light->getLastLightInfo();
        String prelude(" 'id':'" + light->getObjectReference().toString() + "',");
        return printLightInfoToString(prelude.c_str(), light->getLastLightInfo(), NULL);
    }


    //--------------------------------------------------------------------------
    // Initialize the light moods. These can be changed later.
    //--------------------------------------------------------------------------

    void initLightMoods() {
        std::cout << "dbm debug: initLightMoods" << std::endl;
        if (mMoodLightsInited)
            return;

        for (int i = 0; i < numMoods(); ++i) {
            mSpotLightMoods[i].mWhichFields          = 0;
            mPointLightMoods[i].mWhichFields         = 0;
            mDirectionalLightMoods[i].mWhichFields   = 0;
        }

        mSpotLightMoods[0].setLightDiffuseColor(Color(.7,.6,.5)).setLightRange(10.).setLightFalloff(1.,-.10,.05).setLightType(LightInfo::SPOTLIGHT).setLightPower(1);
        mSpotLightMoods[1].setLightDiffuseColor(Color(.8,.7,.6)).setLightRange(10.).setLightFalloff(1.,-.10,.05).setLightType(LightInfo::SPOTLIGHT).setLightPower(1);
        mSpotLightMoods[2].setLightDiffuseColor(Color(.9,.9,.8)).setLightRange(10.).setLightFalloff(1.,-.10,.05).setLightType(LightInfo::SPOTLIGHT).setLightPower(1);
        mSpotLightMoods[3].setLightDiffuseColor(Color(1.,1.,1.)).setLightRange(10.).setLightFalloff(1.,-.10,.05).setLightType(LightInfo::SPOTLIGHT).setLightPower(1);

        mPointLightMoods[0].setLightDiffuseColor(Color(1.2,1.1,1.0)).setLightRange(200.).setLightFalloff(1., .20,.06).setLightType(LightInfo::POINT).setLightPower(1.15);
        mPointLightMoods[1].setLightDiffuseColor(Color(1.05,1.,.95)).setLightRange(200.).setLightFalloff(1.,.3,.10).setLightType(LightInfo::POINT).setLightPower(1.5);
        mPointLightMoods[2].setLightDiffuseColor(Color(1.,1.,1.05)).setLightRange(200.).setLightFalloff(1.,0.,.15).setLightType(LightInfo::POINT).setLightPower(1.33);
        mPointLightMoods[3].setLightDiffuseColor(Color(1.,1.,1.)).setLightRange(200.).setLightFalloff(1.,0.25,.05).setLightType(LightInfo::POINT).setLightPower(1.25);

        mDirectionalLightMoods[0].setLightDiffuseColor(Color(.7,.7,.7)).setLightType(LightInfo::DIRECTIONAL).setLightPower(0.1);
        mDirectionalLightMoods[1].setLightDiffuseColor(Color(.7,.7,.7)).setLightType(LightInfo::DIRECTIONAL).setLightPower(0.2);
        mDirectionalLightMoods[2].setLightDiffuseColor(Color(.7,.7,.7)).setLightType(LightInfo::DIRECTIONAL).setLightPower(0.2);
        mDirectionalLightMoods[3].setLightDiffuseColor(Color(.7,.7,.7)).setLightType(LightInfo::DIRECTIONAL).setLightPower(0.3);

        mMoodLightsInited = true;
    }


    //--------------------------------------------------------------------------
    // Get the light info for the specified mood and type
    //--------------------------------------------------------------------------
    const LightInfo& getMoodLightInfo(int mood, LightInfo::LightTypes type)  {
        initLightMoods();
        return type == LightInfo::SPOTLIGHT ? mSpotLightMoods[mood] :
               type == LightInfo::POINT     ? mPointLightMoods[mood] :
                                              mDirectionalLightMoods[mood];
    }


    //--------------------------------------------------------------------------
    // Set the lightinfo for the specified mood.
    //--------------------------------------------------------------------------
    void setMoodLightInfo(int mood, const LightInfo &li) {
        initLightMoods();
        switch (li.mType) {
            case LightInfo::SPOTLIGHT:   mSpotLightMoods[mood]        = li; break;
            case LightInfo::POINT:       mPointLightMoods[mood]       = li; break;
            case LightInfo::DIRECTIONAL: mDirectionalLightMoods[mood] = li; break;
        }
    }


    //--------------------------------------------------------------------------
    // Set the lightinfo for the specified mood.
    //--------------------------------------------------------------------------
    int numMoods() const { return 4; }


    //--------------------------------------------------------------------------
    // JSON list of the IDs of the lights
    // Invoked as
    //      light list
    //--------------------------------------------------------------------------

    void lightList(const String& arg, size_t argCaret) {
        bool foundFirstLight = false;
        bool visibility = true;
        String list;
        list = "[";
        for (OgreSystem::SceneEntitiesMap::const_iterator iter = mParent->mSceneEntities.begin();
            iter != mParent->mSceneEntities.end(); ++iter
        ) {
            Entity *ent = iter->second;                                     if (!ent)   continue;
            ProxyObject *obj = ent->getProxyPtr().get();                    if (!obj)   continue;
            ProxyLightObject *light = dynamic_cast<ProxyLightObject*>(obj); if (!light) continue;
            if (list.size() > 1)
                list += ", ";
            list += obj->getObjectReference().toString();
        }
        list += "]";
        WebViewManager::getSingleton().evaluateJavaScript("__chrome",
            "debug('" + list + "');"
        );
    }


    //--------------------------------------------------------------------------
    // JSON dump of the light info of one or all of the lights.
    // Invoked as
    //     light get              , or
    //     light get 1            , or
    //     light get 292ae805-393b-f239-8df8-8f129f9ddb03:12345678-1111-1111-1111-defa01759ace
    //--------------------------------------------------------------------------

    void lightGet(const String& arg, size_t argCaret) {
            String info;
        int ix = 0;
        bool isIndex = false;
        int index = -1;
        SpaceObjectReference sor = SpaceObjectReference::null();
        String id;
        bool printOne = getNextToken(arg, &argCaret, &id);     // id or index
        if (printOne) {
            isIndex = id.find_first_of(':') == String::npos;
            if (isIndex)    index = strtol(id.c_str(), NULL, 10);
            else            sor = SpaceObjectReference(id);
        }
        else {
            info = "[ ";
        }

        for (OgreSystem::SceneEntitiesMap::const_iterator iter = mParent->mSceneEntities.begin(); iter != mParent->mSceneEntities.end(); ++iter) {
            Entity *ent = iter->second;                                     if (!ent)   continue;
            ProxyObject *obj = ent->getProxyPtr().get();                    if (!obj)   continue;
            ProxyLightObject *light = dynamic_cast<ProxyLightObject*>(obj); if (!light) continue;
            if (!printOne && ix > 0)
                info += ",\\n  ";
            if (!printOne || (isIndex && ix == index) || obj->getObjectReference() == sor)
                info += printLightInfoToString(light);
            ++ix;
        }

        if (!printOne)
            info += " ]";

        WebViewManager::getSingleton().evaluateJavaScript("__chrome",
            "receiveLight(\"" + info + "\");"
        );
    }

    //--------------------------------------------------------------------------
    // Find the light proxy for the given ID.
    //--------------------------------------------------------------------------

    ProxyLightObject* getLightProxyForID(SpaceObjectReference sor) {
        for (OgreSystem::SceneEntitiesMap::const_iterator iter = mParent->mSceneEntities.begin();
            iter != mParent->mSceneEntities.end(); ++iter
        ) {
            Entity *ent = iter->second;                                     if (!ent)   continue;
            ProxyObject *obj = ent->getProxyPtr().get();                    if (!obj)   continue;
            ProxyLightObject *light = dynamic_cast<ProxyLightObject*>(obj); if (!light) continue;
            if (obj->getObjectReference() == sor)
                return light;
        }
        return NULL;
    }
    ProxyLightObject* getLightProxyForID(String id) {
        return getLightProxyForID(SpaceObjectReference(id));
    }


    //--------------------------------------------------------------------------
    // Set the parameters of a light, with specifications in JSON format.
    // Invoked as
    //     light set { 'id':'<id>' ... }
    //     light set [ { 'id':'<id>' ... }, ... { 'id':'<id>' ... } ]
    //--------------------------------------------------------------------------
    void lightSet(const String& arg, size_t argCaret) {
        String params;
        argCaret += strspn(arg.c_str() + argCaret, mWhiteSpace);
        if (arg[argCaret] == '[') {                         // Array
            ++argCaret;                                     // Skip over '['
            while (getNextObject(arg, &argCaret, &params))  // Get next "{...}" string
                lightSet(params, 0);
            return;
        }

        params = arg.substr(argCaret);
        String id;
        JavascriptArgumentParser jap(params);
        if (!jap.getAttributeValue("id", &id)) {
            SILOG(input, error, "lightHandler: no light id specified");
            return;
        }

        ProxyLightObject *light = getLightProxyForID(id);
        if (light == NULL) {
            SILOG(input, error, "lightSet: cannot find light id \"" + id + "\"");
            return;
        }
        LightInfo li = light->getLastLightInfo();
        setLightInfoFromString(params, &li);
        light->update(li);
    }

    //--------------------------------------------------------------------------
    // Select the lighting mood. Choose from { 0, 1, 2, 3 }
    // Invoked as
    //     light selectmood <level>
    //--------------------------------------------------------------------------
    int mLastMoodSelected;
  public:       /// yes I did
    void lightSelectMood(const String& arg, size_t argCaret) {
        long mood;
        if (!getNextTokenAsLong(arg, &argCaret, &mood)) {
            SILOG(input, error, "lightSelectMood: no mood level was specified");
            return;
        }
        if (mood < 0 || mood >= numMoods()) {
            SILOG(input, error, "lightSelectMood: mood level out of range");
            return;
        }
        mLastMoodSelected=mood;
        std::cout << "dbm debug lightSelectMood setting mood: " << mood << std::endl;
        for (OgreSystem::SceneEntitiesMap::const_iterator iter = mParent->mSceneEntities.begin();
            iter != mParent->mSceneEntities.end(); ++iter
        ) {
            Entity *ent = iter->second;                                     if (!ent)   continue;
            ProxyObject *obj = ent->getProxyPtr().get();                    if (!obj)   continue;
            ProxyLightObject *light = dynamic_cast<ProxyLightObject*>(obj); if (!light) continue;
            LightInfo li = light->getLastLightInfo();
            li = getMoodLightInfo(mood, li.mType);
            light->update(li);
        }
    }
  private:

    //--------------------------------------------------------------------------
    // Get the lighting mood as a string. Choose from { 0, 1, 2, 3 }
    // Invoked as
    //     light getmood <level> <type>
    //     light getmood <level>
    //     light getmood
    // The second form will return all types of the given mood level.
    // The last form will get all moods for all levels and types.
    //--------------------------------------------------------------------------

    String lightGetMoodString(const String& arg, size_t argCaret) {
        int firstMood = 0, lastMood = numMoods() - 1;                           // If no mood level is specified, print them all
        int mood;
        String info;
        LightInfo::LightTypes lightType = static_cast<LightInfo::LightTypes>(-1);
        if (getNextTokenAsInt(arg, &argCaret, &mood)) {                         // Found a mood level parameter
            firstMood = lastMood = mood;
            if (mood < 0 || mood >= numMoods()) {
                SILOG(input, error, "lightGetMood: mood level out of range");
                return info;
            }
            String lightTypeString;
            if (getNextToken(arg, &argCaret, &lightTypeString))                 // Found a light type parameter
                lightType = getLightTypeEnum(lightTypeString);
        }

        if (lightType < 0) {                                                    // No light type was specified
            info = "[ ";
            for (mood = firstMood; mood <= lastMood; ++mood) {
                String prelude(string_printf(" 'mood':%d,", mood));
                if (info.size() > 2)
                    info += ",\\n  ";
                info += printLightInfoToString(prelude.c_str(), getMoodLightInfo(mood, LightInfo::DIRECTIONAL), NULL);   info += ",\\n  ";
                info += printLightInfoToString(prelude.c_str(), getMoodLightInfo(mood, LightInfo::POINT),       NULL);   info += ",\\n  ";
                info += printLightInfoToString(prelude.c_str(), getMoodLightInfo(mood, LightInfo::SPOTLIGHT),   NULL);
            }
            info += " ]";
        }
        else {                                                                  // Light type and mood were specified
            const LightInfo *moodPtr = &getMoodLightInfo(mood, lightType);
            String prelude(string_printf(" 'mood':%d,", mood));
            info = printLightInfoToString(prelude.c_str(), *moodPtr, NULL);
        }
        return info;
    }


    //--------------------------------------------------------------------------
    // Get a string for saving the moods.
    // String info = getMoodStringForSaving();  // This saves into a string
    // lightSetMood(info, 0);                   // This restores from a string

    //--------------------------------------------------------------------------
    String getMoodStringForSaving() {
        String info(lightGetMoodString("", 0));
        for (String::iterator s = info.begin(); s != info.end(); ++s) {
            if (s[0] == '\\' && s[1] == 'n') {  // Found a '\' followed by a 'n'
                s[0] = ' ';                     // Convert them both to spaces.
                s[1] = ' ';
            }
        }
        return info;
    }


    //--------------------------------------------------------------------------
    // Get the lighting mood. Choose from { 0, 1, 2, 3}
    // Invoked as
    //     light getmood <level> <type>
    //     light getmood <level>
    //     light getmood
    // The second form will return all types of the given mood level.
    // The last form will get all moods for all levels and types.
    //--------------------------------------------------------------------------

    void lightGetMood(const String& arg, size_t argCaret) {
        String info(lightGetMoodString(arg, argCaret));
        WebViewManager::getSingleton().evaluateJavaScript("__chrome",
            "receiveLightMood(\"" + info + "\");"
        );
    }


    //--------------------------------------------------------------------------
    // Set the lighting mood. Choose from { 0, 1, 2, 3}
    // Invoked as
    //     light setmood { mood:<level>, type:<type> ... }
    //     light setmood [ { mood:<level>, type:<type> ... }, ... { ... } ]
    //--------------------------------------------------------------------------

    void lightSetMood(const String& arg, size_t argCaret) {
        String params;
        argCaret += strspn(arg.c_str() + argCaret, mWhiteSpace);
        if (arg[argCaret] == '[') {                         // Array
            ++argCaret;                                     // Skip over '['
            while (getNextObject(arg, &argCaret, &params))  // Get next "{...}" string
                lightSetMood(params, 0);
            return;
        }
        params = arg.substr(argCaret);
        JavascriptArgumentParser jap(params);
        int mood;
        if (!jap.getAttributeValue("mood", &mood)) {
            SILOG(input, error, "lightSetMood: no mood level specified");
            return;
        }
        if (mood < 0 || mood >= numMoods()) {
            SILOG(input, error, "lightSetMood: mood level out of range");
            return;
        }
        LightInfo li;
        li.mWhichFields = 0;
        setLightInfoFromString(params, &li);
        setMoodLightInfo(mood, li);
    }

    //--------------------------------------------------------------------------
    // Place a spotlight on the selected object.
    // Invoked as
    //     light selected
    //     light selected { ... }
    // The latter form allows you to initalize it with specific parameters rather than the default.
    //--------------------------------------------------------------------------

    void lightSelected(const String& arg, size_t argCaret) {
        LightInfo li = mSpotLightMoods[numMoods() - 1];     // Initialize with the max mood.
        Location lightLocation;
        setLightInfoFromString(arg, &li);
        initSelectedObjectLightLocation(mDefaultSpotLightInclination, mDefaultLightHeight, &lightLocation, &li);

        CameraEntity *camera = mParent->mPrimaryCamera;
        if (!camera) return;
        SpaceObjectReference newId = SpaceObjectReference(camera->id().space(), ObjectReference(UUID::random()));
        ProxyManager *proxyMgr = camera->getProxy().getProxyManager();
        Time now(SpaceTimeOffsetManager::getSingleton().now(newId.space()));

        std::tr1::shared_ptr<ProxyLightObject> newLightObject(new ProxyLightObject(proxyMgr, newId));
        proxyMgr->createObject(newLightObject);
        newLightObject->update(li);

        Entity *parentent = mParent->getEntity(mCurrentGroup);
        if (parentent) {
            Location localLoc = lightLocation.toLocal(parentent->getProxy().globalLocation(now));
            newLightObject->setParent(parentent->getProxyPtr(), now, lightLocation, localLoc);
            newLightObject->resetLocation(now, lightLocation);
        }
        else {
            newLightObject->resetLocation(now, lightLocation);
        }
#if ATTACH_MESH_TO_LIGHTS
            // Attach a light bulb mesh
            Entity *ent = mParent->getEntity(newId);
            if (ent)
                LightBulb::AttachLightBulb(camera, ent, String("LightMesh" + newId.toString()).c_str(), lightLocation);
#endif // ATTACH_MESH_TO_LIGHTS
    //    mSelectedObjects.clear();
    //    mSelectedObjects.insert(newLightObject);
    //    Entity *ent = mParent->getEntity(newId);
    //    if (ent) {
    //        ent->setSelected(true);
    //    }
    }


    //--------------------------------------------------------------------------
    // Remove the specified light.
    // Invoked as
    //     light remove <id>
    //--------------------------------------------------------------------------

    void lightRemove(const String& arg, size_t argCaret) {
        String id;
        if (!getNextToken(arg, &argCaret, &id)) {
            SILOG(input, error, "lightRemove: no light id specified");
            return;
        }
        int index = -1, ix = 0;
        SpaceObjectReference sor = SpaceObjectReference::null();
        bool isIndex = id.find_first_of(':') == String::npos;
        if (isIndex)    index = strtol(id.c_str(), NULL, 10);
        else            sor = SpaceObjectReference(id);

        for (OgreSystem::SceneEntitiesMap::const_iterator iter = mParent->mSceneEntities.begin(); iter != mParent->mSceneEntities.end(); ++iter) {
            Entity *ent = iter->second;                                     if (!ent)   continue;
            ProxyObject *obj = ent->getProxyPtr().get();                    if (!obj)   continue;
            ProxyLightObject *light = dynamic_cast<ProxyLightObject*>(obj); if (!light) continue;
            if ((isIndex && ix == index) || obj->getObjectReference() == sor)
                ent->getProxy().getProxyManager()->destroyObject(ent->getProxyPtr());
            ++ix;
        }
    }


    //--------------------------------------------------------------------------
    // Get the power of one or all of the lights.
    // Invoked as
    //     light getpower average
    //     light getpower <id>
    //     light getpower
    // The latter form will get the power level of all lights.
    //--------------------------------------------------------------------------

    void lightGetPower(const String& arg, size_t argCaret) {
        String id;
        bool getOne = getNextToken(arg, &argCaret, &id) && id != "average";
        SpaceObjectReference sor = SpaceObjectReference::null();
        String info;
        if (getOne)
            sor = SpaceObjectReference(id);
        else
            info = "[ ";

        int ix = 0;
        float avgPower = 0;
        for (OgreSystem::SceneEntitiesMap::const_iterator iter = mParent->mSceneEntities.begin(); iter != mParent->mSceneEntities.end(); ++iter) {
            Entity *ent = iter->second;                                     if (!ent)   continue;
            ProxyObject *obj = ent->getProxyPtr().get();                    if (!obj)   continue;
            ProxyLightObject *light = dynamic_cast<ProxyLightObject*>(obj); if (!light) continue;
            if (getOne && !(light->getObjectReference() == sor))                        continue;
            if (!getOne && info.size() > 2)
                info += ", ";
            LightInfo li = light->getLastLightInfo();
            float power = light->getLastLightInfo().mPower;
            string_appendf(&info, "{ 'id':'%s', 'power':%.7g }", light->getObjectReference().toString().c_str(), power);
            ++ix;
            avgPower += (power - avgPower) / (float)ix;
        }

        if (!getOne)
            info += " ]";

        if (id == "average")
            info = string_printf("%.7g", avgPower);

        WebViewManager::getSingleton().evaluateJavaScript("__chrome",
            "receivePower(\"" + info + "\");"
        );
    }


    //--------------------------------------------------------------------------
    // Set the power of one or all of the lights.
    // Invoked as
    //     light setpower <power>
    //     light setpower <power> <id>
    //     light setpower { 'id':'<id>', 'power':<power> }
    //     light setpower [ { 'id':'<id>', 'power':<power> }, { ... } ]
    // The first form sets the power level of all lights to the same value.
    // The second and third forms set the power level of a particular light.
    // The fourth form sets the power level of every light to a different level.
    //--------------------------------------------------------------------------

    void lightSetPower(const String& arg, size_t argCaret) {
        argCaret += strspn(arg.c_str() + argCaret, mWhiteSpace);
        if (arg[argCaret] == '[') {                         // Array
            String params;
            ++argCaret;                                     // Skip over '['
            while (getNextObject(arg, &argCaret, &params))  // Get next "{...}" string
                lightSetPower(params, 0);
            return;
        }

        SpaceObjectReference sor = SpaceObjectReference::null();
        float power;
        String id;
        bool setOne;
        if (arg[argCaret] == '{') {                         // Object
            String params = arg.substr(argCaret);
            JavascriptArgumentParser jap(params);
            setOne = jap.getAttributeValue("id", &id);
            if (!jap.getAttributeValue("power", &power)) {
                SILOG(input, error, "lightSetPower: no power level was specified");
                return;
            }
        }
        else {                                              // Simple parameters
            if (!getNextTokenAsFloat(arg, &argCaret, &power)) {
                SILOG(input, error, "lightSetPower: no power level was specified");
                return;
            }
            setOne = getNextToken(arg, &argCaret, &id);
        }

        if (setOne)
            sor = SpaceObjectReference(id);

        for (OgreSystem::SceneEntitiesMap::const_iterator iter = mParent->mSceneEntities.begin(); iter != mParent->mSceneEntities.end(); ++iter) {
            Entity *ent = iter->second;                                     if (!ent)   continue;
            ProxyObject *obj = ent->getProxyPtr().get();                    if (!obj)   continue;
            ProxyLightObject *light = dynamic_cast<ProxyLightObject*>(obj); if (!light) continue;
            if (setOne && !(light->getObjectReference() == sor))                        continue;
            LightInfo li = light->getLastLightInfo();
            li.setLightPower(power);
            light->update(li);
        }
    }


    //--------------------------------------------------------------------------
    //  light list
    //  light get [<id>]
    //  light set <id> { ... }
    //  light mood <level>
    //  light setmood <level> { type=<type> ... }
    //  light getmood <level> [<type>]
    //  light selected
    //  light remove <id>
    //  light getpower [<id> | average]
    //  light setpower <power> [<id>]
    //  light setpower { id:<id>, power:<power> }
    //--------------------------------------------------------------------------

    void lightHandler(WebViewManager::NavigationAction action, const String& arg) {
        String token;
        size_t argCaret = 0;
        bool success = true, rotate = false;
        double speed;
        typedef void (OgreSystem::MouseHandler::*LightHandlerProc)(const String& arg, size_t argCaret);
        struct LightDispatch {
            const char              *command;
            LightHandlerProc        handler;
        };
        static const LightDispatch dispatchTable[] = {
            { "list",           &Sirikata::Graphics::OgreSystem::MouseHandler::lightList        },
            { "get",            &Sirikata::Graphics::OgreSystem::MouseHandler::lightGet         },
            { "set",            &Sirikata::Graphics::OgreSystem::MouseHandler::lightSet         },
            { "selectmood",     &Sirikata::Graphics::OgreSystem::MouseHandler::lightSelectMood  },
            { "setmood",        &Sirikata::Graphics::OgreSystem::MouseHandler::lightSetMood     },
            { "getmood",        &Sirikata::Graphics::OgreSystem::MouseHandler::lightGetMood     },
            { "selected",       &Sirikata::Graphics::OgreSystem::MouseHandler::lightSelected    },
            { "remove",         &Sirikata::Graphics::OgreSystem::MouseHandler::lightRemove      },
            { "getpower",       &Sirikata::Graphics::OgreSystem::MouseHandler::lightGetPower    },
            { "setpower",       &Sirikata::Graphics::OgreSystem::MouseHandler::lightSetPower    },
            { NULL,             NULL                                                            }
        };

        String command;
        getNextToken(arg, &argCaret, &command);                         // light (already parsed)
        success = success && getNextToken(arg, &argCaret, &command);    // subcommand

        const LightDispatch *dp;
        for (dp = dispatchTable; dp->command != NULL; ++dp)
            if (strcaseeq(dp->command, command.c_str()))
                break;
        if (dp->handler != NULL)
            (this->*(dp->handler))(arg, argCaret);
        else
            SILOG(input, error, "lightHandler: unknown command: \"" << command << "\"");
    }


    //--------------------------------------------------------------------------
    /// generic message mechanism (to send messages from JScript to Camera/Python thru C++, for instance)
    //--------------------------------------------------------------------------

    void genericStringMessage(WebViewManager::NavigationAction action, const String& arg) {
        // Get the command (first word)
        String command;
        size_t i = 0;
        if (!getNextToken(arg, &i, &command)) {
            SILOG(input, error, "genericStringMessage: no command found in \"" << arg << "\"");
            return;
        }

        // Dispatch on the command
        typedef void (OgreSystem::MouseHandler::*StringMessageHandler)(WebViewManager::NavigationAction action, const String& arg);
        struct StringMessageDispatch {
            const char              *command;
            StringMessageHandler    handler;
        };
        static const StringMessageDispatch dispatchTable[] = {
            { "reset",                  &Sirikata::Graphics::OgreSystem::MouseHandler::resetHandler },
            { "inventory",              &Sirikata::Graphics::OgreSystem::MouseHandler::inventoryHandler },
            { "walk",                   &Sirikata::Graphics::OgreSystem::MouseHandler::walkHandler },
            { "step",                   &Sirikata::Graphics::OgreSystem::MouseHandler::stepHandler },
            { "getselectedids",         &Sirikata::Graphics::OgreSystem::MouseHandler::getSelectedIDHandler },
            { "light",                  &Sirikata::Graphics::OgreSystem::MouseHandler::lightHandler },
            { NULL,                     NULL }
        };
        const StringMessageDispatch *dp;
        for (dp = dispatchTable; dp->command != NULL; ++dp)
            if (strcaseeq(dp->command, command.c_str()))
                break;
        if (dp->handler != NULL)
            (this->*(dp->handler))(action, arg);
        else
            SILOG(input, error, "genericStringMessage: unknown command: \"" << command << "\"");
    }

    ///////////////// DEVICE FUNCTIONS ////////////////

    EventResponse deviceListener(EventPtr evbase) {
        std::tr1::shared_ptr<InputDeviceEvent> ev (std::tr1::dynamic_pointer_cast<InputDeviceEvent>(evbase));
        if (!ev) {
            return EventResponse::nop();
        }
        switch (ev->mType) {
        case InputDeviceEvent::ADDED:
            if (!!(std::tr1::dynamic_pointer_cast<SDLMouse>(ev->mDevice))) {
                SubscriptionId subId = mParent->mInputManager->subscribeId(
                    AxisEvent::getEventId(),
                    std::tr1::bind(&MouseHandler::axisHandler, this, _1));
                mEvents.push_back(subId);
                mDeviceSubscriptions.insert(DeviceSubMap::value_type(&*ev->mDevice, subId));
            }
            if (!!(std::tr1::dynamic_pointer_cast<SDLKeyboard>(ev->mDevice))) {
                // Key Pressed
                {
                    SubscriptionId subId = mParent->mInputManager->subscribeId(
                        ButtonPressed::getEventId(),
                        std::tr1::bind(&MouseHandler::keyHandler, this, _1)
                    );
                    mEvents.push_back(subId);
                    mDeviceSubscriptions.insert(DeviceSubMap::value_type(&*ev->mDevice, subId));
                }
                // Key Released
                {
                    SubscriptionId subId = mParent->mInputManager->subscribeId(
                        ButtonReleased::getEventId(),
                        std::tr1::bind(&MouseHandler::keyHandler, this, _1)
                    );
                    mEvents.push_back(subId);
                    mDeviceSubscriptions.insert(DeviceSubMap::value_type(&*ev->mDevice, subId));
                }
            }
            break;
        case InputDeviceEvent::REMOVED: {
            DeviceSubMap::iterator iter;
            while ((iter = mDeviceSubscriptions.find(&*ev->mDevice))!=mDeviceSubscriptions.end()) {
                mParent->mInputManager->unsubscribe((*iter).second);
                mDeviceSubscriptions.erase(iter);
            }
        }
        break;
        }
        return EventResponse::nop();
    }

public:
    MouseHandler(OgreSystem *parent)
     : mParent(parent),
       mCurrentGroup(SpaceObjectReference::null()),
       mLastCameraTime(Task::LocalTime::now()),
       mWhichRayObject(0),
       mLastMoodSelected(3)
    {
        initLightMoods();
        lightSelectMood("3", 0);
        mCamSpeed = 0.3;
        if (gMode=="funmode") {
            mCamSpeed = 1.0;
        }
        mLastHitCount=0;
        mLastHitX=0;
        mLastHitY=0;

        cameraPathLoad();
        mRunningCameraPath = false;
        mCameraPathIndex = 0;
        mCameraPathTime = mCameraPath.keyFrameTime(0);

        mEvents.push_back(mParent->mInputManager->registerDeviceListener(
                              std::tr1::bind(&MouseHandler::deviceListener, this, _1)));

        mDragAction[1] = DragActionRegistry::get("moveObjectOnWall");       /// let this be the default for all time
        if (gMode=="critic") mDragAction[1]=0;
        mDragAction[2] = DragActionRegistry::get("zoomCamera");
        mDragAction[3] = DragActionRegistry::get("panCamera");
        mDragAction[4] = DragActionRegistry::get("rotateCamera");

        mEvents.push_back(mParent->mInputManager->subscribeId(
                MouseHoverEvent::getEventId(),
                std::tr1::bind(&MouseHandler::mouseHoverHandler, this, _1)));

        mEvents.push_back(mParent->mInputManager->subscribeId(
                MousePressedEvent::getEventId(),
                std::tr1::bind(&MouseHandler::mousePressedHandler, this, _1)));

        mEvents.push_back(mParent->mInputManager->subscribeId(
                MouseDragEvent::getEventId(),
                std::tr1::bind(&MouseHandler::mouseDragHandler, this, _1)));

        mEvents.push_back(mParent->mInputManager->subscribeId(
                MouseClickEvent::getEventId(),
                std::tr1::bind(&MouseHandler::mouseClickHandler, this, _1)));

        mEvents.push_back(mParent->mInputManager->subscribeId(
                TextInputEvent::getEventId(),
                std::tr1::bind(&MouseHandler::textInputHandler, this, _1)));

        mEvents.push_back(mParent->mInputManager->subscribeId(
                WebViewEvent::Id,
                std::tr1::bind(&MouseHandler::webviewHandler, this, _1)));

        const float trnSpeed = 0.20f;   // Speed for translational motion, in meters/sec.
        const float rotSpeed = 0.75f;   // Speed for  rotational   motion, in radians/sec.
        // ----------------------------------------------------------------------------------------------------------  ------ direction --------  ---on--- off
        mInputResponses["moveForward"]  = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::moveAction, this, Vector3f( 0,  0, -1), _1), trnSpeed, 0);
        mInputResponses["moveBackward"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::moveAction, this, Vector3f( 0,  0, +1), _1), trnSpeed, 0);
        mInputResponses["moveLeft"]     = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::moveAction, this, Vector3f(-1,  0,  0), _1), trnSpeed, 0);
        mInputResponses["moveRight"]    = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::moveAction, this, Vector3f(+1,  0,  0), _1), trnSpeed, 0);
        mInputResponses["moveDown"]     = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::moveAction, this, Vector3f( 0, -1,  0), _1), trnSpeed, 0);
        mInputResponses["moveUp"]       = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::moveAction, this, Vector3f( 0,  1,  0), _1), trnSpeed, 0);

        mInputResponses["rotateXPos"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::rotateAction, this, Vector3f(+1,  0,  0), _1), rotSpeed, 0);
        mInputResponses["rotateXNeg"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::rotateAction, this, Vector3f(-1,  0,  0), _1), rotSpeed, 0);
        mInputResponses["rotateYPos"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::rotateAction, this, Vector3f( 0, +1,  0), _1), rotSpeed, 0);
        mInputResponses["rotateYNeg"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::rotateAction, this, Vector3f( 0, -1,  0), _1), rotSpeed, 0);
        mInputResponses["rotateZPos"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::rotateAction, this, Vector3f( 0,  0, +1), _1), rotSpeed, 0);
        mInputResponses["rotateZNeg"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::rotateAction, this, Vector3f( 0,  0, -1), _1), rotSpeed, 0);

        mInputResponses["stableRotatePos"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::stableRotateAction, this, +1.f, _1),      rotSpeed, 0);
        mInputResponses["stableRotateNeg"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::stableRotateAction, this, -1.f, _1),      rotSpeed, 0);

        mInputResponses["createLight"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::createLightAction, this));
        mInputResponses["toggleLightVisibility"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::toggleLightVisibilityAction, this));
        mInputResponses["enterObject"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::enterObjectAction, this));
        mInputResponses["leaveObject"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::leaveObjectAction, this));
        mInputResponses["groupObjects"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::groupObjectsAction, this));
        mInputResponses["ungroupObjects"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::ungroupObjectsAction, this));
        mInputResponses["deleteObjects"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::deleteObjectsAction, this));
        mInputResponses["cloneObjects"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cloneObjectsAction, this));
        mInputResponses["import"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::importAction, this));
        mInputResponses["saveScene"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::saveSceneAction, this));

        mInputResponses["selectObject"] = new Vector2fInputResponse(std::tr1::bind(&MouseHandler::selectObjectAction, this, _1, 1));
        mInputResponses["selectObjectReverse"] = new Vector2fInputResponse(std::tr1::bind(&MouseHandler::selectObjectAction, this, _1, -1));

        mInputResponses["zoom"] = new AxisInputResponse(std::tr1::bind(&MouseHandler::zoomAction, this, _1, _2));

        mInputResponses["setDragModeNone"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setDragModeAction, this, ""));
        mInputResponses["setDragModeMoveObject"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setDragModeAction, this, "moveObject"));
        mInputResponses["setDragModeMoveObjectOnWall"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setDragModeAction, this, "moveObjectOnWall"));
        mInputResponses["setDragModeRotateObject"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setDragModeAction, this, "rotateObject"));
        mInputResponses["setDragModeScaleObject"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setDragModeAction, this, "scaleObject"));
        mInputResponses["setDragModeRotateCamera"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setDragModeAction, this, "rotateCamera"));
        mInputResponses["setDragModePanCamera"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setDragModeAction, this, "panCamera"));

        mInputResponses["setCameraSpeed1"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setCameraSpeed, this, 0.1));
        mInputResponses["setCameraSpeed2"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setCameraSpeed, this, 0.3));
        mInputResponses["setCameraSpeed3"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setCameraSpeed, this, 1.0));
        mInputResponses["setCameraSpeed4"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setCameraSpeed, this, 3.0));
        mInputResponses["setCameraSpeed5"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setCameraSpeed, this, 10.0));

        mInputResponses["cameraPathLoad"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathLoad, this));
        mInputResponses["cameraPathSave"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathSave, this));
        mInputResponses["cameraPathNextKeyFrame"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathNext, this));
        mInputResponses["cameraPathPreviousKeyFrame"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathPrevious, this));
        mInputResponses["cameraPathInsertKeyFrame"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathInsert, this));
        mInputResponses["cameraPathDeleteKeyFrame"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathDelete, this));
        mInputResponses["cameraPathRun"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathRun, this));
        mInputResponses["cameraPathSpeedUp"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathChangeSpeed, this, -0.1f));
        mInputResponses["cameraPathSlowDown"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathChangeSpeed, this, 0.1f));
        mInputResponses["fireAction"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::fireAction, this));
        mInputResponses["resetAction"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::resetAction, this));
        mInputResponses["debugAction1"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::debugAction, this, 1));
        mInputResponses["debugAction2"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::debugAction, this, 2));
        mInputResponses["debugAction3"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::debugAction, this, 3));
        mInputResponses["quitAction"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::debugAction, this, -1));

        mInputResponses["webNewTab"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateAction, this, WebViewManager::NavigateNewTab));
        mInputResponses["webBack"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateAction, this, WebViewManager::NavigateBack));
        mInputResponses["webForward"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateAction, this, WebViewManager::NavigateForward));
        mInputResponses["webRefresh"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateAction, this, WebViewManager::NavigateRefresh));
        mInputResponses["webHome"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateAction, this, WebViewManager::NavigateHome));
        mInputResponses["webGo"] = new StringInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateStringAction, this, WebViewManager::NavigateGo, _1));

//        mInputResponses["webCommand"] = new StringInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateStringAction, this, WebViewManager::NavigateCommand, _1));

        mInputResponses["genericMessage"] = new StringInputResponse(std::tr1::bind(&MouseHandler::genericStringMessage, this, WebViewManager::NavigateCommand, _1));

        // Movement
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_W), mInputResponses["moveForward"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_S), mInputResponses["moveBackward"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_D), mInputResponses["moveRight"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_A), mInputResponses["moveLeft"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_PAGEUP), mInputResponses["moveUp"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_PAGEDOWN), mInputResponses["moveDown"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_UP, Input::MOD_SHIFT), mInputResponses["rotateXPos"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_DOWN, Input::MOD_SHIFT), mInputResponses["rotateXNeg"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_UP), mInputResponses["moveForward"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_DOWN), mInputResponses["moveBackward"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_LEFT), mInputResponses["stableRotatePos"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_RIGHT), mInputResponses["stableRotateNeg"]);

        // Various other actions
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_B), mInputResponses["createLight"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_B, Input::MOD_SHIFT), mInputResponses["toggleLightVisibility"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_KP_ENTER), mInputResponses["enterObject"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_RETURN), mInputResponses["enterObject"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_KP_0), mInputResponses["leaveObject"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_ESCAPE), mInputResponses["leaveObject"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_G), mInputResponses["groupObjects"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_G, Input::MOD_ALT), mInputResponses["ungroupObjects"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_DELETE), mInputResponses["deleteObjects"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_V, Input::MOD_CTRL), mInputResponses["cloneObjects"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_O, Input::MOD_CTRL), mInputResponses["import"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_S, Input::MOD_CTRL), mInputResponses["saveScene"]);

        // Drag modes
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_Q, Input::MOD_CTRL), mInputResponses["setDragModeNone"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_W, Input::MOD_CTRL), mInputResponses["setDragModeMoveObject"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_W, Input::MOD_CTRL|Input::MOD_SHIFT), mInputResponses["setDragModeMoveObjectOnWall"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_E, Input::MOD_CTRL), mInputResponses["setDragModeRotateObject"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_R, Input::MOD_CTRL), mInputResponses["setDragModeScaleObject"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_T, Input::MOD_CTRL), mInputResponses["setDragModeRotateCamera"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_Y, Input::MOD_CTRL), mInputResponses["setDragModePanCamera"]);


        // camera speeds
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_1), mInputResponses["setCameraSpeed1"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_2), mInputResponses["setCameraSpeed2"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_3), mInputResponses["setCameraSpeed3"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_4), mInputResponses["setCameraSpeed4"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_5), mInputResponses["setCameraSpeed5"]);

        // Mouse Zooming
#if 0   // This would be great if it workd, but it only zooms out
        mInputBinding.add(InputBindingEvent::Axis(SDLMouse::WHEELY), mInputResponses["zoom"]);
#endif

        // Selection
        mInputBinding.add(InputBindingEvent::MouseClick(1), mInputResponses["selectObject"]);
        //mInputBinding.add(InputBindingEvent::MouseClick(3), mInputResponses["selectObjectReverse"]);

        // Camera Path
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_F1), mInputResponses["cameraPathLoad"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_F2), mInputResponses["cameraPathSave"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_F3), mInputResponses["cameraPathNextKeyFrame"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_F4), mInputResponses["cameraPathPreviousKeyFrame"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_F5), mInputResponses["cameraPathInsertKeyFrame"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_F6), mInputResponses["cameraPathDeleteKeyFrame"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_F7), mInputResponses["cameraPathRun"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_F8), mInputResponses["cameraPathSpeedUp"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_O), mInputResponses["cameraPathSpeedUp"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_F9), mInputResponses["cameraPathSlowDown"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_P), mInputResponses["cameraPathSlowDown"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_SPACE), mInputResponses["fireAction"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_R), mInputResponses["resetAction"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_MINUS), mInputResponses["debugAction1"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_EQUALS), mInputResponses["debugAction2"]);
        if(gDebug) mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_F10), mInputResponses["debugAction3"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_Q, Input::MOD_CTRL|Input::MOD_SHIFT), mInputResponses["quitAction"]);

        // WebView Chrome
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navnewtab"), mInputResponses["webNewTab"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navback"), mInputResponses["webBack"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navforward"), mInputResponses["webForward"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navrefresh"), mInputResponses["webRefresh"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navhome"), mInputResponses["webHome"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navgo", 1), mInputResponses["webGo"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navmoveforward", 1), mInputResponses["moveForward"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navturnleft", 1), mInputResponses["stableRotatePos"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navturnright", 1), mInputResponses["stableRotateNeg"]);

    //    mInputBinding.add(InputBindingEvent::Web("__chrome", "navcommand", 1), mInputResponses["webCommand"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navcommand", 1), mInputResponses["genericMessage"]);
    }

    ~MouseHandler() {
        for (std::vector<SubscriptionId>::const_iterator iter = mEvents.begin();
             iter != mEvents.end();
             ++iter) {
            mParent->mInputManager->unsubscribe(*iter);
        }
        for (InputResponseMap::iterator iter=mInputResponses.begin(),iterend=mInputResponses.end();iter!=iterend;++iter) {
            delete iter->second;
        }
        for (std::map<int, ActiveDrag*>::iterator iter=mActiveDrag.begin(),iterend=mActiveDrag.end();iter!=iterend;++iter) {
            if(iter->second!=NULL)
                delete iter->second;
        }
    }
    void setParentGroupAndClear(const SpaceObjectReference &id) {
        clearSelection();
        mCurrentGroup = id;
    }
    const SpaceObjectReference &getParentGroup() const {
        return mCurrentGroup;
    }
    void addToSelection(const ProxyObjectPtr &obj) {
        mSelectedObjects.insert(obj);
    }

    void tick(const Task::LocalTime& t) {
        cameraPathTick(t);
    }
};


// MouseHandler static members and their initialization
const char  OgreSystem::MouseHandler::mWhiteSpace[]                 = " \t\n\r";        // Space between tokens
const char  OgreSystem::MouseHandler::mArraySpace[]                 = " \t\n\r,";       // Space between array elements
float       OgreSystem::MouseHandler::mDefaultSpotLightInclination  = 30 * M_PI / 180;  // 30 degree default
float       OgreSystem::MouseHandler::mDefaultLightHeight           = 5;                // 5 meters ~= 16.4 feet
bool        OgreSystem::MouseHandler::mMoodLightsInited             = false;            // This indicates when the moods below have been initialized
LightInfo   OgreSystem::MouseHandler::mSpotLightMoods[4];                               // These moods are initialized programatically
LightInfo   OgreSystem::MouseHandler::mPointLightMoods[4];
LightInfo   OgreSystem::MouseHandler::mDirectionalLightMoods[4];


////////////////////////////////////////////////////////////////////////////////
//                                  OgreSystem                                //
////////////////////////////////////////////////////////////////////////////////

void OgreSystem::allocMouseHandler() {
    mMouseHandler = new MouseHandler(this);
}
void OgreSystem::destroyMouseHandler() {
    if (mMouseHandler) {
        delete mMouseHandler;
    }
}

void OgreSystem::selectObject(Entity *obj, bool replace) {
    if (replace) {
        mMouseHandler->setParentGroupAndClear(obj->getProxy().getParent());
    }
    if (mMouseHandler->getParentGroup() == obj->getProxy().getParent()) {
        mMouseHandler->addToSelection(obj->getProxyPtr());
        obj->setSelected(true);
    }
}

void OgreSystem::setLightsFromString(String s) {
    mMouseHandler->lightSelectMood(s.substr(10,1), 0);
}

void OgreSystem::tickInputHandler(const Task::LocalTime& t) const {
    if (mMouseHandler != NULL)
        mMouseHandler->tick(t);
}

} // namespace Graphics
} // namespace Sirikata
