/*  Sirikata Graphical Object Host
 *  CameraEntity.hpp
 *
 *  Copyright (c) 2009, Ken Turkowski
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

#ifndef SIRIKATA_GRAPHICS_LIGHTBULB_HPP__
#define SIRIKATA_GRAPHICS_LIGHTBULB_HPP__


namespace Sirikata {

class Location;

namespace Graphics {


class CameraEntity;
class Entity;

class LightBulb {
public:
    static void AttachLightBulb(
        CameraEntity *camera,           // This points to the scene
        Entity *entity,                 // The mesh will be attached to this entity
        const char *name,               // Name of the mesh (must be unique)
        const Location &loc             // Location of the mesh
    );
private:        
    LightBulb();

    static void LightBulb::initialize() {
        if (inited)
            return;
        initializeMesh();
        initializeMaterial();
        inited = true;
    }

    static void initializeMesh();
    static void initializeMaterial();
    static bool inited;
};


} // namespace Graphics
} // namespace Sirikata


#endif // SIRIKATA_GRAPHICS_LIGHTBULB_HPP__
