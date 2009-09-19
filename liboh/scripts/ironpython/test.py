import uuid
import traceback

import protocol.Sirikata_pb2 as pbSiri
import protocol.Persistence_pb2 as pbPer
import protocol.MessageHeader_pb2 as pbHead
import protocol.Physics_pb2 as pbPhy

from Sirikata.Runtime import HostedObject
import System
import util
import math
import cPickle as pkl
import os
import array
import time

import blog

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

def tokenize(s):
    toks = []
    quoting=False
    t = ""
    for i in s:
        if quoting:
            if i=='"':
                quoting=False
            else:
                t += i
        else:
            if i=='"':
                quoting=True
            elif i==' ':
                toks.append(t)
                t = ""
            else:
                t += i
    toks.append(t)
    return toks

hexchars = '0123456789abcdef'

def unHex(s):
    a = array.array('B', [])
    for i in range(0, len(s), 2):
        lo = hexchars.index(s[i])
        hi = hexchars.index(s[i+1])
        a.append(lo + (hi<<4))
    return a.tostring()

def hexify(s):
    ret = array.array('c',())
    for c in s:
        lo = ord(c) & 15
        hi = ord(c) >> 4
        ret.append(hex(lo)[-1])
        ret.append(hex(hi)[-1])
    return ret.tostring()

example_singleton=None
class exampleclass:
    def __init__(self):
        global example_singleton
        if example_singleton:
            return example_singleton
        example_singleton=self
        if DEBUG_OUTPUT: print "PY: exampleclass.__init__ pid:", os.getpid(), "id:", id(self)
        self.objects={}
        self.ammoNum=0
        self.ammoMod=6
        self.arthits=set()
        self.oldhits=0
        f=open("mode.txt")
        self.mode=f.read().strip()
        f.close()
        if self.mode=="funmode":
            self.reset_funmode()
        if self.mode=="curator":
            self.reset_curator()

    def reset_funmode(self):
        #initialize Jscript
        body = pbSiri.MessageBody()
        body.message_names.append("EvaluateJavascript")
        if len(self.arthits) > self.oldhits:
            msg = 'popUpMessage("score: 0", 10, 10);'
            body.message_arguments.append(msg)
            header = pbHead.Header()
            header.destination_space = util.tupleFromUUID(self.spaceid)
            header.destination_object = util.tupleFromUUID(self.objid)
            HostedObject.SendMessage(util.toByteArray(header.SerializeToString()+body.SerializeToString()))

    def reset_curator(self):
        print "PY dbm debug: reset_curator objects length:", len(self.objects)
        for art, uid in self.objects.items():
            print "PY dbm debug reset_curator art, uid:", art, uid
            if art[:8]=="artwork_":
                self.setPosition(objid=uid, position = (0, -10, 0), orientation = (0,0,0,1) )

    pinstate = {
        "pin_1": ((-24.3, -6.35, -16.91), Euler2QuatPYR(-0.01, -50.61, -0.01) ),
        "pin_2": ((-23.66,-3.85,-17.3), Euler2QuatPYR(0,-46.63,-0.01)),
        "pin_3": ((-23.36,-6.35,-18.02), Euler2QuatPYR(0,-50.91,0))
        }

    def reallyProcessRPC(self,serialheader,name,serialarg):
        print "PY: Got an RPC named",name, "pid:", os.getpid(), "id:", id(self)
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
            tok = tokenize(s)

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
                    description = tok[3]
                    moodstring = tok[4]
                    print "PY: saveState", filename, "description:", description, "mood:", moodstring
                    self.saveStateArt={}
                    for art, uid in self.objects.items():
                        if art[:8]=="artwork_":
                            print "  PY: save", art, uid
                            self.saveStateArt[uid]={"name":art}
                            self.getPosition(objid=uid, position=1, orientation=1)
                    self.saveStateFile=filename
                    if DEBUG_OUTPUT:
                        description += " TIMESTAMP:" + str(time.ctime())
                    self.saveStateDesc=description
                    self.saveStateMood = moodstring

                elif tok[1]=="loadState":
                    data = unHex(tok[2])
                    arts = pkl.loads(data)
                    if DEBUG_OUTPUT: print "loadState:", arts
                    for art in arts:
                        if "pos" in art:
                            pos = art["pos"]
                            rot = art["rot"]
                            nam = art["name"]
                            uid = self.objects[nam]
                            self.setPosition(objid=uid, position = pos, orientation = rot,
                                 velocity = (0,0,0), axis=(0,1,0), angular_speed=0)
                        else:
                            mood = art[-1]
                    body = pbSiri.MessageBody()
                    body.message_names.append("EvaluateJavascript")
#                    msg = 'setLightMood(' +'"'+mood+'");'
                    msg = 'Client.event("navmessage", "light setmood "'+mood+'");'
                    body.message_arguments.append(msg)
                    header = pbHead.Header()
                    header.destination_space = util.tupleFromUUID(self.spaceid)
                    header.destination_object = util.tupleFromUUID(self.objid)
                    HostedObject.SendMessage(util.toByteArray(header.SerializeToString()+body.SerializeToString()))

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
                    offset = 0.5                                 ## move ammo out from inside avatar
                    x += ox*offset
                    y += oy*offset
                    z += oz*offset
                    vel = 9.0                                   ## initial ammo velocity
                    vx = -zx*vel
                    vy = -zy*vel
                    vz = -zz*vel
                    if DEBUG_OUTPUT: print "PY: rot:", qx, qy, qz, qw, "axis:", zx, zy, zz, "pos+off:", x, y, z, "vel:", vx, vy, vz
                    self.setPosition(objid=self.objects[ammo], position = (x, y, z), orientation = (qx, qy, qz, qw),
                                     velocity = (vx, vy, vz), axis=(0,1,0), angular_speed=0)

            elif tok[0]=="reset":
                if self.mode=="funmode":
                    if DEBUG_OUTPUT: print "PY: funmode reset"
                    self.arthits=set()
                    self.oldhits=0
                    print "PY debug: reset hits:", self.oldhits, self.arthits, id(self)
                    hexed = "82c60703a082460713a065e616d656a00723a06516274777f627b6f50343a00733a0376507f637a00743a08264d273e2535303030303139303733343836333a064d223e21313939393938383535353930383a06433e21393030303030353732323034363a0470753a0376527f647a00763a08264d203e27303731303637363930383439333034323a06403e203a06403e203a06403e27303731303637393332383831363433393a0470773a0371682460783a07623a06516274777f627b6f50333a00793a0377643a08264d273e24363939393937393031393136353a064d213e2037303030303035323435323038373a064d203e2035393939393939383635383839353439333a047071303a0377663a0826403e203a064d203e27333133353337303031363039383032323a06403e203a06403e26383139393833363136323632313630383a047071313a037168246071323a07623a06516274777f627b6f50323a0071333a0377643a08264d293e2334303030303135323538373839313a064d213e203a06413e253a047071343a0377663a0826403e20303031373434323636303436343433393534313a064d203e2033343839393439353534323034393430383a06463e20393131313036363239363436333833356d2030363a06403e29393933393038313138313934363435343a047071353a0371665465637362796074796f6e6c20216c637f6027796478602370716365637024594d454354514d405a364279602355607021383021303a34353a333730223030393a0071363a016e2"
                    arts = pkl.loads(unHex(hexed))
                    for art in arts:
                        if "pos" in art:
                            pos = art["pos"]
                            rot = art["rot"]
                            nam = art["name"]
                            uid = self.objects[nam]
                            self.setPosition(objid=uid, position = pos, orientation = rot,
                                 velocity = (0,0,0), axis=(0,1,0), angular_speed=0)
                    i=0
                    for ammo in self.objects:
                        print "PY checking for ammo:", ammo
                        if ammo[:5] == "ammo_":
                            print "PY ------- found ammo"
                            uid = self.objects[ammo]
                            self.setPosition(objid=uid, position = (i,-10,0), orientation = (0,0,0,1),
                                 velocity = (0,0,0), axis=(0,1,0), angular_speed=0)
                            i+=1
                    self.setPosition(objid=self.objects["Avatar_fun"], position = (-9,-1.4,2.5), orientation = (0,0,0,1),
                                 velocity = (0,0,0), axis=(0,1,0), angular_speed=0)
                    self.reset_funmode()
                if self.mode=="curator":
                    if DEBUG_OUTPUT: print "PY: curator reset"
                    self.reset_curator()

            else:
                print "PY: unknown JavascriptMessage:", tok

        elif name == "BegCol":
            collision = pbPhy.CollisionBegin()
            collision.ParseFromString(util.fromByteArray(serialarg))
            other = util.tupleToUUID(collision.other_object_reference)
            if not other in self.arthits:
                if other in self.objects.values():
                    oname = self.objects.keys()[self.objects.values().index(other)]
                    if oname[:8]=="artwork_":
                        self.arthits.add(other)
                        print "hit another painting! score now", len(self.arthits), id(self)
                        body = pbSiri.MessageBody()
                        body.message_names.append("EvaluateJavascript")
                        if len(self.arthits) > self.oldhits:
                            msg = 'popUpMessage("score: ' + str(len(self.arthits)) + '", 10, 10);'
                            body.message_arguments.append(msg)
                            header = pbHead.Header()
                            header.destination_space = util.tupleFromUUID(self.spaceid)
                            header.destination_object = util.tupleFromUUID(self.objid)
                            HostedObject.SendMessage(util.toByteArray(header.SerializeToString()+body.SerializeToString()))
                            self.oldhits=len(self.arthits)
                else:
                    print "PY debug: unknown object:", other

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
                arts = [i for i in self.saveStateArt.values()]
                arts.append(self.saveStateDesc)
                arts.append(self.saveStateMood)
                #if DEBUG_OUTPUT: print "           PY save art done:", arts
                #fname = self.saveStateFile.replace(" ", "_").replace('"',"")
                #f = open("art/" + fname, "w")
                #pkl.dump(arts, f)
                #f.close()
                #cmd = "python domail.py " + fname
                #print "PY test sendmail-->"+ cmd + "<--"
                #os.system(cmd)
                data = pkl.dumps(arts)
                print "PY: will hexify this data %s" % (data)
                hexart=hexify(data)
                blog.saveMuseum(self.saveStateFile, self.saveStateDesc, hexart)

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
            parentProperty.value=util.tupleFromUUID(uuid)
            se.data=parentProperty.SerializeToString()
            header = pbHead.Header()
            header.destination_space=util.tupleFromUUID(self.spaceid)
            header.destination_object=util.tupleFromUUID(self.objid)
            header.destination_port=5#FIXME this should be PERSISTENCE_SERVICE_PORT
            HostedObject.SendMessage(header.SerializeToString()+rws.SerializeToString())
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
            header.destination_space = util.tupleFromUUID(self.spaceid)

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
