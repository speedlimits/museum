import sys,os
import smtplib
import mimetypes
from email.MIMEMultipart import MIMEMultipart
from email.MIMEBase import MIMEBase
from email.MIMEText import MIMEText
from email.MIMEAudio import MIMEAudio
from email.MIMEImage import MIMEImage
from email.Encoders import encode_base64
import array
import cPickle as pkl

template = """
:start
&nbsp;
[js] top.window.postMessage("%s", "*"); [/js]
&nbsp;
:excerptstart
%s
:excerptend
:end

"""

def hexify(s):
    ret = array.array('c')
    for c in s:
        lo = ord(c) & 15
        hi = ord(c) >> 4
        ret.append(hex(lo)[-1])
        ret.append(hex(hi)[-1])
    return ret.tostring()

def sendMail(title, description , data):
    text = template % (data, description)
    gmailUser = 'bornholm_museum@dennisschaaf.com'
    gmailPassword = '4154184042'
    recipient = 'bornholm_museum@dennisschaaf.com'
    msg = MIMEMultipart()
    msg['From'] = gmailUser
    msg['To'] = recipient
    msg['Subject'] = title
    msg.attach(MIMEText(text))
    # for attachmentFilePath in attachmentFilePaths:
    #   msg.attach(getAttachment(attachmentFilePath))
    mailServer = smtplib.SMTP('smtp.gmail.com', 587)
    mailServer.ehlo()
    mailServer.starttls()
    mailServer.ehlo()
    mailServer.login(gmailUser, gmailPassword)
    mailServer.sendmail(gmailUser, recipient, msg.as_string())
    mailServer.close()
    print('Sent email to %s' % recipient)

f=open(sys.argv[1])
art=f.read()
f.close()
hexart=hexify(art)
pyart = pkl.loads(art)
desc = pyart[-1]
print "sending mail:", sys.argv[1], desc, hexart
sendMail(sys.argv[1], desc, hexart)
