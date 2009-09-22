#!/usr/bin/python
import sys
import os
import random
def sha256(filename):
    cmd = "./sha256 -256 " + filename
    sin, sout = os.popen2(cmd)
    s = sout.read().split()[-1]
    return s

for filename in sys.argv[1:]:
    f=open(filename)
    s=f.read()
    f.close()
    sfixed=s.replace("scene_blend alpha_blend","scene_blend  alpha_blend\ndepth_write off");
    sfixed=sfixed.replace("scene_blend src_alpha one","scene_blend  src_alpha one\ndepth_write off");
    if sfixed!=s:
        tempname=str(random.randint(0,1<<30))
        of=open(tempname,"wb")
        of.write(sfixed)
        of.close()
        ss=sha256(tempname)
        os.rename(tempname,ss)
        for redirect in os.listdir("../names"):
            redirect="../names/"+redirect
            n=open(redirect)
            s=n.read()
            n.close()
            sfixed=s.replace(filename,ss)
            if (sfixed!=s):
                os.rename(redirect,redirect+"munged")
                n=open(redirect,'w')
                n.write(s.replace(filename,ss))
                n.close()
