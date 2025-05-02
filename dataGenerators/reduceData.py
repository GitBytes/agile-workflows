# The AGILE Workflows
#
# Copyright (c) 2025 Battelle Memorial Institute
#
# Battelle Memorial Institute (hereinafter Battelle) hereby grants permission to
# any person or entity lawfully obtaining a copy of this software and associated
# documentation files (hereinafter “the Software”) to redistribute and use the
# Software in source and binary forms, with or without modification.  Such person
# or entity may use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and may permit others to do so, subject to the
# following conditions:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimers.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Other than as used herein, neither the name Battelle Memorial Institute or
#    Battelle may be used in any form whatsoever without the express written
#    consent of Battelle.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL BATTELLE OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os
import sys

data = open(sys.argv[1], "r")
dataOut = open(sys.argv[2], "w")

line1 = "#delimieter: ,\n"
line2 = "#columns:type,person1,person2,forum,forum_event,publication,topic,date,lat,lon\n"
line3 = "#types:STRING,UINT,UINT,UINT,UINT,UINT,UINT,USDATE,DOUBLE,DOUBLE\n"

dataOut.write(line1)
dataOut.write(line2)
dataOut.write(line3)
lines = { "Sale,1419850416906085161,1128501731262832684,,,,2869238,09/28/2018,,\n",
          "Sale,477384404927196020,1128501731262832684,,,,271997,09/30/2018,,\n",
          "Sale,1472154222902711100,1128501731262832684,,,,185785,09/28/2018,,\n",
          "Sale,735713441679521195,1128501731262832684,,,,11650,10/10/2018,,\n",
          "Sale,1472154222902711100,529550602103217450,,,,185785,2/17/2019,,\n"
        }

# initialize persons, publications and forum_events to insure pattern
persons      = {"1128501731262832684", "1419850416906085161", "477384404927196020", 
                "1472154222902711100", "735713441679521195",  "529550602103217450"}
forums       = {"1202482536733844323", "1372844135435303981"}
forumEvents  = {"1114502034902546550", "1513662032452523252", "1060309546214304182", \
                "1209342585680609487", "932362105613871012"}
publications = {"1433303251800176474"}
topics = {"60", "11650", "43035", "44311", "127197", "179057", "185785", "271997", "771572", \
          "1049632", "2030894", "2869238", "69871376"}

sale_count = 0
forumEvent_count = 0
publication_count = 0

skip = int( 1.0 / float(sys.argv[3]) )

########### CHOOSE PURCHASES, FORUM EVENTS AND PUBLICATIONS ##########
###########        GATHER BUYERS, SELLERS AND FORUMS        ##########
while True:
  line = data.readline()
  if line == "": break
  if line[0] == '#': continue

  fields = line[:-1].split(',')

# sale edge
  if fields[0] == "Sale":
     if ((sale_count % skip) == 0):
        lines.add(line)
        if fields[1] != "": persons.add(fields[1])
        if fields[2] != "": persons.add(fields[2])
        if fields[6] != "": topics.add(fields[6])
     sale_count += 1

# forum event vertex
  elif fields[0] == "ForumEvent":
     if fields[4] in forumEvents:
        dataOut.write(line)
     elif (forumEvent_count % skip) == 0:
        dataOut.write(line)
        if fields[3] != "": forums.add(fields[3])
        if fields[4] != "": forumEvents.add(fields[4])
     forumEvent_count += 1

# publication vertex
  elif fields[0] == "Publication":
     if fields[5] in publications:
        dataOut.write(line)
     elif (publication_count % skip) == 0:
        dataOut.write(line)
        if fields[5] != "": publications.add(fields[5])
     publication_count += 1

data.close()
data = open(sys.argv[1], "r")

########### WRITE AUTHORS, FORUMS, INCLUDES, HAS_TOPICS AND HAS_ORG RECORDS ##########
###########              ADD AUTHORS TO PERSONS; GATHER TOPICS              ##########
while True:
  line = data.readline()
  if line == "": break
  if line[0] == '#': continue

  fields = line[:-1].split(',')

# forum vertex
  if fields[0] == "Forum":
     dataOut.write(line)

  elif fields[0] == "Includes":
     if fields[4] in forumEvents: lines.add(line)

  elif fields[0] == "Author":
     if (fields[4] in forumEvents) or (fields[5] in publications):
        lines.add(line)
        if fields[1] != "": persons.add(fields[1])

  elif fields[0] == "HasOrg":
     if fields[5] in publications:
        lines.add(line)
        if fields[6] != "": topics.add(fields[6])

  elif fields[0] == "HasTopic":
     if (fields[3] in forums) or (fields[4] in forumEvents) or (fields[5] in publications):
        lines.add(line)
        if fields[6] != "": topics.add(fields[6])

data.close()
data = open(sys.argv[1], "r")

########### WRITE PERSON AND TOPIC RECORDS ##########
while True:
  line = data.readline()
  if line == "": break
  if line[0] == '#': continue

  fields = line[:-1].split(',')

# person vertex
  if fields[0] == "Person":
     if fields[1] in persons: dataOut.write(line)

  elif fields[0] == "Topic":
     if fields[6] in topics: dataOut.write(line)

########### WRITE ALL EDGES ##########
for line in lines: dataOut.write(line)
