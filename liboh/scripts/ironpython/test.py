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

DEBUG_OUTPUT=False
DEG2RAD = 0.0174532925

#picture placement for fun mode (get it from critic output)
FUNSTATE = "82c60703a082460713a065e616d656a00723a06516274777f627b6f51303a00733a0376507f637a00743a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a0470753a0376527f647a00763a0826403e203a06403e203a06403e203a06413e203a0470773a0371682460783a07623a06516274777f627b6f53333a00793a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a047071303a0377663a0826403e203a06403e203a06403e203a06413e203a047071313a037168246071323a07623a06516274777f627b6f50373a0071333a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a047071343a0377663a0826403e203a06403e203a06403e203a06413e203a047071353a037168246071363a07623a06516274777f627b6f53313a0071373a0377643a08264d263e2937373431333537313837313338333a064d223e21363930303030343535303037363a06423e28333632373534393837313032323a047071383a0377663a08264d203e24353935313137383636393932393530343a06403e25333734343636373736383437383339343a06403e25333734343636313830383031333931363a06403e24353935313138323730393934303138363a047071393a037168246072303a07623a06516274777f627b6f53393a0072313a0377643a08264d293e2239313138303030303030303030313a06403e2038393933323330303030303030303030373a06403e28323333373039393939393939393939363a047072323a0377663a0826453e29353532393936383435303133363935356d2030373a06403e29393933393039303031333530343032383a064d213e27303535303939333036313632343437356d2030353a06403e20333438393733393837303032333132333a047072333a037168246072343a07623a06516274777f627b6f50343a0072353a0377643a08264d273e2338303135393536313834393639383a064d213e2230303030303031373838313339333a064d203e263233363432343839393634323434313a047072363a0377663a0826403e203a064d203e2733313335333735393736353632353a06403e203a06403e26383139393832393737303830353437363a047072373a037168246072383a07623a06516274777f627b6f50353a0072393a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a047073303a0377663a0826403e203a06403e203a06403e203a06413e203a047073313a037168246073323a07623a06516274777f627b6f50323a0073333a0377643a08264d293e2735313937303236343338353134373a064d213e2230303030303031373838313339333a064d243e2833393630303633363636393234383a047073343a0377663a0826403e203a064d203e20333438393935363235393732373437383a06403e203a06403e29393933393038323437313739393733343a047073353a037168246073363a07623a06516274777f627b6f52383a0073373a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a047073383a0377663a0826403e203a06403e203a06403e203a06413e203a047073393a037168246074303a07623a06516274777f627b6f53353a0074313a0377643a08264d223e27393035373a064d213e25343431323a06423e263631373a047074323a0377663a0826403e203a064d203e273331333533393938313834323034313a06403e203a06403e26383139393830343230333532393733353a047074333a037168246074343a07623a06516274777f627b6f51333a0074353a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a047074363a0377663a0826403e203a06403e203a06403e203a06413e203a047074373a037168246074383a07623a06516274777f627b6f52323a0074393a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a047075303a0377663a0826403e203a06403e203a06403e203a06413e203a047075313a037168246075323a07623a06516274777f627b6f53373a0075333a0377643a08264d21323e2931353a064d213e23373635373a06403e25363939383730303030303030303030323a047075343a0377663a0826403e203a06403e29393933393130313933343433323938333a06403e203a06403e2033343839333938383739333231393833313a047075353a037168246075363a07623a06516274777f627b6f53323a0075373a0377643a08264d253e2938303335353838373938363137363a064d213e2630303030303032333834313835383a06403e23333131383335303637333234383330393a047075383a0377663a0826403e203a064d203e27333133353334363137343234303131323a06403e203a06403e26383139393836313732393837343935353a047075393a037168246076303a07623a06516274777f627b6f50313a0076313a0377643a08264d21333e2135373438303836313336363a064d213e23303030303030313933373135313a064d253e20373737333736383232313639353a047076323a0377663a0826403e203a064d203e20333438393935363235393732373437383a06403e203a06403e29393933393038323437313739393733343a047076333a037168246076343a07623a06516274777f627b6f52353a0076353a0377643a08264d273e2538313433303132393430363434383a064d213e2630303030303032333834313835383a06443e2636323434323130383638303634393a047076363a0377663a0826403e203a06403e29393933393038343035333033393535313a06403e203a06403e2033343839393130393738373335303936373a047076373a037168246076383a07623a06516274777f627b6f53383a0076393a0377643a08264d21313e223335353a064d213e23323834393a06403e26383734313339393939393939393939373a047077303a0377663a08264d243e28363438343930373436393237353634356d2030373a06403e29393933393038343035333033393535313a06413e23393331303937373832393135343538356d2030353a06403e2033343839393130373030333433363935313a047077313a037168246077323a07623a06516274777f627b6f52373a0077333a0377643a08264d293e27333232333a064d213e25383935343a06413e22343336333a047077343a0377663a0826423e28303630393839323739373635363336356d2030353a064d203e2033343837363538383733323030343136363a06493e27393236353637383332343733363635356d2030373a06403e29393933393136323633323537353635313a047077353a037168246077363a07623a06516274777f627b6f51393a0077373a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a047077383a0377663a0826403e203a06403e203a06403e203a06413e203a047077393a037168246078303a07623a06516274777f627b6f52333a0078313a0377643a08264d253e29303838353a064d213e26393537373a06443e27323932383a047078323a0377663a0826403e203a06403e29393933393038343035333033393535313a06403e203a06403e2033343839393130393738373335303936373a047078333a037168246078343a07623a06516274777f627b6f51313a0078353a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a047078363a0377663a0826403e203a06403e203a06403e203a06413e203a047078373a037168246078383a07623a06516274777f627b6f52393a0078393a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a047079303a0377663a0826403e203a06403e203a06403e203a06413e203a047079313a037168246079323a07623a06516274777f627b6f53303a0079333a0377643a08264d21303e263034323a064d213e27323035313a06423e243938363a047079343a0377663a0826403e203a06403e26383139393830313434353030373332343a06403e203a06403e27333133353430323339303738313833343a047079353a037168246079363a07623a06516274777f627b6f50333a0079373a0377643a08264d273e21383536313734373930363236313a064d213e23303030303030313933373135313a064d233e2430353731363434373531333338333a047079383a0377663a0826403e203a064d203e2733313335333735393736353632353a06403e203a06403e26383139393832393737303830353437363a047079393a03716824607130303a07623a06516274777f627b6f51363a007130313a0377643a08264d233e27313436373a064d213e26383734373a06443e28383237313a04707130323a0377663a0826403e203a06403e29393933393038343035333033393535313a06403e203a06403e2033343839393130393738373335303936373a04707130333a03716824607130343a07623a06516274777f627b6f50393a007130353a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a04707130363a0377663a0826403e203a06403e203a06403e203a06413e203a04707130373a03716824607130383a07623a06516274777f627b6f51343a007130393a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a04707131303a0377663a0826403e203a06403e203a06403e203a06413e203a04707131313a03716824607131323a07623a06516274777f627b6f51373a007131333a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a04707131343a0377663a0826403e203a06403e203a06403e203a06413e203a04707131353a03716824607131363a07623a06516274777f627b6f50363a007131373a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a04707131383a0377663a0826403e203a06403e203a06403e203a06413e203a04707131393a03716824607132303a07623a06516274777f627b6f51323a007132313a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a04707132323a0377663a0826403e203a06403e203a06403e203a06413e203a04707132333a03716824607132343a07623a06516274777f627b6f52343a007132353a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a04707132363a0377663a0826403e203a06403e203a06403e203a06413e203a04707132373a03716824607132383a07623a06516274777f627b6f50383a007132393a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a04707133303a0377663a0826403e203a06403e203a06403e203a06413e203a04707133313a03716824607133323a07623a06516274777f627b6f51383a007133333a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a04707133343a0377663a0826403e203a06403e203a06403e203a06413e203a04707133353a03716824607133363a07623a06516274777f627b6f51353a007133373a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a04707133383a0377663a0826403e203a06403e203a06403e203a06413e203a04707133393a03716824607134303a07623a06516274777f627b6f52313a007134313a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a04707134323a0377663a0826403e203a06403e203a06403e203a06413e203a04707134333a03716824607134343a07623a06516274777f627b6f52363a007134353a0377643a0826403e203a064d21303e203a06403e22393939393939393939393939393939393a04707134363a0377663a0826403e203a06403e203a06403e203a06413e203a04707134373a03716824607134383a07623a06516274777f627b6f52303a007134393a0377643a08264d223e29303133373a064d213e25313533333a06443e22343631313a04707135303a0377663a0826403e203a064d203e273331333533393938313834323034313a06403e203a06403e26383139393830343230333532393733353a04707135313a03716824607135323a07623a06516274777f627b6f53363a007135333a0377643a08264d21343e263335383a064d213e23373138393a064d223e21353234373a04707135343a0377663a0826403e203a06403e26383139393830313434353030373332343a06403e203a06403e27333133353430323339303738313833343a04707135353a03716824607135363a07623a06516274777f627b6f53343a007135373a0377643a08264d233e2430323839313231393136363237363a064d213e2530303030303032323335313734323a06403e25363635373135303031313930363435343a04707135383a0377663a0826403e203a064d203e2033343839393436323031343433363732323a06403e203a06403e29393933393038323832333034323930323a04707135393a0371665e65656460247f6021646460216c6c602071696e64796e676370247f602e2363767a007136303a01665b5a007136313a016e2"
#that probably broke our style guide at > 9000 columns
FLYSTATE=FUNSTATE

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

def tokenize(s, last=99999):
    #last == last token; returns remainder of string after
    toks = []
    quoting=False
    t = ""
    for n, i in enumerate(s):
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
        if len(toks) == last:
            toks.append(s[n+1:])
            return toks
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
        global example_singleton, DEBUG_OUTPUT
        if example_singleton:
            return example_singleton
        example_singleton=self
        if DEBUG_OUTPUT: print "PY: exampleclass.__init__ pid:", os.getpid(), "id:", id(self)
        self.objects={}
        self.ammoNum=0
        self.ammoMod=6
        self.arthits=set()
        self.oldhits=0
        self.score=0
        f=open("mode.txt")
        while 1:
            s = f.readline().strip()
            if s=="":
                break
            w = s.split("=")
            if w[0]=="mode":
                self.mode = w[1]
            if w[0]=="debug":
                DEBUG_OUTPUT=True if w[1]=="True" else False
        print "PY: mode=",self.mode
        f.close()
        if self.mode=="funmode":
            self.reset_funmode()
        if self.mode=="curator":
            self.reset_curator()

    gamenum=0.0
    gameon=0
    def reset_funmode(self):
        if DEBUG_OUTPUT: print "PY dbm debug: reset_funmode objects length:", len(self.objects)
        self.gamenum += 0.5     #please don't ask
        self.timestart=time.time()
        self.lasttime=0.0
        self.gameon=0
        #reposition avatar
        if "Avatar_fun" in self.objects:
            if DEBUG_OUTPUT: print "PY dbm debug reset_funmode Avatar_fun"
            self.setPosition(objid=self.objects["Avatar_fun"], position = (-13.16,-1.4,-4.22), orientation = (0,-.86,0.0,.51),
                     velocity = (0,0,0), axis=(0,1,0), angular_speed=0)

    def reset_curator(self):
        if DEBUG_OUTPUT: print "PY dbm debug: reset_curator objects length:", len(self.objects)
        if "Avatar" in self.objects:
            if DEBUG_OUTPUT: print "PY dbm debug reset_curator Avatar"
            self.setPosition(objid=self.objects["Avatar"], position = (-13.16,-1.0,-4.22), orientation = (0,-.86,0.0,.51),
                         velocity = (0,0,0), axis=(0,1,0), angular_speed=0)
        for art, uid in self.objects.items():
            if art[:8]=="artwork_":
                if DEBUG_OUTPUT: print "PY dbm debug reset_curator art, uid:", art, uid
                self.setPosition(objid=uid, position = (0, -10, 0), orientation = (0,0,0,1) )

    def reset_critic(self):
        if DEBUG_OUTPUT: print "PY dbm debug: reset_critic objects length:", len(self.objects)
        if "Avatar" in self.objects:
            if DEBUG_OUTPUT: print "PY dbm debug reset_critic Avatar"
            self.setPosition(objid=self.objects["Avatar"], position = (-13.16,-1.0,-4.22), orientation = (0,-.86,0.0,.51),
                         velocity = (0,0,0), axis=(0,1,0), angular_speed=0)
        for art, uid in self.objects.items():
            if art[:8]=="artwork_":
                self.setPosition(objid=uid, position = (0, -10, 0), orientation = (0,0,0,1) )

    def reset_flythru(self):
        if DEBUG_OUTPUT: print "PY dbm debug: reset_flythru objects length:", len(self.objects)
        #close the doors
        self.setPosition(objid=self.objects["b7_firedoor_door"], position = (-14.9, -1.16, -4.41))
        self.setPosition(objid=self.objects["entry_door_01"], orientation = (0,0,0,1))
        self.setPosition(objid=self.objects["entry_door_02"], orientation = (0,0,0,1))

    def popUpMsg(self, s):
        body = pbSiri.MessageBody()
        body.message_names.append("EvaluateJavascript")
        msg = 'popUpMessage("' + s  + '", 10, 10);'
        if DEBUG_OUTPUT: print "PY:", msg
        body.message_arguments.append(msg)
        header = pbHead.Header()
        header.destination_space = util.tupleFromUUID(self.spaceid)
        header.destination_object = util.tupleFromUUID(self.objid)
        HostedObject.SendMessage(util.toByteArray(header.SerializeToString()+body.SerializeToString()))

    def updateBanner(self):
        t = int(65-self.lasttime)
        if t>60:
            s = "<h1>ER DU KLAR???</h1>"
        elif t<=0:
            s = "<h1>GAME OVER. DIN SCORE VAR " + str(self.score) + "</h1>"
            self.gameon=0
        else:
            self.gameon=1
            s = "<h1>TID " + str(t) + " - SCORE " + str(self.score) + "</h1>"
        #+ " ID: " + str(id(self))  + " test.py"
        self.popUpMsg(s)

    def reallyProcessRPC(self,serialheader,name,serialarg):
        if DEBUG_OUTPUT: print "PY: Got an RPC named",name, "pid:", os.getpid(), "id:", id(self)
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
                    tok = tokenize(s,4)
                    filename = tok[2]
                    description = tok[3]
                    moodstring = tok[4]             #this will be remainder of string, not a token
                    print "PY: saveState", filename, "description:", description, "mood:", moodstring
                    self.saveStateArt={}
                    for art, uid in self.objects.items():
                        if art[:8]=="artwork_":
                            print "  PY: save", art, uid
                            self.saveStateArt[uid]={"name":art}
                            self.getPosition(objid=uid, position=1, orientation=1)
                    self.saveStateFile=filename
                    #if DEBUG_OUTPUT:
                    #    description += " TIMESTAMP:" + str(time.ctime())
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
                            mood = arts[-1]
                    body = pbSiri.MessageBody()
                    body.message_names.append("SetLightMood")
                    msg = mood
                    if DEBUG_OUTPUT: print "PY: loadState set lighting mood-->" + mood + "<--", " len:", len(mood)
                    body.message_arguments.append(msg)
                    header = pbHead.Header()
                    header.destination_space = util.tupleFromUUID(self.spaceid)
                    header.destination_object = util.tupleFromUUID(self.objid)
                    HostedObject.SendMessage(util.toByteArray(header.SerializeToString()+body.SerializeToString()))

            elif tok[0]=="funmode" and self.mode=="funmode":
                if tok[1]=="fire":
                    if not self.gameon:
                        return
                    if DEBUG_OUTPUT: print "PY: fire the cannon!", s, "id:", id(self)
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
                    vel = 7.0                                   ## initial ammo velocity
                    vx = -zx*vel
                    vy = -zy*vel
                    vz = -zz*vel
                    if DEBUG_OUTPUT: print "PY: rot:", qx, qy, qz, qw, "axis:", zx, zy, zz, "pos+off:", x, y, z, "vel:", vx, vy, vz
                    self.setPosition(objid=self.objects[ammo], position = (x, y, z), orientation = (qx, qy, qz, qw),
                                     velocity = (vx, vy, vz), axis=(0,1,0), angular_speed=0)
                elif tok[1]=="timer":
                    t = time.time()-self.timestart
                    if t > self.lasttime+1.0:
                        self.lasttime+=1.0
                        if DEBUG_OUTPUT: print "PY: timer tick, time now:", self.lasttime
                        self.updateBanner()

            elif tok[0]=="reset":
                if self.mode=="funmode" or self.mode=="flythru":
                    if DEBUG_OUTPUT: print "PY: funmode/flythru reset paintings"
                    hexed = FUNSTATE if self.mode=="funmode" else FLYSTATE
                    arts = pkl.loads(unHex(hexed))
                    count = 0
                    for art in arts:
                        if "pos" in art:
                            pos = art["pos"]
                            rot = art["rot"]
                            nam = art["name"]
                            if nam in self.objects:
                                count +=1
                                uid = self.objects[nam]
                                if self.mode=="funmode":
                                    if nam=="artwork_32":       #must you?
                                        pos = (-5, 0, 3)
                                        rot = (.71, 0, 0, .71)
                                    self.setPosition(objid=uid, position = pos, orientation = rot,
                                         velocity = (0,0,0), axis=(0,1,0), angular_speed=0)
                                else:
                                    self.setPosition(objid=uid, position = pos, orientation = rot)
                    if DEBUG_OUTPUT: print "PY debug: total artwork repositioned:", count
                    if self.mode=="flythru":
                        if DEBUG_OUTPUT: print "PY: flythru reset"
                        self.reset_flythru()
                    else:

                        self.arthits=set()
                        self.oldhits=0
                        print "PY debug: reset hits:", self.oldhits, self.arthits, id(self)
                        i=0
                        for ammo in self.objects:
                            if DEBUG_OUTPUT: print "PY checking for ammo:", ammo
                            if ammo[:5] == "ammo_":
                                if DEBUG_OUTPUT: print "PY ------- found ammo"
                                uid = self.objects[ammo]
                                self.setPosition(objid=uid, position = (i,-10,0), orientation = (0,0,0,1),
                                     velocity = (0,0,0), axis=(0,1,0), angular_speed=0)
                                i+=1
                        self.reset_funmode()
                elif self.mode=="curator":
                    if DEBUG_OUTPUT: print "PY: curator reset"
                    self.reset_curator()

                elif self.mode=="critic":
                    if DEBUG_OUTPUT: print "PY: curator reset"
                    self.reset_critic()

                else:
                    print "PY ERROR: unknown mode", self.mode

##                if self.mode=="flythru":
##                    if DEBUG_OUTPUT: print "PY: flythru reset"
##                    self.reset_flythru()

            elif tok[0]=="flythru":
                if tok[1]=="animation":
                    door = "b7_firedoor_door"
                    left = "entry_door_01"
                    right = "entry_door_02"
                    if tok[2]=="gallery_open_start":
                        print "PY: Gallery doors open start"
                        if door in self.objects:
                            print "    -- got the door!", self.objects[door]
                            zmove = 1.0/float(tok[3])
                            xmove = zmove * -.08
                            self.setPosition(objid=self.objects[door], velocity = (xmove,0,zmove))
                    elif tok[2]=="gallery_open_finish":
                        if door in self.objects:
                            self.setPosition(objid=self.objects[door], velocity = (0,0,0))
                        print "PY: Gallery doors open finish"
                    elif tok[2]=="front_open_start":
                        print "PY: front doors open start"
                        if left in self.objects and right in self.objects:
                            speed = 1.0/float(tok[3])
                            self.setPosition(objid=self.objects[left], axis = (0,1,0), angular_speed=speed)
                            self.setPosition(objid=self.objects[right], axis = (0,1,0), angular_speed=-speed)
                    elif tok[2]=="front_open_finish":
                        print "PY: front doors open finish"
                        if left in self.objects and right in self.objects:
                            self.setPosition(objid=self.objects[left], axis = (0,1,0), angular_speed=0)
                            self.setPosition(objid=self.objects[right], axis = (0,1,0), angular_speed=0)

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
                        if len(self.arthits) > self.oldhits:
                            self.score = len(self.arthits)
##                            self.popUpMsg('game: ' + str(int(self.gamenum)) + ' score: ' + str(len(self.arthits)) )
                            self.oldhits=len(self.arthits)
                            self.updateBanner()
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
