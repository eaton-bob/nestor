# fty-exdialogue
for test purpose. It make some "chat" between one server and two client.

The default configuration values are in fty-exdialogue.cfg file (section default)

## Protocol 

The following exchanges can occur between the Client and Server, using MAILBOX

Where :

* -> : message from client to server 
* <- : reply of the server to client
* NameC = Client Name
* NameS = Server Name

---
-> "NameC" "Hello !"

<- "NameS" "Hello NameC."

---

-> "NameC" "Who are you ?"

<- "NameS" "My name is NameS"

---

-> "NameC" "Good bye NameS !"

<- "NameS" "Good bye NameC."

---

On other messages, server reply with the message "ERROR"

## How to build
To build fty-exdialogue project run:
```bash
./autogen.sh [clean]
./configure
make
make check # to run self-test
```