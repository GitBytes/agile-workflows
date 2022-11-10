import os
import sys
import random
import datetime

num_records = 0
data = open(sys.argv[1], "r")
dataOut = open(sys.argv[2], "w")

line1 = "#delimieter: ,\n"
line2 = "#columns:type,person1,person2,forum,forum_event,publication,topic,date,lat,lon\n"
line3 = "#types:STRING,UINT,UINT,UINT,UINT,UINT,UINT,USDATE,DOUBLE,DOUBLE\n"

dataOut.write(line1)
dataOut.write(line2)
dataOut.write(line3)

topics = set()
forums = set()
persons = set()
products = set()
organizations = set()

########### GATHER PRODUCTS AND ORGANIZATIONS ##########
while True:
  line = data.readline()
  if line == "": break
  if line[0] == '#': continue

  num_records += 1
  dataOut.write(line)
  fields = line[:-1].split(',')

# product
  if fields[0] == "Sale":
     persons.add(fields[1])
     persons.add(fields[2])
     products.add(fields[6])

  elif fields[0] == "Forum":
     forums.add(fields[3])

  elif fields[0] == "Author":
     persons.add(fields[1])

  elif fields[0] == "HasTopic":
     topics.add(fields[6])

  elif fields[0] == "HasOrg":
     organizations.add(fields[6])

Topics        = list(topics)
Forums        = list(forums)
Persons       = list(persons)
Products      = list(products)
Organizations = list(organizations)

Forums.sort()
Topics.sort()
Forums.sort()
Persons.sort()
Products.sort()
Organizations.sort()

num_days          = 8050 
num_topics        = (len(Topics))
num_forums        = (len(Forums))
num_persons       = (len(Persons))
num_products      = (len(Products))
num_organizations = (len(Organizations))

random.seed(int(sys.argv[4]))
next_label = 99999990000000000
min_date = datetime.datetime.strptime("01/01/2000", "%m/%d/%Y")
num_records = num_records + int( float(sys.argv[3]) ) - 1     # one less because we write out original file

while num_records > 0:
  rnd = random.random()

########## NEW SALE ##########
  if rnd <= 0.40:

     if random.random() < 0.95:
        buyer = Persons[ int(random.random() * num_persons) ]
     else:
        buyer = str(next_label)
        Persons.append(buyer)
        next_label  += 1
        num_persons += 1
        num_records -= 1
        dataOut.write("Person," + buyer + ",,,,,,,,\n")
     
     if random.random() < 0.95:
        seller = Persons[ int(random.random() * num_persons) ]
     else:
        seller = str(next_label)
        Persons.append(seller)
        next_label  += 1
        num_persons += 1
        num_records -= 1
        dataOut.write("Person," + seller + ",,,,,,,,\n")

     product = Products[ int(random.random() * num_products) ]
     date    = min_date + datetime.timedelta( days = int(random.random() * num_days) )

     num_records -= 1
     dataOut.write("Sale,"     + seller + "," + buyer  + ",,,," + product + "," + date.strftime("%m/%d/%Y") + ",,\n")
     # dataOut.write("Purchase," + buyer  + "," + seller + ",,,," + product + "," + date.strftime("%m/%d/%Y") + ",,\n")
     
########## NEW FORUM EVENT ##########
  elif rnd <= 0.97:
     forum_event = str(next_label)
     forum = Forums[ int(random.random() * num_forums) ]
     num_forum_event_topics = random.choice( [1, 1, 1, 2, 2, 3] )
     date  = min_date + datetime.timedelta( days = int(random.random() * num_days) )

     next_label  += 1
     num_records -= 2
     dataOut.write("ForumEvent,,," + forum + "," + forum_event + ",,," + date.strftime("%m/%d/%Y") + ",,\n")
     dataOut.write("Includes,,," + forum + "," + forum_event + ",,,,,\n")

     if random.random() < 0.95:
        author = Persons[ int(random.random() * num_persons) ]
     else:
        author = str(next_label)
        Persons.append(author)
        next_label  += 1
        num_persons += 1
        num_records -= 1
        dataOut.write("Person," + author + ",,,,,,,,\n")

     num_records -= 1
     dataOut.write("Author," + author + ",,," + forum_event + ",,,,,\n")

     for i in range(num_forum_event_topics):
        num_records -= 1
        topic = Topics[ int(random.random() * num_topics) ]
        dataOut.write("HasTopic,,,," + forum_event + ",," + topic + ",,,\n")
     
########## NEW PUBLICATION ##########
  else:
     publication = str(next_label)
     num_publication_authors = random.choice( [1, 2, 3, 4] )
     num_publication_topics = random.choice( [1, 1, 1, 2, 2, 3] )
     num_publication_orgs = int(random.random() * num_publication_authors) + 1
     date = min_date + datetime.timedelta( days = int(random.random() * num_days) )

     next_label  += 1
     num_records -= 1
     dataOut.write("Publication,,,,," + publication + ",," + date.strftime("%m/%d/%Y") + ",,\n")

     for i in range(num_publication_authors):
         if random.random() < 0.95:
            author = Persons[ int(random.random() * num_persons) ]
         else:
            author = str(next_label)
            Persons.append(author)
            next_label  += 1
            num_persons += 1
            num_records -= 1
            dataOut.write("Person," + author + ",,,,,,,,\n")

         num_records -= 1
         dataOut.write("Author," + author + ",,,," + publication + ",,,,\n")

     for i in range(num_publication_topics):
        num_records -= 1
        topic = Topics[ int(random.random() * num_topics) ]
        dataOut.write("HasTopic,,,,," + publication + "," + topic + ",,,\n")

     for i in range(num_publication_orgs):
        num_records -= 1
        org = Organizations[ int(random.random() * num_organizations) ]
        dataOut.write("HasOrg,,,,," + publication + "," + org + ",,,\n")
