#!/usr/bin/python3
# version 0.9.1
# How to run python3 extra_script.py
# Utility used to convert html file in Cpp file with Get Set facility.
#  python Convertor html to Cpp  installed in plateformio.ini and convert at compile.
#  extra_scripts = pre:extra_script.py
#  custom_in_html = src/eau.html
#  custom_out_h = src/eau.h
# ---------------------------
# sample : extract list of keyword (%%___%%)
# MacBook-Pro-de-Bruno:Esp32_Jeedom dupuyb$ python3 extra_script.py -f src/eau.html 
# Key list   : ['Cmd', 'DATE', 'DFE', 'FL', 'HOS', 'IP', 'IPL', 'MAC', 'MFREE', 'MT', 'RB', 'TITLE', 'TOE', 'Tst', 'VL']
# Number Key : 15
# Max Key len: 5

import sys, getopt, datetime, pathlib, re
from tempfile import mkstemp
from shutil import move
from os import remove

# desabled pylint ...
# pylint: disable=unused-variable

# Pattern arroud KEY here ##Key##
REGPAT=r"%%\w*%%"
REGS="%%"
REGE="%%"

def help(opt):
    print ('FrameWeb.py  -f <find_file> extract '+REGS+'__'+REGE+' pattern from file.')
    print ('             -i <inputfile> [-o <outputfile>] compress htnl to cpp')
    print ('             -i <inputfile> -b <outputfile> build a new cpp')
    sys.exit(opt)

def replace(source_file_path, code):
    fh,target_file_path = mkstemp()
    cpok=True
    error=True
    with open(target_file_path, 'w') as target_file:
        with open(source_file_path, 'r') as source_file:
            for line in source_file:
                if line.startswith('//---- Start Generated'):
                    cpok = False
                    for x in range(0, len(code)):
                        target_file.write(str(code[x]+"\n"))
                if cpok:
                    target_file.write(line)
                if line.startswith('//---- End Generated'):
                    if cpok == False:
                        error=False
                    cpok=True
    if error:
        print('>>> Error into replace HTML Generated file :'+ str(source_file))
        sys.exit(5)
    remove(source_file_path)
    move(target_file_path, source_file_path)

def GetListOfSubstrings(stringSubject,string1,string2):
    MyList = []
    intstart=0
    strlength=len(stringSubject)
    continueloop = 1
    while(intstart < strlength and continueloop == 1):
        intindex1=stringSubject.find(string1,intstart)
        if(intindex1 != -1): #The substring was found, lets proceed
            intindex1 = intindex1+len(string1)
            intindex2 = stringSubject.find(string2,intindex1)
            if(intindex2 != -1):
                subsequence=stringSubject[intindex1:intindex2]
                MyList.append(subsequence)
                intstart=intindex2+len(string2)
            else:
                continueloop=0
        else:
            continueloop=0
    return MyList

def findPattern(filename):
    tag = []
    pattern = re.compile(REGPAT, re.IGNORECASE)
    with open(filename, "rt") as myfile:
        for line in myfile:
            if pattern.search(line) != None:
                # print(line, end='')
                ll = GetListOfSubstrings(line, REGS, REGE)
                for x in range(0, len(ll)):
                    tag.append(ll[x])
    tag = list(set(tag))
    tag.sort()
    strlen = 0
    for x in range(0, len(tag)):
        strlen=max (len(str(tag[x])), strlen)
    return tag, strlen

def buildEsp32Cpp(inputfile, outputfile):
    if (outputfile.strip()):
        fo = open(outputfile, 'a') 
    na=outputfile
    na=na.replace(".h", "").strip().upper()
    fo.write("#ifndef "+na+"_h\n")
    fo.write("#define "+na+"_h\n\n")
    ln = conpressHtml(inputfile)
    for x in range(0, len(ln)):
        fo.write(str(ln[x])+"\n")
    fo.write("\n// -------- Web tranlat wrapper Get Set  -------------\n")
    tg, strlen = findPattern(inputfile)
    strlen = strlen + len(REGS) + len(REGE) + 1
    for x in range(0, len(tg)):
        fo.write("char* get"+str(tg[x])+"() { /* DOTO code here */ return NULL;}\n")
        fo.write("void  set"+str(tg[x])+"(String s) { /* DOTO code here */ ;}\n")
    fo.write("\nstruct Equiv {\n  char key["+str(strlen)+"];\n  char* (*get_ptr)(void);\n  void  (*set_ptr)(String);\n};\n\n")
    fo.write("#define NBRITEMINDICO "+ str(len(tg))+"\n\n" )
    fo.write("Equiv dico[] ={\n")
    for x in range(0, len(tg)):
        fo.write("{\"%%"+str(tg[x])+"%%\", &get"+str(tg[x])+", &set"+str(tg[x])+" },\n")
    fo.write("};\n\n")
    fo.write("String sentHtml"+na+"(){\n")
    fo.write("// get if action\n\
  if (frame.server.method() == HTTP_POST frame.server.method() == HTTP_GET) {\n\
    for (uint8_t i=0; i<frame.server.args(); i++) {\n\
      String str = \"Arg->[\" + frame.server.argName(i) + \"]:[\" + frame.server.arg(i)+\"]\"; Serial.println( str.c_str() );\n\
      // Decoder code here\n\
    }\n\
    /* Save here */;\n\
  }  \n\
  } \n\
  return rt; \n\
}\n\n") 
    fo.write("#endif\n")

def conpressHtml(inputfile):
    ret=[]
    try:
        fn = open(inputfile,"r")
    except IOError:
        print('file not found:',inputfile)
        help(3)
    ret.append("//---- Start Generated from "+ inputfile +" file --- "+ str(datetime.datetime.now()) )
    sub_str = "<!-- const char HTTP_"
    espline = ""
    mode = 0
    # read the content of the file line by line 
    cont = fn.readlines() 
    type(cont) 
    for i in range(0, len(cont)): 
        if mode==1:
            if (cont[i].strip().find(sub_str) != 0):
                line = cont[i].replace('"', '\\"').replace('\n', '').strip() 
                espline += line
            else:
                espline +='";'
                ret.append(espline)
                ret.append("//---- len : "+str(len(espline))+" bytes")
                espline=""
                mode=0
        if (cont[i].strip().find(sub_str) == 0):
            if mode==0:
                mode=1
                line = cont[i]
                line = line.replace("<!--", "").replace("-->", "").strip() + ' "'
                espline += line
    if (mode==1):
        espline +='";'
        ret.append(espline)
        ret.append("//---- len : "+str(len(espline))+" bytes")
    ret.append("//---- End Generated ")
    # close all files 
    fn.close() 
    return ret

def compressHtml(inputfile, outputfile):
    if (outputfile.strip()):
        fo = open(outputfile, 'a') 
    ln = conpressHtml(inputfile)
    for x in range(0, len(ln)):
        if (outputfile.strip()):
            fo.write(str(ln[x])+"\n")
        else:
            print(ln[x])
    if outputfile.strip():
        fo.close

def selesctApp(argv):
    app2call=0
    inputfile = ''
    outputfile = ''
    findfile = ''
    try:
      opts, args = getopt.getopt(argv,"hi:o:f:b:",["ifile=","ffile","ofile=","bfile"])
    except getopt.GetoptError:
      help(2)
    for opt, arg in opts:
      if opt == '-h':
        help(1)
      elif opt in ("-i", "--ifile"):
        app2call=1
        inputfile = arg.strip()
      elif opt in ("-o", "--ofile"):
        app2call=1
        outputfile = arg.strip()
      elif opt in ("-b", "--bfile"):
        app2call=3
        outputfile = arg.strip()
      elif opt in ("-f", "--ffile"):
        app2call=2
        findfile = arg.strip()
    if app2call==1:
        compressHtml(inputfile, outputfile)
    elif app2call==2:
        tg, ln = findPattern(findfile)
        print ('Key list   :',tg)
        print ('Number Key :', len(tg))
        print ('Max Key len:',ln)
    elif app2call==3:
        buildEsp32Cpp(inputfile, outputfile)
    else:
        help(3)

if __name__ == "__main__":
    selesctApp(sys.argv[1:])
    sys.exit(1)

# code here executed when called from platformIO
try:
    import configparser
except ImportError:
    import ConfigParser as configparser
#Import("env")
config = configparser.ConfigParser()
config.read("platformio.ini")
print(config)
inputfile = config.get("env:esp32dev", "custom_in_html")
outputfile = config.get("env:esp32dev", "custom_out_h")
print('---> EXTRACT HTML FILE :'+inputfile+'--------------------')
tg, ln = findPattern(inputfile)
print ('Key list   :',tg)
print ('Number Key :',len(tg))
print ('Max Key len:',ln)
code = conpressHtml(inputfile)
replace(outputfile, code )
print('---> END OF HTML FILE :'+outputfile+'--------------------')
