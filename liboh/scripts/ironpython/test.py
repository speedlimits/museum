import uuid
import traceback

import protocol.Sirikata_pb2 as pbSiri
import protocol.Persistence_pb2 as pbPer
import protocol.MessageHeader_pb2 as pbHead

from Sirikata.Runtime import HostedObject
print dir(HostedObject)
import System
import util

DEBUG_OUTPUT=True#False

class exampleclass:
    def __init__(self):
        self.avatars={}
        try:
            f = open("avatar.id")
            self.local_avatar = f.read().strip()
        except:
            self.local_avatar = None
        if DEBUG_OUTPUT: print "PY: init exampleclass, local_avatar:", self.local_avatar

    def reallyProcessRPC(self,serialheader,name,serialarg):
        if DEBUG_OUTPUT: print "PY: Got an RPC named",name
        header = pbHead.Header()
        header.ParseFromString(util.fromByteArray(serialheader))
        if name == "RetObj":
            retobj = pbSiri.RetObj()
            #print repr(util.fromByteArray(serialarg))
            try:
                retobj.ParseFromString(util.fromByteArray(serialarg))
            except:
                pass
            if DEBUG_OUTPUT: print "PY debug retobj:", dir(retobj)
            self.objid = util.tupleToUUID(retobj.object_reference)
            self.spaceid = util.tupleToUUID(header.source_space)
            if DEBUG_OUTPUT: print "PY: sendprox1", self.spaceid
            self.sendNewProx()
##            self.setPosition(angular_speed=0, axis=(0,.6,.8))
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
            if DEBUG_OUTPUT: print "PY: JavascriptMessage:", s
            tok = s.split()
            if tok[0]=="MitoMessage":
                recepient = tok[1]
                if not recepient in self.avatars:
                    print "PY ERROR from unknown avatar-->" + recepient + "<--"
                msg = self.local_avatar + "_" + tok[2]
                if DEBUG_OUTPUT: print "PY: MitoMessage", msg, "to", recepient
                self.sendMitoMessage(recepient, msg)
##            if tok[1]=="placeObject":
##                painting = tok[2]
##                if not painting in self.paintings:
##                    print "PY ERROR painting-->" + painting + "<--", type(painting), "paintings:", self.paintings.keys()
##                x = float(tok[5])
##                y = float(tok[6])
##                z = float(tok[7])
##                qx = float(tok[8])
##                qy = float(tok[9])
##                qz = float(tok[10])
##                qw = float(tok[11])
##                print "PY:   moving", painting, self.paintings[painting], "to", x, y, z, "quat:", qx, qy, qz, qw
##                self.setPosition(objid=self.paintings[painting], position = (x, y, z), orientation = (qx, qy, qz, qw) )

    def sawAnotherObject(self,persistence,header,retstatus):
        if DEBUG_OUTPUT: print "PY: sawAnotherObject1"
        if header.HasField('return_status') or retstatus:
            return
        uuid = util.tupleToUUID(header.source_object)
        myName = ""
        if DEBUG_OUTPUT: print "PY: sawAnotherObject2"
        for field in persistence.reads:
            if DEBUG_OUTPUT: print "PY: field:", field.field_name
            if field.field_name == 'Name':
                if field.HasField('data'):
                    nameStruct=pbSiri.StringProperty()
                    nameStruct.ParseFromString(field.data)
                    myName = nameStruct.value
            if field.field_name == "IsCamera":
                if DEBUG_OUTPUT: print "PY: this is the camera"
        if DEBUG_OUTPUT: print "PY: Object",uuid,"has name-->" + myName + "<--",type(myName)
        if myName==self.local_avatar:
            if DEBUG_OUTPUT: print "PY: this should be our parent"
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
        if myName[:6]=="Avatar":
            self.avatars[myName]=uuid
            if DEBUG_OUTPUT: print "PY: avatar list:", self.avatars.keys()

    def processRPC(self,header,name,arg):
        try:
            self.reallyProcessRPC(header,name,arg)
        except:
            print "PY: Error processing RPC",name
            traceback.print_exc()

    def sendMitoMessage(self, recepient, message):
        if DEBUG_OUTPUT: print "PY: sendMitoMessage", message, "to:", recepient
        body = pbSiri.MessageBody()
        body.message_names.append("MitoMessage")
        body.message_arguments.append(message)
        header = pbHead.Header()
        header.destination_space = util.tupleFromUUID(self.spaceid)
        header.destination_object = util.tupleFromUUID(self.avatars[recepient])
        HostedObject.SendMessage(util.toByteArray(header.SerializeToString()+body.SerializeToString()))

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
        if DEBUG_OUTPUT: print "PY: sendprox2"
        try:
            body = pbSiri.MessageBody()
            prox = pbSiri.NewProxQuery()
            prox.query_id = 123
            prox.max_radius = 1.0e+30
            body.message_names.append("NewProxQuery")
            body.message_arguments.append(prox.SerializeToString())
            header = pbHead.Header()
            header.destination_space = util.tupleFromUUID(self.spaceid);
            if DEBUG_OUTPUT: print "PY: time locally ",HostedObject.GetLocalTime().microseconds();

            from System import Array, Byte
            arry=Array[Byte](tuple(Byte(c) for c in util.tupleFromUUID(self.spaceid)))
            if DEBUG_OUTPUT: print "PY: time on spaceA ",HostedObject.GetTimeFromByteArraySpace(arry).microseconds()
            #print "time on spaceB ",HostedObject.GetTime(self.spaceid).microseconds()
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
