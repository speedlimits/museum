/*  Sirikata Ogre Plugin
 *  CameraPath.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_OGRE_CAMERA_PATH_HPP_
#define _SIRIKATA_OGRE_CAMERA_PATH_HPP_

#include <util/Platform.hpp>
#include <task/Time.hpp>

using namespace std;

namespace Sirikata {
namespace Graphics {

struct CameraPoint {
    Vector3d position;
    Quaternion orientation;
    Task::DeltaTime dt;
    Task::DeltaTime time;
    String msg;
    int screen_x;
    int screen_y;
    String animate;

    CameraPoint(const Vector3d& pos, const Quaternion& orient, const Task::DeltaTime& _dt, const String& s)
            : position(pos),
            orientation(orient),
            dt(_dt),
            time(Task::DeltaTime::zero()),
            msg(s),
            screen_x(100),
            screen_y(100) {
    }
}; // struct CameraPoint


class CameraPath {
public:
    CameraPath();

    CameraPoint& operator[](int idx);
    const CameraPoint& operator[](int idx) const;

    uint32 numPoints() const;
    bool empty() const;

    Task::DeltaTime startTime() const;
    Task::DeltaTime endTime() const;

    int32 clampKeyIndex(int32 idx) const;

    Task::DeltaTime keyFrameTime(int32 idx) const;
    void changeTimeDelta(int32 idx, const Task::DeltaTime& d_dt);

    int32 insert(int32 idx, const Vector3d& pos, const Quaternion& orient, const Task::DeltaTime& dt, const String& msg);
    int32 remove(int32 idx);

    void load(const String& filename);
    void save(const String& filename);

    void normalizePath();
    void computeDensities();
    void computeTimes();

    bool evaluate(const Task::DeltaTime& t, Vector3d* pos_out, Quaternion* orient_out, String& s, int*x, int*y, String& anim);
private:
    String last_animate;
    float last_anim_vel_x, last_anim_vel_y, last_anim_vel_z;
    std::vector<CameraPoint> mPathPoints;
    std::vector<double> mDensities;
    bool mDirty;

    double str2dbl(string s) {
        float f;
        if (s=="") return 0.0;
        sscanf(s.c_str(), "%f", &f);
        return (double)f;
    }

    int str2int(string s) {
        int d;
        if (s=="") return 0;
        sscanf(s.c_str(), "%d", &d);
        return d;
    }

    void parse_csv_values(string line, vector<string>& values) {
        values.clear();
        string temp("");

        for (unsigned int i=0; i<line.size(); i++) {
            char c = line[i];
            if (c!=',') {
                if (c!='"') {
                    temp.push_back(c);
                }
            }
            else {
                values.push_back(temp);
                temp.clear();
            }
        }
        values.push_back(temp);
    }

    vector<string> headings;

    void getline(FILE* f, string& s) {
        s.clear();
        while (true) {
            char c = fgetc(f);
            if (c==-1) {
                s.clear();
                break;
            }
            if (c==0x0d && s.size()==0) continue;            /// should deal with windows \n
            if (c==0x0a || c==0x0d) {
                break;
            }
            s.push_back(c);
        }
    }

    void parse_csv_headings(FILE* fil) {
        string line;
        getline(fil, line);
        parse_csv_values(line, headings);
    }

    map<string, string>* parse_csv_line(FILE* fil) {
        string line;
        getline(fil, line);
        vector<string> values;
        map<string, string> *row;
        row = (new map<string, string>());
        if (line.size()>0) {
            parse_csv_values(line, values);
            if (values.size() == headings.size()) {
                for (unsigned int i=0; i < values.size(); i++) {
                    (*row)[headings[i]] = values[i];
                }
            }
        }
        return row;
    }
    bool loadCamPathLine(FILE *fp, CameraPoint& cp) {
        map<string, string>& row = *parse_csv_line(fp);
        if (row["pos_x"][0]=='#' or row["pos_x"]==string("")) {
            return false;                                         /// comment or blank line
        }
        else {
            cp.position.x = str2dbl(row["pos_x"]);
            cp.position.y = str2dbl(row["pos_y"]);
            cp.position.z = str2dbl(row["pos_z"]);
            cp.orientation.x = str2dbl(row["rot_x"]);
            cp.orientation.y = str2dbl(row["rot_y"]);
            cp.orientation.z = str2dbl(row["rot_z"]);
            cp.orientation.w = str2dbl(row["rot_w"]);
            cp.dt = Task::DeltaTime::seconds(str2dbl(row["delay"]));
            cp.msg = row["text"];
            cp.screen_x = str2int(row["text_x"]);
            cp.screen_y = str2int(row["text_y"]);
            return true;
        }
    }
};
} // namespace Graphics
} // namespace Sirikata

#endif //_SIRIKATA_OGRE_CAMERA_PATH_HPP_
