import uuid
import traceback

import protocol.Sirikata_pb2 as pbSiri
import protocol.Persistence_pb2 as pbPer
import protocol.MessageHeader_pb2 as pbHead

from Sirikata.Runtime import HostedObject
import System
import util
import math

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


class exampleclass:
    def __init__(self):
        if DEBUG_OUTPUT: print "exampleclass.__init__"
        self.objects={}

    pinstate = {
        "pin_1": ((-24.3, -6.35, -16.91), Euler2QuatPYR(-0.01, -50.61, -0.01) ),
        "pin_2": ((-23.66,-3.85,-17.3), Euler2QuatPYR(0,-46.63,-0.01)),
        "pin_3": ((-23.36,-6.35,-18.02), Euler2QuatPYR(0,-50.91,0))
        }

    def reallyProcessRPC(self,serialheader,name,serialarg):
        print "Got an RPC named",name
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
            self.setPosition(angular_speed=0, axis=(0,1,0))
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
            if DEBUG_OUTPUT: print "PY", name, s
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
                    if DEBUG_OUTPUT: print "PY:   moving", painting, self.objects[painting], "to", x, y, z, "quat:", qx, qy, qz, qw
                    self.setPosition(objid=self.objects[painting], position = (x, y, z), orientation = (qx, qy, qz, qw) )
            elif tok[0]=="funmode":
                if tok[1]=="fire":
                    if DEBUG_OUTPUT: print "PY: fire the cannon!", s
                    ammo = tok[2]
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
                    for i in self.objects:
                        if i[:4] == "pin_":
                            pos, rot = self.pinstate[i]
                            self.setPosition(objid=self.objects[i], position = pos, orientation = rot,
                                             velocity = (0,0,0), axis=(0,1,0), angular_speed=0)
            else:
                print "PY: unknown JavascriptMessage:", tok

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
        else:
            self.objects[myName]=uuid
##        elif myName[:8]=="artwork_":
##            if DEBUG_OUTPUT: print "PY: adding artwork", myName, ":", uuid
##            self.paintings[myName]=uuid
##        elif myName[:5]=="ammo_":
##            if DEBUG_OUTPUT: print "PY: adding ammo", myName, ":", uuid
##            self.ammo[myName]=uuid

    def processRPC(self,header,name,arg):
        try:
            self.reallyProcessRPC(header,name,arg)
        except:
            print "Error processing RPC",name
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
            print "ERORR"
            traceback.print_exc()

    def processMessage(self,header,body):
        if DEBUG_OUTPUT: print "PY: Got a message"

    def tick(self,tim):
        x=str(tim)
        if DEBUG_OUTPUT: print "PY: Current time is "+x;
        #HostedObject.SendMessage(tuple(Byte(ord(c)) for c in x));# this seems to get into hosted object...but fails due to bad encoding
