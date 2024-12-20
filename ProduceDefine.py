#!/usr/bin/env python

## (c) O. Kolkman 2022
## 

import sys
import re
import os.path
import gzip


def bytes_to_c_arr(data):
    return [format(b, '#04x') for b in data]


path = sys.argv[1]

print("//Generated from "+path)

_inputname = os.path.basename(path)

p1 = re.compile('-min\.')
filename = p1.sub(".",_inputname)

p2 = re.compile('\.')
_name = p2.sub("_", filename)




try:
    _file = open(path, "r")
    _data = _file.read()

except:
    print("Could not open "+path)
finally:
    _file.close()


print("#ifndef WEB"+_name+"_H")
print("#define WEB"+_name+"_H")
print()
# server->on("/FILENAME.svg", HTTP_GET, std::bind(&webInterface::handleFileName, this, std::placeholders::_1));
#
_compr = gzip.compress(bytes(_data, 'utf-8'))

print("unsigned char "+_name +
      "[] = {{{}}};".format(",".join(bytes_to_c_arr(_compr))))
print("unsigned int "+_name+"_len = ", len(_compr), ";")


print("\n\n")
print("#define DEF_HANDLE_"+_name + "  server->on(\"/"+filename, end='')
print("\", HTTP_GET, std::bind (&webInterface::handleFile, this, std::placeholders::_1", end='')
print(",\"" + sys.argv[2]+"\","+_name+","+_name+"_len));")


print("#endif")
