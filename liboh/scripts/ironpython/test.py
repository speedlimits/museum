import uuid
import traceback

import protocol.Sirikata_pb2 as pbSiri
import protocol.Persistence_pb2 as pbPer
import protocol.MessageHeader_pb2 as pbHead

from Sirikata.Runtime import HostedObject
import System
import util
import math
import cPickle as pkl

DEBUG_OUTPUT=True
DEG2RAD = 0.0174532925

def Euler2QuatPYR(pitch, yaw, roll):
    k = DEG2RAD*.5
    yawcos = math.cos(yaw*k)
    yawsin = math.sin(yaw*k)
    pitchcos = math.cos(pitch*k)
    pitchsin = math.sin(pitch*k)
    rollcos = math.cos(roll*k)
    rollsin = math.sin(roll*k)

    return (rollcos * pitchsin * yawcos + rollsin * pitchcos * yawsin,
            rollcos * pitchcos * yawsin - rollsin * pitchsin * yawcos,
            rollsin * pitchcos * yawcos - rollcos * pitchsin * yawsin,
            rollcos * pitchcos * yawcos + rollsin * pitchsin * yawsin)


def pbj2Quat(q):
    #fix weird pbj encoding
    if q[0] >= 3:
        sign = -1
        sub = 3
    else:
        sub = 0
        sign = 1
    x = q[0]-sub
    y = q[1]
    z = q[2]
    wsq = 1.0 - (x**2 + y**2 + z**2)
    w = wsq**0.5 * sign
    return x, y, z, w

DEG2RAD = 0.0174532925

def Euler2QuatPYR(pitch, yaw, roll):
    k = DEG2RAD*.5
    yawcos = math.cos(yaw*k)
    yawsin = math.sin(yaw*k)
    pitchcos = math.cos(pitch*k)
    pitchsin = math.sin(pitch*k)
    rollcos = math.cos(roll*k)
    rollsin = math.sin(roll*k)

    return (rollcos * pitchsin * yawcos + rollsin * pitchcos * yawsin,
            rollcos * pitchcos * yawsin - rollsin * pitchsin * yawcos,
            rollsin * pitchcos * yawcos - rollcos * pitchsin * yawsin,
            rollcos * pitchcos * yawcos + rollsin * pitchsin * yawsin)


class exampleclass:
    def __init__(self):
        if DEBUG_OUTPUT: print "PY: exampleclass.__init__"
        self.objects={}
        self.ammoNum=0
        self.ammoMod=6

    pinstate = {
        "pin_1": ((-24.3, -6.35, -16.91), Euler2QuatPYR(-0.01, -50.61, -0.01) ),
        "pin_2": ((-23.66,-3.85,-17.3), Euler2QuatPYR(0,-46.63,-0.01)),
        "pin_3": ((-23.36,-6.35,-18.02), Euler2QuatPYR(0,-50.91,0))
        }

    def reallyProcessRPC(self,serialheader,name,serialarg):
        print "PY: Got an RPC named",name
        header = pbHead.Header()
        header.ParseFromString(util.fromByteArray(serialheader))
        if name == "RetObj":
            retobj = pbSiri.RetObj()
            #print repr(util.fromByteArray(serialarg))
            try:
                retobj.ParseFromString(util.fromByteArray(serialarg))
            except:
                pass
            self.objid = util.tupleToUUID(retobj.object_reference)
            self.spaceid = util.tupleToUUID(header.source_space)

            self.sendNewProx()
        elif name == "ProxCall":
            proxcall = pbSiri.ProxCall()
            proxcall.ParseFromString(util.fromByteArray(serialarg))
            objRef = util.tupleToUUID(proxcall.proximate_object)
            if proxcall.proximity_event == pbSiri.ProxCall.ENTERED_PROXIMITY:
                myhdr = pbHead.Header()
                myhdr.destination_space = util.tupleFromUUID(self.spaceid)
                myhdr.destination_object = proxcall.proximate_object
                dbQuery = util.PersistenceRead(self.sawAnotherObject)
                field = dbQuery.reads.add()
                field.field_name = 'Name'
                dbQuery.send(HostedObject, myhdr)
            if proxcall.proximity_event == pbSiri.ProxCall.EXITED_PROXIMITY:
                pass
        elif name == "JavascriptMessage":
            s = "".join(chr(i) for i in serialarg)
            if DEBUG_OUTPUT: print "PY JavascriptMessage:", name, s
            tok = s.split()

            if tok[0]=="inventory":                
                if tok[1]=="placeObject":
                    painting = tok[2]
                    if not painting in self.objects:
                        print "PY ERROR painting-->" + painting + "<--", type(painting), "objects:", self.objects.keys()
                    x = float(tok[5])
                    y = float(tok[6])
                    z = float(tok[7])
                    qx = float(tok[8])
                    qy = float(tok[9])
                    qz = float(tok[10])
                    qw = float(tok[11])
                    if DEBUG_OUTPUT: print "PY moving", painting, self.objects[painting], "to", x, y, z, "quat:", qx, qy, qz, qw
                    self.setPosition(objid=self.objects[painting], position = (x, y, z), orientation = (qx, qy, qz, qw) )

                elif tok[1]=="saveState":
                    filename = tok[2]
                    print "PY: saveState", filename
                    self.saveStateArt={}
                    for art, uid in self.objects.items():
                        if art[:8]=="artwork_":
                            print "  PY: save", art, uid
                            self.saveStateArt[uid]={"name":art}
                            self.getPosition(objid=uid, position=1, orientation=1)
                    self.saveStateFile=filename

                elif tok[1]=="loadState":
                    filename = tok[2]
                    print "PY: loadState", filename
                    f = open("art/" + filename)
                    self.saveStateArt = pkl.load(f)
                    f.close()
                    if DEBUG_OUTPUT: print "loadState:", self.saveStateArt
                    for art in self.saveStateArt.values():
                        pos = art["pos"]
                        rot = art["rot"]
                        nam = art["name"]
                        uid = self.objects[nam]
                        self.setPosition(objid=uid, position = pos, orientation = rot,
                             velocity = (0,0,0), axis=(0,1,0), angular_speed=0)

            elif tok[0]=="funmode":
                if tok[1]=="fire":
                    if DEBUG_OUTPUT: print "PY: fire the cannon!", s
                    ammo = tok[2] + "_" + str(self.ammoNum)
                    self.ammoNum = (self.ammoNum+1) % self.ammoMod
                    retire = tok[2] + "_" + str(self.ammoNum)
                    self.setPosition(objid=self.objects[retire], position = (0, -10, 0), orientation = (0, 0, 0, 1),
                                     velocity = (0, 0, 0), axis=(0,1,0), angular_speed=0)
                    x = float(tok[3])
                    y = float(tok[4])
                    z = float(tok[5])
                    qx = float(tok[6])
                    qy = float(tok[7])
                    qz = float(tok[8])
                    qw = float(tok[9])
                    zx = float(tok[10])                          ## we fire down -Z axis (camera view direction)
                    zy = float(tok[11])
                    zz = float(tok[12])
##                    ox=-zx; oy=-zy; oz=-zz
                    ox=0; oy=1; oz=0
                    offset = 1.0                                ## move ammo out from inside avatar
                    x += ox*offset
                    y += oy*offset
                    z += oz*offset
                    vel = 15.0                                  ## initial ammo velocity
                    vx = -zx*vel
                    vy = -zy*vel
                    vz = -zz*vel
                    if DEBUG_OUTPUT: print "PY: rot:", qx, qy, qz, qw, "axis:", zx, zy, zz, "pos+off:", x, y, z, "vel:", vx, vy, vz
                    self.setPosition(objid=self.objects[ammo], position = (x, y, z), orientation = (qx, qy, qz, qw),
                                     velocity = (vx, vy, vz), axis=(0,1,0), angular_speed=0)
                elif tok[1]=="reset":
                    self.setPosition(objid=self.objects["Avatar_fun"], position = (0,-2.5,0), orientation = (0,0,0,1),
                                     velocity = (0,0,0), axis=(0,1,0), angular_speed=0)
                    for i in self.objects:
                        if i[:4] == "pin_":
                            pos, rot = self.pinstate[i]
                            self.setPosition(objid=self.objects[i], position = pos, orientation = rot,
                                             velocity = (0,0,0), axis=(0,1,0), angular_speed=0)
            else:
                print "PY: unknown JavascriptMessage:", tok

        elif header.reply_id==12345:
            if DEBUG_OUTPUT: print "PY: response to our location query.  Dunno why it has no name"
            loc = pbSiri.ObjLoc()
            loc.ParseFromString(util.fromByteArray(serialarg))
            pos = (loc.position[0], loc.position[1], loc.position[2])
            rot = pbj2Quat(loc.orientation)
            uid = util.tupleToUUID(header.source_object)
            art = self.saveStateArt[uid]["name"]
            self.saveStateArt[uid]["pos"]=pos
            self.saveStateArt[uid]["rot"]=rot
            done = True
            for i in self.saveStateArt.values():
                if not "pos" in i:
                    done = False
                    break
            if done:
                if DEBUG_OUTPUT: print "           PY save art done:", self.saveStateArt
                f = open("art/"+self.saveStateFile, "w")
                pkl.dump(self.saveStateArt, f)
                f.close()

    def sawAnotherObject(self,persistence,header,retstatus):
        if header.HasField('return_status') or retstatus:
            return
        uuid = util.tupleToUUID(header.source_object)
        myName = ""
        for field in persistence.reads:
            if field.field_name == 'Name':
                if field.HasField('data'):
                    nameStruct=pbSiri.StringProperty()
                    nameStruct.ParseFromString(field.data)
                    myName = nameStruct.value
        if DEBUG_OUTPUT: print "PY: Object",uuid,"has name-->" + myName + "<--",type(myName)
        if myName[:6]=="Avatar":
            rws=pbPer.ReadWriteSet()
            se=rws.writes.add()
            se.field_name="Parent"
            parentProperty=pbSiri.ParentProperty()
            parentProperty.value=util.tupleFromUUID(uuid);
            se.data=parentProperty.SerializeToString()
            header = pbHead.Header()
            header.destination_space=util.tupleFromUUID(self.spaceid);
            header.destination_object=util.tupleFromUUID(self.objid);
            header.destination_port=5#FIXME this should be PERSISTENCE_SERVICE_PORT
            HostedObject.SendMessage(header.SerializeToString()+rws.SerializeToString());
        self.objects[myName]=uuid

    def processRPC(self,header,name,arg):
        try:
            self.reallyProcessRPC(header,name,arg)
        except:
            print "PY: Error processing RPC",name
            traceback.print_exc()

    def setPosition(self,position=None,orientation=None,velocity=None,angular_speed=None,axis=None,force=False,objid=None):
        if not objid: objid = self.objid
        objloc = pbSiri.ObjLoc()
        if position is not None:
            for i in range(3):
                objloc.position.append(position[i])
        if velocity is not None:
            for i in range(3):
                objloc.velocity.append(velocity[i])
        if orientation is not None:
            total = 0
            for i in range(4):
                total += orientation[i]*orientation[i]
            total = total**.5
            for i in range(3):
                objloc.orientation.append(orientation[i]/total)
        if angular_speed is not None:
            objloc.angular_speed = angular_speed
        if axis is not None:
            total = 0
            for i in range(3):
                total += axis[i]*axis[i]
            total = total**.5
            for i in range(2):
                objloc.rotational_axis.append(axis[i]/total)
        if force:
            objloc.update_flags = pbSiri.ObjLoc.FORCE
        body = pbSiri.MessageBody()
        body.message_names.append("SetLoc")
        body.message_arguments.append(objloc.SerializeToString())
        header = pbHead.Header()
        header.destination_space = util.tupleFromUUID(self.spaceid)
        header.destination_object = util.tupleFromUUID(objid)
        HostedObject.SendMessage(util.toByteArray(header.SerializeToString()+body.SerializeToString()))

    def getPosition(self,position=0,orientation=0,velocity=0,angular_speed=0,axis=0,objid=None):
        if not objid: objid = self.objid
        locReq = pbSiri.LocRequest()
        flags = position*1 + orientation*2 + velocity*4 + axis*8 + angular_speed*16
        body = pbSiri.MessageBody()
        body.message_names.append("LocRequest")
        body.message_arguments.append(locReq.SerializeToString())
        header = pbHead.Header()
        header.destination_space = util.tupleFromUUID(self.spaceid)
        header.destination_object = util.tupleFromUUID(objid)
        print "PY debug A"
##        header.id=util.tupleFromUUID(self.objid)
        header.id=12345
        print "PY debug B"
        HostedObject.SendMessage(util.toByteArray(header.SerializeToString()+body.SerializeToString()))

    def sendNewProx(self):
        try:
            body = pbSiri.MessageBody()
            prox = pbSiri.NewProxQuery()
            prox.query_id = 123
            prox.max_radius = 1.0e+30
            body.message_names.append("NewProxQuery")
            body.message_arguments.append(prox.SerializeToString())
            header = pbHead.Header()
            header.destination_space = util.tupleFromUUID(self.spaceid);

            from System import Array, Byte
            arry=Array[Byte](tuple(Byte(c) for c in util.tupleFromUUID(self.spaceid)))
            header.destination_object = util.tupleFromUUID(uuid.UUID(int=0))
            header.destination_port = 3 # libcore/src/util/KnownServices.hpp
            headerstr = header.SerializeToString()
            bodystr = body.SerializeToString()
            HostedObject.SendMessage(util.toByteArray(headerstr+bodystr))
        except:
            print "PY: ERORR"
            traceback.print_exc()

    def processMessage(self,header,body):
        if DEBUG_OUTPUT: print "PY: Got a message"

    def tick(self,tim):
        x=str(tim)
        if DEBUG_OUTPUT: print "PY: Current time is "+x;
        #HostedObject.SendMessage(tuple(Byte(ord(c)) for c in x));# this seems to get into hosted object...but fails due to bad encoding
