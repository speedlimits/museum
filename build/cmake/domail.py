import sys,os
import smtplib
import mimetypes
from email.MIMEMultipart import MIMEMultipart
from email.MIMEBase import MIMEBase
from email.MIMEText import MIMEText
from email.MIMEAudio import MIMEAudio
from email.MIMEImage import MIMEImage
from email.Encoders import encode_base64

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

args = []
for i in sys.argv[1:]:
    args.append(i.replace("_"," "))

f=open(args[-1])
art=f.read()
f.close()
args[-1]=art
print "sending mail:", args
##sendMail(*args)
